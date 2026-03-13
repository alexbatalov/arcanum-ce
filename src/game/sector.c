#include "game/sector.h"

#include <inttypes.h>
#include <stdio.h>

#include "game/gamelib.h"
#include "game/li.h"
#include "game/map.h"
#include "game/obj_file.h"
#include "game/obj_private.h"
#include "game/terrain.h"
#include "game/tile.h"
#include "game/timeevent.h"
#include "game/townmap.h"

typedef uint32_t DifferenceFileFlags;

#define DIF_HAVE_LIGHT_LIST 0x0001u
#define DIF_HAVE_TILE_LIST 0x0002u
#define DIF_HAVE_ROOF_LIST 0x0004u
#define DIF_HAVE_OBJ_LIST 0x0008u
#define DIF_HAVE_TILE_SCRIPT_LIST 0x0010u
#define DIF_HAVE_SECTOR_SCRIPT_LIST 0x0020u
#define DIF_HAVE_TOWNMAP 0x0040u
#define DIF_HAVE_APTITUDE_ADJ 0x0080u
#define DIF_HAVE_LIGHT_SCHEME 0x0100u
#define DIF_HAVE_SOUND_LIST 0x0200u
#define DIF_HAVE_BLOCK_LIST 0x0400u

typedef bool(SectorSaveFunc)(Sector* sector);
typedef bool(SectorLoadFunc)(int64_t id, Sector* sector);

typedef struct SectorCacheEntry {
    /* 0000 */ bool used;
    /* 0004 */ int refcount;
    /* 0008 */ unsigned int timestamp;
    /* 000C */ int field_C;
    /* 0010 */ Sector sector;
} SectorCacheEntry;

typedef struct SectorHistoryEntry {
    int64_t id;
    DateTime datetime;
} SectorHistoryEntry;

// Serializeable.
static_assert(sizeof(SectorHistoryEntry) == 0x10, "wrong size");

static bool sector_cache_init(unsigned int capacity);
static void sector_block_clear(void);
static void sector_history_clear(void);
static int sub_4D1310(int64_t a1, int64_t a2, int a3, int64_t* a4);
static SectorListNode* sector_list_node_create(void);
static void sector_list_node_reserve(void);
static void sub_4D1400(Sector* sector);
static bool sector_load_editor(int64_t id, Sector* sector);
static bool sector_load_game(int64_t id, Sector* sector);
static bool sector_save_editor(Sector* sector);
static bool sub_4D2460(Sector* sector, const char* base_path);
static bool sector_save_game(Sector* sector);
static void sector_validate_game(const char* section);
static void sector_validate_editor(const char* section);
static bool sector_cache_find_by_id(int64_t id, int* index_ptr);
static bool sector_cache_find_unused(unsigned int* index_ptr);
static void sector_load_demo_limits(void);
static void sector_art_cache_clear(void);
static void sector_precache_art(Sector* sector);
static void sector_art_cache_reset(void);
static void sector_art_cache_sort(void);
static int sector_art_cache_compare(const void* va, const void* vb);
static int sector_block_find(int64_t sec);
static void sector_block_add(int64_t sec);
static void sector_block_remove(int idx);
static bool sector_block_save_internal(void);
static bool sector_block_load_internal(const char* base_map_name, const char* current_map_name);

// 0x5B7CD0
static DateTime qword_5B7CD0 = { .days = -1, .milliseconds = -1 };

// 0x5B7CD8
static int sector_art_cache_state = 1;

// 0x601780
static unsigned int sector_cache_capacity;

// 0x601784
static unsigned int sector_cache_size;

// 0x601788
static SectorSaveFunc* sector_save_func;

// 0x60178C
static bool in_sector_lock;

// 0x601790
static int sector_blocked_sectors_cnt;

// 0x601794
static int64_t* sector_blocked_sectors;

// 0x601798
static bool sector_blocked_sectors_changed;

// 0x60179C
static char* sector_base_path;

// 0x6017A0
static int64_t sector_limit_x;

// 0x6017A8
static bool sector_editor;

// 0x6017AC
static tig_color_t dword_6017AC;

// 0x6017B0
static SectorCacheEntry* sector_cache_entries;

// 0x6017B4
static SectorLoadFunc* sector_load_func;

// 0x6017B8
static IsoInvalidateRectFunc* sector_iso_window_invalidate_rect;

// 0x6017BC
static int dword_6017BC;

// 0x6017C0
static bool in_sector_enumerate;

// 0x6017C4
static int sector_art_cache_capacity;

// 0x6017C8
static int64_t sector_demo_limits_max_y;

// 0x6017D0
static int64_t sector_limit_y;

// 0x6017D8
static int sector_art_cache_size;

// 0x6017E0
static int64_t sector_demo_limits_max_x;

// 0x6017E8
static int64_t* dword_6017E8;

// 0x6017EC
static int64_t* dword_6017EC;

// 0x6017F0
static tig_art_id_t* sector_art_cache;

// 0x6017F4
static int* sector_cache_indexes;

// 0x6017F8
static int sector_iso_window_handle;

// 0x6017FC
static char* sector_save_path;

// 0x601800
static ViewOptions sector_view_options;

// 0x601808
static SectorListNode* sector_list_free_node_head;

// 0x60180C
static SectorHistoryEntry* sector_history_entries;

// 0x601810
static int64_t sector_demo_limits_min_y;

// 0x601818
static int64_t sector_demo_limits_min_x;

// 0x601820
static SectorLockFunc* dword_601820;

// 0x601824
static bool sector_demo_limits_set;

// 0x601828
static tig_color_t dword_601828;

// 0x60182C
static int dword_60182C;

// 0x601830
static bool sector_grid_enabled;

// 0x601834
static unsigned int sector_history_size;

// 0x601838
static int sector_refcount;

// 0x4CEF70
bool sector_init(GameInitInfo* init_info)
{
    dword_601820 = NULL;
    sector_iso_window_handle = init_info->iso_window_handle;
    sector_iso_window_invalidate_rect = init_info->invalidate_rect_func;
    sector_view_options.type = VIEW_TYPE_ISOMETRIC;
    sector_grid_enabled = false;
    dword_6017AC = tig_color_make(255, 255, 0);
    dword_601828 = tig_color_make(255, 255, 0);
    sector_editor = init_info->editor;

    if (sector_editor) {
        sector_load_func = sector_load_editor;
        sector_save_func = sector_save_editor;
    } else {
        sector_load_func = sector_load_game;
        sector_save_func = sector_save_game;
    }

    sector_base_path = (char*)CALLOC(TIG_MAX_PATH, sizeof(*sector_base_path));
    sector_save_path = (char*)CALLOC(TIG_MAX_PATH, sizeof(*sector_save_path));

    dword_6017E8 = (int64_t*)CALLOC(151, sizeof(*dword_6017E8));
    dword_6017EC = (int64_t*)CALLOC(151, sizeof(*dword_6017EC));

    sector_limits_set(0x4000000, 0x4000000);

    if (!sector_cache_init(16)) {
        return false;
    }

    return true;
}

// 0x4CF0D0
void sector_reset(void)
{
    sub_4D0B40();
    sector_history_size = 0;
}

// 0x4CF0E0
void sector_exit(void)
{
    SectorListNode* node;
    unsigned int index;
    Sector* sector;

    sub_4D0B40();

    while (sector_list_free_node_head != NULL) {
        node = sector_list_free_node_head->next;
        FREE(sector_list_free_node_head);
        sector_list_free_node_head = node;
    }

    for (index = 0; index < sector_cache_capacity; index++) {
        sector = &(sector_cache_entries[index].sector);
        if (!sector_object_list_exit(&(sector->objects))) {
            tig_debug_printf("Error destroying object list in sector_exit()\n");
        }

        if (!sector_block_list_exit(&(sector->blocks))) {
            tig_debug_printf("Error destroying tile block list in sector_exit()\n");
        }

        if (!sector_sound_list_exit(&(sector->sounds))) {
            tig_debug_printf("Error destroying sound list in sector_exit()\n");
        }

        if (!sector_script_list_exit(&(sector->sector_scripts))) {
            tig_debug_printf("Error destroying sector script list in sector_exit()\n");
        }

        if (!tile_script_list_exit(&(sector->tile_scripts))) {
            tig_debug_printf("Error destroying tile script list in sector_exit()\n");
        }

        if (!sector_roof_list_exit(&(sector->roofs))) {
            tig_debug_printf("Error destroying roof list in sector_exit()\n");
        }

        if (!sector_tile_list_exit(&(sector->tiles))) {
            tig_debug_printf("Error destroying tile list in sector_exit()\n");
        }

        if (!sector_light_list_exit(&(sector->lights))) {
            tig_debug_printf("Error destroying light list in sector_exit()\n");
        }
    }

    sector_cache_size = 0;
    FREE(sector_cache_indexes);
    FREE(sector_cache_entries);
    FREE(sector_base_path);
    FREE(sector_save_path);
    FREE(dword_6017E8);
    FREE(dword_6017EC);
    sector_art_cache_clear();
}

