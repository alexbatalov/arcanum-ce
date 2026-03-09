#include "game/roof.h"

#include "game/gamelib.h"
#include "game/light.h"
#include "game/location.h"
#include "game/mes.h"
#include "game/sector.h"
#include "game/tile.h"

static int roof_id_from_loc(int64_t loc);
static int sub_4395C0(int a1);
static void roof_xy(int64_t loc, int64_t* sx, int64_t* sy);
static tig_art_id_t roof_art_id_get(int64_t loc);
static bool roof_art_id_set(int64_t loc, tig_art_id_t aid);
static void sub_43A140(int x, int y, tig_art_id_t aid, TigRect* rect);
static void roof_fill(int64_t loc, bool fill, int a3);

// 0x5A53A0
static unsigned int roof_blit_flags = TIG_ART_BLT_BLEND_ALPHA_CONST;

// 0x5A53A4
static char byte_5A53A4[TIG_ART_ROOF_PIECE_COUNT][4][4] = {
    /* TIG_ART_ROOF_PIECE_NORTH_WEST_OUTSIDE */
    {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
    },
    /* TIG_ART_ROOF_PIECE_WEST */
    {
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
    },
    /* TIG_ART_ROOF_PIECE_NORTH */
    {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
    },
    /* TIG_ART_ROOF_PIECE_NORTH_WEST_INSIDE */
    {
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
    },
    /* TIG_ART_ROOF_PIECE_SOUTH_WEST_OUTSIDE */
    {
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
        { 1, 1, 1, 0 },
        { 0, 0, 0, 0 },
    },
    /* TIG_ART_ROOF_PIECE_SOUTH_WEST_INSIDE */
    {
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 0 },
    },
    /* TIG_ART_ROOF_PIECE_NORTH_EAST_INSIDE */
    {
        { 0, 0, 1, 1 },
        { 0, 0, 1, 1 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
    },
    /* TIG_ART_ROOF_PIECE_NORTH_EAST_OUTSIDE */
    {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 0, 1, 1 },
        { 0, 0, 1, 1 },
    },
    /* TIG_ART_ROOF_PIECE_CENTER */
    {
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
    },
    /* TIG_ART_ROOF_PIECE_SOUTH_EAST_OUTSIDE */
    {
        { 0, 0, 1, 1 },
        { 0, 0, 1, 1 },
        { 0, 0, 1, 1 },
        { 0, 0, 0, 0 },
    },
    /* TIG_ART_ROOF_PIECE_SOUTH */
    {
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 0, 0, 0, 0 },
    },
    /* TIG_ART_ROOF_PIECE_EAST */
    {
        { 0, 0, 1, 1 },
        { 0, 0, 1, 1 },
        { 0, 0, 1, 1 },
        { 0, 0, 1, 1 },
    },
    /* TIG_ART_ROOF_PIECE_SOUTH_EAST_INSIDE */
    {
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 0, 0, 1, 1 },
    },
};

// 0x5E2E30
static uint8_t roof_partial_opacity;

// 0x5E2E31
static uint8_t roof_full_transparency;

// 0x5E2E34
static IsoInvalidateRectFunc* roof_iso_window_invalidate_rect;

// 0x5E2E38
static bool dword_5E2E38;

// 0x5E2E3C
static bool roof_editor;

// 0x5E2E40
static bool roof_enabled;

// 0x5E2E44
static tig_window_handle_t roof_iso_window_handle;

// 0x5E2E48
static uint8_t roof_full_opacity;

// 0x5E2E50
static ViewOptions roof_view_options;

