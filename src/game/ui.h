#ifndef ARCANUM_GAME_UI_H_
#define ARCANUM_GAME_UI_H_

#include "game/context.h"
#include "game/schematic.h"
#include "game/timeevent.h"
#include "game/written.h"

typedef enum UiMessageType {
    UI_MSG_TYPE_LEVEL,
    UI_MSG_TYPE_POISON,
    UI_MSG_TYPE_CURSE,
    UI_MSG_TYPE_BLESS,
    UI_MSG_TYPE_EXCLAMATION,
    UI_MSG_TYPE_QUESTION,
    UI_MSG_TYPE_FEEDBACK,
    UI_MSG_TYPE_SKILL,
    UI_MSG_TYPE_SPELL,
    UI_MSG_TYPE_COLLEGE,
    UI_MSG_TYPE_TECH,
    UI_MSG_TYPE_DEGREE,
    UI_MSG_TYPE_STAT,
    UI_MSG_TYPE_SCHEMATIC,
    UI_MSG_TYPE_COUNT,
} UiMessageType;

typedef struct UiMessage {
    int type;
    char* str;
    int field_8;
    int field_C;
    int64_t field_10;
} UiMessage;

typedef enum UiPrimaryButton {
    UI_PRIMARY_BUTTON_CHAR,
    UI_PRIMARY_BUTTON_LOGBOOK,
    UI_PRIMARY_BUTTON_TOWNMAP,
    UI_PRIMARY_BUTTON_WORLDMAP,
    UI_PRIMARY_BUTTON_INVENTORY,
    UI_PRIMARY_BUTTON_COUNT,
} UiPrimaryButton;

// TODO: Remove forward declarations.
typedef struct Packet81 Packet81;
typedef struct TargetDescriptor TargetDescriptor;

