// =====================================================================
//  Kingmakers RP - administration & moderation (levels 1-5)
// =====================================================================

stock bool:RequireAdmin(playerid, minlevel)
{
    if (Player[playerid][pAdmin] < minlevel)
    {
        Error(playerid, "You are not authorized to use this command.");
        return false;
    }
    return true;
}

CMD:ahelp(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    new lvl = Player[playerid][pAdmin];
    SendClientMessage(playerid, COLOR_GOLD, "___________ KINGMAKERS ADMIN COMMANDS ___________");
    if (lvl >= 1) SendClientMessage(playerid, COLOR_WHITE, "Level 1 (Helper): /a, /aduty, /check, /spec, /specoff, /slap, /freeze, /unfreeze, /goto");
    if (lvl >= 2) SendClientMessage(playerid, COLOR_WHITE, "Level 2 (Moderator): /gethere, /kick, /mute, /unmute, /jail, /unjail, /respawnveh");
    if (lvl >= 3) SendClientMessage(playerid, COLOR_WHITE, "Level 3 (Senior): /veh, /destroyveh, /sethp, /setarmour, /setskin, /ban, /unban, /setweather, /settime");
    if (lvl >= 4) SendClientMessage(playerid, COLOR_WHITE, "Level 4 (Lead): /setfaction, /setjob, /setleader, /givemoney, /createhouse, /createbiz");
    if (lvl >= 5) SendClientMessage(playerid, COLOR_WHITE, "Level 5 (Owner): /makeadmin, /restart, /asay, /broadcast");
    return 1;
}

CMD:a(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    if (isnull(params)) return Usage(playerid, "/a [admin message]");
    new text[128], msg[180];
    format(text, sizeof text, "%s", params);
    SanitizeText(text);
    format(msg, sizeof msg, "[ADMIN L%d] %s: %s", Player[playerid][pAdmin], RPName(playerid), text);
    SendAdminMessage(COLOR_ADMIN, msg);
    return 1;
}

CMD:aduty(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    Player[playerid][pAdminDuty] = !Player[playerid][pAdminDuty];
    if (Player[playerid][pAdminDuty])
    {
        SetPlayerColor(playerid, COLOR_ADMIN);
        SetPlayerArmour(playerid, 100.0);
        SetPlayerHealth(playerid, 100.0);
        new msg[128];
        format(msg, sizeof msg, "[ADMIN] %s is now on admin duty.", RPName(playerid));
        SendAdminMessage(COLOR_ADMIN, msg);
    }
    else
    {
        ApplyPlayerColor(playerid);
        Info(playerid, "You are no longer on admin duty.");
    }
    return 1;
}

CMD:check(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/check [id/name]");
    ShowStats(playerid, target);
    return 1;
}

CMD:goto(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || target == playerid) return Usage(playerid, "/goto [id/name]");
    new Float:x, Float:y, Float:z;
    GetPlayerPos(target, x, y, z);
    SetPlayerInterior(playerid, GetPlayerInterior(target));
    SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(target));
    SetPlayerPos(playerid, x + 1.0, y + 1.0, z);
    Info(playerid, "Teleported to %s.", RPName(target));
    return 1;
}

CMD:gethere(playerid, params[])
{
    if (!RequireAdmin(playerid, 2)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || target == playerid) return Usage(playerid, "/gethere [id/name]");
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    SetPlayerInterior(target, GetPlayerInterior(playerid));
    SetPlayerVirtualWorld(target, GetPlayerVirtualWorld(playerid));
    SetPlayerPos(target, x + 1.0, y + 1.0, z);
    SendClientMessage(target, COLOR_YELLOW, "Admin %s teleported you.", RPName(playerid));
    Info(playerid, "Teleported %s to you.", RPName(target));
    return 1;
}

CMD:spec(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID || target == playerid) return Usage(playerid, "/spec [id/name]");
    TogglePlayerSpectating(playerid, true);
    SetPlayerInterior(playerid, GetPlayerInterior(target));
    SetPlayerVirtualWorld(playerid, GetPlayerVirtualWorld(target));
    if (IsPlayerInAnyVehicle(target)) PlayerSpectateVehicle(playerid, GetPlayerVehicleID(target));
    else PlayerSpectatePlayer(playerid, target);
    Player[playerid][pSpecTarget] = target;
    Info(playerid, "Now spectating %s. Use /specoff to stop.", RPName(target));
    return 1;
}

