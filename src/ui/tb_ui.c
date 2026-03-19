#include "ui/tb_ui.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/critter.h"
#include "game/gamelib.h"
#include "game/hrp.h"
#include "game/magictech.h"
#include "game/mes.h"
#include "game/multiplayer.h"
#include "game/object.h"
#include "game/player.h"
#include "game/script.h"
#include "game/ui.h"
#include "ui/anim_ui.h"
#include "ui/charedit_ui.h"
#include "ui/dialog_ui.h"
#include "ui/follower_ui.h"
#include "ui/gameuilib.h"
#include "ui/hotkey_ui.h"
#include "ui/intgame.h"
#include "ui/inven_ui.h"
#include "ui/item_ui.h"
#include "ui/mainmenu_ui.h"
#include "ui/schematic_ui.h"
#include "ui/skill_ui.h"
#include "ui/sleep_ui.h"
#include "ui/slide_ui.h"
#include "ui/spell_ui.h"
#include "ui/tech_ui.h"
#include "ui/wmap_rnd.h"
#include "ui/wmap_ui.h"
#include "ui/written_ui.h"

static void sub_57CAE0(void);
static void tb_inven_ui_update(int64_t obj);
static void sub_57CAF0(int64_t obj);
static void show_inven_loot(int64_t pc_obj, int64_t target_obj);
static void show_inven_identify(int64_t pc_obj, int64_t target_obj);
static void show_inven_npc_identify(int64_t pc_obj, int64_t target_obj);
static void sub_57CBE0(char* str);
static void refresh_fatigue_bar(int64_t obj);
static void sub_57CC70(int64_t a1, int64_t a2);
static void end_death(void);
static void end_game(void);
static void sub_57CDF0(int a1);
static void sub_57CE00(void);
static void sub_57CE10(void);
static void sub_57CE30(int64_t obj, void* a2, int a3);
static void handle_sector_changed(int64_t sec, int64_t obj);
static void charedit_error_msg(int type, int param);
static void sub_57CF70(int64_t a1, int64_t a2);
static void sub_57CFA0(void);
static bool sub_57CFB0(void);
static bool sub_57D080(void);
static bool sub_57D150(const char* name, const char* a2);

