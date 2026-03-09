#include "game/reputation.h"

#include <stdio.h>

#include "game/critter.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/player.h"
#include "game/random.h"
#include "game/reaction.h"
#include "game/stat.h"

/**
 * The maximum number of reputations the engine supports.
 */
#define MAX_REPUTATIONS 1000

/**
 * Converts a reputation index (0-based) to a reputation number (1000-based).
 */
#define rep_idx_to_num(idx) ((idx) + 1000)

/**
 * Converts a reputation number (1000-based) to a reputation index (0-based).
 */
#define rep_num_to_idx(num) ((num) - 1000)

typedef enum ReputationType {
    GAME_RP_NPC_M2M,
    GAME_RP_NPC_M2F,
    GAME_RP_NPC_F2F,
    GAME_RP_NPC_F2M,
    GAME_RP_COUNT,
} ReputationType;

typedef struct ReputationEffectInfo {
    /* 0000 */ int adj;
    /* 0004 */ int origin;
    /* 0008 */ int faction;
} ReputationEffectInfo;

typedef struct ReputationInfo {
    /* 0000 */ int num_effects;
    /* 0004 */ int factions[3];
    /* 0010 */ ReputationEffectInfo effects[5];
} ReputationInfo;

static bool reputation_parse(const char* path, int start, int end);

/**
 * Array of message file names for reputation-based greetings.
 *
 * 0x5B69C0
 */
static const char* off_5B69C0[GAME_RP_COUNT] = {
    /* GAME_RP_NPC_M2M */ "game_rp_npc_m2m",
    /* GAME_RP_NPC_M2F */ "game_rp_npc_m2f",
    /* GAME_RP_NPC_F2F */ "game_rp_npc_f2f",
    /* GAME_RP_NPC_F2M */ "game_rp_npc_f2m",
};

/**
 * "gamereplog.mes"
 *
 * 0x5FC8AC
 */
static mes_file_handle_t reputation_log_mes_file;

/**
 * Array of reputations metadata.
 *
 * 0x5FC8B0
 */
static ReputationInfo* reputations;

/**
 * Array of message file handles for reputation-based greetings.
 *
 * 0x5FC8B4
 */
static mes_file_handle_t* reputation_mes_files;

// 0x5FC8B8
static int dword_5FC8B8[1000];

/**
 * Called when the game is initialized.
 *
 * 0x4C15D0
 */
