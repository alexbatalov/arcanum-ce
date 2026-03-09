#include "game/item_effect.h"

#include "game/mes.h"

/**
 * The maximum number of item effect description message.
 *
 * 0x6018C0
 */
static int item_effect_max_num;

/**
 * "gameitemeffect.mes"
 *
 * 0x6018C4
 */
static mes_file_handle_t game_item_effect_mes_file;

/**
 * "item_effect.mes"
 *
 * 0x6018C8
 */
static mes_file_handle_t item_effect_mes_file;

/**
 * Called when the game is initialized.
 *
 * 0x4D3F60
 */
bool item_effect_init(GameInitInfo* init_info)
{
    int cnt;
    MesFileEntry mes_file_entry;

    (void)init_info;

    // Load system-wide item effect descriptions message file (required).
    if (!mes_load("mes\\item_effect.mes", &item_effect_mes_file)) {
        return false;
    }

    // Retrieve the last message to determine the highest message number.
    cnt = mes_entries_count(item_effect_mes_file);
    if (cnt != 0) {
        mes_get_entry(item_effect_mes_file, cnt - 1, &mes_file_entry);
        item_effect_max_num = mes_file_entry.num;
    } else {
        item_effect_max_num = 0;
    }

    game_item_effect_mes_file = MES_FILE_HANDLE_INVALID;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4D3FE0
 */
void item_effect_exit(void)
{
    mes_unload(item_effect_mes_file);
}

/**
 * Called when a module is being loaded.
 *
 * 0x4D3FF0
 */
bool item_effect_mod_load(void)
{
    int cnt;
    MesFileEntry mes_file_entry;

    // Load module-specific item effect descriptions message file (optional).
    if (mes_load("mes\\gameitemeffect.mes", &game_item_effect_mes_file)) {
        // Retrieve the last message to determine the highest message number.
        cnt = mes_entries_count(game_item_effect_mes_file);
        if (cnt != 0) {
            mes_get_entry(game_item_effect_mes_file, cnt - 1, &mes_file_entry);
            item_effect_max_num = mes_file_entry.num;
        }
    }

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4D4050
 */
void item_effect_mod_unload(void)
{
    mes_unload(game_item_effect_mes_file);
    game_item_effect_mes_file = MES_FILE_HANDLE_INVALID;
}

/**
 * Retrieves the item effect description for a specified number.
 *
 * 0x4D4080
 */
char* item_effect_get(int num)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    if (num < 0 || num > item_effect_max_num) {
        return NULL;
    }

    // Select system-wide vs. module-specific message file.
    if (num < 30000) {
        mes_file = item_effect_mes_file;
    } else {
        mes_file = game_item_effect_mes_file;
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
