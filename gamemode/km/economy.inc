// =====================================================================
//  Kingmakers RP - economy: bank/ATM, payday, levels, City Hall
// =====================================================================

new const Float:ATMs[][3] =
{
    { 1497.70, -1749.90, 15.45 },   // City Hall plaza
    { 1833.20, -1847.00, 13.58 },   // Unity Station 24/7
    { 2233.20, -1715.30, 13.55 },   // Ganton gym
    { 1366.40, -1290.10, 13.55 },   // Market Ammunation
    { 1172.20, -1307.60, 13.99 },   // All Saints hospital
    { 2105.50, -1806.50, 13.55 },   // Idlewood pizza
    { 1555.00, -1711.00, 5.89 },    // LSPD garage
    { 2131.50, -1135.80, 25.40 }    // Dealership
};

new gPickupATM[sizeof ATMs];
new gPickupCityHall, gPickupJobCenter, gPickupDealer;

stock CreateEconomyPickups()
{
    for (new i = 0; i < sizeof ATMs; i++)
    {
        gPickupATM[i] = CreatePickup(1274, 1, ATMs[i][0], ATMs[i][1], ATMs[i][2], -1);
        Create3DTextLabel("{FFC83D}ATM\n{FFFFFF}Kingmakers Bank\n{AFAFAF}/atm", COLOR_WHITE, ATMs[i][0], ATMs[i][1], ATMs[i][2] + 0.6, 15.0, 0, true);
    }
    gPickupCityHall = CreatePickup(1239, 1, CITYHALL_X, CITYHALL_Y, CITYHALL_Z, -1);
    Create3DTextLabel("{FFC83D}City Hall\n{FFFFFF}Licenses & documents\n{AFAFAF}/cityhall", COLOR_WHITE, CITYHALL_X, CITYHALL_Y, CITYHALL_Z + 0.6, 20.0, 0, true);
    gPickupJobCenter = CreatePickup(1210, 1, CITYHALL_X + 3.0, CITYHALL_Y, CITYHALL_Z, -1);
    Create3DTextLabel("{FFC83D}Job Center\n{FFFFFF}Find work in Los Santos\n{AFAFAF}/jobs", COLOR_WHITE, CITYHALL_X + 3.0, CITYHALL_Y, CITYHALL_Z + 0.6, 20.0, 0, true);
    gPickupDealer = CreatePickup(1274, 1, DEALER_X, DEALER_Y, DEALER_Z, -1);
    Create3DTextLabel("{FFC83D}Kingmakers Motors\n{FFFFFF}Vehicle dealership\n{AFAFAF}/buyveh", COLOR_WHITE, DEALER_X, DEALER_Y, DEALER_Z + 0.6, 20.0, 0, true);
    for (new i = 0; i < sizeof GasStations; i++)
        Create3DTextLabel("{FFC83D}Gas Station\n{FFFFFF}/refuel (engine off)", COLOR_WHITE, GasStations[i][0], GasStations[i][1], GasStations[i][2] + 1.0, 30.0, 0, true);
}

stock bool:IsNearATM(playerid)
{
    for (new i = 0; i < sizeof ATMs; i++)
        if (IsPlayerInRangeOfPoint(playerid, 3.0, ATMs[i][0], ATMs[i][1], ATMs[i][2])) return true;
    return false;
}

stock ShowATM(playerid)
{
    ShowPlayerDialog(playerid, DLG_ATM, DIALOG_STYLE_LIST, C_GOLD "Kingmakers Bank " C_WHITE "- ATM",
        "Check balance\nDeposit cash\nWithdraw cash\nTransfer to player", "Select", "Close");
}