CMD:specoff(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    if (Player[playerid][pSpecTarget] == INVALID_PLAYER_ID) return Error(playerid, "You are not spectating.");
    Player[playerid][pSpecTarget] = INVALID_PLAYER_ID;
    TogglePlayerSpectating(playerid, false);
    SetPlayerPos(playerid, Player[playerid][pPosX], Player[playerid][pPosY], Player[playerid][pPosZ]);
    SetPlayerInterior(playerid, Player[playerid][pInt]);
    SetPlayerVirtualWorld(playerid, Player[playerid][pVW]);
    Info(playerid, "Spectate disabled.");
    return 1;
}

CMD:slap(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/slap [id/name]");
    new Float:x, Float:y, Float:z;
    GetPlayerPos(target, x, y, z);
    SetPlayerPos(target, x, y, z + 5.0);
    PlayerPlaySound(target, 1130, 0.0, 0.0, 0.0);
    Info(playerid, "Slapped %s.", RPName(target));
    return 1;
}

CMD:freeze(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/freeze [id/name]");
    TogglePlayerControllable(target, false);
    Player[target][pFrozen] = true;
    SendClientMessage(target, COLOR_RED, "You were frozen by admin %s.", RPName(playerid));
    Info(playerid, "Froze %s.", RPName(target));
    return 1;
}

CMD:unfreeze(playerid, params[])
{
    if (!RequireAdmin(playerid, 1)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/unfreeze [id/name]");
    TogglePlayerControllable(target, true);
    Player[target][pFrozen] = false;
    SendClientMessage(target, COLOR_GREEN, "You were unfrozen by admin %s.", RPName(playerid));
    Info(playerid, "Unfroze %s.", RPName(target));
    return 1;
}

CMD:kick(playerid, params[])
{
    if (!RequireAdmin(playerid, 2)) return 1;
    new target = ParamPlayer(params, 0), reason[64];
    if (target == INVALID_PLAYER_ID || !GetParamRest(params, 1, reason)) return Usage(playerid, "/kick [id/name] [reason]");
    if (Player[target][pAdmin] >= Player[playerid][pAdmin]) return Error(playerid, "Cannot kick a higher or equal admin.");
    new msg[160];
    format(msg, sizeof msg, "[ADMIN] %s was kicked by %s. Reason: %s", RPName(target), RPName(playerid), reason);
    SendClientMessageToAll(COLOR_RED, "%s", msg);
    SavePlayer(target);
    SetTimerEx("DelayedKick", 500, false, "i", target);
    return 1;
}

CMD:mute(playerid, params[])
{
    if (!RequireAdmin(playerid, 2)) return 1;
    new target = ParamPlayer(params, 0), mins, reason[64];
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, mins) || !GetParamRest(params, 2, reason))
        return Usage(playerid, "/mute [id/name] [minutes] [reason]");
    Player[target][pMuteTime] = mins * 60;
    new msg[160];
    format(msg, sizeof msg, "[ADMIN] %s was muted by %s for %d minutes. Reason: %s", RPName(target), RPName(playerid), mins, reason);
    SendClientMessageToAll(COLOR_RED, "%s", msg);
    SavePlayer(target);
    return 1;
}

CMD:unmute(playerid, params[])
{
    if (!RequireAdmin(playerid, 2)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/unmute [id/name]");
    Player[target][pMuteTime] = 0;
    SendClientMessage(target, COLOR_GREEN, "You were unmuted by %s.", RPName(playerid));
    Info(playerid, "Unmuted %s.", RPName(target));
    SavePlayer(target);
    return 1;
}

CMD:jail(playerid, params[])
{
    if (!RequireAdmin(playerid, 2)) return 1;
    new target = ParamPlayer(params, 0), mins, reason[64];
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, mins) || !GetParamRest(params, 2, reason))
        return Usage(playerid, "/jail [id/name] [minutes] [reason]");
    Player[target][pJailTime] = mins * 60;
    PutInJail(target);
    new msg[160];
    format(msg, sizeof msg, "[ADMIN] %s was jailed by %s for %d minutes. Reason: %s", RPName(target), RPName(playerid), mins, reason);
    SendClientMessageToAll(COLOR_RED, "%s", msg);
    SavePlayer(target);
    return 1;
}

CMD:unjail(playerid, params[])
{
    if (!RequireAdmin(playerid, 2)) return 1;
    new target = ParamPlayer(params, 0);
    if (target == INVALID_PLAYER_ID) return Usage(playerid, "/unjail [id/name]");
    ReleaseFromJail(target);
    Info(playerid, "Released %s from jail.", RPName(target));
    return 1;
}