bool reputation_init(GameInitInfo* init_info)
{
    (void)init_info;

    reputations = (ReputationInfo*)CALLOC(MAX_REPUTATIONS, sizeof(*reputations));
    reputation_mes_files = (mes_file_handle_t*)CALLOC(GAME_RP_COUNT, sizeof(*reputation_mes_files));

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4C1600
 */
void reputation_exit(void)
{
    FREE(reputations);
    FREE(reputation_mes_files);
}

/**
 * Called when a module is being loaded.
 *
 * 0x4C1620
 */
bool reputation_mod_load(void)
{
    int index;
    char path[TIG_MAX_PATH];

    for (index = 0; index < GAME_RP_COUNT; index++) {
        reputation_mes_files[index] = MES_FILE_HANDLE_INVALID;
    }

    reputation_log_mes_file = MES_FILE_HANDLE_INVALID;

    // Parse reputations meta from file (optional).
    if (!reputation_parse("rules\\gamerep.mes", 1000, 1999)) {
        return true;
    }

    // Load reputation logbook descriptions.
    if (!mes_load("mes\\gamereplog.mes", &reputation_log_mes_file)) {
        return true;
    }

    // Load reputation-specific greetings.
    for (index = 0; index < GAME_RP_COUNT; index++) {
        sprintf(path, "mes\\%s.mes", off_5B69C0[index]);
        mes_load(path, &(reputation_mes_files[index]));
    }

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4C16E0
 */
void reputation_mod_unload(void)
{
    int index;

    for (index = 0; index < GAME_RP_COUNT; index++) {
        mes_unload(reputation_mes_files[index]);
        reputation_mes_files[index] = MES_FILE_HANDLE_INVALID;
    }

    mes_unload(reputation_log_mes_file);
    reputation_log_mes_file = MES_FILE_HANDLE_INVALID;
}

/**
 * Parses reputation info from a message file from the specified range.
 *
 * 0x4C1730
 */
bool reputation_parse(const char* path, int start, int end)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    int num;
    char str[MAX_STRING];
    char* tok;
    int effect;
    bool err;

    // Load the message file.
    if (!mes_load(path, &mes_file)) {
        return false;
    }

    // Parse each entry in the specified range.
    for (num = start; num <= end; num++) {
        mes_file_entry.num = num;

        if (mes_search(mes_file, &mes_file_entry)) {
            // NOTE: There is pretty good explanation of the format the
            // `gamerep.mes` itself (slightly formatted for readability):
            //
            // "Each reputation starts with 3 faction numbers, indicating the
            // three factions that will treat the reputation owner as being part
            // of their faction. One or all of these faction numbers can be
            // zero, which is the empty faction.
            //
            // Each reputation can then have up to 5 effects, separated by
            // commas. Each effect is 3 numbers, separated by spaces:
            //  1. Reaction adjustment.
            //  2. Origin (0 means ANY origin).
            //  3. Faction (0 means ANY faction)."
            strcpy(str, mes_file_entry.str);

            tok = strtok(str, ",");

            // Parse three faction numbers from the first token.
            err = true;
            do {
                if (tok == NULL) break;

                reputations[rep_num_to_idx(num)].factions[0] = atoi(tok);
                while (SDL_isspace(*tok)) {
                    tok++;
                }

                while (SDL_isdigit(*tok)) {
                    tok++;
                }

                if (*tok == '\0') {
                    break;
                }

                reputations[rep_num_to_idx(num)].factions[1] = atoi(tok);
                while (SDL_isspace(*tok)) {
                    tok++;
                }

                while (SDL_isdigit(*tok)) {
                    tok++;
                }

                if (*tok == '\0') {
                    break;
                }

                reputations[rep_num_to_idx(num)].factions[2] = atoi(tok);

                err = false;
            } while (0);

            if (err) {
                tig_debug_printf("Warning! Reputation %d is missing required faction values!\n", num);

                // FIX: Release mes file.
                mes_unload(mes_file);

                return false;
            }

            tok = strtok(NULL, ",");

            // NOTE: Original code is different, but the same thing.
            for (effect = 0; effect < 5; effect++) {
                if (tok == NULL) {
                    break;
                }

                err = true;
                do {
                    reputations[rep_num_to_idx(num)].effects[effect].adj = atoi(tok);
                    while (SDL_isspace(*tok)) {
                        tok++;
                    }

                    while (SDL_isdigit(*tok)) {
                        tok++;
                    }

                    if (*tok == '\0') {
                        break;
                    }

                    reputations[rep_num_to_idx(num)].effects[effect].origin = atoi(tok);
                    while (SDL_isspace(*tok)) {
                        tok++;
                    }

                    while (SDL_isdigit(*tok)) {
                        tok++;
                    }

                    if (*tok == '\0') {
                        break;
                    }

                    reputations[rep_num_to_idx(num)].effects[effect].faction = atoi(tok);

                    err = false;
                } while (0);

                if (err) {
                    tig_debug_printf("Warning! Reputation %d has effect with less than 3 parameters\n", num);
                }

                tok = strtok(NULL, ",");
            }

            reputations[rep_num_to_idx(num)].num_effects = effect;

            // Check if something else is left.
            if (effect == 5 && tok != NULL) {
                tig_debug_printf("Warning! Too many reputation effects for reputation %d\n", num);
            }
        }
    }

    mes_unload(mes_file);
    return true;
}

/**
 * Calculates the reaction adjustments for an NPC based PC's reputation.
 *
 * 0x4C1AC0
 */
int reputation_reaction_adj(int64_t pc_obj, int64_t npc_obj)
{
    int adj = 0;
    int cnt;
    int idx;
    int i;
    int j;
    int origin;
    int faction;
    ReputationEffectInfo* effect;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 0;
    }

    // Make sure `npc_obj` is NPC.
    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return 0;
    }

    cnt = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_REPUTATION_IDX);
    if (cnt > 0) {
        origin = critter_origin_get(npc_obj);
        faction = critter_faction_get(npc_obj);

        // Iterate through each reputation the PC has.
        for (i = 0; i < cnt; i++) {
            idx = rep_num_to_idx(obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_REPUTATION_IDX, i));

            // Apply each effect that matches the NPC's origin or faction.
            for (j = 0; j < reputations[idx].num_effects; j++) {
                effect = &(reputations[idx].effects[j]);
                if ((effect->origin == 0 || effect->origin == origin)
                    && (effect->faction == 0 || effect->faction == faction)) {
                    adj += effect->adj;
                }
            }
        }
    }

    return adj;
}

