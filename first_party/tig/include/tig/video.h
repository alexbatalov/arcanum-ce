#ifndef TIG_VIDEO_H_
#define TIG_VIDEO_H_

#include "tig/color.h"
#include "tig/rect.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned TigFadeFlags;

#define TIG_FADE_OUT 0x0u
#define TIG_FADE_IN 0x1u

typedef unsigned int TigVideoBufferCreateFlags;

#define TIG_VIDEO_BUFFER_CREATE_COLOR_KEY 0x0001
#define TIG_VIDEO_BUFFER_CREATE_VIDEO_MEMORY 0x0002
#define TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY 0x0004
#define TIG_VIDEO_BUFFER_CREATE_RENDER_TARGET 0x0008
#define TIG_VIDEO_BUFFER_CREATE_TEXTURE 0x0010

typedef unsigned int TigVideoBufferFlags;

#define TIG_VIDEO_BUFFER_LOCKED 0x0001
#define TIG_VIDEO_BUFFER_COLOR_KEY 0x0002
#define TIG_VIDEO_BUFFER_VIDEO_MEMORY 0x0004
#define TIG_VIDEO_BUFFER_SYSTEM_MEMORY 0x0008
#define TIG_VIDEO_BUFFER_RENDER_TARGET 0x0010
#define TIG_VIDEO_BUFFER_TEXTURE 0x0020

typedef unsigned int TigVideoBufferBlitFlags;

#define TIG_VIDEO_BUFFER_BLIT_FLIP_X 0x0001
#define TIG_VIDEO_BUFFER_BLIT_FLIP_Y 0x0002
#define TIG_VIDEO_BUFFER_BLIT_BLEND_ADD 0x0004
#define TIG_VIDEO_BUFFER_BLIT_BLEND_MUL 0x0010
#define TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_AVG 0x0020
#define TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_CONST 0x0040
#define TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_SRC 0x0080
#define TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_LERP 0x0100
#define TIG_VIDEO_BUFFER_BLIT_BLEND_COLOR_CONST 0x0200
#define TIG_VIDEO_BUFFER_BLIT_BLEND_COLOR_LERP 0x0400

#define TIG_VIDEO_BUFFER_BLIT_FLIP_ANY (TIG_VIDEO_BUFFER_BLIT_FLIP_X | TIG_VIDEO_BUFFER_BLIT_FLIP_Y)

// Opaque handle.
typedef struct TigVideoBuffer TigVideoBuffer;

typedef struct TigVideoBufferCreateInfo {
    /* 0000 */ TigVideoBufferCreateFlags flags;
    /* 0004 */ int width;
    /* 0008 */ int height;
    /* 000C */ unsigned int background_color;
    /* 0010 */ unsigned int color_key;
} TigVideoBufferCreateInfo;

typedef struct TigVideoBufferData {
    /* 0000 */ TigVideoBufferFlags flags;
    /* 0004 */ int width;
    /* 0008 */ int height;
    /* 000C */ int pitch;
    /* 0010 */ int background_color;
    /* 0014 */ int color_key;
    /* 0018 */ int bpp;
    /* 001C */ union {
        void* pixels;
        uint8_t* p8;
        uint16_t* p16;
        uint32_t* p32;
    } surface_data;
} TigVideoBufferData;

typedef struct TigVideoBufferBlitInfo {
    /* 0000 */ TigVideoBufferBlitFlags flags;
    /* 0004 */ TigVideoBuffer* src_video_buffer;
    /* 0008 */ TigRect* src_rect;
    /* 000C */ uint8_t alpha[4];
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ int field_18;
    /* 001C */ int field_1C;
    /* 0020 */ TigRect* field_20;
    /* 0024 */ TigVideoBuffer* dst_video_buffer;
    /* 0028 */ TigRect* dst_rect;
} TigVideoBufferBlitInfo;

typedef struct TigVideoScreenshotSettings {
    /* 0000 */ int key;
    /* 0004 */ int field_4;
} TigVideoScreenshotSettings;

typedef enum TigVideoBufferTintMode {
    TIG_VIDEO_BUFFER_TINT_MODE_ADD,
    TIG_VIDEO_BUFFER_TINT_MODE_SUB,
    TIG_VIDEO_BUFFER_TINT_MODE_MUL,
    TIG_VIDEO_BUFFER_TINT_MODE_GRAYSCALE,
    TIG_VIDEO_BUFFER_TINT_MODE_COUNT,
} TigVideoBufferTintMode;

typedef struct TigVideoBufferSaveToBmpInfo {
    /* 0000 */ unsigned int flags;
    /* 0004 */ TigVideoBuffer* video_buffer;
    /* 0008 */ char path[TIG_MAX_PATH];
    /* 010C */ TigRect* rect;
} TigVideoBufferSaveToBmpInfo;

int tig_video_init(TigInitInfo* init_info);
void tig_video_exit();
int tig_video_window_get(SDL_Window** window_ptr);
int tig_video_renderer_get(SDL_Renderer** renderer_ptr);
void tig_video_display_fps();
int tig_video_blit(TigVideoBuffer* src_video_buffer, TigRect* src_rect, TigRect* dst_rect);
int tig_video_fill(const TigRect* rect, tig_color_t color);
int tig_video_flip();
int tig_video_screenshot_set_settings(TigVideoScreenshotSettings* settings);
int tig_video_screenshot_make();
int tig_video_get_bpp(int* bpp);
int tig_video_get_palette(unsigned int* colors);
int tig_video_3d_check_initialized();
int tig_video_3d_check_hardware();
int tig_video_3d_begin_scene();
int tig_video_3d_end_scene();
int tig_video_check_gamma_control();
int tig_video_fade(tig_color_t color, int steps, float duration, TigFadeFlags flags);
int tig_video_set_gamma(float gamma);
int tig_video_buffer_create(TigVideoBufferCreateInfo* vb_create_info, TigVideoBuffer** video_buffer);
int tig_video_buffer_destroy(TigVideoBuffer* video_buffer);
int tig_video_buffer_data(TigVideoBuffer* video_buffer, TigVideoBufferData* video_buffer_data);
int tig_video_buffer_set_color_key(TigVideoBuffer* video_buffer, int color_key);
int tig_video_buffer_lock(TigVideoBuffer* video_buffer);
int tig_video_buffer_unlock(TigVideoBuffer* video_buffer);
int tig_video_buffer_outline(TigVideoBuffer* video_buffer, TigRect* rect, int color);
int tig_video_buffer_fill(TigVideoBuffer* video_buffer, TigRect* rect, int color);
int tig_video_buffer_line(TigVideoBuffer* video_buffer, TigLine* line, TigRect* a3, unsigned int color);
int sub_520FB0(TigVideoBuffer* video_buffer, unsigned int flags);
int tig_video_buffer_blit(TigVideoBufferBlitInfo* blit_info);
int tig_video_buffer_get_pixel_color(TigVideoBuffer* video_buffer, int x, int y, unsigned int* color);
int tig_video_buffer_tint(TigVideoBuffer* video_buffer, TigRect* rect, tig_color_t tint_color, TigVideoBufferTintMode mode);
int tig_video_buffer_save_to_bmp(TigVideoBufferSaveToBmpInfo* save_info);
int tig_video_buffer_load_from_bmp(const char* filename, TigVideoBuffer** video_buffer_ptr, unsigned int flags);

#ifdef __cplusplus
}
#endif

#endif /* TIG_VIDEO_H_ */
