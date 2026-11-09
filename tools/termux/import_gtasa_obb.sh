#!/data/data/com.termux/files/usr/bin/bash
# =====================================================================
#  Kingmakers RP - copy your own GTA: San Andreas OBB into Kingmakers RP
#
#  Copies Android/obb/com.rockstargames.gtasa/*.obb into
#  Android/data/com.russia.game/files/import/, where the Kingmakers RP
#  launcher imports it automatically and then deletes the copy.
#
#  Android 11+ blocks normal apps (Termux included) from those folders, so
#  the copy runs as the "shell" user through one of:
#    1. direct access   (Android 10 and older, after termux-setup-storage)
#    2. Shizuku's rish  (if you use Shizuku)
#    3. wireless ADB to this same phone (android-tools, no PC or root needed)
#
#  Usage in Termux:
#    curl -fsSLO https://raw.githubusercontent.com/thuwanishark2001-art/kingmakers-rp/main/tools/termux/import_gtasa_obb.sh
#    bash import_gtasa_obb.sh
# =====================================================================
set -u

SRC="/storage/emulated/0/Android/obb/com.rockstargames.gtasa"
APP_PKG="${KM_APP_PKG:-com.russia.game}"
DST="/storage/emulated/0/Android/data/${APP_PKG}/files/import"

say()  { printf '\033[1;33m[KM]\033[0m %s\n' "$*"; }
fail() { printf '\033[1;31m[KM] %s\033[0m\n' "$*"; exit 1; }

# Shell command that does the copy; run as whichever user can reach both folders
COPY_CMD="set -e; ls '$SRC'/*.obb >/dev/null; mkdir -p '$DST'; for f in '$SRC'/*.obb; do echo \"copying \$(basename \"\$f\") (\$(du -h \"\$f\" | cut -f1))\"; cp \"\$f\" '$DST/'; done; ls -l '$DST'"

say "Kingmakers RP - GTA: San Andreas data helper"

# --- 1. Direct access (Android 10 and older) ---------------------------
if ls "$SRC"/*.obb >/dev/null 2>&1; then
    say "Direct access works, copying..."
    if sh -c "$COPY_CMD"; then
        say "Done. Open Kingmakers RP and tap PLAY: the import starts by itself."
        exit 0
    fi
    say "Direct copy failed, trying shell access instead."
fi

# --- 2. Shizuku (rish) -------------------------------------------------
if command -v rish >/dev/null 2>&1; then
    say "Using Shizuku (rish)..."
    if rish -c "$COPY_CMD"; then
        say "Done. Open Kingmakers RP and tap PLAY: the import starts by itself."
        exit 0
    fi
    say "rish copy failed, trying wireless ADB."
fi

# --- 3. Wireless ADB to this phone -------------------------------------
if ! command -v adb >/dev/null 2>&1; then
    say "Installing android-tools (adb)..."
    pkg install -y android-tools >/dev/null 2>&1 || fail "Could not install android-tools. Run: pkg update && pkg install android-tools"
fi

has_device() { adb devices 2>/dev/null | awk 'NR>1 && $2=="device"' | grep -q .; }

if ! has_device; then
    cat <<'TXT'

  One-time setup: let Termux talk to this phone over wireless ADB
  ---------------------------------------------------------------
  1. Settings > About phone > tap "Build number" 7 times (enables Developer options).
  2. Settings > Developer options > turn on "Wireless debugging" (stay on Wi-Fi).
  3. Open Wireless debugging > "Pair device with pairing code".
     Use split screen (Termux + Settings) so the code stays visible.

TXT
    read -r -p "Pairing port shown on screen (e.g. 37123): " PAIR_PORT
    read -r -p "Pairing code (6 digits): " PAIR_CODE
    adb pair "127.0.0.1:${PAIR_PORT}" "${PAIR_CODE}" || fail "Pairing failed. Check the port and code, then run the script again."

    echo
    say "Paired. Now use the port shown on the main Wireless debugging screen"
    say "(under 'IP address & Port', the number after the colon - it differs from the pairing port)."
    read -r -p "Connection port: " CONN_PORT
    adb connect "127.0.0.1:${CONN_PORT}" >/dev/null 2>&1
    sleep 1
    has_device || fail "Could not connect. Check Wireless debugging is still on and run the script again."
fi

say "Connected. Checking GTA: San Andreas..."
adb shell "ls '$SRC'/*.obb" >/dev/null 2>&1 || fail "No OBB found in $SRC. Install GTA: San Andreas from Google Play and open it once so it downloads its data."
adb shell "pm path $APP_PKG" >/dev/null 2>&1 || fail "Kingmakers RP ($APP_PKG) is not installed."

say "Copying (1.5-2.6 GB, this can take a few minutes)..."
adb shell "$COPY_CMD" || fail "Copy failed. Check you have enough free space (about 3 GB), then run the script again."

say "Done. Open Kingmakers RP: the import starts by itself, then the copy is deleted."
say "You can turn Wireless debugging off again."
