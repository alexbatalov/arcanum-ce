#include "ui/inven_ui.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/critter.h"
#include "game/dialog.h"
#include "game/gsound.h"
#include "game/hrp.h"
#include "game/item.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/player.h"
#include "game/portrait.h"
#include "game/proto.h"
#include "game/reaction.h"
#include "game/script.h"
#include "game/sfx.h"
#include "game/skill.h"
#include "game/snd.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/target.h"
#include "ui/hotkey_ui.h"
#include "ui/intgame.h"
#include "ui/item_ui.h"
#include "ui/mainmenu_ui.h"
#include "ui/scrollbar_ui.h"
#include "ui/skill_ui.h"
#include "ui/spell_ui.h"
#include "ui/tb_ui.h"

typedef enum InvenUiPanel {
    INVEN_UI_PANEL_INVENTORY,
    INVEN_UI_PANEL_PAPERDOLL,
} InvenUiPanel;

typedef enum InvenUiSummary {
    INVEN_UI_SUMMARY_WEIGHT,
    INVEN_UI_SUMMARY_ENCUMBRANCE,
    INVEN_UI_SUMMARY_INFO_SPEED,
} InvenUiSummary;

typedef enum InvenUiTotalStat {
    INVEN_UI_TOTAL_ATTACK,
    INVEN_UI_TOTAL_DEFENSE,
} InvenUiTotalStat;

typedef struct S5754C0 {
    /* 0000 */ int x;
    /* 0004 */ int y;
    /* 0008 */ int64_t field_8;
    /* 0010 */ int64_t field_10;
    /* 0018 */ int64_t field_18;
    /* 0020 */ int mode;
    /* 0024 */ int field_24;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
} S5754C0;

// Serializeable.
static_assert(sizeof(S5754C0) == 0x30, "wrong size");

static bool sub_572340();
static bool inven_ui_message_filter(TigMessage* msg);
static void sub_574FD0(bool next);
static bool sub_575100(bool* a1);
static bool sub_575180(bool* a1);
static void sub_575200(int a1);
static void sub_575360(int a1);
static void sub_5754C0(int x, int y);
static bool sub_575580(void* userinfo);
static bool sub_5755A0(void* userinfo);
static void sub_575BE0();
static int sub_575CB0(int x, int y, int64_t* a3);
static int64_t sub_575FA0(int x, int y, int64_t* a3);
static void redraw_inven_fix_bad_inven_obj(int64_t a1);
static bool redraw_inven_fix_bad_inven(int64_t a1, int64_t a2);
static void redraw_inven(bool a1);
static tig_art_id_t sub_5782D0();
static void sub_578330(int64_t a1, int64_t a2);
static void sub_5786C0(int64_t obj);
static void sub_578760(int64_t obj);
static void sub_5788C0(int64_t a1, int64_t a2, int a3, int a4);
static void sub_579770(int64_t from_obj, int64_t to_obj);
static bool sub_579840(int64_t obj, bool a2);
static void sub_579B60(int64_t obj);
static void inven_ui_target_inventory_scrollbar_create();
static void inven_ui_target_inventory_scrollbar_destroy();
static void inven_ui_target_inventory_scrollbar_show();
static void inven_ui_target_inventory_scrollbar_hide();
static void sub_579E00(int a1);
static void sub_579E30(TigRect* rect);

// 0x5CAC58
static TigRect inven_ui_inventory_paperdoll_inv_slot_rects[9] = {
    /*    HELMET */ { 151, 107, 64, 64 },
    /*     RING1 */ { 247, 107, 32, 32 },
    /*     RING2 */ { 279, 107, 32, 32 },
    /* MEDALLION */ { 247, 139, 64, 32 },
    /*    WEAPON */ { 23, 170, 96, 128 },
    /*    SHIELD */ { 247, 171, 96, 128 },
    /*     ARMOR */ { 119, 171, 128, 160 },
    /*  GAUNTLET */ { 55, 107, 64, 64 },
    /*     BOOTS */ { 150, 331, 64, 64 },
};

// 0x5CACE8
static TigRect inven_ui_barter_pc_paperdoll_inv_slot_rects[9] = {
    /*    HELMET */ { 499, 107, 64, 64 },
    /*     RING1 */ { 595, 107, 32, 32 },
    /*     RING2 */ { 627, 107, 32, 32 },
    /* MEDALLION */ { 595, 139, 64, 32 },
    /*    WEAPON */ { 371, 170, 96, 128 },
    /*    SHIELD */ { 595, 171, 96, 128 },
    /*     ARMOR */ { 467, 171, 128, 160 },
    /*  GAUNTLET */ { 403, 107, 64, 64 },
    /*     BOOTS */ { 498, 331, 64, 64 },
};

// 0x5CAD78
static TigRect inven_ui_barter_npc_paperdoll_inv_slot_rects[9] = {
    /*    HELMET */ { 157, 182, 42, 42 },
    /*     RING1 */ { 220, 182, 21, 21 },
    /*     RING2 */ { 241, 182, 21, 21 },
    /* MEDALLION */ { 220, 203, 42, 21 },
    /*    WEAPON */ { 73, 223, 63, 85 },
    /*    SHIELD */ { 220, 224, 63, 85 },
    /*     ARMOR */ { 136, 224, 84, 106 },
    /*  GAUNTLET */ { 94, 182, 42, 42 },
    /*     BOOTS */ { 156, 330, 42, 42 },
};

// 0x5CAE08
static TigRect inven_ui_loot_npc_paperdoll_inv_slot_rects[9] = {
    /*    HELMET */ { 157, 182, 42, 42 },
    /*     RING1 */ { 220, 182, 21, 21 },
    /*     RING2 */ { 241, 182, 21, 21 },
    /* MEDALLION */ { 220, 203, 42, 21 },
    /*    WEAPON */ { 73, 223, 63, 85 },
    /*    SHIELD */ { 220, 224, 63, 85 },
    /*     ARMOR */ { 136, 224, 84, 106 },
    /*  GAUNTLET */ { 94, 182, 42, 42 },
    /*     BOOTS */ { 156, 330, 42, 42 },
};

// 0x5CAE98
static TigRect inven_ui_inventory_summary_rects[] = {
    /*      WEIGHT */ { 145, 29, 185, 15 },
    /* ENCUMBRANCE */ { 145, 44, 185, 15 },
    /*  INFO_SPEED */ { 145, 59, 185, 15 },
};

// 0x5CAEC8
static TigRect inven_ui_barter_pc_summary_rects[] = {
    /*      WEIGHT */ { 431, 32, 185, 15 },
    /* ENCUMBRANCE */ { 431, 47, 185, 15 },
    /*  INFO_SPEED */ { 431, 62, 185, 15 },
};

// 0x5CAEF8
static TigRect inven_ui_inventory_total_stat_rects[] = {
    /*  ATTACK */ { 80, 358, 23, 35 },
    /* DEFENSE */ { 261, 358, 23, 35 },
};

// 0x5CAF18
static TigRect inven_ui_barter_pc_total_stat_rects[] = {
    /*  ATTACK */ { 425, 358, 23, 35 },
    /* DEFENSE */ { 609, 358, 23, 35 },
};

// 0x5CAF38
static TigRect inven_ui_barter_npc_total_stat_rects[] = {
    /*  ATTACK */ { 105, 354, 22, 16 },
    /* DEFENSE */ { 227, 354, 22, 16 },
};

// 0x5CAF58
static int item_ui_item_silhouette_nums[] = {
    /*    HELMET */ 241, // cvr_helmet.art
    /*     RING1 */ 246, // cvr_ring1.art
    /*     RING2 */ 247, // cvr_ring2.art
    /* MEDALLION */ 245, // cvr_medalion.art
    /*    WEAPON */ 249, // cvr_weapon.art
    /*    SHIELD */ 248, // cvr_shield.art
    /*     ARMOR */ 244, // cvr_armor.art
    /*  GAUNTLET */ 243, // cvr_gauntlet.art
    /*     BOOTS */ 242, // cvr_boot.art
};

// 0x5CAF80
static TigRect inven_ui_loot_target_portrait_rect = { 183, 33, 100, 78 };

// 0x5CAF90
static TigRect inven_ui_barter_scrollbar_rect = { 330, 168, 17, 224 };

// 0x5CAFA0
static TigRect inven_ui_loot_scrollbar_rect = { 330, 136, 17, 256 };

// 0x5CAFB0
static TigRect inven_ui_npc_action_rect = { 180, 29, 155, 65 };

// 0x5CAFC0
static int inven_ui_gold_ammo_type_x[5] = {
    729,
    729,
    729,
    729,
    729,
};

// 0x5CAFD4
static int inven_ui_gold_ammo_type_y[5] = {
    70,
    93,
    116,
    139,
    162,
};

// 0x6810E0
static int64_t inven_ui_drag_item_obj;

// 0x6810E8
static int dword_6810E8;

// 0x6810F0
static ScrollbarId inven_ui_target_inventory_scrollbar;

// 0x6810F8
static mes_file_handle_t inven_ui_mes_file;

// 0x6810FC
static bool dword_6810FC;

// 0x681100
static tig_button_handle_t inven_ui_cycle_left_btn;

// 0x681108
static TigRect inven_ui_gamble_box_frame;

// 0x681118
static bool inven_ui_have_use_box;

// 0x68111C
static int dword_68111C[120];

// 0x6812FC
static char byte_6812FC[128];

// 0x681390
static tig_font_handle_t dword_681390;

// 0x68137C
static tig_window_handle_t inven_ui_window_handle;

// 0x681380
static TigRect inven_ui_use_box_frame;

// 0x681394
static bool inven_ui_have_drop_box;

// 0x681398
static tig_button_handle_t inven_ui_cycle_right_btn;

// 0x68139C
static bool inven_ui_created;

// 0x6813A0
static tig_button_handle_t inven_ui_target_total_attack_btn;

// 0x6813A4
static int inven_ui_drop_box_image;

// 0x6813A8
static int64_t qword_6813A8;

// 0x6813B0
static TigRect inven_ui_drop_box_frame;

// 0x6813C0
static char byte_6813C0[128];

// 0x681440
static int dword_681440;

// 0x681444
static tig_button_handle_t inven_ui_arrange_items_btn;

// 0x681448
static bool inven_ui_have_gamble_box;

// 0x681450
static int64_t qword_681450;

// 0x681458
static int64_t qword_681458;

// 0x681460
static tig_button_handle_t inven_ui_total_attack_value_btn;

// 0x681464
static tig_button_handle_t inven_ui_take_all_btn;

// 0x681468
static char byte_681468[128];

// 0x6814E8
static tig_button_handle_t inven_ui_total_defence_image_btn;

// 0x6814EC
static tig_button_handle_t inven_ui_inventory_btn;

// 0x6814F0
static bool inven_ui_arrange_vertical;

// 0x6814F8
static int64_t inven_ui_pc_obj;

// 0x681500
static tig_button_handle_t inven_ui_target_paperdoll_btn;

// 0x681504
static int inven_ui_use_box_image;

// 0x681508
static int dword_681508;

// 0x68150C
static InvenUiPanel inven_ui_panel;

// 0x681510
static int dword_681510;

// 0x681514
static InvenUiPanel inven_ui_target_panel;

// 0x681518
static int dword_681518[960];

// 0x682418
static tig_font_handle_t dword_682418;

// 0x68241C
static char byte_68241C[1000];

// 0x682804
static char byte_682804[1000];

// 0x682BEC
static char byte_682BEC[128];

// 0x682C6C
static tig_button_handle_t inven_ui_total_attack_image_btn;

// 0x682C70
static int inven_ui_gamble_box_image;

// 0x682C74
static tig_font_handle_t dword_682C74;

// 0x682C78
static int64_t qword_682C78;

// 0x682C80
static tig_button_handle_t inven_ui_paperdoll_btn;

// 0x682C84
static tig_button_handle_t inven_ui_target_total_defence_btn;

// 0x682C88
static tig_button_handle_t inven_ui_total_defence_value_btn;

// 0x682C8C
static char byte_682C8C[MAX_STRING];

// 0x68345C
static tig_font_handle_t dword_68345C;

// 0x683460
static tig_button_handle_t inven_ui_target_inventory_btn;

// 0x683464
static InvenUiMode inven_ui_mode;

// 0x68346C
static bool dword_68346C;

// 0x683470
static int dword_683470;

// 0x739F58
static int dword_739F58;

// 0x739F60
static int dword_739F60;

// 0x739F68
static int64_t qword_739F68;

// 0x739F70
static int64_t qword_739F70;

// 0x739F78
static int64_t qword_739F78;

// 0x739F80
static int dword_739F80;

// 0x739F84
static int dword_739F84;

// 0x572060
bool inven_ui_init(GameInitInfo* init_info)
{
    TigFont font;

    (void)init_info;

    if (!mes_load("mes\\inven_ui.mes", &inven_ui_mes_file)) {
        return false;
    }

    font.flags = 0;
    tig_art_interface_id_create(27, 0, 0, 0, &(font.art_id));
    font.str = NULL;
    font.color = tig_color_make(255, 255, 255);
    tig_font_create(&font, &dword_682418);

    tig_art_interface_id_create(229, 0, 0, 0, &(font.art_id));
    tig_font_create(&font, &dword_682C74);

    tig_art_interface_id_create(486, 0, 0, 0, &(font.art_id));
    tig_font_create(&font, &dword_681390);

    font.flags = TIG_FONT_CENTERED;
    tig_art_interface_id_create(171, 0, 0, 0, &(font.art_id));
    tig_font_create(&font, &dword_68345C);

    return true;
}

// 0x572190
void inven_ui_exit()
{
    tig_font_destroy(dword_682418);
    tig_font_destroy(dword_682C74);
    tig_font_destroy(dword_681390);
    tig_font_destroy(dword_68345C);
    mes_unload(inven_ui_mes_file);
}

// 0x5721D0
void inven_ui_reset()
{
    if (inven_ui_created) {
        inven_ui_destroy();
    }

    qword_682C78 = 0;
    qword_6813A8 = 0;
    inven_ui_pc_obj = OBJ_HANDLE_NULL;
    inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
    qword_681450 = 0;
    qword_681458 = 0;
    dword_6810E8 = -1;
}

// 0x572240
bool inven_ui_open(int64_t pc_obj, int64_t target_obj, int mode)
{
    if (sub_572340()) {
        return true;
    }

    if (tig_net_is_active() && !tig_net_is_host()) {
        Packet100 pkt;

        pkt.type = 100;
        pkt.subtype = 12;
        sub_4F0640(pc_obj, &(pkt.d.z.field_8));
        sub_4F0640(target_obj, &(pkt.d.z.field_20));
        pkt.d.z.field_38 = mode;
        pkt.d.z.field_3C = 0;
        tig_net_send_app_all(&pkt, sizeof(pkt));

        return true;
    }

    if (!sub_572370(pc_obj, target_obj, mode)) {
        return false;
    }

    if (!sub_572510(pc_obj, target_obj, mode)) {
        return false;
    }

    sub_572640(pc_obj, target_obj, mode);
    return inven_ui_create(pc_obj, target_obj, mode);
}

// 0x572340
bool sub_572340()
{
    if (!inven_ui_created) {
        return false;
    }

    if (inven_ui_drag_item_obj != OBJ_HANDLE_NULL) {
        sub_575770();
    }

    inven_ui_destroy();

    return true;
}

