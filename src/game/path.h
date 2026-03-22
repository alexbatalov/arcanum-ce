#ifndef ARCANUM_GAME_PATH_H_
#define ARCANUM_GAME_PATH_H_

#include "game/context.h"

typedef unsigned int PathFlags;

#define PATH_FLAG_0x0001 0x0001u
#define PATH_FLAG_0x0002 0x0002u
#define PATH_FLAG_0x0004 0x0004u
#define PATH_FLAG_0x0008 0x0008u
#define PATH_FLAG_0x0010 0x0010u
#define PATH_FLAG_0x0020 0x0020u
#define PATH_FLAG_0x0040 0x0040u
#define PATH_FLAG_0x0080 0x0080u
#define PATH_FLAG_0x0100 0x0100u
#define PATH_FLAG_0x0200 0x0200u
#define PATH_FLAG_0x0400 0x0400u
#define PATH_FLAG_0x0800 0x0800u
#define PATH_FLAG_0x1000 0x1000u

typedef struct PathCreateInfo {
    /* 0000 */ int64_t obj;
    /* 0008 */ int64_t from;
    /* 0010 */ int64_t to;
    /* 0018 */ int max_rotations;
    /* 001C */ int8_t* rotations;
    /* 0020 */ PathFlags flags;
    /* 0024 */ int field_24;
} PathCreateInfo;

typedef struct WmapPathInfo {
    /* 0000 */ int64_t from;
    /* 0008 */ int64_t to;
    /* 0010 */ int max_rotations;
    /* 0014 */ int8_t* rotations;
    /* 0018 */ int field_18;
} WmapPathInfo;

int path_make(PathCreateInfo* path_create_info);
unsigned int path_translate_flags(PathFlags flags);
int path_make_straight(int64_t from, int64_t to, int8_t* rotations);
int wmap_path_make(WmapPathInfo* path_info);
bool path_set_limit(int value);
bool path_set_time_limit(int value);

#endif /* ARCANUM_GAME_PATH_H_ */
