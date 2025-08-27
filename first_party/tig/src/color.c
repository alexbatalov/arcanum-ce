#include "tig/color.h"

#include "tig/memory.h"

typedef enum ColorComponent {
    CC_RED,
    CC_GREEN,
    CC_BLUE,
    CC_COUNT,
} ColorComponent;

typedef enum ColorSettings {
    CS_CURRENT,
    CS_8,
    CS_16,
    CS_24,
    CS_32,
    CS_COUNT,
} ColorSettings;

static void tig_color_set_mask(int color_component, unsigned int mask);
static int sub_52C760(tig_color_t color, int color_component, int color_setting);
static void tig_color_mult_tables_init();
static void tig_color_mult_tables_exit();
static void tig_color_grayscale_table_init();
static void tig_color_grayscale_table_exit();
static void tig_color_rgb_conversion_tables_init();
static void tig_color_rgb_conversion_tables_exit();
static void tig_color_table_4_init();
static void tig_color_table_4_exit();

// This table contains masks which should be applied to `tig_color_t` value in order
// to extract appropriate color component (see `tig_color_shifts_table`).
//
// The first set indexed by `CS_CURRENT` is a special case - it contains masks
// for current video mode. The remaining sets are sensible defaults that should
// not to be changed (unless you know what you're doing).
//
// 0x5C215C
static int tig_color_masks_table[CS_COUNT][CC_COUNT] = {
    { 0, 0, 0 },
    { 0xE0, 0x1C, 0x03 }, // 332
    { 0xF800, 0x7E0, 0x1F }, // 565
    { 0xFF0000, 0xFF00, 0xFF }, // 888
    { 0xFF0000, 0xFF00, 0xFF }, // 888
};

// This table contains number of bits which should be right-shifted after
// applying appropriate mask (see `tig_color_masks_table`) in order to extract
// color component.
//
// The first set indexed by `CS_CURRENT` is a special case - it contains shifts
// for current video mode. The remaining sets derived from the defaults found in
// `tig_color_masks_table`.
//
// 0x5C2198
static int tig_color_shifts_table[CS_COUNT][CC_COUNT] = {
    { 0, 0, 0 },
    { 5, 2, 0 },
    { 11, 5, 0 },
    { 16, 8, 0 },
    { 16, 8, 0 },
};

// This table contains number of intensity levels which can be represented in
// each color component.
//
// The first set indexed by `CS_CURRENT` is a special case - it contains ranges
// for current video mode. The remaining sets derived from the defaults found in
// `tig_color_masks_table`.
//
// 0x5C21D4
static int tig_color_range_table[CS_COUNT][CC_COUNT] = {
    { 0, 0, 0 },
    { 7, 7, 3 },
    { 31, 63, 31 },
    { 255, 255, 255 },
    { 255, 255, 255 },
};

// 0x62B29C
static int tig_color_bpp;

// 0x62B2A0
static bool tig_color_initialized;

// 0x739E88
uint8_t* tig_color_green_mult_table;

// 0x739E8C
unsigned int tig_color_rgba_green_range_bit_length;

// 0x739E90
unsigned int tig_color_blue_mask;

// 0x739E94
unsigned int tig_color_green_range;

// 0x739E98
uint8_t* tig_color_blue_rgb_to_platform_table;

// 0x739E9C
unsigned int tig_color_rgba_alpha_range;

// 0x739EA0
uint8_t* tig_color_red_platform_to_rgb_table;

// 0x739EA4
uint8_t* tig_color_blue_mult_table;

// 0x739EA8
uint8_t* tig_color_rgba_red_table;

// 0x739EAC
unsigned int tig_color_blue_shift;

// 0x739EB0
unsigned int tig_color_red_range_bit_length;

// 0x739EB4
unsigned int tig_color_rgba_green_shift;

// 0x739EB8
unsigned int tig_color_green_mask;

