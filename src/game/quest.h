#ifndef ARCANUM_GAME_QUEST_H_
#define ARCANUM_GAME_QUEST_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

typedef enum QuestState {
    QUEST_STATE_UNKNOWN,
    QUEST_STATE_MENTIONED,
    QUEST_STATE_ACCEPTED,
    QUEST_STATE_ACHIEVED,
    QUEST_STATE_COMPLETED,
    QUEST_STATE_OTHER_COMPLETED,
    QUEST_STATE_BOTCHED,
    QUEST_STATE_COUNT,
} QuestState;

typedef struct QuestLogbookEntry {
    /* 0000 */ int num;
    /* 0004 */ DateTime datetime;
    /* 0010 */ int state;
} QuestLogbookEntry;

typedef struct PcQuestState {
    /* 0000 */ DateTime datetime;
    /* 0008 */ int state;
    /* 000C */ int padding_C;
} PcQuestState;

// Serializeable.
static_assert(sizeof(PcQuestState) == 0x10, "wrong size");

bool quest_init(GameInitInfo* init_info);
void quest_reset(void);
void quest_exit(void);
bool quest_mod_load(void);
void quest_mod_unload(void);
bool quest_load(GameLoadInfo* load_info);
bool quest_save(TigFile* stream);
int quest_dialog_line(int64_t pc_obj, int64_t npc_obj, int num);
int quest_state_get(int64_t pc_obj, int num);
int quest_state_set(int64_t pc_obj, int num, int state, int64_t npc_obj);
int quest_unbotch(int64_t obj, int num);
int quest_global_state_get(int num);
int quest_global_state_set(int num, int state);
void quest_copy_description(int64_t obj, int quest_id, char* buffer);
int quest_get_logbook_data(int64_t obj, QuestLogbookEntry* logbook_entries);
int quest_get_xp(int xp_level);
bool quest_copy_accepted(int64_t src_obj, int64_t dst_obj);

#endif /* ARCANUM_GAME_QUEST_H_ */