typedef void(UI_CALLBACKS_FUNC_0)(void);
typedef void(UI_CALLBACKS_FUNC_4)(int64_t obj);
typedef void(UiUpdateInven)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_C)(void);
typedef void(UiRefreshHealthBar)(int64_t obj);
typedef void(UiRefreshFatigueBar)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_18)(int64_t obj);
typedef void(UiNotifyItemInsertedOrRemoved)(int64_t item_obj, bool removed, int inventory_location);
typedef void(UiShowInvenLoot)(int64_t pc_obj, int64_t target_obj);
typedef void(UiShowInvenIdentify)(int64_t pc_obj, int64_t target_obj);
typedef void(UiTechAdjustDegree)(int64_t obj, int tech, int action);
typedef void(UiSkillPreprocess)(int64_t obj, int type);
typedef void(UiSkillActivate)(TargetDescriptor* td, int64_t obj, int a3);
typedef void(UiAdjustSkillFunc)(int64_t obj, int skill, int action);
typedef void(UiSetSkillTrainingFunc)(int64_t obj, int skill, int training);
typedef void(UI_CALLBACKS_FUNC_3C)(int64_t a1, int64_t a2);
typedef bool(UiIsInDialog)(int64_t obj);
typedef void(UiSpellAdd)(int64_t obj, int spell);
typedef bool(UiSpellMaintainAdd)(int mt_id);
typedef void(UiSpellMaintainEnd)(int magictech);
typedef void(UiSpellMaintainRefresh)(void);
typedef void(UI_CALLBACKS_FUNC_58)(int64_t a1, int64_t a2);
typedef void(UiEndDeath)(void);
typedef void(UiEndGame)(void);
typedef void(UI_CALLBACKS_FUNC_64)(int a1);
typedef void(UI_CALLBACKS_FUNC_68)(int64_t a1, int64_t a2, int a3);
typedef bool(UI_CALLBACKS_FUNC_6C)(int64_t a1, int a2);
typedef void(UI_CALLBACKS_FUNC_70)(void);
typedef void(UI_CALLBACKS_FUNC_74)(void);
typedef void(UI_CALLBACKS_FUNC_78)(int64_t a1, void* a3, int a4);
typedef void(UI_CALLBACKS_FUNC_7C)(char* str);
typedef void(UI_CALLBACKS_FUNC_80)(UiMessage* ui_message);
typedef void(UI_CALLBACKS_FUNC_84)(int a1);
typedef void(UI_CALLBACKS_FUNC_88)(void);
typedef void(UiWrittenStartObj)(int64_t written_obj, int64_t pc_obj);
typedef void(UiWrittenStartType)(WrittenType written_type, int num);
typedef void(UiItemActivate)(int64_t owner_obj, int64_t item_obj);
typedef void(UiFollowerRefresh)(void);
typedef void(UiFollowerUpdate)(void);
typedef void(UiTogglePrimaryButton)(UiPrimaryButton btn, bool on);
typedef void(UiSetMapButton)(UiPrimaryButton btn);
typedef void(UiNotifySectorChanged)(int64_t a1, int64_t a2);
typedef void(UiWrittenNewspaperHeadline)(int newspaper, char* str);
typedef void(UiSleepToggle)(int64_t bed_obj);
typedef void(UiChareditErrorMsg)(int type, int param);
typedef void(UiChareditRefresh)(void);
typedef void(UiProgressBarInit)(int max_value);
typedef void(UiProgressBarUpdate)(int value);
typedef void(UiWmapSelect)(int64_t obj, int spell);
typedef bool(UiSchematicProcess)(int schematic, int64_t primary_obj, int64_t secondary_obj);
typedef bool(UiSchematicFeedback)(bool success, int64_t primary_obj, int64_t secondary_obj);
typedef void(UI_CALLBACKS_FUNC_D0)(int64_t a1, int64_t a2);
typedef bool(UI_CALLBACKS_FUNC_D4)(Packet81* pkt);
typedef void(UI_CALLBACKS_FUNC_D8)(int64_t obj);
typedef void(UI_CALLBACKS_FUNC_DC)(void);
typedef void(UI_CALLBACKS_FUNC_E0)(int64_t obj);
typedef void(UiGameUiLibReset)(void);
typedef void(UiStartDialog)(int64_t pc_obj, int64_t npc_obj, int script_num, int script_line, int num);
typedef void(UiEndDialog)(int64_t pc_obj, int a3);
typedef void(UI_CALLBACKS_FUNC_F8)(int a1, int a2, void* a3);
typedef bool(UI_CALLBACKS_FUNC_FC)(tig_button_handle_t button_handle);
typedef void(UiRefreshCursor)(void);
typedef int(UI_CALLBACKS_FUNC_104)(int a1, TargetDescriptor* td);
typedef void(UI_CALLBACKS_FUNC_108)(void);
typedef bool(UiWmapRndTimeEventProcess)(TimeEvent* timeevent);
typedef void(UI_CALLBACKS_FUNC_110)(int64_t a1, int64_t a2);
typedef void(UiQueueSlide)(int slide);
typedef bool(UiGameUiLibModLoad)(void);
typedef bool(UI_CALLBACKS_FUNC_11C)(void);
typedef bool(UI_CALLBACKS_FUNC_120)(void);
typedef bool(UI_CALLBACKS_FUNC_124)(const char* a1, const char* a2);
typedef void(UiSchematicInfoGet)(int schematic, SchematicInfo* schematic_info);
typedef int64_t(UI_CALLBACKS_FUNC_12C)(void);
typedef void(UI_CALLBACKS_FUNC_130)(int64_t obj);
typedef bool(UI_CALLBACKS_FUNC_134)(int64_t a1, int64_t a2, int a3);
typedef bool(UI_CALLBACKS_FUNC_138)(int64_t a1, int64_t a2, int a3);
typedef void(UI_CALLBACKS_FUNC_13C)(int64_t a1, int64_t a2, int a3);
typedef bool(UiInvenCreate)(int64_t pc_obj, int64_t target_obj, int mode);
typedef void(UI_CALLBACKS_FUNC_144)(const char* a1);
typedef void(UI_CALLBACKS_FUNC_148)(const char* a1);
typedef void(UI_CALLBACKS_FUNC_14C)(const char* a1, const char* a2);
typedef void(UI_CALLBACKS_FUNC_150)(const char* a1, const char* a2);
typedef void(UI_CALLBACKS_FUNC_154)(int a1, int a2);
typedef void(UI_CALLBACKS_FUNC_158)(const char* a1, const char* a2);
typedef void(UI_CALLBACKS_FUNC_15C)(void);
typedef void(UiShowInvenNpcIdentify)(int64_t pc_obj, int64_t target_obj);
typedef void(UiMpChareditCacheTraits)(int player);
typedef void(UiMultiplayerChareditTraitInc)(int player, int trait, int param);
typedef void(UiMultiplayerChareditTraitDec)(int player, int trait, int param);

