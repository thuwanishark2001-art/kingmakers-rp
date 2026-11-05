// =====================================================================
//  Kingmakers RP - civilian jobs: Taxi, Trucker, Miner, Mechanic
// =====================================================================

new const Float:JobHQ[MAX_JOBS][3] =
{
    { 0.0, 0.0, 0.0 },
    { 1775.00, -1905.00, 13.39 },   // Taxi depot - Unity Station
    { 2195.00, -2240.00, 13.55 },   // Trucker depot - Ocean Docks
    { 690.00,  845.00,  -42.96 },   // Hunter Quarry
    { 2063.00, -1831.00, 13.55 }    // Idlewood garage
};

new const JobDescriptions[MAX_JOBS][] =
{
    "",
    "Drive a cab. Pick up NPC fares with /startwork or set a /fare for real players.",
    "Load cargo at the docks and deliver it across the city.",
    "Mine ore at Hunter Quarry and carry it to the conveyor. No vehicle needed.",
    "Repair and refuel vehicles for other players. Earn what you charge."
};

// Street-level destinations around Los Santos (fares, deliveries).
new const Float:CityPoints[][3] =
{
    { 1481.00, -1740.00, 13.55 }, { 1172.00, -1355.00, 13.40 }, { 2229.00, -1730.00, 13.55 },
    { 1950.00, -1765.00, 13.55 }, { 1367.00, -1280.00, 13.55 }, { 2104.00, -1795.00, 13.55 },
    { 1830.00, -1680.00, 13.55 }, { 1000.00, -1340.00, 13.40 }, { 1530.00, -1655.00, 13.55 },
    { 2500.00, -1670.00, 13.40 }, { 1680.00, -2250.00, 13.55 }, { 2015.00, -1115.00, 26.20 },
    { 2850.00, -1550.00, 11.00 }, { 2380.00, -1930.00, 13.55 }, { 1300.00, -1850.00, 13.55 },
    { 2120.00, -1380.00, 24.00 }, { 1730.00, -1590.00, 13.55 }, { 2420.00, -1240.00, 24.00 }
};

new const Float:MineRocks[][3] =
{
    { 662.00, 830.00, -42.96 }, { 640.00, 870.00, -42.96 }, { 700.00, 900.00, -42.96 },
    { 725.00, 860.00, -42.96 }, { 610.00, 845.00, -42.96 }
};
#define MINE_DROP_X 690.00
#define MINE_DROP_Y 815.00
#define MINE_DROP_Z -42.96

// Service offers (mechanic repair/refuel, EMS heal)
enum
{
    OFFER_NONE = 0,
    OFFER_REPAIR,
    OFFER_REFILL,
    OFFER_HEAL
}
new OfferFrom[MAX_PLAYERS] = { INVALID_PLAYER_ID, ... };
new OfferType[MAX_PLAYERS];
new OfferPrice[MAX_PLAYERS];
new MiningTimer[MAX_PLAYERS];

stock CreateJobPickups()
{
    new label[128];
    for (new j = 1; j < MAX_JOBS; j++)
    {
        CreatePickup(1239, 1, JobHQ[j][0], JobHQ[j][1], JobHQ[j][2], -1);
        format(label, sizeof label, "{FFC83D}%s HQ\n{FFFFFF}/startwork  -  /stopwork", JobNames[j]);
        Create3DTextLabel(label, COLOR_WHITE, JobHQ[j][0], JobHQ[j][1], JobHQ[j][2] + 0.7, 20.0, 0, true);
    }
    Create3DTextLabel("{FFC83D}Ore Conveyor\n{FFFFFF}Drop your ore here", COLOR_WHITE, MINE_DROP_X, MINE_DROP_Y, MINE_DROP_Z + 1.0, 20.0, 0, true);
}

stock ShowJobCenter(playerid)
{
    new body[768];
    body = "Job\tDescription\n";
    for (new j = 1; j < MAX_JOBS; j++) format(body, sizeof body, "%s" C_GOLD "%s\t" C_WHITE "%s\n", body, JobNames[j], JobDescriptions[j]);
    ShowPlayerDialog(playerid, DLG_JOBCENTER, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "Los Santos Job Center", body, "Take job", "Close");
}

