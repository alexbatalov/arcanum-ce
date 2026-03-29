#include "tig/palette.h"

#include "tig/color.h"
#include "tig/debug.h"
#include "tig/memory.h"

#define GROW 16

typedef struct TigPaletteListNode {
    void* data;
    struct TigPaletteListNode* next;
} TigPaletteListNode;

static void tig_palette_node_reserve(void);
static void tig_palette_node_clear(void);

// 0x6301F8
static bool tig_palette_initialized;

// 0x630200
static size_t tig_palette_size;

// 0x630204
static TigPaletteListNode* tig_palette_head;

// 0x533D50
int tig_palette_init(TigInitInfo* init_info)
{
    int index;

    (void)init_info;

    if (tig_palette_initialized) {
        return TIG_ERR_ALREADY_INITIALIZED;
    }

    tig_palette_size = sizeof(TigPalette);

    tig_palette_initialized = true;

    // Warm palette cache (preallocated 256 palettes).
    for (index = 0; index < GROW; index++) {
        tig_palette_node_reserve();
    }

    return TIG_OK;
}

// 0x533DD0
void tig_palette_exit(void)
{
    if (tig_palette_initialized) {
        tig_palette_node_clear();
        tig_palette_initialized = false;
    }
}

// 0x533DF0
TigPalette* tig_palette_create(void)
{
    TigPaletteListNode* node;

    node = tig_palette_head;
    if (node == NULL) {
        tig_palette_node_reserve();
        node = tig_palette_head;
    }

    tig_palette_head = node->next;

    // Return "entries" area of the palette.
    return (TigPalette*)((unsigned char*)node->data + sizeof(TigPaletteListNode*));
}

// 0x533E20
void tig_palette_destroy(TigPalette* palette)
{
    TigPaletteListNode* node;

    node = *(TigPaletteListNode**)((unsigned char*)palette - sizeof(TigPaletteListNode*));
    node->next = tig_palette_head;
    tig_palette_head = node;
}

// 0x533E40
void tig_palette_fill(TigPalette* palette, unsigned int color)
{
    int index;

    for (index = 0; index < 256; index++) {
        palette->colors[index] = color;
    }
}

// 0x533E90
void tig_palette_copy(TigPalette* dst, const TigPalette* src)
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

    if (modify_info->dst_palette != modify_info->src_palette) {
        tig_palette_copy(modify_info->dst_palette, modify_info->src_palette);
    }

    if ((modify_info->flags & TIG_PALETTE_MODIFY_GRAYSCALE) != 0) {
        for (index = 0; index < 256; index++) {
            modify_info->dst_palette->colors[index] = tig_color_rgb_to_grayscale(modify_info->dst_palette->colors[index]);
        }
    }

    if ((modify_info->flags & TIG_PALETTE_MODIFY_TINT) != 0) {
        for (index = 0; index < 256; index++) {
            modify_info->dst_palette->colors[index] = tig_color_mul(modify_info->dst_palette->colors[index], modify_info->tint_color);
        }
    }
}

// 0x534290
size_t tig_palette_system_memory_size(void)
{
    return tig_palette_size + sizeof(TigPaletteListNode*);
}

// 0x5342A0
void tig_palette_node_reserve(void)
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
void tig_palette_node_clear(void)
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