// 0x438F90
bool roof_init(GameInitInfo* init_info)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    roof_iso_window_handle = init_info->iso_window_handle;
    roof_iso_window_invalidate_rect = init_info->invalidate_rect_func;
    roof_view_options.type = VIEW_TYPE_ISOMETRIC;
    roof_editor = init_info->editor;
    roof_enabled = true;

    if (!mes_load("art\\roof\\roofshade.mes", &mes_file)) {
        return false;
    }

    mes_file_entry.num = 0;
    if (!mes_search(mes_file, &mes_file_entry)) {
        mes_unload(mes_file);
        return false;
    }
    roof_full_opacity = (uint8_t)atoi(mes_file_entry.str);

    mes_file_entry.num = 1;
    if (!mes_search(mes_file, &mes_file_entry)) {
        mes_unload(mes_file);
        return false;
    }
    roof_partial_opacity = (uint8_t)atoi(mes_file_entry.str);

    mes_file_entry.num = 2;
    if (!mes_search(mes_file, &mes_file_entry)) {
        mes_unload(mes_file);
        return false;
    }
    roof_full_transparency = (uint8_t)atoi(mes_file_entry.str);

    mes_unload(mes_file);

    dword_5E2E38 = tig_video_3d_check_initialized() == TIG_OK;
    if (!dword_5E2E38) {
        roof_blit_flags = TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D;
    }

    return true;
}

// 0x4390D0
void roof_exit()
{
    roof_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    roof_iso_window_invalidate_rect = NULL;
}

// 0x4390F0
void roof_resize(GameResizeInfo* resize_info)
{
    roof_iso_window_handle = resize_info->window_handle;
}

// 0x439100
void roof_update_view(ViewOptions* view_options)
{
    roof_view_options = *view_options;
}

// 0x439120
void roof_toggle()
{
    roof_enabled = !roof_enabled;
}

