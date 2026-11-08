# Tanzanite — Geometry Dash mod menu

A [Geode](https://geode-sdk.org) mod written in C++ (Windows, GD 2.2081, Geode 5.11.2).

**Press the key above Tab (`²` on AZERTY, `` ` `` on QWERTY) to open or close the menu**, anywhere in the game. `Escape` also closes it.

## Categories
| Category | Options |
|---|---|
| Player | Noclip, Speedhack (speed 0.05 to 10, 0.5x / 1x / 1.5x / 2x presets) |
| Level | Safe Mode, Auto Safe Mode (on by default), Accurate Percentage |
| Labels | Cheat Indicator, FPS, CPS, Noclip Accuracy, Noclip Deaths, Attempts, Jumps, Level Time, Clock, Custom Message — adjustable corner, size and opacity |
| Bot | Record / Play macros, saved macros list, Position Fix, Frame Stepper |
| Info | Shortcuts, version, repository link |

Safe Mode makes the level count as a test run: no progress is saved.
Auto Safe Mode turns it on for any attempt where noclip or the speedhack was used, even if you turn the cheat off before the end of the attempt.

## Macro bot
1. Open the menu, go to **Bot** and pick **Record**.
2. Play the level until you beat it. Practice mode works: when you respawn at a checkpoint, everything recorded after it is removed automatically.
3. When you beat the level, the macro is saved by itself (you can also name it and press **Save**).
4. Pick **Play** and open the level: its macro is loaded automatically and plays from the start of every attempt.

Why it does not break:
- Inputs are stored by physics tick (240 per second of level time), not by real time, so the speedhack and your FPS have no effect.
- With **Position Fix** on, every input also stores the exact player state (position, rotation, velocities), restored right before the input on playback, so small differences can never add up.
- At practice checkpoints a full snapshot of both players is stored and restored on respawn.
- While a macro plays, your own clicks are ignored.

Macros are saved as `.tzm` text files in the mod save folder (`geode/mods/saltoback.tanzanite/macros`).

## Frame stepper
Turn on **Frame Stepper** in the Bot tab. In a level the game freezes; press **F** to advance one physics tick, or hold **F** to keep stepping. Works while recording, so you can place inputs on exact frames.

The bot and the frame stepper count as cheats: with Auto Safe Mode, no progress is saved.

## Building
Every push to GitHub runs the **Build Tanzanite** workflow (Actions tab), which produces `saltoback.tanzanite.geode`.
Put that file in `Geometry Dash/geode/mods/` (the **Node IDs** mod is also required; Geode offers to install it).

Locally: Visual Studio 2022 (C++), CMake, Geode CLI + SDK (`geode sdk install`), then `geode build`.

Note: the speedhack does not change the music speed.

## License
Open source under the MIT license (see `LICENSE`).