// 0x572370
bool sub_572370(int64_t pc_obj, int64_t target_obj, int mode)
{
    int rot;
    tig_art_id_t old_art_id;
    tig_art_id_t new_art_id;
    int err;
    int sound_id;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    if (pc_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (critter_is_dead(pc_obj)) {
        return false;
    }

    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    if (mode == INVEN_UI_MODE_LOOT
        || mode == INVEN_UI_MODE_STEAL) {
        // FIX: To determine if a PC can see the target object, the PC is
        // instantly rotated toward the target before calling `ai_can_see`.
        // However, when the PC is walking or running along a plotted path
        // (expressed as a series of rotations), this instant rotation disrupts
        // the path, causing the PC to run straight to the target and ignore any
        // obstacles. The fix is straightforward: restore the previous rotation
        // if the PC cannot see the target.
        rot = object_rot(pc_obj, target_obj);
        old_art_id = obj_field_int32_get(pc_obj, OBJ_F_CURRENT_AID);
        new_art_id = tig_art_id_rotation_set(old_art_id, rot);
        object_set_current_aid(pc_obj, new_art_id);
        if (ai_can_see(pc_obj, target_obj) != 0) {
            object_set_current_aid(pc_obj, old_art_id);
            return false;
        }
    }

    if (target_obj != OBJ_HANDLE_NULL
        && (mode == INVEN_UI_MODE_LOOT
            || mode == INVEN_UI_MODE_IDENTIFY)) {
        if (obj_field_int32_get(target_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
            if (mode == INVEN_UI_MODE_LOOT) {
                err = ai_attempt_open_container(pc_obj, target_obj);
                if (err != AI_ATTEMPT_OPEN_CONTAINER_OK) {
                    mes_file_entry.num = err;
                    mes_get_msg(inven_ui_mes_file, &mes_file_entry);

                    ui_message.type = UI_MSG_TYPE_EXCLAMATION;
                    ui_message.str = mes_file_entry.str;
                    sub_550750(&ui_message);

                    sound_id = sfx_container_sound(target_obj, CONTAINER_SOUND_LOCKED);
                    gsound_play_sfx(sound_id, 1);
                    return false;
                }

                if (!object_script_execute(pc_obj, target_obj, OBJ_HANDLE_NULL, SAP_USE, 0)) {
                    sound_id = sfx_container_sound(target_obj, CONTAINER_SOUND_LOCKED);
                    gsound_play_sfx(sound_id, 1);
                    return false;
                }
            }
        } else {
            if (mode == INVEN_UI_MODE_LOOT) {
                if (!object_script_execute(pc_obj, target_obj, OBJ_HANDLE_NULL, SAP_USE, 0)) {
                    return false;
                }
            }
        }
    }

    return true;
}

// 0x572510
bool sub_572510(int64_t pc_obj, int64_t target_obj, int mode)
{
    if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
        return false;
    }

    switch (mode) {
    case INVEN_UI_MODE_INVENTORY:
        if (!intgame_mode_set(INTGAME_MODE_INVEN)) {
            return false;
        }
        return true;
    case INVEN_UI_MODE_BARTER:
        if (!intgame_mode_set(INTGAME_MODE_BARTER)) {
            return false;
        }
        if (pc_obj == target_obj) {
            return false;
        }
        return true;
    case INVEN_UI_MODE_LOOT:
    case INVEN_UI_MODE_IDENTIFY:
        if (!intgame_mode_set(INTGAME_MODE_LOOT)) {
            return false;
        }
        if (pc_obj == target_obj) {
            return false;
        }
        return true;
    case INVEN_UI_MODE_STEAL:
        if (!intgame_mode_set(INTGAME_MODE_STEAL)) {
            return false;
        }
        if (pc_obj == target_obj) {
            return false;
        }
        return true;
    case INVEN_UI_MODE_NPC_IDENTIFY:
        if (!intgame_mode_set(INTGAME_MODE_NPC_IDENTIFY)) {
            return false;
        }
        if (pc_obj == target_obj) {
            return false;
        }
        return true;
    case INVEN_UI_MODE_NPC_REPAIR:
        if (!intgame_mode_set(INTGAME_MODE_NPC_REPAIR)) {
            return false;
        }
        if (pc_obj == target_obj) {
            return false;
        }
        return true;
    default:
        return false;
    }
}

// 0x572640
void sub_572640(int64_t pc_obj, int64_t target_obj, int mode)
{
    int64_t substitute_inventory_obj;
    int64_t prototype_obj;
    int64_t gold_obj;
    int amt;
    int sound_id;

    if (target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (mode == INVEN_UI_MODE_BARTER) {
        substitute_inventory_obj = critter_substitute_inventory_get(target_obj);
        if (substitute_inventory_obj != OBJ_HANDLE_NULL) {
            prototype_obj = sub_468570(8);
            gold_obj = sub_462540(substitute_inventory_obj, prototype_obj, 0);
            while (gold_obj != OBJ_HANDLE_NULL) {
                amt = item_gold_get(gold_obj);
                item_gold_transfer(substitute_inventory_obj, target_obj, amt, gold_obj);
                gold_obj = sub_462540(substitute_inventory_obj, prototype_obj, 0);
            }
        }

        if (critter_pc_leader_get(target_obj) == OBJ_HANDLE_NULL) {
            item_identify_all(target_obj);
            item_identify_all(substitute_inventory_obj);
        }
    } else if (mode == INVEN_UI_MODE_LOOT || mode == INVEN_UI_MODE_IDENTIFY) {
        if (obj_field_int32_get(target_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
            if (mode == INVEN_UI_MODE_LOOT) {
                if (tig_art_id_frame_get(obj_field_int32_get(target_obj, OBJ_F_CURRENT_AID)) == 0) {
                    mp_container_open(target_obj);

                    sound_id = sfx_container_sound(target_obj, CONTAINER_SOUND_OPEN);
                    gsound_play_sfx(sound_id, 1);
                }
            }
            if ((obj_field_int32_get(target_obj, OBJ_F_CONTAINER_FLAGS) & OCOF_INVEN_SPAWN_ONCE) != 0) {
                sub_463E20(target_obj);
            }
        } else {
            if (mode == INVEN_UI_MODE_LOOT) {
                gsound_play_sfx(SND_INTERFACE_ITEM_DROP, 1);
            }
        }
    }
}

// 0x5727B0
bool inven_ui_create(int64_t pc_obj, int64_t target_obj, int mode)
{
    TigRect rect;
    TigButtonData button_data;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    PcLens pc_lens;
    tig_art_id_t art_id;
    tig_button_handle_t button_group[2];
    unsigned int critter_flags2;

    inven_ui_mode = mode;
    qword_6813A8 = target_obj;
    qword_682C78 = target_obj;
    inven_ui_target_panel = INVEN_UI_PANEL_INVENTORY;
    inven_ui_panel = INVEN_UI_PANEL_INVENTORY;

    if (!intgame_big_window_lock(inven_ui_message_filter, &inven_ui_window_handle)) {
        intgame_mode_set(INTGAME_MODE_MAIN);
        return false;
    }

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = inven_ui_window_handle;
    tig_art_interface_id_create(340, 0, 0, 0, &(button_data.art_id));
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    button_data.x = 763;
    button_data.y = 197;
    if (tig_button_create(&button_data, &inven_ui_arrange_items_btn) != TIG_OK) {
        intgame_big_window_unlock();
        intgame_mode_set(INTGAME_MODE_MAIN);
        return false;
    }

    inven_ui_paperdoll_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_inventory_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_target_paperdoll_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_target_inventory_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_total_attack_image_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_total_defence_image_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_total_attack_value_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_total_defence_value_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_target_total_attack_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_target_total_defence_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_take_all_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_cycle_left_btn = TIG_BUTTON_HANDLE_INVALID;
    inven_ui_cycle_right_btn = TIG_BUTTON_HANDLE_INVALID;

    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        button_data.mouse_down_snd_id = -1;
        button_data.mouse_up_snd_id = -1;
        tig_art_interface_id_create(676, 0, 0, 0, &(button_data.art_id));
        button_data.x = 29;
        button_data.y = 318;
        if (tig_button_create(&button_data, &inven_ui_total_attack_image_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        tig_art_interface_id_create(677, 0, 0, 0, &(button_data.art_id));
        button_data.x = 254;
        button_data.y = 319;
        if (tig_button_create(&button_data, &inven_ui_total_defence_image_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        button_data.y = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_ATTACK].y;
        button_data.x = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_ATTACK].x;
        button_data.art_id = TIG_ART_ID_INVALID;
        button_data.width = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_ATTACK].width;
        button_data.height = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_ATTACK].height;
        if (tig_button_create(&button_data, &inven_ui_total_attack_value_btn)) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        button_data.x = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].x;
        button_data.y = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].y;
        button_data.art_id = TIG_ART_ID_INVALID;
        button_data.width = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].width;
        button_data.height = inven_ui_inventory_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].height;
        if (tig_button_create(&button_data, &inven_ui_total_defence_value_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
        button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    } else {
        button_data.flags = TIG_BUTTON_HIDDEN;
        button_data.mouse_down_snd_id = -1;
        button_data.mouse_up_snd_id = -1;
        tig_art_interface_id_create(676, 0, 0, 0, &(button_data.art_id));
        button_data.x = 374;
        button_data.y = 318;
        if (tig_button_create(&button_data, &inven_ui_total_attack_image_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        tig_art_interface_id_create(677, 0, 0, 0, &(button_data.art_id));
        button_data.x = 599;
        button_data.y = 319;
        if (tig_button_create(&button_data, &inven_ui_total_defence_image_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        button_data.y = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].y;
        button_data.x = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].x;
        button_data.art_id = TIG_ART_ID_INVALID;
        button_data.width = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].width;
        button_data.height = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].height;
        if (tig_button_create(&button_data, &inven_ui_total_attack_value_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        button_data.x = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].x;
        button_data.y = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].y;
        button_data.art_id = TIG_ART_ID_INVALID;
        button_data.width = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].width;
        button_data.height = inven_ui_barter_pc_total_stat_rects[INVEN_UI_TOTAL_DEFENSE].height;
        if (tig_button_create(&button_data, &inven_ui_total_defence_value_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        button_data.flags = TIG_BUTTON_MOMENTARY;
        button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
        button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
        if (inven_ui_mode == INVEN_UI_MODE_LOOT) {
            tig_art_interface_id_create(808, 0, 0, 0, &(button_data.art_id));
            button_data.x = 301;
            button_data.y = 47;
            if (tig_button_create(&button_data, &inven_ui_take_all_btn) != TIG_OK) {
                intgame_big_window_unlock();
                intgame_mode_set(INTGAME_MODE_MAIN);
                return false;
            }
        }

        button_data.flags = TIG_BUTTON_TOGGLE | TIG_BUTTON_LATCH;
        tig_art_interface_id_create(338, 0, 0, 0, &(button_data.art_id));
        button_data.x = 713;
        button_data.y = 37;
        if (tig_button_create(&button_data, &inven_ui_paperdoll_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        tig_art_interface_id_create(339, 0, 0, 0, &(button_data.art_id));
        button_data.x = 751;
        button_data.y = 37;
        if (tig_button_create(&button_data, &inven_ui_inventory_btn) != TIG_OK) {
            intgame_big_window_unlock();
            intgame_mode_set(INTGAME_MODE_MAIN);
            return false;
        }

        button_group[0] = inven_ui_inventory_btn;
        button_group[1] = inven_ui_paperdoll_btn;
        tig_button_radio_group_create(2, button_group, 0);

        if (obj_field_int32_get(qword_6813A8, OBJ_F_TYPE) != OBJ_TYPE_CONTAINER) {
            button_data.flags = TIG_BUTTON_TOGGLE | TIG_BUTTON_LATCH;
            tig_art_interface_id_create(338, 0, 0, 0, &button_data.art_id);
            if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
                button_data.x = 8;
                button_data.y = 123;
            } else {
                button_data.x = 23;
                button_data.y = 108;
            }
            if (tig_button_create(&button_data, &inven_ui_target_paperdoll_btn) != TIG_OK) {
                intgame_big_window_unlock();
                intgame_mode_set(INTGAME_MODE_MAIN);
                return false;
            }

            tig_art_interface_id_create(339, 0, 0, 0, &button_data.art_id);
            if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
                button_data.x = 46;
                button_data.y = 123;
            } else {
                button_data.x = 74;
                button_data.y = 108;
            }
            if (tig_button_create(&button_data, &inven_ui_target_inventory_btn) != TIG_OK) {
                intgame_big_window_unlock();
                intgame_mode_set(INTGAME_MODE_MAIN);
                return false;
            }

            button_group[0] = inven_ui_target_inventory_btn;
            button_group[1] = inven_ui_target_paperdoll_btn;
            tig_button_radio_group_create(2, button_group, 0);

            button_data.flags = TIG_BUTTON_HIDDEN;
            button_data.mouse_down_snd_id = -1;
            button_data.mouse_up_snd_id = -1;
            button_data.art_id = TIG_ART_ID_INVALID;
            button_data.x = 75;
            button_data.y = 329;
            button_data.width = 55;
            button_data.height = 40;
            if (tig_button_create(&button_data, &inven_ui_target_total_attack_btn) != TIG_OK) {
                intgame_big_window_unlock();
                intgame_mode_set(INTGAME_MODE_MAIN);
                return false;
            }

            button_data.art_id = TIG_ART_ID_INVALID;
            button_data.x = 225;
            button_data.y = 329;
            button_data.width = 55;
            button_data.height = 40;
            if (tig_button_create(&button_data, &inven_ui_target_total_defence_btn) != TIG_OK) {
                intgame_big_window_unlock();
                intgame_mode_set(INTGAME_MODE_MAIN);
                return false;
            }
        }
    }

    inven_ui_pc_obj = pc_obj;
    item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);

    if (inven_ui_mode == INVEN_UI_MODE_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        inven_ui_have_drop_box = false;
        inven_ui_have_use_box = false;
    } else {
        inven_ui_have_drop_box = true;
        inven_ui_drop_box_image = 224;
        inven_ui_drop_box_frame.x = 703;
        inven_ui_drop_box_frame.y = 385;

        inven_ui_have_use_box = true;
        inven_ui_use_box_image = 342;
        inven_ui_use_box_frame.x = 703;
        inven_ui_use_box_frame.y = 331;
    }

    if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
        inven_ui_have_gamble_box = true;
        inven_ui_gamble_box_image = 345;
        inven_ui_gamble_box_frame.x = 703;
        inven_ui_gamble_box_frame.y = 277;
    } else {
        inven_ui_have_gamble_box = false;
    }

    dword_681510 = 5;

    if (qword_6813A8 != OBJ_HANDLE_NULL) {
        object_examine(qword_682C78, inven_ui_pc_obj, byte_682C8C);
        dword_681510 = 4;
        if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
            dword_681510 = 5;
            qword_6813A8 = critter_substitute_inventory_get(qword_682C78);
            if (qword_6813A8 != OBJ_HANDLE_NULL) {
                dword_681510 = 89;
            } else {
                qword_6813A8 = qword_682C78;
            }
        } else if ((inven_ui_mode == INVEN_UI_MODE_LOOT
                       || inven_ui_mode == INVEN_UI_MODE_IDENTIFY)
            && obj_field_int32_get(qword_6813A8, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
            dword_681510 = 88;
            item_decay_process_disable();
        }

        item_inventory_slots_get(qword_6813A8, dword_681518);
    }

    byte_68241C[0] = '\0';
    dword_681440 = -1;

    if (inven_ui_mode == INVEN_UI_MODE_BARTER
        && critter_leader_get(qword_682C78) == pc_obj) {
        dword_6810FC = 1;
        if (critter_follower_next(qword_682C78) != qword_682C78) {
            button_data.flags = TIG_BUTTON_MOMENTARY;
            button_data.window_handle = inven_ui_window_handle;
            tig_art_interface_id_create(757, 0, 0, 0, &(button_data.art_id));
            button_data.x = 155;
            button_data.y = 55;
            if (tig_button_create(&button_data, &inven_ui_cycle_left_btn) != TIG_OK) {
                intgame_big_window_unlock();
                intgame_mode_set(INTGAME_MODE_MAIN);
                return false;
            }

            tig_art_interface_id_create(758, 0, 0, 0, &(button_data.art_id));
            button_data.x = 253;
            button_data.y = 55;
            if (tig_button_create(&button_data, &inven_ui_cycle_right_btn) != TIG_OK) {
                intgame_big_window_unlock();
                intgame_mode_set(INTGAME_MODE_MAIN);
                return false;
            }
        }
    } else {
        dword_6810FC = 0;
    }

    dword_681508 = 0;
    qword_681458 = OBJ_HANDLE_NULL;

    sub_4A53B0(inven_ui_pc_obj, qword_6813A8);
    redraw_inven_fix_bad_inven_obj(inven_ui_pc_obj);
    redraw_inven_fix_bad_inven_obj(qword_6813A8);

    if (!dword_6810FC
        && qword_6813A8 != OBJ_HANDLE_NULL
        && obj_field_int32_get(qword_6813A8, OBJ_F_TYPE) != OBJ_TYPE_PC
        && sub_4A5460(qword_6813A8) <= 1) {
        item_arrange_inventory(qword_6813A8, false);
        item_inventory_slots_get(qword_6813A8, dword_681518);
    }

    inven_ui_target_inventory_scrollbar_create();
    redraw_inven(true);
    inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
    location_origin_set(obj_field_int64_get(inven_ui_pc_obj, OBJ_F_LOCATION));

    pc_lens.window_handle = inven_ui_window_handle;
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        rect.x = 11;
        rect.y = 9;
        art_id = 233;
    } else {
        rect.x = 16;
        rect.y = 17;
        art_id = inven_ui_mode == INVEN_UI_MODE_BARTER ? 231 : 232;
    }

    rect.width = 89;
    rect.height = 89;
    pc_lens.rect = &rect;
    tig_art_interface_id_create(art_id, 0, 0, 0, &(pc_lens.art_id));
    if (!mainmenu_ui_is_active()) {
        intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);
    }

    dword_739F58 = 0;

    if (player_is_local_pc_obj(pc_obj)) {
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_INVENTORY, false);
    }

    if (qword_6813A8 != OBJ_HANDLE_NULL) {
        sub_4640C0(qword_6813A8);
    }

    critter_flags2 = obj_field_int32_get(inven_ui_pc_obj, OBJ_F_CRITTER_FLAGS2);
    if ((critter_flags2 & OCF2_ITEM_STOLEN) != 0) {
        critter_flags2 &= ~OCF2_ITEM_STOLEN;
        obj_field_int32_set(inven_ui_pc_obj, OBJ_F_CRITTER_FLAGS2, critter_flags2);

        mes_file_entry.num = 7;
        mes_get_msg(inven_ui_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_EXCLAMATION;
        ui_message.str = mes_file_entry.str;
        sub_550750(&ui_message);
    }

    inven_ui_arrange_vertical = false;
    inven_ui_created = true;

    return true;
}