// 0x739EBC
uint8_t* tig_color_blue_platform_to_rgb_table;

// 0x739EC0
uint8_t* tig_color_green_platform_to_rgb_table;

// 0x739EC4
unsigned int tig_color_rgba_red_shift;

// 0x739EC8
unsigned int tig_color_rgba_blue_mask;

// 0x739ECC
uint8_t* tig_color_rgba_blue_table;

// 0x739ED0
unsigned int tig_color_blue_range;

// 0x739ED4
unsigned int tig_color_rgba_red_range_bit_length;

// 0x739ED8
uint8_t* tig_color_red_mult_table;

// 0x739EDC
uint8_t* tig_color_rgba_green_table;

// 0x739EE0
unsigned int tig_color_rgba_alpha_mask;

// 0x739EE4
unsigned int tig_color_rgba_blue_range;

// 0x739EE8
unsigned int tig_color_rgba_alpha_shift;

// 0x739EEC
unsigned int tig_color_red_shift;

// 0x739EF0
unsigned int tig_color_green_shift;

// 0x739EF4
unsigned int tig_color_blue_range_bit_length;

// 0x739EF8
unsigned int tig_color_rgba_green_mask;

// 0x739EFC
unsigned int tig_color_rgba_red_range;

// 0x739F00
unsigned int tig_color_red_mask;

// 0x739F04
unsigned int tig_color_rgba_blue_shift;

// 0x739F08
unsigned int tig_color_rgba_red_mask;

// 0x739F0C
unsigned int tig_color_red_range;

// 0x739F10
unsigned int tig_color_rgba_alpha_range_bit_length;

// 0x739F14
unsigned int tig_color_green_range_bit_length;

// 0x739F18
uint8_t* tig_color_blue_grayscale_table;

// 0x739F1C
unsigned int tig_color_rgba_green_range;

// 0x739F20
uint8_t* tig_color_red_grayscale_table;

// 0x739F24
unsigned int tig_color_rgba_blue_range_bit_length;

// 0x739F28
uint8_t* tig_color_green_rgb_to_platform_table;

// 0x739F2C
uint8_t* tig_color_red_rgb_to_platform_table;

// 0x739F30
uint8_t* tig_color_green_grayscale_table;

// 0x52BFC0
int tig_color_init(TigInitInfo* init_info)
{
    int cc;

    if (tig_color_initialized) {
        return 2;
    }

    tig_color_bpp = init_info->bpp;

    for (cc = 0; cc < CC_COUNT; cc++) {
        tig_color_masks_table[CS_CURRENT][cc] = tig_color_masks_table[tig_color_bpp / 8][cc];
        tig_color_shifts_table[CS_CURRENT][cc] = tig_color_shifts_table[tig_color_bpp / 8][cc];
        tig_color_range_table[CS_CURRENT][cc] = tig_color_range_table[tig_color_bpp / 8][cc];
    }

    tig_color_initialized = true;

    return 0;
}

// 0x52C040
void tig_color_exit()
{
    if (tig_color_initialized) {
        tig_color_table_4_exit();
        tig_color_rgb_conversion_tables_exit();
        tig_color_grayscale_table_exit();
        tig_color_mult_tables_exit();
        tig_color_initialized = false;
    }
}