// 0x57C6E0
bool tb_ui_init(GameInitInfo* init_info)
{
    UiCallbacks callbacks;

    (void)init_info;

    callbacks.field_7C = sub_57CBE0;
    callbacks.field_80 = intgame_message_window_display_msg;
    callbacks.field_58 = sub_57CCF0;
    callbacks.spell_add = spell_ui_add;
    callbacks.spell_maintain_add = spell_ui_maintain_add;
    callbacks.spell_maintain_end = spell_ui_maintain_end;
    callbacks.spell_maintain_refresh = spell_ui_maintain_refresh;
    callbacks.field_0 = sub_57CAE0;
    callbacks.field_4 = sub_57CAF0;
    callbacks.update_inven = tb_inven_ui_update;
    callbacks.field_C = NULL;
    callbacks.refresh_fatigue_bar = refresh_fatigue_bar;
    callbacks.refresh_health_bar = intgame_refresh_health_bar;
    callbacks.field_18 = sub_5570A0;
    callbacks.notify_item_inserted_or_removed = intgame_notify_item_inserted_or_removed;
    callbacks.show_inven_loot = show_inven_loot;
    callbacks.show_inven_identify = show_inven_identify;
    callbacks.tech_adjust_degree = tech_ui_adjust_degree;
    callbacks.skill_preprocess = skill_ui_preprocess;
    callbacks.skill_activate = skill_ui_activate;
    callbacks.adjust_skill = skill_ui_adjust_skill;
    callbacks.set_skill_training = skill_ui_set_training;
    callbacks.field_3C = sub_57CC70;
    callbacks.is_in_dialog = dialog_ui_is_in_dialog;
    callbacks.end_death = end_death;
    callbacks.end_game = end_game;
    callbacks.field_64 = sub_57CDF0;
    callbacks.field_68 = sub_57EDF0;
    callbacks.field_6C = sub_57EED0;
    callbacks.field_70 = sub_57CE10;
    callbacks.field_74 = sub_57CE00;
    callbacks.field_78 = sub_57CE30;
    callbacks.field_88 = gameuilib_wants_mainmenu_set;
    callbacks.field_84 = sub_557730;
    callbacks.written_start_obj = written_ui_start_obj;
    callbacks.written_start_type = written_ui_start_type;
    callbacks.item_activate = item_ui_activate;
    callbacks.follower_refresh = follower_ui_refresh;
    callbacks.follower_update = follower_ui_update;
    callbacks.toggle_primary_button = intgame_toggle_primary_button;
    callbacks.set_map_button = intgame_set_map_button;
    callbacks.notify_sector_changed = handle_sector_changed;
    callbacks.written_newspaper_headline = written_ui_newspaper_headline;
    callbacks.sleep_toggle = sleep_ui_toggle;
    callbacks.charedit_error_msg = charedit_error_msg;
    callbacks.charedit_refresh = charedit_refresh;
    callbacks.progressbar_init = mainmenu_ui_progressbar_init;
    callbacks.progressbar_update = mainmenu_ui_progressbar_update;
    callbacks.wmap_select = wmap_ui_select;
    callbacks.schematic_process = schematic_ui_process;
    callbacks.schematic_feedback = schematic_ui_feedback;
    callbacks.field_D0 = sub_57CF70;
    callbacks.field_D4 = sub_578EA0;
    callbacks.field_D8 = sub_573630;
    callbacks.field_DC = sub_57CFA0;
    callbacks.field_E0 = sub_575C50;
    callbacks.gameuilib_reset = gameuilib_reset;
    callbacks.start_dialog = dialog_ui_start_dialog;
    callbacks.end_dialog = dialog_ui_end_dialog;
    callbacks.field_F8 = NULL;
    callbacks.field_FC = sub_549310;
    callbacks.refresh_cursor = intgame_refresh_cursor;
    callbacks.field_104 = sub_57A6C0;
    callbacks.field_108 = sub_575770;
    callbacks.wmap_rnd_timeevent_process = wmap_rnd_timeevent_process;
    callbacks.queue_slide = slide_ui_enqueue;
    callbacks.gameuilib_mod_load = gameuilib_mod_load;
    callbacks.field_11C = sub_57CFB0;
    callbacks.field_120 = sub_57D080;
    callbacks.field_124 = sub_57D150;
    callbacks.schematic_info_get = schematic_ui_info_get;
    callbacks.field_12C = sub_573600;
    callbacks.field_130 = sub_557790;
    callbacks.field_134 = sub_572370;
    callbacks.field_138 = sub_572510;
    callbacks.field_13C = sub_572640;
    callbacks.inven_create = inven_ui_create;
    callbacks.field_150 = NULL;
    callbacks.field_144 = NULL;
    callbacks.field_148 = NULL;
    callbacks.field_14C = NULL;
    callbacks.field_154 = NULL;
    callbacks.field_158 = NULL;
    callbacks.field_15C = NULL;
    callbacks.show_inven_npc_identify = show_inven_npc_identify;
    callbacks.mp_charedit_cache_traits = mp_charedit_cache_traits;
    callbacks.mp_charedit_trait_inc = mp_charedit_trait_inc;
    callbacks.mp_charedit_trait_dec = mp_charedit_trait_dec;
    ui_init(&callbacks);

    return true;
}

// 0x57CA90
void tb_ui_reset(void)
{
}

// 0x57CAA0
void tb_ui_exit(void)
{
    ui_exit();
}

// 0x57CAB0
void tb_inven_ui_update(int64_t obj)
{
    inven_ui_update(obj);
    if (player_is_local_pc_obj(obj)) {
        sub_556E60();
        intgame_counters_refresh();
    }
}

// 0x57CAE0
void sub_57CAE0(void)
{
    iso_interface_refresh();
    charedit_refresh();
}

// 0x57CAF0
void sub_57CAF0(int64_t obj)
{
    sub_573590(obj);
    dialog_ui_notify_object_destroyed(obj);
}

// 0x57CB10
void show_inven_loot(int64_t pc_obj, int64_t target_obj)
{
    if ((obj_field_int32_get(target_obj, OBJ_F_SPELL_FLAGS) & OSF_STONED) != 0) {
        return;
    }

    if (obj_type_is_critter(obj_field_int32_get(target_obj, OBJ_F_TYPE))
        && obj_field_int32_get(target_obj, OBJ_F_CRITTER_INVENTORY_NUM) == 0) {
        intgame_there_is_nothing_to_loot();
        return;
    }

    inven_ui_open(pc_obj, target_obj, INVEN_UI_MODE_LOOT);
}

