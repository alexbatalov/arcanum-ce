#include "tig/art.h"

#include <limits.h>
#include <stdio.h>

#include "tig/bmp.h"
#include "tig/color.h"
#include "tig/core.h"
#include "tig/debug.h"
#include "tig/file.h"
#include "tig/memory.h"
#include "tig/palette.h"
#include "tig/timer.h"
#include "tig/video.h"

#define ART_ID_TYPE_SHIFT 28
#define ART_ID_PALETTE_SHIFT 4
#define ART_ID_ROTATION_SHIFT 11

#define ART_ID_MAX_NUM 512
#define ART_ID_NUM_SHIFT 19

#define ART_ID_MAX_FRAME 32
#define ART_ID_FRAME_SHIFT 14

#define TILE_ID_MAX_NUM 64
#define TILE_ID_NUM1_SHIFT 22
#define TILE_ID_NUM2_SHIFT 16
#define TILE_ID_FLIPPABLE1_SHIFT 7
#define TILE_ID_FLIPPABLE2_SHIFT 6
#define FACADE_ID_TILE_NUM_SHIFT 11

#define TILE_ID_MAX_TYPE 2
#define TILE_ID_TYPE_SHIFT 8

#define CRITTER_ID_SHIELD_SHIFT 19
#define CRITTER_ID_ARMOR_SHIFT 20
#define CRITTER_ID_BODY_TYPE_SHIFT 24
#define CRITTER_ID_GENDER_SHIFT 27

#define PORTAL_ID_MAX_TYPE 2
#define PORTAL_ID_TYPE_SHIFT 10

#define WALL_ID_MAX_NUM 256
#define WALL_ID_NUM_SHIFT 20

#define WALL_ID_MAX_P_PIECE 64
#define WALL_ID_P_PIECE_SHIFT 14

#define WALL_ID_MAX_VARIATION 4
#define WALL_ID_VARIATION_SHIFT 8

#define SCENERY_ID_MAX_TYPE 32
#define SCENERY_ID_TYPE_SHIFT 6

#define INTERFACE_ID_MAX_NUM 4096
#define INTERFACE_ID_NUM_SHIFT 16

#define INTERFACE_ID_MAX_FRAME 256
#define INTERFACE_ID_FRAME_SHIFT 8

#define ITEM_ID_MAX_ARMOR_COVERAGE 8
#define ITEM_ID_ARMOR_COVERAGE_SHIFT 14

#define ITEM_ID_MAX_DISPOSITION 4
#define ITEM_ID_DISPOSITION_SHIFT 12

#define ITEM_ID_MAX_SUBTYPE 16
#define ITEM_ID_SUBTYPE_SHIFT 6

#define ITEM_ID_MAX_TYPE 16
#define ITEM_ID_TYPE_SHIFT 0

#define CONTAINER_ID_MAX_TYPE 32
#define CONTAINER_ID_TYPE_SHIFT 6

#define LIGHT_ID_MAX_FRAME 128
#define LIGHT_ID_FRAME_SHIFT 12

#define LIGHT_ID_ROTATION_SHIFT 9

#define ROOF_FADE_SHIFT 12
#define ROOF_FILL_SHIFT 13

// NOTE: For unknown reason facade num is split into two parts. The base number
// is stored in bits 17..24. If bit 27 is set, then base number should be added
// 256.
#define FACADE_ID_MAX_NUM 512
#define FACADE_ID_NUM_HIGH_SHIFT 27
#define FACADE_ID_NUM_LOW_SHIFT 17

#define FACADE_ID_FLIPPABLE_SHIFT 26
#define FACADE_ID_TYPE_SHIFT 25

#define FACADE_ID_MAX_FRAME 1024
#define FACADE_ID_FRAME_SHIFT 1

#define UNIQUE_NPC_ID_MAX_NUM 256
#define UNIQUE_NPC_ID_NUM_SHIFT 20

#define EYE_CANDY_ID_MAX_FRAME 128
#define EYE_CANDY_ID_FRAME_SHIFT 12

#define EYE_CANDY_ID_ROTATION_SHIFT 9
#define EYE_CANDY_ID_TYPE_SHIFT 6

#define MONSTER_ID_ARMOR_SHIFT 20
#define MONSTER_ID_SPECIE_SHIFT 23

#define MAX_PALETTES 4
#define MAX_ROTATIONS 8

// Represents memory size used in art cache.
//
// The type is signed. When used in "available" memory variables the value below
// zero denotes we've used too much memory and should evict something.
typedef int art_size_t;

typedef struct TigArtFileFrameData {
    /* 0000 */ int width;
    /* 0004 */ int height;
    /* 0008 */ int data_size;
    /* 000C */ int hot_x;
    /* 0010 */ int hot_y;
    /* 0014 */ int offset_x;
    /* 0018 */ int offset_y;
} TigArtFileFrameData;

// Serializeable.
static_assert(sizeof(TigArtFileFrameData) == 0x1C, "wrong size");

typedef struct TigArtHeader {
    /* 0000 */ unsigned int flags;
    /* 0004 */ int fps;
    /* 0008 */ int bpp;
    /* 000C */ uint32_t* palette_tbl[MAX_PALETTES];
    /* 001C */ int action_frame;
    /* 0020 */ int num_frames;
    /* 0024 */ TigArtFileFrameData* frames_tbl[MAX_ROTATIONS];
    /* 0044 */ int data_size[MAX_ROTATIONS];
    /* 0064 */ uint8_t* pixels_tbl[MAX_ROTATIONS];
} TigArtHeader;

#define TIG_ART_CACHE_ENTRY_MODIFIED 0x02

typedef struct TigArtCacheEntry {
    /* 0000 */ unsigned int flags;
    /* 0004 */ char path[TIG_MAX_PATH];
    /* 0108 */ tig_timestamp_t time;
    /* 010C */ unsigned int art_id;
    /* 0110 */ TigArtHeader hdr;
    /* 0194 */ uint8_t dirty[MAX_PALETTES][MAX_ROTATIONS];
    /* 01B4 */ TigVideoBuffer** video_buffers[MAX_PALETTES][MAX_ROTATIONS];
    /* 0234 */ uint8_t** pixels_tbl[MAX_ROTATIONS];
    /* 0254 */ TigPalette palette_tbl[MAX_PALETTES];
    /* 0264 */ art_size_t system_memory_usage;
    /* 0268 */ art_size_t video_memory_usage;
} TigArtCacheEntry;

static int art_get_video_buffer(int cache_entry_index, tig_art_id_t art_id, TigVideoBuffer** video_buffer_ptr);
static int sub_505940(unsigned int art_blt_flags, unsigned int* vb_blt_flags_ptr);
static int sub_5059F0(int cache_entry_index, TigArtBlitInfo* blit_info);
static int art_blit(int cache_entry_index, TigArtBlitInfo* blit_info);
static int sub_51AA90(tig_art_id_t art_id);
static void tig_art_cache_check_fullness();
static int tig_art_cache_entry_compare_time(const void* a1, const void* a2);
static int tig_art_cache_entry_compare_name(const void* a1, const void* a2);
static int tig_art_build_path(unsigned int art_id, char* path);
static bool tig_art_cache_find(const char* path, int* index);
static bool tig_art_cache_entry_load(tig_art_id_t art_id, const char* path, int index);
static void tig_art_cache_entry_unload(int cache_entry_index);
static void art_invalidate(int cache_entry_index);
static void sub_51B650(int cache_entry_index);
static int sub_51B710(tig_art_id_t art_id, const char* filename, TigArtHeader* hdr, void** palettes, int a5, art_size_t* size_ptr);
static int sub_51BE30(TigArtHeader* hdr);
static void sub_51BE50(TigFile* stream, TigArtHeader* hdr, TigPalette* palette_tbl);
static void sub_51BF20(TigArtHeader* hdr);
static bool art_read_header(TigArtHeader* hdr, TigFile* stream);

// 0x5BE880
static int dword_5BE880[16] = {
    0,
    1,
    8,
    3,
    4,
    5,
    6,
    7,
    8,
    3,
    10,
    11,
    6,
    7,
    14,
    15,
};

// 0x5BE8C0
static int dword_5BE8C0[16] = {
    0,
    1,
    2,
    9,
    4,
    5,
    12,
    13,
    2,
    9,
    10,
    11,
    12,
    13,
    14,
    15,
};

// 0x5BE900
static int dword_5BE900[32] = {
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    1,
    1,
    1,
    0,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
};

// 0x5BE980
static int dword_5BE980[5] = {
    1,
    0,
    0,
    2,
    1,
};

// 0x5BE994
static int dword_5BE994[TIG_ART_MONSTER_SPECIE_COUNT] = {
    0,
    0,
    1,
    2,
    1,
    1,
    1,
    1,
    2,
    0,
    0,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    1,
    1,
    0,
    2,
    2,
    2,
    1,
    1,
    1,
};

// 0x5BEA14
static bool tig_art_mirroring_enabled = true;

// 0x5BEA18
static int dword_5BEA18[4] = {
    6,
    1,
    3,
    2,
};

// 0x5BEA28
static int dword_5BEA28[4] = {
    2,
    4,
    1,
    5,
};

// 0x5BEA38
float tig_art_cache_video_memory_fullness = 0.3f;

// 0x604710
static TigArtFilePathResolver* tig_art_file_path_resolver;

// 0x604714
static int dword_604714;

// 0x604718
static bool dword_604718;

// 0x60471C
static int tig_art_cache_entries_length;

// 0x604720
static TigArtCacheEntry* tig_art_cache_entries;

// 0x604724
static art_size_t tig_art_total_system_memory;

// 0x60472C
static int tig_art_cache_entries_capacity;

// 0x604730
static art_size_t tig_art_available_video_memory;

// 0x604744
static TigArtBlitPaletteAdjustCallback* dword_604744;

// 0x604738
static int tig_art_bytes_per_pixel;

// 0x60473C
static bool tig_art_initialized;

// 0x604740
static art_size_t tig_art_available_system_memory;

// 0x604748
static art_size_t tig_art_total_video_memory;

// 0x60474C
static int tig_art_bits_per_pixel;

// 0x604750
static TigArtIdResetFunc* tig_art_id_reset_func;

// 0x604754
static int dword_604754;

// 0x500590
int tig_art_init(TigInitInfo* init_info)
{
    size_t total_memory;
    size_t available_memory;

    if (tig_art_initialized) {
        return TIG_ERR_ALREADY_INITIALIZED;
    }

    tig_art_cache_entries_capacity = 512;
    tig_art_cache_entries_length = 0;
    tig_art_cache_entries = (TigArtCacheEntry*)MALLOC(sizeof(TigArtCacheEntry) * tig_art_cache_entries_capacity);

    tig_memory_get_system_status(&total_memory, &available_memory);

    // Prevent overlow on x64.
    if (total_memory > UINT_MAX) {
        total_memory = UINT_MAX;
    }

    tig_art_available_system_memory = (art_size_t)(total_memory >> 2);
    tig_art_total_system_memory = (art_size_t)(total_memory >> 2);

    tig_art_available_video_memory = tig_art_available_system_memory;
    tig_art_total_video_memory = tig_art_total_system_memory;
    tig_debug_printf("Art mem vid avail is %d\n", tig_art_available_video_memory);

    tig_art_bits_per_pixel = init_info->bpp;
    tig_art_bytes_per_pixel = tig_art_bits_per_pixel / 8;
    tig_art_file_path_resolver = init_info->art_file_path_resolver;
    tig_art_id_reset_func = init_info->art_id_reset_func;

    tig_art_initialized = true;

    dword_604718 = tig_video_3d_check_initialized() == TIG_OK;

    return TIG_OK;
}

// 0x500690
void tig_art_exit()
{
    if (tig_art_initialized) {
        tig_art_flush();

        if (tig_art_cache_entries != NULL) {
            FREE(tig_art_cache_entries);
            tig_art_cache_entries = NULL;
            tig_art_cache_entries_length = 0;
            tig_art_cache_entries_capacity = 0;
        }

        tig_art_initialized = false;
    }
}

// 0x5006D0
void tig_art_ping()
{
}

// 0x501DD0
int tig_art_misc_id_create(unsigned int num, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (num >= ART_ID_MAX_NUM
        || palette >= MAX_PALETTES) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_MISC << ART_ID_TYPE_SHIFT)
        | ((num & (ART_ID_MAX_NUM - 1)) << ART_ID_NUM_SHIFT)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT);

    return TIG_OK;
}

// 0x5021E0
void tig_art_flush()
{
    int index;

    if (!tig_art_initialized) {
        return;
    }

    for (index = 0; index < tig_art_cache_entries_length; index++) {
        tig_art_cache_entry_unload(index);
    }

    tig_art_cache_entries_length = 0;
}

// 0x502220
int tig_art_exists(tig_art_id_t art_id)
{
    char path[TIG_MAX_PATH];
    if (tig_art_build_path(art_id, path) != TIG_OK) {
        return TIG_ERR_GENERIC;
    }

    if (!tig_file_exists(path, NULL)) {
        return TIG_ERR_GENERIC;
    }

    return TIG_OK;
}

// 0x502290
int tig_art_touch(tig_art_id_t art_id)
{
    int cache_entry_index;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_IO;
    }

    return TIG_OK;
}

// 0x5022B0
void sub_5022B0(TigArtBlitPaletteAdjustCallback* callback)
{
    dword_604744 = callback;
}

// 0x5022C0
TigArtBlitPaletteAdjustCallback* sub_5022C0()
{
    return dword_604744;
}

// 0x5022D0
void sub_5022D0()
{
    int index;
    unsigned int palette;

    for (index = 0; index < tig_art_cache_entries_length; index++) {
        for (palette = 0; palette < MAX_PALETTES; palette++) {
            if (tig_art_cache_entries[index].hdr.palette_tbl[palette] != NULL) {
                sub_505000(tig_art_cache_entries[index].art_id,
                    tig_art_cache_entries[index].hdr.palette_tbl[palette],
                    tig_art_cache_entries[index].palette_tbl[palette]);
            }
        }
        art_invalidate(index);
    }
}

// 0x502360
int tig_art_blit(TigArtBlitInfo* blit_info)
{
    TigArtBlitInfo mut_art_blit_info;
    TigVideoBuffer* video_buffer;
    TigVideoBufferData video_buffer_data;
    TigVideoBufferBlitInfo vb_blit_info;
    int rc;
    int cache_entry_index;
    unsigned int type;

    mut_art_blit_info = *blit_info;

    cache_entry_index = sub_51AA90(mut_art_blit_info.art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_IO;
    }

    type = tig_art_type(mut_art_blit_info.art_id);
    if (type == TIG_ART_TYPE_TILE) {
        if (tig_art_tile_id_flippable_get(mut_art_blit_info.art_id)) {
            unsigned int flags = tig_art_id_flags_get(mut_art_blit_info.art_id);
            if ((flags & 0x1) != 0) {
                if ((mut_art_blit_info.flags & TIG_ART_BLT_FLIP_X) != 0) {
                    mut_art_blit_info.flags &= ~TIG_ART_BLT_FLIP_X;
                } else {
                    mut_art_blit_info.flags |= TIG_ART_BLT_FLIP_X;
                }
            }
            mut_art_blit_info.art_id = tig_art_id_flags_set(mut_art_blit_info.art_id, flags & ~0x1);
        }
    } else {
        if (tig_art_mirroring_enabled
            && (type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)) {
            int rotation = tig_art_id_rotation_get(mut_art_blit_info.art_id);
            if (rotation > 0 && rotation < 4) {
                mut_art_blit_info.art_id = tig_art_id_rotation_set(mut_art_blit_info.art_id, MAX_ROTATIONS - rotation);
                if ((mut_art_blit_info.flags & TIG_ART_BLT_FLIP_X) != 0) {
                    mut_art_blit_info.flags &= ~TIG_ART_BLT_FLIP_X;
                } else {
                    mut_art_blit_info.flags |= TIG_ART_BLT_FLIP_X;
                }
            }
        }
    }

    if (sub_505940(mut_art_blit_info.flags, &(vb_blit_info.flags)) == TIG_OK
        && sub_520FB0(mut_art_blit_info.dst_video_buffer, vb_blit_info.flags) == TIG_OK
        && art_get_video_buffer(cache_entry_index, mut_art_blit_info.art_id, &video_buffer) == TIG_OK) {
        if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_COLOR_CONST) != 0) {
            vb_blit_info.field_10 = mut_art_blit_info.color;
        } else if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_COLOR_LERP) != 0) {
            vb_blit_info.field_10 = mut_art_blit_info.field_14[0];
            vb_blit_info.field_14 = mut_art_blit_info.field_14[1];
            vb_blit_info.field_18 = mut_art_blit_info.field_14[2];
            vb_blit_info.field_1C = mut_art_blit_info.field_14[3];
            vb_blit_info.field_20 = mut_art_blit_info.field_18;
        } else if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_COLOR_ARRAY) != 0) {
            vb_blit_info.field_10 = mut_art_blit_info.field_14[0];
            vb_blit_info.field_14 = mut_art_blit_info.field_14[1];
            vb_blit_info.field_18 = mut_art_blit_info.field_14[1];
            vb_blit_info.field_1C = mut_art_blit_info.field_14[0];
            vb_blit_info.field_20 = 0;
        }

        if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
            vb_blit_info.alpha[0] = mut_art_blit_info.alpha[0];
        } else if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_ALPHA_LERP_X) != 0) {
            vb_blit_info.alpha[0] = mut_art_blit_info.alpha[0];
            vb_blit_info.alpha[1] = mut_art_blit_info.alpha[0];
            vb_blit_info.alpha[2] = mut_art_blit_info.alpha[1];
            vb_blit_info.alpha[3] = mut_art_blit_info.alpha[1];
        } else if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_ALPHA_LERP_Y) != 0) {
            vb_blit_info.alpha[0] = mut_art_blit_info.alpha[0];
            vb_blit_info.alpha[1] = mut_art_blit_info.alpha[0];
            vb_blit_info.alpha[2] = mut_art_blit_info.alpha[3];
            vb_blit_info.alpha[3] = mut_art_blit_info.alpha[3];
        } else if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH) != 0) {
            vb_blit_info.alpha[0] = mut_art_blit_info.alpha[0];
            vb_blit_info.alpha[1] = mut_art_blit_info.alpha[1];
            vb_blit_info.alpha[2] = mut_art_blit_info.alpha[2];
            vb_blit_info.alpha[3] = mut_art_blit_info.alpha[3];
        }

        vb_blit_info.src_rect = mut_art_blit_info.src_rect;
        vb_blit_info.src_video_buffer = video_buffer;
        vb_blit_info.dst_rect = mut_art_blit_info.dst_rect;
        vb_blit_info.dst_video_buffer = mut_art_blit_info.dst_video_buffer;
        return tig_video_buffer_blit(&vb_blit_info);
    }

    if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_ANY) != 0) {
        tig_video_buffer_data(mut_art_blit_info.dst_video_buffer, &video_buffer_data);

        if ((video_buffer_data.flags & TIG_VIDEO_BUFFER_VIDEO_MEMORY) != 0
            && (mut_art_blit_info.flags & TIG_ART_BLT_SCRATCH_VALID) != 0) {
            rc = tig_video_buffer_data(mut_art_blit_info.scratch_video_buffer, &video_buffer_data);
            if (rc != TIG_OK) {
                return rc;
            }

            if ((mut_art_blit_info.flags & TIG_ART_BLT_BLEND_ALPHA_ANY) != 0) {
                if ((video_buffer_data.flags & TIG_VIDEO_BUFFER_SYSTEM_MEMORY) == 0) {
                    tig_debug_printf("Warning: using video memory surface as a scratch buffer.  This will cause slower performance.\n");
                }

                vb_blit_info.flags = 0;
                vb_blit_info.src_rect = mut_art_blit_info.dst_rect;
                vb_blit_info.dst_rect = mut_art_blit_info.dst_rect;
                vb_blit_info.src_video_buffer = mut_art_blit_info.dst_video_buffer;
                vb_blit_info.dst_video_buffer = mut_art_blit_info.scratch_video_buffer;
                rc = tig_video_buffer_blit(&vb_blit_info);
                if (rc != TIG_OK) {
                    return rc;
                }
            } else {
                tig_video_buffer_fill(mut_art_blit_info.scratch_video_buffer,
                    mut_art_blit_info.dst_rect,
                    video_buffer_data.color_key);
            }

            video_buffer = mut_art_blit_info.dst_video_buffer;
            mut_art_blit_info.dst_video_buffer = mut_art_blit_info.scratch_video_buffer;

            rc = art_blit(cache_entry_index, &mut_art_blit_info);
            if (rc != TIG_OK) {
                return rc;
            }

            mut_art_blit_info.dst_video_buffer = video_buffer;

            vb_blit_info.flags = 0;
            vb_blit_info.src_rect = mut_art_blit_info.dst_rect;
            vb_blit_info.src_video_buffer = mut_art_blit_info.scratch_video_buffer;
            vb_blit_info.dst_rect = mut_art_blit_info.dst_rect;
            vb_blit_info.dst_video_buffer = mut_art_blit_info.dst_video_buffer;
            return tig_video_buffer_blit(&vb_blit_info);
        }
    }

    return art_blit(cache_entry_index, &mut_art_blit_info);
}

