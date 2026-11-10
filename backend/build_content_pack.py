#!/usr/bin/env python3
"""Build a deterministic Kingmakers-owned content ZIP and integrity manifests.

This packages only client-owned files under SAMP/, gui/, fonts/, kingmakers/,
and audio/streams/. It deliberately excludes Rockstar game data (models, anim,
texdb, data, and audio SFX); users must import those from their own installation.

Environment overrides:
  KM_CONTENT_SRC      source directory (defaults to Android app game assets)
  KM_CONTENT_ZIP_OUT  output ZIP path
  KM_CONTENT_VERSION  content version recorded in the embedded manifest
"""

from __future__ import annotations

import hashlib
import json
import os
import stat
import sys
import tempfile
import zipfile
from pathlib import Path, PurePosixPath
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT = SCRIPT_DIR.parent
SOURCE_DIR = Path(os.environ.get(
    "KM_CONTENT_SRC",
    str(REPO_ROOT / "android-client/app/src/main/assets/game"),
))
ZIP_OUT = Path(os.environ.get(
    "KM_CONTENT_ZIP_OUT",
    str(REPO_ROOT / "backend/public/downloads/kingmakers_content_pack_v1.0.zip"),
))
VERSION = os.environ.get("KM_CONTENT_VERSION", "1.0")

# Keep in sync with GameDataManager.OWN_CONTENT_DIRS. These are client assets,
# not the base game's copyrighted engine data.
OWN_CONTENT_DIRS = ("SAMP/", "gui/", "fonts/", "kingmakers/", "audio/streams/")
MANIFEST_PATH = "kingmakers/content-manifest.json"
CHUNK_SIZE = 1024 * 1024
ZIP_EPOCH = (1980, 1, 1, 0, 0, 0)  # ZIP's minimum supported timestamp.


def owned_path(rel_path: str) -> bool:
    normalized = rel_path.replace("\\", "/").casefold()
    return any(normalized.startswith(prefix.casefold()) for prefix in OWN_CONTENT_DIRS)


def safe_archive_path(rel_path: str) -> str:
    """Validate and return a normalized, relative ZIP path."""
    normalized = rel_path.replace("\\", "/")
    path = PurePosixPath(normalized)
    if path.is_absolute() or not normalized or any(p in ("", ".", "..") for p in normalized.split("/")):
        raise ValueError(f"unsafe archive path: {rel_path!r}")
    return path.as_posix()


def file_digests(path: Path) -> tuple[str, str, int]:
    sha256 = hashlib.sha256()
    md5 = hashlib.md5()  # Per-file checkpoint for launchers; package integrity uses SHA-256.
    size = 0
    with path.open("rb") as stream:
        while True:
            block = stream.read(CHUNK_SIZE)
            if not block:
                break
            size += len(block)
            sha256.update(block)
            md5.update(block)
    return sha256.hexdigest(), md5.hexdigest(), size


def collect_files(source: Path) -> tuple[list[tuple[Path, str]], list[str]]:
    if not source.is_dir():
        raise FileNotFoundError(f"Source directory not found: {source}")

    included: list[tuple[Path, str]] = []
    skipped: list[str] = []
    seen: dict[str, str] = {}

    # Walk without following symlinks, and keep directory traversal deterministic.
    for current, dirs, names in os.walk(source, followlinks=False):
        current_path = Path(current)
        dirs[:] = sorted(
            [name for name in dirs if not (current_path / name).is_symlink()],
            key=str.casefold,
        )
        for name in sorted(names, key=str.casefold):
            full_path = current_path / name
            rel_path = full_path.relative_to(source).as_posix()
            if full_path.is_symlink() or not full_path.is_file():
                skipped.append(f"{rel_path} (symlink or non-regular file)")
                continue
            if not owned_path(rel_path):
                skipped.append(rel_path)
                continue
            if rel_path.casefold() == MANIFEST_PATH.casefold():
                # Always regenerate the embedded manifest from the exact payload.
                continue
            archive_path = safe_archive_path(rel_path)
            folded = archive_path.casefold()
            if folded in seen:
                raise ValueError(f"case-insensitive path collision: {seen[folded]} and {archive_path}")
            seen[folded] = archive_path
            included.append((full_path, archive_path))

    included.sort(key=lambda item: item[1].casefold())
    if not included:
        raise ValueError(f"No Kingmakers-owned content found under {source}")
    return included, skipped


def zip_info(archive_path: str, mode: int = 0o644) -> zipfile.ZipInfo:
    info = zipfile.ZipInfo(archive_path, date_time=ZIP_EPOCH)
    info.compress_type = zipfile.ZIP_DEFLATED
    info.compresslevel = 6
    info.create_system = 3
    info.external_attr = (stat.S_IFREG | mode) << 16
    return info


def build(source: Path, output: Path, version: str) -> dict[str, Any]:
    files, skipped = collect_files(source)
    entries: list[dict[str, Any]] = []
    for full_path, archive_path in files:
        sha256, md5, size = file_digests(full_path)
        entries.append({
            "path": archive_path,
            "size": size,
            "sha256": sha256,
            "md5": md5,
        })

    manifest: dict[str, Any] = {
        "format": "kingmakers-content-pack",
        "format_version": 1,
        "content_version": version,
        "hash_algorithms": ["sha256", "md5"],
        "files": entries,
    }
    manifest_bytes = (json.dumps(manifest, sort_keys=True, indent=2) + "\n").encode("utf-8")

    output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(prefix=output.name + ".", suffix=".tmp", dir=output.parent)
    os.close(fd)
    temporary = Path(temporary_name)
    try:
        with zipfile.ZipFile(temporary, "w", allowZip64=True) as archive:
            for full_path, archive_path in files:
                archive.write(full_path, archive_path, compress_type=zipfile.ZIP_DEFLATED, compresslevel=6)
            archive.writestr(zip_info(MANIFEST_PATH), manifest_bytes)
        os.replace(temporary, output)
    finally:
        if temporary.exists():
            temporary.unlink()

    package_sha256, _package_md5, package_size = file_digests(output)
    sha_sidecar = output.with_name(output.name + ".sha256")
    sha_sidecar.write_text(package_sha256 + "\n", encoding="ascii")
    external_manifest = output.with_name(output.name + ".manifest.json")
    external_manifest.write_bytes(manifest_bytes)

    print(f"Content pack: {output}")
    print(f"  version: {version}")
    print(f"  files:   {len(files)} (+ embedded manifest)")
    print(f"  size:    {package_size / (1024 * 1024):.2f} MiB")
    print(f"  sha256:  {package_sha256}")
    print(f"  manifest:{external_manifest}")
    for path in skipped:
        print(f"  skipped (not packaged): {path}")
    return manifest


def main() -> int:
    try:
        build(SOURCE_DIR, ZIP_OUT, VERSION)
    except (OSError, ValueError, zipfile.BadZipFile) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
