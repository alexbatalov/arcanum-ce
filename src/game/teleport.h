#ifndef ARCANUM_GAME_TELEPORT_H_
#define ARCANUM_GAME_TELEPORT_H_

#include "game/context.h"
#include "game/gfade.h"

typedef unsigned int TeleportFlags;

#define TELEPORT_MOVIE1 0x0001u
#define TELEPORT_FADE_OUT 0x0002u
#define TELEPORT_FADE_IN 0x0004u
#define TELEPORT_TIME 0x0008u
#define TELEPORT_SOUND 0x0010u
#define TELEPORT_RENDER_LOCK 0x0020u
#define TELEPORT_MOVIE2 0x0040u
#define TELEPORT_SKIP_FOLLOWERS 0x0100u

typedef struct TeleportData {
    /* 0000 */ TeleportFlags flags;
    /* 0008 */ int64_t obj;
    /* 0010 */ int64_t loc;
    /* 0018 */ int map;
    /* 001C */ int movie1;
    /* 0020 */ unsigned int movie_flags1;
    /* 0024 */ int movie2;
    /* 0028 */ unsigned int movie_flags2;
    /* 002C */ FadeData fade_out;
    /* 0040 */ FadeData fade_in;
    /* 0054 */ int sound_id;
    /* 0058 */ int time;
} TeleportData;

bool teleport_init(GameInitInfo* init_info);
void teleport_reset(void);
void teleport_exit(void);
void teleport_ping(tig_timestamp_t timestamp);
bool teleport_do(TeleportData* teleport_data);
bool teleport_is_teleporting(void);
bool teleport_is_teleporting_obj(int64_t obj);

#endif /* ARCANUM_GAME_TELEPORT_H_ */
