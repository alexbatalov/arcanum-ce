#include "game/object.h"

#include <inttypes.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/critter.h"
#include "game/description.h"
#include "game/descriptions.h"
#include "game/effect.h"
#include "game/gamelib.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/light.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/obj_file.h"
#include "game/obj_find.h"
#include "game/obj_private.h"
#include "game/party.h"
#include "game/player.h"
#include "game/portal.h"
#include "game/proto.h"
#include "game/reaction.h"
#include "game/resistance.h"
#include "game/roof.h"
#include "game/scroll.h"
#include "game/sector.h"
#include "game/sfx.h"
#include "game/stat.h"
#include "game/tb.h"
#include "game/tf.h"
#include "game/tile.h"
#include "game/tile_script.h"
#include "game/timeevent.h"
#include "game/townmap.h"
#include "game/trap.h"
#include "game/ui.h"
#include "game/wall.h"
#include "game/wallcheck.h"

typedef struct ObjectBlitInfo {
    /* 0000 */ TigArtBlitInfo blit_info;
    /* 002C */ int order;
    /* 0030 */ int rect_index;
} ObjectBlitInfo;

typedef struct ObjectBlitRectInfo {
    /* 0000 */ TigRect src_rect;
    /* 0010 */ TigRect dst_rect;
} ObjectBlitRectInfo;

typedef struct ObjectRenderColors {
    int colors[160];
} ObjectRenderColors;

typedef struct IndexedObjectRenderColors {
    /* 0000 */ void* ptr;
    /* 0004 */ ObjectRenderColors colors;
} IndexedObjectRenderColors;

typedef struct ObjectRenderColorsNode {
    IndexedObjectRenderColors* data;
    struct ObjectRenderColorsNode* next;
} ObjectRenderColorsNode;

typedef struct ObjectDropInfo {
    /* 0000 */ int64_t obj;
    /* 0008 */ int64_t loc;
} ObjectDropInfo;

typedef struct ObjectPickupInfo {
    /* 0000 */ int64_t item_obj;
    /* 0008 */ int64_t parent_obj;
} ObjectPickupInfo;

static void sub_43C5C0(GameDrawInfo* draw_info);
static void sub_43CEA0(int64_t obj, unsigned int flags, const char* path);
static int sub_43D690(int64_t obj);
static int sub_43D630(int64_t obj);
static int sub_43FE00(int64_t a1, int64_t a2, int a3, int a4, unsigned int flags, int64_t* a6, int* a7, int* a8);
static void object_list_vicinity_loc(int64_t loc, unsigned int flags, ObjectList* objects);
static bool object_drop_success(void* userinfo);
static bool object_drop_failure(void* userinfo);
static bool object_pickup_success(void* userinfo);
static bool object_pickup_failure(void* userinfo);
static bool object_create_func(int64_t proto_obj, int64_t loc, int64_t* obj_ptr, ObjectID oid);
static bool object_duplicate_func(int64_t proto_obj, int64_t loc, ObjectID* oids, int64_t* obj_ptr);
static bool sub_442260(int64_t obj, int64_t loc);
static void sub_4423E0(int64_t obj, int offset_x, int offset_y);
static void object_render_palette_clear(int64_t obj);
static void sub_442D90(int64_t obj, ObjectRenderColors* colors);
static TigPalette object_render_palette_get(int64_t obj);
static void object_render_colors_set(int64_t obj, ObjectRenderColors* colors);
static void object_render_colors_clear(int64_t obj);
static ObjectRenderColors* render_color_array_alloc();
static void render_color_array_free(ObjectRenderColors* colors);
static void render_color_array_reserve();
static void render_color_array_clear();
static void object_toggle_flat(int64_t obj, bool a2);
static void object_setup_blit(int64_t obj, TigArtBlitInfo* blit_info);
static void object_enqueue_blit(TigArtBlitInfo* blit_info, int order);
static void object_flush_pending_blits();
static int object_compare_blits(const void* va, const void* vb);
static void sub_443620(unsigned int flags, int scale, int x, int y, tig_art_id_t art_id, TigRect* rect);
static void sub_4437C0(int64_t obj);
static bool sub_443880(TigRect* rect, tig_art_id_t art_id);
static bool load_reaction_colors();
static void sub_444270(int64_t obj, int a2);
static void object_lighting_changed();

// 0x5A548C
static int dword_5A548C[8] = {
    50,
    63,
    75,
    87,
    100,
    130,
    160,
    200,
};

// 0x5A54AC
static unsigned int object_blit_flags = TIG_ART_BLT_BLEND_ALPHA_CONST;

// 0x5E2E60
static int64_t qword_5E2E60;

// 0x5E2E68
int dword_5E2E68;

// 0x5E2E6C
int dword_5E2E6C;

// 0x5E2E70
static TigRectListNode* object_dirty_rects_head;

// 0x5E2E74
static ObjectBlitInfo* object_pending_blits;

// 0x5E2E78
static tig_color_t object_reaction_colors[REACTION_COUNT];

// 0x5E2E94
bool dword_5E2E94;

// 0x5E2E98
static TigRect object_iso_content_rect;

// 0x5E2EA8
tig_art_id_t object_hover_underlay_art_id;

// 0x5E2EAC
tig_color_t object_hover_color;

// 0x5E2EB0
static int object_blit_queue_size;

// 0x5E2EB4
static IsoInvalidateRectFunc* object_iso_invalidate_rect;

// 0x5E2EB8
static ViewOptions object_view_options;

// 0x5E2EC0
static int object_hover_overlay_fps;

// 0x5E2EC4
static int object_hover_underlay_fps;

// 0x5E2EC8
static unsigned int dword_5E2EC8;

// 0x5E2ECC
tig_art_id_t object_hover_overlay_art_id;

// 0x5E2ED0
static bool object_dirty;

// 0x5E2ED4
static bool object_type_visibility[18];

// 0x5E2F1C
static int object_bpp;

// 0x5E2F20
static int object_blit_queue_capacity;

// 0x5E2F24
static tig_window_handle_t object_iso_window_handle;

// 0x5E2F28
static bool dword_5E2F28;

// 0x5E2F2C
static bool object_lighting;

// 0x5E2F30
static TigRect object_iso_content_rect_ex;

// 0x5E2F44
static ObjectRenderColorsNode* object_render_color_array_node_head;

// 0x5E2F48
static bool dword_5E2F48;

// 0x5E2F50
static int64_t qword_5E2F50;

// 0x5E2F58
static bool object_editor;

// 0x5E2F60
Ryan stru_5E2F60;

// 0x5E2F88
static unsigned int dword_5E2F88;

// 0x5E2F8C
static ObjectBlitRectInfo* object_pending_rects;

// 0x5E2F90
int64_t object_hover_obj;

// 0x5E2F98
static int dword_5E2F98;

// 0x43A330
bool object_init(GameInitInfo* init_info)
{
    TigWindowData window_data;
    TigArtAnimData art_anim_data;
    int idx;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    object_iso_window_handle = init_info->iso_window_handle;
    object_iso_invalidate_rect = init_info->invalidate_rect_func;
    object_view_options.type = VIEW_TYPE_ISOMETRIC;
    object_editor = init_info->editor;

    object_iso_content_rect.x = 0;
    object_iso_content_rect.y = 0;
    object_iso_content_rect.width = window_data.rect.width;
    object_iso_content_rect.height = window_data.rect.height;

    object_iso_content_rect_ex.x = -256;
    object_iso_content_rect_ex.y = -256;
    object_iso_content_rect_ex.width = window_data.rect.width + 512;
    object_iso_content_rect_ex.height = window_data.rect.height + 512;

    qword_5E2F50 = window_data.rect.width / 40 / 2 + 2;
    qword_5E2E60 = window_data.rect.height / 20 / 2 + 2;

    for (idx = 0; idx < 18; idx++) {
        object_type_visibility[idx] = true;
    }

    if (!load_reaction_colors()) {
        return false;
    }

    tig_art_interface_id_create(467, dword_5E2E6C, 1, 0, &object_hover_underlay_art_id);
    tig_art_interface_id_create(468, dword_5E2E68, 1, 0, &object_hover_overlay_art_id);

    tig_art_anim_data(object_hover_underlay_art_id, &art_anim_data);
    object_hover_underlay_fps = 1000 / art_anim_data.fps;

    tig_art_anim_data(object_hover_overlay_art_id, &art_anim_data);
    object_hover_overlay_fps = 1000 / art_anim_data.fps;

    if (object_editor) {
        dword_5E2F88 = 0;
        dword_5E2EC8 = 0;
        dword_5E2F28 = 0;
    } else {
        dword_5E2F88 = OF_DESTROYED | OF_OFF;
        dword_5E2EC8 = OF_DONTDRAW;
    }

    dword_5E2F48 = tig_video_3d_check_initialized() == TIG_OK;
    if (!dword_5E2F48) {
        object_blit_flags = TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D;
    }

    tig_video_get_bpp(&object_bpp);

    settings_register(&settings, OBJECT_LIGHTING_KEY, "1", object_lighting_changed);
    object_lighting_changed();

    return true;
}

// 0x43A570
void object_resize(GameResizeInfo* resize_info)
{
    object_iso_window_handle = resize_info->window_handle;
    object_iso_content_rect = resize_info->content_rect;
    object_iso_content_rect_ex.x = object_iso_content_rect.x - 256;
    object_iso_content_rect_ex.y = object_iso_content_rect.y - 256;
    object_iso_content_rect_ex.width = object_iso_content_rect.width + 512;
    object_iso_content_rect_ex.height = object_iso_content_rect.height + 512;
    qword_5E2F50 = object_iso_content_rect.width / 40 / 2 + 2;
    qword_5E2E60 = object_iso_content_rect.height / 20 / 2 + 2;
}

// 0x43A650
void object_reset()
{
    TigRectListNode* next;
    int index;

    while (object_dirty_rects_head != NULL) {
        next = object_dirty_rects_head->next;
        tig_rect_node_destroy(object_dirty_rects_head);
        object_dirty_rects_head = next;
    }

    for (index = 0; index < 18; index++) {
        object_type_visibility[index] = true;
    }
}

// 0x43A690
void object_exit()
{
    if (object_pending_blits != NULL) {
        FREE(object_pending_blits);
        FREE(object_pending_rects);
    }

    object_reset();
    render_color_array_clear();

    object_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    object_iso_invalidate_rect = NULL;
}

// 0x43A6D0
void object_ping(tig_timestamp_t timestamp)
{
    // 0x5E2F40
    static tig_timestamp_t dword_5E2F40;

    // 0x5E2E58
    static tig_timestamp_t dword_5E2E58;

    TigRect rect;
    TigRect bounds;
    TigRectListNode* next;
    LocRect loc_rect;
    SomeSectorStuff v1;
    int col;
    int row;
    SomeSectorStuffEntry* v2;
    int indexes[3];
    int widths[3];
    bool locks[3];
    Sector* sectors[3];
    int v4;
    int v3;
    ObjectNode* obj_node;
    unsigned int render_flags;
    TigRect obj_rect;

    if (tig_timer_between(dword_5E2F40, timestamp) >= object_hover_underlay_fps) {
        object_hover_underlay_art_id = tig_art_id_frame_inc(object_hover_underlay_art_id);
        dword_5E2F40 = timestamp;
        if (dword_5E2E94) {
            sub_443880(&rect, object_hover_underlay_art_id);
            object_iso_invalidate_rect(&rect);
        }
    }

    if (tig_timer_between(dword_5E2E58, timestamp) >= object_hover_overlay_fps) {
        object_hover_overlay_art_id = tig_art_id_frame_inc(object_hover_overlay_art_id);
        dword_5E2E58 = timestamp;
        if (dword_5E2E94) {
            sub_443880(&rect, object_hover_overlay_art_id);
            object_iso_invalidate_rect(&rect);
        }
    }

    if (object_view_options.type != VIEW_TYPE_ISOMETRIC) {
        return;
    }

    if (!object_dirty) {
        return;
    }

    bounds.x = object_iso_content_rect.x - object_iso_content_rect.width;
    bounds.y = object_iso_content_rect.y - object_iso_content_rect.height;
    bounds.width = object_iso_content_rect.width * 2;
    bounds.height = object_iso_content_rect.height * 2;

    while (object_dirty_rects_head != NULL) {
        next = object_dirty_rects_head->next;
        if (tig_rect_intersection(&(object_dirty_rects_head->rect), &bounds, &rect) == TIG_OK
            && sub_4B9130(&rect, &loc_rect)
            && sub_4D0090(&loc_rect, &v1)) {
            for (col = 0; col < v1.height; col++) {
                v2 = &(v1.field_8[col]);

                for (row = 0; row < v2->width; row++) {
                    indexes[row] = v2->field_38[row];
                    widths[row] = 64 - v2->field_44[row];
                    if (sub_4D04E0(v2->field_20[row])) {
                        locks[row] = sector_lock(v2->field_20[row], &(sectors[row]));
                    } else {
                        locks[row] = false;
                    }
                }

                for (v3 = 0; v3 < v2->field_50; v3++) {
                    for (row = 0; row < v2->width; row++) {
                        if (locks[row]) {
                            for (v4 = 0; v4 < v2->field_44[row]; v4++) {
                                obj_node = sectors[row]->objects.heads[indexes[row]];
                                while (obj_node != NULL) {
                                    render_flags = obj_field_int32_get(obj_node->obj, OBJ_F_RENDER_FLAGS);
                                    render_flags &= ~(ORF_02000000 | ORF_04000000);
                                    obj_field_int32_set(obj_node->obj, OBJ_F_RENDER_FLAGS, render_flags);
                                    object_get_rect(obj_node->obj, RECT_FLAG_FULL, &obj_rect);
                                    object_iso_invalidate_rect(&obj_rect);
                                    obj_node = obj_node->next;
                                }
                                indexes[row]++;
                            }
                            indexes[row] += widths[row];
                        }
                    }
                }

                for (row = 0; row < v2->width; row++) {
                    if (locks[row]) {
                        sector_unlock(v2->field_20[row]);
                    }
                }
            }
        }
        tig_rect_node_destroy(object_dirty_rects_head);
        object_dirty_rects_head = next;
    }

    object_dirty = false;
}

// 0x43AA40
void object_update_view(ViewOptions* view_options)
{
    object_view_options = *view_options;
}

// 0x43AA60
void object_map_close()
{
    object_reset();
}

// 0x43AA70
bool object_type_is_enabled(int obj_type)
{
    return object_type_visibility[obj_type];
}

// 0x43AA80
void object_type_toggle(int obj_type)
{
    object_type_visibility[obj_type] = !object_type_visibility[obj_type];
}

// 0x43AAA0
bool sub_43AAA0()
{
    return dword_5E2F28;
}

// 0x43AAB0
void sub_43AAB0()
{
    dword_5E2F28 = !dword_5E2F28;
    if (dword_5E2F28) {
        dword_5E2F88 = OF_DESTROYED | OF_OFF;
        dword_5E2EC8 = OF_DONTDRAW;
    } else {
        dword_5E2F88 = 0;
        dword_5E2EC8 = 0;
    }
}

// NOTE: Convenience uninline used in `object_draw`.
static inline bool object_render_check_rotation(int64_t obj)
{
    tig_art_id_t art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    int rot = tig_art_id_rotation_get(art_id);
    return rot != 0
        && rot != 1
        && rot != 6
        && rot != 7;
}

