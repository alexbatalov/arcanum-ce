#ifndef ARCANUM_GAME_CI_H_
#define ARCANUM_GAME_CI_H_

#include "game/context.h"

bool ci_init(GameInitInfo* init_info);
void ci_exit(void);
void sub_4BB8E0(void);
void sub_4BB8F0(void);
int sub_4BB900(void);
void sub_4BB910(void);
void ci_redraw(void);

#endif /* ARCANUM_GAME_CI_H_ */
