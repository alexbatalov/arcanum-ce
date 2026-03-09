#include "game/tile.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include "game/a_name.h"
#include "game/gamelib.h"
#include "game/light.h"
#include "game/random.h"
#include "game/roof.h"
#include "game/sector.h"
#include "game/tile_block.h"

#define TILE_CACHE_CAPACITY 64

typedef struct TileCacheEntry {
    unsigned int art_id;
    TigVideoBuffer* video_buffer;
    unsigned int time;
} TileCacheEntry;

static void sub_4D7820(int64_t loc, tig_art_id_t art_id);
static void sub_4D7980(void);
static void sub_4D79C0(ViewOptions* view_options);
static void sub_4D7A00(void);
static void sub_4D7A40(int zoom);
static void sub_4D7A90(void);
static void sub_4D7AC0(int zoom);
static void sub_4D7C70(void);
static TigVideoBuffer* sub_4D7E90(unsigned int art_id);
static void tile_draw_topdown(GameDrawInfo* draw_info);
static void tile_draw_iso(GameDrawInfo* draw_info);

// 0x602AE0
static TileCacheEntry stru_602AE0[TILE_CACHE_CAPACITY];

// 0x602DE0
static TigVideoBuffer* dword_602DE0;

// 0x602DE4
static uint8_t* dword_602DE4;

// 0x602DE8
static uint8_t* dword_602DE8;

// 0x602DEC
static IsoInvalidateRectFunc* tile_invalidate_rect;

// 0x602DF0
static TigVideoBuffer* dword_602DF0;

// 0x602DF4
static bool tile_visible;

// 0x602DF8
static ViewOptions tile_view_options;

// 0x602E00
static bool dword_602E00;

// 0x602E04
static tig_window_handle_t tile_iso_window_handle;

// 0x602E08
static bool dword_602E08;

// 0x4D6840
bool tile_init(GameInitInfo* init_info)
{
    if (tig_window_vbid_get(init_info->iso_window_handle, &dword_602DF0) != TIG_OK) {
        return false;
    }

    dword_602E00 = tig_video_3d_check_initialized() == TIG_OK;
    tile_iso_window_handle = init_info->iso_window_handle;
    tile_invalidate_rect = init_info->invalidate_rect_func;
    tile_view_options.type = VIEW_TYPE_ISOMETRIC;
    tile_visible = true;

    return true;
}

// 0x4D68A0
void tile_exit(void)
{
    sub_4D7980();
    tile_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    tile_invalidate_rect = NULL;
}

// 0x4D68C0
void tile_resize(GameResizeInfo* resize_info)
{
    if (tig_window_vbid_get(resize_info->window_handle, &dword_602DF0) != TIG_OK) {
        tig_debug_printf("tile_resize: ERROR: couldn't grab window vbid!");
        exit(EXIT_FAILURE);
    }

    tile_iso_window_handle = resize_info->window_handle;
}

// 0x4D6900
void tile_update_view(ViewOptions* view_options)
{
    sub_4D79C0(view_options);
    tile_view_options = *view_options;
}

// 0x4D6930
void tile_toggle_visibility(void)
{
    tile_visible = !tile_visible;
}

// 0x4D6950
void tile_draw(GameDrawInfo* draw_info)
{
    if (!tile_visible) {
        return;
    }

    switch (tile_view_options.type) {
    case VIEW_TYPE_ISOMETRIC:
        tile_draw_iso(draw_info);
        break;
    case VIEW_TYPE_TOP_DOWN:
        // NOTE: Refactored into separate function for clarity.
        tile_draw_topdown(draw_info);
        break;
    }
}

// 0x4D7090
int tile_id_from_loc(int64_t loc)
{
    int tile_x;
    int tile_y;

    tile_x = LOCATION_GET_X(loc) & 0x3F;
    tile_y = LOCATION_GET_Y(loc) & 0x3F;

    return TILE_MAKE(tile_x, tile_y);
}