// 0x52C070
int tig_color_set_rgb_settings(unsigned int red_mask, unsigned int green_mask, unsigned int blue_mask)
{
    unsigned int bits;

    tig_color_set_mask(CC_RED, red_mask);
    tig_color_set_mask(CC_GREEN, green_mask);
    tig_color_set_mask(CC_BLUE, blue_mask);

    tig_color_red_mask = tig_color_masks_table[CS_CURRENT][CC_RED];
    tig_color_green_mask = tig_color_masks_table[CS_CURRENT][CC_GREEN];
    tig_color_blue_mask = tig_color_masks_table[CS_CURRENT][CC_BLUE];

    tig_color_red_shift = tig_color_shifts_table[CS_CURRENT][CC_RED];
    tig_color_green_shift = tig_color_shifts_table[CS_CURRENT][CC_GREEN];
    tig_color_blue_shift = tig_color_shifts_table[CS_CURRENT][CC_BLUE];

    tig_color_red_range = tig_color_range_table[CS_CURRENT][CC_RED];
    tig_color_green_range = tig_color_range_table[CS_CURRENT][CC_GREEN];
    tig_color_blue_range = tig_color_range_table[CS_CURRENT][CC_BLUE];

    tig_color_red_range_bit_length = 0;
    bits = tig_color_red_range;
    while (bits != 0) {
        tig_color_red_range_bit_length++;
        bits >>= 1;
    }

    tig_color_green_range_bit_length = 0;
    bits = tig_color_green_range;
    while (bits != 0) {
        tig_color_green_range_bit_length++;
        bits >>= 1;
    }

    tig_color_blue_range_bit_length = 0;
    bits = tig_color_blue_range;
    while (bits != 0) {
        tig_color_blue_range_bit_length++;
        bits >>= 1;
    }

    tig_color_mult_tables_init();
    tig_color_grayscale_table_init();
    tig_color_rgb_conversion_tables_init();

    return 0;
}

// 0x52C160
int tig_color_set_rgba_settings(unsigned int red_mask, unsigned int green_mask, unsigned int blue_mask, unsigned int alpha_mask)
{
    unsigned int mask;
    int bit;

    // RED

    tig_color_rgba_red_mask = red_mask;
    mask = red_mask;
    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) != 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_red_shift = bit;

    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) == 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_red_range = (1 << bit) - 1;

    tig_color_rgba_red_range_bit_length = 0;
    mask = tig_color_rgba_red_range;
    while (mask != 0) {
        tig_color_rgba_red_range_bit_length++;
        mask >>= 1;
    }

    // GREEN

    tig_color_rgba_green_mask = green_mask;
    mask = green_mask;
    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) != 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_green_shift = bit;

    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) == 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_green_range = (1 << bit) - 1;

    tig_color_rgba_green_range_bit_length = 0;
    mask = tig_color_rgba_green_range;
    while (mask != 0) {
        tig_color_rgba_green_range_bit_length++;
        mask >>= 1;
    }

    // BLUE

    tig_color_rgba_blue_mask = blue_mask;
    mask = blue_mask;
    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) != 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_blue_shift = bit;

    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) == 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_blue_range = (1 << bit) - 1;

    tig_color_rgba_blue_range_bit_length = 0;
    mask = tig_color_rgba_blue_range;
    while (mask != 0) {
        tig_color_rgba_blue_range_bit_length++;
        mask >>= 1;
    }

    // ALPHA

    tig_color_rgba_alpha_mask = alpha_mask;
    mask = alpha_mask;
    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) != 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_alpha_shift = bit;

    for (bit = 0; bit < 32; bit++) {
        if ((mask & 1) == 0) {
            break;
        }
        mask >>= 1;
    }
    tig_color_rgba_alpha_range = (1 << bit) - 1;

    tig_color_rgba_alpha_range_bit_length = 0;
    mask = tig_color_rgba_alpha_range;
    while (mask != 0) {
        tig_color_rgba_alpha_range_bit_length++;
        mask >>= 1;
    }

    tig_color_table_4_init();

    return 0;
}

// 0x52C2B0
void tig_color_get_masks(unsigned int* red_mask, unsigned int* green_mask, unsigned int* blue_mask)
{
    *red_mask = tig_color_masks_table[CS_CURRENT][CC_RED];
    *green_mask = tig_color_masks_table[CS_CURRENT][CC_GREEN];
    *blue_mask = tig_color_masks_table[CS_CURRENT][CC_BLUE];
}

