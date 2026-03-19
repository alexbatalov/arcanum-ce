#include "ui/dialog_ui.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/broadcast.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/dialog.h"
#include "game/gsound.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
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
static void sub_567D60(DialogUiEntry* entry);
static bool sub_567E30(DialogUiEntry* entry, int a2);
static bool dialog_ui_message_filter(TigMessage* msg);
static bool sub_5681B0(DialogUiEntry* entry);
static bool sub_568280(DialogUiEntry* a1);
static void sub_568480(DialogUiEntry* entry, int a2);
static void sub_5684C0(DialogUiEntry* entry);
static void sub_568540(int64_t npc_obj, int64_t pc_obj, int type, int expires_in, const char* str, int speech_id);
static void sub_5686C0(int64_t pc_obj, int64_t npc_obj, int type, int expires_in, const char* str);
static void dialog_ui_speech_start(int64_t npc_obj, int64_t pc_obj, int speech_id);
static void dialog_ui_speech_stop(void);

// 0x66DAB8
static DialogUiEntry stru_66DAB8[8];

// 0x679DB8
static unsigned char byte_679DB8[8]; // boolean

// 0x67B960
static tig_sound_handle_t dialog_ui_speech_handle;

// 0x67B964
static bool dword_67B964;

// 0x679DC0
static struct {
    PacketDialog pkt;
    char buffer[6000];
} stru_679DC0;

