#include "game/light.h"

#include "game/critter.h"
#include "game/gamelib.h"
#include "game/object.h"
#include "game/sector.h"
#include "game/sector_block_list.h"
#include "game/tile.h"

typedef struct LightCreateInfo {
    /* 0000 */ int64_t obj;
    /* 0008 */ int64_t loc;
    /* 0010 */ int offset_x;
    /* 0014 */ int offset_y;
    /* 0018 */ tig_art_id_t art_id;
    /* 001C */ unsigned int flags;
    /* 0020 */ uint8_t r;
    /* 0021 */ uint8_t g;
    /* 0022 */ uint8_t b;
} LightCreateInfo;

typedef void(LightCreateFunc)(LightCreateInfo* create_info, Light** light_ptr);

// NOTE: This structure represents light data as it is stored on disk. Its
// memory layout is set in stone and should never change. The main difference
// from `Light` is the type of the `palette` member. Originally, it is a
// pointer, which breaks the memory layout on x64 platforms. This value is
// always `0`, regardless of the pointer value. The engine always nullifies it
// when it's read back.
typedef struct LightSerializedData {
    /* 0000 */ int64_t obj;
    /* 0008 */ int64_t loc;
    /* 0010 */ int offset_x;
    /* 0014 */ int offset_y;
    /* 0018 */ unsigned int flags;
    /* 001C */ tig_art_id_t art_id;
    /* 0020 */ uint8_t r;
    /* 0021 */ uint8_t b;
    /* 0022 */ uint8_t g;
    /* 0024 */ tig_color_t tint_color;
    /* 0028 */ int palette;
    /* 002C */ int padding_2C;
} LightSerializedData;

// Serializeable.
static_assert(sizeof(LightSerializedData) == 0x30, "wrong size");

static bool sub_4D89E0(int64_t loc, int a2, int a3, int a4, tig_color_t* color_ptr);
static void sub_4D9310(LightCreateInfo* create_info, Light** light_ptr);
static void sub_4D93B0(Light* light);
static void sub_4D9510(Light* light);
static void sub_4DD320(Light* light, int64_t loc, int offset_x, int offset_y);
static void sub_4DD500(Light* light, int offset_x, int offset_y);
static void light_set_aid(Light* light, tig_art_id_t art_id);
static tig_art_id_t light_get_aid(Light* light);
static void light_set_custom_color(Light* light, uint8_t r, uint8_t g, uint8_t b);
static bool sub_4DDD90(Sector* sector);
static void shadows_changed();
static bool sub_4DDF50();
static void sub_4DE060();
static bool sub_4DE0B0(tig_art_id_t art_id, TigPaletteModifyInfo* modify_info);
static void sub_4DE200();
static void sub_4DE250();
static void light_get_rect_internal(Light* light, TigRect* rect);
static void sub_4DE390(Light* light);
static void sub_4DE4D0(Light* light);
static void sub_4DE4F0(Light* light, int offset_x, int offset_y);
static bool sub_4DE5D0();
static void sub_4DE730();
static Shadow* shadow_node_allocate();
static void shadow_node_deallocate(Shadow* node);
static void shadow_node_reserve();
static void shadow_node_clear();
static bool sub_4DE820(TimeEvent* timeevent);
static void sub_4DE870(LightCreateInfo* create_info, Light** light_ptr);
static void light_render_internal(GameDrawInfo* draw_info);
static void sub_4DF1D0(TigRect* rect);

// 0x5B9044
static int dword_5B9044[] = {
    0,
    40,
};

// 0x5B904C
static int dword_5B904C[] = {
    0,
    20,
};

// 0x602E10
static int light_bpp;

// 0x602E18
static TigVideoBufferData darker_vb_data;

// 0x602E38
static ViewOptions light_view_options;

// 0x602E40
static void* light_indoor_palette;

// 0x602E44
static int dword_602E44;

// 0x602E48
static TigRect light_iso_content_rect;

// 0x602E58
static TigPalette* dword_602E58;

// 0x602E5C
static TigVideoBuffer* darker_vb;

// 0x602E60
static Shadow* shadow_node_head;

// 0x602E68
static TigVideoBufferData lighter_vb_data;

// 0x602E88
static TigPalette light_outdoor_palette;

// 0x602E8C
static IsoInvalidateRectFunc* light_iso_window_invalidate_rect;

// 0x602E90
static bool light_shadows_enabled;

// 0x602E94
static TigVideoBuffer* lighter_vb;

// 0x602E98
static uint32_t* darker_colors;

// 0x602E9C
static void* dword_602E9C;

// 0x602EA0
static void* dword_602EA0;

// 0x602EA4
static int dword_602EA4;

// 0x602EA8
static tig_color_t light_outdoor_color;

// 0x602EAC
static uint32_t* lighter_colors;

// 0x602EB0
static void* dword_602EB0;

// 0x602EB4
static void* dword_602EB4;

// 0x602EB8
static bool dword_602EB8;

// 0x602EBC
static int darker_pitch;

// 0x602EC0
static bool light_editor;

// 0x602EC4
static tig_window_handle_t light_iso_window_handle;

// 0x602EC8
static int lighter_pitch;

// 0x602ECC
static bool dword_602ECC;

// 0x602ED0
static int dword_602ED0;

// 0x602ED4
static int dword_602ED4;

// 0x602ED8
static TigRect stru_602ED8;

// 0x602EE8
static TigBmp light_shadowmap_bmp;

// 0x603400
static Light* dword_603400;

// 0x603404
static int light_buffers_lock_cnt;

// 0x603408
static tig_color_t light_indoor_color;

// 0x60340C
static bool dword_60340C;

// 0x603418
static int dword_603418;

// 0x60341C
static int dword_60341C;

// 0x4D7F30
bool light_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    dword_602E58 = (TigPalette*)CALLOC(7, sizeof(*dword_602E58));
    sub_4F8330();
    light_iso_window_handle = init_info->iso_window_handle;
    light_iso_window_invalidate_rect = init_info->invalidate_rect_func;
    light_editor = init_info->editor;
    light_view_options.type = VIEW_TYPE_ISOMETRIC;
    dword_602ECC = true;
    dword_602EB8 = true;

    if (tig_video_get_bpp(&light_bpp) != TIG_OK) {
        FREE(dword_602E58);
        return false;
    }

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        FREE(dword_602E58);
        return false;
    }

    light_iso_content_rect.x = 0;
    light_iso_content_rect.y = 0;
    light_iso_content_rect.width = window_data.rect.width;
    light_iso_content_rect.height = window_data.rect.height;

    dword_60340C = tig_video_3d_check_initialized() == TIG_OK;

    settings_register(&settings, SHADOWS_KEY, "0", shadows_changed);
    light_shadows_enabled = settings_get_value(&settings, SHADOWS_KEY);

    if (!sub_4DDF50()) {
        FREE(dword_602E58);
        return false;
    }

    if (!sub_4DE5D0()) {
        FREE(dword_602E58);
        return false;
    }

    light_outdoor_color = tig_color_make(255, 255, 255);
    light_indoor_color = tig_color_make(255, 255, 255);

    sub_4DE200();
    sub_5022B0(sub_4DE0B0);
    sub_5022D0();

    return true;
}

// 0x4D8120
void light_exit()
{
    sub_4DE730();
    sub_4DE250();
    sub_4DE060();
    light_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    light_iso_window_invalidate_rect = NULL;
    sub_4F8340();
    FREE(dword_602E58);
}

// 0x4D8160
void light_resize(GameResizeInfo* resize_info)
{
    light_iso_window_handle = resize_info->window_handle;
    light_iso_content_rect = resize_info->content_rect;
    sub_4DE060();

    if (!sub_4DDF50()) {
        tig_debug_printf("light_resize: ERROR: Failed to rebuild the vbuffer!\n");
    }
}

// 0x4D81B0
void light_update_view(ViewOptions* view_options)
{
    light_view_options = *view_options;
}

// 0x4D81F0
void sub_4D81F0()
{
    dword_602ECC = !dword_602ECC;

    if (!dword_60340C) {
        sub_5022D0();
    }

    sector_enumerate(sub_4DDD90);
    light_invalidate_rect(NULL, false);
}

// 0x4D8210
void light_buffers_lock()
{
    light_buffers_lock_cnt++;
    if (light_buffers_lock_cnt == 1) {
        tig_video_buffer_lock(lighter_vb);
        tig_video_buffer_data(lighter_vb, &lighter_vb_data);

        tig_video_buffer_lock(darker_vb);
        tig_video_buffer_data(darker_vb, &darker_vb_data);

        switch (light_bpp) {
        case 8:
            break;
        case 16:
            lighter_pitch = lighter_vb_data.pitch / 2;
            dword_602EB4 = lighter_vb_data.surface_data.pixels;
            darker_pitch = darker_vb_data.pitch / 2;
            dword_602E9C = darker_vb_data.surface_data.pixels;
            break;
        case 24:
            lighter_pitch = lighter_vb_data.pitch;
            dword_602EB0 = lighter_vb_data.surface_data.pixels;
            darker_pitch = darker_vb_data.pitch;
            dword_602EA0 = darker_vb_data.surface_data.pixels;
            break;
        case 32:
            lighter_pitch = lighter_vb_data.pitch / 4;
            lighter_colors = (uint32_t*)lighter_vb_data.surface_data.pixels;
            darker_pitch = darker_vb_data.pitch / 4;
            darker_colors = (uint32_t*)darker_vb_data.surface_data.pixels;
            break;
        }
    }
}

// 0x4D8320
void light_buffers_unlock()
{
    light_buffers_lock_cnt--;
    if (light_buffers_lock_cnt == 0) {
        tig_video_buffer_unlock(lighter_vb);
        tig_video_buffer_unlock(darker_vb);
    }
}

