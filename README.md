# ![ccapi_ico](cli/gfx/clilauncher.png)
# ccapi
An API for downloading minecraft assets.

This API is in WIP state.

## Features
- Simple API
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

# Building

### Windows
1. Download the latest version of [MSYS2](https://www.msys2.org/). After installation open the MINGW64 shell.
2. Install the following dependency:
    ```sh
    pacman -S mingw-w64-x86_64-libzip
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

If you ever want to clean or rebuild:
   ```sh
   rm -rf build
   ```
   And run the build commands again.
### Linux
In the works...

### License
All the code is licensed under the [MIT license](LICENSE).
