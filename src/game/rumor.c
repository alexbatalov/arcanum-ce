#include "game/rumor.h"

#include <stdio.h>

#include "game/critter.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/stat.h"

/**
 * Size of the rumor state bit array.
 */
#define RUMOR_QSTATE_SIZE ((MAX_RUMORS / 8) + 1)

/**
 * Converts a rumor index (0-based) to a rumor number (1000-based).
 */
#define rumor_idx_to_num(idx) ((idx) + 1000)

/**
 * Converts a rumor number (1000-based) to a rumor index (0-based).
 */
#define rumor_num_to_idx(num) ((num) - 1000)

/**
 * Describes interaction types for NPC dialog response.
 *
 * Each interaction type have a corresponding `.mes` file which defines NPC
 * response when a PC asks about a rumor.
 *
 * The `RUMOR_INTERACTION_TYPE_DUMB` is a special case - it's not really an
 * interaction type. Instead it's used to obtain logbook description for player
 * characters with low intelligence (while `RUMOR_INTERACTION_TYPE_MALE_TO_MALE`
 * is used otherwise).
 */
typedef enum RumorInteractionType {
    RUMOR_INTERACTION_TYPE_MALE_TO_MALE,
    RUMOR_INTERACTION_TYPE_MALE_TO_FEMALE,
    RUMOR_INTERACTION_TYPE_FEMALE_TO_FEMALE,
    RUMOR_INTERACTION_TYPE_FEMALE_TO_MALE,
    RUMOR_INTERACTION_TYPE_DUMB,
    RUMOR_INTERACTION_TYPE_COUNT,
} RumorInteractionType;

static void rumor_set_known_internal(int64_t obj, int rumor, int64_t timestamp);
static int rumor_logbook_entry_compare(const void* va, const void* vb);

/**
 * Array of message file names for different rumor interaction types.
 *
 * Used to load dialog based on NPC and PC gender. The special DUMB option is
 * used when a PC have low intelligence.
 *
 * 0x5B6E98
 */
static const char* rumor_mes_file_names[RUMOR_INTERACTION_TYPE_COUNT] = {
    /*     RUMOR_INTERACTION_TYPE_MALE_TO_MALE */ "game_rd_npc_m2m",
    /*   RUMOR_INTERACTION_TYPE_MALE_TO_FEMALE */ "game_rd_npc_m2f",
    /* RUMOR_INTERACTION_TYPE_FEMALE_TO_FEMALE */ "game_rd_npc_f2f",
    /*   RUMOR_INTERACTION_TYPE_FEMALE_TO_MALE */ "game_rd_npc_f2m",
    /*             RUMOR_INTERACTION_TYPE_DUMB */ "game_rd_npc_m2m_dumb",
};

/**
 * Array of message file handles for rumor dialog files.
 *
 * 0x5FF420
 */
static mes_file_handle_t* rumor_mes_files;

/**
 * Bit array storing the quelled state of each rumor.
 *
 * 0x6876C8
 */
static uint8_t* rumor_qstate;

/**
 * Called when the game is initialized.
 *
 * 0x4C5500
 */
