#ifndef TIG_PALETTE_H_
#define TIG_PALETTE_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int TigPaletteModifyFlags;

#define TIG_PALETTE_MODIFY_TINT 0x1
#define TIG_PALETTE_MODIFY_GRAYSCALE 0x2

typedef struct TigPaletteModifyInfo {
    /* 0000 */ TigPaletteModifyFlags flags;
    /* 0004 */ unsigned int tint_color;
    /* 0008 */ TigPalette src_palette;
    /* 000C */ TigPalette dst_palette;
} TigPaletteModifyInfo;

// Initializes PALETTE subsystem.
int tig_palette_init(TigInitInfo* init_info);

// Releases PALETTE subsystem.
void tig_palette_exit();

// Creates a new palette object.
//
// The new palette is in uninitialized state, be sure to fill it with
// `tig_palette_fill` or use as a destination in `tig_palette_modify`.
//
// It can also be filled manually, be sure to cast to either `uint16_t` for 16
// bpp video mode, or to `uint32_t` for 24 and 32 bpp video mode.
TigPalette tig_palette_create();

// Releases specified palette object.
void tig_palette_destroy(TigPalette palette);

// Fill palette object with specified color.
void tig_palette_fill(TigPalette palette, unsigned int color);

// Copy `src` palette into `dst`.
//
// The destination palette should be previously allocated with
// `tig_palette_create`.
void tig_palette_copy(TigPalette dst, const TigPalette src);

// Performs batch palette operation.
//
// This function supports two operations - grayscaling all colors in palette
// (with `TIG_PALETTE_MODIFY_GRAYSCALE`) or tinting all colors in palette (with
// `TIG_PALETTE_MODIFY_TINT`). For the latter you need to specify `tint_color`.
//
// Both `src_palette` and `dst_palette` must not be NULL, but might be the same
// palette object, in this case the operation is performed in-place. Otherwise
// `src_palette` is copied into `dst_palette`.
void tig_palette_modify(const TigPaletteModifyInfo* modify_info);

// Returns amount of system memory required for palette (including overhead).
//
// This value is used to calculate system memory usage in art cache.
size_t tig_palette_system_memory_size();

#ifdef __cplusplus
}
#endif

#endif /* TIG_PALETTE_H_ */