// 0x43B390
void object_draw(GameDrawInfo* draw_info)
{
    SomeSectorStuff* v1;
    SomeSectorStuffEntry* v2;
    bool is_detecting_invisible;
    int col;
    int row;
    int64_t locations[3];
    int indexes[3];
    int widths[3];
    bool locks[3];
    Sector* sectors[3];
    int v5[3];
    int v3;
    int v4;
    ObjectNode* obj_node;
    int obj_type;
    int obj_flags;
    int64_t loc;
    int64_t loc_x;
    int64_t loc_y;
    int scale;
    int idx;
    tig_art_id_t art_id;
    TigRect eye_candy_rect;
    TigRectListNode* rect_node;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect tmp_rect;
    int underlay_order = 0;
    int flat_order = 200000000;
    int shadow_order = 400000000;
    int non_flat_order = 600000000;
    int overlay_order = 700000000;
    int order;
    bool art_blit_info_initialized;
    bool v71;

    if (object_view_options.type != VIEW_TYPE_ISOMETRIC) {
        return;
    }

    v1 = draw_info->field_8;
    is_detecting_invisible = magictech_check_env_sf(OSF_DETECTING_INVISIBLE);

    for (col = 0; col < v1->height; col++) {
        v2 = &(v1->field_8[col]);

        for (row = 0; row < v2->width; row++) {
            locations[row] = v2->field_8[row];
            indexes[row] = v2->field_38[row];
            widths[row] = 64 - v2->field_44[row];
            v5[row] = -v2->field_44[row]; // TODO: Unclear.
            locks[row] = sector_lock(v2->field_20[row], &(sectors[row]));
        }

        for (v3 = 0; v3 < v2->field_50; v3++) {
            for (row = 0; row < v2->width; row++) {
                if (locks[row]) {
                    for (v4 = 0; v4 < v2->field_44[row]; v4++) {
                        obj_node = sectors[row]->objects.heads[indexes[row]];
                        if (obj_node != NULL) {
                            loc = locations[row];

                            if (!sub_43A030(loc, true)) {
                                while (obj_node != NULL) {
                                    obj_type = obj_field_int32_get(obj_node->obj, OBJ_F_TYPE);
                                    if (object_type_visibility[obj_type]) {
                                        obj_flags = obj_field_int32_get(obj_node->obj, OBJ_F_FLAGS);
                                        if ((dword_5E2F88 & obj_flags) == 0) {
                                            if (obj_type != OBJ_TYPE_WALL
                                                || (obj_field_int32_get(obj_node->obj, OBJ_F_WALL_FLAGS) & (OWAF_TRANS_LEFT | OWAF_TRANS_RIGHT)) == 0
                                                || object_render_check_rotation(obj_node->obj)
                                                || !roof_is_faded(loc)) {
                                                location_xy(loc, &loc_x, &loc_y);
                                                loc_x += obj_field_int32_get(obj_node->obj, OBJ_F_OFFSET_X);
                                                loc_y += obj_field_int32_get(obj_node->obj, OBJ_F_OFFSET_Y);
                                                scale = obj_field_int32_get(obj_node->obj, OBJ_F_BLIT_SCALE);

                                                loc_x += 40;
                                                loc_y += 20;

                                                // 0x43B65B
                                                if ((obj_flags & OF_HAS_UNDERLAYS) != 0) {
                                                    for (idx = 0; idx < 4; idx++) {
                                                        art_id = obj_arrayfield_uint32_get(obj_node->obj, OBJ_F_UNDERLAY, idx);
                                                        if (art_id != TIG_ART_ID_INVALID) {
                                                            // FIX: Ignore shrunk objects so they match 100% scale. This makes the reaction
                                                            // underlay perfectly match the hover underlay size (the hover underlay is not
                                                            // scaled and is unaffected by shrinking).
                                                            sub_443620(obj_flags & ~OF_SHRUNK, 100, (int)loc_x, (int)loc_y, art_id, &eye_candy_rect);

                                                            rect_node = *draw_info->rects;
                                                            while (rect_node != NULL) {
                                                                if (tig_rect_intersection(&eye_candy_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                                                    src_rect.x = dst_rect.x - eye_candy_rect.x;
                                                                    src_rect.y = dst_rect.y - eye_candy_rect.y;
                                                                    src_rect.width = dst_rect.width;
                                                                    src_rect.height = dst_rect.height;

                                                                    art_blit_info.flags = 0;
                                                                    if (tig_art_eye_candy_id_translucency_get(art_id) != 0) {
                                                                        art_blit_info.flags |= TIG_ART_BLT_BLEND_ADD;
                                                                    }

                                                                    if (obj_type == OBJ_TYPE_NPC
                                                                        && tig_art_num_get(art_id) == 433) {
                                                                        unsigned int reaction_flags = obj_field_int32_get(obj_node->obj, OBJ_F_CRITTER_FLAGS2) & (OCF2_REACTION_0 | OCF2_REACTION_1 | OCF2_REACTION_2 | OCF2_REACTION_3 | OCF2_REACTION_4 | OCF2_REACTION_5 | OCF2_REACTION_6);
                                                                        if (reaction_flags != 0) {
                                                                            reaction_flags >>= 14;

                                                                            int reaction = 0;
                                                                            while (reaction_flags != 0) {
                                                                                reaction_flags >>= 1;
                                                                                reaction++;
                                                                            }

                                                                            reaction--;

                                                                            if (reaction < 0) {
                                                                                reaction = 0;
                                                                            } else if (reaction > REACTION_COUNT) {
                                                                                reaction = REACTION_COUNT;
                                                                            }

                                                                            art_blit_info.flags |= TIG_ART_BLT_BLEND_COLOR_CONST;
                                                                            art_blit_info.color = object_reaction_colors[reaction];
                                                                        }
                                                                    }

                                                                    art_blit_info.art_id = art_id;
                                                                    art_blit_info.src_rect = &src_rect;
                                                                    art_blit_info.dst_rect = &dst_rect;
                                                                    object_enqueue_blit(&art_blit_info, underlay_order++);
                                                                }
                                                                rect_node = rect_node->next;
                                                            }
                                                        }
                                                    }
                                                } /* OF_HAS_UNDERLAYS */

                                                // 0x43B7D6
                                                if ((obj_flags & OF_HAS_OVERLAYS) != 0) {
                                                    for (idx = 6; idx >= 0; idx--) {
                                                        for (int fld = OBJ_F_OVERLAY_FORE; fld <= OBJ_F_OVERLAY_BACK; fld++) {
                                                            art_id = obj_arrayfield_uint32_get(obj_node->obj, fld, idx);
                                                            if (art_id != TIG_ART_ID_INVALID) {
                                                                sub_443620(obj_flags, scale, (int)loc_x, (int)loc_y, art_id, &eye_candy_rect);

                                                                rect_node = *draw_info->rects;
                                                                while (rect_node != NULL) {
                                                                    if (tig_rect_intersection(&eye_candy_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                                                        src_rect.x = dst_rect.x - eye_candy_rect.x;
                                                                        src_rect.y = dst_rect.y - eye_candy_rect.y;
                                                                        src_rect.width = dst_rect.width;
                                                                        src_rect.height = dst_rect.height;

                                                                        art_blit_info.flags = 0;
                                                                        if (tig_art_eye_candy_id_translucency_get(art_id) != 0) {
                                                                            art_blit_info.flags |= TIG_ART_BLT_BLEND_ADD;
                                                                        }

                                                                        art_blit_info.art_id = art_id;
                                                                        art_blit_info.src_rect = &src_rect;
                                                                        art_blit_info.dst_rect = &dst_rect;

                                                                        int scale_type = tig_art_eye_candy_id_scale_get(art_id);
                                                                        int overlay_scale = scale_type != 4
                                                                            ? scale * dword_5A548C[scale_type] / 100
                                                                            : scale;

                                                                        if (scale_type != 100) {
                                                                            src_rect.x = (int)((float)src_rect.x / (float)overlay_scale * 100.0f);
                                                                            src_rect.y = (int)((float)src_rect.y / (float)overlay_scale * 100.0f);
                                                                            src_rect.width = (int)((float)src_rect.width / (float)overlay_scale * 100.0f);
                                                                            src_rect.height = (int)((float)src_rect.height / (float)overlay_scale * 100.0f);
                                                                        }

                                                                        if ((obj_flags & OF_SHRUNK) != 0) {
                                                                            src_rect.x *= 2;
                                                                            src_rect.y *= 2;
                                                                            src_rect.width *= 2;
                                                                            src_rect.height *= 2;
                                                                        }

                                                                        if (obj_type == OBJ_TYPE_ARMOR) {
                                                                            order = non_flat_order++;
                                                                        } else {
                                                                            order = overlay_order++;
                                                                        }

                                                                        object_enqueue_blit(&art_blit_info, order);
                                                                    }
                                                                    rect_node = rect_node->next;
                                                                }
                                                            }
                                                        }
                                                    }
                                                } /* OF_HAS_OVERLAYS */

                                                // 0x43B9FD
                                                if (((obj_flags & OF_INVISIBLE) == 0 || is_detecting_invisible)
                                                    && (dword_5E2EC8 & obj_flags) == 0) {
                                                    art_blit_info.flags = TIG_ART_BLT_BLEND_COLOR_CONST | TIG_ART_BLT_BLEND_SUB;
                                                    art_blit_info.src_rect = &src_rect;
                                                    art_blit_info.dst_rect = &dst_rect;

                                                    if ((obj_flags & OF_FLAT) == 0) {
                                                        unsigned int render_flags = obj_field_int32_get(obj_node->obj, OBJ_F_RENDER_FLAGS);
                                                        if ((render_flags & ORF_04000000) == 0) {
                                                            if (shadow_apply(obj_node->obj)) {
                                                                render_flags |= ORF_10000000;
                                                            }
                                                            render_flags |= ORF_04000000;
                                                            obj_field_int32_set(obj_node->obj, OBJ_F_RENDER_FLAGS, render_flags);
                                                        }

                                                        if ((render_flags & ORF_10000000) != 0) {
                                                            for (idx = 0; idx < SHADOW_HANDLE_MAX; idx++) {
                                                                Shadow* shadow = (Shadow*)obj_arrayfield_ptr_get(obj_node->obj, OBJ_F_SHADOW_HANDLES, idx);
                                                                if (shadow == NULL) {
                                                                    break;
                                                                }

                                                                sub_443620(obj_flags, scale, (int)loc_x, (int)loc_y, shadow->art_id, &eye_candy_rect);
                                                                art_blit_info.art_id = shadow->art_id;
                                                                art_blit_info.color = shadow->color;

                                                                if ((obj_flags & OF_WADING) != 0) {
                                                                    art_blit_info.color = tig_color_mul(art_blit_info.color, tig_color_make(92, 92, 92));
                                                                }

                                                                rect_node = *draw_info->rects;
                                                                while (rect_node != NULL) {
                                                                    if (tig_rect_intersection(&eye_candy_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                                                        src_rect.x = dst_rect.x - eye_candy_rect.x;
                                                                        src_rect.y = dst_rect.y - eye_candy_rect.y;
                                                                        src_rect.width = dst_rect.width;
                                                                        src_rect.height = dst_rect.height;

                                                                        if (scale != 100) {
                                                                            src_rect.x = (int)((float)src_rect.x / (float)scale * 100.0f);
                                                                            src_rect.y = (int)((float)src_rect.y / (float)scale * 100.0f);
                                                                            src_rect.width = (int)((float)src_rect.width / (float)scale * 100.0f);
                                                                            src_rect.height = (int)((float)src_rect.height / (float)scale * 100.0f);
                                                                        }

                                                                        if ((obj_flags & OF_SHRUNK) != 0) {
                                                                            src_rect.x *= 2;
                                                                            src_rect.y *= 2;
                                                                            src_rect.width *= 2;
                                                                            src_rect.height *= 2;
                                                                        }

                                                                        object_enqueue_blit(&art_blit_info, shadow_order++);
                                                                    }
                                                                    rect_node = rect_node->next;
                                                                }
                                                            }
                                                        }
                                                    }

                                                    object_get_rect(obj_node->obj, RECT_FLAG_NONE, &eye_candy_rect);

                                                    v71 = false;
                                                    if ((obj_flags & OF_WADING) != 0 && (obj_flags & OF_WATER_WALKING) == 0) {
                                                        tmp_rect = eye_candy_rect;
                                                        if ((obj_flags & OF_FLAT) == 0) {
                                                            tmp_rect.height = 15;
                                                            tmp_rect.y = eye_candy_rect.height + eye_candy_rect.y - 15;
                                                        } else {
                                                            v71 = true;
                                                        }

                                                        art_blit_info_initialized = false;
                                                        rect_node = *draw_info->rects;
                                                        while (rect_node != NULL) {
                                                            if (tig_rect_intersection(&tmp_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                                                if (!art_blit_info_initialized) {
                                                                    object_setup_blit(obj_node->obj, &art_blit_info);

                                                                    art_blit_info.flags &= ~0x19E80;
                                                                    art_blit_info.flags |= TIG_ART_BLT_BLEND_ALPHA_CONST;
                                                                    art_blit_info.src_rect = &src_rect;
                                                                    art_blit_info.dst_rect = &dst_rect;
                                                                    art_blit_info.alpha[0] = 92;

                                                                    art_blit_info_initialized = true;

                                                                    if ((obj_flags & OF_FLAT) == 0) {
                                                                        order = non_flat_order++;
                                                                    } else {
                                                                        order = flat_order++;
                                                                    }
                                                                }

                                                                src_rect.x = dst_rect.x - tmp_rect.x;
                                                                src_rect.y = dst_rect.y - tmp_rect.y + eye_candy_rect.height - 15;
                                                                src_rect.width = dst_rect.width;
                                                                src_rect.height = dst_rect.height;

                                                                if (scale != 100) {
                                                                    src_rect.x = (int)((float)src_rect.x / (float)scale * 100.0f);
                                                                    src_rect.y = (int)((float)src_rect.y / (float)scale * 100.0f);
                                                                    src_rect.width = (int)((float)src_rect.width / (float)scale * 100.0f);
                                                                    src_rect.height = (int)((float)src_rect.height / (float)scale * 100.0f);
                                                                }

                                                                if ((obj_flags & OF_SHRUNK) != 0) {
                                                                    src_rect.x *= 2;
                                                                    src_rect.y *= 2;
                                                                    src_rect.width *= 2;
                                                                    src_rect.height *= 2;
                                                                }

                                                                object_enqueue_blit(&art_blit_info, order);
                                                            }
                                                            rect_node = rect_node->next;
                                                        }

                                                        if ((obj_flags & OF_FLAT) == 0) {
                                                            eye_candy_rect.height -= 15;
                                                        }
                                                    }

                                                    // 0x43BF8F
                                                    if (!v71) {
                                                        art_blit_info_initialized = false;

                                                        rect_node = *draw_info->rects;
                                                        while (rect_node != NULL) {
                                                            if (tig_rect_intersection(&eye_candy_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                                                if (!art_blit_info_initialized) {
                                                                    object_setup_blit(obj_node->obj, &art_blit_info);
                                                                    art_blit_info.src_rect = &src_rect;
                                                                    art_blit_info.dst_rect = &dst_rect;

                                                                    art_blit_info_initialized = true;

                                                                    if ((obj_flags & OF_FLAT) == 0) {
                                                                        order = non_flat_order++;
                                                                    } else {
                                                                        order = flat_order++;
                                                                    }
                                                                }

                                                                src_rect.x = dst_rect.x - eye_candy_rect.x;
                                                                src_rect.y = dst_rect.y - eye_candy_rect.y;
                                                                src_rect.width = dst_rect.width;
                                                                src_rect.height = dst_rect.height;

                                                                if (scale != 100) {
                                                                    object_iso_invalidate_rect(&eye_candy_rect);
                                                                    src_rect.x = (int)((float)src_rect.x / (float)scale * 100.0f);
                                                                    src_rect.y = (int)((float)src_rect.y / (float)scale * 100.0f);
                                                                    src_rect.width = (int)((float)src_rect.width / (float)scale * 100.0f);
                                                                    src_rect.height = (int)((float)src_rect.height / (float)scale * 100.0f);
                                                                }

                                                                if ((obj_flags & OF_SHRUNK) != 0) {
                                                                    src_rect.x *= 2;
                                                                    src_rect.y *= 2;
                                                                    src_rect.width *= 2;
                                                                    src_rect.height *= 2;
                                                                }

                                                                object_enqueue_blit(&art_blit_info, order);
                                                            }
                                                            rect_node = rect_node->next;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    obj_node = obj_node->next;
                                }
                            }
                        }

                        indexes[row]++;
                        locations[row]++;
                    }

                    indexes[row] += widths[row];
                    locations[row] = LOCATION_MAKE(LOCATION_GET_X(locations[row]) + v5[row], LOCATION_GET_Y(locations[row]) + 1); // TODO: Unclear.
                }
            }
        }

        for (row = 0; row < v2->width; row++) {
            if (locks[row]) {
                sector_unlock(v2->field_20[row]);
            }
        }
    }

    sub_43C5C0(draw_info);
    object_flush_pending_blits();
}

// 0x43C270
void object_hover_obj_set(int64_t obj)
{
    TigRect rect;
    int type;
    int64_t pc_obj;
    int reaction_level;
    int reaction_type;

    if (object_hover_obj != obj) {
        if (sub_4437E0(&rect)) {
            object_iso_invalidate_rect(&rect);
        }

        dword_5E2E94 = false;
        object_hover_obj = obj;
        object_hover_color = tig_color_make(255, 255, 255);

        tig_art_interface_id_create(467, dword_5E2E6C, 1, 0, &object_hover_underlay_art_id);
        tig_art_interface_id_create(468, dword_5E2E68, 1, 0, &object_hover_overlay_art_id);

        if (object_hover_obj != OBJ_HANDLE_NULL) {
            type = obj_field_int32_get(object_hover_obj, OBJ_F_TYPE);
            if (type != OBJ_TYPE_WALL && type != OBJ_TYPE_PROJECTILE) {
                if (type >= OBJ_TYPE_PC) {
                    pc_obj = player_get_local_pc_obj();
                    if (pc_obj == OBJ_HANDLE_NULL) {
                        object_hover_obj = OBJ_HANDLE_NULL;
                        sub_443EB0(OBJ_HANDLE_NULL, &stru_5E2F60);
                        return;
                    }

                    dword_5E2E94 = true;

                    if (type == OBJ_TYPE_NPC) {
                        reaction_level = reaction_get(object_hover_obj, pc_obj);
                        reaction_type = reaction_translate(reaction_level);
                        object_hover_color = object_reaction_colors[reaction_type];

                        if (combat_critter_is_combat_mode_active(pc_obj)) {
                            tig_art_interface_id_create(555, dword_5E2E6C, 1, 0, &object_hover_underlay_art_id);
                            tig_art_interface_id_create(555, dword_5E2E68, 1, 0, &object_hover_overlay_art_id);
                        }
                    }
                }

                if (sub_4437E0(&rect)) {
                    object_iso_invalidate_rect(&rect);
                }
            }
        }

        sub_443EB0(object_hover_obj, &stru_5E2F60);
    }
}

// 0x43C570
int64_t object_hover_obj_get()
{
    if (object_hover_obj == OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    if (!sub_444020(&object_hover_obj, &stru_5E2F60)) {
        object_hover_obj_set(OBJ_HANDLE_NULL);
        return OBJ_HANDLE_NULL;
    }

    return object_hover_obj;
}

// 0x43C5C0
void sub_43C5C0(GameDrawInfo* draw_info)
{
    TigRect rect;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigRectListNode* node;

    if (dword_5E2E94) {
        if (sub_443880(&rect, object_hover_underlay_art_id)) {
            art_blit_info.flags = TIG_ART_BLT_BLEND_ADD | TIG_ART_BLT_BLEND_COLOR_CONST;
            art_blit_info.art_id = object_hover_underlay_art_id;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            art_blit_info.color = object_hover_color;

            node = *draw_info->rects;
            while (node != NULL) {
                if (tig_rect_intersection(&rect, &(node->rect), &dst_rect) == TIG_OK) {
                    src_rect.x = dst_rect.x - rect.x;
                    src_rect.y = dst_rect.y - rect.y;
                    src_rect.width = dst_rect.width;
                    src_rect.height = dst_rect.height;
                    object_enqueue_blit(&art_blit_info, 99999999);
                }
                node = node->next;
            }
        }
    }
}

// 0x43C690
void sub_43C690(GameDrawInfo* draw_info)
{
    TigRectListNode* rect_node;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    bool blit_info_initialized = false;

    object_hover_obj = object_hover_obj_get();

    if (object_hover_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (dword_5E2E94) {
        TigRect rect;

        if (sub_443880(&rect, object_hover_overlay_art_id)) {
            art_blit_info.flags = TIG_ART_BLT_BLEND_COLOR_CONST | TIG_ART_BLT_BLEND_ADD;
            art_blit_info.art_id = object_hover_overlay_art_id;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            art_blit_info.color = object_hover_color;

            rect_node = *draw_info->rects;
            while (rect_node != NULL) {
                if (tig_rect_intersection(&rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                    src_rect.y = dst_rect.y - rect.y;
                    src_rect.x = dst_rect.x - rect.x;
                    src_rect.width = dst_rect.width;
                    src_rect.height = dst_rect.height;
                    object_enqueue_blit(&art_blit_info, INT_MAX);
                }
                rect_node = rect_node->next;
            }
        }
    } else {
        int obj_type;
        tig_color_t color;
        bool is_detecting_invisible;
        unsigned int flags;
        TigRect obj_rect;
        TigRect tmp_rect;

        obj_type = obj_field_int32_get(object_hover_obj, OBJ_F_TYPE);
        if (obj_type == OBJ_TYPE_WALL
            || obj_type == OBJ_TYPE_PROJECTILE
            || obj_type >= OBJ_TYPE_PC) {
            return;
        }

        color = tig_color_make(200, 200, 200);
        is_detecting_invisible = magictech_check_env_sf(OSF_DETECTING_INVISIBLE);
        flags = obj_field_int32_get(object_hover_obj, OBJ_F_FLAGS);
        if (!is_detecting_invisible
            && (flags & (OF_DONTDRAW | OF_INVISIBLE)) != 0) {
            return;
        }

        object_get_rect(object_hover_obj, RECT_FLAG_NONE, &obj_rect);

        if ((flags & OF_WADING) != 0 && (flags & OF_WATER_WALKING) == 0) {
            tmp_rect = obj_rect;
            tmp_rect.y = obj_rect.y + obj_rect.height - 15;
            tmp_rect.height = 15;

            rect_node = *draw_info->rects;
            while (rect_node != NULL) {
                if (tig_rect_intersection(&tmp_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                    if (!blit_info_initialized) {
                        object_setup_blit(object_hover_obj, &art_blit_info);

                        art_blit_info.flags &= ~0x3DF80;
                        art_blit_info.flags |= TIG_ART_BLT_BLEND_COLOR_CONST | TIG_ART_BLT_BLEND_ADD;
                        art_blit_info.src_rect = &src_rect;
                        art_blit_info.dst_rect = &dst_rect;

                        if (!dword_5E2F48) {
                            art_blit_info.flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
                        }

                        art_blit_info.color = color;

                        blit_info_initialized = true;
                    }

                    src_rect.x = dst_rect.x - tmp_rect.x;
                    src_rect.y = dst_rect.y - tmp_rect.y + obj_rect.height - 15;
                    src_rect.width = dst_rect.width;
                    src_rect.height = dst_rect.height;

                    if ((flags & OF_SHRUNK) != 0) {
                        src_rect.x *= 2;
                        src_rect.y *= 2;
                        src_rect.width *= 2;
                        src_rect.height *= 2;
                    }

                    object_enqueue_blit(&art_blit_info, INT_MAX);
                }
                rect_node = rect_node->next;
            }

            obj_rect.height -= 15;
        }

        blit_info_initialized = false;
        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&obj_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                if (!blit_info_initialized) {
                    object_setup_blit(object_hover_obj, &art_blit_info);

                    art_blit_info.flags &= ~0x3DF80;
                    art_blit_info.flags |= TIG_ART_BLT_BLEND_COLOR_CONST | TIG_ART_BLT_BLEND_ADD;
                    art_blit_info.src_rect = &src_rect;
                    art_blit_info.dst_rect = &dst_rect;

                    if (!dword_5E2F48) {
                        art_blit_info.flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
                    }

                    art_blit_info.color = color;

                    blit_info_initialized = true;
                }

                src_rect.x = dst_rect.x - obj_rect.x;
                src_rect.y = dst_rect.y - obj_rect.y;
                src_rect.width = dst_rect.width;
                src_rect.height = dst_rect.height;

                if ((flags & OF_SHRUNK) != 0) {
                    src_rect.x *= 2;
                    src_rect.y *= 2;
                    src_rect.width *= 2;
                    src_rect.height *= 2;
                }

                object_enqueue_blit(&art_blit_info, INT_MAX);
            }

            rect_node = rect_node->next;
        }
    }
}

// 0x43CB10
void object_invalidate_rect(TigRect* rect)
{
    if (rect == NULL) {
        rect = &object_iso_content_rect;
    }

    if (object_dirty_rects_head != NULL) {
        sub_52D480(&object_dirty_rects_head, rect);
    } else {
        object_dirty_rects_head = tig_rect_node_create();
        object_dirty_rects_head->rect = *rect;
    }

    object_dirty = true;
}

// 0x43CB70
void object_flush()
{
    TigRectListNode* next;

    while (object_dirty_rects_head != NULL) {
        next = object_dirty_rects_head->next;
        tig_rect_node_destroy(object_dirty_rects_head);
        object_dirty_rects_head = next;
    }
}

// 0x43CBA0
bool object_create(int64_t proto_obj, int64_t loc, int64_t* obj_ptr)
{
    ObjectID oid;

    oid.type = OID_TYPE_BLOCKED;
    return object_create_func(proto_obj, loc, obj_ptr, oid);
}

// 0x43CBF0
bool object_create_ex(int64_t proto_obj, int64_t loc, ObjectID oid, int64_t* obj_ptr)
{
    return object_create_func(proto_obj, loc, obj_ptr, oid);
}

// 0x43CC30
bool object_duplicate(int64_t proto_obj, int64_t loc, int64_t* obj_ptr)
{
    ObjectID oid;

    oid.type = OID_TYPE_BLOCKED;
    return object_duplicate_func(proto_obj, loc, &oid, obj_ptr);
}

// 0x43CC70
bool object_duplicate_ex(int64_t proto_obj, int64_t loc, ObjectID* oids, int64_t* obj_ptr)
{
    return object_duplicate_func(proto_obj, loc, oids, obj_ptr);
}

// 0x43CCA0
void object_destroy(int64_t obj)
{
    TigRect rect;
    int obj_type;
    int64_t pc_obj;
    int64_t parent_obj;
    int64_t loc;
    ObjectList objects;
    ObjectNode* node;
    unsigned int flags;

    if (object_editor) {
        char* save_path;
        map_paths(NULL, &save_path);
        sub_43CEA0(obj, -1, save_path);
        return;
    }

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return;
    }

    if (!object_script_execute(obj, obj, OBJ_HANDLE_NULL, SAP_DESTROY, 0)) {
        return;
    }

    if (!multiplayer_is_locked() && tig_net_is_host()) {
        mp_object_destroy(obj);
    }

    multiplayer_lock();

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)) {
        sub_45E180(obj);
        ai_timeevent_clear(obj);

        if (obj_type == OBJ_TYPE_NPC) {
            pc_obj = sub_4C1110(obj);
            if (pc_obj != OBJ_HANDLE_NULL) {
                sub_460A20(pc_obj, 0);
            }
        }

        sub_4639E0(obj, true);
    } else if (obj_type_is_item(obj_type)) {
        if (item_parent(obj, &parent_obj)) {
            loc = obj_field_int64_get(parent_obj, OBJ_F_LOCATION);
            item_remove(obj);
            object_drop(obj, loc);
        }
    } else if (obj_type == OBJ_TYPE_CONTAINER) {
        sub_4639E0(obj, true);
    }

    sub_423FF0(obj);
    sub_459740(obj);
    sub_4601D0(obj);
    sub_443770(obj);

    if (obj_type == OBJ_TYPE_WALL) {
        loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
        object_list_location(loc, OBJ_TM_PORTAL, &objects);
        node = objects.head;
        while (node != NULL) {
            object_destroy(node->obj);
            node = node->next;
        }
        object_list_destroy(&objects);
    }

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    flags |= OF_DESTROYED;
    obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

    multiplayer_unlock();
}

// 0x43CEA0
void sub_43CEA0(int64_t obj, unsigned int flags, const char* path)
{
    TigRect rect;
    int type;

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);

    sub_463B30(obj, false);

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (type == OBJ_TYPE_PROJECTILE
        || type == OBJ_TYPE_CONTAINER
        || obj_type_is_critter(type)
        || obj_type_is_item(type)) {
        objf_solitary_delete(obj, path, ".mob");
    }

    switch (type) {
    case OBJ_TYPE_WALL:
        wall_delete(obj, (flags & 0x1) != 0);
        break;
    case OBJ_TYPE_PORTAL:
        portal_delete(obj, (flags & 0x1) != 0);
        break;
    default:
        sub_43CF70(obj);
        object_delete(obj);
        break;
    }
}

// 0x43CF70
void sub_43CF70(int64_t obj)
{
    int64_t sector_id;
    Sector* sector;

    sub_443770(obj);

    sector_id = sector_id_from_loc(obj_field_int64_get(obj, OBJ_F_LOCATION));
    if (object_is_static(obj) || sub_4D04E0(sector_id)) {
        sector_lock(sector_id, &sector);
        objlist_remove(&(sector->objects), obj);

        // FIXME: Unused.
        obj_field_int32_get(obj, OBJ_F_FLAGS);

        sector_unlock(sector_id);
    }
}

// 0x43CFF0
void object_delete(int64_t obj)
{
    unsigned int flags;

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    if ((flags & OF_TEXT) != 0) {
        tb_remove(obj);
    }
    if ((flags & OF_TEXT_FLOATER) != 0) {
        tf_remove(obj);
    }
    sub_423FF0(obj);
    sub_459740(obj);
    ai_timeevent_clear(obj);
    sub_4601D0(obj);
    sub_443770(obj);
    obj_deallocate(obj);
}

// 0x43D0E0
void object_flags_set(int64_t obj, unsigned int flags)
{
    TigRect rect;
    unsigned int render_flags = 0;
    unsigned int cur_render_flags;
    unsigned int cur_flags;
    unsigned int extra_flags = 0;

    if ((flags & OF_FLAT) != 0) {
        object_toggle_flat(obj, true);
        flags &= ~OF_FLAT;
    }

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);

    cur_flags = obj_field_int32_get(obj, OBJ_F_FLAGS);

    // FIXME: Unused.
    cur_render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);

    if ((flags & OF_OFF) != 0) {
        if ((cur_flags & OF_OFF) == 0) {
            if ((cur_flags & OF_TEXT) != 0) {
                tb_remove(obj);
            }
            if ((cur_flags & OF_TEXT_FLOATER) != 0) {
                tf_remove(obj);
            }
            light_set_flags(obj, LF_OFF);
            extra_flags |= OF_OFF;
        }
        flags &= ~OF_OFF;
    }

    if ((flags & OF_STONED) != 0) {
        if ((cur_flags & OF_STONED) == 0) {
            render_flags |= ORF_02000000;
            extra_flags |= OF_STONED;
        }
        flags &= ~OF_STONED;
    }

    if ((flags & OF_FROZEN) != 0) {
        if ((cur_flags & OF_FROZEN) == 0) {
            render_flags |= ORF_02000000;
            extra_flags |= OF_FROZEN;
        }
        flags &= ~OF_FROZEN;
    }

    if ((flags & OF_ANIMATED_DEAD) != 0) {
        if ((cur_flags & OF_ANIMATED_DEAD) == 0) {
            render_flags |= ORF_02000000;
            extra_flags |= OF_ANIMATED_DEAD;
        }
        flags &= ~OF_ANIMATED_DEAD;
    }

    if ((flags & OF_DONTLIGHT) != 0) {
        render_flags |= ORF_02000000;
    }

    obj_field_int32_set(obj,
        OBJ_F_FLAGS,
        obj_field_int32_get(obj, OBJ_F_FLAGS) | flags | extra_flags);
    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~render_flags);

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);
}

// 0x43D280
void object_flags_unset(int64_t obj, unsigned int flags)
{
    TigRect rect;
    unsigned int render_flags = 0;
    unsigned int cur_render_flags;
    unsigned int cur_flags;
    unsigned int extra_flags = 0;
    bool visibility_changed = false;

    if ((flags & OF_FLAT) != 0) {
        object_toggle_flat(obj, false);
        flags &= ~OF_FLAT;
    }

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);

    cur_flags = obj_field_int32_get(obj, OBJ_F_FLAGS);

    // FIXME: Unused.
    cur_render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);

    if ((flags & OF_OFF) != 0) {
        render_flags |= ORF_04000000 | ORF_02000000 | ORF_01000000;
        light_unset_flags(obj, LF_OFF);
        extra_flags |= OF_OFF;
        flags &= ~OF_OFF;
        visibility_changed = true;
    }

    if ((flags & OF_STONED) != 0) {
        if ((cur_flags & OF_STONED) != 0) {
            render_flags |= ORF_02000000;
            extra_flags |= OF_STONED;
        }
        flags &= ~OF_STONED;
    }

    if ((flags & OF_FROZEN) != 0) {
        if ((cur_flags & OF_FROZEN) != 0) {
            render_flags |= ORF_02000000;
            extra_flags |= OF_FROZEN;
        }
        flags &= ~OF_FROZEN;
    }

    if ((flags & OF_ANIMATED_DEAD) != 0) {
        if ((cur_flags & OF_ANIMATED_DEAD) != 0) {
            render_flags |= ORF_02000000;
            extra_flags |= OF_ANIMATED_DEAD;
        }
        flags &= ~OF_ANIMATED_DEAD;
    }

    if ((flags & OF_DONTLIGHT) != 0) {
        render_flags |= ORF_02000000;
    }

    obj_field_int32_set(obj,
        OBJ_F_FLAGS,
        obj_field_int32_get(obj, OBJ_F_FLAGS) & ~(flags | extra_flags));
    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~render_flags);

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);

    if (visibility_changed) {
        if (!object_editor) {
            sub_43F710(obj);
        }
    }
}

// 0x43D410
int object_hp_pts_get(int64_t obj)
{
    return obj_field_int32_get(obj, OBJ_F_HP_PTS);
}

// 0x43D430
int object_hp_pts_set(int64_t obj, int value)
{
    Packet51 pkt;

    if (!multiplayer_is_locked()) {
        pkt.type = 51;
        pkt.field_4 = 0;
        pkt.field_8 = obj_get_id(obj);
        pkt.field_20 = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));

        if (!tig_net_is_host()) {
            return value;
        }
    }

    if (value < 0) {
        value = 0;
    }

    obj_field_int32_set(obj, OBJ_F_HP_PTS, value);
    ui_refresh_health_bar(obj);

    return value;
}

