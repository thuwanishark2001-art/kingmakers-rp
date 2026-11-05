// =====================================================================
//  Kingmakers RP - database schema & account system
// =====================================================================

stock DB_Exec(const query[])
{
    new DBResult:r = DB_ExecuteQuery(gDB, query);
    if (r) DB_FreeResultSet(r);
}

stock InitDatabase()
{
    gDB = DB_Open(DATABASE_FILE);
    if (!gDB)
    {
        print("[KM] FATAL: could not open database " DATABASE_FILE);
        return 0;
    }
    DB_Exec("PRAGMA journal_mode = WAL");
    DB_Exec("PRAGMA synchronous = NORMAL");
    DB_Exec("CREATE TABLE IF NOT EXISTS accounts (\
        id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT UNIQUE COLLATE NOCASE, pass TEXT, salt TEXT, ip TEXT,\
        regdate INTEGER DEFAULT 0, lastlogin INTEGER DEFAULT 0, money INTEGER DEFAULT 0, bank INTEGER DEFAULT 0,\
        level INTEGER DEFAULT 1, exp INTEGER DEFAULT 0, skin INTEGER DEFAULT 26, gender INTEGER DEFAULT 0, age INTEGER DEFAULT 18,\
        x REAL DEFAULT 0, y REAL DEFAULT 0, z REAL DEFAULT 0, a REAL DEFAULT 0, interior INTEGER DEFAULT 0, vw INTEGER DEFAULT 0,\
        health REAL DEFAULT 100, armour REAL DEFAULT 0, job INTEGER DEFAULT 0, faction INTEGER DEFAULT 0, rank INTEGER DEFAULT 0,\
        admin INTEGER DEFAULT 0, phone INTEGER DEFAULT 0, wanted INTEGER DEFAULT 0, jail INTEGER DEFAULT 0, mute INTEGER DEFAULT 0,\
        playmin INTEGER DEFAULT 0, paymin INTEGER DEFAULT 0, drivelic INTEGER DEFAULT 0, gunlic INTEGER DEFAULT 0,\
        medkits INTEGER DEFAULT 0, food INTEGER DEFAULT 0)");
    DB_Exec("CREATE TABLE IF NOT EXISTS vehicles (id INTEGER PRIMARY KEY AUTOINCREMENT, owner INTEGER, model INTEGER,\
        x REAL, y REAL, z REAL, a REAL, c1 INTEGER, c2 INTEGER, fuel INTEGER DEFAULT 100, locked INTEGER DEFAULT 0)");
    DB_Exec("CREATE TABLE IF NOT EXISTS houses (id INTEGER PRIMARY KEY AUTOINCREMENT, owner INTEGER DEFAULT 0, price INTEGER,\
        x REAL, y REAL, z REAL, itype INTEGER, locked INTEGER DEFAULT 0, safe INTEGER DEFAULT 0)");
    DB_Exec("CREATE TABLE IF NOT EXISTS businesses (id INTEGER PRIMARY KEY AUTOINCREMENT, owner INTEGER DEFAULT 0, type INTEGER,\
        price INTEGER, x REAL, y REAL, z REAL, till INTEGER DEFAULT 0)");
    DB_Exec("CREATE TABLE IF NOT EXISTS contacts (id INTEGER PRIMARY KEY AUTOINCREMENT, owner INTEGER, name TEXT, number INTEGER)");
    DB_Exec("CREATE TABLE IF NOT EXISTS sms (id INTEGER PRIMARY KEY AUTOINCREMENT, from_num INTEGER, to_num INTEGER, text TEXT,\
        time INTEGER, seen INTEGER DEFAULT 0)");
    DB_Exec("CREATE TABLE IF NOT EXISTS bans (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT COLLATE NOCASE, ip TEXT, reason TEXT,\
        admin TEXT, time INTEGER)");
    DB_Exec("CREATE TABLE IF NOT EXISTS turfs (id INTEGER PRIMARY KEY, faction INTEGER)");
    print("[KM] Database ready: " DATABASE_FILE);
    return 1;
}

