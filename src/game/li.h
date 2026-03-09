#ifndef ARCANUM_GAME_LI_H_
#define ARCANUM_GAME_LI_H_

#include "game/context.h"

bool li_init(GameInitInfo* init_info);
void li_exit(void);
void li_resize(GameResizeInfo* resize_info);
void sub_4BBC00(void);
void sub_4BBC10(void);
int sub_4BBC20(void);
void li_update(void);
void li_redraw(void);

#endif /* ARCANUM_GAME_LI_H_ */
