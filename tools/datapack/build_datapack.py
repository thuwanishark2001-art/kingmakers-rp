#!/usr/bin/env python3
"""
Kingmakers RP - data package builder

Turns a folder of YOUR OWN assets into the data package the client loads, and
tells you what is still missing. It never generates placeholder game data: a
file that is not in the source folder is reported, not invented.

Source folder layout (create it with `init`):

    data/            text files: gta.dat, default.dat, *.ide, handling.cfg, ... and your map IDE/IPL
    anim/ped.ifp     core animation block
    models/gta3/     loose .dff .col .ifp .ipl files  -> packed into texdb/gta3.img
    models/gta_int/  loose interior files             -> packed into texdb/gta_int.img
    texdb/<name>/    texture databases, already built (see README: textures)
    audio/           sound banks and streams, copied as they are

Commands:
    init  <src>                 create the folder layout
    check <src>                 progress report against manifest.json
    build <src> -o <out.zip>    build the IMG archives, verify, zip, write .sha256
                                (fails if engine/gamemode items are missing;
                                 --allow-incomplete builds a test package anyway)
"""

import argparse
import hashlib
import json
import os
import struct
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
MANIFEST = os.path.join(HERE, "manifest.json")

SECTOR = 2048
IMG_SOURCES = {"texdb/gta3.img": "models/gta3", "texdb/gta_int.img": "models/gta_int"}
REQUIRED_TIERS = ("engine", "gamemode")
# Folders of the client's own content pack; a data package must not carry them.
RESERVED_DIRS = ("samp", "gui", "fonts", "kingmakers")


# ---------------------------------------------------------------- IMG v2 ----

def img_entries(folder):
    """Sorted (archive_name, path) for every file to pack; validates names."""
    entries, seen = [], {}
    if not os.path.isdir(folder):
        return entries
    for name in sorted(os.listdir(folder), key=str.lower):
        path = os.path.join(folder, name)
        if not os.path.isfile(path) or name.startswith("."):
            continue
        try:
            encoded = name.encode("ascii")
        except UnicodeEncodeError:
            raise SystemExit(f"error: {path}: IMG entry names must be ASCII")
        if len(encoded) > 23:
            raise SystemExit(f"error: {path}: IMG entry names are limited to 23 characters")
        if os.path.getsize(path) == 0:
            raise SystemExit(f"error: {path}: empty file")
        key = name.lower()
        if key in seen:
            raise SystemExit(f"error: {path} and {seen[key]} differ only in case")
        seen[key] = path
        entries.append((name, path))
    return entries


def write_img(out_path, entries):
    """Write a GTA IMG version 2 archive holding the real bytes of every entry."""
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    first = (8 + 32 * len(entries) + SECTOR - 1) // SECTOR
    directory, offset = [], first
    for name, path in entries:
        sectors = (os.path.getsize(path) + SECTOR - 1) // SECTOR
        if sectors > 0xFFFF:
            raise SystemExit(f"error: {path}: larger than the 128 MB an IMG entry can hold")
        directory.append((offset, sectors, name))
        offset += sectors
    with open(out_path, "wb") as f:
        f.write(b"VER2" + struct.pack("<I", len(entries)))
        for off, sectors, name in directory:
            f.write(struct.pack("<IHH", off, sectors, 0) + name.encode("ascii").ljust(24, b"\0"))
        for (off, sectors, _name), (_n, path) in zip(directory, entries):
            f.seek(off * SECTOR)
            with open(path, "rb") as src:
                data = src.read()
            f.write(data + b"\0" * (sectors * SECTOR - len(data)))
        if not entries:
            f.write(b"\0" * (first * SECTOR - f.tell()))


def read_img_names(path):
    with open(path, "rb") as f:
        if f.read(4) != b"VER2":
            raise ValueError("not an IMG v2 archive")
        (count,) = struct.unpack("<I", f.read(4))
        return [struct.unpack("<IHH24s", f.read(32))[3].split(b"\0", 1)[0].decode("ascii").lower()
                for _ in range(count)]


# ------------------------------------------------------------------- IFP ----

def ifp_animation_names(path):
    """Animation names in an ANP3 block, or None when the layout is not understood."""
    with open(path, "rb") as f:
        data = f.read()
    if len(data) < 36 or data[:4] != b"ANP3":
        return None
    try:
        (count,) = struct.unpack_from("<I", data, 32)
        pos, names = 36, []
        for _ in range(count):
            name = data[pos:pos + 24].split(b"\0", 1)[0].decode("ascii").lower()
            objects, _size, _flag = struct.unpack_from("<III", data, pos + 24)
            pos += 36
            for _ in range(objects):
                frame_type, frames, _bone = struct.unpack_from("<III", data, pos + 24)
                per_frame = {4: 16, 3: 10}.get(frame_type)
                if per_frame is None:
                    return None
                pos += 36 + frames * per_frame
            if not name or pos > len(data):
                return None
            names.append(name)
        return names
    except (struct.error, UnicodeDecodeError):
        return None