// ---------------------------------------------------------------------
//  Connection / login flow
// ---------------------------------------------------------------------
stock ResetPlayerData(playerid)
{
    new blank[E_PLAYER];
    Player[playerid] = blank;
    Player[playerid][pLevel] = 1;
    Player[playerid][pCallWith] = INVALID_PLAYER_ID;
    Player[playerid][pCallerOf] = INVALID_PLAYER_ID;
    Player[playerid][pSpecTarget] = INVALID_PLAYER_ID;
    Player[playerid][pLastPM] = INVALID_PLAYER_ID;
    Player[playerid][pTransferTarget] = INVALID_PLAYER_ID;
    Player[playerid][pJobVeh] = INVALID_VEHICLE_ID;
    Player[playerid][pAdminVeh] = INVALID_VEHICLE_ID;
    Player[playerid][pHouseInside] = -1;
    Player[playerid][pBizInside] = -1;
    Player[playerid][pPhoneOn] = true;
    GetPlayerName(playerid, Player[playerid][pName], MAX_PLAYER_NAME);
}

stock bool:CheckBanned(playerid)
{
    new ip[16], q[256], esc[MAX_PLAYER_NAME * 2];
    GetPlayerIp(playerid, ip, sizeof ip);
    SQLEscape(GetName(playerid), esc);
    format(q, sizeof q, "SELECT reason, admin FROM bans WHERE name = '%s' OR ip = '%s' LIMIT 1", esc, ip);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    if (r && DB_GetRowCount(r) > 0)
    {
        new reason[64], admin[MAX_PLAYER_NAME];
        DB_GetFieldStringByName(r, "reason", reason);
        DB_GetFieldStringByName(r, "admin", admin);
        DB_FreeResultSet(r);
        SendClientMessage(playerid, COLOR_RED, "You are banned from " SERVER_NAME ". Reason: %s (by %s)", reason, admin);
        SendClientMessage(playerid, COLOR_GREY, "Appeal on " SERVER_WEBSITE);
        SetTimerEx("DelayedKick", 500, false, "i", playerid);
        return true;
    }
    if (r) DB_FreeResultSet(r);
    return false;
}

forward DelayedKick(playerid);
public DelayedKick(playerid)
{
    Kick(playerid);
    return 1;
}

forward ShowLoginScreen(playerid);
public ShowLoginScreen(playerid)
{
    if (!IsPlayerConnected(playerid) || Player[playerid][pLogged]) return 0;
    TogglePlayerSpectating(playerid, true);
    InterpolateCameraPos(playerid, LOGIN_CAM_X, LOGIN_CAM_Y, LOGIN_CAM_Z, LOGIN_CAM_X + 250.0, LOGIN_CAM_Y + 80.0, LOGIN_CAM_Z - 30.0, 25000, CAMERA_MOVE);
    InterpolateCameraLookAt(playerid, LOGIN_LOOK_X, LOGIN_LOOK_Y, LOGIN_LOOK_Z, LOGIN_LOOK_X + 150.0, LOGIN_LOOK_Y + 100.0, LOGIN_LOOK_Z, 25000, CAMERA_MOVE);
    SetPlayerTime(playerid, 20, 30);
    SetPlayerWeather(playerid, 1);
    ShowLoginTextDraws(playerid);

    if (!IsValidRPName(GetName(playerid)))
    {
        ShowPlayerDialog(playerid, DLG_RPNAME, DIALOG_STYLE_INPUT, C_GOLD "KINGMAKERS RP " C_WHITE "- Roleplay Name",
            C_WHITE "Welcome to " C_GOLD "Kingmakers RP" C_WHITE "!\n\n\
            Your nickname must be a roleplay name in the format\n" C_GOLD "Firstname_Lastname" C_WHITE " (e.g. Marcus_King).\n\n\
            Enter your character name below:", "Continue", "Quit");
        return 1;
    }
    LookupAccount(playerid);
    return 1;
}

