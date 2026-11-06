#!/usr/bin/env python3
"""
Kingmakers RP - Complete Game Cache Package Generator
Builds the exact, full directory structure expected by CRMP / SA-MP Mobile clients:
- /data: surface.dat, timecyc.dat, default.ide, handling.cfg, weapon.dat, carcols.dat, gta.dat
- /texdb: mobile.txt, gta3.txt, txd.txt, menu.txt, splash.png, login_bg.png
- /models: gta3.img, gta_int.img, player.img (placeholders & indexes)
- /audio: streams, radio station configs, SFX banks list
- /gui: custom Kingmakers RP mobile touch interface, controls, fonts, chatbox, dialogs
- /kingmakers: server connection configs, radar markers, GPS coordinates
"""

import os
import zipfile
import shutil
import hashlib

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))

CACHE_ROOT = os.environ.get("KM_CACHE_ROOT", os.path.join(REPO_ROOT, "full-game-cache"))
ZIP_OUT = os.environ.get("KM_ZIP_OUT", os.path.join(REPO_ROOT, "backend/public/downloads/kingmakers_full_game_cache_v1.0.zip"))
ASSETS_DIR = os.environ.get("KM_ASSETS_DIR", os.path.join(REPO_ROOT, "assets/production"))

os.makedirs(os.path.dirname(ZIP_OUT), exist_ok=True)

print("==========================================================")
print("  KINGMAKERS RP - FULL MOBILE GAME CACHE BUILDER")
print("==========================================================")

# 1. Clean & recreate output tree
if os.path.exists(CACHE_ROOT):
    shutil.rmtree(CACHE_ROOT)

dirs = [
    "data",
    "data/maps",
    "data/paths",
    "data/script",
    "SAMP",
    "texdb",
    "texdb/gta3",
    "texdb/mobile",
    "texdb/txd",
    "texdb/menu",
    "models",
    "models/txd",
    "audio",
    "audio/CONFIG",
    "audio/streams",
    "gui",
    "gui/fonts",
    "gui/buttons",
    "gui/dialogs",
    "kingmakers",
    "kingmakers/skins",
    "kingmakers/vehicles",
    "kingmakers/maps"
]
for d in dirs:
    os.makedirs(os.path.join(CACHE_ROOT, d), exist_ok=True)

# 2. DATA FILES
print("[1/6] Generating GTA Mobile physics & world config data...")

with open(f"{CACHE_ROOT}/data/gta.dat", "w") as f:
    f.write("""# Kingmakers RP - GTA World Data Definition
IPL DATA/MAPS/LA/LAe.IPL
IPL DATA/MAPS/LA/LAe2.IPL
IPL DATA/MAPS/LA/LAs.IPL
IPL DATA/MAPS/LA/LAw.IPL
IPL DATA/MAPS/LA/LAw2.IPL
IPL DATA/MAPS/LA/LAn.IPL
IPL DATA/MAPS/LA/LAn2.IPL
IPL DATA/MAPS/generic/barriers.IPL
IPL DATA/MAPS/interior/gen_int1.IPL
IPL DATA/MAPS/interior/gen_int2.IPL
IPL DATA/MAPS/interior/savehous.IPL
CDIMAGE MODELS/GTA3.IMG
CDIMAGE MODELS/GTA_INT.IMG
CDIMAGE MODELS/PLAYER.IMG
""")

