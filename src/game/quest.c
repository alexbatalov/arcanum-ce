#include "game/quest.h"

#include "game/critter.h"
#include "game/mes.h"
#include "game/object.h"
#include "game/party.h"
#include "game/player.h"
#include "game/reaction.h"
#include "game/snd.h"
#include "game/stat.h"
#include "game/ui.h"

#define MAX_QUEST 1000

#define QUEST_BOTCHED_MODIFIER 0x100

#define quest_num_to_idx(num) ((num) - 1000)

typedef struct Quest {
    int experience_level;
    int alignment_adjustment;
    int normal_dialog[QUEST_STATE_COUNT];
    int bad_reaction_dialog[QUEST_STATE_COUNT];
    int dumb_dialog[QUEST_STATE_COUNT];
    char* normal_description;
    char* dumb_description;
} Quest;

static bool quest_parse(const char* path, int start, int end);
static int quest_state_set_internal(int64_t pc_obj, int num, int state, int64_t npc_obj);
static int quest_logbook_entry_compare(const void* va, const void* vb);

/**
 * "gamequestlog.mes"
 *
 * 0x5B6E34
 */
static mes_file_handle_t quest_log_mes_file = MES_FILE_HANDLE_INVALID;

/**
 * "gamequestlogdumb.mes"
 *
 * 0x5B6E38
 */
static mes_file_handle_t quest_log_dumb_mes_file = MES_FILE_HANDLE_INVALID;

/**
 * "xp_quest.mes"
 *
 * 0x5FF414
 */
static mes_file_handle_t quest_xp_mes_file;

/**
 * Array storing the global state of each quest.
 *
 * The g-state uses reduced set of values from `QuestState` enum:
 *  - `QUEST_STATE_ACCEPTED`: this value is the default even if there is no PC
 *    who accepted the quest.
 *  - `QUEST_STATE_COMPLETED`: indicates that some PC have completed the quest.
 *  - `QUEST_STATE_BOTCHED`: indicates the quest can no longer be completed by
 *    by anyone.
 *
 * 0x5FF418
 */
static int* quest_gstate;

/**
 * Array of quests metadata.
 *
 * 0x5FF41C
 */
static Quest* quests;

/**
 * Called when the game is initialized.
 *
 * 0x4C45C0
 */