// 0x4CF2C0
void sector_resize(GameResizeInfo* resize_info)
{
    sector_iso_window_handle = resize_info->window_handle;
}

// 0x4CF320
void sector_map_close(void)
{
    sub_4D0B40();
    if (!gamelib_in_load()) {
        sector_history_size = 0;
    }
}

// 0x4CF340
void sector_update_view(ViewOptions* view_options)
{
    sector_view_options = *view_options;
}

// 0x4CF360
void sub_4CF360(SectorLockFunc* func)
{
    dword_601820 = func;
}

// 0x4CF370
void sector_grid_toggle(void)
{
    sector_grid_enabled = !sector_grid_enabled;
    sector_iso_window_invalidate_rect(NULL);
}

// 0x4CF390
void sector_draw(GameDrawInfo* draw_info)
{
    SectorListNode* node;
    int sector_x;
    int sector_y;
    int townmap;
    tig_color_t color;
    int64_t location;
    int64_t location_x;
    int64_t location_y;
    int x;
    int y;
    int size;
    TigRect rect;
    TigRect dirty_rect;
    TigRectListNode* rect_node;

    if (!sector_grid_enabled) {
        return;
    }

    if (sector_view_options.type != VIEW_TYPE_TOP_DOWN) {
        return;
    }

    node = draw_info->sectors;
    while (node != NULL) {
        sector_x = SECTOR_X(node->sec);
        sector_y = SECTOR_Y(node->sec);

        townmap = townmap_get(node->sec);
        if (townmap != 0) {
            int red = townmap * (255 / townmap_count());
            if ((townmap & 1) != 0) {
                color = tig_color_make(red, 255 - red, 0);
            } else {
                color = tig_color_make(red, 0, 255 - red);
            }
        } else {
            color = dword_6017AC;
        }

        location = location_make(sector_x + 63, sector_y);
        location_xy(location, &location_x, &location_y);

        x = location_x & 0xFFFFFFFF;
        y = location_y & 0xFFFFFFFF;
        size = sector_view_options.zoom * 64;

        rect.x = x;
        rect.y = y;
        rect.width = size;
        rect.height = 1;

        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&rect, &(rect_node->rect), &dirty_rect) == TIG_OK) {
                tig_window_box(sector_iso_window_handle, &dirty_rect, color);
            }
            rect_node = rect_node->next;
        }

        rect.x = x;
        rect.y = y;
        rect.width = 1;
        rect.height = size;

        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&rect, &(rect_node->rect), &dirty_rect) == TIG_OK) {
                tig_window_box(sector_iso_window_handle, &dirty_rect, color);
            }
            rect_node = rect_node->next;
        }

        rect.x = size + x - 1;
        rect.y = y;
        rect.width = 1;
        rect.height = size;

        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&rect, &(rect_node->rect), &dirty_rect) == TIG_OK) {
                tig_window_box(sector_iso_window_handle, &dirty_rect, color);
            }
            rect_node = rect_node->next;
        }

        rect.x = x;
        rect.y = size + y - 1;
        rect.width = size;
        rect.height = 1;

        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&rect, &(rect_node->rect), &dirty_rect) == TIG_OK) {
                tig_window_box(sector_iso_window_handle, &dirty_rect, color);
            }
            rect_node = rect_node->next;
        }

        rect.x = size / 2 - sector_view_options.zoom / 2 + x - 1;
        rect.y = y - sector_view_options.zoom;
        rect.width = 2;
        rect.height = 2 * sector_view_options.zoom;

        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&rect, &(rect_node->rect), &dirty_rect) == TIG_OK) {
                tig_window_box(sector_iso_window_handle, &dirty_rect, dword_601828);
            }
            rect_node = rect_node->next;
        }

        rect.x = x - sector_view_options.zoom;
        rect.y = size / 2 + y - 1;
        rect.width = 2 * sector_view_options.zoom;
        rect.height = 2;

        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&rect, &(rect_node->rect), &dirty_rect) == TIG_OK) {
                tig_window_box(sector_iso_window_handle, &dirty_rect, dword_601828);
            }
            rect_node = rect_node->next;
        }

        node = node->next;
    }
}

// 0x4CF790
bool sector_limits_set(int64_t x, int64_t y)
{
    if (x > 0x4000000 || y > 0x4000000) {
        return false;
    }

    sector_limit_x = x;
    sector_limit_y = y;

    return true;
}

// 0x4CF7E0
void sector_limits_get(int64_t* x, int64_t* y)
{
    *x = sector_limit_x;
    *y = sector_limit_y;
}

// 0x4CF810
bool sector_cache_init(unsigned int capacity)
{
    unsigned int index;
    Sector* sector;

    if (sector_refcount > 0) {
        return false;
    }

    sector_flush(0);

    if (capacity < 8) {
        capacity = 8;
    } else if (capacity > 128) {
        capacity = 128;
    }

    if (capacity < sector_cache_capacity) {
        // Requested size is smaller - shrink current cache.
        for (index = sector_cache_capacity - 1; index >= capacity; index--) {
            sector = &(sector_cache_entries[index].sector);
            sector_object_list_exit(&(sector->objects));
            sector_block_list_exit(&(sector->blocks));
            sector_sound_list_exit(&(sector->sounds));
            sector_script_list_exit(&(sector->sector_scripts));
            tile_script_list_exit(&(sector->tile_scripts));
            sector_roof_list_exit(&(sector->roofs));
            sector_light_list_exit(&(sector->lights));
        }

        sector_cache_entries = (SectorCacheEntry*)REALLOC(sector_cache_entries, sizeof(*sector_cache_entries) * capacity);
        sector_cache_indexes = (int*)REALLOC(sector_cache_indexes, sizeof(*sector_cache_indexes) * capacity);
    } else if (capacity > sector_cache_capacity) {
        sector_cache_entries = (SectorCacheEntry*)REALLOC(sector_cache_entries, sizeof(*sector_cache_entries) * capacity);
        sector_cache_indexes = (int*)REALLOC(sector_cache_indexes, sizeof(*sector_cache_indexes) * capacity);

        for (index = sector_cache_capacity; index < capacity; index++) {
            memset(&(sector_cache_entries[index]), 0, sizeof(*sector_cache_entries));
            sector_cache_indexes[index] = 0;

            sector = &(sector_cache_entries[index].sector);
            if (!sector_light_list_init(&(sector->lights))) {
                return false;
            }

            if (!sector_roof_list_init(&(sector->roofs))) {
                sector_light_list_init(&(sector->lights)); // FIXME: Should be sector_light_list_exit.
                return false;
            }

            if (!tile_script_list_init(&(sector->tile_scripts))) {
                sector_roof_list_init(&(sector->roofs)); // FIXME: Should be sector_roof_list_exit.
                sector_light_list_init(&(sector->lights)); // FIXME: Should be sector_light_list_exit.
                return false;
            }

            if (!sector_script_list_init(&(sector->sector_scripts))) {
                tile_script_list_init(&(sector->tile_scripts)); // FIXME: Should be tile_script_list_exit.
                sector_roof_list_init(&(sector->roofs)); // FIXME: Should be sector_roof_list_exit.
                sector_light_list_init(&(sector->lights)); // FIXME: Should be sector_light_list_exit.
                return false;
            }

            if (!sector_sound_list_init(&(sector->sounds))) {
                sector_script_list_init(&(sector->sector_scripts)); // FIXME: Should be sector_script_list_exit.
                tile_script_list_init(&(sector->tile_scripts)); // FIXME: Should be tile_script_list_exit.
                sector_roof_list_init(&(sector->roofs)); // FIXME: Should be sector_roof_list_exit.
                sector_light_list_init(&(sector->lights)); // FIXME: Should be sector_light_list_exit.
                return false;
            }

            if (!sector_block_list_init(&(sector->blocks))) {
                sector_sound_list_exit(&(sector->sounds));
                sector_script_list_init(&(sector->sector_scripts)); // FIXME: Should be sector_script_list_exit.
                tile_script_list_init(&(sector->tile_scripts)); // FIXME: Should be tile_script_list_exit.
                sector_roof_list_init(&(sector->roofs)); // FIXME: Should be sector_roof_list_exit.
                sector_light_list_init(&(sector->lights)); // FIXME: Should be sector_light_list_exit.
                return false;
            }

            if (!sector_object_list_init(&(sector->objects))) {
                sector_block_list_exit(&(sector->blocks));
                sector_sound_list_exit(&(sector->sounds));
                sector_script_list_init(&(sector->sector_scripts)); // FIXME: Should be sector_script_list_exit.
                tile_script_list_init(&(sector->tile_scripts)); // FIXME: Should be tile_script_list_exit.
                sector_roof_list_init(&(sector->roofs)); // FIXME: Should be sector_roof_list_exit.
                sector_light_list_init(&(sector->lights)); // FIXME: Should be sector_light_list_exit.
                return false;
            }

            sector->townmap_info = 0;
            sector->aptitude_adj = 0;
            sector->light_scheme = 0;
        }
    }

    sector_cache_capacity = capacity;
    sector_history_clear();

    return true;
}

