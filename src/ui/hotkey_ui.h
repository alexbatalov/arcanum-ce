#ifndef ARCANUM_UI_HOTKEY_UI_H_
#define ARCANUM_UI_HOTKEY_UI_H_

#include "game/context.h"
#include "game/obj.h"
#include "ui/types.h"

extern int dword_5CB4E4;
extern int dword_6839B0;

bool hotkey_ui_init(GameInitInfo* init_info);
void hotkey_ui_exit();
void hotkey_ui_resize(GameResizeInfo* resize_info);
bool hotkey_ui_start(tig_window_handle_t primary_window_handle, TigRect* rect, tig_window_handle_t secondary_window_handle, bool fullscreen);
void hotkey_ui_end();
void hotkey_ui_reset_recent_actions();
bool hotkey_ui_save(TigFile* stream);
bool hotkey_ui_load(GameLoadInfo* load_info);
void sub_57DC20();
bool hotkey_ui_process_event(TigMessage* msg);
bool sub_57DE00();
void intgame_hotkey_refresh(int index);
void sub_57E5A0(Hotkey* hotkey);
bool hotkey_ui_begin_drag();
void sub_57EF90(int index);
void sub_57EFA0(int type, int data, int64_t item_obj);
void sub_57E8B0();
bool sub_57E8D0(TigMessageMouseEvent mouse_event);
bool sub_57EDA0(TigMessageMouseEvent mouse_event);
void sub_57EDF0(int64_t obj, int64_t a2, int a3);
bool sub_57EED0(int64_t obj, int a2);
void sub_57F1D0(int index);
void sub_57F210(int index);
void hotkey_ui_notify_item_inserted_or_removed(int64_t item_obj, bool removed);
Hotkey* sub_57F240(int index);
bool sub_57F260();
void hotkey_ui_notify_spell_removed(int spell);
void intgame_hotkeys_recover();

#endif /* ARCANUM_UI_HOTKEY_UI_H_ */