typedef struct UiCallbacks {
    /* 0000 */ UI_CALLBACKS_FUNC_0* field_0;
    /* 0004 */ UI_CALLBACKS_FUNC_4* field_4;
    /* 0008 */ UiUpdateInven* update_inven;
    /* 000C */ UI_CALLBACKS_FUNC_C* field_C;
    /* 0010 */ UiRefreshHealthBar* refresh_health_bar;
    /* 0014 */ UiRefreshFatigueBar* refresh_fatigue_bar;
    /* 0018 */ UI_CALLBACKS_FUNC_18* field_18;
    /* 001C */ UiNotifyItemInsertedOrRemoved* notify_item_inserted_or_removed;
    /* 0020 */ UiShowInvenLoot* show_inven_loot;
    /* 0024 */ UiShowInvenIdentify* show_inven_identify;
    /* 0028 */ UiTechAdjustDegree* tech_adjust_degree;
    /* 002C */ UiSkillPreprocess* skill_preprocess;
    /* 0030 */ UiSkillActivate* skill_activate;
    /* 0034 */ UiAdjustSkillFunc* adjust_skill;
    /* 0038 */ UiSetSkillTrainingFunc* set_skill_training;
    /* 003C */ UI_CALLBACKS_FUNC_3C* field_3C;
    /* 0044 */ UiIsInDialog* is_in_dialog;
    /* 0048 */ UiSpellAdd* spell_add;
    /* 004C */ UiSpellMaintainAdd* spell_maintain_add;
    /* 0050 */ UiSpellMaintainEnd* spell_maintain_end;
    /* 0054 */ UiSpellMaintainRefresh* spell_maintain_refresh;
    /* 0058 */ UI_CALLBACKS_FUNC_58* field_58;
    /* 005C */ UiEndDeath* end_death;
    /* 0060 */ UiEndGame* end_game;
    /* 0064 */ UI_CALLBACKS_FUNC_64* field_64;
    /* 0068 */ UI_CALLBACKS_FUNC_68* field_68;
    /* 006C */ UI_CALLBACKS_FUNC_6C* field_6C;
    /* 0070 */ UI_CALLBACKS_FUNC_70* field_70;
    /* 0074 */ UI_CALLBACKS_FUNC_74* field_74;
    /* 0078 */ UI_CALLBACKS_FUNC_78* field_78;
    /* 007C */ UI_CALLBACKS_FUNC_7C* field_7C;
    /* 0080 */ UI_CALLBACKS_FUNC_80* field_80;
    /* 0084 */ UI_CALLBACKS_FUNC_84* field_84;
    /* 0088 */ UI_CALLBACKS_FUNC_88* field_88;
    /* 008C */ UiWrittenStartObj* written_start_obj;
    /* 0090 */ UiWrittenStartType* written_start_type;
    /* 0094 */ UiItemActivate* item_activate;
    /* 0098 */ UiFollowerRefresh* follower_refresh;
    /* 009C */ UiFollowerUpdate* follower_update;
    /* 00A0 */ UiTogglePrimaryButton* toggle_primary_button;
    /* 00A4 */ UiSetMapButton* set_map_button;
    /* 00A8 */ UiNotifySectorChanged* notify_sector_changed;
    /* 00AC */ UiWrittenNewspaperHeadline* written_newspaper_headline;
    /* 00B0 */ UiSleepToggle* sleep_toggle;
    /* 00B4 */ UiChareditErrorMsg* charedit_error_msg;
    /* 00B8 */ UiChareditRefresh* charedit_refresh;
    /* 00BC */ UiProgressBarInit* progressbar_init;
    /* 00C0 */ UiProgressBarUpdate* progressbar_update;
    /* 00C4 */ UiWmapSelect* wmap_select;
    /* 00C8 */ UiSchematicProcess* schematic_process;
    /* 00CC */ UiSchematicFeedback* schematic_feedback;
    /* 00D0 */ UI_CALLBACKS_FUNC_D0* field_D0;
    /* 00D4 */ UI_CALLBACKS_FUNC_D4* field_D4;
    /* 00D8 */ UI_CALLBACKS_FUNC_D8* field_D8;
    /* 00DC */ UI_CALLBACKS_FUNC_DC* field_DC;
    /* 00E0 */ UI_CALLBACKS_FUNC_E0* field_E0;
    /* 00E4 */ UiGameUiLibReset* gameuilib_reset;
    /* 00E8 */ UiStartDialog* start_dialog;
    /* 00EC */ UiEndDialog* end_dialog;
    /* 00F8 */ UI_CALLBACKS_FUNC_F8* field_F8;
    /* 00FC */ UI_CALLBACKS_FUNC_FC* field_FC;
    /* 0100 */ UiRefreshCursor* refresh_cursor;
    /* 0104 */ UI_CALLBACKS_FUNC_104* field_104;
    /* 0108 */ UI_CALLBACKS_FUNC_108* field_108;
    /* 010C */ UiWmapRndTimeEventProcess* wmap_rnd_timeevent_process;
    /* 0110 */ UI_CALLBACKS_FUNC_110* field_110;
    /* 0114 */ UiQueueSlide* queue_slide;
    /* 0118 */ UiGameUiLibModLoad* gameuilib_mod_load;
    /* 011C */ UI_CALLBACKS_FUNC_11C* field_11C;
    /* 0120 */ UI_CALLBACKS_FUNC_120* field_120;
    /* 0124 */ UI_CALLBACKS_FUNC_124* field_124;
    /* 0128 */ UiSchematicInfoGet* schematic_info_get;
    /* 012C */ UI_CALLBACKS_FUNC_12C* field_12C;
    /* 0130 */ UI_CALLBACKS_FUNC_130* field_130;
    /* 0134 */ UI_CALLBACKS_FUNC_134* field_134;
    /* 0138 */ UI_CALLBACKS_FUNC_138* field_138;
    /* 013C */ UI_CALLBACKS_FUNC_13C* field_13C;
    /* 0140 */ UiInvenCreate* inven_create;
    /* 0144 */ UI_CALLBACKS_FUNC_144* field_144;
    /* 0148 */ UI_CALLBACKS_FUNC_148* field_148;
    /* 014C */ UI_CALLBACKS_FUNC_14C* field_14C;
    /* 0150 */ UI_CALLBACKS_FUNC_150* field_150;
    /* 0154 */ UI_CALLBACKS_FUNC_154* field_154;
    /* 0158 */ UI_CALLBACKS_FUNC_158* field_158;
    /* 015C */ UI_CALLBACKS_FUNC_15C* field_15C;
    /* 0160 */ UiShowInvenNpcIdentify* show_inven_npc_identify;
    /* 0164 */ UiMpChareditCacheTraits* mp_charedit_cache_traits;
    /* 0168 */ UiMultiplayerChareditTraitInc* mp_charedit_trait_inc;
    /* 016C */ UiMultiplayerChareditTraitDec* mp_charedit_trait_dec;
} UiCallbacks;