// 0x573440
void inven_ui_destroy()
{
    tig_art_id_t art_id;
    int sound_id;
    IntgameMode mode;

    if (!inven_ui_created) {
        return;
    }

    inven_ui_created = false;
    sub_4A53B0(inven_ui_pc_obj, 0);

    if ((inven_ui_mode == INVEN_UI_MODE_LOOT
            || inven_ui_mode == INVEN_UI_MODE_IDENTIFY)
        && obj_field_int32_get(qword_6813A8, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        if (inven_ui_mode == INVEN_UI_MODE_LOOT) {
            art_id = obj_field_int32_get(qword_6813A8, OBJ_F_CURRENT_AID);
            if (tig_art_id_frame_get(art_id) == 1
                && (obj_field_int32_get(qword_6813A8, OBJ_F_CONTAINER_FLAGS) & OCOF_STICKY) == 0) {
                object_dec_current_aid(qword_6813A8);

                sound_id = sfx_container_sound(qword_6813A8, CONTAINER_SOUND_CLOSE);
                gsound_play_sfx(sound_id, 1);
            }
        }
        item_decay_process_enable();
        sub_4ED6C0(qword_6813A8);
    }

    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
    inven_ui_target_inventory_scrollbar_destroy();
    intgame_big_window_unlock();
    iso_interface_refresh();
    mode = intgame_mode_get();
    if (mode != INTGAME_MODE_INVEN
        && mode != INTGAME_MODE_LOOT
        && mode != INTGAME_MODE_STEAL
        && mode != INTGAME_MODE_BARTER
        && mode != INTGAME_MODE_NPC_IDENTIFY
        && mode != INTGAME_MODE_NPC_REPAIR) {
        intgame_mode_set(INTGAME_MODE_MAIN);
    }
    intgame_mode_set(INTGAME_MODE_MAIN);
}

// 0x573590
void sub_573590(int64_t obj)
{
    if (!inven_ui_created) {
        return;
    }

    if (obj == inven_ui_pc_obj
        || obj == qword_6813A8
        || obj == qword_682C78) {
        inven_ui_destroy();
    }
}

// 0x5735F0
int inven_ui_is_created()
{
    return inven_ui_created;
}

// 0x573600
int64_t sub_573600()
{
    return inven_ui_created ? qword_682C78 : OBJ_HANDLE_NULL;
}

// 0x573620
int64_t inven_ui_drag_item_obj_get()
{
    return inven_ui_drag_item_obj;
}

// 0x573630
void sub_573630(int64_t obj)
{
    tig_art_id_t inv_art_id;
    tig_art_id_t mouse_art_id;

    if (!inven_ui_created && inven_ui_pc_obj == OBJ_HANDLE_NULL) {
        inven_ui_pc_obj = player_get_local_pc_obj();
    }

    qword_681450 = inven_ui_pc_obj;
    inven_ui_drag_item_obj = obj;
    dword_6810E8 = obj_field_int32_get(obj, OBJ_F_ITEM_INV_LOCATION);

    inv_art_id = obj_field_int32_get(inven_ui_drag_item_obj, OBJ_F_ITEM_INV_AID);
    intgame_refresh_cursor();
    mouse_art_id = tig_mouse_cursor_get_art_id();
    tig_mouse_hide();
    tig_mouse_cursor_set_art_id(inv_art_id);
    tig_mouse_cursor_overlay(mouse_art_id, 2, 2);
    tig_mouse_cursor_set_offset(2, 2);
    tig_mouse_show();
}

// 0x5736E0
void sub_5736E0()
{
    tig_art_id_t inv_art_id;
    tig_art_id_t mouse_art_id;

    if (inven_ui_drag_item_obj == OBJ_HANDLE_NULL) {
        return;
    }

    inv_art_id = obj_field_int32_get(inven_ui_drag_item_obj, OBJ_F_ITEM_INV_AID);
    mouse_art_id = tig_mouse_cursor_get_art_id();
    tig_mouse_hide();
    tig_mouse_cursor_set_art_id(inv_art_id);
    tig_mouse_cursor_overlay(mouse_art_id, 2, 2);
    tig_mouse_cursor_set_offset(2, 2);
    tig_mouse_show();
}

// 0x573730
void sub_573730()
{
    dword_68346C = false;
}

// 0x573740
void sub_573740(int64_t a1, bool a2)
{
    tig_art_id_t mouse_art_id;
    tig_art_id_t inv_art_id;

    if (!inven_ui_created && inven_ui_pc_obj == OBJ_HANDLE_NULL) {
        inven_ui_pc_obj = player_get_local_pc_obj();
    }

    mouse_art_id = tig_mouse_cursor_get_art_id();
    qword_681450 = inven_ui_pc_obj;
    inven_ui_drag_item_obj = a1;
    dword_6810E8 = obj_field_int32_get(a1, OBJ_F_ITEM_INV_LOCATION);

    if (!inven_ui_created) {
        sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, a2 ? 4 : 36);
        if (inven_ui_drag_item_obj == OBJ_HANDLE_NULL) {
            return;
        }
    }

    inv_art_id = obj_field_int32_get(inven_ui_drag_item_obj, OBJ_F_ITEM_INV_AID);
    tig_mouse_hide();
    tig_mouse_cursor_set_art_id(inv_art_id);
    tig_mouse_cursor_overlay(mouse_art_id, 2, 2);
    tig_mouse_cursor_set_offset(2, 2);
    tig_mouse_show();
}

// 0x573840
void sub_573840()
{
    inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
}

// 0x573897
static inline bool inven_ui_message_filter_handle_button_pressed(TigMessage* msg)
{
    if (msg->data.button.button_handle == inven_ui_inventory_btn) {
        inven_ui_panel = INVEN_UI_PANEL_INVENTORY;
        tig_button_hide(inven_ui_total_attack_image_btn);
        tig_button_hide(inven_ui_total_defence_image_btn);
        tig_button_hide(inven_ui_total_attack_value_btn);
        tig_button_hide(inven_ui_total_defence_value_btn);
        redraw_inven(false);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_paperdoll_btn) {
        inven_ui_panel = INVEN_UI_PANEL_PAPERDOLL;
        tig_button_show(inven_ui_total_attack_image_btn);
        tig_button_show(inven_ui_total_defence_image_btn);
        tig_button_show(inven_ui_total_attack_value_btn);
        tig_button_show(inven_ui_total_defence_value_btn);
        redraw_inven(false);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_target_inventory_btn) {
        inven_ui_target_panel = INVEN_UI_PANEL_INVENTORY;
        inven_ui_target_inventory_scrollbar_show();
        tig_button_hide(inven_ui_target_total_attack_btn);
        tig_button_hide(inven_ui_target_total_defence_btn);
        redraw_inven(false);
        redraw_inven(false);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_target_paperdoll_btn) {
        inven_ui_target_panel = INVEN_UI_PANEL_PAPERDOLL;
        inven_ui_target_inventory_scrollbar_hide();
        tig_button_show(inven_ui_target_total_attack_btn);
        tig_button_show(inven_ui_target_total_defence_btn);
        redraw_inven(false);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_total_attack_image_btn
        || msg->data.button.button_handle == inven_ui_total_attack_value_btn) {
        iso_interface_window_set(ROTWIN_TYPE_MSG);
        intgame_message_window_display_attack(inven_ui_pc_obj);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_total_defence_image_btn
        || msg->data.button.button_handle == inven_ui_total_defence_value_btn) {
        iso_interface_window_set(ROTWIN_TYPE_MSG);
        intgame_message_window_display_defense(inven_ui_pc_obj);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_target_total_attack_btn) {
        iso_interface_window_set(ROTWIN_TYPE_MSG);
        intgame_message_window_display_attack(qword_682C78);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_target_total_defence_btn) {
        iso_interface_window_set(ROTWIN_TYPE_MSG);
        intgame_message_window_display_defense(qword_682C78);
        return true;
    }

    return false;
}

// 0x573A95
static inline bool inven_ui_message_filter_handle_button_released(TigMessage* msg)
{
    if (msg->data.button.button_handle == inven_ui_arrange_items_btn) {
        item_arrange_inventory(inven_ui_pc_obj, inven_ui_arrange_vertical);
        item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
        if (dword_6810FC) {
            item_arrange_inventory(qword_6813A8, inven_ui_arrange_vertical);
            item_inventory_slots_get(qword_6813A8, dword_681518);
        }
        inven_ui_arrange_vertical = !inven_ui_arrange_vertical;
        redraw_inven(false);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_take_all_btn) {
        sub_579770(qword_6813A8, inven_ui_pc_obj);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_cycle_left_btn) {
        sub_574FD0(false);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_cycle_right_btn) {
        sub_574FD0(true);
        return true;
    }

    return false;
}

// 0x573BAF
static inline bool inven_ui_mssage_filter_handle_button_hovered(TigMessage* msg)
{
    if (msg->data.button.button_handle == inven_ui_total_attack_image_btn || msg->data.button.button_handle == inven_ui_total_attack_value_btn) {
        intgame_message_window_display_attack(inven_ui_pc_obj);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_total_defence_image_btn || msg->data.button.button_handle == inven_ui_total_defence_value_btn) {
        intgame_message_window_display_defense(inven_ui_pc_obj);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_target_total_attack_btn) {
        intgame_message_window_display_attack(qword_682C78);
        return true;
    }

    if (msg->data.button.button_handle == inven_ui_target_total_defence_btn) {
        intgame_message_window_display_defense(qword_682C78);
        return true;
    }

    return false;
}

// 0x573C80
static inline void inven_ui_message_filter_handle_mouse_idle(TigMessage* msg)
{
    int64_t v1;
    int64_t v2;

    if (inven_ui_drag_item_obj == OBJ_HANDLE_NULL) {
        v1 = sub_575FA0(msg->data.mouse.x, msg->data.mouse.y, &v2);
        if (v1 != qword_681458) {
            qword_681458 = v1;
            if (qword_681458 != OBJ_HANDLE_NULL) {
                sub_57CCF0(inven_ui_pc_obj, qword_681458);
                switch (inven_ui_mode) {
                case INVEN_UI_MODE_BARTER:
                    sub_578330(qword_681458, v2);
                    break;
                case INVEN_UI_MODE_NPC_IDENTIFY:
                    sub_5786C0(qword_681458);
                    break;
                case INVEN_UI_MODE_NPC_REPAIR:
                    sub_578760(qword_681458);
                    break;
                default:
                    redraw_inven(false);
                    break;
                }
            } else {
                byte_68241C[0] = '\0';
                dword_681440 = -1;
                redraw_inven(false);
            }
        }
    }
}

// 0x573D61
static inline bool inven_ui_message_filter_handle_mouse_move(TigMessage* msg)
{
    if (inven_ui_drag_item_obj != OBJ_HANDLE_NULL) {
        if (inven_ui_have_drop_box) {
            if (msg->data.mouse.x >= inven_ui_drop_box_frame.x
                && msg->data.mouse.x < inven_ui_drop_box_frame.x + inven_ui_drop_box_frame.width
                && msg->data.mouse.y >= inven_ui_drop_box_frame.y
                && msg->data.mouse.y < inven_ui_drop_box_frame.y + inven_ui_drop_box_frame.height) {
                if (inven_ui_drop_box_image == 224) {
                    if (qword_681450 == qword_6813A8) {
                        switch (inven_ui_mode) {
                        case INVEN_UI_MODE_BARTER:
                            if (dword_6810FC) {
                                inven_ui_drop_box_image = 225;
                            } else {
                                inven_ui_drop_box_image = 348;
                            }
                            break;
                        case INVEN_UI_MODE_LOOT:
                        case INVEN_UI_MODE_STEAL:
                            inven_ui_drop_box_image = 348;
                            break;
                        default:
                            inven_ui_drop_box_image = 225;
                            break;
                        }
                    } else {
                        inven_ui_drop_box_image = 225;
                    }

                    redraw_inven(false);
                    return true;
                }
            } else {
                if (inven_ui_drop_box_image != 224) {
                    inven_ui_drop_box_image = 224;
                    redraw_inven(false);
                    return true;
                }
            }
        }

        if (inven_ui_have_gamble_box) {
            if (msg->data.mouse.x >= inven_ui_gamble_box_frame.x
                && msg->data.mouse.x < inven_ui_gamble_box_frame.x + inven_ui_gamble_box_frame.width
                && msg->data.mouse.y >= inven_ui_gamble_box_frame.y
                && msg->data.mouse.y < inven_ui_gamble_box_frame.y + inven_ui_gamble_box_frame.height) {
                if (inven_ui_gamble_box_image == 345) {
                    inven_ui_gamble_box_image = sub_579840(inven_ui_drag_item_obj, 0) ? 344 : 346;
                    redraw_inven(false);
                    return true;
                }
            } else {
                if (inven_ui_gamble_box_image != 345) {
                    inven_ui_gamble_box_image = 345;
                    redraw_inven(false);
                    return true;
                }
            }
        }

        if (inven_ui_have_use_box) {
            if (msg->data.mouse.x >= inven_ui_use_box_frame.x
                && msg->data.mouse.x < inven_ui_use_box_frame.x + inven_ui_use_box_frame.width
                && msg->data.mouse.y >= inven_ui_use_box_frame.y
                && msg->data.mouse.y < inven_ui_use_box_frame.y + inven_ui_use_box_frame.height) {
                if (inven_ui_use_box_image == 342) {
                    if ((qword_681450 == qword_6813A8
                            && (inven_ui_mode == INVEN_UI_MODE_BARTER
                                || inven_ui_mode == INVEN_UI_MODE_LOOT
                                || inven_ui_mode == INVEN_UI_MODE_STEAL))
                        || sub_462C30(inven_ui_pc_obj, inven_ui_drag_item_obj)) {
                        inven_ui_use_box_image = 347;
                    } else {
                        inven_ui_use_box_image = 343;
                    }
                    redraw_inven(false);
                    return true;
                }
            } else {
                if (inven_ui_use_box_image != 342) {
                    inven_ui_use_box_image = 342;
                    redraw_inven(false);
                    return true;
                }
            }
        }
    }

    return false;
}

// 0x574080
static inline bool inven_ui_message_filter_handle_mouse_lbutton_down(TigMessage* msg)
{
    int64_t v1;
    int64_t v2;
    S4F2810 v3;

    switch (intgame_mode_get()) {
    case INTGAME_MODE_INVEN:
    case INTGAME_MODE_LOOT:
    case INTGAME_MODE_STEAL:
    case INTGAME_MODE_BARTER:
        dword_68346C = true;
        if (inven_ui_drag_item_obj == OBJ_HANDLE_NULL) {
            inven_ui_drag_item_obj = sub_575FA0(msg->data.mouse.x, msg->data.mouse.y, &qword_681450);
            if (inven_ui_drag_item_obj != OBJ_HANDLE_NULL) {
                dword_683470 = sub_575CB0(msg->data.mouse.x, msg->data.mouse.y, &v2);
                dword_6810E8 = item_inventory_location_get(inven_ui_drag_item_obj);
                dword_683470 -= dword_6810E8;
                sub_5754C0(msg->data.mouse.x, msg->data.mouse.y);
            }
            return true;
        }
        break;
    case INTGAME_MODE_SKILL:
        v1 = sub_575FA0(msg->data.mouse.x, msg->data.mouse.y, &qword_681450);
        if (v1 != OBJ_HANDLE_NULL) {
            sub_4F2810(&v3, v1);
            sub_57A1F0(&v3);
            skill_ui_cancel();
            return true;
        }
        break;
    case INTGAME_MODE_SPELL:
        v1 = sub_575FA0(msg->data.mouse.x, msg->data.mouse.y, &qword_681450);
        if (v1 != OBJ_HANDLE_NULL) {
            sub_4F2810(&v3, v1);
            spell_ui_apply(&v3);
            return true;
        }
        break;
    case 16:
        v1 = sub_575FA0(msg->data.mouse.x, msg->data.mouse.y, &qword_681450);
        if (v1 != OBJ_HANDLE_NULL) {
            sub_4F2810(&v3, v1);
            item_ui_apply(&v3);
            return true;
        }
        break;
    }

    return false;
}

// 0x5747E4
static inline bool inven_ui_message_filter_handle_mouse_lbutton_up_accept_drop(TigMessage* msg, bool* v45)
{
    int64_t v3;
    int64_t v2;
    int inventory_location;
    int new_inventory_location;
    int err;
    int64_t old_item_obj;
    int v5;

    if (inven_ui_have_drop_box
        && msg->data.mouse.x >= inven_ui_drop_box_frame.x
        && msg->data.mouse.x < inven_ui_drop_box_frame.x + inven_ui_drop_box_frame.width
        && msg->data.mouse.y >= inven_ui_drop_box_frame.y
        && msg->data.mouse.y < inven_ui_drop_box_frame.y + inven_ui_drop_box_frame.height) {
        if (qword_681450 == qword_6813A8) {
            switch (inven_ui_mode) {
            case INVEN_UI_MODE_BARTER:
                if (dword_6810FC) {
                    sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, 2);
                } else {
                    sub_575770();
                }
                break;
            case INVEN_UI_MODE_LOOT:
            case INVEN_UI_MODE_STEAL:
                sub_575770();
                break;
            default:
                sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, 2);
                break;
            }
        } else {
            sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, 2);
        }

        inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        inven_ui_drop_box_image = 224;

        return false;
    }

    if (inven_ui_have_gamble_box
        && msg->data.mouse.x >= inven_ui_gamble_box_frame.x
        && msg->data.mouse.x < inven_ui_gamble_box_frame.x + inven_ui_gamble_box_frame.width
        && msg->data.mouse.y >= inven_ui_gamble_box_frame.y
        && msg->data.mouse.y < inven_ui_gamble_box_frame.y + inven_ui_gamble_box_frame.height) {
        v3 = inven_ui_drag_item_obj;
        sub_575770();
        sub_579B60(v3);
        inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        inven_ui_gamble_box_image = 345;

        return false;
    }

    if (inven_ui_have_use_box
        && msg->data.mouse.x >= inven_ui_use_box_frame.x
        && msg->data.mouse.x < inven_ui_use_box_frame.x + inven_ui_use_box_frame.width
        && msg->data.mouse.y >= inven_ui_use_box_frame.y
        && msg->data.mouse.y < inven_ui_use_box_frame.y + inven_ui_use_box_frame.height) {
        if ((qword_681450 == qword_6813A8
                    && (inven_ui_mode == INVEN_UI_MODE_BARTER
                        || inven_ui_mode == INVEN_UI_MODE_LOOT
                        || inven_ui_mode == INVEN_UI_MODE_STEAL)
                || sub_462C30(inven_ui_pc_obj, inven_ui_drag_item_obj))) {
            sub_575770();
            inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
            inven_ui_use_box_image = 342;

            return false;
        }

        if ((obj_field_int32_get(inven_ui_drag_item_obj, OBJ_F_ITEM_FLAGS) & OIF_USE_IS_THROW) == 0
            || inven_ui_mode != INVEN_UI_MODE_INVENTORY) {
            v3 = inven_ui_drag_item_obj;
            sub_575770();
            if (!combat_turn_based_is_active()
                || combat_turn_based_whos_turn_get() == inven_ui_pc_obj) {
                if (tig_kb_get_modifier(SDL_KMOD_SHIFT)) {
                    item_use_on_obj(inven_ui_pc_obj, v3, inven_ui_pc_obj);
                } else {
                    item_use_on_obj(inven_ui_pc_obj, v3, OBJ_HANDLE_NULL);
                }
            }

            if (!inven_ui_created) {
                return true;
            }

            inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
            inven_ui_use_box_image = 342;
        } else {
            sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, 4);
            redraw_inven(false);
            item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
            if (qword_6813A8 != OBJ_HANDLE_NULL) {
                item_inventory_slots_get(qword_6813A8, dword_681518);
            }
        }

        return false;
    }

    inventory_location = sub_575CB0(msg->data.mouse.x, msg->data.mouse.y, &v2);
    if (IS_HOTKEY_INV_LOC(inventory_location)) {
        return false;
    }

    if (IS_WEAR_INV_LOC(inventory_location)) {
        if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
            if (qword_681450 != inven_ui_pc_obj && v2 != inven_ui_pc_obj && !dword_6810FC) {
                sub_575770();
                return false;
            }
        } else {
            if (inven_ui_mode == INVEN_UI_MODE_STEAL && qword_681450 != inven_ui_pc_obj && v2 != inven_ui_pc_obj) {
                sub_575770();
                return false;
            }
        }

        err = sub_464D20(inven_ui_drag_item_obj, inventory_location, v2);
        if (err != 0) {
            item_error_msg(v2, err);
            sub_575770();
        }

        old_item_obj = item_wield_get(v2, inventory_location);
        if (old_item_obj == OBJ_HANDLE_NULL || old_item_obj == inven_ui_drag_item_obj) {
            if (v2 == inven_ui_pc_obj) {
                if (!sub_575180(v45)) {
                    sub_575200(inventory_location);
                }
            } else {
                if (!sub_575100(v45)) {
                    sub_575360(inventory_location);
                }
            }

            inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
            return false;
        }

        if (v2 != inven_ui_pc_obj) {
            sub_575770();
            return false;
        }

        err = item_check_remove(old_item_obj);
        if (err != ITEM_CANNOT_OK) {
            if (v2 == inven_ui_pc_obj) {
                item_error_msg(v2, err);
            }
            sub_575770();
            return false;
        }

        if ((inven_ui_mode == INVEN_UI_MODE_BARTER
                || inven_ui_mode == INVEN_UI_MODE_STEAL)
            && qword_681450 != v2) {
            sub_575770();
            return false;
        }

        critter_encumbrance_recalc_feedback_disable();
        item_remove(inven_ui_drag_item_obj);
        new_inventory_location = item_inventory_location_get(old_item_obj);
        item_remove(old_item_obj);
        err = item_check_insert(old_item_obj, v2, &v5);
        if (err == ITEM_CANNOT_OK) {
            item_insert(inven_ui_drag_item_obj, qword_681450, dword_6810E8);
            sub_5788C0(inven_ui_drag_item_obj, v2, inventory_location, 1);
            item_insert(old_item_obj, v2, v5);
            inven_ui_drag_item_obj = old_item_obj;
            dword_6810E8 = v5;
            qword_681450 = v2;
            dword_683470 = 0;
            sub_5754C0(-1, -1);
        } else {
            item_error_msg(v2, err);
            item_insert(old_item_obj, v2, new_inventory_location);
            sub_575930();
        }
        critter_encumbrance_recalc_feedback_enable();

        return false;
    }

    if (inventory_location - dword_683470 < 0) {
        if (!sub_57EDA0(TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP)) {
            sub_575770();
        }
    } else if (v2 == inven_ui_pc_obj) {
        if (!sub_575180(v45)) {
            sub_575200(inventory_location - dword_683470);
        }
    } else {
        if (sub_575100(v45)) {
            sub_575360(inventory_location - dword_683470);
        }
    }

    sub_57F260();

    inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
    return false;
}