stock QuitJob(playerid)
{
    StopWork(playerid);
    SendClientMessage(playerid, COLOR_GOLD, "You quit your job as %s.", JobNames[Player[playerid][pJob]]);
    Player[playerid][pJob] = JOB_NONE;
}

stock StopWork(playerid)
{
    if (Player[playerid][pCPType] >= CP_TAXI_PICKUP) ClearCP(playerid);
    Player[playerid][pJobStage] = 0;
    Player[playerid][pJobPay] = 0;
    Player[playerid][pTaxiFare] = 0;
    if (Player[playerid][pCarryOre])
    {
        Player[playerid][pCarryOre] = 0;
        RemovePlayerAttachedObject(playerid, 0);
        SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
    }
    if (MiningTimer[playerid]) { KillTimer(MiningTimer[playerid]); MiningTimer[playerid] = 0; }
}

stock Jobs_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
    #pragma unused inputtext
    if (dialogid != DLG_JOBCENTER) return 0;
    if (!response) return 1;
    new job = listitem + 1;
    if (job <= JOB_NONE || job >= MAX_JOBS) return 1;
    if (Player[playerid][pJob] == job) return Error(playerid, "You already have this job."), 1;
    if (Player[playerid][pJob] != JOB_NONE) StopWork(playerid);
    Player[playerid][pJob] = job;
    SendClientMessage(playerid, COLOR_GREEN, "You are now employed as a %s! A GPS marker was set to your job HQ.", JobNames[job]);
    SendClientMessage(playerid, COLOR_WHITE, "%s", JobDescriptions[job]);
    SetCP(playerid, CP_GPS, JobHQ[job][0], JobHQ[job][1], JobHQ[job][2], 3.0);
    SavePlayer(playerid);
    return 1;
}

stock NextTaxiFare(playerid)
{
    new idx = random(sizeof CityPoints);
    Player[playerid][pJobStage] = 1;
    SetCP(playerid, CP_TAXI_PICKUP, CityPoints[idx][0], CityPoints[idx][1], CityPoints[idx][2], 5.0);
    ShowInfoText(playerid, "~y~New fare!~w~ Drive to the client", 3000);
}

stock NextTruckDelivery(playerid)
{
    new idx = random(sizeof CityPoints);
    new Float:x, Float:y, Float:z;
    GetPlayerPos(playerid, x, y, z);
    Player[playerid][pJobPay] = 350 + floatround(GetDistanceBetween(x, y, z, CityPoints[idx][0], CityPoints[idx][1], CityPoints[idx][2]) * 0.35);
    Player[playerid][pJobStage] = 2;
    SetCP(playerid, CP_TRUCK_DROP, CityPoints[idx][0], CityPoints[idx][1], CityPoints[idx][2], 6.0);
    SendClientMessage(playerid, COLOR_GOLD, "[TRUCKER] {FFFFFF}Cargo loaded. Deliver it to the marker. Payment on delivery: %s", FormatMoney(Player[playerid][pJobPay]));
}

stock NextMineRock(playerid)
{
    new idx = random(sizeof MineRocks);
    Player[playerid][pJobStage] = 1;
    SetCP(playerid, CP_MINE_ROCK, MineRocks[idx][0], MineRocks[idx][1], MineRocks[idx][2], 2.0);
}

forward FinishMining(playerid);
public FinishMining(playerid)
{
    MiningTimer[playerid] = 0;
    if (!IsPlayerConnected(playerid) || Player[playerid][pJob] != JOB_MINER) return 0;
    ClearAnimations(playerid);
    TogglePlayerControllable(playerid, true);
    Player[playerid][pCarryOre] = 1;
    SetPlayerSpecialAction(playerid, SPECIAL_ACTION_CARRY);
    SetPlayerAttachedObject(playerid, 0, 2936, 5, 0.1, 0.1, 0.18, 0.0, 0.0, 0.0, 0.35, 0.35, 0.35);
    Player[playerid][pJobStage] = 2;
    SetCP(playerid, CP_MINE_DROP, MINE_DROP_X, MINE_DROP_Y, MINE_DROP_Z, 2.5);
    ShowInfoText(playerid, "~w~Carry the ~y~ore~w~ to the conveyor", 3000);
    return 1;
}

