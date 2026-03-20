#include "game/ui.h"

// 0x5E8658
static UiCallbacks ui_callbacks;

// 0x5E87C8
static TigIdxTable stru_5E87C8;

// 0x5E87D8
static int dword_5E87D8;

// 0x45FD40
void ui_init(UiCallbacks* callbacks)
{
    if (callbacks != NULL) {
        ui_callbacks = *callbacks;
    }

    tig_idxtable_init(&stru_5E87C8, sizeof(UiMessage));
}

// 0x460150
void ui_exit(void)
{
    tig_idxtable_exit(&stru_5E87C8);
}

// 0x460160
bool ui_timeevent_process(TimeEvent* timeevent)
{
    UiMessage ui_message;

    if (timeevent->params[0].integer_value != 0) {
        tig_debug_printf("UI: Unknown type of timeevent passed to ui_timeevent_process (%d).\n",
            timeevent->params[0].integer_value);
    } else {
        tig_idxtable_get(&stru_5E87C8, timeevent->params[1].integer_value, &ui_message);
        ui_display_msg(&ui_message);
        FREE(ui_message.str);
    }

    return true;
}

// 0x4601C0
void sub_4601C0(void)
{
    if (ui_callbacks.field_0 != NULL) {
        ui_callbacks.field_0();
    }
}

// 0x4601D0
void sub_4601D0(int64_t obj)
{
    if (ui_callbacks.field_4 != NULL) {
        ui_callbacks.field_4(obj);
    }
}

// 0x4601F0
void ui_update_inven(int64_t obj)
{
    if (ui_callbacks.update_inven != NULL) {
        ui_callbacks.update_inven(obj);
    }
}

// 0x460240
void ui_refresh_health_bar(int64_t obj)
{
    if (ui_callbacks.refresh_health_bar != NULL) {
        ui_callbacks.refresh_health_bar(obj);
    }
}

// 0x460260
void ui_refresh_fatigue_bar(int64_t obj)
{
    if (ui_callbacks.refresh_fatigue_bar != NULL) {
        ui_callbacks.refresh_fatigue_bar(obj);
    }
}

// 0x460280
void sub_460280(int64_t obj)
{
    if (ui_callbacks.field_18 != NULL) {
        ui_callbacks.field_18(obj);
    }
}

// 0x4602A0
void ui_notify_item_inserted_or_removed(int64_t item_obj, bool removed, int inventory_location)
{
    if (ui_callbacks.notify_item_inserted_or_removed != NULL) {
        ui_callbacks.notify_item_inserted_or_removed(item_obj, removed, inventory_location);
    }
}

// 0x4602D0
void ui_show_inven_loot(int64_t pc_obj, int64_t target_obj)
{
    if (ui_callbacks.show_inven_loot != NULL) {
        ui_callbacks.show_inven_loot(pc_obj, target_obj);
    }
}

// 0x460300
void ui_show_inven_identify(int64_t pc_obj, int64_t target_obj)
{
    if (ui_callbacks.show_inven_identify != NULL) {
        ui_callbacks.show_inven_identify(pc_obj, target_obj);
    }
}

// 0x460330
void ui_tech_adjust_degree(int64_t obj, int tech, int action)
{
    if (ui_callbacks.tech_adjust_degree != NULL) {
        ui_callbacks.tech_adjust_degree(obj, tech, action);
    }
}

// 0x460360
void ui_skill_preprocess(int64_t obj, int type)
{
    if (ui_callbacks.skill_preprocess != NULL) {
        ui_callbacks.skill_preprocess(obj, type);
    }
}

// 0x460380
void ui_skill_activate(TargetDescriptor* td, int64_t obj, int a3)
{
    if (ui_callbacks.skill_activate != NULL) {
        ui_callbacks.skill_activate(td, obj, a3);
    }
}

// 0x4603B0
void ui_adjust_skill(int64_t obj, int skill, int action)
{
    if (ui_callbacks.adjust_skill != NULL) {
        ui_callbacks.adjust_skill(obj, skill, action);
    }
}