bool quest_init(GameInitInfo* init_info)
{
    (void)init_info;

    // Allocate memory for global quest states and quests meta.
    quest_gstate = (int*)CALLOC(MAX_QUEST, sizeof(*quest_gstate));
    quests = (Quest*)CALLOC(MAX_QUEST, sizeof(*quests));

    // Load xp rewards data from file (required).
    if (!mes_load("Rules\\xp_quest.mes", &quest_xp_mes_file)) {
        FREE(quest_gstate);
        FREE(quests);
        return false;
    }

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4C4620
 */
void quest_reset(void)
{
    int index;

    for (index = 0; index < MAX_QUEST; index++) {
        quest_gstate[index] = QUEST_STATE_ACCEPTED;
    }
}

/**
 * Called when the game shuts down.
 *
 * 0x4C4640
 */
void quest_exit(void)
{
    mes_unload(quest_xp_mes_file);
    FREE(quest_gstate);
    FREE(quests);
}

/**
 * Called when a module is being loaded.
 *
 * 0x4C4670
 */
bool quest_mod_load(void)
{
    int index;
    int state;
    MesFileEntry mes_file_entry;
    int num;

    // Initialize quests meta.
    for (index = 0; index < MAX_QUEST; index++) {
        for (state = 0; state < QUEST_STATE_COUNT; state++) {
            quests[index].normal_dialog[state] = -1;
            quests[index].bad_reaction_dialog[state] = -1;
            quests[index].dumb_dialog[state] = -1;
        }

        quests[index].experience_level = 1;
        quests[index].normal_description = NULL;
        quests[index].dumb_description = NULL;
    }

    // Parse quests meta from file, which is optional. If this file exists
    // proceed to loading logbook descriptions.
    if (quest_parse("rules\\gamequest.mes", 1000, 1999)) {
        // Load quest logbook descriptions (for characters with normal
        // intelligence).
        if (mes_load("mes\\gamequestlog.mes", &quest_log_mes_file)) {
            for (num = 1000; num < 2000; num++) {
                mes_file_entry.num = num;
                if (mes_search(quest_log_mes_file, &mes_file_entry)) {
                    quests[quest_num_to_idx(num)].normal_description = mes_file_entry.str;
                }
            }
        }

        // Load quest logbook descriptions (for characters with low
        // intelligence).
        if (mes_load("mes\\gamequestlogdumb.mes", &quest_log_dumb_mes_file)) {
            for (num = 1000; num < 2000; num++) {
                mes_file_entry.num = num;
                if (mes_search(quest_log_mes_file, &mes_file_entry)) {
                    quests[quest_num_to_idx(num)].dumb_description = mes_file_entry.str;
                }
            }
        }

        quest_reset();
    }

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4C47C0
 */
void quest_mod_unload(void)
{
    if (quest_log_mes_file != MES_FILE_HANDLE_INVALID) {
        mes_unload(quest_log_mes_file);
        quest_log_mes_file = MES_FILE_HANDLE_INVALID;
    }

    if (quest_log_dumb_mes_file != MES_FILE_HANDLE_INVALID) {
        mes_unload(quest_log_dumb_mes_file);
        quest_log_dumb_mes_file = MES_FILE_HANDLE_INVALID;
    }
}

/**
 * Called when the game is being loaded.
 *
 * 0x4C4800
 */
bool quest_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(quest_gstate, sizeof(*quest_gstate) * MAX_QUEST, 1, load_info->stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x4C4830
 */
bool quest_save(TigFile* stream)
{
    if (tig_file_fwrite(quest_gstate, sizeof(*quest_gstate) * MAX_QUEST, 1, stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Parses quest info from a message file from the specified range.
 *
 * 0x4C4860
 */
bool quest_parse(const char* path, int start, int end)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    int num;
    char temp[2000];
    Quest* quest;

    // Load the message file.
    if (!mes_load(path, &mes_file)) {
        return false;
    }

    // Parse each entry in the specified range.
    for (num = start; num <= end; num++) {
        mes_file_entry.num = num;

        if (mes_search(mes_file, &mes_file_entry)) {
            // NOTE: There is pretty good explanation of the format in the
            // WorldEd manual:
            //
            // "The first two numbers are experience level of the quest and how
            // it will affect the PC's alignment when completed.
            //
            // The next 21 numbers are used by the Q: generated dialog operator.
            // These numbers are arranged in three banks of seven numbers each.
            // The banks, in order, are normal dialog, bad reaction dialog (used
            // if the PC has a reaction of 20 or lower), and stupid PC dialog
            // (used if the PC has an intelligence of 4 or lower). Each of the
            // seven numbers in each bank is the dialog entry point for the
            // quest in each of its seven states. The number refers to a PC
            // dialog line which will be used in place of the Q: line based on
            // the associated quest state. You can use a –1 to indicate no
            // dialog line, meaning no message will be printed."
            strcpy(temp, mes_file_entry.str);

            quest = &(quests[quest_num_to_idx(num)]);
            quest->experience_level = atoi(strtok(temp, " "));
            quest->alignment_adjustment = atoi(strtok(NULL, " "));

            quest->normal_dialog[QUEST_STATE_UNKNOWN] = atoi(strtok(NULL, " "));
            quest->normal_dialog[QUEST_STATE_MENTIONED] = atoi(strtok(NULL, " "));
            quest->normal_dialog[QUEST_STATE_ACCEPTED] = atoi(strtok(NULL, " "));
            quest->normal_dialog[QUEST_STATE_ACHIEVED] = atoi(strtok(NULL, " "));
            quest->normal_dialog[QUEST_STATE_COMPLETED] = atoi(strtok(NULL, " "));
            quest->normal_dialog[QUEST_STATE_OTHER_COMPLETED] = atoi(strtok(NULL, " "));
            quest->normal_dialog[QUEST_STATE_BOTCHED] = atoi(strtok(NULL, " "));

            quest->bad_reaction_dialog[QUEST_STATE_UNKNOWN] = atoi(strtok(NULL, " "));
            quest->bad_reaction_dialog[QUEST_STATE_MENTIONED] = atoi(strtok(NULL, " "));
            quest->bad_reaction_dialog[QUEST_STATE_ACCEPTED] = atoi(strtok(NULL, " "));
            quest->bad_reaction_dialog[QUEST_STATE_ACHIEVED] = atoi(strtok(NULL, " "));
            quest->bad_reaction_dialog[QUEST_STATE_COMPLETED] = atoi(strtok(NULL, " "));
            quest->bad_reaction_dialog[QUEST_STATE_OTHER_COMPLETED] = atoi(strtok(NULL, " "));
            quest->bad_reaction_dialog[QUEST_STATE_BOTCHED] = atoi(strtok(NULL, " "));

            quest->dumb_dialog[QUEST_STATE_UNKNOWN] = atoi(strtok(NULL, " "));
            quest->dumb_dialog[QUEST_STATE_MENTIONED] = atoi(strtok(NULL, " "));
            quest->dumb_dialog[QUEST_STATE_ACCEPTED] = atoi(strtok(NULL, " "));
            quest->dumb_dialog[QUEST_STATE_ACHIEVED] = atoi(strtok(NULL, " "));
            quest->dumb_dialog[QUEST_STATE_COMPLETED] = atoi(strtok(NULL, " "));
            quest->dumb_dialog[QUEST_STATE_OTHER_COMPLETED] = atoi(strtok(NULL, " "));
            quest->dumb_dialog[QUEST_STATE_BOTCHED] = atoi(strtok(NULL, " "));
        }
    }

    mes_unload(mes_file);

    return true;
}

/**
 * Retrieves the dialog line for a quest.
 *
 * 0x4C4C00
 */
int quest_dialog_line(int64_t pc_obj, int64_t npc_obj, int num)
{
    int state;

    // Verify that the object is a PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 0;
    }

    state = quest_state_get(pc_obj, num);

    // Check if the PC has low intelligence.
    if (stat_level_get(pc_obj, STAT_INTELLIGENCE) <= LOW_INTELLIGENCE) {
        return quests[quest_num_to_idx(num)].dumb_dialog[state];
    }

    // Check if the PC has bad reaction.
    if (reaction_get(npc_obj, pc_obj) < 20) {
        return quests[quest_num_to_idx(num)].bad_reaction_dialog[state];
    }

    // Normal option.
    return quests[quest_num_to_idx(num)].normal_dialog[state];
}

/**
 * Retrieves the current state of a quest for a player character.
 *
 * 0x4C4CB0
 */
int quest_state_get(int64_t pc_obj, int num)
{
    PcQuestState pc_quest_state;
    int state;

    // Check if obj is not really a PC. In this case return global quest state.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return quest_global_state_get(num);
    }

    // Retrieve the PC's quest state.
    obj_arrayfield_pc_quest_get(pc_obj, OBJ_F_PC_QUEST_IDX, num, &pc_quest_state);

    state = pc_quest_state.state;

    // Check if quest is botched. If so, expose state as botched, rather than
    // the exact value.
    if ((state & QUEST_BOTCHED_MODIFIER) != 0) {
        state = QUEST_STATE_BOTCHED;
    }

    return state;
}

/**
 * Sets the quest state for a player character.
 *
 * 0x4C4D20
 */
int quest_state_set(int64_t pc_obj, int num, int state, int64_t npc_obj)
{
    int old_state;
    int64_t party_member_obj;
    int party_member_index;
    int64_t player_obj;
    int player;

    // Obtain the current quest state.
    old_state = quest_state_get(pc_obj, num);

    // Prevent changes if the quest is already completed or botched.
    if (old_state == QUEST_STATE_COMPLETED
        || old_state == QUEST_STATE_OTHER_COMPLETED
        || old_state == QUEST_STATE_BOTCHED) {
        return old_state;
    }

    // Prevent setting a state lower than the current one.
    if (old_state >= state) {
        return old_state;
    }

    // Award experience points if the quest is completed.
    if (state == QUEST_STATE_COMPLETED) {
        critter_give_xp(pc_obj, quest_get_xp(quests[quest_num_to_idx(num)].experience_level));
    }

    return quest_state_set_internal(pc_obj, num, state, npc_obj);
}

/**
 * Internal function to set a quest state and handle side effects.
 *
 * 0x4C4E60
 */
int quest_state_set_internal(int64_t pc_obj, int num, int state, int64_t npc_obj)
{
    int old_state;
    PcQuestState pc_quest_state;
    int reaction;

    // Verify the object is a player character.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return state;
    }

    old_state = quest_state_get(pc_obj, num);

    if (quest_gstate[quest_num_to_idx(num)] == QUEST_STATE_ACCEPTED) {
        // Update global quest state based on current state.
        if (state == QUEST_STATE_COMPLETED || state == QUEST_STATE_OTHER_COMPLETED) {
            quest_gstate[quest_num_to_idx(num)] = QUEST_STATE_COMPLETED;
        } else if (state == QUEST_STATE_BOTCHED) {
            quest_gstate[quest_num_to_idx(num)] = QUEST_STATE_BOTCHED;
        }
    } else {
        // Adjust state if the global state is already completed or botched.
        if (quest_gstate[quest_num_to_idx(num)] == QUEST_STATE_COMPLETED) {
            state = QUEST_STATE_OTHER_COMPLETED;
        } else {
            state = QUEST_STATE_BOTCHED;
        }
    }

    // Update the player's quest state.
    obj_arrayfield_pc_quest_get(pc_obj, OBJ_F_PC_QUEST_IDX, num, &pc_quest_state);
    if (state == QUEST_STATE_BOTCHED) {
        // Keep current quest state, but mark as it botched.
        pc_quest_state.state |= QUEST_BOTCHED_MODIFIER;
    } else {
        pc_quest_state.state = state;
    }

    pc_quest_state.datetime = sub_45A7C0();
    obj_arrayfield_pc_quest_set(pc_obj, OBJ_F_PC_QUEST_IDX, num, &pc_quest_state);

    // Apply alignment adjustment.
    if (state == QUEST_STATE_COMPLETED) {
        stat_base_set(pc_obj,
            STAT_ALIGNMENT,
            stat_base_get(pc_obj, STAT_ALIGNMENT) + quests[quest_num_to_idx(num)].alignment_adjustment);

        // Play quest completed sound.
        tig_sound_quick_play(SND_INTERFACE_QUESTCOMPLETE);
    }

    // Adjust NPC reaction based on quest state.
    if (npc_obj != OBJ_HANDLE_NULL) {
        if (state == QUEST_STATE_ACCEPTED) {
            // Quest accepted - immediately set reaction to be at least neutral.
            reaction = reaction_get(npc_obj, pc_obj);
            if (reaction < 41) {
                reaction_adj(npc_obj, pc_obj, 41 - reaction);
            }
        } else if (state == QUEST_STATE_COMPLETED) {
            // Quest completed - slightly improve NPC reaction.
            reaction_adj(npc_obj, pc_obj, 10);
        }
    }

    // Highlight logbook button if this is a local PC.
    if (player_is_local_pc_obj(pc_obj)) {
        if ((pc_quest_state.state & ~QUEST_BOTCHED_MODIFIER) != QUEST_STATE_UNKNOWN) {
            ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
        }
    }

    return state;
}

/**
 * Attempts to unbotch a quest.
 *
 * 0x4C5070
 */
int quest_unbotch(int64_t obj, int num)
{
    int state;
    PcQuestState pc_quest_state;

    // Check if the quest is botched globally.
    state = quest_global_state_get(num);
    if (state != QUEST_STATE_BOTCHED) {
        return state;
    }

    // Verify the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return state;
    }

    // Reset the global quest state ("accepted" is the default state).
    quest_gstate[quest_num_to_idx(num)] = QUEST_STATE_ACCEPTED;

    // Remove botched modifier from the player's quest state, effectively
    // returning quest to it's previous state.
    obj_arrayfield_pc_quest_get(obj, OBJ_F_PC_QUEST_IDX, num, &pc_quest_state);
    pc_quest_state.state &= ~QUEST_BOTCHED_MODIFIER;
    pc_quest_state.datetime = sub_45A7C0();
    obj_arrayfield_pc_quest_set(obj, OBJ_F_PC_QUEST_IDX, num, &pc_quest_state);

    if (player_is_local_pc_obj(obj)) {
        if (pc_quest_state.state == QUEST_STATE_COMPLETED) {
            // Play quest completed sound.
            tig_sound_quick_play(SND_INTERFACE_QUESTCOMPLETE);
        }

        if (pc_quest_state.state != QUEST_STATE_UNKNOWN) {
            ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, true);
        }
    }

    return pc_quest_state.state;
}

