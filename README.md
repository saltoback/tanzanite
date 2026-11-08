# Tanzanite — Geometry Dash mod menu

A [Geode](https://geode-sdk.org) mod written in C++ (Windows, GD 2.2081, Geode 5.11.2).

**Press the key above Tab (`²` on AZERTY, `` ` `` on QWERTY) to open or close the menu**, anywhere in the game. `Escape` also closes it.

## Categories
| Category | Options |
|---|---|
| Player | Noclip, Speedhack (speed 0.05 to 10, 0.5x / 1x / 1.5x / 2x presets) |
| Level | Safe Mode, Auto Safe Mode (on by default), Accurate Percentage |
| Labels | Cheat Indicator, FPS, CPS, Noclip Accuracy, Noclip Deaths, Attempts, Jumps, Level Time, Clock, Custom Message — adjustable corner, size and opacity |
| Info | Shortcuts, version, repository link |

Safe Mode makes the level count as a test run: no progress is saved.
Auto Safe Mode turns it on for any attempt where noclip or the speedhack was used, even if you turn the cheat off before the end of the attempt.

## Building
Every push to GitHub runs the **Build Tanzanite** workflow (Actions tab), which produces `saltoback.tanzanite.geode`.
Put that file in `Geometry Dash/geode/mods/` (the **Node IDs** mod is also required; Geode offers to install it).

Locally: Visual Studio 2022 (C++), CMake, Geode CLI + SDK (`geode sdk install`), then `geode build`.

Note: the speedhack does not change the music speed.

## License
Open source under the MIT license (see `LICENSE`).