// 0x4CFC50
int64_t sector_id_from_loc(int64_t loc)
{
    int64_t x = LOCATION_GET_X(loc) >> 6;
    int64_t y = LOCATION_GET_Y(loc) >> 6;

    return SECTOR_MAKE(x, y);
}

// 0x4CFC90
int64_t sector_loc_from_id(int64_t sector_id)
{
    int64_t x = SECTOR_X(sector_id) << 6;
    int64_t y = SECTOR_Y(sector_id) << 6;

    return LOCATION_MAKE(x, y);
}

// 0x4CFCD0
int sector_rot(int64_t a, int64_t b)
{
    int dx;
    int dy;
    int sx;
    int sy;
    int ddx;
    int ddy;
    int rot;

    dx = (int)SECTOR_X(b) - (int)SECTOR_X(a);
    dy = (int)SECTOR_Y(b) - (int)SECTOR_Y(a);

    ddx = 2 * abs(dx);
    ddy = 2 * abs(dy);

    if (dx < 0) {
        sx = -1;
    } else if (dx > 0) {
        sx = 1;
    } else {
        sx = 0;
    }

    if (dy < 0) {
        sy = -1;
    } else if (dy > 0) {
        sy = 1;
    } else {
        sy = 0;
    }

    if (ddx > ddy) {
        if (ddy - ddx / 2 < 0) {
            sy = 0;
        }
    } else {
        if (ddx - ddy / 2 < 0) {
            sx = 0;
        }
    }

    if (sy >= 0) {
        if (sx >= 0) {
            if (sy > 0) {
                if (sx > 0) {
                    rot = 4;
                } else {
                    rot = 3;
                }
            } else {
                rot = 5;
            }
        } else if (sy > 0) {
            rot = 2;
        } else {
            rot = 1;
        }
    } else if (sx > -1) {
        if (sx > 0) {
            rot = 6;
        } else {
            rot = 7;
        }
    } else {
        rot = 0;
    }

    return rot;
}

// 0x4CFFA0
bool sector_in_dir(int64_t sec, int dir, int64_t* new_sec_ptr)
{
    int64_t x;
    int64_t y;

    x = SECTOR_X(sec);
    y = SECTOR_Y(sec);

    switch (dir) {
    case 0:
        x--;
        y--;
        break;
    case 1:
        x--;
        break;
    case 2:
        x--;
        y++;
        break;
    case 3:
        y++;
        break;
    case 4:
        x++;
        y++;
        break;
    case 5:
        x++;
        break;
    case 6:
        x++;
        y--;
        break;
    case 7:
        y--;
        break;
    }

    if (x < 0
        || x >= sector_limit_x
        || y < 0
        || y >= sector_limit_y) {
        return false;
    }

    *new_sec_ptr = SECTOR_MAKE(x, y);

    return true;
}

// 0x4D0090
bool sub_4D0090(LocRect* rect, SomeSectorStuff* a2)
{
    int x;
    int y;
    int width;
    int height;
    int64_t horizontal[4];
    int64_t vertical[4];

    width = sub_4D1310(rect->x1, rect->x2 + 1, 64, horizontal) - 1;
    if (width == 0) {
        return false;
    }

    height = sub_4D1310(rect->y1, rect->y2 + 1, 64, vertical) - 1;
    if (height == 0) {
        return false;
    }

    for (y = 0; y < height; y++) {
        a2->field_8[y].width = 0;
        a2->field_8[y].field_50 = (int)(vertical[y + 1] - vertical[y]);

        for (x = 0; x < width; x++) {
            a2->field_8[y].field_8[x] = LOCATION_MAKE(horizontal[x], vertical[y]);
            a2->field_8[y].field_20[x] = sector_id_from_loc(a2->field_8[y].field_8[x]);
            a2->field_8[y].field_38[x] = tile_id_from_loc(a2->field_8[y].field_8[x]);
            a2->field_8[y].field_44[x] = (int)(horizontal[x + 1] - horizontal[x]);
        }

        a2->field_8[y].width = width;
    }

    a2->height = height;

    return true;
}

// 0x4D02E0
SectorListNode* sector_list_create(LocRect* loc_rect)
{
    int x;
    int y;
    int width;
    int height;
    SectorListNode* prev;
    SectorListNode* node;

    width = sub_4D1310(loc_rect->x1, loc_rect->x2 + 1, 64, dword_6017E8) - 1;
    if (width == 0) {
        return NULL;
    }

    height = sub_4D1310(loc_rect->y1, loc_rect->y2 + 1, 64, dword_6017EC) - 1;
    if (height == 0) {
        return NULL;
    }

    prev = NULL;
    node = NULL;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            node = sector_list_node_create();
            node->next = prev;
            node->loc = LOCATION_MAKE(dword_6017E8[x], dword_6017EC[y]);
            node->sec = sector_id_from_loc(node->loc);
            node->width = (dword_6017E8[x + 1] & 0xFFFFFFFF) - (dword_6017E8[x] & 0xFFFFFFFF);
            node->height = (dword_6017EC[y + 1] & 0xFFFFFFFF) - (dword_6017EC[y] & 0xFFFFFFFF);
            prev = node;
        }
    }

    return node;
}

// 0x4D0400
void sector_list_destroy(SectorListNode* node)
{
    SectorListNode* tail;

    tail = node;
    while (tail->next != NULL) {
        tail = tail->next;
    }

    tail->next = sector_list_free_node_head;
    sector_list_free_node_head = node;
}

// 0x4D0440
bool sector_map_name_set(const char* base_path, const char* save_path)
{
    strcpy(sector_base_path, base_path);
    strcpy(sector_save_path, save_path);

    sector_load_demo_limits();

    return true;
}

// 0x4D04A0
bool sector_exists(uint64_t id)
{
    char path[TIG_MAX_PATH];
    sprintf(path, "%s\\%" PRIu64 ".sec", sector_base_path, id);
    return tig_file_exists(path, NULL);
}

// 0x4D04E0
bool sector_loaded(int64_t id)
{
    int index;

    if (!sector_cache_find_by_id(id, &index)) {
        return false;
    }

    if ((sector_cache_entries[sector_cache_indexes[index]].sector.flags & SECTOR_IS_NEW) != 0) {
        return false;
    }

    return true;
}