/**
 * Retrieves the name of a reputation for display in the logbook.
 *
 * 0x4C1BD0
 */
void reputation_name(int reputation, char* buffer)
{
    MesFileEntry mes_file_entry;

    buffer[0] = '\0';

    if (reputation >= 1000) {
        mes_file_entry.num = reputation;
        if (mes_search(reputation_log_mes_file, &mes_file_entry)) {
            strcpy(buffer, mes_file_entry.str);
        }
    }
}

/**
 * Populates reputation logbook entries array with player character's
 * reputations. The resulting array is sorted by game time (earliest first).
 *
 * Returns the number of entries written to `logbook_entries`.
 *
 * 0x4C1C30
 */
int reputation_get_logbook_data(int64_t pc_obj, ReputationLogbookEntry* logbook_entries)
{
    int cnt;
    int index;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 0;
    }

    // Collect available reputations.
    cnt = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_REPUTATION_IDX);
    for (index = 0; index < cnt; index++) {
        logbook_entries[index].reputation = obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_REPUTATION_IDX, index);
        logbook_entries[index].datetime.value = obj_arrayfield_int64_get(pc_obj, OBJ_F_PC_REPUTATION_TS_IDX, index);
    }

    return cnt;
}

/**
 * Checks if a player character has a specific reputation.
 *
 * 0x4C1CB0
 */
bool reputation_has(int64_t pc_obj, int reputation)
{
    int cnt;
    int index;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // Search for the reputation in the PC's reputation list.
    cnt = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_REPUTATION_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_REPUTATION_IDX, index) == reputation) {
            return true;
        }
    }

    return false;
}

/**
 * Adds a reputation to a player character.
 *
 * 0x4C1D20
 */
void reputation_add(int64_t pc_obj, int reputation)
{
    int index;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    // Check if the reputation is already present.
    if (reputation_has(pc_obj, reputation)) {
        return;
    }

    if (!multiplayer_is_locked()) {
        PacketChangeReputation pkt;

        // Only host can send reputation changes.
        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 105;
        pkt.pc_oid = obj_get_id(pc_obj);
        pkt.reputation = reputation;
        pkt.action = CHANGE_REPUTATION_ACTION_ADD;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    // Append reputation and current time to the reputation field arrays.
    index = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_REPUTATION_IDX);
    obj_arrayfield_uint32_set(pc_obj, OBJ_F_PC_REPUTATION_IDX, index, reputation);
    obj_arrayfield_int64_set(pc_obj, OBJ_F_PC_REPUTATION_TS_IDX, index, sub_45A7C0().value);

    // Highlight logbook button if this is a local PC.
    if (player_is_local_pc_obj(pc_obj)) {
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
    }
}

/**
 * Removes a reputation from a player character.
 *
 * 0x4C1E10
 */
void reputation_remove(int64_t pc_obj, int reputation)
{
    int cnt;
    int index;
    int tmp_rep;
    uint64_t tmp_ts;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    if (!multiplayer_is_locked()) {
        PacketChangeReputation pkt;

        // Only host can send reputation changes.
        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 105;
        pkt.pc_oid = obj_get_id(pc_obj);
        pkt.reputation = reputation;
        pkt.action = CHANGE_REPUTATION_ACTION_REMOVE;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    // Find index of the reputation in the reputation array.
    cnt = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_REPUTATION_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_REPUTATION_IDX, index) == reputation) {
            break;
        }
    }

    if (index >= cnt) {
        // Index not found, attempt to remove reputation which is not added.
        return;
    }

    // Shift subsequent reputations up.
    while (index < cnt - 1) {
        tmp_rep = obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_REPUTATION_IDX, index + 1);
        tmp_ts = obj_arrayfield_int64_get(pc_obj, OBJ_F_PC_REPUTATION_TS_IDX, index + 1);
        obj_arrayfield_uint32_set(pc_obj, OBJ_F_PC_REPUTATION_IDX, index, tmp_rep);
        obj_arrayfield_int64_set(pc_obj, OBJ_F_PC_REPUTATION_TS_IDX, index, tmp_ts);
        index++;
    }

    // Decrease the length of the blessing arrays.
    obj_arrayfield_length_set(pc_obj, OBJ_F_PC_REPUTATION_IDX, cnt - 1);
    obj_arrayfield_length_set(pc_obj, OBJ_F_PC_REPUTATION_TS_IDX, cnt - 1);

    // Highlight logbook button if this is a local PC.
    if (player_is_local_pc_obj(pc_obj)) {
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
    }
}