CMD:ban(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new target = ParamPlayer(params, 0), reason[64];
    if (target == INVALID_PLAYER_ID || !GetParamRest(params, 1, reason)) return Usage(playerid, "/ban [id/name] [reason]");
    if (Player[target][pAdmin] >= Player[playerid][pAdmin]) return Error(playerid, "Cannot ban a higher or equal admin.");
    new ip[16], q[384], escN[64], escR[128], escA[64];
    GetPlayerIp(target, ip, sizeof ip);
    SQLEscape(GetName(target), escN);
    SQLEscape(reason, escR);
    SQLEscape(GetName(playerid), escA);
    format(q, sizeof q, "INSERT INTO bans (name, ip, reason, admin, time) VALUES ('%s', '%s', '%s', '%s', %d)",
        escN, ip, escR, escA, gettime());
    DB_Exec(q);
    new msg[160];
    format(msg, sizeof msg, "[BAN] %s was banned by %s. Reason: %s", RPName(target), RPName(playerid), reason);
    SendClientMessageToAll(COLOR_RED, "%s", msg);
    SavePlayer(target);
    SetTimerEx("DelayedKick", 500, false, "i", target);
    return 1;
}

CMD:unban(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new name[MAX_PLAYER_NAME];
    if (!GetParam(params, 0, name)) return Usage(playerid, "/unban [exact_name]");
    new q[128], esc[64];
    SQLEscape(name, esc);
    format(q, sizeof q, "DELETE FROM bans WHERE name = '%s'", esc);
    DB_Exec(q);
    Info(playerid, "Unbanned account: %s", name);
    return 1;
}

CMD:veh(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new model;
    if (!ParamInt(params, 0, model) || model < 400 || model > 611) return Usage(playerid, "/veh [model 400-611]");
    new Float:x, Float:y, Float:z, Float:a;
    GetPlayerPos(playerid, x, y, z);
    GetPlayerFacingAngle(playerid, a);
    if (Player[playerid][pAdminVeh] != INVALID_VEHICLE_ID) DestroyVehicle(Player[playerid][pAdminVeh]);
    new vid = CreateVehicle(model, x + 2.0, y + 2.0, z + 1.0, a, 1, 1, -1);
    SetupVehicle(vid, VT_ADMIN);
    Player[playerid][pAdminVeh] = vid;
    PutPlayerInVehicle(playerid, vid, 0);
    Info(playerid, "Spawned admin vehicle (model %d).", model);
    return 1;
}

CMD:destroyveh(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new vid = GetPlayerVehicleID(playerid);
    if (!vid) return Error(playerid, "You must be inside a vehicle.");
    if (VehType[vid] == VT_OWNED) return Error(playerid, "Cannot delete owned player vehicles with /destroyveh. Use /respawnveh.");
    DestroyVehicle(vid);
    Info(playerid, "Vehicle destroyed.");
    return 1;
}

CMD:sethp(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new target = ParamPlayer(params, 0), hp;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, hp)) return Usage(playerid, "/sethp [id/name] [0-100]");
    SetPlayerHealth(target, float(hp));
    Info(playerid, "Set %s's HP to %d.", RPName(target), hp);
    return 1;
}

CMD:setarmour(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new target = ParamPlayer(params, 0), arm;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, arm)) return Usage(playerid, "/setarmour [id/name] [0-100]");
    SetPlayerArmour(target, float(arm));
    Info(playerid, "Set %s's armour to %d.", RPName(target), arm);
    return 1;
}

CMD:setskin(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new target = ParamPlayer(params, 0), skin;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, skin) || skin < 1 || skin > 311) return Usage(playerid, "/setskin [id/name] [1-311]");
    Player[target][pSkin] = skin;
    SetPlayerSkin(target, skin);
    Info(playerid, "Set %s's skin to %d.", RPName(target), skin);
    SavePlayer(target);
    return 1;
}

CMD:setweather(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new w;
    if (!ParamInt(params, 0, w) || w < 0 || w > 45) return Usage(playerid, "/setweather [0-45]");
    gServerWeather = w;
    SetWeather(w);
    Info(playerid, "Server weather set to %d.", w);
    return 1;
}

CMD:settime(playerid, params[])
{
    if (!RequireAdmin(playerid, 3)) return 1;
    new h;
    if (!ParamInt(params, 0, h) || h < 0 || h > 23) return Usage(playerid, "/settime [0-23]");
    gServerHour = h;
    SetWorldTime(h);
    Info(playerid, "Server time set to %02d:00.", h);
    return 1;
}

