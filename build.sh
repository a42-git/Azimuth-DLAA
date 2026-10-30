#!/bin/sh
# Builds the unified Azimuth-DLAA dxgi.dll (x86_64 Windows) with Zig.
#   ZIG=/path/to/zig ./build.sh
#   ZIG=/path/to/zig ./build.sh release
#
# azimuth_ngx.dll is built separately with MSVC using bridge/build_bridge.bat.
set -e
cd "$(dirname "$0")"
VERSION=0.2.0-alpha.1
ZIG=${ZIG:-./zig}

mkdir -p build
"$ZIG" cc -target x86_64-windows-gnu -shared -O2 -Wall -Wextra -Wno-unused-parameter -Werror \
    -o build/dxgi.dll \
    src/proxy.c src/hooks.c src/dlss.c src/game_profiles.c \
    src/games/fh2.c src/games/fh3.c src/dxgi.def \
    -luser32 -lkernel32
echo "built build/dxgi.dll"

if [ "$1" = "release" ]; then
    if [ ! -f build/azimuth_ngx.dll ]; then
        echo "build/azimuth_ngx.dll is missing: run bridge\\build_bridge.bat first (MSVC)" >&2
        exit 1
    fi
    OUT=dist/Azimuth-DLAA-$VERSION
    rm -rf "$OUT" "$OUT.zip"
    mkdir -p "$OUT"
    cp build/dxgi.dll build/azimuth_ngx.dll "$OUT/"
    cp LICENSE "$OUT/LICENSE.txt"
    [ -f LICENSE-NVIDIA-DLSS.txt ] && cp LICENSE-NVIDIA-DLSS.txt "$OUT/"
    cp release/README.txt "$OUT/"
    if command -v zip >/dev/null 2>&1; then
        (cd dist && zip -q -r "Azimuth-DLAA-$VERSION.zip" "Azimuth-DLAA-$VERSION")
    else
        (cd dist && "${SYSTEMROOT:-C:/Windows}/System32/tar.exe" -a -cf "Azimuth-DLAA-$VERSION.zip" "Azimuth-DLAA-$VERSION")
    fi
    echo "packaged dist/Azimuth-DLAA-$VERSION.zip"
    echo "note: nvngx_dlss.dll is intentionally not bundled"
fi
