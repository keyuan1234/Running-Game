# Subway Runner 2D

A strict C11 Windows pseudo-3D runner game built with Win32/GDI. The project
does not use EasyX because the bundled EasyX for MinGW headers require C++.

## Build And Run

```powershell
mingw32-make
.\running_game.exe
```

Useful commands:

```powershell
mingw32-make clean
mingw32-make run
```

## Controls

- `1`: start game / character select
- `2`: help
- `3`: exit
- `4`: recent runs
- `A`: move one lane left
- `D`: move one lane right
- `Space`: jump over red low barriers
- `S`: slide under blue high barriers; in air, dive down quickly
- `W`: hold to boost speed
- `Esc`: return
- `R`: restart after game over
- `L`: recent runs after game over

## Gameplay

Run in three lanes while obstacles approach from the distance.

- Red low barriers should be jumped over.
- Blue high barriers should be avoided by sliding.
- Train blocks should be avoided by changing lanes.
- Taking damage gives temporary invincibility.
- Passing obstacles and surviving longer increases the score.
- Saved results keep only the latest 10 runs.

## Characters

Choose a character before each run.

- Runner: balanced.
- Sprinter: faster but fragile.
- Tank: more HP and low-barrier immunity.
- Jumper: double jump.

Tank has a payment popup gate. Keep `Pay.png` in the project root. The first
time Tank is confirmed in a program session, the game shows `Pay.png` instead
of starting. Close it with `Esc`, `Enter`, `Space`, or the `X` button, then
confirm Tank again to play.

## Optional Assets

The game works without external BMP assets because each model has a GDI
placeholder. Obstacle images are drawn inside the obstacle shape; they no longer
replace the whole obstacle outline. This keeps red low barriers, blue high
barriers, and train blocks visible even when custom images are very large.

Optional files:

- `assets/player_run_0.bmp`
- `assets/player_run_1.bmp`
- `assets/player_jump.bmp`
- `assets/player_crouch.bmp`
- `assets/obstacle_ground.bmp`
- `assets/obstacle_air.bmp`
- `assets/obstacle_train.bmp`
- `assets/hit.wav`

`Pay.png` is loaded separately from the project root using GDI+.

Suggested obstacle image sizes:

- `obstacle_ground.bmp`: about `100x60`
- `obstacle_air.bmp`: about `120x70`
- `obstacle_train.bmp`: about `140x180`