// 0x52C2E0
void tig_color_get_shifts(unsigned int* red_shift, unsigned int* green_shift, unsigned int* blue_shift)
{
    *red_shift = tig_color_shifts_table[CS_CURRENT][CC_RED];
    *green_shift = tig_color_shifts_table[CS_CURRENT][CC_GREEN];
    *blue_shift = tig_color_shifts_table[CS_CURRENT][CC_BLUE];
}

// 0x52C310
int tig_color_get_red(tig_color_t color)
{
    return sub_52C760(color, CC_RED, CS_CURRENT);
}

// 0x52C330
int tig_color_get_green(tig_color_t color)
{
    return sub_52C760(color, CC_GREEN, CS_CURRENT);
}

// 0x52C350
int tig_color_get_blue(tig_color_t color)
{
    return sub_52C760(color, CC_BLUE, CS_CURRENT);
}

// 0x52C370
tig_color_t tig_color_rgb_to_grayscale(tig_color_t color)
{
    unsigned int red;
    unsigned int green;
    unsigned int blue;
    unsigned int gray;

    red = (color & tig_color_red_mask) >> tig_color_red_shift;
    green = (color & tig_color_green_mask) >> tig_color_green_shift;
    blue = (color & tig_color_blue_mask) >> tig_color_blue_shift;

    gray = tig_color_red_grayscale_table[red]
        + tig_color_green_grayscale_table[green]
        + tig_color_blue_grayscale_table[blue];

    return (tig_color_red_rgb_to_platform_table[gray] << tig_color_red_shift)
        | (tig_color_green_rgb_to_platform_table[gray] << tig_color_green_shift)
        | (tig_color_blue_rgb_to_platform_table[gray] << tig_color_blue_shift);
}

// 0x52C410
int sub_52C410(unsigned int a1)
{
    unsigned int red_index;
    unsigned int green_index;
    unsigned int blue_index;

    red_index = 255 * ((a1 & tig_color_masks_table[CS_CURRENT][CC_RED]) >> tig_color_shifts_table[CS_CURRENT][CC_RED]) / tig_color_range_table[CS_CURRENT][CC_RED];
    green_index = 255 * ((a1 & tig_color_masks_table[CS_CURRENT][CC_GREEN]) >> tig_color_shifts_table[CS_CURRENT][CC_GREEN]) / tig_color_range_table[CS_CURRENT][CC_GREEN];
    blue_index = 255 * ((a1 & tig_color_masks_table[CS_CURRENT][CC_BLUE]) >> tig_color_shifts_table[CS_CURRENT][CC_BLUE]) / tig_color_range_table[CS_CURRENT][CC_BLUE];

    return red_index | (green_index << 8) | (blue_index << 16);
}

// 0x52C490
int sub_52C490(unsigned int color)
{
    unsigned int red_index;
    unsigned int green_index;
    unsigned int blue_index;

    red_index = color & 0xFF;
    green_index = (color >> 8) & 0xFF;
    blue_index = (color >> 16) & 0xFF;

    return (tig_color_red_rgb_to_platform_table[red_index] << tig_color_red_shift) | (tig_color_green_rgb_to_platform_table[green_index] << tig_color_green_shift) | (tig_color_blue_rgb_to_platform_table[blue_index] << tig_color_blue_shift);
}

// 0x52C500
int sub_52C500(tig_color_t color)
{
    return sub_52C760(color, CC_RED, CS_24);
}

// 0x52C520
int sub_52C520(tig_color_t color)
{
    return sub_52C760(color, CC_GREEN, CS_24);
}

// 0x52C540
int sub_52C540(tig_color_t color)
{
    return sub_52C760(color, CC_BLUE, CS_24);
}

