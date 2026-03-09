#ifndef ARCANUM_GAME_SECTOR_SCRIPT_H_
#define ARCANUM_GAME_SECTOR_SCRIPT_H_

#include "game/context.h"
#include "game/script.h"

bool sector_script_init(GameInitInfo* init_info);
void sector_script_reset(void);
void sector_script_exit(void);
bool sector_script_get(int64_t sector_id, Script* scr);
bool sector_script_set(int64_t sector_id, Script* scr);

#endif /* ARCANUM_GAME_SECTOR_SCRIPT_H_ */