/**
 * Retrieves the global state of a quest.
 *
 * 0x4C51A0
 */
int quest_global_state_get(int num)
{
    return quest_gstate[quest_num_to_idx(num)];
}

/**
 * Sets the global state of a quest.
 *
 * 0x4C51C0
 */
int quest_global_state_set(int num, int state)
{
    int old_state;

    old_state = quest_gstate[quest_num_to_idx(num)];

    // Prevent changes if already completed or botched.
    if (old_state == QUEST_STATE_COMPLETED || old_state == QUEST_STATE_BOTCHED) {
        return old_state;
    }

    if (state == QUEST_STATE_COMPLETED || state == QUEST_STATE_BOTCHED) {
        quest_gstate[quest_num_to_idx(num)] = state;
    } else {
        state = QUEST_STATE_ACCEPTED;
    }

    return state;
}

/**
 * Retrieves the quest description to a buffer based on player intelligence.
 *
 * 0x4C5250
 */
void quest_copy_description(int64_t obj, int num, char* buffer)
{
    if (quests[quest_num_to_idx(num)].dumb_description != NULL
        && stat_level_get(obj, STAT_INTELLIGENCE) <= LOW_INTELLIGENCE) {
        strcpy(buffer, quests[quest_num_to_idx(num)].dumb_description);
    } else if (quests[quest_num_to_idx(num)].normal_description != NULL) {
        strcpy(buffer, quests[quest_num_to_idx(num)].normal_description);
    }
}