// Returns 1 if the checkpoint belonged to a job.
stock Jobs_OnEnterCP(playerid)
{
    switch (Player[playerid][pCPType])
    {
        case CP_TAXI_PICKUP:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid || VehJob[vid] != JOB_TAXI) return Error(playerid, "You need your taxi to pick up the client."), 1;
            new idx = random(sizeof CityPoints);
            new Float:x, Float:y, Float:z;
            GetPlayerPos(playerid, x, y, z);
            Player[playerid][pJobPay] = 120 + floatround(GetDistanceBetween(x, y, z, CityPoints[idx][0], CityPoints[idx][1], CityPoints[idx][2]) * 0.3);
            Player[playerid][pJobStage] = 2;
            SetCP(playerid, CP_TAXI_DROP, CityPoints[idx][0], CityPoints[idx][1], CityPoints[idx][2], 5.0);
            ShowInfoText(playerid, "~g~Client picked up!~w~ Drive to the destination", 3000);
            return 1;
        }
        case CP_TAXI_DROP:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid || VehJob[vid] != JOB_TAXI) return Error(playerid, "The client is waiting in your taxi!"), 1;
            new pay = Player[playerid][pJobPay];
            new tip = random(60);
            GiveMoney(playerid, pay + tip);
            SendClientMessage(playerid, COLOR_GREEN, "[TAXI] Fare completed: %s + tip %s.", FormatMoney(pay), FormatMoney(tip));
            NextTaxiFare(playerid);
            return 1;
        }
        case CP_TRUCK_LOAD:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid || VehJob[vid] != JOB_TRUCKER) return Error(playerid, "You need a company truck to load cargo."), 1;
            NextTruckDelivery(playerid);
            return 1;
        }
        case CP_TRUCK_DROP:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid || VehJob[vid] != JOB_TRUCKER) return Error(playerid, "You need the truck with the cargo."), 1;
            new Float:hp;
            GetVehicleHealth(vid, hp);
            new pay = Player[playerid][pJobPay];
            if (hp < 700.0) { pay = pay * 70 / 100; Info(playerid, "The cargo got damaged: 30% penalty."); }
            GiveMoney(playerid, pay);
            SendClientMessage(playerid, COLOR_GREEN, "[TRUCKER] Delivery completed! You earned %s. Head back to the docks for another load.", FormatMoney(pay));
            Player[playerid][pJobStage] = 1;
            SetCP(playerid, CP_TRUCK_LOAD, JobHQ[JOB_TRUCKER][0], JobHQ[JOB_TRUCKER][1], JobHQ[JOB_TRUCKER][2], 6.0);
            return 1;
        }
        case CP_MINE_ROCK:
        {
            if (IsPlayerInAnyVehicle(playerid)) return Error(playerid, "Get out of the vehicle to mine."), 1;
            if (MiningTimer[playerid]) return 1;
            DisablePlayerCheckpoint(playerid);
            TogglePlayerControllable(playerid, false);
            ApplyAnimation(playerid, "BASEBALL", "Bat_4", 4.1, true, false, false, false, 0, SYNC_ALL);
            ShowInfoText(playerid, "~w~Mining...", 6000);
            MiningTimer[playerid] = SetTimerEx("FinishMining", 6000, false, "i", playerid);
            return 1;
        }
        case CP_MINE_DROP:
        {
            if (!Player[playerid][pCarryOre]) return 1;
            Player[playerid][pCarryOre] = 0;
            RemovePlayerAttachedObject(playerid, 0);
            SetPlayerSpecialAction(playerid, SPECIAL_ACTION_NONE);
            new pay = 110 + random(70);
            if (random(12) == 0) { pay += 400; SendClientMessage(playerid, COLOR_GOLD, "[MINER] You found a gold nugget! Bonus $400."); }
            GiveMoney(playerid, pay);
            SendClientMessage(playerid, COLOR_GREEN, "[MINER] Ore delivered: %s.", FormatMoney(pay));
            NextMineRock(playerid);
            return 1;
        }
    }
    return 0;
}