CMD:setfaction(playerid, params[])
{
    if (!RequireAdmin(playerid, 4)) return 1;
    new target = ParamPlayer(params, 0), f, rank;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, f) || !ParamInt(params, 2, rank) || f < 0 || f >= MAX_FACTIONS)
        return Usage(playerid, "/setfaction [id/name] [faction 0-4] [rank 0-5]");
    Player[target][pFaction] = f;
    Player[target][pRank] = rank;
    Player[target][pOnDuty] = false;
    SetPlayerSkin(target, Player[target][pSkin]);
    ApplyPlayerColor(target);
    Info(playerid, "Set %s's faction to %s (rank %d).", RPName(target), FactionNames[f], rank);
    SendClientMessage(target, COLOR_LIGHTBLUE, "Admin %s set your faction to %s (%s).", RPName(playerid), FactionNames[f], FactionRankNames[f][rank]);
    SavePlayer(target);
    return 1;
}

CMD:setjob(playerid, params[])
{
    if (!RequireAdmin(playerid, 4)) return 1;
    new target = ParamPlayer(params, 0), job;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, job) || job < 0 || job >= MAX_JOBS)
        return Usage(playerid, "/setjob [id/name] [job 0-4]");
    StopWork(target);
    Player[target][pJob] = job;
    Info(playerid, "Set %s's job to %s.", RPName(target), JobNames[job]);
    SendClientMessage(target, COLOR_LIGHTBLUE, "Admin %s set your job to %s.", RPName(playerid), JobNames[job]);
    SavePlayer(target);
    return 1;
}

CMD:givemoney(playerid, params[])
{
    if (!RequireAdmin(playerid, 4)) return 1;
    new target = ParamPlayer(params, 0), amount;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, amount)) return Usage(playerid, "/givemoney [id/name] [amount]");
    GiveMoney(target, amount);
    Info(playerid, "Gave %s to %s.", FormatMoney(amount), RPName(target));
    SendClientMessage(target, COLOR_GREEN, "Admin %s gave you %s.", RPName(playerid), FormatMoney(amount));
    SavePlayer(target);
    return 1;
}

CMD:createhouse(playerid, params[])
{
    if (!RequireAdmin(playerid, 4)) return 1;
    new itype, price;
    if (!ParamInt(params, 0, itype) || !ParamInt(params, 1, price) || itype < 0 || itype >= sizeof HouseInteriors)
        return Usage(playerid, "/createhouse [interior 0-4] [price]");
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new h = AddHouse(x, y, z, itype, price);
    if (h == -1) return Error(playerid, "House limit reached.");
    Info(playerid, "Created house #%d (%s, %s).", h, HouseInteriors[itype][hiName], FormatMoney(price));
    return 1;
}

CMD:createbiz(playerid, params[])
{
    if (!RequireAdmin(playerid, 4)) return 1;
    new type, price;
    if (!ParamInt(params, 0, type) || !ParamInt(params, 1, price) || type < 0 || type >= MAX_BIZ_TYPES)
        return Usage(playerid, "/createbiz [type 0=24/7, 1=Ammu, 2=Binco] [price]");
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    new b = AddBiz(x, y, z, type, price);
    if (b == -1) return Error(playerid, "Business limit reached.");
    Info(playerid, "Created business #%d (%s, %s).", b, BizTypeNames[type], FormatMoney(price));
    return 1;
}

CMD:makeadmin(playerid, params[])
{
    if (!RequireAdmin(playerid, 5)) return 1;
    new target = ParamPlayer(params, 0), lvl;
    if (target == INVALID_PLAYER_ID || !ParamInt(params, 1, lvl) || lvl < 0 || lvl > 5) return Usage(playerid, "/makeadmin [id/name] [0-5]");
    Player[target][pAdmin] = lvl;
    Info(playerid, "Set %s's admin level to %d.", RPName(target), lvl);
    SendClientMessage(target, COLOR_GOLD, "Admin %s set your admin level to %d.", RPName(playerid), lvl);
    SavePlayer(target);
    return 1;
}

CMD:broadcast(playerid, params[])
{
    if (!RequireAdmin(playerid, 5)) return 1;
    if (isnull(params)) return Usage(playerid, "/broadcast [message]");
    new clean[128], msg[180];
    format(clean, sizeof clean, "%s", params);
    SanitizeText(clean);
    format(msg, sizeof msg, "[SERVER BROADCAST] %s", clean);
    SendClientMessageToAll(COLOR_GOLD, "%s", msg);
    return 1;
}

CMD:restart(playerid, params[])
{
    if (!RequireAdmin(playerid, 5)) return 1;
    SendClientMessageToAll(COLOR_RED, "[SERVER] Server restarting in 5 seconds for an update. Progress is being saved...");
    for (new i = 0; i < MAX_PLAYERS; i++) if (IsPlayerConnected(i) && Player[i][pLogged]) SavePlayer(i);
    SetTimer("TriggerGMX", 5000, false);
    return 1;
}

forward TriggerGMX();
public TriggerGMX()
{
    SendRconCommand("gmx");
    return 1;
}
