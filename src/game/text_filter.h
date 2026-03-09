#ifndef ARCANUM_GAME_TEXT_FILTER_H_
#define ARCANUM_GAME_TEXT_FILTER_H_

#include "game/context.h"

bool text_filter_init(void);
void text_filter_exit(void);
bool text_filter_process(int64_t obj, const char* src, char** dst);

#endif /* ARCANUM_GAME_TEXT_FILTER_H_ */