// 0x4D8350
void light_draw(GameDrawInfo* draw_info)
{
    int64_t center_loc;
    int64_t cx;
    int64_t cy;

    if (!dword_602EB8) {
        return;
    }

    if (light_view_options.type != VIEW_TYPE_ISOMETRIC) {
        return;
    }

    location_at(light_iso_content_rect.width / 2, light_iso_content_rect.height / 2, &center_loc);
    location_xy(center_loc, &cx, &cy);
    cx += 40;
    cy += 20;

    dword_602ED0 = (int)cx - dword_602E44 / 2;
    dword_602ED4 = (int)cy - dword_602EA4 / 2;
    light_render_internal(draw_info);
}

// 0x4D84B0
void light_build_color(uint8_t red, uint8_t green, uint8_t blue, unsigned int* color)
{
    *color = (red << 16) | (green << 8) | blue;
}

// 0x4D84D0
void light_get_color_components(unsigned int color, uint8_t* red, uint8_t* green, uint8_t* blue)
{
    *red = (color >> 16) & 0xFF;
    *green = (color >> 8) & 0xFF;
    *blue = color & 0xFF;
}

// 0x4D8500
tig_color_t light_get_outdoor_color()
{
    return light_outdoor_color;
}

// 0x4D8530
tig_color_t light_get_indoor_color()
{
    return light_indoor_color;
}

// 0x4D8560
void light_set_colors(tig_color_t indoor_color, tig_color_t outdoor_color)
{
    light_indoor_color = indoor_color;
    light_outdoor_color = outdoor_color;
    sub_4DE200();

    if (dword_602ECC) {
        if (!dword_60340C) {
            sub_5022D0();
        }

        sector_enumerate(sub_4DDD90);
        light_invalidate_rect(NULL, false);
    }
}

// 0x4D8590
void light_start_animating(Light* light)
{
    tig_art_id_t art_id;
    TigArtAnimData art_anim_data;
    TimeEvent timeevent;
    DateTime datetime;

    if ((light_get_flags(light) & (LF_00000020 | LF_ANIMATING)) == 0) {
        art_id = light_get_aid(light);
        if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK
            && art_anim_data.num_frames > 1) {
            timeevent.type = TIMEEVENT_TYPE_LIGHT;
            timeevent.params[0].pointer_value = light;
            timeevent.params[1].integer_value = 1000 / art_anim_data.fps;

            sub_45A950(&datetime, 1000 / art_anim_data.fps);
            if (timeevent_add_delay(&timeevent, &datetime)) {
                light_set_flags_internal(light, LF_ANIMATING);
            }
        }
    }
}

// 0x4D8620
void light_stop_animating(Light* light)
{
    if ((light_get_flags(light) & LF_ANIMATING) != 0) {
        dword_603400 = light;
        timeevent_clear_all_ex(TIMEEVENT_TYPE_LIGHT, sub_4DE820);
        light_unset_flags_internal(light, LF_ANIMATING);
    }
}

// 0x4D8660
bool light_timeevent_process(TimeEvent* timeevent)
{
    DateTime datetime;
    TimeEvent next_timeevent;

    light_inc_frame((Light*)timeevent->params[0].pointer_value);

    next_timeevent = *timeevent;
    sub_45A950(&datetime, next_timeevent.params[1].integer_value);
    timeevent_add_delay(&next_timeevent, &datetime);

    return true;
}

// 0x4D89E0
bool sub_4D89E0(int64_t loc, int offset_x, int offset_y, int a4, tig_color_t* color_ptr)
{
    int lx;
    int ly;
    int64_t loc_x;
    int64_t loc_y;
    TigRect tmp_rect;
    LocRect loc_rect;
    tig_color_t indoor_color;
    tig_color_t outdoor_color;
    SectorListNode* head;
    SectorListNode* node;
    Sector* sector;
    SectorBlockListNode* light_node;
    Light* light;
    TigArtAnimData art_anim_data;
    unsigned int color;

    location_xy(loc, &loc_x, &loc_y);

    if (loc_x < INT_MIN
        || loc_x > INT_MAX
        || loc_y < INT_MIN
        || loc_y > INT_MAX) {
        return false;
    }

    tmp_rect.x = (int)loc_x + offset_x - 728;
    tmp_rect.y = (int)loc_y + offset_y - 492;
    tmp_rect.width = 1536;
    tmp_rect.height = 1024;

    lx = (int)loc_x + offset_x + 40;
    ly = (int)loc_y + offset_y + 20;

    if (!sub_4B9130(&tmp_rect, &loc_rect)) {
        return false;
    }

    head = sector_list_create(&loc_rect);
    if (head == NULL) {
        return false;
    }

    indoor_color = light_get_indoor_color();
    outdoor_color = light_get_outdoor_color();

    if (a4) {
        if (tig_art_tile_id_type_get(tile_art_id_at(loc))) {
            *color_ptr = outdoor_color;
        } else {
            *color_ptr = indoor_color;
        }
    } else {
        *color_ptr = 0;
    }

    node = head;
    while (node != NULL) {
        if (sector_lock(node->sec, &sector)) {
            light_node = sector->lights.head;
            while (light_node != NULL) {
                light = (Light*)light_node->data;
                if ((light->flags & LF_OFF) == 0) {
                    light_get_rect_internal(light, &tmp_rect);

                    if (lx >= tmp_rect.x
                        && ly >= tmp_rect.y
                        && lx < tmp_rect.x + tmp_rect.width
                        && ly < tmp_rect.y + tmp_rect.height
                        && sub_502E50(light->art_id, lx - tmp_rect.x, ly - tmp_rect.y, &color) == TIG_OK
                        && tig_art_anim_data(light->art_id, &art_anim_data) == TIG_OK
                        && color != art_anim_data.color_key) {
                        if (((light->flags & LF_INDOOR) != 0
                                && light->tint_color != indoor_color)
                            || ((light->flags & LF_OUTDOOR) != 0
                                && light->tint_color != outdoor_color)) {
                            sub_4DE390(light);
                            light_invalidate_rect(&tmp_rect, true);
                        }

                        switch (light_bpp) {
                        case 8:
                        case 16:
                        case 24:
                            // TODO: Incomplete.
                        case 32: {
                            if (light->palette != NULL) {
                                color = tig_color_mul(color, light->tint_color);
                            }

                            if ((light->flags & LF_DARK) != 0) {
                                *color_ptr = tig_color_sub(color, *color_ptr);
                            } else {
                                *color_ptr = tig_color_add(color, *color_ptr);
                            }
                            break;
                        }
                        default:
                            // Should be unreachable.
                            assert(0);
                        }
                    }
                }
                light_node = light_node->next;
            }

            sector_unlock(node->sec);
        }
        node = node->next;
    }

    sector_list_destroy(head);
    return true;
}

// 0x4D9240
tig_color_t sub_4D9240(int64_t loc, int offset_x, int offset_y)
{
    tig_color_t color;

    if (!sub_4D89E0(loc, offset_x, offset_y, true, &color)) {
        color = tig_color_make(255, 255, 255);
    }

    // TODO: Probably wrong, might return uint8_t, check.
    return tig_color_rgb_to_grayscale(color);
}

// 0x4D9310
void sub_4D9310(LightCreateInfo* create_info, Light** light_ptr)
{
    int64_t sector_id;
    Sector* sector;
    TigRect rect;

    sub_4DE870(create_info, light_ptr);
    if (*light_ptr != NULL) {
        sector_id = sector_id_from_loc(create_info->loc);
        if (sub_4DD110(*light_ptr) || sub_4D04E0(sector_id)) {
            sector_lock(sector_id, &sector);
            sector_light_list_add(&(sector->lights), *light_ptr);
            sector_unlock(sector_id);
        }

        light_get_rect(*light_ptr, &rect);
        light_invalidate_rect(&rect, true);
    }
}

// 0x4D93B0
void sub_4D93B0(Light* light)
{
    TigRect rect;
    int index;

    light_stop_animating(light);

    light_get_rect_internal(light, &rect);
    light_invalidate_rect(&rect, true);

    if (light->obj != OBJ_HANDLE_NULL) {
        if ((Light*)obj_field_ptr_get(light->obj, OBJ_F_LIGHT_HANDLE) == light) {
            obj_field_ptr_set(light->obj, OBJ_F_LIGHT_HANDLE, 0);
            sub_4D9510(light);
            sub_4D9570(light);
        } else {
            for (index = 0; index < 4; index++) {
                if ((Light*)obj_arrayfield_ptr_get(light->obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index) == light) {
                    obj_arrayfield_ptr_set(light->obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index, NULL);
                    sub_4D9510(light);
                    sub_4D9570(light);
                    break;
                }
            }
        }
    } else {
        if (light_editor) {
            sub_4D9510(light);
            sub_4D9570(light);
        } else {
            light_set_flags_internal(light, 0x100);
        }
    }
}

// 0x4D94B0
bool light_is_modified(Light* light)
{
    return (light->flags & LF_MODIFIED) != 0;
}

// 0x4D94C0
void light_clear_modified(Light* light)
{
    light->flags &= ~LF_MODIFIED;
}

// 0x4D94D0
bool light_read_dif(TigFile* stream, Light** light_ptr)
{
    return light_read(stream, light_ptr);
}

// 0x4D94F0
bool light_write_dif(TigFile* stream, Light* light)
{
    return light_write(stream, light);
}

// 0x4D9510
void sub_4D9510(Light* light)
{
    int64_t sector_id;
    Sector* sector;

    sector_id = sector_id_from_loc(light->loc);
    if (sub_4DD110(light) || sub_4D04E0(sector_id)) {
        sector_lock(sector_id, &sector);
        sector_light_list_remove(&(sector->lights), light);
        sector_unlock(sector_id);
    }
}

// 0x4D9570
void sub_4D9570(Light* light)
{
    light_stop_animating(light);
    sub_4DE4D0(light);
    FREE(light);
}

