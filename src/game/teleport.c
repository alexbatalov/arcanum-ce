#include "game/teleport.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/gamelib.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/light.h"
#include "game/magictech.h"
#include "game/map.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/player.h"
#include "game/reaction.h"
#include "game/roof.h"
#include "game/ui.h"
#include "game/wallcheck.h"

/**
 * Internal flag set automatically when the teleporting object is the local PC.
 */
#define TELEPORT_IS_LOCAL_PC 0x80000000u

typedef struct TeleportObjectNode {
    int64_t loc;
    int64_t obj;
    struct TeleportObjectNode* next;
} TeleportObjectNode;

static bool teleport_process(TeleportData* teleport_data);
static bool schedule_teleport_obj_recursively(int64_t obj, int64_t loc);
static bool sub_4D39A0(TeleportData* teleport_data);
static void sub_4D3D60(int64_t obj);
static void sub_4D3E20(int64_t obj);
static void sub_4D3E80();
static TeleportObjectNode* teleport_obj_node_create();
static void teleport_obj_node_destroy(TeleportObjectNode* node);
static void teleport_obj_node_reserve();
static void teleport_clear();

// 0x601840
static TeleportObjectNode* teleport_obj_node_head;

// 0x601844
static bool teleport_pending;

// 0x601848
static IsoInvalidateRectFunc* teleport_iso_window_invalidate_rect;

// 0x60184C
static bool teleport_processing;

// 0x601850
static IsoRedrawFunc* teleport_iso_window_draw;

// 0x601858
static TeleportData current_teleport_data;

// 0x6018B8
static TeleportObjectNode* teleport_free_head;

// 0x6018BC
static bool teleport_is_teleporting_pc;

// 0x4D32D0
bool teleport_init(GameInitInfo* init_info)
{
    teleport_iso_window_invalidate_rect = init_info->invalidate_rect_func;
    teleport_iso_window_draw = init_info->draw_func;
    teleport_pending = false;

    return true;
}

// 0x4D3300
void teleport_reset()
{
    teleport_clear();
    teleport_pending = false;
}

// 0x4D3320
void teleport_exit()
{
    teleport_reset();
}

// 0x4D3330
void teleport_ping(tig_timestamp_t timestamp)
{
    (void)timestamp;

    if (teleport_pending) {
        teleport_processing = true;
        teleport_process(&current_teleport_data);
        teleport_processing = false;

        teleport_pending = false;

        if ((current_teleport_data.flags & TELEPORT_IS_LOCAL_PC) != 0
            && (current_teleport_data.flags & TELEPORT_RENDER_LOCK) != 0) {
            gamelib_renderlock_release();
        }
    }
}

// 0x4D3380
bool teleport_do(TeleportData* teleport_data)
{
    if (teleport_pending) {
        if ((current_teleport_data.flags & TELEPORT_IS_LOCAL_PC) != 0) {
            return false;
        }
    }

    current_teleport_data = *teleport_data;
    teleport_pending = true;

    if (player_is_local_pc_obj(teleport_data->obj)) {
        current_teleport_data.flags |= TELEPORT_IS_LOCAL_PC;

        if ((current_teleport_data.flags & TELEPORT_FADE_OUT) != 0) {
            gfade_run(&(current_teleport_data.fade_out));
        }

        if ((current_teleport_data.flags & TELEPORT_RENDER_LOCK) != 0) {
            gamelib_clear_screen();
            gamelib_renderlock_acquire();
        }
    }

    return true;
}

// 0x4D3410
bool teleport_is_teleporting()
{
    return teleport_processing;
}

// 0x4D3420
bool teleport_is_teleporting_obj(int64_t obj)
{
    TeleportObjectNode* node;

    node = teleport_obj_node_head;
    while (node != NULL) {
        if (node->obj == obj) {
            return true;
        }
        node = node->next;
    }

    return false;
}

