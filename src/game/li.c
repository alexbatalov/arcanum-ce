#include "game/li.h"

static void li_blit(void);

// 0x5FC3F8
static tig_art_id_t li_indicator_art_id;

// 0x5FC400
static TigRect li_indicator_frame;

// 0x5FC410
static TigArtBlitInfo li_indicator_blit_info;

// 0x5FC43C
static int dword_5FC43C;

// 0x5FC440
static TigVideoBuffer* li_indicator_vb;

// 0x5FC448
static TigRect li_indicator_bounds;

// 0x4BB9A0
bool li_init(GameInitInfo* init_info)
{
    TigArtFrameData art_frame_data;
    TigWindowData window_data;
    TigVideoBufferCreateInfo vb_create_info;

    tig_art_interface_id_create(601, 0, 0, 0, &li_indicator_art_id);

    if (tig_art_frame_data(li_indicator_art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    vb_create_info.width = art_frame_data.width;
    vb_create_info.height = art_frame_data.height;
    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_COLOR_KEY | TIG_VIDEO_BUFFER_CREATE_VIDEO_MEMORY;
    vb_create_info.color_key = tig_color_make(0, 0, 255);
    vb_create_info.background_color = vb_create_info.color_key;
    if (tig_video_buffer_create(&vb_create_info, &li_indicator_vb) != TIG_OK) {
        return false;
    }

    li_indicator_bounds.x = 0;
    li_indicator_bounds.y = 0;
    li_indicator_bounds.width = art_frame_data.width;
    li_indicator_bounds.height = art_frame_data.height;

    li_indicator_frame.x = window_data.rect.width - art_frame_data.width + window_data.rect.x - 2;
    li_indicator_frame.y = window_data.rect.y + 2;
    li_indicator_frame.width = art_frame_data.width;
    li_indicator_frame.height = art_frame_data.height;

    li_indicator_blit_info.flags = 0;
    li_indicator_blit_info.src_rect = &li_indicator_bounds;
    li_indicator_blit_info.dst_video_buffer = li_indicator_vb;
    li_indicator_blit_info.dst_rect = &li_indicator_bounds;

    li_blit();

    return true;
}

// 0x4BBB00
void li_exit(void)
{
    if (li_indicator_vb != NULL) {
        tig_video_buffer_destroy(li_indicator_vb);
        li_indicator_vb = NULL;
    }
}

// 0x4BBB20
void li_resize(GameResizeInfo* resize_info)
{
    TigArtFrameData art_frame_data;
    TigWindowData window_data;

    tig_art_interface_id_create(601, 0, 0, 0, &li_indicator_art_id);

    if (tig_art_frame_data(li_indicator_art_id, &art_frame_data) != TIG_OK) {
        tig_debug_printf("li_resize: ERROR: couldn't grab frame data!\n");
        exit(EXIT_FAILURE);
    }

    if (tig_window_data(resize_info->window_handle, &window_data) != TIG_OK) {
        tig_debug_printf("li_resize: ERROR: couldn't grab window data!");
        exit(EXIT_FAILURE);
    }

    li_indicator_bounds.x = 0;
    li_indicator_bounds.y = 0;
    li_indicator_bounds.width = art_frame_data.width;
    li_indicator_bounds.height = art_frame_data.height;

    li_indicator_frame.x = window_data.rect.width - art_frame_data.width + window_data.rect.x - 2;
    li_indicator_frame.y = window_data.rect.y + 2;
    li_indicator_frame.width = art_frame_data.width;
    li_indicator_frame.height = art_frame_data.height;

    li_blit();
}

// 0x4BBC00
void sub_4BBC00(void)
{
    dword_5FC43C++;
}

// 0x4BBC10
void sub_4BBC10(void)
{
    dword_5FC43C--;
}

// 0x4BBC20
int sub_4BBC20(void)
{
    return dword_5FC43C;
}

// 0x4BBC30
void li_update(void)
{
    if (dword_5FC43C <= 0) {
        tig_video_blit(li_indicator_vb, &li_indicator_bounds, &li_indicator_frame);
        li_indicator_art_id = tig_art_id_frame_inc(li_indicator_art_id);
        li_blit();
        tig_window_invalidate_rect(&li_indicator_frame);
    }
}

// 0x4BBC80
void li_redraw(void)
{
    tig_video_buffer_fill(li_indicator_vb, &li_indicator_bounds, tig_color_make(0, 0, 255));
    li_blit();
}

// 0x4BBCE0
void li_blit(void)
{
    li_indicator_blit_info.art_id = li_indicator_art_id;
    tig_art_blit(&li_indicator_blit_info);
}
