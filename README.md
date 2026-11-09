# Tanzanite — Geometry Dash mod menu

A [Geode](https://geode-sdk.org) mod written in C++ (Windows and Android, GD 2.2081, Geode 5.11.2).

**Press the key above Tab (`²` on AZERTY, `` ` `` on QWERTY) to open or close the menu**, anywhere in the game. `Escape` also closes it.
On **Android**, tap the floating gem button instead (drag it to move it anywhere on the screen).

## Categories
| Category | Options |
|---|---|
| Player | Noclip, Speedhack (speed 0.05 to 10, presets), Show Hitboxes, Show Trajectory |
| Level | Safe Mode, Auto Safe Mode (on by default), Accurate Percentage |
| Labels | Cheat Indicator, FPS, CPS, Noclip Accuracy, Noclip Deaths, Attempts, Jumps, Level Time, Clock, Custom Message — adjustable corner, size and opacity |
| Bot | Enter / Exit Macro Mode, saved macros list, Position Fix, Show Trajectory |
| Info | Shortcuts, version, repository link |

Safe Mode makes the level count as a test run: no progress is saved.
Auto Safe Mode turns it on for any attempt where noclip or the speedhack was used, even if you turn the cheat off before the end of the attempt.

## Macro mode
1. Open the menu, go to **Bot** and press **Enter Macro Mode**.
2. Open a level: a macro interface appears at the bottom of the screen and practice mode turns on by itself.
3. Everything is on that bar, no need to go back to the menu:
   - **Record** / **Play**: switching restarts the level so the macro starts at the beginning
   - **Pause** / **Resume** and **+1 Frame** (or the **F** key) to go frame by frame
   - **Restart**, **Save**, **Exit**
   - **Speed** `-` / `+` (click the value to turn the speedhack on or off)
   - **Hitboxes**, **Trajectory**
   - **+ CP** / **- CP** to place or remove practice checkpoints
4. Beat the level while recording: the macro is saved by itself. Press **Play** to watch it.

Clicks on the bar never make the player jump.

Why it does not break:
- Inputs are stored by physics tick (240 per second of level time), not by real time, so the speedhack and your FPS have no effect.
- With **Position Fix** on, every input also stores the exact player state (position, rotation, velocities), restored right before the input on playback, so small differences can never add up.
- At practice checkpoints a full snapshot of both players is stored and restored on respawn.
- While a macro plays, your own clicks are ignored.

Macros are saved as `.tzm` text files in the mod save folder (`geode/mods/saltoback.tanzanite/macros`).

## Show Trajectory
Shows where the player goes in the next 0.75 s if you hold the button (green) or release it (red), with a cross where it would die. Available in the Player tab and on the macro bar. For now the trajectory ignores portals, orbs and pads (it never activates anything in the level).

The bot and the frame stepper count as cheats: with Auto Safe Mode, no progress is saved.

## Building
Every push to GitHub runs the **Build Tanzanite** workflow (Actions tab), which produces `saltoback.tanzanite.geode`.
Put that file in `Geometry Dash/geode/mods/` (the **Node IDs** mod is also required; Geode offers to install it).

Locally: Visual Studio 2022 (C++), CMake, Geode CLI + SDK (`geode sdk install`), then `geode build`.

Note: the speedhack does not change the music speed.

## License
Open source under the MIT license (see `LICENSE`).
