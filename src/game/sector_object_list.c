#include "game/sector_object_list.h"

#include "game/anim.h"
#include "game/gsound.h"
#include "game/light.h"
#include "game/light_scheme.h"
#include "game/location.h"
#include "game/obj.h"
#include "game/obj_find.h"
#include "game/object.h"
#include "game/sector.h"
#include "game/sector_script.h"
#include "game/tile.h"
#include "game/ui.h"
#include "game/wallcheck.h"

static void sub_4F20A0(SectorObjectList* list, ObjectNode* node);
static bool objlist_insert_internal(SectorObjectList* list, int64_t obj);
static bool objlist_remove_internal(SectorObjectList* list, int64_t obj, ObjectNode** node_ptr);

// 0x4F1150
bool sector_object_list_init(SectorObjectList* list)
{
    memset(list, 0, sizeof(*list));

    return true;
}

// 0x4F1170
bool sector_object_list_reset(SectorObjectList* list)
{
    int index;
    ObjectNode* node;

    for (index = 0; index < 4096; index++) {
        node = list->heads[index];
        while (node != NULL) {
            list->heads[index] = node->next;
            sub_4D9A90(node->obj);

            if (object_is_static(node->obj)) {
                object_delete(node->obj);
            } else {
                sub_443770(node->obj);
            }

            object_node_destroy(node);
            node = list->heads[index];
        }
    }

    return true;
}

// 0x4F1200
bool sector_object_list_exit(SectorObjectList* list)
{
    return sector_object_list_reset(list);
}

// 0x4F1210
bool objlist_insert(SectorObjectList* list, int64_t obj)
{
    if (!objlist_insert_internal(list, obj)) {
        return false;
    }

    if (object_is_static(obj)) {
        list->modified = 1;
    }

    return true;
}

// 0x4F1260
bool objlist_remove(SectorObjectList* list, int64_t obj)
{
    ObjectNode* node;

    if (!objlist_remove_internal(list, obj, &node)) {
        return false;
    }

    object_node_destroy(node);

    if (object_is_static(obj)) {
        list->modified = 1;
    }

    return true;
}

// 0x4F12C0
bool objlist_move(SectorObjectList* list, int64_t obj, int64_t loc, int offset_x, int offset_y)
{
    ObjectNode* node;

    if (!objlist_remove_internal(list, obj, &node)) {
        return false;
    }

    obj_field_int64_set(obj, OBJ_F_LOCATION, loc);
    obj_field_int32_set(obj, OBJ_F_OFFSET_X, offset_x);
    obj_field_int32_set(obj, OBJ_F_OFFSET_Y, offset_y);
    sub_4F20A0(list, node);

    if (object_is_static(obj)) {
        list->modified = 1;
    }

    return true;
}

// 0x4F1350
bool objlist_load(SectorObjectList* list, TigFile* stream, int64_t sector_id)
{
    int pos;
    int cnt;
    int x;
    int y;
    int idx;
    int tile;
    int64_t obj;
    int64_t loc;

    pos = tig_file_ftell(stream);
    if (pos == -1) {
        return false;
    }

    if (tig_file_fseek(stream, -(int)sizeof(int), 2) != 0) {
        return false;
    }

    if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fseek(stream, pos, 0) != 0) {
        return false;
    }

    if (sector_id != -1) {
        x = (int)SECTOR_X(sector_id) << 6;
        y = (int)SECTOR_Y(sector_id) << 6;
    } else {
        x = -1;
        y = -1;
    }

    list->next_temp_id = 0;

    for (idx = 0; idx < cnt; idx++) {
        if (!obj_read(stream, &obj)) {
            break;
        }

        if (sector_id != -1) {
            tile = tile_id_from_loc(obj_field_int64_get(obj, OBJ_F_LOCATION));
            loc = location_make(x + TILE_X(tile), y + TILE_Y(tile));
            obj_field_int64_set(obj, OBJ_F_LOCATION, loc);
            sub_406B80(obj);
        }

        sub_406520(obj);
        obj_field_int32_set(obj, OBJ_F_TEMP_ID, list->next_temp_id);

        if (!objlist_insert_internal(list, obj)) {
            break;
        }

        list->next_temp_id++;
    }

    if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    if (idx != cnt) {
        sector_object_list_reset(list);
        return false;
    }

    list->modified = 0;
    return true;
}

