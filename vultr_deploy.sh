#!/usr/bin/env bash
# =====================================================================
#  KINGMAKERS RP - 1-Click Automated Deployment for Vultr Cloud VPS
#  Target OS: Ubuntu 22.04 / 24.04 LTS or Debian 12 (x86_64)
#  Installs 32-bit runtime, Node.js LTS, systemd services & firewall
# =====================================================================

set -e

echo "=========================================================="
echo "   KINGMAKERS RP - Vultr VPS 1-Click Installer"
echo "=========================================================="

if [ "$EUID" -ne 0 ]; then
    echo "[!] Please run as root (or use: sudo bash $0)"
    exit 1
fi

export DEBIAN_FRONTEND=noninteractive

echo "[1/7] Updating system packages..."
apt-get update -y && apt-get upgrade -y

echo "[2/7] Enabling 32-bit architecture & installing runtime libraries..."
dpkg --add-architecture i386
apt-get update -y
apt-get install -y \
    libc6:i386 \
    libstdc++6:i386 \
    libgcc-s1:i386 \
    libatomic1:i386 \
    sqlite3 \
    curl \
    wget \
    tar \
    unzip \
    git \
    ufw

echo "[3/7] Installing Node.js 20 LTS for Kingmakers Backend API..."
if ! command -v node >/dev/null 2>&1; then
    curl -fsSL https://deb.nodesource.com/setup_20.x | bash -
    apt-get install -y nodejs
fi
node -v
npm -v

echo "[4/7] Creating kingmakers dedicated service user..."
id -u kingmakers >/dev/null 2>&1 || useradd -m -s /bin/bash kingmakers

INSTALL_DIR="/home/kingmakers/kingmakers-rp"
echo "[5/7] Deploying Kingmakers RP to ${INSTALL_DIR}..."
mkdir -p "${INSTALL_DIR}"

# Copy files from repository/bundle into /home/kingmakers/kingmakers-rp
if [ -d "./server" ]; then
    cp -r ./server ./backend ./assets "${INSTALL_DIR}/"
fi
chown -R kingmakers:kingmakers /home/kingmakers

# Install backend dependencies
cd "${INSTALL_DIR}/backend"
sudo -u kingmakers npm install --production

echo "[6/7] Creating systemd background services..."

# Service 1: Game server (open.mp)
cat > /etc/systemd/system/km-server.service <<EOF
[Unit]
Description=Kingmakers RP Game Server (open.mp)
After=network.target

[Service]
Type=simple
User=kingmakers
WorkingDirectory=${INSTALL_DIR}/server
ExecStart=${INSTALL_DIR}/server/omp-server
Restart=always
RestartSec=5
LimitNOFILE=65535

[Install]
WantedBy=multi-user.target
EOF

# Service 2: Backend API & Web Portal
cat > /etc/systemd/system/km-backend.service <<EOF
[Unit]
Description=Kingmakers RP Backend API & Web Dashboard
After=network.target km-server.service

[Service]
Type=simple
User=kingmakers
WorkingDirectory=${INSTALL_DIR}/backend
Environment=PORT=8080
Environment=SAMP_HOST=127.0.0.1
Environment=SAMP_PORT=7777
Environment=DB_PATH=${INSTALL_DIR}/server/scriptfiles/kingmakers.db
ExecStart=/usr/bin/node server.js
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

systemctl daemon-reload
systemctl enable km-server km-backend
systemctl restart km-server km-backend

echo "[7/7] Configuring UFW firewall rules..."
ufw allow 22/tcp comment 'SSH'
ufw allow 7777/udp comment 'Kingmakers RP SA-MP / CRMP Game Port'
ufw allow 8080/tcp comment 'Kingmakers RP Web Dashboard & API'
ufw --force enable

PUBLIC_IP=$(curl -s https://api.ipify.org || echo "YOUR_VULTR_IP")

echo ""
echo "=========================================================="
echo "   KINGMAKERS RP DEPLOYMENT COMPLETE!"
echo "=========================================================="
echo "   Server Status:       systemctl status km-server"
echo "   Backend Status:      systemctl status km-backend"
echo "   Game Connection:     ${PUBLIC_IP}:7777 (UDP)"
echo "   Web Dashboard:       http://${PUBLIC_IP}:8080"
echo "   Database:            ${INSTALL_DIR}/server/scriptfiles/kingmakers.db"
echo "   Server Logs:         ${INSTALL_DIR}/server/log.txt"
echo "=========================================================="