// 0x574559
static inline bool inven_ui_message_filter_handle_mouse_lbutton_up(TigMessage* msg, bool* v45)
{
    int64_t v1;
    int64_t v2;

    dword_68346C = 0;
    if (intgame_pc_lens_check_pt_unscale(msg->data.mouse.x, msg->data.mouse.y)) {
        if (!mainmenu_ui_is_active()) {
            if (inven_ui_drag_item_obj != OBJ_HANDLE_NULL) {
                if (inven_ui_mode == INVEN_UI_MODE_INVENTORY) {
                    sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, 4);
                    redraw_inven(false);
                    item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
                    if (qword_6813A8) {
                        item_inventory_slots_get(qword_6813A8, dword_681518);
                    }
                    return false;
                }

                sub_575770();
            }

            *v45 = true;
        }

        return false;
    }

    if (inven_ui_drag_item_obj == OBJ_HANDLE_NULL) {
        v1 = sub_575FA0(msg->data.mouse.x, msg->data.mouse.y, &v2);
        if (v1 == OBJ_HANDLE_NULL) {
            return false;
        }

        switch (inven_ui_mode) {
        case INVEN_UI_MODE_NPC_IDENTIFY:
            sub_5786C0(v1);
            if (dword_681440 > 0
                && item_gold_get(inven_ui_pc_obj) >= dword_681440) {
                item_perform_identify_service(v1, qword_682C78, inven_ui_pc_obj, dword_681440);
            } else {
                dialog_copy_npc_not_enough_money_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
            }
            break;
        case INVEN_UI_MODE_NPC_REPAIR:
            sub_578760(v1);
            if (dword_681440 > 0
                && item_gold_get(inven_ui_pc_obj) >= dword_681440) {
                skill_perform_repair_service(v1, qword_682C78, inven_ui_pc_obj, dword_681440);
            } else {
                dialog_copy_npc_not_enough_money_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
            }
            break;
        default:
            return false;
        }

        byte_68241C[0] = '\0';
        dword_681440 = -1;
        redraw_inven(false);

        return false;
    }

    if (inven_ui_have_drop_box
        && msg->data.mouse.x >= inven_ui_drop_box_frame.x
        && msg->data.mouse.y >= inven_ui_drop_box_frame.y
        && msg->data.mouse.x < inven_ui_drop_box_frame.x + inven_ui_drop_box_frame.width
        && msg->data.mouse.y < inven_ui_drop_box_frame.y + inven_ui_drop_box_frame.height) {
        if (qword_681450 != qword_6813A8
            || (inven_ui_mode == INVEN_UI_MODE_BARTER && dword_6810FC)
            || (inven_ui_mode != INVEN_UI_MODE_STEAL
                && inven_ui_mode != INVEN_UI_MODE_LOOT)) {
            sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, 2);
        } else {
            sub_575770();
        }
        inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        inven_ui_drop_box_image = 224;
    } else if (inven_ui_have_gamble_box
        && msg->data.mouse.x >= inven_ui_gamble_box_frame.x
        && msg->data.mouse.y >= inven_ui_gamble_box_frame.y
        && msg->data.mouse.x < inven_ui_gamble_box_frame.x + inven_ui_gamble_box_frame.width
        && msg->data.mouse.y < inven_ui_gamble_box_frame.y + inven_ui_gamble_box_frame.height) {
        int64_t item_obj = inven_ui_drag_item_obj;
        sub_575770();
        sub_579B60(item_obj);
        inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        inven_ui_gamble_box_image = 345;
    } else if (inven_ui_have_use_box
        && msg->data.mouse.x >= inven_ui_use_box_frame.x
        && msg->data.mouse.y >= inven_ui_use_box_frame.y
        && msg->data.mouse.x < inven_ui_use_box_frame.x + inven_ui_use_box_frame.width
        && msg->data.mouse.y < inven_ui_use_box_frame.y + inven_ui_use_box_frame.height) {
        if ((qword_681450 != qword_6813A8
                || (inven_ui_mode != INVEN_UI_MODE_BARTER
                    && inven_ui_mode != INVEN_UI_MODE_LOOT
                    && inven_ui_mode != INVEN_UI_MODE_STEAL))
            && sub_462C30(inven_ui_pc_obj, inven_ui_drag_item_obj) == ITEM_CANNOT_OK) {
            if ((obj_field_int32_get(inven_ui_drag_item_obj, OBJ_F_ITEM_FLAGS) & OIF_USE_IS_THROW) != 0
                && inven_ui_mode == INVEN_UI_MODE_INVENTORY) {
                sub_5788C0(inven_ui_drag_item_obj, OBJ_HANDLE_NULL, -1, 0x4);
                redraw_inven(false);
                item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
                if (qword_6813A8 != OBJ_HANDLE_NULL) {
                    item_inventory_slots_get(qword_6813A8, dword_681518);
                }
            } else {
                int64_t item_obj = inven_ui_drag_item_obj;
                sub_575770();
                if (!combat_turn_based_is_active()
                    || combat_turn_based_whos_turn_get() == inven_ui_pc_obj) {
                    if (tig_kb_get_modifier(SDL_KMOD_SHIFT)) {
                        item_use_on_obj(inven_ui_pc_obj, item_obj, inven_ui_pc_obj);
                    } else {
                        item_use_on_obj(inven_ui_pc_obj, item_obj, OBJ_HANDLE_NULL);
                    }
                }
                if (!inven_ui_created) {
                    return true;
                }
            }
        } else {
            sub_575770();
        }
        inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        inven_ui_use_box_image = 342;
    } else {
        int64_t parent_obj;
        int inventory_location = sub_575CB0(msg->data.mouse.x, msg->data.mouse.y, &parent_obj);
        if (!IS_HOTKEY_INV_LOC(inventory_location)) {
            if (IS_WEAR_INV_LOC(inventory_location)) {
                if ((inven_ui_mode != INVEN_UI_MODE_BARTER
                        || qword_681450 == inven_ui_pc_obj
                        || parent_obj == inven_ui_pc_obj
                        || dword_6810FC)
                    && (inven_ui_mode != INVEN_UI_MODE_STEAL
                        || qword_681450 == inven_ui_pc_obj
                        || parent_obj == inven_ui_pc_obj)) {
                    int wield_reason = sub_464D20(inven_ui_drag_item_obj, inventory_location, parent_obj);
                    if (wield_reason == ITEM_CANNOT_OK) {
                        int64_t old_item_obj = item_wield_get(parent_obj, inventory_location);
                        if (old_item_obj != OBJ_HANDLE_NULL
                            && old_item_obj != inven_ui_drag_item_obj) {
                            if (parent_obj == inven_ui_pc_obj) {
                                int unwield_reason = item_check_remove(old_item_obj);
                                if (unwield_reason == ITEM_CANNOT_OK) {
                                    if ((inven_ui_mode != INVEN_UI_MODE_BARTER
                                            && inven_ui_mode != INVEN_UI_MODE_STEAL)
                                        || qword_681450 == parent_obj) {
                                        critter_encumbrance_recalc_feedback_disable();
                                        item_remove(inven_ui_drag_item_obj);
                                        int old_inventory_location = item_inventory_location_get(old_item_obj);
                                        item_remove(old_item_obj);
                                        int new_inventory_location;
                                        int transfer_reason = item_check_insert(old_item_obj, parent_obj, &new_inventory_location);
                                        if (transfer_reason == ITEM_CANNOT_OK) {
                                            item_insert(inven_ui_drag_item_obj, qword_681450, dword_6810E8);
                                            sub_5788C0(inven_ui_drag_item_obj, parent_obj, inventory_location, 0x1);
                                            item_insert(old_item_obj, parent_obj, new_inventory_location);
                                            inven_ui_drag_item_obj = old_item_obj;
                                            dword_6810E8 = new_inventory_location;
                                            qword_681450 = parent_obj;
                                            dword_683470 = 0;
                                            sub_5754C0(-1, -1);
                                        } else {
                                            item_error_msg(parent_obj, transfer_reason);
                                            item_insert(old_item_obj, parent_obj, old_inventory_location);
                                            sub_575930();
                                        }
                                        critter_encumbrance_recalc_feedback_enable();
                                    } else {
                                        sub_575770();
                                    }
                                } else {
                                    if (parent_obj == inven_ui_pc_obj) {
                                        item_error_msg(parent_obj, unwield_reason);
                                    }
                                }
                            } else {
                                sub_575770();
                            }
                        } else {
                            if (parent_obj == inven_ui_pc_obj) {
                                if (!sub_575180(v45)) {
                                    sub_575200(inventory_location);
                                }
                            } else {
                                if (!sub_575100(v45)) {
                                    sub_575360(inventory_location);
                                }
                            }
                            inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
                        }
                    } else {
                        item_error_msg(parent_obj, wield_reason);
                        sub_575770();
                    }
                } else {
                    sub_575770();
                }
            } else {
                if (inventory_location - dword_683470 < 0) {
                    if (!sub_57EDA0(TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP)) {
                        sub_575770();
                    }
                } else if (parent_obj == inven_ui_pc_obj) {
                    if (!sub_575180(v45)) {
                        sub_575200(inventory_location - dword_683470);
                    }
                } else {
                    if (!sub_575100(v45)) {
                        sub_575360(inventory_location - dword_683470);
                    }
                }
                sub_57F260();
                inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
            }
        }
    }

    redraw_inven(false);
    item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
    if (qword_6813A8) {
        item_inventory_slots_get(qword_6813A8, dword_681518);
    }

    if (!inven_ui_drag_item_obj) {
        intgame_refresh_cursor();
    }
    if (!v45) {
        return true;
    }

    return false;
}

// 0x5742A4
static inline bool inven_ui_message_filter_handle_mouse_rbutton_up(TigMessage* msg, bool* v45)
{
    int64_t v1;
    int64_t v2;
    int64_t v3;
    int rc;
    int v5;
    int64_t v6;
    int inventory_location;

    v1 = sub_575FA0(msg->data.mouse.x, msg->data.mouse.y, &v2);
    if (v1 == OBJ_HANDLE_NULL) {
        return false;
    }

    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY) {
        inventory_location = item_inventory_location_get(v1);
        if (!IS_WEAR_INV_LOC(inventory_location)) {
            inventory_location = item_location_get(v1);
            if (inventory_location == -1
                && sub_462C30(inven_ui_pc_obj, v1) == ITEM_CANNOT_OK) {
                if (!combat_turn_based_is_active()
                    || combat_turn_based_whos_turn_get() == inven_ui_pc_obj) {
                    item_use_on_obj(inven_ui_pc_obj, v1, inven_ui_pc_obj);
                }

                if (!inven_ui_created) {
                    return false;
                }
            }
        }
    } else if (inven_ui_mode == INVEN_UI_MODE_BARTER
        || inven_ui_mode == INVEN_UI_MODE_LOOT
        || inven_ui_mode == INVEN_UI_MODE_STEAL) {
        v3 = inven_ui_pc_obj;
        if (v2 == inven_ui_pc_obj) {
            v3 = qword_6813A8;
            v6 = qword_6813A8;
        } else {
            v6 = inven_ui_pc_obj;
        }

        rc = item_check_insert(v1, v3, &v5);
        if (rc != ITEM_CANNOT_OK) {
            item_error_msg(v2, rc);
            return true;
        }

        inven_ui_drag_item_obj = v1;
        qword_681450 = v2;
        dword_683470 = sub_575CB0(msg->data.mouse.x, msg->data.mouse.y, &v1);
        dword_6810E8 = item_inventory_location_get(inven_ui_drag_item_obj);
        dword_683470 -= dword_6810E8;
        sub_5754C0(msg->data.mouse.x, msg->data.mouse.y);
        if (inven_ui_drag_item_obj != OBJ_HANDLE_NULL) {
            if (v6 == inven_ui_pc_obj) {
                if (!sub_575180(v45)) {
                    sub_575200(v5);
                }
            } else {
                if (!sub_575100(v45)) {
                    sub_575360(v5);
                }
            }
            inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        }
    }

    redraw_inven(false);
    item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
    if (qword_6813A8 != OBJ_HANDLE_NULL) {
        item_inventory_slots_get(qword_6813A8, dword_681518);
    }

    if (inven_ui_drag_item_obj == OBJ_HANDLE_NULL) {
        intgame_refresh_cursor();
    }

    if (!v45) {
        return true;
    }

    return false;
}

// NOTE: This function is huge and hard to work with. Some chunks were
// externalized into separate inline functions above. This leads to some
// duplication of the code and acts as an optimization fence for compiler.
//
// 0x573850
bool inven_ui_message_filter(TigMessage* msg)
{
    bool v45 = false;

    // Convert mouse position from screen coordinate system to centered 800x600
    // area.
    if (msg->type == TIG_MESSAGE_MOUSE) {
        TigRect rect = { 0, 0, 800, 600 };
        hrp_apply(&rect, GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);
        msg->data.mouse.x -= rect.x;
        msg->data.mouse.y -= rect.y;
    }

    if (inven_ui_drag_item_obj != OBJ_HANDLE_NULL) {
        scrollbar_ui_begin_ignore_events();
    } else {
        scrollbar_ui_end_ignore_events();
    }

    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        // CE: Ignore button events while dragging an item to steal it. This
        // prevents handling press events that would switch between the
        // inventory and paperdoll panels (these buttons form a radio group and
        // use the "pressed" event as their main handler, not "released" as with
        // usual buttons).
        if (!(inven_ui_mode == INVEN_UI_MODE_STEAL
                && inven_ui_drag_item_obj != OBJ_HANDLE_NULL)) {
            switch (msg->data.button.state) {
            case TIG_BUTTON_STATE_PRESSED:
                // 0x573897
                if (inven_ui_message_filter_handle_button_pressed(msg)) {
                    return true;
                }
                break;
            case TIG_BUTTON_STATE_RELEASED:
                // 0x573A95
                if (inven_ui_message_filter_handle_button_released(msg)) {
                    return true;
                }
                break;
            case TIG_BUTTON_STATE_MOUSE_INSIDE:
                // 0x573BAF
                if (inven_ui_mssage_filter_handle_button_hovered(msg)) {
                    return true;
                }
                break;
            }
        }
        break;
    case TIG_MESSAGE_MOUSE:
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_IDLE) {
            // 0x573C80
            inven_ui_message_filter_handle_mouse_idle(msg);
        } else if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_MOVE) {
            // 0x573D61
            if (inven_ui_message_filter_handle_mouse_move(msg)) {
                return true;
            }
        } else if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN
            && !dword_68346C
            && inven_ui_mode != INVEN_UI_MODE_IDENTIFY
            && inven_ui_mode != INVEN_UI_MODE_NPC_IDENTIFY
            && inven_ui_mode != INVEN_UI_MODE_NPC_REPAIR) {
            // 0x574080
            if (inven_ui_message_filter_handle_mouse_lbutton_down(msg)) {
                return true;
            }
        } else if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP
            || (msg->data.mouse.event == TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP && sub_54EB50())) {
            // 0x574559
            if (inven_ui_message_filter_handle_mouse_lbutton_up(msg, &v45)) {
                return true;
            }
        } else if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP
            && inven_ui_drag_item_obj == OBJ_HANDLE_NULL
            && (inven_ui_mode == INVEN_UI_MODE_INVENTORY
                || inven_ui_mode == INVEN_UI_MODE_BARTER
                || inven_ui_mode == INVEN_UI_MODE_LOOT
                || inven_ui_mode == INVEN_UI_MODE_STEAL)) {
            // 0x5742A4
            if (inven_ui_message_filter_handle_mouse_rbutton_up(msg, &v45)) {
                return true;
            }
        }
        break;
    }

    if (inven_ui_mode == INVEN_UI_MODE_BARTER
        && inven_ui_drag_item_obj == OBJ_HANDLE_NULL
        && intgame_mode_get() != INTGAME_MODE_QUANTITY
        && (critter_is_sleeping(qword_682C78) || !critter_is_active(qword_682C78))) {
        v45 = true;
    }

    if (v45 || dword_739F58) {
        inven_ui_destroy();
        return true;
    }

    return false;
}

// 0x574FD0
void sub_574FD0(bool next)
{
    int64_t follower_obj;
    int64_t pc_obj;
    int mode;

    sub_575770();

    follower_obj = qword_6813A8;

    if (next) {
        do {
            follower_obj = critter_follower_next(follower_obj);
        } while (!sub_575080(inven_ui_pc_obj, follower_obj));
    } else {
        do {
            follower_obj = critter_follower_prev(follower_obj);
        } while (!sub_575080(inven_ui_pc_obj, follower_obj));
    }

    if (follower_obj != qword_6813A8) {
        pc_obj = inven_ui_pc_obj;
        mode = inven_ui_mode;
        inven_ui_destroy();
        inven_ui_open(pc_obj, follower_obj, mode);
    }
}

