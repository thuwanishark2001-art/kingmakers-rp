// =====================================================================
//  Kingmakers RP - properties: houses & businesses
// =====================================================================

enum E_HOUSE_INT { hiInt, Float:hiX, Float:hiY, Float:hiZ, hiName[16] }
new const HouseInteriors[][E_HOUSE_INT] =
{
    { 1, 223.20,  1287.08, 1082.14, "Studio" },
    { 2, 2468.84, -1698.24, 1013.51, "Apartment" },
    { 3, 2496.05, -1692.08, 1014.74, "Family house" },
    { 5, 1260.64, -785.37,  1091.90, "Mansion" },
    { 8, 2807.62, -1174.10, 1025.57, "Villa" }
};

enum { BIZ_247 = 0, BIZ_AMMU, BIZ_CLOTHES, MAX_BIZ_TYPES }
new const BizTypeNames[MAX_BIZ_TYPES][] = { "24/7 Store", "Ammu-Nation", "Binco Clothes" };
new const BizInteriors[MAX_BIZ_TYPES][E_HOUSE_INT] =
{
    { 6,  -26.69, -55.71,  1003.55, "24/7" },
    { 1,  286.15, -40.64,  1001.52, "Ammu" },
    { 15, 207.74, -110.97, 1005.13, "Binco" }
};

enum E_HOUSE
{
    bool:hExists,
    hDBID,
    hOwner,
    hOwnerName[MAX_PLAYER_NAME],
    hPrice,
    Float:hX, Float:hY, Float:hZ,
    hIType,
    bool:hLocked,
    hPickup,
    Text3D:hLabel
}
new House[MAX_HOUSES][E_HOUSE];

enum E_BIZ
{
    bool:bExists,
    bDBID,
    bOwner,
    bOwnerName[MAX_PLAYER_NAME],
    bType,
    bPrice,
    Float:bX, Float:bY, Float:bZ,
    bTill,
    bPickup,
    Text3D:bLabel
}
new Biz[MAX_BUSINESSES][E_BIZ];

// ---------------------------------------------------------------------
//  Loading / seeding
// ---------------------------------------------------------------------
stock SeedProperties()
{
    new DBResult:r = DB_ExecuteQuery(gDB, "SELECT COUNT(*) FROM houses");
    new count = 0;
    if (r) { count = DB_GetFieldInt(r, 0); DB_FreeResultSet(r); }
    if (count == 0)
    {
        DB_Exec("INSERT INTO houses (price, x, y, z, itype) VALUES \
            (25000, 2495.34, -1691.14, 14.77, 2), (18000, 2513.80, -1650.30, 14.36, 1), (18000, 2486.40, -1644.50, 14.08, 1),\
            (18000, 2498.60, -1642.30, 14.11, 1), (22000, 2524.50, -1658.60, 15.49, 2), (12000, 2459.40, -1691.60, 13.55, 0),\
            (12000, 2451.80, -1641.40, 14.07, 0), (14000, 2065.10, -1703.50, 14.15, 1), (14000, 2067.00, -1731.60, 13.88, 1),\
            (14000, 2018.00, -1703.30, 14.23, 1), (9000,  1981.90, -1718.70, 17.03, 0), (250000, 1298.60, -798.00, 84.14, 3),\
            (180000, 1093.90, -806.90, 107.42, 4), (160000, 251.30, -1220.10, 76.10, 4)");
        print("[KM] Seeded default houses.");
    }
    r = DB_ExecuteQuery(gDB, "SELECT COUNT(*) FROM businesses");
    count = 0;
    if (r) { count = DB_GetFieldInt(r, 0); DB_FreeResultSet(r); }
    if (count == 0)
    {
        DB_Exec("INSERT INTO businesses (type, price, x, y, z) VALUES \
            (0, 90000, 1833.00, -1842.50, 13.58), (0, 75000, 1929.00, -1776.30, 13.55), (0, 70000, 1000.60, -920.00, 42.33),\
            (1, 150000, 1368.35, -1279.06, 13.55), (2, 110000, 2244.32, -1665.52, 15.48)");
        print("[KM] Seeded default businesses.");
    }
}

