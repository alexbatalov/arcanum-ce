#ifndef ARCANUM_GAME_TC_H_
#define ARCANUM_GAME_TC_H_

#include "game/context.h"

bool tc_init(GameInitInfo* init_info);
void tc_exit(void);
void tc_resize(GameResizeInfo* resize_info);
void tc_draw(GameDrawInfo* draw_info);
void tc_scroll(int dx, int dy);
void tc_show(void);
void tc_hide(void);
void tc_clear(bool compact);
void tc_set_option(int index, const char* str);
int tc_handle_message(TigMessage* msg);
int tc_check_size(const char* str);

/**
 * Returns the height (in pixels) reserved at the bottom of the iso view by the
 * conversation option box for the given dialogue scale. Used by the text bubble
 * module to avoid overlapping enlarged dialogue options.
 */
int tc_reserved_height(float dialog_scale);

#endif /* ARCANUM_GAME_TC_H_ */
