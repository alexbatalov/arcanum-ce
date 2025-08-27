#ifndef TIG_BMP_H_
#define TIG_BMP_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TigBmp {
    /* 0000 */ char name[TIG_MAX_PATH];
    /* 0104 */ int bpp;
    /* 0108 */ unsigned int palette[256];
    /* 0508 */ void* pixels;
    /* 050C */ int width;
    /* 0510 */ int height;
    /* 0514 */ int pitch;
} TigBmp;

int tig_bmp_create(TigBmp* bmp);
int tig_bmp_destroy(TigBmp* bmp);
int tig_bmp_copy_to_video_buffer(TigBmp* bmp, const TigRect* src_rect, TigVideoBuffer* video_buffer, const TigRect* dst_rect);
int tig_bmp_create_from_video_buffer(TigVideoBuffer* video_buffer, int a2, TigBmp* bmp);
int tig_bmp_copy_to_bmp(TigBmp* src, TigBmp* dst);
int tig_bmp_copy_to_window(TigBmp* bmp, const TigRect* src_rect, tig_window_handle_t window_handle, const TigRect* dst_rect);

#ifdef __cplusplus
}
#endif

#endif /* TIG_BMP_H_ */
