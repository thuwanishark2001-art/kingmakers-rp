#!/bin/bash
# =====================================================================
# Kingmakers RP - One-Click VPS Updater & Sync Script
# Updates git repo, syncs server/backend to service directory,
# updates permissions, restarts services, and verifies API config.
# =====================================================================

set -e

REPO_DIR="/root/kingmakers-rp"
SERVICE_DIR="/home/kingmakers/kingmakers-rp"

echo "=========================================================="
echo "  KINGMAKERS RP - VPS UPDATE & SYNC"
echo "=========================================================="

# 1. Pull latest code from GitHub
cd "$REPO_DIR"
echo "[1/4] Pulling latest repository updates..."
git pull origin main

# 2. Stop running services temporarily for clean file synchronization (prevents "Text file busy")
echo "[2/5] Stopping services for clean update..."
systemctl stop km-server km-backend 2>/dev/null || true

# 3. Sync backend and server code to the service directory
echo "[3/5] Syncing backend, server, and public files to $SERVICE_DIR..."
mkdir -p "$SERVICE_DIR/backend" "$SERVICE_DIR/server" "$SERVICE_DIR/backend/public/downloads"
cp -r "$REPO_DIR/backend"/* "$SERVICE_DIR/backend/"
if [ -d "$REPO_DIR/server" ]; then
    cp -r "$REPO_DIR/server"/* "$SERVICE_DIR/server/"
fi
cp "$REPO_DIR"/*.sh "$SERVICE_DIR/" 2>/dev/null || true

# Build the Kingmakers RP content pack (our own client files only) and remove the
# old placeholder "full game cache" archives, which contained no real game data.
echo "      Building Kingmakers RP content pack..."
KM_CONTENT_ZIP_OUT="$SERVICE_DIR/backend/public/downloads/kingmakers_content_pack_v1.0.zip" \
    python3 "$REPO_DIR/backend/build_content_pack.py"
rm -f "$SERVICE_DIR/backend/public/downloads"/kingmakers_full_game_cache_* \
      "$SERVICE_DIR/backend/public/downloads"/kingmakers_game_cache_*

# 4. Fix permissions and executable flags
echo "[4/5] Updating user permissions and executable flags..."
chmod +x "$REPO_DIR"/*.sh "$REPO_DIR/server"/*.sh "$REPO_DIR/server/qawno/pawncc" 2>/dev/null || true
chmod +x "$SERVICE_DIR"/*.sh "$SERVICE_DIR/server"/*.sh "$SERVICE_DIR/server/omp-server" 2>/dev/null || true
chmod +x "$SERVICE_DIR/backend"/*.py 2>/dev/null || true
chown -R kingmakers:kingmakers "$SERVICE_DIR"

# 5. Start services
echo "[5/5] Starting km-server and km-backend services..."
systemctl start km-server km-backend

echo ""
echo "=========================================================="
echo "  UPDATE COMPLETE! LIVE LAUNCHER CONFIG:"
echo "=========================================================="
sleep 1
curl -s http://127.0.0.1:8080/api/launcher/config | jq .
