#ifndef ARCANUM_UI_LOGBOOK_UI_H_
#define ARCANUM_UI_LOGBOOK_UI_H_

#include "game/context.h"

bool logbook_ui_init(GameInitInfo* init_info);
void logbook_ui_exit(void);
void logbook_ui_reset(void);
void logbook_ui_open(int64_t obj);
void logbook_ui_close(void);
bool logbook_ui_is_created(void);
void logbook_ui_check(void);

#endif /* ARCANUM_UI_LOGBOOK_UI_H_ */
