#!/usr/bin/env python3
"""
Kingmakers RP - greybox test package generator

Generates a deliberately plain, fully original set of game data: a flat grey
ground slab with collision, the map definition that places it, static
animations and a first set of data files. Nothing here comes from GTA: San
Andreas. Its only purpose is to find out, on a phone, what the engine needs
before real art is made.

It writes into a data package source folder (see ../build_datapack.py):

    python3 tools/datapack/greybox/gen_greybox.py datapack-src
    python3 tools/datapack/build_datapack.py build datapack-src -o greybox.zip --allow-incomplete

Stage 1 covers only formats that can be written without testing against the
engine being a guess in the dark: see STAGE and the README. Every later stage
is driven by the "[file] MISS" lines the client logs on a device run.
"""

import json
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
MANIFEST = os.path.join(HERE, "..", "manifest.json")
STAGE = 1

RW_VERSION = 0x1803FFFF          # RenderWare 3.6.0.3, the version GTA SA models use

# Ground slab: covers the whole SA-sized map square, top surface at GROUND_Z.
MAP_HALF = 3000.0
GROUND_Z = 12.0
SLAB_THICKNESS = 2.0
GROUND_MODEL = "kmgb_ground"
GROUND_ID = 19900                # free object id (stock data stops well below)
INT_MODEL = "kmgb_room"
INT_ID = 19901
GREY = (128, 128, 128, 255)


# ------------------------------------------------------------ RenderWare ----

def chunk(type_id, payload):
    return struct.pack("<III", type_id, len(payload), RW_VERSION) + payload


def rw_string(text):
    data = text.encode("ascii") + b"\0"
    data += b"\0" * (-len(data) % 4)
    return chunk(0x02, data)