// 0x43D4C0
int object_hp_adj_get(int64_t obj)
{
    return obj_field_int32_get(obj, OBJ_F_HP_ADJ);
}

// 0x43D4E0
int object_hp_adj_set(int64_t obj, int value)
{
    obj_field_int32_set(obj, OBJ_F_HP_ADJ, value);
    ui_refresh_health_bar(obj);
    return value;
}

// 0x43D510
int object_hp_damage_get(int64_t obj)
{
    return obj_field_int32_get(obj, OBJ_F_HP_DAMAGE);
}

// 0x43D530
int object_hp_damage_set(int64_t obj, int value)
{
    int obj_type;

    if (tig_net_is_active() && !tig_net_is_host()) {
        return 0;
    }

    if (value < 0) {
        value = 0;
    }

    mp_obj_field_int32_set(obj, OBJ_F_HP_DAMAGE, value);

    if (value > 0) {
        obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
        if (obj_type_is_critter(obj_type)) {
            critter_resting_timeevent_schedule(obj);
        }
    }

    sub_4EE1D0(obj);

    return value;
}

// 0x43D5A0
int object_hp_max(int64_t obj)
{
    int value;
    int obj_type;

    value = object_hp_adj_get(obj) + sub_43D690(obj);
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)) {
        value = effect_adjust_max_hit_points(obj, sub_43D630(obj) + value);
    }
    return value;
}

// 0x43D600
int object_hp_current(int64_t obj)
{
    return object_hp_max(obj) - object_hp_damage_get(obj);
}

// 0x43D630
int sub_43D630(int64_t obj)
{
    int obj_type;

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)) {
        return stat_level_get(obj, STAT_WILLPOWER)
            + 2 * (stat_level_get(obj, STAT_STRENGTH) + stat_level_get(obj, STAT_LEVEL))
            + 4;
    }

    return 0;
}

// 0x43D690
int sub_43D690(int64_t obj)
{
    int obj_type;

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)) {
        return 4 * object_hp_pts_get(obj);
    }

    return object_hp_pts_get(obj);
}

// 0x43D6D0
int object_get_resistance(int64_t obj, int resistance_type, bool a2)
{
    int value;
    int obj_type;
    int inventory_location;
    int64_t item_obj;
    int adj;

    value = obj_arrayfield_int32_get(obj, OBJ_F_RESISTANCE_IDX, resistance_type);
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);

    if (obj_type_is_critter(obj_type)) {
        for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
            item_obj = item_wield_get(obj, inventory_location);
            if (item_obj != OBJ_HANDLE_NULL
                && obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_ARMOR) {
                adj = obj_arrayfield_int32_get(item_obj, OBJ_F_ARMOR_RESISTANCE_ADJ_IDX, resistance_type);
                value += adj;

                if (!a2
                    || obj_field_int32_get(item_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) <= 0
                    || item_is_identified(item_obj)) {
                    adj = obj_arrayfield_int32_get(item_obj, OBJ_F_ARMOR_MAGIC_RESISTANCE_ADJ_IDX, resistance_type);
                    value += item_adjust_magic(item_obj, obj, adj);
                }
            }
        }

        if (resistance_type == RESISTANCE_TYPE_POISON) {
            adj = 5 * (stat_level_get(obj, STAT_CONSTITUTION) - 4);
            if (adj > 0) {
                value += adj;
            }
        }

        value = effect_adjust_resistance(obj, resistance_type, value);
        if (obj_type == OBJ_TYPE_NPC) {
            if (critter_is_monstrous(obj)) {
                return value;
            }
        }
    }

    if (value < 0) {
        value = 0;
    } else if (value > 95) {
        value = 95;
    }

    return value;
}

// 0x43D880
int object_get_ac(int64_t obj, bool a2)
{
    int ac;
    int obj_type;
    int index;
    int64_t item_obj;

    ac = obj_field_int32_get(obj, OBJ_F_AC);
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)) {
        for (index = FIRST_WEAR_INV_LOC; index <= LAST_WEAR_INV_LOC; index++) {
            item_obj = item_wield_get(obj, index);
            if (item_obj != OBJ_HANDLE_NULL) {
                ac += item_armor_ac_adj(obj, item_obj, a2);
            }
        }

        ac = effect_adjust_stat_level(obj, STAT_AC_ADJUSTMENT, ac);
        ac += stat_level_get(obj, STAT_AC_ADJUSTMENT);
    } else if (obj_type == OBJ_TYPE_ARMOR) {
        ac += item_armor_ac_adj(obj, OBJ_HANDLE_NULL, a2);
    }

    if (ac < 0) {
        ac = 0;
    } else if (ac > 95) {
        ac = 95;
    }

    return ac;
}

// 0x43D940
bool sub_43D940(int64_t obj)
{
    int obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_PROJECTILE
        || obj_type == OBJ_TYPE_CONTAINER
        || obj_type_is_critter(obj_type)
        || obj_type_is_item(obj_type)) {
        return false;
    }
    return true;
}

// 0x43D990
bool object_is_static(int64_t obj)
{
    int obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_PROJECTILE
        || obj_type == OBJ_TYPE_CONTAINER
        || obj_type_is_critter(obj_type)
        || obj_type_is_item(obj_type)) {
        return false;
    }
    return (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DYNAMIC) == 0;
}