// 0x4D0540
bool sector_lock(int64_t id, Sector** sector_ptr)
{
    SectorCacheEntry* cache_entry;
    unsigned int index;
    unsigned int oldest = -1;
    DateTime datetime;

    if (in_sector_lock) {
        tig_debug_printf("Warning: recursive sector lock detected\n");
        return false;
    }

    if (in_sector_enumerate) {
        return false;
    }

    if (!map_is_valid()) {
        tig_debug_printf("ERROR: Attempt to lock a sector when the map is not valid!!!\n");
    }

    if (SECTOR_X(id) < 0
        || SECTOR_X(id) >= sector_limit_x
        || SECTOR_Y(id) < 0
        || SECTOR_Y(id) >= sector_limit_y) {
        return false;
    }

    in_sector_lock = true;
    dword_6017BC++;

    cache_entry = &(sector_cache_entries[sector_cache_indexes[dword_60182C]]);
    if (cache_entry->used && cache_entry->sector.id == id) {
        cache_entry->refcount++;
        cache_entry->timestamp = dword_6017BC;
    } else if (sector_cache_find_by_id(id, &dword_60182C)) {
        cache_entry = &(sector_cache_entries[sector_cache_indexes[dword_60182C]]);
        cache_entry->refcount++;
        cache_entry->timestamp = dword_6017BC;
    } else {
        if (sector_cache_size >= sector_cache_capacity) {
            for (index = 0; index < sector_cache_capacity; index++) {
                if (sector_cache_entries[sector_cache_indexes[index]].refcount == 0) {
                    if (oldest == -1
                        || sector_cache_entries[sector_cache_indexes[index]].timestamp < sector_cache_entries[sector_cache_indexes[oldest]].timestamp) {
                        oldest = index;
                    }
                }
            }

            if (oldest == -1) {
                tig_debug_println("Warning: attempt to lock sector in cache failed due to lack of unlocked slots available.  This is bad.  Help.\n");
                in_sector_lock = false;
                return false;
            }

            tig_debug_printf("Sector cache full, removing oldest (%I64u)...\n",
                sector_cache_entries[sector_cache_indexes[oldest]].sector.id);

            sector_save_func(&(sector_cache_entries[sector_cache_indexes[oldest]].sector));
            sub_4D1400(&(sector_cache_entries[sector_cache_indexes[oldest]].sector));
            sector_cache_entries[sector_cache_indexes[oldest]].used = false;

            memmove(&(sector_cache_indexes[oldest]),
                &(sector_cache_indexes[oldest + 1]),
                sizeof(*sector_cache_indexes) * (sector_cache_size - oldest - 1));
            sector_cache_size--;

            sector_cache_find_by_id(id, &dword_60182C);
        }

        if (!sector_cache_find_unused(&index)) {
            tig_debug_printf("Error: sector_cache_find_unused() failed to find an available slot in the art cache!\n");
            in_sector_lock = false;
            return false;
        }

        datetime = qword_5B7CD0;
        for (unsigned int j = 0; j < sector_history_size; j++) {
            if (sector_history_entries[j].id == id) {
                datetime = sub_45A7D0(&(sector_history_entries[j].datetime));
                memmove(&(sector_history_entries[j]),
                    &(sector_history_entries[j + 1]),
                    sizeof(*sector_history_entries) * (sector_history_size - j - 1));
                sector_history_size--;
                break;
            }
        }

        cache_entry = &(sector_cache_entries[index]);
        cache_entry->sector.datetime = datetime;

        if (!sector_load_func(id, &(cache_entry->sector))) {
            tig_debug_printf("Error: attempt to lock sector %I64u in cache failed due to error in load.  This is bad.  Help.\n", id);
            in_sector_lock = false;
            return false;
        }

        memmove(&(sector_cache_indexes[dword_60182C + 1]),
            &(sector_cache_indexes[dword_60182C]),
            sizeof(*sector_cache_indexes) * (sector_cache_size - dword_60182C));
        sector_cache_indexes[dword_60182C] = index;
        sector_cache_size++;

        cache_entry->used = true;
        cache_entry->refcount = 1;
        cache_entry->timestamp = dword_6017BC;
        cache_entry->sector.id = id;
    }

    *sector_ptr = &(cache_entry->sector);

    in_sector_lock = false;
    sector_refcount++;

    return true;
}

// 0x4D0AE0
bool sector_unlock(int64_t id)
{
    int index;

    if (!sector_cache_find_by_id(id, &index)) {
        return false;
    }

    sector_cache_entries[sector_cache_indexes[index]].refcount--;
    sector_refcount--;

    return true;
}

// 0x4D0B40
void sub_4D0B40(void)
{
    unsigned int index;

    for (index = 0; index < sector_cache_size; index++) {
        sub_4D1400(&(sector_cache_entries[sector_cache_indexes[index]].sector));
        sector_cache_entries[sector_cache_indexes[index]].used = false;
    }

    sector_cache_size = 0;
}

// 0x4D0BC0
void sector_flush(unsigned int flags)
{
    unsigned int index;
    SectorCacheEntry* cache_entry;

    for (index = 0; index < sector_cache_size; index++) {
        cache_entry = &(sector_cache_entries[sector_cache_indexes[index]]);
        if (cache_entry->refcount == 0) {
            sector_save_func(&(cache_entry->sector));
            if ((flags & 0x1) == 0) {
                sub_4D1400(&(cache_entry->sector));
                cache_entry->used = false;
                memmove(&(sector_cache_indexes[index]),
                    &(sector_cache_indexes[index + 1]),
                    sizeof(*sector_cache_indexes) * (sector_cache_size - 1));
                index--;
                sector_cache_size--;
            }
        }
    }

    sector_block_save_internal();
}

// 0x4D0DE0
bool sector_check_demo_limits(int64_t sec)
{
    if (!sector_demo_limits_set) {
        return true;
    }

    if (SECTOR_X(sec) > sector_demo_limits_max_x
        || SECTOR_X(sec) < sector_demo_limits_min_x
        || SECTOR_Y(sec) > sector_demo_limits_max_y
        || SECTOR_Y(sec) < sector_demo_limits_min_y) {
        return false;
    }

    return true;
}

// 0x4D0E70
void sector_art_cache_register(tig_art_id_t art_id)
{
    if (sector_art_cache_size == sector_art_cache_capacity) {
        sector_art_cache_capacity += 128;
        sector_art_cache = (tig_art_id_t*)REALLOC(sector_art_cache, sizeof(*sector_art_cache) * sector_art_cache_capacity);
    }

    sector_art_cache[sector_art_cache_size++] = art_id;
}

// 0x4D0EE0
bool sector_is_blocked(int64_t sec)
{
    return sector_block_find(sec) != -1;
}

// 0x4D0F00
void sector_block_set(int64_t sec, bool blocked)
{
    int idx;

    idx = sector_block_find(sec);
    if (blocked) {
        if (idx == -1) {
            sector_block_add(sec);
        }
    } else {
        if (idx != -1) {
            sector_block_remove(idx);
        }
    }
}

// 0x4D0F40
void sector_block_init(void)
{
    sector_block_clear();
}

// 0x4D0F50
bool sector_block_load(const char* base_map_name, const char* current_map_name)
{
    sector_block_clear();

    if (!sector_block_load_internal(base_map_name, current_map_name)) {
        return false;
    }

    return true;
}

// 0x4D0F70
void sector_block_clear(void)
{
    if (sector_blocked_sectors != NULL) {
        FREE(sector_blocked_sectors);
        sector_blocked_sectors = NULL;
    }

    sector_blocked_sectors_cnt = 0;
    sector_blocked_sectors_changed = false;
}

// 0x4D1040
void sector_art_cache_enable(void)
{
    sector_art_cache_state++;
}

// 0x4D1050
void sector_art_cache_disable(void)
{
    sector_art_cache_state--;
}

// 0x4D1060
void sector_enumerate(SectorEnumerateFunc* func)
{
    unsigned int index;

    in_sector_enumerate = true;

    for (index = 0; index < sector_cache_size; index++) {
        if (!func(&(sector_cache_entries[sector_cache_indexes[index]].sector))) {
            break;
        }
    }

    in_sector_enumerate = false;
}

// 0x4D10C0
bool sector_history_init(GameInitInfo* init_info)
{
    (void)init_info;

    sector_history_clear();

    return true;
}

// 0x4D10D0
void sector_history_reset(void)
{
    sector_history_size = 0;
}

// 0x4D10E0
void sector_history_exit(void)
{
    FREE(sector_history_entries);
    sector_history_size = 0;
}

// 0x4D1100
void sector_history_clear(void)
{
    sector_history_entries = (SectorHistoryEntry*)REALLOC(sector_history_entries, sizeof(*sector_history_entries) * sector_cache_capacity * 2);
    memset(sector_history_entries, 0, sizeof(*sector_history_entries) * sector_cache_capacity * 2);
    sector_history_size = 0;
}

// 0x4D1150
bool sector_history_save(TigFile* stream)
{
    unsigned int index;
    unsigned int k;
    SectorCacheEntry* cache_entry;

    for (index = 0; index < sector_cache_capacity; index++) {
        cache_entry = &(sector_cache_entries[index]);
        if (cache_entry->used) {
            for (k = 0; k < sector_history_size; k++) {
                if (sector_history_entries[k].id == cache_entry->sector.id) {
                    break;
                }
            }

            if (k >= sector_history_size) {
                if (sector_history_size == 2 * sector_cache_capacity) {
                    memmove(&(sector_history_entries[0]),
                        &(sector_history_entries[1]),
                        sizeof(*sector_history_entries) * (sector_history_size - 1));
                    sector_history_size--;
                }

                sector_history_entries[sector_history_size].id = cache_entry->sector.id;
                sector_history_entries[sector_history_size].datetime = sub_45A7C0();
                sector_history_size++;
            }
        }
    }

    if (tig_file_fwrite(&sector_history_size, sizeof(sector_history_size), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fwrite(sector_history_entries, sizeof(*sector_history_entries), sector_history_size, stream) != sector_history_size) {
        return false;
    }

    return true;
}

// 0x4D12B0
bool sector_history_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(&sector_history_size, sizeof(sector_history_size), 1, load_info->stream) != 1) {
        return false;
    }

    if (sector_history_size > 2 * sector_cache_capacity) {
        return false;
    }

    if (tig_file_fread(sector_history_entries, sizeof(*sector_history_entries), sector_history_size, load_info->stream) != sector_history_size) {
        return false;
    }

    return true;
}