stock LookupAccount(playerid)
{
    new q[160], esc[MAX_PLAYER_NAME * 2];
    SQLEscape(GetName(playerid), esc);
    format(q, sizeof q, "SELECT id, pass, salt FROM accounts WHERE name = '%s' LIMIT 1", esc);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    if (r && DB_GetRowCount(r) > 0)
    {
        Player[playerid][pID] = DB_GetFieldIntByName(r, "id");
        DB_GetFieldStringByName(r, "pass", Player[playerid][pPassHash], 65);
        DB_GetFieldStringByName(r, "salt", Player[playerid][pSalt], 17);
        Player[playerid][pRegistered] = true;
        DB_FreeResultSet(r);
        ShowLoginDialog(playerid, "");
    }
    else
    {
        if (r) DB_FreeResultSet(r);
        Player[playerid][pRegistered] = false;
        ShowRegisterDialog(playerid, "");
    }
}

stock ShowLoginDialog(playerid, const err[])
{
    ShowPlayerDialog(playerid, DLG_LOGIN, DIALOG_STYLE_PASSWORD, C_GOLD "KINGMAKERS RP " C_WHITE "- Login",
        C_WHITE "Welcome back, " C_GOLD "%s" C_WHITE "!\n\nThis account is registered. Enter your password to log in.\n%s",
        "Log In", "Quit", RPName(playerid), err);
}

stock ShowRegisterDialog(playerid, const err[])
{
    ShowPlayerDialog(playerid, DLG_REGISTER, DIALOG_STYLE_PASSWORD, C_GOLD "KINGMAKERS RP " C_WHITE "- Create Account",
        C_WHITE "Welcome to " C_GOLD "Kingmakers RP" C_WHITE ", %s!\n\n\
        Rule the streets, build your empire, become a Kingmaker.\n\n\
        Choose a password (6-32 characters) to create your account.\n%s",
        "Register", "Quit", RPName(playerid), err);
}

stock HashPassword(const password[], const salt[], out[], size = sizeof out)
{
    SHA256_PassHash(password, salt, out, size);
}

// Returns 1 if the dialog was handled by the account module.
stock Account_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid)
    {
        case DLG_RPNAME:
        {
            if (!response) return Kick(playerid), 1;
            if (!IsValidRPName(inputtext))
            {
                ShowPlayerDialog(playerid, DLG_RPNAME, DIALOG_STYLE_INPUT, C_GOLD "KINGMAKERS RP " C_WHITE "- Roleplay Name",
                    C_RED "Invalid name!\n\n" C_WHITE "Use the format " C_GOLD "Firstname_Lastname" C_WHITE ":\n\
                    - Only letters and one underscore\n- Both names start with a capital letter\n- Each part at least 3 letters",
                    "Continue", "Quit");
                return 1;
            }
            for (new i = 0; i < MAX_PLAYERS; i++)
            {
                if (i != playerid && IsPlayerConnected(i) && !strcmp(GetName(i), inputtext, true))
                {
                    ShowPlayerDialog(playerid, DLG_RPNAME, DIALOG_STYLE_INPUT, C_GOLD "KINGMAKERS RP " C_WHITE "- Roleplay Name",
                        C_RED "That name is currently online. " C_WHITE "Choose another Firstname_Lastname:", "Continue", "Quit");
                    return 1;
                }
            }
            SetPlayerName(playerid, inputtext);
            GetPlayerName(playerid, Player[playerid][pName], MAX_PLAYER_NAME);
            if (CheckBanned(playerid)) return 1;
            LookupAccount(playerid);
            return 1;
        }
        case DLG_LOGIN:
        {
            if (!response) return Kick(playerid), 1;
            new hash[65];
            HashPassword(inputtext, Player[playerid][pSalt], hash);
            if (strcmp(hash, Player[playerid][pPassHash], false) != 0 || !inputtext[0])
            {
                Player[playerid][pLoginTries]++;
                if (Player[playerid][pLoginTries] >= 3)
                {
                    SendClientMessage(playerid, COLOR_RED, "Too many wrong passwords. You have been kicked.");
                    SetTimerEx("DelayedKick", 500, false, "i", playerid);
                    return 1;
                }
                new err[64];
                format(err, sizeof err, "\n" C_RED "Wrong password! Attempts left: %d", 3 - Player[playerid][pLoginTries]);
                ShowLoginDialog(playerid, err);
                return 1;
            }
            LoadAccount(playerid);
            return 1;
        }
        case DLG_REGISTER:
        {
            if (!response) return Kick(playerid), 1;
            new len = strlen(inputtext);
            if (len < 6 || len > 32)
            {
                ShowRegisterDialog(playerid, "\n" C_RED "Password must be 6-32 characters long.");
                return 1;
            }
            RandomString(Player[playerid][pSalt], 16);
            HashPassword(inputtext, Player[playerid][pSalt], Player[playerid][pPassHash], 65);
            ShowPlayerDialog(playerid, DLG_REG_GENDER, DIALOG_STYLE_LIST, C_GOLD "Character Creation " C_WHITE "- Gender",
                "Male\nFemale", "Next", "");
            return 1;
        }
        case DLG_REG_GENDER:
        {
            Player[playerid][pGender] = (response && listitem == 1) ? 1 : 0;
            ShowPlayerDialog(playerid, DLG_REG_AGE, DIALOG_STYLE_INPUT, C_GOLD "Character Creation " C_WHITE "- Age",
                C_WHITE "How old is your character? (16 - 80)", "Next", "");
            return 1;
        }
        case DLG_REG_AGE:
        {
            new age = strval(inputtext);
            if (!response || age < 16 || age > 80)
            {
                ShowPlayerDialog(playerid, DLG_REG_AGE, DIALOG_STYLE_INPUT, C_GOLD "Character Creation " C_WHITE "- Age",
                    C_RED "Invalid age.\n" C_WHITE "How old is your character? (16 - 80)", "Next", "");
                return 1;
            }
            Player[playerid][pAge] = age;
            ShowSkinDialog(playerid);
            return 1;
        }
        case DLG_REG_SKIN:
        {
            if (!response) listitem = 0;
            Player[playerid][pSkin] = GetStarterSkin(Player[playerid][pGender], listitem);
            CreateAccount(playerid);
            return 1;
        }
    }
    return 0;
}

