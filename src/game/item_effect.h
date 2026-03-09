#ifndef ARCANUM_GAME_ITEM_EFFECT_H_
#define ARCANUM_GAME_ITEM_EFFECT_H_

#include "game/context.h"

bool item_effect_init(GameInitInfo* init_info);
void item_effect_exit(void);
bool item_effect_mod_load(void);
void item_effect_mod_unload(void);
char* item_effect_get(int num);

#endif /* ARCANUM_GAME_ITEM_EFFECT_H_ */