bool rumor_init(GameInitInfo* init_info)
{
    (void)init_info;

    rumor_qstate = (uint8_t*)CALLOC(RUMOR_QSTATE_SIZE, sizeof(*rumor_qstate));
    rumor_mes_files = (mes_file_handle_t*)CALLOC(RUMOR_INTERACTION_TYPE_COUNT, sizeof(*rumor_mes_files));

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4C5530
 */
void rumor_reset(void)
{
    int index;

    // Clear the rumor q-state bit array.
    for (index = 0; index < RUMOR_QSTATE_SIZE; index++) {
        rumor_qstate[index] = 0;
    }
}

/**
 * Called when the game shuts down.
 *
 * 0x4C5550
 */
void rumor_exit(void)
{
    FREE(rumor_qstate);
    FREE(rumor_mes_files);
}

/**
 * Called when a module is being loaded.
 *
 * 0x4C5570
 */
bool rumor_mod_load(void)
{
    int index;
    char path[TIG_MAX_PATH];

    // Load message files for each interaction type.
    for (index = 0; index < RUMOR_INTERACTION_TYPE_COUNT; index++) {
        sprintf(path, "mes\\%s.mes", rumor_mes_file_names[index]);
        mes_load(path, &(rumor_mes_files[index]));
    }

    // Reset rumor q-state.
    rumor_reset();

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4C55C0
 */
void rumor_mod_unload(void)
{
    int index;

    // Unload each message file and reset its handle.
    for (index = 0; index < RUMOR_INTERACTION_TYPE_COUNT; index++) {
        mes_unload(rumor_mes_files[index]);
        rumor_mes_files[index] = MES_FILE_HANDLE_INVALID;
    }
}

/**
 * Called when the game is being loaded.
 *
 * 0x4C55F0
 */
bool rumor_load(GameLoadInfo* load_info)
{
    // Read rumor q-state bit array from the file stream.
    if (tig_file_fread(rumor_qstate, RUMOR_QSTATE_SIZE, 1, load_info->stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x4C5620
 */
bool rumor_save(TigFile* stream)
{
    // Write rumor q-state bit array to the file stream.
    if (tig_file_fwrite(rumor_qstate, RUMOR_QSTATE_SIZE, 1, stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Checks if a rumor is quelled.
 *
 * 0x4C5650
 */
bool rumor_qstate_get(int rumor)
{
    int pos = rumor_num_to_idx(rumor) / 8;
    int bit = rumor_num_to_idx(rumor) % 8;
    return ((rumor_qstate[pos] >> bit) & 1) != 0;
}

/**
 * Marks a rumor as quelled.
 *
 * NOTE: There is no way to reset "quelled" state of a particular rumor.
 *
 * 0x4C5690
 */
void rumor_qstate_set(int rumor)
{
    int pos;
    int bit;

    if (!multiplayer_is_locked()) {
        PacketRumorQStateSet pkt;

        // Only host can send q-state change.
        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 37;
        pkt.rumor = rumor;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    pos = rumor_num_to_idx(rumor) / 8;
    bit = rumor_num_to_idx(rumor) % 8;
    rumor_qstate[pos] |= 1 << bit;
}

/**
 * Retrieves the NPC dialog response for the specified rumor.
 *
 * The phrase is based on PC gender, NPC gender, and NPC social class (if it's
 * present).
 *
 * 0x4C5700
 */
void rumor_copy_interaction_str(int64_t pc_obj, int64_t npc_obj, int rumor, char* buffer)
{
    int interaction_type;
    MesFileEntry mes_file_entry;

    // Determine interaction type based on NPC and PC gender.
    if (stat_level_get(npc_obj, STAT_GENDER) == GENDER_MALE) {
        if (stat_level_get(pc_obj, STAT_GENDER) == GENDER_MALE) {
            interaction_type = RUMOR_INTERACTION_TYPE_MALE_TO_MALE;
        } else {
            interaction_type = RUMOR_INTERACTION_TYPE_MALE_TO_FEMALE;
        }
    } else {
        if (stat_level_get(pc_obj, STAT_GENDER) == GENDER_MALE) {
            interaction_type = RUMOR_INTERACTION_TYPE_FEMALE_TO_MALE;
        } else {
            interaction_type = RUMOR_INTERACTION_TYPE_FEMALE_TO_FEMALE;
        }
    }

    // Try to find a social-class-specific entry.
    mes_file_entry.num = critter_social_class_get(npc_obj) + 20 * rumor + 1;
    if (!mes_search(rumor_mes_files[interaction_type], &mes_file_entry)) {
        // Fall back to the default entry.
        mes_file_entry.num = 20 * rumor + 1;
        if (!mes_search(rumor_mes_files[interaction_type], &mes_file_entry)) {
            // If no entry is found, return an empty string.
            buffer[0] = '\0';
            return;
        }
    }

    // Copy the dialog string to the specified buffer.
    strcpy(buffer, mes_file_entry.str);
}

/**
 * Marks a rumor as known to a PC.
 *
 * 0x4C57E0
 */
void rumor_known_set(int64_t obj, int rumor)
{
    DateTime datetime;

    // Get the current game time.
    datetime = sub_45A7C0();

    if (!multiplayer_is_locked()) {
        PacketRumorKnownSet pkt;

        // Only host can send known rumor changes.
        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 38;
        pkt.rumor = rumor;
        pkt.oid = obj_get_id(obj);
        pkt.datetime = datetime;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    // Ensure the rumor is not already known.
    if (rumor_known_get(obj, rumor)) {
        return;
    }

    rumor_set_known_internal(obj, rumor, datetime.value);
}

/**
 * Internal helper thats sets a rumor as known for a player character.
 *
 * 0x4C58A0
 */
void rumor_set_known_internal(int64_t obj, int rumor, int64_t timestamp)
{
    obj_arrayfield_int64_set(obj, OBJ_F_PC_RUMOR_IDX, rumor_num_to_idx(rumor), timestamp);
}

/**
 * Checks if a rumor is known by a player character.
 *
 * 0x4C58D0
 */
bool rumor_known_get(int64_t obj, int rumor)
{
    // Ensure the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // If rumor is known it would have a non-zero timestamp.
    if (obj_arrayfield_int64_get(obj, OBJ_F_PC_RUMOR_IDX, rumor_num_to_idx(rumor)) == 0) {
        return false;
    }

    return true;
}

/**
 * Retrieves the specified rumor's logbook description.
 *
 * The resulting text is based on PC's intelligence.
 *
 * 0x4C5920
 */
void rumor_copy_logbook_str(int64_t obj, int rumor, char* buffer)
{
    if (stat_level_get(obj, STAT_INTELLIGENCE) > LOW_INTELLIGENCE) {
        rumor_copy_logbook_normal_str(rumor, buffer);
    } else {
        rumor_copy_logbook_dumb_str(rumor, buffer);
    }
}

/**
 * Retrieves the specified rumor's logbook description as seen by normal
 * intelligence PC.
 *
 * 0x4C5960
 */
void rumor_copy_logbook_normal_str(int rumor, char* buffer)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = 20 * rumor;
    if (!mes_search(rumor_mes_files[RUMOR_INTERACTION_TYPE_MALE_TO_MALE], &mes_file_entry)) {
        buffer[0] = '\0';
        return;
    }

    strcpy(buffer, mes_file_entry.str);
}

/**
 * Retrieves the specified rumors' logbook description as seen by low
 * intelligence PC.
 *
 * 0x4C59D0
 */
void rumor_copy_logbook_dumb_str(int rumor, char* buffer)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = 20 * rumor;
    if (!mes_search(rumor_mes_files[RUMOR_INTERACTION_TYPE_DUMB], &mes_file_entry)) {
        buffer[0] = '\0';
        return;
    }

    strcpy(buffer, mes_file_entry.str);
}

/**
 * Populates rumor logbook entries array known by a player character. The
 * resulting array is sorted by game time (earliest first).
 *
 * Returns the number of entries written to `logbook_entries`.
 *
 * 0x4C5A40
 */
int rumor_get_logbook_data(int64_t obj, RumorLogbookEntry* logbook_entries)
{
    int index;
    uint64_t timestamps[MAX_RUMORS];
    int cnt;

    // Copy all rumor timestamps for the player.
    obj_arrayfield_pc_rumor_copy_to_flat(obj, OBJ_F_PC_RUMOR_IDX, MAX_RUMORS - 1, timestamps);

    // Collect known rumors (non-zero timestamps).
    cnt = 0;
    for (index = 0; index < MAX_RUMORS; index++) {
        if (timestamps[index] != 0) {
            logbook_entries[cnt].num = rumor_idx_to_num(index);
            logbook_entries[cnt].datetime.value = timestamps[index];
            logbook_entries[cnt].quelled = rumor_qstate_get(rumor_idx_to_num(index));
            cnt++;
        }
    }

    // Sort entries by timestamp.
    qsort(logbook_entries, cnt, sizeof(*logbook_entries), rumor_logbook_entry_compare);

    return cnt;
}

/**
 * Compares two rumor logbook entries by timestamp.
 *
 * 0x4C5AF0
 */
int rumor_logbook_entry_compare(const void* va, const void* vb)
{
    const RumorLogbookEntry* a = (const RumorLogbookEntry*)va;
    const RumorLogbookEntry* b = (const RumorLogbookEntry*)vb;

    return datetime_compare(&(a->datetime), &(b->datetime));
}

/**
 * Copies known rumors from one player to another.
 *
 * 0x4C5B10
 */
bool rumor_copy_known(int64_t src_obj, int64_t dst_obj)
{
    int index;
    uint64_t timestamps[MAX_RUMORS];

    // Ensure both objects are player characters.
    if (obj_field_int32_get(src_obj, OBJ_F_TYPE) != OBJ_TYPE_PC
        || obj_field_int32_get(dst_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // Copy source player's rumor timestamps.
    obj_arrayfield_pc_rumor_copy_to_flat(src_obj, OBJ_F_PC_RUMOR_IDX, MAX_RUMORS - 1, timestamps);

    // Set known rumors (with non-zero timestamps) for the destination player.
    for (index = 0; index < MAX_RUMORS; index++) {
        if (timestamps[index] != 0) {
            rumor_known_set(dst_obj, index);
        }
    }

    return true;
}
