#!/usr/bin/env python3
"""
Kingmakers RP - data package manifest generator

Derives the list of everything a self-contained data package must provide from
this repository itself, so the list follows the code instead of memory:

  * animations  <- android-client/.../game/Animation/AnimAssoc*.{cpp,h}
                   (the engine's animation groups: IFP block + animation names)
  * models      <- android-client/.../assets/game/SAMP/{peds,vehicles,default}.ide
  * core files  <- file names the client opens (game.cpp, Streaming.cpp, hooks.cpp)
  * priorities  <- gamemode_usage.json (what the Kingmakers gamemode spawns)

Writes manifest.json next to this script. Re-run after changing any input.
"""

import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
CPP = os.path.join(REPO, "android-client/app/src/main/cpp/multiplayer/game")
IDE_DIR = os.path.join(REPO, "android-client/app/src/main/assets/game/SAMP")
OUT = os.path.join(HERE, "manifest.json")

# Files the engine opens by name at startup. Path is where the package must put
# them; "kind" tells the builder how the file is produced.
CORE_FILES = [
    {"path": "data/gta.dat",       "kind": "text",  "why": "world definition: lists the IDE/IPL files and IMG archives to load"},
    {"path": "data/default.dat",   "kind": "text",  "why": "base definition list (default.ide, vehicles.ide, peds.ide)"},
    {"path": "data/default.ide",   "kind": "text",  "why": "weapon and generic object definitions"},
    {"path": "data/vehicles.ide",  "kind": "text",  "why": "vehicle definitions"},
    {"path": "data/peds.ide",      "kind": "text",  "why": "ped (skin) definitions"},
    {"path": "data/handling.cfg",  "kind": "text",  "why": "one handling line per handling id used in vehicles.ide"},
    {"path": "data/carcols.dat",   "kind": "text",  "why": "vehicle colour palette"},
    {"path": "data/timecyc.dat",   "kind": "text",  "why": "lighting and sky per hour and weather"},
    {"path": "data/surface.dat",   "kind": "text",  "why": "surface friction table"},
    {"path": "data/weapon.dat",    "kind": "text",  "why": "weapon stats and animation groups"},
    {"path": "data/fonts.dat",     "kind": "text",  "why": "font glyph widths"},
    {"path": "anim/ped.ifp",       "kind": "ifp",   "why": "core animation block, loaded at startup"},
    {"path": "texdb/gta3.img",     "kind": "img",   "why": "streamed models, collision and animation blocks (built from models/gta3/)"},
    {"path": "texdb/gta_int.img",  "kind": "img",   "why": "interior models (built from models/gta_int/)"},
]

# Texture databases loaded in CGame::InitialiseRenderWare (VER_SAMP branch).
TEXTURE_DATABASES = ["mobile", "txd", "gta3", "gta_int", "cutscene", "player", "menu"]
# Loaded by the client only when installed (game.cpp); they hold its own UI textures.
OPTIONAL_DATABASES = ["samp", "gui"]


def read(path):
    with open(path, "r", encoding="latin-1") as f:
        return f.read()


def parse_animations():
    """IFP block -> sorted unique animation names, from the engine's own tables."""
    anims_h = read(os.path.join(CPP, "Animation/AnimAssocAnimations.h"))
    desc_cpp = read(os.path.join(CPP, "Animation/AnimAssocDescriptions.cpp"))

    arrays = {}
    for m in re.finditer(r"static\s+const\s+char\s*\*\s*(\w+)\s*\[\]\s*=\s*\{(.*?)\};", anims_h, re.S):
        body = re.sub(r"//[^\n]*", "", m.group(2))
        arrays[m.group(1)] = re.findall(r'"([^"]+)"', body)

    blocks, groups = {}, []
    for m in re.finditer(r'\{\s*"([^"]+)"\s*,\s*"([^"]+)"\s*,\s*\w+\s*,\s*awc\((\w+)\)', desc_cpp):
        group, block, array = m.groups()
        names = arrays.get(array)
        if names is None:
            print(f"warning: animation array {array} not found", file=sys.stderr)
            continue
        groups.append({"group": group, "block": block.lower(), "count": len(names)})
        blocks.setdefault(block.lower(), set()).update(n.lower() for n in names)

    return {b: sorted(n) for b, n in sorted(blocks.items())}, groups


def parse_client_textures():
    """Textures the client code itself looks up by name: database -> names."""
    root = os.path.dirname(CPP)
    found = {}
    for dirpath, _dirs, files in os.walk(root):
        if os.sep + "vendor" in dirpath:
            continue
        for name in files:
            if not name.endswith((".cpp", ".h")):
                continue
            for line in read(os.path.join(dirpath, name)).splitlines():
                code = line.split("//", 1)[0]
                for db, tex in re.findall(r'LoadTextureFromDB\(\s*"([^"]+)"\s*,\s*"([^"]+)"', code):
                    found.setdefault(db.lower(), set()).add(tex)
    return {db: sorted(names) for db, names in sorted(found.items())}


