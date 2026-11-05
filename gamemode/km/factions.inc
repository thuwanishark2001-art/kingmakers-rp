// =====================================================================
//  Kingmakers RP - factions: LSPD, EMS, Grove Street, Ballas + turfs
// =====================================================================

enum E_TURF
{
    tID,
    tName[24],
    Float:tMinX, Float:tMinY, Float:tMaxX, Float:tMaxY,
    tZone,
    tOwner,
    tAttacker,
    tCaptureLeft
}
new Turf[4][E_TURF] =
{
    { 0, "Ganton",          2400.0, -1760.0, 2560.0, -1620.0, -1, FACTION_GROVE,  FACTION_NONE, 0 },
    { 1, "Idlewood",        1980.0, -1840.0, 2180.0, -1700.0, -1, FACTION_GROVE,  FACTION_NONE, 0 },
    { 2, "Glen Park",       1860.0, -1260.0, 2080.0, -1100.0, -1, FACTION_BALLAS, FACTION_NONE, 0 },
    { 3, "Jefferson",       2080.0, -1420.0, 2260.0, -1240.0, -1, FACTION_BALLAS, FACTION_NONE, 0 }
};

new FactionInvite[MAX_PLAYERS];
new FactionInviter[MAX_PLAYERS] = { INVALID_PLAYER_ID, ... };

stock CreateFactionPickups()
{
    CreatePickup(1247, 1, LSPD_X, LSPD_Y, LSPD_Z, -1);
    Create3DTextLabel("{3A7BFF}Los Santos Police Dept.\n{FFFFFF}/duty  -  /arrest (at garage)", COLOR_WHITE, LSPD_X, LSPD_Y, LSPD_Z + 0.7, 20.0, 0, true);
    Create3DTextLabel("{3A7BFF}LSPD Booking\n{FFFFFF}/arrest [id]", COLOR_WHITE, 1528.50, -1678.00, 5.89 + 0.7, 15.0, 0, true);

    CreatePickup(1240, 1, FactionHQ[FACTION_EMS][0], FactionHQ[FACTION_EMS][1], FactionHQ[FACTION_EMS][2], -1);
    Create3DTextLabel("{FF8282}All Saints Hospital (EMS)\n{FFFFFF}/duty  -  /heal  -  /revive", COLOR_WHITE, FactionHQ[FACTION_EMS][0], FactionHQ[FACTION_EMS][1], FactionHQ[FACTION_EMS][2] + 0.7, 20.0, 0, true);

    Create3DTextLabel("{2DB82D}Grove Street Families\n{FFFFFF}/capture to take a turf", COLOR_WHITE, FactionHQ[FACTION_GROVE][0], FactionHQ[FACTION_GROVE][1], FactionHQ[FACTION_GROVE][2] + 0.7, 20.0, 0, true);
    Create3DTextLabel("{B040E0}Ballas\n{FFFFFF}/capture to take a turf", COLOR_WHITE, FactionHQ[FACTION_BALLAS][0], FactionHQ[FACTION_BALLAS][1], FactionHQ[FACTION_BALLAS][2] + 0.7, 20.0, 0, true);
}

stock LoadTurfs()
{
    for (new i = 0; i < sizeof Turf; i++)
    {
        new q[96];
        format(q, sizeof q, "SELECT faction FROM turfs WHERE id = %d", i);
        new DBResult:r = DB_ExecuteQuery(gDB, q);
        if (r && DB_GetRowCount(r) > 0) Turf[i][tOwner] = DB_GetFieldInt(r, 0);
        else
        {
            format(q, sizeof q, "INSERT OR REPLACE INTO turfs (id, faction) VALUES (%d, %d)", i, Turf[i][tOwner]);
            DB_Exec(q);
        }
        if (r) DB_FreeResultSet(r);
        Turf[i][tZone] = GangZoneCreate(Turf[i][tMinX], Turf[i][tMinY], Turf[i][tMaxX], Turf[i][tMaxY]);
    }
}