// 0x43D9F0
bool sub_43D9F0(int x, int y, int64_t* obj_ptr, unsigned int flags)
{
    TigRect rect;
    LocRect loc_rect;
    SomeSectorStuff v1;
    SomeSectorStuffEntry* v3;
    int row;
    int col;
    int indexes[3] = { 0 };
    int widths[3] = { 0 };
    bool locks[3] = { false };
    Sector* sectors[3];
    int64_t v65;
    int64_t v67;
    int64_t v69;
    int v62;
    int v60;
    ObjectNode* obj_node;
    ObjectType obj_type;
    ObjectFlags obj_flags;
    TigRect obj_rect;
    tig_art_id_t aid;
    int scale;
    int test_x;
    int test_y;
    int ext_x;
    int ext_y;

    if (object_view_options.type == VIEW_TYPE_TOP_DOWN) {
        return false;
    }

    if (sub_439890(x, y)) {
        return false;
    }

    rect.x = x - 256;
    rect.y = y - 256;
    rect.width = 512;
    rect.height = 512;

    if (!sub_4B9130(&rect, &loc_rect)) {
        return false;
    }

    if (!sub_4D0090(&loc_rect, &v1)) {
        return false;
    }

    v65 = OBJ_HANDLE_NULL;
    v67 = OBJ_HANDLE_NULL;
    v69 = OBJ_HANDLE_NULL;

    for (row = v1.height - 1; row >= 0; row--) {
        v3 = &(v1.field_8[row]);
        for (col = v3->width - 1; col >= 0; col--) {
            indexes[col] = v3->field_50 * 64 + v3->field_38[col] + v3->field_44[col] - 64 - 1;
            widths[col] = 64 - v3->field_44[col];
            locks[col] = sector_lock(v3->field_20[col], &(sectors[col]));
        }

        for (v62 = v3->field_50 - 1; v62 >= 0; v62--) {
            for (col = v3->width - 1; col >= 0; col--) {
                if (locks[col]) {
                    for (v60 = v3->field_44[col] - 1; v60 >= 0; v60--) {
                        for (obj_node = sectors[col]->objects.heads[indexes[col]]; obj_node != NULL; obj_node = obj_node->next) {
                            obj_type = obj_field_int32_get(obj_node->obj, OBJ_F_TYPE);
                            if (!object_type_visibility[obj_type]) {
                                continue;
                            }

                            // FIX: Original code writes obj flags into `flags`
                            // parameter, which probably leads to wrong results.
                            obj_flags = obj_field_int32_get(obj_node->obj, OBJ_F_FLAGS);
                            if ((dword_5E2F88 & obj_flags) != 0) {
                                continue;
                            }

                            if (!object_editor) {
                                if ((obj_flags & OF_CLICK_THROUGH) != 0) {
                                    continue;
                                }
                            }

                            object_get_rect(obj_node->obj, RECT_FLAG_NONE, &obj_rect);

                            aid = obj_field_int32_get(obj_node->obj, OBJ_F_CURRENT_AID);
                            scale = obj_field_int32_get(obj_node->obj, OBJ_F_BLIT_SCALE);

                            if ((obj_flags & OF_SHRUNK) != 0) {
                                scale /= 2;
                            }

                            if (x >= obj_rect.x
                                && y >= obj_rect.y
                                && x < obj_rect.x + obj_rect.width
                                && y < obj_rect.y + obj_rect.height) {
                                if ((flags & 0x02) != 0 && v65 != OBJ_HANDLE_NULL) {
                                    v65 = obj_node->obj;
                                }

                                test_x = x - obj_rect.x;
                                test_y = y - obj_rect.y;

                                if (scale != 100) {
                                    test_x = (int)((float)test_x / (float)scale * 100.0f);
                                    test_y = (int)((float)test_y / (float)scale * 100.0f);
                                }

                                if (!sub_502FD0(aid, test_x, test_y)) {
                                    v67 = obj_node->obj;

                                    if (obj_type == OBJ_TYPE_WALL
                                        && (obj_field_int32_get(obj_node->obj, OBJ_F_WALL_FLAGS) & (OWAF_TRANS_LEFT | OWAF_TRANS_RIGHT)) != 0) {
                                        v67 = OBJ_HANDLE_NULL;
                                    }
                                }
                            }

                            if ((flags & 0x01) != 0
                                && v69 == OBJ_HANDLE_NULL
                                && v67 == OBJ_HANDLE_NULL) {
                                for (ext_y = y - 2; ext_y < y + 3; ext_y++) {
                                    for (ext_x = x - 2; ext_x < x + 3; ext_x++) {
                                        if (ext_x >= obj_rect.x
                                            && ext_y >= obj_rect.y
                                            && ext_x < obj_rect.x + obj_rect.width
                                            && ext_y < obj_rect.y + obj_rect.height) {

                                            test_x = ext_x - obj_rect.x;
                                            test_y = ext_y - obj_rect.y;

                                            if (scale != 100) {
                                                test_x = (int)((float)test_x / (float)scale * 100.0f);
                                                test_y = (int)((float)test_y / (float)scale * 100.0f);
                                            }

                                            if (!sub_502FD0(aid, test_x, test_y)) {
                                                v69 = obj_node->obj;
                                                if (obj_type == OBJ_TYPE_WALL
                                                    && (obj_field_int32_get(obj_node->obj, OBJ_F_WALL_FLAGS) & (OWAF_TRANS_LEFT | OWAF_TRANS_RIGHT)) != 0) {
                                                    v69 = OBJ_HANDLE_NULL;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (v67 != OBJ_HANDLE_NULL) {
                            *obj_ptr = v67;

                            for (col = 0; col < v3->width; col++) {
                                if (locks[col]) {
                                    sector_unlock(v3->field_20[col]);
                                }
                            }

                            return true;
                        }

                        indexes[col]--;
                    }

                    indexes[col] -= widths[col];
                }
            }
        }

        for (col = 0; col < v3->width; col++) {
            if (locks[col]) {
                sector_unlock(v3->field_20[col]);
            }
        }
    }

    if (flags != 0) {
        if ((flags & 0x01) != 0 && v69 != OBJ_HANDLE_NULL) {
            *obj_ptr = v69;
            return true;
        }

        if ((flags & 0x02) != 0 && v65 != OBJ_HANDLE_NULL) {
            *obj_ptr = v65;
            return true;
        }
    }

    return false;
}

// 0x43DFD0
void object_get_rect(int64_t obj, unsigned int flags, TigRect* rect)
{
    unsigned int obj_flags;
    int64_t loc;
    int64_t loc_x;
    int64_t loc_y;
    int64_t limit_x;
    int64_t limit_y;
    TigArtFrameData art_frame_data;
    int hot_x;
    int hot_y;
    int width;
    int height;
    int offset_x;
    int offset_y;
    int scale;
    int base_x;
    int base_y;
    unsigned int render_flags;
    int idx;
    TigRect extra_rect;

    obj_flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    if ((flags & RECT_FLAG_IGNORE_VIS) == 0 && (obj_flags & dword_5E2F88) != 0) {
        rect->x = 0;
        rect->y = 0;
        rect->width = 0;
        rect->height = 0;
        return;
    }

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    location_limits_get(&limit_x, &limit_y);

    if (LOCATION_GET_X(loc) < limit_x && LOCATION_GET_Y(loc) < limit_y) {
        location_xy(loc, &loc_x, &loc_y);

        // NOTE: Where is `loc_y` validation?
        if (loc_x < INT_MIN || loc_x > INT_MAX) {
            rect->x = 0;
            rect->y = 0;
            rect->width = 0;
            rect->height = 0;
            return;
        }
    } else {
        tig_debug_println("Error:  object_get_rect found object with location outside the limits for the current map");
        tig_debug_printf("        x: 0x%I64x  y: 0x%I64x\n        limit_x: 0x%I64x  limit_y: 0x%I64x\n",
            LOCATION_GET_X(loc),
            LOCATION_GET_Y(loc),
            limit_x,
            limit_y);

        loc_x = 0;
        loc_y = 0;
    }

    if ((int)loc_x < object_iso_content_rect_ex.x
        || (int)loc_y < object_iso_content_rect_ex.y
        || (int)loc_x >= object_iso_content_rect_ex.x + object_iso_content_rect_ex.width
        || (int)loc_y >= object_iso_content_rect_ex.y + object_iso_content_rect_ex.height) {
        rect->x = 0;
        rect->y = 0;
        rect->width = 0;
        rect->height = 0;
        return;
    }

    if ((obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ORF_08000000) != 0) {
        rect->x = (int)loc_x - obj_field_int32_get(obj, OBJ_F_RENDER_X)
            + obj_field_int32_get(obj, OBJ_F_OFFSET_X) + 40;
        rect->y = (int)loc_y - obj_field_int32_get(obj, OBJ_F_RENDER_Y)
            + obj_field_int32_get(obj, OBJ_F_OFFSET_Y) + 20;
        rect->width = obj_field_int32_get(obj, OBJ_F_RENDER_WIDTH);
        rect->height = obj_field_int32_get(obj, OBJ_F_RENDER_HEIGHT);
        tig_debug_printf("Error: object_get_rect() running invalid code\n");
        return;
    }

    if (tig_art_frame_data(obj_field_int32_get(obj, OBJ_F_CURRENT_AID), &art_frame_data) != TIG_OK) {
        rect->x = 0;
        rect->y = 0;
        rect->width = 0;
        rect->height = 0;
        return;
    }

    hot_x = art_frame_data.hot_x;
    hot_y = art_frame_data.hot_y;
    width = art_frame_data.width;
    height = art_frame_data.height;

    offset_x = obj_field_int32_get(obj, OBJ_F_OFFSET_X);
    offset_y = obj_field_int32_get(obj, OBJ_F_OFFSET_Y);
    scale = obj_field_int32_get(obj, OBJ_F_BLIT_SCALE);

    if (scale != 100) {
        hot_x = (int)((float)hot_x * (float)scale / 100.0f);
        hot_y = (int)((float)hot_y * (float)scale / 100.0f);
        width = (int)((float)width * (float)scale / 100.0f);
        height = (int)((float)height * (float)scale / 100.0f);
    }

    if ((obj_flags & OF_SHRUNK) != 0) {
        hot_x /= 2;
        hot_y /= 2;
        width /= 2;
        height /= 2;
    }

    base_x = (int)loc_x + offset_x + 40;
    base_y = (int)loc_y + offset_y + 20;

    rect->x = base_x - hot_x;
    rect->y = base_y - hot_y;
    rect->width = width;
    rect->height = height;

    if (flags != RECT_FLAG_NONE) {
        if ((flags & RECT_FLAG_OVERLAYS) != 0 && (obj_flags & OF_HAS_OVERLAYS) != 0) {
            for (int fld = OBJ_F_OVERLAY_FORE; fld <= OBJ_F_OVERLAY_BACK; fld++) {
                for (idx = 0; idx < 7; idx++) {
                    tig_art_id_t art_id = obj_arrayfield_uint32_get(obj, fld, idx);
                    if (art_id != TIG_ART_ID_INVALID
                        && tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
                        int scale_type = tig_art_eye_candy_id_scale_get(art_id);
                        int overlay_scale = scale_type != 4
                            ? scale * dword_5A548C[scale_type] / 100
                            : scale;

                        hot_x = art_frame_data.hot_x;
                        hot_y = art_frame_data.hot_y;
                        height = art_frame_data.height;
                        width = art_frame_data.width;

                        if (overlay_scale != 100) {
                            hot_x = (int)((float)hot_x * (float)overlay_scale / 100.0f);
                            hot_y = (int)((float)hot_y * (float)overlay_scale / 100.0f);
                            width = (int)((float)width * (float)overlay_scale / 100.0f);
                            height = (int)((float)height * (float)overlay_scale / 100.0f);
                        }

                        if ((obj_flags & OF_SHRUNK) != 0) {
                            hot_x /= 2;
                            hot_y /= 2;
                            width /= 2;
                            height /= 2;
                        }

                        extra_rect.x = base_x - hot_x;
                        extra_rect.y = base_y - hot_y;
                        extra_rect.width = width;
                        extra_rect.height = height;
                        tig_rect_union(rect, &extra_rect, rect);
                    }
                }
            }
        }

        if ((flags & RECT_FLAG_UNDERLAYS) != 0 && (obj_flags & OF_HAS_UNDERLAYS) != 0) {
            for (idx = 0; idx < 4; idx++) {
                tig_art_id_t art_id = obj_arrayfield_uint32_get(obj, OBJ_F_UNDERLAY, idx);
                if (art_id != TIG_ART_ID_INVALID
                    && tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
                    hot_x = art_frame_data.hot_x;
                    hot_y = art_frame_data.hot_y;
                    height = art_frame_data.height;
                    width = art_frame_data.width;

                    // NOTE: Where is scaling?

                    extra_rect.x = base_x - hot_x;
                    extra_rect.y = base_y - hot_y;
                    extra_rect.width = width;
                    extra_rect.height = height;
                    tig_rect_union(rect, &extra_rect, rect);
                }
            }
        }

        if ((flags & RECT_FLAG_SHADOWS) != 0) {
            render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
            if ((render_flags & ORF_04000000) == 0) {
                if (shadow_apply(obj)) {
                    render_flags |= ORF_10000000;
                }
                render_flags |= ORF_04000000;
                obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags);
            }

            if ((render_flags & ORF_10000000) != 0) {
                for (idx = 0; idx < SHADOW_HANDLE_MAX; idx++) {
                    Shadow* shadow = (Shadow*)obj_arrayfield_ptr_get(obj, OBJ_F_SHADOW_HANDLES, idx);
                    if (shadow == NULL) {
                        break;
                    }

                    if (tig_art_frame_data(shadow->art_id, &art_frame_data) == TIG_OK) {
                        hot_x = art_frame_data.hot_x;
                        hot_y = art_frame_data.hot_y;
                        height = art_frame_data.height;
                        width = art_frame_data.width;

                        if (scale != 100) {
                            hot_x = (int)((float)hot_x * (float)scale / 100.0f);
                            hot_y = (int)((float)hot_y * (float)scale / 100.0f);
                            width = (int)((float)width * (float)scale / 100.0f);
                            height = (int)((float)height * (float)scale / 100.0f);
                        }

                        if ((obj_flags & OF_SHRUNK) != 0) {
                            hot_x /= 2;
                            hot_y /= 2;
                            width /= 2;
                            height /= 2;
                        }

                        extra_rect.x = base_x - hot_x;
                        extra_rect.y = base_y - hot_y;
                        extra_rect.width = width;
                        extra_rect.height = height;
                        tig_rect_union(rect, &extra_rect, rect);
                    }
                }
            }
        }

        if (obj == object_hover_obj) {
            sub_4437E0(&extra_rect);
            tig_rect_union(rect, &extra_rect, rect);
        }
    }

    if ((obj_flags & OF_WADING) != 0 && (obj_flags & OF_WATER_WALKING) == 0) {
        rect->y += 15;
    }
}

// 0x43E770
void object_move_to_location(int64_t obj, int64_t loc, int offset_x, int offset_y)
{
    unsigned int flags;
    int64_t cur_loc;
    TigRect obj_rect;
    int64_t cur_sector_id;
    int64_t sector_id;
    bool is_static;
    Sector* sector;

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    if ((flags & OF_DESTROYED) != 0) {
        return;
    }

    cur_loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    if (cur_loc == loc) {
        sub_4423E0(obj, offset_x, offset_y);
        return;
    }

    if ((flags & OF_TEXT) != 0) {
        tb_notify_moved(obj, loc, offset_x, offset_y);
    }

    if ((flags & OF_TEXT_FLOATER) != 0) {
        tf_notify_moved(obj, loc, offset_x, offset_y);
    }

    object_get_rect(obj, RECT_FLAG_FULL, &obj_rect);
    object_iso_invalidate_rect(&obj_rect);

    cur_sector_id = sector_id_from_loc(cur_loc);
    sector_id = sector_id_from_loc(loc);
    is_static = object_is_static(obj);

    if (cur_sector_id == sector_id) {
        if (is_static || sub_4D04E0(sector_id)) {
            sector_lock(sector_id, &sector);
            objlist_move(&(sector->objects), obj, loc, offset_x, offset_y);
            sector_unlock(sector_id);
        } else {
            obj_field_int64_set(obj, OBJ_F_LOCATION, loc);
            obj_field_int32_set(obj, OBJ_F_OFFSET_X, offset_x);
            obj_field_int32_set(obj, OBJ_F_OFFSET_Y, offset_y);
        }
    } else {
        if (is_static || sub_4D04E0(cur_sector_id)) {
            sector_lock(cur_sector_id, &sector);
            objlist_remove(&(sector->objects), obj);
            sector_unlock(cur_sector_id);
        }

        obj_field_int64_set(obj, OBJ_F_LOCATION, loc);
        obj_field_int32_set(obj, OBJ_F_OFFSET_X, offset_x);
        obj_field_int32_set(obj, OBJ_F_OFFSET_Y, offset_y);

        if (is_static || sub_4D04E0(sector_id)) {
            sector_lock(sector_id, &sector);
            objlist_insert(&(sector->objects), obj);
            sector_unlock(sector_id);
        }
    }

    sub_4DD020(obj, loc, offset_x, offset_y);

    if (cur_sector_id == sector_id) {
        sub_444270(obj, 1);
    } else {
        sub_444270(obj, 2);
    }
}

// 0x43EA20
void sub_43EA20(int64_t a1, int64_t a2)
{
    int64_t v1;

    if (a1 != OBJ_HANDLE_NULL) {
        if (!sub_4B99C0(a2, &v1)) {
            v1 = a2;
        }
        sub_4EDF20(a1, v1, 0, 0, false);
    }
}

// 0x43EAC0
bool object_teleported_timeevent_process(TimeEvent* timeevent)
{
    if (timeevent->params[0].object_value == OBJ_HANDLE_NULL) {
        return false;
    }

    sub_444270(timeevent->params[0].object_value, 2);

    return true;
}

// 0x43EAF0
void object_set_offset(int64_t obj, int offset_x, int offset_y)
{
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        sub_4423E0(obj, offset_x, offset_y);
    }
}

// 0x43EB30
void object_set_current_aid(int64_t obj, tig_art_id_t aid)
{
    TigRect dirty_rect;
    TigRect update_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        obj_field_int32_set(obj, OBJ_F_CURRENT_AID, aid);
        obj_field_int32_set(obj,
            OBJ_F_RENDER_FLAGS,
            obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_08000000);
        object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
        tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
        object_iso_invalidate_rect(&dirty_rect);
        sub_43F710(obj);
    }
}

// 0x43EBD0
void object_set_light(int64_t obj, unsigned int flags, tig_art_id_t aid, tig_color_t color)
{
    Light* light;

    if (aid == TIG_ART_ID_INVALID) {
        light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
        if (light != NULL) {
            light_stop_animating(light);
        }
    }

    obj_field_int32_set(obj, OBJ_F_LIGHT_FLAGS, flags);
    obj_field_int32_set(obj, OBJ_F_LIGHT_AID, aid);
    obj_field_int32_set(obj, OBJ_F_LIGHT_COLOR, color);
    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_80000000);
    sub_4D9590(obj, true);
}

// 0x43ECF0
void object_overlay_set(int64_t obj, int fld, int index, tig_art_id_t aid)
{
    TigRect dirty_rect;
    TigRect updated_rect;
    unsigned int flags;
    int overlay;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return;
    }

    object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
    obj_arrayfield_uint32_set(obj, fld, index, aid);

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    flags &= ~(OF_HAS_OVERLAYS | OF_HAS_UNDERLAYS);

    for (overlay = 0; overlay < 7; overlay++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_FORE, overlay) != TIG_ART_ID_INVALID) {
            flags |= OF_HAS_OVERLAYS;
            break;
        }
    }

    if ((flags & OF_HAS_OVERLAYS) == 0) {
        for (overlay = 0; overlay < 7; overlay++) {
            if (obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_BACK, overlay) != TIG_ART_ID_INVALID) {
                flags |= OF_HAS_OVERLAYS;
                break;
            }
        }
    }

    for (overlay = 0; overlay < 4; overlay++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_UNDERLAY, overlay) != TIG_ART_ID_INVALID) {
            flags |= OF_HAS_UNDERLAYS;
            break;
        }
    }

    obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_08000000);

    object_get_rect(obj, RECT_FLAG_FULL, &updated_rect);
    tig_rect_union(&dirty_rect, &updated_rect, &dirty_rect);
    object_iso_invalidate_rect(&dirty_rect);
}

// 0x43EE10
void object_set_overlay_light(int64_t obj, int index, unsigned int flags, tig_art_id_t aid, tig_color_t color)
{
    Light* light;

    if (aid == TIG_ART_ID_INVALID) {
        light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index);
        if (light != NULL) {
            light_stop_animating(light);
        }
    }

    obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_FLAGS, index, flags);
    obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_AID, index, aid);
    obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_COLOR, index, color);
    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_80000000);
    sub_4D9590(obj, true);
}

// 0x43EEB0
void object_set_blit_scale(int64_t obj, int value)
{
    TigRect dirty_rect;
    TigRect update_rect;

    object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
    obj_field_int32_set(obj, OBJ_F_BLIT_SCALE, value);
    object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
    tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
    object_iso_invalidate_rect(&dirty_rect);
}

// 0x43EF10
void object_add_flags(int64_t obj, unsigned int flags)
{
    TigRect dirty_rect;
    TigRect update_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        object_flags_set(obj, flags);
        object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
        tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
        object_iso_invalidate_rect(&dirty_rect);
        obj_field_int32_set(obj,
            OBJ_F_RENDER_FLAGS,
            obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~(ORF_04000000 | ORF_01000000));
    }
}

// 0x43EFA0
void object_remove_flags(int64_t obj, unsigned int flags)
{
    TigRect dirty_rect;
    TigRect update_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        object_flags_unset(obj, flags);
        object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
        tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
        object_iso_invalidate_rect(&dirty_rect);
        obj_field_int32_set(obj,
            OBJ_F_RENDER_FLAGS,
            obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~(ORF_04000000 | ORF_01000000));
    }
}

// 0x43F030
void sub_43F030(int64_t obj)
{
    TigRect rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &rect);
        object_iso_invalidate_rect(&rect);
        obj_field_int32_set(obj,
            OBJ_F_RENDER_FLAGS,
            obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~(ORF_04000000 | ORF_02000000 | ORF_01000000));
    }
}

// 0x43F090
void object_cycle_palette(int64_t obj)
{
    tig_art_id_t aid;
    TigRect rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        aid = tig_art_id_palette_set(aid, tig_art_id_palette_get(aid) + 1);
        if (sub_502E00(aid) == TIG_OK) {
            aid = tig_art_id_palette_set(aid, 0);
        }
        obj_field_int32_set(obj, OBJ_F_CURRENT_AID, aid);
        object_get_rect(obj, RECT_FLAG_NONE, &rect);
        object_iso_invalidate_rect(&rect);
        obj_field_int32_set(obj,
            OBJ_F_RENDER_FLAGS,
            obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~(ORF_02000000 | ORF_01000000));
    }
}

// 0x43F130
void sub_43F130(int64_t obj, int palette)
{
    tig_art_id_t aid;
    TigRect rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        aid = tig_art_id_palette_set(aid, palette);
        // NOTE: Looks wrong.
        if (sub_502E00(aid) != TIG_OK) {
            obj_field_int32_set(obj, OBJ_F_CURRENT_AID, aid);
            object_get_rect(obj, RECT_FLAG_NONE, &rect);
            object_iso_invalidate_rect(&rect);
            obj_field_int32_set(obj,
                OBJ_F_RENDER_FLAGS,
                obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_02000000);
        }
    }
}

// 0x43F1C0
void object_bust(int64_t obj, int64_t triggerer_obj)
{
    ObjectList objects;
    DateTime datetime;
    TimeEvent timeevent;
    int64_t loc;
    unsigned int scenery_flags;
    unsigned int trap_flags;
    tig_art_id_t destroyed_art_id;
    int sound_id;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return;
    }

    sub_424070(obj, 5, 0, 1);

    switch (obj_field_int32_get(obj, OBJ_F_TYPE)) {
    case OBJ_TYPE_WALL:
        sub_4E1490(obj, triggerer_obj);
        break;
    case OBJ_TYPE_PORTAL:
        portal_bust(obj, triggerer_obj);
        break;
    case OBJ_TYPE_CONTAINER:
        if (sub_49B290(obj) == BP_JUNK_PILE) {
            object_hp_damage_set(obj, 0);
        } else {
            sub_463730(obj, true);

            loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
            object_destroy(obj);
            object_list_location(loc, 32740, &objects);
            if (objects.head != NULL) {
                magictech_fx_add(objects.head->obj, MAGICTECH_FX_CHEST_BREAK);
            }
            object_list_destroy(&objects);
        }
        break;
    case OBJ_TYPE_SCENERY:
        scenery_flags = obj_field_int32_get(obj, OBJ_F_SCENERY_FLAGS);
        if ((scenery_flags & OSCF_RESPAWNABLE) != 0) {
            int scenery_respawn_delay;

            object_script_execute(triggerer_obj, obj, OBJ_HANDLE_NULL, SAP_BUST, 0);
            scenery_flags |= OSCF_RESPAWNING;
            obj_field_int32_set(obj, OBJ_F_SCENERY_FLAGS, scenery_flags);
            object_flags_set(obj, OF_OFF);

            object_hp_damage_set(obj, 0);

            // Setup respawn time event.
            timeevent.type = TIMEEVENT_TYPE_SCENERY_RESPAWN;
            timeevent.params[0].object_value = obj;

            scenery_respawn_delay = obj_field_int32_get(obj, OBJ_F_SCENERY_RESPAWN_DELAY);
            if (scenery_respawn_delay == 0) {
                sub_45A950(&datetime, 0);
                datetime.days = 1;
            } else if (scenery_respawn_delay > 0) {
                sub_45A950(&datetime, 8000 * scenery_respawn_delay);
            } else {
                sub_45A950(&datetime, -60000 * scenery_respawn_delay);
            }

            timeevent_add_delay(&timeevent, &datetime);
        } else {
            destroyed_art_id = obj_field_int32_get(obj, OBJ_F_DESTROYED_AID);
            if ((scenery_flags & OSCF_BUSTED) != 0) {
                object_destroy(obj);
                sound_id = sfx_misc_sound(obj, MISC_SOUND_DESTROYING);
            } else {
                if (destroyed_art_id != TIG_ART_ID_INVALID) {
                    object_set_current_aid(obj, destroyed_art_id);
                }

                object_hp_damage_set(obj, object_hp_max(obj));
                obj_field_int32_set(obj, OBJ_F_SCENERY_FLAGS, scenery_flags | OSCF_BUSTED);
                sound_id = sfx_misc_sound(obj, MISC_SOUND_BUSTING);
                object_flags_set(obj, OF_INVULNERABLE);
                object_script_execute(triggerer_obj, obj, OBJ_HANDLE_NULL, SAP_BUST, 0);
            }

            gsound_play_sfx_on_obj(sound_id, 1, obj);
        }
        break;
    case OBJ_TYPE_PROJECTILE:
        // NOOP
        return;
    case OBJ_TYPE_WEAPON:
    case OBJ_TYPE_AMMO:
    case OBJ_TYPE_ARMOR:
    case OBJ_TYPE_GOLD:
    case OBJ_TYPE_FOOD:
    case OBJ_TYPE_SCROLL:
    case OBJ_TYPE_KEY:
    case OBJ_TYPE_KEY_RING:
    case OBJ_TYPE_WRITTEN:
    case OBJ_TYPE_GENERIC:
        if (tig_art_item_id_destroyed_get(obj_field_int32_get(obj, OBJ_F_CURRENT_AID)) == 0) {
            destroyed_art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
            destroyed_art_id = tig_art_item_id_destroyed_set(destroyed_art_id, 1);
            object_set_current_aid(obj, destroyed_art_id);

            destroyed_art_id = obj_field_int32_get(obj, OBJ_F_ITEM_INV_AID);
            destroyed_art_id = tig_art_item_id_destroyed_set(destroyed_art_id, 1);
            obj_field_int32_set(obj, OBJ_F_ITEM_INV_AID, destroyed_art_id);

            object_hp_damage_set(obj, 0);

            if (item_parent(obj, NULL) && !sub_464C80(obj)) {
                item_drop(obj);
            }

            sound_id = sfx_misc_sound(obj, MISC_SOUND_BUSTING);
            object_script_execute(triggerer_obj, obj, OBJ_HANDLE_NULL, SAP_BUST, 0);
        } else {
            object_destroy(obj);
            sound_id = sfx_misc_sound(obj, MISC_SOUND_DESTROYING);
        }
        gsound_play_sfx_on_obj(sound_id, 1, obj);
        break;
    case OBJ_TYPE_PC:
    case OBJ_TYPE_NPC:
        // NOOP
        return;
    case OBJ_TYPE_TRAP:
        destroyed_art_id = obj_field_int32_get(obj, OBJ_F_DESTROYED_AID);
        if (destroyed_art_id != TIG_ART_ID_INVALID) {
            trap_flags = obj_field_int32_get(obj, OBJ_F_TRAP_FLAGS);
            if ((trap_flags & 0x2) == 0) {
                object_set_current_aid(obj, destroyed_art_id);
                object_hp_damage_set(obj, 0);
                obj_field_int32_set(obj, OBJ_F_TRAP_FLAGS, trap_flags | 0x2);
                sound_id = sfx_misc_sound(obj, MISC_SOUND_BUSTING);
                object_script_execute(triggerer_obj, obj, OBJ_HANDLE_NULL, SAP_BUST, 0);
            } else {
                object_destroy(obj);
                sound_id = sfx_misc_sound(obj, MISC_SOUND_DESTROYING);
            }
        } else {
            object_destroy(obj);
            sound_id = sfx_misc_sound(obj, MISC_SOUND_DESTROYING);
        }
        gsound_play_sfx_on_obj(sound_id, 1, obj);
        break;
    }

    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~(ORF_08000000 | ORF_02000000 | ORF_01000000));
}

// 0x43F5F0
bool object_scenery_respawn_timeevent_process(TimeEvent* timeevent)
{
    int64_t obj;

    obj = timeevent->params[0].object_value;
    obj_field_int32_set(obj,
        OBJ_F_SCENERY_FLAGS,
        obj_field_int32_get(obj, OBJ_F_SCENERY_FLAGS) & ~OSCF_RESPAWNING);
    object_flags_unset(obj, OF_OFF);

    return true;
}

