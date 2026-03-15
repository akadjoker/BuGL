#!/usr/bin/env bash
# ============================================================
#  BuGL release script
#  Uso: ./tools/release.sh [--skip-build] [--skip-linux] [--skip-win] [--local-only] [TAG]
#
#  Exemplos:
#    ./tools/release.sh                      → build tudo + publica
#    ./tools/release.sh --skip-build         → só empacota + publica (já built)
#    ./tools/release.sh --skip-win v2.1.0    → só Linux + publica
#    ./tools/release.sh --local-only          → build+package sem tag/publish
#    GITHUB_TOKEN=ghp_xxx ./tools/release.sh
#
#  Requer: cmake, ninja (ou make), x86_64-w64-mingw32-g++, curl
#  GITHUB_TOKEN deve estar no ambiente
# ============================================================
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLS_DIR="$ROOT_DIR/tools"

# ── Flags ────────────────────────────────────────────────────
SKIP_BUILD=0
SKIP_LINUX=0
SKIP_WIN=0
LOCAL_ONLY=0
TAG=""

for arg in "$@"; do
    case "$arg" in
        --skip-build) SKIP_BUILD=1 ;;
        --skip-linux) SKIP_LINUX=1 ;;
        --skip-win)   SKIP_WIN=1   ;;
        --local-only|--no-publish) LOCAL_ONLY=1 ;;
        *)            TAG="$arg"   ;;
    esac
done

# ── Versão ──────────────────────────────────────────────────
if [[ -z "$TAG" ]]; then
    CMAKE_VERSION="$(grep -m1 'project(bugl VERSION' "$ROOT_DIR/CMakeLists.txt" | grep -oP '\d+\.\d+\.\d+')"
    TAG="v${CMAKE_VERSION}"
fi

echo "==> Release tag: $TAG"
if [[ $LOCAL_ONLY -eq 1 ]]; then
    echo "==> Modo: local-only (sem git tag / GitHub release)"
fi

# ── GitHub repo/token (apenas publish) ───────────────────────
REPO_SLUG=""
if [[ $LOCAL_ONLY -eq 0 ]]; then
    REPO_URL="$(git -C "$ROOT_DIR" remote get-url origin)"
    # extrai  owner/repo  de https://github.com/owner/repo.git
    REPO_SLUG="$(echo "$REPO_URL" | sed 's|.*/github.com/||;s|\.git$||')"
    echo "==> Repo: $REPO_SLUG"

    if [[ -z "${GITHUB_TOKEN:-}" ]]; then
        # tenta ler do ficheiro de credenciais do gh CLI
        GH_HOSTS="$HOME/.config/gh/hosts.yml"
        if [[ -f "$GH_HOSTS" ]]; then
            GITHUB_TOKEN="$(grep -A2 'github.com' "$GH_HOSTS" | grep 'oauth_token' | awk '{print $2}')"
        fi
    fi
    if [[ -z "${GITHUB_TOKEN:-}" ]]; then
        echo "error: GITHUB_TOKEN não encontrado."
        echo "  Exporta GITHUB_TOKEN=ghp_... ou usa --local-only."
        exit 1
    fi
fi

# ── Número de cores ──────────────────────────────────────────
JOBS="$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)"

# ── Cache de compilação ───────────────────────────────────────
CACHE_TOOL=""
if command -v sccache >/dev/null 2>&1; then
    CACHE_TOOL="sccache"
elif command -v ccache >/dev/null 2>&1; then
    CACHE_TOOL="ccache"
    export CCACHE_BASEDIR="$ROOT_DIR"
    export CCACHE_CPP2=true
fi

if [[ -n "$CACHE_TOOL" ]]; then
    echo "==> Compiler cache: $CACHE_TOOL"
else
    echo "==> Compiler cache: desativado (instala sccache ou ccache para acelerar rebuilds)"
fi

# ── Detecta gerador ─────────────────────────────────────────
if command -v ninja >/dev/null 2>&1; then
    GENERATOR="Ninja"
else
    GENERATOR="Unix Makefiles"
fi

# ============================================================
#  1. BUILD LINUX RELEASE
# ============================================================
BUILD_LINUX="$ROOT_DIR/build-release"
if [[ $SKIP_BUILD -eq 0 && $SKIP_LINUX -eq 0 ]]; then
    if [[ -f "$BUILD_LINUX/CMakeCache.txt" ]] && grep -Eq "^CMAKE_TOOLCHAIN_FILE:.*mingw" "$BUILD_LINUX/CMakeCache.txt"; then
        echo "warning: build-release contém cache mingw; a limpar para reconfigurar Linux nativo"
        rm -rf "$BUILD_LINUX"
    fi
    echo ""
    echo "==> [1/4] Configurar Linux Release..."
    cmake -S "$ROOT_DIR" -B "$BUILD_LINUX" \
        -G "$GENERATOR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUGL_PORTABLE_RELEASE=ON \
        -DBUGL_ENABLE_COMPILER_CACHE=ON \
        -DBUGL_STRIP_RELEASE=ON

    echo "==> [1/4] Build Linux Release (${JOBS} jobs)..."
    cmake --build "$BUILD_LINUX" -j"$JOBS"
else
    echo "==> [1/4] Build Linux ignorado (--skip-build/--skip-linux)"
fi

LINUX_ARCHIVE=""
if [[ $SKIP_LINUX -eq 0 ]]; then
    echo "==> [1/4] Empacotar Linux..."
    bash "$TOOLS_DIR/package_release.sh"
    LINUX_ARCHIVE="$(ls "$ROOT_DIR/dist/bugl-linux-"*.tar.gz 2>/dev/null | head -1)"
    echo "    -> $LINUX_ARCHIVE"
