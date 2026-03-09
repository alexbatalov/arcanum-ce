#ifndef ARCANUM_GAME_BLESS_H_
#define ARCANUM_GAME_BLESS_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

typedef struct BlessLogbookEntry {
    /* 0000 */ int id;
    /* 0008 */ DateTime datetime;
} BlessLogbookEntry;

bool bless_mod_load(void);
void bless_mod_unload(void);
void bless_copy_name(int bless, char* buffer);
void bless_copy_description(int bless, char* buffer);
int bless_get_logbook_data(int64_t obj, BlessLogbookEntry* logbook_entries);
bool bless_has(int64_t obj, int bless);
void bless_add(int64_t obj, int bless);
int bless_get_effect(int bless);
void bless_remove(int64_t obj, int bless);

#endif /* ARCANUM_GAME_BLESS_H_ */
