#ifndef ARCANUM_GAME_ANTITELEPORT_H_
#define ARCANUM_GAME_ANTITELEPORT_H_

#include "game/context.h"

bool antiteleport_init(GameInitInfo* init_info);
void antiteleport_exit(void);
bool antiteleport_mod_load(void);
void antiteleport_mod_unload(void);
bool antiteleport_check(int64_t obj, int64_t to_loc);

#endif /* ARCANUM_GAME_ANTITELEPORT_H_ */
