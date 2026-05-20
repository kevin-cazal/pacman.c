# Docker builds

For a one-shot native + WASM build, use [`../build.sh`](../build.sh) from the repo root.

These Dockerfiles match the ad-hoc builds used during development (`gcc` + `emscripten/emsdk` images).

## Native Linux (`Dockerfile.native`)

Build and tag:

```bash
docker build -f docker/Dockerfile.native -t pacman-native .
```

Run the game (needs X11 display on the host):

```bash
xhost +local:docker   # once per session, if needed
docker run --rm -it \
  -e DISPLAY="$DISPLAY" \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  pacman-native --mod mods/aggressive_blinky.lua
```

Build only (artifact on host via volume):

```bash
docker run --rm -v "$(pwd):/src" -w /src/build \
  debian:bookworm-slim bash -lc '
    apt-get update -qq && apt-get install -y -qq cmake gcc pkg-config \
      libgl-dev libx11-dev libxi-dev libxcursor-dev libxrandr-dev libasound2-dev &&
    cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build . -j$(nproc)
  '
```

## WASM (`Dockerfile.wasm`)

Vendor Monaco once on the host (required before WASM build; `build.sh wasm` does this automatically):

```bash
./scripts/vendor-monaco.sh
```

Build and serve on port 8080:

```bash
docker build -f docker/Dockerfile.wasm -t pacman-wasm .
docker run --rm -p 8080:8080 pacman-wasm
```

Open [http://localhost:8080/pacman.html](http://localhost:8080/pacman.html).

Build only (output in `build-wasm/` on the host):

```bash
mkdir -p build-wasm
docker run --rm -v "$(pwd):/src" -w /src/build-wasm emscripten/emsdk:latest \
  bash -lc 'emcmake cmake -G"Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel .. && cmake --build . -j$(nproc)'
```

## Notes

- Native image installs **build** and **runtime** X11/OpenGL/ALSA packages (same set as README Linux deps).
- WASM image embeds `mods/default.lua` at configure time; edit that file and rebuild to change web behavior.
- `.dockerignore` at the repo root keeps `build/` and `build-wasm/` out of the build context.