// 0x502700
int tig_art_type(tig_art_id_t art_id)
{
    return art_id >> ART_ID_TYPE_SHIFT;
}

// 0x502710
unsigned int tig_art_num_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
        return 0;
    case TIG_ART_TYPE_UNIQUE_NPC:
        return (art_id >> UNIQUE_NPC_ID_NUM_SHIFT) & (UNIQUE_NPC_ID_MAX_NUM - 1);
    case TIG_ART_TYPE_ITEM:
        return (art_id >> 17) & 0x7FF;
    case TIG_ART_TYPE_INTERFACE:
        return (art_id >> INTERFACE_ID_NUM_SHIFT) & (INTERFACE_ID_MAX_NUM - 1);
    default:
        return (art_id >> ART_ID_NUM_SHIFT) & (ART_ID_MAX_NUM - 1);
    }
}

// 0x502780
tig_art_id_t tig_art_num_set(tig_art_id_t art_id, unsigned int value)
{
    unsigned int max;
    unsigned int mask;
    unsigned int shift;

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
        return 0;
    case TIG_ART_TYPE_UNIQUE_NPC:
        max = UNIQUE_NPC_ID_MAX_NUM;
        mask = (UNIQUE_NPC_ID_MAX_NUM - 1) << UNIQUE_NPC_ID_NUM_SHIFT;
        shift = UNIQUE_NPC_ID_NUM_SHIFT;
        break;
    case TIG_ART_TYPE_ITEM:
        max = sub_502830(art_id);
        mask = 0xFFE0000;
        shift = 17;
        break;
    case TIG_ART_TYPE_INTERFACE:
        max = INTERFACE_ID_MAX_NUM;
        mask = (INTERFACE_ID_MAX_NUM - 1) << INTERFACE_ID_NUM_SHIFT;
        shift = INTERFACE_ID_NUM_SHIFT;
        break;
    default:
        max = ART_ID_MAX_NUM;
        mask = (ART_ID_MAX_NUM - 1) << ART_ID_NUM_SHIFT;
        shift = ART_ID_NUM_SHIFT;
        break;
    }

    if (value < max) {
        return (art_id & ~mask) | (value << shift);
    }

    tig_debug_println("Range exceeded in art set.");

    return art_id;
}

// 0x502830
unsigned int sub_502830(tig_art_id_t art_id)
{
    unsigned int max;
    int type;

    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    max = 1000;

    type = tig_art_item_id_type_get(art_id);
    if (type == TIG_ART_ITEM_TYPE_WEAPON
        || type == TIG_ART_ITEM_TYPE_AMMO
        || (type == TIG_ART_ITEM_TYPE_ARMOR
            && tig_art_item_id_armor_coverage_get(art_id) == TIG_ART_ARMOR_COVERAGE_TORSO)) {
        max = 20;
    }

    return max;
}

// 0x502880
int tig_art_id_rotation_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
    case TIG_ART_TYPE_ROOF:
    case TIG_ART_TYPE_ITEM:
    case TIG_ART_TYPE_FACADE:
        return 0;
    case TIG_ART_TYPE_LIGHT:
        return (art_id >> LIGHT_ID_ROTATION_SHIFT) & (MAX_ROTATIONS - 1);
    case TIG_ART_TYPE_EYE_CANDY:
        return (art_id >> EYE_CANDY_ID_ROTATION_SHIFT) & (MAX_ROTATIONS - 1);
    default:
        return (art_id >> ART_ID_ROTATION_SHIFT) & (MAX_ROTATIONS - 1);
    }
}

// 0x5028E0
tig_art_id_t tig_art_id_rotation_inc(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
    case TIG_ART_TYPE_ROOF:
    case TIG_ART_TYPE_ITEM:
    case TIG_ART_TYPE_FACADE:
        return art_id;
    }

    return tig_art_id_rotation_set(art_id, tig_art_id_rotation_get(art_id) + 1);
}

// 0x502930
tig_art_id_t tig_art_id_rotation_set(tig_art_id_t art_id, int rotation)
{
    int type;
    unsigned int mask;
    int shift;
    int old_rotation;
    int old_cardinal_direction;
    int cardinal_direction;
    int v1;
    int v2;
    int v3;

    type = tig_art_type(art_id);

    switch (type) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_ITEM:
    case TIG_ART_TYPE_MISC:
    case TIG_ART_TYPE_ROOF:
    case TIG_ART_TYPE_FACADE:
        return art_id;
    }

    if (sub_504790(art_id)) {
        return art_id;
    }

    switch (type) {
    case TIG_ART_TYPE_EYE_CANDY:
        mask = (MAX_ROTATIONS - 1) << EYE_CANDY_ID_ROTATION_SHIFT;
        shift = EYE_CANDY_ID_ROTATION_SHIFT;
        break;
    case TIG_ART_TYPE_LIGHT:
        mask = (MAX_ROTATIONS - 1) << LIGHT_ID_ROTATION_SHIFT;
        shift = LIGHT_ID_ROTATION_SHIFT;
        break;
    default:
        mask = (MAX_ROTATIONS - 1) << ART_ID_ROTATION_SHIFT;
        shift = ART_ID_ROTATION_SHIFT;
        break;
    }

    if (rotation >= MAX_ROTATIONS) {
        rotation = 0;
    } else if (rotation < 0) {
        rotation = MAX_ROTATIONS - 1;
    }

    old_rotation = tig_art_id_rotation_get(art_id);
    art_id = (art_id & ~mask) | (rotation << shift);

    if (type == TIG_ART_TYPE_WALL) {
        v1 = tig_art_wall_id_p_piece_get(art_id);
        old_cardinal_direction = old_rotation / 2;
        cardinal_direction = rotation / 2;

        if (old_cardinal_direction != cardinal_direction) {
            if (dword_5BEA18[old_cardinal_direction] == v1) {
                v1 = dword_5BEA18[cardinal_direction];
            } else if (dword_5BEA28[old_cardinal_direction] == v1) {
                v1 = dword_5BEA28[cardinal_direction];
            } else if (dword_5BEA18[(old_cardinal_direction + 2) % 4] == v1) {
                v1 = dword_5BEA18[(cardinal_direction + 2) % 4];
            } else if (dword_5BEA28[(old_cardinal_direction + 2) % 4] == v1) {
                v1 = dword_5BEA28[(cardinal_direction + 2) % 4];
            }

            art_id = tig_art_wall_id_p_piece_set(art_id, v1);
            if (((old_cardinal_direction == 0 || old_cardinal_direction == 3)
                    && (cardinal_direction == 1 || cardinal_direction == 2))
                || ((old_cardinal_direction == 1 || old_cardinal_direction == 2)
                    && (cardinal_direction == 0 || cardinal_direction == 3))) {
                v2 = tig_art_id_damaged_get(art_id);
                v3 = 0;
                if ((v2 & 0x400) != 0) {
                    v3 |= 0x80;
                }
                if ((v2 & 0x80) != 0) {
                    v3 |= 0x400;
                }
                art_id = tig_art_id_damaged_set(art_id, v3);
            }
        }

        if (cardinal_direction != 1 && cardinal_direction != 3) {
            art_id = tig_art_id_flags_set(art_id, 0);
            art_id = tig_art_id_palette_set(art_id, 0);
        } else {
            art_id = tig_art_id_flags_set(art_id, 1);
            art_id = tig_art_id_palette_set(art_id, 1);
        }
    } else if (type == TIG_ART_TYPE_PORTAL) {
        if (rotation / 2 != 1 && rotation / 2 != 3) {
            art_id = tig_art_id_flags_set(art_id, 0);
            art_id = tig_art_id_palette_set(art_id, 0);
        } else {
            art_id = tig_art_id_flags_set(art_id, 1);
            art_id = tig_art_id_palette_set(art_id, 1);
        }
    }

    return art_id;
}

// 0x502B50
int tig_art_id_frame_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_ITEM:
        return 0;
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
        return (art_id >> INTERFACE_ID_FRAME_SHIFT) & (INTERFACE_ID_MAX_FRAME - 1);
    case TIG_ART_TYPE_FACADE:
        return (art_id >> FACADE_ID_FRAME_SHIFT) & (FACADE_ID_MAX_FRAME - 1);
    case TIG_ART_TYPE_LIGHT:
        return (art_id >> LIGHT_ID_FRAME_SHIFT) & (LIGHT_ID_MAX_FRAME - 1);
    case TIG_ART_TYPE_EYE_CANDY:
        return (art_id >> EYE_CANDY_ID_FRAME_SHIFT) & (EYE_CANDY_ID_MAX_FRAME - 1);
    default:
        return (art_id >> ART_ID_FRAME_SHIFT) & (ART_ID_MAX_FRAME - 1);
    }
}

// 0x502BC0
tig_art_id_t tig_art_id_frame_inc(tig_art_id_t art_id)
{
    int frame;

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_ITEM:
        return art_id;
    }

    frame = tig_art_id_frame_get(art_id);
    return tig_art_id_frame_set(art_id, frame + 1);
}

// 0x502C00
tig_art_id_t tig_art_id_frame_dec(tig_art_id_t art_id)
{
    int frame;

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_ITEM:
        return art_id;
    }

    frame = tig_art_id_frame_get(art_id);
    if (frame > 0) {
        --frame;
    }

    return tig_art_id_frame_set(art_id, frame);
}

// 0x502C40
tig_art_id_t tig_art_id_frame_set(tig_art_id_t art_id, int value)
{
    TigArtAnimData art_anim_data;
    int type;

    type = tig_art_type(art_id);

    if (type == TIG_ART_TYPE_TILE
        || type == TIG_ART_TYPE_WALL
        || type == TIG_ART_TYPE_ITEM) {
        return art_id;
    }

    if (value > 0) {
        if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
            return art_id;
        }

        if (value >= art_anim_data.num_frames) {
            value = 0;
        }
    } else if (value < 0) {
        if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
            return art_id;
        }

        value = art_anim_data.num_frames - 1;
    }

    switch (type) {
    case TIG_ART_TYPE_INTERFACE:
    case TIG_ART_TYPE_MISC:
        return (art_id & ~((INTERFACE_ID_MAX_FRAME - 1) << INTERFACE_ID_FRAME_SHIFT))
            | (value << INTERFACE_ID_FRAME_SHIFT);
    case TIG_ART_TYPE_FACADE:
        return (art_id & ~((FACADE_ID_MAX_FRAME - 1) << FACADE_ID_FRAME_SHIFT))
            | (value << FACADE_ID_FRAME_SHIFT);
    case TIG_ART_TYPE_LIGHT:
        return (art_id & ~((LIGHT_ID_MAX_FRAME - 1) << LIGHT_ID_FRAME_SHIFT))
            | (value << LIGHT_ID_FRAME_SHIFT);
    case TIG_ART_TYPE_EYE_CANDY:
        return (art_id & ~((EYE_CANDY_ID_MAX_FRAME - 1) << EYE_CANDY_ID_FRAME_SHIFT))
            | (value << EYE_CANDY_ID_FRAME_SHIFT);
    default:
        return (art_id & ~((ART_ID_MAX_FRAME - 1) << ART_ID_FRAME_SHIFT))
            | (value << ART_ID_FRAME_SHIFT);
    }
}

// 0x502D30
tig_art_id_t tig_art_id_flags_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_PORTAL:
    case TIG_ART_TYPE_ROOF:
        if (value >= 16) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0xF) | value;
    default:
        return art_id;
    }
}

// 0x502D80
int tig_art_id_palette_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_LIGHT:
    case TIG_ART_TYPE_FACADE:
        return 0;
    default:
        return (art_id >> ART_ID_PALETTE_SHIFT) & (MAX_PALETTES - 1);
    }
}

// 0x502DB0
tig_art_id_t tig_art_id_palette_set(tig_art_id_t art_id, int value)
{
    if (value >= MAX_PALETTES) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_LIGHT:
    case TIG_ART_TYPE_FACADE:
        return art_id;
    default:
        return (art_id & ~((MAX_PALETTES - 1) << ART_ID_PALETTE_SHIFT))
            | (value << ART_ID_PALETTE_SHIFT);
    }
}

// 0x502E00
int sub_502E00(tig_art_id_t art_id)
{
    int cache_entry_index;
    int palette;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_IO;
    }

    palette = tig_art_id_palette_get(art_id);
    if (tig_art_cache_entries[cache_entry_index].hdr.palette_tbl[palette] == 0) {
        return TIG_ERR_GENERIC;
    }

    return TIG_OK;
}

// 0x502E50
int sub_502E50(tig_art_id_t art_id, int x, int y, unsigned int* color_ptr)
{
    TigArtCacheEntry* cache_entry;
    int cache_entry_index;
    int rotation;
    int frame;
    int type;
    int palette;
    uint8_t byte;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_IO;
    }

    cache_entry = &(tig_art_cache_entries[cache_entry_index]);

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);

    if (tig_art_mirroring_enabled) {
        type = tig_art_type(art_id);
        if ((type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)
            && rotation > 0
            && rotation < 4) {
            rotation = MAX_ROTATIONS - rotation;
            x = cache_entry->hdr.frames_tbl[rotation][frame].width - x - 1;
        }
    }

    byte = cache_entry->pixels_tbl[rotation][frame][y * cache_entry->hdr.frames_tbl[rotation][frame].width + x];

    palette = tig_art_id_palette_get(art_id);
    if (cache_entry->hdr.palette_tbl[palette] == NULL) {
        palette = 0;
    }

    if (tig_art_bits_per_pixel != 8) {
        switch (tig_art_bits_per_pixel) {
        case 16:
            *color_ptr = *((uint16_t*)cache_entry->hdr.palette_tbl[palette] + byte);
            break;
        case 24:
            *color_ptr = *((uint32_t*)cache_entry->hdr.palette_tbl[palette] + byte);
            break;
        case 32:
            *color_ptr = *((uint32_t*)cache_entry->hdr.palette_tbl[palette] + byte);
            break;
        }
    }

    return TIG_OK;
}

