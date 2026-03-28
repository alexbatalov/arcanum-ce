#include "tig/color.h"

#include "tig/memory.h"

static void tig_color_mult_tables_init(void);
static void tig_color_mult_tables_exit(void);
static unsigned int tig_color_mask_to_shift(unsigned int mask);

// 0x62B2A0
static bool tig_color_initialized;

unsigned int tig_color_red_mask;
unsigned int tig_color_green_mask;
unsigned int tig_color_blue_mask;
unsigned int tig_color_alpha_mask;

unsigned int tig_color_red_shift;
unsigned int tig_color_green_shift;
unsigned int tig_color_blue_shift;
unsigned int tig_color_alpha_shift;

uint8_t* tig_color_mult_table;

// 0x52BFC0
int tig_color_init(TigInitInfo* init_info)
{
    (void)init_info;

    if (tig_color_initialized) {
        return TIG_ERR_ALREADY_INITIALIZED;
    }

    tig_color_initialized = true;

    return TIG_OK;
}

// 0x52C040
void tig_color_exit(void)
{
    if (!tig_color_initialized) {
        return;
    }

    tig_color_mult_tables_exit();

    tig_color_initialized = false;
}

// 0x52C160
int tig_color_set_rgba_settings(unsigned int red_mask, unsigned int green_mask, unsigned int blue_mask, unsigned int alpha_mask)
{
    tig_color_red_mask = red_mask;
    tig_color_green_mask = green_mask;
    tig_color_blue_mask = blue_mask;
    tig_color_alpha_mask = alpha_mask;

    tig_color_red_shift = tig_color_mask_to_shift(red_mask);
    tig_color_green_shift = tig_color_mask_to_shift(green_mask);
    tig_color_blue_shift = tig_color_mask_to_shift(blue_mask);
    tig_color_alpha_shift = tig_color_mask_to_shift(alpha_mask);

    tig_color_mult_tables_init();

    return 0;
}

// 0x52C2B0
void tig_color_get_masks(unsigned int* red_mask, unsigned int* green_mask, unsigned int* blue_mask)
{
    *red_mask = tig_color_red_mask;
    *green_mask = tig_color_green_mask;
    *blue_mask = tig_color_blue_mask;
}

// 0x52C2E0
void tig_color_get_shifts(unsigned int* red_shift, unsigned int* green_shift, unsigned int* blue_shift)
{
    *red_shift = tig_color_red_shift;
    *green_shift = tig_color_green_shift;
    *blue_shift = tig_color_blue_shift;
}

// 0x52C370
tig_color_t tig_color_rgb_to_grayscale(tig_color_t color)
{
    unsigned int r;
    unsigned int g;
    unsigned int b;
    unsigned int gray;

    r = (color & tig_color_red_mask) >> tig_color_red_shift;
    g = (color & tig_color_green_mask) >> tig_color_green_shift;
    b = (color & tig_color_blue_mask) >> tig_color_blue_shift;

    gray = (r * 77u + g * 150u + b * 28u) / 255u;

    return tig_color_make((int)gray, (int)gray, (int)gray);
}

// 0x52C560
unsigned int tig_color_index_of(tig_color_t color)
{
    int red;
    int green;
    int blue;

    red = (int)((color >> 16) & 0xFFu);
    green = (int)((color >> 8) & 0xFFu);
    blue = (int)(color & 0xFFu);

    return tig_color_make(red, green, blue);
}

// 0x52C7C0
void tig_color_mult_tables_init(void)
{
    unsigned int a;
    unsigned int b;

    tig_color_mult_tables_exit();

    tig_color_mult_table = (uint8_t*)MALLOC(256 * 256);
    for (a = 0; a < 256; a++) {
        for (b = 0; b < 256; b++) {
            tig_color_mult_table[256 * a + b] = (uint8_t)(a * b / 255);
        }
    }
}

// 0x52C940
void tig_color_mult_tables_exit(void)
{
    if (tig_color_mult_table != NULL) {
        FREE(tig_color_mult_table);
        tig_color_mult_table = NULL;
    }
}

unsigned int tig_color_mask_to_shift(unsigned int mask)
{
    unsigned int shift = 0;
    if (mask == 0) {
        return 0;
    }
    while ((mask & 1) == 0) {
        mask >>= 1;
        shift++;
    }
    return shift;
}
