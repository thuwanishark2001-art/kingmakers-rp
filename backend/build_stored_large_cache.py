#!/usr/bin/env python3
"""
Builds an uncompressed (ZIP_STORED) 1.5 GB package so the zip file itself
measures ~1.5 GB on disk, matching standard full mobile OBB distributions.
"""

import os
import zipfile
import hashlib

BUILD_DIR = "/home/antigravity/kingmakers-rp/large-game-cache"
ZIP_OUT = "/home/antigravity/kingmakers-rp/backend/public/downloads/kingmakers_full_game_cache_1.5gb_uncompressed.zip"

print("Packaging 1.5GB game archive with ZIP_STORED (fast mobile extraction)...")
file_count = 0
with zipfile.ZipFile(ZIP_OUT, 'w', zipfile.ZIP_STORED) as zf:
    for root, dirs, files in os.walk(BUILD_DIR):
        for f in files:
            full_p = os.path.join(root, f)
            arcname = os.path.relpath(full_p, BUILD_DIR)
            zf.write(full_p, arcname)
            file_count += 1

size = os.path.getsize(ZIP_OUT)
print(f"File created: {ZIP_OUT}")
print(f"Actual Zip File Size on disk: {size / (1024*1024*1024):.2f} GB ({size / (1024*1024):.1f} MB)")
