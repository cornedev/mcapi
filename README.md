<p align="center"><img width="256px" src="gfx/mcapi.png"></p>

# mcapi
An API for downloading minecraft assets & launcher.

The goal of this project is to create an easy-use API for downloading all sorts of assets from mojangs servers, and to create a lightweight launcher that can be used anywhere.

Download the latest builds at the [builds branch](https://github.com/cornedev/mcapi/tree/builds).

Note: the builds are broken at the moment. Github actions still needs fixing. Please download the latest release from the releases tab.

## Features
- API
    - Full [vanilla](https://github.com/cornedev/mcapi/blob/main/api/mcapi_vanilla.cpp) downloading support (all assets & launching needs).
    - Full [fabric](https://github.com/cornedev/mcapi/blob/main/api/mcapi_fabric.cpp) downloading support.
    - [Minecraft starting](https://github.com/cornedev/mcapi/blob/main/api/mcapi_process.cpp) support.
    - [Java downloading](https://github.com/cornedev/mcapi/blob/main/api/mcapi_java.cpp) support using the adoptium API.
    - Official [Microsoft account authentication](https://github.com/cornedev/mcapi/blob/main/api/mcapi_auth.cpp) with mojang servers.
    - Full cross-platform support (windows, macos and linux).
    - A [gui implementation showcase launcher](https://github.com/cornedev/mcapi/tree/main/gui) using this API (showcase image at the bottom of this readme).
## Planned
- Server process starting.
- Skin support.

## Building
## cli
Note: the cli application is not being updated anymore, it's archived and located [here.](api/archived/cli) if you still would like to build it, make sure to move the cli folder to the repository root first. 

I can't guarantee the cli application will still work in future API changes.
### Windows
1. Download the latest version of [MSYS2](https://www.msys2.org/). After installation open the MINGW64 shell.
2. Install the following dependencies:
    ```sh
    pacman -S mingw-w64-x86_64-libarchive
    pacman -S mingw-w64-x86_64-nlohmann-json
    pacman -S mingw-w64-x86_64-cmake
    ```
3. Clone this repository:
   ```sh
   git clone --recurse-submodules https://github.com/cornedev/mcapi.git
   cd mcapi/cli
   ```
4. Build the application:
   ```sh
   cmake -S . -B build
   ```
   and then:
   ```sh
   cmake --build build
   ```
That's it. The executable will be in `/build/` and all dependencies will be automatically copied from `/runtime/bin/`

### Linux
1. Get the following dependencies on your system:
   - [nlohmann/json](https://github.com/nlohmann/json)
   - [libarchive](https://github.com/libarchive/libarchive)
   - [cmake](https://github.com/Kitware/CMake)

2. Clone this repository:
   ```sh
   git clone --recurse-submodules https://github.com/cornedev/mcapi.git
   cd mcapi/cli
   ```
3. Build the application:
   ```sh
   cmake -S . -B build
   ```
   and then:
   ```sh
   cmake --build build
   ```
That's it. The executable will be in `/build/` and you can run it with:
   ```sh
   ./build/mcapi_cli
   ```

## gui
### Windows
1. Download the latest version of [MSYS2](https://www.msys2.org/). After installation open the MINGW64 shell.
2. Install the following dependencies:
    ```sh
    pacman -S mingw-w64-x86_64-libarchive
    pacman -S mingw-w64-x86_64-nlohmann-json
    pacman -S mingw-w64-x86_64-qt6
    pacman -S mingw-w64-x86_64-cmake
    ```
3. Clone this repository:
   ```sh
   git clone --recurse-submodules https://github.com/cornedev/mcapi.git
   cd mcapi/gui
   ```
4. Build the application:
   ```sh
   cmake -S . -B build
   ```
   and then:
   ```sh
   cmake --build build
   ```
That's it. The executable will be in `/build/` and all dependencies will be automatically copied from `/runtime/bin/`

### Linux
1. Get the following dependencies on your system:
   - [nlohmann/json](https://github.com/nlohmann/json)
   - [libarchive](https://github.com/libarchive/libarchive)
   - [Qt](https://github.com/qt)
   - [curl](https://github.com/curl/curl)
   - [cmake](https://github.com/Kitware/CMake)

2. Clone this repository:
   ```sh
   git clone --recurse-submodules https://github.com/cornedev/mcapi.git
   cd mcapi/gui
   ```
3. Build the application:
   ```sh
   cmake -S . -B build
   ```
   and then:
   ```sh
   cmake --build build
   ```
That's it. The executable will be in `/build/` and you can run it with:
   ```sh
   ./build/mcapi_gui
   ```

<br>

If you ever want to clean or rebuild (for both applications):
   ```sh
   rm -rf build
   ```
   And run the build commands again.

## Showcase

### gui launcher
<img src="gfx/mcapi_gui_showcase.png" alt="mcapi_gui" width="450">

## License
All the code is licensed under the [MIT license](LICENSE).

Copyright (c) 2025-2026 cornedev

## Disclaimer
Note that this project is __unofficial__ and not affiliated with Microsoft or Mojang. All assets are fetched directly from official Mojang servers, this API does not redistribute the assets. Please follow the [Minecraft EULA](https://www.minecraft.net/en-us/eula) and [Guidelines](https://www.minecraft.net/en-us/usage-guidelines)
