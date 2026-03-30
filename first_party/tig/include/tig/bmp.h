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

#ifdef __cplusplus
}
#endif

#endif /* TIG_BMP_H_ */
