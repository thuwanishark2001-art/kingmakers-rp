#!/usr/bin/env python3
"""
Kingmakers RP - Content Pack Builder

Packages ONLY Kingmakers RP's own client files (SA-MP scripts and IDEs, GUI,
fonts, connection configs, radio list) into the zip the launcher downloads.

It deliberately contains no GTA: San Andreas engine data (models/, anim/,
texdb/, data/, audio SFX). Players import that from their own copy of the
game in the launcher, and the client refuses to extract those folders from
this pack anyway.
"""

import hashlib
import os
import sys
import zipfile

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))

SOURCE_DIR = os.environ.get(
    "KM_CONTENT_SRC", os.path.join(REPO_ROOT, "android-client/app/src/main/assets/game"))
ZIP_OUT = os.environ.get(
    "KM_CONTENT_ZIP_OUT", os.path.join(REPO_ROOT, "backend/public/downloads/kingmakers_content_pack_v1.0.zip"))

# Must match GameDataManager.OWN_CONTENT_DIRS in the Android client.
OWN_CONTENT_DIRS = ("SAMP/", "gui/", "fonts/", "kingmakers/", "audio/streams/")


def is_own_content(rel_path: str) -> bool:
    p = rel_path.replace("\\", "/").lower()
    return any(p.startswith(prefix.lower()) for prefix in OWN_CONTENT_DIRS)


def main() -> int:
    if not os.path.isdir(SOURCE_DIR):
        print(f"Source directory not found: {SOURCE_DIR}", file=sys.stderr)
        return 1

    os.makedirs(os.path.dirname(ZIP_OUT), exist_ok=True)

    files, skipped = [], []
    for root, _dirs, names in os.walk(SOURCE_DIR):
        for name in names:
            full = os.path.join(root, name)
            rel = os.path.relpath(full, SOURCE_DIR).replace(os.sep, "/")
            (files if is_own_content(rel) else skipped).append((full, rel))

    with zipfile.ZipFile(ZIP_OUT, "w", zipfile.ZIP_DEFLATED) as zf:
        for full, rel in sorted(files, key=lambda x: x[1]):
            zf.write(full, rel)

    sha = hashlib.sha256()
    with open(ZIP_OUT, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 16), b""):
            sha.update(chunk)
    with open(ZIP_OUT + ".sha256", "w") as f:
        f.write(sha.hexdigest())

    print(f"Content pack: {ZIP_OUT}")
    print(f"  files:  {len(files)}")
    print(f"  size:   {os.path.getsize(ZIP_OUT) / (1024 * 1024):.2f} MB")
    print(f"  sha256: {sha.hexdigest()}")
    for _full, rel in skipped:
        print(f"  skipped (not Kingmakers content): {rel}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
