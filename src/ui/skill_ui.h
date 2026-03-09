#ifndef ARCANUM_UI_SKILL_UI_H_
#define ARCANUM_UI_SKILL_UI_H_

#include "game/context.h"
#include "game/target.h"

bool skill_ui_init(GameInitInfo* init_info);
void skill_ui_reset(void);
void skill_ui_exit(void);
int sub_579F50(int index);
int sub_579F70(int index);
int sub_579F90(void);
void sub_579FA0(int64_t obj, int type);
void skill_ui_preprocess(int64_t obj, int type);
void skill_ui_cancel(void);
void sub_57A1F0(S4F2810* a1);
void skill_ui_activate(S4F2810* a1, int64_t obj, int a3);
int sub_57A6A0(int index);
int sub_57A6C0(int a1, S4F2810* a2);
char* sub_57A700(int index);
void skill_ui_adjust_skill(int64_t obj, int skill, int action);
void skill_ui_set_training(int64_t obj, int skill, int training);
void skill_ui_inc_skill(int64_t obj, int skill);
void skill_ui_dec_skill(int64_t obj, int skill);

#endif /* ARCANUM_UI_SKILL_UI_H_ */
