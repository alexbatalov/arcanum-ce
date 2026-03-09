#include "game/facade.h"

#include "game/tile.h"
#include "game/walkmask.h"

static void sub_4CA0F0(int a1, int a2, int a3);
static void sub_4CA240(void);
static void sub_4CA7C0(TigRect* rect);

// 0x5FF570
static int dword_5FF570;

// 0x5FF574
static int dword_5FF574;

// 0x5FF578
static int64_t qword_5FF578;

// 0x5FF580
static IsoInvalidateRectFunc* facade_iso_invalidate_rect;

// 0x5FF584
static bool facade_editor;

// 0x5FF588
static int dword_5FF588;

// 0x5FF58C
static tig_window_handle_t facade_iso_window_handle;

// 0x5FF590
static bool facade_initialized;

// 0x5FF598
static ViewOptions facade_view_options;

// 0x5FF5A0
static tig_art_id_t* dword_5FF5A0;

// 0x5FF5A4
static TigVideoBuffer** dword_5FF5A4;

// 0x4C9DA0
bool facade_init(GameInitInfo* init_info)
{
    facade_iso_window_handle = init_info->iso_window_handle;
    facade_iso_invalidate_rect = init_info->invalidate_rect_func;
    facade_editor = init_info->editor;
    facade_view_options.type = VIEW_TYPE_ISOMETRIC;
    facade_initialized = true;

    return true;
}

// 0x4C9DE0
void facade_exit(void)
{
    sub_4CA240();
    facade_initialized = false;
}

// 0x4C9DF0
void facade_resize(GameResizeInfo* resize_info)
{
    facade_iso_window_handle = resize_info->window_handle;
}

// 0x4C9E00
void facade_update_view(ViewOptions* view_options)
{
    if (dword_5FF5A0 != NULL) {
        sub_4CA240();
        facade_view_options = *view_options;
        sub_4CA0F0(dword_5FF588, LOCATION_GET_X(qword_5FF578), LOCATION_GET_Y(qword_5FF578));
        facade_iso_invalidate_rect(NULL);
    } else {
        facade_view_options = *view_options;
    }
}

// 0x4C9E70
void facade_draw(GameDrawInfo* draw_info)
{
    LocRect loc_rect;
    int start_x;
    int start_y;
    TigRect tile_rect;
    int64_t y;
    int64_t x;
    TigArtBlitInfo art_blit_info;
    int index;
    int64_t tile_x;
    int64_t tile_y;
    TigRectListNode* rect_node;
    TigRect src_rect;
    TigRect dst_rect;

    if (dword_5FF5A0 == NULL) {
        return;
    }

    loc_rect = *draw_info->loc_rect;
    if (!sub_4CA6B0(&loc_rect, &start_x, &start_y)) {
        return;
    }

    if (facade_view_options.type == VIEW_TYPE_TOP_DOWN) {
        tile_rect.width = facade_view_options.zoom;
        tile_rect.height = facade_view_options.zoom;
    } else {
        tile_rect.width = 80;
        tile_rect.height = 40;
    }

    art_blit_info.flags = 0;

    y = loc_rect.y1;
    while (y <= loc_rect.y2) {
        index = start_y * dword_5FF574 + start_x;

        x = loc_rect.x1;
        while (x <= loc_rect.x2) {
            art_blit_info.art_id = dword_5FF5A0[index];
            if (art_blit_info.art_id != TIG_ART_ID_INVALID) {
                location_xy(LOCATION_MAKE(x, y), &tile_x, &tile_y);
                tile_rect.x = (int)tile_x;
                tile_rect.y = (int)tile_y;

                if (facade_view_options.type == VIEW_TYPE_ISOMETRIC) {
                    tile_rect.x++;
                }

                rect_node = *draw_info->rects;
                while (rect_node != NULL) {
                    if (tig_rect_intersection(&tile_rect, &(rect_node->rect), &src_rect) == TIG_OK) {
                        dst_rect = src_rect;

                        src_rect.x -= (int)tile_x;
                        src_rect.y -= (int)tile_y;

                        art_blit_info.src_rect = &src_rect;
                        art_blit_info.dst_rect = &dst_rect;

                        if (facade_view_options.type == VIEW_TYPE_ISOMETRIC) {
                            src_rect.x--;
                            tig_window_blit_art(facade_iso_window_handle, &art_blit_info);
                        } else {
                            tig_window_copy_from_vbuffer(facade_iso_window_handle, &dst_rect, dword_5FF5A4[index], &src_rect);
                        }
                    }
                    rect_node = rect_node->next;
                }
            }

            index++;
            x = LOCATION_MAKE(LOCATION_GET_X(x) + 1, LOCATION_GET_Y(x));
        }

        start_y++;
        y = LOCATION_MAKE(LOCATION_GET_X(y), LOCATION_GET_Y(y) + 1);
    }
}