stock ShowTurfsForPlayer(playerid)
{
    for (new i = 0; i < sizeof Turf; i++)
    {
        new col = (FactionColors[Turf[i][tOwner]] & 0xFFFFFF00) | 0x66;
        GangZoneShowForPlayer(playerid, Turf[i][tZone], col);
        if (Turf[i][tAttacker] != FACTION_NONE)
            GangZoneFlashForPlayer(playerid, Turf[i][tZone], (FactionColors[Turf[i][tAttacker]] & 0xFFFFFF00) | 0x88);
    }
}

stock GetPlayerTurf(playerid)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    for (new i = 0; i < sizeof Turf; i++)
        if (x >= Turf[i][tMinX] && x <= Turf[i][tMaxX] && y >= Turf[i][tMinY] && y <= Turf[i][tMaxY]) return i;
    return -1;
}

stock PutInJail(playerid)
{
    ResetPlayerWeapons(playerid);
    Player[playerid][pWanted] = 0;
    Player[playerid][pCuffed] = false;
    SetPlayerWantedLevel(playerid, 0);
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
    TogglePlayerControllable(playerid, true);
    SetPlayerInterior(playerid, JAIL_INT);
    SetPlayerVirtualWorld(playerid, 500);
    SetPlayerPos(playerid, JAIL_X, JAIL_Y, JAIL_Z);
}

stock ReleaseFromJail(playerid)
{
    Player[playerid][pJailTime] = 0;
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerPos(playerid, LSPD_X, LSPD_Y, LSPD_Z);
    SetCameraBehindPlayer(playerid);
    SendClientMessage(playerid, COLOR_GREEN, "You served your sentence and have been released from jail.");
    SavePlayer(playerid);
}

stock Factions_OnSecond()
{
    for (new i = 0; i < sizeof Turf; i++)
    {
        if (Turf[i][tAttacker] == FACTION_NONE) continue;
        // Count attackers still inside the turf and alive
        new alive = 0;
        for (new p = 0; p < MAX_PLAYERS; p++)
        {
            if (!IsPlayerConnected(p) || !Player[p][pLogged] || Player[p][pInjured]) continue;
            if (Player[p][pFaction] == Turf[i][tAttacker] && GetPlayerTurf(p) == i) alive++;
        }
        if (alive == 0)
        {
            new msg[128];
            format(msg, sizeof msg, "[TURF] %s failed to capture %s (no members remained in the zone).", FactionNames[Turf[i][tAttacker]], Turf[i][tName]);
            SendClientMessageToAll(COLOR_ORANGE, "%s", msg);
            Turf[i][tAttacker] = FACTION_NONE;
            Turf[i][tCaptureLeft] = 0;
            GangZoneStopFlashForAll(Turf[i][tZone]);
            continue;
        }
        Turf[i][tCaptureLeft]--;
        if (Turf[i][tCaptureLeft] <= 0)
        {
            Turf[i][tOwner] = Turf[i][tAttacker];
            Turf[i][tAttacker] = FACTION_NONE;
            new q[96], msg[128];
            format(q, sizeof q, "UPDATE turfs SET faction = %d WHERE id = %d", Turf[i][tOwner], i);
            DB_Exec(q);
            GangZoneStopFlashForAll(Turf[i][tZone]);
            GangZoneShowForAll(Turf[i][tZone], (FactionColors[Turf[i][tOwner]] & 0xFFFFFF00) | 0x66);
            format(msg, sizeof msg, "[TURF] %s has captured %s!", FactionNames[Turf[i][tOwner]], Turf[i][tName]);
            SendClientMessageToAll(COLOR_ORANGE, "%s", msg);
        }
    }
}

// ---------------------------------------------------------------------
//  Shared faction commands
// ---------------------------------------------------------------------
CMD:f(playerid, params[])
{
    new f = Player[playerid][pFaction];
    if (f == FACTION_NONE) return Error(playerid, "You are not in a faction.");
    if (isnull(params)) return Usage(playerid, "/f [message]");
    new text[128], msg[180];
    format(text, sizeof text, "%s", params);
    SanitizeText(text);
    format(msg, sizeof msg, "[%s] %s %s: %s", FactionShort[f], FactionRankNames[f][Player[playerid][pRank]], RPName(playerid), text);
    SendFactionMessage(f, COLOR_FACTION, msg);
    return 1;
}

CMD:r(playerid, params[]) return cmd_f(playerid, params);

