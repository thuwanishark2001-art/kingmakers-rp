// =====================================================================
//  Kingmakers RP - vehicles: ownership, dealership, fuel, engine, locks
// =====================================================================

enum
{
    VT_NONE = 0,
    VT_OWNED,
    VT_JOB,
    VT_FACTION,
    VT_ADMIN,
    VT_RENTAL
}

new VehType[MAX_VEHICLES];
new VehOwner[MAX_VEHICLES];       // account id
new VehDBID[MAX_VEHICLES];
new VehJob[MAX_VEHICLES];
new VehFaction[MAX_VEHICLES];
new bool:VehLocked[MAX_VEHICLES];
new VehFuelTick[MAX_VEHICLES];

enum E_DEALER
{
    dModel,
    dName[24],
    dPrice,
    dLevel
}
new const DealerCars[][E_DEALER] =
{
    { 462, "Faggio Scooter",      2500,   1 },
    { 404, "Perennial",           6000,   1 },
    { 466, "Glendale",            8500,   1 },
    { 468, "Sanchez",             9000,   1 },
    { 421, "Washington",          11000,  2 },
    { 445, "Admiral",             12000,  2 },
    { 581, "BF-400",              12000,  2 },
    { 400, "Landstalker",         14000,  2 },
    { 405, "Sentinel",            15000,  2 },
    { 536, "Blade",               16000,  3 },
    { 567, "Savanna",             18000,  3 },
    { 579, "Huntley",             35000,  3 },
    { 402, "Buffalo",             38000,  4 },
    { 562, "Elegy",               40000,  4 },
    { 560, "Sultan",              45000,  4 },
    { 522, "NRG-500",             60000,  5 },
    { 415, "Cheetah",             110000, 6 },
    { 451, "Turismo",             130000, 7 },
    { 541, "Bullet",              140000, 7 },
    { 411, "Infernus",            150000, 8 }
};

new const Float:GasStations[][3] =
{
    { 1940.70, -1772.90, 13.39 },   // Idlewood
    { 1004.10, -939.30,  42.18 },   // Mulholland
    { 655.60,  -565.00,  16.34 },   // Dillimore
    { -90.50,  -1169.40, 2.42 },    // Flint County
    { 2202.00, 2474.00,  10.82 }    // Las Venturas
};

stock GetVehicleNameByModel(model, dest[], size = sizeof dest)
{
    for (new i = 0; i < sizeof DealerCars; i++)
    {
        if (DealerCars[i][dModel] == model) { format(dest, size, "%s", DealerCars[i][dName]); return 1; }
    }
    switch (model)
    {
        case 420: format(dest, size, "Taxi");
        case 438: format(dest, size, "Cabbie");
        case 414: format(dest, size, "Mule");
        case 525: format(dest, size, "Tow Truck");
        case 596: format(dest, size, "Police Cruiser");
        case 523: format(dest, size, "HPV-1000");
        case 416: format(dest, size, "Ambulance");
        case 492: format(dest, size, "Greenwood");
        case 566: format(dest, size, "Tahoma");
        case 486: format(dest, size, "Dozer");
        default: format(dest, size, "Vehicle #%d", model);
    }
    return 1;
}

stock SetupVehicle(vid, type, Float:fuel = 100.0)
{
    if (vid == INVALID_VEHICLE_ID || vid <= 0 || vid >= MAX_VEHICLES) return;
    VehType[vid] = type;
    VehOwner[vid] = 0;
    VehDBID[vid] = 0;
    VehJob[vid] = JOB_NONE;
    VehFaction[vid] = FACTION_NONE;
    VehFuel[vid] = floatround(fuel);
    VehLocked[vid] = false;
    VehFuelTick[vid] = 0;
    SetVehicleParamsEx(vid, 0, 0, 0, 0, 0, 0, 0);
}

