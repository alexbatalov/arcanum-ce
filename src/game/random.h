#ifndef ARCANUM_GAME_RANDOM_H_
#define ARCANUM_GAME_RANDOM_H_

#include "game/context.h"

bool random_init(GameInitInfo* init_info);
void random_exit(void);
void random_seed(int value);
int random_seed_generate(void);
int random_between(int lower, int upper);
int random_rand(void);

#endif /* ARCANUM_GAME_RANDOM_H_ */
