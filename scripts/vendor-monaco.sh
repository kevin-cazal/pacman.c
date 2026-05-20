#!/usr/bin/env bash
# Vendor monaco-editor min/vs into web/monaco/vs for offline WASM mod lab.
set -euo pipefail

MONACO_VERSION="${MONACO_VERSION:-0.52.2}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DEST="$ROOT/web/monaco/vs"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

cd "$TMP"

echo "==> Fetching monaco-editor@${MONACO_VERSION}"
if command -v npm >/dev/null 2>&1; then
    npm pack "monaco-editor@${MONACO_VERSION}" --silent
    TARBALL="monaco-editor-${MONACO_VERSION}.tgz"
else
    URL="https://registry.npmjs.org/monaco-editor/-/monaco-editor-${MONACO_VERSION}.tgz"
    TARBALL="monaco-editor.tgz"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL -o "$TARBALL" "$URL"
    elif command -v wget >/dev/null 2>&1; then
        wget -q -O "$TARBALL" "$URL"
    else
        echo "error: need npm, curl, or wget to download monaco-editor" >&2
        exit 1
    fi
fi

tar xf "$TARBALL"
rm -rf "$DEST"
mkdir -p "$(dirname "$DEST")"
cp -a package/min/vs "$DEST"

echo "==> Installed monaco-editor ${MONACO_VERSION} -> web/monaco/vs"
echo "    ($(du -sh "$DEST" | cut -f1))"