// 0x4F1590
bool objlist_load_with_difs(SectorObjectList* list, TigFile* sec_stream, TigFile* dif_stream, int64_t sector_id)
{
    int pos;
    int cnt;
    int rc;
    int idx;
    bool dif;
    int extent;
    int64_t x;
    int64_t y;
    int64_t obj;
    int64_t loc;
    int tile;

    pos = tig_file_ftell(sec_stream);
    if (pos == -1) {
        tig_debug_println("ERROR: Tell failed in objlist_load_with_difs.");
        return false;
    }

    if (tig_file_fseek(sec_stream, -(int)sizeof(pos), 2) != 0) {
        return false;
    }

    rc = (int)tig_file_fread(&cnt, sizeof(cnt), 1, sec_stream);
    if (rc != 1) {
        tig_debug_printf("ERROR: Can't read initial count in objlist_load_with_difs.  Result: %d  Error: %d  EOF: %d\n",
            rc,
            tig_file_ferror(sec_stream),
            tig_file_feof(sec_stream));
        return false;
    }

    if (tig_file_fseek(sec_stream, pos, 0) != 0) {
        tig_debug_println("ERROR: Seek failed in objlist_load_with_difs.");
        return false;
    }

    if (sector_id != -1) {
        x = SECTOR_X(sector_id);
        y = SECTOR_Y(sector_id);
    }

    dif = false;
    extent = 0;
    list->next_temp_id = 0;

    for (idx = 0; idx < cnt; idx++) {
        if (extent == 0) {
            rc = (int)tig_file_fread(&extent, sizeof(extent), 1, dif_stream);
            if (rc != 1) {
                tig_debug_printf("ERROR: Can't read dif_count in objlist_load_with_difs.\n       Result: %d  Error: %d  EOF: %d\n",
                    rc,
                    tig_file_ferror(sec_stream),
                    tig_file_feof(sec_stream));
                return false;
            }

            dif = (extent & 0x80000000) != 0;
            extent &= ~0x80000000;
        }

        if (!obj_read(sec_stream, &obj)) {
            tig_debug_println("ERROR: obj_read failed in objlist_load_with_difs.");
            return false;
        }

        if (sector_id != -1) {
            loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
            tile = tile_id_from_loc(loc);
            obj_field_int64_set(obj, OBJ_F_LOCATION, LOCATION_MAKE(x + (tile & 0x3F), y + ((tile >> 6) & 0x3F)));
            sub_406B80(obj);
        }

        if (dif) {
            if (!obj_dif_read(dif_stream, obj)) {
                tig_debug_println("ERROR: obj_dif_read failed in objlist_load_with_difs.");
                return false;
            }
        }

        extent--;
        obj_field_int32_set(obj, OBJ_F_TEMP_ID, list->next_temp_id++);

        if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_EXTINCT) == 0) {
            if (!objlist_insert_internal(list, obj)) {
                tig_debug_println("ERROR: objlist_insert_internal failed in objlist_load_with_difs.");
                return false;
            }

            sub_406520(obj);
        } else {
            obj_deallocate(obj);
        }
    }

    if (idx != cnt) {
        tig_debug_println("ERROR: Count doesn't match in objlist_load_with_difs.");
        return false;
    }

    list->next_temp_id = idx;

    rc = (int)tig_file_fread(&cnt, sizeof(cnt), 1, sec_stream);
    if (rc != 1) {
        tig_debug_printf("ERROR: Can't read final count in objlist_load_with_difs.\n       Result: %d  Error: %d  EOF: %d\n",
            rc,
            tig_file_ferror(sec_stream),
            tig_file_feof(sec_stream));
        return false;
    }

    list->modified = 1;
    return true;
}