stock LoadProperties()
{
    SeedProperties();
    new DBResult:r = DB_ExecuteQuery(gDB, "SELECT h.*, IFNULL(a.name, '') AS oname FROM houses h LEFT JOIN accounts a ON a.id = h.owner");
    new i = 0;
    if (r && DB_GetRowCount(r) > 0)
    {
        do
        {
            if (i >= MAX_HOUSES) break;
            House[i][hExists] = true;
            House[i][hDBID] = DB_GetFieldIntByName(r, "id");
            House[i][hOwner] = DB_GetFieldIntByName(r, "owner");
            DB_GetFieldStringByName(r, "oname", House[i][hOwnerName], MAX_PLAYER_NAME);
            House[i][hPrice] = DB_GetFieldIntByName(r, "price");
            House[i][hX] = DB_GetFieldFloatByName(r, "x");
            House[i][hY] = DB_GetFieldFloatByName(r, "y");
            House[i][hZ] = DB_GetFieldFloatByName(r, "z");
            House[i][hIType] = DB_GetFieldIntByName(r, "itype");
            if (House[i][hIType] < 0 || House[i][hIType] >= sizeof HouseInteriors) House[i][hIType] = 0;
            House[i][hLocked] = DB_GetFieldIntByName(r, "locked") != 0;
            House[i][hPickup] = -1;
            UpdateHouseVisual(i);
            i++;
        }
        while (DB_SelectNextRow(r));
    }
    if (r) DB_FreeResultSet(r);
    printf("[KM] Loaded %d houses.", i);

    r = DB_ExecuteQuery(gDB, "SELECT b.*, IFNULL(a.name, '') AS oname FROM businesses b LEFT JOIN accounts a ON a.id = b.owner");
    i = 0;
    if (r && DB_GetRowCount(r) > 0)
    {
        do
        {
            if (i >= MAX_BUSINESSES) break;
            Biz[i][bExists] = true;
            Biz[i][bDBID] = DB_GetFieldIntByName(r, "id");
            Biz[i][bOwner] = DB_GetFieldIntByName(r, "owner");
            DB_GetFieldStringByName(r, "oname", Biz[i][bOwnerName], MAX_PLAYER_NAME);
            Biz[i][bType] = DB_GetFieldIntByName(r, "type");
            if (Biz[i][bType] < 0 || Biz[i][bType] >= MAX_BIZ_TYPES) Biz[i][bType] = BIZ_247;
            Biz[i][bPrice] = DB_GetFieldIntByName(r, "price");
            Biz[i][bX] = DB_GetFieldFloatByName(r, "x");
            Biz[i][bY] = DB_GetFieldFloatByName(r, "y");
            Biz[i][bZ] = DB_GetFieldFloatByName(r, "z");
            Biz[i][bTill] = DB_GetFieldIntByName(r, "till");
            Biz[i][bPickup] = -1;
            UpdateBizVisual(i);
            i++;
        }
        while (DB_SelectNextRow(r));
    }
    if (r) DB_FreeResultSet(r);
    printf("[KM] Loaded %d businesses.", i);
}

stock UpdateHouseVisual(h)
{
    new label[200];
    if (House[h][hPickup] != -1) DestroyPickup(House[h][hPickup]);
    if (House[h][hLabel] != Text3D:0) Delete3DTextLabel(House[h][hLabel]);
    if (House[h][hOwner] == 0)
    {
        House[h][hPickup] = CreatePickup(1273, 1, House[h][hX], House[h][hY], House[h][hZ], 0);
        format(label, sizeof label, "{5CE65C}House for sale\n{FFFFFF}%s\nPrice: {5CE65C}%s\n{AFAFAF}/buyhouse  -  /enter", HouseInteriors[House[h][hIType]][hiName], FormatMoney(House[h][hPrice]));
    }
    else
    {
        new owner[MAX_PLAYER_NAME];
        format(owner, sizeof owner, "%s", House[h][hOwnerName]);
        for (new c = 0; owner[c]; c++) if (owner[c] == '_') owner[c] = ' ';
        House[h][hPickup] = CreatePickup(1272, 1, House[h][hX], House[h][hY], House[h][hZ], 0);
        format(label, sizeof label, "{FFC83D}%s\n{FFFFFF}Owner: %s\n%s\n{AFAFAF}/enter", HouseInteriors[House[h][hIType]][hiName], owner, House[h][hLocked] ? ("{FF4B4B}Locked") : ("{5CE65C}Unlocked"));
    }
    House[h][hLabel] = Create3DTextLabel(label, COLOR_WHITE, House[h][hX], House[h][hY], House[h][hZ] + 0.7, 15.0, 0, true);
}