def ide_sections(path):
    """Yield (section, fields) for every data line of an IDE file."""
    section = None
    for raw in read(path).splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        if "," not in line:
            section = None if line.lower() == "end" else line.lower()
            continue
        if section:
            yield section, [f.strip() for f in line.split(",")]


def parse_models():
    peds, vehicles, weapons, objects = [], [], [], []
    for sec, f in ide_sections(os.path.join(IDE_DIR, "peds.ide")):
        if sec == "peds" and len(f) >= 3:
            peds.append({"id": int(f[0]), "model": f[1].lower(), "txd": f[2].lower(),
                         "anim_group": f[5].lower() if len(f) > 5 else ""})
    for sec, f in ide_sections(os.path.join(IDE_DIR, "vehicles.ide")):
        if sec == "cars" and len(f) >= 5:
            vehicles.append({"id": int(f[0]), "model": f[1].lower(), "txd": f[2].lower(),
                             "type": f[3].lower(), "handling": f[4].upper()})
    for sec, f in ide_sections(os.path.join(IDE_DIR, "default.ide")):
        if len(f) < 3:
            continue
        entry = {"id": int(f[0]), "model": f[1].lower(), "txd": f[2].lower()}
        if sec == "weap":
            entry["anim_block"] = f[3].lower() if len(f) > 3 and f[3].lower() != "null" else ""
            weapons.append(entry)
        elif sec in ("objs", "tobj", "hier"):
            entry["section"] = sec
            objects.append(entry)
    return peds, vehicles, weapons, objects


def main():
    usage = json.load(open(os.path.join(HERE, "gamemode_usage.json")))
    anim_blocks, anim_groups = parse_animations()
    peds, vehicles, weapons, objects = parse_models()
    client_textures = parse_client_textures()

    def tier(entries, used, engine=()):
        for e in entries:
            e["tier"] = "engine" if e["id"] in engine else ("gamemode" if e["id"] in used else "defined")

    tier(peds, set(usage["skins"]), set(usage["engine_peds"]))
    tier(vehicles, set(usage["vehicles"]))
    tier(weapons, set(usage["weapon_models"]))
    tier(objects, set())

    # Weapon animation blocks actually needed by gamemode weapons
    needed_blocks = {"ped"} | {w["anim_block"] for w in weapons if w["tier"] == "gamemode" and w["anim_block"]}
    blocks = [{"block": b, "file": "anim/ped.ifp" if b == "ped" else f"models/gta3/{b}.ifp",
               "tier": "engine" if b == "ped" else ("gamemode" if b in needed_blocks else "defined"),
               "animations": names} for b, names in anim_blocks.items()]

    defined_ids = {e["id"] for e in peds + vehicles + weapons + objects}
    manifest = {
        "_generated_by": "tools/datapack/gen_manifest.py - do not edit by hand",
        "tiers": {
            "engine":   "needed before the game can start at all",
            "gamemode": "spawned by the Kingmakers gamemode today; needed for a playable first release",
            "defined":  "only declared in the definition files; make it, or delete its line from the IDE",
        },
        "core_files": CORE_FILES,
        "texture_databases": [{"name": n, "path": f"texdb/{n}/", "index": f"texdb/{n}/{n}.txt"} for n in TEXTURE_DATABASES],
        "optional_texture_databases": [{"name": n, "path": f"texdb/{n}/", "index": f"texdb/{n}/{n}.txt",
                                        "note": "client UI textures; loaded only when present"} for n in OPTIONAL_DATABASES],
        "client_textures": client_textures,
        "animation_blocks": blocks,
        "animation_groups": anim_groups,
        "models": {"peds": peds, "vehicles": vehicles, "weapons": weapons, "objects": objects},
        "gamemode_objects": [{"id": i, "note": "used by the gamemode but not declared in default.ide; "
                              "needs an objs line in one of your map IDE files plus its model"}
                             for i in usage["objects"] if i not in defined_ids],
    }
    with open(OUT, "w") as f:
        json.dump(manifest, f, indent=1)
        f.write("\n")

    def count(entries, t):
        return sum(1 for e in entries if e["tier"] == t)

    print(f"wrote {os.path.relpath(OUT, REPO)}")
    for label, entries in (("peds", peds), ("vehicles", vehicles), ("weapons", weapons), ("objects", objects)):
        print(f"  {label:9s} {len(entries):4d} defined | engine {count(entries, 'engine'):3d} | gamemode {count(entries, 'gamemode'):3d}")
    total_anims = sum(len(b["animations"]) for b in blocks)
    ped = next(b for b in blocks if b["block"] == "ped")
    print(f"  animation blocks {len(blocks)} ({total_anims} animations; ped.ifp alone needs {len(ped['animations'])})")
    print("  client-code textures: " + ", ".join(f"{db} {len(n)}" for db, n in client_textures.items()))
    missing = [i for i in usage["skins"] + usage["vehicles"] + usage["weapon_models"] if i not in defined_ids]
    if missing:
        print(f"  warning: gamemode ids not in the IDE files: {missing}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
