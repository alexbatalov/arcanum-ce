#ifndef ARCANUM_UI_INTGAME_H_
#define ARCANUM_UI_INTGAME_H_

#include "game/context.h"
#include "game/mes.h"
#include "game/target.h"
#include "game/timeevent.h"
#include "game/ui.h"
#include "ui/types.h"

typedef enum PcLensMode {
    PC_LENS_MODE_NONE,
    PC_LENS_MODE_PASSTHROUGH,
    PC_LENS_MODE_BLACKOUT,
} PcLensMode;

typedef struct PcLens {
    tig_window_handle_t window_handle;
    TigRect* rect;
    tig_art_id_t art_id;
} PcLens;

typedef enum IntgameBar {
    INTGAME_BAR_HEALTH,
    INTGAME_BAR_FATIGUE,
    INTGAME_BAR_COUNT,
} IntgameBar;

typedef enum IntgameMode {
    INTGAME_MODE_MAIN,
    INTGAME_MODE_SPELL,
    INTGAME_MODE_SKILL,
    INTGAME_MODE_DIALOG,
    INTGAME_MODE_BARTER,
    INTGAME_MODE_WMAP,
    INTGAME_MODE_SLEEP,
    INTGAME_MODE_LOGBOOK,
    INTGAME_MODE_INVEN,
    INTGAME_MODE_CHAREDIT,
    INTGAME_MODE_LOOT,
    INTGAME_MODE_STEAL,
    INTGAME_MODE_12,
    INTGAME_MODE_QUANTITY,
    INTGAME_MODE_SCHEMATIC,
    INTGAME_MODE_WRITTEN,
    INTGAME_MODE_ITEM,
    INTGAME_MODE_17,
    INTGAME_MODE_FOLLOWER,
    INTGAME_MODE_NPC_IDENTIFY,
    INTGAME_MODE_NPC_REPAIR,
    INTGAME_MODE_COUNT,
} IntgameMode;

typedef enum RotatingWindowType {
    ROTWIN_TYPE_INVALID = -1,
    ROTWIN_TYPE_MSG,
    ROTWIN_TYPE_SPELLS,
    ROTWIN_TYPE_SKILLS,
    ROTWIN_TYPE_CHAT,
    ROTWIN_TYPE_TRAPS,
    ROTWIN_TYPE_DIALOGUE,
    ROTWIN_TYPE_MAP_NOTE,
    ROTWIN_TYPE_BROADCAST,
    ROTWIN_TYPE_MAGICTECH,
    ROTWIN_TYPE_QUANTITY,
    ROTWIN_TYPE_MP_KICKBAN,
    ROTWIN_TYPE_COUNT,
} RotatingWindowType;

extern tig_font_handle_t intgame_morph15_white_font;

