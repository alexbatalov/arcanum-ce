#ifndef ARCANUM_GAME_SCROLL_H_
#define ARCANUM_GAME_SCROLL_H_

#include "game/context.h"
#include "game/location.h"

typedef enum ScrollSpeed {
    SCROLL_SPEED_SLOW,
    SCROLL_SPEED_NORMAL,
    SCROLL_SPEED_FAST,
    SCROLL_SPEED_VERY_FAST,
} ScrollSpeed;

typedef enum ScrollDirection {
    SCROLL_DIRECTION_UP,
    SCROLL_DIRECTION_UP_RIGHT,
    SCROLL_DIRECTION_RIGHT,
    SCROLL_DIRECTION_DOWN_RIGHT,
    SCROLL_DIRECTION_DOWN,
    SCROLL_DIRECTION_DOWN_LEFT,
    SCROLL_DIRECTION_LEFT,
    SCROLL_DIRECTION_UP_LEFT,
} ScrollDirection;

typedef void(ScrollFunc)(int direction);

bool scroll_init(GameInitInfo* init_info);
void scroll_exit(void);
void scroll_reset(void);
void scroll_resize(GameResizeInfo* resize_info);
void scroll_update_view(ViewOptions* view_options);
void scroll_speed_set(ScrollSpeed value);
ScrollSpeed scroll_speed_get(void);
void scroll_start(int direction);
void scroll_stop(void);
void scroll_fps_set(int fps);
void scroll_distance_set(int distance);
int scroll_distance_get(void);
void scroll_set_center(int64_t location);
void scroll_set_scroll_func(ScrollFunc* func);

#endif /* ARCANUM_GAME_SCROLL_H_ */