// 0x439140
void roof_draw(GameDrawInfo* draw_info)
{
    TigArtBlitInfo art_blit_info;
    TigRect dst_rect;
    TigRect src_rect;
    LocRect loc_rect;
    int64_t y;
    int64_t x;
    unsigned int flags;
    tig_art_id_t aid;
    int64_t loc_x;
    int64_t loc_y;
    TigRect roof_rect;
    TigRectListNode* node;

    if (!roof_enabled) {
        return;
    }

    if (roof_view_options.type != VIEW_TYPE_ISOMETRIC) {
        return;
    }

    if (dword_5E2E38) {
        art_blit_info.color = light_get_outdoor_color();
        flags = TIG_ART_BLT_BLEND_COLOR_CONST;
    } else {
        flags = 0;
    }

    loc_rect = *draw_info->loc_rect;

    for (y = loc_rect.y1; y <= loc_rect.y2; y += 4) {
        for (x = loc_rect.x1; x <= loc_rect.x2; x += 4) {
            aid = roof_art_id_get(LOCATION_MAKE(x, y));
            if (aid != TIG_ART_ID_INVALID
                && !tig_art_roof_id_fill_get(aid)) {
                roof_xy(LOCATION_MAKE(x, y), &loc_x, &loc_y);
                if (loc_x > INT_MIN
                    && loc_x < INT_MAX
                    && loc_y > INT_MIN
                    && loc_y < INT_MAX) {
                    sub_43A140((int)loc_x, (int)loc_y, aid, &roof_rect);
                    node = *draw_info->rects;
                    while (node != NULL) {
                        if (tig_rect_intersection(&roof_rect, &node->rect, &dst_rect) == TIG_OK) {
                            src_rect.x = dst_rect.x - roof_rect.x;
                            src_rect.y = dst_rect.y - roof_rect.y;
                            src_rect.width = dst_rect.width;
                            src_rect.height = dst_rect.height;

                            art_blit_info.art_id = aid;
                            art_blit_info.src_rect = &src_rect;
                            art_blit_info.dst_rect = &dst_rect;
                            art_blit_info.flags = flags;

                            if (tig_art_roof_id_fade_get(aid)) {
                                if (roof_blit_flags == TIG_ART_BLT_BLEND_ALPHA_CONST) {
                                    art_blit_info.flags |= TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH;

                                    switch (tig_art_roof_id_piece_get(aid)) {
                                    case TIG_ART_ROOF_PIECE_NORTH_WEST_OUTSIDE:
                                        art_blit_info.alpha[0] = roof_full_transparency;
                                        art_blit_info.alpha[1] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_partial_opacity;
                                        art_blit_info.alpha[3] = roof_full_transparency;
                                        break;
                                    case TIG_ART_ROOF_PIECE_WEST:
                                        art_blit_info.alpha[0] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_partial_opacity;
                                        art_blit_info.alpha[1] = roof_full_opacity;
                                        art_blit_info.alpha[3] = roof_full_transparency;
                                        break;
                                    case TIG_ART_ROOF_PIECE_NORTH:
                                        art_blit_info.alpha[0] = roof_full_transparency;
                                        art_blit_info.alpha[1] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_full_opacity;
                                        art_blit_info.alpha[3] = roof_partial_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_NORTH_WEST_INSIDE:
                                        art_blit_info.alpha[0] = roof_partial_opacity;
                                        art_blit_info.alpha[1] = roof_full_opacity;
                                        art_blit_info.alpha[2] = roof_full_opacity;
                                        art_blit_info.alpha[3] = roof_partial_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_SOUTH_WEST_OUTSIDE:
                                        art_blit_info.alpha[0] = roof_partial_opacity;
                                        art_blit_info.alpha[1] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_full_transparency;
                                        art_blit_info.alpha[3] = roof_full_transparency;
                                        break;
                                    case TIG_ART_ROOF_PIECE_SOUTH_WEST_INSIDE:
                                        art_blit_info.alpha[0] = roof_full_opacity;
                                        art_blit_info.alpha[1] = roof_full_opacity;
                                        art_blit_info.alpha[2] = roof_partial_opacity;
                                        art_blit_info.alpha[3] = roof_partial_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_NORTH_EAST_INSIDE:
                                        art_blit_info.alpha[0] = roof_partial_opacity;
                                        art_blit_info.alpha[1] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_full_opacity;
                                        art_blit_info.alpha[3] = roof_full_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_NORTH_EAST_OUTSIDE:
                                        art_blit_info.alpha[0] = roof_full_transparency;
                                        art_blit_info.alpha[1] = roof_full_transparency;
                                        art_blit_info.alpha[2] = roof_partial_opacity;
                                        art_blit_info.alpha[3] = roof_partial_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_CENTER:
                                        art_blit_info.alpha[0] = roof_full_opacity;
                                        art_blit_info.alpha[1] = roof_full_opacity;
                                        art_blit_info.alpha[2] = roof_full_opacity;
                                        art_blit_info.alpha[3] = roof_full_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_SOUTH_EAST_OUTSIDE:
                                        art_blit_info.alpha[0] = roof_partial_opacity;
                                        art_blit_info.alpha[1] = roof_full_transparency;
                                        art_blit_info.alpha[2] = roof_full_transparency;
                                        art_blit_info.alpha[3] = roof_partial_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_SOUTH:
                                        art_blit_info.alpha[0] = roof_full_opacity;
                                        art_blit_info.alpha[1] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_full_transparency;
                                        art_blit_info.alpha[3] = roof_partial_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_EAST:
                                        art_blit_info.alpha[0] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_partial_opacity;
                                        art_blit_info.alpha[1] = roof_full_transparency;
                                        art_blit_info.alpha[3] = roof_full_opacity;
                                        break;
                                    case TIG_ART_ROOF_PIECE_SOUTH_EAST_INSIDE:
                                        art_blit_info.alpha[0] = roof_full_opacity;
                                        art_blit_info.alpha[1] = roof_partial_opacity;
                                        art_blit_info.alpha[2] = roof_partial_opacity;
                                        art_blit_info.alpha[3] = roof_full_opacity;
                                        break;
                                    default:
                                        // Should be unreachable.
                                        assert(0);
                                    }
                                } else {
                                    art_blit_info.flags |= roof_blit_flags;
                                }
                            }

                            art_blit_info.flags |= TIG_ART_BLT_SCRATCH_VALID;
                            art_blit_info.scratch_video_buffer = gamelib_scratch_video_buffer;
                            tig_window_blit_art(roof_iso_window_handle, &art_blit_info);
                        }
                        node = node->next;
                    }
                }
            }
        }
    }
}

