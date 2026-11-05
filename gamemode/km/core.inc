// =====================================================================
//  Kingmakers RP - core data, enums & utility helpers
// =====================================================================

#define CMD:%1(%2) forward cmd_%1(%2); public cmd_%1(%2)
#define isnull(%1) ((%1[0] == 0) || (%1[0] == 1 && %1[1] == 0))

// ---- Dialog IDs ----
enum
{
    DLG_NONE = 0,
    DLG_RPNAME,
    DLG_LOGIN,
    DLG_REGISTER,
    DLG_REG_GENDER,
    DLG_REG_AGE,
    DLG_REG_SKIN,
    DLG_HELP,
    DLG_ATM,
    DLG_ATM_DEPOSIT,
    DLG_ATM_WITHDRAW,
    DLG_ATM_TRANSFER_ID,
    DLG_ATM_TRANSFER_AMT,
    DLG_JOBCENTER,
    DLG_CITYHALL,
    DLG_DEALER,
    DLG_DEALER_CONFIRM,
    DLG_MYVEH,
    DLG_HOUSE_BUY,
    DLG_HOUSE_MENU,
    DLG_BIZ_BUY,
    DLG_SHOP_247,
    DLG_SHOP_AMMU,
    DLG_SHOP_CLOTHES,
    DLG_BIZ_MENU,
    DLG_PHONE,
    DLG_PHONE_CONTACTS,
    DLG_PHONE_ADDCONTACT_NAME,
    DLG_PHONE_ADDCONTACT_NUM,
    DLG_PHONE_CONTACT_ACTION,
    DLG_PHONE_SMS_NUM,
    DLG_PHONE_SMS_TEXT,
    DLG_PHONE_INBOX,
    DLG_PHONE_CALL,
    DLG_PHONE_GPS,
    DLG_PHONE_SERVICES,
    DLG_PHONE_BANK,
    DLG_STATS,
    DLG_REFUEL,
    DLG_INFO
}

// ---- Checkpoint owners (only one CP per player in SA-MP) ----
enum
{
    CP_NONE = 0,
    CP_GPS,
    CP_TAXI_PICKUP,
    CP_TAXI_DROP,
    CP_TRUCK_LOAD,
    CP_TRUCK_DROP,
    CP_MINE_ROCK,
    CP_MINE_DROP
}

// ---- Jobs ----
enum
{
    JOB_NONE = 0,
    JOB_TAXI,
    JOB_TRUCKER,
    JOB_MINER,
    JOB_MECHANIC,
    MAX_JOBS
}
new const JobNames[MAX_JOBS][] = { "Unemployed", "Taxi Driver", "Trucker", "Miner", "Mechanic" };

// ---- Factions ----
enum
{
    FACTION_NONE = 0,
    FACTION_LSPD,
    FACTION_EMS,
    FACTION_GROVE,
    FACTION_BALLAS,
    MAX_FACTIONS
}
new const FactionNames[MAX_FACTIONS][] = { "Civilian", "Los Santos Police Dept.", "Emergency Medical Services", "Grove Street Families", "Ballas" };
new const FactionShort[MAX_FACTIONS][] = { "CIV", "LSPD", "EMS", "GSF", "BALLAS" };
new const FactionColors[MAX_FACTIONS] = { 0xFFFFFF00, 0x3A7BFFFF, 0xFF8282FF, 0x2DB82DFF, 0xB040E0FF };
new const FactionRankNames[MAX_FACTIONS][6][] =
{
    { "-", "-", "-", "-", "-", "-" },
    { "Cadet", "Officer", "Sergeant", "Lieutenant", "Captain", "Chief of Police" },
    { "Trainee", "EMT", "Paramedic", "Senior Paramedic", "Doctor", "Chief Medical Officer" },
    { "Youngster", "Homie", "Soldier", "OG", "Shot Caller", "Big Boss" },
    { "Youngster", "Hustler", "Soldier", "Enforcer", "Lieutenant", "Kingpin" }
};
new const FactionSalary[MAX_FACTIONS] = { 0, 900, 850, 0, 0 };
new const Float:FactionHQ[MAX_FACTIONS][3] =
{
    { 0.0, 0.0, 0.0 },
    { 1554.80, -1675.60, 16.20 },
    { 1172.40, -1323.30, 15.40 },
    { 2495.30, -1687.50, 13.52 },
    { 2022.90, -1120.30, 26.42 }
};
new const FactionDutySkins[MAX_FACTIONS] = { 0, 280, 274, 0, 0 };

