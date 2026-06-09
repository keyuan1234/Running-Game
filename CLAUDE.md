# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```powershell
mingw32-make          # build with MinGW-w64 (gcc -std=c11 -Wall -Wextra -O2)
mingw32-make clean    # remove .o files and .exe
mingw32-make run      # build and launch
.\running_game.exe    # run directly after building
```

The project is **strict C11** — no C++ features. The bundled EasyX headers are C++ only and intentionally unused; everything uses raw Win32/GDI.

## Architecture Overview

### Game Loop (`src/main.c`)

The game is driven by `WinMain` → simple `while (g_running)` loop. Each iteration:

1. Pump Windows messages via `PeekMessageA` + `DispatchMessageA`
2. Compute `dt` from `GetTickCount` (capped at **50ms** to prevent physics tunneling)
3. `game_update()` — physics, spawning, collision
4. `render_frame()` — double-buffered: draw to a memory DC, then `BitBlt` to the window
5. Frame rate limiter via `Sleep(FRAME_TIME_MS - frame_time)` targeting 60 FPS

### State Machine

Four states in `GameState` enum (`src/types.h`):

```
STATE_MENU → STATE_PLAYING (1 key) or STATE_HELP (2 key)
STATE_HELP → STATE_MENU (Esc)
STATE_PLAYING → STATE_GAME_OVER (hp <= 0) or STATE_MENU (Esc)
STATE_GAME_OVER → STATE_PLAYING (R key) or STATE_MENU (Esc)
```

`game_reset()` reinitializes everything while preserving loaded assets. In menu/help/gameover states, `game_update()` only advances the scrolling background (at 45% of start speed) — obstacles and player physics are paused.

### Input Handling

Key state is stored in `InputState` (mutable per-frame requests for lane/jump/crouch; persistent `boost_down` flag for W key). **Lane change requests are consumed** (set to 0 after processing) each frame, preventing repeated lane shifts from a single keypress. Boost is continuous — it toggles on `WM_KEYDOWN`/`WM_KEYUP` for W.

`WM_KEYDOWN` uses the `lparam & 0x40000000` bit to detect first-press vs. auto-repeat for movement keys.

### Perspective Projection (The Shared "Lens")

Both `obstacle.c` and `background.c` use this perspective function:

```
t = clamp(1.0 - depth, 0, 1)
perspective = t² * (1.18 - 0.18*t)
```

This transforms a normalized depth value (1.0 = far at horizon, 0.0 = near at camera) into a screen-Y and scale factor. The formula is **duplicated** in both files — changing it requires updating both.

### Obstacle System (`src/obstacle.c`)

- **Spawn**: timer-driven with cooldown gated by `can_spawn_group()` which enforces `MIN_GROUP_DEPTH_GAP` between active obstacle clusters
- **Streak-breaking**: `random_kind()` prevents more than 2 consecutive obstacles of the same type by forcing a different kind on the 3rd consecutive roll
- **Depth → screen**: `obstacle_project()` computes screen position and `screen_rect` from lane + depth. Near obstacles are larger, far obstacles are smaller and closer to `HORIZON_Y`
- **Scoring**: obstacles auto-score (15 or 25 points) when depth passes `OBSTACLE_REMOVE_DEPTH * 0.35` — the player doesn't need to jump/dodge to earn base points
- **Deactivation**: obstacles with depth < `OBSTACLE_REMOVE_DEPTH` (-0.18) are deactivated
- **Rendering**: depth-sorted painter's algorithm — farther obstacles draw first
- **Hitboxes** are tighter than visual rects, defined per-kind in `obstacle_get_rect()`
- Hit detection checks lane match, depth range (`HIT_DEPTH_MIN` to `HIT_DEPTH_MAX`), and rect overlap

### Player System (`src/player.c`)

- **Lane movement**: instant lane change with smooth `screen_x` interpolation via `LANE_MOVE_SPEED`
- **Jump**: sets `vy = PLAYER_JUMP_SPEED`, gravity pulls back down. Landing resets to ground. S in air → `PLAYER_AIR_DIVE` with `PLAYER_DIVE_SPEED` (-980), which transitions to slide on landing
- **Slide**: limited by `SLIDE_DURATION` (0.45s). While sliding, hitbox dimensions change to `PLAYER_SLIDE_WIDTH`/`PLAYER_SLIDE_HEIGHT`
- **Invincibility**: 3s after taking a hit; player flashes (drawn every other fast tick)
- **Animation**: toggles between `player_run[0]`/`[1]` every ~110ms
- **Fallback rendering**: if sprite isn't loaded, `draw_player_placeholder()` draws with GDI primitives

### Background (`src/background.c`)

Four-layer parallax, drawn back-to-front:
1. Sky gradient (per-scanline pen color)
2. Building silhouettes (two mirrored rows, scrolling at `building_offset`)
3. Runway trapezoid + converging perspective lines + sleepers + center dashes
4. Side props (poles, lamps, signs) on both edges

Each layer scrolls at a different fraction of current game speed.

### Config (`src/config.h`)

Every tunable constant: window dimensions, physics values, lane spacing, obstacle limits, spawn intervals, hitbox debug flag (`DEBUG_HITBOX`), speed curve. No magic numbers in source — all values reference named defines.

### Assets (`src/resources.c`)

Optional BMP sprites loaded from `assets/` via `LoadImageA`. `hit.wav` detected by file existence check only. Game fully functional without any assets — every drawable falls back to GDI primitive rendering.

### Sound (`src/sound.c`)

Hit sound plays via `PlaySoundA` (async) if `assets/hit.wav` exists; falls back to `MessageBeep`.

### Module Dependency Graph

```
main.c
 ├── types.h (no deps beyond config.h + windows.h)
 ├── config.h
 ├── player.h → types.h
 ├── obstacle.h → types.h
 ├── background.h → types.h
 ├── resources.h → types.h
 ├── sound.h → types.h
 └── util.h → windows.h (no types.h)

obstacle.c → resources.h, util.h
player.c → resources.h
background.c → (standalone)
```

`types.h` is the central header — it defines all structs and enums, includes `config.h` and `windows.h`. Every other module depends on it. Modules don't depend on each other's headers (e.g., `obstacle.h` doesn't include `player.h`).

## No Test Suite

The project has no automated tests. Validation is manual: build, run, and play through the game states. The `DEBUG_HITBOX` define in `config.h` (set to 0 by default) can be flipped to 1 to render obstacle hitboxes as yellow outlines.