// 0x52C560
unsigned int tig_color_index_of(tig_color_t color)
{
    int red_index;
    int green_index;
    int blue_index;

    red_index = 255 * ((color & tig_color_masks_table[CS_24][CC_RED]) >> tig_color_shifts_table[CS_24][CC_RED]) / tig_color_range_table[CS_24][CC_RED];
    green_index = 255 * ((color & tig_color_masks_table[CS_24][CC_GREEN]) >> tig_color_shifts_table[CS_24][CC_GREEN]) / tig_color_range_table[CS_24][CC_GREEN];
    blue_index = 255 * ((color & tig_color_masks_table[CS_24][CC_BLUE]) >> tig_color_shifts_table[CS_24][CC_BLUE]) / tig_color_range_table[CS_24][CC_BLUE];

    return (tig_color_red_rgb_to_platform_table[red_index] << tig_color_red_shift) | (tig_color_green_rgb_to_platform_table[green_index] << tig_color_green_shift) | (tig_color_blue_rgb_to_platform_table[blue_index] << tig_color_blue_shift);
}

// 0x52C610
unsigned int tig_color_to_24_bpp(int red, int green, int blue)
{
    unsigned int normalized_red;
    unsigned int normalized_green;
    unsigned int normalized_blue;

    if (red > 255) {
        red = 255;
    } else if (red < 0) {
        red = 0;
    }

    normalized_red = (tig_color_range_table[CS_24][CC_RED] * red / 255) << tig_color_shifts_table[CS_24][CC_RED];

    if (green > 255) {
        green = 255;
    } else if (green < 0) {
        green = 0;
    }

    normalized_green = (tig_color_range_table[CS_24][CC_GREEN] * green / 255) << tig_color_shifts_table[CS_24][CC_GREEN];

    if (blue > 255) {
        blue = 255;
    } else if (blue < 0) {
        blue = 0;
    }

    normalized_blue = (tig_color_range_table[CS_24][CC_BLUE] * blue / 255) << tig_color_shifts_table[CS_24][CC_BLUE];

    return normalized_red + normalized_green + normalized_blue;
}

// 0x52C6E0
void tig_color_set_mask(int color_component, unsigned int mask)
{
    int bit;

    if (tig_color_bpp == 8) {
        tig_color_masks_table[CS_CURRENT][color_component] = tig_color_masks_table[CS_8][color_component];
        tig_color_shifts_table[CS_CURRENT][color_component] = tig_color_shifts_table[CS_8][color_component];
        tig_color_range_table[CS_CURRENT][color_component] = tig_color_range_table[CS_8][color_component];
    } else {
        tig_color_masks_table[CS_CURRENT][color_component] = mask;

        for (bit = 0; bit < 32; bit++) {
            if ((mask & 1) != 0) {
                break;
            }
            mask >>= 1;
        }

        tig_color_shifts_table[CS_CURRENT][color_component] = bit;

        for (bit = 0; bit < 32; bit++) {
            if ((mask & 1) == 0) {
                break;
            }
            mask >>= 1;
        }

        tig_color_range_table[CS_CURRENT][color_component] = (1 << bit) - 1;
    }
}

// 0x52C760
int sub_52C760(tig_color_t color, int color_component, int color_setting)
{
    int value;

    value = 255 * ((color & tig_color_masks_table[color_setting][color_component]) >> tig_color_shifts_table[color_setting][color_component]) / tig_color_range_table[color_setting][color_component];

    if (tig_color_bpp == 8 && value > 0) {
        value += (255 - value) / 8;
    }

    return value;
}

