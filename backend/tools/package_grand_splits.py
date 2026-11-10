#!/usr/bin/env python3
"""
Kingmakers RP - Split APK Bundle Packager
Packages Android App Bundle (AAB) or extracted splits into Grand Mobile RP compatible split bundle:
- base.apk: Core application, DEX files, activities, resources
- split_config.arm64_v8a.apk: Native RenderWare GTA SA 2.10 64-bit engine and multiplayer binaries
- split_game.apk: Kingmakers content from the ":game" install-time asset pack
  (GUI, fonts, SAMP scripts/IDE, server config) - bundletool's game-master.apk
- split_config.xhdpi.apk: Screen density graphical resources
- split_config.en.apk: Language localization resources

Outputs:
1. Kingmakers_RP_Split_Bundle_v1.0.apks (Standard Google Bundletool APK Set)
2. Kingmakers_RP_Split_APKs_v1.0.zip / apks.zip (Grand Mobile RP Split APK archive)
"""

import os
import sys
import shutil
import zipfile
import subprocess
import glob

def package_splits(splits_dir, output_dir, zip_name="Kingmakers_RP_Split_APKs_v1.0.zip"):
    os.makedirs(output_dir, exist_ok=True)
    if os.path.dirname(zip_name):
        out_zip_path = os.path.abspath(zip_name)
    else:
        out_zip_path = os.path.join(output_dir, zip_name)
    os.makedirs(os.path.dirname(out_zip_path), exist_ok=True)
    apks_zip_alias = os.path.join(os.path.dirname(out_zip_path), "apks.zip")

    print("==========================================================")
    print("  PACKAGING GRAND MOBILE STYLE SPLIT APK BUNDLE")
    print("==========================================================")
    print(f"  Source Splits Dir: {splits_dir}")
    print(f"  Output Dir:        {output_dir}")

    # Search for all apk files in splits_dir recursively
    apk_files = glob.glob(os.path.join(splits_dir, "**", "*.apk"), recursive=True)
    if not apk_files:
        print(f"Error: No APK files found in {splits_dir}")
        sys.exit(1)

    print(f"Found {len(apk_files)} split candidate APKs:")
    for f in apk_files:
        print(f"  - {os.path.basename(f)} ({os.path.getsize(f) / (1024*1024):.2f} MB)")

    # Mapping to the Grand Mobile RP naming convention, by exact file name.
    # bundletool's default mode emits one set of splits per variant: the first
    # variant has no suffix (base-master.apk), later ones add _2, _3, ...
    # (base-master_2.apk). Only the suffix-less names are taken, so every split
    # comes from the same variant and the result does not depend on glob order.
    # Variant 1 targets the lowest SDK, so it installs on every newer device too.
    by_name = {}
    for f in apk_files:
        by_name.setdefault(os.path.basename(f).lower(), f)

    candidates = {
        "base.apk": ["base-master.apk", "base.apk"],
        "split_config.arm64_v8a.apk": ["base-arm64_v8a.apk", "split_config.arm64_v8a.apk"],
        "split_config.xhdpi.apk": ["base-xhdpi.apk", "split_config.xhdpi.apk"],
        "split_config.en.apk": ["base-en.apk", "split_config.en.apk"],
        "split_game.apk": ["game-master.apk", "split_game.apk"],
    }
    final_splits = {}
    for target_name, names in candidates.items():
        for n in names:
            if n in by_name:
                final_splits[target_name] = by_name[n]
                break

    missing = [n for n in ("base.apk", "split_config.arm64_v8a.apk", "split_game.apk") if n not in final_splits]
    if missing:
        print(f"Error: required splits not found: {', '.join(missing)}")
        sys.exit(1)

    print("\nResolved Split Mapping (Grand Mobile RP Schema):")
    for target_name, src_path in final_splits.items():
        dst_copy = os.path.join(output_dir, target_name)
        shutil.copy2(src_path, dst_copy)
        print(f"  -> {target_name:30s} <= {os.path.basename(src_path)} ({os.path.getsize(dst_copy) / (1024*1024):.2f} MB)")

    # Create the zip bundle (Grand Mobile RP apks.zip / Kingmakers_RP_Split_APKs_v1.0.zip)
    print(f"\nCreating Split APK Archive: {out_zip_path}...")
    with zipfile.ZipFile(out_zip_path, 'w', zipfile.ZIP_DEFLATED) as zf:
        for target_name in final_splits.keys():
            full_p = os.path.join(output_dir, target_name)
            zf.write(full_p, target_name)

    shutil.copy2(out_zip_path, apks_zip_alias)

    zip_size = os.path.getsize(out_zip_path)
    print("==========================================================")
    print("  SPLIT APK BUNDLE GENERATED SUCCESSFULLY!")
    print(f"  Archive:     {out_zip_path} ({zip_size / (1024*1024):.2f} MB)")
    print(f"  Alias:       {apks_zip_alias}")
    print(f"  Split Count: {len(final_splits)} APKs")
    print("==========================================================")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: package_grand_splits.py <splits_dir> <output_dir> [zip_name]")
        sys.exit(1)
    zname = sys.argv[3] if len(sys.argv) > 3 else "Kingmakers_RP_Split_APKs_v1.0.zip"
    package_splits(sys.argv[1], sys.argv[2], zname)