// Player entered a taxi as passenger - charge fare.
stock Jobs_OnPassenger(playerid, vid)
{
    if (VehJob[vid] != JOB_TAXI) return;
    for (new i = 0; i < MAX_PLAYERS; i++)
    {
        if (!IsPlayerConnected(i) || GetPlayerVehicleID(i) != vid || GetPlayerState(i) != PLAYER_STATE_DRIVER) continue;
        new fare = Player[i][pTaxiFare];
        if (fare <= 0) return;
        if (Player[playerid][pMoney] < fare)
        {
            RemovePlayerFromVehicle(playerid);
            Error(playerid, "You can't afford this taxi fare.");
            return;
        }
        GiveMoney(playerid, -fare);
        GiveMoney(i, fare);
        SendClientMessage(playerid, COLOR_YELLOW, "You paid the taxi fare of %s to %s.", FormatMoney(fare), RPName(i));
        SendClientMessage(i, COLOR_GREEN, "%s entered your taxi and paid %s.", RPName(playerid), FormatMoney(fare));
        return;
    }
}

// ---------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------
CMD:jobs(playerid, params[])
{
    if (!IsPlayerInRangeOfPoint(playerid, 8.0, CITYHALL_X, CITYHALL_Y, CITYHALL_Z)) return Error(playerid, "Visit the Job Center at City Hall (/gps).");
    ShowJobCenter(playerid);
    return 1;
}

CMD:quitjob(playerid, params[])
{
    if (Player[playerid][pJob] == JOB_NONE) return Error(playerid, "You don't have a job.");
    QuitJob(playerid);
    SavePlayer(playerid);
    return 1;
}

CMD:startwork(playerid, params[])
{
    new job = Player[playerid][pJob];
    if (job == JOB_NONE) return Error(playerid, "You don't have a job. Visit the Job Center at City Hall.");
    if (Player[playerid][pJobStage] != 0) return Error(playerid, "You are already working. /stopwork to finish.");
    switch (job)
    {
        case JOB_TAXI:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid || VehJob[vid] != JOB_TAXI) return Error(playerid, "Get in a taxi at the Unity Station depot first.");
            SendJobMessage(JOB_TAXI, COLOR_YELLOW, "[TAXI] A colleague is now on duty.");
            NextTaxiFare(playerid);
            Info(playerid, "You are on duty. Use /fare [price] to charge real passengers. Players can call you via the phone.");
        }
        case JOB_TRUCKER:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid || VehJob[vid] != JOB_TRUCKER) return Error(playerid, "Get in a company truck at the Ocean Docks depot first.");
            Player[playerid][pJobStage] = 1;
            SetCP(playerid, CP_TRUCK_LOAD, JobHQ[JOB_TRUCKER][0], JobHQ[JOB_TRUCKER][1], JobHQ[JOB_TRUCKER][2], 6.0);
            Info(playerid, "Drive to the loading bay marker to load cargo.");
        }
        case JOB_MINER:
        {
            if (!IsPlayerInRangeOfPoint(playerid, 60.0, JobHQ[JOB_MINER][0], JobHQ[JOB_MINER][1], JobHQ[JOB_MINER][2]))
                return Error(playerid, "You must be at Hunter Quarry to start mining.");
            NextMineRock(playerid);
            Info(playerid, "Walk to the marked rock to mine it, then carry the ore to the conveyor.");
        }
        case JOB_MECHANIC:
        {
            Player[playerid][pJobStage] = 1;
            Info(playerid, "You are on duty as a mechanic. Use /repair [id] [price] and /refill [id] [price] near a vehicle.");
            SendJobMessage(JOB_MECHANIC, COLOR_YELLOW, "[MECHANIC] A colleague is now on duty.");
        }
    }
    return 1;
}

CMD:stopwork(playerid, params[])
{
    if (Player[playerid][pJobStage] == 0) return Error(playerid, "You are not working.");
    StopWork(playerid);
    Info(playerid, "You stopped working.");
    return 1;
}

CMD:fare(playerid, params[])
{
    new price;
    if (Player[playerid][pJob] != JOB_TAXI) return Error(playerid, "You are not a taxi driver.");
    if (!ParamInt(params, 0, price) || price < 0 || price > 500) return Usage(playerid, "/fare [0-500]");
    Player[playerid][pTaxiFare] = price;
    if (price > 0)
    {
        new msg[96];
        format(msg, sizeof msg, "[TAXI] %s is on duty. Fare: %s. Call via phone > Services.", RPName(playerid), FormatMoney(price));
        SendClientMessageToAll(COLOR_YELLOW, "%s", msg);
    }
    else Info(playerid, "You are no longer charging a fare.");
    return 1;
}

