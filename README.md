# BG3 WASD Movement

A mod for Baldur's Gate 3 that allows direct character movement with WASD keys (or others).

Find it on [Nexusmods](https://www.nexusmods.com/baldursgate3/mods/781).

# Camera Follow Fork

This is my personal fork of BG3WASD, adding a smoother third-person camera-follow setup and an optional mouse-steering camera mode.

It keeps the original BG3WASD movement system, but adds extra camera behaviour for people who want WASD movement to feel a bit more like a third-person game.

Fork-specific changes include:

- Standard Camera Follow Mode, toggled with F7
- Mouse Steering Follow Mode, toggled with F6
- In-game ON/OFF notifications for F6 and F7
- Character-facing yaw bridge through Script Extender
- Adjustable camera offsets and smoothing
- Reduced close-range camera snapping
- Manual camera override support
- Camera modes disabled during combat
- TOML configurable camera settings
- Various camera and movement tuning changes

Camera Modes:

Standard Camera Follow Mode - follows your active character’s facing direction for smoother third-person WASD movement.

Mouse Steering Follow Mode - is layered on top of Standard Camera Follow Mode. When both F7 and F6 are enabled, holding W and moving the mouse left/right steers the camera direction, giving a tighter behind-character movement feel.

Known Notes:

- Mouse Steering Follow Mode currently focuses on left/right steering. Vertical camera angle is not actively controlled, so set your preferred camera angle, height, and zoom before moving.

- Real middle mouse input overrides Mouse Steering Follow Mode properly.

- Standard Camera Follow Mode intentionally limits straight W-only/S-only follow correction to prevent camera-relative circular turning.

- Diagonal movement such as W+A, W+D, S+A, and S+D generally behaves more naturally.
Leaving F6 and F7 off gives a more original BG3WASD-style movement setup.

This project is based on the original BG3WASD by Ch4nKyy.

Original project:
https://github.com/Ch4nKyy/BG3WASD

Demo:
https://www.youtube.com/watch?v=HPqF5g48S04

Still a work in progress, but now at the point where it feels like its own proper camera-control fork rather than just a tiny test tweak. It's a little bit scuffed, but it works.

Script Extender Yaw Bridge:

This fork uses a small companion .pak mod called BG3YawBridge.

The .pak runs a Script Extender Lua script that reads the active character’s facing direction and writes it out for the modified BG3WASD DLL. The DLL then uses that yaw data for the camera-follow behaviour.

It also handles the in-game toggle notifications for the fork-specific camera modes.

Both parts are needed:

- BG3YawBridge.pak gets the character yaw and handles toggle notifications.
- BG3WASD.dll handles the actual camera-follow and mouse-steering behaviour.

The BG3YawBridge folder in this repository contains the source files used to build BG3YawBridge.pak. The packaged .pak is included in release downloads.

## Building

### Requirements

- [CMake](https://cmake.org/)
  - Add this to your `PATH`
- [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
- [Vcpkg](https://github.com/microsoft/vcpkg)
  - Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing
  vcpkg
- [Visual Studio Community 2022](https://visualstudio.microsoft.com/)
  - Desktop development with C++
- [Baldur's Gate 3 Steam Distribution](https://store.steampowered.com/app/1086940/Baldurs_Gate_3/)
  - Add the environment variable `BG3PATH` with the value as path to game install folder
  - Add `BG3PATH2` if you have a secondary installation (E.g. one for Steam, one for GOG)
- [7zip](https://www.7-zip.org/)
  - Install to default dir
  
### Register Visual Studio as a Generator

- Open `x64 Native Tools Command Prompt`
- Run `cmake`
- Close the cmd window

### Building

```
git clone https://github.com/Ch4nKyy/BG3WASD.git
cd BG3WASD
git submodule init
git submodule update --remote
.\build-release.ps1
```

### Solution Generation (Optional)
If you want to generate a Visual Studio solution, run the following command:
```
.\generate-sln.ps1
```

> ***Note:*** *This will generate a `BG3WASD.sln` file in the **build** directory.*

### VSCode Intellisense (Optional)

To fix Intellisense in VSCode, do the following:

- Install the extensions ```ms-vscode.cpptools``` and ```ms-vscode.cmake-tools```.
- Build the solution with the cmake tools extension.
- In your ```c_cpp_properties.json```, use ```"configurationProvider": "ms-vscode.cmake-tools"```.
This is the only needed parameter, apart from "name".