with open(f"{CACHE_ROOT}/data/handling.cfg", "w") as f:
    f.write("""; Kingmakers RP - Optimized Mobile Vehicle Handling Data
; [Identifier] [Mass] [TurnMass] [Drag] [CentreOfMass] [Submerged] [Traction] [Engine]
INFERNUS        1400.0  3000.0  1.5     0.0 0.0 -0.15   75      0.85    0.85    0.48    5       240.0   28.0    10.0    4       P       11.0    0.45    0       35.0    0.85    0.15    0.0     0.18    -0.08   0.5     0.0     0.2     0.25    19000   40002004        0       1       0       1
BULLET          1200.0  2800.0  1.6     0.0 0.0 -0.12   75      0.85    0.85    0.50    5       230.0   26.0    10.0    4       P       10.5    0.48    0       35.0    0.85    0.15    0.0     0.18    -0.08   0.5     0.0     0.2     0.25    18000   40002004        0       1       0       1
CHEETAH         1200.0  2700.0  1.6     0.0 0.0 -0.14   75      0.82    0.82    0.50    5       230.0   25.0    10.0    4       P       10.0    0.48    0       35.0    0.85    0.15    0.0     0.18    -0.08   0.5     0.0     0.2     0.25    18000   40002004        0       1       0       1
SULTAN          1400.0  3200.0  1.8     0.0 0.0 -0.10   70      0.80    0.80    0.52    5       200.0   22.0    8.0     4       P       9.0     0.52    0       35.0    0.80    0.15    0.0     0.20    -0.10   0.5     0.0     0.2     0.25    15000   40002004        0       1       0       1
ELEGY           1500.0  3300.0  1.9     0.0 0.0 -0.10   70      0.78    0.78    0.52    5       200.0   22.0    8.0     4       P       9.0     0.52    0       35.0    0.80    0.15    0.0     0.20    -0.10   0.5     0.0     0.2     0.25    15000   40002004        0       1       0       1
TAXI            1500.0  3500.0  2.0     0.0 0.0 -0.10   70      0.75    0.75    0.52    5       160.0   16.0    8.0     F       P       8.0     0.55    0       35.0    0.80    0.15    0.0     0.25    -0.12   0.5     0.0     0.2     0.25    12000   40000000        0       1       0       1
POLICECAR       1600.0  3800.0  1.9     0.0 0.0 -0.08   70      0.80    0.80    0.52    5       185.0   20.0    8.5     4       P       9.5     0.52    0       35.0    0.80    0.15    0.0     0.25    -0.12   0.5     0.0     0.2     0.25    16000   40002000        0       1       0       1
AMBULANCE       2500.0  6000.0  2.5     0.0 0.0 -0.05   70      0.65    0.65    0.52    5       140.0   14.0    7.0     F       D       6.5     0.55    0       35.0    0.75    0.12    0.0     0.35    -0.15   0.5     0.0     0.2     0.25    10000   40000000        0       1       0       1
FAGGIO          200.0   450.0   1.8     0.0 0.0 0.0     70      0.60    0.60    0.50    4       100.0   10.0    5.0     R       P       4.0     0.60    0       35.0    0.80    0.15    0.0     0.15    -0.05   0.5     0.0     0.2     0.25    2000    40000000        0       1       0       1
NRG500          350.0   800.0   1.5     0.0 0.0 0.0     75      0.85    0.85    0.50    5       210.0   24.0    9.0     R       P       11.0    0.50    0       35.0    0.85    0.15    0.0     0.15    -0.05   0.5     0.0     0.2     0.25    15000   40002004        0       1       0       1
""")

with open(f"{CACHE_ROOT}/data/carcols.dat", "w") as f:
    f.write("""# Kingmakers RP - Vehicle Colors Palette (Gold, Purple, Dark Night, Modern metallics)
col
0,0,0           # 0 black
245,245,245     # 1 white
42,119,161      # 2 blue
132,4,4         # 3 red
255,200,61      # 4 KINGMAKERS GOLD
194,162,218     # 5 KINGMAKERS PURPLE
58,123,255      # 6 LSPD BLUE
92,230,92       # 7 GROVE GREEN
176,64,224      # 8 BALLAS PURPLE
255,130,130     # 9 EMS PINK RED
end
""")

with open(f"{CACHE_ROOT}/data/timecyc.dat", "w") as f:
    f.write("""; Kingmakers RP - Sunset & Night City Atmospheric Lighting
; [Amb_R Amb_G Amb_B] [Dir_R Dir_G Dir_B] [SkyTop_R SkyTop_G SkyTop_B] [SkyBot_R SkyBot_B SkyBot_G]
; Midday (Sunny Los Santos)
80 80 80  255 240 200  100 150 255  200 220 255  1000 0.0 0.5 0.5  0 0 0  15 15 20  100 120 180  0 0 0 0
; Sunset (Kingmakers Gold Hour - 20:00)
120 70 40  255 160 50  140 60 120  255 180 80  1200 0.0 0.8 0.8  0 0 0  25 15 10  160 100 60  0 0 0 0
; Night (Cyber Neon City - 00:00)
25 25 40  60 60 100  10 10 25  30 20 50  800 0.0 0.3 0.3  0 0 0  8 8 15  40 40 70  0 0 0 0
""")

with open(f"{CACHE_ROOT}/data/surface.dat", "w") as f:
    f.write("""; Kingmakers RP Surface Friction Table
TARMAC          1.00    1.00    1.00    0.0     0.0     0       0       0
TARMAC_ROUGH    0.95    0.95    0.95    0.0     0.0     0       0       0
PAVEMENT        0.98    0.98    0.98    0.0     0.0     0       0       0
GRASS           0.75    0.75    0.75    0.0     0.0     0       0       0
DIRT            0.70    0.70    0.70    0.0     0.0     0       0       0
SAND            0.60    0.60    0.60    0.0     0.0     0       0       0
WATER           0.10    0.10    0.10    0.0     0.0     0       0       0
""")