// 0x575080
bool sub_575080(int64_t a1, int64_t a2)
{
    if (inven_ui_created && a2 == qword_6813A8) {
        return true;
    }

    if (ai_can_speak(a2, a1, false) != AI_SPEAK_OK) {
        return false;
    }

    if ((obj_field_int32_get(a2, OBJ_F_CRITTER_FLAGS) & (OCF_ANIMAL | OCF_MECHANICAL)) != 0) {
        return false;
    }

    if ((obj_field_int32_get(a2, OBJ_F_SPELL_FLAGS) & OSF_SUMMONED) != 0) {
        return false;
    }

    return true;
}

// 0x575100
bool sub_575100(bool* a1)
{
    int64_t v1;

    if (inven_ui_mode != INVEN_UI_MODE_STEAL || qword_681450 != inven_ui_pc_obj) {
        return false;
    }

    v1 = inven_ui_drag_item_obj;
    sub_575770();

    if (skill_plant_item(inven_ui_pc_obj, qword_682C78, v1)) {
        *a1 = true;
    }

    return true;
}

// 0x575180
bool sub_575180(bool* a1)
{
    int64_t v1;

    if (inven_ui_mode != INVEN_UI_MODE_STEAL || qword_681450 == inven_ui_pc_obj) {
        return false;
    }

    v1 = inven_ui_drag_item_obj;
    sub_575770();

    if (skill_steal_item(inven_ui_pc_obj, qword_682C78, v1)) {
        *a1 = true;
    }

    return true;
}

// 0x575200
void sub_575200(int a1)
{
    if (inven_ui_mode == INVEN_UI_MODE_BARTER && qword_681450 != inven_ui_pc_obj) {
        if (dword_681440 == -1 || inven_ui_drag_item_obj != qword_681458) {
            sub_578330(inven_ui_drag_item_obj, qword_682C78);
        }

        if (dword_681440 != 0 || dword_6810FC) {
            if (item_gold_get(inven_ui_pc_obj) < dword_681440) {
                dialog_copy_npc_not_enough_money_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
                sub_575770();
            } else {
                sub_5788C0(inven_ui_drag_item_obj, inven_ui_pc_obj, a1, 1);
            }
        } else {
            sub_575770();
            dword_681440 = -1;
        }
    } else {
        sub_5788C0(inven_ui_drag_item_obj, inven_ui_pc_obj, a1, 1);
    }

    qword_681458 = OBJ_HANDLE_NULL;
}

// 0x575360
void sub_575360(int a1)
{
    int cost;

    if (inven_ui_mode != INVEN_UI_MODE_BARTER) {
        sub_5788C0(inven_ui_drag_item_obj, qword_6813A8, a1, 1);
        qword_681458 = OBJ_HANDLE_NULL;
        return;
    }

    if (qword_681450 != inven_ui_pc_obj) {
        if (!dword_6810FC) {
            sub_575770();
        } else {
            sub_5788C0(inven_ui_drag_item_obj, qword_6813A8, a1, 1);
        }
        qword_681458 = OBJ_HANDLE_NULL;
        return;
    }

    if (dword_681440 == -1 || inven_ui_drag_item_obj != qword_681458) {
        sub_578330(inven_ui_drag_item_obj, inven_ui_pc_obj);
    }

    if (dword_681440 != 0 || dword_6810FC) {
        cost = item_gold_get(qword_682C78);
        if (cost < dword_681440) {
            dword_681440 = cost;
        }

        sub_5788C0(inven_ui_drag_item_obj, qword_6813A8, a1, 1);
        qword_681458 = OBJ_HANDLE_NULL;
        dword_681440 = -1;
    } else {
        sub_575770();
        qword_681458 = OBJ_HANDLE_NULL;
        dword_681440 = -1;
    }
}

// 0x5754C0
void sub_5754C0(int x, int y)
{
    S5754C0* entry;

    entry = (S5754C0*)MALLOC(sizeof(*entry));
    entry->x = x;
    entry->y = y;
    entry->field_8 = inven_ui_drag_item_obj;
    entry->field_10 = qword_681450;
    entry->field_18 = inven_ui_pc_obj;
    entry->mode = inven_ui_mode;
    entry->field_24 = dword_6810E8;
    entry->field_28 = dword_681508;

    inven_ui_drag_item_obj = OBJ_HANDLE_NULL;

    sub_4A3230(obj_get_id(entry->field_8), sub_5755A0, entry, sub_575580, entry);
}

// 0x575580
bool sub_575580(void* userinfo)
{
    S5754C0* entry = (S5754C0*)userinfo;

    FREE(entry);

    return true;
}

// 0x5755A0
bool sub_5755A0(void* userinfo)
{
    S5754C0* entry = (S5754C0*)userinfo;
    tig_art_id_t art_id;
    int sound_id;
    TigArtFrameData art_frame_data;

    inven_ui_drag_item_obj = entry->field_8;
    sub_4A50D0(player_get_local_pc_obj(), entry->field_8);
    art_id = obj_field_int32_get(entry->field_8, OBJ_F_ITEM_INV_AID);

    if ((entry->x == -1 && entry->y == -1)
        || IS_WEAR_INV_LOC(entry->field_24)
        || IS_HOTKEY_INV_LOC(entry->field_24)) {
        entry->x = 0;
        entry->y = 0;
    } else {
        if (entry->field_10 == entry->field_18) {
            entry->x -= 368 + 32 * (entry->field_24 % 10);
            entry->y -= 49 + 32 * (entry->field_24 / 10);
        } else if (entry->mode == 1) {
            entry->x -= 8 + 32 * (entry->field_24 % 10);
            entry->y -= 209 - 32 * (entry->field_28 - entry->field_24 / 10);
        } else {
            entry->x -= 8 + 32 * (entry->field_24 % 10);
            entry->y -= 177 - 32 * (entry->field_28 - entry->field_24 / 10);
        }
    }

    // CE: Fix item dragging visual glitches by maintaining both item icon
    // position (which is now perfectly aligned to the occupied slots), and
    // arrow cursor position (which now stays intact). That makes dragging
    // within the inventories more predicable.
    if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
        int width;
        int height;

        // Retrieve icon size and scale it to slot size.
        item_inv_icon_size(entry->field_8, &width, &height);
        width *= 32;
        height *= 32;

        // Adjust position to the center of the occupied slots.
        entry->x -= (width - art_frame_data.width) / 2;
        entry->y -= (height - art_frame_data.height) / 2;

        // Slightly move icon to the top and left, which gives the illusion of
        // depth.
        entry->x += 4;
        entry->y += 4;
    } else {
        // NOTE: This should not normally happen, but if it is, here is the
        // fallback (original approach).
        entry->x += 2;
        entry->y += 2;
    }

    tig_mouse_hide();
    tig_mouse_cursor_set_art_id(art_id);
    tig_art_interface_id_create(0, 0, 0, 0, &art_id);

    // CE: Maintain cursor position. Without adjustment for hot point the cursor
    // jumps down when button is pressed down.
    if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
        entry->x -= art_frame_data.hot_x;
        entry->y -= art_frame_data.hot_y;
        tig_mouse_cursor_overlay(art_id, entry->x, entry->y);
        entry->x += art_frame_data.hot_x;
        entry->y += art_frame_data.hot_y;
    } else {
        // NOTE: This should not normally happen.
        tig_mouse_cursor_overlay(art_id, entry->x, entry->y);
    }

    tig_mouse_cursor_set_offset(entry->x, entry->y);
    tig_mouse_show();

    redraw_inven(false);

    sound_id = sfx_item_sound(entry->field_8, entry->field_10, OBJ_HANDLE_NULL, ITEM_SOUND_PICKUP);
    gsound_play_sfx(sound_id, 1);

    FREE(entry);

    return true;
}

// 0x575770
void sub_575770()
{
    int64_t parent_obj;
    int sound_id;

    if (inven_ui_drag_item_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (!inven_ui_created) {
        if (inven_ui_pc_obj == OBJ_HANDLE_NULL) {
            inven_ui_pc_obj = player_get_local_pc_obj();
        }

        if (IS_HOTKEY_INV_LOC(dword_6810E8)) {
            sound_id = sfx_item_sound(inven_ui_drag_item_obj, qword_681450, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
            gsound_play_sfx(sound_id, 1);

            if (!sub_57F260()) {
                item_insert(inven_ui_drag_item_obj, qword_681450, dword_6810E8);
            }

            inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
            return;
        }

        if (!inven_ui_created) {
            sub_575930();
            return;
        }
    }

    if (item_parent(inven_ui_drag_item_obj, &parent_obj)
        && obj_field_int32_get(inven_ui_drag_item_obj, OBJ_F_ITEM_INV_LOCATION) != -1) {
        if (!tig_net_is_active() || !multiplayer_is_locked()) {
            sub_4A51C0(player_get_local_pc_obj(), inven_ui_drag_item_obj);
        }

        sound_id = sfx_item_sound(inven_ui_drag_item_obj, qword_681450, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
        gsound_play_sfx(sound_id, 1);

        sub_57F260();
        inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        redraw_inven(false);
    } else {
        sub_575930();
    }
}

// 0x575930
void sub_575930()
{
    int sound_id;
    int inventory_location;

    if (!inven_ui_created) {
        if (inven_ui_pc_obj == OBJ_HANDLE_NULL) {
            inven_ui_pc_obj = player_get_local_pc_obj();
        }
    }

    if (!sub_4642C0(inven_ui_drag_item_obj, qword_681450)) {
        if (IS_WEAR_INV_LOC(dword_6810E8)
            && item_wield_get(qword_681450, dword_6810E8) == OBJ_HANDLE_NULL) {
            item_insert(inven_ui_drag_item_obj, qword_681450, dword_6810E8);
        } else if (IS_HOTKEY_INV_LOC(dword_6810E8)
            && !sub_465690(qword_681450, dword_6810E8)) {
            item_insert(inven_ui_drag_item_obj, qword_681450, dword_6810E8);
        } else if (!IS_WEAR_INV_LOC(dword_6810E8)
            && !IS_HOTKEY_INV_LOC(dword_6810E8)
            && qword_681450 == inven_ui_pc_obj
            && item_inventory_slots_has_room_for(inven_ui_drag_item_obj, inven_ui_pc_obj, dword_6810E8, dword_68111C)) {
            item_insert(inven_ui_drag_item_obj, qword_681450, dword_6810E8);
        } else if (!IS_WEAR_INV_LOC(dword_6810E8)
            && !IS_HOTKEY_INV_LOC(dword_6810E8)
            && qword_681450 == qword_6813A8
            && item_inventory_slots_has_room_for(inven_ui_drag_item_obj, qword_6813A8, dword_6810E8, dword_681518)) {
            item_insert(inven_ui_drag_item_obj, qword_681450, dword_6810E8);
        } else if (item_check_insert(inven_ui_drag_item_obj, qword_681450, &inventory_location) == ITEM_CANNOT_OK) {
            item_insert(inven_ui_drag_item_obj, qword_681450, inventory_location);
        } else {
            sub_575BE0();
        }
    }

    sub_4A51C0(player_get_local_pc_obj(), inven_ui_drag_item_obj);

    sound_id = sfx_item_sound(inven_ui_drag_item_obj, qword_681450, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
    gsound_play_sfx(sound_id, 1);

    inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
}

// 0x575BE0
void sub_575BE0()
{
    int64_t location;
    tig_sound_handle_t sound_id;

    location = obj_field_int64_get(qword_681450, OBJ_F_LOCATION);
    sub_466E50(inven_ui_drag_item_obj, location);

    sound_id = sfx_item_sound(inven_ui_drag_item_obj, qword_681450, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
    gsound_play_sfx(sound_id, 1);
}

// 0x575C50
void sub_575C50(int64_t obj)
{
    if (inven_ui_drag_item_obj != OBJ_HANDLE_NULL && obj == inven_ui_drag_item_obj) {
        sub_4A51C0(player_get_local_pc_obj(), inven_ui_drag_item_obj);
        inven_ui_drag_item_obj = OBJ_HANDLE_NULL;
        intgame_refresh_cursor();
        redraw_inven(false);
    }
}

// 0x575CB0
int sub_575CB0(int x, int y, int64_t* parent_obj_ptr)
{
    int idx;
    int col;
    int row;

    switch (inven_ui_mode) {
    case INVEN_UI_MODE_INVENTORY:
    case INVEN_UI_MODE_NPC_IDENTIFY:
    case INVEN_UI_MODE_NPC_REPAIR:
        *parent_obj_ptr = inven_ui_pc_obj;

        for (idx = 0; idx < 9; idx++) {
            if (x >= inven_ui_inventory_paperdoll_inv_slot_rects[idx].x
                && y - 41 >= inven_ui_inventory_paperdoll_inv_slot_rects[idx].y
                && x < inven_ui_inventory_paperdoll_inv_slot_rects[idx].x + inven_ui_inventory_paperdoll_inv_slot_rects[idx].width
                && y - 41 < inven_ui_inventory_paperdoll_inv_slot_rects[idx].y + inven_ui_inventory_paperdoll_inv_slot_rects[idx].height) {
                return idx + 1000;
            }
        }

        break;
    case INVEN_UI_MODE_BARTER:
        if (inven_ui_target_panel == INVEN_UI_PANEL_PAPERDOLL) {
            *parent_obj_ptr = qword_682C78;

            for (idx = 0; idx < 9; idx++) {
                if (x >= inven_ui_barter_npc_paperdoll_inv_slot_rects[idx].x
                    && y - 41 >= inven_ui_barter_npc_paperdoll_inv_slot_rects[idx].y
                    && x < inven_ui_barter_npc_paperdoll_inv_slot_rects[idx].x + inven_ui_barter_npc_paperdoll_inv_slot_rects[idx].width
                    && y - 41 < inven_ui_barter_npc_paperdoll_inv_slot_rects[idx].y + inven_ui_barter_npc_paperdoll_inv_slot_rects[idx].height) {
                    return idx + 1000;
                }
            }
        } else {
            *parent_obj_ptr = qword_6813A8;

            if (x - 8 >= 0 && y - 209 >= 0) {
                col = (x - 8) / 32;
                row = (y - 209) / 32;
                if (col < 10 && row < 7) {
                    return col + 10 * (dword_681508 + row);
                }
            }
        }

        break;
    default:
        if (inven_ui_target_panel == INVEN_UI_PANEL_PAPERDOLL) {
            *parent_obj_ptr = qword_682C78;

            for (idx = 0; idx < 9; idx++) {
                if (x >= inven_ui_loot_npc_paperdoll_inv_slot_rects[idx].x
                    && y - 41 >= inven_ui_loot_npc_paperdoll_inv_slot_rects[idx].y
                    && x < inven_ui_loot_npc_paperdoll_inv_slot_rects[idx].x + inven_ui_loot_npc_paperdoll_inv_slot_rects[idx].width
                    && y - 41 < inven_ui_loot_npc_paperdoll_inv_slot_rects[idx].y + inven_ui_loot_npc_paperdoll_inv_slot_rects[idx].height) {
                    return idx + 1000;
                }
            }
        } else {
            *parent_obj_ptr = qword_6813A8;

            if (x - 8 >= 0 && y - 177 >= 0) {
                col = (x - 8) / 32;
                row = (y - 177) / 32;
                if (col < 10 && row < 8) {
                    return col + 10 * (dword_681508 + row);
                }
            }
        }

        break;
    }

    *parent_obj_ptr = inven_ui_pc_obj;

    if (inven_ui_panel == INVEN_UI_PANEL_PAPERDOLL) {
        for (idx = 0; idx < 9; idx++) {
            if (x >= inven_ui_barter_pc_paperdoll_inv_slot_rects[idx].x
                && y - 41 >= inven_ui_barter_pc_paperdoll_inv_slot_rects[idx].y
                && x < inven_ui_barter_pc_paperdoll_inv_slot_rects[idx].x + inven_ui_barter_pc_paperdoll_inv_slot_rects[idx].width
                && y - 41 < inven_ui_barter_pc_paperdoll_inv_slot_rects[idx].y + inven_ui_barter_pc_paperdoll_inv_slot_rects[idx].height) {
                return idx + 1000;
            }
        }
    } else {
        if (x - 368 >= 0 && y - 49 >= 0) {
            col = (x - 368) / 32;
            row = (y - 49) / 32;
            if (col < 10 && row < 12) {
                return col + 10 * row;
            }
        }
    }

    return -1;
}

// 0x575FA0
int64_t sub_575FA0(int x, int y, int64_t* parent_obj_ptr)
{
    int inventory_location;
    int64_t parent_obj;
    int64_t item_obj = OBJ_HANDLE_NULL;

    inventory_location = sub_575CB0(x, y, &parent_obj);
    if (inventory_location != -1) {
        if (IS_WEAR_INV_LOC(inventory_location)) {
            item_obj = item_wield_get(parent_obj, inventory_location);
        } else if (IS_HOTKEY_INV_LOC(inventory_location)) {
            item_obj = sub_465690(parent_obj, inventory_location);
        } else {
            if (parent_obj == inven_ui_pc_obj) {
                if (dword_68111C[inventory_location] != 0) {
                    item_obj = obj_arrayfield_handle_get(parent_obj,
                        OBJ_F_CRITTER_INVENTORY_LIST_IDX,
                        dword_68111C[inventory_location] - 1);
                }
            } else {
                if (dword_681518[inventory_location] != 0) {
                    item_obj = obj_arrayfield_handle_get(parent_obj,
                        obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER
                            ? OBJ_F_CONTAINER_INVENTORY_LIST_IDX
                            : OBJ_F_CRITTER_INVENTORY_LIST_IDX,
                        dword_681518[inventory_location] - 1);
                }
            }
        }

        if (item_obj != OBJ_HANDLE_NULL) {
            if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_OFF) != 0) {
                return OBJ_HANDLE_NULL;
            }
            if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_NO_DISPLAY) != 0) {
                return OBJ_HANDLE_NULL;
            }
        }
    }

    if (parent_obj_ptr != NULL) {
        *parent_obj_ptr = parent_obj;
    }

    return item_obj;
}

// 0x576100
void inven_ui_update(int64_t obj)
{
    if (!inven_ui_created) {
        return;
    }

    if (obj == inven_ui_pc_obj) {
        item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
        redraw_inven(false);
    } else if (obj == qword_6813A8 || obj == qword_682C78) {
        item_inventory_slots_get(qword_6813A8, dword_681518);
        redraw_inven(false);
    } else if (obj == OBJ_HANDLE_NULL) {
        item_inventory_slots_get(inven_ui_pc_obj, dword_68111C);
        if (qword_6813A8 != OBJ_HANDLE_NULL) {
            item_inventory_slots_get(qword_6813A8, dword_681518);
        }
        redraw_inven(false);
    }
}

// 0x5761D0
void redraw_inven_fix_bad_inven_obj(int64_t obj)
{
    int type;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int i;
    int j;
    int k;
    int64_t item_obj;
    tig_art_id_t art_id;
    int inventory_location;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    for (i = 0; i < cnt; i++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, i);
        for (j = i + 1; j < cnt; j++) {
            if (item_obj == obj_arrayfield_handle_get(obj, inventory_list_fld, j)) {
                tig_debug_printf("redraw_inven_fix_bad_inven_obj: WARNING: Found duplicate handle for object in inventory...CORRECTING!\n");
                cnt--;
                for (k = j; k < cnt; k++) {
                    item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, k + 1);
                    obj_arrayfield_obj_set(obj, inventory_list_fld, k, item_obj);
                }

                obj_arrayfield_length_set(obj, inventory_list_fld, cnt);
                obj_field_int32_set(obj, inventory_num_fld, cnt);
                j--;
            }
        }
    }

    for (i = 0; i < cnt; i++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, i);
        if (redraw_inven_fix_bad_inven(item_obj, obj) && i < cnt - 1) {
            cnt = obj_field_int32_get(obj, inventory_num_fld);
            i--;
        }
    }

    for (i = 0; i < cnt; i++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, i);

        art_id = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_AID);
        if (tig_art_item_id_disposition_get(art_id) != TIG_ART_ITEM_DISPOSITION_GROUND) {
            continue;
        }

        art_id = tig_art_item_id_disposition_set(art_id, TIG_ART_ITEM_DISPOSITION_INVENTORY);
        obj_field_int32_set(item_obj, OBJ_F_ITEM_INV_AID, art_id);

        inventory_location = item_inventory_location_get(item_obj);
        if (IS_WEAR_INV_LOC(inventory_location) || IS_HOTKEY_INV_LOC(inventory_location)) {
            continue;
        }

        if (item_check_remove(item_obj) == ITEM_CANNOT_OK) {
            item_drop_nearby(item_obj);
            item_arrange_inventory(item_obj, false);
            if (item_check_insert(item_obj, obj, &inventory_location) == ITEM_CANNOT_OK) {
                if (!item_transfer_ex(item_obj, obj, inventory_location)) {
                    cnt--;
                }
            } else {
                cnt--;
            }
            i = -1;
        } else {
            tig_debug_printf("Warning! A object with bad inventory art could not be removed and re-inserted. It may cause inventory graphic errors.\n");
        }
    }
}

