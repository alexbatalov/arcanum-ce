#ifndef ARCANUM_UI_COMBAT_UI_H_
#define ARCANUM_UI_COMBAT_UI_H_

#include "game/context.h"

bool combat_ui_init(GameInitInfo* init_info);
void combat_ui_exit(void);
void combat_ui_reset(void);
void combat_ui_resize(GameResizeInfo* resize_info);
void combat_ui_refresh(void);

#endif /* ARCANUM_UI_COMBAT_UI_H_ */