// 0x4603E0
void ui_set_skill_training(int64_t obj, int skill, int training)
{
    if (ui_callbacks.set_skill_training != NULL) {
        ui_callbacks.set_skill_training(obj, skill, training);
    }
}

// 0x460410
void sub_460410(int64_t a1, int64_t a2)
{
    if (ui_callbacks.field_3C != NULL) {
        ui_callbacks.field_3C(a1, a2);
    }
}

// 0x460460
int ui_is_in_dialog(int64_t obj)
{
    if (ui_callbacks.is_in_dialog != NULL) {
        return ui_callbacks.is_in_dialog(obj);
    } else {
        return 0;
    }
}

// 0x460480
void ui_spell_add(int64_t obj, int spell)
{
    if (ui_callbacks.spell_add != NULL) {
        ui_callbacks.spell_add(obj, spell);
    }
}

// 0x4604A0
bool ui_spell_maintain_add(int mt_id)
{
    if (ui_callbacks.spell_maintain_add != NULL) {
        return ui_callbacks.spell_maintain_add(mt_id);
    } else {
        return false;
    }
}

// 0x4604C0
void ui_spell_maintain_end(int mt_id)
{
    if (ui_callbacks.spell_maintain_end != NULL) {
        ui_callbacks.spell_maintain_end(mt_id);
    }
}

// 0x4604E0
void ui_spell_maintain_refresh(void)
{
    if (ui_callbacks.spell_maintain_refresh != NULL) {
        ui_callbacks.spell_maintain_refresh();
    }
}

// 0x4604F0
void sub_4604F0(int64_t a1, int64_t a2)
{
    if (ui_callbacks.field_58 != NULL) {
        ui_callbacks.field_58(a1, a2);
    }
}

// 0x460520
void ui_end_death(void)
{
    if (ui_callbacks.end_death != NULL) {
        ui_callbacks.end_death();
    }
}

// 0x460530
void ui_end_game(void)
{
    if (ui_callbacks.end_game != NULL) {
        ui_callbacks.end_game();
    }
}

// 0x460540
void sub_460540(int a1)
{
    if (ui_callbacks.field_64 != NULL) {
        ui_callbacks.field_64(a1);
    }
}

// 0x460560
void sub_460560(int64_t a1, int64_t a2, int a3)
{
    if (ui_callbacks.field_68 != NULL) {
        ui_callbacks.field_68(a1, a2, a3);
    }
}

// 0x460590
int sub_460590(int64_t a1, int a2)
{
    if (ui_callbacks.field_6C != NULL) {
        return ui_callbacks.field_6C(a1, a2);
    } else {
        return 0;
    }
}

// 0x4605C0
void sub_4605C0(void)
{
    if (ui_callbacks.field_70 != NULL) {
        ui_callbacks.field_70();
    }
}

// 0x4605D0
void sub_4605D0(void)
{
    if (ui_callbacks.field_74 != NULL) {
        ui_callbacks.field_74();
    }
}

// 0x4605E0
void sub_4605E0(int64_t a1, void* a2, int a3)
{
    if (ui_callbacks.field_78 != NULL) {
        ui_callbacks.field_78(a1, a2, a3);
    }
}

// 0x460610
void ui_display_warning(char* str)
{
    if (ui_callbacks.display_warning != NULL) {
        ui_callbacks.display_warning(str);
    }
}

// 0x460630
void ui_display_msg(UiMessage* ui_message)
{
    if (ui_callbacks.display_msg != NULL) {
        ui_callbacks.display_msg(ui_message);
    }
}

// 0x460650
void sub_460650(UiMessage* ui_message, unsigned int milliseconds)
{
    int id;

    DateTime datetime;
    TimeEvent timeevent;

    id = dword_5E87D8++;
    ui_message->str = STRDUP(ui_message->str);
    timeevent.type = TIMEEVENT_TYPE_UI;
    timeevent.params[0].integer_value = 0;
    timeevent.params[1].integer_value = id;
    sub_45A950(&datetime, milliseconds);
    tig_idxtable_set(&stru_5E87C8, id, ui_message);
    timeevent_add_delay(&timeevent, &datetime);
}