// 0x4D9590
void sub_4D9590(int64_t obj, bool a2)
{
    unsigned int light_flags[4];
    unsigned int render_flags;
    LightCreateFunc* create_func;
    tig_art_id_t art_id;
    Light* light;
    tig_color_t color;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    TigRect updated_rect;
    TigRect tmp_rect;
    LightCreateInfo create_info;
    int overlay;

    light_flags[0] = LF_OVERLAY_0;
    light_flags[1] = LF_OVERLAY_1;
    light_flags[2] = LF_OVERLAY_2;
    light_flags[3] = LF_OVERLAY_3;

    render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
    if ((render_flags & ORF_80000000) != 0) {
        return;
    }

    create_func = a2 ? sub_4D9310 : sub_4DE870;

    art_id = obj_field_int32_get(obj, OBJ_F_LIGHT_AID);
    light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
    if (art_id != TIG_ART_ID_INVALID) {
        color = obj_field_int32_get(obj, OBJ_F_LIGHT_COLOR);
        light_get_color_components(color, &r, &g, &b);
        if (light != NULL) {
            light_get_rect(light, &updated_rect);
            light_set_aid(light, art_id);
            light_set_flags_internal(light, obj_field_int32_get(obj, OBJ_F_LIGHT_FLAGS));
            light_set_custom_color(light, r, g, b);
            light_get_rect(light, &tmp_rect);
            tig_rect_union(&updated_rect, &tmp_rect, &updated_rect);
            light_invalidate_rect(&updated_rect, true);
        } else {
            create_info.obj = obj;
            create_info.loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
            create_info.offset_x = obj_field_int32_get(obj, OBJ_F_OFFSET_X);
            create_info.offset_y = obj_field_int32_get(obj, OBJ_F_OFFSET_Y);
            create_info.art_id = art_id;
            create_info.flags = obj_field_int32_get(obj, OBJ_F_LIGHT_FLAGS) | LF_08000000;
            create_info.r = r;
            create_info.g = g;
            create_info.b = b;
            create_func(&create_info, &light);
            obj_field_ptr_set(obj, OBJ_F_LIGHT_HANDLE, light);
        }
    } else {
        if (light != NULL) {
            sub_4D93B0(light);
        }
    }

    for (overlay = 0; overlay < 4; overlay++) {
        art_id = obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_AID, overlay);
        light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, overlay);
        if (art_id != TIG_ART_ID_INVALID) {
            color = obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_COLOR, overlay);
            light_get_color_components(color, &r, &g, &b);
            if (light != NULL) {
                light_get_rect(light, &updated_rect);
                light_set_aid(light, art_id);
                light_set_flags_internal(light, obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_FLAGS, overlay));
                light_set_custom_color(light, r, g, b);
                light_get_rect(light, &tmp_rect);
                tig_rect_union(&updated_rect, &tmp_rect, &updated_rect);
                light_invalidate_rect(&updated_rect, true);
            } else {
                create_info.obj = obj;
                create_info.loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
                create_info.offset_x = obj_field_int32_get(obj, OBJ_F_OFFSET_X);
                create_info.offset_y = obj_field_int32_get(obj, OBJ_F_OFFSET_Y);
                create_info.art_id = art_id;
                create_info.flags = obj_field_int32_get(obj, OBJ_F_LIGHT_FLAGS) | light_flags[overlay];
                create_info.r = r;
                create_info.g = g;
                create_info.b = b;
                create_func(&create_info, &light);
                obj_arrayfield_ptr_set(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, overlay, light);
            }
        } else {
            if (light != NULL) {
                sub_4D93B0(light);
            }
        }
    }

    obj_field_int32_set(obj, OBJ_F_RENDER_FLAGS, render_flags | ORF_80000000);
}

// 0x4D9990
void sub_4D9990(int64_t obj)
{
    Light* light;
    unsigned int color;
    int index;

    light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
    if (light != NULL) {
        obj_field_int32_set(obj, OBJ_F_LIGHT_FLAGS, light->flags);
        obj_field_int32_set(obj, OBJ_F_LIGHT_AID, light->art_id);
        light_build_color(light->r, light->g, light->b, &color);
        obj_field_int32_set(obj, OBJ_F_LIGHT_COLOR, color);
    } else {
        obj_field_int32_set(obj, OBJ_F_LIGHT_AID, TIG_ART_ID_INVALID);
    }

    for (index = 0; index < 4; index++) {
        light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index);
        if (light != NULL) {
            obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_FLAGS, index, light->flags);
            obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_AID, index, light->art_id);
            light_build_color(light->r, light->g, light->b, &color);
            obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_COLOR, index, color);
        } else {
            obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_AID, index, TIG_ART_ID_INVALID);
        }
    }
}

// 0x4D9A90
void sub_4D9A90(int64_t obj)
{
    unsigned int render_flags;
    Light* light;
    int index;

    render_flags = obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS);
    if ((render_flags & ORF_80000000) != 0) {
        light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
        if (light != NULL) {
            sub_4D93B0(light);
        }

        for (index = 0; index < 4; index++) {
            light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index);
            if (light != NULL) {
                sub_4D93B0(light);
            }
        }

        obj_field_int32_set(obj,
            OBJ_F_RENDER_FLAGS,
            render_flags & ~ORF_80000000);
    }
}

// 0x4D9B20
bool shadow_apply(int64_t obj)
{
    tig_art_id_t art_id;
    int64_t loc;
    int64_t loc_x;
    int64_t loc_y;
    tig_color_t color;
    LocRect loc_rect;
    SectorListNode* head;
    SectorListNode* node;
    Sector* sector;
    SectorBlockListNode* light_node;
    Light* light;
    uint8_t color_index;
    int lx;
    int ly;
    int cnt = 0;
    Shadow shadows[SHADOW_HANDLE_MAX - 1];
    int frames[SHADOW_HANDLE_MAX - 1];
    Shadow* shadow;
    int palette;
    int idx;

    art_id = obj_field_int32_get(obj, OBJ_F_SHADOW);
    if (art_id == TIG_ART_ID_INVALID) {
        return false;
    }

    if (obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))
        && critter_is_concealed(obj)) {
        return false;
    }

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    if (tig_art_tile_id_type_get(tile_art_id_at(loc)) == 0) {
        color = light_get_indoor_color();
    } else {
        color = light_get_outdoor_color();
    }

    uint8_t gray = tig_color_red_grayscale_table[(color & tig_color_red_mask) >> tig_color_red_shift]
        + tig_color_green_grayscale_table[(color & tig_color_green_mask) >> tig_color_green_shift]
        + tig_color_blue_grayscale_table[(color & tig_color_blue_mask) >> tig_color_blue_shift];

    if (light_shadows_enabled) {
        location_xy(loc, &loc_x, &loc_y);
        loc_x += obj_field_int32_get(obj, OBJ_F_OFFSET_X) + 40 - stru_602ED8.width / 2;
        loc_y += obj_field_int32_get(obj, OBJ_F_OFFSET_Y) + 20 - stru_602ED8.height / 2;

        if (loc_x < INT_MIN
            || loc_x > INT_MAX
            || loc_y < INT_MIN
            || loc_y > INT_MAX) {
            return false;
        }

        stru_602ED8.x = (int)loc_x;
        stru_602ED8.y = (int)loc_y;

        if (!sub_4B9130(&stru_602ED8, &loc_rect)) {
            return false;
        }

        head = sector_list_create(&loc_rect);

        node = head;
        while (node != NULL) {
            if (cnt >= SHADOW_HANDLE_MAX - 1) {
                break;
            }

            if (sector_lock(node->sec, &sector)) {
                light_node = sector->lights.head;
                while (light_node != NULL) {
                    if (cnt >= SHADOW_HANDLE_MAX - 1) {
                        break;
                    }

                    light = (Light*)light_node->data;
                    if ((light->flags & 0x43) == 0
                        && light->obj != obj) {
                        location_xy(light->loc, &loc_x, &loc_y);
                        lx = (int)loc_x + light->offset_x + 40;
                        ly = (int)loc_y + light->offset_y + 20;

                        if (lx >= stru_602ED8.x
                            && ly >= stru_602ED8.y
                            && lx < stru_602ED8.x + stru_602ED8.width
                            && ly < stru_602ED8.y + stru_602ED8.height) {
                            color_index = ((uint8_t*)light_shadowmap_bmp.pixels)[light_shadowmap_bmp.pitch * (ly - stru_602ED8.y) + lx - stru_602ED8.x];
                            if (color_index != 0) {
                                uint8_t light_gray = tig_color_red_grayscale_table[(light->tint_color & tig_color_red_mask) >> tig_color_red_shift]
                                    + tig_color_green_grayscale_table[(light->tint_color & tig_color_green_mask) >> tig_color_green_shift]
                                    + tig_color_blue_grayscale_table[(light->tint_color & tig_color_blue_mask) >> tig_color_blue_shift];
                                int delta = light_gray - gray;
                                if (delta > 0) {
                                    int v1 = (int)((float)delta * 0.4f);
                                    int frame = color_index - 32;

                                    shadows[cnt].art_id = sub_504730(art_id, (frame / 7 + 16) % 32);

                                    frame %= 7;
                                    frames[cnt] = frame;

                                    shadows[cnt].palette = dword_602E58[frame];
                                    shadows[cnt].art_id = tig_art_id_frame_set(shadows[cnt].art_id, frame);

                                    int shadow_grey = v1 - frame * (v1 / 7);
                                    shadows[cnt].color = tig_color_make(shadow_grey, shadow_grey, shadow_grey);

                                    cnt++;
                                }
                            }
                        }
                    }
                    light_node = light_node->next;
                }
                sector_unlock(node->sec);
            }
            node = node->next;
        }
        sector_list_destroy(head);

        if (cnt != 0) {
            for (int i = 0; i < cnt - 1; i++) {
                for (int j = 0; j < cnt - i - 1; j++) {
                    if (frames[j] > frames[j + 1]) {
                        int tmp_frame = frames[j];
                        frames[j] = frames[j + 1];
                        frames[j + 1] = tmp_frame;

                        Shadow tmp_shadow = shadows[j];
                        shadows[j] = shadows[j + 1];
                        shadows[j + 1] = tmp_shadow;
                    }
                }
            }

            palette = 8 - frames[0];
        } else {
            palette = 2;
        }
    } else {
        palette = 2;
    }

    idx = 0;
    if (palette < 7) {
        shadow = (Shadow*)obj_arrayfield_ptr_get(obj, OBJ_F_SHADOW_HANDLES, idx);
        if (shadow == NULL) {
            shadow = shadow_node_allocate();
        }

        shadow->art_id = sub_504730(art_id, 2);
        shadow->palette = dword_602E58[palette];
        shadow->color = tig_color_make((int)((float)gray * 0.4f), (int)((float)gray * 0.4f), (int)((float)gray * 0.4f));
        obj_arrayfield_ptr_set(obj, OBJ_F_SHADOW_HANDLES, idx, shadow);
        idx++;
    }

    for (int i = 0; i < cnt; i++) {
        shadow = (Shadow*)obj_arrayfield_ptr_get(obj, OBJ_F_SHADOW_HANDLES, idx);
        if (shadow == NULL) {
            shadow = shadow_node_allocate();
        }

        shadow->art_id = shadows[i].art_id;
        shadow->palette = shadows[i].palette;
        shadow->color = shadows[i].color;

        obj_arrayfield_ptr_set(obj, OBJ_F_SHADOW_HANDLES, idx, shadow);
        idx++;
    }

    while (idx < SHADOW_HANDLE_MAX) {
        shadow = (Shadow*)obj_arrayfield_ptr_get(obj, OBJ_F_SHADOW_HANDLES, idx);
        if (shadow != NULL) {
            shadow_node_deallocate(shadow);
            obj_arrayfield_ptr_set(obj, OBJ_F_SHADOW_HANDLES, idx, NULL);
        }
        idx++;
    }

    return true;
}

