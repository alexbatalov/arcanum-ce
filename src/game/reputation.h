#ifndef ARCANUM_GAME_REPUTATION_H_
#define ARCANUM_GAME_REPUTATION_H_

#include "game/context.h"
#include "game/timeevent.h"

typedef struct ReputationLogbookEntry {
    /* 0000 */ int reputation;
    /* 0008 */ DateTime datetime;
} ReputationLogbookEntry;

bool reputation_init(GameInitInfo* init_info);
void reputation_exit(void);
bool reputation_mod_load(void);
void reputation_mod_unload(void);
int reputation_reaction_adj(int64_t pc_obj, int64_t npc_obj);
void reputation_name(int reputation, char* buffer);
int reputation_get_logbook_data(int64_t pc_obj, ReputationLogbookEntry* logbook_entries);
bool reputation_has(int64_t pc_obj, int reputation);
void reputation_add(int64_t pc_obj, int reputation);
void reputation_remove(int64_t pc_obj, int reputation);
int reputation_pick(int64_t pc_obj, int64_t npc_obj);
void reputation_copy_greeting(int64_t pc_obj, int64_t npc_obj, int reputation, char* buffer);
bool reputation_check_faction(int64_t pc_obj, int faction);

#endif /* ARCANUM_GAME_REPUTATION_H_ */