// ---- Player data ----
enum E_PLAYER
{
    pID,
    pName[MAX_PLAYER_NAME],
    pPassHash[65],
    pSalt[17],
    bool:pLogged,
    bool:pRegistered,
    pLoginTries,
    pMoney,
    pBank,
    pLevel,
    pExp,
    pSkin,
    pGender,
    pAge,
    Float:pPosX,
    Float:pPosY,
    Float:pPosZ,
    Float:pPosA,
    pInt,
    pVW,
    Float:pHealth,
    Float:pArmour,
    pJob,
    pFaction,
    pRank,
    pAdmin,
    pPhone,
    bool:pPhoneOn,
    pWanted,
    pJailTime,
    pMuteTime,
    pPlayMinutes,
    pPayMinutes,
    pDriveLic,
    pGunLic,
    pMedkits,
    pFood,
    // runtime only
    bool:pOnDuty,
    bool:pCuffed,
    bool:pInjured,
    pInjuredTime,
    bool:pFrozen,
    bool:pAdminDuty,
    pCPType,
    pJobStage,
    pJobPay,
    pJobVeh,
    pCarryOre,
    pTaxiFare,
    pCallWith,
    pCallerOf,
    pSMSTarget,
    pTransferTarget,
    pDealerChoice,
    pContactSel,
    pTempName[32],
    pSpecTarget,
    pHouseInside,
    pBizInside,
    pAdminVeh,
    pLastPM,
    pRegStep,
    pSaveTick
}
new Player[MAX_PLAYERS][E_PLAYER];

new DB:gDB;

// ---------------------------------------------------------------------
//  Generic helpers
// ---------------------------------------------------------------------
stock GetName(playerid)
{
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof name);
    return name;
}

// Name with underscore replaced by a space, for RP text.
stock RPName(playerid)
{
    new name[MAX_PLAYER_NAME];
    GetPlayerName(playerid, name, sizeof name);
    for (new i = 0; name[i]; i++) if (name[i] == '_') name[i] = ' ';
    return name;
}