// 0x4D3460
bool teleport_process(TeleportData* teleport_data)
{
    int map;

    if (!tig_net_is_active()) {
        if ((teleport_data->flags & TELEPORT_FADE_OUT) != 0) {
            gfade_run(&(teleport_data->fade_out));
        }
    }

    if ((teleport_data->flags & TELEPORT_SOUND) != 0) {
        gsound_play_sfx_on_obj(teleport_data->sound_id, 1, teleport_data->obj);
    }

    if (!tig_net_is_active()) {
        if ((teleport_data->flags & TELEPORT_MOVIE1) != 0) {
            gmovie_play(teleport_data->movie1, teleport_data->movie_flags1, 0);
        }

        if ((teleport_data->flags & TELEPORT_MOVIE2) != 0) {
            gmovie_play(teleport_data->movie2, teleport_data->movie_flags2, 0);
        }

        if ((teleport_data->flags & TELEPORT_TIME) != 0) {
            DateTime datetime;

            sub_45A950(&datetime, 1000 * teleport_data->time);
            timeevent_inc_datetime(&datetime);
        }
    }

    map = map_current_map();
    if (teleport_data->map == -1) {
        teleport_data->map = map;
    }

    if (!multiplayer_is_locked()) {
        if (!tig_net_is_host()) {
            return false;
        }

        if (teleport_data->map != map) {
            return false;
        }
    }

    if ((teleport_data->flags & TELEPORT_SKIP_FOLLOWERS) == 0) {
        ObjectList objects;
        ObjectNode* node;

        object_list_followers(teleport_data->obj, &objects);
        node = objects.head;
        while (node != NULL) {
            if (critter_is_unconscious(node->obj)) {
                ai_npc_wait(node->obj);
            }
            node = node->next;
        }
        object_list_destroy(&objects);
    }

    schedule_teleport_obj_recursively(teleport_data->obj, teleport_data->loc);

    if (teleport_is_teleporting_pc) {
        object_flush();
        wallcheck_flush();
    }

    if (teleport_data->map != map && player_is_pc_obj(teleport_data->obj)) {
        sub_459F50();
        sub_437980();
        timeevent_notify_pc_teleported(teleport_data->map);
    }

    if (!sub_4D39A0(teleport_data)) {
        return false;
    }

    if (teleport_is_teleporting_pc) {
        ObjectID oid;
        int64_t obj;

        oid.type = OID_TYPE_NULL;
        if (obj_field_int32_get(teleport_data->obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            oid = obj_get_id(teleport_data->obj);
        } else if (obj_field_int32_get(teleport_data->obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            oid = obj_get_id(critter_pc_leader_get(teleport_data->obj));
        }

        teleport_is_teleporting_pc = false;

        if (teleport_data->map != map) {
            if (!map_open_in_game(teleport_data->map, false, false)) {
                return false;
            }

            location_origin_set(teleport_data->loc);
            sub_40D860();
        }

        if (oid.type != OID_TYPE_NULL) {
            obj = objp_perm_lookup(oid);
            if (obj != OBJ_HANDLE_NULL) {
                mp_map_precache_sectors(teleport_data->loc, obj);
            }

            // CE: Using Teleport spell may cause fatigue exhaustion. All
            // effects have already been applied in `critter_fatigue_damage_set`
            // (particularly, demaintaing spells and notifying MT-Item system).
            // However, since animations are reset, we need to knock down the
            // unconscious PC.
            if (critter_is_unconscious(obj)) {
                anim_goal_knockdown(obj);
            }
        }
    }

    if (!tig_net_is_active()) {
        if ((teleport_data->flags & TELEPORT_FADE_IN) != 0) {
            teleport_iso_window_invalidate_rect(NULL);
            teleport_iso_window_draw();
            tig_window_display();
            gfade_run(&(teleport_data->fade_in));
        }
    }

    return true;
}

// 0x4D3760
bool schedule_teleport_obj_recursively(int64_t obj, int64_t loc)
{
    int obj_type;
    unsigned int flags;
    int inventory_num_fld;
    int inventory_list_fld;
    TeleportObjectNode* node;

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);

    if ((flags & OF_TEXT) != 0) {
        mp_tb_remove(obj);
    }

    if ((flags & OF_TEXT_FLOATER) != 0) {
        mp_tf_remove(obj);
    }

    if (obj_type_is_critter(obj_type)) {
        ObjectList objects;
        ObjectNode* obj_node;
        int64_t v1;

        object_list_followers(obj, &objects);
        obj_node = objects.head;
        while (obj_node != NULL) {
            if ((obj_field_int32_get(obj_node->obj, OBJ_F_NPC_FLAGS) & ONF_AI_WAIT_HERE) == 0) {
                if (tig_net_is_active()) {
                    v1 = sub_4C1110(obj_node->obj);
                    if (v1 != OBJ_HANDLE_NULL) {
                        sub_460A20(v1, 0);
                    }
                }

                if (!schedule_teleport_obj_recursively(obj_node->obj, loc)) {
                    object_list_destroy(&objects);
                    return false;
                }
            }
            obj_node = obj_node->next;
        }

        object_list_destroy(&objects);
    }

    if (obj_type_is_critter(obj_type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        // NOTE: Set both fields to -1 to bypass code path below.
        inventory_num_fld = -1;
        inventory_list_fld = -1;
    }

    if (inventory_num_fld != -1) {
        int cnt;
        int inv_size;
        int index;
        int64_t item_obj;
        int64_t item_loc;

        cnt = obj_field_int32_get(obj, inventory_num_fld);
        inv_size = obj_arrayfield_length_get(obj, inventory_list_fld);
        if (cnt != inv_size) {
            tig_debug_printf("Inventory array count does not equal associated num field on teleport.  Array: %d, Field: %d\n",
                inv_size,
                cnt);
            return false;
        }

        for (index = 0; index < cnt; index++) {
            item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, index);
            item_loc = obj_field_int64_get(item_obj, OBJ_F_LOCATION);
            if (!schedule_teleport_obj_recursively(item_obj, item_loc)) {
                return false;
            }
        }
    }

    node = teleport_obj_node_create();
    node->loc = loc;
    node->obj = obj;
    node->next = teleport_obj_node_head;
    teleport_obj_node_head = node;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        teleport_is_teleporting_pc = true;
    }

    return true;
}