// 0x4CA0F0
void sub_4CA0F0(int a1, int a2, int a3)
{
    TigRect rect;
    TigVideoBufferCreateInfo vb_create_info;
    int index;

    sub_4CA240();

    if (walkmask_load(a1, &dword_5FF5A0, &dword_5FF574, &dword_5FF570)) {
        dword_5FF588 = a1;
        qword_5FF578 = location_make(a2 - dword_5FF574 / 2, a3 - dword_5FF570 / 2);

        if (facade_view_options.type == VIEW_TYPE_TOP_DOWN) {
            dword_5FF5A4 = (TigVideoBuffer**)MALLOC(sizeof(*dword_5FF5A4) * dword_5FF570 * dword_5FF574);

            vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
            vb_create_info.width = facade_view_options.zoom;
            vb_create_info.height = facade_view_options.zoom;
            vb_create_info.background_color = 0;

            for (index = 0; index < dword_5FF570 * dword_5FF574; index++) {
                if (dword_5FF5A0[index] != TIG_ART_ID_INVALID) {
                    tig_video_buffer_create(&vb_create_info, &(dword_5FF5A4[index]));
                    sub_4D7590(dword_5FF5A0[index], dword_5FF5A4[index]);
                } else {
                    dword_5FF5A4[index] = NULL;
                }
            }
        }

        sub_4CA7C0(&rect);
        facade_iso_invalidate_rect(&rect);
    }
}

// 0x4CA240
void sub_4CA240(void)
{
    int index;

    if (dword_5FF5A4 != NULL) {
        for (index = 0; index < dword_5FF574 * dword_5FF570; index++) {
            if (dword_5FF5A4[index] != NULL) {
                tig_video_buffer_destroy(dword_5FF5A4[index]);
            }
        }
        FREE(dword_5FF5A4);
        dword_5FF5A4 = NULL;
    }

    if (dword_5FF5A0 != NULL) {
        FREE(dword_5FF5A0);
        dword_5FF5A0 = NULL;
    }
}

// 0x4CA2C0
void sub_4CA2C0(void)
{
    // TODO: Incomplete.
}

// 0x4CA6B0
bool sub_4CA6B0(LocRect* loc_rect, int* a2, int* a3)
{
    int64_t min_x;
    int64_t min_y;
    int64_t max_x;
    int64_t max_y;

    min_x = location_make(LOCATION_GET_X(qword_5FF578), 0);
    min_y = location_make(0, LOCATION_GET_Y(qword_5FF578));
    max_x = location_make(LOCATION_GET_X(qword_5FF578) + dword_5FF574 - 1, 0);
    max_y = location_make(0, LOCATION_GET_Y(qword_5FF578) + dword_5FF570 - 1);

    if (loc_rect->x1 < min_x) {
        loc_rect->x1 = min_x;
    }

    if (loc_rect->y1 < min_y) {
        loc_rect->y1 = min_y;
    }

    if (loc_rect->x2 >= max_x) {
        loc_rect->x2 = max_x;
    }

    if (loc_rect->y2 >= max_y) {
        loc_rect->y2 = max_y;
    }

    if (qword_5FF578 > loc_rect->y1) {
        loc_rect->y1 = qword_5FF578;
    }

    if (loc_rect->x2 - loc_rect->x1 + 1 < 0) {
        return false;
    }

    if (loc_rect->y2 - loc_rect->y1 + 1 < 0) {
        return false;
    }

    *a2 = (int)(loc_rect->x1 - min_x);
    *a3 = (int)(loc_rect->y1 - min_y);

    return true;
}

// 0x4CA7C0
void sub_4CA7C0(TigRect* rect)
{
    int64_t tmp;
    int64_t min_x;
    int64_t min_y;
    int64_t max_x;
    int64_t max_y;

    switch (facade_view_options.type) {
    case VIEW_TYPE_ISOMETRIC:
        location_xy(qword_5FF578, &tmp, &min_y);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(qword_5FF578) + dword_5FF574, LOCATION_GET_Y(qword_5FF578)), &min_x, &tmp);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(qword_5FF578), LOCATION_GET_Y(qword_5FF578) + dword_5FF574), &max_x, &tmp);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(qword_5FF578) + dword_5FF574, LOCATION_GET_Y(qword_5FF578) + dword_5FF574), &tmp, &max_y);
        max_x += 80;
        max_y += 40;
        break;
    case VIEW_TYPE_TOP_DOWN:
        location_xy(qword_5FF578, &min_x, &min_y);
        location_xy(LOCATION_MAKE(LOCATION_GET_X(qword_5FF578) + dword_5FF574, LOCATION_GET_Y(qword_5FF578) + dword_5FF574), &max_x, &max_y);
        max_x += facade_view_options.zoom;
        max_y += facade_view_options.zoom;
        break;
    default:
        // Should be unreachable.
        assert(0);
    }

    rect->x = (int)min_x;
    rect->y = (int)min_y;
    rect->width = (int)(max_x - min_x + 1);
    rect->height = (int)(max_y - min_y + 1);
}