stock bool:IsValidRPName(const name[])
{
    new len = strlen(name), us = -1, count = 0;
    if (len < 7 || len > 22) return false;
    for (new i = 0; i < len; i++)
    {
        if (name[i] == '_') { us = i; count++; continue; }
        if (!((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z'))) return false;
    }
    if (count != 1 || us < 3 || len - us - 1 < 3) return false;
    if (!(name[0] >= 'A' && name[0] <= 'Z')) return false;
    if (!(name[us + 1] >= 'A' && name[us + 1] <= 'Z')) return false;
    return true;
}

// Escapes single quotes for SQLite string literals.
stock SQLEscape(const input[], output[], size = sizeof output)
{
    new j = 0;
    for (new i = 0; input[i] && j < size - 2; i++)
    {
        if (input[i] == '\'') { output[j++] = '\''; }
        output[j++] = input[i];
    }
    output[j] = 0;
}

// Strip characters that would break dialog / colour formatting.
stock SanitizeText(text[])
{
    for (new i = 0; text[i]; i++)
    {
        if (text[i] == '{' || text[i] == '}' || text[i] == '%' || text[i] == '~') text[i] = ' ';
    }
}

// Splits params by spaces. Returns the word at "index" into dest.
stock GetParam(const params[], index, dest[], size = sizeof dest)
{
    new cur = 0, i = 0, j = 0;
    dest[0] = 0;
    if (isnull(params)) return 0;
    while (params[i] == ' ') i++;
    while (params[i])
    {
        if (params[i] == ' ')
        {
            while (params[i] == ' ') i++;
            cur++;
            continue;
        }
        if (cur == index && j < size - 1) dest[j++] = params[i];
        else if (cur > index) break;
        i++;
    }
    dest[j] = 0;
    return j;
}

// Returns everything after the word at "index" (inclusive).
stock GetParamRest(const params[], index, dest[], size = sizeof dest)
{
    new cur = 0, i = 0;
    dest[0] = 0;
    if (isnull(params)) return 0;
    while (params[i] == ' ') i++;
    while (params[i] && cur < index)
    {
        if (params[i] == ' ')
        {
            while (params[i] == ' ') i++;
            cur++;
            continue;
        }
        i++;
    }
    if (cur < index) return 0;
    strmid(dest, params, i, strlen(params), size);
    return strlen(dest);
}

stock bool:IsNumeric(const str[])
{
    if (!str[0]) return false;
    for (new i = 0; str[i]; i++)
    {
        if (str[i] == '-' && i == 0) continue;
        if (str[i] < '0' || str[i] > '9') return false;
    }
    return true;
}

// Finds a player by ID or (partial) name. Returns INVALID_PLAYER_ID if none.
stock FindPlayer(const str[])
{
    if (!str[0]) return INVALID_PLAYER_ID;
    if (IsNumeric(str))
    {
        new id = strval(str);
        if (id >= 0 && id < MAX_PLAYERS && IsPlayerConnected(id) && Player[id][pLogged]) return id;
        return INVALID_PLAYER_ID;
    }
    new found = INVALID_PLAYER_ID;
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (!IsPlayerConnected(i) || !Player[i][pLogged]) continue;
        if (strfind(GetName(i), str, true) != -1)
        {
            if (found != INVALID_PLAYER_ID) return INVALID_PLAYER_ID; // ambiguous
            found = i;
        }
    }
    return found;
}

stock ParamPlayer(const params[], index)
{
    new tmp[MAX_PLAYER_NAME];
    GetParam(params, index, tmp);
    return FindPlayer(tmp);
}

stock ParamInt(const params[], index, &value)
{
    new tmp[16];
    if (!GetParam(params, index, tmp) || !IsNumeric(tmp)) return 0;
    value = strval(tmp);
    return 1;
}

stock Usage(playerid, const text[])
{
    SendClientMessage(playerid, COLOR_GREY, "USAGE: %s", text);
    return 1;
}

stock Error(playerid, const text[])
{
    SendClientMessage(playerid, COLOR_RED, "[!] {FFFFFF}%s", text);
    return 1;
}

stock Info(playerid, const text[])
{
    SendClientMessage(playerid, COLOR_GOLD, "[KM] {FFFFFF}%s", text);
    return 1;
}

stock FormatMoney(amount)
{
    new str[24], tmp[24], len, neg = amount < 0, j = 0;
    format(tmp, sizeof tmp, "%d", neg ? -amount : amount);
    len = strlen(tmp);
    str[j++] = '$';
    if (neg) str[j++] = '-';
    for (new i = 0; i < len; i++)
    {
        str[j++] = tmp[i];
        if ((len - i - 1) % 3 == 0 && i != len - 1) str[j++] = ',';
    }
    str[j] = 0;
    return str;
}

stock GiveMoney(playerid, amount)
{
    Player[playerid][pMoney] += amount;
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, Player[playerid][pMoney]);
}

stock SyncMoney(playerid)
{
    ResetPlayerMoney(playerid);
    GivePlayerMoney(playerid, Player[playerid][pMoney]);
}

// Local chat with distance fading.
stock SendNearby(playerid, Float:range, colour, const text[])
{
    new Float:x, Float:y, Float:z, vw = GetPlayerVirtualWorld(playerid);
    GetPlayerPos(playerid, x, y, z);
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (!IsPlayerConnected(i) || !Player[i][pLogged]) continue;
        if (GetPlayerVirtualWorld(i) != vw) continue;
        new Float:d = GetPlayerDistanceFromPoint(i, x, y, z);
        if (d > range) continue;
        new c = colour;
        if (colour == COLOR_WHITE)
        {
            if (d > range * 0.75) c = COLOR_FADE3;
            else if (d > range * 0.5) c = COLOR_FADE2;
            else if (d > range * 0.25) c = COLOR_FADE1;
        }
        SendClientMessage(i, c, "%s", text);
    }
}

stock SendFactionMessage(faction, colour, const text[])
{
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Player[i][pLogged] && Player[i][pFaction] == faction)
            SendClientMessage(i, colour, "%s", text);
    }
}

stock SendAdminMessage(colour, const text[])
{
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Player[i][pLogged] && Player[i][pAdmin] > 0)
            SendClientMessage(i, colour, "%s", text);
    }
}

stock SendJobMessage(job, colour, const text[])
{
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (IsPlayerConnected(i) && Player[i][pLogged] && Player[i][pJob] == job)
            SendClientMessage(i, colour, "%s", text);
    }
}

stock Float:GetDistanceBetween(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2)
{
    return floatsqroot((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) + (z1 - z2) * (z1 - z2));
}

stock bool:IsNearPlayer(playerid, targetid, Float:range)
{
    new Float:x, Float:y, Float:z;
    GetPlayerPos(targetid, x, y, z);
    if (GetPlayerVirtualWorld(playerid) != GetPlayerVirtualWorld(targetid)) return false;
    return IsPlayerInRangeOfPoint(playerid, range, x, y, z);
}

stock SetCP(playerid, type, Float:x, Float:y, Float:z, Float:size = 4.0)
{
    DisablePlayerCheckpoint(playerid);
    Player[playerid][pCPType] = type;
    SetPlayerCheckpoint(playerid, x, y, z, size);
}

stock ClearCP(playerid)
{
    DisablePlayerCheckpoint(playerid);
    Player[playerid][pCPType] = CP_NONE;
}

stock RandomString(dest[], len)
{
    static const chars[] = "abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    for (new i = 0; i < len; i++) dest[i] = chars[random(sizeof(chars) - 1)];
    dest[len] = 0;
}

stock ExpForLevel(level) return level * 4;