// 0x4D70B0
tig_art_id_t tile_art_id_at(int64_t loc)
{
    int64_t sector_id;
    Sector* sector;
    int tile;
    tig_art_id_t art_id;

    sector_id = sector_id_from_loc(loc);
    if (!sector_lock(sector_id, &sector)) {
        return TIG_ART_ID_INVALID;
    }

    tile = tile_id_from_loc(loc);
    art_id = sector->tiles.art_ids[tile];
    sector_unlock(sector_id);

    return art_id;
}

// 0x4D7110
bool tile_is_blocking(int64_t loc, bool a2)
{
    tig_art_id_t art_id;
    bool v1;

    if (tileblock_is_tile_blocked(loc)) {
        return true;
    }

    art_id = tile_art_id_at(loc);
    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return !tig_art_facade_id_walkable_get(art_id);
    }

    v1 = a_name_tile_is_blocking(art_id);
    if (a2) {
        if (v1) {
            // FIXME: Useless.
            tile_is_sinkable(loc);
        }
        v1 = false;
    }

    return v1;
}

// 0x4D7180
bool tile_is_soundproof(int64_t loc)
{
    return a_name_tile_is_soundproof(tile_art_id_at(loc));
}

// 0x4D71A0
bool tile_is_sinkable(int64_t loc)
{
    return a_name_tile_is_sinkable(tile_art_id_at(loc));
}

// 0x4D71C0
bool tile_is_slippery(int64_t loc)
{
    return a_name_tile_is_slippery(tile_art_id_at(loc));
}

// 0x4D71E0
void sub_4D71E0(void)
{
    // TODO: Incomplete.
}

// 0x4D7430
void sub_4D7430(int64_t loc)
{
    tig_art_id_t art_id;

    art_id = tile_art_id_at(loc);
    do {
        if (art_id != TIG_ART_ID_INVALID) {
            art_id = sub_503800(art_id, sub_5037B0(art_id) + 1);
        }
    } while (tig_art_exists(art_id) != TIG_OK);

    sub_4D7820(loc, art_id);
}

// 0x4D7480
tig_art_id_t sub_4D7480(tig_art_id_t art_id, int num2, bool flippable2, int a4)
{
    int num1;
    int type;
    int flippable1;
    int palette;
    int v1;
    int v2;
    int cnt;
    int tmp;

    num1 = tig_art_tile_id_num1_get(art_id);
    type = tig_art_tile_id_type_get(art_id);
    flippable1 = tig_art_tile_id_type_get(art_id);
    palette = tig_art_id_palette_get(art_id);

    if (flippable1) {
        if (flippable2 && num2 < num1) {
            v1 = 15 - a4;

            tmp = num2;
            num2 = num1;
            num1 = tmp;

            tmp = flippable2;
            flippable2 = flippable1;
            flippable1 = tmp;
        } else {
            v1 = a4;
        }
    } else {
        if (flippable2 || num2 < num1) {
            v1 = 15 - a4;

            tmp = num2;
            num2 = num1;
            num1 = tmp;

            tmp = flippable2;
            flippable2 = flippable1;
            flippable1 = tmp;
        } else {
            v1 = a4;
        }
    }

    if (v1 == 0) {
        flippable1 = flippable2;
        num1 = num2;
    } else if (num1 == num2 && flippable1 == flippable2) {
        v1 = 0;
    } else if (v1 == 15) {
        flippable2 = flippable1;
        num2 = num1;
        v1 = 0;
    }

    cnt = sub_4EBEF0(num1, num2, v1, type, flippable1, flippable2);
    v2 = random_between(0, cnt - 1);

    if (flippable1
        && flippable2
        && num1 != num2
        && random_between(0, 1) != 0) {
        v2 += 8;
    }

    tig_art_tile_id_create(num1, num2, v1, v2, type, flippable1, flippable2, palette, &art_id);

    return art_id;
}

