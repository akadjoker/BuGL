#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist/bugl-win64"
BIN_DIR="$DIST_DIR/bin"
PLUGIN_DIR="$BIN_DIR/plugins"

if [[ ! -f "$ROOT_DIR/bin/bugl.exe" ]]; then
    echo "error: missing $ROOT_DIR/bin/bugl.exe"
    echo "build first with: cmake --build build-win64-release --target main bu_ode bu_box2d bu_jolt bu_chip2d"
    exit 1
fi

if [[ ! -d "$ROOT_DIR/bin/plugins" ]]; then
    echo "error: missing $ROOT_DIR/bin/plugins"
    exit 1
fi

rm -rf "$DIST_DIR"
mkdir -p "$PLUGIN_DIR"

cp -a "$ROOT_DIR/bin/bugl.exe" "$BIN_DIR/"
shopt -s nullglob
win_plugins=("$ROOT_DIR/bin/plugins/"*.dll)
if [[ ${#win_plugins[@]} -eq 0 ]]; then
    echo "error: no Windows plugins (.dll) found in $ROOT_DIR/bin/plugins"
    exit 1
fi
cp -a "${win_plugins[@]}" "$PLUGIN_DIR/"
shopt -u nullglob
cp -a "$ROOT_DIR/scripts" "$DIST_DIR/"
cp -a "$ROOT_DIR/assets" "$DIST_DIR/"
cp -a "$ROOT_DIR/README.md" "$DIST_DIR/"
cp -a "$ROOT_DIR/LICENSE" "$DIST_DIR/"

ARCHIVE_PATH="$ROOT_DIR/dist/bugl-win64.zip"
rm -f "$ARCHIVE_PATH"

if command -v zip >/dev/null 2>&1; then
    (
        cd "$ROOT_DIR/dist"
        zip -rq "$(basename "$ARCHIVE_PATH")" "$(basename "$DIST_DIR")"
    )
else
    echo "warning: zip not found, leaving unpacked folder only"
fi

echo "release folder: $DIST_DIR"
if [[ -f "$ARCHIVE_PATH" ]]; then
    echo "release archive: $ARCHIVE_PATH"
fi
