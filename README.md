# Subway Runner 2D

A strict C11 Windows pseudo-3D running game built with Win32/GDI. It keeps the
project in C and does not include EasyX because the bundled EasyX for MinGW
header requires C++ compilation.

## Build

Use MinGW-w64 on Windows:

```powershell
mingw32-make
```

Then run:

```powershell
.\running_game.exe
```

## Controls

- `1`: start game from the main menu
- `2`: show game help
- `3`: exit from the main menu
- `A`: move one lane left
- `D`: move one lane right
- `Space`: jump over low barriers
- `S`: slide under high barriers; while jumping, dive down immediately
- `W`: hold to boost speed
- `Esc`: return to the menu
- `R`: restart after game over

## Gameplay

The player runs in three lanes. Obstacles come from the distance and grow larger
as they approach, creating a 2D pseudo-3D subway-runner view.

- Red low barriers should be jumped over.
- Blue high barriers should be avoided by sliding.
- Large train blocks should be avoided by changing lanes.
- Pressing `S` in the air makes the player dive down quickly and slide after
  landing.
- After taking damage, the player is invincible for 3 seconds and flashes.

Passing obstacles and surviving over time increases the score. Hitting an
obstacle decreases HP unless invincibility is active. The game ends when HP
reaches zero.

## Optional Assets

The game works without external assets. If these files exist, they are loaded
from `assets/`; otherwise the game draws fallback placeholders:

- `assets/player_run_0.bmp`
- `assets/player_run_1.bmp`
- `assets/player_jump.bmp`
- `assets/player_crouch.bmp`
- `assets/obstacle_ground.bmp`
- `assets/obstacle_air.bmp`
- `assets/hit.wav`

BMP files are recommended because they are loaded directly through Win32/GDI.
