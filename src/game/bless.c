#include "game/bless.h"

#include "game/effect.h"
#include "game/mes.h"
#include "game/object.h"
#include "game/player.h"
#include "game/timeevent.h"
#include "game/ui.h"

#define BLESS_F_NAME 0
#define BLESS_F_EFFECT 1
#define BLESS_F_DESCRIPTION 2

static void bless_copy_field(int bless, int field, char* buffer);

/**
 * "gamebless.mes"
 *
 * 0x5FF410
 */
static mes_file_handle_t bless_mes_file;

/**
 * Called when a module is being loaded.
 *
 * 0x4C4110
 */
bool bless_mod_load(void)
{
    mes_load("mes\\gamebless.mes", &bless_mes_file);
    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4C4130
 */
void bless_mod_unload(void)
{
    mes_unload(bless_mes_file);
    bless_mes_file = MES_FILE_HANDLE_INVALID;
}

/**
 * Copies the name of a blessing into the provided buffer.
 *
 * 0x4C4150
 */
void bless_copy_name(int bless, char* buffer)
{
    bless_copy_field(bless, BLESS_F_NAME, buffer);
}

/**
 * Internal helper that copies a field from the message file for the given
 * blessing.
 *
 * 0x4C4170
 */
void bless_copy_field(int bless, int field, char* buffer)
{
    MesFileEntry mes_file_entry;

    // Clear output buffer.
    buffer[0] = '\0';

    // NOTE: For unknown reason blessing IDs starts at 50. Probably there was a
    // separation between core blessings (0-50) and module-specific blessings
    // (50+), but either it was not completed, or it was removed.
    if (bless < 50) {
        return;
    }

    // Blessings data comes in a banks of 10.
    mes_file_entry.num = bless * 10 + field;
    if (mes_search(bless_mes_file, &mes_file_entry)) {
        strcpy(buffer, mes_file_entry.str);
    }
}

/**
 * Copies the description of a blessing into the provided buffer.
 *
 * 0x4C41E0
 */
void bless_copy_description(int bless, char* buffer)
{
    bless_copy_field(bless, BLESS_F_DESCRIPTION, buffer);
}

/**
 * Populates blessing logbook entries array for a player character.
 *
 * Returns the number of entries written to `logbook_entries`.
 *
 * 0x4C4200
 */
int bless_get_logbook_data(int64_t obj, BlessLogbookEntry* logbook_entries)
{
    int cnt;
    int index;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 0;
    }

    // Iterate through all blessing entries and populate the array.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_BLESSING_IDX);
    for (index = 0; index < cnt; index++) {
        logbook_entries[index].id = obj_arrayfield_uint32_get(obj, OBJ_F_PC_BLESSING_IDX, index);
        logbook_entries[index].datetime.value = obj_arrayfield_int64_get(obj, OBJ_F_PC_BLESSING_TS_IDX, index);
    }

    return cnt;
}

/**
 * Checks if the player character has a specific blessing.
 *
 * 0x4C4280
 */
bool bless_has(int64_t obj, int bless)
{
    int cnt;
    int index;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // Check blessings one by one.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_BLESSING_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_PC_BLESSING_IDX, index) == bless) {
            return true;
        }
    }

    return false;
}

/**
 * Adds a blessing to a player character.
 *
 * 0x4C42F0
 */
void bless_add(int64_t obj, int bless)
{
    int cnt;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    // Prevent adding duplicate blessings.
    if (bless_has(obj, bless)) {
        return;
    }

    // Append blessing ID and current time to the blessing field arrays.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_BLESSING_IDX);
    obj_arrayfield_uint32_set(obj, OBJ_F_PC_BLESSING_IDX, cnt, bless);
    obj_arrayfield_int64_set(obj, OBJ_F_PC_BLESSING_TS_IDX, cnt, sub_45A7C0().value);

    // Apply the associated effect.
    effect_add(obj, bless_get_effect(bless), EFFECT_CAUSE_BLESS);

    if (player_is_local_pc_obj(obj)) {
        mes_file_entry.num = 1000; // "You have received a Blessing!"
        mes_get_msg(bless_mes_file, &mes_file_entry);

        // Add UI message.
        ui_message.type = UI_MSG_TYPE_BLESS;
        ui_message.str = mes_file_entry.str;
        ui_message.field_8 = bless;
        ui_display_msg(&ui_message);

        // Highlight logbook button.
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
    }
}

/**
 * Retrieves the effect ID associated with a blessing.
 *
 * 0x4C4420
 */
int bless_get_effect(int bless)
{
    char buffer[80];
    bless_copy_field(bless, BLESS_F_EFFECT, buffer);
    return atoi(buffer);
}

/**
 * Removes a blessing from a player character.
 *
 * 0x4C4450
 */
void bless_remove(int64_t obj, int bless)
{
    int cnt;
    int index;
    int tmp_bless;
    int64_t tmp_ts;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_BLESSING_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_PC_BLESSING_IDX, index) == bless) {
            // Shift subsequent blessings up.
            while (index < cnt - 1) {
                tmp_bless = obj_arrayfield_uint32_get(obj, OBJ_F_PC_BLESSING_IDX, index + 1);
                tmp_ts = obj_arrayfield_int64_get(obj, OBJ_F_PC_BLESSING_TS_IDX, index + 1);
                obj_arrayfield_uint32_set(obj, OBJ_F_PC_BLESSING_IDX, index, tmp_bless);
                obj_arrayfield_int64_set(obj, OBJ_F_PC_BLESSING_TS_IDX, index, tmp_ts);
                index++;
            }

            // Decrease the length of the blessing arrays.
            obj_arrayfield_length_set(obj, OBJ_F_PC_BLESSING_IDX, cnt - 1);
            obj_arrayfield_length_set(obj, OBJ_F_PC_BLESSING_TS_IDX, cnt - 1);

            // Remove the effect associated with the blessing.
            effect_remove_one_typed(obj, bless_get_effect(bless));

            // Highlight logbook button if this is a local PC.
            if (player_is_local_pc_obj(obj)) {
                ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
            }
            break;
        }
    }
}