stock CreateJobVehicle(model, Float:x, Float:y, Float:z, Float:a, c1, c2, job)
{
    new vid = CreateVehicle(model, x, y, z, a, c1, c2, 600);
    SetupVehicle(vid, VT_JOB);
    VehJob[vid] = job;
    return vid;
}

stock CreateFactionVehicle(model, Float:x, Float:y, Float:z, Float:a, c1, c2, faction)
{
    new vid = CreateVehicle(model, x, y, z, a, c1, c2, 600, faction == FACTION_LSPD || faction == FACTION_EMS);
    SetupVehicle(vid, VT_FACTION);
    VehFaction[vid] = faction;
    return vid;
}

stock CreateStaticVehicles()
{
    // Taxi depot (Unity Station)
    for (new i = 0; i < 6; i++) CreateJobVehicle(420, 1777.0 + (i * 4.0), -1932.0, 13.39, 0.0, 6, 1, JOB_TAXI);
    // Trucker depot (Ocean Docks)
    for (new i = 0; i < 6; i++) CreateJobVehicle(414, 2180.0 + (i * 5.0), -2250.0, 13.55, 225.0, 1, 1, JOB_TRUCKER);
    // Mechanic garage (Idlewood)
    for (new i = 0; i < 4; i++) CreateJobVehicle(525, 2060.0 + (i * 5.0), -1870.0, 13.55, 0.0, 1, 1, JOB_MECHANIC);

    // LSPD garage
    for (new i = 0; i < 6; i++) CreateFactionVehicle(596, 1545.0 + (i * 4.5), -1680.0, 5.89, 90.0, 0, 1, FACTION_LSPD);
    for (new i = 0; i < 2; i++) CreateFactionVehicle(523, 1545.0 + (i * 3.0), -1650.0, 5.89, 90.0, 0, 1, FACTION_LSPD);
    // EMS (All Saints)
    for (new i = 0; i < 4; i++) CreateFactionVehicle(416, 1178.0 + (i * 5.0), -1338.5, 14.0, 270.0, 1, 3, FACTION_EMS);
    // Grove Street
    for (new i = 0; i < 3; i++) CreateFactionVehicle(492, 2473.0 + (i * 5.0), -1695.0, 13.5, 0.0, 86, 86, FACTION_GROVE);
    // Ballas
    for (new i = 0; i < 3; i++) CreateFactionVehicle(566, 2010.0 + (i * 5.0), -1130.0, 25.3, 90.0, 85, 85, FACTION_BALLAS);
}

// ---------------------------------------------------------------------
//  Owned vehicles
// ---------------------------------------------------------------------
stock LoadPlayerVehicles(playerid)
{
    new q[128];
    format(q, sizeof q, "SELECT * FROM vehicles WHERE owner = %d", Player[playerid][pID]);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    if (!r) return;
    if (DB_GetRowCount(r) > 0)
    {
        do
        {
            new vid = CreateVehicle(DB_GetFieldIntByName(r, "model"), DB_GetFieldFloatByName(r, "x"), DB_GetFieldFloatByName(r, "y"),
                DB_GetFieldFloatByName(r, "z"), DB_GetFieldFloatByName(r, "a"), DB_GetFieldIntByName(r, "c1"), DB_GetFieldIntByName(r, "c2"), -1);
            if (vid == INVALID_VEHICLE_ID) continue;
            SetupVehicle(vid, VT_OWNED, float(DB_GetFieldIntByName(r, "fuel")));
            VehOwner[vid] = Player[playerid][pID];
            VehDBID[vid] = DB_GetFieldIntByName(r, "id");
            VehLocked[vid] = DB_GetFieldIntByName(r, "locked") != 0;
            SetVehicleParamsEx(vid, 0, 0, 0, VehLocked[vid] ? 1 : 0, 0, 0, 0);
        }
        while (DB_SelectNextRow(r));
    }
    DB_FreeResultSet(r);
}