// 0x4606C0
void sub_4606C0(int a1)
{
    if (ui_callbacks.field_84 != NULL) {
        ui_callbacks.field_84(a1);
    }
}

// 0x4606E0
void sub_4606E0(void)
{
    if (ui_callbacks.field_88 != NULL) {
        ui_callbacks.field_88();
    }
}

// 0x4606F0
void ui_written_start_obj(int64_t written_obj, int64_t pc_obj)
{
    if (ui_callbacks.written_start_obj != NULL) {
        ui_callbacks.written_start_obj(written_obj, pc_obj);
    }
}

// 0x460720
void ui_written_start_type(WrittenType written_type, int num)
{
    if (ui_callbacks.written_start_type != NULL) {
        ui_callbacks.written_start_type(written_type, num);
    }
}

// 0x460740
void ui_item_activate(int64_t owner_obj, int64_t item_obj)
{
    if (ui_callbacks.item_activate != NULL) {
        ui_callbacks.item_activate(owner_obj, item_obj);
    }
}

// 0x460770
void ui_follower_refresh(void)
{
    if (ui_callbacks.follower_refresh != NULL) {
        ui_callbacks.follower_refresh();
    }
}

// 0x460780
void ui_follower_update(void)
{
    if (ui_callbacks.follower_update != NULL) {
        ui_callbacks.follower_update();
    }
}

// 0x460790
void ui_toggle_primary_button(UiPrimaryButton btn, bool on)
{
    if (ui_callbacks.toggle_primary_button != NULL) {
        ui_callbacks.toggle_primary_button(btn, on);
    }
}

// 0x4607B0
void ui_set_map_button(UiPrimaryButton btn)
{
    if (ui_callbacks.set_map_button != NULL) {
        ui_callbacks.set_map_button(btn);
    }
}

// 0x4607D0
void ui_notify_sector_changed(int64_t sec, int64_t obj)
{
    if (ui_callbacks.notify_sector_changed != NULL) {
        ui_callbacks.notify_sector_changed(sec, obj);
    }
}

// 0x460800
void ui_written_newspaper_headline(int newspaper, char* str)
{
    if (ui_callbacks.written_newspaper_headline != NULL) {
        ui_callbacks.written_newspaper_headline(newspaper, str);
    }
}

// 0x460820
void ui_sleep_toggle(int64_t bed_obj)
{
    if (ui_callbacks.sleep_toggle != NULL) {
        ui_callbacks.sleep_toggle(bed_obj);
    }
}

// 0x460840
void ui_charedit_error_msg(int type, int param)
{
    if (ui_callbacks.charedit_error_msg != NULL) {
        ui_callbacks.charedit_error_msg(type, param);
    }
}

// 0x460860
void ui_charedit_refresh(void)
{
    if (ui_callbacks.charedit_refresh != NULL) {
        ui_callbacks.charedit_refresh();
    }
}

// 0x460870
void ui_progressbar_init(int max_value)
{
    if (ui_callbacks.progressbar_init != NULL) {
        ui_callbacks.progressbar_init(max_value);
    }
}

// 0x460890
void ui_progressbar_update(int value)
{
    if (ui_callbacks.progressbar_update != NULL) {
        ui_callbacks.progressbar_update(value);
    }
}

// 0x4608B0
void ui_wmap_select(int64_t obj, int spell)
{
    if (ui_callbacks.wmap_select != NULL) {
        ui_callbacks.wmap_select(obj, spell);
    }
}

// 0x4608D0
void ui_schematic_process(int schematic, int64_t primary_obj, int64_t secondary_obj)
{
    if (ui_callbacks.schematic_process != NULL) {
        ui_callbacks.schematic_process(schematic, primary_obj, secondary_obj);
    }
}