// 0x4395A0
int roof_id_from_loc(int64_t loc)
{
    return sub_4395C0(tile_id_from_loc(loc));
}

// 0x4395C0
int sub_4395C0(int a1)
{
    return ((a1 >> 2) & 0xF) + ((a1 >> 8) << 4);
}

// 0x4395E0
int64_t roof_normalize_loc(int64_t loc)
{
    int64_t x = LOCATION_GET_X(loc);
    int64_t y = LOCATION_GET_Y(loc);

    return LOCATION_MAKE(x - x % 4 + 2, y - y % 4 + 2);
}

// 0x439640
void roof_xy(int64_t loc, int64_t* sx, int64_t* sy)
{
    loc = roof_normalize_loc(loc);
    location_xy(loc, sx, sy);

    *sx -= 120;
    *sy -= 200;
}

// 0x4396A0
tig_art_id_t roof_art_id_get(int64_t loc)
{
    int64_t sector_id;
    Sector* sector;
    tig_art_id_t aid;

    sector_id = sector_id_from_loc(loc);
    if (!sector_lock(sector_id, &sector)) {
        return TIG_ART_ID_INVALID;
    }

    aid = sector->roofs.art_ids[roof_id_from_loc(loc)];

    sector_unlock(sector_id);

    return aid;
}

// 0x439700
bool roof_art_id_set(int64_t loc, tig_art_id_t aid)
{
    int64_t sec;
    Sector* sector;
    tig_art_id_t old_aid;
    int64_t sx;
    int64_t sy;
    TigRect rect;

    loc = roof_normalize_loc(loc);
    sec = sector_id_from_loc(loc);
    if (!sector_lock(sec, &sector)) {
        return false;
    }

    old_aid = sector->roofs.art_ids[roof_id_from_loc(loc)];
    sector->roofs.art_ids[roof_id_from_loc(loc)] = aid;
    sector->roofs.empty = 0;

    sector_unlock(sec);

    if (aid == TIG_ART_ID_INVALID) {
        aid = old_aid;
    }

    roof_xy(loc, &sx, &sy);
    if (sx > INT_MIN
        && sx < INT_MAX
        && sy > INT_MIN
        && sy < INT_MAX) {
        sub_43A140((int)sx, (int)sy, aid, &rect);
        roof_iso_window_invalidate_rect(&rect);
    }

    return true;
}

// 0x439890
bool roof_hit_test(int x, int y)
{
    int64_t loc;
    int64_t loc_x;
    int64_t loc_y;
    tig_art_id_t aid;
    TigRect rect;

    if (!roof_enabled) {
        return false;
    }

    if (!location_at(x, y + 120, &loc)) {
        return false;
    }

    aid = roof_art_id_get(loc);
    if (aid == TIG_ART_ID_INVALID) {
        return false;
    }

    if (tig_art_roof_id_fill_get(aid)) {
        return false;
    }

    roof_xy(loc, &loc_x, &loc_y);
    sub_43A140((int)loc_x, (int)loc_y, aid, &rect);
    if (x >= rect.x
        && y >= rect.y
        && x < rect.x + rect.width
        && y < rect.y + rect.height
        && sub_502FD0(aid, x - rect.x, y - rect.y) == TIG_OK) {
        return true;
    }

    return false;
}