/**
 * Populates quest logbook entries array with quests known to a player
 * character. The resulting array is sorted by game time (earliest first).
 *
 * Returns the number of entries written to `logbook_entries`.
 *
 * 0x4C52E0
 */
int quest_get_logbook_data(int64_t obj, QuestLogbookEntry* logbook_entries)
{
    int index;
    PcQuestState pc_quests[2000];
    int cnt;

    // Verify the object is a player character.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 0;
    }

    // Copy all PC quest states.
    obj_arrayfield_pc_quest_copy_to_flat(obj, OBJ_F_PC_QUEST_IDX, 1999, pc_quests);

    // Collect active quests (non-unknown states).
    cnt = 0;
    for (index = 0; index < 2000; index++) {
        if ((pc_quests[index].state & ~QUEST_BOTCHED_MODIFIER) != QUEST_STATE_UNKNOWN) {
            logbook_entries[cnt].num = index;
            logbook_entries[cnt].datetime = pc_quests[index].datetime;
            if ((pc_quests[index].state & QUEST_BOTCHED_MODIFIER) != 0) {
                logbook_entries[cnt].state = QUEST_STATE_BOTCHED;
            } else {
                logbook_entries[cnt].state = pc_quests[index].state;
            }
            cnt++;
        }
    }

    // Sort entries by timestamp.
    qsort(logbook_entries, cnt, sizeof(*logbook_entries), quest_logbook_entry_compare);

    return cnt;
}

