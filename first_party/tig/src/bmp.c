#include "tig/bmp.h"

#include "tig/color.h"
#include "tig/file.h"
#include "tig/memory.h"
#include "tig/video.h"
#include "tig/window.h"

// 0x535D80
int tig_bmp_create(TigBmp* bmp)
{
    SDL_IOStream* io;
    SDL_Surface* surface;
    const SDL_PixelFormatDetails* fmt;
    SDL_Palette* palette;
    int data_size;
    int index;

    if (bmp == NULL) {
        return TIG_ERR_INVALID_PARAM;
    }

    bmp->pixels = NULL;

    io = tig_file_io_open(bmp->name, "rb");
    if (io == NULL) {
        return TIG_ERR_IO;
    }

    surface = SDL_LoadBMP_IO(io, true);
    if (surface == NULL) {
        return TIG_ERR_GENERIC;
    }

    fmt = SDL_GetPixelFormatDetails(surface->format);
    if (fmt == NULL) {
        SDL_DestroySurface(surface);
        return TIG_ERR_GENERIC;
    }

    if (fmt->bits_per_pixel != 4
        && fmt->bits_per_pixel != 8
        && fmt->bits_per_pixel != 24) {
        SDL_DestroySurface(surface);
        return TIG_ERR_GENERIC;
    }

    bmp->bpp = fmt->bits_per_pixel;

    palette = SDL_GetSurfacePalette(surface);
    if (palette != NULL) {
        for (index = 0; index < palette->ncolors; index++) {
            bmp->palette[index] = tig_color_make(palette->colors[index].r,
                palette->colors[index].g,
                palette->colors[index].b);
        }
    }

    bmp->pitch = surface->pitch;
    bmp->width = surface->w;
    bmp->height = surface->h;

    data_size = bmp->pitch * bmp->height;
    bmp->pixels = MALLOC(data_size);
    if (bmp->pixels == NULL) {
        SDL_DestroySurface(surface);
        return TIG_ERR_OUT_OF_MEMORY;
    }

    memcpy(bmp->pixels, surface->pixels, data_size);

    SDL_DestroySurface(surface);

    return TIG_OK;
}

// 0x5360D0
int tig_bmp_destroy(TigBmp* bmp)
{
    if (bmp->pixels != NULL) {
        FREE(bmp->pixels);
    }

    return TIG_OK;
}