// 0x439D30
void roof_recalc(int64_t loc)
{
    tig_art_id_t aid;
    int piece;
    int tile;
    int v5;
    int v6;
    bool fill;

    aid = roof_art_id_get(loc);
    if (aid == TIG_ART_ID_INVALID) {
        return;
    }

    piece = tig_art_roof_id_piece_get(aid);
    tile = tile_id_from_loc(loc);
    v5 = tile & 3;
    v6 = (tile >> 6) % 4;

    switch (piece) {
    case TIG_ART_ROOF_PIECE_NORTH_WEST_OUTSIDE:
        fill = v6 > 1 && v5 < 3;
        break;
    case TIG_ART_ROOF_PIECE_WEST:
        fill = v5 < 3;
        break;
    case TIG_ART_ROOF_PIECE_NORTH:
        fill = v6 > 1;
        break;
    case TIG_ART_ROOF_PIECE_NORTH_WEST_INSIDE:
        if (v5 >= 3 && v6 <= 1) {
            fill = false;
        } else {
            fill = true;
        }
        break;
    case TIG_ART_ROOF_PIECE_SOUTH_WEST_OUTSIDE:
        if (v5 >= 3) {
            fill = false;
        } else {
            if (v6 < 3) {
                fill = true;
            } else {
                fill = false;
            }
        }
        break;
    case TIG_ART_ROOF_PIECE_SOUTH_WEST_INSIDE:
        if (v5 >= 3) {
            if (v6 < 3) {
                fill = true;
            } else {
                fill = false;
            }
        } else {
            fill = true;
        }
        break;
    case TIG_ART_ROOF_PIECE_NORTH_EAST_INSIDE:
        fill = v5 > 1 || v6 > 1;
        break;
    case TIG_ART_ROOF_PIECE_NORTH_EAST_OUTSIDE:
        if (v5 <= 1 || v6 <= 1) {
            fill = false;
        } else {
            fill = true;
        }
        break;
    case TIG_ART_ROOF_PIECE_CENTER:
        fill = true;
        break;
    case TIG_ART_ROOF_PIECE_SOUTH_EAST_OUTSIDE:
        if (v5 <= 1) {
            fill = false;
        } else {
            if (v6 < 3) {
                fill = true;
            } else {
                fill = false;
            }
        }
        break;
    case TIG_ART_ROOF_PIECE_SOUTH:
        fill = v6 < 3;
        break;
    case TIG_ART_ROOF_PIECE_EAST:
        fill = v5 > 1;
        break;
    case TIG_ART_ROOF_PIECE_SOUTH_EAST_INSIDE:
        fill = v5 > 1 || v6 < 3;
        break;
    default:
        // Should be unreachable.
        assert(0);
    }

    if (tig_art_roof_id_fill_get(aid) != fill) {
        roof_fill(roof_normalize_loc(loc), fill, -1);
    }
}

// 0x439EA0
void roof_fill_off(int64_t loc)
{
    roof_fill(roof_normalize_loc(loc), false, -1);
}

// 0x439EC0
void roof_fill_on(int64_t loc)
{
    roof_fill(roof_normalize_loc(loc), true, -1);
}

// 0x439EE0
void roof_fade_on(int64_t loc)
{
    tig_art_id_t aid;

    aid = roof_art_id_get(loc);
    if (!tig_art_roof_id_fade_get(aid)) {
        aid = tig_art_roof_id_fade_set(aid, 1);
        roof_art_id_set(loc, aid);
    }
}

// 0x439F20
void roof_fade_off(int64_t loc)
{
    tig_art_id_t aid;

    aid = roof_art_id_get(loc);
    if (tig_art_roof_id_fade_get(aid)) {
        aid = tig_art_roof_id_fade_set(aid, 0);
        roof_art_id_set(loc, aid);
    }
}

// 0x439FA0
bool roof_is_faded(int64_t loc)
{
    tig_art_id_t aid;

    if (roof_enabled) {
        aid = roof_art_id_get(loc);
        if (aid != TIG_ART_ID_INVALID
            && !tig_art_roof_id_fill_get(aid)
            && tig_art_roof_id_fade_get(aid)) {
            return true;
        }
    }

    return false;
}

// 0x439FF0
bool roof_is_covered_xy(int64_t x, int64_t y, bool check_faded)
{
    int64_t loc;

    location_at(x, y, &loc);
    return roof_is_covered_loc(loc, check_faded);
}

