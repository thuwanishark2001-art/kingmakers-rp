#!/usr/bin/env python3
"""
Builds an uncompressed (ZIP_STORED) 1.5 GB package so the zip file itself
measures ~1.5 GB on disk, matching standard full mobile OBB distributions.
"""

import os
import zipfile
import hashlib

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))

BUILD_DIR = os.environ.get("KM_BUILD_DIR", os.path.join(REPO_ROOT, "large-game-cache"))
ZIP_OUT = os.environ.get("KM_UNCOMPRESSED_ZIP_OUT", os.path.join(REPO_ROOT, "backend/public/downloads/kingmakers_full_game_cache_1.5gb_uncompressed.zip"))

os.makedirs(os.path.dirname(ZIP_OUT), exist_ok=True)

# Ensure large cache files (IMG models) exist
if not os.path.exists(os.path.join(BUILD_DIR, "models/gta3.img")):
    print("Large game cache models not found. Running build_large_cache.py first...")
    import subprocess
    subprocess.run(["python3", os.path.join(SCRIPT_DIR, "build_large_cache.py")], check=True)

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

# Calculate SHA256 Checksum
print("Generating SHA-256 checksum...")
sha256 = hashlib.sha256()
with open(ZIP_OUT, "rb") as f:
    while chunk := f.read(1024 * 1024):
        sha256.update(chunk)
checksum = sha256.hexdigest()

with open(f"{ZIP_OUT}.sha256", "w") as f:
    f.write(checksum)

print(f"SHA256 Checksum: {checksum}")
print("Build complete!")