// 0x5764B0
bool redraw_inven_fix_bad_inven(int64_t a1, int64_t a2)
{
    int64_t parent_obj;

    if (!item_parent(a1, &parent_obj)) {
        tig_debug_printf("Warning: redraw_inven_fix_bad_inven called on item that doesn't think it has a parent.\n");
        item_force_remove(a1, a2);
        return true;
    }

    if (parent_obj != a2) {
        tig_debug_printf("Warning: redraw_inven_fix_bad_inven called on item with different parent.\n");
        item_force_remove(a1, a2);
        return true;
    }

    return false;
}

// 0x576520
void redraw_inven(bool a1)
{
    TigArtBlitInfo art_blit_info;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigRect src_rect;
    TigRect dst_rect;
    TigFont font_desc;
    TigPaletteModifyInfo palette_modify_info;
    MesFileEntry mes_file_entry1;
    MesFileEntry mes_file_entry2;
    TigRect* text_rects;
    char str[80];
    int portrait;
    size_t pos;
    int index;
    bool filled_inv_slots[9];
    int inventory_cnt;
    int64_t item_obj;
    int item_type;
    unsigned int item_flags;
    int inventory_location;
    int weapon_min_str;
    bool weapon_too_heavy;

    art_blit_info.art_id = sub_5782D0();
    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

    // 0x5765A5
    switch (inven_ui_mode) {
    case INVEN_UI_MODE_BARTER:
        if (intgame_examine_portrait(inven_ui_pc_obj, qword_682C78, &portrait)) {
            portrait_draw_native(qword_682C78, portrait, inven_ui_window_handle, 183, 33);
        } else {
            tig_art_interface_id_create(portrait, 0, 0, 0, &(art_blit_info.art_id));
            tig_art_frame_data(art_blit_info.art_id, &art_frame_data);

            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.width;

            dst_rect.x = 183;
            dst_rect.y = 33;
            dst_rect.width = art_frame_data.width;
            dst_rect.height = art_frame_data.width;

            art_blit_info.flags = 0;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
        }

        tig_font_push(dword_682418);
        src_rect.x = 215;
        src_rect.y = 102;
        src_rect.width = 200;
        src_rect.height = 20;

        font_desc.str = byte_682C8C;
        font_desc.width = 0;
        tig_font_measure(&font_desc);

        src_rect.x -= font_desc.width / 2;
        tig_window_text_write(inven_ui_window_handle, font_desc.str, &src_rect);
        tig_font_pop();

        tig_font_push(dword_682C74);
        src_rect.x = 89;
        src_rect.y = 128;
        src_rect.width = 253;
        src_rect.height = 30;

        if (dword_6810FC && byte_68241C[0] == '\0') {
            int encumbrance_level;
            int carry_weight;

            mes_file_entry1.num = 9;
            mes_get_msg(inven_ui_mes_file, &mes_file_entry1);

            mes_file_entry2.num = 6;
            mes_get_msg(inven_ui_mes_file, &mes_file_entry2);

            sprintf(byte_68241C,
                "%s: %d %s\n",
                mes_file_entry1.str,
                item_total_weight(qword_682C78),
                mes_file_entry2.str);
            pos = strlen(byte_68241C);

            mes_file_entry1.num = 10;
            mes_get_msg(inven_ui_mes_file, &mes_file_entry1);

            encumbrance_level = critter_encumbrance_level_get(qword_682C78);
            carry_weight = stat_level_get(qword_682C78, STAT_CARRY_WEIGHT);

            sprintf(&(byte_68241C[pos]),
                "%s: %s (%d)\n",
                mes_file_entry1.str,
                critter_encumbrance_level_name(encumbrance_level),
                critter_encumbrance_level_ratio(encumbrance_level) * carry_weight / 100);
        } else {
            pos = 0;
        }

        tig_window_text_write(inven_ui_window_handle, byte_68241C, &src_rect);

        if (pos != 0) {
            byte_68241C[0] = '\0';
        }
        tig_font_pop();
        break;
    case INVEN_UI_MODE_LOOT:
    case INVEN_UI_MODE_STEAL:
    case INVEN_UI_MODE_IDENTIFY:
        if (intgame_examine_portrait(inven_ui_pc_obj, qword_682C78, &portrait)) {
            portrait_draw_native(qword_682C78, portrait, inven_ui_window_handle, inven_ui_loot_target_portrait_rect.x, inven_ui_loot_target_portrait_rect.y);
        } else {
            tig_art_interface_id_create(portrait, 0, 0, 0, &(art_blit_info.art_id));
            tig_art_frame_data(art_blit_info.art_id, &art_frame_data);

            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.width;

            dst_rect.x = inven_ui_loot_target_portrait_rect.x;
            dst_rect.y = inven_ui_loot_target_portrait_rect.y;
            dst_rect.width = art_frame_data.width;
            dst_rect.height = art_frame_data.width;

            art_blit_info.flags = 0;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
        }
        break;
    case INVEN_UI_MODE_NPC_IDENTIFY:
    case INVEN_UI_MODE_NPC_REPAIR:
        if (intgame_examine_portrait(inven_ui_pc_obj, qword_682C78, &portrait)) {
            portrait_draw_32x32(qword_682C78, portrait, inven_ui_window_handle, 141, 36);
        } else {
            tig_art_interface_id_create(portrait, 0, 0, 0, &(art_blit_info.art_id));
            tig_art_frame_data(art_blit_info.art_id, &art_frame_data);

            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.width;

            dst_rect.x = 141;
            dst_rect.y = 36;
            dst_rect.width = art_frame_data.width / 2;
            dst_rect.height = art_frame_data.width / 2;

            art_blit_info.flags = 0;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
        }

        tig_font_push(dword_682C74);
        src_rect = inven_ui_npc_action_rect;
        tig_window_text_write(inven_ui_window_handle, byte_68241C, &src_rect);
        tig_font_pop();
        break;
    }

    // 0x576B04
    tig_art_interface_id_create(221, 0, 0, 0, &(art_blit_info.art_id));
    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.width;

    dst_rect.x = 358;
    dst_rect.y = 0;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.width;

    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

    if (inven_ui_panel == INVEN_UI_PANEL_PAPERDOLL) {
        tig_art_interface_id_create(341, 0, 0, 0, &(art_blit_info.art_id));
        if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
            return;
        }

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.x = 361;
        dst_rect.y = 0;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.height;

        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
    }

    // 0x576C38
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY) {
        text_rects = inven_ui_inventory_summary_rects;
    } else {
        if (inven_ui_panel == INVEN_UI_PANEL_PAPERDOLL) {
            text_rects = inven_ui_barter_pc_summary_rects;
        } else {
            text_rects = NULL;
        }
    }

    if (text_rects != NULL) {
        int encumbrance_level;
        int carry_weight;

        tig_font_push(dword_682C74);

        mes_file_entry1.num = 9;
        mes_get_msg(inven_ui_mes_file, &mes_file_entry1);

        mes_file_entry2.num = 6;
        mes_get_msg(inven_ui_mes_file, &mes_file_entry2);

        sprintf(str,
            "%s: %d %s",
            mes_file_entry1.str,
            item_total_weight(inven_ui_pc_obj),
            mes_file_entry2.str);
        tig_window_text_write(inven_ui_window_handle, str, &(text_rects[INVEN_UI_SUMMARY_WEIGHT]));

        mes_file_entry1.num = 10;
        mes_get_msg(inven_ui_mes_file, &mes_file_entry1);

        encumbrance_level = critter_encumbrance_level_get(inven_ui_pc_obj);
        carry_weight = stat_level_get(inven_ui_pc_obj, STAT_CARRY_WEIGHT);

        sprintf(str,
            "%s: %s (%d)",
            mes_file_entry1.str,
            critter_encumbrance_level_name(encumbrance_level),
            critter_encumbrance_level_ratio(encumbrance_level) * carry_weight / 100);
        tig_window_text_write(inven_ui_window_handle, str, &(text_rects[INVEN_UI_SUMMARY_ENCUMBRANCE]));

        mes_file_entry1.num = 11;
        mes_get_msg(inven_ui_mes_file, &mes_file_entry1);

        sprintf(str,
            "%s: %d",
            mes_file_entry1.str,
            stat_level_get(inven_ui_pc_obj, STAT_SPEED));
        tig_window_text_write(inven_ui_window_handle, str, &(text_rects[INVEN_UI_SUMMARY_INFO_SPEED]));

        tig_font_pop();
    }

    // 0x576DFE
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        text_rects = inven_ui_inventory_total_stat_rects;
    } else {
        if (inven_ui_panel == INVEN_UI_PANEL_PAPERDOLL) {
            text_rects = inven_ui_barter_pc_total_stat_rects;
        } else {
            text_rects = NULL;
        }
    }

    if (text_rects != NULL) {
        tig_font_push(dword_681390);

        sprintf(str, "%d", item_total_attack(inven_ui_pc_obj));

        font_desc.str = str;
        font_desc.width = 0;
        tig_font_measure(&font_desc);

        src_rect.x = text_rects[0].x + text_rects[0].width - font_desc.width;
        src_rect.y = text_rects[0].y;
        src_rect.width = text_rects[0].width;
        src_rect.height = text_rects[0].height;
        tig_window_text_write(inven_ui_window_handle, str, &src_rect);

        sprintf(str, "%d", item_total_defence(inven_ui_pc_obj));
        tig_window_text_write(inven_ui_window_handle, str, &(text_rects[1]));

        tig_font_pop();
    }

    // 0x576F05
    tig_font_push(dword_68345C);
    for (index = 0; index < 5; index++) {
        int value;

        if (index == 0) {
            value = item_gold_get(inven_ui_pc_obj);
        } else {
            value = item_ammo_quantity_get(inven_ui_pc_obj, index - 1);
        }

        src_rect.x = inven_ui_gold_ammo_type_x[index];
        src_rect.y = inven_ui_gold_ammo_type_y[index];
        src_rect.width = 58;
        src_rect.height = 20;

        sprintf(str, "%d", value);
        tig_window_text_write(inven_ui_window_handle, str, &src_rect);
    }
    tig_font_pop();

    // 0x576FBE
    if (inven_ui_have_drop_box) {
        tig_art_interface_id_create(inven_ui_drop_box_image, 0, 0, 0, &(art_blit_info.art_id));
        if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
            return;
        }

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.width;

        dst_rect.x = 703;
        dst_rect.y = 344;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.width;

        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

        inven_ui_drop_box_frame.width = art_frame_data.width;
        inven_ui_drop_box_frame.height = art_frame_data.height;
    }

    // 0x577070
    if (inven_ui_have_gamble_box) {
        tig_art_interface_id_create(inven_ui_gamble_box_image, 0, 0, 0, &(art_blit_info.art_id));
        if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
            return;
        }

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.width;

        dst_rect.x = 703;
        dst_rect.y = 236;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.width;

        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

        inven_ui_gamble_box_frame.width = art_frame_data.width;
        inven_ui_gamble_box_frame.height = art_frame_data.height;
    }

    // 0x57711C
    if (inven_ui_have_use_box) {
        tig_art_interface_id_create(inven_ui_use_box_image, 0, 0, 0, &(art_blit_info.art_id));
        if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
            return;
        }

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.width;

        dst_rect.x = 703;
        dst_rect.y = 290;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.width;

        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

        inven_ui_use_box_frame.width = art_frame_data.width;
        inven_ui_use_box_frame.height = art_frame_data.height;
    }

    // 0x5771C8
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        text_rects = inven_ui_inventory_paperdoll_inv_slot_rects;
        memset(filled_inv_slots, 0, sizeof(filled_inv_slots));
    } else {
        if (inven_ui_panel == INVEN_UI_PANEL_PAPERDOLL) {
            text_rects = inven_ui_barter_pc_paperdoll_inv_slot_rects;
            memset(filled_inv_slots, 0, sizeof(filled_inv_slots));
        } else {
            text_rects = NULL;
        }
    }

    inventory_cnt = obj_field_int32_get(inven_ui_pc_obj, OBJ_F_CRITTER_INVENTORY_NUM);
    dst_rect = src_rect;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    for (index = 0; index < inventory_cnt; index++) {
        item_obj = obj_arrayfield_handle_get(inven_ui_pc_obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, index);
        if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_OFF) != 0) {
            continue;
        }

        item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
        if ((item_flags & OIF_NO_DISPLAY) != 0) {
            if (!a1) {
                continue;
            }
            item_flags &= ~OIF_NO_DISPLAY;
            obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, item_flags);
        }

        // 0x5772EE
        inventory_location = item_inventory_location_get(item_obj);
        if (IS_HOTKEY_INV_LOC(inventory_location)) {
            continue;
        }

        // 0x57730A
        if (IS_WEAR_INV_LOC(inventory_location)) {
            if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
                || inven_ui_panel == INVEN_UI_PANEL_PAPERDOLL
                || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
                || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
                weapon_too_heavy = false;
                if (inventory_location == ITEM_INV_LOC_WEAPON) {
                    weapon_min_str = item_weapon_min_strength(item_obj, inven_ui_pc_obj);
                    if (stat_level_get(inven_ui_pc_obj, STAT_STRENGTH) < weapon_min_str) {
                        dst_rect = text_rects[4];
                        tig_window_tint(inven_ui_window_handle,
                            &dst_rect,
                            tig_color_make(64, 0, 0),
                            0);
                        weapon_too_heavy = true;
                    }
                }

                switch (obj_field_int32_get(item_obj, OBJ_F_TYPE)) {
                case OBJ_TYPE_WEAPON:
                    art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_WEAPON_PAPER_DOLL_AID);
                    break;
                case OBJ_TYPE_ARMOR:
                    art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ARMOR_PAPER_DOLL_AID);
                    break;
                default:
                    art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_AID);
                    break;
                }

                if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) == TIG_OK) {
                    src_rect.x = 0;
                    src_rect.y = 0;
                    src_rect.height = art_frame_data.height;
                    src_rect.width = art_frame_data.width;

                    dst_rect.x = text_rects[inventory_location - 1000].x
                        + (text_rects[inventory_location - 1000].width - art_frame_data.width) / 2;
                    dst_rect.y = text_rects[inventory_location - 1000].y
                        + (text_rects[inventory_location - 1000].height - art_frame_data.height) / 2;
                    dst_rect.width = art_frame_data.width;
                    dst_rect.height = art_frame_data.height;

                    tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

                    if (weapon_too_heavy) {
                        mes_file_entry1.num = 8;
                        mes_get_msg(inven_ui_mes_file, &mes_file_entry1);

                        sprintf(str, "%s: %d", mes_file_entry1.str, weapon_min_str);
                        tig_font_push(dword_682C74);
                        font_desc.str = str;
                        font_desc.width = 0;
                        tig_font_measure(&font_desc);
                        dst_rect.height = font_desc.height;
                        dst_rect.y += dst_rect.height - font_desc.height;
                        tig_window_text_write(inven_ui_window_handle, str, &dst_rect);
                        tig_font_pop();
                    }
                }

                filled_inv_slots[inventory_location - 1000] = true;
            }

            continue;
        }

        // 0x5775A7
        if (inven_ui_panel == INVEN_UI_PANEL_INVENTORY) {
            int x;
            int y;
            int width;
            int height;

            x = 32 * (inventory_location % 10) + 368;
            y = 32 * (inventory_location / 10) + 8;
            item_inv_icon_size(item_obj, &width, &height);
            width *= 32;
            height *= 32;

            if (item_obj == qword_681458) {
                dst_rect.x = x;
                dst_rect.y = y;
                dst_rect.width = width;
                dst_rect.height = height;

                tig_window_tint(inven_ui_window_handle,
                    &dst_rect,
                    tig_color_make(64, 0, 0),
                    0);
            }

            art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_AID);
            if (tig_art_item_id_destroyed_get(obj_field_int32_get(item_obj, OBJ_F_CURRENT_AID)) != 0) {
                art_blit_info.art_id = tig_art_item_id_destroyed_set(art_blit_info.art_id, 0);
                tig_art_anim_data(art_blit_info.art_id, &art_anim_data);

                palette_modify_info.dst_palette = tig_palette_create();
                palette_modify_info.src_palette = art_anim_data.palette1;
                palette_modify_info.flags = TIG_PALETTE_MODIFY_TINT;
                palette_modify_info.tint_color = tig_color_make(255, 0, 0);
                tig_palette_modify(&palette_modify_info);

                art_blit_info.flags = TIG_ART_BLT_PALETTE_OVERRIDE;
                art_blit_info.palette = palette_modify_info.dst_palette;
            } else {
                palette_modify_info.dst_palette = NULL;
            }

            if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) == TIG_OK) {
                src_rect.x = 0;
                src_rect.y = 0;
                src_rect.height = art_frame_data.height;
                src_rect.width = art_frame_data.width;

                dst_rect.x = x + (width - art_frame_data.width) / 2;
                dst_rect.y = y + (height - art_frame_data.height) / 2;
                dst_rect.width = art_frame_data.width;
                dst_rect.height = art_frame_data.height;

                tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
            }

            if (palette_modify_info.dst_palette != NULL) {
                tig_palette_destroy(palette_modify_info.dst_palette);
            }
        }
    }

    // 0x577837
    art_blit_info.flags = 0;
    if (text_rects != NULL) {
        for (index = 0; index < 9; index++) {
            if (!filled_inv_slots[index]) {
                tig_art_interface_id_create(item_ui_item_silhouette_nums[index], 0, 0, 0, &(art_blit_info.art_id));

                dst_rect.x = text_rects[index].x;
                dst_rect.y = text_rects[index].y;
                dst_rect.width = text_rects[index].width;
                dst_rect.height = text_rects[index].height;

                src_rect.x = 0;
                src_rect.y = 0;
                src_rect.width = inven_ui_inventory_paperdoll_inv_slot_rects[index].width;
                src_rect.height = inven_ui_inventory_paperdoll_inv_slot_rects[index].height;

                tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
            }
        }
    }

    // 0x5778D8
    if (qword_6813A8 != OBJ_HANDLE_NULL
        && inven_ui_mode != INVEN_UI_MODE_NPC_IDENTIFY
        && inven_ui_mode != INVEN_UI_MODE_NPC_REPAIR) {
        int64_t target_obj;
        int v102;
        int v109;
        int v112;
        int inventory_list_fld;

        if (inven_ui_target_panel == INVEN_UI_PANEL_PAPERDOLL) {
            target_obj = qword_682C78;
            memset(filled_inv_slots, 0, sizeof(filled_inv_slots));

            if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
                tig_art_interface_id_create(337, 0, 0, 0, &(art_blit_info.art_id));
            } else {
                tig_art_interface_id_create(349, 0, 0, 0, &(art_blit_info.art_id));
            }
            if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
                return;
            }

            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.width;

            dst_rect.x = 0;
            dst_rect.y = 0;
            dst_rect.width = art_frame_data.width;
            dst_rect.height = art_frame_data.width;

            if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
                dst_rect.y = 157;
                text_rects = inven_ui_barter_npc_paperdoll_inv_slot_rects;
            } else {
                dst_rect.y = 134;
                text_rects = inven_ui_loot_npc_paperdoll_inv_slot_rects;
            }

            art_blit_info.flags = 0;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

            tig_font_push(dword_682C74);
            sprintf(str, "%d", item_total_attack(qword_682C78));
            font_desc.str = str;
            font_desc.width = 0;
            tig_font_measure(&font_desc);
            src_rect.x = inven_ui_barter_npc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].x + inven_ui_barter_npc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].width - font_desc.width;
            src_rect.y = inven_ui_barter_npc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].y;
            src_rect.width = inven_ui_barter_npc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].width;
            src_rect.height = inven_ui_barter_npc_total_stat_rects[INVEN_UI_TOTAL_ATTACK].height;
            tig_window_text_write(inven_ui_window_handle, str, &src_rect);
            sprintf(str, "%d", item_total_defence(qword_682C78));
            tig_window_text_write(inven_ui_window_handle, str, &(inven_ui_barter_npc_total_stat_rects[INVEN_UI_TOTAL_DEFENSE]));
            tig_font_pop();
        } else {
            target_obj = qword_6813A8;
            v112 = 8;
            if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
                v102 = 168;
                v109 = 224;
            } else {
                v102 = 136;
                v109 = 256;
            }
        }

        if (obj_field_int32_get(target_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
            inventory_cnt = obj_field_int32_get(target_obj, OBJ_F_CONTAINER_INVENTORY_NUM);
            inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
        } else {
            inventory_cnt = obj_field_int32_get(target_obj, OBJ_F_CRITTER_INVENTORY_NUM);
            inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
        }

        src_rect.x = 0;
        src_rect.y = 0;

        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;

        for (index = 0; index < inventory_cnt; index++) {
            item_obj = obj_arrayfield_handle_get(target_obj, inventory_list_fld, index);
            if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_OFF) != 0) {
                continue;
            }

            item_type = obj_field_int32_get(item_obj, OBJ_F_TYPE);
            item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
            if ((item_flags & OIF_NO_DISPLAY) != 0) {
                if (!a1 || (item_type != OBJ_TYPE_GOLD && item_type != OBJ_TYPE_AMMO)) {
                    continue;
                }
                item_flags &= ~OIF_NO_DISPLAY;
                obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, item_flags);
            }

            inventory_location = item_inventory_location_get(item_obj);

            art_blit_info.flags = 0;
            if (IS_HOTKEY_INV_LOC(inventory_location)) {
                continue;
            }

            if (IS_WEAR_INV_LOC(inventory_location)) {
                // 0x577CA0
                if (inven_ui_target_panel == INVEN_UI_PANEL_PAPERDOLL) {
                    weapon_too_heavy = false;
                    if (inventory_location == ITEM_INV_LOC_WEAPON) {
                        weapon_min_str = item_weapon_min_strength(item_obj, target_obj);
                        if (stat_level_get(target_obj, STAT_STRENGTH) < weapon_min_str) {
                            dst_rect = text_rects[4];
                            tig_window_tint(inven_ui_window_handle,
                                &dst_rect,
                                tig_color_make(64, 0, 0),
                                0);
                            weapon_too_heavy = true;
                        }
                    }

                    switch (item_type) {
                    case OBJ_TYPE_WEAPON:
                        art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_WEAPON_PAPER_DOLL_AID);
                        break;
                    case OBJ_TYPE_ARMOR:
                        art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ARMOR_PAPER_DOLL_AID);
                        break;
                    default:
                        art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_AID);
                        break;
                    }

                    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) == TIG_OK) {
                        src_rect.width = art_frame_data.width;
                        src_rect.height = art_frame_data.height;

                        dst_rect.width = art_frame_data.width * 2 / 3;
                        dst_rect.height = art_frame_data.height * 2 / 3;
                        dst_rect.x = text_rects[inventory_location - 1000].x + (text_rects[inventory_location - 1000].width - dst_rect.width) / 2;
                        dst_rect.y = text_rects[inventory_location - 1000].y + (text_rects[inventory_location - 1000].height - dst_rect.height) / 2;

                        tig_window_blit_art(inven_ui_window_handle, &art_blit_info);

                        if (weapon_too_heavy) {
                            mes_file_entry1.num = 8;
                            mes_get_msg(inven_ui_mes_file, &mes_file_entry1);
                            sprintf(str, "%s: %d", mes_file_entry1.str, weapon_min_str);
                            tig_font_push(dword_682C74);
                            font_desc.str = str;
                            font_desc.width = 0;
                            tig_font_measure(&font_desc);
                            dst_rect.y += dst_rect.height - font_desc.height;
                            dst_rect.height = font_desc.height;
                            tig_window_text_write(inven_ui_window_handle, str, &dst_rect);
                            tig_font_pop();
                        }

                        filled_inv_slots[inventory_location - 1000] = true;
                    }
                }

                continue;
            }

            // 0x577F41
            if (inven_ui_target_panel == INVEN_UI_PANEL_INVENTORY) {
                int x;
                int y;
                int width;
                int height;

                x = v112 + 32 * (inventory_location % 10);
                y = v102 + 32 * (inventory_location / 10 - dword_681508);

                if (y < v109 + v102) {
                    art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_AID);
                    if (tig_art_item_id_destroyed_get(obj_field_int32_get(item_obj, OBJ_F_CURRENT_AID))) {
                        art_blit_info.art_id = tig_art_item_id_destroyed_set(art_blit_info.art_id, 0);
                        tig_art_anim_data(art_blit_info.art_id, &art_anim_data);

                        palette_modify_info.dst_palette = tig_palette_create();
                        palette_modify_info.src_palette = art_anim_data.palette1;
                        palette_modify_info.flags = TIG_PALETTE_MODIFY_TINT;
                        palette_modify_info.tint_color = tig_color_make(255, 0, 0);
                        tig_palette_modify(&palette_modify_info);

                        art_blit_info.flags = TIG_ART_BLT_PALETTE_OVERRIDE;
                        art_blit_info.palette = palette_modify_info.dst_palette;
                    } else {
                        palette_modify_info.dst_palette = NULL;
                    }

                    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) == TIG_OK) {
                        if (y + art_frame_data.height > v102) {
                            src_rect.x = 0;
                            src_rect.y = 0;
                            src_rect.width = art_frame_data.width;
                            src_rect.height = art_frame_data.height;

                            item_inv_icon_size(item_obj, &width, &height);
                            x += (32 * width - art_frame_data.width) / 2;
                            y += (32 * height - art_frame_data.height) / 2;

                            if (y < v102) {
                                height = y - v102 + src_rect.height;
                                src_rect.y += v102 - y;
                                src_rect.height = height;
                                y = v102;
                            } else {
                                height = src_rect.height;
                            }

                            if (y + height > v102 + v109) {
                                height = v102 + v109 - y;
                                src_rect.height = height;
                            }

                            dst_rect.x = x;
                            dst_rect.y = y;
                            dst_rect.width = src_rect.width;
                            dst_rect.height = height;

                            tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
                        }
                    }

                    if (palette_modify_info.dst_palette != NULL) {
                        tig_palette_destroy(palette_modify_info.dst_palette);
                    }
                }
            }
        }

        art_blit_info.flags = 0;
        if (inven_ui_target_panel == INVEN_UI_PANEL_PAPERDOLL) {
            for (index = 0; index < 9; index++) {
                if (!filled_inv_slots[index]) {
                    tig_art_interface_id_create(item_ui_item_silhouette_nums[index], 0, 0, 0, &(art_blit_info.art_id));

                    dst_rect.x = text_rects[index].x;
                    dst_rect.y = text_rects[index].y;
                    dst_rect.width = text_rects[index].width;
                    dst_rect.height = text_rects[index].height;

                    src_rect.width = inven_ui_inventory_paperdoll_inv_slot_rects[index].width;
                    src_rect.height = inven_ui_inventory_paperdoll_inv_slot_rects[index].height;

                    tig_window_blit_art(inven_ui_window_handle, &art_blit_info);
                }
            }
        }
    }

    intgame_pc_lens_redraw();
    scrollbar_ui_control_redraw(inven_ui_target_inventory_scrollbar);
}

