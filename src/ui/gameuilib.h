#ifndef ARCANUM_UI_GAMEUILIB_H_
#define ARCANUM_UI_GAMEUILIB_H_

#include "game/context.h"

bool gameuilib_init(GameInitInfo* init_info);
void gameuilib_exit(void);
void gameuilib_reset(void);
void gameuilib_resize(GameResizeInfo* resize_info);
bool gameuilib_mod_load(void);
void gameuilib_mod_unload(void);
bool gameuilib_save(void);
bool gameuilib_load(void);
bool gameuilib_wants_mainmenu(void);
void gameuilib_wants_mainmenu_set(void);
void gameuilib_wants_mainmenu_unset(void);

#endif /* ARCANUM_UI_GAMEUILIB_H_ */