// 0x4DA310
void shadow_clear(int64_t obj)
{
    int index;
    Shadow* shadow;

    for (index = 0; index < SHADOW_HANDLE_MAX; index++) {
        shadow = (Shadow*)obj_arrayfield_ptr_get(obj, OBJ_F_SHADOW_HANDLES, index);
        if (shadow == NULL) {
            break;
        }

        shadow_node_deallocate(shadow);
        obj_arrayfield_ptr_set(obj, OBJ_F_SHADOW_HANDLES, index, NULL);
    }
}

// 0x4DA360
bool sub_4DA360(int x, int y, tig_color_t color, tig_color_t* colors)
{
    int dx;
    int dy;
    int idx;

    light_buffers_lock();

    dx = (x - dword_602ED0) / 40;
    dy = (y - dword_602ED4) / 20;

    if (dx < 0
        || dx + 2 >= dword_603418
        || dy < 0
        || dy + 2 >= dword_60341C) {
        light_buffers_unlock();
        return false;
    }

    if (light_bpp != 8) {
        switch (light_bpp) {
        case 16:
        case 24:
            // TODO: Incomplete.
            break;
        case 32:
            colors[0] = tig_color_add(lighter_colors[dx + dy * lighter_pitch], color);
            colors[0] = tig_color_sub(darker_colors[dx + dy * darker_pitch], colors[0]);

            colors[1] = tig_color_add(lighter_colors[dx + 1 + dy * lighter_pitch], color);
            colors[1] = tig_color_sub(darker_colors[dx + 1 + dy * darker_pitch], colors[1]);

            colors[2] = tig_color_add(lighter_colors[dx + 2 + dy * lighter_pitch], color);
            colors[2] = tig_color_sub(darker_colors[dx + 2 + dy * darker_pitch], colors[2]);

            colors[3] = tig_color_add(lighter_colors[dx + (dy + 1) * lighter_pitch], color);
            colors[3] = tig_color_sub(darker_colors[dx + (dy + 1) * darker_pitch], colors[3]);

            colors[4] = tig_color_add(lighter_colors[dx + 1 + (dy + 1) * lighter_pitch], color);
            colors[4] = tig_color_sub(darker_colors[dx + 1 + (dy + 1) * darker_pitch], colors[4]);

            colors[5] = tig_color_add(lighter_colors[dx + 2 + (dy + 1) * lighter_pitch], color);
            colors[5] = tig_color_sub(darker_colors[dx + 2 + (dy + 1) * darker_pitch], colors[5]);

            colors[6] = tig_color_add(lighter_colors[dx + (dy + 2) * lighter_pitch], color);
            colors[6] = tig_color_sub(darker_colors[dx + (dy + 2) * darker_pitch], colors[6]);

            colors[7] = tig_color_add(lighter_colors[dx + 1 + (dy + 2) * lighter_pitch], color);
            colors[7] = tig_color_sub(darker_colors[dx + 1 + (dy + 2) * darker_pitch], colors[7]);

            colors[8] = tig_color_add(lighter_colors[dx + 2 + (dy + 2) * lighter_pitch], color);
            colors[8] = tig_color_sub(darker_colors[dx + 2 + (dy + 2) * darker_pitch], colors[8]);
            break;
        }
    }

    light_buffers_unlock();

    for (idx = 0; idx < 8; idx++) {
        if (colors[idx] != colors[idx + 1]) {
            break;
        }
    }

    return idx != 8;
}

// 0x4DC210
void sub_4DC210(int64_t obj, int* colors, int* cnt_ptr)
{
    int64_t loc;
    int obj_type;
    int rot;
    int offset_x;
    int offset_y;
    tig_color_t color;

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);

    if (obj_type == OBJ_TYPE_WALL) {
        tig_color_t indoor_color = light_get_indoor_color();
        tig_color_t outdoor_color = light_get_outdoor_color();
        tig_color_t tile_color = tig_art_tile_id_type_get(tile_art_id_at(loc)) == 0
            ? indoor_color
            : outdoor_color;

        tig_art_id_t art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        rot = tig_art_id_rotation_get(art_id);
        if ((rot & 1) != 0) {
            rot--;
        }

        int64_t loc_x;
        int64_t loc_y;
        location_xy(loc, &loc_x, &loc_y);
        int v87 = (int)LOCATION_GET_X(loc);
        int v88 = (int)LOCATION_GET_Y(loc);

        int v1;
        switch (rot) {
        case 0:
            loc_x += 40;
            v1 = 1;
            break;
        case 2:
            tile_color = outdoor_color;
            loc_x += 40;
            loc_y += 39;
            v1 = -1;
            break;
        case 4:
            tile_color = outdoor_color;
            loc_y += 20;
            v1 = 1;
            break;
        case 6:
            loc_y += 20;
            v1 = -1;
            break;
        default:
            // Should be unreachable.
            assert(0);
        }

        // FIXME: Useless.
        tig_art_wall_id_p_piece_get(art_id);

        TigRect obj_rect;
        object_get_rect(obj, RECT_FLAG_NONE, &obj_rect);

        loc_y += (obj_rect.x - loc_x) * v1;
        loc_x = obj_rect.x;

        if ((obj_rect.width & 1) != 0) {
            obj_rect.width++;
        }

        obj_rect.height = obj_rect.width / 2;
        obj_rect.y = v1 > 0 ? (int)loc_y : (int)loc_y - obj_rect.height + 1;

        for (int x = 0; x < obj_rect.width; x++) {
            colors[x] = tile_color;
        }

        TigRect rect;
        rect.x = obj_rect.x - 768;
        rect.y = obj_rect.y - 512;
        rect.width = obj_rect.width + 1536;
        rect.height = obj_rect.height + 1024;

        LocRect loc_rect;
        if (sub_4B9130(&rect, &loc_rect)) {
            SectorListNode* v2 = sector_list_create(&loc_rect);
            SectorListNode* curr = v2;
            while (curr != NULL) {
                Sector* sector;
                if (sector_lock(curr->sec, &sector)) {
                    SectorBlockListNode* node = sector->lights.head;
                    while (node != NULL) {
                        Light* light = (Light*)node->data;
                        if ((light->flags & LF_OFF) == 0) {
                            int light_x = (int)LOCATION_GET_X(light->loc);
                            int light_y = (int)LOCATION_GET_Y(light->loc);

                            if ((rot == 0 && light_x >= v87)
                                || (rot == 2 && light_y > v88)
                                || (rot == 4 && light_x > v87)
                                || (rot == 6 && light_y >= v88)) {

                                TigRect light_rect;
                                light_get_rect_internal(light, &light_rect);

                                TigRect affected_rect;
                                TigArtAnimData art_anim_data;
                                if (tig_rect_intersection(&light_rect, &obj_rect, &affected_rect) == TIG_OK
                                    && tig_art_anim_data(light->art_id, &art_anim_data) == TIG_OK) {
                                    int tmp_x = (int)loc_x;
                                    int tmp_y = (int)loc_y;

                                    for (int x = 0; x < affected_rect.width; x++) {
                                        if (tmp_x >= affected_rect.x
                                            && tmp_y >= affected_rect.y
                                            && tmp_x < affected_rect.x + affected_rect.width
                                            && tmp_y < affected_rect.y + affected_rect.height
                                            && sub_502E50(light->art_id, tmp_x - light_rect.x, tmp_y - light_rect.y, &color) == TIG_OK
                                            && color != art_anim_data.color_key) {
                                            if (((light->flags & LF_INDOOR) != 0
                                                    && light->tint_color != indoor_color)
                                                || ((light->flags & LF_OUTDOOR) != 0
                                                    && light->tint_color != outdoor_color)) {
                                                sub_4DE390(light);
                                                light_invalidate_rect(&obj_rect, 1);
                                            }

                                            switch (light_bpp) {
                                            case 8:
                                                break;
                                            case 16:
                                            case 24:
                                                // TODO: Incomplete.
                                                break;
                                            case 32:
                                                if (light->palette != NULL) {
                                                    color = tig_color_mul(light->tint_color, color);
                                                }

                                                if ((light->flags & LF_DARK) != 0) {
                                                    colors[x] = tig_color_sub(color, colors[x]);
                                                } else {
                                                    colors[x] = tig_color_add(color, colors[x]);
                                                }
                                                break;
                                            }
                                        }

                                        if ((x & 1) != 0) {
                                            tmp_y += v1;
                                        }

                                        tmp_x++;
                                    }
                                }
                            }
                        }
                        node = node->next;
                    }
                    sector_unlock(curr->sec);
                }
                curr = curr->next;
            }

            sector_list_destroy(v2);
        }

        if (dword_60340C) {
            colors[1] = colors[obj_rect.width - 1];
            if (colors[0] != colors[1]) {
                *cnt_ptr = 2;
            } else {
                *cnt_ptr = 1;
            }
        } else {
            int idx;

            for (idx = 0; idx < obj_rect.width - 2; idx++) {
                if (colors[idx] != colors[idx + 1]) {
                    break;
                }
            }

            if (idx != obj_rect.width - 2) {
                *cnt_ptr = 2;
            } else if (colors[0] != outdoor_color) {
                *cnt_ptr = 1;
            } else {
                *cnt_ptr = 0;
            }
        }
    } else {
        if (obj_type == OBJ_TYPE_PORTAL) {
            rot = tig_art_id_rotation_get(obj_field_int32_get(obj, OBJ_F_CURRENT_AID));
            if ((rot & 1) == 0) {
                rot++;
            }

            switch (rot) {
            case 1:
                offset_x = 20;
                offset_y = -10;
                break;
            case 3:
                offset_x = -20;
                offset_y = -10;
                location_in_dir(loc, 3, &loc);
                break;
            case 5:
                offset_x = 20;
                offset_y = -10;
                location_in_dir(loc, 5, &loc);
                break;
            default:
                offset_x = -20;
                offset_y = -10;
                break;
            }
        } else {
            offset_x = obj_field_int32_get(obj, OBJ_F_OFFSET_X);
            offset_y = obj_field_int32_get(obj, OBJ_F_OFFSET_Y);
        }

        if (sub_4D89E0(loc, offset_x, offset_y, true, &color)) {
            if (color != light_get_outdoor_color()) {
                *cnt_ptr = 1;
            } else {
                *cnt_ptr = 0;
            }
            colors[0] = color;
        } else {
            *cnt_ptr = 0;
        }
    }
}

