#include "game/wall.h"

#include "game/obj.h"
#include "game/obj_private.h"
#include "game/object.h"

static void sub_4E1C00(GameDrawInfo* draw_info);
static void sub_4E1EB0(GameDrawInfo* draw_info);
static void sub_4E20A0(int64_t obj);
static void sub_4E25B0(int64_t obj);
static void sub_4E2C50(int64_t obj);

// 0x603420
static TigRect wall_iso_content_rect;

// 0x603430
static bool wall_editor;

// 0x603434
static int dword_603434;

// 0x603438
static IsoInvalidateRectFunc* wall_iso_window_invalidate_rect;

// 0x60343C
static tig_window_handle_t wall_iso_window_handle;

// 0x603440
static ViewOptions wall_view_options;

// 0x603448
static bool wall_enabled;

// 0x4DF390
bool wall_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    wall_iso_window_handle = init_info->iso_window_handle;
    wall_iso_window_invalidate_rect = init_info->invalidate_rect_func;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    wall_iso_content_rect.x = 0;
    wall_iso_content_rect.y = 0;
    wall_iso_content_rect.width = window_data.rect.width;
    wall_iso_content_rect.height = window_data.rect.height;

    wall_view_options.type = VIEW_TYPE_ISOMETRIC;
    wall_editor = init_info->editor;
    wall_enabled = true;
    dword_603434 = tig_color_make(255, 0, 100);

    return true;
}

// 0x4DF460
void wall_exit(void)
{
    wall_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    wall_iso_window_invalidate_rect = NULL;
}

// 0x4DF480
void wall_resize(GameResizeInfo* resize_info)
{
    wall_iso_window_handle = resize_info->window_handle;
    wall_iso_content_rect = resize_info->content_rect;
}

// 0x4DF4C0
void wall_update_view(ViewOptions* view_options)
{
    wall_view_options = *view_options;
}

// 0x4DF4E0
void wall_toggle(void)
{
    wall_enabled = !wall_enabled;
    wall_iso_window_invalidate_rect(NULL);
}

// 0x4DF500
void wall_draw(GameDrawInfo* draw_info)
{
    if (wall_view_options.type != VIEW_TYPE_TOP_DOWN) {
        return;
    }

    if (!wall_enabled) {
        return;
    }

    sub_4E1C00(draw_info);
    sub_4E1EB0(draw_info);
}

// 0x4E1490
void sub_4E1490(int64_t a1, int64_t a2)
{
    if (!wall_editor) {
        return;
    }

    // TODO: Incomplete.
}

// 0x4E18F0
void wall_delete(int64_t obj, bool a2)
{
    tig_art_id_t art_id;
    int p_piece;

    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    p_piece = tig_art_wall_id_p_piece_get(art_id);

    if (p_piece >= 9 && p_piece <= 20) {
        sub_4E20A0(obj);
    } else if (p_piece >= 21 && p_piece <= 33) {
        sub_4E25B0(obj);
    } else if (p_piece >= 34 && p_piece <= 45) {
        sub_4E2C50(obj);
    }

    if (a2) {
        if (obj_handle_is_valid(obj)) {
            sub_43CF70(obj);
            object_delete(obj);
        }
    }
}

