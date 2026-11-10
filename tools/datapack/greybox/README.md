# Greybox test package

A deliberately plain, fully original data set used to find out what the engine
needs before real art is made. Nothing in it comes from GTA: San Andreas. It
is a test tool, not something to publish to players.

## Stage 1 contents

| File | What it is |
|---|---|
| `texdb/gta3.img` | flat grey ground slab (model + collision), static weapon animation blocks |
| `texdb/gta_int.img` | one small box, so the interior archive is not empty |
| `data/gta.dat`, `data/default.dat` | world and base definition lists |
| `data/maps/greybox/greybox.ide`, `.ipl` | defines and places the slab |
| `anim/ped.ifp` | all 227 core animations as static poses |
| `data/timecyc.dat`, `carcols.dat`, `handling.cfg` | neutral lighting, a small palette, one generic car |

Not in stage 1: texture databases, character / vehicle / weapon models,
sounds, and whatever else the engine turns out to ask for. Stage 1 is not
expected to reach gameplay. Its job is to produce the first log.

## Test loop

1. Install the latest APK.
2. On the phone, download `dist/greybox_stage1.zip`.
3. In Kingmakers RP: **SELECT GTA SA OBB FILE** (or long-press **Copy log** on
   the main screen, then **REPLACE GAME DATA**) and pick the zip.
4. Press **PLAY NOW** and let it run until it crashes or stops.
5. Reopen the launcher, tap **Copy log**, and paste the text to the developer.

The log lists every file the engine asked for and did not find
(`[file] MISS ...`), in order, plus the last lines before it stopped. The next
stage is built from that list.

## Regenerating

```bash
python3 tools/datapack/build_datapack.py init /tmp/gb
python3 tools/datapack/greybox/gen_greybox.py /tmp/gb
python3 tools/datapack/build_datapack.py build /tmp/gb \
        -o tools/datapack/greybox/dist/greybox_stage1.zip --allow-incomplete
```

## Unverified assumptions

These formats were written from their known layouts and checked only for
internal consistency, not against the engine: the model (RenderWare clump),
the collision file (COL2, one box), the animation blocks (ANP3) and
`timecyc.dat` (52 values per line). If the log shows a crash right after one
of these files is opened, that file's format is the first suspect.