// 0x52C7C0
void tig_color_mult_tables_init()
{
    unsigned int x;
    unsigned int y;
    unsigned int index;

    tig_color_mult_tables_exit();

    tig_color_red_mult_table = (uint8_t*)MALLOC((tig_color_red_range + 1) * (tig_color_red_range + 1));
    for (y = 0; y <= tig_color_red_range; y++) {
        for (x = 0; x <= tig_color_red_range; x++) {
            // NOTE: Original code is slightly different.
            index = x * (tig_color_red_range + 1) + y;
            tig_color_red_mult_table[index] = (uint8_t)(y * x / tig_color_red_range);
        }
    }

    if (tig_color_green_mask >> tig_color_green_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_green_mult_table = tig_color_red_mult_table;
    } else {
        tig_color_green_mult_table = (uint8_t*)MALLOC((tig_color_green_range + 1) * (tig_color_green_range + 1));
        for (y = 0; y <= tig_color_green_range; y++) {
            for (x = 0; x <= tig_color_green_range; x++) {
                // NOTE: Original code is slightly different.
                index = x * (tig_color_green_range + 1) + y;
                tig_color_green_mult_table[index] = (uint8_t)(y * x / tig_color_green_range);
            }
        }
    }

    if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_blue_mult_table = tig_color_red_mult_table;
    } else if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_green_mask >> tig_color_green_shift) {
        tig_color_blue_mult_table = tig_color_green_mult_table;
    } else {
        tig_color_blue_mult_table = (uint8_t*)MALLOC((tig_color_blue_range + 1) * (tig_color_blue_range + 1));
        for (y = 0; y <= tig_color_blue_range; y++) {
            for (x = 0; x <= tig_color_blue_range; x++) {
                // NOTE: Original code is slightly different.
                index = x * (tig_color_blue_range + 1) + y;
                tig_color_blue_mult_table[index] = (uint8_t)(y * x / tig_color_blue_range);
            }
        }
    }
}

// 0x52C940
void tig_color_mult_tables_exit()
{
    if (tig_color_blue_mult_table != NULL) {
        if (tig_color_blue_mult_table != tig_color_green_mult_table && tig_color_blue_mult_table != tig_color_red_mult_table) {
            FREE(tig_color_blue_mult_table);
        }
        tig_color_blue_mult_table = NULL;
    }

    if (tig_color_green_mult_table != NULL) {
        if (tig_color_green_mult_table != tig_color_red_mult_table) {
            FREE(tig_color_green_mult_table);
        }
        tig_color_green_mult_table = NULL;
    }

    if (tig_color_red_mult_table != NULL) {
        FREE(tig_color_red_mult_table);
        tig_color_red_mult_table = NULL;
    }
}

// Creates grayscale lookup tables for each color channel.
//
// NOTE: The math behind conversion is 30% red, 59% green, 11% blue. I haven't
// found if this mix is some kind of standard or not, but it does mentioned very
// often when googling for grayscale conversion.
//
// 0x52C9B0
void tig_color_grayscale_table_init()
{
    unsigned int index;

    // FIXME: Missing call to exit routine which is seen in other initializers.

    tig_color_red_grayscale_table = (uint8_t*)MALLOC(tig_color_red_range + 1);
    for (index = 0; index <= tig_color_red_range; index++) {
        tig_color_red_grayscale_table[index] = (uint8_t)((double)index / (double)tig_color_red_range * 76.5);
    }

    tig_color_green_grayscale_table = (uint8_t*)MALLOC(tig_color_green_range + 1);
    for (index = 0; index <= tig_color_green_range; index++) {
        tig_color_green_grayscale_table[index] = (uint8_t)((double)index / (double)tig_color_green_range * 150.45);
    }

    tig_color_blue_grayscale_table = (uint8_t*)MALLOC(tig_color_blue_range + 1);
    for (index = 0; index <= tig_color_blue_range; index++) {
        tig_color_blue_grayscale_table[index] = (uint8_t)((double)index / (double)tig_color_blue_range * 28.05);
    }
}

// 0x52CAC0
void tig_color_grayscale_table_exit()
{
    if (tig_color_blue_grayscale_table != NULL) {
        FREE(tig_color_blue_grayscale_table);
        tig_color_blue_grayscale_table = NULL;
    }

    if (tig_color_green_grayscale_table != NULL) {
        FREE(tig_color_green_grayscale_table);
        tig_color_green_grayscale_table = NULL;
    }

    if (tig_color_red_grayscale_table != NULL) {
        FREE(tig_color_red_grayscale_table);
        tig_color_red_grayscale_table = NULL;
    }
}

