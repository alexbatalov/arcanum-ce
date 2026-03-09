#ifndef ARCANUM_GAME_LEVEL_H_
#define ARCANUM_GAME_LEVEL_H_

#include "game/context.h"

bool level_init(GameInitInfo* init_info);
void level_exit(void);
int level_experience_points_to_next_level(int64_t obj);
int level_progress_towards_next_level(int64_t obj);
void level_recalc(int64_t pc_obj);
int auto_level_scheme_get(int64_t obj);
int auto_level_scheme_set(int64_t obj, int value);
const char* auto_level_scheme_name(int scheme);
const char* auto_level_scheme_rule(int scheme);
void level_set(int64_t obj, int level);

#endif /* ARCANUM_GAME_LEVEL_H_ */