// 0x502FD0
int sub_502FD0(tig_art_id_t art_id, int x, int y)
{
    int cache_entry_index;
    int rotation;
    int frame;
    int type;
    int index;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_IO;
    }

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);

    if (tig_art_mirroring_enabled) {
        type = tig_art_type(art_id);
        if ((type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)
            && rotation > 0
            && rotation < 4) {
            rotation = MAX_ROTATIONS - rotation;
            x = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width - x - 1;
        }
    }

    index = y * tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width + x;
    if (tig_art_cache_entries[cache_entry_index].pixels_tbl[rotation][frame][index] < 2) {
        return TIG_ERR_GENERIC;
    }

    return TIG_OK;
}

// 0x5030B0
int tig_art_anim_data(tig_art_id_t art_id, TigArtAnimData* data)
{
    int cache_entry_index;
    TigArtCacheEntry* cache_entry;
    int palette;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_GENERIC;
    }

    cache_entry = &(tig_art_cache_entries[cache_entry_index]);

    data->flags = cache_entry->hdr.flags;
    data->fps = cache_entry->hdr.fps;
    data->bpp = tig_art_bits_per_pixel;
    data->num_frames = cache_entry->hdr.num_frames;
    data->action_frame = cache_entry->hdr.action_frame;

    palette = tig_art_id_palette_get(art_id);

    data->palette1 = cache_entry->palette_tbl[palette];
    if (data->palette1 == NULL) {
        data->palette1 = cache_entry->palette_tbl[0];
    }

    data->palette2 = cache_entry->hdr.palette_tbl[palette];
    if (data->palette2 == 0) {
        data->palette2 = cache_entry->hdr.palette_tbl[0];
    }

    switch (tig_art_bits_per_pixel) {
    case 8:
        data->color_key = ((uint8_t*)data->palette1)[0];
        break;
    case 16:
        data->color_key = ((uint16_t*)data->palette1)[0];
        break;
    case 24:
        data->color_key = ((uint32_t*)data->palette1)[0];
        break;
    case 32:
        data->color_key = ((uint32_t*)data->palette1)[0];
        break;
    }

    return TIG_OK;
}

// 0x5031C0
int tig_art_frame_data(tig_art_id_t art_id, TigArtFrameData* data)
{
    int cache_entry_index;
    TigArtCacheEntry* cache_entry;
    int rotation;
    int frame;
    bool mirrored;
    int type;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_GENERIC;
    }

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);
    type = tig_art_type(art_id);

    mirrored = false;
    if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)
        && rotation > 0
        && rotation < 4) {
        mirrored = true;
        rotation = MAX_ROTATIONS - rotation;
    }

    cache_entry = &(tig_art_cache_entries[cache_entry_index]);

    data->width = cache_entry->hdr.frames_tbl[rotation][frame].width;
    data->height = cache_entry->hdr.frames_tbl[rotation][frame].height;
    data->hot_x = cache_entry->hdr.frames_tbl[rotation][frame].hot_x;
    data->hot_y = cache_entry->hdr.frames_tbl[rotation][frame].hot_y;
    data->offset_x = cache_entry->hdr.frames_tbl[rotation][frame].offset_x;
    data->offset_y = cache_entry->hdr.frames_tbl[rotation][frame].offset_y;

    if ((type == TIG_ART_TYPE_WALL || type == TIG_ART_TYPE_PORTAL)
        && (rotation < 2 || rotation > 5)) {
        data->hot_x -= 40;
        data->hot_y += 20;
    }

    if (mirrored) {
        data->hot_x = data->width - data->hot_x - 1;
        data->offset_x = -data->offset_x;
    }

    if ((tig_art_id_flags_get(art_id) & 0x1) != 0) {
        if (type == TIG_ART_TYPE_ROOF) {
            data->hot_x = 0;
            data->offset_x = 0;
        } else {
            data->hot_x = cache_entry->hdr.frames_tbl[rotation][frame].width - data->hot_x - 2;
            data->offset_x = -data->offset_x;
        }
    }

    return TIG_OK;
}

// 0x503340
int sub_503340(tig_art_id_t art_id, uint8_t* dst, int pitch)
{
    int cache_entry_index;
    int rotation;
    int v2;
    uint8_t* src;
    int width;
    int height;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_IO;
    }

    rotation = tig_art_id_rotation_get(art_id);
    v2 = tig_art_id_frame_get(art_id);
    src = tig_art_cache_entries[cache_entry_index].pixels_tbl[rotation][v2];
    width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][v2].width;
    height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][v2].height;

    while (height > 0) {
        memcpy(dst, src, width);
        dst += pitch;
        height--;
    }

    return TIG_OK;
}

// 0x5033E0
int sub_5033E0(tig_art_id_t art_id, int a2, int a3)
{
    int cache_entry_index;
    int rotation;
    int frame;
    int type;
    int delta;
    int v0;
    int v1;
    int v2;
    int v3;
    int v4;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return 0;
    }

    rotation = tig_art_id_rotation_get(art_id);
    frame = tig_art_id_frame_get(art_id);

    // NOTE: Initialize to keep compiler happy.
    delta = 0;

    if (tig_art_mirroring_enabled) {
        delta = 1;

        type = tig_art_type(art_id);
        if ((type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)
            && rotation > 0
            && rotation < 4) {
            delta = -1;
            rotation = MAX_ROTATIONS - rotation;
        }
    }

    v0 = 0;

    if (a2 < 0) {
        a2 = -a2;
    }

    if (a3 < 0) {
        a3 = -a3;
    }

    v1 = 0;
    v2 = 0;
    while ((v1 < a2 || v2 < a3) && v0 < 100) {
        frame++;
        if (frame == tig_art_cache_entries[cache_entry_index].hdr.num_frames) {
            frame = 0;
        }

        v3 = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].offset_x;
        v4 = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].offset_y;

        if (tig_art_mirroring_enabled) {
            v3 *= delta;
            v4 *= delta;
        }

        if (v3 >= 0) {
            v1 += v3;
        } else {
            v1 -= v3;
        }

        if (v4 >= 0) {
            v2 += v4;
        } else {
            v2 -= v4;
        }

        v0++;
    }

    return v0;
}

// 0x503510
int tig_art_size(tig_art_id_t art_id, int* width_ptr, int* height_ptr)
{
    TigArtCacheEntry* art;
    TigArtFileFrameData* frm;
    int cache_entry_index;
    int type;
    int rotation_offset;
    int num_rotations;
    int rotation;
    int frame;
    int width = 0;
    int height = 0;

    cache_entry_index = sub_51AA90(art_id);
    if (cache_entry_index == -1) {
        return TIG_ERR_GENERIC;
    }

    art = &(tig_art_cache_entries[cache_entry_index]);
    type = tig_art_type(art_id);

    if ((art->hdr.flags & TIG_ART_0x01) != 0) {
        rotation_offset = 0;
        num_rotations = 1;
    } else if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)) {
        rotation_offset = 4;
        num_rotations = 5;
    } else {
        rotation_offset = 0;
        num_rotations = MAX_ROTATIONS;
    }

    for (frame = 0; frame < art->hdr.num_frames; ++frame) {
        for (rotation = 0; rotation < num_rotations; ++rotation) {
            frm = &(art->hdr.frames_tbl[(rotation + rotation_offset) % MAX_ROTATIONS][frame]);
            if (width < frm->width) {
                width = frm->width;
            }
            if (height < frm->height) {
                height = frm->height;
            }
        }
    }

    *width_ptr = width;
    *height_ptr = height;

    return TIG_OK;
}

// 0x503600
int tig_art_tile_id_create(unsigned int num1, unsigned int num2, unsigned int a3, unsigned int a4, unsigned int type, unsigned int flippable1, unsigned int flippable2, unsigned int a8, tig_art_id_t* art_id_ptr)
{
    if (num1 >= TILE_ID_MAX_NUM
        || num2 >= TILE_ID_MAX_NUM
        || a3 >= 0x10
        || a4 >= 0x10
        || type >= TILE_ID_MAX_TYPE
        || flippable1 >= 2
        || flippable2 >= 2
        || a8 >= 4) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_TILE << ART_ID_TYPE_SHIFT)
        | ((num1 & (TILE_ID_MAX_NUM - 1)) << TILE_ID_NUM1_SHIFT)
        | ((num2 & (TILE_ID_MAX_NUM - 1)) << TILE_ID_NUM2_SHIFT)
        | ((type & (TILE_ID_MAX_TYPE - 1)) << TILE_ID_TYPE_SHIFT)
        | ((flippable1 & 1) << TILE_ID_FLIPPABLE1_SHIFT)
        | ((flippable2 & 1) << TILE_ID_FLIPPABLE2_SHIFT)
        | ((a8 & 3) << 4);

    *art_id_ptr = sub_503740(*art_id_ptr, a3);
    *art_id_ptr = sub_503800(*art_id_ptr, a4);

    return TIG_OK;
}

// 0x5036B0
int tig_art_tile_id_num1_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
        return (art_id >> TILE_ID_NUM1_SHIFT) & (TILE_ID_MAX_NUM - 1);
    case TIG_ART_TYPE_FACADE:
        return (art_id >> FACADE_ID_TILE_NUM_SHIFT) & (TILE_ID_MAX_NUM - 1);
    default:
        return 0;
    }
}

// 0x5036E0
int tig_art_tile_id_num2_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return 0;
    }

    return (art_id >> TILE_ID_NUM2_SHIFT) & (TILE_ID_MAX_NUM - 1);
}

// 0x503700
int sub_503700(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return 0;
    }

    int v1 = (art_id >> 12) & 0xF;
    if ((tig_art_id_flags_get(art_id) & 1) != 0) {
        v1 = dword_5BE8C0[v1];
    }
    return v1;
}

// 0x503740
tig_art_id_t sub_503740(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return art_id;
    }

    if (value >= 16) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    if (value != dword_5BE880[value]) {
        if (tig_art_tile_id_flippable_get(art_id)) {
            art_id = tig_art_id_flags_set(art_id, tig_art_id_flags_get(art_id) | 1);
            value = dword_5BE880[value];
        }
    }

    art_id = (art_id & ~0xF000) | (value << 12);

    return art_id;
}

// 0x5037B0
int sub_5037B0(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return 0;
    }

    int v1 = (art_id >> 9) & 7;
    int v2 = sub_503700(art_id);
    if (dword_5BE8C0[v2] == dword_5BE880[v2]) {
        if ((tig_art_id_flags_get(art_id) & 1) != 0) {
            v1 += 8;
        }
    }
    return v1;
}

// 0x503800
tig_art_id_t sub_503800(tig_art_id_t art_id, int value)
{
    int v1;

    if (tig_art_type(art_id) != TIG_ART_TYPE_TILE) {
        return art_id;
    }

    if (value >= 16) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    v1 = sub_503700(art_id);
    if (value < 8) {
        if (dword_5BE8C0[v1] == dword_5BE880[v1]) {
            art_id = tig_art_id_flags_set(art_id, tig_art_id_flags_get(art_id) & ~1);
        }
        art_id = (art_id & ~0xE00) | (value << 9);
    } else {
        if (dword_5BE8C0[v1] == dword_5BE880[v1]) {
            if (tig_art_tile_id_flippable_get(art_id)) {
                art_id = tig_art_id_flags_set(art_id, tig_art_id_flags_get(art_id) | 1);
            }
        }
        art_id = (art_id & ~0xE00) | ((value - 8) << 9);
    }

    return art_id;
}

// 0x5038C0
int tig_art_tile_id_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return (art_id >> TILE_ID_TYPE_SHIFT) & (TILE_ID_MAX_TYPE - 1);
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return (art_id >> FACADE_ID_TYPE_SHIFT) & (TILE_ID_MAX_TYPE - 1);
    }

    return 0;
}

// 0x503900
int tig_art_tile_id_flippable_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return (art_id >> FACADE_ID_FLIPPABLE_SHIFT) & 1;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return tig_art_tile_id_flippable1_get(art_id) != 0
            && tig_art_tile_id_flippable2_get(art_id) != 0;
    }

    return 0;
}

// 0x503950
int tig_art_tile_id_flippable1_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return (art_id >> TILE_ID_FLIPPABLE1_SHIFT) & 1;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return tig_art_tile_id_flippable_get(art_id);
    }

    return 0;
}

// 0x503990
int tig_art_tile_id_flippable2_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_TILE) {
        return (art_id >> TILE_ID_FLIPPABLE2_SHIFT) & 1;
    }

    if (tig_art_type(art_id) == TIG_ART_TYPE_FACADE) {
        return tig_art_tile_id_flippable_get(art_id);
    }

    return 0;
}

// 0x5039D0
int tig_art_wall_id_create(unsigned int num, int p_piece, int variation, int rotation, unsigned int palette, int damaged, tig_art_id_t* art_id_ptr)
{
    if (num >= WALL_ID_MAX_NUM
        || p_piece >= 46 // FIXME: Likely error, should be WALL_ID_MAX_P_PIECE(64).
        || variation >= WALL_ID_MAX_VARIATION
        || rotation >= MAX_ROTATIONS
        || palette >= MAX_PALETTES
        || (damaged & ~0x480) != 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_WALL << ART_ID_TYPE_SHIFT)
        | ((num & (WALL_ID_MAX_NUM - 1)) << WALL_ID_NUM_SHIFT)
        | ((p_piece & (WALL_ID_MAX_P_PIECE - 1)) << WALL_ID_P_PIECE_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((variation & (WALL_ID_MAX_VARIATION - 1)) << WALL_ID_VARIATION_SHIFT)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | damaged;

    *art_id_ptr = tig_art_id_rotation_set(*art_id_ptr, rotation);

    return TIG_OK;
}

// 0x503A60
int tig_art_wall_id_p_piece_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return 0;
    }

    return (art_id >> 14) & 0x3F;
}

// 0x503A90
tig_art_id_t tig_art_wall_id_p_piece_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return art_id;
    }

    if (value >= WALL_ID_MAX_P_PIECE) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    return (art_id & ~((WALL_ID_MAX_P_PIECE - 1) << WALL_ID_P_PIECE_SHIFT))
        | (value << WALL_ID_P_PIECE_SHIFT);
}

// 0x503AD0
int tig_art_wall_id_num_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return 0;
    }

    return (art_id >> WALL_ID_NUM_SHIFT) & (WALL_ID_MAX_NUM - 1);
}

// 0x503B00
int tig_art_wall_id_variation_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return 0;
    }

    return (art_id >> WALL_ID_VARIATION_SHIFT) & (WALL_ID_MAX_VARIATION - 1);
}

// 0x503B30
tig_art_id_t tig_art_wall_id_variation_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_WALL) {
        return art_id;
    }

    if (value >= 5) {
        tig_debug_println("Range exceeded in art set.");
        return art_id;
    }

    return (art_id & ~((WALL_ID_MAX_VARIATION - 1) << WALL_ID_VARIATION_SHIFT))
        | (value << WALL_ID_VARIATION_SHIFT);
}

// 0x503B70
int tig_art_id_damaged_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_WALL:
        return art_id & 0x480;
    case TIG_ART_TYPE_PORTAL:
        return art_id & 0x200;
    case TIG_ART_TYPE_ITEM:
        return art_id & 0x800;
    default:
        return 0;
    }
}

// 0x503BB0
tig_art_id_t tig_art_id_damaged_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_WALL:
        return (art_id & ~0x480) | value;
    case TIG_ART_TYPE_PORTAL:
        return (art_id & ~0x200) | value;
    case TIG_ART_TYPE_ITEM:
        return (art_id & ~0x800) | value;
    default:
        return art_id;
    }
}

// 0x503C00
int tig_art_critter_id_create(unsigned int gender, int body_type, int armor, unsigned int shield, unsigned int frame, int rotation, int anim, int weapon, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (gender >= 2
        || body_type >= 8
        || armor >= 16
        || shield >= 2
        || frame >= 0x20
        || rotation >= MAX_ROTATIONS
        || anim >= 32
        || weapon >= 16
        || palette >= MAX_PALETTES) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_CRITTER << ART_ID_TYPE_SHIFT)
        | ((gender & 1) << CRITTER_ID_GENDER_SHIFT)
        | ((body_type & 7) << CRITTER_ID_BODY_TYPE_SHIFT)
        | ((armor & 0xF) << CRITTER_ID_ARMOR_SHIFT)
        | ((shield & 1) << CRITTER_ID_SHIELD_SHIFT)
        | ((frame & 0x1F) << ART_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((anim & 0x1F) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | (weapon & 0xF);

    return TIG_OK;
}

// 0x503CD0
int tig_art_monster_id_create(int specie, int armor, unsigned int shield, unsigned int frame, int rotation, int anim, int weapon, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (specie >= TIG_ART_MONSTER_SPECIE_COUNT
        || armor >= 8
        || shield >= 2
        || frame >= 0x20
        || rotation >= MAX_ROTATIONS
        || anim >= 32
        || weapon >= 16
        || palette >= MAX_PALETTES) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_MONSTER << ART_ID_TYPE_SHIFT)
        | ((specie & (TIG_ART_MONSTER_SPECIE_COUNT - 1)) << MONSTER_ID_SPECIE_SHIFT)
        | ((armor & 7) << MONSTER_ID_ARMOR_SHIFT)
        | ((shield & 1) << CRITTER_ID_SHIELD_SHIFT)
        | ((frame & 0x1F) << ART_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((anim & 0x1F) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | (weapon & 0xF);

    return TIG_OK;
}

// 0x503D80
int tig_art_unique_npc_id_create(int num, unsigned int shield, unsigned int frame, int rotation, int anim, int weapon, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (num >= UNIQUE_NPC_ID_MAX_NUM
        || shield >= 2
        || frame >= ART_ID_MAX_FRAME
        || rotation >= MAX_ROTATIONS
        || anim >= 32
        || weapon >= 16
        || palette >= MAX_PALETTES) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_UNIQUE_NPC << ART_ID_TYPE_SHIFT)
        | ((num & (UNIQUE_NPC_ID_MAX_NUM - 1)) << UNIQUE_NPC_ID_NUM_SHIFT)
        | ((shield & 1) << CRITTER_ID_SHIELD_SHIFT)
        | ((frame & (ART_ID_MAX_FRAME - 1)) << ART_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((anim & 0x1F) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | (weapon & 0xF);

    return TIG_OK;
}

// 0x503E20
int tig_art_id_anim_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        return (art_id >> 6) & 0x1F;
    default:
        return 0;
    }
}

// 0x503E50
tig_art_id_t tig_art_id_anim_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 26) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x7C0) | (value << 6);
    default:
        return art_id;
    }
}

