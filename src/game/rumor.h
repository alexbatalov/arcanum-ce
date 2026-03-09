#ifndef ARCANUM_GAME_RUMOR_H_
#define ARCANUM_GAME_RUMOR_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

#define MAX_RUMORS 2000

// FIXME: Waste memory due to alignment (current size is 24 bytes, but could
// be reduced to 16 just by rearranging `datetime`).
typedef struct RumorLogbookEntry {
    /* 0000 */ int num;
    /* 0004 */ DateTime datetime;
    /* 0010 */ bool quelled;
} RumorLogbookEntry;

bool rumor_init(GameInitInfo* init_info);
void rumor_reset(void);
void rumor_exit(void);
bool rumor_mod_load(void);
void rumor_mod_unload(void);
bool rumor_load(GameLoadInfo* load_info);
bool rumor_save(TigFile* stream);
bool rumor_qstate_get(int rumor);
void rumor_qstate_set(int rumor);
void rumor_copy_interaction_str(int64_t pc_obj, int64_t npc_obj, int rumor, char* buffer);
void rumor_known_set(int64_t obj, int rumor);
bool rumor_known_get(int64_t obj, int rumor);
void rumor_copy_logbook_str(int64_t obj, int rumor, char* buffer);
void rumor_copy_logbook_normal_str(int rumor, char* buffer);
void rumor_copy_logbook_dumb_str(int rumor, char* buffer);
int rumor_get_logbook_data(int64_t obj, RumorLogbookEntry* logbook_entries);
bool rumor_copy_known(int64_t src_obj, int64_t dst_obj);

#endif /* ARCANUM_GAME_RUMOR_H_ */
