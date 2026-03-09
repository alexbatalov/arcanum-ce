#ifndef ARCANUM_UI_BROADCAST_UI_H_
#define ARCANUM_UI_BROADCAST_UI_H_

#include "game/context.h"

bool broadcast_ui_init(GameInitInfo* init_info);
void broadcast_ui_exit(void);
void broadcast_ui_reset(void);
void broadcast_ui_open(void);
void broadcast_ui_close(void);

#endif /* ARCANUM_UI_BROADCAST_UI_H_ */