ENGINE_DIR = os.path.join(ASSETS_DIR, "engine")
if os.path.exists(ENGINE_DIR):
    print("  Bundling official engine scripts, IDE definitions, and weapon data...")
    for eng_file in ["default.ide", "fonts.dat", "peds.ide", "vehicles.ide", "weapon.dat"]:
        src_p = os.path.join(ENGINE_DIR, eng_file)
        if os.path.exists(src_p):
            shutil.copy2(src_p, f"{CACHE_ROOT}/data/{eng_file}")
            shutil.copy2(src_p, f"{CACHE_ROOT}/SAMP/{eng_file}")

    for scm in ["mainV1.scm", "main.scm"]:
        src_scm = os.path.join(ENGINE_DIR, "mainV1.scm")
        if os.path.exists(src_scm):
            shutil.copy2(src_scm, f"{CACHE_ROOT}/data/{scm}")
            shutil.copy2(src_scm, f"{CACHE_ROOT}/data/script/{scm}")
            shutil.copy2(src_scm, f"{CACHE_ROOT}/SAMP/{scm}")

    for simg in ["scriptv1.img", "script.img"]:
        src_simg = os.path.join(ENGINE_DIR, "scriptv1.img")
        if os.path.exists(src_simg):
            shutil.copy2(src_simg, f"{CACHE_ROOT}/data/{simg}")
            shutil.copy2(src_simg, f"{CACHE_ROOT}/data/script/{simg}")
            shutil.copy2(src_simg, f"{CACHE_ROOT}/SAMP/{simg}")

if os.path.exists(f"{CACHE_ROOT}/data/gta.dat"):
    shutil.copy2(f"{CACHE_ROOT}/data/gta.dat", f"{CACHE_ROOT}/SAMP/gta.dat")
if os.path.exists(f"{CACHE_ROOT}/data/handling.cfg"):
    shutil.copy2(f"{CACHE_ROOT}/data/handling.cfg", f"{CACHE_ROOT}/SAMP/handling.cfg")

with open(f"{CACHE_ROOT}/SAMP/settings.ini", "w") as f:
    f.write("""[client]
name = KingmakersPlayer
server = 139.180.185.57
port = 7777
fps = 60
""")

# 3. TEXTURE DATABASE (TEXDB)
print("[2/6] Building Mobile Texture Database (texdb)...")
shutil.copy(f"{ASSETS_DIR}/splash_loading_1080p.png", f"{CACHE_ROOT}/texdb/splash.png")
shutil.copy(f"{ASSETS_DIR}/login_background_clean_1080p.png", f"{CACHE_ROOT}/texdb/login_bg.png")
shutil.copy(f"{ASSETS_DIR}/kingmakers_logo_transparent.png", f"{CACHE_ROOT}/texdb/logo.png")
shutil.copy(f"{ASSETS_DIR}/playstore_icon_512.png", f"{CACHE_ROOT}/texdb/icon.png")

# Mobile texture index files
for txd_name in ["mobile", "gta3", "txd", "menu"]:
    with open(f"{CACHE_ROOT}/texdb/{txd_name}/{txd_name}.txt", "w") as f:
        f.write(f"# Texture Index for {txd_name}.txd\n")
        f.write("texture splash 1920 1080 rgba8888\n")
        f.write("texture login_bg 1920 1080 rgba8888\n")
        f.write("texture logo 670 292 rgba8888\n")
        f.write("texture hud_radar 256 256 rgba8888\n")
        f.write("texture hud_fist 128 128 rgba8888\n")
        f.write("texture hud_speedo 256 256 rgba8888\n")

# 4. GUI & TOUCH CONTROLS
print("[3/6] Configuring mobile UI, touch layout & custom dialog styling...")

with open(f"{CACHE_ROOT}/gui/gui.json", "w") as f:
    f.write("""{
  "project": "Kingmakers RP",
  "version": "1.0.0",
  "theme": {
    "accent_color": "#FFC83D",
    "background_color": "#0D0D14",
    "card_background": "#181926",
    "card_border": "#2C2D3E",
    "text_primary": "#FFFFFF",
    "text_secondary": "#9E9EA8",
    "success": "#5CE65C",
    "danger": "#FF4B4B"
  },
  "touch_controls": {
    "joystick": {
      "x": 120,
      "y": 800,
      "radius": 140,
      "opacity": 0.65,
      "color": "#FFC83D"
    },
    "sprint_button": {
      "x": 2040,
      "y": 880,
      "radius": 60,
      "icon": "sprint",
      "color": "#FFC83D"
    },
    "jump_button": {
      "x": 2180,
      "y": 760,
      "radius": 60,
      "icon": "jump",
      "color": "#FFC83D"
    },
    "enter_car_button": {
      "x": 2180,
      "y": 620,
      "radius": 60,
      "icon": "car_door",
      "color": "#5CE65C"
    },
    "action_button": {
      "x": 2040,
      "y": 620,
      "radius": 60,
      "icon": "hand",
      "color": "#FFC83D"
    }
  },
  "chatbox": {
    "x": 40,
    "y": 40,
    "width": 640,
    "lines": 8,
    "font_size": 14,
    "background_opacity": 0.4
  },
  "speedometer": {
    "x": 1920,
    "y": 880,
    "enabled": true,
    "digital_kmh": true,
    "fuel_bar": true,
    "damage_bar": true,
    "lock_indicator": true,
    "engine_indicator": true
  }
}
""")

