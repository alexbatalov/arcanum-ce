#ifndef ARCANUM_UI_ANIM_UI_H_
#define ARCANUM_UI_ANIM_UI_H_

#include "game/context.h"

typedef enum AnimUiEventType {
    ANIM_UI_EVENT_TYPE_UPDATE_HEALTH_BAR,
    ANIM_UI_EVENT_TYPE_UPDATE_FATIGUE_BAR,
    ANIM_UI_EVENT_TYPE_2,
    ANIM_UI_EVENT_TYPE_3,
    ANIM_UI_EVENT_TYPE_4,
    ANIM_UI_EVENT_TYPE_5,
    ANIM_UI_EVENT_TYPE_6,
    ANIM_UI_EVENT_TYPE_7,
    ANIM_UI_EVENT_TYPE_ROTATE_INTERFACE,
    ANIM_UI_EVENT_TYPE_9,
    ANIM_UI_EVENT_TYPE_END_DEATH,
    ANIM_UI_EVENT_TYPE_END_GAME,
    ANIM_UI_EVENT_TYPE_REFRESH_COMBAT_UI,
    ANIM_UI_EVENT_TYPE_END_RANDOM_ENCOUNTER,
    ANIM_UI_EVENT_TYPE_HIDE_COMPACT_UI,
} AnimUiEventType;

bool anim_ui_init(GameInitInfo* init_info);
void anim_ui_exit(void);
void anim_ui_reset(void);
bool anim_ui_save(TigFile* stream);
bool anim_ui_load(GameLoadInfo* load_info);
void anim_ui_event_add(int type, int param);
void anim_ui_event_add_delay(int type, int param, int milliseconds);
void anim_ui_event_remove(int type, int param);
void ambient_lighting_enable(void);
void ambient_lighting_disable(void);

#endif /* ARCANUM_UI_ANIM_UI_H_ */
