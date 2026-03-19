#include "ui/dialog_ui.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/broadcast.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/dialog.h"
#include "game/gsound.h"
#include "game/obj_private.h"
#include "game/player.h"
#include "game/script.h"
#include "game/script_name.h"
#include "game/stat.h"
#include "game/tb.h"
#include "ui/charedit_ui.h"
#include "ui/intgame.h"
#include "ui/inven_ui.h"
#include "ui/schematic_ui.h"
#include "ui/tb_ui.h"
#include "ui/wmap_ui.h"

#define MAX_ENTRIES 8

typedef struct DialogUiEntry {
    /* 0000 */ int slot;
    /* 0004 */ int dlg;
    /* 0008 */ DialogState state;
    /* 1850 */ int field_1850;
    /* 1854 */ int script_num;
    /* 1858 */ int script_line;
    /* 185C */ char field_185C;
    /* 185D */ char field_185D;
    /* 185E */ char field_185E;
    /* 185F */ char field_185F;
} DialogUiEntry;

static DialogUiEntry* sub_567420(int64_t obj);
static void sub_5679C0(DialogUiEntry* entry);
static bool dialog_ui_process_option(DialogUiEntry* entry, int option);
static bool dialog_ui_message_filter(TigMessage* msg);
static bool sub_5681B0(DialogUiEntry* entry);
static void sub_568480(DialogUiEntry* entry, int a2);
static void sub_5684C0(DialogUiEntry* entry);
static void dialog_ui_npc_say(int64_t npc_obj, int64_t pc_obj, int type, int expires_in, const char* str, int speech_id);
static void dialog_ui_pc_say(int64_t pc_obj, int64_t npc_obj, int type, int expires_in, const char* str);
static void dialog_ui_speech_start(int64_t npc_obj, int64_t pc_obj, int speech_id);
static void dialog_ui_speech_stop(void);

// 0x66DAB8
static DialogUiEntry stru_66DAB8[8];

// 0x67B960
static tig_sound_handle_t dialog_ui_speech_handle;

// 0x67B964
static bool dialog_ui_local_pc_dialog_active;

// 0x567330
bool dialog_ui_init(GameInitInfo* init_info)
{
    int index;

    (void)init_info;

    for (index = 0; index < MAX_ENTRIES; index++) {
        stru_66DAB8[index].field_1850 = false;
        stru_66DAB8[index].slot = index;
    }

    script_set_callbacks(dialog_ui_start_dialog, dialog_ui_float_line);
    ai_set_callbacks(dialog_ui_end_dialog, dialog_ui_float_line);
    broadcast_set_float_line_func(dialog_ui_float_line);
    dialog_ui_speech_handle = TIG_SOUND_HANDLE_INVALID;

    return true;
}

// 0x5673A0
void dialog_ui_exit(void)
{
    dialog_ui_speech_stop();
}

// 0x5673B0
void dialog_ui_reset(void)
{
    int index;

    if (dialog_ui_local_pc_dialog_active) {
        intgame_dialog_end();
        dialog_ui_local_pc_dialog_active = false;
    }

    for (index = 0; index < MAX_ENTRIES; index++) {
        if (stru_66DAB8[index].field_1850) {
            sub_5679C0(&(stru_66DAB8[index]));
        }
    }
}

// 0x567400
bool dialog_ui_is_in_dialog(int64_t obj)
{
    return sub_567420(obj)->field_1850;
}

// 0x567420
DialogUiEntry* sub_567420(int64_t obj)
{
    int index = 0;

    return &(stru_66DAB8[index]);
}

