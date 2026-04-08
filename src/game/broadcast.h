#ifndef ARCANUM_GAME_BROADCAST_H_
#define ARCANUM_GAME_BROADCAST_H_

#include "game/context.h"

typedef struct Broadcast {
    int64_t loc;
    char str[128];
} Broadcast;

// Serializeable.
static_assert(sizeof(Broadcast) == 0x88, "wrong size");

typedef void(BroadcastFloatLineFunc)(int64_t npc_obj, int64_t pc_obj, const char* str, int speech_id);

bool broadcast_init(GameInitInfo* init_info);
void broadcast_exit(void);
void broadcast_set_float_line_func(BroadcastFloatLineFunc* func);
void broadcast_msg(int64_t obj, Broadcast* bcast);
void broadcast_msg_client(int64_t pc_obj, Broadcast* bcast);
void sub_4C3BE0(unsigned int a1, int64_t loc);

#endif /* ARCANUM_GAME_BROADCAST_H_ */