// 0x5782D0
tig_art_id_t sub_5782D0()
{
    tig_art_id_t art_id;
    int num;

    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        num = 223;
    } else if (inven_ui_mode == INVEN_UI_MODE_BARTER) {
        num = inven_ui_cycle_left_btn != -1 ? 825 : 235;
    } else {
        num = 234;
    }

    tig_art_interface_id_create(num, 0, 0, 0, &art_id);

    return art_id;
}

// 0x578330
void sub_578330(int64_t a1, int64_t a2)
{
    int gold;
    int pos;

    if (dword_6810FC) {
        dword_681440 = 0;
        return;
    }

    if (a2 == inven_ui_pc_obj) {
        dword_681440 = item_cost(a1, inven_ui_pc_obj, qword_682C78, 0);
        if (dword_681440 != 0) {
            gold = item_gold_get(qword_682C78);
            if (object_script_execute(a1, qword_682C78, inven_ui_pc_obj, SAP_BUY_OBJECT, 0)) {
                pos = 0;
            } else {
                dialog_copy_npc_normally_wont_buy_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
                pos = (int)strlen(byte_68241C);
            }

            if (dword_681440 > gold) {
                dialog_copy_npc_buy_for_less_msg(qword_682C78, inven_ui_pc_obj, byte_682804);
                sprintf(&(byte_68241C[pos]), byte_682804, dword_681440, gold);
            } else {
                dialog_copy_npc_buy_msg(qword_682C78, inven_ui_pc_obj, byte_682804);
                sprintf(&(byte_68241C[pos]), byte_682804, dword_681440);
            }
        } else {
            if ((obj_field_int32_get(a1, OBJ_F_ITEM_FLAGS) & OIF_STOLEN) == 0) {
                dialog_copy_npc_wont_buy_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
            } else {
                dialog_copy_npc_wont_buy_stolen_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
            }
        }
    } else {
        dword_681440 = item_cost(a1, qword_682C78, inven_ui_pc_obj, 0);
        if (dword_681440 != 0) {
            if ((obj_field_int32_get(a1, OBJ_F_ITEM_FLAGS) & OIF_WONT_SELL) != 0
                || IS_WEAR_INV_LOC(item_inventory_location_get(a1))) {
                dialog_copy_npc_normally_wont_sell_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
                pos = (int)strlen(byte_68241C);
            } else {
                pos = 0;
            }

            dialog_copy_npc_sell_msg(qword_682C78, inven_ui_pc_obj, byte_682804);
            sprintf(&(byte_68241C[pos]), byte_682804, dword_681440);

            if (basic_skill_training_get(inven_ui_pc_obj, BASIC_SKILL_HAGGLE) >= TRAINING_APPRENTICE) {
                int worth;
                int discount;

                worth = item_worth(a1);
                discount = 100 * (dword_681440 - worth) / worth;
                if (discount == 0) {
                    discount = 1;
                }

                sprintf(byte_682804, "(%d%%)", discount);
                strcat(byte_68241C, byte_682804);
            }
        } else {
            dialog_copy_npc_wont_sell_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
        }
    }

    redraw_inven(false);
}

// 0x5786C0
void sub_5786C0(int64_t obj)
{
    if (item_is_identified(obj)) {
        dialog_copy_npc_wont_identify_already_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
    } else {
        dword_681440 = spell_money(SPELL_DIVINE_MAGICK);
        dialog_copy_npc_identify_msg(qword_682C78, inven_ui_pc_obj, byte_682804);
        sprintf(byte_68241C, byte_682804, dword_681440);
    }
    redraw_inven(false);
}

// 0x578760
void sub_578760(int64_t obj)
{
    int hp_max;
    int hp_dam;
    tig_art_id_t art_id;

    hp_max = object_hp_max(obj);
    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    if (tig_art_item_id_destroyed_get(art_id) != 0) {
        if (tech_skill_training_get(qword_682C78, TECH_SKILL_REPAIR) != TRAINING_MASTER) {
            dialog_copy_npc_wont_repair_broken_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
            redraw_inven(false);
            return;
        }

        hp_dam = hp_max;
    } else {
        hp_dam = object_hp_damage_get(obj);
    }

    if (hp_dam != 0) {
        dword_681440 = hp_dam * (item_cost(obj, inven_ui_pc_obj, qword_682C78, 1) / 2) / hp_max;
        if (dword_681440 < 2) {
            dword_681440 = 2;
        }

        dialog_copy_npc_repair_msg(qword_682C78, inven_ui_pc_obj, byte_682804);
        sprintf(byte_68241C, byte_682804, dword_681440);
        redraw_inven(false);
    } else {
        dialog_copy_npc_wont_repair_undamaged_msg(qword_682C78, inven_ui_pc_obj, byte_68241C);
        redraw_inven(false);
    }
}