// 0x43F630
bool object_is_busted(int64_t obj)
{
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return false;
    }

    switch (obj_field_int32_get(obj, OBJ_F_TYPE)) {
    case OBJ_TYPE_PORTAL:
        return (obj_field_int32_get(obj, OBJ_F_PORTAL_FLAGS) & OPF_BUSTED) != 0;
    case OBJ_TYPE_CONTAINER:
        return (obj_field_int32_get(obj, OBJ_F_CONTAINER_FLAGS) & OCOF_BUSTED) != 0;
    case OBJ_TYPE_SCENERY:
        return (obj_field_int32_get(obj, OBJ_F_SCENERY_FLAGS) & OSCF_BUSTED) != 0;
    case OBJ_TYPE_WEAPON:
    case OBJ_TYPE_AMMO:
    case OBJ_TYPE_ARMOR:
    case OBJ_TYPE_GOLD:
    case OBJ_TYPE_FOOD:
    case OBJ_TYPE_SCROLL:
    case OBJ_TYPE_KEY:
    case OBJ_TYPE_KEY_RING:
    case OBJ_TYPE_WRITTEN:
    case OBJ_TYPE_GENERIC:
        return tig_art_item_id_destroyed_get(obj_field_int32_get(obj, OBJ_F_CURRENT_AID)) != 0;
    case OBJ_TYPE_TRAP:
        return (obj_field_int32_get(obj, OBJ_F_TRAP_FLAGS) & OTF_BUSTED) != 0;
    }

    return false;
}

// 0x43F710
void sub_43F710(int64_t obj)
{
    TigArtAnimData art_anim_data;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & (OF_DESTROYED | OF_OFF)) != 0
        || obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_SCENERY) {
        return;
    }

    if ((obj_field_int32_get(obj, OBJ_F_SCENERY_FLAGS) & OSCF_NO_AUTO_ANIMATE) != 0) {
        if (!sub_44CB60()) {
            sub_423FF0(obj);
        }
    } else {
        if (tig_art_anim_data(obj_field_int32_get(obj, OBJ_F_CURRENT_AID), &art_anim_data) == TIG_OK) {
            if (art_anim_data.num_frames <= 1 && sfx_misc_sound(obj, MISC_SOUND_ANIMATING) == -1) {
                if (!sub_44CB60()) {
                    sub_423FF0(obj);
                }
            } else {
                anim_goal_animate_loop(obj);
            }
        }
    }
}

// 0x43F7F0
void object_inc_current_aid(int64_t obj)
{
    tig_art_id_t current_aid;
    tig_art_id_t next_aid;
    TigArtFrameData art_frame_data;
    TigRect dirty_rect;
    TigRect update_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        current_aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        next_aid = tig_art_id_frame_inc(current_aid);
        if (current_aid != next_aid) {
            obj_field_int32_set(obj, OBJ_F_CURRENT_AID, next_aid);
            if (tig_art_frame_data(next_aid, &art_frame_data) == TIG_OK) {
                if (art_frame_data.offset_x != 0 || art_frame_data.offset_y != 0) {
                    sub_4423E0(obj,
                        art_frame_data.offset_x + obj_field_int32_get(obj, OBJ_F_OFFSET_X),
                        art_frame_data.offset_y + obj_field_int32_get(obj, OBJ_F_OFFSET_Y));
                }
            }
            obj_field_int32_set(obj,
                OBJ_F_RENDER_FLAGS,
                obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_08000000);
            object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
            tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
            object_iso_invalidate_rect(&dirty_rect);
        }
    }
}

// 0x43F8F0
void object_dec_current_aid(int64_t obj)
{
    tig_art_id_t current_aid;
    tig_art_id_t prev_aid;
    TigArtFrameData art_frame_data;
    TigRect dirty_rect;
    TigRect update_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        current_aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        prev_aid = tig_art_id_frame_dec(current_aid);
        if (current_aid != prev_aid) {
            obj_field_int32_set(obj, OBJ_F_CURRENT_AID, prev_aid);
            if (tig_art_frame_data(prev_aid, &art_frame_data) == TIG_OK) {
                if (art_frame_data.offset_x != 0 || art_frame_data.offset_y != 0) {
                    sub_4423E0(obj,
                        art_frame_data.offset_x + obj_field_int32_get(obj, OBJ_F_OFFSET_X),
                        art_frame_data.offset_y + obj_field_int32_get(obj, OBJ_F_OFFSET_Y));
                }
            }
            obj_field_int32_set(obj,
                OBJ_F_RENDER_FLAGS,
                obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_08000000);
            object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
            tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
            object_iso_invalidate_rect(&dirty_rect);
        }
    }
}

// 0x43F9F0
void object_eye_candy_aid_inc(int64_t obj, int fld, int index)
{
    tig_art_id_t current_aid;
    tig_art_id_t next_aid;
    TigRect dirty_rect;
    TigRect update_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        current_aid = obj_arrayfield_uint32_get(obj, fld, index);
        next_aid = tig_art_id_frame_inc(current_aid);
        if (current_aid != next_aid) {
            obj_arrayfield_uint32_set(obj, fld, index, next_aid);
            obj_field_int32_set(obj,
                OBJ_F_RENDER_FLAGS,
                obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_08000000);
            object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
            tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
            object_iso_invalidate_rect(&dirty_rect);
        }
    }
}

// 0x43FAB0
void object_eye_candy_aid_dec(int64_t obj, int fld, int index)
{
    tig_art_id_t current_aid;
    tig_art_id_t prev_aid;
    TigRect dirty_rect;
    TigRect update_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        current_aid = obj_arrayfield_uint32_get(obj, fld, index);
        prev_aid = tig_art_id_frame_get(current_aid) > 0
            ? tig_art_id_frame_dec(current_aid)
            : current_aid;
        if (current_aid != prev_aid) {
            obj_arrayfield_uint32_set(obj, fld, index, prev_aid);
            obj_field_int32_set(obj,
                OBJ_F_RENDER_FLAGS,
                obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_08000000);
            object_get_rect(obj, RECT_FLAG_FULL, &update_rect);
            tig_rect_union(&dirty_rect, &update_rect, &dirty_rect);
            object_iso_invalidate_rect(&dirty_rect);
        }
    }
}

// 0x43FB80
void object_overlay_light_frame_set_first(int64_t obj, int index)
{
    tig_art_id_t aid;

    aid = obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_AID, index);
    if (tig_art_id_frame_get(aid) == 0) {
        return;
    }

    aid = tig_art_id_frame_set(aid, 0);
    obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_AID, index, aid);
    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_80000000);
    sub_4D9590(obj, true);
}

// 0x43FBF0
void object_overlay_light_frame_set_last(int64_t obj, int index)
{
    tig_art_id_t aid;
    TigArtAnimData art_anim_data;

    aid = obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_AID, index);
    if (tig_art_anim_data(aid, &art_anim_data) != TIG_OK) {
        return;
    }

    if (tig_art_id_frame_get(aid) == art_anim_data.num_frames - 1) {
        return;
    }

    aid = tig_art_id_frame_set(aid, art_anim_data.num_frames - 1);
    obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_AID, index, aid);
    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_80000000);
    sub_4D9590(obj, true);
}

// 0x43FC80
void object_overlay_light_frame_inc(int64_t obj, int index)
{
    Light* light;

    light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index);
    if (light != NULL) {
        light_inc_frame(light);
    }
}

// 0x43FCB0
void object_overlay_light_frame_dec(int64_t obj, int index)
{
    Light* light;

    light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index);
    if (light != NULL) {
        light_dec_frame(light);
    }
}

// 0x43FCE0
void object_cycle_rotation(int64_t obj)
{
    tig_art_id_t current_aid;
    tig_art_id_t next_aid;
    TigRect dirty_rect;

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
        object_get_rect(obj, RECT_FLAG_FULL, &dirty_rect);
        current_aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        next_aid = tig_art_id_rotation_inc(current_aid);
        if (current_aid != next_aid) {
            object_iso_invalidate_rect(&dirty_rect);
            obj_field_int32_set(obj, OBJ_F_CURRENT_AID, next_aid);
            obj_field_int32_set(obj,
                OBJ_F_RENDER_FLAGS,
                obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~ORF_08000000);
            sub_4423E0(obj, 0, 0);
        }
    }
}

// 0x43FD70
bool sub_43FD70(int64_t a1, int64_t a2, int rotation, unsigned int flags, int* a5)
{
    int64_t block_obj;
    int block_obj_type;

    return sub_43FDC0(a1, a2, rotation, flags, &block_obj, &block_obj_type, a5) != 0
        || block_obj != OBJ_HANDLE_NULL;
}

// 0x43FDC0
int sub_43FDC0(int64_t a1, int64_t a2, int rotation, unsigned int flags, int64_t* block_obj_ptr, int* block_obj_type_ptr, int* a7)
{
    return sub_43FE00(a1, a2, rotation, rotation, flags, block_obj_ptr, block_obj_type_ptr, a7);
}

// 0x43FE00
int sub_43FE00(int64_t a1, int64_t a2, int a3, int a4, unsigned int flags, int64_t* block_obj_ptr, int* block_obj_type_ptr, int* a8)
{
    bool done = false;
    int cost = 0;
    int64_t tmp_loc;
    ObjectList objects;
    ObjectNode* node;
    int obj_type;
    tig_art_id_t art_id;
    int art_rot;
    int p_piece;
    unsigned int obj_flags;

    *block_obj_ptr = OBJ_HANDLE_NULL;

    if (a8 != NULL) {
        *a8 = 0;
    }

    if ((a3 & 1) == 0) {
        int ccw_rot = (a3 + 7) % 8;
        int cw_rot = a3 + 1;

        cost += sub_43FE00(a1, a2, ccw_rot, a4, flags, block_obj_ptr, block_obj_type_ptr, a8);
        if (*block_obj_ptr != OBJ_HANDLE_NULL) {
            return cost;
        }

        if (!location_in_dir(a2, ccw_rot, &tmp_loc)) {
            return 1;
        }

        cost += sub_43FE00(a1, tmp_loc, cw_rot, a4, flags, block_obj_ptr, block_obj_type_ptr, a8);
        if (*block_obj_ptr != OBJ_HANDLE_NULL) {
            return cost;
        }

        cost += sub_43FE00(a1, a2, cw_rot, a4, flags, block_obj_ptr, block_obj_type_ptr, a8);
        if (*block_obj_ptr != OBJ_HANDLE_NULL) {
            return cost;
        }

        if (!location_in_dir(a2, cw_rot, &tmp_loc)) {
            return 1;
        }

        cost += sub_43FE00(a1, tmp_loc, ccw_rot, a4, flags, block_obj_ptr, block_obj_type_ptr, a8);

        return cost;
    }

    object_list_location(a2, OBJ_TM_WALL | OBJ_TM_PORTAL, &objects);
    node = objects.head;
    while (node != NULL) {
        bool v1 = false;

        obj_type = obj_field_int32_get(node->obj, OBJ_F_TYPE);
        art_id = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
        art_rot = tig_art_id_rotation_get(art_id);
        if ((art_rot & 1) == 0) {
            art_rot++;
        }

        if (art_rot == a3) {
            if (obj_type == OBJ_TYPE_WALL) {
                if ((flags & 0x20) != 0) {
                    cost += 2;
                } else {
                    p_piece = tig_art_wall_id_p_piece_get(art_id);
                    if (p_piece == 10
                        || p_piece == 13
                        || p_piece == 14
                        || p_piece == 17
                        || p_piece == 18
                        || p_piece == 19
                        || p_piece == 22
                        || p_piece == 25
                        || p_piece == 26
                        || p_piece == 29
                        || p_piece == 30
                        || p_piece == 31
                        || p_piece == 32) {
                        if ((flags & 0x02) != 0 || (a4 & 1) == 0) {
                            if (p_piece == 10
                                || p_piece == 13
                                || p_piece == 14
                                || p_piece == 17
                                || p_piece == 18
                                || p_piece == 19) {
                                done = true;
                                *block_obj_ptr = node->obj;
                                *block_obj_type_ptr = OBJ_TYPE_WALL;
                                break;
                            }
                        } else {
                            if (a8 != NULL
                                && (p_piece == 10
                                    || p_piece == 13
                                    || p_piece == 14
                                    || p_piece == 17
                                    || p_piece == 18
                                    || p_piece == 19)) {
                                *a8 = 1;
                            }
                        }
                    } else {
                        if ((obj_field_int32_get(node->obj, OBJ_F_SPELL_FLAGS) & OSF_PASSWALLED) == 0) {
                            done = true;
                            *block_obj_ptr = node->obj;
                            *block_obj_type_ptr = OBJ_TYPE_WALL;
                            break;
                        }

                        v1 = true;
                    }
                }
            } else {
                if (tig_art_id_damaged_get(art_id) != 512
                    && !portal_is_open(node->obj)) {
                    if ((flags & 0x20) != 0) {
                        cost += 1;
                    } else {
                        v1 = true;
                        if ((flags & 0x01) != 0
                            || ((flags & 0x08) == 0
                                && a1 != OBJ_HANDLE_NULL
                                && ai_attempt_open_portal(a1, node->obj, a3) != AI_ATTEMPT_OPEN_PORTAL_OK)) {
                            done = true;
                            *block_obj_ptr = node->obj;
                            *block_obj_type_ptr = OBJ_TYPE_PORTAL;
                            break;
                        }
                    }
                }
            }
        }

        if ((flags & 0x08) != 0 && v1) {
            obj_flags = obj_field_int32_get(node->obj, OBJ_F_FLAGS);

            if ((obj_flags & OF_SHOOT_THROUGH) != 0) {
                done = true;
                *block_obj_ptr = node->obj;
                *block_obj_type_ptr = obj_type;
                break;
            }

            if ((obj_flags & OF_SEE_THROUGH) != 0) {
                if ((obj_flags & OF_PROVIDES_COVER) != 0) {
                    cost += 20;
                }
            } else {
                cost += 50;
            }
        }

        node = node->next;
    }
    object_list_destroy(&objects);

    if (done) {
        return cost;
    }

    if (!location_in_dir(a2, a3, &tmp_loc)) {
        return 1;
    }

    if ((flags & 0x20) != 0) {
        if (tile_is_soundproof(tmp_loc)) {
            cost += 8;
        }
    }

    object_list_location(tmp_loc, 0x3801F, &objects);
    node = objects.head;
    while (node != NULL) {
        bool v2 = false;

        obj_type = obj_field_int32_get(node->obj, OBJ_F_TYPE);
        if (obj_type == OBJ_TYPE_WALL || obj_type == OBJ_TYPE_PORTAL) {
            art_id = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
            art_rot = tig_art_id_rotation_get(art_id);
            if ((art_rot & 1) == 0) {
                art_rot++;
            }

            if ((art_rot + 4) % 8 == a3) {
                if (obj_type == OBJ_TYPE_WALL) {
                    if ((flags & 0x20) != 0) {
                        cost += 2;
                    } else {
                        p_piece = tig_art_wall_id_p_piece_get(art_id);
                        if (p_piece == 10
                            || p_piece == 13
                            || p_piece == 14
                            || p_piece == 17
                            || p_piece == 18
                            || p_piece == 19
                            || p_piece == 22
                            || p_piece == 25
                            || p_piece == 26
                            || p_piece == 29
                            || p_piece == 30
                            || p_piece == 31
                            || p_piece == 32) {
                            if ((flags & 0x02) != 0
                                || (a4 & 1) == 0) {
                                if (p_piece == 10
                                    || p_piece == 13
                                    || p_piece == 14
                                    || p_piece == 17
                                    || p_piece == 18
                                    || p_piece == 19) {
                                    *block_obj_ptr = node->obj;
                                    *block_obj_type_ptr = obj_type;
                                    break;
                                }
                            } else {
                                if (a8 != NULL
                                    && (p_piece == 10
                                        || p_piece == 13
                                        || p_piece == 14
                                        || p_piece == 17
                                        || p_piece == 18
                                        || p_piece == 19)) {
                                    *a8 = 1;
                                }
                            }
                        } else {
                            if ((obj_field_int32_get(node->obj, OBJ_F_SPELL_FLAGS) & OSF_PASSWALLED) == 0) {
                                *block_obj_ptr = node->obj;
                                *block_obj_type_ptr = obj_type;
                                break;
                            }

                            v2 = true;
                        }
                    }
                } else {
                    if (tig_art_id_damaged_get(art_id) != 512
                        && !portal_is_open(node->obj)) {
                        if ((flags & 0x20) != 0) {
                            cost += 1;
                        } else {
                            v2 = 1;
                            if ((flags & 0x01) != 0
                                || ((flags & 0x08) == 0
                                    && a1 != OBJ_HANDLE_NULL
                                    && ai_attempt_open_portal(a1, node->obj, a3) != AI_ATTEMPT_OPEN_PORTAL_OK)) {
                                *block_obj_ptr = node->obj;
                                *block_obj_type_ptr = obj_type;
                                break;
                            }
                        }
                    }
                }
            }
        } else if ((flags & 0x30) == 0) {
            if (obj_type_is_critter(obj_type)) {
                v2 = true;
                if ((flags & 0x04) == 0
                    && !critter_is_dead(node->obj)) {
                    *block_obj_ptr = node->obj;
                    *block_obj_type_ptr = obj_type;
                    break;
                }
            } else {
                v2 = true;
                obj_flags = obj_field_int32_get(node->obj, OBJ_F_FLAGS);
                if ((obj_flags & OF_NO_BLOCK) == 0
                    && ((flags & 0x08) == 0
                        || (obj_flags & OF_SHOOT_THROUGH) == 0)) {
                    *block_obj_ptr = node->obj;
                    *block_obj_type_ptr = obj_type;
                    break;
                }
            }
        }

        // 0x44063F
        if ((flags & 0x08) != 0
            && v2
            && (!obj_type_is_critter(obj_type) || !critter_is_dead(node->obj))) {
            obj_flags = obj_field_int32_get(node->obj, OBJ_F_FLAGS);
            if ((obj_flags & OF_SHOOT_THROUGH) == 0) {
                *block_obj_ptr = node->obj;
                *block_obj_type_ptr = obj_type;
                break;
            }

            if ((obj_flags & OF_SEE_THROUGH) != 0) {
                if ((obj_flags & OF_PROVIDES_COVER) != 0) {
                    cost += 20;
                }
            } else {
                cost += 50;
            }
        }

        node = node->next;
    }
    object_list_destroy(&objects);

    return cost;
}

// 0x440700
bool sub_440700(int64_t obj, int64_t loc, int rot, unsigned int flags, int64_t* block_obj_ptr)
{
    int block_obj_type;

    sub_43FE00(obj, loc, rot, rot, flags | 0x4 | 0x1, block_obj_ptr, &block_obj_type, NULL);

    if (*block_obj_ptr == OBJ_HANDLE_NULL) {
        return false;
    }

    if (block_obj_type != OBJ_TYPE_PORTAL) {
        return false;
    }

    return true;
}

// 0x4407C0
void object_list_location(int64_t loc, unsigned int flags, ObjectList* objects)
{
    bool types[18];
    int64_t sector_id;
    int64_t limit_x;
    int64_t limit_y;
    Sector* sector;
    ObjectNode* new_node;
    ObjectNode** parent_ptr;

    memset(types, 0, sizeof(types));
    if ((flags & OBJ_TM_WALL) != 0) types[OBJ_TYPE_WALL] = true;
    if ((flags & OBJ_TM_PORTAL) != 0) types[OBJ_TYPE_PORTAL] = true;
    if ((flags & OBJ_TM_CONTAINER) != 0) types[OBJ_TYPE_CONTAINER] = true;
    if ((flags & OBJ_TM_SCENERY) != 0) types[OBJ_TYPE_SCENERY] = true;
    if ((flags & OBJ_TM_PROJECTILE) != 0) types[OBJ_TYPE_PROJECTILE] = true;
    if ((flags & OBJ_TM_WEAPON) != 0) types[OBJ_TYPE_WEAPON] = true;
    if ((flags & OBJ_TM_AMMO) != 0) types[OBJ_TYPE_AMMO] = true;
    if ((flags & OBJ_TM_ARMOR) != 0) types[OBJ_TYPE_ARMOR] = true;
    if ((flags & OBJ_TM_GOLD) != 0) types[OBJ_TYPE_GOLD] = true;
    if ((flags & OBJ_TM_FOOD) != 0) types[OBJ_TYPE_FOOD] = true;
    if ((flags & OBJ_TM_SCROLL) != 0) types[OBJ_TYPE_SCROLL] = true;
    if ((flags & OBJ_TM_KEY) != 0) types[OBJ_TYPE_KEY] = true;
    if ((flags & OBJ_TM_KEY_RING) != 0) types[OBJ_TYPE_KEY_RING] = true;
    if ((flags & OBJ_TM_WRITTEN) != 0) types[OBJ_TYPE_WRITTEN] = true;
    if ((flags & OBJ_TM_GENERIC) != 0) types[OBJ_TYPE_GENERIC] = true;
    if ((flags & OBJ_TM_PC) != 0) types[OBJ_TYPE_PC] = true;
    if ((flags & OBJ_TM_NPC) != 0) types[OBJ_TYPE_NPC] = true;
    if ((flags & OBJ_TM_TRAP) != 0) types[OBJ_TYPE_TRAP] = true;

    objects->num_sectors = 0;
    objects->head = NULL;
    parent_ptr = &(objects->head);

    sector_id = sector_id_from_loc(loc);
    sector_limits_get(&limit_x, &limit_y);

    if (SECTOR_X(sector_id) < 0
        || SECTOR_X(sector_id) >= limit_x
        || SECTOR_Y(sector_id) < 0
        || SECTOR_Y(sector_id) >= limit_y) {
        tig_debug_println("Warning: trying to build a list of objects from\n  a sector that is out of range for this map.");
        return;
    }

    if ((flags & (OBJ_TM_TRAP | OBJ_TM_SCENERY | OBJ_TM_PORTAL | OBJ_TM_WALL)) != 0
        || sub_4D04E0(sector_id)) {
        if (sector_lock(sector_id, &sector)) {
            ObjectNode* node;

            objects->sectors[objects->num_sectors++] = sector_id;

            node = sector->objects.heads[tile_id_from_loc(loc)];
            while (node != NULL) {
                if ((dword_5E2F88 & obj_field_int32_get(node->obj, OBJ_F_FLAGS)) == 0
                    && types[obj_field_int32_get(node->obj, OBJ_F_TYPE)]) {
                    new_node = object_node_create();
                    new_node->obj = node->obj;
                    new_node->next = NULL;

                    *parent_ptr = new_node;
                    parent_ptr = &((*parent_ptr)->next);
                }
                node = node->next;
            }
        }
    } else {
        int64_t obj;
        FindNode* iter;

        if (obj_find_walk_first(sector_id, &obj, &iter)) {
            do {
                if (!sub_43D940(obj)
                    && (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_INVENTORY) == 0
                    && obj_field_int64_get(obj, OBJ_F_LOCATION) == loc
                    && (dword_5E2F88 & obj_field_int32_get(obj, OBJ_F_FLAGS)) == 0
                    && types[obj_field_int32_get(obj, OBJ_F_TYPE)]) {
                    new_node = object_node_create();
                    new_node->obj = obj;
                    new_node->next = NULL;

                    *parent_ptr = new_node;
                    parent_ptr = &((*parent_ptr)->next);
                }
            } while (obj_find_walk_next(&obj, &iter));
        }
    }

    dword_5E2F98++;
    return;
}