// 0x4F1960
bool objlist_save(SectorObjectList* list, TigFile* stream)
{
    int cnt = 0;
    ObjectNode* node;
    int index;

    for (index = 0; index < 4096; index++) {
        node = list->heads[index];
        while (node != NULL) {
            if (object_is_static(node->obj)) {
                sub_4064B0(node->obj);
                if (!obj_write(stream, node->obj)) {
                    return false;
                }
                sub_406520(node->obj);
                sub_406B80(node->obj);
                cnt++;
            }
            node = node->next;
        }
    }

    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    list->modified = 0;

    return true;
}

// 0x4F1A20
bool objlist_is_modified(SectorObjectList* list)
{
    ObjectNode* node;
    int index;

    if (list->modified) {
        return true;
    }

    for (index = 0; index < 4096; index++) {
        node = list->heads[index];
        while (node != NULL) {
            if (object_is_static(node->obj) && obj_is_modified(node->obj)) {
                return true;
            }
            node = node->next;
        }
    }

    return false;
}

// 0x4F1A90
bool objlist_save_with_dif(SectorObjectList* list, TigFile* stream)
{
    ObjectNode* node;
    int idx;
    bool dif = false;
    int extent = 0;
    int pos = 0;
    int write_pos;

    for (idx = 0; idx < 4096; idx++) {
        node = list->heads[idx];
        while (node != NULL) {
            if (object_is_static(node->obj)) {
                sub_4064B0(node->obj);
                if (obj_is_modified(node->obj)) {
                    if (!dif) {
                        if (extent != 0) {
                            if (tig_file_fwrite(&extent, sizeof(extent), 1, stream) != 1) {
                                return false;
                            }
                        }

                        pos = tig_file_ftell(stream);
                        if (pos == -1) {
                            return false;
                        }

                        extent = 0;
                        if (tig_file_fwrite(&extent, sizeof(extent), 1, stream) != 1) {
                            return false;
                        }

                        dif = true;
                    }

                    if (!obj_dif_write(stream, node->obj)) {
                        return false;
                    }
                } else {
                    if (dif) {
                        write_pos = tig_file_ftell(stream);
                        if (write_pos == -1) {
                            return false;
                        }

                        if (tig_file_fseek(stream, pos, 0) != 0) {
                            return false;
                        }

                        extent |= 0x80000000;
                        if (tig_file_fwrite(&extent, sizeof(extent), 1, stream) != 1) {
                            return false;
                        }

                        if (tig_file_fseek(stream, write_pos, 0) != 0) {
                            return false;
                        }

                        dif = false;
                        extent = 0;
                    }
                }

                sub_406520(node->obj);
                extent++;
            }
            node = node->next;
        }
    }

    if (dif) {
        write_pos = tig_file_ftell(stream);
        if (write_pos == -1) {
            return false;
        }

        if (tig_file_fseek(stream, pos, 0) != 0) {
            return false;
        }

        extent |= 0x80000000;
        if (tig_file_fwrite(&extent, sizeof(extent), 1, stream) != 1) {
            return false;
        }

        if (tig_file_fseek(stream, write_pos, 0) != 0) {
            return false;
        }
    } else {
        if (tig_file_fwrite(&extent, sizeof(extent), 1, stream) != 1) {
            return false;
        }
    }

    return true;
}

// 0x4F1CD0
void objlist_fold(SectorObjectList* list, int64_t location, int a4)
{
    int64_t obj;
    FindNode* iter;

    if (obj_find_walk_first(location, &obj, &iter)) {
        do {
            if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_INVENTORY) == 0
                && !object_is_static(obj)) {
                objlist_insert_internal(list, obj);
                sub_441FC0(obj, a4);
                sub_438530(obj);
            }
        } while (obj_find_walk_next(&obj, &iter));
    }
}

