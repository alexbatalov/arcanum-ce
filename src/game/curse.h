#ifndef ARCANUM_GAME_CURSE_H_
#define ARCANUM_GAME_CURSE_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

typedef struct CurseLogbookEntry {
    /* 0000 */ int id;
    /* 0008 */ DateTime datetime;
} CurseLogbookEntry;

bool curse_mod_load(void);
void curse_mod_unload(void);
void curse_copy_name(int curse, char* buffer);
void curse_copy_description(int curse, char* buffer);
int curse_get_logbook_data(int64_t obj, CurseLogbookEntry* logbook_entries);
bool curse_has(int64_t obj, int curse);
void curse_add(int64_t obj, int curse);
int curse_get_effect(int curse);
void curse_remove(int64_t obj, int curse);

#endif /* ARCANUM_GAME_CURSE_H_ */