// 0x4D39A0
bool sub_4D39A0(TeleportData* teleport_data)
{
    int curr_map;
    char* curr_map_name;
    char curr_map_path[TIG_MAX_PATH];
    char* dst_map_name;
    char dst_map_path[TIG_MAX_PATH];
    TeleportObjectNode* node;
    unsigned int flags;
    char path[TIG_MAX_PATH];
    TigFile* stream;

    curr_map = map_current_map();
    if (curr_map != teleport_data->map) {
        if (!map_get_name(curr_map, &curr_map_name)) {
            return false;
        }

        strcpy(curr_map_path, "Save\\Current");
        strcat(curr_map_path, "\\maps\\");
        strcat(curr_map_path, curr_map_name);

        if (!map_get_name(teleport_data->map, &dst_map_name)) {
            return false;
        }

        strcpy(dst_map_path, "Save\\Current");
        strcat(dst_map_path, "\\maps\\");
        strcat(dst_map_path, dst_map_name);
        tig_file_mkdir_ex(dst_map_path);

        node = teleport_obj_node_head;
        while (node != NULL) {
            sub_4D3E20(node->obj);
            node = node->next;
        }

        node = teleport_obj_node_head;
        while (node != NULL) {
            sub_4064B0(node->obj);
            node = node->next;
        }

        node = teleport_obj_node_head;
        while (node != NULL) {
            flags = obj_field_int32_get(node->obj, OBJ_F_FLAGS);
            flags |= OF_DYNAMIC;
            flags |= OF_TELEPORTED;
            obj_field_int32_set(node->obj, OBJ_F_FLAGS, flags);
            obj_field_int64_set(node->obj, OBJ_F_LOCATION, teleport_data->loc);
            obj_field_int32_set(node->obj, OBJ_F_OFFSET_X, 0);
            obj_field_int32_set(node->obj, OBJ_F_OFFSET_Y, 0);

            sprintf(path, "%s\\mobile.mdy", dst_map_path);
            stream = tig_file_fopen(path, "ab");
            if (stream == NULL) {
                return false;
            }

            if (!obj_write(stream, node->obj)) {
                // FIXME: Leaking file.
                return false;
            }

            tig_file_fclose(stream);
            flags &= ~OF_DYNAMIC;
            flags &= ~OF_TELEPORTED;
            flags |= OF_DESTROYED;
            flags |= OF_EXTINCT;
            obj_field_int32_set(node->obj, OBJ_F_FLAGS, flags);

            node = node->next;
        }

        node = teleport_obj_node_head;
        while (node != NULL) {
            sub_406520(node->obj);
            node = node->next;
        }

        sub_4D3E80();
        return true;
    }

    node = teleport_obj_node_head;
    while (node != NULL) {
        if ((obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_INVENTORY) == 0) {
            sub_4D3D60(node->obj);

            if (!tig_net_is_active()
                && player_is_local_pc_obj(node->obj)) {
                wallcheck_flush();
                roof_fill_off(obj_field_int64_get(node->obj, OBJ_F_LOCATION));
            }

            if (tig_net_is_active()
                && tig_net_is_host()) {
                sub_424070(node->obj, 5, false, false);
            }

            sub_4EDF20(node->obj, node->loc, 0, 0, 1);
        }
        node = node->next;
    }

    sub_4D3E80();

    return true;
}

