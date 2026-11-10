# Kingmakers RP data package pipeline

Builds the game data package from **your own assets**, so players do not need
GTA: San Andreas installed. It packs what you give it and reports what is
missing. It never creates placeholder game data, and you must not feed it
Rockstar's files or another server's package: everything in it has to be
yours or licensed to you.

## Files

| File | What it is |
|---|---|
| `manifest.json` | Everything the engine and the gamemode need. Generated, do not edit. |
| `gen_manifest.py` | Regenerates `manifest.json` from the client source, the IDE files and `gamemode_usage.json`. |
| `gamemode_usage.json` | Model ids the gamemode spawns (skins, vehicles, weapons, pickups). Edit when the gamemode changes. |
| `build_datapack.py` | `init`, `check` and `build` commands. |

## Workflow

```bash
python3 tools/datapack/build_datapack.py init  datapack-src      # create the folder layout
python3 tools/datapack/build_datapack.py check datapack-src      # what is done, what is missing
python3 tools/datapack/build_datapack.py check datapack-src -v   # full lists
python3 tools/datapack/build_datapack.py build datapack-src \
        -o backend/public/downloads/kingmakers_data_pack_v1.zip
```

`build` refuses to produce a package while required items are missing.
`--allow-incomplete` makes a test package anyway; it will not run a full game.

## Publishing the package

The launcher offers the package as soon as the backend finds both files in
its downloads folder:

```
backend/public/downloads/kingmakers_data_pack_v1.zip
backend/public/downloads/kingmakers_data_pack_v1.zip.sha256
```

`build ... -o backend/public/downloads/kingmakers_data_pack_v1.zip` writes
both. They are not committed to git (too large), so build or upload them on
the VPS, in the service directory's `backend/public/downloads/`.
`/api/launcher/config` then returns `dataPackUrl`, `dataPackSizeMb` and
`dataPackSha256`, and players see "DOWNLOAD GAME DATA" instead of the GTA: San
Andreas import. Remove either file to switch the offer off again.

The launcher resumes interrupted downloads, verifies the checksum, runs the
same data check as for an imported GTA SA, and refuses to start the game if
the package is incomplete. Do not publish a `--allow-incomplete` build.

## Source layout

```
datapack-src/
  data/            gta.dat, default.dat, *.ide, handling.cfg, carcols.dat, timecyc.dat,
                   surface.dat, weapon.dat, fonts.dat, plus your map IDE/IPL files
  anim/ped.ifp     core animation block
  models/gta3/     loose .dff .col .ifp .ipl  -> packed into texdb/gta3.img
  models/gta_int/  loose interior files       -> packed into texdb/gta_int.img
  texdb/<name>/    texture databases: mobile, txd, gta3, gta_int, cutscene, player, menu
  audio/           sound banks and streams
```

## Priority tiers

- **engine**: needed before the game starts at all (core files, texture
  databases, `ped.ifp`, the base ped model).
- **gamemode**: what the Kingmakers gamemode spawns today: 22 skins, 28
  vehicles, 8 weapons and their animation blocks. Needed for a first release.
- **defined**: only declared in the definition files. Make the asset, or
  delete its line from the IDE so nothing can request it.

To shrink the job, cut the IDE files down to what you ship and change the
gamemode to match, then re-run `gen_manifest.py`.

## What the tool cannot do or verify

- **Textures.** Android GTA SA reads textures from texture databases
  (`texdb/<name>/<name>.txt` plus `.dat/.toc/.tmb` files), not from `.txd`
  files inside the IMG. This tool copies databases you have already built and
  checks that each one exists; it does not build them or check that every
  texture a model needs is inside.
- **Client UI textures.** The client looks up some textures by name
  (`client_textures` in the manifest): a few in `txd`, `mobile` and `gta3`,
  and its own icons in the optional `samp` and `gui` databases, which are
  loaded only when present. A missing one shows a fallback or nothing; it is
  not checked here.
- **Animations.** Names are checked in `ANP3` blocks only. The skeleton and
  bone ids your animations use must match your ped models; that is not checked.
- **Model contents.** A `.dff` is checked for presence, not for being a valid
  model.
- **Engine startup requests.** The engine is closed source and may ask for
  models at startup beyond this list. Expect to find a few on the first device
  test; add them to `gamemode_usage.json` under the right list.
- **Gamemode object models** (pickups, attached objects) come from map IDE
  files, which this repository does not have. `check` lists their ids.

The definition files currently in `android-client/.../assets/game/SAMP/`
(`peds.ide`, `vehicles.ide`, `default.ide`, `weapon.dat`, `main.scm`,
`script.img`) are GTA: San Andreas's own and name its models. A fully custom
package needs your own versions of these as well.