stock SaveVehicle(vid)
{
    if (VehType[vid] != VT_OWNED || !VehDBID[vid]) return;
    new q[160];
    format(q, sizeof q, "UPDATE vehicles SET fuel = %d, locked = %d WHERE id = %d", VehFuel[vid], VehLocked[vid] ? 1 : 0, VehDBID[vid]);
    DB_Exec(q);
}

stock UnloadPlayerVehicles(playerid)
{
    for (new v = 1; v < MAX_VEHICLES; v++)
    {
        if (VehType[v] == VT_OWNED && VehOwner[v] == Player[playerid][pID])
        {
            SaveVehicle(v);
            DestroyVehicle(v);
            VehType[v] = VT_NONE;
            VehOwner[v] = 0;
            VehDBID[v] = 0;
        }
    }
}

stock CountOwnedVehicles(playerid)
{
    new q[96], c = 0;
    format(q, sizeof q, "SELECT COUNT(*) FROM vehicles WHERE owner = %d", Player[playerid][pID]);
    new DBResult:r = DB_ExecuteQuery(gDB, q);
    if (r) { c = DB_GetFieldInt(r, 0); DB_FreeResultSet(r); }
    return c;
}

stock bool:IsVehicleOwner(playerid, vid)
{
    return VehType[vid] == VT_OWNED && VehOwner[vid] == Player[playerid][pID];
}

stock GetNearestOwnedVehicle(playerid, Float:range = 6.0)
{
    new Float:x, Float:y, Float:z, best = INVALID_VEHICLE_ID, Float:bestd = range;
    for (new v = 1; v < MAX_VEHICLES; v++)
    {
        if (!IsVehicleOwner(playerid, v)) continue;
        GetVehiclePos(v, x, y, z);
        new Float:d = GetPlayerDistanceFromPoint(playerid, x, y, z);
        if (d < bestd) { bestd = d; best = v; }
    }
    return best;
}

stock ShowDealerDialog(playerid)
{
    new body[2048];
    body = "Vehicle\tPrice\tMin. Level\n";
    for (new i = 0; i < sizeof DealerCars; i++)
    {
        format(body, sizeof body, "%s%s\t" C_GREEN "%s\t%s%d\n", body, DealerCars[i][dName], FormatMoney(DealerCars[i][dPrice]),
            Player[playerid][pLevel] >= DealerCars[i][dLevel] ? (C_WHITE) : (C_RED), DealerCars[i][dLevel]);
    }
    ShowPlayerDialog(playerid, DLG_DEALER, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "Kingmakers Motors " C_WHITE "- Dealership", body, "Select", "Close");
}

stock BuyVehicle(playerid, idx)
{
    if (Player[playerid][pLevel] < DealerCars[idx][dLevel]) return Error(playerid, "Your level is too low for this vehicle.");
    if (Player[playerid][pMoney] < DealerCars[idx][dPrice]) return Error(playerid, "You can't afford this vehicle (cash required).");
    if (CountOwnedVehicles(playerid) >= MAX_PLAYER_VEHICLES) return Error(playerid, "You already own the maximum number of vehicles. /sellveh one first.");

    new c1 = random(126), c2 = random(126), q[320];
    format(q, sizeof q, "INSERT INTO vehicles (owner, model, x, y, z, a, c1, c2, fuel, locked) VALUES (%d, %d, %f, %f, %f, %f, %d, %d, 100, 0)",
        Player[playerid][pID], DealerCars[idx][dModel], DEALER_SPAWN_X, DEALER_SPAWN_Y, DEALER_SPAWN_Z, DEALER_SPAWN_A, c1, c2);
    DB_Exec(q);
    new dbid = 0;
    new DBResult:r = DB_ExecuteQuery(gDB, "SELECT last_insert_rowid()");
    if (r) { dbid = DB_GetFieldInt(r, 0); DB_FreeResultSet(r); }

    GiveMoney(playerid, -DealerCars[idx][dPrice]);
    new vid = CreateVehicle(DealerCars[idx][dModel], DEALER_SPAWN_X, DEALER_SPAWN_Y, DEALER_SPAWN_Z, DEALER_SPAWN_A, c1, c2, -1);
    SetupVehicle(vid, VT_OWNED);
    VehOwner[vid] = Player[playerid][pID];
    VehDBID[vid] = dbid;
    PutPlayerInVehicle(playerid, vid, 0);
    SendClientMessage(playerid, COLOR_GREEN, "Congratulations! You bought a %s for %s.", DealerCars[idx][dName], FormatMoney(DealerCars[idx][dPrice]));
    Info(playerid, "Start the engine with /engine (or N). Save its spot with /park. Lock it with /lock.");
    SavePlayer(playerid);
    return 1;
}

