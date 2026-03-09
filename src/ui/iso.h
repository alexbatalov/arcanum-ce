#ifndef ARCANUM_UI_ISO_H_
#define ARCANUM_UI_ISO_H_

#include "game/context.h"

bool iso_init(GameInitInfo* init_info);
void iso_exit(void);
void iso_reset(void);
void iso_resize(GameResizeInfo* resize_info);
void sub_557FD0(const char* str);
bool iso_redraw(void);
void sub_558130(TigRect* rect);
void iso_invalidate_rect(TigRect* rect);

#endif /* ARCANUM_UI_ISO_H_ */
