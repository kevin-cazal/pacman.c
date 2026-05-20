#!/usr/bin/env bash
# Collect WASM build artifacts for GitHub Pages (or static hosting).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${1:-$ROOT/build-wasm}"
DEST="${2:-$ROOT/site}"

if [[ ! -f "$SRC/index.html" ]]; then
    echo "error: $SRC/index.html not found — run ./build.sh wasm first" >&2
    exit 1
fi

rm -rf "$DEST"
mkdir -p "$DEST"

for f in index.html index.js index.wasm mod_loader.js mod_editor.js mod_lab.html; do
    if [[ -f "$SRC/$f" ]]; then
        cp -f "$SRC/$f" "$DEST/"
    fi
done

if [[ -d "$SRC/monaco" ]]; then
    cp -a "$SRC/monaco" "$DEST/"
fi

touch "$DEST/.nojekyll"

echo "==> Staged $(du -sh "$DEST" | cut -f1) -> $DEST"