// 0x4DCE10
uint8_t sub_4DCE10(int64_t obj)
{
    tig_color_t color;

    if ((obj_field_int32_get(obj, OBJ_F_RENDER_FLAGS) & ORF_02000000) == 0) {
        sub_442520(obj);
    }

    color = obj_field_int32_get(obj, OBJ_F_COLOR);

    // TODO: Check, probably inlining.
    return tig_color_rgb_to_grayscale(color);
}

// 0x4DCEA0
void light_set_flags(int64_t obj, unsigned int flags)
{
    Light* light;
    int idx;

    light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
    if (light != NULL) {
        light_set_flags_internal(light, flags);
    } else {
        if (obj_field_int32_get(obj, OBJ_F_LIGHT_AID) != TIG_ART_ID_INVALID) {
            obj_field_int32_set(obj,
                OBJ_F_LIGHT_FLAGS,
                obj_field_int32_get(obj, OBJ_F_LIGHT_FLAGS) | flags);
        }
    }

    for (idx = 0; idx < 4; idx++) {
        light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, idx);
        if (light != NULL) {
            light_set_flags_internal(light, flags);
        } else {
            if (obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_AID, idx) != TIG_ART_ID_INVALID) {
                obj_arrayfield_uint32_set(obj,
                    OBJ_F_OVERLAY_LIGHT_FLAGS,
                    idx,
                    obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_FLAGS, idx) | flags);
            }
        }
    }
}

// 0x4DCF60
void light_unset_flags(int64_t obj, unsigned int flags)
{
    Light* light;
    int idx;

    light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
    if (light != NULL) {
        light_unset_flags_internal(light, flags);
    } else {
        if (obj_field_int32_get(obj, OBJ_F_LIGHT_AID) != TIG_ART_ID_INVALID) {
            obj_field_int32_set(obj,
                OBJ_F_LIGHT_FLAGS,
                obj_field_int32_get(obj, OBJ_F_LIGHT_FLAGS) & ~flags);
        }
    }

    for (idx = 0; idx < 4; idx++) {
        light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, idx);
        if (light != NULL) {
            light_unset_flags_internal(light, flags);
        } else {
            if (obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_AID, idx) != TIG_ART_ID_INVALID) {
                obj_arrayfield_uint32_set(obj,
                    OBJ_F_OVERLAY_LIGHT_FLAGS,
                    idx,
                    obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_FLAGS, idx) & ~flags);
            }
        }
    }
}

// 0x4DD020
void sub_4DD020(int64_t obj, int64_t loc, int offset_x, int offset_y)
{
    Light* light;
    int idx;

    light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
    if (light != NULL) {
        sub_4DD320(light, loc, offset_x, offset_y);
    }

    for (idx = 0; idx < 4; idx++) {
        light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, idx);
        if (light != NULL) {
            sub_4DD320(light, loc, offset_x, offset_y);
        }
    }
}

// 0x4DD0A0
void sub_4DD0A0(int64_t obj, int offset_x, int offset_y)
{
    Light* light;
    int idx;

    light = (Light*)obj_field_ptr_get(obj, OBJ_F_LIGHT_HANDLE);
    if (light != NULL) {
        sub_4DD500(light, offset_x, offset_y);
    }

    for (idx = 0; idx < 4; idx++) {
        light = (Light*)obj_arrayfield_ptr_get(obj, OBJ_F_OVERLAY_LIGHT_HANDLES, idx);
        if (light != NULL) {
            sub_4DD500(light, offset_x, offset_y);
        }
    }
}

// 0x4DD110
bool sub_4DD110(Light* light)
{
    return light->obj == OBJ_HANDLE_NULL;
}

// 0x4DD130
void light_get_rect(Light* light, TigRect* rect)
{
    light_get_rect_internal(light, rect);
}

// 0x4DD150
void light_set_flags_internal(Light* light, unsigned int flags)
{
    bool v1;
    int overlay;
    TigRect rect;

    v1 = false;
    if ((flags & LF_OUTDOOR) != 0) {
        v1 = true;
        flags &= ~LF_OUTDOOR;
        light->flags &= ~LF_INDOOR;
        light->flags |= LF_OUTDOOR;
    }
    if ((flags & LF_INDOOR) != 0) {
        v1 = true;
        flags &= ~LF_INDOOR;
        light->flags &= ~LF_OUTDOOR;
        light->flags |= LF_INDOOR;
    }

    light->flags |= flags;
    light->flags |= LF_MODIFIED;

    if (light->obj != OBJ_HANDLE_NULL) {
        if ((light->flags & LF_08000000) != 0) {
            obj_field_int32_set(light->obj, OBJ_F_LIGHT_FLAGS, light->flags);
        } else {
            if ((light->flags & LF_OVERLAY_0) != 0) {
                overlay = 0;
            } else if ((light->flags & LF_OVERLAY_1) != 0) {
                overlay = 1;
            } else if ((light->flags & LF_OVERLAY_2) != 0) {
                overlay = 2;
            } else if ((light->flags & LF_OVERLAY_3) != 0) {
                overlay = 3;
            } else {
                // Should be unreachable.
                assert(0);
            }

            obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_FLAGS, overlay, light->flags);
        }
    }

    if (v1) {
        sub_4DE390(light);
    }

    light_get_rect_internal(light, &rect);
    light_invalidate_rect(&rect, true);
}

// 0x4DD230
void light_unset_flags_internal(Light* light, unsigned int flags)
{
    bool v1;
    int overlay;
    TigRect rect;

    v1 = false;
    if ((flags & LF_OUTDOOR) != 0) {
        v1 = true;
        flags &= ~LF_OUTDOOR;
        light->flags &= ~LF_OUTDOOR;
    }
    if ((flags & LF_INDOOR) != 0) {
        v1 = true;
        flags &= ~LF_INDOOR;
        light->flags &= ~LF_INDOOR;
    }

    light->flags &= ~flags;
    light->flags |= LF_MODIFIED;

    if (light->obj != OBJ_HANDLE_NULL) {
        if ((light->flags & LF_08000000) != 0) {
            obj_field_int32_set(light->obj, OBJ_F_LIGHT_FLAGS, light->flags);
        } else {
            if ((light->flags & LF_OVERLAY_0) != 0) {
                overlay = 0;
            } else if ((light->flags & LF_OVERLAY_1) != 0) {
                overlay = 1;
            } else if ((light->flags & LF_OVERLAY_2) != 0) {
                overlay = 2;
            } else if ((light->flags & LF_OVERLAY_3) != 0) {
                overlay = 3;
            } else {
                // Should be unreachable.
                assert(0);
            }

            obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_FLAGS, overlay, light->flags);
        }
    }

    if (v1) {
        sub_4DE390(light);
    }

    light_get_rect_internal(light, &rect);
    light_invalidate_rect(&rect, true);
}

// 0x4DD310
unsigned int light_get_flags(Light* light)
{
    return light->flags;
}

// 0x4DD320
void sub_4DD320(Light* light, int64_t loc, int offset_x, int offset_y)
{
    TigRect dirty_rect;
    TigRect updated_rect;
    int64_t old_sector_id;
    int64_t new_sector_id;
    Sector* sector;
    bool v1;

    if (light->loc == loc) {
        sub_4DE4F0(light, offset_x, offset_y);
        return;
    }

    light_get_rect_internal(light, &dirty_rect);

    old_sector_id = sector_id_from_loc(light->loc);
    new_sector_id = sector_id_from_loc(loc);
    v1 = sub_4DD110(light);

    if (old_sector_id == new_sector_id) {
        if (v1 || sub_4D04E0(new_sector_id)) {
            sector_lock(new_sector_id, &sector);
            sector_light_list_update(&(sector->lights), light, loc, offset_x, offset_y);
            sector_unlock(new_sector_id);
        } else {
            light_set_location(light, loc);
            light_set_offset(light, offset_x, offset_y);
        }
    } else {
        if (v1 || sub_4D04E0(new_sector_id)) {
            sector_lock(old_sector_id, &sector);
            sector_light_list_remove(&(sector->lights), light);
            sector_unlock(old_sector_id);
        }

        light_set_location(light, loc);
        light_set_offset(light, offset_x, offset_y);

        if (v1 || sub_4D04E0(old_sector_id)) {
            sector_lock(new_sector_id, &sector);
            sector_light_list_add(&(sector->lights), light);
            sector_unlock(new_sector_id);
        }
    }

    light->flags |= LF_MODIFIED;

    light_get_rect_internal(light, &updated_rect);
    tig_rect_union(&dirty_rect, &updated_rect, &dirty_rect);
    light_invalidate_rect(&dirty_rect, true);
}