// 0x5788C0
void sub_5788C0(int64_t item_obj, int64_t target_obj, int new_inventory_location, int a4)
{
    int reason;
    int64_t parent_obj;
    int old_inventory_location;
    int sound_id;
    int obj_type;
    int qty_fld;
    int qty;

    sub_4A51C0(player_get_local_pc_obj(), item_obj);

    if (target_obj == qword_681450) {
        reason = item_check_remove(item_obj);
        if (reason == 0) {
            if (!item_parent(item_obj, &parent_obj)) {
                return;
            }

            old_inventory_location = item_inventory_location_get(item_obj);
            if (!IS_WEAR_INV_LOC(new_inventory_location)) {
                if (target_obj == inven_ui_pc_obj) {
                    item_inventory_slots_set(item_obj, old_inventory_location, dword_68111C, 0);
                    if (!item_inventory_slots_has_room_for(item_obj, target_obj, new_inventory_location, dword_68111C)) {
                        return;
                    }
                } else {
                    item_inventory_slots_set(item_obj, old_inventory_location, dword_681518, 0);
                    if (!item_inventory_slots_has_room_for(item_obj, target_obj, new_inventory_location, dword_681518)) {
                        return;
                    }
                }
            }

            critter_encumbrance_recalc_feedback_disable();

            item_remove(item_obj);

            if (item_check_insert(item_obj, target_obj, NULL)) {
                item_insert(item_obj, parent_obj, old_inventory_location);
            } else {
                item_insert(item_obj, target_obj, new_inventory_location);
            }
            critter_encumbrance_recalc_feedback_enable();
        } else {
            if (item_parent(item_obj, &parent_obj)) {
                item_error_msg(item_obj, reason);
            }
        }

        sound_id = sfx_item_sound(item_obj, target_obj, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
        gsound_play_sfx(sound_id, 1);
    } else {
        obj_type = obj_field_int32_get(item_obj, OBJ_F_TYPE);
        qword_739F70 = qword_681450;
        dword_739F80 = new_inventory_location;
        qword_739F78 = target_obj;
        qword_739F68 = item_obj;

        if ((a4 & 0x6) != 0) {
            dword_681440 = -1;
        }

        dword_739F84 = dword_681440;
        dword_739F60 = a4;

        if ((obj_type == OBJ_TYPE_GOLD || obj_type == OBJ_TYPE_AMMO)) {
            sub_575770();

            sub_462410(item_obj, &qty_fld);
            qty = obj_field_int32_get(item_obj, qty_fld);
            if (qty != 1) {
                if (inven_ui_mode == INVEN_UI_MODE_LOOT
                        && (a4 & 0x01) != 0
                        && target_obj == inven_ui_pc_obj
                    || (a4 & 0x20) != 0) {
                    sub_578B80(qty);
                } else {
                    intgame_mode_set(INTGAME_MODE_QUANTITY);
                    iso_interface_window_set(ROTWIN_TYPE_QUANTITY);
                    intgame_refresh_cursor();
                }

                return;
            }
        }

        sub_578B80(1);
    }
}

// 0x578B80
void sub_578B80(int a1)
{
    Packet81* pkt;

    pkt = (Packet81*)MALLOC(sizeof(*pkt));
    pkt->type = 81;
    pkt->field_5C = dword_739F84;
    pkt->field_8 = dword_739F60;

    if (qword_739F70 != OBJ_HANDLE_NULL) {
        pkt->field_28 = obj_get_id(qword_739F70);
    } else {
        pkt->field_28.type = OID_TYPE_NULL;
    }

    pkt->field_58 = dword_739F80;

    if (qword_739F68 != OBJ_HANDLE_NULL) {
        pkt->field_10 = obj_get_id(qword_739F68);
    } else {
        pkt->field_10.type = OID_TYPE_NULL;
    }

    if (qword_739F78 != OBJ_HANDLE_NULL) {
        pkt->field_40 = obj_get_id(qword_739F78);
    } else {
        pkt->field_40.type = OID_TYPE_NULL;
    }

    if (qword_682C78 != OBJ_HANDLE_NULL) {
        pkt->field_70 = obj_get_id(qword_682C78);
    } else {
        pkt->field_70.type = OID_TYPE_NULL;
    }

    if (qword_6813A8 != OBJ_HANDLE_NULL) {
        pkt->field_88 = obj_get_id(qword_6813A8);
    } else {
        pkt->field_88.type = OID_TYPE_NULL;
    }

    if (inven_ui_pc_obj != OBJ_HANDLE_NULL) {
        pkt->field_A0 = obj_get_id(inven_ui_pc_obj);
    } else {
        pkt->field_A0.type = OID_TYPE_NULL;
    }

    pkt->field_60 = a1;
    pkt->field_68 = sub_529520();

    if (tig_net_is_active()) {
        object_examine(objp_perm_lookup(pkt->field_10),
            objp_perm_lookup(pkt->field_10),
            byte_682BEC);
        objid_id_to_str(byte_6812FC, pkt->field_10);

        if (pkt->field_40.type != OID_TYPE_NULL) {
            object_examine(objp_perm_lookup(pkt->field_40),
                objp_perm_lookup(pkt->field_40),
                byte_6813C0);
        } else {
            strcpy(byte_6813C0, "nobody");
        }

        if (pkt->field_28.type != OID_TYPE_NULL) {
            object_examine(objp_perm_lookup(pkt->field_28),
                objp_perm_lookup(pkt->field_28),
                byte_681468);
        } else {
            strcpy(byte_681468, "nobody");
        }

        if (pkt->field_60 > 1) {
            tig_debug_printf("UI: InvenItemTransfer - %d %ss(%s) going from %s to %s\n",
                pkt->field_60,
                byte_682BEC,
                byte_6812FC,
                byte_681468,
                byte_6813C0);
        } else {
            tig_debug_printf("UI: InvenItemTransfer - %s(%s) going from %s to %s\n",
                byte_682BEC,
                byte_6812FC,
                byte_681468,
                byte_6813C0);
        }
    }

    sub_578EA0(pkt);
}

// 0x578EA0
bool sub_578EA0(Packet81* pkt)
{
    int64_t v1;
    int64_t v2;
    int64_t v3;
    int64_t v4;
    int64_t v5;
    int64_t v6;
    int qty;
    int gold_amt;
    int ammo_type;
    unsigned int flags;
    int cost;
    int inventory_location;
    bool error;
    unsigned int item_type;
    int worth;

    qty = pkt->field_60;
    flags = pkt->field_8;
    cost = pkt->field_5C;
    inventory_location = pkt->field_58;

    v1 = objp_perm_lookup(pkt->field_28);
    v2 = objp_perm_lookup(pkt->field_10);
    v3 = objp_perm_lookup(pkt->field_40);
    v4 = objp_perm_lookup(pkt->field_70);
    v5 = objp_perm_lookup(pkt->field_88);
    v6 = objp_perm_lookup(pkt->field_A0);

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        tig_net_send_app_all(pkt, sizeof(*pkt));
    }

    FREE(pkt);

    if (qty == 0) {
        return true;
    }

    if (v6 != OBJ_HANDLE_NULL
        && player_is_local_pc_obj(v6)
        && (flags & 0x18) != 0) {
        gold_amt = qty * item_cost(v2, v4, v6, false);
        if (gold_amt > item_gold_get(v6)) {
            dialog_copy_npc_gamble_msg(v4, v6, 4, byte_682804);
            sprintf(byte_68241C, byte_682804, gold_amt);
            return true;
        }
    }

    error = false;
    item_type = obj_field_int32_get(v2, OBJ_F_TYPE);

    if ((flags & 0x10) == 0) {
        if (item_type == OBJ_TYPE_GOLD) {
            if (!tig_net_is_active() || tig_net_is_host()) {
                if (!item_gold_transfer(v1, v3, qty, v2)) {
                    return true;
                }

                if ((flags & 0x06) != 0) {
                    int64_t loc = obj_field_int64_get(v1, OBJ_F_LOCATION);
                    int64_t gold_obj = item_gold_create(qty, loc);
                    if (v6 != OBJ_HANDLE_NULL) {
                        int sound_id = sfx_item_sound(gold_obj, v1, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
                        sub_4EED00(v6, sound_id);
                    }
                    if ((flags & 0x04) != 0) {
                        object_pickup(gold_obj, v1);
                        if (v6 != OBJ_HANDLE_NULL && player_is_local_pc_obj(v6)) {
                            sub_573630(gold_obj);
                            qword_681450 = v1;
                        }
                    } else if ((flags & 0x02) != 0) {
                        object_pickup(gold_obj, v1);
                        sub_466E50(gold_obj, loc);
                    }
                }
            } else {
                if ((flags & 0x04) != 0) {
                    qword_681450 = v1;
                }
            }
        } else if (item_type == OBJ_TYPE_AMMO) {
            ammo_type = obj_field_int32_get(v2, OBJ_F_AMMO_TYPE);
            if (!tig_net_is_active() || tig_net_is_host()) {
                if (!item_ammo_transfer(v1, v3, qty, ammo_type, v2)) {
                    return true;
                }

                if ((flags & 0x06) != 0) {
                    int64_t loc = obj_field_int64_get(v1, OBJ_F_LOCATION);
                    int64_t ammo_obj = item_ammo_create(qty, ammo_type, loc);
                    if (v6 != OBJ_HANDLE_NULL) {
                        int sound_id = sfx_item_sound(ammo_obj, v1, OBJ_HANDLE_NULL, ITEM_SOUND_PICKUP);
                        sub_4EED00(v6, sound_id);
                    }
                    if ((flags & 0x04) != 0) {
                        object_pickup(ammo_obj, v1);
                        if (v6 != OBJ_HANDLE_NULL && player_is_local_pc_obj(v6)) {
                            sub_573630(ammo_obj);
                            qword_681450 = v1;
                        }
                    } else if ((flags & 0x02) != 0) {
                        object_pickup(ammo_obj, v1);
                        sub_466E50(ammo_obj, loc);
                    }
                }
            } else {
                if ((flags & 0x04) != 0) {
                    qword_681450 = v1;
                }
            }
        } else {
            if ((flags & 0x04) != 0) {
                if (!tig_net_is_active() || tig_net_is_host()) {
                    if (item_check_remove(v2) == ITEM_CANNOT_OK) {
                        item_remove(v2);
                    } else {
                        error = true;
                    }
                }
            } else if ((flags & 0x01) != 0) {
                if (!tig_net_is_active() || tig_net_is_host()) {
                    if (v3 != v6) {
                        if (IS_WEAR_INV_LOC(inventory_location)) {
                            if (!item_transfer_ex(v2, v4, inventory_location)) {
                                error = true;
                            }
                        } else {
                            if (!item_transfer_ex(v2, v5, inventory_location)) {
                                error = true;
                            }
                        }
                    } else {
                        if (!item_transfer_ex(v2, v3, inventory_location)) {
                            error = true;
                        }
                    }

                    if (v6 != OBJ_HANDLE_NULL) {
                        int sound_id = sfx_item_sound(v2, v3, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
                        sub_4EED00(v6, sound_id);
                    }
                }
            } else if ((flags & 0x02) != 0) {
                if (!tig_net_is_active() || tig_net_is_host()) {
                    item_drop(v2);
                }
                if (v6 != OBJ_HANDLE_NULL) {
                    int sound_id = sfx_item_sound(v2, v3, OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
                    sub_4EED00(v6, sound_id);
                }
            }
        }
    }

    // 68
    if ((flags & 0x08) == 0) {
        if ((flags & 0x10) != 0) {
            cost = item_cost(v2, v4, v6, false);
        }
    } else {
        cost = 0;
    }

    worth = item_worth(v2) * qty;
    if (!error) {
        if (cost > 0) {
            gold_amt = qty * cost;
            if (!tig_net_is_active() || tig_net_is_host()) {
                if (v3 == v6) {
                    if (item_gold_transfer(v3, v4, gold_amt, OBJ_HANDLE_NULL)) {
                        if (gold_amt < worth) {
                            gsound_play_sfx(8158, 1);
                        } else {
                            sub_4C11D0(v4, v6, gold_amt - worth);
                        }
                    } else {
                        dialog_copy_npc_not_enough_money_msg(v4, v6, byte_68241C);
                        error = true;
                    }
                } else {
                    if (item_gold_transfer(v4, v1, gold_amt, OBJ_HANDLE_NULL)) {
                        if (worth < gold_amt) {
                            gsound_play_sfx(8158, 1);
                        } else {
                            sub_4C11D0(v4, v6, worth - gold_amt);
                        }
                    } else {
                        error = true;
                    }
                }
            }
        }
    }

    if (error) {
        if ((flags & 0x10) == 0) {
            if (!tig_net_is_active() || tig_net_is_host()) {
                if (item_type == OBJ_TYPE_GOLD) {
                    item_gold_transfer(v3, v1, qty, OBJ_HANDLE_NULL);
                } else if (item_type == OBJ_TYPE_AMMO) {
                    item_ammo_transfer(v3, v1, qty, ammo_type, OBJ_HANDLE_NULL);
                }
            }
        } else {
            gsound_play_sfx(8158, 1);
        }
    }

    // 85
    if (!error
        && v3 != OBJ_HANDLE_NULL
        && v3 != v6
        && inven_ui_mode == INVEN_UI_MODE_BARTER
        && !dword_6810FC
        && (!tig_net_is_active() || tig_net_is_host())) {
        unsigned int item_flags = obj_field_int32_get(v2, OBJ_F_ITEM_FLAGS);
        if ((item_flags & OIF_IDENTIFIED) == 0) {
            item_flags |= OIF_IDENTIFIED;
            mp_obj_field_int32_set(v2, OBJ_F_ITEM_FLAGS, item_flags);
        }
    }

    if (inven_ui_created && player_is_local_pc_obj(v6)) {
        redraw_inven(false);
        item_inventory_slots_get(v6, dword_68111C);
        if (v5 != OBJ_HANDLE_NULL) {
            item_inventory_slots_get(v5, dword_681518);
        }
    }

    if (v6 != OBJ_HANDLE_NULL
        && player_is_local_pc_obj(v6)
        && (flags & 0x04) != 0
        && !error) {
        dword_739F58 = 1;
    }

    if ((flags & 0x08) != 0) {
        dialog_copy_npc_gamble_msg(v4, v6, 0, byte_68241C);
        sub_4C11D0(v4, v6, -worth);
    } else if ((flags & 0x10) != 0) {
        dialog_copy_npc_gamble_msg(v4, v6, 1, byte_682804);
        sprintf(byte_68241C, byte_682804, gold_amt);
    }

    return true;
}

// 0x579760
int64_t sub_579760()
{
    return qword_739F68;
}

// 0x579770
void sub_579770(int64_t from_obj, int64_t to_obj)
{
    int64_t* items;
    int cnt;
    int idx;
    int inventory_location;

    qword_681450 = from_obj;

    cnt = item_list_get(from_obj, &items);
    for (idx = 0; idx < cnt; idx++) {
        if ((obj_field_int32_get(items[idx], OBJ_F_FLAGS) & OF_OFF) == 0
            && (obj_field_int32_get(items[idx], OBJ_F_ITEM_FLAGS) & OIF_NO_DISPLAY) == 0
            && item_check_insert(items[idx], to_obj, &inventory_location) == ITEM_CANNOT_OK) {
            sub_5788C0(items[idx], to_obj, inventory_location, 0x21);
        }
    }

    item_list_free(items);
}

// 0x579840
bool sub_579840(int64_t obj, bool a2)
{
    int v1;
    bool v2 = false;
    int training;
    size_t pos;

    if (qword_681450 == inven_ui_pc_obj) {
        if (!a2) {
            dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 2, byte_68241C);
        }
        return false;
    }

    if (basic_skill_points_get(qword_682C78, BASIC_SKILL_GAMBLING) == 0) {
        if (!a2) {
            dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 9, byte_68241C);
        }
        return false;
    }

    v1 = item_cost(obj, qword_682C78, inven_ui_pc_obj, 0);
    if (v1 > skill_gambling_max_item_cost(inven_ui_pc_obj)) {
        if (!a2) {
            dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 3, byte_68241C);
        }
        return false;
    }

    if (v1 > item_gold_get(inven_ui_pc_obj)) {
        if (!a2) {
            dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 4, byte_682804);
            sprintf(byte_68241C, byte_682804, v1);
        }
        return false;
    }

    training = basic_skill_training_get(qword_682C78, BASIC_SKILL_GAMBLING);
    if (IS_WEAR_INV_LOC(item_inventory_location_get(obj))) {
        if (training < TRAINING_EXPERT) {
            if (!a2) {
                dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 5, byte_68241C);
            }
            return false;
        }

        v2 = true;
    }

    if ((obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS) & OIF_WONT_SELL) != 0) {
        if (training < TRAINING_MASTER) {
            if (!a2) {
                dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 6, byte_68241C);
            }
            return false;
        }

        v2 = true;
    }

    if (!a2) {
        if (v2) {
            dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 7, byte_68241C);
            strcat(byte_68241C, " ");
            pos = strlen(byte_68241C);
        } else {
            pos = 0;
        }

        dialog_copy_npc_gamble_msg(qword_682C78, inven_ui_pc_obj, 8, byte_682804);
        sprintf(&(byte_68241C[pos]), byte_682804, v1);
    }

    return true;
}

// 0x579B60
void sub_579B60(int64_t obj)
{
    SkillInvocation skill_invocation;

    if (sub_579840(obj, 1)) {
        skill_invocation_init(&skill_invocation);
        sub_4440E0(inven_ui_pc_obj, &(skill_invocation.source));
        sub_4440E0(qword_682C78, &(skill_invocation.target));
        sub_4440E0(obj, &(skill_invocation.item));
        skill_invocation.skill = SKILL_GAMBLING;
        skill_invocation.modifier = 0;
        if (skill_invocation_run(&skill_invocation)) {
            // FIXME: Unclear flags.
            sub_5788C0(obj, inven_ui_pc_obj, 0, (skill_invocation.flags & SKILL_INVOCATION_SUCCESS) != 0 ? 0x12 : 0x11);
            dword_681440 = -1;
        }
    }
}

// 0x579C40
void inven_ui_target_inventory_scrollbar_create()
{
    ScrollbarUiControlInfo info;

    scrollbar_ui_end_ignore_events();

    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        return;
    }

    info.flags = 0x39F;
    info.field_3C = sub_579E00;
    info.value = dword_681508;
    info.max_value = dword_681510;
    info.min_value = 0;

    if (inven_ui_mode == 1) {
        info.scrollbar_rect = inven_ui_barter_scrollbar_rect;
        info.content_rect.x = 8;
        info.content_rect.y = 168;
        info.content_rect.width = inven_ui_barter_scrollbar_rect.width + 320;
        info.content_rect.height = 224;
    } else {
        info.scrollbar_rect = inven_ui_loot_scrollbar_rect;
        info.content_rect.x = 8;
        info.content_rect.y = 136;
        info.content_rect.width = inven_ui_loot_scrollbar_rect.width + 320;
        info.content_rect.height = 256;
    }

    info.field_40 = sub_579E30;
    info.field_2C = 1;
    scrollbar_ui_control_create(&inven_ui_target_inventory_scrollbar, &info, inven_ui_window_handle);
    scrollbar_ui_control_redraw(inven_ui_target_inventory_scrollbar);
}

// 0x579D70
void inven_ui_target_inventory_scrollbar_destroy()
{
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        return;
    }

    scrollbar_ui_control_destroy(inven_ui_target_inventory_scrollbar);
    scrollbar_ui_end_ignore_events();
}

// 0x579DA0
void inven_ui_target_inventory_scrollbar_show()
{
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        return;
    }

    scrollbar_ui_control_show(inven_ui_target_inventory_scrollbar);
}

// 0x579DD0
void inven_ui_target_inventory_scrollbar_hide()
{
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        return;
    }

    scrollbar_ui_control_hide(inven_ui_target_inventory_scrollbar);
}

// 0x579E00
void sub_579E00(int a1)
{
    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        return;
    }

    dword_681508 = a1;
    redraw_inven(false);
}

// 0x579E30
void sub_579E30(TigRect* rect)
{
    TigArtBlitInfo blit_info;

    if (inven_ui_mode == INVEN_UI_MODE_INVENTORY
        || inven_ui_mode == INVEN_UI_MODE_NPC_IDENTIFY
        || inven_ui_mode == INVEN_UI_MODE_NPC_REPAIR) {
        return;
    }

    blit_info.flags = 0;
    blit_info.art_id = sub_5782D0();
    blit_info.src_rect = rect;
    blit_info.dst_rect = rect;
    tig_window_blit_art(inven_ui_window_handle, &blit_info);
}
