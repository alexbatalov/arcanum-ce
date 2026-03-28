#ifndef TIG_COLOR_H_
#define TIG_COLOR_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int tig_color_red_mask;
extern unsigned int tig_color_green_mask;
extern unsigned int tig_color_blue_mask;
extern unsigned int tig_color_alpha_mask;

extern unsigned int tig_color_red_shift;
extern unsigned int tig_color_green_shift;
extern unsigned int tig_color_blue_shift;
extern unsigned int tig_color_alpha_shift;

extern uint8_t* tig_color_mult_table;

int tig_color_init(TigInitInfo* init_info);
void tig_color_exit(void);
int tig_color_set_rgba_settings(unsigned int red_mask, unsigned int green_mask, unsigned int blue_mask, unsigned int alpha_mask);
void tig_color_get_masks(unsigned int* red_mask, unsigned int* green_mask, unsigned int* blue_mask);
void tig_color_get_shifts(unsigned int* red_shift, unsigned int* green_shift, unsigned int* blue_shift);
tig_color_t tig_color_rgb_to_grayscale(tig_color_t color);
unsigned int tig_color_index_of(tig_color_t color);

// Creates platform-specific color from RGB components (0-255).
static inline tig_color_t tig_color_make(int red, int green, int blue)
{
    return tig_color_alpha_mask
        | (red << tig_color_red_shift)
        | (green << tig_color_green_shift)
        | (blue << tig_color_blue_shift);
}

// Extracts the 8-bit red component from a platform color.
static inline int tig_color_get_red(tig_color_t color)
{
    return (int)((color & tig_color_red_mask) >> tig_color_red_shift);
}

// Extracts the 8-bit green component from a platform color.
static inline int tig_color_get_green(tig_color_t color)
{
    return (int)((color & tig_color_green_mask) >> tig_color_green_shift);
}

// Extracts the 8-bit blue component from a platform color.
static inline int tig_color_get_blue(tig_color_t color)
{
    return (int)((color & tig_color_blue_mask) >> tig_color_blue_shift);
}

// Multiply the components of two platform-specific colors.
static inline tig_color_t tig_color_mul(tig_color_t src, tig_color_t dst)
{
    unsigned int r1 = (src & tig_color_red_mask) >> tig_color_red_shift;
    unsigned int g1 = (src & tig_color_green_mask) >> tig_color_green_shift;
    unsigned int b1 = (src & tig_color_blue_mask) >> tig_color_blue_shift;

    unsigned int r2 = (dst & tig_color_red_mask) >> tig_color_red_shift;
    unsigned int g2 = (dst & tig_color_green_mask) >> tig_color_green_shift;
    unsigned int b2 = (dst & tig_color_blue_mask) >> tig_color_blue_shift;

    return tig_color_alpha_mask
        | (tig_color_mult_table[256 * r1 + r2] << tig_color_red_shift)
        | (tig_color_mult_table[256 * g1 + g2] << tig_color_green_shift)
        | (tig_color_mult_table[256 * b1 + b2] << tig_color_blue_shift);
}

// Sum the components of two platform-specific colors.
static inline tig_color_t tig_color_add(tig_color_t src, tig_color_t dst)
{
    unsigned int r1 = src & tig_color_red_mask;
    unsigned int g1 = src & tig_color_green_mask;
    unsigned int b1 = src & tig_color_blue_mask;

    unsigned int r2 = dst & tig_color_red_mask;
    unsigned int g2 = dst & tig_color_green_mask;
    unsigned int b2 = dst & tig_color_blue_mask;

    return tig_color_alpha_mask
        | SDL_min(r1 + r2, tig_color_red_mask)
        | SDL_min(g1 + g2, tig_color_green_mask)
        | SDL_min(b1 + b2, tig_color_blue_mask);
}

// Subtract the components of `src` color from `dst` color.
static inline tig_color_t tig_color_sub(tig_color_t src, tig_color_t dst)
{
    unsigned int r1 = src & tig_color_red_mask;
    unsigned int g1 = src & tig_color_green_mask;
    unsigned int b1 = src & tig_color_blue_mask;

    unsigned int r2 = dst & tig_color_red_mask;
    unsigned int g2 = dst & tig_color_green_mask;
    unsigned int b2 = dst & tig_color_blue_mask;

    return tig_color_alpha_mask
        | ((r2 < r1 ? r1 : r2) - r1)
        | ((g2 < g1 ? g1 : g2) - g1)
        | ((b2 < b1 ? b1 : b2) - b1);
}

// Blends two colors using standard alpha blending rules (src over dst).
//
// The `alpha` specifies amount of src in blended color:
// - `0`: src is completely transparent
// - `255`: src is completely opaque
static inline tig_color_t tig_color_blend_alpha(tig_color_t src, tig_color_t dst, int alpha)
{
    unsigned int r1 = (src & tig_color_red_mask);
    unsigned int g1 = (src & tig_color_green_mask);
    unsigned int b1 = (src & tig_color_blue_mask);

    unsigned int r2 = (dst & tig_color_red_mask);
    unsigned int g2 = (dst & tig_color_green_mask);
    unsigned int b2 = (dst & tig_color_blue_mask);

    return tig_color_alpha_mask
        | ((r2 + ((alpha * (r1 - r2)) >> 8)) & tig_color_red_mask)
        | ((g2 + ((alpha * (g1 - g2)) >> 8)) & tig_color_green_mask)
        | ((b2 + ((alpha * (b1 - b2)) >> 8)) & tig_color_blue_mask);
}

static inline int tig_color_alpha(tig_color_t color)
{
    return (color & tig_color_red_mask) >> tig_color_red_shift;
}

#ifdef __cplusplus
}
#endif

#endif /* TIG_COLOR_H_ */