new const MaleSkins[] = { 26, 29, 47, 60, 101, 170, 184, 240, 250, 299 };
new const FemaleSkins[] = { 12, 40, 55, 56, 69, 91, 93, 192, 193, 233 };

stock GetStarterSkin(gender, idx)
{
    if (gender == 1) return FemaleSkins[(idx >= 0 && idx < sizeof FemaleSkins) ? idx : 0];
    return MaleSkins[(idx >= 0 && idx < sizeof MaleSkins) ? idx : 0];
}

stock ShowSkinDialog(playerid)
{
    new body[512];
    body = "Skin ID\tStyle\n";
    if (Player[playerid][pGender] == 1)
    {
        new const styles[][] = { "Classy", "Street", "Casual", "Business", "Sporty", "Downtown", "Beach", "Elegant", "Urban", "Night out" };
        for (new i = 0; i < sizeof FemaleSkins; i++) format(body, sizeof body, "%s%d\t%s\n", body, FemaleSkins[i], styles[i]);
    }
    else
    {
        new const styles[][] = { "Backpacker", "Street", "Casual", "Hipster", "Hoodie", "Tourist", "Chill", "Businessman", "Surfer", "Leather" };
        for (new i = 0; i < sizeof MaleSkins; i++) format(body, sizeof body, "%s%d\t%s\n", body, MaleSkins[i], styles[i]);
    }
    ShowPlayerDialog(playerid, DLG_REG_SKIN, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "Character Creation " C_WHITE "- Appearance", body, "Create", "");
}

