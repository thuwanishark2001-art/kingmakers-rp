#!/bin/bash
# =====================================================================
# Kingmakers RP - Gamemode Compiler
# Compiles gamemode/kingmakers.pwn into server/gamemodes/kingmakers.amx
# =====================================================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

PAWNCC="$SCRIPT_DIR/server/qawno/pawncc"
INCLUDES="-i$SCRIPT_DIR/server/qawno/include -i$SCRIPT_DIR/gamemode"
OUT="$SCRIPT_DIR/server/gamemodes/kingmakers.amx"
SRC="$SCRIPT_DIR/gamemode/kingmakers.pwn"

echo "=========================================================="
echo "  KINGMAKERS RP - GAMEMODE COMPILER"
echo "=========================================================="

PAWNDIR="$SCRIPT_DIR/server/qawno"
PAWNCC="$PAWNDIR/pawncc"
export LD_LIBRARY_PATH="$PAWNDIR:${LD_LIBRARY_PATH}"

chmod +x "$PAWNCC" 2>/dev/null || true

if [ -f /lib/ld-linux.so.2 ]; then
    /lib/ld-linux.so.2 --library-path "$PAWNDIR:/lib/i386-linux-gnu:/usr/lib/i386-linux-gnu:/lib32:/usr/lib32" "$PAWNCC" "$SRC" -o"$OUT" $INCLUDES -d3 -Z+
elif [ -x "$PAWNCC" ]; then
    "$PAWNCC" "$SRC" -o"$OUT" $INCLUDES -d3 -Z+ 2>/dev/null || {
        echo "Notice: pawncc requires 32-bit libraries on 64-bit Linux. If not yet installed, run:"
        echo "  sudo dpkg --add-architecture i386 && sudo apt-get update && sudo apt-get install -y libc6:i386 libstdc++6:i386"
        exit 1
    }
else
    echo "Notice: pawncc is a 32-bit compiler tool. On Ubuntu/Debian run:"
    echo "  sudo dpkg --add-architecture i386 && sudo apt-get update && sudo apt-get install -y libc6:i386 libstdc++6:i386"
    exit 1
fi

if [ -f "$OUT" ]; then
    echo "Build success! Output: $OUT ($(ls -lh "$OUT" | awk '{print $5}'))"
fi
