#include "game/mp_utils.h"

#include "game/a_name.h"
#include "game/area.h"
#include "game/critter.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/map.h"
#include "game/material.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/player.h"
#include "game/portal.h"
#include "game/proto.h"
#include "game/random.h"
#include "game/sector.h"
#include "game/spell.h"
#include "game/tb.h"
#include "game/tf.h"
#include "game/tile.h"
#include "game/townmap.h"
#include "game/ui.h"
#include "game/wall.h"

// 0x4ED510
void sub_4ED510(AnimID anim_id, int64_t loc, AnimRunInfo* run_info)
{
    int size;
    Packet16* pkt;

    // NOTE: The size of this packet is unclear. It consists of two chunks -
    // fixed length header and rotations array. The rotations array appended
    // at +0x58 offset implying header size (including padding) to be 0x58,
    // but the original code allocates 0x60 bytes for it.
    size = sizeof(*pkt) + 8 + run_info->path.max;
    pkt = (Packet16*)MALLOC(size);
    pkt->type = 16;
    pkt->anim_id = anim_id;
    pkt->loc = loc;
    pkt->path_flags = run_info->path.flags;
    pkt->path_base_rot = run_info->path.baseRot;
    pkt->path_curr = run_info->path.curr;
    pkt->path_max = run_info->path.max;
    pkt->path_subsequence = run_info->path.subsequence;
    pkt->path_max_path_length = run_info->path.maxPathLength;
    pkt->path_abs_max_path_length = run_info->path.absMaxPathLength;
    pkt->field_48 = run_info->path.field_E8;
    pkt->field_50 = run_info->path.field_F0;
    pkt->offset_x = obj_field_int32_get(run_info->anim_obj, OBJ_F_OFFSET_X);
    pkt->offset_y = obj_field_int32_get(run_info->anim_obj, OBJ_F_OFFSET_Y);
    pkt->art_id = obj_field_int32_get(run_info->anim_obj, OBJ_F_CURRENT_AID);
    pkt->anim_flags = run_info->flags;
    memcpy((uint8_t*)(pkt + 1), run_info->path.rotations, run_info->path.max);
    tig_net_send_app_all(&pkt, size);
    FREE(pkt);
}

// 0x4ED630
void sub_4ED630(Packet16* pkt, AnimRunInfo* run_info)
{
    run_info->path.flags = pkt->path_flags;
    run_info->path.baseRot = pkt->path_base_rot;
    run_info->path.curr = pkt->path_curr;
    run_info->path.max = pkt->path_max;
    run_info->path.subsequence = pkt->path_subsequence;
    run_info->path.maxPathLength = pkt->path_max_path_length;
    run_info->path.absMaxPathLength = pkt->path_abs_max_path_length;
    run_info->path.field_E8 = pkt->field_48;
    run_info->path.field_F0 = pkt->field_50;
    memcpy(run_info->path.rotations, (uint8_t*)(pkt + 1), run_info->path.max);
}

// 0x4ED6C0
bool sub_4ED6C0(int64_t obj)
{
    Packet29 pkt;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        return sub_463540(obj);
    }

    pkt.type = 29;
    sub_4F0640(obj, &(pkt.oid));
    tig_net_send_app_all(&pkt, sizeof(pkt));

    return false;
}