// 0x4D7590
void sub_4D7590(tig_art_id_t art_id, TigVideoBuffer* video_buffer)
{
    TigRect rect;
    TigArtBlitInfo art_blit_spec;
    TigVideoBufferData src_video_buffer_data;
    TigVideoBufferData dst_video_buffer_data;

    if (tile_view_options.type == VIEW_TYPE_TOP_DOWN) {
        rect.x = 0;
        rect.y = 0;
        rect.width = 80;
        rect.height = 40;

        art_blit_spec.flags = TIG_ART_BLT_PALETTE_ORIGINAL;
        art_blit_spec.art_id = art_id;
        art_blit_spec.src_rect = &rect;
        art_blit_spec.dst_video_buffer = dword_602DE0;
        art_blit_spec.dst_rect = &rect;
        tig_art_blit(&art_blit_spec);

        tig_video_buffer_lock(dword_602DE0);
        tig_video_buffer_data(dword_602DE0, &src_video_buffer_data);

        tig_video_buffer_lock(video_buffer);
        tig_video_buffer_data(video_buffer, &dst_video_buffer_data);

        switch (src_video_buffer_data.bpp) {
        case 8:
            for (int y = 0; y < tile_view_options.zoom; y++) {
                for (int x = 0; x < tile_view_options.zoom; x++) {
                    int index = y * tile_view_options.zoom + x;
                    int src_index = dword_602DE4[index] * src_video_buffer_data.pitch + dword_602DE8[index];
                    int dst_index = y * dst_video_buffer_data.pitch + x;
                    dst_video_buffer_data.surface_data.p8[dst_index] = src_video_buffer_data.surface_data.p8[src_index];
                }
            }
            break;
        case 16:
            for (int y = 0; y < tile_view_options.zoom; y++) {
                for (int x = 0; x < tile_view_options.zoom; x++) {
                    int index = y * tile_view_options.zoom + x;
                    int src_index = dword_602DE4[index] * src_video_buffer_data.pitch / 2 + dword_602DE8[index];
                    int dst_index = y * dst_video_buffer_data.pitch / 2 + x;
                    dst_video_buffer_data.surface_data.p16[dst_index] = src_video_buffer_data.surface_data.p16[src_index];
                }
            }
            break;
        case 24:
            // TODO: Same implementation as in 32bpp, check.
            for (int y = 0; y < tile_view_options.zoom; y++) {
                for (int x = 0; x < tile_view_options.zoom; x++) {
                    int index = y * tile_view_options.zoom + x;
                    int src_index = dword_602DE4[index] * src_video_buffer_data.pitch / 4 + dword_602DE8[index];
                    int dst_index = y * dst_video_buffer_data.pitch / 4 + x;
                    dst_video_buffer_data.surface_data.p32[dst_index] = src_video_buffer_data.surface_data.p32[src_index];
                }
            }
            break;
        case 32:
            for (int y = 0; y < tile_view_options.zoom; y++) {
                for (int x = 0; x < tile_view_options.zoom; x++) {
                    int index = y * tile_view_options.zoom + x;
                    int src_index = dword_602DE4[index] * src_video_buffer_data.pitch / 4 + dword_602DE8[index];
                    int dst_index = y * dst_video_buffer_data.pitch / 4 + x;
                    dst_video_buffer_data.surface_data.p32[dst_index] = src_video_buffer_data.surface_data.p32[src_index];
                }
            }
            break;
        }

        tig_video_buffer_unlock(video_buffer);
        tig_video_buffer_unlock(dword_602DE0);
    }
}

