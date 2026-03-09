#ifndef ARCANUM_UI_COMPACT_UI_H_
#define ARCANUM_UI_COMPACT_UI_H_

#include "game/context.h"

bool compact_ui_init(GameInitInfo* init_info);
void compact_ui_exit(void);
void compact_ui_reset(void);
bool compact_ui_create(void);
bool compact_ui_destroy(void);
void compact_ui_draw(void);
tig_window_handle_t compact_ui_message_window_acquire(void);
void compact_ui_message_window_box(void);
void compact_ui_message_window_hide(void);
void compact_ui_message_window_release(void);

#endif /* ARCANUM_UI_COMPACT_UI_H_ */
