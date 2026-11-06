#!/usr/bin/env bash
# Runs the Windows NeuralHub manager on Linux (Wine + WebView2).
# usage: linux/neuralhub-wine.sh /path/to/NeuralHub_v0.8.0_setup.exe
# The in-game part of the mod is still Windows-only; see the README.
set -euo pipefail

SETUP="${1:?usage: $0 /path/to/NeuralHub_vX.X.X_setup.exe}"
BASE="${NEURALHUB_HOME:-$HOME/.local/share/neuralhub}"
export WINEPREFIX="${WINEPREFIX:-$BASE/wineprefix}"
export WINEDEBUG="${WINEDEBUG:--all}"
mkdir -p "$BASE"

command -v 7z >/dev/null || { echo "7z is required (p7zip)" >&2; exit 1; }
command -v wine >/dev/null || { echo "wine is required" >&2; exit 1; }

if [ ! -f "$BASE/neuralhub.exe" ]; then
  7z e -o"$BASE" -y "$SETUP" neuralhub.exe >/dev/null
fi

if [ ! -d "$WINEPREFIX/drive_c/Program Files (x86)/Microsoft/EdgeWebView" ]; then
  echo "installing WebView2 into $WINEPREFIX (once, ~150 MB)"
  wineboot -i
  W="$(command -v winetricks || true)"
  if [ -z "$W" ] || ! "$W" list-all 2>/dev/null | grep -qx webview2; then
    [ -x "$BASE/winetricks" ] || curl -sL -o "$BASE/winetricks" https://raw.githubusercontent.com/Winetricks/winetricks/master/src/winetricks
    chmod +x "$BASE/winetricks"
    W="$BASE/winetricks"
  fi
  "$W" -q webview2
fi

# The manager only looks for Steam in the registry; in Wine, Z: is /.
STEAM="${STEAM_PATH:-}"
for d in "$STEAM" "$HOME/.local/share/Steam" "$HOME/.steam/steam" "$HOME/.var/app/com.valvesoftware.Steam/data/Steam"; do
  if [ -n "$d" ] && [ -d "$d/steamapps" ]; then STEAM="$d"; break; fi
done
if [ -n "$STEAM" ] && ! wine reg query 'HKCU\Software\Valve\Steam' /v SteamPath >/dev/null 2>&1; then
  wine reg add 'HKCU\Software\Valve\Steam' /v SteamPath /t REG_SZ /d "Z:${STEAM//\//\\}" /f >/dev/null
fi

exec wine "$BASE/neuralhub.exe"
