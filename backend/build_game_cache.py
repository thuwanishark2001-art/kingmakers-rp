#!/usr/bin/env python3
"""
Kingmakers RP - Game Cache Builder
Generates a complete, structured game data package (textures, audio configs,
custom models, mobile UI elements, and server settings) into a single zip
suitable for direct mobile distribution or CDN cache downloads.
"""

import os
import zipfile
import shutil
from datetime import datetime

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))

CACHE_DIR = os.environ.get("KM_GAME_CACHE_DIR", os.path.join(REPO_ROOT, "game-cache"))
ZIP_OUT = os.environ.get("KM_GAME_ZIP_OUT", os.path.join(REPO_ROOT, "backend/public/downloads/kingmakers_game_cache_v1.0.zip"))
ASSETS_DIR = os.environ.get("KM_ASSETS_DIR", os.path.join(REPO_ROOT, "assets/production"))

print("=== Building Kingmakers RP Complete Game Data Package ===")

os.makedirs(f"{CACHE_DIR}/texdb", exist_ok=True)
os.makedirs(f"{CACHE_DIR}/models", exist_ok=True)
os.makedirs(f"{CACHE_DIR}/audio/streams", exist_ok=True)
os.makedirs(f"{CACHE_DIR}/gui", exist_ok=True)
os.makedirs(os.path.dirname(ZIP_OUT), exist_ok=True)

# 1. Texture Database Assets (GTA SA Mobile texdb layout)
# Splash and loading screen textures
shutil.copy(f"{ASSETS_DIR}/splash_loading_1080p.png", f"{CACHE_DIR}/texdb/splash.png")
shutil.copy(f"{ASSETS_DIR}/login_background_clean_1080p.png", f"{CACHE_DIR}/texdb/login_bg.png")
shutil.copy(f"{ASSETS_DIR}/kingmakers_logo_transparent.png", f"{CACHE_DIR}/gui/logo_main.png")

# 2. Mobile GUI configuration (CRMP Mobile HUD & Dialog definition)
with open(f"{CACHE_DIR}/gui/gui.json", "w") as f:
    f.write("""{
  "theme": "kingmakers_gold",
  "colors": {
    "primary": "#FFC83D",
    "primary_dark": "#D49B1F",
    "background": "#0D0D14",
    "card": "#181926",
    "card_border": "#2C2D3E",
    "text": "#FFFFFF",
    "muted": "#9E9EA8",
    "success": "#5CE65C",
    "danger": "#FF4B4B"
  },
  "hud": {
    "speedometer": {
      "enabled": true,
      "position": "bottom_right",
      "show_fuel": true,
      "show_engine_status": true,
      "show_doors_status": true
    },
    "radar": {
      "shape": "rounded_rect",
      "border_color": "#FFC83D"
    }
  }
}
""")

# 3. Client Configuration
with open(f"{CACHE_DIR}/kingmakers_config.ini", "w") as f:
    f.write(f"""; Kingmakers RP Mobile Game Data Package Configuration
[Server]
ip = 139.180.185.57
port = 7777
name = Kingmakers RP | Mobile Roleplay | v1.0.0
version = 1.0.0
timestamp = {datetime.utcnow().isoformat()}

[Graphics]
fps_limit = 60
draw_distance = 1.2
post_processing = high
shadows = medium

[Audio]
radio_stream_url = http://139.180.185.57:8080/radio/live
voice_chat = true
""")

# 4. Create ZIP package
with zipfile.ZipFile(ZIP_OUT, 'w', zipfile.ZIP_DEFLATED) as zf:
    for root, dirs, files in os.walk(CACHE_DIR):
        for f in files:
            path = os.path.join(root, f)
            arcname = os.path.relpath(path, CACHE_DIR)
            zf.write(path, arcname)

print("Game Data Package created successfully:")
print("Path:", ZIP_OUT)
print("Size:", os.path.getsize(ZIP_OUT), "bytes")
