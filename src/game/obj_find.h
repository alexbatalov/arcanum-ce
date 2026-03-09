#ifndef ARCANUM_GAME_OBJ_FIND_H_
#define ARCANUM_GAME_OBJ_FIND_H_

#include "game/context.h"

typedef struct FindNode FindNode;

void obj_find_init(void);
void obj_find_exit(void);
void obj_find_add(int64_t obj);
void obj_find_remove(int64_t obj);
void obj_find_move(int64_t obj);
bool obj_find_walk_first(int64_t sec, int64_t* obj_ptr, FindNode** iter_ptr);
bool obj_find_walk_next(int64_t* obj_ptr, FindNode** iter_ptr);

#endif /* ARCANUM_GAME_OBJ_FIND_H_ */
