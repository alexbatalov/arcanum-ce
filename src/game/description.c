#include "game/description.h"

#include "game/mes.h"

/**
 * "gamekey.mes"
 *
 * 0x6018CC
 */
static mes_file_handle_t gamekey_mes_file;

/**
 * "gamedesc.mes"
 *
 * 0x6018D0
 */
static mes_file_handle_t gamedesc_mes_file;

/**
 * "description.mes"
 *
 * 0x6018D4
 */
static mes_file_handle_t description_mes_file;

/**
 * The maximum number of description message.
 *
 * 0x6018D8
 */
static int description_max_num;

/**
 * Called when the game is initialized.
 *
 * 0x4D40D0
 */
bool description_init(GameInitInfo* init_info)
{
    int cnt;
    MesFileEntry mes_file_entry;

    (void)init_info;

    // Load system-wide descriptions message file (required).
    if (!mes_load("mes\\description.mes", &description_mes_file)) {
        return false;
    }

    // Retrieve the last message to determine the highest message number.
    cnt = mes_entries_count(description_mes_file);
    if (cnt != 0) {
        mes_get_entry(description_mes_file, cnt - 1, &mes_file_entry);
        description_max_num = mes_file_entry.num;
    } else {
        description_max_num = 0;
    }

    gamedesc_mes_file = MES_FILE_HANDLE_INVALID;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4D4150
 */
void description_exit(void)
{
    mes_unload(description_mes_file);
}

/**
 * Called when a module is being loaded.
 *
 * 0x4D4160
 */
bool description_mod_load(void)
{
    int cnt;
    MesFileEntry mes_file_entry;

    // Load module-specific descriptions message file (optional).
    if (mes_load("mes\\gamedesc.mes", &gamedesc_mes_file)) {
        // Retrieve the last message to determine the highest message number.
        cnt = mes_entries_count(gamedesc_mes_file);
        if (cnt != 0) {
            mes_get_entry(gamedesc_mes_file, cnt - 1, &mes_file_entry);
            description_max_num = mes_file_entry.num;
        }
    }

    // Load module-specific keys description message file (optional).
    mes_load("mes\\gamekey.mes", &gamekey_mes_file);

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * NOTE: There is minor bug - `description_mes_max_num` should be recalculated
 * since `gamedesc_mes_file` gets unloaded, but it does not have negative
 * impact.
 *
 * 0x4D41D0
 */
void description_mod_unload(void)
{
    mes_unload(gamedesc_mes_file);
    gamedesc_mes_file = MES_FILE_HANDLE_INVALID;

    mes_unload(gamekey_mes_file);
    gamekey_mes_file = MES_FILE_HANDLE_INVALID;
}

/**
 * Retrieves the description for a specified number.
 *
 * 0x4D4210
 */
const char* description_get(int num)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    if (num < 0 || num > description_max_num) {
        return NULL;
    }

    // Select system-wide vs. module-specific message file.
    if (num < 30000) {
        mes_file = description_mes_file;
    } else {
        mes_file = gamedesc_mes_file;
        if (mes_file == MES_FILE_HANDLE_INVALID) {
            return NULL;
        }
    }

    mes_file_entry.num = num;
    if (!mes_search(mes_file, &mes_file_entry)) {
        return NULL;
    }

    return mes_file_entry.str;
}

/**
 * Retrieves the name of a specified key.
 *
 * 0x4D4260
 */
const char* key_description_get(int num)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = num;
    if (!mes_search(gamekey_mes_file, &mes_file_entry)) {
        return NULL;
    }

    return mes_file_entry.str;
}
