// =====================================================================
//  Kingmakers RP - phone system: contacts, calls, SMS, GPS, emergency
// =====================================================================

enum E_GPS { gName[32], Float:gX, Float:gY, Float:gZ }
new const GPSLocations[][E_GPS] =
{
    { "City Hall & Job Center",  CITYHALL_X, CITYHALL_Y, CITYHALL_Z },
    { "Unity Station (Spawn)",    SPAWN_X,    SPAWN_Y,    SPAWN_Z },
    { "Los Santos Police Dept.", LSPD_X,     LSPD_Y,     LSPD_Z },
    { "All Saints Hospital (EMS)",HOSPITAL_X, HOSPITAL_Y, HOSPITAL_Z },
    { "Kingmakers Motors (Dealer)",DEALER_X,   DEALER_Y,   DEALER_Z },
    { "Hunter Quarry (Miner)",   690.00,     845.00,     -42.96 },
    { "Ocean Docks (Trucker)",   2195.00,    -2240.00,   13.55 },
    { "Taxi Depot",              1775.00,    -1905.00,   13.39 },
    { "Mechanic Garage",         2063.00,    -1831.00,   13.55 },
    { "Ammu-Nation (Market)",    1368.35,    -1279.06,   13.55 },
    { "Idlewood Gas Station",    1940.70,    -1772.90,   13.39 },
    { "Binco Clothes (Ganton)",  2244.32,    -1665.52,   15.48 },
    { "Bank / ATM (City Hall)",  1497.70,    -1749.90,   15.45 }
};

stock ShowPhone(playerid)
{
    if (!Player[playerid][pPhone]) return Error(playerid, "You don't have a phone. Buy one at a 24/7 store (/gps).");
    if (!Player[playerid][pPhoneOn]) return Error(playerid, "Your phone is turned off. Use /togglephone to turn it on.");
    new body[256];
    format(body, sizeof body, "KingPhone OS  -  #%d\n\n\
        Contacts\n\
        Messages (SMS)\n\
        Call number\n\
        GPS Navigator\n\
        Emergency Services (911)\n\
        Bank app\n\
        Turn off phone", Player[playerid][pPhone]);
    ShowPlayerDialog(playerid, DLG_PHONE, DIALOG_STYLE_LIST, C_GOLD "KingPhone", body, "Open", "Put away");
    return 1;
}

stock FindPlayerByPhone(number)
{
    if (number == 0) return INVALID_PLAYER_ID;
    for (new i = 0; i < MAX_PLAYERS; i++)
        if (IsPlayerConnected(i) && Player[i][pLogged] && Player[i][pPhone] == number) return i;
    return INVALID_PLAYER_ID;
}

stock CheckUnreadSMS(playerid)
{
    if (!Player[playerid][pPhone]) return;
    new q[128];
    format(q, sizeof q, "SELECT COUNT(*) FROM sms WHERE to_num = %d AND seen = 0", Player[playerid][pPhone]);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    if (r)
    {
        new count = DB_GetFieldInt(r, 0);
        DB_FreeResultSet(r);
        if (count > 0) SendClientMessage(playerid, COLOR_PHONE, "[PHONE] You have %d unread text message(s). Check /phone > Messages.", count);
    }
}

stock ShowPhoneContacts(playerid)
{
    new q[128];
    format(q, sizeof q, "SELECT id, name, number FROM contacts WHERE owner = %d ORDER BY name ASC", Player[playerid][pID]);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    new body[1024];
    body = "Name\tNumber\n" C_GREEN "+ Add new contact\t-\n";
    if (r && DB_GetRowCount(r) > 0)
    {
        do
        {
            new cname[32], num;
            DB_GetFieldStringByName(r, "name", cname);
            num = DB_GetFieldIntByName(r, "number");
            format(body, sizeof body, "%s%s\t%d\n", body, cname, num);
        }
        while (DB_SelectNextRow(r));
    }
    if (r) DB_FreeResultSet(r);
    ShowPlayerDialog(playerid, DLG_PHONE_CONTACTS, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "KingPhone " C_WHITE "- Contacts", body, "Select", "Back");
}