bool intgame_init(GameInitInfo* init_info);
void intgame_reset(void);
void intgame_resize(GameResizeInfo* resize_info);
void intgame_exit(void);
bool intgame_save(TigFile* stream);
bool intgame_load(GameLoadInfo* load_info);
void iso_interface_create(tig_window_handle_t window_handle);
void iso_interface_destroy(void);
void sub_54AA30(void);
bool intgame_button_create_ex(tig_window_handle_t window_handle, TigRect* rect, UiButtonInfo* button_info, unsigned int flags);
bool intgame_button_create(UiButtonInfo* button_info);
void intgame_button_destroy(UiButtonInfo* button_info);
void intgame_draw_bar(int bar);
void intgame_draw_bars(void);
void intgame_counters_refresh(void);
bool sub_54B5D0(TigMessage* msg);
void intgame_process_event(TigMessage* msg);
void sub_54EA80(S4F2810* a1);
bool intgame_hotkey_is_dragging(void);
void intgame_hotkey_activate(Hotkey* hotkey);
void intgame_hotkey_highlight(Hotkey* hotkey);
void iso_interface_window_set(RotatingWindowType window_type);
void intgame_message_window_clear(void);
void sub_550750(UiMessage* ui_message);
void sub_550770(int a1, char* str);
void sub_5507D0(void (*func)(UiMessage* ui_message));
void intgame_message_window_display_spell(int spl);
void intgame_message_window_display_college(int college);
void intgame_message_window_display_skill(int value);
void intgame_message_window_clear_internal(void);
void intgame_pc_lens_do(PcLensMode mode, PcLens* pc_lens);
bool intgame_pc_lens_check_pt(int x, int y);
bool intgame_pc_lens_check_pt_unscale(int x, int y);
void intgame_pc_lens_redraw(void);
void iso_interface_refresh(void);
bool sub_5517A0(TigMessage* msg);
bool intgame_get_location_under_cursor(int64_t* loc_ptr);
IntgameMode intgame_mode_get(void);
bool intgame_mode_set(IntgameMode mode);
bool intgame_mode_supports_scrolling(IntgameMode mode);
RotatingWindowType iso_interface_window_get(void);
void iso_interface_window_set_animated(RotatingWindowType window_type);
void intgame_text_edit_refresh(const char* str, tig_font_handle_t font);
void intgame_text_edit_refresh_color(const char* str, tig_font_handle_t font, tig_color_t color, bool a4);
bool intgame_clock_process_callback(TimeEvent* timeevent);
bool intgame_dialog_begin(bool (*func)(TigMessage* msg));
void intgame_dialog_end(void);
void intgame_dialog_clear(void);
void intgame_dialog_set_option(int index, const char* str);
int intgame_dialog_get_option(TigMessage* msg);
RotatingWindowType iso_interface_window_get_2(void);
void intgame_spell_maintain_art_set(int slot, tig_art_id_t art_id);
void intgame_spell_maintain_refresh(int slot, bool active);
void intgame_refresh_cursor(void);
void sub_553A60(int art_num);
void intgame_examine_object(int64_t pc_obj, int64_t target_obj, char* str);
bool intgame_examine_portrait(int64_t pc_obj, int64_t target_obj, int* portrait_ptr);
tig_art_id_t sub_554BE0(int64_t obj);
void intgame_message_window_display_attack(int64_t obj);
void intgame_message_window_display_defense(int64_t obj);
void intgame_toggle_primary_button(UiPrimaryButton btn, bool on);
void intgame_set_map_button(UiPrimaryButton btn);
void sub_556E60(void);
void sub_5570A0(int64_t obj);
void intgame_notify_item_inserted_or_removed(int64_t item_obj, bool removed, int inventory_location);
void intgame_refresh_health_bar(int64_t obj);
bool intgame_big_window_lock(TigWindowMessageFilterFunc* func, tig_window_handle_t* window_handle_ptr);
void intgame_big_window_unlock(void);
void sub_557370(int64_t source_obj, int64_t target_obj);
void intgame_there_is_nothing_to_loot(void);
void sub_5576B0(void);
void sub_557730(int index);
void sub_557790(int64_t obj);
unsigned int intgame_get_iso_window_flags(void);
void intgame_set_iso_window_flags(unsigned int flags);
void intgame_set_iso_window_width(int width);
void intgame_set_iso_window_height(int height);
bool intgame_create_iso_window(tig_window_handle_t* window_handle_ptr);
bool intgame_is_compact_interface(void);
void intgame_set_fullscreen(void);
void intgame_toggle_interface(void);
RotatingWindowType iso_interface_window_get_3(void);
int sub_557AB0(void);
void sub_557AC0(int group, int index, UiButtonInfo* button_info);
int64_t sub_557B00(void);
mes_file_handle_t intgame_hotkey_mes_file(void);
UiButtonInfo* intgame_recent_action_button_get(int index);
void intgame_recent_action_button_position_set(int index, int x, int y);
int sub_557B50(int index);
int sub_557B60(void);
int sub_557C00(void);
int sub_557CF0(void);

#endif /* ARCANUM_UI_INTGAME_H_ */
