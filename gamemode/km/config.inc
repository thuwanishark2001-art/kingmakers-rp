// =====================================================================
//  Kingmakers RP - configuration
// =====================================================================

#define SERVER_NAME         "Kingmakers RP"
#define SERVER_SHORT        "KMRP"
#define SERVER_VERSION      "1.0.0"
#define SERVER_WEBSITE      "kingmakers-rp.com"
#define DATABASE_FILE       "kingmakers.db"

// ---- Brand colours (gold / purple night city) ----
#define COLOR_GOLD          0xFFC83DFF
#define COLOR_WHITE         0xFFFFFFFF
#define COLOR_GREY          0xAFAFAFFF
#define COLOR_RED           0xFF4B4BFF
#define COLOR_GREEN         0x5CE65CFF
#define COLOR_YELLOW        0xFFFF66FF
#define COLOR_PURPLE        0xC2A2DAFF
#define COLOR_LIGHTBLUE     0x6FC3FFFF
#define COLOR_ORANGE        0xFF9933FF
#define COLOR_PINK          0xFF66CCFF
#define COLOR_ADMIN         0xFF6347FF
#define COLOR_OOC           0xC8C8FFFF
#define COLOR_FACTION       0x8FD8FFFF
#define COLOR_RADIO         0x99CCFFFF
#define COLOR_PHONE         0xFFEB7AFF
#define COLOR_FADE1         0xE6E6E6FF
#define COLOR_FADE2         0xC8C8C8FF
#define COLOR_FADE3         0xAAAAAAFF

#define C_GOLD              "{FFC83D}"
#define C_WHITE             "{FFFFFF}"
#define C_GREY              "{AFAFAF}"
#define C_GREEN             "{5CE65C}"
#define C_RED               "{FF4B4B}"
#define C_BLUE              "{6FC3FF}"
#define C_PURPLE            "{C2A2DA}"

// ---- Economy tuning ----
#define START_CASH          3500
#define START_BANK          5000
#define PAYDAY_MINUTES      30          // minutes of play time between paychecks
#define BASE_SALARY         350
#define LEVEL_SALARY_BONUS  50
#define BANK_INTEREST_BP    10          // basis points per payday (10 = 0.10%)
#define HOSPITAL_FEE        250
#define MAX_PLAYER_VEHICLES 3
#define FUEL_PRICE          3           // $ per 1% fuel
#define PHONE_PRICE         150
#define LICENSE_DRIVE_PRICE 500
#define LICENSE_GUN_PRICE   2500
#define AUTOSAVE_MINUTES    5
#define INJURED_SECONDS     60          // time before /acceptdeath allowed
#define JAIL_SECONDS_PER_STAR 120
#define TURF_CAPTURE_SECONDS 180
#define TURF_INCOME         250

// ---- Limits ----
#define MAX_HOUSES          200
#define MAX_BUSINESSES      100
#define MAX_OWNED_VEH       (MAX_PLAYERS * MAX_PLAYER_VEHICLES)
#define MAX_TURFS           16

// ---- Key locations (Los Santos) ----
#define SPAWN_X             1742.81
#define SPAWN_Y             -1861.37
#define SPAWN_Z             13.58
#define SPAWN_A             0.0

#define HOSPITAL_X          1177.83
#define HOSPITAL_Y          -1323.57
#define HOSPITAL_Z          14.09

#define CITYHALL_X          1481.04
#define CITYHALL_Y          -1771.31
#define CITYHALL_Z          18.80

#define LSPD_X              1554.80
#define LSPD_Y              -1675.60
#define LSPD_Z              16.20
#define JAIL_X              264.10
#define JAIL_Y              77.60
#define JAIL_Z              1001.04
#define JAIL_INT            6

#define DEALER_X            2131.80
#define DEALER_Y            -1150.60
#define DEALER_Z            24.10
#define DEALER_SPAWN_X      2148.50
#define DEALER_SPAWN_Y      -1138.80
#define DEALER_SPAWN_Z      25.60
#define DEALER_SPAWN_A      270.0

// Login camera - sunset over downtown Los Santos
#define LOGIN_CAM_X         1300.0
#define LOGIN_CAM_Y         -1450.0
#define LOGIN_CAM_Z         160.0
#define LOGIN_LOOK_X        1500.0
#define LOGIN_LOOK_Y        -1300.0
#define LOGIN_LOOK_Z        80.0