stock ShowPhoneInbox(playerid)
{
    new q[160];
    format(q, sizeof q, "SELECT id, from_num, text FROM sms WHERE to_num = %d ORDER BY id DESC LIMIT 15", Player[playerid][pPhone]);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    new body[1024];
    body = "From\tMessage\n" C_GREEN "+ Send new SMS\t-\n";
    if (r && DB_GetRowCount(r) > 0)
    {
        do
        {
            new from, text[64];
            from = DB_GetFieldIntByName(r, "from_num");
            DB_GetFieldStringByName(r, "text", text);
            format(body, sizeof body, "%s#%d\t%s\n", body, from, text);
        }
        while (DB_SelectNextRow(r));
        DB_Exec("UPDATE sms SET seen = 1 WHERE to_num = %d", Player[playerid][pPhone]);
    }
    if (r) DB_FreeResultSet(r);
    ShowPlayerDialog(playerid, DLG_PHONE_INBOX, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "KingPhone " C_WHITE "- Messages", body, "Select", "Back");
}

stock ShowGPSDialog(playerid)
{
    new body[1024];
    for (new i = 0; i < sizeof GPSLocations; i++)
        format(body, sizeof body, "%s%s\n", body, GPSLocations[i][gName]);
    ShowPlayerDialog(playerid, DLG_PHONE_GPS, DIALOG_STYLE_LIST, C_GOLD "GPS Navigator " C_WHITE "- choose destination", body, "Set Route", "Back");
}

stock EndCall(playerid)
{
    new other = Player[playerid][pCallWith];
    if (other != INVALID_PLAYER_ID && IsPlayerConnected(other))
    {
        SendClientMessage(other, COLOR_PHONE, "[PHONE] Call ended.");
        Player[other][pCallWith] = INVALID_PLAYER_ID;
        Player[other][pCallerOf] = INVALID_PLAYER_ID;
        SetPlayerSpecialAction(other, SPECIAL_ACTION_NONE);
    }
    Player[playerid][pCallWith] = INVALID_PLAYER_ID;
    Player[playerid][pCallerOf] = INVALID_PLAYER_ID;
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
    SendClientMessage(playerid, COLOR_PHONE, "[PHONE] Call ended.");
}