stock CreateAccount(playerid)
{
    new q[512], esc[MAX_PLAYER_NAME * 2], ip[16];
    SQLEscape(GetName(playerid), esc);
    GetPlayerIp(playerid, ip, sizeof ip);
    format(q, sizeof q, "INSERT INTO accounts (name, pass, salt, ip, regdate, lastlogin, money, bank, skin, gender, age, x, y, z, a) \
        VALUES ('%s', '%s', '%s', '%s', %d, %d, %d, %d, %d, %d, %d, %f, %f, %f, %f)",
        esc, Player[playerid][pPassHash], Player[playerid][pSalt], ip, gettime(), gettime(), START_CASH, START_BANK,
        Player[playerid][pSkin], Player[playerid][pGender], Player[playerid][pAge], SPAWN_X, SPAWN_Y, SPAWN_Z, SPAWN_A);
    DB_Exec(q);

    new DBResult:r = DB_ExecuteQuery(gDB, "SELECT last_insert_rowid()");
    if (r) { Player[playerid][pID] = DB_GetFieldInt(r, 0); DB_FreeResultSet(r); }

    // First account on a fresh server becomes the owner (admin 5).
    r = DB_ExecuteQuery(gDB, "SELECT COUNT(*) FROM accounts");
    if (r)
    {
        if (DB_GetFieldInt(r, 0) == 1)
        {
            format(q, sizeof q, "UPDATE accounts SET admin = 5 WHERE id = %d", Player[playerid][pID]);
            DB_Exec(q);
        }
        DB_FreeResultSet(r);
    }

    SendClientMessage(playerid, COLOR_GOLD, "==============================================");
    SendClientMessage(playerid, COLOR_WHITE, "  Welcome to {FFC83D}Kingmakers RP{FFFFFF}, %s!", RPName(playerid));
    SendClientMessage(playerid, COLOR_WHITE, "  You arrived at Unity Station with %s cash and %s in the bank.", FormatMoney(START_CASH), FormatMoney(START_BANK));
    SendClientMessage(playerid, COLOR_WHITE, "  Visit the {FFC83D}City Hall{FFFFFF} to get a job and licenses. Use {FFC83D}/help{FFFFFF} for commands.");
    SendClientMessage(playerid, COLOR_GOLD, "==============================================");
    LoadAccount(playerid);
}