CMD:d(playerid, params[])
{
    new f = Player[playerid][pFaction];
    if (f != FACTION_LSPD && f != FACTION_EMS) return Error(playerid, "Only LSPD and EMS can use the department radio.");
    if (isnull(params)) return Usage(playerid, "/d [department message]");
    new text[128], msg[180];
    format(text, sizeof text, "%s", params);
    SanitizeText(text);
    format(msg, sizeof msg, "[DEPT - %s] %s %s: %s", FactionShort[f], FactionRankNames[f][Player[playerid][pRank]], RPName(playerid), text);
    SendFactionMessage(FACTION_LSPD, COLOR_RADIO, msg);
    SendFactionMessage(FACTION_EMS, COLOR_RADIO, msg);
    return 1;
}

CMD:duty(playerid, params[])
{
    new f = Player[playerid][pFaction];
    if (f != FACTION_LSPD && f != FACTION_EMS) return Error(playerid, "Only LSPD and EMS members go on duty.");
    if (!IsPlayerInRangeOfPoint(playerid, 15.0, FactionHQ[f][0], FactionHQ[f][1], FactionHQ[f][2]))
        return Error(playerid, "You must be at your faction HQ to toggle duty.");
    Player[playerid][pOnDuty] = !Player[playerid][pOnDuty];
    ApplyPlayerColor(playerid);
    if (Player[playerid][pOnDuty])
    {
        SetPlayerSkin(playerid, FactionDutySkins[f]);
        SetPlayerHealth(playerid, 100.0);
        if (f == FACTION_LSPD)
        {
            SetPlayerArmour(playerid, 100.0);
            GivePlayerWeapon(playerid, WEAPON_NITESTICK, 1);
            GivePlayerWeapon(playerid, WEAPON_DEAGLE, 100);
            GivePlayerWeapon(playerid, WEAPON_SHOTGUN, 50);
            GivePlayerWeapon(playerid, WEAPON_MP5, 200);
        }
        new msg[128];
        format(msg, sizeof msg, "[%s] %s %s is now on duty.", FactionShort[f], FactionRankNames[f][Player[playerid][pRank]], RPName(playerid));
        SendFactionMessage(f, FactionColors[f], msg);
    }
    else
    {
        ResetPlayerWeapons(playerid);
        SetPlayerArmour(playerid, 0.0);
        SetPlayerSkin(playerid, Player[playerid][pSkin]);
        Info(playerid, "You are now off duty.");
    }
    return 1;
}

CMD:invite(playerid, params[])
{
    new f = Player[playerid][pFaction];
    if (f == FACTION_NONE || Player[playerid][pRank] < 4) return Error(playerid, "Only faction leaders (rank 4+) can invite.");
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || target == playerid) return Usage(playerid, "/invite [id/name]");
    if (Player[target][pFaction] != FACTION_NONE) return Error(playerid, "That player is already in a faction.");
    FactionInvite[target] = f;
    FactionInviter[target] = playerid;
    SendClientMessage(target, COLOR_LIGHTBLUE, "%s invited you to join %s. Type /acceptinvite to join.", RPName(playerid), FactionNames[f]);
    SendClientMessage(playerid, COLOR_LIGHTBLUE, "You invited %s to join %s.", RPName(target), FactionNames[f]);
    return 1;
}

CMD:acceptinvite(playerid, params[])
{
    new f = FactionInvite[playerid];
    if (f == FACTION_NONE) return Error(playerid, "You have no faction invitation.");
    FactionInvite[playerid] = FACTION_NONE;
    Player[playerid][pFaction] = f;
    Player[playerid][pRank] = 0;
    ApplyPlayerColor(playerid);
    new msg[128];
    format(msg, sizeof msg, "[%s] %s has joined the faction!", FactionShort[f], RPName(playerid));
    SendFactionMessage(f, FactionColors[f], msg);
    SavePlayer(playerid);
    return 1;
}