// 0x4DD500
void sub_4DD500(Light* light, int offset_x, int offset_y)
{
    sub_4DE4F0(light, offset_x, offset_y);
}

// 0x4DD720
void light_inc_frame(Light* light)
{
    TigRect dirty_rect;
    TigRect updated_rect;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    int overlay;

    light_get_rect_internal(light, &dirty_rect);

    art_id = tig_art_id_frame_inc(light->art_id);
    if (light->art_id != art_id) {
        light->art_id = art_id;
        if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
            if (art_frame_data.offset_x != 0 || art_frame_data.offset_y != 0) {
                light->offset_x += art_frame_data.offset_x;
                light->offset_y += art_frame_data.offset_y;
            }
        }

        light->flags |= LF_MODIFIED;

        if (light->obj != OBJ_HANDLE_NULL) {
            if ((light->flags & LF_08000000) != 0) {
                obj_field_int32_set(light->obj, OBJ_F_LIGHT_AID, light->art_id);
            } else {
                if ((light->flags & LF_OVERLAY_0) != 0) {
                    overlay = 0;
                } else if ((light->flags & LF_OVERLAY_1) != 0) {
                    overlay = 1;
                } else if ((light->flags & LF_OVERLAY_2) != 0) {
                    overlay = 2;
                } else if ((light->flags & LF_OVERLAY_3) != 0) {
                    overlay = 3;
                } else {
                    // Should be unreachable.
                    assert(0);
                }

                obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_AID, overlay, light->art_id);
            }
        }

        light_get_rect_internal(light, &updated_rect);
        tig_rect_union(&dirty_rect, &updated_rect, &dirty_rect);
        light_invalidate_rect(&dirty_rect, true);
    }
}

// 0x4DD830
void light_dec_frame(Light* light)
{
    TigRect dirty_rect;
    TigRect updated_rect;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    int overlay;

    light_get_rect_internal(light, &dirty_rect);

    art_id = tig_art_id_frame_dec(light->art_id);
    if (light->art_id != art_id) {
        light->art_id = art_id;
        if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
            if (art_frame_data.offset_x != 0 || art_frame_data.offset_y != 0) {
                light->offset_x += art_frame_data.offset_x;
                light->offset_y += art_frame_data.offset_y;
            }
        }

        light->flags |= LF_MODIFIED;

        if (light->obj != OBJ_HANDLE_NULL) {
            if ((light->flags & LF_08000000) != 0) {
                obj_field_int32_set(light->obj, OBJ_F_LIGHT_AID, light->art_id);
            } else {
                if ((light->flags & LF_OVERLAY_0) != 0) {
                    overlay = 0;
                } else if ((light->flags & LF_OVERLAY_1) != 0) {
                    overlay = 1;
                } else if ((light->flags & LF_OVERLAY_2) != 0) {
                    overlay = 2;
                } else if ((light->flags & LF_OVERLAY_3) != 0) {
                    overlay = 3;
                } else {
                    // Should be unreachable.
                    assert(0);
                }

                obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_AID, overlay, light->art_id);
            }
        }

        light_get_rect_internal(light, &updated_rect);
        tig_rect_union(&dirty_rect, &updated_rect, &dirty_rect);
        light_invalidate_rect(&dirty_rect, true);
    }
}

// 0x4DD940
void light_cycle_rotation(Light* light)
{
    TigRect dirty_rect;
    TigRect updated_rect;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    int overlay;

    light_get_rect_internal(light, &dirty_rect);

    art_id = tig_art_id_rotation_inc(light->art_id);
    if (light->art_id != art_id) {
        light->art_id = art_id;
        if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
            if (art_frame_data.offset_x != 0 || art_frame_data.offset_y != 0) {
                light->offset_x += art_frame_data.offset_x;
                light->offset_y += art_frame_data.offset_y;
            }
        }

        light->flags |= LF_MODIFIED;

        if (light->obj != OBJ_HANDLE_NULL) {
            if ((light->flags & LF_08000000) != 0) {
                obj_field_int32_set(light->obj, OBJ_F_LIGHT_AID, light->art_id);
            } else {
                if ((light->flags & LF_OVERLAY_0) != 0) {
                    overlay = 0;
                } else if ((light->flags & LF_OVERLAY_1) != 0) {
                    overlay = 1;
                } else if ((light->flags & LF_OVERLAY_2) != 0) {
                    overlay = 2;
                } else if ((light->flags & LF_OVERLAY_3) != 0) {
                    overlay = 3;
                } else {
                    // Should be unreachable.
                    assert(0);
                }

                obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_AID, overlay, light->art_id);
            }
        }

        light_get_rect_internal(light, &updated_rect);
        tig_rect_union(&dirty_rect, &updated_rect, &dirty_rect);
        light_invalidate_rect(&dirty_rect, true);
    }
}

// 0x4DDA40
void light_set_location(Light* light, int64_t loc)
{
    light->loc = loc;
    light->flags |= LF_MODIFIED;
}

// 0x4DDA60
int64_t light_get_location(Light* light)
{
    return light->loc;
}

// 0x4DDA70
void light_set_offset(Light* light, int offset_x, int offset_y)
{
    light->offset_x = offset_x;
    light->offset_y = offset_y;
    light->flags |= LF_MODIFIED;
}

// 0x4DDA90
void light_get_offset(Light* light, int* offset_x, int* offset_y)
{
    *offset_x = light->offset_x;
    *offset_y = light->offset_y;
}

// 0x4DDAB0
void light_set_aid(Light* light, tig_art_id_t art_id)
{
    light->art_id = art_id;
    light->flags |= LF_MODIFIED;

    if (light->obj != OBJ_HANDLE_NULL) {
        if ((light->flags & LF_08000000) != 0) {
            obj_field_int32_set(light->obj, OBJ_F_LIGHT_AID, art_id);
        } else if ((light->flags & LF_OVERLAY_0) != 0) {
            obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_AID, 0, art_id);
        } else if ((light->flags & LF_OVERLAY_1) != 0) {
            obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_AID, 1, art_id);
        } else if ((light->flags & LF_OVERLAY_2) != 0) {
            obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_AID, 2, art_id);
        } else if ((light->flags & LF_OVERLAY_3) != 0) {
            obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_AID, 3, art_id);
        }
    }
}

// 0x4DDB70
tig_art_id_t light_get_aid(Light* light)
{
    return light->art_id;
}

// 0x4DDB80
void light_set_custom_color(Light* light, uint8_t r, uint8_t g, uint8_t b)
{
    unsigned int color;
    int overlay;

    if ((light->flags & LF_INDOOR) != 0) {
        light_unset_flags_internal(light, LF_INDOOR);
    }

    if ((light->flags & LF_OUTDOOR) != 0) {
        light_unset_flags_internal(light, LF_OUTDOOR);
    }

    light->flags |= LF_MODIFIED;

    if (light->obj != OBJ_HANDLE_NULL) {
        light_build_color(r, g, b, &color);

        if ((light->flags & LF_08000000) != 0) {
            obj_field_int32_set(light->obj, OBJ_F_LIGHT_COLOR, color);
        } else {
            if ((light->flags & LF_OVERLAY_0) != 0) {
                overlay = 0;
            } else if ((light->flags & LF_OVERLAY_1) != 0) {
                overlay = 1;
            } else if ((light->flags & LF_OVERLAY_2) != 0) {
                overlay = 2;
            } else if ((light->flags & LF_OVERLAY_3) != 0) {
                overlay = 3;
            } else {
                // Should be unreachable.
                assert(0);
            }

            obj_arrayfield_uint32_set(light->obj, OBJ_F_OVERLAY_LIGHT_COLOR, overlay, color);
        }
    }

    light->r = r;
    light->g = g;
    light->b = b;
    sub_4DE390(light);
}

// 0x4DDD20
bool light_read(TigFile* stream, Light** light_ptr)
{
    Light* light;
    LightSerializedData serialized;

    light = (Light*)MALLOC(sizeof(*light));

    if (tig_file_fread(&serialized, sizeof(serialized), 1, stream) != 1) {
        return false;
    }

    light->obj = serialized.obj;
    light->loc = serialized.loc;
    light->offset_x = serialized.offset_x;
    light->offset_y = serialized.offset_y;
    light->flags = serialized.flags;
    light->art_id = serialized.art_id;
    light->r = serialized.r;
    light->b = serialized.b;
    light->g = serialized.g;
    light->tint_color = serialized.tint_color;
    light->palette = NULL;

    sub_4DE390(light);

    *light_ptr = light;

    return true;
}

// 0x4DDD70
bool light_write(TigFile* stream, Light* light)
{
    LightSerializedData serialized;

    memset(&serialized, 0, sizeof(serialized));
    serialized.obj = light->obj;
    serialized.loc = light->loc;
    serialized.offset_x = light->offset_x;
    serialized.offset_y = light->offset_y;
    serialized.flags = light->flags;
    serialized.art_id = light->art_id;
    serialized.r = light->r;
    serialized.b = light->b;
    serialized.g = light->g;
    serialized.tint_color = light->tint_color;

    return tig_file_fwrite(&serialized, sizeof(serialized), 1, stream) == 1;
}