// 0x460900
void ui_schematic_feedback(bool success, int64_t primary_obj, int64_t secondary_obj)
{
    if (ui_callbacks.schematic_feedback != NULL) {
        ui_callbacks.schematic_feedback(success, primary_obj, secondary_obj);
    }
}

// 0x460930
void sub_460930(int64_t a1, int64_t a2)
{
    if (ui_callbacks.field_D0 != NULL) {
        ui_callbacks.field_D0(a1, a2);
    }
}

// 0x460960
void sub_460960(Packet81* pkt)
{
    if (ui_callbacks.field_D4 != NULL) {
        ui_callbacks.field_D4(pkt);
    }
}

// 0x460980
void sub_460980(int64_t a1)
{
    if (ui_callbacks.field_D8 != NULL) {
        ui_callbacks.field_D8(a1);
    }
}

// 0x4609D0
void ui_gameuilib_reset(void)
{
    if (ui_callbacks.gameuilib_reset != NULL) {
        ui_callbacks.gameuilib_reset();
    }
}

// 0x4609E0
void ui_start_dialog(int64_t pc_obj, int64_t npc_obj, int script_num, int script_line, int num)
{
    if (ui_callbacks.start_dialog != NULL) {
        ui_callbacks.start_dialog(pc_obj, npc_obj, script_num, script_line, num);
    }
}

// 0x460A20
void ui_end_dialog(int64_t pc_obj, int a3)
{
    if (ui_callbacks.end_dialog != NULL) {
        ui_callbacks.end_dialog(pc_obj, a3);
    }
}

// 0x460AB0
void sub_460AB0(int a1, int a2, void* a3)
{
    if (ui_callbacks.field_F8 != NULL) {
        ui_callbacks.field_F8(a1, a2, a3);
    }
}

// 0x460AD0
void sub_460AD0(int a1)
{
    if (ui_callbacks.field_FC != NULL) {
        ui_callbacks.field_FC(a1);
    }
}

// 0x460AF0
void ui_refresh_cursor(void)
{
    if (ui_callbacks.refresh_cursor != NULL) {
        ui_callbacks.refresh_cursor();
    }
}

// 0x460B00
int sub_460B00(int a1, TargetDescriptor* td)
{
    if (ui_callbacks.field_104 != NULL) {
        return ui_callbacks.field_104(a1, td);
    } else {
        return 0;
    }
}

// 0x460B20
void sub_460B20(void)
{
    if (ui_callbacks.field_108 != NULL) {
        ui_callbacks.field_108();
    }
}

// 0x460B30
bool ui_wmap_rnd_timeevent_process(TimeEvent* timeevent)
{
    if (ui_callbacks.wmap_rnd_timeevent_process != NULL) {
        return ui_callbacks.wmap_rnd_timeevent_process(timeevent);
    } else {
        return false;
    }
}

// 0x460B50
void sub_460B50(int64_t a1, int64_t a2)
{
    if (ui_callbacks.field_110 != NULL) {
        ui_callbacks.field_110(a1, a2);
    }
}

// 0x460B80
void ui_queue_slide(int slide)
{
    if (ui_callbacks.queue_slide != NULL) {
        ui_callbacks.queue_slide(slide);
    }
}

// 0x460BA0
bool ui_gameuilib_mod_load(void)
{
    if (ui_callbacks.gameuilib_mod_load != NULL) {
        return ui_callbacks.gameuilib_mod_load();
    } else {
        return false;
    }
}

// 0x460BB0
int sub_460BB0(void)
{
    if (ui_callbacks.field_11C != NULL) {
        return ui_callbacks.field_11C();
    } else {
        return 0;
    }
}

// 0x460BC0
int sub_460BC0(void)
{
    // FIXME: Checks the wrong callback for NULL.
    if (ui_callbacks.field_11C != NULL) {
        return ui_callbacks.field_120();
    } else {
        return 0;
    }
}

// 0x460BE0
int sub_460BE0(const char* a1, const char* a2)
{
    if (ui_callbacks.field_124 != NULL) {
        return ui_callbacks.field_124(a1, a2);
    } else {
        return 0;
    }
}