// 0x4E1C00
void sub_4E1C00(GameDrawInfo* draw_info)
{
    LocRect* loc_rect;
    int64_t x;
    int64_t y;
    int64_t loc;
    ObjectList walls;
    ObjectNode* node;
    int64_t loc_x;
    int64_t loc_y;
    tig_art_id_t aid;
    int rotation;
    TigRect rect;

    loc_rect = draw_info->loc_rect;
    for (y = loc_rect->y1; y <= loc_rect->y2; y++) {
        for (x = loc_rect->x1; x <= loc_rect->x2; x++) {
            loc = location_make(x, y);
            object_list_location(loc, OBJ_TM_WALL, &walls);
            node = walls.head;
            while (node != NULL) {
                location_xy(loc, &loc_x, &loc_y);
                if (loc_x >= -wall_view_options.zoom
                    && loc_x <= wall_iso_content_rect.width
                    && loc_y >= -wall_view_options.zoom
                    && loc_y <= wall_iso_content_rect.height) {
                    aid = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
                    rotation = tig_art_id_rotation_get(aid);
                    if ((rotation & 1) != 0) {
                        rotation--;
                    }

                    switch (rotation) {
                    case 0:
                        rect.x = (int)loc_x + wall_view_options.zoom;
                        rect.y = (int)loc_y;
                        rect.width = 2;
                        rect.height = wall_view_options.zoom + 1;
                        break;
                    case 2:
                        rect.x = (int)loc_x;
                        rect.y = (int)loc_y + wall_view_options.zoom;
                        rect.width = wall_view_options.zoom + 1;
                        rect.height = 2;
                        break;
                    case 4:
                        rect.x = (int)loc_x;
                        rect.y = (int)loc_y;
                        rect.width = 2;
                        rect.height = wall_view_options.zoom + 1;
                        break;
                    default:
                        rect.x = (int)loc_x;
                        rect.y = (int)loc_y;
                        rect.width = wall_view_options.zoom + 1;
                        rect.height = 2;
                        break;
                    }

                    tig_window_fill(wall_iso_window_handle, &rect, tig_color_make(255, 0, 0));
                }
                node = node->next;
            }
            object_list_destroy(&walls);
        }
    }
}

// 0x4E1EB0
void sub_4E1EB0(GameDrawInfo* draw_info)
{
    LocRect loc_rect;
    TigRect rect;
    int size;
    int64_t x;
    int64_t y;
    int64_t loc_x;
    int64_t loc_y;
    int center;
    TigRectListNode* rect_node;
    TigRect tmp_rect;

    loc_rect = *draw_info->loc_rect;

    if (loc_rect.x1 % 4 >= 3) {
        loc_rect.x1 += 3;
    } else {
        loc_rect.x1 += 2 - loc_rect.x1 % 4;
    }

    if (loc_rect.y1 % 4 >= 3) {
        loc_rect.y1 += 3;
    } else {
        loc_rect.y1 += 2 - loc_rect.y1 % 4;
    }

    size = wall_view_options.zoom / 4;
    rect.width = size;
    rect.height = size;
    if (size < 4) {
        size = 4;
        rect.width = size;
        rect.height = size;
    }
    if (size > wall_view_options.zoom) {
        size = wall_view_options.zoom;
        rect.width = size;
        rect.height = size;
    }

    center = (wall_view_options.zoom - size) / 2;
    for (y = loc_rect.y1; y <= loc_rect.y2; y += 4) {
        for (x = loc_rect.x1; x <= loc_rect.x2; x += 4) {
            location_xy(LOCATION_MAKE(x, y), &loc_x, &loc_y);
            rect.x = center + (int)loc_x;
            rect.y = center + (int)loc_y;

            rect_node = *draw_info->rects;
            while (rect_node != NULL) {
                if (tig_rect_intersection(&rect, &(rect_node->rect), &tmp_rect) == TIG_OK) {
                    tig_window_fill(wall_iso_window_handle, &tmp_rect, dword_603434);
                }
                rect_node = rect_node->next;
            }
        }
    }
}

// 0x4E20A0
void sub_4E20A0(int64_t obj)
{
    // TODO: Incomplete.
}

// 0x4E25B0
void sub_4E25B0(int64_t obj)
{
    // TODO: Incomplete.
}