CMD:uninvite(playerid, params[])
{
    new f = Player[playerid][pFaction];
    if (f == FACTION_NONE || Player[playerid][pRank] < 4) return Error(playerid, "Only faction leaders (rank 4+) can kick members.");
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || target == playerid) return Usage(playerid, "/uninvite [id/name]");
    if (Player[target][pFaction] != f) return Error(playerid, "That player is not in your faction.");
    Player[target][pFaction] = FACTION_NONE;
    Player[target][pRank] = 0;
    Player[target][pOnDuty] = false;
    SetPlayerSkin(target, Player[target][pSkin]);
    ApplyPlayerColor(target);
    SendClientMessage(target, COLOR_RED, "You were removed from %s by %s.", FactionNames[f], RPName(playerid));
    SendClientMessage(playerid, COLOR_YELLOW, "You removed %s from the faction.", RPName(target));
    SavePlayer(target);
    return 1;
}

CMD:giverank(playerid, params[])
{
    new f = Player[playerid][pFaction], rank;
    if (f == FACTION_NONE || Player[playerid][pRank] < 5) return Error(playerid, "Only the faction leader (rank 5) can change ranks.");
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, rank) || rank < 0 || rank > 4) return Usage(playerid, "/giverank [id/name] [0-4]");
    if (Player[target][pFaction] != f) return Error(playerid, "That player is not in your faction.");
    Player[target][pRank] = rank;
    SendClientMessage(target, COLOR_LIGHTBLUE, "%s set your rank to %s (%d).", RPName(playerid), FactionRankNames[f][rank], rank);
    SendClientMessage(playerid, COLOR_LIGHTBLUE, "You set %s's rank to %s.", RPName(target), FactionRankNames[f][rank]);
    SavePlayer(target);
    return 1;
}

// ---------------------------------------------------------------------
//  LSPD commands
// ---------------------------------------------------------------------
stock bool:IsCopOnDuty(playerid)
{
    return Player[playerid][pFaction] == FACTION_LSPD && Player[playerid][pOnDuty];
}

CMD:su(playerid, params[])
{
    if (!IsCopOnDuty(playerid)) return Error(playerid, "You must be on LSPD duty.");
    new target = ParamPlayer(params, 0), stars, reason[64];
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, stars) || !GetParamRest(params, 2, reason))
        return Usage(playerid, "/su [id/name] [stars 1-6] [reason]");
    if (stars < 1 || stars > 6) return Error(playerid, "Stars must be 1-6.");
    if (Player[target][pFaction] == FACTION_LSPD) return Error(playerid, "You cannot put wanted level on an officer.");
    Player[target][pWanted] += stars;
    if (Player[target][pWanted] > 6) Player[target][pWanted] = 6;
    SetPlayerWantedLevel(target, Player[target][pWanted]);
    SendClientMessage(target, COLOR_RED, "Officer %s reported you for: %s (+%d stars, total %d).", RPName(playerid), reason, stars, Player[target][pWanted]);
    new msg[160];
    format(msg, sizeof msg, "[DISPATCH] %s reported %s (%d): %s (wanted %d).", RPName(playerid), RPName(target), target, reason, Player[target][pWanted]);
    SendFactionMessage(FACTION_LSPD, FactionColors[FACTION_LSPD], msg);
    return 1;
}

CMD:wanted(playerid, params[])
{
    if (Player[playerid][pFaction] != FACTION_LSPD) return Error(playerid, "LSPD only.");
    SendClientMessage(playerid, FactionColors[FACTION_LSPD], "___ Wanted list ___");
    new count = 0;
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Player[i][pLogged] && Player[i][pWanted] > 0)
        {
            SendClientMessage(playerid, COLOR_WHITE, "  %s (ID %d) - %d stars", RPName(i), i, Player[i][pWanted]);
            count++;
        }
    }
    if (!count) SendClientMessage(playerid, COLOR_GREY, "  No wanted suspects online.");
    return 1;
}

CMD:cuff(playerid, params[])
{
    if (!IsCopOnDuty(playerid)) return Error(playerid, "You must be on LSPD duty.");
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || target == playerid) return Usage(playerid, "/cuff [id/name]");
    if (!IsNearPlayer(playerid, target, 4.0) || IsPlayerInAnyVehicle(target)) return Error(playerid, "You must be close to the suspect (on foot).");
    Player[target][pCuffed] = true;
    TogglePlayerControllable(target, false);
    SetPlayerSpecialAction(target, SPECIAL_ACTION_CUFFED);
    new msg[96];
    format(msg, sizeof msg, "* Officer %s handcuffs %s.", RPName(playerid), RPName(target));
    SendNearby(playerid, 15.0, COLOR_PURPLE, msg);
    return 1;
}