// 0x440B40
void object_list_rect(LocRect* loc_rect, unsigned int flags, ObjectList* objects)
{
    bool types[18];
    SomeSectorStuff v1;
    SomeSectorStuffEntry* v2;
    int col;
    int row;
    int64_t obj;
    FindNode* iter;
    int64_t loc;
    ObjectNode** prev_ptr;
    ObjectNode* new_node;
    ObjectNode* obj_node;
    int indexes[3];
    int widths[3];
    bool locks[3];
    Sector* sectors[3];
    int v3;
    int v4;

    memset(types, 0, sizeof(types));
    if ((flags & OBJ_TM_WALL) != 0) types[OBJ_TYPE_WALL] = true;
    if ((flags & OBJ_TM_PORTAL) != 0) types[OBJ_TYPE_PORTAL] = true;
    if ((flags & OBJ_TM_CONTAINER) != 0) types[OBJ_TYPE_CONTAINER] = true;
    if ((flags & OBJ_TM_SCENERY) != 0) types[OBJ_TYPE_SCENERY] = true;
    if ((flags & OBJ_TM_PROJECTILE) != 0) types[OBJ_TYPE_PROJECTILE] = true;
    if ((flags & OBJ_TM_WEAPON) != 0) types[OBJ_TYPE_WEAPON] = true;
    if ((flags & OBJ_TM_AMMO) != 0) types[OBJ_TYPE_AMMO] = true;
    if ((flags & OBJ_TM_ARMOR) != 0) types[OBJ_TYPE_ARMOR] = true;
    if ((flags & OBJ_TM_GOLD) != 0) types[OBJ_TYPE_GOLD] = true;
    if ((flags & OBJ_TM_FOOD) != 0) types[OBJ_TYPE_FOOD] = true;
    if ((flags & OBJ_TM_SCROLL) != 0) types[OBJ_TYPE_SCROLL] = true;
    if ((flags & OBJ_TM_KEY) != 0) types[OBJ_TYPE_KEY] = true;
    if ((flags & OBJ_TM_KEY_RING) != 0) types[OBJ_TYPE_KEY_RING] = true;
    if ((flags & OBJ_TM_WRITTEN) != 0) types[OBJ_TYPE_WRITTEN] = true;
    if ((flags & OBJ_TM_GENERIC) != 0) types[OBJ_TYPE_GENERIC] = true;
    if ((flags & OBJ_TM_PC) != 0) types[OBJ_TYPE_PC] = true;
    if ((flags & OBJ_TM_NPC) != 0) types[OBJ_TYPE_NPC] = true;
    if ((flags & OBJ_TM_TRAP) != 0) types[OBJ_TYPE_TRAP] = true;

    objects->num_sectors = 0;
    objects->head = NULL;

    prev_ptr = &(objects->head);

    if (!sub_4D0090(loc_rect, &v1)) {
        return;
    }

    if ((flags & (OBJ_TM_TRAP | OBJ_TM_SCENERY | OBJ_TM_PORTAL | OBJ_TM_WALL)) == 0) {
        for (col = 0; col < v1.height; col++) {
            v2 = &(v1.field_8[col]);

            for (row = 0; row < v2->width; row++) {
                if (obj_find_walk_first(v2->field_20[row], &obj, &iter)) {
                    do {
                        if (!sub_43D940(obj)
                            && (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_INVENTORY) == 0
                            && (dword_5E2F88 & obj_field_int32_get(obj, OBJ_F_FLAGS)) == 0
                            && types[obj_field_int32_get(obj, OBJ_F_TYPE)]) {
                            loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
                            if (LOCATION_GET_X(loc) >= loc_rect->x1
                                && LOCATION_GET_X(loc) <= loc_rect->x2
                                && LOCATION_GET_Y(loc) >= loc_rect->y1
                                && LOCATION_GET_Y(loc) <= loc_rect->y2) {
                                new_node = object_node_create();
                                new_node->obj = obj;
                                new_node->next = NULL;
                                *prev_ptr = new_node;
                                prev_ptr = &(new_node->next);
                            }
                        }
                    } while (obj_find_walk_next(&obj, &iter));
                }
            }
        }
    } else {
        for (col = 0; col < v1.height; col++) {
            v2 = &(v1.field_8[col]);

            for (row = 0; row < v2->width; row++) {
                indexes[row] = v2->field_38[row];
                widths[row] = 64 - v2->field_44[row];
                locks[row] = sector_lock(v2->field_20[row], &(sectors[row]));

                if (locks[row]) {
                    objects->sectors[objects->num_sectors++] = v2->field_20[row];
                }
            }

            for (v3 = 0; v3 < v2->field_50; v3++) {
                for (row = 0; row < v2->width; row++) {
                    if (locks[row]) {
                        for (v4 = 0; v4 < v2->field_44[row]; v4++) {
                            obj_node = sectors[row]->objects.heads[indexes[row]];
                            while (obj_node != NULL) {
                                if ((dword_5E2F88 & obj_field_int32_get(obj_node->obj, OBJ_F_FLAGS)) == 0
                                    && types[obj_field_int32_get(obj_node->obj, OBJ_F_TYPE)]) {
                                    new_node = object_node_create();
                                    new_node->obj = obj_node->obj;
                                    new_node->next = NULL;
                                    *prev_ptr = new_node;
                                    prev_ptr = &(new_node->next);
                                }
                                obj_node = obj_node->next;
                            }

                            indexes[row]++;
                        }

                        indexes[row] += widths[row];
                    }
                }
            }
        }
    }

    dword_5E2F98++;
}

// 0x440FC0
void object_list_vicinity(int64_t obj, unsigned int flags, ObjectList* objects)
{
    object_list_vicinity_loc(obj_field_int64_get(obj, OBJ_F_LOCATION), flags, objects);
}

// 0x440FF0
void object_list_vicinity_loc(int64_t loc, unsigned int flags, ObjectList* objects)
{
    TigWindowData window_data;
    TigRect screen_rect;
    int64_t center_loc;
    int64_t center_x;
    int64_t center_y;
    int64_t x;
    int64_t y;
    LocRect loc_rect;

    tig_window_data(object_iso_window_handle, &window_data);

    screen_rect = window_data.rect;
    location_at(window_data.rect.width / 2, window_data.rect.height / 2, &center_loc);
    location_xy(center_loc, &center_x, &center_y);
    location_xy(loc, &x, &y);

    screen_rect.x = (int)(x - center_x);
    screen_rect.y = (int)(y - center_y);

    if (sub_4B9130(&screen_rect, &loc_rect)) {
        object_list_rect(&loc_rect, flags, objects);
    } else {
        objects->num_sectors = 0;
        objects->head = NULL;
    }
}

// 0x4410E0
void object_list_destroy(ObjectList* objects)
{
    ObjectNode* node;
    ObjectNode* next;
    int index;

    node = objects->head;
    while (node != NULL) {
        next = node->next;
        object_node_destroy(node);
        node = next;
    }

    for (index = 0; index < objects->num_sectors; index++) {
        sector_unlock(objects->sectors[index]);
    }

    --dword_5E2F98;
}

// 0x441140
bool object_list_remove(ObjectList* objects, int64_t obj)
{
    ObjectNode* node;
    ObjectNode* prev;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (objects->num_sectors > 0) {
        return false;
    }

    node = objects->head;
    prev = objects->head;
    while (node != NULL) {
        if (node->obj == obj) {
            break;
        }
        prev = node;
        node = node->next;
    }

    if (node == NULL) {
        return false;
    }

    if (node == prev) {
        objects->head = node->next;
    } else {
        prev->next = node->next;
    }

    if (objects->num_sectors > 0) {
        sector_id_from_loc(obj_field_int64_get(node->obj, OBJ_F_LOCATION));
    }

    object_node_destroy(node);

    return true;
}

// 0x4411C0
void object_list_followers(int64_t obj, ObjectList* objects)
{
    int cnt;
    int index;
    int64_t follower_obj;
    ObjectNode** parent_ptr;
    ObjectNode* node;

    objects->num_sectors = 0;
    objects->head = NULL;
    parent_ptr = &(objects->head);

    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_FOLLOWER_IDX);
    for (index = 0; index < cnt; index++) {
        follower_obj = obj_arrayfield_handle_get(obj, OBJ_F_CRITTER_FOLLOWER_IDX, index);

        node = object_node_create();
        node->obj = follower_obj;
        node->next = NULL;

        *parent_ptr = node;
        parent_ptr = &((*parent_ptr)->next);
    }

    ++dword_5E2F98;
}

// 0x441260
void object_list_all_followers(int64_t obj, ObjectList* objects)
{
    ObjectNode** parent_ptr;
    ObjectNode* node;
    int cnt;
    ObjectList tmp_list;
    ObjectNode* tmp_node;
    ObjectNode* new_node;

    objects->num_sectors = 0;
    objects->head = NULL;
    parent_ptr = &(objects->head);

    object_list_followers(obj, objects);

    cnt = 0;
    node = objects->head;
    while (node != NULL) {
        node = node->next;
        parent_ptr = &((*parent_ptr)->next);
        cnt++;
    }

    node = objects->head;
    while (node != NULL && cnt > 0) {
        object_list_all_followers(node->obj, &tmp_list);

        tmp_node = tmp_list.head;
        while (tmp_node != NULL) {
            new_node = object_node_create();
            new_node->obj = tmp_node->obj;
            new_node->next = NULL;

            *parent_ptr = new_node;
            parent_ptr = &((*parent_ptr)->next);
        }

        object_list_destroy(&tmp_list);

        node = node->next;
        cnt--;
    }
}

// 0x441310
void object_list_party(int64_t obj, ObjectList* objects)
{
    ObjectNode* node;
    int64_t party_member_obj;
    int party_member_index;
    ObjectNode** parent_ptr;

    objects->num_sectors = 0;
    objects->head = NULL;
    parent_ptr = &(objects->head);

    if (tig_net_is_active()) {
        party_member_obj = party_find_first(obj, &party_member_index);
        do {
            node = object_node_create();
            node->obj = party_member_obj;
            node->next = NULL;

            *parent_ptr = node;
            parent_ptr = &((*parent_ptr)->next);
        } while ((party_member_obj = party_find_next(&party_member_index)) != OBJ_HANDLE_NULL);
    } else {
        node = object_node_create();
        node->obj = obj;
        node->next = NULL;

        objects->head = node;
    }

    ++dword_5E2F98;
}

// 0x4413E0
void object_list_team(int64_t obj, ObjectList* objects)
{
    ObjectNode** parent_ptr;
    ObjectNode* node;
    int cnt;
    ObjectList tmp_list;
    ObjectNode* tmp_node;
    ObjectNode* new_node;

    objects->num_sectors = 0;
    objects->head = NULL;
    parent_ptr = &(objects->head);

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        object_list_party(obj, objects);

        cnt = 0;
        node = objects->head;
        while (node != NULL) {
            cnt++;
            parent_ptr = &((*parent_ptr)->next);
            node = node->next;
        }

        node = objects->head;
        while (node != NULL && cnt > 0) {
            object_list_all_followers(node->obj, &tmp_list);

            tmp_node = tmp_list.head;
            while (tmp_node != NULL) {
                new_node = object_node_create();
                new_node->obj = tmp_node->obj;
                new_node->next = NULL;
                *parent_ptr = new_node;

                parent_ptr = &((*parent_ptr)->next);
                tmp_node = tmp_node->next;
            }

            object_list_destroy(&tmp_list);

            node = node->next;
            cnt--;
        }
    } else {
        new_node = object_node_create();
        new_node->obj = obj;
        new_node->next = NULL;
        *parent_ptr = new_node;
    }
}

// 0x4414E0
void object_list_copy(ObjectList* dst, ObjectList* src)
{
    ObjectNode* dst_node;
    ObjectNode* src_node;

    src_node = src->head;
    while (src_node != NULL) {
        dst_node = dst->head;
        while (dst_node != NULL) {
            if (src_node->obj == dst_node->obj) {
                break;
            }
            dst_node = dst_node->next;
        }

        if (dst_node == NULL) {
            dst_node = object_node_create();
            dst_node->obj = src_node->obj;
            dst_node->next = dst->head;
            dst->head = dst_node;
        }

        src_node = src_node->next;
    }
}

// 0x441540
int sub_441540(int64_t obj)
{
    int cnt = 0;
    int party_member_index;

    party_find_first(obj, &party_member_index);
    do {
        cnt++;
    } while (party_find_next(&party_member_index) != OBJ_HANDLE_NULL);

    return cnt;
}

// 0x4415C0
void object_drop(int64_t obj, int64_t loc)
{
    unsigned int flags;
    int64_t sec;
    Sector* sector;
    TigRect rect;

    if (tig_net_is_active() && !multiplayer_is_locked()) {
        ObjectDropInfo* drop_info;

        drop_info = (ObjectDropInfo*)MALLOC(sizeof(*drop_info));
        drop_info->obj = obj;
        drop_info->loc = loc;
        sub_4A3230(obj_get_id(obj), object_drop_success, drop_info, object_drop_failure, drop_info);

        return;
    }

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    if ((flags & OF_DESTROYED) != 0) {
        return;
    }

    flags &= ~OF_INVENTORY;
    obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
    obj_field_int64_set(obj, OBJ_F_LOCATION, loc);
    obj_field_int32_set(obj, OBJ_F_OFFSET_X, 0);
    obj_field_int32_set(obj, OBJ_F_OFFSET_Y, 0);

    sec = sector_id_from_loc(loc);
    if (sub_4D04E0(sec)) {
        sector_lock(sec, &sector);
        objlist_insert(&(sector->objects), obj);
        sector_unlock(sec);
    }

    obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, 0);
    sub_4D9590(obj, true);
    sub_444270(obj, 2);
    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);
}

// 0x441710
bool object_drop_success(void* userinfo)
{
    ObjectDropInfo* entry = (ObjectDropInfo*)userinfo;

    if (entry != NULL) {
        Packet22 pkt;

        multiplayer_lock();

        pkt.type = 22;
        sub_4F0640(entry->obj, &(pkt.oid));
        pkt.loc = entry->loc;
        tig_net_send_app_all(&pkt, sizeof(pkt));

        object_drop(entry->obj, entry->loc);
        multiplayer_unlock();
        FREE(entry);
    }

    return true;
}

// 0x441780
bool object_drop_failure(void* userinfo)
{
    ObjectDropInfo* drop_info = (ObjectDropInfo*)userinfo;

    FREE(drop_info);

    return true;
}

// 0x4417A0
void object_pickup(int64_t item_obj, int64_t parent_obj)
{
    unsigned int flags;
    TigRect rect;
    int64_t loc;
    int64_t sec;
    Sector* sector;

    if (tig_net_is_active() && !multiplayer_is_locked()) {
        ObjectPickupInfo* pickup_info;

        pickup_info = (ObjectPickupInfo*)MALLOC(sizeof(*pickup_info));
        pickup_info->item_obj = item_obj;
        pickup_info->parent_obj = parent_obj;
        sub_4A3230(obj_get_id(item_obj), object_pickup_success, pickup_info, object_pickup_failure, pickup_info);

        return;
    }

    flags = obj_field_int32_get(item_obj, OBJ_F_FLAGS);
    if ((flags & OF_DESTROYED) != 0) {
        return;
    }

    sub_4D9990(item_obj);
    sub_4D9A90(item_obj);
    object_get_rect(item_obj, RECT_FLAG_FULL, &rect);
    loc = obj_field_int64_get(item_obj, OBJ_F_LOCATION);
    sec = sector_id_from_loc(loc);
    if (sub_4D04E0(sec)) {
        sector_lock(sec, &sector);
        objlist_remove(&(sector->objects), item_obj);
        sector_unlock(sec);
    }

    flags |= OF_INVENTORY;
    obj_field_int32_set(item_obj, OBJ_F_FLAGS, flags);
    obj_field_handle_set(item_obj, OBJ_F_ITEM_PARENT, parent_obj);
    object_iso_invalidate_rect(&rect);
}

// 0x4418E0
bool object_pickup_success(void* userinfo)
{
    ObjectPickupInfo* pickup_info = (ObjectPickupInfo*)userinfo;

    if (pickup_info != NULL) {
        Packet23 pkt;

        multiplayer_lock();

        pkt.type = 23;
        sub_4F0640(pickup_info->item_obj, &(pkt.item_oid));
        sub_4F0640(pickup_info->parent_obj, &(pkt.parent_oid));

        object_pickup(pickup_info->item_obj, pickup_info->parent_obj);
        multiplayer_unlock();
        FREE(pickup_info);
    }

    return true;
}

// 0x441960
bool object_pickup_failure(void* userinfo)
{
    ObjectPickupInfo* pickup_info = (ObjectPickupInfo*)userinfo;

    FREE(pickup_info);

    return true;
}

// 0x441980
bool object_script_execute(int64_t triggerer_obj, int64_t attachee_obj, int64_t extra_obj, int attachment_point, int line)
{
    Script scr;
    Script scr_before;
    ScriptInvocation invocation;
    bool rc;

    if (object_editor) {
        return true;
    }

    if (attachment_point == SAP_USE
        && !trap_is_spotted(triggerer_obj, attachee_obj)
        && trap_attempt_spot(triggerer_obj, attachee_obj)) {
        return false;
    }

    obj_arrayfield_script_get(attachee_obj, OBJ_F_SCRIPTS_IDX, attachment_point, &scr);

    if (scr.num == 0) {
        return true;
    }

    scr_before = scr;
    invocation.script = &scr;
    invocation.attachee_obj = attachee_obj;
    invocation.triggerer_obj = triggerer_obj;
    invocation.extra_obj = extra_obj;
    invocation.attachment_point = attachment_point;
    invocation.line = line;
    rc = script_execute(&invocation);

    if (scr.num != 0 || obj_field_int32_get(attachee_obj, OBJ_F_TYPE) != OBJ_TYPE_TRAP) {
        if (scr_before.num != scr.num
            || scr_before.hdr.flags != scr.hdr.flags
            || scr_before.hdr.counters != scr.hdr.counters) {
            mp_obj_arrayfield_script_set(attachee_obj, OBJ_F_SCRIPTS_IDX, attachment_point, &scr);
        }
    } else {
        object_destroy(attachee_obj);
    }

    return rc;
}

// 0x441AE0
int64_t object_dist(int64_t a, int64_t b)
{
    int64_t a_loc;
    int64_t b_loc;

    a_loc = obj_field_int64_get(a, OBJ_F_LOCATION);
    b_loc = obj_field_int64_get(b, OBJ_F_LOCATION);
    return location_dist(a_loc, b_loc);
}

// 0x441B20
int object_rot(int64_t a, int64_t b)
{
    int64_t a_loc;
    int64_t b_loc;

    a_loc = obj_field_int64_get(a, OBJ_F_LOCATION);
    b_loc = obj_field_int64_get(b, OBJ_F_LOCATION);
    return location_rot(a_loc, b_loc);
}

// 0x441B60
void object_examine(int64_t obj, int64_t pc_obj, char* buffer)
{
    int type;
    const char* name;

    buffer[0] = '\0';

    type = obj_field_int32_get(obj, OBJ_F_TYPE);

    // Special case for keys - key name is derived from key id and it is stored
    // in a separate file (gamekey.mes).
    if (type == OBJ_TYPE_KEY) {
        name = key_description_get(obj_field_int32_get(obj, OBJ_F_KEY_KEY_ID));
        if (name != NULL) {
            strcpy(buffer, name);
        }
        return;
    }

    // Special case for PC - name is stored in a specific object field.
    if (type == OBJ_TYPE_PC) {
        char* player_name;

        obj_field_string_get(obj, OBJ_F_PC_PLAYER_NAME, &player_name);
        if (player_name != NULL) {
            strcpy(buffer, player_name);
            FREE(player_name);
        }

        return;
    }

    // Special case for NPC - it's either description or unknown description
    // depending on who's asking.
    if (type == OBJ_TYPE_NPC) {
        name = description_get(critter_description_get(obj, pc_obj));
        if (name != NULL) {
            strcpy(buffer, name);
        }
        return;
    }

    name = description_get(obj_field_int32_get(obj, OBJ_F_DESCRIPTION));
    if (name != NULL) {
        strcpy(buffer, name);
    }
}