stock Phone_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid)
    {
        case DLG_PHONE:
        {
            if (!response) return 1;
            switch (listitem)
            {
                case 0: ShowPhoneContacts(playerid);
                case 1: ShowPhoneInbox(playerid);
                case 2: ShowPlayerDialog(playerid, DLG_PHONE_CALL, DIALOG_STYLE_INPUT, C_GOLD "KingPhone " C_WHITE "- Call", C_WHITE "Enter phone number:", "Call", "Back");
                case 3: ShowGPSDialog(playerid);
                case 4:
                {
                    ShowPlayerDialog(playerid, DLG_PHONE_SERVICES, DIALOG_STYLE_LIST, C_GOLD "Emergency Services",
                        "Police (LSPD - 911)\nAmbulance / EMS (912)\nTaxi Dispatch (333)\nMechanic Dispatch (555)", "Call", "Back");
                }
                case 5:
                {
                    ShowPlayerDialog(playerid, DLG_INFO, DIALOG_STYLE_MSGBOX, C_GOLD "Kingmakers Bank Mobile",
                        C_WHITE "Holder: " C_GOLD "%s\n\n" C_WHITE "Balance: " C_GREEN "%s\n\n" C_WHITE "Use an ATM or bank branch for cash operations.",
                        "Close", "", RPName(playerid), FormatMoney(Player[playerid][pBank]));
                }
                case 6:
                {
                    Player[playerid][pPhoneOn] = false;
                    Info(playerid, "You turned off your phone. /togglephone to turn it back on.");
                }
            }
            return 1;
        }
        case DLG_PHONE_GPS:
        {
            if (!response) return ShowPhone(playerid), 1;
            if (listitem < 0 || listitem >= sizeof GPSLocations) return 1;
            SetCP(playerid, CP_GPS, GPSLocations[listitem][gX], GPSLocations[listitem][gY], GPSLocations[listitem][gZ], 4.0);
            SendClientMessage(playerid, COLOR_GREEN, "[GPS] Route set to: %s. Follow the red checkpoint on your radar.", GPSLocations[listitem][gName]);
            return 1;
        }
        case DLG_PHONE_SERVICES:
        {
            if (!response) return ShowPhone(playerid), 1;
            new Float:x, Float:y, Float:z, msg[160];
            GetPlayerPos(playerid, x, y, z);
            switch (listitem)
            {
                case 0: // 911 Police
                {
                    format(msg, sizeof msg, "[911 CALL] %s reported an emergency near (%.0f, %.0f). Phone: #%d.", RPName(playerid), x, y, Player[playerid][pPhone]);
                    SendFactionMessage(FACTION_LSPD, FactionColors[FACTION_LSPD], msg);
                    SendClientMessage(playerid, COLOR_LIGHTBLUE, "Your 911 call was dispatched to the LSPD.");
                }
                case 1: // EMS
                {
                    format(msg, sizeof msg, "[EMS DISPATCH] %s requested medical assistance near (%.0f, %.0f). Phone: #%d.", RPName(playerid), x, y, Player[playerid][pPhone]);
                    SendFactionMessage(FACTION_EMS, FactionColors[FACTION_EMS], msg);
                    SendClientMessage(playerid, COLOR_LIGHTBLUE, "Your medical call was dispatched to All Saints Hospital.");
                }
                case 2: // Taxi
                {
                    format(msg, sizeof msg, "[TAXI DISPATCH] %s requested a cab near (%.0f, %.0f). Phone: #%d.", RPName(playerid), x, y, Player[playerid][pPhone]);
                    SendJobMessage(JOB_TAXI, COLOR_YELLOW, msg);
                    SendClientMessage(playerid, COLOR_YELLOW, "Your ride request was broadcast to all active taxi drivers.");
                }
                case 3: // Mechanic
                {
                    format(msg, sizeof msg, "[MECHANIC DISPATCH] %s requested roadside assistance near (%.0f, %.0f). Phone: #%d.", RPName(playerid), x, y, Player[playerid][pPhone]);
                    SendJobMessage(JOB_MECHANIC, COLOR_YELLOW, msg);
                    SendClientMessage(playerid, COLOR_YELLOW, "Your roadside assistance request was sent to active mechanics.");
                }
            }
            return 1;
        }
        case DLG_PHONE_CONTACTS:
        {
            if (!response) return ShowPhone(playerid), 1;
            if (listitem == 0)
            {
                ShowPlayerDialog(playerid, DLG_PHONE_ADDCONTACT_NAME, DIALOG_STYLE_INPUT, C_GOLD "Add Contact " C_WHITE "- Name",
                    C_WHITE "Enter contact name:", "Next", "Back");
                return 1;
            }
            // fetch chosen contact
            new q[144];
            format(q, sizeof q, "SELECT id, name, number FROM contacts WHERE owner = %d ORDER BY name ASC LIMIT 1 OFFSET %d", Player[playerid][pID], listitem - 1);
            new DBResult:r = DB_ExecuteQuery(gDB, q);
            if (r && DB_GetRowCount(r) > 0)
            {
                Player[playerid][pContactSel] = DB_GetFieldIntByName(r, "id");
                new cname[32], num;
                DB_GetFieldStringByName(r, "name", cname);
                num = DB_GetFieldIntByName(r, "number");
                DB_FreeResultSet(r);
                Player[playerid][pSMSTarget] = num;
                new body[128];
                format(body, sizeof body, "Call (#%d)\nSend SMS\nDelete contact", num);
                ShowPlayerDialog(playerid, DLG_PHONE_CONTACT_ACTION, DIALOG_STYLE_LIST, cname, body, "Select", "Back");
            }
            else if (r) DB_FreeResultSet(r);
            return 1;
        }
        case DLG_PHONE_ADDCONTACT_NAME:
        {
            if (!response) return ShowPhoneContacts(playerid), 1;
            if (strlen(inputtext) < 2 || strlen(inputtext) > 24) return Error(playerid, "Name must be 2-24 characters."), ShowPhoneContacts(playerid), 1;
            format(Player[playerid][pTempName], 32, "%s", inputtext);
            ShowPlayerDialog(playerid, DLG_PHONE_ADDCONTACT_NUM, DIALOG_STYLE_INPUT, C_GOLD "Add Contact " C_WHITE "- Number",
                C_WHITE "Enter phone number:", "Save", "Back");
            return 1;
        }
        case DLG_PHONE_ADDCONTACT_NUM:
        {
            if (!response) return ShowPhoneContacts(playerid), 1;
            new num = strval(inputtext);
            if (num < 100000 || num > 999999) return Error(playerid, "Invalid 6-digit phone number."), ShowPhoneContacts(playerid), 1;
            new q[256], esc[64];
            SQLEscape(Player[playerid][pTempName], esc);
            format(q, sizeof q, "INSERT INTO contacts (owner, name, number) VALUES (%d, '%s', %d)", Player[playerid][pID], esc, num);
            DB_Exec(q);
            SendClientMessage(playerid, COLOR_GREEN, "Contact saved: %s (#%d)", Player[playerid][pTempName], num);
            ShowPhoneContacts(playerid);
            return 1;
        }
        case DLG_PHONE_CONTACT_ACTION:
        {
            if (!response) return ShowPhoneContacts(playerid), 1;
            new num = Player[playerid][pSMSTarget];
            if (listitem == 0) // Call
            {
                new target = FindPlayerByPhone(num);
                if (target == INVALID_PLAYER_ID) return Error(playerid, "That number is currently unreachable.");
                CallPlayer(playerid, target);
            }
            else if (listitem == 1) // SMS
            {
                ShowPlayerDialog(playerid, DLG_PHONE_SMS_TEXT, DIALOG_STYLE_INPUT, C_GOLD "Send SMS",
                    C_WHITE "Type your message to #" C_GOLD "%d" C_WHITE ":", "Send", "Back", num);
            }
            else if (listitem == 2) // Delete
            {
                new q[96];
                format(q, sizeof q, "DELETE FROM contacts WHERE id = %d AND owner = %d", Player[playerid][pContactSel], Player[playerid][pID]);
                DB_Exec(q);
                Info(playerid, "Contact deleted.");
                ShowPhoneContacts(playerid);
            }
            return 1;
        }
        case DLG_PHONE_CALL:
        {
            if (!response) return ShowPhone(playerid), 1;
            new num = strval(inputtext);
            new target = FindPlayerByPhone(num);
            if (target == INVALID_PLAYER_ID) return Error(playerid, "Number unreachable or player offline.");
            CallPlayer(playerid, target);
            return 1;
        }
        case DLG_PHONE_SMS_TEXT:
        {
            if (!response) return ShowPhone(playerid), 1;
            SendSMSToNumber(playerid, Player[playerid][pSMSTarget], inputtext);
            return 1;
        }
        case DLG_PHONE_INBOX:
        {
            if (!response) return ShowPhone(playerid), 1;
            if (listitem == 0)
            {
                ShowPlayerDialog(playerid, DLG_PHONE_SMS_NUM, DIALOG_STYLE_INPUT, C_GOLD "Send SMS " C_WHITE "- Recipient",
                    C_WHITE "Enter phone number:", "Next", "Back");
            }
            return 1;
        }
        case DLG_PHONE_SMS_NUM:
        {
            if (!response) return ShowPhoneInbox(playerid), 1;
            new num = strval(inputtext);
            if (num < 100000 || num > 999999) return Error(playerid, "Invalid number."), ShowPhoneInbox(playerid), 1;
            Player[playerid][pSMSTarget] = num;
            ShowPlayerDialog(playerid, DLG_PHONE_SMS_TEXT, DIALOG_STYLE_INPUT, C_GOLD "Send SMS",
                C_WHITE "Type your message to #" C_GOLD "%d" C_WHITE ":", "Send", "Back", num);
            return 1;
        }
    }
    return 0;
}