// 0x57CB80
void show_inven_identify(int64_t pc_obj, int64_t target_obj)
{
    if (player_is_local_pc_obj(pc_obj)) {
        inven_ui_open(pc_obj, target_obj, INVEN_UI_MODE_IDENTIFY);
    }
}

// 0x57CBC0
void show_inven_npc_identify(int64_t pc_obj, int64_t target_obj)
{
    inven_ui_open(pc_obj, target_obj, INVEN_UI_MODE_NPC_IDENTIFY);
}

// 0x57CBE0
void sub_57CBE0(char* str)
{
    UiMessage ui_message;

    ui_message.type = UI_MSG_TYPE_EXCLAMATION;
    ui_message.str = str;
    intgame_message_window_display_msg(&ui_message);
}

// 0x57CC10
void refresh_fatigue_bar(int64_t obj)
{
    if (player_is_local_pc_obj(obj)) {
        anim_ui_event_add(ANIM_UI_EVENT_TYPE_UPDATE_FATIGUE_BAR, -1);

        if (!critter_is_active(obj)) {
            sub_575770();
        }
    }
    if (object_hover_obj_get() == obj) {
        sub_57CCF0(player_get_local_pc_obj(), obj);
    }
    follower_ui_update_obj(obj);
}

// 0x57CC70
void sub_57CC70(int64_t a1, int64_t a2)
{
    int64_t danger_source_obj;

    if (object_script_execute(a1, a2, OBJ_HANDLE_NULL, SAP_DIALOG, 0) == 1) {
        if (ai_surrendered(a2, &danger_source_obj) && critter_party_same(danger_source_obj, a1)) {
            ai_flee(a2, a1);
        } else {
            sub_5681C0(a1, a2);
        }
    }
}

// 0x57CCF0
void sub_57CCF0(int64_t source_obj, int64_t target_obj)
{
    char buffer[2000];

    if (sub_57CD60(source_obj, target_obj, buffer)) {
        intgame_examine_object(source_obj, target_obj, buffer);
        sub_557370(source_obj, target_obj);
    }
}

// 0x57CD60
bool sub_57CD60(int64_t source_obj, int64_t target_obj, char* buffer)
{
    if (!player_is_local_pc_obj(source_obj)) {
        return false;
    }

    if (!object_script_execute(source_obj, target_obj, OBJ_HANDLE_NULL, SAP_EXAMINE, 0)) {
        return false;
    }

    object_examine(target_obj, source_obj, buffer);

    return true;
}

// 0x57CDC0
void end_death(void)
{
    anim_ui_event_add_delay(ANIM_UI_EVENT_TYPE_END_DEATH, -1, 5000);
}

// 0x57CDE0
void end_game(void)
{
    anim_ui_event_add_delay(ANIM_UI_EVENT_TYPE_END_GAME, -1, 50);
}

// 0x57CDF0
void sub_57CDF0(int a1)
{
    sub_57EF90(a1);
}

// 0x57CE00
void sub_57CE00(void)
{
    intgame_counters_refresh();
}

// 0x57CE10
void sub_57CE10(void)
{
    intgame_hotkeys_recover();
    intgame_mode_set(INTGAME_MODE_MAIN);
    intgame_mode_set(INTGAME_MODE_MAIN);

    if (object_hover_obj != OBJ_HANDLE_NULL) {
        dword_5E2E94 = false;
        object_hover_obj = OBJ_HANDLE_NULL;

        object_hover_color = tig_color_make(255, 255, 255);
        tig_art_interface_id_create(467, dword_5E2E6C, 1, 0, &object_hover_underlay_art_id);
        tig_art_interface_id_create(468, dword_5E2E68, 1, 0, &object_hover_overlay_art_id);
        sub_443EB0(OBJ_HANDLE_NULL, &stru_5E2F60);
    }
}

// 0x57CE30
void sub_57CE30(int64_t obj, void* a2, int a3)
{
    if (!sub_45A030(a3)) {
        if (spell_ui_activate(obj, a3) == SPELL_UI_ACTIVATE_OK) {
            spell_ui_apply(a2);
        }
        spell_ui_cancel();
    }
}

// 0x57CE70
void handle_sector_changed(int64_t sec, int64_t obj)
{
    if (obj != OBJ_HANDLE_NULL) {
        if (tig_net_is_active()) {
            if (multiplayer_find_slot_from_obj(obj) != -1) {
                wmap_ui_notify_sector_changed(obj, sec);
            }
        } else {
            if (player_is_local_pc_obj(obj)) {
                wmap_ui_notify_sector_changed(obj, sec);
            }
        }
    }
}