// 0x4DDD90
bool sub_4DDD90(Sector* sector)
{
    int hour;
    bool is_day;
    int tile;
    ObjectNode* node;
    unsigned int obj_flags;
    unsigned int render_flags;
    unsigned int scenery_flags;
    bool update;
    Light* light;
    int index;

    hour = datetime_current_hour();
    is_day = hour >= 6 && hour < 18;

    for (tile = 0; tile < 4096; tile++) {
        node = sector->objects.heads[tile];
        while (node != NULL) {
            obj_flags = obj_field_int32_get(node->obj, OBJ_F_FLAGS);

            render_flags = obj_field_int32_get(node->obj, OBJ_F_RENDER_FLAGS);
            render_flags &= ~(ORF_04000000 | ORF_02000000);
            obj_field_int32_set(node->obj, OBJ_F_RENDER_FLAGS, render_flags);

            if (obj_field_int32_get(node->obj, OBJ_F_TYPE) == OBJ_TYPE_SCENERY) {
                scenery_flags = obj_field_int32_get(node->obj, OBJ_F_SCENERY_FLAGS);
                if ((scenery_flags & OSCF_RESPAWNING) == 0
                    && (scenery_flags & OSCF_NOCTURNAL) != 0) {
                    update = false;
                    if (tig_art_tile_id_type_get(sector->tiles.art_ids[tile]) != 0) {
                        if (is_day) {
                            if ((obj_flags & OF_OFF) == 0) {
                                object_flags_set(node->obj, OF_OFF);
                                update = true;
                            }
                        } else {
                            if ((obj_flags & OF_OFF) != 0) {
                                object_flags_unset(node->obj, OF_OFF);
                                update = true;
                            }
                        }
                    } else {
                        if ((obj_flags & OF_OFF) != 0) {
                            object_flags_unset(node->obj, OF_OFF);
                            update = true;
                        }
                    }

                    if (update) {
                        sub_4D9590(node->obj, false);
                        light = (Light*)obj_field_ptr_get(node->obj, OBJ_F_LIGHT_HANDLE);
                        if (light != NULL) {
                            sector_light_list_add(&(sector->lights), light);
                        }

                        for (index = 0; index < 4; index++) {
                            light = (Light*)obj_arrayfield_ptr_get(node->obj, OBJ_F_OVERLAY_LIGHT_HANDLES, index);
                            if (light != NULL) {
                                sector_light_list_add(&(sector->lights), light);
                            }
                        }
                    }
                }
            }

            node = node->next;
        }
    }

    return true;
}

// 0x4DDF20
void shadows_changed()
{
    light_shadows_enabled = settings_get_value(&settings, SHADOWS_KEY);

    if (light_iso_window_invalidate_rect != NULL) {
        light_iso_window_invalidate_rect(NULL);
    }
}

// 0x4DDF50
bool sub_4DDF50()
{
    sub_4DE060();

    dword_602E44 = light_iso_content_rect.width + 320;
    dword_603418 = dword_602E44 / 40 + 1;

    dword_602EA4 = light_iso_content_rect.height + 160;
    dword_60341C = dword_602EA4 / 20 + 1;

    TigVideoBufferCreateInfo vb_create_info;
    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY | TIG_VIDEO_BUFFER_CREATE_COLOR_KEY;
    vb_create_info.width = dword_603418;
    vb_create_info.height = dword_60341C;
    vb_create_info.color_key = tig_color_make(0, 255, 0);
    vb_create_info.background_color = vb_create_info.color_key;

    if (tig_video_buffer_create(&vb_create_info, &darker_vb) != TIG_OK) {
        return false;
    }

    if (tig_video_buffer_create(&vb_create_info, &lighter_vb) != TIG_OK) {
        return false;
    }

    return true;
}

// 0x4DE060
void sub_4DE060()
{
    if (lighter_vb != NULL) {
        tig_video_buffer_destroy(lighter_vb);
        dword_60341C = 0;
        dword_603418 = 0;
        lighter_vb = NULL;
    }

    if (darker_vb != NULL) {
        tig_video_buffer_destroy(darker_vb);
        dword_60341C = 0;
        dword_603418 = 0;
        darker_vb = NULL;
    }
}

// 0x4DE0B0
bool sub_4DE0B0(tig_art_id_t art_id, TigPaletteModifyInfo* modify_info)
{
    if (!dword_602ECC) {
        return false;
    }

    modify_info->flags = TIG_PALETTE_MODIFY_TINT;
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
        if (tig_art_tile_id_type_get(art_id) == 0) {
            modify_info->tint_color = light_indoor_color;
        } else {
            modify_info->tint_color = light_outdoor_color;
        }
        return true;
    case TIG_ART_TYPE_WALL:
        modify_info->tint_color = light_outdoor_color;
        return true;
    case TIG_ART_TYPE_CRITTER:
        modify_info->tint_color = light_outdoor_color;
        return true;
    case TIG_ART_TYPE_PORTAL:
        modify_info->tint_color = light_outdoor_color;
        return true;
    case TIG_ART_TYPE_SCENERY:
        modify_info->tint_color = light_outdoor_color;
        return true;
    case TIG_ART_TYPE_INTERFACE:
        return false;
    case TIG_ART_TYPE_ITEM:
        if (tig_art_item_id_disposition_get(art_id) == TIG_ART_ITEM_DISPOSITION_GROUND) {
            modify_info->tint_color = light_outdoor_color;
            return true;
        }
        return false;
    case TIG_ART_TYPE_CONTAINER:
        modify_info->tint_color = light_outdoor_color;
        return true;
    case TIG_ART_TYPE_MISC:
        return false;
    case TIG_ART_TYPE_LIGHT:
        return false;
    case TIG_ART_TYPE_ROOF:
        modify_info->tint_color = light_outdoor_color;
        return true;
    case TIG_ART_TYPE_FACADE:
        if (tig_art_tile_id_type_get(art_id) == 0) {
            modify_info->tint_color = light_indoor_color;
        } else {
            modify_info->tint_color = light_outdoor_color;
        }
        return true;
    case TIG_ART_TYPE_MONSTER:
        modify_info->tint_color = light_outdoor_color;
        return true;
    case TIG_ART_TYPE_UNIQUE_NPC:
        modify_info->tint_color = light_outdoor_color;
        return true;
    default:
        return false;
    }
}

// 0x4DE200
void sub_4DE200()
{
    if (!dword_60340C) {
        if (light_indoor_palette == NULL) {
            light_indoor_palette = tig_palette_create();
        }
        tig_palette_fill(light_indoor_palette, light_indoor_color);

        if (light_outdoor_palette == NULL) {
            light_outdoor_palette = tig_palette_create();
        }
        tig_palette_fill(light_outdoor_palette, light_outdoor_color);
    }
}

// 0x4DE250
void sub_4DE250()
{
    if (!dword_60340C) {
        tig_palette_destroy(light_indoor_palette);
        light_indoor_palette = NULL;

        tig_palette_destroy(light_outdoor_palette);
        light_outdoor_palette = NULL;
    }
}

// 0x4DE290
void light_get_rect_internal(Light* light, TigRect* rect)
{
    int64_t x;
    int64_t y;
    TigArtFrameData art_frame_data;

    rect->x = 0;
    rect->y = 0;
    rect->width = 0;
    rect->height = 0;

    location_xy(light->loc, &x, &y);
    if (x >= INT_MIN && x < INT_MAX
        && y >= INT_MIN && y < INT_MAX) {
        if (tig_art_frame_data(light->art_id, &art_frame_data) == TIG_OK) {
            rect->x = light->offset_x + (int)x + 40;
            rect->y = light->offset_y + (int)y + 20;
            rect->width = art_frame_data.width;
            rect->height = art_frame_data.height;
            rect->x -= art_frame_data.hot_x;
            rect->y -= art_frame_data.hot_y;
        }
    }
}

// 0x4DE390
void sub_4DE390(Light* light)
{
    tig_color_t color;
    bool have_color;
    TigPaletteModifyInfo palette_modify_info;
    TigArtAnimData art_anim_data;

    if ((light->flags & LF_INDOOR) != 0) {
        color = light_get_indoor_color();
        have_color = true;
    } else if ((light->flags & LF_OUTDOOR) != 0) {
        color = light_get_outdoor_color();
        have_color = true;
    } else {
        have_color = false;
    }

    if (have_color) {
        light->r = (uint8_t)tig_color_get_red(color);
        light->g = (uint8_t)tig_color_get_green(color);
        light->b = (uint8_t)tig_color_get_blue(color);
    }

    light->tint_color = tig_color_make(light->r, light->g, light->b);
    if (light->tint_color == tig_color_make(255, 255, 255)) {
        sub_4DE4D0(light);
    } else {
        if (light->palette == NULL) {
            light->palette = tig_palette_create();
        }

        tig_art_anim_data(light->art_id, &art_anim_data);
        palette_modify_info.flags = TIG_PALETTE_MODIFY_TINT;
        palette_modify_info.tint_color = light->tint_color;
        palette_modify_info.src_palette = art_anim_data.palette1;
        palette_modify_info.dst_palette = light->palette;
        tig_palette_modify(&palette_modify_info);
    }
}

// 0x4DE4D0
void sub_4DE4D0(Light* light)
{
    if (light->palette != NULL) {
        tig_palette_destroy(light->palette);
        light->palette = NULL;
    }
}

// 0x4DE4F0
void sub_4DE4F0(Light* light, int offset_x, int offset_y)
{
    TigRect dirty_rect;
    TigRect updated_rect;
    int64_t sector_id;
    Sector* sector;

    if (light->offset_x != offset_x || light->offset_y != offset_y) {
        light_get_rect_internal(light, &dirty_rect);
        sector_id = sector_id_from_loc(light->loc);
        if (sub_4DD110(light) || sub_4D04E0(sector_id)) {
            sector_lock(sector_id, &sector);
            sector_light_list_update(&(sector->lights), light, light->loc, offset_x, offset_y);
            sector_unlock(sector_id);
        } else {
            light->offset_x = offset_x;
            light->offset_y = offset_y;
        }

        light->flags |= LF_MODIFIED;
        light_get_rect_internal(light, &updated_rect);
        tig_rect_union(&dirty_rect, &updated_rect, &dirty_rect);
        light_invalidate_rect(&dirty_rect, true);
    }
}