// 0x4D7820
void sub_4D7820(int64_t loc, tig_art_id_t art_id)
{
    int64_t sector_id;
    Sector* sector;
    int tile;
    int64_t x;
    int64_t y;
    TigRect rect;

    sector_id = sector_id_from_loc(loc);
    if (sector_lock(sector_id, &sector)) {
        tile = tile_id_from_loc(loc);
        sector->tiles.art_ids[tile] = art_id;
        sector->tiles.difmask[tile / 32] |= 1 << (tile & 31);
        sector->tiles.dif = 1;
        sector_unlock(sector_id);

        location_xy(loc, &x, &y);
        if (x > INT_MIN && x < INT_MAX
            && y > INT_MIN && y < INT_MAX) {
            rect.x = (int)x;
            rect.y = (int)y;
            if (tile_view_options.type == VIEW_TYPE_ISOMETRIC) {
                rect.width = 80;
                rect.height = 40;
            } else {
                rect.width = tile_view_options.zoom;
                rect.height = tile_view_options.zoom;
            }
            tile_invalidate_rect(&rect);
        }
    }
}

// 0x4D7980
void sub_4D7980(void)
{
    if (dword_602DE0 != NULL) {
        tig_video_buffer_destroy(dword_602DE0);
        dword_602DE0 = NULL;
    }

    sub_4D7C70();
    sub_4D7A90();

    dword_602E08 = false;
}

// 0x4D79C0
void sub_4D79C0(ViewOptions* view_options)
{
    if (view_options->type == VIEW_TYPE_TOP_DOWN) {
        if (view_options->zoom != tile_view_options.zoom) {
            if (!dword_602E08) {
                sub_4D7A00();
            }

            sub_4D7A40(view_options->zoom);
            sub_4D7AC0(view_options->zoom);
        }
    }
}

// 0x4D7A00
void sub_4D7A00(void)
{
    TigVideoBufferCreateInfo vb_create_info;

    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
    vb_create_info.width = 80;
    vb_create_info.height = 40;
    vb_create_info.background_color = 0;
    tig_video_buffer_create(&vb_create_info, &dword_602DE0);

    dword_602E08 = true;
}

// 0x4D7A40
void sub_4D7A40(int zoom)
{
    TigVideoBufferCreateInfo vb_create_info;
    int index;

    sub_4D7A90();

    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
    vb_create_info.width = zoom;
    vb_create_info.height = zoom;
    vb_create_info.background_color = 0;

    for (index = 0; index < TILE_CACHE_CAPACITY; index++) {
        stru_602AE0[index].art_id = TIG_ART_ID_INVALID;
        tig_video_buffer_create(&vb_create_info, &(stru_602AE0[index].video_buffer));
    }
}

// 0x4D7A90
void sub_4D7A90(void)
{
    int index;

    for (index = 0; index < TILE_CACHE_CAPACITY; index++) {
        if (stru_602AE0[index].video_buffer != NULL) {
            tig_video_buffer_destroy(stru_602AE0[index].video_buffer);
        }

        stru_602AE0[index].video_buffer = NULL;
        stru_602AE0[index].art_id = TIG_ART_ID_INVALID;
    }
}

// 0x4D7AC0
void sub_4D7AC0(int zoom)
{
    double v1;
    double v2;
    double v3;
    double v4;
    double scale;
    double v5;
    double v6;
    double v7;
    double v8;
    int x;
    int y;

    sub_4D7C70();

    v1 = zoom * 0.5 - 1.0;

    dword_602DE8 = (uint8_t*)MALLOC(sizeof(*dword_602DE8) * (zoom * zoom));
    dword_602DE4 = (uint8_t*)MALLOC(sizeof(*dword_602DE4) * (zoom * zoom));

    v2 = ((zoom - 1) - v1) * M_SQRT1_2;
    v3 = 80.0 / (v2 * 4.0);
    v4 = 40.0 / (v2 + v1 * M_SQRT1_2 + v2 + v1 * M_SQRT1_2);

    if (zoom < 8) {
        scale = 0.5;
    } else if (zoom < 16) {
        scale = 0.65;
    } else if (zoom < 24) {
        scale = 0.7;
    } else if (zoom < 32) {
        scale = 0.8;
    } else if (zoom < 40) {
        scale = 0.9;
    } else if (zoom < 48) {
        scale = 0.91;
    } else if (zoom < 56) {
        scale = 0.92;
    } else if (zoom < 60) {
        scale = 0.93;
    } else if (zoom < 62) {
        scale = 0.94;
    } else {
        scale = 0.95;
    }

    v5 = v3 * scale;
    v6 = v4 * scale;

    for (y = 0; y < zoom; y++) {
        v7 = (y - v1) * M_SQRT1_2;
        for (x = 0; x < zoom; x++) {
            v8 = (x - v1) * M_SQRT1_2;
            dword_602DE8[y * zoom + x] = (uint8_t)((v7 + v8) * v5 + 40.0);
            dword_602DE4[y * zoom + x] = (uint8_t)((v7 - v8) * v6 + 20.0);
        }
    }
}

