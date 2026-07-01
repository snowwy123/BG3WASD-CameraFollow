# BG3 WASD Camera Follow Fork

A Baldur's Gate 3 mod based on [BG3WASD by Ch4nKyy](https://github.com/Ch4nKyy/BG3WASD).

This fork keeps the original WASD movement idea, then adds smoother third-person camera behaviour for players who want movement to feel closer to a modern over-the-shoulder RPG.

Version: **1.2.0**

## What this fork adds

- **Camera Follow** mode, toggled with **F7**
- **Mouse Steering** mode, toggled with **F6**
- Optional vertical mouse steering that behaves like holding middle mouse while moving
- In-game ON/OFF notifications for the F6 and F7 camera modes
- Script Extender yaw bridge for reading the active character's facing direction
- Smooth camera-follow behaviour with configurable responsiveness and turn speed
- Over-the-shoulder offset tuning for W+A / W+D movement
- Manual camera override support, so Q/E and middle mouse can take control cleanly
- Caps/freecam movement-mode override support
- Combat pause support for the fork's camera modes
- TOML config cleanup with old setting-name fallbacks

## Camera modes

### F7: Camera Follow

Camera Follow turns the camera toward the active character's facing direction while moving.

It is designed to make WASD movement feel more natural without constantly snapping the camera back to the character.

### F6: Mouse Steering

Mouse Steering is separate from Camera Follow. It does not require F7 to be on.

By default, Mouse Steering uses BG3's own camera-rotate input while you move. This makes it feel similar to holding middle mouse while moving, including vertical camera movement.

The old yaw-only steering style is still available in the TOML:

```toml
[MouseSteering]
AllowVerticalMouseSteering = false
```

## Default hotkeys

```text
F7 = Toggle Camera Follow
F6 = Toggle Mouse Steering
```

## Important notes

- The fork-specific camera modes are intended for exploration movement.
- Camera Follow and Mouse Steering pause during combat when `PauseInCombat = true`.
- Manual camera input with Q/E or middle mouse temporarily takes priority over Camera Follow.
- Standing still after manual camera movement keeps the camera where you placed it.
- Starting movement again resumes character-facing Camera Follow.
- Caps/freecam movement mode pauses Camera Follow so the systems do not fight each other.
- Leaving both F6 and F7 off gives a more original BG3WASD-style setup.

## Script Extender yaw bridge

This fork uses a small companion `.pak` mod called **BG3YawBridge**.

The `.pak` runs a Script Extender Lua script that reads the active character's facing direction and writes it to:

```text
%LOCALAPPDATA%\Larian Studios\Baldur's Gate 3\Script Extender\YawBridge.txt
```

The modified `BG3WASD.dll` reads that file and uses the yaw value for Camera Follow.

Both parts are needed:

- `BG3YawBridge.pak` reads the active character yaw and handles toggle notifications.
- `BG3WASD.dll` handles movement, Camera Follow, and Mouse Steering.

## Version 1.2.0 highlights

- Cleaned up Camera Follow config names while keeping fallbacks for older TOML settings.
- Added independent F6 Mouse Steering toggle behaviour.
- Added Caps/freecam movement-mode override support.
- Improved manual camera override behaviour while moving and standing still.
- Added vertical Mouse Steering mode, enabled by default.
- Added settle polish to reduce tiny final camera offsets and one-frame bounce corrections.
- Slightly slowed the shoulder transition so W+A / W+D changes feel smoother.

## Building

### Requirements

- [CMake](https://cmake.org/)
  - Add this to your `PATH`
- [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
- [Vcpkg](https://github.com/microsoft/vcpkg)
  - Set the `VCPKG_ROOT` environment variable to your vcpkg folder
- [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
  - Install **Desktop development with C++**
- [Baldur's Gate 3 Steam Distribution](https://store.steampowered.com/app/1086940/Baldurs_Gate_3/)
  - Set `BG3PATH` to your BG3 install folder
  - Add `BG3PATH2` if you have a second install, for example Steam and GOG
- [7-Zip](https://www.7-zip.org/)
  - Install to the default directory

### Recommended local build

This fork is currently built with Visual Studio 2022 and the static-md vcpkg triplet:

```powershell
cmake -S . -B build `
  -G "Visual Studio 17 2022" `
  -A x64 `
  -DVCPKG_TARGET_TRIPLET=x64-windows-static-md

cmake --build build --config Release
```

After that, normal source edits only need:

```powershell
cmake --build build --config Release
```

### Original helper scripts

The original helper scripts are still included:

```powershell
.\build-release.ps1
```

Optional Visual Studio solution generation:

```powershell
.\generate-sln.ps1
```

## Credits

This project is based on the original **BG3WASD** by Ch4nKyy.

Original project:
https://github.com/Ch4nKyy/BG3WASD

Original Nexus page:
https://www.nexusmods.com/baldursgate3/mods/781