# ----------------------------------------------------------------- check ----

def find_ci(base, rel):
    """Resolve rel under base ignoring case; None when absent."""
    parts = [p for p in rel.strip("/").split("/") if p]

    def walk(cur, i):
        if i == len(parts):
            return cur
        if not os.path.isdir(cur):
            return None
        for name in sorted(os.listdir(cur)):
            if name.lower() == parts[i].lower():
                found = walk(os.path.join(cur, name), i + 1)
                if found:
                    return found
        return None

    return walk(base, 0)


class Report:
    def __init__(self):
        self.rows = []       # (area, done, total, required_missing, optional_missing)
        self.warnings = []

    def add(self, area, items):
        """items: (label, tier, present)"""
        done = sum(1 for _l, _t, ok in items if ok)
        req = [l for l, t, ok in items if not ok and t in REQUIRED_TIERS]
        opt = [l for l, t, ok in items if not ok and t not in REQUIRED_TIERS]
        self.rows.append((area, done, len(items), req, opt))

    @property
    def required_missing(self):
        return sum(len(r[3]) for r in self.rows)

    def print(self, verbose):
        print(f"{'area':22s} {'done':>11s}   missing: required / optional")
        for area, done, total, req, opt in self.rows:
            print(f"{area:22s} {done:5d}/{total:<5d}   {len(req):4d} / {len(opt)}")
        for area, _d, _t, req, opt in self.rows:
            if req:
                shown = req if verbose else req[:12]
                more = "" if len(shown) == len(req) else f" ... and {len(req) - len(shown)} more (use -v)"
                print(f"\n{area} - required, missing:\n  " + "\n  ".join(shown) + more)
            if verbose and opt:
                print(f"\n{area} - optional, missing:\n  " + "\n  ".join(opt))
        for w in self.warnings:
            print(f"\nwarning: {w}")
        print(f"\n{'READY' if self.required_missing == 0 else 'NOT READY'}: "
              f"{self.required_missing} required item(s) missing")


def check(src, manifest):
    rep = Report()
    img_names = {}
    for img, folder in IMG_SOURCES.items():
        img_names[img] = {n.lower() for n, _p in img_entries(os.path.join(src, folder))}
    all_packed = set().union(*img_names.values())

    core = []
    for c in manifest["core_files"]:
        if c["kind"] == "img":
            present = bool(img_names[c["path"]])
        else:
            p = find_ci(src, c["path"])
            present = bool(p) and os.path.getsize(p) > 0
        core.append((f"{c['path']}  ({c['why']})", "engine", present))
    rep.add("core files", core)

    dbs = []
    for db in manifest["texture_databases"]:
        folder = find_ci(src, db["path"])
        index = find_ci(src, db["index"])
        data = folder and any(n.lower().endswith((".dat", ".tmb", ".toc")) for n in os.listdir(folder))
        dbs.append((f"{db['path']}  (needs {db['name']}.txt and its .dat/.toc/.tmb files)", "engine", bool(index and data)))
    rep.add("texture databases", dbs)

    for label, key in (("ped models", "peds"), ("vehicle models", "vehicles"),
                       ("weapon models", "weapons"), ("generic objects", "objects")):
        items = []
        for e in manifest["models"][key]:
            if e["model"] == "null":
                continue
            items.append((f"{e['id']:5d}  models/gta3/{e['model']}.dff  (textures: {e['txd']})", e["tier"],
                          f"{e['model']}.dff" in all_packed))
        rep.add(label, items)

    blocks, anims = [], []
    for b in manifest["animation_blocks"]:
        path = find_ci(src, b["file"])
        blocks.append((f"{b['file']}  ({len(b['animations'])} animations)", b["tier"], bool(path)))
        if not path:
            continue
        have = ifp_animation_names(path)
        if have is None:
            rep.warnings.append(f"{b['file']}: could not read animation names (only ANP3 blocks are checked); "
                                "its animations are not verified")
            continue
        have = set(have)
        anims += [(f"{b['file']}: {a}", b["tier"], a in have) for a in b["animations"]]
    rep.add("animation blocks", blocks)
    if anims:
        rep.add("animations (in blocks)", anims)

    for name in os.listdir(src) if os.path.isdir(src) else []:
        if name.lower() in RESERVED_DIRS:
            rep.warnings.append(f"{name}/ belongs to the client content pack; it is left out of the data package")
    if manifest["gamemode_objects"]:
        ids = ", ".join(str(o["id"]) for o in manifest["gamemode_objects"])
        rep.warnings.append("the gamemode also uses object models that no definition file here declares "
                            f"({ids}); add them to one of your map IDE files with their models, "
                            "or change the gamemode. This tool cannot verify them.")
    return rep


