// =====================================================================
//  Kingmakers RP - HUD, branding textdraws & speedometer
// =====================================================================

new Text:TD_Logo[3];
new Text:TD_LoginTitle[4];
new PlayerText:PTD_Speedo[MAX_PLAYERS];
new PlayerText:PTD_Info[MAX_PLAYERS];
new gServerHour = 12;
new gServerWeather = 1;

stock CreateGlobalTextDraws()
{
    // Corner logo: crown + KINGMAKERS RP
    TD_Logo[0] = TextDrawCreate(560.0, 4.0, "~y~]");
    TextDrawFont(TD_Logo[0], TEXT_DRAW_FONT_2);
    TextDrawLetterSize(TD_Logo[0], 0.45, 1.4);
    TextDrawAlignment(TD_Logo[0], TEXT_DRAW_ALIGN_CENTER);
    TextDrawSetOutline(TD_Logo[0], 1);
    TextDrawColour(TD_Logo[0], 0xFFC83DFF);

    TD_Logo[1] = TextDrawCreate(560.0, 16.0, "KINGMAKERS");
    TextDrawFont(TD_Logo[1], TEXT_DRAW_FONT_3);
    TextDrawLetterSize(TD_Logo[1], 0.36, 1.3);
    TextDrawAlignment(TD_Logo[1], TEXT_DRAW_ALIGN_CENTER);
    TextDrawSetOutline(TD_Logo[1], 1);
    TextDrawBackgroundColour(TD_Logo[1], 0x000000AA);
    TextDrawColour(TD_Logo[1], 0xFFC83DFF);

    TD_Logo[2] = TextDrawCreate(560.0, 28.0, "~w~ROLE~y~PLAY");
    TextDrawFont(TD_Logo[2], TEXT_DRAW_FONT_2);
    TextDrawLetterSize(TD_Logo[2], 0.18, 0.8);
    TextDrawAlignment(TD_Logo[2], TEXT_DRAW_ALIGN_CENTER);
    TextDrawSetOutline(TD_Logo[2], 1);
    TextDrawColour(TD_Logo[2], 0xFFFFFFFF);

    // Login screen
    TD_LoginTitle[0] = TextDrawCreate(320.0, 0.0, "_");
    TextDrawUseBox(TD_LoginTitle[0], true);
    TextDrawBoxColour(TD_LoginTitle[0], 0x000000AA);
    TextDrawLetterSize(TD_LoginTitle[0], 0.0, 9.0);
    TextDrawTextSize(TD_LoginTitle[0], 0.0, 640.0);
    TextDrawAlignment(TD_LoginTitle[0], TEXT_DRAW_ALIGN_CENTER);

    TD_LoginTitle[1] = TextDrawCreate(320.0, 18.0, "KINGMAKERS ~y~RP");
    TextDrawFont(TD_LoginTitle[1], TEXT_DRAW_FONT_3);
    TextDrawLetterSize(TD_LoginTitle[1], 0.9, 3.2);
    TextDrawAlignment(TD_LoginTitle[1], TEXT_DRAW_ALIGN_CENTER);
    TextDrawSetOutline(TD_LoginTitle[1], 2);
    TextDrawColour(TD_LoginTitle[1], 0xFFC83DFF);

    TD_LoginTitle[2] = TextDrawCreate(320.0, 52.0, "MOBILE ROLEPLAY  -  RULE THE CITY");
    TextDrawFont(TD_LoginTitle[2], TEXT_DRAW_FONT_2);
    TextDrawLetterSize(TD_LoginTitle[2], 0.25, 1.1);
    TextDrawAlignment(TD_LoginTitle[2], TEXT_DRAW_ALIGN_CENTER);
    TextDrawSetOutline(TD_LoginTitle[2], 1);
    TextDrawColour(TD_LoginTitle[2], 0xFFFFFFFF);

    TD_LoginTitle[3] = TextDrawCreate(320.0, 430.0, "~y~" SERVER_WEBSITE "  ~w~|  v" SERVER_VERSION);
    TextDrawFont(TD_LoginTitle[3], TEXT_DRAW_FONT_1);
    TextDrawLetterSize(TD_LoginTitle[3], 0.22, 1.0);
    TextDrawAlignment(TD_LoginTitle[3], TEXT_DRAW_ALIGN_CENTER);
    TextDrawSetOutline(TD_LoginTitle[3], 1);
}

