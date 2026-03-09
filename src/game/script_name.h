#ifndef ARCANUM_GAME_SCRIPT_NAME_H_
#define ARCANUM_GAME_SCRIPT_NAME_H_

#include "game/context.h"

bool script_name_init(GameInitInfo* ctx);
void script_name_exit(void);
bool script_name_mod_load(void);
void script_name_mod_unload(void);
bool script_name_build_scr_name(int num, char* buffer);
bool script_name_build_dlg_name(int num, char* buffer);

#endif /* ARCANUM_GAME_SCRIPT_NAME_H_ */
