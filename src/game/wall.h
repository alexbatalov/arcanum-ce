#ifndef ARCANUM_GAME_WALL_H_
#define ARCANUM_GAME_WALL_H_

#include "game/context.h"

bool wall_init(GameInitInfo* init_info);
void wall_exit(void);
void wall_resize(GameResizeInfo* resize_info);
void wall_update_view(ViewOptions* view_options);
void wall_toggle(void);
void wall_draw(GameDrawInfo* draw_info);
void sub_4E1490(int64_t a1, int64_t a2);
void wall_delete(int64_t obj, bool a2);

#endif /* ARCANUM_GAME_WALL_H_ */
