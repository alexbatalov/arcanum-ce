#ifndef ARCANUM_GAME_PLAYER_H_
#define ARCANUM_GAME_PLAYER_H_

#include "game/context.h"
#include "game/obj.h"

#define PLAYER_CREATE_INFO_OBJ 0x0001
#define PLAYER_CREATE_INFO_LOC 0x0002
#define PLAYER_CREATE_INFO_NETWORK 0x0004

typedef struct PlayerCreateInfo {
    int64_t obj;
    ObjectID oid;
    int64_t loc;
    unsigned int flags;
    int basic_prototype;
} PlayerCreateInfo;

bool player_init(GameInitInfo* init_info);
void player_reset(void);
void player_exit(void);
bool player_save(TigFile* stream);
bool player_load(GameLoadInfo* load_info);
void sub_40D860(void);
void player_create(void);
bool player_is_local_pc_obj(int64_t obj);
bool player_is_pc_obj(int64_t obj);
int64_t player_get_local_pc_obj(void);
bool sub_40DAB0(void);
bool sub_40DAF0(int64_t obj);
void player_create_info_init(PlayerCreateInfo* player_create_info);
bool player_obj_create_player(PlayerCreateInfo* player_create_info);

#endif /* ARCANUM_GAME_PLAYER_H_ */