// 0x4D7C70
void sub_4D7C70(void)
{
    if (dword_602DE8 != NULL) {
        FREE(dword_602DE8);
        dword_602DE8 = NULL;
    }

    if (dword_602DE4 != NULL) {
        FREE(dword_602DE4);
        dword_602DE4 = NULL;
    }
}

// 0x4D7E90
TigVideoBuffer* sub_4D7E90(unsigned int art_id)
{
    int candidate = -1;
    int found = -1;
    int index;

    for (index = 0; index < TILE_CACHE_CAPACITY; index++) {
        if (stru_602AE0[index].art_id == -1) {
            found = index;
        } else {
            if (stru_602AE0[index].art_id == art_id) {
                stru_602AE0[index].time = gamelib_ping_time;
                return stru_602AE0[index].video_buffer;
            }

            if (candidate == -1) {
                candidate = index;
            } else {
                if (stru_602AE0[index].time < stru_602AE0[candidate].time) {
                    candidate = index;
                }
            }
        }
    }

    if (found == -1) {
        found = candidate;
    }

    sub_4D7590(art_id, stru_602AE0[found].video_buffer);
    stru_602AE0[found].art_id = art_id;
    stru_602AE0[found].time = gamelib_ping_time;

    return stru_602AE0[found].video_buffer;
}