// 0x567460
void dialog_ui_start_dialog(int64_t pc_obj, int64_t npc_obj, int script_num, int script_line, int num)
{
    DialogUiEntry* entry;
    char path[TIG_MAX_PATH];
    char str[2000];

    if (critter_is_dead(pc_obj)) {
        return;
    }

    if (critter_is_unconscious(pc_obj)) {
        return;
    }

    if (ai_can_speak(npc_obj, pc_obj, false) != AI_SPEAK_OK) {
        return;
    }

    if (player_is_local_pc_obj(pc_obj) && intgame_mode_get() == INTGAME_MODE_DIALOG) {
        return;
    }

    if (combat_critter_is_combat_mode_active(pc_obj)) {
        if (!combat_can_exit_combat_mode(pc_obj)) {
            return;
        }
        combat_critter_deactivate_combat_mode(pc_obj);
    }

    entry = sub_567420(pc_obj);

    if (script_num != 0 && script_name_build_dlg_name(script_num, path)) {
        if (!dialog_load(path, &(entry->dlg))) {
            return;
        }

        entry->state.dlg = entry->dlg;
        entry->state.pc_obj = pc_obj;
        entry->state.npc_obj = npc_obj;
        entry->state.num = num;
        entry->state.script_num = script_num;
        if (!sub_412FD0(&(entry->state))) {
            dialog_unload(entry->dlg);
            return;
        }

        if (entry->state.field_17E8 == 4) {
            dialog_ui_npc_say(entry->state.npc_obj,
                entry->state.pc_obj,
                TB_TYPE_WHITE,
                TB_EXPIRE_DEFAULT,
                entry->state.reply,
                entry->state.speech_id);
            sub_413280(&(entry->state));
            dialog_unload(entry->dlg);
            return;
        }

        if (player_is_local_pc_obj(pc_obj)) {
            if (!intgame_dialog_begin(dialog_ui_message_filter)) {
                sub_413280(&(entry->state));
                dialog_unload(entry->dlg);
                return;
            }

            dialog_ui_local_pc_dialog_active = true;

            if (!intgame_is_compact_interface()) {
                sub_57CD60(pc_obj, npc_obj, str);
                intgame_examine_object(pc_obj, npc_obj, str);
                object_hover_obj_set(npc_obj);
            }
        }

        sub_424070(pc_obj, 3, 0, true);
        anim_goal_rotate(pc_obj, object_rot(pc_obj, npc_obj));

        if (critter_is_concealed(pc_obj)) {
            critter_set_concealed(pc_obj, false);
        }

        if (critter_is_concealed(npc_obj)) {
            critter_set_concealed(npc_obj, false);
        }

        entry->script_num = script_num;
        entry->script_line = script_line;
        entry->field_1850 = 1;

        sub_5684C0(entry);
    } else {
        sub_5681C0(pc_obj, npc_obj);
    }
}

// 0x5678D0
void dialog_ui_end_dialog(int64_t obj, int a2)
{
    DialogUiEntry* entry;

    (void)a2;

    entry = sub_567420(obj);
    if (!entry->field_1850) {
        return;
    }

    entry->field_1850 = false;

    if (player_is_local_pc_obj(obj)) {
        intgame_dialog_end();
        dialog_ui_local_pc_dialog_active = 0;
    }

    dialog_unload(entry->dlg);

    tb_expire_in(entry->state.npc_obj, TB_EXPIRE_DEFAULT);

    sub_413280(&(entry->state));
}

// 0x5679C0
void sub_5679C0(DialogUiEntry* entry)
{
    if (!entry->field_1850) {
        return;
    }

    entry->field_1850 = false;

    dialog_unload(entry->dlg);

    sub_413280(&(entry->state));
}

// 0x567A10
bool dialog_ui_is_local_pc_in_dialog(void)
{
    return dialog_ui_local_pc_dialog_active;
}

// 0x567A20
void dialog_ui_notify_dialog_ended(int64_t obj)
{
    sub_567420(obj)->field_1850 = false;
    if (player_is_local_pc_obj(obj)) {
        intgame_dialog_end();
        dialog_ui_local_pc_dialog_active = false;
    }
}