stock Economy_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid)
    {
        case DLG_ATM:
        {
            if (!response) return 1;
            switch (listitem)
            {
                case 0:
                {
                    ShowPlayerDialog(playerid, DLG_INFO, DIALOG_STYLE_MSGBOX, C_GOLD "Kingmakers Bank " C_WHITE "- Balance",
                        C_WHITE "Account holder: " C_GOLD "%s\n\n" C_WHITE "Bank balance: " C_GREEN "%s\n" C_WHITE "Cash on hand: " C_GREEN "%s",
                        "OK", "", RPName(playerid), FormatMoney(Player[playerid][pBank]), FormatMoney(Player[playerid][pMoney]));
                }
                case 1: ShowPlayerDialog(playerid, DLG_ATM_DEPOSIT, DIALOG_STYLE_INPUT, C_GOLD "ATM " C_WHITE "- Deposit", C_WHITE "Cash on hand: " C_GREEN "%s\n\n" C_WHITE "Amount to deposit:", "Deposit", "Back", FormatMoney(Player[playerid][pMoney]));
                case 2: ShowPlayerDialog(playerid, DLG_ATM_WITHDRAW, DIALOG_STYLE_INPUT, C_GOLD "ATM " C_WHITE "- Withdraw", C_WHITE "Bank balance: " C_GREEN "%s\n\n" C_WHITE "Amount to withdraw:", "Withdraw", "Back", FormatMoney(Player[playerid][pBank]));
                case 3: ShowPlayerDialog(playerid, DLG_ATM_TRANSFER_ID, DIALOG_STYLE_INPUT, C_GOLD "ATM " C_WHITE "- Transfer", C_WHITE "Enter the player's ID or name:", "Next", "Back");
            }
            return 1;
        }
        case DLG_ATM_DEPOSIT:
        {
            if (!response) return ShowATM(playerid), 1;
            new amount = strval(inputtext);
            if (amount <= 0 || amount > Player[playerid][pMoney]) return Error(playerid, "Invalid amount."), ShowATM(playerid), 1;
            GiveMoney(playerid, -amount);
            Player[playerid][pBank] += amount;
            SendClientMessage(playerid, COLOR_GREEN, "Deposited %s. New balance: %s", FormatMoney(amount), FormatMoney(Player[playerid][pBank]));
            SavePlayer(playerid);
            return 1;
        }
        case DLG_ATM_WITHDRAW:
        {
            if (!response) return ShowATM(playerid), 1;
            new amount = strval(inputtext);
            if (amount <= 0 || amount > Player[playerid][pBank]) return Error(playerid, "Invalid amount."), ShowATM(playerid), 1;
            Player[playerid][pBank] -= amount;
            GiveMoney(playerid, amount);
            SendClientMessage(playerid, COLOR_GREEN, "Withdrew %s. New balance: %s", FormatMoney(amount), FormatMoney(Player[playerid][pBank]));
            SavePlayer(playerid);
            return 1;
        }
        case DLG_ATM_TRANSFER_ID:
        {
            if (!response) return ShowATM(playerid), 1;
            new target = FindPlayer(inputtext);
            if (target == INVALID_PLAYER_ID || target == playerid) return Error(playerid, "Player not found."), ShowATM(playerid), 1;
            Player[playerid][pTransferTarget] = target;
            ShowPlayerDialog(playerid, DLG_ATM_TRANSFER_AMT, DIALOG_STYLE_INPUT, C_GOLD "ATM " C_WHITE "- Transfer",
                C_WHITE "Transfer to " C_GOLD "%s\n" C_WHITE "Your balance: " C_GREEN "%s\n\n" C_WHITE "Amount:", "Send", "Back",
                RPName(target), FormatMoney(Player[playerid][pBank]));
            return 1;
        }
        case DLG_ATM_TRANSFER_AMT:
        {
            if (!response) return ShowATM(playerid), 1;
            new target = Player[playerid][pTransferTarget], amount = strval(inputtext);
            if (target == INVALID_PLAYER_ID || !IsPlayerConnected(target) || !Player[target][pLogged]) return Error(playerid, "That player is no longer online.");
            if (amount <= 0 || amount > Player[playerid][pBank]) return Error(playerid, "Invalid amount.");
            Player[playerid][pBank] -= amount;
            Player[target][pBank] += amount;
            SendClientMessage(playerid, COLOR_GREEN, "You transferred %s to %s.", FormatMoney(amount), RPName(target));
            SendClientMessage(target, COLOR_GREEN, "[BANK] %s transferred %s to your account.", RPName(playerid), FormatMoney(amount));
            SavePlayer(playerid);
            SavePlayer(target);
            return 1;
        }
        case DLG_CITYHALL:
        {
            if (!response) return 1;
            switch (listitem)
            {
                case 0:
                {
                    if (Player[playerid][pDriveLic]) return Error(playerid, "You already have a driving license.");
                    if (Player[playerid][pMoney] < LICENSE_DRIVE_PRICE) return Error(playerid, "You can't afford the driving license.");
                    GiveMoney(playerid, -LICENSE_DRIVE_PRICE);
                    Player[playerid][pDriveLic] = 1;
                    SendClientMessage(playerid, COLOR_GREEN, "You obtained a driving license for %s.", FormatMoney(LICENSE_DRIVE_PRICE));
                }
                case 1:
                {
                    if (Player[playerid][pGunLic]) return Error(playerid, "You already have a weapon license.");
                    if (Player[playerid][pLevel] < 3) return Error(playerid, "You must be at least level 3.");
                    if (Player[playerid][pWanted] > 0) return Error(playerid, "Wanted persons cannot obtain a weapon license.");
                    if (Player[playerid][pMoney] < LICENSE_GUN_PRICE) return Error(playerid, "You can't afford the weapon license.");
                    GiveMoney(playerid, -LICENSE_GUN_PRICE);
                    Player[playerid][pGunLic] = 1;
                    SendClientMessage(playerid, COLOR_GREEN, "You obtained a weapon license for %s.", FormatMoney(LICENSE_GUN_PRICE));
                }
                case 2: ShowJobCenter(playerid);
                case 3:
                {
                    if (Player[playerid][pJob] == JOB_NONE) return Error(playerid, "You don't have a job.");
                    QuitJob(playerid);
                }
            }
            SavePlayer(playerid);
            return 1;
        }
    }
    return 0;
}

