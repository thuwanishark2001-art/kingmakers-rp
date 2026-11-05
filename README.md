# Kingmakers RP — Mobile Roleplay Stack (v1.0.0)

[![Build Status](https://img.shields.io/badge/status-active-success.svg)]()
[![Platform](https://img.shields.io/badge/platform-Android%20%7C%20CRMP%20%7C%20open.mp-gold.svg)]()
[![Server](https://img.shields.io/badge/server-139.180.185.57%3A7777-blue.svg)]()
[![Web Portal](https://img.shields.io/badge/portal-8080-purple.svg)]()

Complete, original mobile multiplayer roleplay game stack built on modern **open.mp / CRMP** architecture — the exact same technical foundation used by mobile RP giants like **Grand Mobile RP, Black Russia, and One State RP**.

---

## 🎮 Server Details

- **Public IP:** `139.180.185.57`
- **Game Port:** `7777 UDP` (open.mp)
- **Web Dashboard:** `http://139.180.185.57:8080`
- **Database:** SQLite3 with WAL journal mode (`kingmakers.db`)

---

## 🏛 Systems & Features

1. **Roleplay Accounts & Auth:**
   - Strict `Firstname_Lastname` character validation
   - SHA-256 hashed password security with individual player salts
   - Interactive cinematic camera intro over Los Santos sunset
   - Character creation: Gender, Age, and 10 Starter Skins
   - Automatic 5-minute database autosaves

2. **Economy & Banking:**
   - Cash and Bank balance system
   - 8 City ATM locations with balance, deposit, withdraw, and direct wire transfers
   - 30-minute Payday cycles with salary, faction pay, interest, and taxes
   - Respect points and Level Up mechanics
   - City Hall licenses: Driving and Weapon licenses

3. **Vehicles & Dealership:**
   - Kingmakers Motors dealership with 20 purchasable vehicles
   - Persistent owned vehicles with custom spawn locations (`/park`)
   - Manual engine toggle (`/engine` or `N` key) and doors lock (`/lock` or `Y` key)
   - Real-time speedometer textdraw with live fuel gauge, engine status, and HP
   - Gas stations throughout the city with `/refuel`

4. **Civilian Jobs:**
   - **Taxi Driver:** Dispatches NPC fares or set custom player fares with `/fare`
   - **Ocean Docks Trucker:** Cargo transport with condition-based payouts
   - **Hunter Quarry Miner:** Physical rock mining animation, carrying ore to conveyor, random gold nugget rewards
   - **Mechanic:** Roadside assistance with `/repair`, `/refill`, and player `/accept` service requests

5. **Factions & Law Enforcement:**
   - **LSPD:** Duty locker, `/su` (crime reports), `/cuff`, `/arrest` with timed jail sentences, `/ticket`, megaphone `/m`
   - **EMS:** Paramedic duty, roadside treatment `/heal`, defibrillator `/revive` bonus, and hospital care
   - **Gangs:** Grove Street Families and Ballas fighting for control of 4 territories (Ganton, Idlewood, Glen Park, Jefferson) via `/capture`

6. **KingPhone Mobile Device:**
   - Persistent contacts list with direct calling and SMS messaging
   - 13-location GPS Navigator with active radar checkpoints
   - 911 Emergency dispatch for Police, EMS, Taxi, and Mechanics

7. **Properties & Businesses:**
   - 14 pre-seeded residential houses with private virtual worlds
   - Player-owned businesses: 24/7 stores, Ammu-Nation gun stores, and Binco clothes stores
   - Business till income: owners earn 30% revenue on every retail purchase

8. **Administration Suite:**
   - 5-Tier staff system: Helper, Moderator, Senior, Lead, and Owner
   - Commands: `/a`, `/aduty`, `/check`, `/spec`, `/slap`, `/freeze`, `/kick`, `/mute`, `/jail`, `/ban`, `/unban`, `/veh`, `/sethp`, `/setfaction`, `/setjob`, `/givemoney`, `/createhouse`, `/createbiz`, `/makeadmin`, `/broadcast`, `/restart`

---

## 📱 Mobile App (Android Client)

The `android-client/` directory contains the complete Android Studio project targeting **Android 14 (API 34)**:
- Native Landscape Launcher UI themed in Kingmakers gold & dark night city
- Real-time server status ping & online player count from the backend API
- One-touch bridge into the SA-MP / CRMP mobile client runtime
- Integrated game cache downloader with progress bar
- Full density mipmap launcher icons (`mdpi` through `xxxhdpi`) + 512x512 Play Store icon

---

## ⚡ Server Management CLI

Control your live server on Vultr with the included CLI:

```bash
# Check service status
./server-control.sh status

# Restart both game server and backend
./server-control.sh restart

# Stream live server logs
./server-control.sh logs

# Manual database backup
./server-control.sh backup
```

---

## 📄 License & Credits

All custom gamemode systems, backend APIs, mobile launcher, and UI themes created for **Kingmakers RP**.
Built on [open.mp](https://www.open.mp/).