stock ShowMyVehicles(playerid)
{
    new body[512], name[24], count = 0;
    body = "Vehicle\tFuel\tStatus\n";
    for (new v = 1; v < MAX_VEHICLES; v++)
    {
        if (!IsVehicleOwner(playerid, v)) continue;
        GetVehicleNameByModel(GetVehicleModel(v), name);
        format(body, sizeof body, "%s%s (ID %d)\t%d%%\t%s\n", body, name, v, VehFuel[v], VehLocked[v] ? (C_RED "Locked") : (C_GREEN "Unlocked"));
        count++;
    }
    if (!count) return Error(playerid, "You don't own any vehicles. Visit Kingmakers Motors (GPS > Dealership).");
    ShowPlayerDialog(playerid, DLG_MYVEH, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "My Vehicles " C_WHITE "- select to locate", body, "Locate", "Close");
    return 1;
}

stock ToggleEngine(playerid)
{
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Error(playerid, "You must be driving a vehicle.");
    if (IsBicycle(GetVehicleModel(vid))) return 1;
    if (VehType[vid] == VT_OWNED && !IsVehicleOwner(playerid, vid) && Player[playerid][pAdmin] < 3)
        return Error(playerid, "You don't have the keys to this vehicle.");
    new engine, lights, alarm, doors, bonnet, boot, objective;
    GetVehicleParamsEx(vid, engine, lights, alarm, doors, bonnet, boot, objective);
    if (engine != 1)
    {
        if (VehFuel[vid] <= 0) return Error(playerid, "The tank is empty! Get fuel at a gas station (/refuel) or call a mechanic.");
        new Float:hp;
        GetVehicleHealth(vid, hp);
        if (hp < 300.0) return Error(playerid, "The engine is too damaged to start. Call a mechanic.");
        SetVehicleParamsEx(vid, 1, lights, alarm, doors, bonnet, boot, objective);
        new msg[96];
        format(msg, sizeof msg, "* %s turns the key and starts the engine.", RPName(playerid));
        SendNearby(playerid, 15.0, COLOR_PURPLE, msg);
    }
    else
    {
        SetVehicleParamsEx(vid, 0, lights, alarm, doors, bonnet, boot, objective);
        new msg[96];
        format(msg, sizeof msg, "* %s turns off the engine.", RPName(playerid));
        SendNearby(playerid, 15.0, COLOR_PURPLE, msg);
    }
    return 1;
}

stock ToggleLock(playerid)
{
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || !IsVehicleOwner(playerid, vid)) vid = GetNearestOwnedVehicle(playerid);
    if (vid == INVALID_VEHICLE_ID || !vid) return Error(playerid, "You are not near one of your vehicles.");
    new engine, lights, alarm, doors, bonnet, boot, objective;
    GetVehicleParamsEx(vid, engine, lights, alarm, doors, bonnet, boot, objective);
    VehLocked[vid] = !VehLocked[vid];
    SetVehicleParamsEx(vid, engine, lights, alarm, VehLocked[vid] ? 1 : 0, bonnet, boot, objective);
    PlayerPlaySound(playerid, 1145, 0.0, 0.0, 0.0);
    ShowInfoText(playerid, VehLocked[vid] ? ("~r~Vehicle locked") : ("~g~Vehicle unlocked"), 2000);
    SaveVehicle(vid);
    return 1;
}