// 0x43A030
bool roof_is_covered_loc(int64_t loc, bool check_faded)
{
    tig_art_id_t aid;
    int piece;
    int row;
    int col;

    if (!roof_enabled) {
        return false;
    }

    aid = tile_art_id_at(loc);
    if (tig_art_tile_id_type_get(aid) != 0) {
        return false;
    }

    aid = roof_art_id_get(location_make(location_get_x(loc) + 3, location_get_y(loc) + 3));
    if (aid == TIG_ART_ID_INVALID) {
        return false;
    }

    if (tig_art_roof_id_fill_get(aid)) {
        return false;
    }

    piece = tig_art_roof_id_piece_get(aid);
    row = (location_get_y(loc) + 3) % 4;
    col = (location_get_x(loc) + 3) % 4;
    if (!byte_5A53A4[piece][row][col]) {
        return false;
    }

    if (check_faded) {
        return true;
    }

    if (tig_art_roof_id_fade_get(aid)) {
        return false;
    }

    return true;
}

// 0x43A110
void roof_blit_flags_set(unsigned int flags)
{
    roof_blit_flags = flags;
    if (roof_iso_window_invalidate_rect != NULL) {
        roof_iso_window_invalidate_rect(NULL);
    }
}

// 0x43A130
unsigned int roof_blit_flags_get()
{
    return roof_blit_flags;
}

// 0x43A140
void sub_43A140(int x, int y, tig_art_id_t aid, TigRect* rect)
{
    TigArtFrameData art_frame_data;

    rect->x = 0;
    rect->y = 0;
    rect->width = 0;
    rect->height = 0;

    if (tig_art_frame_data(aid, &art_frame_data) == TIG_OK) {
        rect->x = x - art_frame_data.hot_x;
        rect->y = y - art_frame_data.hot_y;
        rect->width = art_frame_data.width;
        rect->height = art_frame_data.height;
    }
}

// 0x43A1A0
void roof_fill(int64_t loc, bool fill, int a3)
{
    tig_art_id_t aid;
    int piece;

    aid = roof_art_id_get(loc);
    if (aid == TIG_ART_ID_INVALID) {
        return;
    }

    if (tig_art_roof_id_fill_get(aid) == fill) {
        return;
    }

    if (a3 != -1) {
        piece = tig_art_roof_id_piece_get(aid);
        switch (a3) {
        case 1:
            switch (piece) {
            case TIG_ART_ROOF_PIECE_NORTH_WEST_OUTSIDE:
            case TIG_ART_ROOF_PIECE_WEST:
            case TIG_ART_ROOF_PIECE_SOUTH_WEST_OUTSIDE:
                return;
            };
            break;
        case 3:
            switch (piece) {
            case TIG_ART_ROOF_PIECE_NORTH_WEST_OUTSIDE:
            case TIG_ART_ROOF_PIECE_NORTH:
            case TIG_ART_ROOF_PIECE_NORTH_EAST_OUTSIDE:
                return;
            }
            break;
        case 5:
            switch (piece) {
            case TIG_ART_ROOF_PIECE_SOUTH_EAST_OUTSIDE:
            case TIG_ART_ROOF_PIECE_EAST:
            case TIG_ART_ROOF_PIECE_NORTH_EAST_OUTSIDE:
                return;
            }
            break;
        case 7:
            switch (piece) {
            case TIG_ART_ROOF_PIECE_SOUTH_WEST_OUTSIDE:
            case TIG_ART_ROOF_PIECE_SOUTH:
            case TIG_ART_ROOF_PIECE_SOUTH_EAST_OUTSIDE:
                return;
            }
            break;
        }
    }

    aid = tig_art_roof_id_fill_set(aid, fill);
    roof_art_id_set(loc, aid);

    roof_fill(location_make(location_get_x(loc) + 4, location_get_y(loc)), fill, 5);
    roof_fill(location_make(location_get_x(loc) - 4, location_get_y(loc)), fill, 1);
    roof_fill(location_make(location_get_x(loc), location_get_y(loc) + 4), fill, 3);
    roof_fill(location_make(location_get_x(loc), location_get_y(loc) - 4), fill, 7);
}
