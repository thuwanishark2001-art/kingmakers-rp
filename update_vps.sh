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

# 2. Sync backend and server code to the service directory
echo "[2/4] Syncing backend, server, and public files to $SERVICE_DIR..."
mkdir -p "$SERVICE_DIR/backend" "$SERVICE_DIR/server" "$SERVICE_DIR/backend/public/downloads"
cp -r "$REPO_DIR/backend"/* "$SERVICE_DIR/backend/"
if [ -d "$REPO_DIR/server" ]; then
    cp -r "$REPO_DIR/server"/* "$SERVICE_DIR/server/"
fi

# 3. Fix permissions for kingmakers service user
echo "[3/4] Updating user permissions..."
chown -R kingmakers:kingmakers "$SERVICE_DIR"
chmod +x "$SERVICE_DIR/backend"/*.py 2>/dev/null || true

# 4. Restart services
echo "[4/4] Restarting km-backend service..."
systemctl restart km-backend

echo ""
echo "=========================================================="
echo "  UPDATE COMPLETE! LIVE LAUNCHER CONFIG:"
echo "=========================================================="
sleep 1
curl -s http://127.0.0.1:8080/api/launcher/config | jq .