def box_mesh(min_xyz, max_xyz):
    """24 vertices (4 per face, so each face has its own normal) and 12 triangles."""
    (x0, y0, z0), (x1, y1, z1) = min_xyz, max_xyz
    faces = [   # 4 corners counter-clockwise seen from outside, and the face normal
        ([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], (0, 0, 1)),
        ([(x0, y1, z0), (x1, y1, z0), (x1, y0, z0), (x0, y0, z0)], (0, 0, -1)),
        ([(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], (0, -1, 0)),
        ([(x1, y1, z0), (x0, y1, z0), (x0, y1, z1), (x1, y1, z1)], (0, 1, 0)),
        ([(x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (x1, y0, z1)], (1, 0, 0)),
        ([(x0, y1, z0), (x0, y0, z0), (x0, y0, z1), (x0, y1, z1)], (-1, 0, 0)),
    ]
    verts, normals, tris = [], [], []
    for corners, normal in faces:
        base = len(verts)
        verts += corners
        normals += [normal] * 4
        tris += [(base, base + 1, base + 2), (base, base + 2, base + 3)]
    return verts, normals, tris


def build_dff(name, min_xyz, max_xyz, colour=GREY):
    """A static, untextured, pre-lit box as a RenderWare clump."""
    verts, normals, tris = box_mesh(min_xyz, max_xyz)
    centre = [(a + b) / 2 for a, b in zip(min_xyz, max_xyz)]
    radius = sum(((b - a) / 2) ** 2 for a, b in zip(min_xyz, max_xyz)) ** 0.5

    # Geometry: positions | prelit | normals | light | modulate material colour, no texture sets
    flags = 0x02 | 0x08 | 0x10 | 0x20 | 0x40
    geo = struct.pack("<IIII", flags, len(tris), len(verts), 1)
    geo += b"".join(struct.pack("<4B", *colour) for _ in verts)
    geo += b"".join(struct.pack("<HHHH", b, a, 0, c) for a, b, c in tris)     # v2, v1, material, v3
    geo += struct.pack("<4f", *centre, radius) + struct.pack("<II", 1, 1)
    geo += b"".join(struct.pack("<3f", *v) for v in verts)
    geo += b"".join(struct.pack("<3f", *n) for n in normals)

    material = chunk(0x07, chunk(0x01, struct.pack("<I4BII3f", 0, *colour, 0, 0, 1.0, 0.0, 1.0)) + chunk(0x03, b""))
    material_list = chunk(0x08, chunk(0x01, struct.pack("<Ii", 1, -1)) + material)

    indices = [i for t in tris for i in t]
    bin_mesh = chunk(0x50E, struct.pack("<III", 0, 1, len(indices)) + struct.pack("<II", len(indices), 0)
                     + struct.pack(f"<{len(indices)}I", *indices))
    geometry = chunk(0x0F, chunk(0x01, geo) + material_list + chunk(0x03, bin_mesh))

    identity = (1, 0, 0, 0, 1, 0, 0, 0, 1)
    frame = struct.pack("<9f3fiI", *identity, 0, 0, 0, -1, 0)
    frame_name = chunk(0x0253F2FE, name.encode("ascii"))
    frame_list = chunk(0x0E, chunk(0x01, struct.pack("<I", 1) + frame) + chunk(0x03, frame_name))

    atomic = chunk(0x14, chunk(0x01, struct.pack("<IIII", 0, 0, 5, 0)) + chunk(0x03, b""))
    clump = (chunk(0x01, struct.pack("<III", 1, 0, 0)) + frame_list
             + chunk(0x1A, chunk(0x01, struct.pack("<I", 1)) + geometry) + atomic + chunk(0x03, b""))
    return chunk(0x10, clump)


def check_rw(data):
    """Walk the chunk tree; raises if any chunk overruns its parent. Returns chunk count."""
    containers = {0x10, 0x0E, 0x1A, 0x0F, 0x08, 0x07, 0x14, 0x03}

    def walk(buf, start, end, parent):
        pos, count = start, 0
        while pos < end:
            type_id, size, version = struct.unpack_from("<III", buf, pos)
            if version != RW_VERSION or pos + 12 + size > end:
                raise ValueError(f"bad chunk 0x{type_id:X} at {pos}")
            count += 1
            if type_id in containers and not (type_id == 0x03 and size == 0):
                count += walk(buf, pos + 12, pos + 12 + size, type_id)
            pos += 12 + size
        if pos != end:
            raise ValueError("chunk sizes do not add up")
        return count

    return walk(data, 0, len(data), None)


# --------------------------------------------------------------- collision ----

def build_col(name, model_id, min_xyz, max_xyz, material=0):
    """COL2 collision file with one box. Layout: header, then the box array."""
    centre = [(a + b) / 2 for a, b in zip(min_xyz, max_xyz)]
    radius = sum(((b - a) / 2) ** 2 for a, b in zip(min_xyz, max_xyz)) ** 0.5

    body = name.encode("ascii").ljust(22, b"\0") + struct.pack("<H", model_id)
    body += struct.pack("<3f3f3ff", *min_xyz, *max_xyz, *centre, radius)
    # Offsets count from the size field (file start + 4). Header after the size field is 100 bytes.
    box_offset = 4 + 100
    body += struct.pack("<HHHBB", 0, 1, 0, 0, 0)             # spheres, boxes, faces, lines, pad
    body += struct.pack("<I", 0x02)                          # flags: has collision data
    body += struct.pack("<6I", 0, box_offset, 0, 0, 0, 0)    # spheres, boxes, lines, verts, faces, planes
    assert len(body) == 100, len(body)
    body += struct.pack("<3f3f4B", *min_xyz, *max_xyz, material, 0, 255, 0)
    return b"COL2" + struct.pack("<I", len(body)) + body


# -------------------------------------------------------------- animation ----

def build_ifp(block_name, anim_names):
    """ANP3 block where every animation is one static key on the root bone."""
    out = b""
    for anim in anim_names:
        key = struct.pack("<4hh3h", 0, 0, 0, 4096, 0, 0, 0, 0)      # quaternion, time, translation
        obj = b"Root".ljust(24, b"\0") + struct.pack("<III", 4, 1, 0) + key
        out += anim.encode("ascii")[:23].ljust(24, b"\0") + struct.pack("<III", 1, len(obj), 1) + obj
    header = block_name.encode("ascii").ljust(24, b"\0") + struct.pack("<I", len(anim_names))
    return b"ANP3" + struct.pack("<I", len(header) + len(out)) + header + out


# ------------------------------------------------------------- text data ----

def timecyc():
    """8 times of day for each of 23 weather types, one neutral overcast look."""
    hours = [(0, 0.25), (5, 0.45), (6, 0.7), (7, 0.9), (12, 1.0), (19, 0.85), (20, 0.55), (22, 0.3)]
    lines = ["// greybox test data - neutral lighting",
             "// Amb Amb_Obj Dir SkyTop SkyBot SunCore SunCorona SunSz SprSz SprBght Shdw LightShd PoleShd "
             "FarClp FogSt LightOnGround LowCloudsRGB BottomCloudRGB WaterRGBA Alpha1 RGB1 Alpha2 RGB2 "
             "CloudAlpha HighLightMin WaterFogAlpha DirMult"]
    for weather in range(23):
        lines.append(f"// weather {weather}")
        for _hour, level in hours:
            a = int(60 * level)          # ambient
            d = int(255 * level)         # directional / sun
            s1, s2 = int(110 * level), int(170 * level)
            row = [a, a, a, a, a, a, 255, 255, 255, s1, s1 + 20, s2, s2, s2, s2 + 20,
                   d, d, d, d, d, d, "1.0", "1.0", "1.0", 120, 100, 100, "800.0", "100.0", "1.0",
                   s2, s2, s2, s1, s1, s1, 90, 110, 130, 200, 0, 0, 0, 0, 0, 0, 0, 0, 200, 0, 0, "1.0"]
            lines.append(" ".join(str(v) for v in row))
    return "\n".join(lines) + "\n"


def carcols(vehicle_models):
    lines = ["# greybox test data - vehicle colours", "col",
             "0,0,0\t\t# 0 black", "245,245,245\t# 1 white", "128,128,128\t# 2 grey",
             "170,30,30\t# 3 red", "30,60,150\t# 4 blue", "200,170,60\t# 5 gold", "end", "", "car"]
    lines += [f"{m}, 2,2, 0,0, 1,1, 3,3, 4,4" for m in vehicle_models]
    lines += ["end", ""]
    return "\n".join(lines)


def handling(handling_ids):
    """One generic mid-size car line per handling id (a..ah columns, 36 values)."""
    lines = ["; greybox test data - every vehicle handles like one generic car",
             "; A=id B=mass C=turnMass D=drag F..H=centreOfMass I=submerged J=traction K=tractionLoss",
             "; L=tractionBias M=gears N=maxVel O=accel P=inertia Q=drive R=engine S=brake T=brakeBias",
             "; U=abs V=steerLock a=suspForce b=suspDamp c=suspHighSpdDamp d=suspUpper e=suspLower",
             "; f=suspBias g=antiDive aa=seatOffset ab=damage ac=value af=modelFlags ag=handlingFlags",
             "; ah=frontLights ai=rearLights aj=animGroup"]
    for hid in handling_ids:
        lines.append(f"{hid:<12s} 1600.0 3500.0 2.0   0.0 0.0 -0.1   75   0.75 0.85 0.5   5 180.0 20.0 10.0  R P   "
                     "8.0 0.5 0   35.0   1.0 0.1 0.0   0.3 -0.15 0.5 0.3   0.25 0.5 20000   0 0   0 1 0")
    return "\n".join(lines) + "\n"


# ------------------------------------------------------------------ main ----

def write(src, rel, data):
    path = os.path.join(src, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb" if isinstance(data, bytes) else "w") as f:
        f.write(data)
    return rel


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 1
    src = sys.argv[1]
    with open(MANIFEST) as f:
        manifest = json.load(f)
    written = []

    # World: one slab for the outdoor map, one small box so the interior archive is not empty
    slab_min = (-MAP_HALF, -MAP_HALF, -SLAB_THICKNESS)
    slab_max = (MAP_HALF, MAP_HALF, 0.0)
    room_min, room_max = (-10.0, -10.0, -1.0), (10.0, 10.0, 0.0)
    for folder, model, model_id, lo, hi in (("models/gta3", GROUND_MODEL, GROUND_ID, slab_min, slab_max),
                                           ("models/gta_int", INT_MODEL, INT_ID, room_min, room_max)):
        dff = build_dff(model, lo, hi)
        check_rw(dff)
        written.append(write(src, f"{folder}/{model}.dff", dff))
        written.append(write(src, f"{folder}/{model}.col", build_col(model, model_id, lo, hi)))

    written.append(write(src, "data/maps/greybox/greybox.ide",
                         "# greybox test data - object definitions\nobjs\n"
                         f"{GROUND_ID}, {GROUND_MODEL}, kmgb, 3000, 0\n"
                         f"{INT_ID}, {INT_MODEL}, kmgb, 300, 0\nend\n"))
    written.append(write(src, "data/maps/greybox/greybox.ipl",
                         "# greybox test data - object placement\ninst\n"
                         f"{GROUND_ID}, {GROUND_MODEL}, 0, 0.0, 0.0, {GROUND_Z}, 0, 0, 0, 1, -1\nend\n"))
    written.append(write(src, "data/gta.dat",
                         "# greybox test data - world definition\n"
                         "IDE DATA\\MAPS\\greybox\\greybox.IDE\n"
                         "IPL DATA\\MAPS\\greybox\\greybox.IPL\n"))
    written.append(write(src, "data/default.dat",
                         "# greybox test data - base definitions\n"
                         "IDE DATA\\DEFAULT.IDE\nIDE DATA\\VEHICLES.IDE\nIDE DATA\\PEDS.IDE\n"))

    # Animations: every block the engine or the gamemode needs, all static
    for block in manifest["animation_blocks"]:
        if block["tier"] != "defined":
            written.append(write(src, block["file"], build_ifp(block["block"], block["animations"])))

    vehicles = [v for v in manifest["models"]["vehicles"] if v["tier"] == "gamemode"]
    written.append(write(src, "data/timecyc.dat", timecyc()))
    written.append(write(src, "data/carcols.dat", carcols([v["model"] for v in vehicles])))
    written.append(write(src, "data/handling.cfg", handling(sorted({v["handling"] for v in vehicles}))))

    print(f"greybox stage {STAGE}: wrote {len(written)} files into {src}")
    for rel in written:
        print(f"  {rel}  ({os.path.getsize(os.path.join(src, rel))} bytes)")
    print("\nnot generated yet (decided by the first device run): texture databases, ped / vehicle / weapon\n"
          "models, sounds, and any further data files the engine asks for.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
