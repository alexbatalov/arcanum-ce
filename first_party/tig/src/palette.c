#include "tig/palette.h"

#include "tig/color.h"
#include "tig/debug.h"
#include "tig/memory.h"

#define GROW 16

typedef struct TigPaletteListNode {
    void* data;
    struct TigPaletteListNode* next;
} TigPaletteListNode;

static void tig_palette_node_reserve();
static void tig_palette_node_clear();

// 0x6301F8
static bool tig_palette_initialized;

// 0x6301FC
static int tig_palette_bpp;

// 0x630200
static size_t tig_palette_size;

// 0x630204
static TigPaletteListNode* tig_palette_head;

// 0x533D50
int tig_palette_init(TigInitInfo* init_info)
{
    int index;

    if (tig_palette_initialized) {
        return TIG_ERR_ALREADY_INITIALIZED;
    }

    switch (init_info->bpp) {
    case 8:
        break;
    case 16:
        tig_palette_size = sizeof(uint16_t) * 256;
        break;
    case 24:
    case 32:
        tig_palette_size = sizeof(uint32_t) * 256;
        break;
    default:
        tig_debug_println("Unknown BPP in tig_palette_init()\n");
        return TIG_ERR_INVALID_PARAM;
    }

    tig_palette_bpp = init_info->bpp;
    tig_palette_initialized = true;

    // Warm palette cache (preallocated 256 palettes).
    for (index = 0; index < GROW; index++) {
        tig_palette_node_reserve();
    }

    return TIG_OK;
}

// 0x533DD0
void tig_palette_exit()
{
    if (tig_palette_initialized) {
        tig_palette_node_clear();
        tig_palette_initialized = false;
    }
}

// 0x533DF0
TigPalette tig_palette_create()
{
    TigPaletteListNode* node;

    node = tig_palette_head;
    if (node == NULL) {
        tig_palette_node_reserve();
        node = tig_palette_head;
    }

    tig_palette_head = node->next;

    // Return "entries" area of the palette.
    return (TigPalette)((unsigned char*)node->data + sizeof(TigPaletteListNode*));
}

// 0x533E20
void tig_palette_destroy(TigPalette palette)
{
    TigPaletteListNode* node;

    node = *(TigPaletteListNode**)((unsigned char*)palette - sizeof(TigPaletteListNode*));
    node->next = tig_palette_head;
    tig_palette_head = node;
}

// 0x533E40
void tig_palette_fill(TigPalette palette, unsigned int color)
{
    int index;

    switch (tig_palette_bpp) {
    case 8:
        break;
    case 16:
        for (index = 0; index < 256; index++) {
            ((uint16_t*)palette)[index] = (uint16_t)color;
        }
        break;
    case 24:
    case 32:
        for (index = 0; index < 256; index++) {
            ((uint32_t*)palette)[index] = color;
        }
        break;
    }
}

// 0x533E90
void tig_palette_copy(TigPalette dst, const TigPalette src)
{
    memcpy(dst, src, tig_palette_size);
}

// 0x533EC0
void tig_palette_modify(const TigPaletteModifyInfo* modify_info)
{
    int index;

    if (modify_info->flags == 0) {
        return;
    }

    switch (tig_palette_bpp) {
    case 16:
        if (modify_info->dst_palette != modify_info->src_palette) {
            // NOTE: Does not reuse `tig_palette_copy`.
            memcpy(modify_info->dst_palette, modify_info->src_palette, sizeof(uint16_t) * 256);
        }

        if ((modify_info->flags & TIG_PALETTE_MODIFY_GRAYSCALE) != 0) {
            for (index = 0; index < 256; index++) {
                ((uint16_t*)modify_info->dst_palette)[index] = (uint16_t)tig_color_rgb_to_grayscale(((uint16_t*)modify_info->dst_palette)[index]);
            }
        }

        if ((modify_info->flags & TIG_PALETTE_MODIFY_TINT) != 0) {
            for (index = 0; index < 256; index++) {
                ((uint16_t*)modify_info->dst_palette)[index] = (uint16_t)tig_color_mul(((uint16_t*)modify_info->dst_palette)[index], modify_info->tint_color);
            }
        }
        break;
    case 24:
        if (modify_info->dst_palette != modify_info->src_palette) {
            // NOTE: Does not reuse `tig_palette_copy`.
            memcpy(modify_info->dst_palette, modify_info->src_palette, sizeof(uint32_t) * 256);
        }

        if ((modify_info->flags & TIG_PALETTE_MODIFY_GRAYSCALE) != 0) {
            for (index = 0; index < 256; index++) {
                ((uint32_t*)modify_info->dst_palette)[index] = tig_color_rgb_to_grayscale(((uint32_t*)modify_info->dst_palette)[index]);
            }
        }

        if ((modify_info->flags & TIG_PALETTE_MODIFY_TINT) != 0) {
            for (index = 0; index < 256; index++) {
                ((uint32_t*)modify_info->dst_palette)[index] = tig_color_mul(((uint32_t*)modify_info->dst_palette)[index], modify_info->tint_color);
            }
        }
        break;
    case 32:
        // NOTE: The code in this branch is binary identical to 24 bpp, but for
        // unknown reason the generated assembly is not collapsed.
        if (modify_info->dst_palette != modify_info->src_palette) {
            // NOTE: Does not reuse `tig_palette_copy`.
            memcpy(modify_info->dst_palette, modify_info->src_palette, sizeof(uint32_t) * 256);
        }

        if ((modify_info->flags & TIG_PALETTE_MODIFY_GRAYSCALE) != 0) {
            for (index = 0; index < 256; index++) {
                ((uint32_t*)modify_info->dst_palette)[index] = tig_color_rgb_to_grayscale(((uint32_t*)modify_info->dst_palette)[index]);
            }
        }

        if ((modify_info->flags & TIG_PALETTE_MODIFY_TINT) != 0) {
            for (index = 0; index < 256; index++) {
                ((uint32_t*)modify_info->dst_palette)[index] = tig_color_mul(((uint32_t*)modify_info->dst_palette)[index], modify_info->tint_color);
            }
        }
        break;
    }
}

// 0x534290
size_t tig_palette_system_memory_size()
{
    return tig_palette_size + sizeof(TigPaletteListNode*);
}

// 0x5342A0
void tig_palette_node_reserve()
{
    int index;
    TigPaletteListNode* node;

    for (index = 0; index < GROW; index++) {
        node = (TigPaletteListNode*)MALLOC(sizeof(*node));
        node->data = MALLOC(tig_palette_size + sizeof(TigPaletteListNode*));
        // Link back from palette to node.
        *(TigPaletteListNode**)node->data = node;
        node->next = tig_palette_head;
        tig_palette_head = node;
    }
}

// 0x5342E0
void tig_palette_node_clear()
{
    TigPaletteListNode* curr;
    TigPaletteListNode* next;

    curr = tig_palette_head;
    while (curr != NULL) {
        next = curr->next;
        FREE(curr->data);
        FREE(curr);
        curr = next;
    }

    tig_palette_head = NULL;
}
