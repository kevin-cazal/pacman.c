# pacman.c 

A Pacman clone written in C99 with minimal dependencies for Windows, macOS, Linux and WASM.

[WASM version (GitHub Pages)](https://kevin-cazal.github.io/pacman.c/)

For implementation details see comments in the pacman.c source file (I've tried
to structure the source code so that it can be read from top to bottom).

Related projects:

- Zig version: https://github.com/floooh/pacman.zig

## Clone, Build and Run (Linux, macOS, Windows)

On the command line:

```
git clone https://github.com/floooh/pacman.c
cd pacman.c
mkdir build
cd build
cmake ..
cmake --build .
```

> NOTE: on Linux you'll need to install the OpenGL, X11 and ALSA development packages (e.g. mesa-common-dev, libx11-dev and libasound2-dev).

On Mac and Linux this will create an executable called 'pacman'
in the build directory:

```
./pacman
```

with nix:
```bash
nix run github:floooh/pacman.c
```

On Windows, the executable is in a subdirectory:

```
Debug/pacman.exe
```

## Lua mods (branch `feature/lua-mods`)

Pacman and ghost AI can be customized with Lua mod files. See [mods/README.md](mods/README.md) for the hook API and examples.

```bash
cmake --build build
./build/pacman --mod mods/aggressive_blinky.lua
```

The WASM build embeds `mods/default.lua` (vanilla behavior). Rebuild after editing that file.

Quick build (native + WASM): `./build.sh` (uses local cmake/emcc when available, otherwise Docker). The WASM mod lab uses a self-hosted Monaco editor for Lua (`./scripts/vendor-monaco.sh` runs automatically on first `./build.sh wasm`). Pushes to `main` deploy the WASM build to GitHub Pages (`index.html` at the site root).

Docker images: see [docker/README.md](docker/README.md) (`docker/Dockerfile.native`, `docker/Dockerfile.wasm`).

## Build and Run WASM/HTML version via Emscripten

> NOTE: You'll run into various problems running the Emscripten SDK tools on Windows, might be better to run this stuff in WSL.

Setup the emscripten SDK as described here:

https://emscripten.org/docs/getting_started/downloads.html#installation-instructions

Don't forget to run ```source ./emsdk_env.sh``` after activating the SDK.

And then in the pacman.c directory:

```
mkdir build
cd build
emcmake cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel ..
cmake --build .
```

To run the compilation result in the system web browser:

```
> emrun index.html
```

## IDE Support

On Windows with Visual Studio cmake will automatically create a **Visual Studio** solution file which can be opened with ```cmake --open .```:
```
cd build
cmake ..
cmake --open .
```

On macOS, the cmake **Xcode** generator can be used to create an
Xcode project which can be opened with ```cmake --open .```
```
cd build
cmake -GXcode ..
cmake --open .
```

On all platforms with **Visual Studio Code** and the Microsoft C/C++ and
CMake Tools extensions, simply open VSCode in the root directory of the
project. The CMake Tools extension will detect the CMakeLists.txt file and
take over from there:
```
cd pacman.c
code .
```