// 0x503EA0
int tig_art_critter_id_body_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_CRITTER) {
        return (art_id >> CRITTER_ID_BODY_TYPE_SHIFT) & 7;
    }

    return 0;
}

// 0x503ED0
tig_art_id_t tig_art_critter_id_body_type_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= TIG_ART_CRITTER_BODY_TYPE_COUNT) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x7000000) | (value << CRITTER_ID_BODY_TYPE_SHIFT);
    default:
        return art_id;
    }
}

// 0x503F20
int tig_art_monster_id_specie_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_MONSTER) {
        return (art_id >> MONSTER_ID_SPECIE_SHIFT) & (TIG_ART_MONSTER_SPECIE_COUNT - 1);
    }

    return 0;
}

// 0x503F50
int sub_503F50(int a1)
{
    return dword_5BE900[a1];
}

// 0x503F60
int sub_503F60(tig_art_id_t art_id)
{
    int v1;
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
        v1 = tig_art_num_get(art_id);
        return dword_5BE980[v1];
    case TIG_ART_TYPE_MONSTER:
        v1 = tig_art_monster_id_specie_get(art_id);
        return dword_5BE994[v1];
    default:
        return 1;
    }
}

// 0x503FB0
int tig_art_critter_id_gender_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_CRITTER) {
        return (art_id >> CRITTER_ID_GENDER_SHIFT) & 1;
    } else {
        return 0;
    }
}

// 0x503FE0
tig_art_id_t tig_art_critter_id_gender_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= TIG_ART_CRITTER_GENDER_COUNT) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x8000000) | (value << CRITTER_ID_GENDER_SHIFT);
    default:
        return art_id;
    }
}

// 0x504030
int tig_art_critter_id_armor_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
        return (art_id >> CRITTER_ID_ARMOR_SHIFT) & 0xF;
    case TIG_ART_TYPE_MONSTER:
        return (art_id >> MONSTER_ID_ARMOR_SHIFT) & 7;
    default:
        return 0;
    }
}

// 0x504060
tig_art_id_t tig_art_critter_id_armor_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
        if (value >= TIG_ART_ARMOR_TYPE_COUNT) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0xF00000) | (value << CRITTER_ID_ARMOR_SHIFT);
    case TIG_ART_TYPE_MONSTER:
        if (value >= 8) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x700000) | (value << MONSTER_ID_ARMOR_SHIFT);
    default:
        return art_id;
    }
}

// 0x5040D0
int tig_art_critter_id_weapon_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        return art_id & 0xF;
    default:
        return 0;
    }
}

// 0x504100
tig_art_id_t tig_art_critter_id_weapon_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 15) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0xF) | value;
    default:
        return art_id;
    }
}

// 0x504150
int tig_art_critter_id_shield_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        return (art_id >> CRITTER_ID_SHIELD_SHIFT) & 1;
    default:
        return 0;
    }
}

// 0x504180
tig_art_id_t tig_art_critter_id_shield_set(tig_art_id_t art_id, int value)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_CRITTER:
    case TIG_ART_TYPE_MONSTER:
    case TIG_ART_TYPE_UNIQUE_NPC:
        if (value >= 2) {
            tig_debug_println("Range exceeded in art set.");
            value = 0;
        }
        return (art_id & ~0x80000) | (value << CRITTER_ID_SHIELD_SHIFT);
    default:
        return art_id;
    }
}

// 0x5041D0
int tig_art_portal_id_create(unsigned int num, int type, int a3, unsigned int frame, int rotation, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (num >= ART_ID_MAX_NUM
        || type >= 2
        || frame >= ART_ID_MAX_FRAME
        || rotation >= MAX_ROTATIONS
        || palette >= MAX_PALETTES
        || (a3 & ~0x200u) != 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_PORTAL << ART_ID_TYPE_SHIFT)
        | ((num & (ART_ID_MAX_NUM - 1)) << ART_ID_NUM_SHIFT)
        | ((frame & (ART_ID_MAX_FRAME - 1)) << ART_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((type & (PORTAL_ID_MAX_TYPE - 1)) << PORTAL_ID_TYPE_SHIFT)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | a3;

    return TIG_OK;
}

// 0x504260
int tig_art_portal_id_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_PORTAL) {
        return (art_id >> PORTAL_ID_TYPE_SHIFT) & (PORTAL_ID_MAX_TYPE - 1);
    } else {
        return 0;
    }
}

// 0x504290
int tig_art_scenery_id_create(unsigned int num, int type, unsigned int frame, int rotation, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (num >= ART_ID_MAX_NUM
        || type >= SCENERY_ID_MAX_TYPE
        || frame >= ART_ID_MAX_FRAME
        || rotation >= MAX_ROTATIONS
        || palette >= MAX_PALETTES) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_SCENERY << ART_ID_TYPE_SHIFT)
        | ((num & (ART_ID_MAX_NUM - 1)) << ART_ID_NUM_SHIFT)
        | ((frame & (ART_ID_MAX_FRAME - 1)) << ART_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((type & (SCENERY_ID_MAX_TYPE - 1)) << SCENERY_ID_TYPE_SHIFT)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT);

    return TIG_OK;
}

// 0x504300
int tig_art_scenery_id_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_SCENERY) {
        return 0;
    }

    return (art_id >> SCENERY_ID_TYPE_SHIFT) & (SCENERY_ID_MAX_TYPE - 1);
}

// 0x504330
int tig_art_interface_id_create(unsigned int num, unsigned int frame, unsigned char a3, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    if (num >= INTERFACE_ID_MAX_NUM
        || frame >= INTERFACE_ID_MAX_FRAME
        || palette >= MAX_PALETTES) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_INTERFACE << ART_ID_TYPE_SHIFT)
        | ((num & (INTERFACE_ID_MAX_NUM - 1)) << INTERFACE_ID_NUM_SHIFT)
        | ((frame & (INTERFACE_ID_MAX_FRAME - 1)) << INTERFACE_ID_FRAME_SHIFT)
        | ((a3 & 1) << 7)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT);

    return TIG_OK;
}

// 0x504390
int sub_504390(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_INTERFACE) {
        return (art_id >> 7) & 1;
    } else {
        return 0;
    }
}

// 0x5043C0
int tig_art_item_id_create(int num, int disposition, int damaged, int destroyed, int subtype, int type, int armor_coverage, unsigned int palette, tig_art_id_t* art_id_ptr)
{
    // NOTE: Unsigned compare
    if ((unsigned int)num >= 0x3E8
        || disposition >= ITEM_ID_MAX_DISPOSITION
        || (damaged & ~0x800) != 0
        || (destroyed & ~0x400) != 0
        || subtype >= ITEM_ID_MAX_SUBTYPE
        || type >= ITEM_ID_MAX_TYPE
        || armor_coverage >= ITEM_ID_MAX_ARMOR_COVERAGE
        || palette >= MAX_PALETTES) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_ITEM << ART_ID_TYPE_SHIFT)
        | ((num & 0x7FF) << 17)
        | ((armor_coverage & (ITEM_ID_MAX_ARMOR_COVERAGE - 1)) << ITEM_ID_ARMOR_COVERAGE_SHIFT)
        | ((disposition & (ITEM_ID_MAX_DISPOSITION - 1)) << ITEM_ID_DISPOSITION_SHIFT)
        | ((subtype & (ITEM_ID_MAX_SUBTYPE - 1)) << ITEM_ID_SUBTYPE_SHIFT)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | ((type & (ITEM_ID_MAX_TYPE - 1)) << ITEM_ID_TYPE_SHIFT)
        | damaged
        | destroyed;

    // NOTE: Signed compare.
    if ((int)num >= (int)sub_502830(*art_id_ptr)) {
        return TIG_ERR_INVALID_PARAM;
    }

    return TIG_OK;
}

// 0x504490
int tig_art_item_id_disposition_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    return (art_id >> ITEM_ID_DISPOSITION_SHIFT) & (ITEM_ID_MAX_DISPOSITION - 1);
}

// 0x5044C0
tig_art_id_t tig_art_item_id_disposition_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    if (value >= ITEM_ID_MAX_DISPOSITION) {
        value = 0;
    }

    return (art_id & ~((ITEM_ID_MAX_DISPOSITION - 1) << ITEM_ID_DISPOSITION_SHIFT))
        | (value << ITEM_ID_DISPOSITION_SHIFT);
}

// 0x5044F0
int tig_art_item_id_subtype_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    return (art_id >> ITEM_ID_SUBTYPE_SHIFT) & (ITEM_ID_MAX_SUBTYPE - 1);
}

// 0x504520
tig_art_id_t tig_art_item_id_subtype_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return art_id;
    }

    if (value >= ITEM_ID_MAX_SUBTYPE) {
        return art_id;
    }

    return (art_id & ~((ITEM_ID_MAX_SUBTYPE - 1) << ITEM_ID_SUBTYPE_SHIFT))
        | (value << ITEM_ID_SUBTYPE_SHIFT);
}

// 0x504550
int tig_art_item_id_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    return (art_id >> ITEM_ID_TYPE_SHIFT) & (ITEM_ID_MAX_TYPE - 1);
}

// 0x504570
int tig_art_item_id_armor_coverage_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    return (art_id >> ITEM_ID_ARMOR_COVERAGE_SHIFT) & (ITEM_ID_MAX_ARMOR_COVERAGE - 1);
}

// 0x5045A0
int tig_art_item_id_destroyed_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return 0;
    }

    return art_id & 0x400;
}

// 0x5045C0
tig_art_id_t tig_art_item_id_destroyed_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ITEM) {
        return art_id;
    }

    if (value) {
        art_id |= 0x400;
    } else {
        art_id &= ~0x400;
    }

    return art_id;
}

// 0x5045F0
int tig_art_container_id_create(unsigned int num, int type, unsigned int frame, int rotation, unsigned int a5, tig_art_id_t* art_id_ptr)
{
    if (num >= ART_ID_MAX_NUM
        || type >= CONTAINER_ID_MAX_TYPE
        || frame >= ART_ID_MAX_FRAME
        || rotation >= MAX_ROTATIONS
        || a5 >= 4) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_CONTAINER << ART_ID_TYPE_SHIFT)
        | ((num & (ART_ID_MAX_NUM - 1)) << ART_ID_NUM_SHIFT)
        | ((frame & (ART_ID_MAX_FRAME - 1)) << ART_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << ART_ID_ROTATION_SHIFT)
        | ((type & (CONTAINER_ID_MAX_TYPE - 1)) << CONTAINER_ID_TYPE_SHIFT)
        | ((a5 & 3) << 4);

    return TIG_OK;
}

// 0x504660
int tig_art_container_id_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_CONTAINER) {
        return 0;
    }

    return (art_id >> CONTAINER_ID_TYPE_SHIFT) & (CONTAINER_ID_MAX_TYPE - 1);
}

// 0x504690
int tig_art_light_id_create(unsigned int num, unsigned int frame, unsigned int rotation, int a4, tig_art_id_t* art_id_ptr)
{
    if (num >= ART_ID_MAX_NUM
        || frame >= LIGHT_ID_MAX_FRAME
        || (a4 ? rotation >= 32 : rotation >= 8)) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_LIGHT << ART_ID_TYPE_SHIFT)
        | ((num & (ART_ID_MAX_NUM - 1)) << ART_ID_NUM_SHIFT)
        | ((frame & (LIGHT_ID_MAX_FRAME - 1)) << LIGHT_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << LIGHT_ID_ROTATION_SHIFT)
        | ((rotation & 0x1F) << 4)
        | (a4 & 1);

    return TIG_OK;
}

// 0x504700
int sub_504700(tig_art_id_t art_id)
{
    if (sub_504790(art_id) != 0) {
        return (art_id >> 4) & 0x1F;
    } else {
        return tig_art_id_rotation_get(art_id);
    }
}

// 0x504730
tig_art_id_t sub_504730(tig_art_id_t art_id, int rotation)
{
    if (!sub_504790(art_id)) {
        return tig_art_id_rotation_set(art_id, rotation);
    }

    if (rotation >= 32) {
        rotation = 0;
    } else if (rotation < 0) {
        rotation = 31;
    }

    // TODO: Check.
    return (art_id & ~0xFF0)
        | ((rotation % 8) << 9)
        | ((rotation & 0xF) << 4);
}

// 0x504790
int sub_504790(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_LIGHT) {
        return art_id & 1;
    } else {
        return 0;
    }
}

// 0x5047B0
int tig_art_roof_id_create(unsigned int num, int piece, unsigned int fill, unsigned int fade, tig_art_id_t* art_id_ptr)
{
    int flipped;
    int palette;

    if (num >= ART_ID_MAX_NUM
        || piece >= 13
        || fill > 1
        || fade > 1) {
        return TIG_ERR_INVALID_PARAM;
    }

    if (piece >= 9) {
        piece -= 9;
        flipped = 1;
        palette = 1;
    } else {
        flipped = 0;
        palette = 0;
    }

    *art_id_ptr = (TIG_ART_TYPE_ROOF << ART_ID_TYPE_SHIFT)
        | ((num & (ART_ID_MAX_NUM - 1)) << ART_ID_NUM_SHIFT)
        | ((piece & 0x1F) << 14)
        | ((fill & 1) << ROOF_FILL_SHIFT)
        | ((fade & 1) << ROOF_FADE_SHIFT)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT);
    *art_id_ptr = tig_art_id_flags_set(*art_id_ptr, flipped);

    return TIG_OK;
}

// 0x504840
int tig_art_roof_id_piece_get(tig_art_id_t art_id)
{
    int piece;

    if (tig_art_type(art_id) != TIG_ART_TYPE_ROOF) {
        return -1;
    }

    piece = tig_art_id_frame_get(art_id);
    if ((tig_art_id_flags_get(art_id) & 1) != 0) {
        piece += 9;
    }

    return piece;
}

// 0x504880
tig_art_id_t tig_art_roof_id_piece_set(tig_art_id_t art_id, int frame)
{
    int flipped = 0;
    int palette = 0;

    if (tig_art_type(art_id) != TIG_ART_TYPE_ROOF) {
        return art_id;
    }

    if (frame >= 9) {
        frame = frame - 9;
        flipped = 1;
        palette = 1;
    }

    art_id = tig_art_id_frame_set(art_id, frame);
    art_id = tig_art_id_flags_set(art_id, flipped);
    art_id = tig_art_id_palette_set(art_id, palette);

    return art_id;
}

// 0x5048D0
unsigned int tig_art_roof_id_fill_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ROOF) {
        return 0;
    }

    return (art_id >> ROOF_FILL_SHIFT) & 1;
}

// 0x504900
tig_art_id_t tig_art_roof_id_fill_set(tig_art_id_t art_id, unsigned int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ROOF) {
        return art_id;
    }

    if (value > 1) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    art_id &= ~(1 << ROOF_FILL_SHIFT);
    art_id |= value << ROOF_FILL_SHIFT;

    return art_id;
}

// 0x504940
unsigned int tig_art_roof_id_fade_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ROOF) {
        return 0;
    }

    return (art_id >> ROOF_FADE_SHIFT) & 1;
}

// 0x504970
tig_art_id_t tig_art_roof_id_fade_set(tig_art_id_t art_id, unsigned int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_ROOF) {
        return art_id;
    }

    if (value > 1) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    art_id &= ~(1 << ROOF_FADE_SHIFT);
    art_id |= value << ROOF_FADE_SHIFT;

    return art_id;
}

// 0x5049B0
int tig_art_facade_id_create(unsigned int num, unsigned int tile_num, unsigned int type, unsigned int flippable, unsigned int frame, unsigned int walkable, tig_art_id_t* art_id_ptr)
{
    if (num >= FACADE_ID_MAX_NUM
        || tile_num >= TILE_ID_MAX_NUM
        || type >= TILE_ID_MAX_TYPE
        || flippable >= 2
        || frame >= FACADE_ID_MAX_FRAME
        || walkable >= 2) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id_ptr = (TIG_ART_TYPE_FACADE << ART_ID_TYPE_SHIFT)
        | ((num < 256 ? 0 : 1) << FACADE_ID_NUM_HIGH_SHIFT)
        | ((flippable & 1) << FACADE_ID_FLIPPABLE_SHIFT)
        | ((type & (TILE_ID_MAX_TYPE - 1)) << FACADE_ID_TYPE_SHIFT)
        | ((num & 0xFF) << FACADE_ID_NUM_LOW_SHIFT)
        | ((tile_num & (TILE_ID_MAX_NUM - 1)) << FACADE_ID_TILE_NUM_SHIFT)
        | ((frame & (FACADE_ID_MAX_FRAME - 1)) << FACADE_ID_FRAME_SHIFT)
        | (walkable & 1);

    return TIG_OK;
}

// 0x504A60
int tig_art_facade_id_num_get(tig_art_id_t art_id)
{
    int num;

    if (tig_art_type(art_id) != TIG_ART_TYPE_FACADE) {
        return 0;
    }

    num = (art_id >> FACADE_ID_NUM_LOW_SHIFT) & 0xFF;
    if ((art_id & (1 << FACADE_ID_NUM_HIGH_SHIFT)) != 0) {
        num += 256;
    }

    return num;
}

