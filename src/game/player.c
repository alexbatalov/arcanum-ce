#include "game/player.h"

#include "game/item.h"
#include "game/level.h"
#include "game/location.h"
#include "game/map.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/proto.h"
#include "game/scroll.h"
#include "game/ui.h"

// 0x5D1138
static ObjectID player_pc_oid;

// 0x5D1150
static int64_t qword_5D1150;

// 0x5D1158
static int64_t player_pc_obj;

// 0x5D1160
static int64_t player_pc_prototype_obj;

// 0x739E5C
bool player_editor;

// 0x40D6C0
bool player_init(GameInitInfo* init_info)
{
    player_pc_oid.type = OID_TYPE_NULL;
    player_editor = init_info->editor;

    return true;
}

// 0x40D6E0
void player_reset(void)
{
    if (player_pc_obj != OBJ_HANDLE_NULL) {
        player_pc_obj = OBJ_HANDLE_NULL;
        player_pc_oid.type = OID_TYPE_NULL;
    }

    qword_5D1150 = OBJ_HANDLE_NULL;
}

// 0x40D720
void player_exit(void)
{
    if (player_pc_obj != OBJ_HANDLE_NULL) {
        player_pc_obj = OBJ_HANDLE_NULL;
    }

    qword_5D1150 = OBJ_HANDLE_NULL;
}