// 0x441C70
void object_set_gender_and_race(int64_t obj, int body_type, int gender, int race)
{
    tig_art_id_t aid;

    stat_base_set(obj, STAT_RACE, race);
    stat_base_set(obj, STAT_GENDER, gender);

    aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    aid = tig_art_critter_id_body_type_set(aid, body_type);
    aid = tig_art_critter_id_gender_set(aid, gender);
    obj_field_int32_set(obj, OBJ_F_AID, aid);
    obj_field_int32_set(obj, OBJ_F_CURRENT_AID, aid);
    obj_field_int32_set(obj, OBJ_F_SOUND_EFFECT, 10 * (gender + 2 * race + 1));
}

// 0x441CF0
bool object_is_lockable(int64_t obj)
{
    int type;

    if (sub_49B290(obj) == BP_JUNK_PILE) {
        return false;
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    return type == OBJ_TYPE_CONTAINER || type == OBJ_TYPE_PORTAL;
}

// 0x441D40
bool object_locked_get(int64_t obj)
{
    int type;
    unsigned int flags;
    int hour;

    if (!object_is_lockable(obj)) {
        return false;
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    flags = obj_field_int32_get(obj, type == OBJ_TYPE_PORTAL ? OBJ_F_PORTAL_FLAGS : OBJ_F_CONTAINER_FLAGS);
    if ((flags & (type == OBJ_TYPE_PORTAL ? OPF_BUSTED : OCOF_BUSTED)) != 0) {
        return false;
    }

    if ((flags & (type == OBJ_TYPE_PORTAL ? OPF_NEVER_LOCKED : OCOF_NEVER_LOCKED)) != 0) {
        return false;
    }

    hour = datetime_current_hour();

    if ((flags & (type == OBJ_TYPE_PORTAL ? OPF_LOCKED_DAY : OCOF_LOCKED_DAY)) != 0
        && (hour < 7 || hour > 21)) {
        return false;
    }

    if ((flags & (type == OBJ_TYPE_PORTAL ? OPF_LOCKED_NIGHT : OCOF_LOCKED_NIGHT)) != 0
        && (hour >= 7 && hour <= 21)) {
        return false;
    }

    return (flags & (type == OBJ_TYPE_PORTAL ? OPF_LOCKED : OCOF_LOCKED)) != 0;
}

// 0x441DD0
bool object_locked_set(int64_t obj, bool locked)
{
    int type;
    unsigned int flags;
    DateTime datetime;
    TimeEvent timeevent;

    if (!object_is_lockable(obj)) {
        return false;
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    flags = obj_field_int32_get(obj, type == OBJ_TYPE_PORTAL ? OBJ_F_PORTAL_FLAGS : OBJ_F_CONTAINER_FLAGS);
    if ((flags & (type == OBJ_TYPE_PORTAL ? OPF_LOCKED : OCOF_LOCKED)) != 0 && !locked) {
        timeevent.type = TIMEEVENT_TYPE_LOCK;
        timeevent.params[0].object_value = obj;
        sub_45A950(&datetime, 3600000);
        timeevent_add_delay(&timeevent, &datetime);
    }

    if (locked) {
        flags |= (type == OBJ_TYPE_PORTAL ? OPF_LOCKED : OCOF_LOCKED);
    } else {
        flags &= ~(type == OBJ_TYPE_PORTAL ? OPF_LOCKED : OCOF_LOCKED);
    }

    obj_field_int32_set(obj,
        type == OBJ_TYPE_PORTAL ? OBJ_F_PORTAL_FLAGS : OBJ_F_CONTAINER_FLAGS,
        flags);

    return object_locked_get(obj);
}

// 0x441E90
bool object_lock_timeevent_process(TimeEvent* timeevent)
{
    int64_t obj;
    tig_art_id_t aid;

    obj = timeevent->params[0].object_value;
    object_locked_set(obj, true);
    object_jammed_set(obj, false);
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PORTAL) {
        if (portal_is_open(obj)) {
            portal_toggle(obj);
        }
    } else {
        aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        if (tig_art_id_frame_get(aid) == 1) {
            object_dec_current_aid(obj);
        }
    }

    return true;
}

// 0x441F10
bool object_jammed_set(int64_t obj, bool jammed)
{
    int type;
    unsigned int flags;
    DateTime datetime;
    TimeEvent timeevent;

    if (!object_is_lockable(obj)) {
        return false;
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    flags = obj_field_int32_get(obj, type == OBJ_TYPE_PORTAL ? OBJ_F_PORTAL_FLAGS : OBJ_F_CONTAINER_FLAGS);
    if ((flags & (type == OBJ_TYPE_PORTAL ? OPF_JAMMED : OCOF_JAMMED)) == 0 && jammed) {
        timeevent.type = TIMEEVENT_TYPE_LOCK;
        timeevent.params[0].object_value = obj;
        sub_45A950(&datetime, 86400000);
        timeevent_add_delay(&timeevent, &datetime);
    }

    if (jammed) {
        flags |= (type == OBJ_TYPE_PORTAL ? OPF_JAMMED : OCOF_JAMMED);
    } else {
        flags &= ~(type == OBJ_TYPE_PORTAL ? OPF_JAMMED : OCOF_JAMMED);
    }

    obj_field_int32_set(obj,
        type == OBJ_TYPE_PORTAL ? OBJ_F_PORTAL_FLAGS : OBJ_F_CONTAINER_FLAGS,
        flags);

    return true;
}

// 0x441FC0
void sub_441FC0(int64_t obj, int a2)
{
    if (!object_editor) {
        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC
            && (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) == 0) {
            sub_4AD790(obj, a2);
        }
    }
}

// 0x442010
void object_blit_flags_set(unsigned int flags)
{
    object_blit_flags = flags;
    if (object_iso_invalidate_rect != NULL) {
        object_iso_invalidate_rect(NULL);
        sub_438530(player_get_local_pc_obj());
    }
}

// 0x442040
int object_blit_flags_get()
{
    return object_blit_flags;
}

// 0x442050
void sub_442050(uint8_t** data_ptr, int* size_ptr, int64_t obj)
{
    sub_4064B0(obj);
    obj_write_mem(data_ptr, size_ptr, obj);
    sub_406520(obj);
}

// 0x4420D0
bool sub_4420D0(uint8_t* data, int64_t* obj_ptr, int64_t loc)
{
    if (!obj_read_mem(data, obj_ptr)) {
        return false;
    }

    sub_406520(*obj_ptr);
    obj_field_int64_set(*obj_ptr, OBJ_F_LOCATION, loc);
    if (!sub_442260(*obj_ptr, loc)) {
        return false;
    }

    return true;
}

// 0x442130
bool object_create_func(int64_t proto_obj, int64_t loc, int64_t* obj_ptr, ObjectID oid)
{
    if (oid.type == OID_TYPE_BLOCKED) {
        sub_4058E0(proto_obj, loc, obj_ptr);
    } else {
        sub_405B30(proto_obj, loc, oid, obj_ptr);
    }

    return sub_442260(*obj_ptr, loc);
}

// 0x4421A0
bool object_duplicate_func(int64_t proto_obj, int64_t loc, ObjectID* oids, int64_t* obj_ptr)
{
    if (oids->type == OID_TYPE_BLOCKED) {
        obj_perm_dup(obj_ptr, proto_obj);
    } else {
        sub_406210(obj_ptr, proto_obj, oids);
    }

    if (!sub_442260(*obj_ptr, loc)) {
        return false;
    }

    object_move_to_location(*obj_ptr, loc, 0, 0);

    if (obj_field_int32_get(*obj_ptr, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        obj_field_int64_set(*obj_ptr, OBJ_F_NPC_STANDPOINT_DAY, loc);
        obj_field_int64_set(*obj_ptr, OBJ_F_NPC_STANDPOINT_NIGHT, loc);
    }

    return true;
}

// 0x442260
bool sub_442260(int64_t obj, int64_t loc)
{
    int type;
    unsigned int flags;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int64_t item_obj;
    int64_t sector_id;
    Sector* sector;
    TigRect rect;

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (!object_editor) {
        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_DYNAMIC;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

        if (type == OBJ_TYPE_CONTAINER) {
            inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
            inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
        } else if (obj_type_is_critter(type)) {
            inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
            inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
        } else {
            inventory_num_fld = -1;
        }

        if (inventory_num_fld != -1) {
            cnt = obj_field_int32_get(obj, inventory_num_fld);
            while (cnt != 0) {
                item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, --cnt);
                flags = obj_field_int32_get(item_obj, OBJ_F_FLAGS);
                flags |= OF_DYNAMIC;
                obj_field_int32_set(item_obj, OBJ_F_FLAGS, flags);
            }
        }
    }

    sector_id = sector_id_from_loc(loc);
    if (object_is_static(obj) || sub_4D04E0(sector_id)) {
        sector_lock(sector_id, &sector);
        objlist_insert(&(sector->objects), obj);
        sector_unlock(sector_id);
    }

    sub_4D9590(obj, true);

    if (!object_editor) {
        if (type == OBJ_TYPE_NPC) {
            sub_4AD6E0(obj);
        }
    }

    sub_463C60(obj);
    sub_444270(obj, 2);

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);

    return true;
}

// 0x4423E0
void sub_4423E0(int64_t obj, int offset_x, int offset_y)
{
    TigRect rect;
    int64_t loc;
    unsigned int flags;
    int64_t sector_id;
    Sector* sector;

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);

    if ((flags & OF_TEXT) != 0) {
        tb_notify_moved(obj, loc, offset_x, offset_y);
    }

    if ((flags & OF_TEXT_FLOATER) != 0) {
        tf_notify_moved(obj, loc, offset_x, offset_y);
    }

    sector_id = sector_id_from_loc(loc);
    if (object_is_static(obj) || sub_4D04E0(sector_id)) {
        sector_lock(sector_id, &sector);
        objlist_move(&(sector->objects), obj, loc, offset_x, offset_y);
        sector_unlock(sector_id);
    } else {
        obj_field_int32_set(obj, OBJ_F_OFFSET_X, offset_x);
        obj_field_int32_set(obj, OBJ_F_OFFSET_Y, offset_y);
    }

    sub_444270(obj, 0);
    sub_4DD0A0(obj, offset_x, offset_y);
}

// 0x442520
void sub_442520(int64_t obj)
{
    unsigned int render_flags;
    unsigned int new_render_flags;
    unsigned int flags;
    TigRect obj_rect;

    render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
    if ((obj_field_int32_get(obj, OBJ_F_BLIT_FLAGS) & (TIG_ART_BLT_BLEND_ADD | TIG_ART_BLT_BLEND_SUB | TIG_ART_BLT_BLEND_MUL)) != 0) {
        render_flags &= ~(ORF_40000000 | ORF_20000000);
        render_flags &= ~(TIG_ART_BLT_PALETTE_ORIGINAL
            | TIG_ART_BLT_PALETTE_OVERRIDE
            | TIG_ART_BLT_BLEND_ALPHA_AVG
            | TIG_ART_BLT_BLEND_ALPHA_CONST
            | TIG_ART_BLT_BLEND_ALPHA_SRC
            | TIG_ART_BLT_BLEND_ALPHA_LERP_X
            | TIG_ART_BLT_BLEND_ALPHA_LERP_Y
            | TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH
            | TIG_ART_BLT_BLEND_COLOR_CONST
            | TIG_ART_BLT_BLEND_COLOR_ARRAY
            | TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S
            | TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D
            | TIG_ART_BLT_BLEND_COLOR_LERP);
        render_flags |= ORF_02000000 | ORF_01000000;
        obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags);
        object_render_palette_clear(obj);
        return;
    }

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    if ((render_flags & ORF_01000000) == 0) {
        new_render_flags = render_flags & ~(TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D | TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S | TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH | TIG_ART_BLT_BLEND_ALPHA_LERP_Y | TIG_ART_BLT_BLEND_ALPHA_LERP_X | TIG_ART_BLT_BLEND_ALPHA_SRC | TIG_ART_BLT_BLEND_ALPHA_CONST | TIG_ART_BLT_BLEND_ALPHA_AVG);
        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_WALL) {
            switch (obj_field_int32_get(obj, OBJ_F_WALL_FLAGS) & (OWAF_TRANS_LEFT | OWAF_TRANS_RIGHT)) {
            case OWAF_TRANS_LEFT | OWAF_TRANS_RIGHT:
                obj_arrayfield_int32_set(obj, OBJ_F_RENDER_ALPHA, 0, 64);
                new_render_flags |= object_blit_flags;
                break;
            case OWAF_TRANS_LEFT:
                if (object_blit_flags == TIG_ART_BLT_BLEND_ALPHA_CONST) {
                    obj_arrayfield_int32_set(obj, OBJ_F_RENDER_ALPHA, 0, 128);
                    obj_arrayfield_int32_set(obj, OBJ_F_RENDER_ALPHA, 1, 255);
                    new_render_flags |= TIG_ART_BLT_BLEND_ALPHA_LERP_X;
                } else {
                    new_render_flags |= object_blit_flags;
                }
                break;
            case OWAF_TRANS_RIGHT:
                if (object_blit_flags == TIG_ART_BLT_BLEND_ALPHA_CONST) {
                    obj_arrayfield_int32_set(obj, OBJ_F_RENDER_ALPHA, 0, 255);
                    obj_arrayfield_int32_set(obj, OBJ_F_RENDER_ALPHA, 1, 128);
                    new_render_flags |= TIG_ART_BLT_BLEND_ALPHA_LERP_X;
                } else {
                    new_render_flags |= object_blit_flags;
                }
                break;
            }
        }

        if ((new_render_flags & (TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D | TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S | TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH | TIG_ART_BLT_BLEND_ALPHA_LERP_Y | TIG_ART_BLT_BLEND_ALPHA_LERP_X | TIG_ART_BLT_BLEND_ALPHA_SRC | TIG_ART_BLT_BLEND_ALPHA_CONST | TIG_ART_BLT_BLEND_ALPHA_AVG)) != 0
            && (flags & (OF_INVISIBLE | OF_TRANSLUCENT)) != 0) {
            obj_arrayfield_int32_set(obj, OBJ_F_RENDER_ALPHA, 0, 128);
            new_render_flags |= object_blit_flags;
        }

        render_flags = new_render_flags | ORF_01000000;
        obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags);
    }

    if ((render_flags & ORF_02000000) == 0) {
        ObjectRenderColors colors;
        int cnt;
        tig_color_t color;

        new_render_flags = render_flags;
        new_render_flags &= ~(ORF_40000000 | ORF_20000000);
        new_render_flags &= ~(TIG_ART_BLT_BLEND_COLOR_LERP
            | TIG_ART_BLT_BLEND_COLOR_ARRAY
            | TIG_ART_BLT_BLEND_COLOR_CONST
            | TIG_ART_BLT_PALETTE_OVERRIDE
            | TIG_ART_BLT_PALETTE_ORIGINAL);
        sub_4DC210(obj, colors.colors, &cnt);

        if (cnt == 1) {
            color = colors.colors[0];
            new_render_flags |= ORF_20000000;
        } else if (cnt == 2) {
            new_render_flags |= ORF_40000000;
            object_get_rect(obj, RECT_FLAG_NONE, &obj_rect);
            color = colors.colors[obj_rect.width / 2];
        } else {
            color = colors.colors[0];
            if (dword_5E2F48) {
                new_render_flags |= ORF_20000000;
            }
        }

        if ((flags & OF_FROZEN) != 0) {
            // TODO: Incomplete.
            switch (object_bpp) {
            case 32:
                color = tig_color_mul(color, tig_color_make(0, 128, 255));
                break;
            }
        } else if ((flags & OF_ANIMATED_DEAD) != 0) {
            // TODO: Incomplete.
            switch (object_bpp) {
            case 32:
                color = tig_color_mul(color, tig_color_make(0, 255, 0));
                break;
            }
        }

        obj_field_int32_set(obj, OBJ_F_COLOR, color);
        obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, new_render_flags | ORF_02000000);
        sub_442D90(obj, &colors);
    }

    object_get_rect(obj, RECT_FLAG_FULL, &obj_rect);
    object_iso_invalidate_rect(&obj_rect);
}

// 0x442D50
void object_render_palette_clear(int64_t obj)
{
    TigPalette palette;

    palette = (TigPalette)obj_field_ptr_get(obj, OBJ_F_RENDER_PALETTE);
    if (palette != NULL) {
        tig_palette_destroy(palette);
        obj_field_ptr_set(obj, OBJ_F_RENDER_PALETTE, NULL);
    }
}

// 0x442D90
void sub_442D90(int64_t obj, ObjectRenderColors* colors)
{
    unsigned int obj_flags;
    unsigned int render_flags;
    unsigned int blit_flags;
    TigPaletteModifyInfo palette_modify_info;
    TigArtAnimData art_anim_data;
    TigPalette render_palette;

    palette_modify_info.flags = 0;
    obj_flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
    blit_flags = obj_field_int32_get(obj, OBJ_F_BLIT_FLAGS);

    if ((obj_flags & OF_STONED) != 0) {
        render_flags |= TIG_ART_BLT_PALETTE_OVERRIDE;
        palette_modify_info.flags |= TIG_PALETTE_MODIFY_GRAYSCALE;
    }

    if ((obj_flags & OF_DONTLIGHT) != 0
        || (blit_flags & (TIG_ART_BLT_BLEND_MUL | TIG_ART_BLT_BLEND_ADD)) != 0) {
        if (!dword_5E2F48) {
            render_flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
        }
        render_flags &= ~(ORF_40000000 | ORF_20000000);
    }

    if ((render_flags & ORF_20000000) != 0) {
        if (dword_5E2F48) {
            render_flags |= TIG_ART_BLT_BLEND_COLOR_CONST;
        } else {
            palette_modify_info.flags |= TIG_PALETTE_MODIFY_TINT;
            palette_modify_info.tint_color = obj_field_int32_get(obj, OBJ_F_COLOR);
            render_flags |= TIG_ART_BLT_PALETTE_OVERRIDE;
        }
    }

    if ((render_flags & ORF_40000000) != 0) {
        object_render_colors_set(obj, colors);
        render_flags |= TIG_ART_BLT_BLEND_COLOR_ARRAY;
        if (!dword_5E2F48) {
            render_flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
        }
    } else {
        object_render_colors_clear(obj);
    }

    if ((render_flags & TIG_ART_BLT_PALETTE_OVERRIDE) != 0) {
        render_palette = object_render_palette_get(obj);
        if (tig_art_anim_data(obj_field_int32_get(obj, OBJ_F_CURRENT_AID), &art_anim_data) != TIG_OK) {
            return;
        }

        palette_modify_info.src_palette = art_anim_data.palette2;
        palette_modify_info.dst_palette = render_palette;
        tig_palette_modify(&palette_modify_info);
    } else {
        object_render_palette_clear(obj);
    }

    obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags);
}

// 0x442ED0
TigPalette object_render_palette_get(int64_t obj)
{
    TigPalette palette;

    palette = (TigPalette)obj_field_ptr_get(obj, OBJ_F_RENDER_PALETTE);
    if (palette == NULL) {
        palette = tig_palette_create();
        obj_field_ptr_set(obj, OBJ_F_RENDER_PALETTE, palette);
    }

    return palette;
}

// 0x442F10
void object_render_colors_set(int64_t obj, ObjectRenderColors* colors)
{
    ObjectRenderColors* render_colors;
    tig_art_id_t aid;
    TigArtFrameData art_frame_data;
    tig_color_t color;

    render_colors = (ObjectRenderColors*)obj_field_ptr_get(obj, OBJ_F_RENDER_COLORS);
    if (render_colors == NULL) {
        render_colors = render_color_array_alloc();
        obj_field_ptr_set(obj, OBJ_F_RENDER_COLORS, render_colors);
    }

    memcpy(render_colors, colors, sizeof(ObjectRenderColors));

    aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    if (tig_art_frame_data(aid, &art_frame_data) == TIG_OK) {
        color = render_colors->colors[art_frame_data.width / 2];
    } else {
        color = render_colors->colors[40];
    }

    obj_field_int32_set(obj, OBJ_F_COLOR, color);
}

// 0x442FA0
void object_render_colors_clear(int64_t obj)
{
    ObjectRenderColors* colors;

    colors = (ObjectRenderColors*)obj_field_ptr_get(obj, OBJ_F_RENDER_COLORS);
    if (colors != NULL) {
        render_color_array_free(colors);
        obj_field_ptr_set(obj, OBJ_F_RENDER_COLORS, NULL);
    }
}

// 0x442FE0
ObjectRenderColors* render_color_array_alloc()
{
    ObjectRenderColorsNode* node;

    node = object_render_color_array_node_head;
    if (node == NULL) {
        render_color_array_reserve();
        node = object_render_color_array_node_head;
    }

    object_render_color_array_node_head = node->next;
    node->next = NULL;

    return &(node->data->colors);
}

// 0x443010
void render_color_array_free(ObjectRenderColors* colors)
{
    ObjectRenderColorsNode* node;

    node = ((IndexedObjectRenderColors*)((unsigned char*)colors - offsetof(IndexedObjectRenderColors, colors)))->ptr;
    node->next = object_render_color_array_node_head;
    object_render_color_array_node_head = node;
}

// 0x443030
void render_color_array_reserve()
{
    int index;
    ObjectRenderColorsNode* node;

    for (index = 0; index < 16; index++) {
        node = (ObjectRenderColorsNode*)MALLOC(sizeof(*node));
        node->data = (IndexedObjectRenderColors*)MALLOC(sizeof(*node->data));
        // Link back from data to node.
        node->data->ptr = node;
        node->next = object_render_color_array_node_head;
        object_render_color_array_node_head = node;
    }
}

// 0x443070
void render_color_array_clear()
{
    ObjectRenderColorsNode* node;
    ObjectRenderColorsNode* next;

    node = object_render_color_array_node_head;
    while (node != NULL) {
        next = node->next;
        FREE(node->data);
        FREE(node);
        node = next;
    }
    object_render_color_array_node_head = NULL;
}

