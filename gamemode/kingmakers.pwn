/*
 * =====================================================================
 *  KINGMAKERS RP - Complete Mobile Roleplay Gamemode
 *  Compatible with SA-MP Mobile, CRMP Mobile, open.mp, and PC SA-MP
 * =====================================================================
 */

#define MIXED_SPELLINGS

#include <open.mp>

#include "km/config.inc"
#include "km/core.inc"
#include "km/forwards.inc"
#include "km/hud.inc"
#include "km/vehicles.inc"
#include "km/account.inc"
#include "km/economy.inc"
#include "km/jobs.inc"
#include "km/property.inc"
#include "km/factions.inc"
#include "km/phone.inc"
#include "km/chat.inc"
#include "km/admin.inc"

forward MainGameTimer();
forward SecondTimer();
forward AutoSaveTimer();

new gMainTimer, gSecondTimer, gAutoSaveTimer;

main()
{
    print("\n========================================================");
    print("   KINGMAKERS RP - MOBILE ROLEPLAY (CRMP / SA-MP)");
    print("   Version: " SERVER_VERSION " | Built on open.mp");
    print("   Ready for mobile clients (Android / iOS / PC)");
    print("========================================================\n");
}

public OnGameModeInit()
{
    SetGameModeText(SERVER_NAME " v" SERVER_VERSION);
    ShowPlayerMarkers(PLAYER_MARKERS_MODE_STREAMED);
    ShowNameTags(true);
    SetNameTagDrawDistance(35.0);
    EnableStuntBonusForAll(false);
    DisableInteriorEnterExits();
    UsePlayerPedAnims();

    if (!InitDatabase()) return 0;

    CreateGlobalTextDraws();
    CreateStaticVehicles();
    CreateEconomyPickups();
    CreateJobPickups();
    CreateFactionPickups();
    LoadProperties();
    LoadTurfs();

    gSecondTimer = SetTimer("SecondTimer", 1000, true);
    gMainTimer = SetTimer("MainGameTimer", 60000, true);
    gAutoSaveTimer = SetTimer("AutoSaveTimer", AUTOSAVE_MINUTES * 60000, true);

    print("[KM] Gamemode initialization complete.");
    return 1;
}

public OnGameModeExit()
{
    print("[KM] Saving all online players and properties...");
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Player[i][pLogged])
        {
            SavePlayer(i);
            UnloadPlayerVehicles(i);
        }
    }
    for (new h = 0; h < MAX_HOUSES; h++) if (House[h][hExists]) SaveHouse(h);
    for (new b = 0; b < MAX_BUSINESSES; b++) if (Biz[b][bExists]) SaveBiz(b);

    KillTimer(gSecondTimer);
    KillTimer(gMainTimer);
    KillTimer(gAutoSaveTimer);
    DB_Close(gDB);
    print("[KM] Shutdown clean.");
    return 1;
}

public OnPlayerConnect(playerid)
{
    ResetPlayerData(playerid);
    CreatePlayerTextDraws(playerid);
    SetPlayerColor(playerid, COLOR_GREY);

    if (CheckBanned(playerid)) return 1;

    SetTimerEx("ShowLoginScreen", 400, false, "i", playerid);
    return 1;
}

public OnPlayerDisconnect(playerid, reason)
{
    if (Player[playerid][pLogged])
    {
        SavePlayer(playerid);
        UnloadPlayerVehicles(playerid);
        StopWork(playerid);
        if (Player[playerid][pCallWith] != INVALID_PLAYER_ID) EndCall(playerid);

        new reasonName[16];
        switch (reason)
        {
            case 0: format(reasonName, sizeof reasonName, "Timeout");
            case 1: format(reasonName, sizeof reasonName, "Left");
            case 2: format(reasonName, sizeof reasonName, "Kicked");
        }
        new msg[128];
        format(msg, sizeof msg, "%s has left Kingmakers RP (%s).", RPName(playerid), reasonName);
        for (new i = 0; i < MAX_PLAYERS; i++)
            if (i != playerid && IsPlayerConnected(i) && Player[i][pLogged]) SendClientMessage(i, COLOR_GREY, "%s", msg);
    }
    ResetPlayerData(playerid);
    return 1;
}