stock LoadAccount(playerid)
{
    new q[128];
    format(q, sizeof q, "SELECT * FROM accounts WHERE id = %d", Player[playerid][pID]);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    if (!r || DB_GetRowCount(r) == 0)
    {
        if (r) DB_FreeResultSet(r);
        Error(playerid, "Account could not be loaded. Please reconnect.");
        SetTimerEx("DelayedKick", 500, false, "i", playerid);
        return;
    }
    Player[playerid][pMoney]      = DB_GetFieldIntByName(r, "money");
    Player[playerid][pBank]       = DB_GetFieldIntByName(r, "bank");
    Player[playerid][pLevel]      = DB_GetFieldIntByName(r, "level");
    Player[playerid][pExp]        = DB_GetFieldIntByName(r, "exp");
    Player[playerid][pSkin]       = DB_GetFieldIntByName(r, "skin");
    Player[playerid][pGender]     = DB_GetFieldIntByName(r, "gender");
    Player[playerid][pAge]        = DB_GetFieldIntByName(r, "age");
    Player[playerid][pPosX]       = DB_GetFieldFloatByName(r, "x");
    Player[playerid][pPosY]       = DB_GetFieldFloatByName(r, "y");
    Player[playerid][pPosZ]       = DB_GetFieldFloatByName(r, "z");
    Player[playerid][pPosA]       = DB_GetFieldFloatByName(r, "a");
    Player[playerid][pInt]        = DB_GetFieldIntByName(r, "interior");
    Player[playerid][pVW]         = DB_GetFieldIntByName(r, "vw");
    Player[playerid][pHealth]     = DB_GetFieldFloatByName(r, "health");
    Player[playerid][pArmour]     = DB_GetFieldFloatByName(r, "armour");
    Player[playerid][pJob]        = DB_GetFieldIntByName(r, "job");
    Player[playerid][pFaction]    = DB_GetFieldIntByName(r, "faction");
    Player[playerid][pRank]       = DB_GetFieldIntByName(r, "rank");
    Player[playerid][pAdmin]      = DB_GetFieldIntByName(r, "admin");
    Player[playerid][pPhone]      = DB_GetFieldIntByName(r, "phone");
    Player[playerid][pWanted]     = DB_GetFieldIntByName(r, "wanted");
    Player[playerid][pJailTime]   = DB_GetFieldIntByName(r, "jail");
    Player[playerid][pMuteTime]   = DB_GetFieldIntByName(r, "mute");
    Player[playerid][pPlayMinutes]= DB_GetFieldIntByName(r, "playmin");
    Player[playerid][pPayMinutes] = DB_GetFieldIntByName(r, "paymin");
    Player[playerid][pDriveLic]   = DB_GetFieldIntByName(r, "drivelic");
    Player[playerid][pGunLic]     = DB_GetFieldIntByName(r, "gunlic");
    Player[playerid][pMedkits]    = DB_GetFieldIntByName(r, "medkits");
    Player[playerid][pFood]       = DB_GetFieldIntByName(r, "food");
    DB_FreeResultSet(r);

    if (Player[playerid][pHealth] < 10.0) Player[playerid][pHealth] = 50.0;
    if (Player[playerid][pFaction] < 0 || Player[playerid][pFaction] >= MAX_FACTIONS) Player[playerid][pFaction] = 0;
    if (Player[playerid][pJob] < 0 || Player[playerid][pJob] >= MAX_JOBS) Player[playerid][pJob] = 0;

    new ip[16];
    GetPlayerIp(playerid, ip, sizeof ip);
    format(q, sizeof q, "UPDATE accounts SET lastlogin = %d, ip = '%s' WHERE id = %d", gettime(), ip, Player[playerid][pID]);
    DB_Exec(q);

    Player[playerid][pLogged] = true;
    HideLoginTextDraws(playerid);
    SyncMoney(playerid);
    SetPlayerScore(playerid, Player[playerid][pLevel]);
    ApplyPlayerColor(playerid);

    LoadPlayerVehicles(playerid);
    CheckUnreadSMS(playerid);

    new Float:sx = Player[playerid][pPosX], Float:sy = Player[playerid][pPosY], Float:sz = Player[playerid][pPosZ];
    if (sx == 0.0 && sy == 0.0) { sx = SPAWN_X; sy = SPAWN_Y; sz = SPAWN_Z; }
    SetSpawnInfo(playerid, NO_TEAM, Player[playerid][pSkin], sx, sy, sz, Player[playerid][pPosA]);
    TogglePlayerSpectating(playerid, false);
    // Spawn is triggered by un-spectating (OnPlayerSpawn handles the rest).

    SendClientMessage(playerid, COLOR_GOLD, "[KM] {FFFFFF}Logged in as {FFC83D}%s{FFFFFF}. Level %d | Cash %s | Bank %s",
        RPName(playerid), Player[playerid][pLevel], FormatMoney(Player[playerid][pMoney]), FormatMoney(Player[playerid][pBank]));
    if (Player[playerid][pAdmin] > 0)
        SendClientMessage(playerid, COLOR_ADMIN, "[ADMIN] You are logged in as a level %d administrator. /ahelp", Player[playerid][pAdmin]);

    new msg[128];
    format(msg, sizeof msg, "%s has joined Kingmakers RP.", RPName(playerid));
    for (new i = 0; i < MAX_PLAYERS; i++)
        if (i != playerid && IsPlayerConnected(i) && Player[i][pLogged]) SendClientMessage(i, COLOR_GREY, "%s", msg);
}

