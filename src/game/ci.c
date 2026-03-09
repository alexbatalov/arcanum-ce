#include "game/ci.h"

static void ci_blit(void);

// 0x5FC398
static tig_art_id_t ci_indicator_art_id;

// 0x5FC3A0
static TigRect ci_indicator_frame;

// 0x5FC3B0
static TigArtBlitInfo ci_indicator_blit_info;

// 0x5FC3DC
static int dword_5FC3DC;

// 0x5FC3E0
static TigVideoBuffer* ci_indicator_vb;

// 0x5FC3E8
static TigRect ci_indicator_bounds;

// 0x4BB720
bool ci_init(GameInitInfo* init_info)
{
    TigArtFrameData art_frame_data;
    TigWindowData window_data;
    TigVideoBufferCreateInfo vb_create_info;

    tig_art_interface_id_create(601, 0, 0, 0, &ci_indicator_art_id);

    if (tig_art_frame_data(ci_indicator_art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    // FIXME: Does the same thing as above.
    if (tig_art_frame_data(ci_indicator_art_id, &art_frame_data) != TIG_OK) {
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
    if (tig_video_buffer_create(&vb_create_info, &ci_indicator_vb) != TIG_OK) {
        return false;
    }

    ci_indicator_bounds.x = 0;
    ci_indicator_bounds.y = 0;
    ci_indicator_bounds.width = art_frame_data.width;
    ci_indicator_bounds.height = art_frame_data.height;

    ci_indicator_frame.x = window_data.rect.width - art_frame_data.width + window_data.rect.x - 2;
    ci_indicator_frame.y = window_data.rect.y + art_frame_data.height + 2 + 2;
    ci_indicator_frame.width = art_frame_data.width;
    ci_indicator_frame.height = art_frame_data.height;

    ci_indicator_blit_info.flags = 0;
    ci_indicator_blit_info.src_rect = &ci_indicator_bounds;
    ci_indicator_blit_info.dst_video_buffer = ci_indicator_vb;
    ci_indicator_blit_info.dst_rect = &ci_indicator_bounds;

    ci_blit();

    return true;
}

// 0x4BB8C0
void ci_exit(void)
{
    if (ci_indicator_vb != NULL) {
        tig_video_buffer_destroy(ci_indicator_vb);
        ci_indicator_vb = NULL;
    }
}

// 0x4BB8E0
void sub_4BB8E0(void)
{
    dword_5FC3DC++;
}

// 0x4BB8F0
void sub_4BB8F0(void)
{
    dword_5FC3DC--;
}

// 0x4BB900
int sub_4BB900(void)
{
    return dword_5FC3DC;
}

// 0x4BB910
void sub_4BB910(void)
{
}

// 0x4BB920
void ci_redraw(void)
{
    tig_video_buffer_fill(ci_indicator_vb, &ci_indicator_bounds, tig_color_make(0, 0, 255));
    ci_blit();
}

// 0x4BB980
void ci_blit(void)
{
    ci_indicator_blit_info.art_id = ci_indicator_art_id;
    tig_art_blit(&ci_indicator_blit_info);
}
