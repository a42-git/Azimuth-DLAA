#!/bin/sh
# Builds dxgi.dll (x86_64 Windows) with Zig, and optionally a release zip.
#   ZIG=/path/to/zig ./build.sh            -> build/dxgi.dll
#   ZIG=/path/to/zig ./build.sh release    -> also dist/FH2-DLAA-<version>.zip
#
# fh2_ngx.dll (the NVIDIA NGX half) must be built separately with MSVC: bridge/build_bridge.bat.
# NVIDIA's DLSS SDK (https://github.com/NVIDIA/DLSS) provides the NGX headers/lib and nvngx_dlss.dll.
set -e
cd "$(dirname "$0")"
VERSION=0.1.0-azimuth-1.0.3-public-test
ZIG=${ZIG:-./zig}
DLSS=third_party/DLSS

mkdir -p build
"$ZIG" cc -target x86_64-windows-gnu -shared -O2 -Wall -Wextra -Wno-unused-parameter -Werror \
    -o build/dxgi.dll src/proxy.c src/hooks.c src/dlss.c src/dxgi.def -luser32 -lkernel32
echo "built build/dxgi.dll"

if [ "$1" = "release" ]; then
    if [ ! -f build/fh2_ngx.dll ]; then
        echo "build/fh2_ngx.dll is missing: run bridge\\build_bridge.bat first (MSVC)" >&2
        exit 1
    fi
    if [ ! -f "$DLSS/lib/Windows_x86_64/rel/nvngx_dlss.dll" ]; then
        rm -rf "$DLSS"
        git clone -q --depth 1 https://github.com/NVIDIA/DLSS.git "$DLSS"
    fi
    OUT=dist/FH2-DLAA-$VERSION
    rm -rf "$OUT" "$OUT.zip"
    mkdir -p "$OUT"
    cp build/dxgi.dll build/fh2_ngx.dll "$OUT/"
    cp "$DLSS/lib/Windows_x86_64/rel/nvngx_dlss.dll" "$OUT/"
    [ -f "$DLSS/LICENSE.txt" ] && cp "$DLSS/LICENSE.txt" "$OUT/LICENSE-NVIDIA-DLSS.txt"
    cp LICENSE "$OUT/LICENSE.txt"
    cp release/README.txt "$OUT/"
    if command -v zip >/dev/null 2>&1; then
        (cd dist && zip -q -r "FH2-DLAA-$VERSION.zip" "FH2-DLAA-$VERSION")
    else
        (cd dist && "${SYSTEMROOT:-C:/Windows}/System32/tar.exe" -a -cf "FH2-DLAA-$VERSION.zip" "FH2-DLAA-$VERSION")
    fi
    echo "packaged dist/FH2-DLAA-$VERSION.zip"
fi