// 0x4DE5D0
bool sub_4DE5D0()
{
    int index;
    tig_art_id_t art_id;
    TigPaletteModifyInfo palette_modify_info;
    TigArtAnimData art_anim_data;

    for (index = 0; index < 1000; index++) {
        if (tig_art_light_id_create(index, 0, 0, 1, &art_id) == TIG_OK
            && tig_art_exists(art_id) == TIG_OK) {
            break;
        }
    }

    if (index == 1000) {
        return false;
    }

    if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
        return false;
    }

    strcpy(light_shadowmap_bmp.name, "art\\light\\shadowmap.bmp");
    if (tig_bmp_create(&light_shadowmap_bmp) != TIG_OK) {
        return false;
    }

    if (light_shadowmap_bmp.bpp != 8) {
        return false;
    }

    stru_602ED8.width = light_shadowmap_bmp.width;
    stru_602ED8.height = light_shadowmap_bmp.height;

    palette_modify_info.flags = TIG_PALETTE_MODIFY_TINT;
    palette_modify_info.src_palette = art_anim_data.palette2;

    for (index = 0; index < 7; index++) {
        dword_602E58[index] = tig_palette_create();
        palette_modify_info.dst_palette = dword_602E58[index];
        palette_modify_info.tint_color = tig_color_make(225 - 13 * index, 225 - 13 * index, 225 - 13 * index);
        tig_palette_modify(&palette_modify_info);
    }

    return true;
}

// 0x4DE730
void sub_4DE730()
{
    int index;

    shadow_node_clear();

    for (index = 0; index < 7; index++) {
        tig_palette_destroy(dword_602E58[index]);
    }

    tig_bmp_destroy(&light_shadowmap_bmp);
}

// 0x4DE770
Shadow* shadow_node_allocate()
{
    Shadow* node;

    node = shadow_node_head;
    if (node == NULL) {
        shadow_node_reserve();
        node = shadow_node_head;
    }

    shadow_node_head = node->next;
    node->next = NULL;

    return node;
}

// 0x4DE7A0
void shadow_node_deallocate(Shadow* node)
{
    node->next = shadow_node_head;
    shadow_node_head = node;
}

// 0x4DE7C0
void shadow_node_reserve()
{
    int index;
    Shadow* node;

    for (index = 0; index < 20; index++) {
        node = (Shadow*)MALLOC(sizeof(*node));
        node->next = shadow_node_head;
        shadow_node_head = node;
    }
}

// 0x4DE7F0
void shadow_node_clear()
{
    Shadow* curr;
    Shadow* next;

    curr = shadow_node_head;
    while (curr != NULL) {
        next = curr->next;
        FREE(curr);
        curr = next;
    }
    shadow_node_head = NULL;
}

// 0x4DE820
bool sub_4DE820(TimeEvent* timeevent)
{
    return (Light*)timeevent->params[0].pointer_value == dword_603400;
}

// 0x4DE870
void sub_4DE870(LightCreateInfo* create_info, Light** light_ptr)
{
    Light* light;

    if (light_editor || create_info->obj != OBJ_HANDLE_NULL) {
        light = (Light*)MALLOC(sizeof(*light));
        light->obj = create_info->obj;
        light->loc = create_info->loc;
        light->offset_x = create_info->offset_x;
        light->offset_y = create_info->offset_y;
        light->art_id = create_info->art_id;
        light->flags = create_info->flags;
        light->r = create_info->r;
        light->g = create_info->g;
        light->b = create_info->b;
        light->palette = NULL;
        sub_4DE390(light);
        *light_ptr = light;
    } else {
        *light_ptr = NULL;
    }
}

// 0x4DE900
void light_render_internal(GameDrawInfo* draw_info)
{
    tig_color_t indoor_color;
    tig_color_t outdoor_color;
    TigRectListNode* rect_node;
    TigRect tmp_rect;
    TigRectListNode* head;
    SectorListNode* v1;
    Sector* sector;
    SectorBlockListNode* light_node;
    Light* light;
    bool art_anim_data_loaded;
    TigArtAnimData art_anim_data;
    int64_t loc;
    int64_t loc_x;
    int64_t loc_y;

    tig_video_buffer_fill(lighter_vb, NULL, 0);
    tig_video_buffer_fill(darker_vb, NULL, 0);
    light_buffers_lock();

    indoor_color = light_get_indoor_color();
    outdoor_color = light_get_outdoor_color();

    head = NULL;
    rect_node = *draw_info->rects;
    while (rect_node != NULL) {
        tmp_rect = rect_node->rect;
        sub_4DF1D0(&tmp_rect);
        if (head != NULL) {
            sub_52D480(&head, &tmp_rect);
        } else {
            head = tig_rect_node_create();
            head->rect = tmp_rect;
        }
        rect_node = rect_node->next;
    }

    v1 = draw_info->sectors;
    while (v1 != NULL) {
        if (sector_lock(v1->sec, &sector)) {
            light_node = sector->lights.head;
            while (light_node != NULL) {
                light = (Light*)light_node->data;
                if ((light->flags & LF_OFF) == 0) {
                    light_get_rect_internal(light, &tmp_rect);

                    art_anim_data_loaded = false;

                    rect_node = head;
                    while (rect_node != NULL) {
                        if (tmp_rect.x < rect_node->rect.x + rect_node->rect.width
                            && tmp_rect.y < rect_node->rect.y + rect_node->rect.height
                            && rect_node->rect.x < tmp_rect.x + tmp_rect.width
                            && rect_node->rect.y < tmp_rect.y + tmp_rect.height) {
                            if (!art_anim_data_loaded) {
                                art_anim_data_loaded = true;
                                tig_art_anim_data(light->art_id, &art_anim_data);
                            }

                            location_at(rect_node->rect.x, rect_node->rect.y, &loc);
                            location_xy(loc, &loc_x, &loc_y);

                            int max_x = rect_node->rect.x + rect_node->rect.width - 1;
                            int max_y = rect_node->rect.y + rect_node->rect.height - 1;

                            for (int y = (int)loc_y; y <= max_y; y += 40) {
                                for (int x = (int)loc_x; x <= max_x; x += 80) {
                                    for (int i = 0; i < 2; i++) {
                                        for (int j = 0; j < 2; j++) {
                                            int lx = x + dword_5B9044[j];
                                            int ly = y + dword_5B904C[i];
                                            unsigned int color;

                                            if (lx >= rect_node->rect.x
                                                && ly >= rect_node->rect.y
                                                && lx < rect_node->rect.x + rect_node->rect.width
                                                && ly < rect_node->rect.y + rect_node->rect.height
                                                && lx >= tmp_rect.x
                                                && ly >= tmp_rect.y
                                                && lx < tmp_rect.x + tmp_rect.width
                                                && ly < tmp_rect.y + tmp_rect.height
                                                && sub_502E50(light->art_id, lx - tmp_rect.x, ly - tmp_rect.y, &color) == TIG_OK
                                                && color != art_anim_data.color_key) {
                                                int cx = (lx - dword_602ED0) / 40;
                                                int cy = (ly - dword_602ED4) / 20;
                                                if (cx >= 0
                                                    && cx < dword_603418
                                                    && cy >= 0
                                                    && cy < dword_60341C) {
                                                    if (((light->flags & LF_INDOOR) != 0
                                                            && light->tint_color != indoor_color)
                                                        || ((light->flags & LF_OUTDOOR) != 0
                                                            && light->tint_color != outdoor_color)) {
                                                        sub_4DE390(light);
                                                        light_invalidate_rect(&tmp_rect, true);
                                                    }

                                                    switch (light_bpp) {
                                                    case 8:
                                                        break;
                                                    case 16:
                                                    case 24:
                                                        // TODO: Incomplete.
                                                        break;
                                                    case 32: {
                                                        uint32_t* dst;
                                                        int idx;

                                                        if (light->palette != NULL) {
                                                            color = tig_color_mul(color, light->tint_color);
                                                        }

                                                        if ((light->flags & LF_DARK) != 0) {
                                                            idx = cx + cy * darker_pitch;
                                                            dst = darker_colors;
                                                        } else {
                                                            idx = cx + cy * lighter_pitch;
                                                            dst = lighter_colors;
                                                        }

                                                        dst[idx] = tig_color_add(color, dst[idx]);

                                                        break;
                                                    }
                                                    default:
                                                        // Should be unreachable.
                                                        assert(0);
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        rect_node = rect_node->next;
                    }
                }
                light_node = light_node->next;
            }
            sector_unlock(v1->sec);
        }
        v1 = v1->next;
    }

    while (head != NULL) {
        rect_node = head->next;
        tig_rect_node_destroy(head);
        head = rect_node;
    }

    light_buffers_unlock();
}

// 0x4DF1D0
void sub_4DF1D0(TigRect* rect)
{
    int64_t x1;
    int64_t y1;
    int64_t x2;
    int64_t y2;
    int64_t loc1;
    int64_t loc2;

    x1 = rect->x;
    y1 = rect->y;
    x2 = rect->x + rect->width - 1;
    y2 = rect->y + rect->height - 1;

    if (location_at(x1, y1, &loc1)) {
        location_xy(loc1, &x1, &y1);
        x1 -= 40;
        y1 -= 20;

        if (location_at(x2, y2, &loc2)) {
            location_xy(loc2, &x2, &y2);
            x2 += 80 + 40;
            y2 += 40 + 20;

            rect->x = (int)(x1);
            rect->y = (int)(y1);
            rect->width = (int)(x2 - x1 + 1);
            rect->height = (int)(y2 - y1 + 1);
        }
    }
}

// 0x4DF310
void light_invalidate_rect(TigRect* rect, bool invalidate_objects)
{
    TigRect dirty_rect;

    if (rect != NULL) {
        dirty_rect = *rect;
    } else {
        dirty_rect = light_iso_content_rect;
    }

    dirty_rect.x -= 40;
    dirty_rect.y -= 20;
    dirty_rect.width += 80;
    dirty_rect.height += 40;
    light_iso_window_invalidate_rect(&dirty_rect);

    if (invalidate_objects) {
        object_invalidate_rect(&dirty_rect);
    }
}
