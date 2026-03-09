#ifndef ARCANUM_UI_TB_UI_H_
#define ARCANUM_UI_TB_UI_H_

#include "game/context.h"

bool tb_ui_init(GameInitInfo* init_info);
void tb_ui_reset(void);
void tb_ui_exit(void);
void sub_57CCF0(int64_t source_obj, int64_t target_obj);
bool sub_57CD60(int64_t source_obj, int64_t target_obj, char* buffer);

#endif /* ARCANUM_UI_TB_UI_H_ */
