#ifndef ARCANUM_GAME_MULTIPLAYER_H_
#define ARCANUM_GAME_MULTIPLAYER_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

#define MULTIPLAYER_ALWAYS_RUN 0x0100u
#define MULTIPLAYER_AUTO_ATTACK 0x0200u
#define MULTIPLAYER_AUTO_SWITCH_WEAPONS 0x0400u
#define MULTIPLAYER_FOLLOWER_SKILLS 0x0800u

typedef void(Func5F0E08)(void);
typedef bool(Func5F0DF8)(tig_button_handle_t button_handle);

bool multiplayer_init(GameInitInfo* init_info);
void multiplayer_exit(void);
void multiplayer_reset(void);
bool multiplayer_save(TigFile* stream);
bool mutliplayer_load(GameLoadInfo* load_info);
bool multiplayer_mod_load(void);
void multiplayer_mod_unload(void);
bool multiplayer_start(void);
bool multiplayer_end(void);
void sub_49CC50(void);
bool sub_49CC70(const char* a1, const char* a2);
bool multiplayer_timeevent_process(TimeEvent* timeevent);
bool multiplayer_map_open_by_name(const char* name);
void sub_4A2B00(Func5F0E08* func);
int multiplayer_find_slot_from_obj(int64_t obj);
int64_t sub_4A2B60(int player);
bool multiplayer_is_locked(void);
void multiplayer_lock(void);
void multiplayer_unlock(void);
int64_t multiplayer_player_find_first(void);
int64_t multiplayer_player_find_next(void);
void sub_4A3230(ObjectID oid, bool (*success_func)(void*), void* success_info, bool (*failure_func)(void*), void* failure_info);
void sub_4A3890(void);
int sub_4A38A0(void);
bool sub_4A38B0(bool (*func)(tig_button_handle_t), tig_button_handle_t button_handle);
void sub_4A39D0(Func5F0DF8* func, tig_button_handle_t button_handle);
bool sub_4A3D00(bool a1);
bool sub_4A3D70(int64_t** objs_ptr, int* cnt_ptr);
void sub_4A40F0(int player, ObjectID oid, int level, void* data, int size);
void* sub_4A4180(int player);
int sub_4A41B0(int player);
ObjectID sub_4A41E0(int player);
void* sub_4A4230(int player);
int sub_4A4240(int player);
void sub_4A4270(void);
void sub_4A4280(void);
bool sub_4A4320(void);
bool multiplayer_notify_level_scheme_changed(int64_t obj);
bool multiplayer_level_scheme_rule(int64_t obj, char* str);
bool multiplayer_level_scheme_name(int64_t obj, char* str);
bool multiplayer_portrait_path(int64_t obj, int size, char* path);
bool sub_4A50D0(int64_t pc_obj, int64_t item_obj);
bool sub_4A51C0(int64_t pc_obj, int64_t item_obj);
int sub_4A5460(int64_t a1);
void multiplayer_ping(tig_timestamp_t timestamp);
void sub_4A53B0(int64_t a1, int64_t a2);
void multiplayer_flags_set(int client_id, unsigned int flags);
void multiplayer_flags_unset(int client_id, unsigned int flags);
unsigned int multiplayer_flags_get(int client_id);
int sub_4A55F0(void);
void sub_4A5600(int a1);
void sub_4A5610(void);
bool sub_4A6190(int64_t a1, int64_t a2, int64_t a3, int64_t a4);
bool sub_4A6470(int64_t pc_obj);

#endif /* ARCANUM_GAME_MULTIPLAYER_H_ */