stock SavePlayer(playerid)
{
    if (!Player[playerid][pLogged] || Player[playerid][pID] == 0) return;

    // Capture live position unless the player is injured / spectating.
    if (GetPlayerState(playerid) != PLAYER_STATE_SPECTATING && !Player[playerid][pInjured])
    {
        GetPlayerPos(playerid, Player[playerid][pPosX], Player[playerid][pPosY], Player[playerid][pPosZ]);
        GetPlayerFacingAngle(playerid, Player[playerid][pPosA]);
        Player[playerid][pInt] = GetPlayerInterior(playerid);
        Player[playerid][pVW] = GetPlayerVirtualWorld(playerid);
        GetPlayerHealth(playerid, Player[playerid][pHealth]);
        GetPlayerArmour(playerid, Player[playerid][pArmour]);
    }
    if (Player[playerid][pInjured])
    {
        Player[playerid][pPosX] = HOSPITAL_X; Player[playerid][pPosY] = HOSPITAL_Y; Player[playerid][pPosZ] = HOSPITAL_Z;
        Player[playerid][pInt] = 0; Player[playerid][pVW] = 0; Player[playerid][pHealth] = 50.0;
    }

    new q[1024];
    format(q, sizeof q, "UPDATE accounts SET money=%d, bank=%d, level=%d, exp=%d, skin=%d, x=%f, y=%f, z=%f, a=%f, interior=%d, vw=%d, \
        health=%f, armour=%f, job=%d, faction=%d, rank=%d, admin=%d, phone=%d, wanted=%d, jail=%d, mute=%d, playmin=%d, paymin=%d, \
        drivelic=%d, gunlic=%d, medkits=%d, food=%d WHERE id=%d",
        Player[playerid][pMoney], Player[playerid][pBank], Player[playerid][pLevel], Player[playerid][pExp], Player[playerid][pSkin],
        Player[playerid][pPosX], Player[playerid][pPosY], Player[playerid][pPosZ], Player[playerid][pPosA],
        Player[playerid][pInt], Player[playerid][pVW], Player[playerid][pHealth], Player[playerid][pArmour],
        Player[playerid][pJob], Player[playerid][pFaction], Player[playerid][pRank], Player[playerid][pAdmin],
        Player[playerid][pPhone], Player[playerid][pWanted], Player[playerid][pJailTime], Player[playerid][pMuteTime],
        Player[playerid][pPlayMinutes], Player[playerid][pPayMinutes], Player[playerid][pDriveLic], Player[playerid][pGunLic],
        Player[playerid][pMedkits], Player[playerid][pFood], Player[playerid][pID]);
    DB_Exec(q);
}

stock ApplyPlayerColor(playerid)
{
    new f = Player[playerid][pFaction];
    if ((f == FACTION_LSPD || f == FACTION_EMS) && Player[playerid][pOnDuty]) SetPlayerColor(playerid, FactionColors[f]);
    else if (f == FACTION_GROVE || f == FACTION_BALLAS) SetPlayerColor(playerid, (FactionColors[f] & 0xFFFFFF00) | 0x80);
    else SetPlayerColor(playerid, 0xFFFFFF60);
}

// ---------------------------------------------------------------------
//  Spawn handling
// ---------------------------------------------------------------------
stock Account_OnSpawn(playerid)
{
    SetPlayerSkin(playerid, Player[playerid][pSkin]);
    SetPlayerInterior(playerid, Player[playerid][pInt]);
    SetPlayerVirtualWorld(playerid, Player[playerid][pVW]);
    SetPlayerHealth(playerid, Player[playerid][pHealth] > 0.0 ? Player[playerid][pHealth] : 100.0);
    SetPlayerArmour(playerid, Player[playerid][pArmour]);
    SetPlayerWantedLevel(playerid, Player[playerid][pWanted]);
    SetCameraBehindPlayer(playerid);
    SetPlayerTime(playerid, gServerHour, 0);
    SetPlayerWeather(playerid, gServerWeather);
    ShowHUD(playerid);

    if (Player[playerid][pJailTime] > 0)
    {
        PutInJail(playerid);
        return;
    }
    if (Player[playerid][pInjured])
    {
        SetPlayerPos(playerid, Player[playerid][pPosX], Player[playerid][pPosY], Player[playerid][pPosZ]);
        SetPlayerInterior(playerid, Player[playerid][pInt]);
        SetPlayerVirtualWorld(playerid, Player[playerid][pVW]);
        SetPlayerHealth(playerid, 15.0);
        TogglePlayerControllable(playerid, false);
        ApplyAnimation(playerid, "CRACK", "crckdeth2", 4.1, true, false, false, true, 0, SYNC_ALL);
        SendClientMessage(playerid, COLOR_RED, "You are critically injured! Wait for EMS (they were notified) or use /acceptdeath after %d seconds.", INJURED_SECONDS);
        new msg[128];
        format(msg, sizeof msg, "[DISPATCH] Injured person reported: %s (ID %d). Use /revive near them.", RPName(playerid), playerid);
        SendFactionMessage(FACTION_EMS, FactionColors[FACTION_EMS], msg);
        return;
    }
    if (Player[playerid][pOnDuty] && FactionDutySkins[Player[playerid][pFaction]] != 0)
        SetPlayerSkin(playerid, FactionDutySkins[Player[playerid][pFaction]]);
}