/**
 * Compares two quest logbook entries by timestamp.
 *
 * 0x4C53A0
 */
int quest_logbook_entry_compare(const void* va, const void* vb)
{
    const QuestLogbookEntry* a = (const QuestLogbookEntry*)va;
    const QuestLogbookEntry* b = (const QuestLogbookEntry*)vb;

    return datetime_compare(&(a->datetime), &(b->datetime));
}

/**
 * Retrieves the experience points for a quest level.
 *
 * 0x4C53C0
 */
int quest_get_xp(int xp_level)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = xp_level;
    if (mes_search(quest_xp_mes_file, &mes_file_entry)) {
        return atoi(mes_file_entry.str);
    } else {
        return 0;
    }
}

/**
 * Copies accepted quests from one player to another.
 *
 * 0x4C5400
 */
bool quest_copy_accepted(int64_t src_obj, int64_t dst_obj)
{
    PcQuestState pc_quests[2000];
    int index;
    int dst_state;
    int src_state;

    // Ensure both objects are player characters.
    if (obj_field_int32_get(src_obj, OBJ_F_TYPE) != OBJ_TYPE_PC
        || obj_field_int32_get(dst_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // Copy source player's quests.
    obj_arrayfield_pc_quest_copy_to_flat(src_obj, OBJ_F_PC_QUEST_IDX, 1999, pc_quests);

    for (index = 0; index < 2000; index++) {
        dst_state = quest_state_get(dst_obj, index);

        // Skip if quest is completed or botched.
        if (dst_state == QUEST_STATE_COMPLETED
            || dst_state == QUEST_STATE_OTHER_COMPLETED
            || dst_state == QUEST_STATE_BOTCHED) {
            continue;
        }

        src_state = pc_quests[index].state & ~QUEST_BOTCHED_MODIFIER;

        // Copy over if dst doesn't know about the quest, or heard something
        // about it (`QUEST_STATE_UNKNOWN` and `QUEST_STATE_MENTIONED`).
        if (dst_state < src_state && src_state == QUEST_STATE_ACCEPTED) {
            quest_state_set_internal(dst_obj, index, QUEST_STATE_ACCEPTED, OBJ_HANDLE_NULL);

            // Mark as botched if needed.
            if ((pc_quests[index].state & QUEST_BOTCHED_MODIFIER) != 0) {
                quest_state_set_internal(dst_obj, index, QUEST_STATE_BOTCHED, OBJ_HANDLE_NULL);
            }
        }
    }

    return true;
}