stock ShowCityHall(playerid)
{
    ShowPlayerDialog(playerid, DLG_CITYHALL, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "Los Santos City Hall",
        "Service\tPrice\nDriving license\t" C_GREEN "$500\nWeapon license (Level 3+)\t" C_GREEN "$2,500\nJob Center\t-\nQuit current job\t-",
        "Select", "Close");
}

// ---------------------------------------------------------------------
//  Payday & levels (called once per minute for each logged player)
// ---------------------------------------------------------------------
stock Economy_OnMinute(playerid)
{
    Player[playerid][pPlayMinutes]++;
    Player[playerid][pPayMinutes]++;
    if (Player[playerid][pPayMinutes] >= PAYDAY_MINUTES)
    {
        Player[playerid][pPayMinutes] = 0;
        Payday(playerid);
    }
}

stock Payday(playerid)
{
    new salary = BASE_SALARY + Player[playerid][pLevel] * LEVEL_SALARY_BONUS;
    new f = Player[playerid][pFaction], factionpay = 0;
    if (f != FACTION_NONE && FactionSalary[f] > 0) factionpay = FactionSalary[f] + Player[playerid][pRank] * 150;
    new interest = (Player[playerid][pBank] / 10000) * BANK_INTEREST_BP;
    if (interest > 5000) interest = 5000;
    new tax = (salary + factionpay) / 10;
    new houses = CountPlayerHouses(playerid), housetax = houses * 100;
    new total = salary + factionpay + interest - tax - housetax;

    Player[playerid][pBank] += total;
    Player[playerid][pExp]++;

    SendClientMessage(playerid, COLOR_GOLD, "____________ KINGMAKERS PAYDAY ____________");
    SendClientMessage(playerid, COLOR_WHITE, "Paycheck: {5CE65C}%s{FFFFFF} | Faction pay: {5CE65C}%s{FFFFFF} | Interest: {5CE65C}%s", FormatMoney(salary), FormatMoney(factionpay), FormatMoney(interest));
    SendClientMessage(playerid, COLOR_WHITE, "Income tax: {FF4B4B}-%s{FFFFFF} | Property tax: {FF4B4B}-%s", FormatMoney(tax), FormatMoney(housetax));
    SendClientMessage(playerid, COLOR_WHITE, "Total deposited: {5CE65C}%s{FFFFFF} | New balance: {5CE65C}%s", FormatMoney(total), FormatMoney(Player[playerid][pBank]));

    new need = ExpForLevel(Player[playerid][pLevel] + 1);
    if (Player[playerid][pExp] >= need)
    {
        Player[playerid][pExp] -= need;
        Player[playerid][pLevel]++;
        SetPlayerScore(playerid, Player[playerid][pLevel]);
        GameTextForPlayer(playerid, "~y~LEVEL UP!", 4000, 1);
        SendClientMessage(playerid, COLOR_GOLD, "Congratulations! You reached level %d.", Player[playerid][pLevel]);
    }
    else SendClientMessage(playerid, COLOR_GREY, "Respect: %d/%d to reach level %d.", Player[playerid][pExp], need, Player[playerid][pLevel] + 1);
    PlayerPlaySound(playerid, 1058, 0.0, 0.0, 0.0);
    SavePlayer(playerid);
}