// 0x504A90
int tig_art_facade_id_frame_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_FACADE) {
        return 0;
    }

    return (art_id >> FACADE_ID_FRAME_SHIFT) & (FACADE_ID_MAX_FRAME - 1);
}

// 0x504AC0
int tig_art_facade_id_walkable_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_FACADE) {
        return 0;
    }

    return art_id & 1;
}

// 0x504AE0
int tig_art_eye_candy_id_create(unsigned int num, unsigned int frame, int rotation, int translucency, int type, unsigned int palette, int scale, tig_art_id_t* art_id)
{
    if (num >= ART_ID_MAX_NUM
        || frame >= EYE_CANDY_ID_MAX_FRAME
        || rotation >= MAX_ROTATIONS
        || translucency >= 2
        || type >= 4 // FIXME: Wrong, should be TIG_ART_EYE_CANDY_TYPE_COUNT(3).
        || palette >= MAX_PALETTES
        || scale < 0
        || scale > 7) {
        return TIG_ERR_INVALID_PARAM;
    }

    *art_id = (TIG_ART_TYPE_EYE_CANDY << ART_ID_TYPE_SHIFT)
        | ((num & (ART_ID_MAX_NUM - 1)) << ART_ID_NUM_SHIFT)
        | ((frame & (EYE_CANDY_ID_MAX_FRAME - 1)) << EYE_CANDY_ID_FRAME_SHIFT)
        | ((rotation & (MAX_ROTATIONS - 1)) << EYE_CANDY_ID_ROTATION_SHIFT)
        | ((translucency & 1) << 8)
        | ((type & TIG_ART_EYE_CANDY_TYPE_COUNT) << 6)
        | ((palette & (MAX_PALETTES - 1)) << ART_ID_PALETTE_SHIFT)
        | ((scale & 7) << 1);

    return TIG_OK;
}

// 0x504B90
int tig_art_eye_candy_id_type_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_EYE_CANDY) {
        return 0;
    }

    return (art_id >> EYE_CANDY_ID_TYPE_SHIFT) & TIG_ART_EYE_CANDY_TYPE_COUNT;
}

// 0x504BC0
tig_art_id_t tig_art_eye_candy_id_type_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) != TIG_ART_TYPE_EYE_CANDY) {
        return art_id;
    }

    if (value >= TIG_ART_EYE_CANDY_TYPE_COUNT) {
        tig_debug_println("Range exceeded in art set.");
        value = 0;
    }

    // NOTE: Rare case - value is additionally ANDed with max value.
    return (art_id & ~(TIG_ART_EYE_CANDY_TYPE_COUNT << EYE_CANDY_ID_TYPE_SHIFT))
        | ((value & TIG_ART_EYE_CANDY_TYPE_COUNT) << EYE_CANDY_ID_TYPE_SHIFT);
}

// 0x504C00
int tig_art_eye_candy_id_translucency_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id >> 8) & 1;
    } else {
        return 0;
    }
}

// 0x504C30
tig_art_id_t tig_art_eye_candy_id_translucency_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id & ~0x100) | ((value & 1) << 8);
    } else {
        return art_id;
    }
}

// 0x504C60
int tig_art_eye_candy_id_scale_get(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id >> 1) & 7;
    } else {
        // NOTE: Rare case - default is not 0.
        return 4;
    }
}

// 0x504C90
tig_art_id_t tig_art_eye_candy_id_scale_set(tig_art_id_t art_id, int value)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_EYE_CANDY) {
        return (art_id & ~0xE) | ((value & 7) << 1);
    } else {
        return art_id;
    }
}

// 0x504FD0
int tig_art_id_flags_get(tig_art_id_t art_id)
{
    switch (tig_art_type(art_id)) {
    case TIG_ART_TYPE_TILE:
    case TIG_ART_TYPE_WALL:
    case TIG_ART_TYPE_PORTAL:
    case TIG_ART_TYPE_ROOF:
        return art_id & 0xF;
    default:
        return 0;
    }
}

// 0x505000
void sub_505000(tig_art_id_t art_id, TigPalette src_palette, TigPalette dst_palette)
{
    TigPaletteModifyInfo modify_info;

    if (dword_604744 != NULL && dword_604744(art_id, &modify_info)) {
        modify_info.src_palette = src_palette;
        modify_info.dst_palette = dst_palette;
        tig_palette_modify(&modify_info);
    } else {
        tig_palette_copy(dst_palette, src_palette);
    }
}

// 0x505060
int art_get_video_buffer(int cache_entry_index, tig_art_id_t art_id, TigVideoBuffer** video_buffer_ptr)
{
    TigVideoBufferCreateInfo vb_create_info;
    TigArtBlitInfo art_blit_info;
    TigRect rect;
    int type;
    int palette;
    int rotation;
    int frame;
    int rc;
    art_size_t system_memory_size = 0;
    art_size_t video_memory_size = 0;

    type = tig_art_type(art_id);
    switch (type) {
    case TIG_ART_TYPE_ITEM:
        if (tig_art_item_id_disposition_get(art_id) != 0) {
            return TIG_ERR_GENERIC;
        }
        break;
    case TIG_ART_TYPE_INTERFACE:
        if (sub_504390(art_id) == 0) {
            return TIG_ERR_GENERIC;
        }
        break;
    case TIG_ART_TYPE_MISC:
        return TIG_ERR_GENERIC;
    case TIG_ART_TYPE_LIGHT:
        if (!dword_604718) {
            return TIG_ERR_GENERIC;
        }
        if (sub_504790(art_id) == 0) {
            return TIG_ERR_GENERIC;
        }
        break;
    case TIG_ART_TYPE_TILE:
        art_id = tig_art_id_flags_set(art_id, tig_art_id_flags_get(art_id) & ~1);
        break;
    }

    if (type == TIG_ART_TYPE_ROOF) {
        palette = 0;
        rotation = 0;
        if (tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] == NULL) {
            system_memory_size = sizeof(TigVideoBuffer*) * 13;
            tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] = (TigVideoBuffer**)MALLOC(system_memory_size);

            vb_create_info.color_key = tig_color_make(0, 255, 0);
            vb_create_info.background_color = vb_create_info.color_key;
            if (dword_604718) {
                vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_TEXTURE;
                vb_create_info.color_key = 0;
            } else {
                vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_COLOR_KEY | TIG_VIDEO_BUFFER_CREATE_VIDEO_MEMORY;
            }

            for (frame = 0; frame < tig_art_cache_entries[cache_entry_index].hdr.num_frames; frame++) {
                vb_create_info.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[0][frame].width;
                vb_create_info.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[0][frame].height;
                video_memory_size += vb_create_info.width * vb_create_info.height * tig_art_bits_per_pixel;

                rc = tig_video_buffer_create(&vb_create_info, &(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame]));
                if (rc != TIG_OK) {
                    while (--frame >= 0) {
                        tig_video_buffer_destroy(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame]);
                    }
                    FREE(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation]);
                    tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] = NULL;
                    return rc;
                }
            }

            for (; frame < 13; frame++) {
                vb_create_info.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[0][frame - 9].width;
                vb_create_info.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[0][frame - 9].height;
                video_memory_size += vb_create_info.width * vb_create_info.height * tig_art_bits_per_pixel;

                rc = tig_video_buffer_create(&vb_create_info, &(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame]));
                if (rc != TIG_OK) {
                    while (--frame >= 0) {
                        tig_video_buffer_destroy(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame]);
                    }
                    FREE(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation]);
                    tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] = NULL;
                    return rc;
                }
            }

            tig_art_cache_entries[cache_entry_index].dirty[palette][rotation] = 1;
        }

        if (tig_art_cache_entries[cache_entry_index].dirty[palette][rotation] == 1) {
            rect.x = 0;
            rect.y = 0;

            art_blit_info.flags = 0;
            art_blit_info.src_rect = &rect;
            art_blit_info.dst_rect = &rect;

            for (frame = 0; frame < tig_art_cache_entries[cache_entry_index].hdr.num_frames; frame++) {
                rect.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width;
                rect.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].height;

                art_blit_info.art_id = tig_art_roof_id_piece_set(art_id, frame);
                art_blit_info.dst_video_buffer = tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame];

                if (dword_604718) {
                    rc = sub_5059F0(cache_entry_index, &art_blit_info);
                } else {
                    rc = art_blit(cache_entry_index, &art_blit_info);
                }

                if (rc != TIG_OK) {
                    return rc;
                }
            }

            for (; frame < 13; frame++) {
                rect.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame - 9].width;
                rect.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame - 9].height;

                art_blit_info.art_id = tig_art_roof_id_piece_set(art_id, frame);
                art_blit_info.dst_video_buffer = tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame];

                if (dword_604718) {
                    rc = sub_5059F0(cache_entry_index, &art_blit_info);
                } else {
                    rc = art_blit(cache_entry_index, &art_blit_info);
                }

                if (rc != TIG_OK) {
                    return rc;
                }
            }

            tig_art_cache_entries[cache_entry_index].dirty[palette][rotation] = 0;
        }

        frame = tig_art_roof_id_piece_get(art_id);
    } else {
        rotation = tig_art_id_rotation_get(art_id);
        if (tig_art_mirroring_enabled
            && (type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC)
            && rotation > 0 && rotation < 4) {
            tig_debug_printf("Error in art_get_video_buffer() - requested rotation %d for mirrored object\n", rotation);
        }

        if ((type == TIG_ART_TYPE_WALL
                || type == TIG_ART_TYPE_PORTAL)
            && (rotation & 1) != 0) {
            art_id = tig_art_id_rotation_set(art_id, rotation - 1);
        }

        palette = tig_art_id_palette_get(art_id);

        if (tig_art_cache_entries[cache_entry_index].palette_tbl[palette] == NULL) {
            palette = 0;
            art_id = tig_art_id_palette_set(art_id, 0);
        }

        if (tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] == NULL) {
            system_memory_size = sizeof(TigVideoBuffer*) * tig_art_cache_entries[cache_entry_index].hdr.num_frames;
            tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] = (TigVideoBuffer**)MALLOC(system_memory_size);

            vb_create_info.color_key = tig_color_make(0, 255, 0);
            vb_create_info.background_color = vb_create_info.color_key;

            if (dword_604718) {
                vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_TEXTURE;
                vb_create_info.color_key = 0;
            } else {
                vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_COLOR_KEY | TIG_VIDEO_BUFFER_CREATE_VIDEO_MEMORY;
            }

            for (frame = 0; frame < tig_art_cache_entries[cache_entry_index].hdr.num_frames; frame++) {
                vb_create_info.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width;
                vb_create_info.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].height;
                video_memory_size += vb_create_info.width * vb_create_info.height * tig_art_bytes_per_pixel;
                rc = tig_video_buffer_create(&vb_create_info, &(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame]));
                if (rc != TIG_OK) {
                    while (--frame >= 0) {
                        tig_video_buffer_destroy(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame]);
                    }

                    FREE(tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation]);
                    tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation] = NULL;

                    return rc;
                }
            }

            tig_art_cache_entries[cache_entry_index].dirty[palette][rotation] = 1;
        }

        if (tig_art_cache_entries[cache_entry_index].dirty[palette][rotation] == 1) {
            rect.x = 0;
            rect.y = 0;

            art_blit_info.flags = 0;
            art_blit_info.src_rect = &rect;
            art_blit_info.dst_rect = &rect;

            for (frame = 0; frame < tig_art_cache_entries[cache_entry_index].hdr.num_frames; frame++) {
                rect.width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width;
                rect.height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].height;

                art_blit_info.art_id = tig_art_id_frame_set(art_id, frame);
                art_blit_info.dst_video_buffer = tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame];

                if (dword_604718) {
                    rc = sub_5059F0(cache_entry_index, &art_blit_info);
                } else {
                    rc = art_blit(cache_entry_index, &art_blit_info);
                }

                if (rc != TIG_OK) {
                    return rc;
                }
            }

            tig_art_cache_entries[cache_entry_index].dirty[palette][rotation] = 0;
        }

        frame = tig_art_id_frame_get(art_id);
    }

    *video_buffer_ptr = tig_art_cache_entries[cache_entry_index].video_buffers[palette][rotation][frame];

    tig_art_cache_entries[cache_entry_index].system_memory_usage += system_memory_size;
    tig_art_cache_entries[cache_entry_index].video_memory_usage += video_memory_size;

    tig_art_available_system_memory -= system_memory_size;
    tig_art_available_video_memory -= video_memory_size;

    return TIG_OK;
}

// 0x505940
int sub_505940(unsigned int art_blt_flags, unsigned int* vb_blt_flags_ptr)
{
    if ((art_blt_flags & 0x1802C) != 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    *vb_blt_flags_ptr = 0;

    if ((art_blt_flags & TIG_ART_BLT_FLIP_X) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_FLIP_X;
    }

    if ((art_blt_flags & TIG_ART_BLT_FLIP_Y) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_FLIP_Y;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_ADD) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_ADD;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_MUL) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_MUL;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_ALPHA_AVG) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_AVG;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_CONST;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_ALPHA_SRC) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_SRC;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_ALPHA_LERP_X) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_LERP;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_ALPHA_LERP_Y) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_LERP;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_LERP;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_COLOR_CONST) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_COLOR_CONST;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_COLOR_ARRAY) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_COLOR_LERP;
    }

    if ((art_blt_flags & TIG_ART_BLT_BLEND_COLOR_LERP) != 0) {
        *vb_blt_flags_ptr |= TIG_VIDEO_BUFFER_BLIT_BLEND_COLOR_LERP;
    }

    return TIG_OK;
}

// 0x5059F0
int sub_5059F0(int cache_entry_index, TigArtBlitInfo* blit_info)
{
    TigVideoBufferData video_buffer_data;
    TigPalette plt;
    int rc;
    int rotation;
    int frame;
    int palette;
    uint8_t* src_pixels;
    int width;
    int height;
    int delta;
    uint8_t* dst_pixels;
    int dst_skip;
    int x;
    int y;
    unsigned int color;

    rc = tig_video_buffer_lock(blit_info->dst_video_buffer);
    if (rc != TIG_OK) {
        return rc;
    }

    rc = tig_video_buffer_data(blit_info->dst_video_buffer, &video_buffer_data);
    if (rc != TIG_OK) {
        tig_video_buffer_unlock(blit_info->dst_video_buffer);
        return rc;
    }

    rotation = tig_art_id_rotation_get(blit_info->art_id);
    frame = tig_art_id_frame_get(blit_info->art_id);
    palette = tig_art_id_palette_get(blit_info->art_id);

    src_pixels = tig_art_cache_entries[cache_entry_index].pixels_tbl[rotation][frame];
    width = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].width;
    height = tig_art_cache_entries[cache_entry_index].hdr.frames_tbl[rotation][frame].height;

    plt = tig_art_cache_entries[cache_entry_index].hdr.palette_tbl[palette];
    if (plt == NULL) {
        plt = tig_art_cache_entries[cache_entry_index].hdr.palette_tbl[0];
    }

    if ((tig_art_id_flags_get(blit_info->art_id) & 1) != 0) {
        src_pixels = src_pixels + width * height - 1;
        delta = -1;
    } else {
        delta = 1;
    }

    switch (tig_art_bits_per_pixel) {
    case 8:
        break;
    case 16:
        if (delta > 0) {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
            dst_skip = video_buffer_data.pitch / 2 - width;
        } else {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 2 * (height - 1);
            dst_skip = -width - video_buffer_data.pitch / 2;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (*src_pixels != 0) {
                    color = tig_color_16_to_32(*((uint16_t*)plt + *src_pixels));
                } else {
                    color = 0;
                }
                *(uint16_t*)dst_pixels = (uint16_t)color;
                src_pixels += delta;
                dst_pixels += 2;
            }
            dst_pixels += dst_skip;
        }
        break;
    case 24:
        if (delta > 0) {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
            dst_skip = video_buffer_data.pitch / 3 - width;
        } else {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 3 * (height - 1);
            dst_skip = -width - video_buffer_data.pitch / 3;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (*src_pixels != 0) {
                    color = tig_color_16_to_32(((uint32_t*)plt)[*src_pixels]);
                } else {
                    color = 0;
                }
                dst_pixels[0] = (uint8_t)color;
                dst_pixels[1] = (uint8_t)(color >> 8);
                dst_pixels[2] = (uint8_t)(color >> 16);
                src_pixels += delta;
                dst_pixels += 3;
            }
            dst_pixels += dst_skip;
        }
        break;
    case 32:
        if (delta > 0) {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels;
            dst_skip = video_buffer_data.pitch / 4 - width;
        } else {
            dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch / 4 * (height - 1);
            dst_skip = -width - video_buffer_data.pitch / 4;
        }
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++) {
                if (*src_pixels != 0) {
                    color = tig_color_16_to_32(((uint32_t*)plt)[*src_pixels]);
                } else {
                    color = 0;
                }
                *(uint32_t*)dst_pixels = (uint32_t)color;
                src_pixels += delta;
                dst_pixels += 4;
            }
            dst_pixels += dst_skip;
        }
        break;
    }

    tig_video_buffer_unlock(blit_info->dst_video_buffer);

    return TIG_OK;
}