/**
 * Selects a random reputation number for a greeting.
 *
 * Returns `0` if no reputation has been picked.
 *
 * 0x4C1F80
 */
int reputation_pick(int64_t pc_obj, int64_t npc_obj)
{
    int cnt;
    int idx;
    int origin;
    int faction;
    int reaction_level;
    int reaction_type;
    int i;
    int j;
    ReputationEffectInfo* effect;
    int selected_cnt;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 0;
    }

    // Make sure `npc_obj` is NPC.
    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return 0;
    }

    cnt = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_REPUTATION_IDX);
    if (cnt <= 0) {
        return 0;
    }

    // Retrieve NPC data.
    origin = critter_origin_get(npc_obj);
    faction = critter_faction_get(npc_obj);
    reaction_level = reaction_get(npc_obj, pc_obj);
    reaction_type = reaction_translate(reaction_level);

    selected_cnt = 0;

    // Iterate through PC's reputations to find those affecting the NPC.
    for (idx = 0; idx < cnt; idx++) {
        i = rep_num_to_idx(obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_REPUTATION_IDX, idx));
        for (j = 0; j < reputations[i].num_effects; j++) {
            effect = &(reputations[i].effects[j]);

            // Check if the effect applies to the NPC's origin and faction.
            if ((effect->origin == 0 || effect->origin == origin)
                && (effect->faction == 0 || effect->faction == faction)) {
                // Select reputation if the adjustment matches current
                // reaction level (negative adjustment - suspicious, dislike,
                // hatred, positive adjustment - love, amiable, courteous).
                if ((effect->adj < 0 && reaction_type > REACTION_NEUTRAL)
                    || (effect->adj > 0 && reaction_type < REACTION_NEUTRAL)) {
                    dword_5FC8B8[selected_cnt++] = idx;
                    break;
                }
            }
        }
    }

    // Randomly select one of the eligible reputations.
    return selected_cnt > 0
        ? rep_idx_to_num(dword_5FC8B8[random_between(0, selected_cnt - 1)])
        : 0;
}

/**
 * Copies a greeting message for a reputation based on NPC and PC genders.
 *
 * 0x4C2100
 */
void reputation_copy_greeting(int64_t pc_obj, int64_t npc_obj, int reputation, char* buffer)
{
    int interaction_type;
    MesFileEntry mes_file_entry;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        buffer[0] = '\0';
        return;
    }

    // Make sure `npc_obj` is NPC.
    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        buffer[0] = '\0';
        return;
    }

    // Determine interaction type based on NPC and PC gender.
    if (stat_level_get(npc_obj, STAT_GENDER) == GENDER_MALE) {
        interaction_type = stat_level_get(pc_obj, STAT_GENDER) == GENDER_MALE
            ? GAME_RP_NPC_M2M
            : GAME_RP_NPC_M2F;
    } else {
        interaction_type = stat_level_get(pc_obj, STAT_GENDER) == GENDER_MALE
            ? GAME_RP_NPC_F2M
            : GAME_RP_NPC_F2F;
    }

    mes_file_entry.num = reputation;
    if (!mes_search(reputation_mes_files[interaction_type], &mes_file_entry)) {
        buffer[0] = '\0';
        return;
    }

    strcpy(buffer, mes_file_entry.str);
}

/**
 * Checks if a player character has a reputation associated with a specific
 * faction.
 *
 * 0x4C21E0
 */
bool reputation_check_faction(int64_t pc_obj, int faction)
{
    int cnt;
    int idx;
    int i;
    int j;

    if (faction == 0) {
        return false;
    }

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // Iterate through PC's reputations to find matching factions.
    cnt = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_REPUTATION_IDX);
    for (idx = 0; idx < cnt; idx++) {
        i = rep_num_to_idx(obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_REPUTATION_IDX, idx));
        for (j = 0; j < 3; j++) {
            if (reputations[i].factions[j] == faction) {
                return true;
            }
        }
    }

    return false;
}