// 0x4D1310
int sub_4D1310(int64_t a1, int64_t a2, int a3, int64_t* a4)
{
    int64_t v1;
    int cnt = 0;

    a4[cnt++] = a1;

    v1 = (a1 / a3 + 1) * a3;
    while (v1 < a2) {
        a4[cnt++] = v1;
        v1 += a3;
    }

    a4[cnt++] = a2;

    return cnt;
}

// 0x4D13A0
SectorListNode* sector_list_node_create(void)
{
    SectorListNode* node;

    if (sector_list_free_node_head == NULL) {
        sector_list_node_reserve();
    }

    node = sector_list_free_node_head;
    sector_list_free_node_head = node->next;

    node->next = NULL;

    return node;
}

// 0x4D13D0
void sector_list_node_reserve(void)
{
    int index;
    SectorListNode* node;

    for (index = 0; index < 4; index++) {
        node = (SectorListNode*)MALLOC(sizeof(*node));
        node->next = sector_list_free_node_head;
        sector_list_free_node_head = node;
    }
}

// 0x4D1400
void sub_4D1400(Sector* sector)
{
    if (sector->id != -1) {
        if (sector_history_size == 2 * sector_cache_capacity) {
            memmove(sector_history_entries,
                &(sector_history_entries[1]),
                sizeof(*sector_history_entries) * (sector_history_size - 1));
            sector_history_size--;
        }

        sector_history_entries[sector_history_size].id = sector->id;
        sector_history_entries[sector_history_size].datetime = sub_45A7C0();
        sector_history_size++;
    }

    sector->flags = SECTOR_IS_NEW;
    sector_light_list_reset(&(sector->lights));
    sector_tile_list_reset(&(sector->tiles));
    sector_roof_list_reset(&(sector->roofs));
    tile_script_list_reset(&(sector->tile_scripts));
    sector->townmap_info = 0;
    sector->aptitude_adj = 0;
    sector->light_scheme = 0;
    sector_script_list_reset(&(sector->sector_scripts));
    sector_sound_list_reset(&(sector->sounds));
    sector_block_list_reset(&(sector->blocks));
    sector_object_list_reset(&(sector->objects));
    sector->flags = 0;
    sector->id = -1;
}

// 0x4D1530
bool sector_load_editor(int64_t id, Sector* sector)
{
    bool generated = false;
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int placeholder;

    if (sector_check_demo_limits(id)) {
        strcpy(path, sector_save_path);
        strcat(path, "\\");
        SDL_ulltoa(id, &path[strlen(path)], 10);
        strcat(path, ".sec");

        if (!tig_file_exists(path, NULL)) {
            strcpy(path, sector_base_path);
            strcat(path, "\\");
            SDL_ulltoa(id, &path[strlen(path)], 10);
            strcat(path, ".sec");

            if (!tig_file_exists(path, NULL)) {
                terrain_sector_path(id, path);
                if (!tig_file_exists(path, NULL)) {
                    terrain_fill(sector);
                    generated = true;
                }
            }
        }
    } else {
        terrain_sector_path(id, path);
        if (!tig_file_exists(path, NULL)) {
            terrain_fill(sector);
            generated = true;
        }
    }

    sector_validate_editor("sector pre-load");

    if (!generated) {
        stream = tig_file_fopen(path, "rb");
        if (stream == NULL) {
            tig_debug_printf("Error opening sector file %s\n", path);
            return false;
        }

        if (!sector_light_list_load(&(sector->lights), stream, id)) {
            tig_debug_printf("Error loading lights from sector file %s\n", path);
            tig_file_fclose(stream);
            return false;
        }

        if (!sector_tile_list_load(&(sector->tiles), stream)) {
            tig_debug_printf("Error loading tiles from sector file %s\n", path);
            tig_file_fclose(stream);
            return false;
        }

        if (!sector_roof_list_load(&(sector->roofs), stream)) {
            tig_debug_printf("Error loading roofs from sector file %s\n", path);
            tig_file_fclose(stream);
            return false;
        }

        if (!objf_read(&placeholder, sizeof(placeholder), stream)
            || placeholder < 0xAA0000 || placeholder > 0xAA0004) {
            tig_debug_printf("Error loading placeholder from sector file %s\n", path);
            tig_file_fclose(stream);
            return false;
        }

        if (placeholder != 0xAA0000) {
            if (!tile_script_list_load(&(sector->tile_scripts), stream)) {
                tig_debug_printf("Error loading tile scripts from sector file %s\n", path);
                tig_file_fclose(stream);
                return false;
            }

            if (placeholder >= 0xAA0002) {
                if (!sector_script_list_load(&(sector->sector_scripts), stream)) {
                    tig_debug_printf("Error loading sector scripts from sector file %s\n", path);
                    tig_file_fclose(stream);
                    return false;
                }
            }

            if (placeholder >= 0xAA0003) {
                if (tig_file_fread(&(sector->townmap_info), sizeof(sector->townmap_info), 1, stream) != 1) {
                    tig_debug_printf("Error loading townmap info from sector file %s\n", path);
                    tig_file_fclose(stream);
                    return false;
                }

                if (tig_file_fread(&(sector->aptitude_adj), sizeof(sector->aptitude_adj), 1, stream) != 1) {
                    tig_debug_printf("Error loading aptitude adjustment from sector file %s\n", path);
                    tig_file_fclose(stream);
                    return false;
                }

                if (tig_file_fread(&(sector->light_scheme), sizeof(sector->light_scheme), 1, stream) != 1) {
                    tig_debug_printf("Error loading light scheme from sector file %s\n", path);
                    tig_file_fclose(stream);
                    return false;
                }

                sector->flags &= ~SECTOR_CHANGED_MASK;

                if (!sector_sound_list_load(&(sector->sounds), stream)) {
                    tig_debug_printf("Error loading sound list from sector file %s\n", path);
                    tig_file_fclose(stream);
                    return false;
                }
            }

            if (placeholder >= 0xAA0004) {
                if (!sector_block_list_load(&(sector->blocks), stream)) {
                    tig_debug_printf("Error loading tile blocking info from sector file %s\n", path);
                    tig_file_fclose(stream);
                    return false;
                }
            }
        }

        if (!objlist_load(&(sector->objects), stream, id)) {
            tig_debug_printf("Error loading objects from sector file %s\n", path);
            tig_file_fclose(stream);
            return false;
        }

        tig_file_fclose(stream);
    }

    sector_validate_editor("sector post-load (pre-fold)");
    objlist_fold(&(sector->objects), id, sub_45A9B0(&(sector->datetime), 3000));
    sector_light_list_fold(&(sector->lights), id, &(sector->tiles));
    sector_validate_editor("sector post-fold");

    return true;
}

