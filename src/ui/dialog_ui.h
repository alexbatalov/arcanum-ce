#ifndef ARCANUM_UI_DIALOG_UI_H_
#define ARCANUM_UI_DIALOG_UI_H_

#include "game/context.h"
#include "game/dialog.h"

bool dialog_ui_init(GameInitInfo* init_info);
void dialog_ui_exit(void);
void dialog_ui_reset(void);
bool dialog_ui_is_in_dialog(int64_t obj);
void dialog_ui_start_dialog(int64_t pc_obj, int64_t npc_obj, int script_num, int script_line, int num);
void dialog_ui_end_dialog(int64_t obj, int a2);
int sub_567A10(void);
void sub_567A20(int64_t obj);
void sub_567A60(int64_t obj);
void sub_567E00(int index, int a2);
void sub_5681C0(int64_t pc_obj, int64_t npc_obj);
void sub_568220(DialogSerializedData* serialized_data, int a2, int a3, int a4, int a5, char* buffer);
void dialog_ui_float_line(int64_t npc_obj, int64_t pc_obj, const char* str, int speech_id);
void sub_568830(int64_t obj);
void sub_568880(int64_t obj, int a2, int a3, int a4, int a5, int a6, const char* str);

#endif /* ARCANUM_UI_DIALOG_UI_H_ */
