#!/usr/bin/env bash
# Installs DLSS-NR-on-AMD + the Linux HIP bridge into a Proton game prefix.
#
# Usage:
#   linux/install-mod.sh <game_binaries_dir> <compatdata_pfx_dir>
#
# Example:
#   linux/install-mod.sh \
#     "$HOME/.local/share/Steam/steamapps/common/Squad/SquadGame/Binaries/Win64" \
#     "$HOME/.local/share/Steam/steamapps/compatdata/393380/pfx"
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BRIDGE_DIR="$SCRIPT_DIR/hipbridge"
OUT_DIR="$BRIDGE_DIR/out"
BASE_DATA="${NEURALHUB_HOME:-$HOME/.local/share/neuralhub}"
CABINET_SRC="$BASE_DATA/cabinet.win2022.dll"

usage() {
    echo "Usage: $0 <game_binaries_dir> <compatdata_pfx_dir>" >&2
    exit 1
}

[ $# -eq 2 ] || usage

GDIR="$1"
PFX="$2"

[ -d "$GDIR" ] || { echo "Error: game directory not found: $GDIR" >&2; exit 1; }
[ -d "$PFX/drive_c/windows/system32" ] || { echo "Error: invalid wine/proton prefix: $PFX" >&2; exit 1; }

# 1. Ensure HIP bridge is built
if [ ! -f "$OUT_DIR/x86_64-windows/amdhip64_7.dll" ] || [ ! -f "$OUT_DIR/x86_64-unix/amdhip64_7.so" ]; then
    echo "Building HIP bridge..."
    (cd "$BRIDGE_DIR" && ./build.sh)
fi
cp -f "$OUT_DIR/x86_64-windows/amdhip64_7.dll" "$OUT_DIR/amdhip64_7.dll"

# 2. Check for native cabinet.dll (required for LZMS decompression)
if [ ! -f "$CABINET_SRC" ]; then
    echo "Warning: genuine Windows cabinet.dll not found at $CABINET_SRC" >&2
    echo "LZMS decompression requires a genuine Windows 10/11/Server 2022 cabinet.dll." >&2
    echo "You can extract one using podman:" >&2
    echo "  podman run --rm mcr.microsoft.com/windows/nanoserver:ltsc2022 tar -c Files/Windows/System32/cabinet.dll | tar -x -O Files/Windows/System32/cabinet.dll > '$CABINET_SRC'" >&2
else
    echo "Installing genuine cabinet.dll into prefix system32..."
    rm -f "$PFX/drive_c/windows/system32/cabinet.dll"
    cp -f "$CABINET_SRC" "$PFX/drive_c/windows/system32/cabinet.dll"
fi

# 3. Locate mod runtime files
RUNTIME_SRC=""
for d in \
    "$BASE_DATA/runtime/v0.8.0" \
    "$HOME/.wine/drive_c/users/$USER/AppData/Local/Danielblnc/NeuralHub/runtime/v0.8.0" \
    "$BASE_DATA"; do
    if [ -f "$d/neuralhub_host.dll" ]; then
        RUNTIME_SRC="$d"
        break
    fi
done

if [ -z "$RUNTIME_SRC" ]; then
    echo "Error: could not find neuralhub_host.dll." >&2
    echo "Run linux/neuralhub-wine.sh first to extract and set up NeuralHub." >&2
    exit 1
fi

STUB_SRC=""
for f in \
    "$BASE_DATA/neuralhub_stub.dll" \
    "$RUNTIME_SRC/neuralhub_stub.dll" \
    "$HOME/.wine/drive_c/users/$USER/AppData/Local/Danielblnc/NeuralHub/stub/v0.8.0/neuralhub_stub.dll"; do
    if [ -f "$f" ]; then
        STUB_SRC="$f"
        break
    fi
done

if [ -z "$STUB_SRC" ]; then
    echo "Error: could not find neuralhub_stub.dll." >&2
    exit 1
fi

# 4. Copy runtime files to prefix C:\NeuralHub\runtime\v0.8.0\
echo "Deploying runtime to $PFX/drive_c/NeuralHub/runtime/v0.8.0/..."
mkdir -p "$PFX/drive_c/NeuralHub/runtime/v0.8.0" "$PFX/drive_c/NeuralHub/data"
cp -f "$RUNTIME_SRC/"*.dll "$PFX/drive_c/NeuralHub/runtime/v0.8.0/"

# 5. Copy stub and HIP bridge into game binaries folder
echo "Installing stub as version.dll and amdhip64_7.dll into $GDIR..."
cp -f "$STUB_SRC" "$GDIR/version.dll"
cp -f "$OUT_DIR/x86_64-windows/amdhip64_7.dll" "$GDIR/amdhip64_7.dll"

# Optional: copy nvngx_dlssnr.dll if available locally
for f in "$PWD/nvngx_dlssnr.dll" "$BASE_DATA/nvngx_dlssnr.dll"; do
    if [ -f "$f" ]; then
        echo "Found nvngx_dlssnr.dll, copying into $GDIR..."
        cp -f "$f" "$GDIR/nvngx_dlssnr.dll"
        break
    fi
done

# 6. Configure prefix registry
echo "Configuring registry in $PFX..."
WINEPREFIX="$PFX" wine reg add "HKCU\\Software\\Danielblnc\\NeuralHub" /v RuntimeAbi /t REG_DWORD /d 1 /f >/dev/null 2>&1 || true
WINEPREFIX="$PFX" wine reg add "HKCU\\Software\\Danielblnc\\NeuralHub" /v RuntimePath /t REG_SZ /d "C:\\NeuralHub\\runtime\\v0.8.0\\" /f >/dev/null 2>&1 || true
WINEPREFIX="$PFX" wine reg add "HKCU\\Software\\Danielblnc\\NeuralHub" /v RuntimeVersion /t REG_SZ /d "v0.8.0" /f >/dev/null 2>&1 || true
WINEPREFIX="$PFX" wine reg add "HKCU\\Software\\Danielblnc\\NeuralHub" /v StorePath /t REG_SZ /d "C:\\NeuralHub\\" /f >/dev/null 2>&1 || true

cat << EOM

Installation successful!

To launch the game with DLSS-NR-on-AMD through Steam, set the game's Launch Options to:

  WINEDLLPATH="$OUT_DIR" WINEDLLOVERRIDES="version=n,b;cabinet=n,b;amdhip64_7=n,b" %command%

If your system has multiple GPUs, also specify:
  MESA_VK_DEVICE_SELECT="<vendor:device>!" (e.g. 1002:7590! for Radeon RX 9060 XT)
EOM