// 0x4D3D60
void sub_4D3D60(int64_t obj)
{
    int type;

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    sub_424070(obj, 5, 0, 1);

    // NOTE: Conditions looks odd, check (note fallthrough from npc block).
    switch (type) {
    case OBJ_TYPE_NPC:
        sub_45F710(obj);
        mp_obj_field_obj_set(obj, OBJ_F_NPC_COMBAT_FOCUS, OBJ_HANDLE_NULL);
        mp_obj_field_obj_set(obj, OBJ_F_NPC_WHO_HIT_ME_LAST, OBJ_HANDLE_NULL);
        sub_4F0500(obj, OBJ_F_NPC_SHIT_LIST_IDX);
        mp_obj_field_obj_set(obj, OBJ_F_NPC_SUBSTITUTE_INVENTORY, OBJ_HANDLE_NULL);
    case OBJ_TYPE_PC:
        mp_obj_field_obj_set(obj, OBJ_F_CRITTER_FLEEING_FROM, OBJ_HANDLE_NULL);
        if (player_is_local_pc_obj(obj)) {
            sub_460B20();
        }
    }

    sub_4D9990(obj);
    sub_4D9A90(obj);
}

// 0x4D3E20
void sub_4D3E20(int64_t obj)
{
    sub_4D3D60(obj);
    combat_critter_deactivate_combat_mode(obj);
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        sub_45F710(obj);
    }
    ai_timeevent_clear(obj);
    sub_4601D0(obj);
    sub_43CF70(obj);
}

// 0x4D3E80
void sub_4D3E80()
{
    TeleportObjectNode* next;

    while (teleport_obj_node_head != NULL) {
        next = teleport_obj_node_head->next;
        teleport_obj_node_destroy(teleport_obj_node_head);
        teleport_obj_node_head = next;
    }
}

// 0x4D3EB0
TeleportObjectNode* teleport_obj_node_create()
{
    TeleportObjectNode* node;

    node = teleport_free_head;
    if (node == NULL) {
        teleport_obj_node_reserve();
        node = teleport_free_head;
    }

    teleport_free_head = node->next;
    node->next = NULL;

    return node;
}

// 0x4D3EE0
void teleport_obj_node_destroy(TeleportObjectNode* node)
{
    node->next = teleport_free_head;
    teleport_free_head = node;
}

// 0x4D3F00
void teleport_obj_node_reserve()
{
    int index;
    TeleportObjectNode* node;

    if (teleport_free_head == NULL) {
        for (index = 0; index < 32; index++) {
            node = (TeleportObjectNode*)MALLOC(sizeof(*node));
            node->next = teleport_free_head;
            teleport_free_head = node;
        }
    }
}

// 0x4D3F30
void teleport_clear()
{
    TeleportObjectNode* curr;
    TeleportObjectNode* next;

    curr = teleport_free_head;
    while (curr != NULL) {
        next = curr->next;
        FREE(curr);
        curr = next;
    }
    teleport_free_head = NULL;
}
