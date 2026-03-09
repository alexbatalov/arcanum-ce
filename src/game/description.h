#ifndef ARCANUM_GAME_DESCRIPTION_H_
#define ARCANUM_GAME_DESCRIPTION_H_

#include "game/context.h"

bool description_init(GameInitInfo* init_info);
void description_exit(void);
bool description_mod_load(void);
void description_mod_unload(void);
const char* description_get(int num);
const char* key_description_get(int num);

#endif /* ARCANUM_GAME_DESCRIPTION_H_ */