else
    echo "==> [1/4] Empacotar Linux ignorado (--skip-linux)"
fi

# ============================================================
#  2. BUILD WINDOWS RELEASE (cross-compile MinGW)
# ============================================================
BUILD_WIN="$ROOT_DIR/build-win64-release"
if [[ $SKIP_BUILD -eq 0 && $SKIP_WIN -eq 0 ]]; then
    if [[ -f "$BUILD_WIN/CMakeCache.txt" ]] && ! grep -Eq "^CMAKE_TOOLCHAIN_FILE:.*mingw-w64\\.cmake" "$BUILD_WIN/CMakeCache.txt"; then
        echo "warning: build-win64-release sem toolchain mingw; a limpar para reconfigurar cross-compile"
        rm -rf "$BUILD_WIN"
    fi
    echo ""
    echo "==> [2/4] Configurar Windows Release (MinGW cross)..."
    cmake -S "$ROOT_DIR" -B "$BUILD_WIN" \
        -G "$GENERATOR" \
        -DCMAKE_TOOLCHAIN_FILE="$ROOT_DIR/mingw-w64.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUGL_PORTABLE_RELEASE=ON \
        -DBUGL_ENABLE_COMPILER_CACHE=ON \
        -DBUGL_STRIP_RELEASE=ON

    echo "==> [2/4] Build Windows Release (${JOBS} jobs)..."
    if ! cmake --build "$BUILD_WIN" -j"$JOBS"; then
        echo "warning: build Windows paralelo falhou; a repetir com -j1 (fallback estável)"
        cmake --build "$BUILD_WIN" -j1
    fi
else
    echo "==> [2/4] Build Windows ignorado (--skip-build/--skip-win)"
fi

WIN_ARCHIVE=""
if [[ $SKIP_WIN -eq 0 ]]; then
    echo "==> [2/4] Empacotar Windows..."
    bash "$TOOLS_DIR/package_release_win64.sh"
    WIN_ARCHIVE="$ROOT_DIR/dist/bugl-win64.zip"
    echo "    -> $WIN_ARCHIVE"
else
    echo "==> [2/4] Empacotar Windows ignorado (--skip-win)"
fi

if [[ "$CACHE_TOOL" == "ccache" ]]; then
    echo "==> ccache stats:"
    ccache -s || true
elif [[ "$CACHE_TOOL" == "sccache" ]]; then
    echo "==> sccache stats:"
    sccache --show-stats || true
fi

if [[ $LOCAL_ONLY -eq 1 ]]; then
    echo ""
    echo "==> Release local concluído (sem publish)."
    [[ -n "$LINUX_ARCHIVE" ]] && echo "    Linux:   $LINUX_ARCHIVE"
    [[ -n "$WIN_ARCHIVE" ]] && echo "    Windows: $WIN_ARCHIVE"
    exit 0
fi

# ============================================================
#  3. TAG GIT
# ============================================================
echo ""
echo "==> [3/4] Criar tag git $TAG..."
cd "$ROOT_DIR"
if git tag | grep -q "^${TAG}$"; then
    echo "    tag $TAG já existe, a saltar criação"
else
    git tag -a "$TAG" -m "Release $TAG"
    git push origin "$TAG"
    echo "    tag $TAG criada e pushed"
fi

# ============================================================
#  4. GITHUB RELEASE + UPLOAD
# ============================================================
echo ""
echo "==> [4/4] Criar GitHub Release $TAG..."

API="https://api.github.com"
AUTH_HEADER="Authorization: Bearer $GITHUB_TOKEN"

# Cria release (ignora se já existe)
RELEASE_JSON="$(curl -sf -X POST "$API/repos/$REPO_SLUG/releases" \
    -H "$AUTH_HEADER" \
    -H "Content-Type: application/json" \
    -d "{\"tag_name\":\"$TAG\",\"name\":\"BuGL $TAG\",\"draft\":false,\"prerelease\":false,\"generate_release_notes\":true}" \
    2>/dev/null || true)"

if [[ -z "$RELEASE_JSON" ]]; then
    echo "    release já existe, a obter upload_url..."
    RELEASE_JSON="$(curl -sf "$API/repos/$REPO_SLUG/releases/tags/$TAG" \
        -H "$AUTH_HEADER")"
fi

UPLOAD_URL="$(echo "$RELEASE_JSON" | python3 -c "import sys,json; print(json.load(sys.stdin)['upload_url'])" | sed 's/{.*}//')"
RELEASE_ID="$(echo "$RELEASE_JSON"  | python3 -c "import sys,json; print(json.load(sys.stdin)['id'])")"
echo "    Release ID: $RELEASE_ID"

upload_asset() {
    local FILE="$1"
    local NAME="$(basename "$FILE")"
    local MIME="application/octet-stream"
    echo "    Upload: $NAME"
    curl -sf -X POST "${UPLOAD_URL}?name=${NAME}" \
        -H "$AUTH_HEADER" \
        -H "Content-Type: $MIME" \
        --data-binary @"$FILE" > /dev/null
    echo "    OK: $NAME"
}

[[ -f "$LINUX_ARCHIVE" ]] && upload_asset "$LINUX_ARCHIVE"
[[ -f "$WIN_ARCHIVE"   ]] && upload_asset "$WIN_ARCHIVE"

echo ""
echo "==> Release $TAG publicada!"
echo "    https://github.com/$REPO_SLUG/releases/tag/$TAG"
