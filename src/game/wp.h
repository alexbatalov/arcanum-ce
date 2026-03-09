#ifndef ARCANUM_GAME_WP_H_
#define ARCANUM_GAME_WP_H_

#include "game/context.h"

bool wp_init(GameInitInfo* init_info);
void wp_exit(void);
void wp_resize(GameResizeInfo* resize_info);
void wp_update_view(ViewOptions* view_options);
bool wp_is_enabled(void);
void wp_toggle(void);
void wp_draw(GameDrawInfo* draw_info);

#endif /* ARCANUM_GAME_WP_H_ */