// 0x567330
bool dialog_ui_init(GameInitInfo* init_info)
{
    int index;

    (void)init_info;

    memset(byte_679DB8, 0, sizeof(byte_679DB8));

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

    if (dword_67B964) {
        intgame_dialog_end();
        dword_67B964 = false;
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

    if (tig_net_is_active()) {
        index = multiplayer_find_slot_from_obj(obj);
    }

    return &(stru_66DAB8[index]);
}

// 0x567460
void dialog_ui_start_dialog(int64_t pc_obj, int64_t npc_obj, int script_num, int script_line, int num)
{
    DialogUiEntry* entry;
    char path[TIG_MAX_PATH];
    char str[2000];
    PacketDialog pkt;

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
    if (multiplayer_is_locked() || tig_net_is_host()) {
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
                sub_568540(entry->state.npc_obj,
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

                dword_67B964 = true;

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

            if (tig_net_is_active()
                && tig_net_is_host()) {
                pkt.type = 44;
                pkt.subtype = 0;
                pkt.d.d.field_8 = obj_get_id(pc_obj);
                pkt.d.d.field_20 = obj_get_id(npc_obj);
                pkt.d.d.field_38 = script_num;
                pkt.d.d.field_3C = script_line;
                pkt.d.d.field_40 = num;
                tig_net_send_app_all(&pkt, sizeof(pkt));
            }

            sub_5684C0(entry);
            sub_567D60(entry);
        } else {
            sub_5681C0(pc_obj, npc_obj);
        }
    } else {
        if (script_num != 0
            && script_name_build_dlg_name(script_num, path)
            && player_is_local_pc_obj(pc_obj)) {
            if (intgame_dialog_begin(dialog_ui_message_filter)) {
                entry->state.num = num;
                entry->state.pc_obj = pc_obj;
                entry->state.npc_obj = npc_obj;
                entry->state.script_num = script_num;
                entry->script_num = script_num;
                entry->script_line = script_line;
                entry->field_1850 = 1;

                dword_67B964 = true;
            }
        }
    }
}

// 0x5678D0
void dialog_ui_end_dialog(int64_t obj, int a2)
{
    DialogUiEntry* entry;
    PacketDialog pkt;

    (void)a2;

    entry = sub_567420(obj);
    if (!entry->field_1850) {
        return;
    }

    entry->field_1850 = false;

    if (player_is_local_pc_obj(obj)) {
        intgame_dialog_end();
        dword_67B964 = 0;
    }

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        dialog_unload(entry->dlg);
    }

    tb_expire_in(entry->state.npc_obj, TB_EXPIRE_DEFAULT);

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        sub_413280(&(entry->state));
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 44;
        pkt.subtype = 1;
        pkt.d.b.field_8 = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x5679C0
void sub_5679C0(DialogUiEntry* entry)
{
    if (!entry->field_1850) {
        return;
    }

    entry->field_1850 = false;

    if (!tig_net_is_active() || tig_net_is_host()) {
        dialog_unload(entry->dlg);
    }

    if (!tig_net_is_active() || tig_net_is_host()) {
        sub_413280(&(entry->state));
    }
}

// 0x567A10
int sub_567A10(void)
{
    return dword_67B964;
}

// 0x567A20
void sub_567A20(int64_t obj)
{
    sub_567420(obj)->field_1850 = false;
    if (player_is_local_pc_obj(obj)) {
        intgame_dialog_end();
        dword_67B964 = false;
    }
}

// 0x567A60
void sub_567A60(int64_t obj)
{
    sub_567420(obj)->field_1850 = true;
    if (player_is_local_pc_obj(obj)) {
        if (intgame_dialog_begin(dialog_ui_message_filter)) {
            dword_67B964 = true;
        }
    }
}

// 0x567AB0
void sub_567AB0(DialogUiEntry* entry, DialogSerializedData* serialized_data, char* buffer)
{
    int index;
    int pos;

    if (entry->state.pc_obj != OBJ_HANDLE_NULL) {
        serialized_data->field_8 = obj_get_id(entry->state.pc_obj);
    } else {
        serialized_data->field_8.type = OID_TYPE_NULL;
    }

    if (entry->state.npc_obj != OBJ_HANDLE_NULL) {
        serialized_data->field_20 = obj_get_id(entry->state.npc_obj);
    } else {
        serialized_data->field_20.type = OID_TYPE_NULL;
    }

    serialized_data->field_3C = entry->state.script_num;
    serialized_data->field_40 = 0;
    serialized_data->field_44 = (int)strlen(entry->state.reply) + 1;
    strncpy(buffer, entry->state.reply, serialized_data->field_44);
    serialized_data->field_78 = entry->state.field_17E8;
    serialized_data->field_7C = entry->state.field_17EC;

    pos = serialized_data->field_44;
    for (index = 0; index < 5; index++) {
        serialized_data->field_50[index] = pos;
        serialized_data->field_64[index] = (int)strlen(entry->state.options[index]) + 1;
        strncpy(&(buffer[pos]), entry->state.options[index], serialized_data->field_64[index]);
        serialized_data->field_80[index] = entry->state.field_17F0[index];
        serialized_data->field_94[index] = entry->state.field_1804[index];
        serialized_data->field_A8[index] = entry->state.field_1818[index];
    }

    serialized_data->field_BC = entry->state.field_1840;
    serialized_data->field_C0 = entry->state.seed;
}

// 0x567C30
void sub_567C30(DialogSerializedData* serialized_data, DialogUiEntry* entry, const char* buffer)
{
    int index;

    if (serialized_data->field_8.type != OID_TYPE_NULL) {
        entry->state.pc_obj = objp_perm_lookup(serialized_data->field_8);
    } else {
        entry->state.pc_obj = OBJ_HANDLE_NULL;
    }

    if (serialized_data->field_20.type != OID_TYPE_NULL) {
        entry->state.npc_obj = objp_perm_lookup(serialized_data->field_20);
    } else {
        entry->state.npc_obj = OBJ_HANDLE_NULL;
    }

    entry->state.script_num = serialized_data->field_3C;
    entry->state.num = serialized_data->field_38;
    strncpy(entry->state.reply, &(buffer[serialized_data->field_40]), serialized_data->field_44);
    entry->state.num_options = serialized_data->field_4C;
    entry->state.field_17E8 = serialized_data->field_78;
    entry->state.field_17EC = serialized_data->field_7C;

    for (index = 0; index < 5; index++) {
        strncpy(entry->state.options[index], &(buffer[serialized_data->field_50[index]]), serialized_data->field_64[index]);
        entry->state.field_17F0[index] = serialized_data->field_80[index];
        entry->state.field_1804[index] = serialized_data->field_94[index];
        entry->state.field_1818[index] = serialized_data->field_A8[index];
    }

    entry->state.field_1840 = serialized_data->field_BC;
    entry->state.seed = serialized_data->field_C0;
}

// 0x567D60
void sub_567D60(DialogUiEntry* entry)
{
    int size;
    int index;

    if (tig_net_is_active()
        && tig_net_is_host()) {
        stru_679DC0.pkt.type = 44;
        stru_679DC0.pkt.subtype = 3;
        stru_679DC0.pkt.d.e.field_8 = entry->slot;
        stru_679DC0.pkt.d.e.field_C = entry->field_1850;
        stru_679DC0.pkt.d.e.field_10 = entry->script_num;
        stru_679DC0.pkt.d.e.field_14 = entry->script_line;
        sub_567AB0(entry, &(stru_679DC0.pkt.d.e.serialized_data), stru_679DC0.buffer);

        size = sizeof(stru_679DC0) + stru_679DC0.pkt.d.e.serialized_data.field_44;
        for (index = 0; index < 5; index++) {
            size += stru_679DC0.pkt.d.e.serialized_data.field_64[index];
        }

        tig_net_send_app_all(&stru_679DC0, size);
    }
}

// 0x567E30
bool sub_567E30(DialogUiEntry* entry, int a2)
{
    bool is_pc;

    is_pc = player_is_local_pc_obj(entry->state.pc_obj);
    sub_5686C0(entry->state.pc_obj,
        entry->state.npc_obj,
        TB_TYPE_GREEN,
        TB_EXPIRE_DEFAULT,
        entry->state.options[a2]);
    tb_remove(entry->state.npc_obj);
    dialog_ui_speech_stop();
    sub_413130(&(entry->state), a2);
    sub_567D60(entry);

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
        if (tig_net_is_active()) {
            dialog_ui_end_dialog(entry->state.pc_obj, 0);
        }
        break;
    case 4:
        sub_568540(entry->state.npc_obj,
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
        if (tig_net_is_active()) {
            dialog_ui_end_dialog(entry->state.pc_obj, 0);
        }
        break;
    case 9:
        if (is_pc) {
            intgame_dialog_clear();
            inven_ui_open(entry->state.pc_obj, entry->state.npc_obj, INVEN_UI_MODE_NPC_REPAIR);
        }
        if (tig_net_is_active()) {
            dialog_ui_end_dialog(entry->state.pc_obj, 0);
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
    PacketDialog pkt;

    entry = sub_567420(player_get_local_pc_obj());
    if (sub_5681B0(entry)) {
        return false;
    }

    option = intgame_dialog_get_option(msg);
    if (option == -1) {
        sub_5517A0(msg);
        return true;
    }

    if (multiplayer_is_locked() || tig_net_is_host()) {
        if (!sub_567E30(entry, option)) {
            sub_5517A0(msg);
        }
        return true;
    }

    player = multiplayer_find_slot_from_obj(player_get_local_pc_obj());
    if (byte_679DB8[player] != 1) {
        byte_679DB8[player] = 1;

        pkt.type = 44;
        pkt.subtype = 2;
        pkt.d.f.field_8 = obj_get_id(player_get_local_pc_obj());
        pkt.d.f.field_20 = entry->slot;
        pkt.d.f.field_24 = option;
        tig_net_send_app_all(&pkt, sizeof(pkt));
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
    sub_568540(npc_obj, pc_obj, TB_TYPE_WHITE, TB_EXPIRE_DEFAULT, text, -1);
}

// 0x568280
bool sub_568280(DialogUiEntry* a1)
{
    bool is_pc;

    is_pc = player_is_local_pc_obj(a1->state.pc_obj);

    if (tig_net_is_active() && !tig_net_is_host()) {
        byte_679DB8[multiplayer_find_slot_from_obj(player_get_local_pc_obj())] = 0;
    }

    switch (a1->state.field_17E8) {
    case 0:
        sub_5684C0(a1);
        break;
    case 1:
    case 2:
        dialog_ui_end_dialog(a1->state.pc_obj, 0);
        break;
    case 3:
        if (is_pc) {
            intgame_dialog_clear();
            inven_ui_open(a1->state.pc_obj, a1->state.npc_obj, INVEN_UI_MODE_BARTER);
        }
        break;
    case 5:
        if (is_pc) {
            intgame_dialog_clear();
            charedit_open(a1->state.npc_obj, CHAREDIT_MODE_PASSIVE);
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
            schematic_ui_toggle(a1->state.npc_obj, a1->state.pc_obj);
        }
        break;
    case 8:
        if (is_pc) {
            intgame_dialog_clear();
            ui_show_inven_npc_identify(a1->state.pc_obj, a1->state.npc_obj);
        }
        break;
    case 9:
        if (is_pc) {
            intgame_dialog_clear();
            inven_ui_open(a1->state.pc_obj, a1->state.npc_obj, INVEN_UI_MODE_NPC_REPAIR);
        }
        break;
    }

    return true;
}

// 0x568430
void dialog_ui_float_line(int64_t npc_obj, int64_t pc_obj, const char* str, int speech_id)
{
    int type;

    type = obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC
        ? TB_TYPE_WHITE
        : TB_TYPE_GREEN;
    sub_568540(npc_obj, pc_obj, type, TB_EXPIRE_DEFAULT, str, speech_id);
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

    sub_568540(entry->state.npc_obj,
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
void sub_568540(int64_t npc_obj, int64_t pc_obj, int type, int expires_in, const char* str, int speech_id)
{
    if (!multiplayer_is_locked()) {
        PacketDialog pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 44;
        pkt.subtype = 4;
        pkt.d.d.field_8 = obj_get_id(npc_obj);
        pkt.d.d.field_20 = obj_get_id(pc_obj);
        pkt.d.d.field_38 = type;
        pkt.d.d.field_3C = expires_in;
        pkt.d.d.field_40 = 0;
        strncpy(pkt.d.d.field_44, str, 1000);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

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
void sub_5686C0(int64_t pc_obj, int64_t npc_obj, int type, int expires_in, const char* str)
{
    if (!multiplayer_is_locked()) {
        PacketDialog pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 44;
        pkt.subtype = 4;
        pkt.d.d.field_8 = obj_get_id(pc_obj);
        pkt.d.d.field_20 = obj_get_id(npc_obj);
        pkt.d.d.field_38 = type;
        pkt.d.d.field_3C = expires_in;
        pkt.d.d.field_40 = 1;
        strncpy(pkt.d.d.field_44, str, 1000);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

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
void sub_568830(int64_t obj)
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