stock CreatePlayerTextDraws(playerid)
{
    PTD_Speedo[playerid] = CreatePlayerTextDraw(playerid, 500.0, 380.0, "_");
    PlayerTextDrawFont(playerid, PTD_Speedo[playerid], TEXT_DRAW_FONT_2);
    PlayerTextDrawLetterSize(playerid, PTD_Speedo[playerid], 0.24, 1.1);
    PlayerTextDrawSetOutline(playerid, PTD_Speedo[playerid], 1);
    PlayerTextDrawUseBox(playerid, PTD_Speedo[playerid], true);
    PlayerTextDrawBoxColour(playerid, PTD_Speedo[playerid], 0x00000088);
    PlayerTextDrawTextSize(playerid, PTD_Speedo[playerid], 630.0, 0.0);

    PTD_Info[playerid] = CreatePlayerTextDraw(playerid, 320.0, 405.0, "_");
    PlayerTextDrawFont(playerid, PTD_Info[playerid], TEXT_DRAW_FONT_1);
    PlayerTextDrawLetterSize(playerid, PTD_Info[playerid], 0.28, 1.2);
    PlayerTextDrawAlignment(playerid, PTD_Info[playerid], TEXT_DRAW_ALIGN_CENTER);
    PlayerTextDrawSetOutline(playerid, PTD_Info[playerid], 1);
}

stock ShowLoginTextDraws(playerid)
{
    for (new i = 0; i < sizeof TD_LoginTitle; i++) TextDrawShowForPlayer(playerid, TD_LoginTitle[i]);
}

stock HideLoginTextDraws(playerid)
{
    for (new i = 0; i < sizeof TD_LoginTitle; i++) TextDrawHideForPlayer(playerid, TD_LoginTitle[i]);
}

stock ShowHUD(playerid)
{
    for (new i = 0; i < sizeof TD_Logo; i++) TextDrawShowForPlayer(playerid, TD_Logo[i]);
}

// Short on-screen hint above the minimap area (auto-hides).
stock ShowInfoText(playerid, const text[], time = 4000)
{
    PlayerTextDrawSetString(playerid, PTD_Info[playerid], text);
    PlayerTextDrawShow(playerid, PTD_Info[playerid]);
    SetTimerEx("HideInfoText", time, false, "i", playerid);
}

forward HideInfoText(playerid);
public HideInfoText(playerid)
{
    if (IsPlayerConnected(playerid)) PlayerTextDrawHide(playerid, PTD_Info[playerid]);
    return 1;
}

stock Float:GetVehicleSpeedKMH(vehicleid)
{
    new Float:vx, Float:vy, Float:vz;
    GetVehicleVelocity(vehicleid, vx, vy, vz);
    return floatsqroot(vx * vx + vy * vy + vz * vz) * 180.0;
}

stock UpdateSpeedo(playerid)
{
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || GetPlayerState(playerid) != PLAYER_STATE_DRIVER)
    {
        PlayerTextDrawHide(playerid, PTD_Speedo[playerid]);
        return;
    }
    new engine, lights, alarm, doors, bonnet, boot, objective, str[160], Float:hp;
    GetVehicleParamsEx(vid, engine, lights, alarm, doors, bonnet, boot, objective);
    GetVehicleHealth(vid, hp);
    format(str, sizeof str, "~y~%d ~w~km/h~n~Fuel: %s%d%%~n~~w~Engine: %s~n~~w~Lock: %s~n~~w~HP: %d%%",
        floatround(GetVehicleSpeedKMH(vid)),
        VehFuel[vid] < 20 ? ("~r~") : ("~g~"), VehFuel[vid],
        engine == 1 ? ("~g~ON") : ("~r~OFF"),
        doors == 1 ? ("~r~LOCKED") : ("~g~OPEN"),
        floatround((hp - 250.0) / 7.5));
    PlayerTextDrawSetString(playerid, PTD_Speedo[playerid], str);
    PlayerTextDrawShow(playerid, PTD_Speedo[playerid]);
}