public OnPlayerRequestClass(playerid, classid)
{
    #pragma unused classid
    if (!Player[playerid][pLogged])
    {
        ShowLoginScreen(playerid);
        return 0;
    }
    SetSpawnInfo(playerid, NO_TEAM, Player[playerid][pSkin], Player[playerid][pPosX], Player[playerid][pPosY], Player[playerid][pPosZ], Player[playerid][pPosA]);
    SpawnPlayer(playerid);
    return 0;
}

public OnPlayerSpawn(playerid)
{
    if (!Player[playerid][pLogged])
    {
        ShowLoginScreen(playerid);
        return 0;
    }
    Account_OnSpawn(playerid);
    ShowTurfsForPlayer(playerid);
    return 1;
}

public OnPlayerDeath(playerid, killerid, WEAPON:reason)
{
    #pragma unused reason
    if (!Player[playerid][pLogged]) return 1;
    if (Player[playerid][pJailTime] > 0) return 1; // already in jail

    // Instead of instaspawn, enter injured/critical state for EMS.
    GetPlayerPos(playerid, Player[playerid][pPosX], Player[playerid][pPosY], Player[playerid][pPosZ]);
    Player[playerid][pInt] = GetPlayerInterior(playerid);
    Player[playerid][pVW] = GetPlayerVirtualWorld(playerid);
    Player[playerid][pInjured] = true;
    Player[playerid][pInjuredTime] = INJURED_SECONDS;

    if (killerid != INVALID_PLAYER_ID && IsPlayerConnected(killerid) && killerid != playerid)
    {
        if (Player[killerid][pFaction] != FACTION_LSPD)
        {
            Player[killerid][pWanted] += 2;
            if (Player[killerid][pWanted] > 6) Player[killerid][pWanted] = 6;
            SetPlayerWantedLevel(killerid, Player[killerid][pWanted]);
            SendClientMessage(killerid, COLOR_RED, "[CRIME] You are now wanted for assault (+2 stars, total %d).", Player[killerid][pWanted]);
            new msg[160];
            format(msg, sizeof msg, "[DISPATCH] %s committed assault on %s near (%.0f, %.0f) - wanted %d.", RPName(killerid), RPName(playerid), Player[playerid][pPosX], Player[playerid][pPosY], Player[killerid][pWanted]);
            SendFactionMessage(FACTION_LSPD, FactionColors[FACTION_LSPD], msg);
        }
    }
    return 1;
}

public OnPlayerText(playerid, text[])
{
    return HandlePlayerChat(playerid, text);
}