stock MakeOffer(playerid, target, type, price)
{
    OfferFrom[target] = playerid;
    OfferType[target] = type;
    OfferPrice[target] = price;
}

CMD:repair(playerid, params[])
{
    new target = ParamPlayer(params, 0), price;
    if (Player[playerid][pJob] != JOB_MECHANIC) return Error(playerid, "You are not a mechanic.");
    if (!ParamInt(params, 1, price)) return Usage(playerid, "/repair [id] [price 50-2000]");
    if (target == INVALID_PLAYER_ID || target == playerid) return Error(playerid, "Player not found.");
    if (price < 50 || price > 2000) return Error(playerid, "Price must be between $50 and $2,000.");
    if (!IsNearPlayer(playerid, target, 8.0) || !IsPlayerInAnyVehicle(target)) return Error(playerid, "The player must be near you inside a vehicle.");
    MakeOffer(playerid, target, OFFER_REPAIR, price);
    SendClientMessage(target, COLOR_LIGHTBLUE, "Mechanic %s offers to repair your vehicle for %s. Type /accept to agree.", RPName(playerid), FormatMoney(price));
    SendClientMessage(playerid, COLOR_LIGHTBLUE, "You offered a repair to %s for %s.", RPName(target), FormatMoney(price));
    return 1;
}

CMD:refill(playerid, params[])
{
    new target = ParamPlayer(params, 0), price;
    if (Player[playerid][pJob] != JOB_MECHANIC) return Error(playerid, "You are not a mechanic.");
    if (!ParamInt(params, 1, price)) return Usage(playerid, "/refill [id] [price 50-1000]");
    if (target == INVALID_PLAYER_ID || target == playerid) return Error(playerid, "Player not found.");
    if (price < 50 || price > 1000) return Error(playerid, "Price must be between $50 and $1,000.");
    if (!IsNearPlayer(playerid, target, 8.0) || !IsPlayerInAnyVehicle(target)) return Error(playerid, "The player must be near you inside a vehicle.");
    MakeOffer(playerid, target, OFFER_REFILL, price);
    SendClientMessage(target, COLOR_LIGHTBLUE, "Mechanic %s offers to refill your tank for %s. Type /accept to agree.", RPName(playerid), FormatMoney(price));
    SendClientMessage(playerid, COLOR_LIGHTBLUE, "You offered a refill to %s for %s.", RPName(target), FormatMoney(price));
    return 1;
}

CMD:accept(playerid, params[])
{
    new from = OfferFrom[playerid], price = OfferPrice[playerid], type = OfferType[playerid];
    if (from == INVALID_PLAYER_ID || type == OFFER_NONE || !IsPlayerConnected(from)) return Error(playerid, "You have no pending offers.");
    OfferFrom[playerid] = INVALID_PLAYER_ID;
    OfferType[playerid] = OFFER_NONE;
    if (!IsNearPlayer(playerid, from, 10.0)) return Error(playerid, "That player is too far away.");
    if (Player[playerid][pMoney] < price) return Error(playerid, "You can't afford it.");
    switch (type)
    {
        case OFFER_REPAIR:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid) return Error(playerid, "You must be in a vehicle.");
            RepairVehicle(vid);
            SendClientMessage(playerid, COLOR_GREEN, "Your vehicle was repaired by %s.", RPName(from));
        }
        case OFFER_REFILL:
        {
            new vid = GetPlayerVehicleID(playerid);
            if (!vid) return Error(playerid, "You must be in a vehicle.");
            VehFuel[vid] = 100;
            SendClientMessage(playerid, COLOR_GREEN, "Your vehicle was refueled by %s.", RPName(from));
        }
        case OFFER_HEAL:
        {
            SetPlayerHealth(playerid, 100.0);
            SendClientMessage(playerid, COLOR_GREEN, "You were treated by %s.", RPName(from));
        }
    }
    GiveMoney(playerid, -price);
    GiveMoney(from, price);
    SendClientMessage(from, COLOR_GREEN, "%s accepted your service. You earned %s.", RPName(playerid), FormatMoney(price));
    return 1;
}