// FIXME: There are many cleanup errors (mosty leaking `dif_stream`).
//
// 0x4D1A30
bool sector_load_game(int64_t id, Sector* sector)
{
    bool generated = false;
    DifferenceFileFlags dif_flags = 0;
    int64_t v2 = -1;
    char sec_path[TIG_MAX_PATH];
    char dif_path[TIG_MAX_PATH];
    TigFile* sec_stream;
    TigFile* dif_stream;
    int placeholder;

    if (sector_check_demo_limits(id)) {
        strcpy(sec_path, sector_base_path);
        strcat(sec_path, "\\");
        SDL_ulltoa(id, &sec_path[strlen(sec_path)], 10);
        strcat(sec_path, ".sec");

        if (!tig_file_exists(sec_path, NULL)) {
            terrain_sector_path(id, sec_path);
            v2 = id;

            if (!tig_file_exists(sec_path, NULL)) {
                tig_debug_printf("Sector: ERROR loading %s terrain sector while attempting to load sector %I64d\n",
                    sec_path,
                    id);
                terrain_fill(sector);
                generated = true;
            }
        }
    } else {
        tig_debug_printf("Sector %I64u is beyond the demo limits\n", id);
        terrain_sector_path(id, sec_path);
        v2 = id;

        if (!tig_file_exists(sec_path, NULL)) {
            terrain_fill(sector);
            generated = true;
        }
    }

    sector_validate_game("sector pre-load");
    if (!generated) {
        li_update();
        sec_stream = tig_file_fopen(sec_path, "rb");
        if (sec_stream == NULL) {
            tig_debug_printf("Error opening sector file %s\n", sec_path);
        }

        strcpy(dif_path, sector_save_path);
        strcat(dif_path, "\\");
        SDL_ulltoa(id, &dif_path[strlen(dif_path)], 10);
        strcat(dif_path, ".dif");

        if (tig_file_exists(dif_path, NULL)) {
            dif_stream = tig_file_fopen(dif_path, "rb");
            if (dif_stream != NULL) {
                if (tig_file_fread(&dif_flags, sizeof(dif_flags), 1, dif_stream) != 1) {
                    tig_debug_printf("Error reading flags from sector differences file %s\n", dif_path);
                    tig_file_fclose(dif_stream);
                    dif_stream = NULL;
                }
            } else {
                tig_debug_printf("Error opening sector differences file %s\n", dif_path);
            }
        } else {
            dif_stream = NULL;
        }

        li_update();
        if ((dif_flags & DIF_HAVE_LIGHT_LIST) != 0) {
            if (!sector_light_list_load_with_dif(&(sector->lights), sec_stream, dif_stream, v2)) {
                tig_debug_printf("Error loading lights with differences from files %s and %s\n", sec_path, dif_path);
                tig_file_fclose(sec_stream);
                tig_file_fclose(dif_stream);
                return false;
            }
        } else {
            if (!sector_light_list_load(&(sector->lights), sec_stream, v2)) {
                tig_debug_printf("Error loading lights from sector file %s\n", sec_path);
                tig_file_fclose(sec_stream);
                return false;
            }
        }

        li_update();
        if (!sector_tile_list_load(&(sector->tiles), sec_stream)) {
            tig_debug_printf("Error loading tiles from sector file %s\n", sec_path);
            tig_file_fclose(sec_stream);
            if (dif_stream != NULL) {
                tig_file_fclose(dif_stream);
            }
            return false;
        }
        if ((dif_flags & DIF_HAVE_TILE_LIST) != 0) {
            if (!sector_tile_list_load_with_dif(&(sector->tiles), dif_stream)) {
                tig_debug_printf("Error loading tiles from differences file %s\n", dif_path);
                tig_file_fclose(sec_stream);
                tig_file_fclose(dif_stream);
                return false;
            }
        }

        li_update();
        if (!sector_roof_list_load(&(sector->roofs), sec_stream)) {
            tig_debug_printf("Error loading roofs from sector file %s\n", sec_path);
            tig_file_fclose(sec_stream);
            if (dif_stream != NULL) {
                tig_file_fclose(dif_stream);
            }
            return false;
        }
        if ((dif_flags & DIF_HAVE_ROOF_LIST) != 0) {
            if (!sector_roof_list_load_with_dif(&(sector->roofs), dif_stream)) {
                tig_debug_printf("Error loading roofs from differences file %s\n", dif_path);
                tig_file_fclose(sec_stream);
                tig_file_fclose(dif_stream);
                return false;
            }
        }

        li_update();
        if (!objf_read(&placeholder, sizeof(placeholder), sec_stream)
            || placeholder < 0xAA0000 || placeholder > 0xAA0004) {
            tig_debug_printf("Error loading placeholder from sector file %s\n", sec_path);
            tig_file_fclose(sec_stream);
            if (dif_stream != NULL) {
                tig_file_fclose(dif_stream);
            }
            return false;
        }

        if (placeholder != 0xAA0000) {
            li_update();
            if (!tile_script_list_load(&(sector->tile_scripts), sec_stream)) {
                tig_debug_printf("Error loading tile scripts from sector file %s\n", sec_path);
                tig_file_fclose(sec_stream);
                return false;
            }
            if ((dif_flags & DIF_HAVE_TILE_SCRIPT_LIST) != 0) {
                if (!tile_script_list_load_with_dif(&(sector->tile_scripts), dif_stream)) {
                    tig_debug_printf("Error loading tile scripts from differences file %s\n", dif_path);
                    tig_file_fclose(sec_stream);
                    return false;
                }
            }

            if (placeholder >= 0xAA0002) {
                li_update();
                if (!sector_script_list_load(&(sector->sector_scripts), sec_stream)) {
                    tig_debug_printf("Error loading sector scripts from sector file %s\n", sec_path);
                    tig_file_fclose(sec_stream);
                    return false;
                }
                if ((dif_flags & DIF_HAVE_SECTOR_SCRIPT_LIST) != 0) {
                    if (!sector_script_list_load_with_dif(&(sector->sector_scripts), dif_stream)) {
                        tig_debug_printf("Error loading sector scripts from differences file %s\n", dif_path);
                        tig_file_fclose(sec_stream);
                        return false;
                    }
                }
            }

            if (placeholder >= 0xAA0003) {
                li_update();
                if (tig_file_fread(&(sector->townmap_info), sizeof(sector->townmap_info), 1, sec_stream) != 1) {
                    tig_debug_printf("Error loading townmap info from sector file %s\n", sec_path);
                    tig_file_fclose(sec_stream);
                    return false;
                }
                if ((dif_flags & DIF_HAVE_TOWNMAP) != 0) {
                    if (tig_file_fread(&(sector->townmap_info), sizeof(sector->townmap_info), 1, dif_stream) != 1) {
                        // FIXME: Wrong message.
                        tig_debug_printf("Error loading sound list from differences file %s\n", dif_path);
                        tig_file_fclose(sec_stream);
                        return false;
                    }
                }

                li_update();
                if (tig_file_fread(&(sector->aptitude_adj), sizeof(sector->aptitude_adj), 1, sec_stream) != 1) {
                    tig_debug_printf("Error loading aptitude adjustment from sector file %s\n", sec_path);
                    tig_file_fclose(sec_stream);
                    return false;
                }
                if ((dif_flags & DIF_HAVE_APTITUDE_ADJ) != 0) {
                    if (tig_file_fread(&(sector->aptitude_adj), sizeof(sector->aptitude_adj), 1, dif_stream) != 1) {
                        tig_debug_printf("Error loading aptitude adjustment from differences file %s\n", dif_path);
                        tig_file_fclose(sec_stream);
                        return false;
                    }
                }

                li_update();
                if (tig_file_fread(&(sector->light_scheme), sizeof(sector->light_scheme), 1, sec_stream) != 1) {
                    tig_debug_printf("Error loading light scheme from sector file %s\n", sec_path);
                    tig_file_fclose(sec_stream);
                    return false;
                }
                if ((dif_flags & DIF_HAVE_LIGHT_SCHEME) != 0) {
                    if (tig_file_fread(&(sector->light_scheme), sizeof(sector->light_scheme), 1, dif_stream) != 1) {
                        tig_debug_printf("Error loading light scheme from differences file %s\n", dif_path);
                        tig_file_fclose(sec_stream);
                        return false;
                    }
                }

                li_update();
                if (!sector_sound_list_load(&(sector->sounds), sec_stream)) {
                    tig_debug_printf("Error loading sound list from sector file %s\n", sec_path);
                    tig_file_fclose(sec_stream);
                    return false;
                }
                if ((dif_flags & DIF_HAVE_SOUND_LIST) != 0) {
                    if (!sector_sound_list_load_with_dif(&(sector->sounds), dif_stream)) {
                        tig_debug_printf("Error loading sound list from differences file %s\n", dif_path);
                        tig_file_fclose(sec_stream);
                        return false;
                    }
                }

                sector->flags &= ~SECTOR_CHANGED_MASK;
            }

            if (placeholder >= 0xAA0004) {
                li_update();
                if (!sector_block_list_load(&(sector->blocks), sec_stream)) {
                    tig_debug_printf("Error loading tile blocking info from sector file %s\n", sec_path);
                    tig_file_fclose(sec_stream);
                    return false;
                }
                if ((dif_flags & DIF_HAVE_BLOCK_LIST) != 0) {
                    if (!sector_block_list_load_with_dif(&(sector->blocks), dif_stream)) {
                        tig_debug_printf("Error loading tile blocking info from differences file %s\n", dif_path);
                        tig_file_fclose(sec_stream);
                        return false;
                    }
                }
            }
        }

        li_update();
        if ((dif_flags & DIF_HAVE_OBJ_LIST) != 0) {
            if (!objlist_load_with_difs(&(sector->objects), sec_stream, dif_stream, v2)) {
                tig_debug_printf("Error loading objects with differences from files %s and %s\n", sec_path, dif_path);
                tig_file_fclose(sec_stream);
                tig_file_fclose(dif_stream);
                return false;
            }
        } else {
            if (!objlist_load(&(sector->objects), sec_stream, v2)) {
                tig_debug_printf("Error loading objects from sector file %s\n", sec_path);
                tig_file_fclose(sec_stream);
                return false;
            }
        }

        tig_file_fclose(sec_stream);
        if (dif_flags != 0) {
            tig_file_fclose(dif_stream);
        }
    }

    sector_validate_game("sector post-load (pre-fold)");
    li_update();
    objlist_fold(&(sector->objects), id, sub_45A9B0(&(sector->datetime), 3000));
    li_update();
    sector_light_list_fold(&(sector->lights), id, &(sector->tiles));
    sector_validate_game("sector post-fold");

    if (sector->townmap_info != 0) {
        sector_precache_art(sector);
    }

    return true;
}

