#ifndef ARCANUM_GAME_ONAME_H_
#define ARCANUM_GAME_ONAME_H_

#include "game/context.h"

bool o_name_init(GameInitInfo* init_info);
void o_name_exit(void);
bool o_name_mod_load(void);
void o_name_mod_unload(void);
int o_name_count(void);
const char* o_name_get(int oname);

#endif /* ARCANUM_GAME_ONAME_H_ */