// 0x4ED720
void mp_critter_fatigue_damage_set(int64_t obj, int damage)
{
    PacketCritterFatigueDamageSet pkt;

    critter_fatigue_damage_set(obj, damage);

    if (tig_net_is_active()) {
        pkt.type = 35;
        sub_4F0640(obj, &(pkt.oid));
        pkt.dam = damage;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4ED780
bool sub_4ED780(int64_t obj, int quest, int state, int64_t a4)
{
    int player;
    char str[256];
    char v1[40];
    PacketQuestStateSet pkt;

    player = multiplayer_find_slot_from_obj(obj);

    if (player == -1) {
        return false;
    }

    object_examine(obj, obj, str);

    // FIXME: Probably useless.
    objid_id_to_str(v1, obj_get_id(obj));
    objid_id_to_str(v1, obj_get_id(a4));

    pkt.type = 39;
    sub_4440E0(obj, &(pkt.field_8));
    pkt.quest = quest;
    pkt.state = state;
    sub_4440E0(a4, &(pkt.field_40));
    tig_net_send_app_except(&pkt, sizeof(pkt), player);

    return true;
}

// 0x4ED8B0
bool mp_object_create(int name, int64_t loc, int64_t* obj_ptr)
{
    if (tig_net_is_active()) {
        if (tig_net_is_host()
            && object_create(sub_4685A0(name), loc, obj_ptr)) {
            Packet70 pkt;

            pkt.type = 70;
            pkt.subtype = 0;
            pkt.s0.name = name;
            pkt.s0.oid = obj_get_id(*obj_ptr);
            pkt.s0.loc = loc;
            pkt.s0.field_30 = 0;
            tig_net_send_app_all(&pkt, sizeof(pkt));
            return true;
        }

        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    return object_create(sub_4685A0(name), loc, obj_ptr);
}

// 0x4ED9E0
void mp_object_destroy(int64_t obj)
{
    PacketObjectDestroy pkt;

    pkt.type = 72;
    pkt.oid = obj_get_id(obj);
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4EDA60
void sub_4EDA60(UiMessage* ui_message, int player, int a3)
{
    Packet84* pkt;
    int extra_length;

    extra_length = (int)strlen(ui_message->str) + 1;
    pkt = (Packet84*)MALLOC(sizeof(*pkt) + extra_length);
    pkt->type = 84;
    pkt->extra_length = extra_length;
    pkt->field_8 = a3;
    pkt->ui_message = *ui_message;
    strncpy((char*)(pkt + 1), ui_message->str, extra_length);
    tig_net_send_app(pkt, sizeof(*pkt) + extra_length, player);
    FREE(pkt);
}

// 0x4EDB70
void mp_tf_add(int64_t obj, int tf_type, const char* str)
{
    PacketTextFloater* pkt;
    int extra_length;

    extra_length = (int)strlen(str) + 1;
    pkt = (PacketTextFloater*)MALLOC(sizeof(*pkt) + extra_length);
    pkt->type = 85;
    pkt->extra_length = extra_length;
    pkt->oid = obj_get_id(obj);
    pkt->tf_type = tf_type;
    strncpy((char*)(pkt + 1), str, extra_length);
    tig_net_send_app_all(pkt, sizeof(*pkt) + extra_length);
    FREE(pkt);
}

// 0x4EDC00
void mp_reaction_adj(int64_t npc_obj, int64_t pc_obj, int value)
{
    PacketReactionAdj pkt;

    pkt.type = 87;
    pkt.npc_oid = obj_get_id(npc_obj);
    pkt.pc_oid = obj_get_id(pc_obj);
    pkt.value = value;
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4EDC70
void mp_trap_mark_known(int64_t pc_obj, int64_t trap_obj, int reason)
{
    PacketTrapMarkKnown pkt;

    pkt.type = 90;
    pkt.pc_oid = obj_get_id(pc_obj);
    pkt.trap_oid = obj_get_id(trap_obj);
    pkt.reason = reason;
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4EDCE0
void sub_4EDCE0(int64_t obj, tig_art_id_t art_id)
{
    Packet92 pkt;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        object_set_current_aid(obj, art_id);

        if (tig_net_is_host()) {
            pkt.type = 92;
            pkt.oid = obj_get_id(obj);
            pkt.art_id = art_id;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }
}

// 0x4EDD50
void mp_ui_update_inven(int64_t obj)
{
    PacketUpdateInven pkt;

    ui_update_inven(obj);

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 94;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EDDB0
void mp_handle_ui_update_inven(PacketUpdateInven* pkt)
{
    int64_t obj;

    obj = objp_perm_lookup(pkt->oid);
    if (obj != OBJ_HANDLE_NULL) {
        ui_update_inven(obj);
    }
}

// 0x4EDDE0
void sub_4EDDE0(int64_t obj)
{
    Packet95 pkt;

    if (obj == OBJ_HANDLE_NULL || player_is_local_pc_obj(obj)) {
        sub_4601C0();
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 95;
        pkt.oid.type = OID_TYPE_NULL;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EDE40
void sub_4EDE40(Packet95* pkt)
{
    if (pkt->oid.type == OID_TYPE_NULL
        || player_is_local_pc_obj(objp_perm_lookup(pkt->oid))) {
        sub_4601C0();
    }
}

// 0x4EDE80
void mp_item_arrange_inventory(int64_t obj, bool vertical)
{
    PacketArrangeInventory pkt;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        pkt.type = 96;
        pkt.oid = obj_get_id(obj);
        pkt.vertical = vertical;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EDEE0
void mp_handle_item_arrange_inventory(PacketArrangeInventory* pkt)
{
    int64_t obj;

    if (tig_net_is_host()) {
        obj = objp_perm_lookup(pkt->oid);
        if (obj != OBJ_HANDLE_NULL) {
            item_arrange_inventory(obj, pkt->vertical);
        }
    }
}

// 0x4EDF20
void sub_4EDF20(int64_t obj, int64_t location, int dx, int dy, bool a7)
{
    Packet99 pkt;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        if (a7 && player_is_local_pc_obj(obj)) {
            location_origin_set(location);
        }
        object_move_to_location(obj, location, dx, dy);
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 99;
        pkt.oid = obj_get_id(obj);
        pkt.location = location;
        pkt.dx = dx;
        pkt.dy = dy;
        pkt.field_30 = a7;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EDFF0
void sub_4EDFF0(Packet99* pkt)
{
    int64_t obj;

    if (tig_net_is_host()) {
        obj = objp_perm_lookup(pkt->oid);
        object_move_to_location(obj, pkt->location, pkt->dx, pkt->dy);

        if (pkt->field_30 && player_is_local_pc_obj(obj)) {
            location_origin_set(pkt->location);
        }
    }
}

// 0x4EE060
void mp_item_activate(int64_t owner_obj, int64_t item_obj)
{
    Packet100 pkt;

    if (player_is_local_pc_obj(owner_obj)) {
        ui_item_activate(owner_obj, item_obj);
        return;
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 100;
        pkt.subtype = 5;
        sub_4F0640(owner_obj, &(pkt.d.s.field_8));
        sub_4F0640(item_obj, &(pkt.d.s.field_20));
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EE0F0
void mp_ui_schematic_feedback(bool success, int64_t primary_obj, int64_t secondary_obj)
{
    Packet100 pkt;

    ui_schematic_feedback(success, primary_obj, secondary_obj);

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 100;
        pkt.subtype = 4;
        pkt.d.c.field_8 = success;
        pkt.d.c.field_10 = obj_get_id(primary_obj);
        pkt.d.c.field_28 = obj_get_id(secondary_obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EE190
void mp_ui_follower_refresh()
{
    Packet100 pkt;

    ui_follower_refresh();

    if (tig_net_is_active()) {
        pkt.type = 100;
        pkt.subtype = 0;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EE1D0
void sub_4EE1D0(int64_t obj)
{
    Packet100 pkt;

    ui_refresh_health_bar(obj);

    if (tig_net_is_active()) {
        pkt.type = 100;
        pkt.subtype = 1;
        pkt.d.b.field_8 = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EE230
void mp_ui_toggle_primary_button(UiPrimaryButton btn, bool on, int client_id)
{
    if (sub_529520() != client_id) {
        Packet100 pkt;

        pkt.type = 100;
        pkt.subtype = 2;
        pkt.d.a.field_8 = btn;
        pkt.d.a.field_C = on;
        tig_net_send_app(&pkt, sizeof(pkt), client_id);

        return;
    }

    ui_toggle_primary_button(btn, on);
}

// 0x4EE290
void mp_ui_written_start_type(int64_t obj, WrittenType written_type, int num)
{
    if (player_get_local_pc_obj() != obj) {
        int client_id = multiplayer_find_slot_from_obj(obj);
        if (client_id != -1) {
            Packet100 pkt;

            pkt.type = 100;
            pkt.subtype = 3;
            pkt.d.a.field_8 = written_type;
            pkt.d.a.field_C = num;
            tig_net_send_app(&pkt, sizeof(pkt), client_id);
        }

        return;
    }

    ui_written_start_type(written_type, num);
}

// 0x4EE310
void mp_ui_show_inven_loot(int64_t pc_obj, int64_t target_obj)
{
    if (!player_is_local_pc_obj(pc_obj)) {
        if (tig_net_is_active() && tig_net_is_host()) {
            Packet100 pkt;

            pkt.type = 100;
            pkt.subtype = 6;
            sub_4F0640(pc_obj, &(pkt.d.s.field_8));
            sub_4F0640(pc_obj, &(pkt.d.s.field_20));
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }

        return;
    }

    ui_show_inven_loot(pc_obj, target_obj);
}

// 0x4EE3A0
void sub_4EE3A0(int64_t obj, int64_t a2)
{
    Packet100 pkt;

    if (player_is_local_pc_obj(obj)) {
        sub_4604F0(obj, a2);
        return;
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 100;
        pkt.subtype = 7;
        sub_4F0640(obj, &(pkt.d.s.field_8));
        sub_4F0640(obj, &(pkt.d.s.field_20));
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EE430
void mp_ui_show_inven_identify(int64_t pc_obj, int64_t target_obj)
{
    if (!player_is_local_pc_obj(pc_obj)) {
        if (tig_net_is_active() && tig_net_is_host()) {
            Packet100 pkt;

            pkt.type = 100;
            pkt.subtype = 8;
            sub_4F0640(pc_obj, &(pkt.d.s.field_8));
            sub_4F0640(pc_obj, &(pkt.d.s.field_20));
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }

        return;
    }

    ui_show_inven_identify(pc_obj, target_obj);
}

// 0x4EE4C0
void sub_4EE4C0(int64_t obj, int64_t a2)
{
    Packet100 pkt;

    if (player_is_local_pc_obj(obj)) {
        sub_460930(obj, a2);
        return;
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 100;
        pkt.subtype = 10;
        sub_4F0640(obj, &(pkt.d.s.field_8));
        sub_4F0640(obj, &(pkt.d.s.field_20));
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EE550
void mp_ui_show_inven_npc_identify(int64_t pc_obj, int64_t target_obj)
{
    if (!player_is_local_pc_obj(pc_obj)) {
        if (tig_net_is_active() && tig_net_is_host()) {
            Packet100 pkt;

            pkt.type = 100;
            pkt.subtype = 9;
            sub_4F0640(pc_obj, &(pkt.d.s.field_8));
            sub_4F0640(pc_obj, &(pkt.d.s.field_20));
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }

        return;
    }

    ui_show_inven_npc_identify(pc_obj, target_obj);
}

// 0x4EE5E0
void sub_4EE5E0(Packet100* pkt)
{
    int64_t v1;
    int64_t v2;
    Packet100 new_pkt;

    switch (pkt->subtype) {
    case 0:
        ui_follower_refresh();
        break;
    case 1:
        ui_refresh_health_bar(objp_perm_lookup(pkt->d.b.field_8));
        break;
    case 2:
        ui_toggle_primary_button(pkt->d.a.field_8, pkt->d.a.field_C);
        break;
    case 3:
        ui_toggle_primary_button(pkt->d.a.field_8, pkt->d.a.field_C);
        break;
    case 4:
        ui_schematic_feedback(pkt->d.c.field_8,
            objp_perm_lookup(pkt->d.c.field_10),
            objp_perm_lookup(pkt->d.c.field_28));
        break;
    case 5:
        sub_4F0690(pkt->d.s.field_20, &v2);
        sub_4F0690(pkt->d.s.field_8, &v1);
        if (player_is_local_pc_obj(v1)) {
            ui_item_activate(v1, v2);
        }
        break;
    case 6:
        sub_4F0690(pkt->d.s.field_8, &v1);
        sub_4F0690(pkt->d.s.field_20, &v2);
        if (player_is_local_pc_obj(v1)) {
            ui_show_inven_loot(v1, v2);
        }
        break;
    case 7:
        sub_4F0690(pkt->d.s.field_8, &v1);
        sub_4F0690(pkt->d.s.field_20, &v2);
        if (player_is_local_pc_obj(v1)) {
            sub_4604F0(v1, v2);
        }
        break;
    case 8:
        sub_4F0690(pkt->d.s.field_8, &v1);
        sub_4F0690(pkt->d.s.field_20, &v2);
        if (player_is_local_pc_obj(v1)) {
            ui_show_inven_identify(v1, v2);
        }
        break;
    case 9:
        sub_4F0690(pkt->d.s.field_8, &v1);
        sub_4F0690(pkt->d.s.field_20, &v2);
        if (player_is_local_pc_obj(v1)) {
            ui_show_inven_npc_identify(v1, v2);
        }
        break;
    case 10:
        sub_4F0690(pkt->d.s.field_8, &v1);
        sub_4F0690(pkt->d.s.field_20, &v2);
        if (player_is_local_pc_obj(v1)) {
            sub_460930(v1, v2);
        }
        break;
    case 11:
        sub_4F0690(pkt->d.x.field_8, &v1);
        if (player_is_local_pc_obj(v1)) {
            item_error_msg(v1, pkt->d.x.field_20);
        }
        // FIXME: Fallthrough?
    case 12:
        sub_4F0690(pkt->d.z.field_8, &v1);
        sub_4F0690(pkt->d.z.field_20, &v2);
        switch (pkt->d.z.field_3C) {
        case 0:
            if (tig_net_is_host()) {
                if (sub_460C50(v1, v2, pkt->d.z.field_38)) {
                    new_pkt = *pkt;
                    new_pkt.d.z.field_3C = 1;
                    tig_net_send_app(&new_pkt, sizeof(new_pkt), sub_52A530());
                }
            }
            break;
        case 1:
            if (tig_net_is_host()) {
                if (sub_460C80(v1, v2, pkt->d.z.field_38)) {
                    new_pkt = *pkt;
                    new_pkt.d.z.field_3C = 2;
                    tig_net_send_app_all(&new_pkt, sizeof(new_pkt));
                }
            }
            break;
        case 2:
            if (tig_net_is_host()) {
                sub_460CB0(v1, v2, pkt->d.z.field_38);

                new_pkt = *pkt;
                new_pkt.d.z.field_3C = 3;
                tig_net_send_app(&new_pkt, sizeof(new_pkt), sub_52A530());
            }
            break;
        case 3:
            if (tig_net_is_host()) {
                ui_inven_create(v1, v2, pkt->d.z.field_38);
            }
            break;
        }
        break;
    }
}

// 0x4EEB90
void mp_handle_area_reset_last_known(PacketAreaResetLastKnown* pkt)
{
    int64_t pc_obj;

    pc_obj = objp_perm_lookup(pkt->oid);
    multiplayer_lock();
    area_reset_last_known_area(pc_obj);
    multiplayer_unlock();
}

// 0x4EEBD0
void mp_handle_area_known_set(PacketAreaKnownSet* pkt)
{
    int64_t pc_obj;

    pc_obj = objp_perm_lookup(pkt->oid);
    multiplayer_lock();
    area_set_known(pc_obj, pkt->area);
    multiplayer_unlock();
}

// 0x4EEC10
void mp_object_locked_set(int64_t obj, int locked)
{
    object_locked_set(obj, locked);

    if (tig_net_is_active()) {
        PacketObjectLock pkt;

        pkt.type = 103;
        pkt.oid = obj_get_id(obj);
        pkt.locked = locked;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EEC80
void sub_4EEC80(PacketObjectLock* pkt)
{
    object_locked_set(objp_perm_lookup(pkt->oid), pkt->locked);
}

// 0x4EECB0
void mp_gsound_play_sfx(int sound_id)
{
    gsound_play_sfx(sound_id, 1);

    if (tig_net_is_active()) {
        PacketPlaySound pkt;

        pkt.type = 106;
        pkt.subtype = 0;
        pkt.sound_id = sound_id;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EED00
void sub_4EED00(int64_t obj, int sound_id)
{
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (player_is_local_pc_obj(obj)) {
        gsound_play_sfx(sound_id, 1);
        return;
    }

    if (tig_net_is_active() && tig_net_is_host()) {
        int client_id = multiplayer_find_slot_from_obj(obj);
        if (client_id != -1) {
            PacketPlaySound pkt;

            pkt.type = 106;
            pkt.subtype = 0;
            pkt.sound_id = sound_id;
            tig_net_send_app(&pkt, sizeof(pkt), client_id);
        }
    }
}

// 0x4EED80
void mp_gsound_play_sfx_on_obj(int sound_id, int loops, int64_t obj)
{
    gsound_play_sfx_on_obj(sound_id, loops, obj);

    if (tig_net_is_active()) {
        PacketPlaySound pkt;

        pkt.type = 106;
        pkt.subtype = 1;
        pkt.sound_id = sound_id;
        pkt.loops = loops;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EEE00
void mp_gsound_play_scheme(int music_scheme_idx, int ambient_scheme_idx)
{
    gsound_play_scheme(music_scheme_idx, ambient_scheme_idx);

    if (tig_net_is_active()) {
        PacketPlaySound pkt;

        pkt.type = 106;
        pkt.subtype = 2;
        pkt.music_scheme_idx = music_scheme_idx;
        pkt.ambient_scheme_idx = ambient_scheme_idx;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EEE50
void mp_handle_gsound_play_scheme(PacketPlaySound* pkt)
{
    switch (pkt->subtype) {
    case 0:
        gsound_play_sfx(pkt->sound_id, 1);
        break;
    case 1:
        gsound_play_sfx_on_obj(pkt->sound_id, pkt->loops, objp_perm_lookup(pkt->oid));
        break;
    case 2:
        gsound_play_scheme(pkt->music_scheme_idx, pkt->ambient_scheme_idx);
        break;
    }
}

// 0x4EEEC0
void mp_container_close(int64_t obj)
{
    object_dec_current_aid(obj);

    if (tig_net_is_active()) {
        Packet129 pkt;

        pkt.type = 129;
        pkt.subtype = 11;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EEF20
void mp_container_open(int64_t obj)
{
    object_inc_current_aid(obj);

    if (tig_net_is_active()) {
        Packet129 pkt;

        pkt.type = 129;
        pkt.subtype = 10;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EEF80
void mp_portal_toggle(int64_t obj)
{
    portal_toggle(obj);

    if (tig_net_is_active()) {
        PacketPortalToggle pkt;

        pkt.type = 107;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EEFE0
void mp_handle_portal_toggle(PacketPortalToggle* pkt)
{
    portal_toggle(objp_perm_lookup(pkt->oid));
}

// 0x4EF010
void mp_sector_block_set(int64_t sec, bool blocked)
{
    PacketSectorBlockSet pkt;

    sector_block_set(sec, blocked);

    if (tig_net_is_active()) {
        pkt.type = 108;
        pkt.sec = sec;
        pkt.blocked = blocked;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EF060
void mp_handle_sector_block_set(PacketSectorBlockSet* pkt)
{
    sector_block_set(pkt->sec, pkt->blocked);
}

// 0x4EF080
void mp_spell_mastery_set(int64_t obj, int college)
{
    PacketSpellMasterySet pkt;

    spell_mastery_set(obj, college);

    if (tig_net_is_active()) {
        pkt.type = 109;
        pkt.oid = obj_get_id(obj);
        pkt.college = college;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EF0F0
void mp_handle_spell_mastery_set(PacketSpellMasterySet* pkt)
{
    spell_mastery_set(objp_perm_lookup(pkt->oid), pkt->college);
}

// 0x4EF120
void mp_townmap_set_known(int map, bool known)
{
    PacketTownmapSetKnown pkt;

    townmap_set_known(map, known);

    if (tig_net_is_active()) {
        pkt.type = 110;
        pkt.map = map;
        pkt.known = known;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EF170
void mp_handle_townmap_set_known(PacketTownmapSetKnown* pkt)
{
    townmap_set_known(pkt->map, pkt->known);
}

// 0x4EF190
void mp_art_touch(tig_art_id_t art_id)
{
    PacketArtTouch pkt;

    tig_art_touch(art_id);

    if (tig_net_is_active()) {
        pkt.type = 111;
        pkt.art_id = art_id;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EF1D0
void mp_handle_art_touch(PacketArtTouch* pkt)
{
    tig_art_touch(pkt->art_id);
}

// 0x4EF1E0
void mp_map_precache_sectors(int64_t loc, int64_t obj)
{
    if (player_get_local_pc_obj() == obj) {
        map_precache_sectors(loc);
        return;
    }

    if (tig_net_is_active()) {
        PacketMapPrecacheSectors pkt;

        pkt.type = 112;
        pkt.oid = obj_get_id(obj);
        pkt.loc = loc;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EF260
void mp_handle_map_precache_sectors(PacketMapPrecacheSectors* pkt)
{
    int64_t obj;

    obj = objp_perm_lookup(pkt->oid);
    if (obj == player_get_local_pc_obj()) {
        map_precache_sectors(pkt->loc);
    }
}

// 0x4EF320
void sub_4EF320()
{
    // TODO: Incomplete.
}

// 0x4EF3D0
void sub_4EF3D0()
{
    // TODO: Incomplete.
}

// 0x4EF540
void sub_4EF540()
{
    // TODO: Incomplete.
}

// 0x4EF5C0
void mp_tf_remove(int64_t obj)
{
    PacketTextRemove pkt;

    tf_remove(obj);

    if (tig_net_is_active()) {
        pkt.type = 116;
        pkt.subtype = 1;
        pkt.action = 0;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EF630
void mp_tb_remove(int64_t obj)
{
    PacketTextRemove pkt;

    tb_remove(obj);

    if (tig_net_is_active()) {
        pkt.type = 116;
        pkt.subtype = 0;
        pkt.action = 0;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EF690
void mp_handle_text_remove(PacketTextRemove* pkt)
{
    if (pkt->action == 0) {
        switch (pkt->subtype) {
        case 0:
            tb_remove(objp_perm_lookup(pkt->oid));
            break;
        case 1:
            tf_remove(objp_perm_lookup(pkt->oid));
            break;
        }
    }
}

// 0x4EF6F0
void mp_item_use(int64_t source_obj, int64_t item_obj, int64_t target_obj)
{
    PacketItemUse pkt;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        item_use_on_obj(source_obj, item_obj, target_obj);
        return;
    }

    pkt.type = 117;
    sub_4F0640(source_obj, &(pkt.source_oid));
    sub_4F0640(item_obj, &(pkt.item_oid));
    sub_4F0640(target_obj, &(pkt.target_oid));
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4EF790
void mp_handle_item_use(PacketItemUse* pkt)
{
    int64_t source_obj;
    int64_t item_obj;
    int64_t target_obj;

    if (pkt == NULL) {
        return;
    }

    if (tig_net_is_host()) {
        sub_4F0690(pkt->source_oid, &source_obj);
        sub_4F0690(pkt->item_oid, &item_obj);
        sub_4F0690(pkt->target_oid, &target_obj);
        item_use_on_obj(source_obj, item_obj, target_obj);
    }
}

// 0x4EF830
void sub_4EF830(int64_t a1, int64_t a2)
{
    Packet118 pkt;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        sub_4445A0(a1, a2);
        return;
    }

    pkt.type = 118;
    sub_4F0640(a1, &(pkt.field_8));
    sub_4F0640(a2, &(pkt.field_20));
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4EF8B0
void sub_4EF8B0(Packet118* pkt)
{
    int64_t v1;
    int64_t v2;

    if (tig_net_is_host()) {
        sub_4F0690(pkt->field_8, &v1);
        sub_4F0690(pkt->field_20, &v2);
        sub_4445A0(v1, v2);
    }
}

// 0x4EF920
bool mp_object_duplicate(int64_t obj, int64_t loc, int64_t* obj_ptr)
{
    bool ret = false;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        ret = object_duplicate(obj, loc, obj_ptr);
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        ObjectID* oids;
        int cnt;
        PacketObjectDuplicate* pkt;
        int size;

        sub_4063A0(*obj_ptr, &oids, &cnt);
        size = sizeof(*pkt) + sizeof(*oids) * cnt;
        pkt = (PacketObjectDuplicate*)MALLOC(size);
        memcpy(pkt + 1, oids, sizeof(*oids) * cnt);
        FREE(oids);

        pkt->type = 119;
        sub_4F0640(obj, &(pkt->oid));
        pkt->loc = loc;
        tig_net_send_app_all(pkt, size);
        FREE(pkt); // FIX: Memory leak.
    }

    return ret;
}

// 0x4EFA10
void mp_handle_object_duplicate(PacketObjectDuplicate* pkt)
{
    int64_t obj;
    int64_t copy_obj;

    sub_4F0690(pkt->oid, &obj);

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    object_duplicate_ex(obj, pkt->loc, (ObjectID*)(pkt + 1), &copy_obj);
}

// 0x4EFA70
void mp_stop_anim_id(AnimID anim_id)
{
    PacketStopAnimId pkt;

    pkt.type = 120;
    pkt.anim_id = anim_id;
    tig_net_send_app_all(&pkt, sizeof(pkt));
}

// 0x4EFAB0
void mp_handle_stop_anim_id(PacketStopAnimId* pkt)
{
    sub_44E2C0(&(pkt->anim_id), 6);

    if (tig_net_is_host()) {
        tig_net_send_app_all(pkt, sizeof(*pkt));
    }
}

// 0x4EFAE0
void sub_4EFAE0(int64_t obj, int a2)
{
    Packet121 pkt;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        sub_463B30(obj, a2);
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        if (!multiplayer_is_locked()) {
            pkt.type = 121;
            sub_4F0640(obj, &(pkt.oid));
            pkt.field_20 = a2;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }
}

// 0x4EFB50
void sub_4EFB50(Packet121* pkt)
{
    int64_t obj;

    multiplayer_lock();
    sub_4F0690(pkt->oid, &obj);
    sub_463B30(obj, pkt->field_20);
    multiplayer_unlock();
}

// 0x4EFBA0
void sub_4EFBA0(int64_t obj)
{
    Packet122 pkt;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        pkt.type = 122;
        sub_4F0640(obj, &(pkt.oid));
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EFBE0
void sub_4EFBE0()
{
    // TODO: Incomplete.
}

// 0x4EFC30
void sub_4EFC30(int64_t pc_obj, const char* name, const char* rule)
{
    int size;
    Packet123* pkt;

    // NOTE: Unclear what additional 4 bytes are for.
    size = sizeof(*pkt) + (int)strlen(name) + (int)strlen(rule) + 2 + 4;
    pkt = (Packet123*)MALLOC(size);
    pkt->type = 123;
    pkt->total_size = size;
    pkt->player = multiplayer_find_slot_from_obj(pc_obj);
    pkt->name_length = (int)strlen(name) + 1;
    pkt->rule_length = (int)strlen(rule) + 1;
    strncpy((char*)(pkt + 1), name, pkt->name_length);
    strncpy((char*)(pkt + 1) + pkt->name_length, rule, pkt->rule_length);
    tig_net_send_app_all(&pkt, size);
    FREE(pkt);
}

// 0x4EFCD0
void sub_4EFCD0(PacketScriptFunc* pkt)
{
    if (!tig_net_is_host()) {
        multiplayer_lock();
        switch (pkt->subtype) {
        case SCRIPT_FUNC_SET_STORY_STATE:
            script_story_state_set(pkt->story_state);
            break;
        case SCRIPT_FUNC_SET_GLOBAL_VAR:
            script_global_var_set(pkt->index, pkt->value);
            break;
        case SCRIPT_FUNC_SET_GLOBAL_FLAG:
            script_global_flag_set(pkt->index, pkt->value);
            break;
        case SCRIPT_FUNC_END_GAME:
            ui_end_game();
            break;
        }
        multiplayer_unlock();
    }
}

// 0x4EFDD0
void mp_obj_field_int32_set(int64_t obj, int fld, int value)
{
    Packet129 pkt;

    obj_field_int32_set(obj, fld, value);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = 0;
        pkt.oid = obj_get_id(obj);
        pkt.d.a.field_28 = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EFE50
void mp_obj_field_int64_set(int64_t obj, int fld, int64_t value)
{
    Packet129 pkt;

    obj_field_int64_set(obj, fld, value);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = 1;
        pkt.oid = obj_get_id(obj);
        pkt.d.a.field_28 = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EFEE0
void mp_object_flags_unset(int64_t obj, unsigned int flags)
{
    Packet129 pkt;

    object_flags_unset(obj, flags);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = 2;
        pkt.oid = obj_get_id(obj);
        pkt.d.b.field_28 = flags;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EFF50
void mp_object_flags_set(int64_t obj, unsigned int flags)
{
    Packet129 pkt;

    object_flags_set(obj, flags);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = 3;
        pkt.oid = obj_get_id(obj);
        pkt.d.c.field_28 = flags;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4EFFC0
void mp_obj_field_obj_set(int64_t obj, int fld, int64_t value)
{
    Packet129 pkt;

    obj_field_handle_set(obj, fld, value);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = 4;
        pkt.oid = obj_get_id(obj);
        pkt.fld = fld;
        if (value != OBJ_HANDLE_NULL) {
            pkt.d.d.oid = obj_get_id(value);
        } else {
            pkt.d.d.oid.type = OID_TYPE_NULL;
        }
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F0070
void sub_4F0070(int64_t obj, int fld, int index, int64_t value)
{
    Packet129 pkt;

    if (!tig_net_is_active()
        || tig_net_is_host()) {
        obj_arrayfield_obj_set(obj, fld, index, value);
    }

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 129;
        pkt.subtype = 5;
        pkt.oid = obj_get_id(obj);
        pkt.fld = fld;
        if (value != OBJ_HANDLE_NULL) {
            pkt.d.e.oid = obj_get_id(value);
        } else {
            pkt.d.e.oid.type = OID_TYPE_NULL;
        }
        pkt.d.e.field_28 = index;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F0150
void mp_obj_arrayfield_int32_set(int64_t obj, int fld, int index, int value)
{
    Packet129 pkt;

    obj_arrayfield_int32_set(obj, fld, index, value);

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 129;
        pkt.subtype = 8;
        sub_4F0640(obj, &(pkt.oid));
        pkt.fld = fld;
        pkt.d.f.field_28 = index;
        pkt.d.f.field_2C = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F01D0
void mp_obj_arrayfield_script_set(int64_t obj, int fld, int index, Script* value)
{
    Packet129 pkt;

    obj_arrayfield_script_set(obj, fld, index, value);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = P129_SUBTYPE_SCRIPT;
        pkt.oid = obj_get_id(obj);
        pkt.fld = fld;
        pkt.scr_val.idx = index;
        pkt.scr_val.scr = *value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F0270
void mp_obj_arrayfield_uint32_set(int64_t obj, int fld, int index, int value)
{
    Packet129 pkt;

    obj_arrayfield_uint32_set(obj, fld, index, value);

    if (tig_net_is_active()
        && tig_net_is_host()) {
        pkt.type = 129;
        pkt.subtype = P129_SUBTYPE_INT32_ARRAY;
        sub_4F0640(obj, &(pkt.oid));
        pkt.fld = fld;
        pkt.int32_array_val.idx = index;
        pkt.int32_array_val.value = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F02F0
void mp_object_set_current_aid(int64_t obj, tig_art_id_t art_id)
{
    Packet129 pkt;

    object_set_current_aid(obj, art_id);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = 9;
        pkt.oid = obj_get_id(obj);
        pkt.d.h.art_id = art_id;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F0360
void mp_object_overlay_set(int64_t obj, int fld, int index, tig_art_id_t aid)
{
    Packet129 pkt;

    object_overlay_set(obj, fld, index, aid);

    if (tig_net_is_active()) {
        pkt.type = 129;
        pkt.subtype = 12;
        sub_4F0640(obj, &(pkt.oid));
        pkt.d.i.field_28 = fld;
        pkt.d.i.field_2C = index;
        pkt.d.i.field_30 = aid;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F03E0
void mp_item_flags_set(int64_t obj, unsigned int flags_to_add)
{
    unsigned int flags;
    Packet129 pkt;

    flags = obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS);
    flags |= flags_to_add;
    obj_field_int32_set(obj, OBJ_F_ITEM_FLAGS, flags);

    if (tig_net_is_active()) {
        if (tig_net_is_host()) {
            mp_obj_field_int32_set(obj, OBJ_F_ITEM_FLAGS, obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS));
        } else {
            pkt.type = 129;
            pkt.subtype = 13;
            sub_4F0640(obj, &(pkt.oid));
            pkt.d.j.flags = flags_to_add;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }
}

// 0x4F0470
void mp_item_flags_unset(int64_t obj, unsigned int flags_to_remove)
{
    unsigned int flags;
    Packet129 pkt;

    flags = obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS);
    flags &= ~flags_to_remove;
    obj_field_int32_set(obj, OBJ_F_ITEM_FLAGS, flags);

    if (tig_net_is_active()) {
        if (tig_net_is_host()) {
            mp_obj_field_int32_set(obj, OBJ_F_ITEM_FLAGS, obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS));
        } else {
            pkt.type = 129;
            pkt.subtype = 14;
            sub_4F0640(obj, &(pkt.oid));
            pkt.d.j.flags = flags_to_remove;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }
}

// 0x4F0500
void sub_4F0500(int64_t obj, int fld)
{
    Packet130 pkt;

    sub_407D50(obj, fld);

    if (tig_net_is_active()) {
        pkt.type = 130;
        pkt.field_4 = 0;
        pkt.fld = fld;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F0570
void sub_4F0570(int64_t obj, int fld, int length)
{
    Packet130 pkt;

    obj_arrayfield_length_set(obj, fld, length);

    if (tig_net_is_active()) {
        pkt.type = 130;
        pkt.field_4 = 1;
        pkt.fld = fld;
        pkt.length = length;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4F05F0
void sub_4F05F0()
{
    // TODO: Incomplete.
}

// 0x4F0640
void sub_4F0640(int64_t obj, ObjectID* oid_ptr)
{
    if (obj != OBJ_HANDLE_NULL) {
        *oid_ptr = obj_get_id(obj);
    } else {
        oid_ptr->type = OID_TYPE_NULL;
    }
}

// 0x4F0690
void sub_4F0690(ObjectID oid, int64_t* obj_ptr)
{
    if (oid.type != OID_TYPE_NULL) {
        *obj_ptr = objp_perm_lookup(oid);
    } else {
        *obj_ptr = OBJ_HANDLE_NULL;
    }
}