// 0x4D22E0
bool sector_save_editor(Sector* sector)
{
    DifferenceFileFlags dif_flags = 0;
    char path[TIG_MAX_PATH];

    if (sector_light_list_is_modified(&(sector->lights))) {
        dif_flags |= DIF_HAVE_LIGHT_LIST;
    }

    if (sector->tiles.dif) {
        dif_flags |= DIF_HAVE_TILE_LIST;
    }

    if (sector_roof_list_is_modified(&(sector->roofs))) {
        dif_flags |= DIF_HAVE_ROOF_LIST;
    }

    if (tile_script_list_is_modified(&(sector->tile_scripts))) {
        dif_flags |= DIF_HAVE_TILE_SCRIPT_LIST;
    }

    if (sector_script_list_is_modified(&(sector->sector_scripts))) {
        dif_flags |= DIF_HAVE_SECTOR_SCRIPT_LIST;
    }

    if ((sector->flags & SECTOR_TOWNMAP_CHANGED) != 0) {
        dif_flags |= DIF_HAVE_TOWNMAP;
    }

    if ((sector->flags & SECTOR_APTITUDE_ADJ_CHANGED) != 0) {
        dif_flags |= DIF_HAVE_APTITUDE_ADJ;
    }

    if ((sector->flags & SECTOR_LIGHT_SCHEME_CHANGED) != 0) {
        dif_flags |= DIF_HAVE_LIGHT_SCHEME;
    }

    if (sector_sound_list_is_modified(&(sector->sounds))) {
        dif_flags |= DIF_HAVE_SOUND_LIST;
    }

    if (sector->blocks.modified) {
        dif_flags |= DIF_HAVE_BLOCK_LIST;
    }

    if (objlist_is_modified(&(sector->objects))) {
        dif_flags |= DIF_HAVE_OBJ_LIST;
    }

    if (dif_flags == 0) {
        return true;
    }

    strcpy(path, sector_save_path);
    strcat(path, "\\");
    SDL_ulltoa(sector->id, &(path[strlen(path)]), 10);

    return sub_4D2460(sector, path);
}