// 0x460C00
void ui_schematic_info_get(int schematic, SchematicInfo* schematic_info)
{
    if (ui_callbacks.schematic_info_get != NULL) {
        ui_callbacks.schematic_info_get(schematic, schematic_info);
    }
}

// 0x460C20
int64_t sub_460C20(void)
{
    if (ui_callbacks.field_12C != NULL) {
        return ui_callbacks.field_12C();
    } else {
        return OBJ_HANDLE_NULL;
    }
}

// 0x460C30
void sub_460C30(int64_t a1)
{
    if (ui_callbacks.field_130 != NULL) {
        ui_callbacks.field_130(a1);
    }
}

// 0x460C50
int sub_460C50(int64_t a1, int64_t a2, int a3)
{
    if (ui_callbacks.field_134 != NULL) {
        return ui_callbacks.field_134(a1, a2, a3);
    } else {
        return 0;
    }
}

// 0x460C80
int sub_460C80(int64_t a1, int64_t a2, int a3)
{
    if (ui_callbacks.field_138 != NULL) {
        return ui_callbacks.field_138(a1, a2, a3);
    } else {
        return 0;
    }
}

// 0x460CB0
void sub_460CB0(int64_t a1, int64_t a2, int a3)
{
    if (ui_callbacks.field_13C != NULL) {
        ui_callbacks.field_13C(a1, a2, a3);
    }
}

// 0x460CE0
bool ui_inven_create(int64_t pc_obj, int64_t target_obj, int mode)
{
    if (ui_callbacks.inven_create != NULL) {
        return ui_callbacks.inven_create(pc_obj, target_obj, mode);
    } else {
        return false;
    }
}

// 0x460D10
void sub_460D10(const char* a1)
{
    if (ui_callbacks.field_144 != NULL) {
        ui_callbacks.field_144(a1);
    }
}

// 0x460D30
void sub_460D30(const char* a1)
{
    if (ui_callbacks.field_148 != NULL) {
        ui_callbacks.field_148(a1);
    }
}

// 0x460D50
void sub_460D50(const char* a1, const char* a2)
{
    if (ui_callbacks.field_14C != NULL) {
        ui_callbacks.field_14C(a1, a2);
    }
}

// 0x460D70
void sub_460D70(const char* a1, const char* a2)
{
    if (ui_callbacks.field_150 != NULL) {
        ui_callbacks.field_150(a1, a2);
    }
}

// 0x460D90
void sub_460D90(int a1, int a2)
{
    if (ui_callbacks.field_154 != NULL) {
        ui_callbacks.field_154(a1, a2);
    }
}

// 0x460DB0
void sub_460DB0(const char* a1, const char* a2)
{
    if (ui_callbacks.field_158 != NULL) {
        ui_callbacks.field_158(a1, a2);
    }
}

// 0x460DD0
void sub_460DD0(void)
{
    if (ui_callbacks.field_15C != NULL) {
        ui_callbacks.field_15C();
    }
}

// 0x460DE0
void ui_show_inven_npc_identify(int64_t pc_obj, int64_t target_obj)
{
    if (ui_callbacks.show_inven_npc_identify != NULL) {
        ui_callbacks.show_inven_npc_identify(pc_obj, target_obj);
    }
}

// 0x460E10
void ui_mp_charedit_cache_traits(int player)
{
    if (ui_callbacks.mp_charedit_cache_traits != NULL) {
        ui_callbacks.mp_charedit_cache_traits(player);
    }
}

// 0x460E30
void ui_mp_charedit_trait_inc(int player, int trait, int param)
{
    if (ui_callbacks.mp_charedit_trait_inc != NULL) {
        ui_callbacks.mp_charedit_trait_inc(player, trait, param);
    }
}

// 0x460E50
void ui_mp_charedit_trait_dec(int player, int trait, int param)
{
    if (ui_callbacks.mp_charedit_trait_dec != NULL) {
        ui_callbacks.mp_charedit_trait_dec(player, trait, param);
    }
}