CMD:uncuff(playerid, params[])
{
    if (!IsCopOnDuty(playerid)) return Error(playerid, "You must be on LSPD duty.");
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/uncuff [id/name]");
    if (!IsNearPlayer(playerid, target, 4.0)) return Error(playerid, "Too far away.");
    Player[target][pCuffed] = false;
    TogglePlayerControllable(target, true);
    SetPlayerSpecialAction(target, SPECIAL_ACTION_NONE);
    new msg[96];
    format(msg, sizeof msg, "* Officer %s removes %s's handcuffs.", RPName(playerid), RPName(target));
    SendNearby(playerid, 15.0, COLOR_PURPLE, msg);
    return 1;
}

CMD:arrest(playerid, params[])
{
    if (!IsCopOnDuty(playerid)) return Error(playerid, "You must be on LSPD duty.");
    if (!IsPlayerInRangeOfPoint(playerid, 15.0, 1528.50, -1678.00, 5.89) && !IsPlayerInRangeOfPoint(playerid, 15.0, LSPD_X, LSPD_Y, LSPD_Z))
        return Error(playerid, "Take the suspect to the LSPD garage or front desk.");
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || target == playerid) return Usage(playerid, "/arrest [id/name]");
    if (!IsNearPlayer(playerid, target, 8.0)) return Error(playerid, "The suspect is not near you.");
    if (Player[target][pWanted] <= 0) return Error(playerid, "That player is not wanted.");
    new stars = Player[target][pWanted];
    new secs = stars * JAIL_SECONDS_PER_STAR;
    new fine = stars * 300;
    Player[target][pJailTime] = secs;
    GiveMoney(target, -fine);
    GiveMoney(playerid, fine);
    PutInJail(target);
    new msg[144];
    format(msg, sizeof msg, "[LSPD] Officer %s arrested %s for %d minutes (fine %s).", RPName(playerid), RPName(target), secs / 60, FormatMoney(fine));
    SendClientMessageToAll(FactionColors[FACTION_LSPD], "%s", msg);
    SavePlayer(target);
    return 1;
}

CMD:ticket(playerid, params[])
{
    if (!IsCopOnDuty(playerid)) return Error(playerid, "You must be on LSPD duty.");
    new target = ParamPlayer(params, 0), amount, reason[64];
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, amount) || !GetParamRest(params, 2, reason))
        return Usage(playerid, "/ticket [id] [amount 50-2000] [reason]");
    if (amount < 50 || amount > 2000) return Error(playerid, "Amount must be $50-$2,000.");
    if (!IsNearPlayer(playerid, target, 5.0)) return Error(playerid, "Too far away.");
    GiveMoney(target, -amount);
    GiveMoney(playerid, amount / 2);
    SendClientMessage(target, COLOR_YELLOW, "Officer %s fined you %s for: %s", RPName(playerid), FormatMoney(amount), reason);
    SendClientMessage(playerid, COLOR_GREEN, "You fined %s %s (%s).", RPName(target), FormatMoney(amount), reason);
    return 1;
}

CMD:m(playerid, params[])
{
    if (!IsCopOnDuty(playerid) && !(Player[playerid][pFaction] == FACTION_EMS && Player[playerid][pOnDuty]))
        return Error(playerid, "You must be on LSPD/EMS duty.");
    if (isnull(params)) return Usage(playerid, "/m [megaphone text]");
    new text[96], msg[144];
    format(text, sizeof text, "%s", params);
    SanitizeText(text);
    format(msg, sizeof msg, "[MEGAPHONE] %s: %s!", RPName(playerid), text);
    SendNearby(playerid, 60.0, COLOR_YELLOW, msg);
    return 1;
}

