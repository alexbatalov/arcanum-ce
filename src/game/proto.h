#ifndef ARCANUM_GAME_PROTO_H_
#define ARCANUM_GAME_PROTO_H_

#include "game/context.h"
#include "game/obj.h"

bool proto_init(GameInitInfo* init_info);
void proto_exit(void);
int64_t sub_468570(ObjectType object_type);
int64_t sub_4685A0(int a1);
bool proto_is_valid(int description);
int sub_49B290(int64_t obj);

#endif /* ARCANUM_GAME_PROTO_H_ */