// 0x505EB0
int art_blit(int cache_entry_index, TigArtBlitInfo* blit_info)
{
    TigVideoBufferData video_buffer_data;
    TigArtCacheEntry* art;
    TigArtFileFrameData* frm;
    TigPalette plt;
    TigRect bounds;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect tmp_rect;
    int rc;
    int rotation;
    int frame;
    int palette;
    uint8_t* src_pixels;
    int src_pitch;
    int src_step;
    int width;
    int height;
    uint8_t* dst_pixels;
    int dst_skip;
    int x;
    int y;
    bool stretched;
    float width_ratio;
    float height_ratio;
    float start_alpha_vertical_step;
    float end_alpha_vertical_step;
    float start_alpha_x;
    float end_alpha_x;
    float alpha_range;
    float start_alpha;
    float end_alpha;
    float current_alpha;
    float alpha_horizontal_step;
    uint32_t* mask;
    unsigned int flip;
    int src_checkerboard_cur_x;
    int src_checkerboard_cur_y;
    int dst_checkerboard_cur_x;
    int dst_checkerboard_cur_y;

    rc = tig_video_buffer_lock(blit_info->dst_video_buffer);
    if (rc != TIG_OK) {
        return rc;
    }

    rc = tig_video_buffer_data(blit_info->dst_video_buffer, &video_buffer_data);
    if (rc != TIG_OK) {
        tig_video_buffer_unlock(blit_info->dst_video_buffer);
        return rc;
    }

    rotation = tig_art_id_rotation_get(blit_info->art_id);
    frame = tig_art_id_frame_get(blit_info->art_id);

    art = &(tig_art_cache_entries[cache_entry_index]);
    frm = &(art->hdr.frames_tbl[rotation][frame]);

    if (blit_info->src_rect->width == blit_info->dst_rect->width
        && blit_info->src_rect->height == blit_info->dst_rect->height) {
        stretched = false;

        // NOTE: Original code does not initialize these values, but we have
        // to keep compiler happy.
        width_ratio = 1;
        height_ratio = 1;
    } else {
        stretched = true;

        if (dword_604754) {
            if (blit_info->dst_rect->width < 1 || blit_info->dst_rect->height < 1) {
                SDL_Window* window;

                if (tig_video_window_get(&window) != TIG_OK) {
                    window = NULL;
                }

                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                    "Halting Execution",
                    "Divide by zero in art_blit",
                    window);

                // FIX: Error code.
                exit(EXIT_FAILURE);
            }
        }

        width_ratio = (float)blit_info->src_rect->width / (float)blit_info->dst_rect->width;
        height_ratio = (float)blit_info->src_rect->height / (float)blit_info->dst_rect->height;
    }

    src_pixels = art->pixels_tbl[rotation][frame];
    width = frm->width;
    height = frm->height;

    bounds.x = 0;
    bounds.y = 0;
    bounds.width = width;
    bounds.height = height;

    if (tig_rect_intersection(blit_info->src_rect, &bounds, &src_rect) != TIG_OK) {
        // Specified source rectangle is out of bounds of the frame, there is
        // nothing to blit.
        tig_video_buffer_unlock(blit_info->dst_video_buffer);
        return TIG_OK;
    }

    tmp_rect = *blit_info->dst_rect;

    if (stretched) {
        tmp_rect.x += (int)((float)(src_rect.x - blit_info->src_rect->x) / width_ratio);
        tmp_rect.y += (int)((float)(src_rect.y - blit_info->src_rect->y) / height_ratio);
        tmp_rect.width -= (int)((float)(blit_info->src_rect->width - src_rect.width) / width_ratio);
        tmp_rect.height -= (int)((float)(blit_info->src_rect->height - src_rect.height) / height_ratio);
    } else {
        tmp_rect.x += src_rect.x - blit_info->src_rect->x;
        tmp_rect.y += src_rect.y - blit_info->src_rect->y;
        tmp_rect.width -= blit_info->src_rect->width - src_rect.width;
        tmp_rect.height -= blit_info->src_rect->height - src_rect.height;
    }

    bounds.x = 0;
    bounds.y = 0;
    bounds.width = video_buffer_data.width;
    bounds.height = video_buffer_data.height;

    if (tig_rect_intersection(&tmp_rect, &bounds, &dst_rect) != TIG_OK) {
        // Specified destination rectangle is out of bounds of destination
        // video buffer bounds, there is nothing to blit.
        tig_video_buffer_unlock(blit_info->dst_video_buffer);
        return TIG_OK;
    }

    if (stretched) {
        src_rect.x += (int)((float)(dst_rect.x - tmp_rect.x) / width_ratio);
        src_rect.y += (int)((float)(dst_rect.y - tmp_rect.y) / height_ratio);
        src_rect.width -= (int)((float)(tmp_rect.width - dst_rect.width) / width_ratio);
        src_rect.height -= (int)((float)(tmp_rect.height - dst_rect.height) / height_ratio);
    } else {
        src_rect.x += dst_rect.x - tmp_rect.x;
        src_rect.y += dst_rect.y - tmp_rect.y;
        src_rect.width -= tmp_rect.width - dst_rect.width;
        src_rect.height -= tmp_rect.height - dst_rect.height;
    }

    if ((blit_info->flags & TIG_ART_BLT_PALETTE_OVERRIDE) != 0) {
        plt = blit_info->palette;
    } else if ((blit_info->flags & TIG_ART_BLT_PALETTE_ORIGINAL) != 0) {
        palette = tig_art_id_palette_get(blit_info->art_id);
        plt = art->hdr.palette_tbl[palette];
        if (plt == NULL) {
            plt = art->hdr.palette_tbl[0];
        }
    } else {
        palette = tig_art_id_palette_get(blit_info->art_id);
        plt = art->palette_tbl[palette];
        if (plt == NULL) {
            plt = art->palette_tbl[0];
        }
    }

    switch (tig_art_bits_per_pixel) {
    case 16:
        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch * dst_rect.y + 2 * dst_rect.x;
        dst_skip = video_buffer_data.pitch - dst_rect.width * 2;
        break;
    case 24:
        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch * dst_rect.y + 3 * dst_rect.x;
        dst_skip = video_buffer_data.pitch - dst_rect.width * 3;
        break;
    case 32:
        dst_pixels = (uint8_t*)video_buffer_data.surface_data.pixels + video_buffer_data.pitch * dst_rect.y + 4 * dst_rect.x;
        dst_skip = video_buffer_data.pitch - dst_rect.width * 4;
        break;
    default:
        // Should be unreachable.
        abort();
    }

    src_checkerboard_cur_x = src_rect.x;
    src_checkerboard_cur_y = src_rect.y;
    dst_checkerboard_cur_x = dst_rect.x;
    dst_checkerboard_cur_y = dst_rect.y;

    flip = blit_info->flags & (TIG_ART_BLT_FLIP_X | TIG_ART_BLT_FLIP_Y);
    if ((tig_art_id_flags_get(blit_info->art_id) & 1) != 0) {
        if ((flip & TIG_ART_BLT_FLIP_X) != 0) {
            flip &= ~TIG_ART_BLT_FLIP_X;
        } else {
            flip |= TIG_ART_BLT_FLIP_X;
        }
    }

    switch (flip) {
    case TIG_ART_BLT_FLIP_X:
        // 0x50642E
        src_pixels += width * src_rect.y + (width - src_rect.x - 1);
        src_pitch = width;
        if (!stretched) {
            src_pitch += src_rect.width;
        }
        src_step = -1;
        break;
    case TIG_ART_BLT_FLIP_Y:
        // 0x50647C
        src_pixels += width * (src_rect.height - src_rect.y - 1) + src_rect.x;
        src_pitch = -width;
        if (!stretched) {
            src_pitch -= src_rect.width;
        }
        src_step = 1;
        break;
    case TIG_ART_BLT_FLIP_X | TIG_ART_BLT_FLIP_Y:
        // 0x5064BC
        src_pixels += width * (src_rect.height - src_rect.y - 1) + (width - src_rect.x - 1);
        src_pitch = -width;
        if (!stretched) {
            src_pitch += src_rect.width;
        }
        src_step = -1;
        break;
    default:
        // 0x5064EE
        src_pixels += width * src_rect.y + src_rect.x;
        src_pitch = width;
        if (!stretched) {
            src_pitch -= src_rect.width;
        }
        src_step = 1;
        break;
    }

    if (stretched) {
        float width_error;
        float height_error;
        uint8_t* prev_src_pixels = src_pixels;

        if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_CONST) != 0) {
            if ((blit_info->flags & TIG_ART_BLT_BLEND_ADD) != 0) {
                // 0x5126AA
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                *(uint32_t*)dst_pixels = tig_color_add(*(uint32_t*)dst_pixels, color);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_SUB) != 0) {
                // 0x512D13
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_sub(tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color),
                                    *(uint32_t*)dst_pixels);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_MUL) != 0) {
                // 0x5133CE
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                *(uint32_t*)dst_pixels = tig_color_mul(color, *(uint32_t*)dst_pixels);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_AVG) != 0) {
                // 0x513B6C
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(color));
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
                // 0x5142F4
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color),
                                    *(uint32_t*)dst_pixels,
                                    blit_info->alpha[0]);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_SRC) != 0) {
                // 0x5154ED
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(blit_info->color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_alpha(((uint32_t*)plt)[*src_pixels]));
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) != 0) {
                // 0x5159AA
                switch (blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) {
                case TIG_ART_BLT_BLEND_ALPHA_LERP_X:
                    start_alpha_vertical_step = 0.0;
                    end_alpha_vertical_step = 0.0;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((float)blit_info->alpha[1] - (float)blit_info->alpha[0]) / width;
                    start_alpha = (float)blit_info->alpha[0] + start_alpha_x * alpha_range;
                    end_alpha = (float)blit_info->alpha[1] - end_alpha_x * alpha_range;
                    break;
                case TIG_ART_BLT_BLEND_ALPHA_LERP_Y:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = start_alpha_vertical_step;
                    start_alpha = src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0];
                    end_alpha = start_alpha;
                    break;
                default:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = ((float)(uint8_t)blit_info->alpha[2] - (float)blit_info->alpha[1]) / height;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0])) / width;
                    start_alpha = (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0]) + start_alpha_x * alpha_range;
                    end_alpha = (src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - end_alpha_x * alpha_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        current_alpha = start_alpha;
                        alpha_horizontal_step = (end_alpha - start_alpha) / src_rect.width;

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color),
                                    *(uint32_t*)dst_pixels,
                                    (int)current_alpha);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                current_alpha += alpha_horizontal_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            start_alpha += start_alpha_vertical_step;
                            end_alpha += end_alpha_vertical_step;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S) != 0) {
                // 0x5149B1
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                }
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                src_checkerboard_cur_x++;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            src_checkerboard_cur_y++;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                        src_checkerboard_cur_x = src_rect.x;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D) != 0) {
                // 0x514F50
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                }
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                dst_checkerboard_cur_x++;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            dst_checkerboard_cur_y++;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                        dst_checkerboard_cur_x = src_rect.x;
                    }
                    break;
                }
            } else {
                // 0x516235
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        } else if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_ARRAY) != 0) {
            if ((blit_info->flags & TIG_ART_BLT_BLEND_ADD) != 0) {
                // 0x516765
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_add(*(uint32_t*)dst_pixels, color);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_SUB) != 0) {
                // 0x516E0A
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_sub(tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask),
                                    *(uint32_t*)dst_pixels);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_MUL) != 0) {
                // 0x5174E0
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_mul(color, *(uint32_t*)dst_pixels);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_AVG) != 0) {
                // 0x517CBE
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(color));
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
                // 0x518494
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    blit_info->alpha[0]);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_SRC) != 0) {
                // 0x51973D
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(*mask,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_alpha(((uint32_t*)plt)[*src_pixels]));
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) != 0) {
                // 0x519C38
                switch (blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) {
                case TIG_ART_BLT_BLEND_ALPHA_LERP_X:
                    start_alpha_vertical_step = 0.0;
                    end_alpha_vertical_step = 0.0;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((float)blit_info->alpha[1] - (float)blit_info->alpha[0]) / width;
                    start_alpha = (float)blit_info->alpha[0] + start_alpha_x * alpha_range;
                    end_alpha = (float)blit_info->alpha[1] - end_alpha_x * alpha_range;
                    break;
                case TIG_ART_BLT_BLEND_ALPHA_LERP_Y:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = start_alpha_vertical_step;
                    start_alpha = src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0];
                    end_alpha = start_alpha;
                    break;
                default:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = ((float)(uint8_t)blit_info->alpha[2] - (float)blit_info->alpha[1]) / height;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0])) / width;
                    start_alpha = (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0]) + start_alpha_x * alpha_range;
                    end_alpha = (src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - end_alpha_x * alpha_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        current_alpha = start_alpha;
                        alpha_horizontal_step = (end_alpha - start_alpha) / src_rect.width;
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    (int)current_alpha);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                current_alpha += alpha_horizontal_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            start_alpha += start_alpha_vertical_step;
                            end_alpha += end_alpha_vertical_step;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S) != 0) {
                // 0x518B9E
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                }
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                src_checkerboard_cur_x++;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            src_checkerboard_cur_y++;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                        src_checkerboard_cur_x = src_rect.x;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D) != 0) {
                // 0x519169
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                }
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                dst_checkerboard_cur_x++;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            dst_checkerboard_cur_y++;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                        dst_checkerboard_cur_x = src_rect.x;
                    }
                    break;
                }
            } else {
                // 0x51A505
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                mask++;
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        } else if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_LERP) != 0) {
            // NOTE: It looks like this blend mode is not implemented for
            // stretched blits.
        } else {
            if ((blit_info->flags & TIG_ART_BLT_BLEND_ADD) != 0) {
                // 0x50FC61
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_add(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_SUB) != 0) {
                // 0x51004E
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_sub(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_MUL) != 0) {
                // 0x5104A6
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_AVG) != 0) {
                // 0x5109EE
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(((uint32_t*)plt)[*src_pixels]));
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
                // 0x510F44
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    blit_info->alpha[0]);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_SRC) != 0) {
                // 0x511960
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    tig_color_alpha(((uint32_t*)plt)[*src_pixels]));
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) != 0) {
                // 0x511E0F
                switch (blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) {
                case TIG_ART_BLT_BLEND_ALPHA_LERP_X:
                    start_alpha_vertical_step = 0.0;
                    end_alpha_vertical_step = 0.0;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((float)blit_info->alpha[1] - (float)blit_info->alpha[0]) / width;
                    start_alpha = (float)blit_info->alpha[0] + start_alpha_x * alpha_range;
                    end_alpha = (float)blit_info->alpha[1] - end_alpha_x * alpha_range;
                    break;
                case TIG_ART_BLT_BLEND_ALPHA_LERP_Y:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = start_alpha_vertical_step;
                    start_alpha = src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0];
                    end_alpha = start_alpha;
                    break;
                default:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = ((float)(uint8_t)blit_info->alpha[2] - (float)blit_info->alpha[1]) / height;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0])) / width;
                    start_alpha = (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0]) + start_alpha_x * alpha_range;
                    end_alpha = (src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - end_alpha_x * alpha_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        current_alpha = start_alpha;
                        alpha_horizontal_step = (end_alpha - start_alpha) / src_rect.width;

                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    (int)current_alpha);
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                current_alpha += alpha_horizontal_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            start_alpha += start_alpha_vertical_step;
                            end_alpha += end_alpha_vertical_step;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S) != 0) {
                // 0x5113B7
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = ((uint32_t*)plt)[*src_pixels];
                                }
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                src_checkerboard_cur_x++;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            src_checkerboard_cur_y++;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                        src_checkerboard_cur_x = src_rect.x;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D) != 0) {
                // 0x51169B
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = ((uint32_t*)plt)[*src_pixels];
                                }
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                dst_checkerboard_cur_x++;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            dst_checkerboard_cur_y++;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                        dst_checkerboard_cur_x = src_rect.x;
                    }
                    break;
                }
            } else {
                // 0x512433
                switch (tig_art_bits_per_pixel) {
                case 32:
                    height_error = 0.5f;
                    for (y = 0; y < dst_rect.height; y++) {
                        width_error = 0.5f;
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = ((uint32_t*)plt)[*src_pixels];
                            }

                            width_error += width_ratio;
                            while (width_error > 1.0f) {
                                src_pixels += src_step;
                                width_error -= 1.0f;
                            }

                            dst_pixels += 4;
                        }

                        src_pixels = prev_src_pixels;
                        height_error += height_ratio;
                        while (height_error > 1.0f) {
                            src_pixels += src_pitch;
                            height_error -= 1.0f;
                        }
                        prev_src_pixels = src_pixels;

                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        }
    } else {
        if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_CONST) != 0) {
            // 0x5084E9
            if ((blit_info->flags & TIG_ART_BLT_BLEND_ADD) != 0) {
                // 0x5084F5
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                *(uint32_t*)dst_pixels = tig_color_add(*(uint32_t*)dst_pixels, color);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_SUB) != 0) {
                // 0x508A1C
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_sub(tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color),
                                    *(uint32_t*)dst_pixels);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_MUL) != 0) {
                // 0x508FB6
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                *(uint32_t*)dst_pixels = tig_color_mul(color, *(uint32_t*)dst_pixels);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_AVG) != 0) {
                // 0x50964F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(color));
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
                // 0x509CD6
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color),
                                    *(uint32_t*)dst_pixels,
                                    blit_info->alpha[0]);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_SRC) != 0) {
                // 0x50ABD1
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(blit_info->color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_alpha(((uint32_t*)plt)[*src_pixels]));
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) != 0) {
                // 0x50AF93
                switch (blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) {
                case TIG_ART_BLT_BLEND_ALPHA_LERP_X:
                    start_alpha_vertical_step = 0.0;
                    end_alpha_vertical_step = 0.0;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((float)blit_info->alpha[1] - (float)blit_info->alpha[0]) / width;
                    start_alpha = (float)blit_info->alpha[0] + start_alpha_x * alpha_range;
                    end_alpha = (float)blit_info->alpha[1] - end_alpha_x * alpha_range;
                    break;
                case TIG_ART_BLT_BLEND_ALPHA_LERP_Y:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = start_alpha_vertical_step;
                    start_alpha = src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0];
                    end_alpha = start_alpha;
                    break;
                default:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = ((float)(uint8_t)blit_info->alpha[2] - (float)blit_info->alpha[1]) / height;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0])) / width;
                    start_alpha = (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0]) + start_alpha_x * alpha_range;
                    end_alpha = (src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - end_alpha_x * alpha_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        current_alpha = start_alpha;
                        alpha_horizontal_step = (end_alpha - start_alpha) / src_rect.width;

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color),
                                    *(uint32_t*)dst_pixels,
                                    (int)current_alpha);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            current_alpha += alpha_horizontal_step;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        start_alpha += start_alpha_vertical_step;
                        end_alpha += end_alpha_vertical_step;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S) != 0) {
                // 0x50A2A6
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                }
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            src_checkerboard_cur_x++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        src_checkerboard_cur_x = src_rect.x;
                        src_checkerboard_cur_y++;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D) != 0) {
                // 0x50A739
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                                }
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            dst_checkerboard_cur_x++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        dst_checkerboard_cur_x = dst_rect.x;
                        dst_checkerboard_cur_y++;
                    }
                    break;
                }
            } else {
                // 0x509647
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], blit_info->color);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        } else if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_ARRAY) != 0) {
            // 0x50BAEF
            if ((blit_info->flags & TIG_ART_BLT_BLEND_ADD) != 0) {
                // 0x50BAFB
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_add(*(uint32_t*)dst_pixels, color);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_SUB) != 0) {
                // 0x50C06F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_sub(tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask),
                                    *(uint32_t*)dst_pixels);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_MUL) != 0) {
                // 0x50C646
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_mul(color, *(uint32_t*)dst_pixels);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_AVG) != 0) {
                // 0x50CD23
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(color));
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
                // 0x50D3F9
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    blit_info->alpha[0]);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_SRC) != 0) {
                // 0x50E3C4
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                // NOTE: This is somewhat different from
                                // surrounding code which uses seemingly inlined
                                // `tig_color_mul`.
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(*mask,
                                    *(uint32_t*)dst_pixels,
                                    tig_color_alpha(((uint32_t*)plt)[*src_pixels]));
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) != 0) {
                // 0x50E7CB
                switch (blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) {
                case TIG_ART_BLT_BLEND_ALPHA_LERP_X:
                    start_alpha_vertical_step = 0.0;
                    end_alpha_vertical_step = 0.0;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((float)blit_info->alpha[1] - (float)blit_info->alpha[0]) / width;
                    start_alpha = (float)blit_info->alpha[0] + start_alpha_x * alpha_range;
                    end_alpha = (float)blit_info->alpha[1] - end_alpha_x * alpha_range;
                    break;
                case TIG_ART_BLT_BLEND_ALPHA_LERP_Y:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = start_alpha_vertical_step;
                    start_alpha = src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0];
                    end_alpha = start_alpha;
                    break;
                default:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = ((float)(uint8_t)blit_info->alpha[2] - (float)blit_info->alpha[1]) / height;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0])) / width;
                    start_alpha = (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0]) + start_alpha_x * alpha_range;
                    end_alpha = (src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - end_alpha_x * alpha_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        current_alpha = start_alpha;
                        alpha_horizontal_step = (end_alpha - start_alpha) / src_rect.width;
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                uint32_t color = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(color,
                                    *(uint32_t*)dst_pixels,
                                    (int)current_alpha);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            current_alpha += alpha_horizontal_step;
                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        start_alpha += start_alpha_vertical_step;
                        end_alpha += end_alpha_vertical_step;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S) != 0) {
                // 0x50DA0F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                }
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            src_checkerboard_cur_x++;
                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        src_checkerboard_cur_x = src_rect.x;
                        src_checkerboard_cur_y++;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D) != 0) {
                // 0x50DEE7
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                                }
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            dst_checkerboard_cur_x++;
                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        dst_checkerboard_cur_x = dst_rect.x;
                        dst_checkerboard_cur_y++;
                    }
                    break;
                }
            } else {
                // 0x50CD1B
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        mask = &(blit_info->field_14[src_rect.x]);

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels], *mask);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            mask++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        } else if ((blit_info->flags & TIG_ART_BLT_BLEND_COLOR_LERP) != 0) {
            // 0x50F3B4
            int tl_r = tig_color_get_red(blit_info->field_14[0]);
            int tl_g = tig_color_get_green(blit_info->field_14[0]);
            int tl_b = tig_color_get_blue(blit_info->field_14[0]);

            int tr_r = tig_color_get_red(blit_info->field_14[1]);
            int tr_g = tig_color_get_green(blit_info->field_14[1]);
            int tr_b = tig_color_get_blue(blit_info->field_14[1]);

            int br_r = tig_color_get_red(blit_info->field_14[2]);
            int br_g = tig_color_get_green(blit_info->field_14[2]);
            int br_b = tig_color_get_blue(blit_info->field_14[2]);

            int bl_r = tig_color_get_red(blit_info->field_14[3]);
            int bl_g = tig_color_get_green(blit_info->field_14[3]);
            int bl_b = tig_color_get_blue(blit_info->field_14[3]);

            float vert_start_step_r = (float)(bl_r - tl_r) / blit_info->field_18->height;
            float vert_start_r = vert_start_step_r * (src_rect.y - blit_info->field_18->y) + tl_r;
            float vert_end_step_r = (float)(br_r - tr_r) / blit_info->field_18->height;
            float vert_end_r = vert_end_step_r * (src_rect.y - blit_info->field_18->y) + tr_r;

            float vert_start_step_g = (float)(bl_g - tl_g) / blit_info->field_18->height;
            float vert_start_g = vert_start_step_g * (src_rect.y - blit_info->field_18->y) + tl_g;
            float vert_end_step_g = (float)(br_g - tr_g) / blit_info->field_18->height;
            float vert_end_g = vert_end_step_g * (src_rect.y - blit_info->field_18->y) + tr_g;

            float vert_start_step_b = (float)(bl_b - tl_b) / blit_info->field_18->height;
            float vert_start_b = vert_start_step_b * (src_rect.y - blit_info->field_18->y) + tl_b;
            float vert_end_step_b = (float)(br_b - tr_b) / blit_info->field_18->height;
            float vert_end_b = vert_end_step_b * (src_rect.y - blit_info->field_18->y) + tr_b;

            switch (tig_art_bits_per_pixel) {
            case 32:
                for (y = 0; y < dst_rect.height; y++) {
                    float hor_step_r = (vert_end_r - vert_start_r) / blit_info->field_18->width;
                    float hor_step_g = (vert_end_g - vert_start_g) / blit_info->field_18->width;
                    float hor_step_b = (vert_end_b - vert_start_b) / blit_info->field_18->width;

                    float r = vert_start_r + hor_step_r * (src_rect.x - blit_info->field_18->x);
                    float g = vert_start_g + hor_step_g * (src_rect.x - blit_info->field_18->x);
                    float b = vert_start_b + hor_step_b * (src_rect.x - blit_info->field_18->x);

                    for (x = 0; x < dst_rect.width; x++) {
                        if (*src_pixels != 0) {
                            uint32_t color = tig_color_make((uint8_t)r, (uint8_t)g, (uint8_t)b);
                            color = tig_color_mul(((uint32_t*)plt)[*src_pixels], color);
                            *(uint32_t*)dst_pixels = color;
                        }
                        src_pixels += src_step;
                        dst_pixels += 4;

                        r += hor_step_r;
                        g += hor_step_g;
                        b += hor_step_b;
                    }
                    src_pixels += src_pitch;
                    dst_pixels += dst_skip;

                    vert_start_r += vert_start_step_r;
                    vert_end_r += vert_end_step_r;

                    vert_start_g += vert_start_step_g;
                    vert_end_g += vert_end_step_g;

                    vert_start_b += vert_start_step_b;
                    vert_end_b += vert_end_step_b;
                }
                break;
            }
        } else {
            // 0x50655B
            if ((blit_info->flags & TIG_ART_BLT_BLEND_ADD) != 0) {
                // 0x506566
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_add(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_SUB) != 0) {
                // 0x506838
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_sub(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_MUL) != 0) {
                // 0x506B87
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_mul(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_AVG) != 0) {
                // 0x506FD0
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    tig_color_rgb_to_grayscale(((uint32_t*)plt)[*src_pixels]));
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_CONST) != 0) {
                // 0x50742A
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    blit_info->alpha[0]);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_SRC) != 0) {
                // 0x507B0F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    tig_color_alpha(((uint32_t*)plt)[*src_pixels]));
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) != 0) {
                // 0x507ECB
                switch (blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_LERP_ANY) {
                case TIG_ART_BLT_BLEND_ALPHA_LERP_X:
                    start_alpha_vertical_step = 0.0;
                    end_alpha_vertical_step = 0.0;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((float)blit_info->alpha[1] - (float)blit_info->alpha[0]) / width;
                    start_alpha = (float)blit_info->alpha[0] + start_alpha_x * alpha_range;
                    end_alpha = (float)blit_info->alpha[1] - end_alpha_x * alpha_range;
                    break;
                case TIG_ART_BLT_BLEND_ALPHA_LERP_Y:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = start_alpha_vertical_step;
                    start_alpha = src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0];
                    end_alpha = start_alpha;
                    break;
                default:
                    start_alpha_vertical_step = ((float)blit_info->alpha[3] - (float)blit_info->alpha[0]) / height;
                    end_alpha_vertical_step = ((float)(uint8_t)blit_info->alpha[2] - (float)blit_info->alpha[1]) / height;
                    start_alpha_x = (float)src_rect.x;
                    end_alpha_x = (float)(width - src_rect.width - src_rect.x);
                    alpha_range = ((src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0])) / width;
                    start_alpha = (src_rect.y * start_alpha_vertical_step + (float)blit_info->alpha[0]) + start_alpha_x * alpha_range;
                    end_alpha = (src_rect.y * end_alpha_vertical_step + (float)blit_info->alpha[1]) - end_alpha_x * alpha_range;
                    break;
                }

                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        current_alpha = start_alpha;
                        alpha_horizontal_step = (end_alpha - start_alpha) / src_rect.width;

                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = tig_color_blend_alpha(((uint32_t*)plt)[*src_pixels],
                                    *(uint32_t*)dst_pixels,
                                    (int)current_alpha);
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            current_alpha += alpha_horizontal_step;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        start_alpha += start_alpha_vertical_step;
                        end_alpha += end_alpha_vertical_step;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S) != 0) {
                // 0x50779F
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((src_checkerboard_cur_x ^ src_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = ((uint32_t*)plt)[*src_pixels];
                                }
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            src_checkerboard_cur_x++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        src_checkerboard_cur_x = src_rect.x;
                        src_checkerboard_cur_y++;
                    }
                    break;
                }
            } else if ((blit_info->flags & TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D) != 0) {
                // 0x507955
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (((dst_checkerboard_cur_x ^ dst_checkerboard_cur_y) & 1) != 0) {
                                if (*src_pixels != 0) {
                                    *(uint32_t*)dst_pixels = ((uint32_t*)plt)[*src_pixels];
                                }
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;

                            dst_checkerboard_cur_x++;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;

                        dst_checkerboard_cur_x = dst_rect.x;
                        dst_checkerboard_cur_y++;
                    }
                    break;
                }
            } else {
                // 0x506FC8
                switch (tig_art_bits_per_pixel) {
                case 32:
                    for (y = 0; y < dst_rect.height; y++) {
                        for (x = 0; x < dst_rect.width; x++) {
                            if (*src_pixels != 0) {
                                *(uint32_t*)dst_pixels = ((uint32_t*)plt)[*src_pixels];
                            }
                            src_pixels += src_step;
                            dst_pixels += 4;
                        }
                        src_pixels += src_pitch;
                        dst_pixels += dst_skip;
                    }
                    break;
                }
            }
        }
    }

    tig_video_buffer_unlock(blit_info->dst_video_buffer);

    return TIG_OK;
}

