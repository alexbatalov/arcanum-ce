#include "game/gameinit.h"

#include "game/map.h"
#include "game/mes.h"
#include "game/player.h"
#include "game/timeevent.h"

#define GAMEINIT_MES_NUM_ADVENTURE_NAME 0
#define GAMEINIT_MES_NUM_ADVENTURE_ID 1
#define GAMEINIT_MES_NUM_STARTING_YEAR 4
#define GAMEINIT_MES_NUM_STARTING_HOUR 5

// 0x5FC300
static const char* gameinit_adventure_name;

// 0x5FC304
static int gameinit_adventure_id;

// 0x5FC308
static int dword_5FC308;

// 0x5FC310
static int dword_5FC310;

// 0x5FC314
static int dword_5FC314;

// 0x5FC318
static int dword_5FC318;

// 0x5FC31C
static int dword_5FC31C;

// 0x5FC320
static mes_file_handle_t gameinit_mes_file;

// 0x5FC324
static bool gameinit_editor;

// 0x5FC328
static bool gameinit_initialized;

// 0x4B9B90
bool gameinit_init(GameInitInfo* init_info)
{
    gameinit_adventure_id = -1;
    gameinit_adventure_name = NULL;
    dword_5FC308 = 0;
    dword_5FC310 = 0;
    dword_5FC314 = 0;
    dword_5FC318 = 0;
    dword_5FC31C = 0;
    gameinit_editor = init_info->editor;
    gameinit_initialized = true;

    return true;
}

// 0x4B9BE0
void gameinit_reset(void)
{
    int map;

    if (!gameinit_editor) {
        map = map_by_type(MAP_TYPE_SHOPPING_MAP);
        if (map == 0) {
            map = map_by_type(MAP_TYPE_START_MAP);
        }
        map_open_in_game(map, 0, 1);
        player_create();
    }
}

// 0x4B9C20
void gameinit_exit(void)
{
    gameinit_initialized = false;
}

// 0x4B9C30
bool gameinit_mod_load(void)
{
    MesFileEntry mes_file_entry;

    if (!gameinit_editor) {
        gameinit_adventure_name = "";
        gameinit_adventure_id = 0;

        if (mes_load("rules\\gameinit.mes", &gameinit_mes_file)) {
            tig_str_parse_set_separator(',');

            mes_file_entry.num = GAMEINIT_MES_NUM_ADVENTURE_NAME;
            if (mes_search(gameinit_mes_file, &mes_file_entry)) {
                mes_get_msg(gameinit_mes_file, &mes_file_entry);
                gameinit_adventure_name = mes_file_entry.str;
            }

            mes_file_entry.num = GAMEINIT_MES_NUM_ADVENTURE_ID;
            if (mes_search(gameinit_mes_file, &mes_file_entry)) {
                mes_get_msg(gameinit_mes_file, &mes_file_entry);
                gameinit_adventure_id = atoi(mes_file_entry.str);
            }

            mes_file_entry.num = GAMEINIT_MES_NUM_STARTING_YEAR;
            if (mes_search(gameinit_mes_file, &mes_file_entry)) {
                mes_get_msg(gameinit_mes_file, &mes_file_entry);
                datetime_set_start_year(atoi(mes_file_entry.str));
            }

            mes_file_entry.num = GAMEINIT_MES_NUM_STARTING_HOUR;
            if (mes_search(gameinit_mes_file, &mes_file_entry)) {
                mes_get_msg(gameinit_mes_file, &mes_file_entry);
                datetime_set_start_hour(atoi(mes_file_entry.str));
            }
        } else {
            datetime_set_start_year(1885);
            datetime_set_start_hour(15);
        }

        gameinit_reset();
    }
    return true;
}

// 0x4B9DB0
void gameinit_mod_unload(void)
{
    if (!gameinit_editor) {
        gameinit_adventure_name = NULL;
        dword_5FC308 = 0;
        dword_5FC310 = 0;
        dword_5FC314 = 0;
        dword_5FC318 = 0;
        dword_5FC31C = 0;
        gameinit_adventure_id = -1;
        mes_unload(gameinit_mes_file);
        gameinit_mes_file = MES_FILE_HANDLE_INVALID;
        player_exit();
    }
}