stock CallPlayer(playerid, target)
{
    if (target == playerid) return Error(playerid, "You cannot call yourself.");
    if (Player[playerid][pCallWith] != INVALID_PLAYER_ID) return Error(playerid, "You are already on a call.");
    if (Player[target][pCallWith] != INVALID_PLAYER_ID) return Error(playerid, "That line is currently busy.");
    if (!Player[target][pPhoneOn]) return Error(playerid, "That phone is turned off.");
    Player[playerid][pCallerOf] = target;
    Player[target][pCallerOf] = playerid;
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_USECELLPHONE);
    PlayerPlaySound(playerid, 1052, 0.0, 0.0, 0.0);
    PlayerPlaySound(target, 1052, 0.0, 0.0, 0.0);
    SendClientMessage(playerid, COLOR_PHONE, "[PHONE] Calling #%d... (wait for them to /p)", Player[target][pPhone]);
    SendClientMessage(target, COLOR_PHONE, "[PHONE] Incoming call from #%d! Use /p to answer, /h to decline.", Player[playerid][pPhone]);
    new msg[96];
    format(msg, sizeof msg, "* %s's phone starts ringing.", RPName(target));
    SendNearby(target, 10.0, COLOR_PURPLE, msg);
}

stock SendSMSToNumber(playerid, number, const text[])
{
    if (isnull(text)) return Error(playerid, "Message cannot be empty.");
    if (Player[playerid][pMoney] < 2) return Error(playerid, "An SMS costs $2.");
    GiveMoney(playerid, -2);
    new clean[128], q[384], esc[256];
    format(clean, sizeof clean, "%s", text);
    SanitizeText(clean);
    SQLEscape(clean, esc);
    format(q, sizeof q, "INSERT INTO sms (from_num, to_num, text, time, seen) VALUES (%d, %d, '%s', %d, 0)",
        Player[playerid][pPhone], number, esc, gettime());
    DB_Exec(q);

    SendClientMessage(playerid, COLOR_PHONE, "[SMS sent to #%d] %s", number, clean);
    new target = FindPlayerByPhone(number);
    if (target != INVALID_PLAYER_ID && Player[target][pPhoneOn])
    {
        SendClientMessage(target, COLOR_PHONE, "[SMS from #%d] %s", Player[playerid][pPhone], clean);
        PlayerPlaySound(target, 1054, 0.0, 0.0, 0.0);
        new msg[96];
        format(msg, sizeof msg, "* %s's phone beeps with a new message.", RPName(target));
        SendNearby(target, 10.0, COLOR_PURPLE, msg);
    }
}

