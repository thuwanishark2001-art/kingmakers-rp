# Kingmakers RP — roadmap to a full mobile roleplay game

Goal: an original mobile multiplayer roleplay game in the same genre as the big
mobile RP servers: one shared city, jobs, factions, cars, homes, businesses and a
phone-driven interface. It is built in Unity (`unity-client/`) with the Node.js
backend (`backend/`), and uses no code or assets from GTA or any other game.

Time estimates assume one developer working with Claude. Each phase ends with
something players can install.

## Phase 0 — First playable (done, in this repository)

Accounts, Kingsport greybox city, walking, driving, fuel, cash/bank/ATMs,
transfers, delivery job, rentals, payday, chat, KingPhone (Bank, GPS, Jobs,
Settings), autosave, CI builds for Android and the Linux server.
See `unity-client/README.md`.

**Next steps you take:** add the Unity secrets so CI builds the APK, deploy the
server build to the VPS, and set `GAME_TOKEN_SECRET` / `GAME_SERVER_KEY` on the
backend.

## Phase 1 — Solid foundations (3–4 weeks)

- **Database:** move `backend/data/game.json` to PostgreSQL (or SQLite with
  WAL). Tables for accounts, characters, vehicles, properties and transactions,
  plus a money audit log.
- **Server-authoritative movement:** the server checks speed and teleports and
  pulls back players who move impossibly. Cars keep client prediction with
  server limits.
- **Interest management:** only sync players and cars near each player
  (NGO visibility / scene-chunking) so 200 players fit on one server.
- **HTTPS** for the backend (Caddy or nginx + Let's Encrypt); turn off the
  plain-HTTP allowance in `ProjectSetup`.
- **Admin tools:** a role field on accounts, `/kick`, `/ban`, teleport,
  spectate, give money, plus a web admin page that reads the audit log.
- **Crash and error reporting** (Unity Cloud Diagnostics or Sentry) and
  server logs shipped to the VPS.

## Phase 2 — Real city art (6–10 weeks, mostly art)

- Replace the greybox with modular buildings, roads with junctions, sidewalks,
  street lights, trees and props (an original low-poly or stylized look; a kit
  of ~60 modular pieces covers most of the city).
- Interiors: apartment, house, shop, bank, police station, hospital.
- Characters: rigged base bodies (male/female), clothing slots, animations
  (idle, walk, run, enter car, sit, phone).
- 10–15 original car models with working wheels and lights.
- Day/night cycle, weather, minimap (render texture or baked map image).
- Addressables for downloadable content, so the APK stays small and new
  districts download later. This replaces the `split_game.apk` approach.

## Phase 3 — Roleplay systems (6–8 weeks)

- **Property:** buy/sell houses and apartments, keys, storage, rent.
- **Owned vehicles:** dealership purchases, garages, insurance, tuning and paint.
- **Inventory:** items, phone, ID card, licences, food/drink, weapons for the
  factions that need them.
- **More jobs:** taxi (players call through the phone), bus driver, mechanic,
  miner, farmer, trucker. Each needs its own loop, vehicle and pay table.
- **Factions:** police (cuff, fine, jail, wanted level), EMS (revive,
  treatment), city government, news. Ranks, duty, faction vehicles, radio chat.
- **Gangs and turf wars** with scheduled capture windows.
- **Businesses:** player-owned shops with stock and income.
- **Phone apps:** contacts, calls/SMS, taxi, marketplace, bank history, map.
- **Proximity voice chat** (Unity Vivox) and local/OOC/faction chat channels.

## Phase 4 — Live game (ongoing)

- **Monetization:** VIP subscription and a premium-currency shop with Google
  Play Billing (Unity IAP). Sell cosmetics, cars and convenience. Never sell
  power, and keep everything you sell server-validated.
- **Events and battle pass:** seasonal city events, daily tasks, rewards track.
- **Several servers** behind a server list (the backend already returns the
  game address).
- **Anti-cheat:** server checks on every economy action (already the rule),
  rate limits, report system and moderator tools.
- Google Play listing: privacy policy, age rating, data safety form, and a
  terms of service covering in-game purchases.

## Retiring the GTA SA stack

The open.mp gamemode (`gamemode/`, `server/`) and the SA-MP Android client
(`android-client/`) ship Rockstar's engine (`libGTASA.so`). Keep them only until
the Unity client reaches feature parity (end of Phase 3), then remove them, their
CI workflow and their download links. Everything worth keeping from the gamemode
(jobs, factions, admin commands, economy numbers) is a design reference for the
Unity systems above.
