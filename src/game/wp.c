#include "game/wp.h"

#include "game/location.h"
#include "game/obj.h"
#include "game/obj_private.h"

typedef struct S5FC668 {
    /* 0000 */ int64_t obj;
    /* 0008 */ tig_color_t field_8;
    /* 000C */ int field_C;
} S5FC668;

static void sub_4C0090(void);
static void sub_4C00D0(int64_t obj);
static void sub_4C0140(S5FC668* a1, GameDrawInfo* draw_info);
static bool sub_4C0280(int64_t location, TigRect* rect);
static void sub_4C0370(void);

// 0x5FC638
static int wp_iso_art_num_frames;

// 0x5FC640
static ViewOptions wp_view_options;

// 0x5FC648
static IsoInvalidateRectFunc* wp_iso_window_invalidate_rect;

// 0x5FC64C
static bool wp_enabled;

// 0x5FC650
static bool wp_initialized;

// 0x5FC654
static int dword_5FC654;

// 0x5FC658
static tig_art_id_t wp_iso_art_id;

// 0x5FC65C
static bool wp_editor;

// 0x5FC660
static tig_window_handle_t wp_iso_window_handle;

// 0x5FC668
static S5FC668 stru_5FC668[32];

// 0x4BFCE0
bool wp_init(GameInitInfo* init_info)
{
    TigArtAnimData art_anim_data;
    int index;

    wp_iso_window_handle = init_info->iso_window_handle;
    wp_iso_window_invalidate_rect = init_info->invalidate_rect_func;
    wp_editor = init_info->editor;
    wp_view_options.type = VIEW_TYPE_ISOMETRIC;

    tig_art_interface_id_create(672, 0, 0, 0, &wp_iso_art_id);

    if (tig_art_anim_data(wp_iso_art_id, &art_anim_data) != TIG_OK) {
        return false;
    }

    wp_iso_art_num_frames = art_anim_data.num_frames;

    for (index = 0; index < 32; index++) {
        stru_5FC668[index].obj = OBJ_HANDLE_NULL;
    }

    wp_enabled = true;
    wp_initialized = true;

    return true;
}

// 0x4BFD80
void wp_exit(void)
{
    int index;

    for (index = 0; index < 32; index++) {
        stru_5FC668[index].obj = OBJ_HANDLE_NULL;
    }

    wp_initialized = false;
}

// 0x4BFDA0
void wp_resize(GameResizeInfo* resize_info)
{
    wp_iso_window_handle = resize_info->window_handle;
}

// 0x4BFDB0
void wp_update_view(ViewOptions* view_options)
{
    wp_view_options = *view_options;
}

// 0x4BFDD0
bool wp_is_enabled(void)
{
    return wp_enabled;
}

// 0x4BFDE0
void wp_toggle(void)
{
    wp_enabled = !wp_enabled;
}

// 0x4BFE00
void wp_draw(GameDrawInfo* draw_info)
{
    int index;

    if (!wp_enabled) {
        return;
    }

    if (wp_view_options.type == VIEW_TYPE_TOP_DOWN) {
        return;
    }

    sub_4C0370();

    for (index = 0; index < dword_5FC654; index++) {
        sub_4C0140(&(stru_5FC668[index]), draw_info);
    }
}

// 0x4C0090
void sub_4C0090(void)
{
    int index;

    if (!wp_editor) {
        return;
    }

    sub_4C0370();

    for (index = 0; index < dword_5FC654; index++) {
        sub_4C00D0(stru_5FC668[index].obj);
    }
}

// 0x4C00D0
void sub_4C00D0(int64_t obj)
{
    int cnt;
    int index;
    int64_t waypoint;
    TigRect rect;

    if (wp_view_options.type == VIEW_TYPE_TOP_DOWN) {
        return;
    }

    cnt = obj_arrayfield_length_get(obj, OBJ_F_NPC_WAYPOINTS_IDX);
    for (index = 0; index < cnt; index++) {
        waypoint = obj_arrayfield_int64_get(obj, OBJ_F_NPC_WAYPOINTS_IDX, index);
        if (sub_4C0280(waypoint, &rect)) {
            wp_iso_window_invalidate_rect(&rect);
        }
    }
}

// 0x4C0140
void sub_4C0140(S5FC668* a1, GameDrawInfo* draw_info)
{
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int cnt;
    int idx;
    int64_t loc;
    TigRect wp_rect;
    TigRectListNode* node;
    tig_art_id_t art_id;

    if (wp_view_options.type == VIEW_TYPE_TOP_DOWN) {
        return;
    }

    art_blit_info.flags = TIG_ART_BLT_BLEND_ADD | TIG_ART_BLT_BLEND_COLOR_CONST;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;
    art_blit_info.color = a1->field_8;

    cnt = obj_arrayfield_length_get(a1->obj, OBJ_F_NPC_WAYPOINTS_IDX);
    for (idx = 0; idx < cnt; idx++) {
        art_id = TIG_ART_ID_INVALID;
        loc = obj_arrayfield_int64_get(a1->obj, OBJ_F_NPC_WAYPOINTS_IDX, idx);
        if (sub_4C0280(loc, &wp_rect)) {
            node = *draw_info->rects;
            while (node != NULL) {
                if (tig_rect_intersection(&wp_rect, &(node->rect), &dst_rect) == TIG_OK) {
                    if (art_id == TIG_ART_ID_INVALID) {
                        art_id = tig_art_id_frame_set(wp_iso_art_id, idx < wp_iso_art_num_frames ? idx : wp_iso_art_num_frames - 1);
                        art_blit_info.art_id = art_id;
                    }

                    src_rect.x = dst_rect.x - wp_rect.x;
                    src_rect.y = dst_rect.y - wp_rect.y;
                    src_rect.width = dst_rect.width;
                    src_rect.height = dst_rect.height;
                    tig_window_blit_art(wp_iso_window_handle, &art_blit_info);
                }
                node = node->next;
            }
        }
    }
}

// 0x4C0280
bool sub_4C0280(int64_t location, TigRect* rect)
{
    int64_t x;
    int64_t y;
    TigArtFrameData art_frame_data;

    location_xy(location, &x, &y);

    if (x < INT_MIN
        || x >= INT_MAX
        || y < INT_MIN
        || y >= INT_MAX) {
        return false;
    }

    if (wp_view_options.type == VIEW_TYPE_TOP_DOWN) {
        return false;
    }

    if (tig_art_frame_data(wp_iso_art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    rect->x = (int)x - art_frame_data.hot_x + 40;
    rect->y = (int)y - (40 - art_frame_data.height) / 2 - art_frame_data.hot_y + 40;
    rect->width = art_frame_data.width;
    rect->height = art_frame_data.height;

    return true;
}

// 0x4C0370
void sub_4C0370(void)
{
    int idx;

    for (idx = 0; idx < dword_5FC654; idx++) {
        if (stru_5FC668[idx].obj == OBJ_HANDLE_NULL
            || !obj_handle_is_valid(stru_5FC668[idx].obj)
            || obj_field_int32_get(stru_5FC668[idx].obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
            dword_5FC654--;
            memmove(&(stru_5FC668[idx]),
                &(stru_5FC668[idx + 1]),
                sizeof(*stru_5FC668) * (dword_5FC654 + idx));
            idx--;
        }
    }
}
