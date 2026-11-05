// =====================================================================
//  Kingmakers RP - chat channels, roleplay actions, animations
// =====================================================================

#define CHAT_NORMAL_RADIUS  20.0
#define CHAT_SHOUT_RADIUS   45.0
#define CHAT_WHISPER_RADIUS 4.0
#define CHAT_ME_RADIUS      20.0

stock HandlePlayerChat(playerid, text[])
{
    if (!Player[playerid][pLogged]) return 0;
    if (Player[playerid][pMuteTime] > 0)
    {
        SendClientMessage(playerid, COLOR_RED, "You are muted for another %d seconds.", Player[playerid][pMuteTime]);
        return 0;
    }
    if (Player[playerid][pInjured])
    {
        SendClientMessage(playerid, COLOR_RED, "You are unconscious and cannot speak.");
        return 0;
    }

    // Live phone call routing
    if (Player[playerid][pCallWith] != INVALID_PLAYER_ID)
    {
        new other = Player[playerid][pCallWith];
        if (IsPlayerConnected(other))
        {
            new clean[128], line[180];
            format(clean, sizeof clean, "%s", text);
            SanitizeText(clean);
            format(line, sizeof line, "[PHONE] %s: %s", RPName(playerid), clean);
            SendClientMessage(playerid, COLOR_PHONE, "%s", line);
            SendClientMessage(other, COLOR_PHONE, "%s", line);
            // Nearby players also hear the caller talking on their phone.
            format(line, sizeof line, "%s (on phone): %s", RPName(playerid), clean);
            SendNearby(playerid, 8.0, COLOR_FADE2, line);
            return 0;
        }
    }

    // Normal IC speech with distance fading
    new clean[128], line[180];
    format(clean, sizeof clean, "%s", text);
    SanitizeText(clean);
    format(line, sizeof line, "%s says: %s", RPName(playerid), clean);
    SendNearby(playerid, CHAT_NORMAL_RADIUS, COLOR_WHITE, line);
    return 0;
}

// ---------------------------------------------------------------------
//  RP action commands
// ---------------------------------------------------------------------
CMD:me(playerid, params[])
{
    if (isnull(params)) return Usage(playerid, "/me [action]");
    new clean[128], line[180];
    format(clean, sizeof clean, "%s", params);
    SanitizeText(clean);
    format(line, sizeof line, "* %s %s", RPName(playerid), clean);
    SendNearby(playerid, CHAT_ME_RADIUS, COLOR_PURPLE, line);
    return 1;
}

CMD:do(playerid, params[])
{
    if (isnull(params)) return Usage(playerid, "/do [description]");
    new clean[128], line[180];
    format(clean, sizeof clean, "%s", params);
    SanitizeText(clean);
    format(line, sizeof line, "* %s (( %s ))", clean, RPName(playerid));
    SendNearby(playerid, CHAT_ME_RADIUS, COLOR_PURPLE, line);
    return 1;
}

CMD:try(playerid, params[])
{
    if (isnull(params)) return Usage(playerid, "/try [action]");
    new clean[128], line[180], bool:success = random(2) == 1;
    format(clean, sizeof clean, "%s", params);
    SanitizeText(clean);
    format(line, sizeof line, "* %s %s | %s", RPName(playerid), clean, success ? ("{5CE65C}SUCCESS") : ("{FF4B4B}FAILED"));
    SendNearby(playerid, CHAT_ME_RADIUS, COLOR_PURPLE, line);
    return 1;
}

CMD:s(playerid, params[]) // shout
{
    if (isnull(params)) return Usage(playerid, "/s [shouted message]");
    new clean[128], line[180];
    format(clean, sizeof clean, "%s", params);
    SanitizeText(clean);
    format(line, sizeof line, "%s shouts: %s!", RPName(playerid), clean);
    SendNearby(playerid, CHAT_SHOUT_RADIUS, COLOR_WHITE, line);
    return 1;
}

CMD:w(playerid, params[]) // whisper
{
    new target = ParamPlayer(params, 0), text[128];
    if (target == INVALID_PLAYER_ID || !GetParamRest(params, 1, text)) return Usage(playerid, "/w [id/name] [whisper text]");
    if (!IsNearPlayer(playerid, target, CHAT_WHISPER_RADIUS)) return Error(playerid, "You are too far to whisper.");
    new clean[128], line[180];
    format(clean, sizeof clean, "%s", text);
    SanitizeText(clean);
    format(line, sizeof line, "%s whispers to you: %s", RPName(playerid), clean);
    SendClientMessage(target, COLOR_YELLOW, "%s", line);
    format(line, sizeof line, "You whisper to %s: %s", RPName(target), clean);
    SendClientMessage(playerid, COLOR_YELLOW, "%s", line);
    return 1;
}

