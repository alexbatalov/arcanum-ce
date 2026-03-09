#ifndef ARCANUM_GAME_TOWNMAP_H_
#define ARCANUM_GAME_TOWNMAP_H_

#include "game/context.h"

#define TOWNMAP_NONE 0

typedef struct TownMapInfo {
    /* 0000 */ int version;
    /* 0004 */ int field_4;
    /* 0008 */ int map;
    /* 000C */ int width;
    /* 0010 */ int height;
    /* 0014 */ int num_hor_tiles;
    /* 0018 */ int num_vert_tiles;
    /* 001C */ int padding_1C;
    /* 0020 */ int64_t loc;
    /* 0028 */ float scale;
    /* 002C */ int padding_2C;
} TownMapInfo;

// Serializeable.
static_assert(sizeof(TownMapInfo) == 0x30, "wrong size");

void townmap_reset(void);
bool townmap_mod_load(void);
void townmap_mod_unload(void);
void townmap_flush(void);
int townmap_get(int64_t sec);
void townmap_set(int64_t sec, int townmap);
int townmap_count(void);
const char* townmap_name(int map);
bool townmap_info(int map, TownMapInfo* tmi);
void townmap_loc_to_coords(TownMapInfo* tmi, int64_t loc, int* x_ptr, int* y_ptr);
void townmap_coords_to_loc(TownMapInfo* tmi, int x, int y, int64_t* loc_ptr);
bool townmap_mark_visited(int64_t loc);
bool townmap_set_known(int map, bool known);
bool townmap_tile_blit_info(int map, int index, TigVideoBufferBlitInfo* vb_blit_info);
bool townmap_is_waitable(int map);

#endif /* ARCANUM_GAME_TOWNMAP_H_ */
