#include "game/curse.h"

#include "game/effect.h"
#include "game/mes.h"
#include "game/object.h"
#include "game/player.h"
#include "game/timeevent.h"
#include "game/ui.h"

#define CURSE_F_NAME 0
#define CURSE_F_EFFECT 1
#define CURSE_F_DESCRIPTION 2

static void curse_copy_field(int curse, int field, char* buffer);

/**
 * "gamecurse.mes"
 *
 * 0x5FF40C
 */
static int curse_mes_file;

/**
 * Called when a module is being loaded.
 *
 * 0x4C3C60
 */
bool curse_mod_load(void)
{
    mes_load("mes\\gamecurse.mes", &curse_mes_file);
    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4C3C80
 */
void curse_mod_unload(void)
{
    mes_unload(curse_mes_file);
    curse_mes_file = MES_FILE_HANDLE_INVALID;
}

/**
 * Copies the name of a curse into the provided buffer.
 *
 * 0x4C3CA0
 */
void curse_copy_name(int curse, char* buffer)
{
    curse_copy_field(curse, CURSE_F_NAME, buffer);
}

/**
 * Internal helper that copies a field from the message file for the given
 * curse.
 *
 * 0x4C3CC0
 */
void curse_copy_field(int curse, int field, char* buffer)
{
    MesFileEntry mes_file_entry;

    // Clear output buffer.
    buffer[0] = '\0';

    // NOTE: Curse IDs starts at 50 for the same reasons as mentioned in
    // `bless.c`.
    if (curse < 50) {
        return;
    }

    // Curses data comes in a banks of 10.
    mes_file_entry.num = curse * 10 + field;
    if (mes_search(curse_mes_file, &mes_file_entry)) {
        strcpy(buffer, mes_file_entry.str);
    }
}

/**
 * Copies the description of a curse into the provided buffer.
 *
 * 0x4C3D30
 */
void curse_copy_description(int curse, char* buffer)
{
    curse_copy_field(curse, CURSE_F_DESCRIPTION, buffer);
}

/**
 * Populates curse logbook entries array for a player character.
 *
 * Returns the number of entries written to `logbook_entries`.
 *
 * 0x4C3D50
 */
int curse_get_logbook_data(int64_t obj, CurseLogbookEntry* logbook_entries)
{
    int cnt;
    int index;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 0;
    }

    // Iterate through all curse entries and populate the array.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_CURSE_IDX);
    for (index = 0; index < cnt; index++) {
        logbook_entries[index].id = obj_arrayfield_uint32_get(obj, OBJ_F_PC_CURSE_IDX, index);
        logbook_entries[index].datetime.value = obj_arrayfield_int64_get(obj, OBJ_F_PC_CURSE_TS_IDX, index);
    }

    return cnt;
}

/**
 * Checks if the player character has a specific curse.
 *
 * 0x4C3DD0
 */
bool curse_has(int64_t obj, int curse)
{
    int cnt;
    int index;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // Check curses one by one.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_CURSE_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_PC_CURSE_IDX, index) == curse) {
            return true;
        }
    }

    return false;
}

/**
 * Adds a curse to a player character.
 *
 * 0x4C3E40
 */
void curse_add(int64_t obj, int curse)
{
    int cnt;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    // Prevent adding duplicate curses.
    if (curse_has(obj, curse)) {
        return;
    }

    // Append curse ID and current time to the curse field arrays.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_CURSE_IDX);
    obj_arrayfield_uint32_set(obj, OBJ_F_PC_CURSE_IDX, cnt, curse);
    obj_arrayfield_int64_set(obj, OBJ_F_PC_CURSE_TS_IDX, cnt, sub_45A7C0().value);

    // Apply the associated effect.
    effect_add(obj, curse_get_effect(curse), EFFECT_CAUSE_CURSE);

    if (player_is_local_pc_obj(obj)) {
        mes_file_entry.num = 1000; // "You have received a curse!"
        mes_get_msg(curse_mes_file, &mes_file_entry);

        // Add UI message.
        ui_message.type = UI_MSG_TYPE_CURSE;
        ui_message.str = mes_file_entry.str;
        ui_message.field_8 = curse;
        sub_460630(&ui_message);

        // Highlight logbook button.
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
    }
}

/**
 * Retrieves the effect ID associated with a curse.
 *
 * 0x4C3F70
 */
int curse_get_effect(int curse)
{
    char buffer[80];
    curse_copy_field(curse, CURSE_F_EFFECT, buffer);
    return atoi(buffer);
}

/**
 * Removes a curse from a player character.
 *
 * 0x4C3FA0
 */
void curse_remove(int64_t obj, int curse)
{
    int cnt;
    int index;
    int tmp_curse;
    int64_t tmp_ts;

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    cnt = obj_arrayfield_length_get(obj, OBJ_F_PC_CURSE_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_PC_CURSE_IDX, index) == curse) {
            // Shift subsequent curses up.
            while (index < cnt - 1) {
                tmp_curse = obj_arrayfield_uint32_get(obj, OBJ_F_PC_CURSE_IDX, index + 1);
                tmp_ts = obj_arrayfield_int64_get(obj, OBJ_F_PC_CURSE_TS_IDX, index + 1);
                obj_arrayfield_uint32_set(obj, OBJ_F_PC_CURSE_IDX, index, tmp_curse);
                obj_arrayfield_int64_set(obj, OBJ_F_PC_CURSE_TS_IDX, index, tmp_ts);
                index++;
            }

            // Decrease the length of the curse arrays.
            obj_arrayfield_length_set(obj, OBJ_F_PC_CURSE_IDX, cnt - 1);
            obj_arrayfield_length_set(obj, OBJ_F_PC_CURSE_TS_IDX, cnt - 1);

            // Remove the effect associated with the curse.
            effect_remove_one_typed(obj, curse_get_effect(curse));

            // Highlight logbook button if this is a local PC.
            if (player_is_local_pc_obj(obj)) {
                ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
            }
            break;
        }
    }
}