// 0x4E2C50
void sub_4E2C50(int64_t obj)
{
    tig_art_id_t art_id;
    int p_piece;
    int variation;
    int64_t loc;
    int cw_size;
    int ccw_size;
    int base_p_piece;
    int rot;
    int cw_rot;
    int ccw_rot;
    ObjectList walls;
    ObjectNode* node;
    int64_t adjacent_loc;

    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    p_piece = tig_art_wall_id_p_piece_get(art_id);
    variation = tig_art_wall_id_variation_get(art_id);
    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    base_p_piece = 0;

    switch (p_piece) {
    case 34:
        cw_size = 0;
        ccw_size = 2;
        variation = 0;
        break;
    case 35:
        cw_size = 1;
        variation = 0;
        ccw_size = 1;
        break;
    case 36:
        ccw_size = 0;
        cw_size = 2;
        variation = 0;
        break;
    case 37:
        cw_size = 0;
        ccw_size = 3;
        base_p_piece = 34;
        break;
    case 38:
        cw_size = 1;
        ccw_size = 2;
        variation = 0;
        break;
    case 39:
        cw_size = 2;
        ccw_size = 1;
        variation = 0;
        break;
    case 40:
        cw_size = 3;
        ccw_size = 0;
        base_p_piece = 36;
        break;
    case 41:
        cw_size = 0;
        ccw_size = 4;
        base_p_piece = 37;
        break;
    case 42:
        cw_size = 1;
        ccw_size = 3;
        variation = 0;
        break;
    case 43:
        cw_size = 2;
        variation = 0;
        ccw_size = 2;
        break;
    case 44:
        cw_size = 3;
        ccw_size = 1;
        variation = 0;
        break;
    case 45:
        cw_size = 4;
        ccw_size = 0;
        base_p_piece = 40;
        break;
    default:
        return;
    }

    rot = tig_art_id_rotation_get(art_id);
    if ((rot & 1) == 0) {
        rot++;
    }

    // TODO: Check rotations.
    if (rot == 1 || rot == 3) {
        cw_rot = (rot + 6) % 8;
        ccw_rot = (rot + 2) % 8;
        if (ccw_rot < 0) {
            ccw_rot = 8 - ccw_rot;
        }
    } else {
        cw_rot = (rot + 2) % 8;
        ccw_rot = (rot + 6) % 8;
        if (ccw_rot < 0) {
            ccw_rot = 8 - ccw_rot;
        }
    }

    art_id = tig_art_wall_id_variation_set(art_id, variation);
    art_id = tig_art_wall_id_p_piece_set(art_id, 0);
    object_set_current_aid(obj, art_id);

    p_piece = base_p_piece;
    adjacent_loc = loc;
    while (cw_size > 0) {
        location_in_dir(adjacent_loc, cw_rot, &adjacent_loc);
        object_list_location(adjacent_loc, OBJ_TM_WALL, &walls);
        node = walls.head;
        while (node != NULL) {
            if (obj_field_int32_get(node->obj, OBJ_F_TYPE) == OBJ_TYPE_WALL) {
                art_id = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
                art_id = tig_art_wall_id_variation_set(art_id, variation);
                art_id = tig_art_wall_id_p_piece_set(art_id, p_piece);
                object_set_current_aid(node->obj, art_id);
            }
            node = node->next;
        }
        object_list_destroy(&walls);

        if (p_piece != 0) {
            p_piece--;
        }

        cw_size--;
    }

    p_piece = base_p_piece;
    adjacent_loc = loc;
    while (ccw_size > 0) {
        location_in_dir(adjacent_loc, ccw_rot, &adjacent_loc);
        object_list_location(adjacent_loc, OBJ_TM_WALL, &walls);
        node = walls.head;
        while (node != NULL) {
            if (obj_field_int32_get(node->obj, OBJ_F_TYPE) == OBJ_TYPE_WALL) {
                art_id = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
                art_id = tig_art_wall_id_variation_set(art_id, variation);
                art_id = tig_art_wall_id_p_piece_set(art_id, p_piece);
                object_set_current_aid(node->obj, art_id);
            }
            node = node->next;
        }
        object_list_destroy(&walls);

        if (p_piece != 0) {
            p_piece++;
        }

        ccw_size--;
    }
}