stock bool:IsBicycle(model)
{
    return model == 481 || model == 509 || model == 510;
}

stock bool:IsNearGasStation(playerid)
{
    for (new i = 0; i < sizeof GasStations; i++)
        if (IsPlayerInRangeOfPoint(playerid, 15.0, GasStations[i][0], GasStations[i][1], GasStations[i][2])) return true;
    return false;
}

// Called every second from the main timer.
stock Vehicle_OnSecond(playerid)
{
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return;
    if (IsBicycle(GetVehicleModel(vid))) return;
    new engine, lights, alarm, doors, bonnet, boot, objective;
    GetVehicleParamsEx(vid, engine, lights, alarm, doors, bonnet, boot, objective);
    if (engine == 1)
    {
        new Float:speed = GetVehicleSpeedKMH(vid);
        VehFuelTick[vid] += (speed > 5.0) ? (1 + floatround(speed / 60.0)) : 1;
        if (VehFuelTick[vid] >= 12)
        {
            VehFuelTick[vid] = 0;
            if (VehType[vid] != VT_ADMIN && VehFuel[vid] > 0) VehFuel[vid]--;
            if (VehFuel[vid] <= 0)
            {
                SetVehicleParamsEx(vid, 0, lights, alarm, doors, bonnet, boot, objective);
                ShowInfoText(playerid, "~r~Out of fuel!", 3000);
                Info(playerid, "Your vehicle ran out of fuel. Call a mechanic from your phone or push it to a gas station.");
            }
            else if (VehFuel[vid] == 15) ShowInfoText(playerid, "~y~Low fuel - find a gas station", 3000);
        }
    }
    UpdateSpeedo(playerid);
}

// Restrict job / faction vehicles when a player becomes driver.
stock Vehicle_OnDriver(playerid, vid)
{
    switch (VehType[vid])
    {
        case VT_FACTION:
        {
            if (Player[playerid][pFaction] != VehFaction[vid] && Player[playerid][pAdmin] < 3)
            {
                RemovePlayerFromVehicle(playerid);
                SendClientMessage(playerid, COLOR_RED, "This vehicle belongs to %s.", FactionNames[VehFaction[vid]]);
                return;
            }
        }
        case VT_JOB:
        {
            if (Player[playerid][pJob] != VehJob[vid] && Player[playerid][pAdmin] < 3)
            {
                RemovePlayerFromVehicle(playerid);
                SendClientMessage(playerid, COLOR_RED, "Only %ss can drive this vehicle. Get the job at City Hall.", JobNames[VehJob[vid]]);
                return;
            }
        }
    }
    new engine, lights, alarm, doors, bonnet, boot, objective;
    GetVehicleParamsEx(vid, engine, lights, alarm, doors, bonnet, boot, objective);
    if (engine != 1 && !IsBicycle(GetVehicleModel(vid))) ShowInfoText(playerid, "~w~Press ~y~N~w~ or type ~y~/engine~w~ to start", 3500);
    if (!Player[playerid][pDriveLic] && !IsBicycle(GetVehicleModel(vid)))
        SendClientMessage(playerid, COLOR_GREY, "Note: you don't have a driving license. Police can fine you. Buy one at City Hall.");
}

// ---------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------
CMD:engine(playerid, params[]) return ToggleEngine(playerid);
CMD:lock(playerid, params[]) return ToggleLock(playerid);
CMD:myveh(playerid, params[]) return ShowMyVehicles(playerid);
CMD:mycars(playerid, params[]) return ShowMyVehicles(playerid);

CMD:lights(playerid, params[])
{
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Error(playerid, "You must be driving a vehicle.");
    new engine, lights, alarm, doors, bonnet, boot, objective;
    GetVehicleParamsEx(vid, engine, lights, alarm, doors, bonnet, boot, objective);
    SetVehicleParamsEx(vid, engine, lights == 1 ? 0 : 1, alarm, doors, bonnet, boot, objective);
    return 1;
}

