#ifndef ARCANUM_UI_ROLLER_UI_H_
#define ARCANUM_UI_ROLLER_UI_H_

#include "game/context.h"

typedef unsigned int RollerFlags;

#define ROLLER_SIGN 0x01u
#define ROLLER_ZERO 0x02u
#define ROLLER_GREEN 0x04u
#define ROLLER_RED 0x08u

bool roller_ui_init(GameInitInfo* init_info);
void roller_ui_exit(void);
void roller_ui_draw(int value, tig_window_handle_t window_handle, int x, int y, int num_digits, RollerFlags flags);

#endif /* ARCANUM_UI_ROLLER_UI_H_ */
