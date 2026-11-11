# Kingsport — Kingmakers RP Unity client & server

An original mobile multiplayer roleplay game in Unity 6. One Unity project
builds both the **Android client** and the **dedicated server**. Accounts and
saves live in the Node.js backend (`backend/game-api.js`, mounted at `/api/v2`).

Nothing here uses GTA: San Andreas or any other game's code or assets. The city,
models and UI are made by this project.

## What is in the first playable build

| Area | What works |
|---|---|
| Accounts | Register / log in (backend, scrypt passwords, signed tokens); the server checks every login token with the backend |
| City | **Kingsport**: 8×8 block grid, four districts (Downtown, Old Town, Harbor, Crown Hills), generated from a fixed seed so server and clients match |
| Movement | Third-person walking with a touch joystick and drag-to-look camera; keyboard + mouse in the editor |
| Vehicles | ~25 parked cars; enter/exit, arcade driving, fuel; rentals at Bayside Motors ($250) |
| Economy | Cash and bank, ATMs and Crown Bank for deposit/withdraw, bank transfers to online players, payday every 15 minutes |
| Jobs | Delivery driver from the Harbor Delivery Depot: random drop-offs, pay by distance, levels from experience |
| Fuel | Refuel at West Fuel / East Fuel |
| Social | Global chat, name tags |
| UI | UI Toolkit HUD: money, location, objective, speedometer and fuel bar, toasts, chat, KingPhone (Bank, GPS, Jobs, Settings) |
| Saving | Server saves every character every 2 minutes and on disconnect |

Every money and job action is checked on the server (distance to the ATM, pump or
drop-off, balance, chat rate). Movement is client-driven for now (see the roadmap).

## Opening the project

1. Install **Unity 6000.0.58f2** (Unity 6 LTS) with **Android Build Support**
   and **Linux Build Support (Mono)** from Unity Hub.
2. Unity Hub → Add → choose this `unity-client` folder.
3. On first open the editor runs **Kingmakers → Run Project Setup** by itself. It
   creates the URP asset, materials, the `Player` and `Car` network prefabs, the
   `Main` scene and the Android player settings. Run it again from the menu any time.
4. Open `Assets/Kingmakers/Scenes/Main.unity` and press **Play**, then
   **Play offline on this device** to try the game without a server.

Desktop controls: WASD move/drive, Shift run, Space brake, E action, F enter/exit,
T chat, P phone, right mouse drag to look.

## Running the server

Build `Kingmakers.EditorTools.BuildScript.BuildLinuxServer` (or take the CI
artifact) and start it on the VPS:

```bash
GAME_SERVER_KEY=<same key as the backend> ./KingsportServer.x86_64 \
    -batchmode -nographics -port 7778 -backend http://127.0.0.1:8080
```

Open UDP 7778 in the firewall. `-offline` lets anyone in as a guest without the
backend (testing only, nothing is saved).

Backend environment variables (`km-backend` service):

| Variable | Purpose |
|---|---|
| `GAME_TOKEN_SECRET` | Signs login tokens. Set a long random value, or every login dies on restart. |
| `GAME_SERVER_KEY` | Shared secret between backend and game server. The server endpoints are off without it. |
| `GAME_HOST` / `GAME_PORT` | Address the client connects to (default: `PUBLIC_HOST`, 7778). |
| `GAME_DATA_FILE` | Account store (default `backend/data/game.json`). |

## CI builds

`.github/workflows/unity-client.yml` builds the APK and the Linux server with
[GameCI](https://game.ci) on every push to `main` that touches `unity-client/`.
It needs three repository secrets (Settings → Secrets and variables → Actions):

- `UNITY_EMAIL`, `UNITY_PASSWORD`: your Unity ID.
- `UNITY_LICENSE`: the contents of your `Unity_lic.ulf` file. Activate Unity
  Personal once in Unity Hub on any computer, then copy the file from
  `C:\ProgramData\Unity\` (Windows), `/Library/Application Support/Unity/` (macOS)
  or `~/.local/share/unity3d/Unity/` (Linux).

Without them the workflow skips the build and leaves a notice.

## Layout

```
Assets/Kingmakers/
  Editor/        ProjectSetup (assets, prefabs, scene, settings), BuildScript (CI)
  Resources/UI/  Main.uxml, Main.uss, RuntimeTheme.tss
  Scripts/Core      GameBootstrap (server/client start), GameConfig, BackendClient, GameInput
  Scripts/Net       ServerSessions (login check), OwnerNetworkTransform
  Scripts/World     CityLayout (data), CityBuilder (geometry), Materials
  Scripts/Player    PlayerState (money, jobs, chat RPCs), PlayerMotor, CameraRig
  Scripts/Vehicles  Vehicle
  Scripts/Gameplay  GameServer (parked cars, payday, autosave)
  Scripts/UI        UIRoot, LoginScreen, Hud, Phone
```

The roadmap to a full game is in [`docs/ROADMAP.md`](../docs/ROADMAP.md).
