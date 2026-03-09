#ifndef ARCANUM_UI_WRITTEN_UI_H_
#define ARCANUM_UI_WRITTEN_UI_H_

#include "game/context.h"
#include "game/written.h"

bool written_ui_mod_load(void);
void written_ui_mod_unload(void);
void written_ui_start_obj(int64_t written_obj, int64_t pc_obj);
void written_ui_start_type(WrittenType written_type, int num);
void written_ui_close(void);
void written_ui_newspaper_headline(int num, char* str);

#endif /* ARCANUM_UI_WRITTEN_UI_H_ */
