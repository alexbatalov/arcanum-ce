#ifndef ARCANUM_UI_CHAREDIT_UI_H_
#define ARCANUM_UI_CHAREDIT_UI_H_

#include "game/context.h"

typedef enum ChareditMode {
    CHAREDIT_MODE_CREATE,
    CHAREDIT_MODE_ACTIVE,
    CHAREDIT_MODE_PASSIVE,
    CHAREDIT_MODE_3,
} ChareditMode;

bool charedit_init(GameInitInfo* init_info);
void charedit_exit(void);
void charedit_reset(void);
bool charedit_open(int64_t obj, ChareditMode mode);
void charedit_close(void);
bool charedit_is_created(void);
void charedit_refresh(void);
void charedit_error_not_enough_character_points(void);
void charedit_error_not_enough_level(void);
void charedit_error_not_enough_intelligence(void);
void charedit_error_not_enough_willpower(void);
void charedit_error_skill_at_max(void);
void charedit_error_not_enough_stat(int stat);
void charedit_error_skill_is_zero(void);
void charedit_error_skill_at_min(void);

#endif /* ARCANUM_UI_CHAREDIT_UI_H_ */
