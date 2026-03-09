#ifndef ARCANUM_GAME_JUMPPOINT_H_
#define ARCANUM_GAME_JUMPPOINT_H_

#include "game/context.h"

typedef struct JumpPoint {
    /* 0000 */ unsigned int flags;
    /* 0004 */ int padding_4;
    /* 0008 */ int64_t loc;
    /* 0010 */ int dst_map;
    /* 0014 */ int padding_14;
    /* 0018 */ int64_t dst_loc;
} JumpPoint;

// Serializeable.
static_assert(sizeof(JumpPoint) == 0x20, "wrong size");

bool jumppoint_init(GameInitInfo* init_info);
void jumppoint_reset(void);
void jumppoint_exit(void);
void jumppoint_resize(GameResizeInfo* resize_info);
bool jumppoint_map_new(MapNewInfo* new_map_info);
bool jumppoint_open(const char* base_path, const char* save_path);
void jumppoint_map_close(void);
bool jumppoint_flush(void);
void jumppoint_update_view(ViewOptions* view_options);
bool jumppoint_is_enabled(void);
void jumppoint_toggle(void);
void jumppoint_draw(GameDrawInfo* draw_info);
bool jumppoint_get(int64_t loc, JumpPoint* jumppoint);
bool jumppoint_add(JumpPoint* jumppoint, bool update);
bool jumppoint_remove(int64_t loc);
bool jumppoint_move(int64_t from, int64_t to);
bool jumppoint_is_empty(void);

#endif /* ARCANUM_GAME_JUMPPOINT_H_ */