// Creates color lookup tables to convert video mode specific colors to RGB and
// vice versa.
//
// This function creates two sets of tables for each color channel:
//
// - `from`: these are used to reduce 0-255 RGB color channel value to video
// mode specific value (which is based on available bits per channel). These
// tables have 256 elements each.
//
// - `to`: these are used to remap video mode specific color channel value to
// it's closest RGB value. The number of elements in these tables are dependent
// on number of bits available for each color channel.
//
// 0x52CB20
void tig_color_rgb_conversion_tables_init()
{
    unsigned int index;

    tig_color_rgb_conversion_tables_exit();

    tig_color_red_rgb_to_platform_table = (uint8_t*)MALLOC(256);
    for (index = 0; index < 256; index++) {
        tig_color_red_rgb_to_platform_table[index] = (uint8_t)(index >> (8 - tig_color_red_range_bit_length));
    }

    if (tig_color_green_mask >> tig_color_green_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_green_rgb_to_platform_table = tig_color_red_rgb_to_platform_table;
    } else {
        tig_color_green_rgb_to_platform_table = (uint8_t*)MALLOC(256);
        for (index = 0; index < 256; index++) {
            tig_color_green_rgb_to_platform_table[index] = (uint8_t)(index >> (8 - tig_color_green_range_bit_length));
        }
    }

    if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_blue_rgb_to_platform_table = tig_color_red_rgb_to_platform_table;
    } else if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_green_mask >> tig_color_green_shift) {
        tig_color_blue_rgb_to_platform_table = tig_color_green_rgb_to_platform_table;
    } else {
        tig_color_blue_rgb_to_platform_table = (uint8_t*)MALLOC(256);
        for (index = 0; index < 256; index++) {
            tig_color_blue_rgb_to_platform_table[index] = (uint8_t)(index >> (8 - tig_color_blue_range_bit_length));
        }
    }

    tig_color_red_platform_to_rgb_table = (uint8_t*)MALLOC(tig_color_red_range + 1);
    for (index = 0; index <= tig_color_red_range; index++) {
        tig_color_red_platform_to_rgb_table[index] = (uint8_t)((double)index / (double)tig_color_red_range * 255.0);
    }

    if (tig_color_green_mask >> tig_color_green_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_green_platform_to_rgb_table = tig_color_red_platform_to_rgb_table;
    } else {
        tig_color_green_platform_to_rgb_table = (uint8_t*)MALLOC(tig_color_green_range + 1);
        for (index = 0; index <= tig_color_green_range; index++) {
            tig_color_green_platform_to_rgb_table[index] = (uint8_t)((double)index / (double)tig_color_green_range * 255.0);
        }
    }

    if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_blue_platform_to_rgb_table = tig_color_red_platform_to_rgb_table;
    } else if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_green_mask >> tig_color_green_shift) {
        tig_color_blue_platform_to_rgb_table = tig_color_green_platform_to_rgb_table;
    } else {
        tig_color_blue_platform_to_rgb_table = (uint8_t*)MALLOC(tig_color_blue_range + 1);
        for (index = 0; index <= tig_color_blue_range; index++) {
            tig_color_blue_platform_to_rgb_table[index] = (uint8_t)((double)index / (double)tig_color_blue_range * 255.0);
        }
    }
}