void ui_init(UiCallbacks* callbacks);
void ui_exit(void);
bool ui_timeevent_process(TimeEvent* timeevent);
void sub_4601C0(void);
void sub_4601D0(int64_t obj);
void ui_update_inven(int64_t obj);
void ui_refresh_health_bar(int64_t obj);
void ui_refresh_fatigue_bar(int64_t obj);
void sub_460280(int64_t obj);
void ui_notify_item_inserted_or_removed(int64_t item_obj, bool removed, int inventory_location);
void ui_show_inven_loot(int64_t pc_obj, int64_t target_obj);
void ui_show_inven_identify(int64_t pc_obj, int64_t target_obj);
void ui_tech_adjust_degree(int64_t obj, int tech, int action);
void ui_skill_preprocess(int64_t obj, int type);
void ui_skill_activate(TargetDescriptor* td, int64_t obj, int a3);
void ui_adjust_skill(int64_t obj, int skill, int action);
void ui_set_skill_training(int64_t obj, int skill, int training);
void sub_460410(int64_t a1, int64_t a2);
int ui_is_in_dialog(int64_t obj);
void ui_spell_add(int64_t obj, int spell);
bool ui_spell_maintain_add(int mt_id);
void ui_spell_maintain_end(int mt_id);
void ui_spell_maintain_refresh(void);
void sub_4604F0(int64_t a1, int64_t a2);
void ui_end_death(void);
void ui_end_game(void);
void sub_460540(int a1);
void sub_460560(int64_t a1, int64_t a2, int a3);
int sub_460590(int64_t a1, int a2);
void sub_4605C0(void);
void sub_4605D0(void);
void sub_4605E0(int64_t a1, void* a2, int a3);
void sub_460610(char* str);
void sub_460630(UiMessage* ui_message);
void sub_460650(UiMessage* ui_message, unsigned int milliseconds);
void sub_4606C0(int a1);
void sub_4606E0(void);
void ui_written_start_obj(int64_t written_obj, int64_t pc_obj);
void ui_written_start_type(WrittenType written_type, int num);
void ui_item_activate(int64_t a1, int64_t a2);
void ui_follower_refresh(void);
void ui_follower_update(void);
void ui_toggle_primary_button(UiPrimaryButton btn, bool on);
void ui_set_map_button(UiPrimaryButton btn);
void ui_notify_sector_changed(int64_t a1, int64_t a2);
void ui_written_newspaper_headline(int newspaper, char* str);
void ui_sleep_toggle(int64_t bed_obj);
void ui_charedit_error_msg(int type, int param);
void ui_charedit_refresh(void);
void ui_progressbar_init(int max_value);
void ui_progressbar_update(int value);
void ui_wmap_select(int64_t obj, int spell);
void ui_schematic_process(int schematic, int64_t primary_obj, int64_t secondary_obj);
void ui_schematic_feedback(bool success, int64_t primary_obj, int64_t secondary_obj);
void sub_460930(int64_t a1, int64_t a2);
void sub_460960(Packet81* pkt);
void sub_460980(int64_t a1);
void ui_gameuilib_reset(void);
void ui_start_dialog(int64_t pc_obj, int64_t npc_obj, int script_num, int script_line, int num);
void ui_end_dialog(int64_t a1, int a3);
void sub_460AB0(int a1, int a2, void* a3);
void sub_460AD0(int a1);
void ui_refresh_cursor(void);
int sub_460B00(int a1, TargetDescriptor* td);
void sub_460B20(void);
bool ui_wmap_rnd_timeevent_process(TimeEvent* timeevent);
void sub_460B50(int64_t a1, int64_t a2);
void ui_queue_slide(int slide);
bool ui_gameuilib_mod_load(void);
int sub_460BB0(void);
int sub_460BC0(void);
int sub_460BE0(const char* a1, const char* a2);
void ui_schematic_info_get(int schematic, SchematicInfo* schematic_info);
int64_t sub_460C20(void);
void sub_460C30(int64_t a1);
int sub_460C50(int64_t a1, int64_t a2, int a3);
int sub_460C80(int64_t a1, int64_t a2, int a3);
void sub_460CB0(int64_t a1, int64_t a2, int a3);
bool ui_inven_create(int64_t pc_obj, int64_t target_obj, int mode);
void sub_460D10(const char* a1);
void sub_460D30(const char* a1);
void sub_460D50(const char* a1, const char* a2);
void sub_460D70(const char* a1, const char* a2);
void sub_460D90(int a1, int a2);
void sub_460DB0(const char* a1, const char* a2);
void sub_460DD0(void);
void ui_show_inven_npc_identify(int64_t pc_obj, int64_t target_obj);
void ui_mp_charedit_cache_traits(int player);
void ui_mp_charedit_trait_inc(int player, int trait, int param);
void ui_mp_charedit_trait_dec(int player, int trait, int param);

#endif /* ARCANUM_GAME_UI_H_ */
