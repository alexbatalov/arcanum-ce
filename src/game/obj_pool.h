#ifndef ARCANUM_GAME_OBJ_POOL_H_
#define ARCANUM_GAME_OBJ_POOL_H_

#include "game/obj_id.h"

#define OBJ_HANDLE_NULL 0LL

void obj_pool_init(int size, bool editor);
void obj_pool_exit(void);
void* obj_pool_allocate(int64_t* obj_ptr);
void* obj_pool_lock(int64_t obj);
void obj_pool_unlock(int64_t obj);
void obj_pool_deallocate(int64_t obj);
void obj_pool_perm_oid_set(ObjectID oid, int64_t obj);
int64_t objp_perm_lookup(ObjectID oid);
ObjectID sub_4E5280(int64_t obj);
void sub_4E52F0(ObjectID oid);
void sub_4E5300(void);
bool obj_pool_walk_first(int64_t* obj_ptr, int* iter_ptr);
bool obj_pool_walk_next(int64_t* obj_ptr, int* iter_ptr);
bool obj_handle_is_valid(int64_t obj);
bool obj_handle_request(int64_t obj);

#endif /* ARCANUM_GAME_OBJ_POOL_H_ */