// 0x4F1D80
void sub_4F1D80(void)
{
    // TODO: Incomplete.
}

// 0x4F1E70
bool objlist_tile_of_object(SectorObjectList* list, int64_t obj, int* tile_ptr)
{
    int times = 0;
    ObjectNode* node;
    int index;
    int found;

    for (index = 0; index < 4096; index++) {
        node = list->heads[index];
        while (node != NULL) {
            if (node->obj == obj) {
                if (times != 0) {
                    tig_debug_printf("objlist_tile_of_object found the same handle %d times", times + 1);
                }
                times++;
                found = index;
            }
        }
    }

    if (times == 0) {
        return false;
    }

    if (tile_ptr != NULL) {
        *tile_ptr = found;
    }

    return true;
}

// 0x4F1F00
void objlist_precache_art(SectorObjectList* list)
{
    ObjectNode* node;
    int index;
    tig_art_id_t art_id;

    for (index = 0; index < 4096; index++) {
        node = list->heads[index];
        while (node != NULL) {
            art_id = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
            sector_art_cache_register(art_id);
            node = node->next;
        }
    }
}

// 0x4F2020
bool objlist_insert_internal(SectorObjectList* list, int64_t obj)
{
    ObjectNode* node;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_INVENTORY) != 0) {
        tig_debug_printf("WARNING: objlist_insert_internal() called with inventory item!\n");
        return false;
    }

    node = object_node_create();
    node->obj = obj;
    node->next = NULL;
    sub_4F20A0(list, node);

    if (sub_43D940(obj)) {
        sub_43F710(obj);
    } else {
        sub_435CE0(obj);
    }

    return true;
}

// 0x4F20A0
void sub_4F20A0(SectorObjectList* list, ObjectNode* new_node)
{
    int64_t new_obj_loc;
    int new_obj_type;
    unsigned int new_obj_flags;
    int v1;
    int v2;
    int v3;
    int v4;
    int new_obj_tile;
    ObjectNode** node_ptr;
    unsigned int flags;
    int obj_type;

    new_obj_loc = obj_field_int64_get(new_node->obj, OBJ_F_LOCATION);
    new_obj_type = obj_field_int32_get(new_node->obj, OBJ_F_TYPE);
    new_obj_flags = obj_field_int32_get(new_node->obj, OBJ_F_FLAGS);
    sub_4F2230(new_node->obj, &v1, &v2);
    new_obj_tile = tile_id_from_loc(new_obj_loc);

    node_ptr = &(list->heads[new_obj_tile]);
    while (*node_ptr != NULL) {
        flags = obj_field_int32_get((*node_ptr)->obj, OBJ_F_FLAGS);
        if ((new_obj_flags & OF_FLAT) != 0) {
            if ((flags & OF_FLAT) == 0) {
                new_node->next = *node_ptr;
                *node_ptr = new_node;
                return;
            }

            if (new_obj_type == OBJ_TYPE_SCENERY
                && (obj_field_int32_get(new_node->obj, OBJ_F_SCENERY_FLAGS) & OSCF_UNDER_ALL) != 0) {
                new_node->next = *node_ptr;
                *node_ptr = new_node;
                return;
            }
        } else {
            obj_type = obj_field_int32_get((*node_ptr)->obj, OBJ_F_TYPE);
            if (new_obj_type == OBJ_TYPE_WALL && obj_type == OBJ_TYPE_PORTAL) {
                new_node->next = *node_ptr;
                *node_ptr = new_node;
                return;
            } else if (new_obj_type == OBJ_TYPE_PORTAL && obj_type == OBJ_TYPE_WALL) {
                new_node->next = (*node_ptr)->next;
                (*node_ptr)->next = new_node;
                return;
            }
        }

        if ((flags & OF_FLAT) == 0) {
            sub_4F2230((*node_ptr)->obj, &v3, &v4);
            if (v2 < v4) {
                new_node->next = *node_ptr;
                *node_ptr = new_node;
                return;
            }

            if (v2 == v4 && v1 < v3) {
                new_node->next = *node_ptr;
                *node_ptr = new_node;
                return;
            }
        }

        node_ptr = &((*node_ptr)->next);
    }

    *node_ptr = new_node;
}