stock UpdateBizVisual(b)
{
    new label[200];
    if (Biz[b][bPickup] != -1) DestroyPickup(Biz[b][bPickup]);
    if (Biz[b][bLabel] != Text3D:0) Delete3DTextLabel(Biz[b][bLabel]);
    Biz[b][bPickup] = CreatePickup(Biz[b][bType] == BIZ_AMMU ? 1254 : (Biz[b][bType] == BIZ_CLOTHES ? 1275 : 1272 + 1), 1, Biz[b][bX], Biz[b][bY], Biz[b][bZ], 0);
    if (Biz[b][bOwner] == 0)
        format(label, sizeof label, "{FFC83D}%s\n{5CE65C}Business for sale: %s\n{AFAFAF}/enter  -  /buybiz", BizTypeNames[Biz[b][bType]], FormatMoney(Biz[b][bPrice]));
    else
    {
        new owner[MAX_PLAYER_NAME];
        format(owner, sizeof owner, "%s", Biz[b][bOwnerName]);
        for (new c = 0; owner[c]; c++) if (owner[c] == '_') owner[c] = ' ';
        format(label, sizeof label, "{FFC83D}%s\n{FFFFFF}Owner: %s\n{AFAFAF}/enter", BizTypeNames[Biz[b][bType]], owner);
    }
    Biz[b][bLabel] = Create3DTextLabel(label, COLOR_WHITE, Biz[b][bX], Biz[b][bY], Biz[b][bZ] + 0.7, 15.0, 0, true);
}

stock SaveHouse(h)
{
    new q[200];
    format(q, sizeof q, "UPDATE houses SET owner=%d, price=%d, locked=%d WHERE id=%d", House[h][hOwner], House[h][hPrice], House[h][hLocked] ? 1 : 0, House[h][hDBID]);
    DB_Exec(q);
}

stock SaveBiz(b)
{
    new q[200];
    format(q, sizeof q, "UPDATE businesses SET owner=%d, price=%d, till=%d WHERE id=%d", Biz[b][bOwner], Biz[b][bPrice], Biz[b][bTill], Biz[b][bDBID]);
    DB_Exec(q);
}

stock CountPlayerHouses(playerid)
{
    new c = 0;
    for (new h = 0; h < MAX_HOUSES; h++) if (House[h][hExists] && House[h][hOwner] == Player[playerid][pID] && Player[playerid][pID] != 0) c++;
    return c;
}

stock GetNearHouseDoor(playerid, Float:range = 2.5)
{
    if (GetPlayerVirtualWorld(playerid) != 0) return -1;
    for (new h = 0; h < MAX_HOUSES; h++)
        if (House[h][hExists] && IsPlayerInRangeOfPoint(playerid, range, House[h][hX], House[h][hY], House[h][hZ])) return h;
    return -1;
}

stock GetNearBizDoor(playerid, Float:range = 2.5)
{
    if (GetPlayerVirtualWorld(playerid) != 0) return -1;
    for (new b = 0; b < MAX_BUSINESSES; b++)
        if (Biz[b][bExists] && IsPlayerInRangeOfPoint(playerid, range, Biz[b][bX], Biz[b][bY], Biz[b][bZ])) return b;
    return -1;
}