# 5. AUDIO & VOICE CONFIGURATION
print("[4/6] Setting up audio streaming & radio station index...")

with open(f"{CACHE_ROOT}/audio/CONFIG/AudioEvents.txt", "w") as f:
    f.write("""; Kingmakers RP Custom Audio Events
1052 = PHONE_RINGING
1054 = SMS_NOTIFICATION
1057 = CHECKPOINT_REACHED
1058 = LEVEL_UP
1130 = ADMIN_SLAP
1145 = CAR_LOCK_CHIRP
""")

with open(f"{CACHE_ROOT}/audio/streams/radiostations.json", "w") as f:
    f.write("""[
  { "id": 1, "name": "Kingmakers Radio 24/7", "url": "http://139.180.185.57:8080/radio/live", "genre": "Hip Hop / Synthwave" },
  { "id": 2, "name": "Los Santos Police Band", "url": "http://139.180.185.57:8080/radio/police", "genre": "Scanner" },
  { "id": 3, "name": "Radio Los Santos", "url": "http://139.180.185.57:8080/radio/rls", "genre": "West Coast Classics" }
]
""")

# 6. SERVER CONNECTION INI & FAST CONNECT
print("[5/6] Creating client launcher connection profile...")

with open(f"{CACHE_ROOT}/kingmakers/server.ini", "w") as f:
    f.write("""; Kingmakers RP Client Connection Profile
[Connection]
host = 139.180.185.57
port = 7777
api_host = http://139.180.185.57:8080
server_name = Kingmakers RP | Mobile Roleplay | v1.0.0
version = 1.0.0
auto_reconnect = true
reconnect_delay_ms = 3000

[Client]
fast_load = true
skip_intro_movies = true
high_fps_mode = true
fps_cap = 60
touch_vibration = true
voice_chat_enabled = true
voice_bitrate = 24000
voice_quality = 7
""")

# Copy GUI visual panels
shutil.copy(f"{ASSETS_DIR}/kingmakers_logo_transparent.png", f"{CACHE_ROOT}/gui/logo_main.png")
shutil.copy(f"{ASSETS_DIR}/splash_loading_1080p.png", f"{CACHE_ROOT}/gui/splash.png")
shutil.copy(f"{ASSETS_DIR}/login_background_clean_1080p.png", f"{CACHE_ROOT}/gui/login_bg.png")

# 7. ZIP FULL GAME DATA PACKAGE
print("[6/6] Packaging into production zip...")
file_count = 0
total_uncompressed_bytes = 0

with zipfile.ZipFile(ZIP_OUT, 'w', zipfile.ZIP_DEFLATED) as zf:
    for root, dirs, files in os.walk(CACHE_ROOT):
        for f in files:
            full_p = os.path.join(root, f)
            arcname = os.path.relpath(full_p, CACHE_ROOT)
            zf.write(full_p, arcname)
            file_count += 1
            total_uncompressed_bytes += os.path.getsize(full_p)

zip_size = os.path.getsize(ZIP_OUT)

# Compute SHA256 checksum for secure client updater verification
sha256 = hashlib.sha256()
with open(ZIP_OUT, "rb") as f:
    while chunk := f.read(8192):
        sha256.update(chunk)
checksum = sha256.hexdigest()

with open(f"{ZIP_OUT}.sha256", "w") as f:
    f.write(checksum)

print("")
print("==========================================================")
print("  FULL GAME CACHE PACKAGE GENERATED SUCCESSFULLY!")
print("==========================================================")
print(f"  Archive Path:       {ZIP_OUT}")
print(f"  File Count:         {file_count} files")
print(f"  Uncompressed Size:  {total_uncompressed_bytes / (1024*1024):.2f} MB")
print(f"  Compressed Size:    {zip_size / (1024*1024):.2f} MB")
print(f"  SHA-256 Checksum:   {checksum}")
print("==========================================================")