# ----------------------------------------------------------------- build ----

def build(src, out_zip, manifest, allow_incomplete):
    rep = check(src, manifest)
    if rep.required_missing and not allow_incomplete:
        rep.print(False)
        raise SystemExit("\nbuild stopped: the package is incomplete (use --allow-incomplete for a test build)")

    stage = out_zip + ".stage"
    members = []   # (archive name, file path)
    for img, folder in IMG_SOURCES.items():
        entries = img_entries(os.path.join(src, folder))
        if not entries:
            continue
        img_path = os.path.join(stage, img)
        write_img(img_path, entries)
        if read_img_names(img_path) != [n.lower() for n, _p in entries]:
            raise SystemExit(f"error: {img} failed its read-back check")
        members.append((img, img_path))
        print(f"packed {img}: {len(entries)} files, {os.path.getsize(img_path) / 1048576:.1f} MB")

    for top in sorted(os.listdir(src)):
        low = top.lower()
        if low == "models" or low in RESERVED_DIRS or top.startswith("."):
            continue
        root = os.path.join(src, top)
        if not os.path.isdir(root):
            continue
        for dirpath, _dirs, files in os.walk(root):
            for name in sorted(files):
                full = os.path.join(dirpath, name)
                rel = os.path.relpath(full, src).replace(os.sep, "/")
                if rel.lower() in IMG_SOURCES:
                    raise SystemExit(f"error: {rel} would overwrite a built archive; put loose files in models/")
                if os.path.getsize(full) > 0 and not name.startswith("."):
                    members.append((rel, full))

    os.makedirs(os.path.dirname(os.path.abspath(out_zip)), exist_ok=True)
    # Stored, not deflated: the phone extracts it faster and the archives barely compress
    with zipfile.ZipFile(out_zip, "w", zipfile.ZIP_STORED, allowZip64=True) as zf:
        for arc, path in sorted(members):
            zf.write(path, arc)

    sha = hashlib.sha256()
    with open(out_zip, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            sha.update(chunk)
    with open(out_zip + ".sha256", "w") as f:
        f.write(sha.hexdigest())

    for _arc, path in members:
        if path.startswith(stage):
            os.remove(path)
    for dirpath, _d, _f in os.walk(stage, topdown=False):
        os.rmdir(dirpath)

    state = "COMPLETE" if rep.required_missing == 0 else f"INCOMPLETE TEST BUILD ({rep.required_missing} required items missing)"
    print(f"\n{out_zip}\n  files:  {len(members)}\n  size:   {os.path.getsize(out_zip) / 1048576:.1f} MB"
          f"\n  sha256: {sha.hexdigest()}\n  state:  {state}")


def init(src):
    for d in ("data/maps", "anim", "models/gta3", "models/gta_int", "audio",
              *(f"texdb/{db}" for db in ("mobile", "txd", "gta3", "gta_int", "cutscene", "player", "menu"))):
        os.makedirs(os.path.join(src, d), exist_ok=True)
    print(f"created the source layout in {src}\nnext: add your assets, then run  check {src}")


def main():
    ap = argparse.ArgumentParser(description="Build the Kingmakers RP data package from your own assets.")
    sub = ap.add_subparsers(dest="cmd", required=True)
    sub.add_parser("init").add_argument("src")
    c = sub.add_parser("check"); c.add_argument("src"); c.add_argument("-v", "--verbose", action="store_true")
    b = sub.add_parser("build"); b.add_argument("src"); b.add_argument("-o", "--out", required=True)
    b.add_argument("--allow-incomplete", action="store_true")
    args = ap.parse_args()

    if args.cmd == "init":
        return init(args.src)
    if not os.path.isdir(args.src):
        raise SystemExit(f"error: {args.src} is not a folder (run: init {args.src})")
    with open(MANIFEST) as f:
        manifest = json.load(f)
    if args.cmd == "check":
        rep = check(args.src, manifest)
        rep.print(args.verbose)
        return 0 if rep.required_missing == 0 else 2
    build(args.src, args.out, manifest, args.allow_incomplete)


if __name__ == "__main__":
    sys.exit(main())