stock AddHouse(Float:x, Float:y, Float:z, itype, price)
{
    new h = -1;
    for (new i = 0; i < MAX_HOUSES; i++) if (!House[i][hExists]) { h = i; break; }
    if (h == -1) return -1;
    new q[200];
    format(q, sizeof q, "INSERT INTO houses (price, x, y, z, itype) VALUES (%d, %f, %f, %f, %d)", price, x, y, z, itype);
    DB_Exec(q);
    new DBResult:r = DB_ExecuteQuery(gDB, "SELECT last_insert_rowid()");
    if (r) { House[h][hDBID] = DB_GetFieldInt(r, 0); DB_FreeResultSet(r); }
    House[h][hExists] = true;
    House[h][hOwner] = 0;
    House[h][hOwnerName][0] = 0;
    House[h][hPrice] = price;
    House[h][hX] = x; House[h][hY] = y; House[h][hZ] = z;
    House[h][hIType] = itype;
    House[h][hLocked] = false;
    House[h][hPickup] = -1;
    House[h][hLabel] = Text3D:0;
    UpdateHouseVisual(h);
    return h;
}

stock AddBiz(Float:x, Float:y, Float:z, type, price)
{
    new b = -1;
    for (new i = 0; i < MAX_BUSINESSES; i++) if (!Biz[i][bExists]) { b = i; break; }
    if (b == -1) return -1;
    new q[200];
    format(q, sizeof q, "INSERT INTO businesses (type, price, x, y, z) VALUES (%d, %d, %f, %f, %f)", type, price, x, y, z);
    DB_Exec(q);
    new DBResult:r = DB_ExecuteQuery(gDB, "SELECT last_insert_rowid()");
    if (r) { Biz[b][bDBID] = DB_GetFieldInt(r, 0); DB_FreeResultSet(r); }
    Biz[b][bExists] = true;
    Biz[b][bOwner] = 0;
    Biz[b][bOwnerName][0] = 0;
    Biz[b][bType] = type;
    Biz[b][bPrice] = price;
    Biz[b][bX] = x; Biz[b][bY] = y; Biz[b][bZ] = z;
    Biz[b][bTill] = 0;
    Biz[b][bPickup] = -1;
    Biz[b][bLabel] = Text3D:0;
    UpdateBizVisual(b);
    return b;
}

// ---------------------------------------------------------------------
//  Enter / exit
// ---------------------------------------------------------------------
stock TryEnter(playerid)
{
    new h = GetNearHouseDoor(playerid);
    if (h != -1)
    {
        if (House[h][hLocked] && House[h][hOwner] != Player[playerid][pID] && Player[playerid][pAdmin] < 2)
            return GameTextForPlayer(playerid, "~r~Locked", 2000, 4), 1;
        new t = House[h][hIType];
        SetPlayerInterior(playerid, HouseInteriors[t][hiInt]);
        SetPlayerVirtualWorld(playerid, 1000 + h);
        SetPlayerPos(playerid, HouseInteriors[t][hiX], HouseInteriors[t][hiY], HouseInteriors[t][hiZ]);
        SetCameraBehindPlayer(playerid);
        Player[playerid][pHouseInside] = h;
        if (House[h][hOwner] == Player[playerid][pID]) Info(playerid, "Welcome home. /house to manage your house, /exit to leave.");
        return 1;
    }
    new b = GetNearBizDoor(playerid);
    if (b != -1)
    {
        new t = Biz[b][bType];
        SetPlayerInterior(playerid, BizInteriors[t][hiInt]);
        SetPlayerVirtualWorld(playerid, 2000 + b);
        SetPlayerPos(playerid, BizInteriors[t][hiX], BizInteriors[t][hiY], BizInteriors[t][hiZ]);
        SetCameraBehindPlayer(playerid);
        Player[playerid][pBizInside] = b;
        SendClientMessage(playerid, COLOR_GOLD, "Welcome to %s! {FFFFFF}Use /buy to shop, /exit to leave.", BizTypeNames[t]);
        return 1;
    }
    return 0;
}