// 0x4D7CB0
void tile_draw_topdown(GameDrawInfo* draw_info)
{
    SectorListNode* sector_node;
    Sector* sector;
    int tile;
    int skip;
    int x;
    int y;
    int64_t loc_x;
    int64_t loc_y;
    TigRectListNode* rect_node;
    TigRect tile_rect;
    TigRect dst_rect;
    TigRect src_rect;

    sector_node = draw_info->sectors;
    while (sector_node != NULL) {
        if (sector_lock(sector_node->sec, &sector)) {
            tile = tile_id_from_loc(sector_node->loc);
            skip = 64 - sector_node->width;
            location_xy(sector_node->loc, &loc_x, &loc_y);

            for (y = 0; y < sector_node->height; y++) {
                tile_rect.x = (int)loc_x;
                tile_rect.y = (int)loc_y + y * tile_view_options.zoom;
                tile_rect.width = tile_view_options.zoom;
                tile_rect.height = tile_view_options.zoom;

                for (x = 0; x < sector_node->width; x++) {
                    rect_node = *draw_info->rects;
                    while (rect_node != NULL) {
                        if (tig_rect_intersection(&tile_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                            src_rect.x = dst_rect.x - tile_rect.x;
                            src_rect.y = dst_rect.y - tile_rect.y;
                            src_rect.width = dst_rect.width;
                            src_rect.height = dst_rect.height;
                            tig_window_copy_from_vbuffer(tile_iso_window_handle,
                                &dst_rect,
                                sub_4D7E90(sector->tiles.art_ids[tile]),
                                &src_rect);
                        }
                        rect_node = rect_node->next;
                    }

                    tile++;
                    tile_rect.x -= tile_view_options.zoom;
                }

                tile += skip;
            }
            sector_unlock(sector_node->sec);
        }
        sector_node = sector_node->next;
    }
}

// NOTE: In the original code this function is a part of `tile_draw`, however
// if `tile_draw_topdown` is definitely there, why `tile_draw_iso` should not?
void tile_draw_iso(GameDrawInfo* draw_info)
{
    SomeSectorStuff* v1;
    SomeSectorStuffEntry* v3;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect tile_rect;
    TigRect tile_subrect;
    tig_color_t indoor_color;
    tig_color_t outdoor_color;
    int v2;
    int v4;
    int indexes[3];
    int widths[3];
    bool sector_lock_results[3];
    Sector* sectors[3];
    int64_t loc_x;
    int64_t loc_y;
    TigRectListNode* rect_node;
    int tile_type;
    tig_color_t color;
    tig_color_t v36[4];
    tig_color_t v51[9];
    int v10;
    int v11;
    int center_x;
    int center_y;
    int v15;
    int v42;
    bool blit_info_initialized;
    int v38;

    v1 = draw_info->field_8;

    art_blit_info.flags = 0; // NOTE: Initialize to silence compiler warning.
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    tile_rect.width = 78;
    tile_rect.height = 40;

    indoor_color = light_get_indoor_color();
    outdoor_color = light_get_outdoor_color();

    light_buffers_lock();

    for (v2 = 0; v2 < v1->height; v2++) {
        v3 = &(v1->field_8[v2]);

        for (v4 = 0; v4 < v3->width; v4++) {
            indexes[v4] = v3->field_38[v4];
            widths[v4] = 64 - v3->field_44[v4];
            sector_lock_results[v4] = sector_lock(v3->field_20[v4], &(sectors[v4]));
        }

        location_xy(v3->field_8[0], &loc_x, &loc_y);

        v10 = 0;
        v11 = 0;

        for (v38 = 0; v38 < v3->field_50; v38++) {
            center_x = (int)loc_x + v10;
            center_y = (int)loc_y + v11;

            for (v15 = 0; v15 < v3->width; v15++) {
                if (sector_lock_results[v15]) {
                    for (v42 = 0; v42 < v3->field_44[v15]; v42++) {
                        blit_info_initialized = false;
                        art_blit_info.art_id = sectors[v15]->tiles.art_ids[indexes[v15]];
                        tile_type = tig_art_tile_id_type_get(art_blit_info.art_id);
                        if (!roof_is_covered_xy(center_x + 40, center_y + 20, false)) {
                            tile_rect.x = center_x + 1;
                            tile_rect.y = center_y;

                            rect_node = *draw_info->rects;
                            while (rect_node != NULL) {
                                if (tig_rect_intersection(&tile_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                    src_rect.x = dst_rect.x - tile_rect.x;
                                    src_rect.y = dst_rect.y - tile_rect.y;
                                    src_rect.width = dst_rect.width;
                                    src_rect.height = dst_rect.height;

                                    if (!blit_info_initialized) {
                                        blit_info_initialized = true;

                                        art_blit_info.dst_video_buffer = dword_602DF0;
                                        art_blit_info.field_14 = v36;

                                        color = !tile_type ? indoor_color : outdoor_color;

                                        if (sub_4DA360(center_x, center_y, color, v51)) {
                                            art_blit_info.flags = TIG_ART_BLT_BLEND_COLOR_LERP;
                                            if (!dword_602E00) {
                                                art_blit_info.flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
                                            }
                                        } else if (v51[0] != color || dword_602E00) {
                                            art_blit_info.flags = TIG_ART_BLT_BLEND_COLOR_CONST;
                                            art_blit_info.color = v51[0];
                                            if (!dword_602E00) {
                                                art_blit_info.flags |= TIG_ART_BLT_PALETTE_ORIGINAL;
                                            }
                                        } else {
                                            art_blit_info.flags = 0;
                                        }
                                    }

                                    if ((art_blit_info.flags & TIG_ART_BLT_BLEND_COLOR_LERP) != 0) {
                                        art_blit_info.field_18 = &tile_subrect;

                                        tile_subrect.x = tile_rect.x;
                                        tile_subrect.y = tile_rect.y;
                                        tile_subrect.width = 39;
                                        tile_subrect.height = 20;
                                        if (tig_rect_intersection(&tile_subrect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                            tile_subrect.x -= tile_rect.x;
                                            tile_subrect.y -= tile_rect.y;

                                            src_rect.x = dst_rect.x - tile_rect.x;
                                            src_rect.y = dst_rect.y - tile_rect.y;
                                            src_rect.width = dst_rect.width;
                                            src_rect.height = dst_rect.height;

                                            v36[0] = v51[0];
                                            v36[1] = v51[1];
                                            v36[2] = v51[4];
                                            v36[3] = v51[3];

                                            tig_art_blit(&art_blit_info);
                                        }

                                        tile_subrect.x = tile_rect.x + 39;
                                        tile_subrect.y = tile_rect.y;
                                        if (tig_rect_intersection(&tile_subrect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                            tile_subrect.x -= tile_rect.x;
                                            tile_subrect.y -= tile_rect.y;

                                            src_rect.x = dst_rect.x - tile_rect.x;
                                            src_rect.y = dst_rect.y - tile_rect.y;
                                            src_rect.width = dst_rect.width;
                                            src_rect.height = dst_rect.height;

                                            v36[0] = v51[1];
                                            v36[1] = v51[2];
                                            v36[2] = v51[5];
                                            v36[3] = v51[4];

                                            tig_art_blit(&art_blit_info);
                                        }

                                        tile_subrect.x = tile_rect.x;
                                        tile_subrect.y = tile_rect.y + 20;
                                        if (tig_rect_intersection(&tile_subrect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                            tile_subrect.x -= tile_rect.x;
                                            tile_subrect.y -= tile_rect.y;

                                            src_rect.x = dst_rect.x - tile_rect.x;
                                            src_rect.y = dst_rect.y - tile_rect.y;
                                            src_rect.width = dst_rect.width;
                                            src_rect.height = dst_rect.height;

                                            v36[0] = v51[3];
                                            v36[1] = v51[4];
                                            v36[2] = v51[7];
                                            v36[3] = v51[6];

                                            tig_art_blit(&art_blit_info);
                                        }

                                        tile_subrect.x = tile_rect.x + 39;
                                        tile_subrect.y = tile_rect.y + 20;
                                        if (tig_rect_intersection(&tile_subrect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                                            tile_subrect.x -= tile_rect.x;
                                            tile_subrect.y -= tile_rect.y;

                                            src_rect.x = dst_rect.x - tile_rect.x;
                                            src_rect.y = dst_rect.y - tile_rect.y;
                                            src_rect.width = dst_rect.width;
                                            src_rect.height = dst_rect.height;

                                            v36[0] = v51[4];
                                            v36[1] = v51[5];
                                            v36[2] = v51[8];
                                            v36[3] = v51[7];

                                            tig_art_blit(&art_blit_info);
                                        }
                                    } else {
                                        tig_art_blit(&art_blit_info);
                                    }
                                }
                                rect_node = rect_node->next;
                            }
                        }

                        indexes[v15]++;
                        center_x -= 40;
                        center_y += 20;
                    }

                    indexes[v15] += widths[v15];
                } else {
                    center_x -= 40 * v3->field_44[v15];
                    center_y += 20 * v3->field_44[v15];
                }
            }

            v10 += 40;
            v11 += 20;
        }

        for (v4 = 0; v4 < v3->width; v4++) {
            if (sector_lock_results[v4]) {
                sector_unlock(v3->field_20[v4]);
            }
        }
    }

    light_buffers_unlock();
}