// 0x4430B0
void object_toggle_flat(int64_t obj, bool flat)
{
    TigRect rect;
    int64_t sector_id;
    Sector* sector;
    unsigned int flags;

    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    sector_id = sector_id_from_loc(obj_field_int64_get(obj, OBJ_F_LOCATION));

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    if (flat) {
        flags |= OF_FLAT;
    } else {
        flags &= ~OF_FLAT;
    }
    obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

    if (sub_4D04E0(sector_id)) {
        sector_lock(sector_id, &sector);
        objlist_remove(&(sector->objects), obj);
        objlist_insert(&(sector->objects), obj);
        sector_unlock(sector_id);
    }

    object_iso_invalidate_rect(&rect);
}

// 0x443170
void object_setup_blit(int64_t obj, TigArtBlitInfo* blit_info)
{
    unsigned int render_flags;
    unsigned int obj_flags;

    render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
    if ((render_flags & (ORF_02000000 | ORF_01000000)) != (ORF_02000000 | ORF_01000000)) {
        sub_442520(obj);
        render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
    }

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_FROZEN) != 0) {
        blit_info->flags = TIG_ART_BLT_BLEND_ADD | TIG_ART_BLT_BLEND_COLOR_CONST;
        if (!dword_5E2F48) {
            blit_info->flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
        }
    } else if ((obj_field_int32_get(obj, OBJ_F_BLIT_FLAGS) & TIG_ART_BLT_BLEND_ADD) != 0) {
        blit_info->flags = TIG_ART_BLT_BLEND_ADD;
        if (!dword_5E2F48) {
            blit_info->flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
        }
    } else if ((obj_field_int32_get(obj, OBJ_F_BLIT_FLAGS) & TIG_ART_BLT_BLEND_MUL) != 0) {
        blit_info->flags = TIG_ART_BLT_BLEND_MUL;
        if (!dword_5E2F48) {
            blit_info->flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
        }
    } else if ((obj_field_int32_get(obj, OBJ_F_BLIT_FLAGS) & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
        if ((blit_info->flags & 0x1D00) == 0) {
            blit_info->flags = TIG_ART_BLT_BLEND_ALPHA_CONST;
            obj_arrayfield_int32_set(obj,
                OBJ_F_RENDER_ALPHA,
                0,
                obj_field_int32_get(obj, OBJ_F_BLIT_ALPHA));
        }
    } else {
        blit_info->flags = render_flags & ~(ORF_01000000 | ORF_02000000 | ORF_04000000 | ORF_08000000 | ORF_10000000 | ORF_20000000 | ORF_40000000 | ORF_80000000);
    }

    blit_info->art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    if (tig_art_type(blit_info->art_id) == TIG_ART_TYPE_EYE_CANDY
        && tig_art_eye_candy_id_translucency_get(blit_info->art_id)) {
        blit_info->flags |= TIG_ART_BLT_BLEND_ADD;
    }

    if ((blit_info->flags & TIG_ART_BLT_PALETTE_OVERRIDE) != 0) {
        blit_info->palette = (TigPalette)obj_field_ptr_get(obj, OBJ_F_RENDER_PALETTE);
    }

    if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_ARRAY) != 0) {
        blit_info->field_14 = (uint32_t*)obj_field_ptr_get(obj, OBJ_F_RENDER_COLORS);
    }

    if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_CONST) != 0) {
        blit_info->color = obj_field_int32_get(obj, OBJ_F_COLOR);
    }

    if ((blit_info->flags & (TIG_ART_BLT_BLEND_ALPHA_CONST | TIG_ART_BLT_BLEND_ALPHA_LERP_X | TIG_ART_BLT_BLEND_ALPHA_LERP_Y | TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH))) {
        blit_info->alpha[0] = (uint8_t)obj_arrayfield_int32_get(obj, OBJ_F_RENDER_ALPHA, 0);

        if ((blit_info->flags & (TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH | TIG_ART_BLT_BLEND_ALPHA_LERP_X)) != 0) {
            blit_info->alpha[1] = (uint8_t)obj_arrayfield_int32_get(obj, OBJ_F_RENDER_ALPHA, 1);
        }

        if ((blit_info->flags & (TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH | TIG_ART_BLT_BLEND_ALPHA_LERP_Y)) != 0) {
            blit_info->alpha[2] = (uint8_t)obj_arrayfield_int32_get(obj, OBJ_F_RENDER_ALPHA, 2);
        }

        if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH) != 0) {
            blit_info->alpha[3] = (uint8_t)obj_arrayfield_int32_get(obj, OBJ_F_RENDER_ALPHA, 3);
        }
    }

    if (object_editor) {
        obj_flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        if ((obj_flags & (OF_OFF | OF_DESTROYED)) != 0) {
            blit_info->flags = TIG_ART_BLT_BLEND_ADD | TIG_ART_BLT_BLEND_COLOR_CONST;
            if (!dword_5E2F48) {
                blit_info->flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
            }

            if ((obj_flags & OF_DESTROYED) != 0) {
                blit_info->color = tig_color_make(255, 0, 0);
            } else {
                blit_info->color = tig_color_make(0, 255, 0);
            }
        }
    }
}

// 0x443440
void object_enqueue_blit(TigArtBlitInfo* blit_info, int order)
{
    blit_info->flags |= TIG_ART_BLT_SCRATCH_VALID;
    blit_info->scratch_video_buffer = gamelib_scratch_video_buffer;

    if (object_blit_queue_size >= object_blit_queue_capacity) {
        object_blit_queue_capacity += 128;
        object_pending_blits = (ObjectBlitInfo*)REALLOC(object_pending_blits, sizeof(*object_pending_blits) * object_blit_queue_capacity);
        object_pending_rects = (ObjectBlitRectInfo*)REALLOC(object_pending_rects, sizeof(*object_pending_rects) * object_blit_queue_capacity);
    }

    object_pending_blits[object_blit_queue_size].order = order;
    object_pending_blits[object_blit_queue_size].blit_info = *blit_info;
    object_pending_blits[object_blit_queue_size].rect_index = object_blit_queue_size;

    object_pending_rects[object_blit_queue_size].src_rect = *blit_info->src_rect;
    object_pending_rects[object_blit_queue_size].dst_rect = *blit_info->dst_rect;

    ++object_blit_queue_size;
}

// 0x443560
void object_flush_pending_blits()
{
    int index;

    if (object_blit_queue_size == 0) {
        return;
    }

    qsort(object_pending_blits, object_blit_queue_size, sizeof(*object_pending_blits), object_compare_blits);

    for (index = 0; index < object_blit_queue_size; index++) {
        object_pending_blits[index].blit_info.src_rect = &(object_pending_rects[object_pending_blits[index].rect_index].src_rect);
        object_pending_blits[index].blit_info.dst_rect = &(object_pending_rects[object_pending_blits[index].rect_index].dst_rect);
        tig_window_blit_art(object_iso_window_handle, &(object_pending_blits[index].blit_info));
    }

    object_blit_queue_size = 0;
}

// 0x443600
int object_compare_blits(const void* va, const void* vb)
{
    const ObjectBlitInfo* a = (const ObjectBlitInfo*)va;
    const ObjectBlitInfo* b = (const ObjectBlitInfo*)vb;

    if (b->order > a->order) {
        return -1;
    } else if (b->order < a->order) {
        return 1;
    } else {
        return 0;
    }
}

// 0x443620
void sub_443620(unsigned int flags, int scale, int x, int y, tig_art_id_t art_id, TigRect* rect)
{
    TigArtFrameData art_frame_data;
    int scale_type;
    int hot_x;
    int hot_y;
    int width;
    int height;

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        rect->x = 0;
        rect->y = 0;
        rect->width = 0;
        rect->height = 0;
        return;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        scale_type = tig_art_eye_candy_id_scale_get(art_id);
        if (scale_type != 4) {
            scale = scale * dword_5A548C[scale_type] / 100;
        }
    }

    hot_x = art_frame_data.hot_x;
    hot_y = art_frame_data.hot_y;
    width = art_frame_data.width;
    height = art_frame_data.height;

    if (scale != 100) {
        hot_x = (int)((float)hot_x * (float)scale / 100.0f);
        hot_y = (int)((float)hot_y * (float)scale / 100.0f);
        width = (int)((float)width * (float)scale / 100.0f);
        height = (int)((float)height * (float)scale / 100.0f);
    }

    if ((flags & OF_SHRUNK) != 0) {
        hot_x /= 2;
        hot_y /= 2;
        width /= 2;
        height /= 2;
    }

    rect->x = x - hot_x;
    rect->y = y - hot_y;
    rect->width = width;
    rect->height = height;

    if ((flags & OF_WADING) != 0
        && (flags & OF_WATER_WALKING) == 0) {
        rect->y += 15;
    }
}

// 0x443770
void sub_443770(int64_t obj)
{
    sub_4D9A90(obj);
    sub_4437C0(obj);
    object_render_palette_clear(obj);
    object_render_colors_clear(obj);
    obj_field_int32_set(obj,
        OBJ_F_RENDER_FLAGS,
        obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ~(ORF_40000000 | ORF_20000000 | ORF_10000000 | ORF_04000000 | ORF_02000000));
}

// 0x4437C0
void sub_4437C0(int64_t obj)
{
    shadow_clear(obj);
}

// 0x4437E0
bool sub_4437E0(TigRect* rect)
{
    TigRect tmp_rect;

    object_hover_obj = object_hover_obj_get();
    if (object_hover_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (!dword_5E2E94) {
        object_get_rect(object_hover_obj, RECT_FLAG_NONE, rect);
        return true;
    }

    if (!sub_443880(rect, object_hover_underlay_art_id)) {
        return sub_443880(rect, object_hover_overlay_art_id);
    }

    if (sub_443880(&tmp_rect, object_hover_overlay_art_id)) {
        tig_rect_union(rect, &tmp_rect, rect);
    }

    return true;
}

// 0x443880
bool sub_443880(TigRect* rect, tig_art_id_t art_id)
{
    TigArtFrameData art_frame_data;
    int64_t loc;
    int64_t x;
    int64_t y;
    unsigned int flags;

    object_hover_obj = object_hover_obj_get();
    if (object_hover_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    loc = obj_field_int64_get(object_hover_obj, OBJ_F_LOCATION);
    location_xy(loc, &x, &y);

    x += obj_field_int32_get(object_hover_obj, OBJ_F_OFFSET_X);
    y += obj_field_int32_get(object_hover_obj, OBJ_F_OFFSET_Y);

    x += art_frame_data.offset_x - art_frame_data.hot_x;
    y += art_frame_data.offset_y - art_frame_data.hot_y;

    x += 40;
    y += 20;

    rect->x = (int)x;
    rect->y = (int)y;
    rect->width = art_frame_data.width;
    rect->height = art_frame_data.height;

    flags = obj_field_int32_get(object_hover_obj, OBJ_F_FLAGS);
    if ((flags & OF_WADING) != 0
        && (flags & OF_WATER_WALKING) == 0) {
        rect->y += 15;
    }

    return true;
}

// 0x4439D0
bool object_save_obj_handle_safe(int64_t* obj_ptr, Ryan* a2, TigFile* stream)
{
    ObjectID oid;
    int64_t loc;
    int map;

    if (stream == NULL) {
        return false;
    }

    if (a2 != NULL) {
        oid = a2->objid;
        loc = a2->location;
        map = a2->map;
    } else if (*obj_ptr != OBJ_HANDLE_NULL) {
        oid = obj_get_id(*obj_ptr);
        loc = obj_field_int64_get(*obj_ptr, OBJ_F_LOCATION);
        map = map_current_map();
    } else {
        oid.type = OID_TYPE_NULL;
        loc = 0;
        map = 0;
    }

    if (tig_file_fwrite(&oid, sizeof(oid), 1, stream) != 1) return false;
    if (tig_file_fwrite(&loc, sizeof(loc), 1, stream) != 1) return false;
    if (tig_file_fwrite(&map, sizeof(map), 1, stream) != 1) return false;

    return true;
}

// 0x443AD0
bool object_load_obj_handle_safe(int64_t* obj_ptr, Ryan* a2, TigFile* stream)
{
    ObjectID oid;
    int64_t loc;
    int map;
    ObjectList objects;
    int64_t obj;
    char buffer[40];

    if (stream == NULL) {
        return false;
    }

    if (obj_ptr == NULL) {
        return false;
    }

    if (tig_file_fread(&oid, sizeof(oid), 1, stream) != 1) return false;
    if (tig_file_fread(&loc, sizeof(loc), 1, stream) != 1) return false;
    if (tig_file_fread(&map, sizeof(map), 1, stream) != 1) return false;

    if (oid.type != OID_TYPE_NULL) {
        if (loc != 0) {
            object_list_location(loc, 0x3FFFF, &objects);
            object_list_destroy(&objects);
        }

        obj = objp_perm_lookup(oid);
        if (obj == OBJ_HANDLE_NULL) {
            tig_debug_printf("Object: object_load_obj_handle_safe: Note: Couldn't match ObjID to HANDLE!\n");
            objid_id_to_str(buffer, oid);
            // FIX: Use 64-bit specifiers instead of casting values to 32-bit
            // ints (which is ok for coordinates, but plain wrong for sector
            // id).
            tig_debug_printf("  Info: MapID: %d, Loc: [%" PRIi64 "x, %" PRIi64 " y], Sector: %" PRIu64 ", ID: %s\n",
                map,
                LOCATION_GET_X(loc),
                LOCATION_GET_Y(loc),
                sector_id_from_loc(loc),
                buffer);
        }

        *obj_ptr = obj;

        if (a2 != NULL) {
            a2->objid = oid;
            a2->location = loc;
            a2->map = map;
        }
    } else {
        *obj_ptr = OBJ_HANDLE_NULL;

        if (a2 != NULL) {
            a2->objid.type = OID_TYPE_NULL;
            a2->location = 0;
            a2->map = 0;
        }
    }

    return true;
}

// 0x443EB0
void sub_443EB0(int64_t obj, Ryan* a2)
{
    if (obj != OBJ_HANDLE_NULL) {
        if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
            a2->objid.type = OID_TYPE_NULL;
            a2->objid.d.a = 0xBEEFBEEF;
            a2->location = 0;
            a2->map = 0;
        } else {
            a2->objid = obj_get_id(obj);
            if (object_is_static(obj)) {
                a2->location = obj_field_int64_get(obj, OBJ_F_LOCATION);
            } else {
                a2->location = 0;
            }
            a2->map = map_current_map();
        }
    } else {
        a2->objid.type = OID_TYPE_NULL;
        a2->location = 0;
        a2->map = 0;
    }
}

// 0x443F80
bool sub_443F80(int64_t* obj_ptr, Ryan* a2)
{
    ObjectList objects;

    if (a2->objid.type == OID_TYPE_NULL) {
        *obj_ptr = OBJ_HANDLE_NULL;
        return true;
    }

    if (a2->location != 0) {
        if (a2->map != map_current_map()) {
            *obj_ptr = OBJ_HANDLE_NULL;
            return false;
        }

        object_list_location(a2->location, 0x3FFFF, &objects);
        object_list_destroy(&objects);
    }

    *obj_ptr = objp_perm_lookup(a2->objid);

    return *obj_ptr != OBJ_HANDLE_NULL;
}

// 0x444020
bool sub_444020(int64_t* obj_ptr, Ryan* a2)
{
    int64_t obj;

    if (obj_ptr == NULL || a2 == NULL) {
        // FIXME: Crash!
        *obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    if (a2->objid.type != OID_TYPE_NULL
        && *obj_ptr != OBJ_HANDLE_NULL
        && !obj_handle_is_valid(*obj_ptr)) {
        if (!sub_443F80(&obj, a2)) {
            tig_debug_printf("Object: ERROR: Object validate recovery FAILED!\n");
            *obj_ptr = OBJ_HANDLE_NULL;
            return false;
        }

        if (obj != OBJ_HANDLE_NULL
            && (obj_field_int32_get(obj, OBJ_F_FLAGS) & (OF_OFF | OF_DESTROYED)) != 0) {
            obj = OBJ_HANDLE_NULL;
        }

        *obj_ptr = obj;
    }

    return true;
}

// 0x4440E0
void sub_4440E0(int64_t obj, FollowerInfo* a2)
{
    if (a2 != NULL) {
        a2->obj = obj;
        sub_443EB0(obj, &(a2->field_8));
    }
}

// 0x444110
bool sub_444110(FollowerInfo* a1)
{
    if (a1 != NULL) {
        return sub_443F80(&(a1->obj), &(a1->field_8));
    } else {
        return false;
    }
}

// 0x444130
bool sub_444130(FollowerInfo* a1)
{
    if (a1 != NULL) {
        return sub_444020(&(a1->obj), &(a1->field_8));
    } else {
        return false;
    }
}

// 0x444150
bool load_reaction_colors()
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;
    char* str;
    int r;
    int g;
    int b;

    if (!mes_load("rules\\reaction_colors.mes", &mes_file)) {
        return false;
    }

    tig_str_parse_set_separator(',');

    for (mes_file_entry.num = 0; mes_file_entry.num < REACTION_COUNT; mes_file_entry.num++) {
        if (!mes_search(mes_file, &mes_file_entry)) {
            tig_debug_printf("Missing entry %d in rules\\reaction_colors.mes", mes_file_entry.num);
            mes_unload(mes_file);
            return false;
        }

        str = mes_file_entry.str;
        tig_str_parse_value(&str, &r);
        tig_str_parse_value(&str, &g);
        tig_str_parse_value(&str, &b);
        object_reaction_colors[mes_file_entry.num] = tig_color_make(r, g, b);
    }

    mes_unload(mes_file);
    return true;
}

// 0x444270
void sub_444270(int64_t obj, int a2)
{
    unsigned int flags;
    unsigned int render_flags;
    int64_t loc;
    TigRect rect;
    int64_t v1;

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
    render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
    render_flags &= ~(ORF_04000000 | ORF_02000000);

    if (a2 == 0) {
        if (!object_lighting) {
            obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags);
        }

        object_get_rect(obj, RECT_FLAG_FULL, &rect);
        object_iso_invalidate_rect(&rect);
        return;
    }

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);

    if ((flags & OF_DISALLOW_WADING) == 0) {
        if (tile_is_sinkable(loc)) {
            if ((flags & OF_WADING) == 0) {
                object_flags_set(obj, OF_WADING);
            }
        } else {
            if ((flags & OF_WADING) != 0) {
                object_flags_unset(obj, OF_WADING);
            }
        }
    }

    if (object_editor) {
        obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags);
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
        object_get_rect(obj, RECT_FLAG_FULL, &rect);
        object_iso_invalidate_rect(&rect);
        return;
    }

    if (player_is_pc_obj(obj)) {
        if (player_is_local_pc_obj(obj)) {
            townmap_mark_visited(loc);
            roof_recalc(loc);
            wallcheck_recalc(loc);
            scroll_set_center(loc);
        }

        map_process_jumppoint(loc, obj);
    }

    if (obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        tile_script_exec(loc, obj);
    }

    v1 = object_hover_obj_get();
    if (v1 != OBJ_HANDLE_NULL) {
        if (v1 == obj || player_is_local_pc_obj(obj)) {
            sub_4604F0(player_get_local_pc_obj(), v1);
        }
    }

    if (a2 != 1) {
        if (player_is_pc_obj(obj) && player_is_local_pc_obj(obj)) {
            objlist_notify_sector_changed(sector_id_from_loc(loc), obj);
        }
        sub_4D9590(obj, true);
    }

    obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags);
    object_get_rect(obj, RECT_FLAG_FULL, &rect);
    object_iso_invalidate_rect(&rect);
}

// 0x444500
int64_t get_fire_at_location(int64_t loc)
{
    ObjectList objects;
    ObjectNode* node;
    int64_t obj = OBJ_HANDLE_NULL;

    object_list_location(loc, OBJ_TM_SCENERY, &objects);

    node = objects.head;
    while (node != NULL) {
        obj = node->obj;
        if (anim_is_current_goal_type(obj, AG_EYE_CANDY_FIRE_DMG, NULL)
            || anim_is_current_goal_type(obj, AG_ANIMATE_LOOP_FIRE_DMG, NULL)) {
            break;
        }
        node = node->next;
    }

    object_list_destroy(&objects);

    return node != NULL ? obj : OBJ_HANDLE_NULL;
}

// 0x4445A0
void sub_4445A0(int64_t a1, int64_t a2)
{
    int type;
    int64_t loc;

    type = obj_field_int32_get(a2, OBJ_F_TYPE);
    if (obj_type_is_critter(type)
        || (obj_type_is_item(type) && (obj_field_int32_get(a2, OBJ_F_TYPE) & OF_INVENTORY) == 0)) {
        if (object_dist(a1, a2) > 1) {
            if (player_is_local_pc_obj(a1)) {
                sub_4606C0(0);
            }
        } else {
            if (tig_net_is_active()) {
                if (tig_net_is_host()) {
                    loc = obj_field_int64_get(a1, OBJ_F_LOCATION);
                    sub_4EDF20(a2, loc, 0, 0, false);
                } else {
                    sub_4EF830(a1, a2);
                }
            } else {
                loc = obj_field_int64_get(a1, OBJ_F_LOCATION);
                object_move_to_location(a2, loc, 0, 0);
            }
        }
    }
}

// 0x444690
void object_lighting_changed()
{
    int value;

    value = settings_get_value(&settings, OBJECT_LIGHTING_KEY);
    if (value < 0) {
        object_lighting = false;
    } else if (value > 1) {
        object_lighting = true;
    }

    object_iso_invalidate_rect(NULL);
}
