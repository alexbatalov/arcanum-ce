#ifndef ARCANUM_UI_SLIDE_UI_H_
#define ARCANUM_UI_SLIDE_UI_H_

#include "game/context.h"

typedef enum SlideUiType {
    SLIDE_UI_TYPE_END_GAME,
    SLIDE_UI_TYPE_DEATH,
    SLIDE_UI_TYPE_CREDITS,
} SlideUiType;

bool slide_ui_mod_load(void);
void slide_ui_mod_unload(void);
bool slide_ui_is_active(void);
void slide_ui_start(int type);
void slide_ui_enqueue(int slide);

#endif /* ARCANUM_UI_SLIDE_UI_H_ */
