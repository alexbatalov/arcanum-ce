#ifndef ARCANUM_UI_LOGBOOK_UI_H_
#define ARCANUM_UI_LOGBOOK_UI_H_

#include "game/context.h"

bool logbook_ui_init(GameInitInfo* init_info);
void logbook_ui_exit();
void logbook_ui_reset();
void logbook_ui_open(int64_t obj);
void logbook_ui_close();
bool logbook_ui_is_created();
void logbook_ui_check();

#endif /* ARCANUM_UI_LOGBOOK_UI_H_ */
