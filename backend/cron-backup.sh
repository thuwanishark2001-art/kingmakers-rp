#!/bin/bash
# Kingmakers RP - Automated Hourly Database Backup Script
BACKUP_DIR="/home/kingmakers/backups"
DB_FILE="/home/kingmakers/kingmakers-rp/server/scriptfiles/kingmakers.db"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")

mkdir -p "$BACKUP_DIR"
if [ -f "$DB_FILE" ]; then
    sqlite3 "$DB_FILE" ".backup '$BACKUP_DIR/kingmakers_$TIMESTAMP.db'"
    echo "[$(date)] Backup completed: $BACKUP_DIR/kingmakers_$TIMESTAMP.db"
    # Keep only the last 48 backups (2 days of hourly backups)
    find "$BACKUP_DIR" -type f -name "kingmakers_*.db" -mtime +2 -delete
fi