// ---------------------------------------------------------------------
//  EMS & injury commands
// ---------------------------------------------------------------------
CMD:heal(playerid, params[])
{
    if (Player[playerid][pFaction] != FACTION_EMS || !Player[playerid][pOnDuty]) return Error(playerid, "You must be on EMS duty.");
    new target = ParamPlayer(params, 0), price;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, price)) return Usage(playerid, "/heal [id/name] [price 20-500]");
    if (price < 20 || price > 500) return Error(playerid, "Price must be $20-$500.");
    if (!IsNearPlayer(playerid, target, 5.0)) return Error(playerid, "Too far away.");
    MakeOffer(playerid, target, OFFER_HEAL, price);
    SendClientMessage(target, COLOR_LIGHTBLUE, "Paramedic %s offers to treat you for %s. Type /accept.", RPName(playerid), FormatMoney(price));
    SendClientMessage(playerid, COLOR_LIGHTBLUE, "You offered treatment to %s for %s.", RPName(target), FormatMoney(price));
    return 1;
}

CMD:revive(playerid, params[])
{
    if (Player[playerid][pFaction] != FACTION_EMS || !Player[playerid][pOnDuty]) return Error(playerid, "You must be on EMS duty.");
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/revive [id/name]");
    if (!IsNearPlayer(playerid, target, 4.0)) return Error(playerid, "Get closer to the patient.");
    if (!Player[target][pInjured]) return Error(playerid, "That player is not critically injured.");
    Player[target][pInjured] = false;
    Player[target][pInjuredTime] = 0;
    ClearAnimations(target);
    TogglePlayerControllable(target, true);
    SetPlayerHealth(target, 60.0);
    GiveMoney(playerid, 250);
    new msg[128];
    format(msg, sizeof msg, "* Paramedic %s revives %s using a defibrillator.", RPName(playerid), RPName(target));
    SendNearby(playerid, 15.0, COLOR_PURPLE, msg);
    SendClientMessage(playerid, COLOR_GREEN, "You revived %s and earned a $250 bonus.", RPName(target));
    SendClientMessage(target, COLOR_GREEN, "You were revived by paramedic %s.", RPName(playerid));
    return 1;
}

CMD:acceptdeath(playerid, params[])
{
    if (!Player[playerid][pInjured]) return Error(playerid, "You are not injured.");
    if (Player[playerid][pInjuredTime] > 0)
    {
        SendClientMessage(playerid, COLOR_RED, "Wait %d more seconds for EMS before giving up.", Player[playerid][pInjuredTime]);
        return 1;
    }
    Player[playerid][pInjured] = false;
    ClearAnimations(playerid);
    TogglePlayerControllable(playerid, true);
    SetPlayerInterior(playerid, 0);
    SetPlayerVirtualWorld(playerid, 0);
    SetPlayerPos(playerid, HOSPITAL_X, HOSPITAL_Y, HOSPITAL_Z);
    SetPlayerHealth(playerid, 100.0);
    GiveMoney(playerid, -HOSPITAL_FEE);
    SendClientMessage(playerid, COLOR_YELLOW, "You were treated at All Saints Hospital. Medical bill: %s.", FormatMoney(HOSPITAL_FEE));
    return 1;
}

// ---------------------------------------------------------------------
//  Gang turf capture
// ---------------------------------------------------------------------
CMD:capture(playerid, params[])
{
    new f = Player[playerid][pFaction];
    if (f != FACTION_GROVE && f != FACTION_BALLAS) return Error(playerid, "Only gangs can capture turfs.");
    new t = GetPlayerTurf(playerid);
    if (t == -1) return Error(playerid, "You are not inside a gang turf.");
    if (Turf[t][tOwner] == f) return Error(playerid, "Your gang already controls this turf.");
    if (Turf[t][tAttacker] != FACTION_NONE) return Error(playerid, "This turf is already being contested.");
    Turf[t][tAttacker] = f;
    Turf[t][tCaptureLeft] = TURF_CAPTURE_SECONDS;
    GangZoneFlashForAll(Turf[t][tZone], (FactionColors[f] & 0xFFFFFF00) | 0x88);
    new msg[128];
    format(msg, sizeof msg, "[TURF WAR] %s started capturing %s from %s! (%d seconds)", FactionNames[f], Turf[t][tName], FactionNames[Turf[t][tOwner]], TURF_CAPTURE_SECONDS);
    SendClientMessageToAll(COLOR_ORANGE, "%s", msg);
    return 1;
}