// ---------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------
CMD:phone(playerid, params[]) return ShowPhone(playerid);
CMD:p(playerid, params[]) // answer call
{
    new caller = Player[playerid][pCallerOf];
    if (caller == INVALID_PLAYER_ID || !IsPlayerConnected(caller)) return Error(playerid, "Your phone is not ringing.");
    Player[playerid][pCallWith] = caller;
    Player[caller][pCallWith] = playerid;
    Player[playerid][pCallerOf] = INVALID_PLAYER_ID;
    Player[caller][pCallerOf] = INVALID_PLAYER_ID;
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_USECELLPHONE);
    SendClientMessage(playerid, COLOR_PHONE, "[PHONE] Call connected. Type into chat normally to talk. Use /h to hang up.");
    SendClientMessage(caller, COLOR_PHONE, "[PHONE] %s answered. Type into chat to talk. Use /h to hang up.", RPName(playerid));
    return 1;
}

CMD:h(playerid, params[])
{
    if (Player[playerid][pCallWith] == INVALID_PLAYER_ID && Player[playerid][pCallerOf] == INVALID_PLAYER_ID)
        return Error(playerid, "You are not on a call.");
    EndCall(playerid);
    return 1;
}

CMD:sms(playerid, params[])
{
    if (!Player[playerid][pPhone]) return Error(playerid, "You don't have a phone.");
    new num, text[128];
    if (!ParamInt(params, 0, num) || !GetParamRest(params, 1, text)) return Usage(playerid, "/sms [number] [message]");
    SendSMSToNumber(playerid, num, text);
    return 1;
}

CMD:call(playerid, params[])
{
    if (!Player[playerid][pPhone]) return Error(playerid, "You don't have a phone.");
    new num;
    if (!ParamInt(params, 0, num)) return Usage(playerid, "/call [number]");
    new target = FindPlayerByPhone(num);
    if (target == INVALID_PLAYER_ID) return Error(playerid, "Number unreachable.");
    CallPlayer(playerid, target);
    return 1;
}

CMD:number(playerid, params[])
{
    if (!Player[playerid][pPhone]) return Error(playerid, "You don't have a phone.");
    Info(playerid, "Your phone number is #%d.", Player[playerid][pPhone]);
    return 1;
}

CMD:togglephone(playerid, params[])
{
    if (!Player[playerid][pPhone]) return Error(playerid, "You don't have a phone.");
    Player[playerid][pPhoneOn] = !Player[playerid][pPhoneOn];
    Info(playerid, Player[playerid][pPhoneOn] ? ("Phone turned ON.") : ("Phone turned OFF."));
    return 1;
}

CMD:gps(playerid, params[])
{
    ShowGPSDialog(playerid);
    return 1;
}