CMD:b(playerid, params[]) // local OOC
{
    if (isnull(params)) return Usage(playerid, "/b [OOC message]");
    new clean[128], line[180];
    format(clean, sizeof clean, "%s", params);
    SanitizeText(clean);
    format(line, sizeof line, "(( %s: %s ))", RPName(playerid), clean);
    SendNearby(playerid, CHAT_NORMAL_RADIUS, COLOR_OOC, line);
    return 1;
}

CMD:pm(playerid, params[])
{
    new target = ParamPlayer(params, 0), text[128];
    if (target == INVALID_PLAYER_ID || !GetParamRest(params, 1, text)) return Usage(playerid, "/pm [id/name] [message]");
    if (target == playerid) return Error(playerid, "Cannot PM yourself.");
    new clean[128];
    format(clean, sizeof clean, "%s", text);
    SanitizeText(clean);
    SendClientMessage(target, COLOR_YELLOW, "PM from %s (ID %d): %s", RPName(playerid), playerid, clean);
    SendClientMessage(playerid, COLOR_YELLOW, "PM to %s (ID %d): %s", RPName(target), target, clean);
    Player[target][pLastPM] = playerid;
    Player[playerid][pLastPM] = target;
    return 1;
}

CMD:re(playerid, params[])
{
    new target = Player[playerid][pLastPM];
    if (target == INVALID_PLAYER_ID || !IsPlayerConnected(target)) return Error(playerid, "No one to reply to.");
    if (isnull(params)) return Usage(playerid, "/re [message]");
    new clean[128];
    format(clean, sizeof clean, "%s", params);
    SanitizeText(clean);
    SendClientMessage(target, COLOR_YELLOW, "PM from %s (ID %d): %s", RPName(playerid), playerid, clean);
    SendClientMessage(playerid, COLOR_YELLOW, "PM to %s (ID %d): %s", RPName(target), target, clean);
    return 1;
}

// ---------------------------------------------------------------------
//  Animations
// ---------------------------------------------------------------------
CMD:anim(playerid, params[])
{
    new name[32];
    if (!GetParam(params, 0, name))
        return Usage(playerid, "/anim [handsup/sit/smoke/dance/lean/wave/crossarms/lay/stop]");
    if (!strcmp(name, "stop", true))
    {
        ClearAnimations(playerid);
        SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
        return 1;
    }
    if (!strcmp(name, "handsup", true)) { SetPlayerSpecialAction(playerid, SPECIAL_ACTION_HANDSUP); return 1; }
    if (!strcmp(name, "sit", true))     { ApplyAnimation(playerid, "PED", "SEAT_idle", 4.1, false, false, false, true, 0, SYNC_ALL); return 1; }
    if (!strcmp(name, "smoke", true))   { ApplyAnimation(playerid, "SMOKING", "M_smkstnd_loop", 4.1, true, false, false, false, 0, SYNC_ALL); return 1; }
    if (!strcmp(name, "dance", true))   { SetPlayerSpecialAction(playerid, SPECIAL_ACTION_DANCE1); return 1; }
    if (!strcmp(name, "lean", true))    { ApplyAnimation(playerid, "GANGS", "leanIDLE", 4.1, false, false, false, true, 0, SYNC_ALL); return 1; }
    if (!strcmp(name, "wave", true))    { ApplyAnimation(playerid, "ON_LOOKERS", "wave_loop", 4.1, false, false, false, false, 0, SYNC_ALL); return 1; }
    if (!strcmp(name, "crossarms", true)) { ApplyAnimation(playerid, "COP_AMBIENT", "Coplook_loop", 4.1, false, false, false, true, 0, SYNC_ALL); return 1; }
    if (!strcmp(name, "lay", true))     { ApplyAnimation(playerid, "BEACH", "bather", 4.1, false, false, false, true, 0, SYNC_ALL); return 1; }
    Usage(playerid, "/anim [handsup/sit/smoke/dance/lean/wave/crossarms/lay/stop]");
    return 1;
}

CMD:handsup(playerid, params[]) { SetPlayerSpecialAction(playerid, SPECIAL_ACTION_HANDSUP); return 1; }
CMD:stopanim(playerid, params[]) { ClearAnimations(playerid); SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE); return 1; }