// 0x567A60
void dialog_ui_notify_dialog_started(int64_t obj)
{
    sub_567420(obj)->field_1850 = true;
    if (player_is_local_pc_obj(obj)) {
        if (intgame_dialog_begin(dialog_ui_message_filter)) {
            dialog_ui_local_pc_dialog_active = true;
        }
    }
}

// 0x567E30
bool dialog_ui_process_option(DialogUiEntry* entry, int option)
{
    bool is_pc;

    is_pc = player_is_local_pc_obj(entry->state.pc_obj);
    dialog_ui_pc_say(entry->state.pc_obj,
        entry->state.npc_obj,
        TB_TYPE_GREEN,
        TB_EXPIRE_DEFAULT,
        entry->state.options[option]);
    tb_remove(entry->state.npc_obj);
    dialog_ui_speech_stop();
    sub_413130(&(entry->state), option);

    switch (entry->state.field_17E8) {
    case 0:
        sub_5684C0(entry);
        break;
    case 1:
        dialog_ui_end_dialog(entry->state.pc_obj, 0);
        sub_568480(entry, 0);
        break;
    case 2:
        dialog_ui_end_dialog(entry->state.pc_obj, 0);
        sub_568480(entry, entry->state.field_17EC);
        break;
    case 3:
        if (is_pc) {
            intgame_dialog_clear();
            inven_ui_open(entry->state.pc_obj, entry->state.npc_obj, INVEN_UI_MODE_BARTER);
        }
        break;
    case 4:
        dialog_ui_npc_say(entry->state.npc_obj,
            entry->state.pc_obj,
            TB_TYPE_WHITE,
            TB_EXPIRE_DEFAULT,
            entry->state.reply,
            entry->state.speech_id);
        dialog_ui_end_dialog(entry->state.pc_obj, 0);
        break;
    case 5:
        if (is_pc) {
            intgame_dialog_clear();
            charedit_open(entry->state.npc_obj, CHAREDIT_MODE_PASSIVE);
        }
        break;
    case 6:
        if (is_pc) {
            intgame_dialog_clear();
            wmap_ui_open();
        }
        break;
    case 7:
        if (is_pc) {
            intgame_dialog_clear();
            schematic_ui_toggle(entry->state.npc_obj, entry->state.pc_obj);
        }
        break;
    case 8:
        if (is_pc) {
            intgame_dialog_clear();
            ui_show_inven_npc_identify(entry->state.pc_obj, entry->state.npc_obj);
        }
        break;
    case 9:
        if (is_pc) {
            intgame_dialog_clear();
            inven_ui_open(entry->state.pc_obj, entry->state.npc_obj, INVEN_UI_MODE_NPC_REPAIR);
        }
        break;
    }

    return true;
}

// 0x5680A0
bool dialog_ui_message_filter(TigMessage* msg)
{
    DialogUiEntry* entry;
    int option;
    int player;

    entry = sub_567420(player_get_local_pc_obj());
    if (sub_5681B0(entry)) {
        return false;
    }

    option = intgame_dialog_get_option(msg);
    if (option == -1) {
        sub_5517A0(msg);
        return true;
    }

    if (!dialog_ui_process_option(entry, option)) {
        sub_5517A0(msg);
    }

    return true;
}

// 0x5681B0
bool sub_5681B0(DialogUiEntry* entry)
{
    (void)entry;

    return false;
}

// 0x5681C0
void sub_5681C0(int64_t pc_obj, int64_t npc_obj)
{
    char text[1000];

    sub_4132A0(npc_obj, pc_obj, text);
    dialog_ui_npc_say(npc_obj, pc_obj, TB_TYPE_WHITE, TB_EXPIRE_DEFAULT, text, -1);
}

// 0x568430
void dialog_ui_float_line(int64_t npc_obj, int64_t pc_obj, const char* str, int speech_id)
{
    int type;

    type = obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC
        ? TB_TYPE_WHITE
        : TB_TYPE_GREEN;
    dialog_ui_npc_say(npc_obj, pc_obj, type, TB_EXPIRE_DEFAULT, str, speech_id);
}