// 0x51AA90
int sub_51AA90(tig_art_id_t art_id)
{
    char path[TIG_MAX_PATH];
    int cache_entry_index;

    if (tig_art_build_path(art_id, path) != TIG_OK) {
        return -1;
    }

    if (dword_604714 < tig_art_cache_entries_length
        && strcmp(path, tig_art_cache_entries[dword_604714].path) == 0) {
        tig_art_cache_entries[dword_604714].time = tig_ping_timestamp;
        return dword_604714;
    }

    // Called twice to check both system and video memory.
    tig_art_cache_check_fullness();
    tig_art_cache_check_fullness();

    if (!tig_art_cache_find(path, &cache_entry_index)) {
        if (!tig_art_cache_entry_load(art_id, path, cache_entry_index)) {
            tig_debug_printf("ART LOAD FAILURE!!! Trying to load %s\n", path);

            if (!tig_art_cache_entry_load(art_id, "art\\badart.art", cache_entry_index)) {
                tig_debug_printf("ART LOAD FAILURE!!! Trying to load badart.art\n");
                return -1;
            }
        }
    }

    tig_art_cache_entries[cache_entry_index].time = tig_ping_timestamp;
    tig_art_cache_entries[cache_entry_index].art_id = art_id;
    dword_604714 = cache_entry_index;

    return cache_entry_index;
}

// 0x51AC00
void tig_art_cache_set_video_memory_fullness(int fullness)
{
    tig_art_cache_video_memory_fullness = (float)fullness / 100.0f;
}

// 0x51AC20
void tig_art_cache_check_fullness()
{
    // Flag that denotes if we should check video memory (true) or system memory
    // (false). This flag alternates on every call.
    //
    // 0x604728
    static bool vid_vs_sys;

    art_size_t acc = 0;
    art_size_t tgt;
    int index;
    int cnt;

    if (vid_vs_sys) {
        // NOTE: Signed compare.
        if (tig_art_available_video_memory > 0) {
            vid_vs_sys = !vid_vs_sys;
            return;
        }

        tig_debug_printf("Art cache full (vid), making some room");

        // Calculate target size we'd like to evict.
        tgt = (art_size_t)((double)tig_art_total_video_memory * tig_art_cache_video_memory_fullness);

        // Sort cache entries by last access time, earliest first.
        qsort(tig_art_cache_entries,
            tig_art_cache_entries_length,
            sizeof(TigArtCacheEntry),
            tig_art_cache_entry_compare_time);

        // Loop thru cache entries until we reach eviction target.
        for (index = 0; index < tig_art_cache_entries_length; ++index) {
            acc += tig_art_cache_entries[index].video_memory_usage;
            if (acc >= tgt) {
                break;
            }
        }
    } else {
        // NOTE: Signed compare.
        if (tig_art_available_system_memory > 0) {
            vid_vs_sys = !vid_vs_sys;
            return;
        }

        tig_debug_printf("Art cache full (sys), making some room");

        // Calculate target size we'd like to evict (30% of total system
        // memory).
        tgt = (art_size_t)((double)tig_art_total_system_memory * 0.3f);

        // Sort cache entries by last access time, earliest first.
        qsort(tig_art_cache_entries,
            tig_art_cache_entries_length,
            sizeof(TigArtCacheEntry),
            tig_art_cache_entry_compare_time);

        // Loop thru cache entries until we reach eviction target.
        for (index = 0; index < tig_art_cache_entries_length; ++index) {
            acc += tig_art_cache_entries[index].system_memory_usage;
            if (acc >= tgt) {
                break;
            }
        }
    }

    // NOTE: Signed compare.
    if (acc < tgt) {
        // We haven't reached eviction target, flush everything.
        tig_art_flush();
        tig_debug_printf("...\n");
        return;
    }

    cnt = index + 1;

    // Free cache entries.
    for (; index >= 0; index--) {
        tig_art_cache_entry_unload(index);
    }

    tig_art_cache_entries_length -= cnt;

    // Move remaining cache entries on top.
    memmove(tig_art_cache_entries,
        &(tig_art_cache_entries[cnt]),
        sizeof(TigArtCacheEntry) * (tig_art_cache_entries_length));

    // Sort back cache entries by name (as `tig_art_cache_find` uses binary
    // search by name).
    qsort(tig_art_cache_entries,
        tig_art_cache_entries_length,
        sizeof(TigArtCacheEntry),
        tig_art_cache_entry_compare_name);

    tig_debug_printf("...\n");
    vid_vs_sys = !vid_vs_sys;
}

// 0x51ADE0
int tig_art_cache_entry_compare_time(const void* a1, const void* a2)
{
    TigArtCacheEntry* a = (TigArtCacheEntry*)a1;
    TigArtCacheEntry* b = (TigArtCacheEntry*)a2;
    return a->time - b->time;
}

// 0x51AE00
int tig_art_cache_entry_compare_name(const void* a1, const void* a2)
{
    TigArtCacheEntry* a = (TigArtCacheEntry*)a1;
    TigArtCacheEntry* b = (TigArtCacheEntry*)a2;
    return strcmp(a->path, b->path);
}