stock TryExit(playerid)
{
    new h = Player[playerid][pHouseInside];
    if (h != -1 && GetPlayerVirtualWorld(playerid) == 1000 + h)
    {
        new t = House[h][hIType];
        if (!IsPlayerInRangeOfPoint(playerid, 3.0, HouseInteriors[t][hiX], HouseInteriors[t][hiY], HouseInteriors[t][hiZ])) return Error(playerid, "Go to the door to exit."), 1;
        SetPlayerInterior(playerid, 0);
        SetPlayerVirtualWorld(playerid, 0);
        SetPlayerPos(playerid, House[h][hX], House[h][hY], House[h][hZ]);
        SetCameraBehindPlayer(playerid);
        Player[playerid][pHouseInside] = -1;
        return 1;
    }
    new b = Player[playerid][pBizInside];
    if (b != -1 && GetPlayerVirtualWorld(playerid) == 2000 + b)
    {
        new t = Biz[b][bType];
        if (!IsPlayerInRangeOfPoint(playerid, 3.0, BizInteriors[t][hiX], BizInteriors[t][hiY], BizInteriors[t][hiZ])) return Error(playerid, "Go to the door to exit."), 1;
        SetPlayerInterior(playerid, 0);
        SetPlayerVirtualWorld(playerid, 0);
        SetPlayerPos(playerid, Biz[b][bX], Biz[b][bY], Biz[b][bZ]);
        SetCameraBehindPlayer(playerid);
        Player[playerid][pBizInside] = -1;
        return 1;
    }
    return 0;
}

stock BizSale(b, amount)
{
    if (b < 0 || !Biz[b][bExists] || Biz[b][bOwner] == 0) return;
    Biz[b][bTill] += amount * 30 / 100;
    SaveBiz(b);
}

stock bool:InBizType(playerid, type)
{
    new b = Player[playerid][pBizInside];
    return b != -1 && Biz[b][bExists] && Biz[b][bType] == type && GetPlayerVirtualWorld(playerid) == 2000 + b;
}