// 0x57CEE0
void charedit_error_msg(int type, int param)
{
    switch (type) {
    case 0:
        charedit_error_not_enough_character_points();
        break;
    case 1:
        charedit_error_not_enough_level();
        break;
    case 2:
        charedit_error_not_enough_intelligence();
        break;
    case 3:
        charedit_error_not_enough_willpower();
        break;
    case 4:
        charedit_error_skill_at_max();
        break;
    case 5:
        charedit_error_not_enough_stat(param);
        break;
    case 6:
        charedit_error_skill_is_zero();
        break;
    case 7:
        charedit_error_skill_at_min();
        break;
    case 8:
        if (charedit_is_created()) {
            charedit_refresh();
        }
        break;
    default:
        tig_debug_printf("UI: ui_charedit_error_msg() unknown error type %d\n", type);
        break;
    }
}

// 0x57CF70
void sub_57CF70(int64_t a1, int64_t a2)
{
    if (player_is_local_pc_obj(a1)) {
        charedit_open(a2, CHAREDIT_MODE_PASSIVE);
    }
}

// 0x57CFA0
void sub_57CFA0(void)
{
    sub_575930();
    intgame_refresh_cursor();
}

// 0x57CFB0
bool sub_57CFB0(void)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    TigWindowModalDialogInfo modal_info;
    TigWindowModalDialogChoice choice;

    mes_load("mes\\MultiPlayer.mes", &mes_file);

    mes_file_entry.num = 1900; // "y"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_OK] = mes_file_entry.str[0];

    mes_file_entry.num = 1901; // "n"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL] = mes_file_entry.str[0];

    mes_file_entry.num = 1902; // "Do you wish to save your character?"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.text = mes_file_entry.str;

    modal_info.process = NULL;
    modal_info.type = TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL;
    modal_info.x = 237;
    modal_info.y = 232;
    modal_info.redraw = gamelib_redraw;
    hrp_center(&(modal_info.x), &(modal_info.y));
    tig_window_modal_dialog(&modal_info, &choice);

    mes_unload(mes_file);

    return choice == TIG_WINDOW_MODAL_DIALOG_CHOICE_OK;
}

// 0x57D080
bool sub_57D080(void)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    TigWindowModalDialogInfo modal_info;
    TigWindowModalDialogChoice choice;

    mes_load("mes\\MultiPlayer.mes", &mes_file);

    mes_file_entry.num = 1900; // "y"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_OK] = mes_file_entry.str[0];

    mes_file_entry.num = 1901; // "n"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL] = mes_file_entry.str[0];

    mes_file_entry.num = 1903; // "Do you wish to export your character to Multiplayer?"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.text = mes_file_entry.str;

    modal_info.process = NULL;
    modal_info.type = TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL;
    modal_info.x = 237;
    modal_info.y = 232;
    modal_info.redraw = gamelib_redraw;
    hrp_center(&(modal_info.x), &(modal_info.y));
    tig_window_modal_dialog(&modal_info, &choice);

    mes_unload(mes_file);

    return choice == TIG_WINDOW_MODAL_DIALOG_CHOICE_OK;
}

// 0x57D150
bool sub_57D150(const char* name, const char* a2)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    char buffer[260];
    TigWindowModalDialogInfo modal_info;
    TigWindowModalDialogChoice choice;

    (void)a2;

    mes_load("mes\\MultiPlayer.mes", &mes_file);

    mes_file_entry.num = 1900; // "y"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_OK] = mes_file_entry.str[0];

    mes_file_entry.num = 1901; // "n"
    mes_get_msg(mes_file, &mes_file_entry);
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL] = mes_file_entry.str[0];

    mes_file_entry.num = 1902; // "The character %s already exists. Do you wish to overwrite the file?""
    mes_get_msg(mes_file, &mes_file_entry);
    snprintf(buffer, sizeof(buffer), mes_file_entry.str, name);
    modal_info.text = buffer;

    modal_info.process = NULL;
    modal_info.type = TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL;
    modal_info.x = 237;
    modal_info.y = 232;
    modal_info.redraw = gamelib_redraw;
    hrp_center(&(modal_info.x), &(modal_info.y));
    tig_window_modal_dialog(&modal_info, &choice);

    mes_unload(mes_file);

    return choice == TIG_WINDOW_MODAL_DIALOG_CHOICE_OK;
}