// 0x51AE50
int tig_art_build_path(unsigned int art_id, char* path)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_MISC) {
        switch (tig_art_num_get(art_id)) {
        case TIG_ART_SYSTEM_MOUSE:
            strcpy(path, "art\\mouse.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_BUTTON:
            strcpy(path, "art\\button.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_UP:
            strcpy(path, "art\\up.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_DOWN:
            strcpy(path, "art\\down.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_CANCEL:
            strcpy(path, "art\\cancel.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_LENS:
            strcpy(path, "art\\lens.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_X:
            strcpy(path, "art\\x.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_PLUS:
            strcpy(path, "art\\plus.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_MINUS:
            strcpy(path, "art\\minus.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_BLANK:
            strcpy(path, "art\\blank.art");
            return TIG_OK;
        case TIG_ART_SYSTEM_FONT:
            strcpy(path, "art\\morph15font.art");
            return TIG_OK;
        }
    } else {
        if (tig_art_file_path_resolver != NULL) {
            return tig_art_file_path_resolver(art_id, path);
        }
    }

    return TIG_ERR_GENERIC;
}

// 0x51B0A0
tig_art_id_t tig_art_id_reset(tig_art_id_t art_id)
{
    if (tig_art_type(art_id) == TIG_ART_TYPE_MISC) {
        art_id = tig_art_id_palette_set(art_id, 0);
        art_id = tig_art_id_frame_set(art_id, 0);
        return art_id;
    }

    if (tig_art_id_reset_func != NULL) {
        return tig_art_id_reset_func(art_id);
    }

    return art_id;
}

// 0x51B0E0
bool tig_art_cache_find(const char* path, int* index)
{
    int l = 0;
    int r = tig_art_cache_entries_length - 1;

    while (l <= r) {
        int mid = (l + r) / 2;
        int cmp = strcmp(tig_art_cache_entries[mid].path, path);
        if (cmp == 0) {
            *index = mid;
            return true;
        }

        if (cmp > 0) {
            r = mid - 1;
        } else {
            l = mid + 1;
        }
    }

    *index = l;
    return false;
}

// 0x51B170
bool tig_art_cache_entry_load(tig_art_id_t art_id, const char* path, int cache_entry_index)
{
    TigArtCacheEntry* art;
    int rc;
    art_size_t size;
    int type;
    int start;
    int num_rotations;
    int rotation;
    int index;
    int frame;
    int offset;

    if (tig_art_cache_entries_length == tig_art_cache_entries_capacity - 1) {
        tig_art_cache_entries_capacity += 32;
        tig_art_cache_entries = (TigArtCacheEntry*)REALLOC(tig_art_cache_entries,
            sizeof(TigArtCacheEntry) * tig_art_cache_entries_capacity);
    }

    memmove(&(tig_art_cache_entries[cache_entry_index + 1]),
        &(tig_art_cache_entries[cache_entry_index]),
        sizeof(TigArtCacheEntry) * (tig_art_cache_entries_length - cache_entry_index));

    art = &(tig_art_cache_entries[cache_entry_index]);

    memset(art, 0, sizeof(TigArtCacheEntry));
    strcpy(art->path, path);

    rc = sub_51B710(art_id,
        path,
        &(art->hdr),
        art->palette_tbl,
        0,
        &size);
    if (rc != TIG_OK) {
        memmove(&(tig_art_cache_entries[cache_entry_index]),
            &(tig_art_cache_entries[cache_entry_index + 1]),
            sizeof(TigArtCacheEntry) * (tig_art_cache_entries_length - cache_entry_index));
        return false;
    }

    art->system_memory_usage += size;

    type = tig_art_type(art_id);
    if ((art->hdr.flags & TIG_ART_0x01) != 0) {
        start = 0;
        num_rotations = 1;
    } else if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)) {
        start = 4;
        num_rotations = 5;
    } else {
        start = 0;
        num_rotations = MAX_ROTATIONS;
    }

    for (index = 0; index < num_rotations; index++) {
        rotation = (index + start) % MAX_ROTATIONS;
        art->pixels_tbl[rotation] = (uint8_t**)MALLOC(sizeof(uint8_t*) * art->hdr.num_frames);
        art->system_memory_usage += sizeof(uint8_t*) * art->hdr.num_frames;

        offset = 0;
        for (frame = 0; frame < art->hdr.num_frames; ++frame) {
            art->pixels_tbl[rotation][frame] = art->hdr.pixels_tbl[rotation] + offset;
            offset += art->hdr.frames_tbl[rotation][frame].width * art->hdr.frames_tbl[rotation][frame].height;
        }
    }

    if (MAX_ROTATIONS - num_rotations > 0) {
        rotation = num_rotations + start;
        for (index = MAX_ROTATIONS - num_rotations; index > 0; --index) {
            art->pixels_tbl[rotation % MAX_ROTATIONS] = art->pixels_tbl[0];
            rotation++;
        }
    }

    tig_art_cache_entries_length++;
    tig_art_available_system_memory -= art->system_memory_usage;

    return true;
}

// 0x51B490
void tig_art_cache_entry_unload(int cache_entry_index)
{
    TigArtCacheEntry* cache_entry;
    int type;
    int rotation_start;
    int num_rotations;
    int rotation;
    int palette;
    int idx;

    cache_entry = &(tig_art_cache_entries[cache_entry_index]);

    tig_art_available_system_memory += cache_entry->system_memory_usage;
    tig_art_available_video_memory += cache_entry->video_memory_usage;

    sub_51B650(cache_entry_index);

    type = tig_art_type(cache_entry->art_id);

    if ((cache_entry->hdr.flags & TIG_ART_0x01) != 0) {
        rotation_start = 0;
        num_rotations = 1;
    } else if (tig_art_mirroring_enabled
        && (type == TIG_ART_TYPE_CRITTER
            || type == TIG_ART_TYPE_MONSTER
            || type == TIG_ART_TYPE_UNIQUE_NPC)) {
        rotation_start = 4;
        num_rotations = 5;
    } else {
        rotation_start = 0;
        num_rotations = MAX_ROTATIONS;
    }

    for (idx = 0; idx < num_rotations; ++idx) {
        rotation = (idx + rotation_start) % MAX_ROTATIONS;
        FREE(cache_entry->pixels_tbl[rotation]);
        FREE(cache_entry->hdr.pixels_tbl[rotation]);
        FREE(cache_entry->hdr.frames_tbl[rotation]);
    }

    for (palette = 0; palette < MAX_PALETTES; ++palette) {
        if (cache_entry->hdr.palette_tbl[palette] != NULL) {
            tig_palette_destroy(cache_entry->hdr.palette_tbl[palette]);
        }

        if (cache_entry->palette_tbl[palette] != NULL) {
            tig_palette_destroy(cache_entry->palette_tbl[palette]);
        }
    }
}

// 0x51B610
void art_invalidate(int cache_entry_index)
{
    int palette;
    int rotation;

    // Mark all palette and rotation variants as dirty so that the subsequent
    // call to `art_get_video_buffer` updates appropriate video buffer.
    for (palette = 0; palette < MAX_PALETTES; palette++) {
        for (rotation = 0; rotation < MAX_ROTATIONS; rotation++) {
            tig_art_cache_entries[cache_entry_index].dirty[palette][rotation] = 1;
        }
    }
}

// 0x51B650
void sub_51B650(int cache_entry_index)
{
    TigArtCacheEntry* art;
    int num_frames;
    int palette;
    int rotation;
    int frame;

    art = &(tig_art_cache_entries[cache_entry_index]);

    if (tig_art_type(art->art_id) == TIG_ART_TYPE_ROOF) {
        num_frames = 13;
    } else {
        num_frames = art->hdr.num_frames;
    }

    for (palette = 0; palette < MAX_PALETTES; palette++) {
        for (rotation = 0; rotation < MAX_ROTATIONS; rotation++) {
            if (art->video_buffers[palette][rotation] != NULL) {
                for (frame = 0; frame < num_frames; frame++) {
                    tig_video_buffer_destroy(art->video_buffers[palette][rotation][frame]);
                }

                FREE(art->video_buffers[palette][rotation]);
                art->video_buffers[palette][rotation] = NULL;
            }
        }
    }
}

// 0x51B710
int sub_51B710(tig_art_id_t art_id, const char* filename, TigArtHeader* hdr, void** palette_tbl, int a5, art_size_t* size_ptr)
{
    TigFile* stream;
    int rotation;
    int palette;
    uint32_t* saved_palette_tbl[MAX_PALETTES];
    uint32_t temp_palette_entries[256];
    art_size_t size_tbl[MAX_ROTATIONS];
    int index;
    int frame;
    int current_palette_index;
    void* current_palette;
    int num_rotations;

    // NOTE: Keep compiler happy.
    current_palette_index = 0;
    current_palette = NULL;

    *size_ptr = 0;

    for (rotation = 0; rotation < MAX_ROTATIONS; rotation++) {
        hdr->frames_tbl[rotation] = NULL;
        hdr->pixels_tbl[rotation] = NULL;
    }

    stream = tig_file_fopen(filename, "rb");
    if (stream == NULL) {
        return TIG_ERR_GENERIC;
    }

    if (!art_read_header(hdr, stream)) {
        tig_file_fclose(stream);
        return TIG_ERR_GENERIC;
    }

    // Only 8-bpp palette-indexed ART files are supported.
    if (hdr->bpp != 8) {
        tig_file_fclose(stream);
        return TIG_ERR_GENERIC;
    }

    if (a5) {
        current_palette_index = tig_art_id_palette_get(art_id);
        if (hdr->palette_tbl[current_palette_index] != NULL) {
            tig_file_fclose(stream);
            return TIG_ERR_GENERIC;
        }
        current_palette = palette_tbl[0];
    }

    for (palette = 0; palette < MAX_PALETTES; palette++) {
        saved_palette_tbl[palette] = hdr->palette_tbl[palette];
        hdr->palette_tbl[palette] = NULL;
        palette_tbl[palette] = NULL;
    }

    for (palette = 0; palette < MAX_PALETTES; palette++) {
        if (saved_palette_tbl[palette] != NULL) {
            if (tig_file_fread(temp_palette_entries, sizeof(uint32_t), 256, stream) != 256) {
                sub_51BE50(stream, hdr, palette_tbl);
                return TIG_ERR_GENERIC;
            }

            if (a5) {
                if (palette == current_palette_index) {
                    for (index = 0; index < 256; index++) {
                        ((uint32_t*)current_palette)[index] = temp_palette_entries[index];
                    }

                    tig_file_fclose(stream);
                    return TIG_OK;
                }
            } else {
                hdr->palette_tbl[palette] = tig_palette_create();
                palette_tbl[palette] = tig_palette_create();
                *size_ptr += (art_size_t)tig_palette_system_memory_size() * 2;

                switch (tig_art_bits_per_pixel) {
                case 8:
                    for (index = 0; index < 256; index++) {
                        ((uint8_t*)hdr->palette_tbl[palette])[index] = (uint8_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                case 16:
                    for (index = 0; index < 256; index++) {
                        ((uint16_t*)hdr->palette_tbl[palette])[index] = (uint16_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                case 24:
                    for (index = 0; index < 256; index++) {
                        ((uint32_t*)hdr->palette_tbl[palette])[index] = (uint32_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                case 32:
                    for (index = 0; index < 256; index++) {
                        ((uint32_t*)hdr->palette_tbl[palette])[index] = (uint32_t)tig_color_index_of(temp_palette_entries[index]);
                    }
                    break;
                }
            }

            sub_505000(art_id, hdr->palette_tbl[palette], palette_tbl[palette]);
        }
    }

    num_rotations = sub_51BE30(hdr);

    for (rotation = 0; rotation < num_rotations; rotation++) {
        hdr->frames_tbl[rotation] = (TigArtFileFrameData*)MALLOC(sizeof(TigArtFileFrameData) * hdr->num_frames);
        *size_ptr += sizeof(TigArtFileFrameData) * hdr->num_frames;

        if (tig_file_fread(hdr->frames_tbl[rotation], sizeof(TigArtFileFrameData), hdr->num_frames, stream) != (size_t)hdr->num_frames) {
            sub_51BE50(stream, hdr, palette_tbl);
            return TIG_ERR_GENERIC;
        }
    }

    for (index = 0; index < num_rotations; index++) {
        uint8_t* bytes;
        art_size_t total_size;

        // Calculate total size of pixels data.
        total_size = 0;
        for (frame = 0; frame < hdr->num_frames; ++frame) {
            total_size += hdr->frames_tbl[index][frame].width * hdr->frames_tbl[index][frame].height;
        }

        // Allocate appropriate memory.
        hdr->pixels_tbl[index] = (uint8_t*)MALLOC(total_size);

        // Store size for later use.
        size_tbl[index] = total_size;
        *size_ptr += total_size;

        // Read raw pixel data for each frame.
        bytes = hdr->pixels_tbl[index];
        for (frame = 0; frame < hdr->num_frames; ++frame) {
            if (hdr->frames_tbl[index][frame].data_size == hdr->frames_tbl[index][frame].width * hdr->frames_tbl[index][frame].height) {
                // Pixels are not compressed, read everything in one go.
                if (tig_file_fread(bytes, 1, hdr->frames_tbl[index][frame].data_size, stream) != (size_t)hdr->frames_tbl[index][frame].data_size) {
                    sub_51BE50(stream, hdr, palette_tbl);
                    return TIG_ERR_GENERIC;
                }
                bytes += hdr->frames_tbl[index][frame].data_size;
            } else if (hdr->frames_tbl[index][frame].data_size > 0) {
                // Pixels are RLE-encoded.
                uint8_t value;
                uint8_t color;
                int len;
                int cnt = 0;

                while (cnt < hdr->frames_tbl[index][frame].data_size) {
                    if (tig_file_fread(&value, 1, 1, stream) != 1) {
                        sub_51BE50(stream, hdr, palette_tbl);
                        return TIG_ERR_GENERIC;
                    }

                    len = value & 0x7F;
                    if ((value & 0x80) != 0) {
                        if (tig_file_fread(bytes, 1, len, stream) != (size_t)len) {
                            sub_51BE50(stream, hdr, palette_tbl);
                            return TIG_ERR_GENERIC;
                        }
                        cnt += 1 + len;
                    } else {
                        if (tig_file_fread(&color, 1, 1, stream) != 1) {
                            sub_51BE50(stream, hdr, palette_tbl);
                            return TIG_ERR_GENERIC;
                        }

                        memset(bytes, color, len);
                        cnt += 2;
                    }
                    bytes += len;
                }
            }
        }
    }

    while (index < MAX_ROTATIONS) {
        hdr->frames_tbl[index] = hdr->frames_tbl[0];
        hdr->pixels_tbl[index] = hdr->pixels_tbl[0];
        index++;
    }

    if (num_rotations > 1) {
        int type = tig_art_type(art_id);
        if (tig_art_mirroring_enabled) {
            if (type == TIG_ART_TYPE_CRITTER
                || type == TIG_ART_TYPE_MONSTER
                || type == TIG_ART_TYPE_UNIQUE_NPC) {
                for (index = 0; index < 3; index++) {
                    *size_ptr -= size_tbl[1 + index];

                    FREE(hdr->pixels_tbl[1 + index]);
                    hdr->pixels_tbl[1 + index] = hdr->pixels_tbl[0];

                    FREE(hdr->frames_tbl[1 + index]);
                    hdr->frames_tbl[1 + index] = hdr->frames_tbl[0];
                }
            }
        }
    }

    tig_file_fclose(stream);
    return TIG_OK;
}

// 0x51BE30
int sub_51BE30(TigArtHeader* hdr)
{
    return (hdr->flags & TIG_ART_0x01) != 0 ? 1 : MAX_ROTATIONS;
}

// 0x51BE50
void sub_51BE50(TigFile* stream, TigArtHeader* hdr, TigPalette* palette_tbl)
{
    int palette;

    if (stream != NULL) {
        tig_file_fclose(stream);
    }

    sub_51BF20(hdr);

    for (palette = 0; palette < MAX_PALETTES; ++palette) {
        if (hdr->palette_tbl[palette] != NULL) {
            tig_palette_destroy(hdr->palette_tbl[palette]);
        }

        if (palette_tbl[palette] != NULL) {
            tig_palette_destroy(palette_tbl[palette]);
        }
    }
}

// 0x51BF20
void sub_51BF20(TigArtHeader* hdr)
{
    int num_rotations;
    int rotation;

    num_rotations = sub_51BE30(hdr);
    for (rotation = 0; rotation < num_rotations; rotation++) {
        if (hdr->pixels_tbl[rotation] != NULL) {
            FREE(hdr->pixels_tbl[rotation]);
        }
        if (hdr->frames_tbl[rotation] != NULL) {
            FREE(hdr->frames_tbl[rotation]);
        }
    }
}

bool art_read_header(TigArtHeader* hdr, TigFile* stream)
{
    int idx;
    int value;

    if (tig_file_fread(&(hdr->flags), sizeof(hdr->flags), 1, stream) != 1) return false;
    if (tig_file_fread(&(hdr->fps), sizeof(hdr->fps), 1, stream) != 1) return false;
    if (tig_file_fread(&(hdr->bpp), sizeof(hdr->bpp), 1, stream) != 1) return false;

    // Read palette table, non-zero value indicates presence of palette entries.
    for (idx = 0; idx < MAX_PALETTES; idx++) {
        if (tig_file_fread(&(value), sizeof(value), 1, stream) != 1) return false;
        hdr->palette_tbl[idx] = (uint32_t*)(intptr_t)value;
    }

    if (tig_file_fread(&(hdr->action_frame), sizeof(hdr->action_frame), 1, stream) != 1) return false;
    if (tig_file_fread(&(hdr->num_frames), sizeof(hdr->num_frames), 1, stream) != 1) return false;

    // Skip frames table, actual values are ignored.
    if (tig_file_fseek(stream, 4 * MAX_ROTATIONS, SEEK_CUR) != 0) return false;

    if (tig_file_fread(&(hdr->data_size), sizeof(hdr->data_size), 1, stream) != 1) return false;

    // Skip pixels table, actual values are ignored.
    if (tig_file_fseek(stream, 4 * MAX_ROTATIONS, SEEK_CUR) != 0) return false;

    return true;
}