stock Property_OnDialog(playerid, dialogid, response, listitem, const inputtext[])
{
    switch (dialogid)
    {
        case DLG_SHOP_247:
        {
            if (!response) return 1;
            new b = Player[playerid][pBizInside], price;
            switch (listitem)
            {
                case 0:
                {
                    if (Player[playerid][pPhone]) return Error(playerid, "You already have a phone."), 1;
                    price = PHONE_PRICE;
                    if (Player[playerid][pMoney] < price) return Error(playerid, "Not enough cash."), 1;
                    new number;
                    for (;;)
                    {
                        number = 100000 + random(900000);
                        new q[96];
                        format(q, sizeof q, "SELECT id FROM accounts WHERE phone = %d", number);
                        new DBResult:r = DB_ExecuteQuery(gDB, q);
                        new taken = r && DB_GetRowCount(r) > 0;
                        if (r) DB_FreeResultSet(r);
                        if (!taken) break;
                    }
                    Player[playerid][pPhone] = number;
                    SendClientMessage(playerid, COLOR_GREEN, "You bought a KingPhone! Your number is %d. Use /phone to open it.", number);
                }
                case 1:
                {
                    price = 300;
                    if (Player[playerid][pMoney] < price) return Error(playerid, "Not enough cash."), 1;
                    if (Player[playerid][pMedkits] >= 5) return Error(playerid, "You can carry at most 5 medkits."), 1;
                    Player[playerid][pMedkits]++;
                    SendClientMessage(playerid, COLOR_GREEN, "You bought a medkit (%d). Use /usemedkit.", Player[playerid][pMedkits]);
                }
                case 2:
                {
                    price = 25;
                    if (Player[playerid][pMoney] < price) return Error(playerid, "Not enough cash."), 1;
                    new Float:hp;
                    GetPlayerHealth(playerid, hp);
                    SetPlayerHealth(playerid, hp + 25.0 > 100.0 ? 100.0 : hp + 25.0);
                    ApplyAnimation(playerid, "FOOD", "EAT_Burger", 4.1, false, false, false, false, 0, SYNC_ALL);
                    SendClientMessage(playerid, COLOR_GREEN, "You ate a burger. (+25 HP)");
                }
                case 3:
                {
                    price = 200;
                    if (Player[playerid][pMoney] < price) return Error(playerid, "Not enough cash."), 1;
                    GivePlayerWeapon(playerid, WEAPON_CAMERA, 50);
                    SendClientMessage(playerid, COLOR_GREEN, "You bought a camera.");
                }
                case 4:
                {
                    price = 50;
                    if (Player[playerid][pMoney] < price) return Error(playerid, "Not enough cash."), 1;
                    GivePlayerWeapon(playerid, WEAPON_FLOWER, 1);
                    SendClientMessage(playerid, COLOR_GREEN, "You bought flowers.");
                }
                case 5:
                {
                    price = 400;
                    if (Player[playerid][pMoney] < price) return Error(playerid, "Not enough cash."), 1;
                    GivePlayerWeapon(playerid, WEAPON_BAT, 1);
                    SendClientMessage(playerid, COLOR_GREEN, "You bought a baseball bat.");
                }
            }
            GiveMoney(playerid, -price);
            BizSale(b, price);
            return 1;
        }
        case DLG_SHOP_AMMU:
        {
            if (!response) return 1;
            new const WEAPON:weps[] = { WEAPON_COLT45, WEAPON_DEAGLE, WEAPON_SHOTGUN, WEAPON_MP5, WEAPON_FIST };
            new const prices[] = { 1500, 4000, 5000, 7500, 2000 };
            new const ammo[] = { 100, 50, 30, 150, 0 };
            if (listitem < 0 || listitem >= sizeof prices) return 1;
            if (Player[playerid][pMoney] < prices[listitem]) return Error(playerid, "Not enough cash."), 1;
            if (listitem == 4) SetPlayerArmour(playerid, 100.0);
            else GivePlayerWeapon(playerid, weps[listitem], ammo[listitem]);
            GiveMoney(playerid, -prices[listitem]);
            BizSale(Player[playerid][pBizInside], prices[listitem]);
            SendClientMessage(playerid, COLOR_GREEN, "Purchase complete: %s.", FormatMoney(prices[listitem]));
            return 1;
        }
        case DLG_SHOP_CLOTHES:
        {
            if (!response) return 1;
            new skin = strval(inputtext);
            if (skin < 1 || skin > 311 || skin == 74) return Error(playerid, "Invalid skin ID (1-311)."), 1;
            if (skin == 280 || skin == 281 || skin == 282 || skin == 283 || skin == 284 || skin == 285 || skin == 286 || skin == 274 || skin == 275 || skin == 276)
                return Error(playerid, "Uniforms are not sold here."), 1;
            if (Player[playerid][pMoney] < 500) return Error(playerid, "Not enough cash."), 1;
            GiveMoney(playerid, -500);
            BizSale(Player[playerid][pBizInside], 500);
            Player[playerid][pSkin] = skin;
            SetPlayerSkin(playerid, skin);
            SendClientMessage(playerid, COLOR_GREEN, "You bought new clothes (skin %d) for $500.", skin);
            SavePlayer(playerid);
            return 1;
        }
        case DLG_HOUSE_MENU:
        {
            if (!response) return 1;
            new h = Player[playerid][pHouseInside];
            if (h == -1 || House[h][hOwner] != Player[playerid][pID]) return 1;
            switch (listitem)
            {
                case 0:
                {
                    House[h][hLocked] = !House[h][hLocked];
                    SaveHouse(h);
                    UpdateHouseVisual(h);
                    Info(playerid, House[h][hLocked] ? ("House locked.") : ("House unlocked."));
                }
                case 1:
                {
                    new refund = House[h][hPrice] / 2;
                    House[h][hOwner] = 0;
                    House[h][hOwnerName][0] = 0;
                    House[h][hLocked] = false;
                    SaveHouse(h);
                    UpdateHouseVisual(h);
                    GiveMoney(playerid, refund);
                    SendClientMessage(playerid, COLOR_GREEN, "You sold your house to the state for %s.", FormatMoney(refund));
                    SavePlayer(playerid);
                }
                case 2:
                {
                    SetPlayerHealth(playerid, 100.0);
                    Info(playerid, "You rested at home and recovered full health.");
                }
            }
            return 1;
        }
        case DLG_BIZ_MENU:
        {
            if (!response) return 1;
            new b = Player[playerid][pBizInside];
            if (b == -1 || Biz[b][bOwner] != Player[playerid][pID]) return 1;
            switch (listitem)
            {
                case 0:
                {
                    new amount = Biz[b][bTill];
                    if (amount <= 0) return Error(playerid, "The till is empty."), 1;
                    Biz[b][bTill] = 0;
                    SaveBiz(b);
                    GiveMoney(playerid, amount);
                    SendClientMessage(playerid, COLOR_GREEN, "You collected %s from the till.", FormatMoney(amount));
                    SavePlayer(playerid);
                }
                case 1:
                {
                    new refund = Biz[b][bPrice] / 2 + Biz[b][bTill];
                    Biz[b][bOwner] = 0;
                    Biz[b][bOwnerName][0] = 0;
                    Biz[b][bTill] = 0;
                    SaveBiz(b);
                    UpdateBizVisual(b);
                    GiveMoney(playerid, refund);
                    SendClientMessage(playerid, COLOR_GREEN, "You sold your business for %s.", FormatMoney(refund));
                    SavePlayer(playerid);
                }
            }
            return 1;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------
//  Commands
// ---------------------------------------------------------------------
CMD:enter(playerid, params[])
{
    if (!TryEnter(playerid)) Error(playerid, "You are not at an entrance.");
    return 1;
}

CMD:exit(playerid, params[])
{
    if (!TryExit(playerid)) Error(playerid, "You are not inside a building (or not at the door).");
    return 1;
}

CMD:buyhouse(playerid, params[])
{
    new h = GetNearHouseDoor(playerid);
    if (h == -1) return Error(playerid, "You are not at a house door.");
    if (House[h][hOwner] != 0) return Error(playerid, "This house is not for sale.");
    if (CountPlayerHouses(playerid) >= 2) return Error(playerid, "You can own at most 2 houses.");
    if (Player[playerid][pLevel] < 2) return Error(playerid, "You must be at least level 2 to buy a house.");
    if (Player[playerid][pMoney] < House[h][hPrice]) return Error(playerid, "You don't have enough cash.");
    GiveMoney(playerid, -House[h][hPrice]);
    House[h][hOwner] = Player[playerid][pID];
    format(House[h][hOwnerName], MAX_PLAYER_NAME, "%s", GetName(playerid));
    SaveHouse(h);
    UpdateHouseVisual(h);
    SendClientMessage(playerid, COLOR_GREEN, "Congratulations! You bought this %s for %s.", HouseInteriors[House[h][hIType]][hiName], FormatMoney(House[h][hPrice]));
    GameTextForPlayer(playerid, "~y~Welcome home!", 3000, 1);
    SavePlayer(playerid);
    return 1;
}

CMD:house(playerid, params[])
{
    new h = Player[playerid][pHouseInside];
    if (h == -1 || GetPlayerVirtualWorld(playerid) != 1000 + h || House[h][hOwner] != Player[playerid][pID])
        return Error(playerid, "You must be inside your own house.");
    ShowPlayerDialog(playerid, DLG_HOUSE_MENU, DIALOG_STYLE_LIST, C_GOLD "House Menu",
        "Lock / unlock door\nSell house to the state (50%)\nRest (restore health)", "Select", "Close");
    return 1;
}

CMD:buybiz(playerid, params[])
{
    new b = GetNearBizDoor(playerid);
    if (b == -1) return Error(playerid, "You are not at a business entrance.");
    if (Biz[b][bOwner] != 0) return Error(playerid, "This business is not for sale.");
    if (Player[playerid][pLevel] < 4) return Error(playerid, "You must be at least level 4 to buy a business.");
    for (new i = 0; i < MAX_BUSINESSES; i++)
        if (Biz[i][bExists] && Biz[i][bOwner] == Player[playerid][pID]) return Error(playerid, "You already own a business.");
    if (Player[playerid][pMoney] < Biz[b][bPrice]) return Error(playerid, "You don't have enough cash.");
    GiveMoney(playerid, -Biz[b][bPrice]);
    Biz[b][bOwner] = Player[playerid][pID];
    format(Biz[b][bOwnerName], MAX_PLAYER_NAME, "%s", GetName(playerid));
    SaveBiz(b);
    UpdateBizVisual(b);
    SendClientMessage(playerid, COLOR_GREEN, "You are now the owner of this %s! You earn 30%% of every sale. Use /bizmenu inside.", BizTypeNames[Biz[b][bType]]);
    SavePlayer(playerid);
    return 1;
}

CMD:bizmenu(playerid, params[])
{
    new b = Player[playerid][pBizInside];
    if (b == -1 || GetPlayerVirtualWorld(playerid) != 2000 + b || Biz[b][bOwner] != Player[playerid][pID])
        return Error(playerid, "You must be inside your own business.");
    ShowPlayerDialog(playerid, DLG_BIZ_MENU, DIALOG_STYLE_LIST, C_GOLD "Business Menu",
        "Collect till\nSell business (50% + till)", "Select", "Close");
    return 1;
}

CMD:buy(playerid, params[])
{
    if (InBizType(playerid, BIZ_247))
    {
        ShowPlayerDialog(playerid, DLG_SHOP_247, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "24/7 Store",
            "Item\tPrice\nKingPhone (phone + SIM)\t" C_GREEN "$150\nMedkit\t" C_GREEN "$300\nBurger (+25 HP)\t" C_GREEN "$25\nCamera\t" C_GREEN "$200\nFlowers\t" C_GREEN "$50\nBaseball bat\t" C_GREEN "$400",
            "Buy", "Close");
        return 1;
    }
    if (InBizType(playerid, BIZ_AMMU))
    {
        if (!Player[playerid][pGunLic]) return Error(playerid, "You need a weapon license (City Hall).");
        ShowPlayerDialog(playerid, DLG_SHOP_AMMU, DIALOG_STYLE_TABLIST_HEADERS, C_GOLD "Ammu-Nation",
            "Item\tPrice\nColt 45 (100 rounds)\t" C_GREEN "$1,500\nDesert Eagle (50 rounds)\t" C_GREEN "$4,000\nShotgun (30 shells)\t" C_GREEN "$5,000\nMP5 (150 rounds)\t" C_GREEN "$7,500\nBody armour\t" C_GREEN "$2,000",
            "Buy", "Close");
        return 1;
    }
    if (InBizType(playerid, BIZ_CLOTHES))
    {
        ShowPlayerDialog(playerid, DLG_SHOP_CLOTHES, DIALOG_STYLE_INPUT, C_GOLD "Binco Clothes",
            C_WHITE "New outfit: " C_GREEN "$500\n\n" C_WHITE "Enter a skin ID (1-311).\nTip: browse skins at open.mp/docs/scripting/resources/skins", "Buy", "Close");
        return 1;
    }
    return Error(playerid, "You are not inside a shop.");
}

CMD:usemedkit(playerid, params[])
{
    if (Player[playerid][pMedkits] <= 0) return Error(playerid, "You don't have any medkits.");
    if (Player[playerid][pInjured]) return Error(playerid, "You are too injured; you need EMS.");
    Player[playerid][pMedkits]--;
    new Float:hp;
    GetPlayerHealth(playerid, hp);
    SetPlayerHealth(playerid, hp + 50.0 > 100.0 ? 100.0 : hp + 50.0);
    new msg[96];
    format(msg, sizeof msg, "* %s uses a medkit.", RPName(playerid));
    SendNearby(playerid, 15.0, COLOR_PURPLE, msg);
    return 1;
}