stock ShowStats(playerid, targetid)
{
    new body[1024], vehcount = CountOwnedVehicles(targetid), houses = CountPlayerHouses(targetid);
    format(body, sizeof body,
        C_GOLD "%s\n\n" \
        C_WHITE "Level: " C_GOLD "%d" C_WHITE " (Respect %d/%d)\n" \
        C_WHITE "Gender: %s  |  Age: %d  |  Skin: %d\n" \
        C_WHITE "Cash: " C_GREEN "%s" C_WHITE "  |  Bank: " C_GREEN "%s\n" \
        C_WHITE "Job: " C_GOLD "%s\n" \
        C_WHITE "Faction: " C_GOLD "%s" C_WHITE " (%s)\n" \
        C_WHITE "Phone number: %d  |  Medkits: %d\n" \
        C_WHITE "Driving license: %s" C_WHITE "  |  Weapon license: %s\n" \
        C_WHITE "Vehicles: %d/%d  |  Houses: %d\n" \
        C_WHITE "Wanted level: %d  |  Hours played: %d",
        RPName(targetid), Player[targetid][pLevel], Player[targetid][pExp], ExpForLevel(Player[targetid][pLevel] + 1),
        Player[targetid][pGender] ? ("Female") : ("Male"), Player[targetid][pAge], Player[targetid][pSkin],
        FormatMoney(Player[targetid][pMoney]), FormatMoney(Player[targetid][pBank]),
        JobNames[Player[targetid][pJob]],
        FactionNames[Player[targetid][pFaction]], Player[targetid][pFaction] ? FactionRankNames[Player[targetid][pFaction]][Player[targetid][pRank]] : ("-"),
        Player[targetid][pPhone], Player[targetid][pMedkits],
        Player[targetid][pDriveLic] ? (C_GREEN "Yes") : (C_RED "No"), Player[targetid][pGunLic] ? (C_GREEN "Yes") : (C_RED "No"),
        vehcount, MAX_PLAYER_VEHICLES, houses,
        Player[targetid][pWanted], Player[targetid][pPlayMinutes] / 60);
    ShowPlayerDialog(playerid, DLG_STATS, DIALOG_STYLE_MSGBOX, C_GOLD "Kingmakers RP " C_WHITE "- Character Stats", body, "Close", "");
}

// ---------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------
CMD:atm(playerid, params[])
{
    if (!IsNearATM(playerid)) return Error(playerid, "You are not near an ATM. Use /gps to find one.");
    ShowATM(playerid);
    return 1;
}

CMD:cityhall(playerid, params[])
{
    if (!IsPlayerInRangeOfPoint(playerid, 6.0, CITYHALL_X, CITYHALL_Y, CITYHALL_Z)) return Error(playerid, "You must be at City Hall (/gps).");
    ShowCityHall(playerid);
    return 1;
}

CMD:buyveh(playerid, params[])
{
    if (!IsPlayerInRangeOfPoint(playerid, 6.0, DEALER_X, DEALER_Y, DEALER_Z)) return Error(playerid, "You must be at Kingmakers Motors (/gps).");
    ShowDealerDialog(playerid);
    return 1;
}

CMD:stats(playerid, params[])
{
    ShowStats(playerid, playerid);
    return 1;
}

CMD:pay(playerid, params[])
{
    new target = ParamPlayer(params, 0), amount;
    if (!ParamInt(params, 1, amount)) return Usage(playerid, "/pay [id/name] [amount]");
    if (target == INVALID_PLAYER_ID || target == playerid) return Error(playerid, "Player not found.");
    if (!IsNearPlayer(playerid, target, 4.0)) return Error(playerid, "You are too far from that player.");
    if (amount <= 0 || amount > Player[playerid][pMoney]) return Error(playerid, "Invalid amount.");
    if (amount > 50000 && Player[playerid][pLevel] < 3) return Error(playerid, "Players below level 3 can pay at most $50,000 at once.");
    GiveMoney(playerid, -amount);
    GiveMoney(target, amount);
    new msg[128];
    format(msg, sizeof msg, "* %s takes out some cash and hands it to %s.", RPName(playerid), RPName(target));
    SendNearby(playerid, 15.0, COLOR_PURPLE, msg);
    SendClientMessage(playerid, COLOR_GREEN, "You paid %s to %s.", FormatMoney(amount), RPName(target));
    SendClientMessage(target, COLOR_GREEN, "%s paid you %s.", RPName(playerid), FormatMoney(amount));
    format(msg, sizeof msg, "[PAY] %s paid %s to %s", GetName(playerid), FormatMoney(amount), GetName(target));
    if (amount >= 100000) SendAdminMessage(COLOR_ADMIN, msg);
    return 1;
}
