#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIST_DIR="$ROOT_DIR/dist/bugl"
BIN_DIR="$DIST_DIR/bin"
PLUGIN_DIR="$BIN_DIR/plugins"

if [[ ! -x "$ROOT_DIR/bin/bugl" ]]; then
    echo "error: missing $ROOT_DIR/bin/bugl"
    echo "build first with: cmake --build build --target main bu_ode bu_box2d bu_jolt bu_chip2d"
    exit 1
fi

if [[ ! -d "$ROOT_DIR/bin/plugins" ]]; then
    echo "error: missing $ROOT_DIR/bin/plugins"
    exit 1
fi

rm -rf "$DIST_DIR"
mkdir -p "$PLUGIN_DIR"

cp -a "$ROOT_DIR/bin/bugl" "$BIN_DIR/"
cp -a "$ROOT_DIR/bin/plugins/." "$PLUGIN_DIR/"
cp -a "$ROOT_DIR/scripts" "$DIST_DIR/"
cp -a "$ROOT_DIR/assets" "$DIST_DIR/"
cp -a "$ROOT_DIR/README.md" "$DIST_DIR/"
cp -a "$ROOT_DIR/LICENSE" "$DIST_DIR/"

OS_NAME="$(uname -s | tr '[:upper:]' '[:lower:]')"
ARCH_NAME="$(uname -m)"
ARCHIVE_PATH="$ROOT_DIR/dist/bugl-${OS_NAME}-${ARCH_NAME}.tar.gz"

rm -f "$ARCHIVE_PATH"
tar -czf "$ARCHIVE_PATH" -C "$ROOT_DIR/dist" bugl

echo "release folder: $DIST_DIR"
echo "release archive: $ARCHIVE_PATH"