// 0x52CDE0
void tig_color_rgb_conversion_tables_exit()
{
    if (tig_color_blue_platform_to_rgb_table != NULL) {
        if (tig_color_blue_platform_to_rgb_table != tig_color_green_platform_to_rgb_table && tig_color_blue_platform_to_rgb_table != tig_color_red_platform_to_rgb_table) {
            FREE(tig_color_blue_platform_to_rgb_table);
        }
        tig_color_blue_platform_to_rgb_table = NULL;
    }

    if (tig_color_green_platform_to_rgb_table != NULL) {
        if (tig_color_green_platform_to_rgb_table != tig_color_red_platform_to_rgb_table) {
            FREE(tig_color_green_platform_to_rgb_table);
        }
        tig_color_green_platform_to_rgb_table = NULL;
    }

    if (tig_color_red_platform_to_rgb_table != NULL) {
        FREE(tig_color_red_platform_to_rgb_table);
        tig_color_red_platform_to_rgb_table = NULL;
    }

    if (tig_color_blue_rgb_to_platform_table != NULL) {
        if (tig_color_blue_rgb_to_platform_table != tig_color_green_rgb_to_platform_table && tig_color_blue_rgb_to_platform_table != tig_color_red_rgb_to_platform_table) {
            FREE(tig_color_blue_rgb_to_platform_table);
        }
        tig_color_blue_rgb_to_platform_table = NULL;
    }

    if (tig_color_green_rgb_to_platform_table != NULL) {
        if (tig_color_green_rgb_to_platform_table != tig_color_red_rgb_to_platform_table) {
            FREE(tig_color_green_rgb_to_platform_table);
        }
        tig_color_green_rgb_to_platform_table = NULL;
    }

    if (tig_color_red_rgb_to_platform_table) {
        FREE(tig_color_red_rgb_to_platform_table);
        tig_color_red_rgb_to_platform_table = NULL;
    }
}

// 0x52CEB0
void tig_color_table_4_init()
{
    unsigned int index;

    tig_color_table_4_exit();

    tig_color_rgba_red_table = (uint8_t*)MALLOC(tig_color_red_range + 1);
    for (index = 0; index <= tig_color_red_range; index++) {
        tig_color_rgba_red_table[index] = (uint8_t)((double)index / (double)tig_color_red_range * (double)tig_color_rgba_red_range);
    }

    if (tig_color_green_mask >> tig_color_green_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_rgba_green_table = tig_color_rgba_red_table;
    } else {
        tig_color_rgba_green_table = (uint8_t*)MALLOC(tig_color_green_range + 1);
        for (index = 0; index <= tig_color_green_range; index++) {
            tig_color_rgba_green_table[index] = (uint8_t)((double)index / (double)tig_color_green_range * (double)tig_color_rgba_green_range);
        }
    }

    if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_red_mask >> tig_color_red_shift) {
        tig_color_rgba_blue_table = tig_color_rgba_red_table;
    } else if (tig_color_blue_mask >> tig_color_blue_shift == tig_color_green_mask >> tig_color_green_shift) {
        tig_color_rgba_blue_table = tig_color_rgba_green_table;
    } else {
        tig_color_rgba_blue_table = (uint8_t*)MALLOC(tig_color_blue_range + 1);
        for (index = 0; index <= tig_color_blue_range; index++) {
            tig_color_rgba_blue_table[index] = (uint8_t)((double)index / (double)tig_color_blue_range * (double)tig_color_rgba_blue_range);
        }
    }
}

// 0x52D070
void tig_color_table_4_exit()
{
    if (tig_color_rgba_blue_table != NULL) {
        if (tig_color_rgba_blue_table != tig_color_rgba_green_table && tig_color_rgba_blue_table != tig_color_rgba_red_table) {
            FREE(tig_color_rgba_blue_table);
        }
        tig_color_rgba_blue_table = NULL;
    }

    if (tig_color_rgba_green_table != NULL) {
        if (tig_color_rgba_green_table != tig_color_rgba_red_table) {
            FREE(tig_color_rgba_green_table);
        }
        tig_color_rgba_green_table = NULL;
    }

    if (tig_color_rgba_red_table != NULL) {
        FREE(tig_color_rgba_red_table);
        tig_color_rgba_red_table = NULL;
    }
}