// 0x40D760
bool player_save(TigFile* stream)
{
    ObjectID temp_oid;

    if (stream == NULL) {
        return false;
    }

    if (player_pc_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    temp_oid = obj_get_id(player_pc_obj);
    if (tig_file_fwrite(&temp_oid, sizeof(temp_oid), 1, stream) != 1) {
        return false;
    }

    return true;
}

// 0x40D7C0
bool player_load(GameLoadInfo* load_info)
{
    ObjectID temp_oid;
    int64_t location;

    if (load_info->stream == NULL) {
        return false;
    }

    if (tig_file_fread(&temp_oid, sizeof(temp_oid), 1, load_info->stream) != 1) {
        return false;
    }

    player_pc_oid = temp_oid;
    player_pc_obj = objp_perm_lookup(temp_oid);
    if (player_pc_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    location = obj_field_int64_get(player_pc_obj, OBJ_F_LOCATION);
    scroll_set_center(location);
    location_origin_set(location);
    ui_spell_maintain_refresh();

    return true;
}

// 0x40D860
void sub_40D860(void)
{
    if (player_pc_oid.type != OID_TYPE_NULL) {
        player_pc_obj = objp_perm_lookup(player_pc_oid);
    }
}

// 0x40D8A0
void player_create(void)
{
    tig_art_id_t art_id;
    int map;
    int64_t x;
    int64_t y;
    int64_t loc;

    qword_5D1150 = sub_468570(OBJ_TYPE_PC);

    map = map_current_map();
    if (map == 0
        || !map_get_starting_location(map, &x, &y)
        || (loc = LOCATION_MAKE(x, y)) == 0) {
        loc = location_center_get();
        if (!player_editor) {
            tig_debug_printf("Player: ERROR: Map loaded that is NOT in map list!\n");
        }
    }

    tig_debug_printf("player_create: loc: X: %d, Y: %d\n",
        (int)LOCATION_GET_X(loc),
        (int)LOCATION_GET_Y(loc));
    if (!object_create(qword_5D1150, loc, &player_pc_obj)) {
        tig_debug_printf("player_create: Error: failed to create player!\n");
        exit(EXIT_FAILURE);
    }

    if (tig_art_critter_id_create(1, 0, 0, 0, 0, 4, 0, 0, 0, &art_id) != TIG_OK) {
        tig_debug_printf("player_create: Error: failed to create player ART!\n");
        exit(EXIT_FAILURE);
    }

    obj_field_int32_set(player_pc_obj, OBJ_F_AID, art_id);
    obj_field_int32_set(player_pc_obj, OBJ_F_CURRENT_AID, art_id);

    player_pc_oid = obj_get_id(player_pc_obj);
}

// 0x40D9F0
bool player_is_local_pc_obj(int64_t obj)
{
    return obj != OBJ_HANDLE_NULL && obj == player_pc_obj;
}

// 0x40DA20
bool player_is_pc_obj(int64_t obj)
{
    return obj != OBJ_HANDLE_NULL && obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC;
}

// 0x40DA50
int64_t player_get_local_pc_obj(void)
{
    int64_t obj = player_pc_obj;

    if (obj != OBJ_HANDLE_NULL) {
        if (obj_handle_is_valid(player_pc_obj) || player_pc_oid.type == OID_TYPE_NULL) {
            obj = player_pc_obj;
        } else {
            player_pc_obj = objp_perm_lookup(player_pc_oid);
            obj = player_pc_obj;
        }
    }

    return obj;
}

// 0x40DAB0
bool sub_40DAB0(void)
{
    if (player_pc_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    object_destroy(player_pc_obj);
    player_pc_obj = OBJ_HANDLE_NULL;
    player_pc_oid.type = OID_TYPE_NULL;

    return true;
}

// 0x40DAF0
bool sub_40DAF0(int64_t obj)
{
    player_pc_obj = obj;
    if (obj != OBJ_HANDLE_NULL) {
        player_pc_oid = obj_get_id(obj);
        ui_spell_maintain_refresh();
    } else {
        player_pc_oid.type = OID_TYPE_NULL;
    }

    return true;
}

// 0x40DB50
void player_create_info_init(PlayerCreateInfo* player_create_info)
{
    player_create_info->obj = OBJ_HANDLE_NULL;
    player_create_info->loc = 0;
    player_create_info->flags = 0;
    player_create_info->basic_prototype = -1;
}

// 0x40DB70
bool player_obj_create_player(PlayerCreateInfo* player_create_info)
{
    int64_t old_handle = OBJ_HANDLE_NULL;
    int64_t loc;
    char str[80];

    if (player_create_info->basic_prototype != -1) {
        player_pc_prototype_obj = objp_perm_lookup(obj_get_id(sub_4685A0(player_create_info->basic_prototype)));
    } else {
        player_pc_prototype_obj = sub_468570(OBJ_TYPE_PC);
    }

    if ((player_create_info->flags & PLAYER_CREATE_INFO_LOC) != 0) {
        loc = player_create_info->loc;
    } else {
        // TODO: Looks the same as above, check.
        loc = player_create_info->loc;
    }

    if ((player_create_info->flags & PLAYER_CREATE_INFO_OBJ) != 0) {
        old_handle = player_create_info->obj;
    }

    if ((player_create_info->flags & PLAYER_CREATE_INFO_NETWORK) == 0) {
        if (player_pc_obj != OBJ_HANDLE_NULL) {
            sub_40DAB0();
        }
    }

    if ((player_create_info->flags & PLAYER_CREATE_INFO_OBJ) != 0) {
        objid_id_to_str(str, player_create_info->oid);
        tig_debug_printf("player_obj_create_player: Player ID: %s\n", str);
        if (!object_create_ex(player_pc_prototype_obj, loc, player_create_info->oid, &(player_create_info->obj))) {
            tig_debug_printf("player_obj_create_player: Error: failed to create player!\n");
            exit(EXIT_FAILURE);
        }
    } else {
        if (!object_create(player_pc_prototype_obj, loc, &(player_create_info->obj))) {
            tig_debug_printf("player_obj_create_player: Error: failed to create player!\n");
            exit(EXIT_FAILURE);
        }

        player_create_info->oid = obj_get_id(player_create_info->obj);
        objid_id_to_str(str, player_create_info->oid);
        tig_debug_printf("player_obj_create_player: Player ID: %s\n", str);
    }

    if ((player_create_info->flags & PLAYER_CREATE_INFO_OBJ) != 0) {
        if (old_handle != player_create_info->obj) {
            // FIXME: Using println with newline character.
            tig_debug_println("ERROR: : object_create created new handle instead of using current one!\n");
        }
    }

    tig_debug_printf("player_obj_create_player: StartLoc: X: %d, Y: %d\n",
        (int)LOCATION_GET_X(loc),
        (int)LOCATION_GET_Y(loc));
    obj_field_int64_set(player_create_info->obj, OBJ_F_CRITTER_TELEPORT_DEST, loc);
    obj_field_int32_set(player_create_info->obj, OBJ_F_CRITTER_TELEPORT_MAP, map_current_map());

    if ((player_create_info->flags & PLAYER_CREATE_INFO_NETWORK) == 0) {
        player_pc_obj = player_create_info->obj;
        player_pc_oid = obj_get_id(player_pc_obj);
        tig_debug_printf("pcObj == %I64u\n", player_pc_obj);
    }

    level_set(player_create_info->obj, 1);
    sub_463E20(player_create_info->obj);
    obj_field_int32_set(player_create_info->obj, OBJ_F_CRITTER_INVENTORY_SOURCE, 0);
    ui_spell_maintain_refresh();

    return true;
}
