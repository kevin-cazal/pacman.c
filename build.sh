#!/usr/bin/env bash
# Build native Linux binary and WASM/HTML bundle.
# Usage:
#   ./build.sh              # both targets
#   ./build.sh native       # build/pacman only
#   ./build.sh wasm         # build-wasm/pacman.html only
#   ./build.sh --docker     # force Docker (no local cmake/emcc required)
#   ./build.sh --local      # require local cmake / emscripten on PATH
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"

BUILD_NATIVE=1
BUILD_WASM=1
MODE="auto"

usage() {
    sed -n '2,8p' "$0" | sed 's/^# \?//'
    exit "${1:-0}"
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help) usage 0 ;;
        native) BUILD_WASM=0; shift ;;
        wasm) BUILD_NATIVE=0; shift ;;
        --docker) MODE=docker; shift ;;
        --local) MODE=local; shift ;;
        *) echo "Unknown argument: $1" >&2; usage 1 ;;
    esac
done

have() { command -v "$1" >/dev/null 2>&1; }

jflag() {
    if have nproc; then
        echo "-j$(nproc)"
    else
        echo "-j4"
    fi
}

ensure_monaco() {
    if [[ -f "$ROOT/web/monaco/vs/loader.js" ]]; then
        return
    fi
    echo "==> Monaco editor not vendored; running scripts/vendor-monaco.sh"
    bash "$ROOT/scripts/vendor-monaco.sh"
}

build_native_local() {
    echo "==> Native (local cmake)"
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build $(jflag)
    echo "    -> build/pacman"
}

build_wasm_local() {
    ensure_monaco
    echo "==> WASM (local emscripten)"
    emcmake cmake -B build-wasm -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel
    cmake --build build-wasm $(jflag)
    echo "    -> build-wasm/pacman.html"
}

build_native_docker() {
    echo "==> Native (Docker)"
    docker run --rm -v "$ROOT:/src" -w /src debian:bookworm-slim bash -lc '
        set -e
        apt-get update -qq
        apt-get install -y -qq cmake gcc pkg-config \
            libgl-dev libx11-dev libxi-dev libxcursor-dev libxrandr-dev libasound2-dev
        rm -rf build
        cmake -B build -DCMAKE_BUILD_TYPE=Release
        cmake --build build -j"$(nproc)"
    '
    echo "    -> build/pacman"
}

build_wasm_docker() {
    ensure_monaco
    echo "==> WASM (Docker emscripten)"
    docker run --rm -v "$ROOT:/src" -w /src emscripten/emsdk:latest bash -lc '
        set -e
        if [[ ! -f web/monaco/vs/loader.js ]]; then
            echo "error: web/monaco/vs missing; run ./scripts/vendor-monaco.sh on the host first" >&2
            exit 1
        fi
        emcmake cmake -B build-wasm -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=MinSizeRel
        cmake --build build-wasm -j"$(nproc)"
    '
    echo "    -> build-wasm/pacman.html"
}

pick_mode() {
    if [[ "$MODE" == "docker" ]]; then
        echo docker
        return
    fi
    if [[ "$MODE" == "local" ]]; then
        echo local
        return
    fi
    if [[ "$BUILD_NATIVE" -eq 1 ]] && have cmake && have gcc; then
        if [[ "$BUILD_WASM" -eq 0 ]] || have emcmake; then
            echo local
            return
        fi
    fi
    echo docker
}

main() {
    if ! have docker && [[ "$(pick_mode)" == "docker" ]]; then
        echo "error: docker not found and local toolchain incomplete" >&2
        echo "  native needs: cmake, gcc" >&2
        echo "  wasm needs: emcmake (emscripten) or use --docker with docker installed" >&2
        exit 1
    fi

    local mode
    mode="$(pick_mode)"
    echo "Build mode: $mode"

    if [[ "$BUILD_NATIVE" -eq 1 ]]; then
        if [[ "$mode" == "local" ]]; then
            build_native_local
        else
            build_native_docker
        fi
    fi

    if [[ "$BUILD_WASM" -eq 1 ]]; then
        if [[ "$mode" == "local" ]] && have emcmake; then
            build_wasm_local
        else
            if [[ "$mode" == "local" ]]; then
                echo "emcmake not found; using Docker for WASM"
            fi
            build_wasm_docker
        fi
    fi

    echo "Done."
}

main