CMD:park(playerid, params[])
{
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || !IsVehicleOwner(playerid, vid) || GetPlayerState(playerid) != PLAYER_STATE_DRIVER)
        return Error(playerid, "You must be driving your own vehicle.");
    new Float:x, Float:y, Float:z, Float:a, q[200];
    GetVehiclePos(vid, x, y, z);
    GetVehicleZAngle(vid, a);
    format(q, sizeof q, "UPDATE vehicles SET x=%f, y=%f, z=%f, a=%f, fuel=%d WHERE id=%d", x, y, z, a, VehFuel[vid], VehDBID[vid]);
    DB_Exec(q);
    SetVehicleToRespawn(vid);
    // Re-create at the new spot so the respawn position updates.
    new model = GetVehicleModel(vid), c1, c2, fuel = VehFuel[vid], dbid = VehDBID[vid], bool:locked = VehLocked[vid];
    GetVehicleColours(vid, c1, c2);
    DestroyVehicle(vid);
    VehType[vid] = VT_NONE;
    new nv = CreateVehicle(model, x, y, z, a, c1, c2, -1);
    SetupVehicle(nv, VT_OWNED, float(fuel));
    VehOwner[nv] = Player[playerid][pID];
    VehDBID[nv] = dbid;
    VehLocked[nv] = locked;
    SetVehicleParamsEx(nv, 0, 0, 0, locked ? 1 : 0, 0, 0, 0);
    Info(playerid, "Vehicle parked. It will respawn here from now on.");
    return 1;
}

CMD:sellveh(playerid, params[])
{
    if (!IsPlayerInRangeOfPoint(playerid, 25.0, DEALER_X, DEALER_Y, DEALER_Z)) return Error(playerid, "You must be at Kingmakers Motors to sell a vehicle.");
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || !IsVehicleOwner(playerid, vid)) return Error(playerid, "You must be inside your own vehicle.");
    new model = GetVehicleModel(vid), price = 0, name[24], q[96];
    for (new i = 0; i < sizeof DealerCars; i++) if (DealerCars[i][dModel] == model) price = DealerCars[i][dPrice] / 2;
    GetVehicleNameByModel(model, name);
    format(q, sizeof q, "DELETE FROM vehicles WHERE id = %d", VehDBID[vid]);
    DB_Exec(q);
    DestroyVehicle(vid);
    VehType[vid] = VT_NONE;
    GiveMoney(playerid, price);
    SendClientMessage(playerid, COLOR_GREEN, "You sold your %s to the dealership for %s.", name, FormatMoney(price));
    SavePlayer(playerid);
    return 1;
}

CMD:refuel(playerid, params[])
{
    new vid = GetPlayerVehicleID(playerid);
    if (!vid || GetPlayerState(playerid) != PLAYER_STATE_DRIVER) return Error(playerid, "You must be driving a vehicle.");
    if (!IsNearGasStation(playerid)) return Error(playerid, "You are not at a gas station. Use /gps to find one.");
    new engine, lights, alarm, doors, bonnet, boot, objective;
    GetVehicleParamsEx(vid, engine, lights, alarm, doors, bonnet, boot, objective);
    if (engine == 1) return Error(playerid, "Turn off the engine first (/engine).");
    new need = 100 - VehFuel[vid];
    if (need <= 0) return Info(playerid, "The tank is already full.");
    new cost = need * FUEL_PRICE;
    if (Player[playerid][pMoney] < cost)
    {
        need = Player[playerid][pMoney] / FUEL_PRICE;
        cost = need * FUEL_PRICE;
        if (need <= 0) return Error(playerid, "You can't afford any fuel.");
    }
    VehFuel[vid] += need;
    GiveMoney(playerid, -cost);
    SendClientMessage(playerid, COLOR_GREEN, "You refueled %d%% for %s. Tank: %d%%", need, FormatMoney(cost), VehFuel[vid]);
    return 1;
}
