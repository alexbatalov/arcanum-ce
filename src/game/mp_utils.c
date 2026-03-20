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

    obj = obj_pool_perm_lookup(pkt->oid);
    if (obj != OBJ_HANDLE_NULL) {
        ui_update_inven(obj);
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
        sub_43E770(obj, location, dx, dy);
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
        obj = obj_pool_perm_lookup(pkt->oid);
        sub_43E770(obj, pkt->location, pkt->dx, pkt->dy);

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
        ui_refresh_health_bar(obj_pool_perm_lookup(pkt->d.b.field_8));
        break;
    case 2:
        ui_toggle_primary_button(pkt->d.a.field_8, pkt->d.a.field_C);
        break;
    case 3:
        ui_toggle_primary_button(pkt->d.a.field_8, pkt->d.a.field_C);
        break;
    case 4:
        ui_schematic_feedback(pkt->d.c.field_8,
            obj_pool_perm_lookup(pkt->d.c.field_10),
            obj_pool_perm_lookup(pkt->d.c.field_28));
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
        gsound_play_sfx_on_obj(pkt->sound_id, pkt->loops, obj_pool_perm_lookup(pkt->oid));
        break;
    case 2:
        gsound_play_scheme(pkt->music_scheme_idx, pkt->ambient_scheme_idx);
        break;
    }
}

// 0x4EF320
void sub_4EF320(void)
{
    // TODO: Incomplete.
}

// 0x4EF3D0
void sub_4EF3D0(void)
{
    // TODO: Incomplete.
}

// 0x4EF540
void sub_4EF540(void)
{
    // TODO: Incomplete.
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
void sub_4EFBE0(void)
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

// 0x4F05F0
void sub_4F05F0(void)
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
        *obj_ptr = obj_pool_perm_lookup(oid);
    } else {
        *obj_ptr = OBJ_HANDLE_NULL;
    }
}
