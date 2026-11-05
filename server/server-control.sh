#!/bin/bash
# Kingmakers RP - Server Control CLI Tool
# Usage: ./server-control.sh [status|restart|stop|start|logs|backup]

case "$1" in
    status)
        echo "=== Kingmakers RP Service Status ==="
        systemctl status km-server --no-pager
        echo ""
        systemctl status km-backend --no-pager
        ;;
    restart)
        echo "Restarting Kingmakers RP Game Server & Backend..."
        systemctl restart km-server km-backend
        echo "Done."
        ;;
    stop)
        echo "Stopping Kingmakers RP..."
        systemctl stop km-server km-backend
        echo "Stopped."
        ;;
    start)
        echo "Starting Kingmakers RP..."
        systemctl start km-server km-backend
        echo "Started."
        ;;
    logs)
        echo "Streaming live server logs (Ctrl+C to stop)..."
        journalctl -u km-server -f
        ;;
    backup)
        echo "Running manual database backup..."
        /home/kingmakers/kingmakers-rp/backend/cron-backup.sh
        ;;
    *)
        echo "Kingmakers RP Management CLI"
        echo "Usage: $0 {status|restart|stop|start|logs|backup}"
        exit 1
        ;;
esac
