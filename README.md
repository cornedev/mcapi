<p align="center"><img width="256px" src="gfx/ccapi.png"></p>

# ccapi
An API for downloading minecraft assets & launcher.

This API is in WIP state.

The goal of this project is to create an easy-use API for downloading all sorts of assets from mojangs servers, and to create a lightweight launcher that can be used anywhere.

## Features
- API
    - Version json downloading.
    - Client jar downloading.
    - Assets downloading.
    - Server jar downloading.
    - Libraries downloading.
    - Natives extracting.
    - Starting a offline Minecraft process.
    - Jdk downloading.
    - Full cross-platform support (windows, macos and linux)
## Planned
- Fabric support.
- Server process starting.
- A gui application using this API.
- Official Microsoft account authentication with mojang servers.

## Building
## cli
### Windows
1. Download the latest version of [MSYS2](https://www.msys2.org/). After installation open the MINGW64 shell.
2. Install the following dependencies:
    ```sh
    pacman -S mingw-w64-x86_64-libarchive
    pacman -S mingw-w64-x86_64-nlohmann-json
    ```
3. Clone this repository:
   ```sh
   git clone https://github.com/cornedev/ccapi.git
   cd ccapi/cli
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

2. Clone this repository:
   ```sh
   git clone https://github.com/cornedev/ccapi.git
   cd ccapi/cli
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
   ./build/ccapi_cli
   ```

<br>

If you ever want to clean or rebuild (for both operating systems):
   ```sh
   rm -rf build
   ```
   And run the build commands again.
## gui
WIP.
### License
All the code is licensed under the [MIT license](LICENSE).