// 0x4F2230
void sub_4F2230(int64_t obj, int* a2, int* a3)
{
    int obj_type;
    int offset_x;
    int offset_y;
    int rot;

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_WALL || obj_type == OBJ_TYPE_PORTAL) {
        offset_x = 0;
        rot = tig_art_id_rotation_get(obj_field_int32_get(obj, OBJ_F_CURRENT_AID));
        if (rot > 1 && rot < 6) {
            offset_y = 19;
        } else {
            offset_y = -20;
        }
    } else {
        offset_x = obj_field_int32_get(obj, OBJ_F_OFFSET_X);
        offset_y = obj_field_int32_get(obj, OBJ_F_OFFSET_Y);
    }

    sub_4B93F0(offset_x, offset_y, a2, a3);
}

// 0x4F22B0
bool objlist_remove_internal(SectorObjectList* list, int64_t obj, ObjectNode** node_ptr)
{
    int tile;
    ObjectNode* node;
    ObjectNode* prev;

    tile = tile_id_from_loc(obj_field_int64_get(obj, OBJ_F_LOCATION));
    node = list->heads[tile];
    prev = list->heads[tile];
    while (node != NULL) {
        if (node->obj == obj) {
            if (prev == node) {
                list->heads[tile] = node->next;
            } else {
                prev->next = node->next;
            }
            *node_ptr = node;
            node->next = NULL;
            return true;
        }
        prev = node;
        node = node->next;
    }

    return false;
}

// 0x4F2330
void objlist_notify_sector_changed(int64_t sec, int64_t pc_obj)
{
    Sector* sector;
    int light_scheme;
    Script scr;
    Script prev_scr;
    ObjectList followers;
    ObjectNode* node;
    ScriptInvocation invocation;

    if (sector_lock(sec, &sector)) {
        light_scheme = sector->light_scheme;
        if (light_scheme == LIGHT_SCHEME_MAP_DEFAULT) {
            light_scheme = light_scheme_get_map_default();
        }
        if (light_scheme != light_scheme_get()) {
            light_scheme_set(light_scheme, datetime_current_hour());
        }
        if (sector->sounds.music_scheme_idx != 0
            || sector->sounds.ambient_scheme_idx != 0) {
            gsound_play_scheme(sector->sounds.music_scheme_idx, sector->sounds.ambient_scheme_idx);
        }
        sector_unlock(sec);
    }

    if (sector_script_get(sec, &scr)) {
        prev_scr = scr;

        invocation.script = &scr;
        invocation.attachment_point = SAP_USE;
        invocation.triggerer_obj = pc_obj;
        invocation.attachee_obj = OBJ_HANDLE_NULL;
        invocation.extra_obj = OBJ_HANDLE_NULL;
        invocation.line = 0;
        script_execute(&invocation);

        if (prev_scr.hdr.flags != scr.hdr.flags
            || prev_scr.hdr.counters != scr.hdr.counters) {
            sector_script_set(sec, &scr);
        }
    }

    object_list_followers(pc_obj, &followers);
    node = followers.head;
    while (node != NULL) {
        object_script_execute(pc_obj, node->obj, OBJ_HANDLE_NULL, SAP_NEW_SECTOR, 0);
        node = node->next;
    }
    object_list_destroy(&followers);

    ui_notify_sector_changed(sec, pc_obj);
}