public OnDialogResponse(playerid, dialogid, response, listitem, inputtext[])
{
    if (Account_OnDialog(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Economy_OnDialog(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Jobs_OnDialog(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Property_OnDialog(playerid, dialogid, response, listitem, inputtext)) return 1;
    if (Phone_OnDialog(playerid, dialogid, response, listitem, inputtext)) return 1;

    // Vehicle dealership dialogs
    if (dialogid == DLG_DEALER)
    {
        if (!response) return 1;
        if (listitem < 0 || listitem >= sizeof DealerCars) return 1;
        Player[playerid][pDealerChoice] = listitem;
        new body[256];
        format(body, sizeof body, C_WHITE "Buy " C_GOLD "%s" C_WHITE " for " C_GREEN "%s" C_WHITE "?\n\n\
            It will spawn parked outside the dealership.\nRequired level: %d",
            DealerCars[listitem][dName], FormatMoney(DealerCars[listitem][dPrice]), DealerCars[listitem][dLevel]);
        ShowPlayerDialog(playerid, DLG_DEALER_CONFIRM, DIALOG_STYLE_MSGBOX, C_GOLD "Confirm Purchase", body, "Buy", "Cancel");
        return 1;
    }
    if (dialogid == DLG_DEALER_CONFIRM)
    {
        if (!response) return ShowDealerDialog(playerid), 1;
        BuyVehicle(playerid, Player[playerid][pDealerChoice]);
        return 1;
    }
    if (dialogid == DLG_MYVEH)
    {
        if (!response) return 1;
        // locate chosen car
        new count = 0;
        for (new v = 1; v < MAX_VEHICLES; v++)
        {
            if (!IsVehicleOwner(playerid, v)) continue;
            if (count == listitem)
            {
                new Float:x, Float:y, Float:z;
                GetVehiclePos(v, x, y, z);
                SetCP(playerid, CP_GPS, x, y, z, 5.0);
                SendClientMessage(playerid, COLOR_GREEN, "[GPS] Vehicle located! Follow the red marker on your radar.");
                return 1;
            }
            count++;
        }
        return 1;
    }
    if (dialogid == DLG_HELP) return 1;
    return 0;
}

public OnPlayerEnterCheckpoint(playerid)
{
    if (Player[playerid][pCPType] == CP_GPS)
    {
        ClearCP(playerid);
        PlayerPlaySound(playerid, 1057, 0.0, 0.0, 0.0);
        ShowInfoText(playerid, "~g~You have reached your destination", 3000);
        return 1;
    }
    if (Jobs_OnEnterCP(playerid)) return 1;
    return 1;
}

public OnPlayerStateChange(playerid, PLAYER_STATE:newstate, PLAYER_STATE:oldstate)
{
    #pragma unused oldstate
    if (newstate == PLAYER_STATE_DRIVER)
    {
        new vid = GetPlayerVehicleID(playerid);
        Vehicle_OnDriver(playerid, vid);
    }
    else if (newstate == PLAYER_STATE_PASSENGER)
    {
        new vid = GetPlayerVehicleID(playerid);
        Jobs_OnPassenger(playerid, vid);
    }
    else if (newstate == PLAYER_STATE_ONFOOT)
    {
        PlayerTextDrawHide(playerid, PTD_Speedo[playerid]);
    }
    return 1;
}

public OnPlayerKeyStateChange(playerid, KEY:newkeys, KEY:oldkeys)
{
    #pragma unused oldkeys
    // KEY_NO (N key / Mobile action button) toggles vehicle engine
    if ((newkeys & KEY_NO) && IsPlayerInAnyVehicle(playerid) && GetPlayerState(playerid) == PLAYER_STATE_DRIVER)
    {
        ToggleEngine(playerid);
        return 1;
    }
    // KEY_YES (Y key) quick-locks nearest owned vehicle
    if ((newkeys & KEY_YES) && !IsPlayerInAnyVehicle(playerid))
    {
        ToggleLock(playerid);
        return 1;
    }
    // KEY_SECONDARY_ATTACK (Enter / F / phone tap) to enter/exit buildings on foot
    if ((newkeys & KEY_SECONDARY_ATTACK) && !IsPlayerInAnyVehicle(playerid))
    {
        if (Player[playerid][pHouseInside] != -1 || Player[playerid][pBizInside] != -1) TryExit(playerid);
        else TryEnter(playerid);
        return 1;
    }
    return 1;
}

// ---------------------------------------------------------------------
//  Timers
// ---------------------------------------------------------------------
public SecondTimer()
{
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (!IsPlayerConnected(i) || !Player[i][pLogged]) continue;

        Vehicle_OnSecond(i);

        if (Player[i][pMuteTime] > 0)
        {
            Player[i][pMuteTime]--;
            if (Player[i][pMuteTime] == 0) SendClientMessage(i, COLOR_GREEN, "Your mute expired. You can speak again.");
        }
        if (Player[i][pJailTime] > 0)
        {
            Player[i][pJailTime]--;
            if (Player[i][pJailTime] == 0) ReleaseFromJail(i);
            else if (Player[i][pJailTime] % 60 == 0)
            {
                new str[64];
                format(str, sizeof str, "~w~Jail sentence: ~r~%d min", Player[i][pJailTime] / 60);
                GameTextForPlayer(i, str, 1500, 3);
            }
        }
        if (Player[i][pInjured])
        {
            if (Player[i][pInjuredTime] > 0) Player[i][pInjuredTime]--;
            ApplyAnimation(i, "CRACK", "crckdeth2", 4.1, true, false, false, true, 0, SYNC_ALL);
        }
    }
    Factions_OnSecond();
    return 1;
}

public MainGameTimer()
{
    // Advance world clock: 1 real minute = 2 in-game minutes
    static minuteCount = 0;
    minuteCount = (minuteCount + 2) % 60;
    if (minuteCount == 0)
    {
        gServerHour = (gServerHour + 1) % 24;
        SetWorldTime(gServerHour);
    }

    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (!IsPlayerConnected(i) || !Player[i][pLogged]) continue;
        Economy_OnMinute(i);
    }
    return 1;
}

public AutoSaveTimer()
{
    print("[KM] Running periodic autosave...");
    for (new i = 0; i < MAX_PLAYERS; i++)
        if (IsPlayerConnected(i) && Player[i][pLogged]) SavePlayer(i);
    for (new h = 0; h < MAX_HOUSES; h++) if (House[h][hExists]) SaveHouse(h);
    for (new b = 0; b < MAX_BUSINESSES; b++) if (Biz[b][bExists]) SaveBiz(b);
    return 1;
}

// ---------------------------------------------------------------------
//  Generic /help command
// ---------------------------------------------------------------------
CMD:help(playerid, params[])
{
    new body[2048];
    body = C_GOLD "KINGMAKERS RP - COMMAND DIRECTORY\n\n\
        " C_GOLD "General & Character:\n\
        " C_WHITE "/stats - character card    /pay [id] [amt] - give cash\n\
        /phone - open KingPhone    /gps - quick navigator    /number - view phone #\n\
        /anim [name] - animations  /handsup - hands up       /stopanim - cancel anim\n\
        /usemedkit - use a first-aid kit\n\n\
        " C_GOLD "Roleplay Chat:\n\
        " C_WHITE "/me [action]   /do [description]   /try [action]\n\
        /s [shout]     /w [id] [whisper]   /b [OOC chat]     /pm [id] [msg]\n\n\
        " C_GOLD "Vehicles:\n\
        " C_WHITE "/engine (or N) - start engine   /lock (or Y) - lock/unlock\n\
        /park - save vehicle spot      /refuel - gas station     /myveh - vehicle list\n\
        /sellveh - sell at dealer      /lights - toggle headlights\n\n\
        " C_GOLD "Properties & Shops:\n\
        " C_WHITE "/enter / /exit (or F/Enter) - buildings\n\
        /buyhouse - buy house          /house - manage house\n\
        /buybiz - buy business         /bizmenu - collect till\n\
        /buy - shop inside 24/7, Ammu-Nation or Binco Clothes\n\n\
        " C_GOLD "Jobs & Factions:\n\
        " C_WHITE "/jobs - job center   /startwork - on duty   /stopwork - clock out\n\
        /fare - taxi fare    /repair - mechanic     /refill - mechanic\n\
        /duty - LSPD/EMS     /f - faction radio     /d - dept radio\n\
        /su - report crime   /cuff - handcuffs      /arrest - book suspect\n\
        /heal - paramedic    /revive - defibrillator /acceptdeath - respawn\n\
        /capture - gang turf war";
    ShowPlayerDialog(playerid, DLG_HELP, DIALOG_STYLE_MSGBOX, C_GOLD "Kingmakers RP " C_WHITE "- Help", body, "Close", "");
    return 1;
}

CMD:menu(playerid, params[]) return cmd_help(playerid, params);
CMD:cmds(playerid, params[]) return cmd_help(playerid, params);
