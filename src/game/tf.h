#ifndef ARCANUM_GAME_TF_H_
#define ARCANUM_GAME_TF_H_

#include "game/context.h"

typedef enum TextFloaterType {
    TF_TYPE_WHITE,
    TF_TYPE_RED,
    TF_TYPE_GREEN,
    TF_TYPE_BLUE,
    TF_TYPE_YELLOW,
    TF_TYPE_COUNT,
} TextFloaterType;

typedef enum TextFloaterLevel {
    TF_LEVEL_NEVER,
    TF_LEVEL_MINIMAL,
    TF_LEVEL_VERBOSE,
    TF_LEVEL_COUNT,
} TextFloaterLevel;

bool tf_init(GameInitInfo* init_info);
void tf_resize(GameResizeInfo* resize_info);
void tf_reset(void);
void tf_exit(void);
void tf_update_view(ViewOptions* view_options);
void tf_map_close(void);
int tf_level_set(int value);
int tf_level_get(void);
void tf_ping(tig_timestamp_t timestamp);
void tf_draw(GameDrawInfo* draw_info);
void tf_add(int64_t obj, int type, const char* str);
void tf_notify_moved(int64_t obj, int64_t loc, int offset_x, int offset_y);
void tf_notify_killed(int64_t obj);
void tf_remove(int64_t obj);

#endif /* ARCANUM_GAME_TF_H_ */