// 0x568480
void sub_568480(DialogUiEntry* entry, int a2)
{
    if (a2 == 0) {
        a2 = entry->script_line + 1;
    }

    object_script_execute(entry->state.pc_obj, entry->state.npc_obj, OBJ_HANDLE_NULL, SAP_DIALOG, a2);
}

// 0x5684C0
void sub_5684C0(DialogUiEntry* entry)
{
    int index;

    dialog_ui_npc_say(entry->state.npc_obj,
        entry->state.pc_obj,
        TB_TYPE_WHITE,
        TB_EXPIRE_NEVER,
        entry->state.reply,
        entry->state.speech_id);

    if (player_is_local_pc_obj(entry->state.pc_obj)) {
        intgame_dialog_clear();

        for (index = 0; index < entry->state.num_options; index++) {
            intgame_dialog_set_option(index, entry->state.options[index]);
        }
    }
}

// 0x568540
void dialog_ui_npc_say(int64_t npc_obj, int64_t pc_obj, int type, int expires_in, const char* str, int speech_id)
{
    if (pc_obj != OBJ_HANDLE_NULL
        && !critter_is_dead(npc_obj)
        && !sub_423300(npc_obj, NULL)) {
        sub_424070(npc_obj, 3, 0, 1);
        anim_goal_rotate(npc_obj, object_rot(npc_obj, pc_obj));
    }

    tb_remove(npc_obj);
    tb_add(npc_obj, type, str);
    tb_expire_in(npc_obj, expires_in);

    dialog_ui_speech_start(npc_obj, pc_obj, speech_id);
}

// 0x5686C0
void dialog_ui_pc_say(int64_t pc_obj, int64_t npc_obj, int type, int expires_in, const char* str)
{
    if (npc_obj != OBJ_HANDLE_NULL
        && !critter_is_dead(pc_obj)
        && !sub_423300(pc_obj, NULL)) {
        sub_424070(pc_obj, 3, 0, 1);
        anim_goal_rotate(pc_obj, object_rot(pc_obj, npc_obj));
    }

    if (!player_is_local_pc_obj(pc_obj)) {
        tb_remove(pc_obj);
        tb_add(pc_obj, type, str);
        tb_expire_in(pc_obj, expires_in);
    }
}

// 0x568830
void dialog_ui_notify_object_destroyed(int64_t obj)
{
    int index;

    for (index = 0; index < 8; index++) {
        if (stru_66DAB8[index].state.pc_obj == obj
            || stru_66DAB8[index].state.npc_obj == obj) {
            dialog_ui_end_dialog(obj, 0);
        }
    }
}

// 0x5688D0
void dialog_ui_speech_start(int64_t npc_obj, int64_t pc_obj, int speech_id)
{
    int v1;
    int v2;
    char gender;
    char path[TIG_MAX_PATH];

    (void)npc_obj;

    if (speech_id == -1) {
        return;
    }

    dialog_ui_speech_stop();
    sub_418A00(speech_id, &v1, &v2);

    gender = stat_level_get(pc_obj, STAT_GENDER) != 0 ? 'm' : 'f';
    sprintf(path, "sound\\speech\\%.5d\\v%d_%c.mp3", v1, v2, gender);
    if (gender == 'f' && !tig_file_exists(path, NULL)) {
        sprintf(path, "sound\\speech\\%.5d\\v%d_%c.mp3", v1, v2, 'm');
    }
    if (tig_file_exists(path, NULL)) {
        dialog_ui_speech_handle = gsound_play_voice(path, 0);
    }
}

// 0x5689B0
void dialog_ui_speech_stop(void)
{
    if (dialog_ui_speech_handle != TIG_SOUND_HANDLE_INVALID) {
        tig_sound_destroy(dialog_ui_speech_handle);
        dialog_ui_speech_handle = TIG_SOUND_HANDLE_INVALID;
    }
}
