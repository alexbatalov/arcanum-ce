#ifndef ARCANUM_GAME_GAMEINIT_H_
#define ARCANUM_GAME_GAMEINIT_H_

#include "game/context.h"

bool gameinit_init(GameInitInfo* init_info);
void gameinit_reset(void);
void gameinit_exit(void);
bool gameinit_mod_load(void);
void gameinit_mod_unload(void);

#endif /* ARCANUM_GAME_GAMEINIT_H_ */