// 0x4D2460
bool sub_4D2460(Sector* sector, const char* base_path)
{
    char fname[TIG_MAX_PATH];
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int placeholder;

    if (!sector_check_demo_limits(sector->id)) {
        return true;
    }

    if (dword_601820 != NULL) {
        compat_splitpath(base_path, NULL, NULL, fname, NULL);
        sprintf(path, "%s_sec.loc", fname);
        if (!dword_601820(path)) {
            tig_debug_println("Trying to save a sector that we don't have write locked.");
            return false;
        }
    }

    sprintf(path, "%s.sec", base_path);

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("Error creating sector data file %s\n", path);
        tig_file_fclose(stream); // FIXME: Crash! (dereferencing NULL).
        return false;
    }

    sector_validate_editor("sector pre-save");

    if (!sector_light_list_save(&(sector->lights), stream)) {
        tig_debug_printf("Error saving lights to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (!sector_tile_list_save(&(sector->tiles), stream)) {
        tig_debug_printf("Error saving tiles to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (!sector_roof_list_save(&(sector->roofs), stream)) {
        tig_debug_printf("Error saving roofs to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    placeholder = 0xAA0004;
    if (!objf_write(&placeholder, sizeof(placeholder), stream)) {
        tig_debug_printf("Error saving placeholder to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (!tile_script_list_save(&(sector->tile_scripts), stream)) {
        tig_debug_printf("Error saving tile scripts to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (!sector_script_list_save(&(sector->sector_scripts), stream)) {
        tig_debug_printf("Error saving sector scripts to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (tig_file_fwrite(&(sector->townmap_info), sizeof(sector->townmap_info), 1, stream) != 1) {
        tig_debug_printf("Error saving townmap info to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (tig_file_fwrite(&(sector->aptitude_adj), sizeof(sector->townmap_info), 1, stream) != 1) {
        tig_debug_printf("Error saving aptitude adjustment to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (tig_file_fwrite(&(sector->light_scheme), sizeof(sector->townmap_info), 1, stream) != 1) {
        tig_debug_printf("Error saving light scheme to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    sector->flags &= ~SECTOR_CHANGED_MASK;

    if (!sector_sound_list_save(&(sector->sounds), stream)) {
        tig_debug_printf("Error saving sound list to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (!sector_block_list_save(&(sector->blocks), stream)) {
        tig_debug_printf("Error saving tile blocking info to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if (!objlist_save(&(sector->objects), stream)) {
        tig_debug_printf("Error saving objects to sector data file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    sector_validate_editor("sector post-save");
    tig_file_fclose(stream);

    return true;
}

// 0x4D2750
bool sector_save_game(Sector* sector)
{
    DifferenceFileFlags dif_flags;
    char path[TIG_MAX_PATH];
    TigFile* stream;

    if (!sector_check_demo_limits(sector->id)) {
        return true;
    }

    dif_flags = 0;

    if (sector_light_list_is_modified(&(sector->lights))) {
        dif_flags |= DIF_HAVE_LIGHT_LIST;
    }

    if (sector->tiles.dif) {
        dif_flags |= DIF_HAVE_TILE_LIST;
    }

    if (sector_roof_list_is_modified(&(sector->roofs))) {
        dif_flags |= DIF_HAVE_ROOF_LIST;
    }

    if (tile_script_list_is_modified(&(sector->tile_scripts))) {
        dif_flags |= DIF_HAVE_TILE_SCRIPT_LIST;
    }

    if (sector_script_list_is_modified(&(sector->sector_scripts))) {
        dif_flags |= DIF_HAVE_SECTOR_SCRIPT_LIST;
    }

    if ((sector->flags & SECTOR_TOWNMAP_CHANGED) != 0) {
        dif_flags |= DIF_HAVE_TOWNMAP;
    }

    if ((sector->flags & SECTOR_APTITUDE_ADJ_CHANGED) != 0) {
        dif_flags |= DIF_HAVE_APTITUDE_ADJ;
    }

    if ((sector->flags & SECTOR_LIGHT_SCHEME_CHANGED) != 0) {
        dif_flags |= DIF_HAVE_LIGHT_SCHEME;
    }

    if (sector_sound_list_is_modified(&(sector->sounds))) {
        dif_flags |= DIF_HAVE_SOUND_LIST;
    }

    if (sector->blocks.modified) {
        dif_flags |= DIF_HAVE_BLOCK_LIST;
    }

    if (objlist_is_modified(&(sector->objects))) {
        dif_flags |= DIF_HAVE_OBJ_LIST;
    }

    if (dif_flags == 0) {
        return true;
    }

    strcpy(path, sector_save_path);
    strcat(path, "\\");
    SDL_ulltoa(sector->id, &(path[strlen(path)]), 10);
    strcat(path, ".dif");

    stream = tig_file_fopen(path, "wb");
    if (stream == NULL) {
        tig_debug_printf("Error creating differences file %s\n", path);
        tig_file_fclose(stream); // FIXME: Crash!
        return false;
    }

    if (tig_file_fwrite(&dif_flags, sizeof(dif_flags), 1, stream) != 1) {
        tig_debug_printf("Error writing flags to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    sector_validate_game("sector pre-save");

    if ((dif_flags & DIF_HAVE_LIGHT_LIST) != 0
        && !sector_light_list_save_with_dif(&(sector->lights), stream)) {
        tig_debug_printf("Error saving lights to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_TILE_LIST) != 0
        && !sector_tile_list_save_with_dif(&(sector->tiles), stream)) {
        tig_debug_printf("Error saving tiles to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_ROOF_LIST) != 0
        && !sector_roof_list_save_with_dif(&(sector->roofs), stream)) {
        tig_debug_printf("Error saving roofs to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_TILE_SCRIPT_LIST) != 0
        && !tile_script_list_save_with_dif(&(sector->tile_scripts), stream)) {
        tig_debug_printf("Error saving tile scripts to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_SECTOR_SCRIPT_LIST) != 0
        && !sector_script_list_save_with_dif(&(sector->sector_scripts), stream)) {
        tig_debug_printf("Error saving sector scripts to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_TOWNMAP) != 0
        && tig_file_fwrite(&(sector->townmap_info), sizeof(sector->townmap_info), 1, stream) != 1) {
        tig_debug_printf("Error saving townmap info to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_APTITUDE_ADJ) != 0
        && tig_file_fwrite(&(sector->aptitude_adj), sizeof(sector->aptitude_adj), 1, stream) != 1) {
        tig_debug_printf("Error saving aptitude adjustment to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_LIGHT_SCHEME) != 0
        && tig_file_fwrite(&(sector->light_scheme), sizeof(sector->light_scheme), 1, stream) != 1) {
        tig_debug_printf("Error saving light scheme to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_SOUND_LIST) != 0
        && !sector_sound_list_save_with_dif(&(sector->sounds), stream)) {
        tig_debug_printf("Error saving sound list to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_BLOCK_LIST) != 0
        && !sector_block_list_save_with_dif(&(sector->blocks), stream)) {
        tig_debug_printf("Error saving sound list to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    if ((dif_flags & DIF_HAVE_OBJ_LIST) != 0
        && !objlist_save_with_dif(&(sector->objects), stream)) {
        tig_debug_printf("Error saving objects to differences file %s\n", path);
        tig_file_fclose(stream);
        tig_file_remove(path);
        return false;
    }

    tig_file_fclose(stream);
    sector_validate_game("sector post-save");

    return true;
}

// 0x4D2C00
void sector_validate_game(const char* section)
{
    if (!obj_validate_system(1)) {
        tig_debug_printf("Object system validate failed in %s\n", section);
        tig_message_post_quit(0);
    }
}

// 0x4D2C30
void sector_validate_editor(const char* section)
{
    char str[300];

    if (!obj_validate_system(1)) {
        snprintf(str, sizeof(str) - 1, "Object system validate failed in %s", section);
        str[sizeof(str) - 1] = '\0';
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Execution Halting", str, NULL);
        tig_message_post_quit(0);
    }
}

// 0x4D2CA0
bool sector_cache_find_by_id(int64_t id, int* index_ptr)
{
    int l;
    int r;
    int m;
    int64_t curr;

    l = 0;
    r = sector_cache_size - 1;
    while (l <= r) {
        m = (l + r) / 2;
        curr = sector_cache_entries[sector_cache_indexes[m]].sector.id;

        if (curr < id) {
            l = m + 1;
        } else if (curr > id) {
            r = m - 1;
        } else {
            *index_ptr = m;
            return true;
        }
    }

    *index_ptr = l;
    return false;
}

// 0x4D2D30
bool sector_cache_find_unused(unsigned int* index_ptr)
{
    unsigned int index;

    // NOTE: Unsigned comparison.
    for (index = 0; index < sector_cache_capacity; index++) {
        if (!sector_cache_entries[index].used) {
            *index_ptr = index;
            return true;
        }
    }

    return false;
}

// 0x4D2D70
void sector_load_demo_limits(void)
{
    char buffer[TIG_MAX_PATH];
    TigFile* stream;

    sector_demo_limits_set = false;

    // Sector with coordinates (2000,2000) + 1 is a special file which defines
    // demo limits.
    sprintf(buffer, "%s\\134217730001.sec", sector_base_path);
    if (!tig_file_exists(buffer, NULL)) {
        return;
    }

    stream = tig_file_fopen(buffer, "rb");
    if (stream == NULL) {
        return;
    }

    do {
        if (tig_file_fgets(buffer, sizeof(buffer), stream) == NULL) break;
        sector_demo_limits_min_x = SDL_strtoll(buffer, NULL, 10);

        if (tig_file_fgets(buffer, sizeof(buffer), stream) == NULL) break;
        sector_demo_limits_max_x = SDL_strtoll(buffer, NULL, 10);

        if (tig_file_fgets(buffer, sizeof(buffer), stream) == NULL) break;
        sector_demo_limits_min_y = SDL_strtoll(buffer, NULL, 10);

        if (tig_file_fgets(buffer, sizeof(buffer), stream) == NULL) break;
        sector_demo_limits_max_y = SDL_strtoll(buffer, NULL, 10);

        sector_demo_limits_set = true;
    } while (0);

    tig_file_fclose(stream);
}

// 0x4D2EA0
void sector_art_cache_clear(void)
{
    if (sector_art_cache != NULL) {
        FREE(sector_art_cache);
    }

    sector_art_cache_size = 0;
    sector_art_cache_capacity = 0;
}

// 0x4D2ED0
void sector_precache_art(Sector* sector)
{
    int index;

    if (sector_art_cache_state > 0) {
        li_update();
        sector_list_tile_precache_art(&(sector->tiles));
        li_update();
        sector_roof_list_precache_art(&(sector->roofs));
        li_update();
        objlist_precache_art(&(sector->objects));
        li_update();
        sector_art_cache_sort();
        li_update();

        for (index = 0; index < sector_art_cache_size; index++) {
            while (index + 1 < sector_art_cache_size && sector_art_cache[index] == sector_art_cache[index + 1]) {
                index++;
            }
            tig_art_touch(sector_art_cache[index]);
            li_update();
        }

        sector_art_cache_reset();
    }
}

// 0x4D2F80
void sector_art_cache_reset(void)
{
    sector_art_cache_size = 0;
}

// 0x4D2F90
void sector_art_cache_sort(void)
{
    qsort(sector_art_cache, sector_art_cache_size, sizeof(*sector_art_cache), sector_art_cache_compare);
}

// 0x4D2FB0
int sector_art_cache_compare(const void* va, const void* vb)
{
    const tig_art_id_t* a = (const tig_art_id_t*)va;
    const tig_art_id_t* b = (const tig_art_id_t*)vb;

    return *a - *b;
}

// 0x4D2FC0
int sector_block_find(int64_t sec)
{
    int index;

    for (index = 0; index < sector_blocked_sectors_cnt; index++) {
        if (sector_blocked_sectors[index] == sec) {
            return index;
        }
    }

    return -1;
}

// 0x4D3000
void sector_block_add(int64_t sec)
{
    sector_blocked_sectors = (int64_t*)REALLOC(sector_blocked_sectors, sizeof(*sector_blocked_sectors) * (sector_blocked_sectors_cnt + 1));
    sector_blocked_sectors[sector_blocked_sectors_cnt++] = sec;
    sector_blocked_sectors_changed = true;
}

// 0x4D3050
void sector_block_remove(int idx)
{
    memmove(&(sector_blocked_sectors[idx]),
        &(sector_blocked_sectors[idx + 1]),
        sizeof(*sector_blocked_sectors) * (sector_blocked_sectors_cnt - idx - 1));
    sector_blocked_sectors_cnt--;
    sector_blocked_sectors_changed = true;
}

// 0x4D30A0
bool sector_block_save_internal(void)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;

    if (sector_blocked_sectors_changed) {
        if (sector_blocked_sectors_cnt != 0) {
            sprintf(path, "%s\\map.sbf", sector_save_path);

            stream = tig_file_fopen(path, "wb");
            if (stream == NULL) {
                return false;
            }

            if (tig_file_fwrite(&sector_blocked_sectors_cnt, sizeof(sector_blocked_sectors_cnt), 1, stream) != 1) {
                tig_file_fclose(stream);
                return false;
            }

            if (tig_file_fwrite(sector_blocked_sectors, sizeof(*sector_blocked_sectors), sector_blocked_sectors_cnt, stream) != sector_blocked_sectors_cnt) {
                tig_file_fclose(stream);
                return false;
            }

            tig_file_fclose(stream);
        } else {
            sprintf(path, "%s\\null.sbf", sector_save_path);

            stream = tig_file_fopen(path, "wb");
            if (stream == NULL) {
                return false;
            }

            tig_file_fputc(1, stream);
            tig_file_fclose(stream);
        }

        sector_blocked_sectors_changed = false;
    }

    return true;
}

// 0x4D31C0
bool sector_block_load_internal(const char* base_map_name, const char* current_map_name)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;

    sprintf(path, "%s\\map.sbf", current_map_name);

    if (!tig_file_exists(path, NULL)) {
        sprintf(path, "%s\\map.sbf", base_map_name);
        if (!tig_file_exists(path, NULL)) {
            return false;
        }
    }

    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }

    if (tig_file_fread(&sector_blocked_sectors_cnt, sizeof(sector_blocked_sectors_cnt), 1, stream) != 1) {
        tig_file_fclose(stream);
        return false;
    }

    sector_blocked_sectors = (int64_t*)MALLOC(sizeof(*sector_blocked_sectors) * sector_blocked_sectors_cnt);
    if (tig_file_fread(sector_blocked_sectors, sizeof(*sector_blocked_sectors), sector_blocked_sectors_cnt, stream) != sector_blocked_sectors_cnt) {
        tig_file_fclose(stream);
        return false;
    }

    tig_file_fclose(stream);
    return true;
}
