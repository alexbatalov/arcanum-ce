#include "game/townmap.h"

#include <stdio.h>

#include "game/mes.h"
#include "game/sector.h"

typedef struct TownMapIndexEntry {
    int map;
    bool w;
} TownMapIndexEntry;

static void townmap_kstate_reset(void);
static void townmap_set_known_internal(int map, bool known);
static void townmap_tile_known_set(int map, int index);
static bool townmap_tile_known_get(int map, int index);
static bool townmap_kstate_load(int map);
static void townmap_kstate_init(int map);
static bool townmap_kstate_flush(void);

/**
 * "townmap.mes"
 *
 * 0x5B6450
 */
static mes_file_handle_t townmap_mes_file = MES_FILE_HANDLE_INVALID;

/**
 * Bounds of a single town map tile (in it's own coordinate system).
 *
 * 0x5FC4C8
 */
static TigRect townmap_tile_bounds;

/**
 * Last town map info read.
 *
 * 0x5FC4D8
 */
static TownMapInfo townmap_info_cache;

/**
 * 0x5FC508
 */
static TownMapIndexEntry* townmap_entries;

/**
 * A flag indicating that k-state array has been modified and needs to be saved.
 *
 * 0x5FC50C
 */
static bool townmap_kstate_modified;

/**
 * Bit array storing the known state of tiles.
 *
 * 0x5FC510
 */
static uint8_t* townmap_kstate;

/**
 * The size of `townmap_kstate` in bytes.
 *
 * 0x5FC514
 */
static int townmap_kstate_size;

/**
 * The town map ID whose k-state is currently loaded.
 *
 * 0x5FC518
 */
static int townmap_kstate_map;

/**
 * The number of entries in `townmap_entries` array.
 *
 * 0x5FC51C
 */
static int townmap_entries_cnt;

/**
 * Called when the game is being reset.
 *
 * 0x4BE1B0
 */
void townmap_reset(void)
{
    // Reset known tiles.
    townmap_kstate_reset();
}

/**
 * Called when a module is being loaded.
 *
 * 0x4BE1C0
 */
bool townmap_mod_load(void)
{
    MesFileEntry mes_file_entry;
    int index;
    char* pch;
    char* back;

    // Load the "townmap.mes" file (might be absent).
    if (!mes_load("rules\\townmap.mes", &townmap_mes_file)) {
        townmap_mes_file = MES_FILE_HANDLE_INVALID;
        return true;
    }

    // Allocate memory for townmap index based on the number of records in the
    // file.
    townmap_entries_cnt = mes_num_entries(townmap_mes_file);
    townmap_entries = (TownMapIndexEntry*)CALLOC(townmap_entries_cnt, sizeof(*townmap_entries));

    // Parse each entry the message file and populate the townmap index.
    if (mes_find_first(townmap_mes_file, &mes_file_entry)) {
        index = 0;
        do {
            townmap_entries[index].map = mes_file_entry.num;

            // Search for the "waitable" flag in the townmap name.
            pch = strstr(mes_file_entry.str, "[w:");
            if (pch == NULL) {
                pch = strstr(mes_file_entry.str, "[W:");
            }

            if (pch != NULL) {
                // Clean-up townmap name by null-terminate it before the flag
                // and remove any trailing whitespace.
                back = pch;
                do {
                    *back-- = '\0';
                } while (SDL_isspace(*back));

                // Jump to the value of the flag.
                pch += 3;
                while (SDL_isspace(*pch)) {
                    pch++;
                }

                if (*pch == '1') {
                    townmap_entries[index].w = true;
                }
            }
            index++;
        } while (mes_find_next(townmap_mes_file, &mes_file_entry));
    }

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4BE310
 */
void townmap_mod_unload(void)
{
    // Reset known tiles array.
    townmap_kstate_reset();

    // Free townmap index.
    if (townmap_entries != NULL) {
        FREE(townmap_entries);
        townmap_entries = NULL;
    }

    townmap_entries_cnt = 0;

    // Unload "townmap.mes".
    if (townmap_mes_file != MES_FILE_HANDLE_INVALID) {
        mes_unload(townmap_mes_file);
        townmap_mes_file = MES_FILE_HANDLE_INVALID;
    }

    // Clear cached town map info.
    memset(&townmap_info_cache, 0, sizeof(townmap_info_cache));
}

/**
 * Called when the map is being flushed.
 *
 * 0x4BE370
 */
void townmap_flush(void)
{
    townmap_kstate_flush();
    townmap_kstate_reset();
}

/**
 * Retrieves the town map ID associated with a sector.
 *
 * 0x4BE380
 */
int townmap_get(int64_t sec)
{
    Sector* sector;
    int num;

    if (!sector_lock(sec, &sector)) {
        return TOWNMAP_NONE;
    }

    num = sector->townmap_info;
    sector_unlock(sec);

    return num;
}

/**
 * Associates a town map ID with a sector.
 *
 * 0x4BE3C0
 */
void townmap_set(int64_t sec, int townmap)
{
    Sector* sector;

    if (!sector_lock(sec, &sector)) {
        return;
    }

    // Set the town map ID and mark the sector as modified.
    sector->townmap_info = townmap;
    sector->flags |= SECTOR_TOWNMAP_CHANGED;

    sector_unlock(sec);
}

/**
 * Returns the total number of town maps defined in the "townmap.mes" file.
 *
 * 0x4BE400
 */
int townmap_count(void)
{
    if (townmap_mes_file == MES_FILE_HANDLE_INVALID) {
        return 0;
    }

    // NOTE: Why not `townmap_entries_cnt`?
    return mes_num_entries(townmap_mes_file);
}

/**
 * Retrieves the name of a town map.
 *
 * 0x4BE420
 */
const char* townmap_name(int map)
{
    MesFileEntry mes_file_entry;

    if (townmap_mes_file == MES_FILE_HANDLE_INVALID) {
        return NULL;
    }

    // Search for the map entry in the mes file.
    mes_file_entry.num = map;
    if (!mes_search(townmap_mes_file, &mes_file_entry)) {
        return NULL;
    }

    return mes_file_entry.str;
}

/**
 * Retrieves detailed information about a town map.
 *
 * 0x4BE4E0
 */
bool townmap_info(int map, TownMapInfo* tmi)
{
    const char* name;
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int version;

    // Ensure town map is valid.
    if (map == TOWNMAP_NONE) {
        return false;
    }

    // Check the cache.
    if (map == townmap_info_cache.map) {
        *tmi = townmap_info_cache;
        return true;
    }

    // Get the map name to build the file path.
    name = townmap_name(map);
    if (name == NULL) {
        return false;
    }

    sprintf(path, "townmap\\%s\\%s.tmi", name, name);

    // Open the ".tmi" file for reading.
    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }

    // Read and verify the file version.
    if (tig_file_fread(&version, sizeof(version), 1, stream) != 1 || version != 2) {
        tig_file_fclose(stream);
        return false;
    }

    // Rewind the file and read "TownMapInfo" struct in one go.
    if (tig_file_fseek(stream, -(int)(sizeof(version)), SEEK_CUR) != 0
        || tig_file_fread(tmi, sizeof(*tmi), 1, stream) != 1) {
        tig_file_fclose(stream);
        return false;
    }

    tig_file_fclose(stream);

    // Calculate a single tile bounds for rendering.
    townmap_tile_bounds.x = 0;
    townmap_tile_bounds.y = 0;
    townmap_tile_bounds.width = (int)(tmi->width / tmi->num_hor_tiles * tmi->scale);
    townmap_tile_bounds.height = (int)(tmi->height / tmi->num_vert_tiles * tmi->scale);

    // Cache the result to speed up future queries.
    townmap_info_cache = *tmi;

    return true;
}

/**
 * Converts a game location to town map coordinates.
 *
 * 0x4BE670
 */
void townmap_loc_to_coords(TownMapInfo* tmi, int64_t loc, int* x_ptr, int* y_ptr)
{
    int64_t center_x;
    int64_t center_y;
    int64_t x;
    int64_t y;

    *x_ptr = 0;
    *y_ptr = 0;

    // Get the map's center and the location's screen coordinates.
    location_xy(tmi->loc, &center_x, &center_y);
    location_xy(loc, &x, &y);

    // Adjust coordinates relative to the difference between geometric center
    // of the town map canvas and it's actual screen coordinates.
    x += tmi->width / 2 - center_x;
    y += tmi->height / 2 - center_y;

    // Check if coordinates are within the map bounds.
    if (x >= 0 && x < tmi->width
        && y >= 0 && y < tmi->height) {
        // Scale from screen coordinate system to town map coordinate system.
        *x_ptr = (int)(x * tmi->scale);
        *y_ptr = (int)(y * tmi->scale);
    }
}

/**
 * Converts town map coordinates to a game location.
 *
 * 0x4BE780
 */
void townmap_coords_to_loc(TownMapInfo* tmi, int x, int y, int64_t* loc_ptr)
{
    int64_t offset_x;
    int64_t offset_y;
    int64_t center_x;
    int64_t center_y;
    int64_t loc;

    *loc_ptr = 0;

    // Scale from town map coordinate system to screen coordinate system and
    // calculate this position as offset from the geometric center of the town
    // map canvas.
    offset_x = (int)(x / tmi->scale) - tmi->width / 2;
    offset_y = (int)(y / tmi->scale) - tmi->height / 2;

    // Get the map's center screen coordinates and compute the final location.
    location_xy(tmi->loc, &center_x, &center_y);
    location_at(center_x + offset_x, center_y + offset_y, &loc);
    *loc_ptr = loc;
}

/**
 * Marks a location as visited on a town map.
 *
 * 0x4BE8F0
 */
bool townmap_mark_visited(int64_t loc)
{
    int64_t sec;
    Sector* sector;
    TownMapInfo tmi;
    int64_t center_x;
    int64_t center_y;
    int64_t x;
    int64_t y;
    int row;
    int col;

    sec = sector_id_from_loc(loc);

    // Ensure the sector exists.
    if (!sector_exists(sec)) {
        return false;
    }

    if (!sector_lock(sec, &sector)) {
        return false;
    }

    // Check if the sector has associated town map.
    if (sector->townmap_info == 0) {
        sector_unlock(sec);
        return false;
    }

    if (townmap_info(sector->townmap_info, &tmi)) {
        // Get the map's center and the location's screen coordinates.
        location_xy(tmi.loc, &center_x, &center_y);
        location_xy(loc, &x, &y);

        // Adjust coordinates relative to the difference between geometric
        // center of the town map canvas and it's actual screen coordinates.
        x += tmi.width / 2 - center_x;
        y += tmi.height / 2 - center_y;

        // Check if coordinates are within the map bounds.
        if (x >= 0 && x < tmi.width
            && y >= 0 && y < tmi.height) {
            // Calculate col/row of the tile, which corresponds to x/y
            // coordinates.
            col = (int)(tmi.num_hor_tiles * x / tmi.width);
            row = (int)(tmi.num_vert_tiles * y / tmi.height);

            townmap_tile_known_set(sector->townmap_info,
                col + tmi.num_hor_tiles * row);
        }
    }

    sector_unlock(sec);
    return true;
}

/**
 * Sets the known state for all tiles in a town map.
 *
 * 0x4BEAB0
 */
bool townmap_set_known(int map, bool known)
{
    TownMapInfo tmi;

    // Ensure town map is valid.
    if (map == TOWNMAP_NONE) {
        return false;
    }

    // Retrieve map info to validate and cache the town map.
    if (!townmap_info(map, &tmi)) {
        return false;
    }

    townmap_set_known_internal(map, known);

    return true;
}

/**
 * Retrieves blit information for a town map tile, used for rendering.
 *
 * 0x4BEAF0
 */
bool townmap_tile_blit_info(int map, int index, TigVideoBufferBlitInfo* vb_blit_info)
{
    uint8_t available_flags[3][3] = {
        { 0x8, 0x8 | 0x4, 0x4 },
        { 0x8 | 0x1, 0, 0x4 | 0x2 },
        { 0x1, 0x2 | 0x1, 0x2 },
    };

    unsigned int flags;
    TownMapInfo tmi;
    tig_color_t color;
    int base_col;
    int base_row;
    int dy;
    int dx;
    int row;
    int col;

    vb_blit_info->flags = 0;

    // Ensure town map is valid.
    if (map == TOWNMAP_NONE) {
        return false;
    }

    // Retrieve town map info.
    if (!townmap_info(map, &tmi)) {
        return false;
    }

    // If the tile is known, no further processing is needed (blit flags are
    // already set to 0, which means one-to-one copy).
    if (townmap_tile_known_get(map, index)) {
        return true;
    }

    flags = 0;

    // Calculate the tile's row and column and offset it by -1. This would be
    // the upper left corner of 3x3 block of tiles for subsequent checks.
    base_col = index % tmi.num_hor_tiles - 1;
    base_row = index / tmi.num_hor_tiles - 1;

    // Iterate through adjacent tiles.
    for (dy = 0; dy < 3; dy++) {
        for (dx = 0; dx < 3; dx++) {
            // Calculate adjacent tile row and column.
            row = base_row + dy;
            col = base_col + dx;

            // Ensure it is within the bounds of the town map.
            if (col >= 0 && col < tmi.num_hor_tiles
                && row >= 0 && row < tmi.num_vert_tiles) {
                // If the adjacent tile is known, add appropriate flags to the
                // bit mask.
                if (townmap_tile_known_get(map, col + tmi.num_hor_tiles * row)) {
                    flags |= available_flags[dy][dx];
                }
            }
        }
    }

    // If no adjacent tiles are known, the tile should not be rendered at all.
    if (flags == 0) {
        return false;
    }

    color = tig_color_make(255, 255, 255);

    // Specify blending properties.
    vb_blit_info->flags = TIG_VIDEO_BUFFER_BLIT_BLEND_COLOR_LERP;
    vb_blit_info->lerp_rect = &townmap_tile_bounds;
    vb_blit_info->lerp_colors[0] = (flags & 0x8) != 0 ? color : 0;
    vb_blit_info->lerp_colors[1] = (flags & 0x4) != 0 ? color : 0;
    vb_blit_info->lerp_colors[2] = (flags & 0x2) != 0 ? color : 0;
    vb_blit_info->lerp_colors[3] = (flags & 0x1) != 0 ? color : 0;

    return true;
}

/**
 * Checks if a town map is waitable.
 *
 * 0x4BECC0
 */
bool townmap_is_waitable(int map)
{
    int index = 0;

    for (index = 0; index < townmap_entries_cnt; index++) {
        if (townmap_entries[index].map == map) {
            return townmap_entries[index].w;
        }
    }

    return false;
}

/**
 * Resets the tile k-state array.
 *
 * 0x4BED00
 */
void townmap_kstate_reset(void)
{
    if (townmap_kstate != NULL) {
        FREE(townmap_kstate);
        townmap_kstate = NULL;
    }
    townmap_kstate_size = 0;
    townmap_kstate_modified = false;
    townmap_kstate_map = 0;
}

/**
 * Internal helper that sets all tiles k-state to the specified value.
 *
 * 0x4BED30
 */
void townmap_set_known_internal(int map, bool known)
{
    int idx;
    uint8_t value;

    if (townmap_kstate_load(map)) {
        // The k-state is a bit array, so for "known" state the 0xFF sets all
        // bits.
        value = known ? 0xFF : 0;
        for (idx = 0; idx < townmap_kstate_size; idx++) {
            townmap_kstate[idx] = value;
        }

        // Mark k-state as modified for subsequent flushes.
        townmap_kstate_modified = true;
    }
}

/**
 * Internal helper that marks the tile (identified by index) as known.
 *
 * 0x4BED90
 */
void townmap_tile_known_set(int map, int index)
{
    if (townmap_kstate_load(map)) {
        townmap_kstate[index / 8] |= 1 << (index % 8);
        townmap_kstate_modified = true;
    }
}

/**
 * Internal helper that checks if a specific tile is known and has a valid
 * bitmap.
 *
 * 0x4BEDD0
 */
bool townmap_tile_known_get(int map, int index)
{
    const char* name;
    char path[TIG_MAX_PATH];

    // Load k-state for the specified map.
    if (!townmap_kstate_load(map)) {
        return false;
    }

    // Check if the tile's bit is set.
    if ((townmap_kstate[index / 8] & (1 << (index % 8))) == 0) {
        return false;
    }

    // Verify that the bitmap for this tile actually exist.
    name = townmap_name(map);
    sprintf(path, "townmap\\%s\\%s%06d.bmp", name, name, index);
    if (!tig_file_exists(path, NULL)) {
        return false;
    }

    return true;
}

/**
 * Loads the k-state for a map.
 *
 * 0x4BEE60
 */
bool townmap_kstate_load(int map)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;

    // Check if the requested map's state is already loaded.
    if (townmap_kstate_map == map) {
        return true;
    }

    // Save previous k-state in case it was modified.
    townmap_kstate_flush();

    // Initialize k-state for the new map.
    townmap_kstate_init(map);

    sprintf(path, "%s\\%s.tmf", "Save\\Current", townmap_name(townmap_kstate_map));
    if (!tig_file_exists(path, NULL)) {
        // File not exists, initialize as unknown.
        townmap_set_known_internal(map, false);
        return true;
    }

    // Proceed to reading k-state from the file.
    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }

    if (tig_file_fread(townmap_kstate, 1u, townmap_kstate_size, stream) != townmap_kstate_size) {
        tig_file_fclose(stream);
        return false;
    }

    tig_file_fclose(stream);
    return true;
}

/**
 * Initializes the k-state for a town map.
 *
 * 0x4BEF40
 */
void townmap_kstate_init(int map)
{
    TownMapInfo tmi;

    townmap_kstate_reset();

    if (townmap_info(map, &tmi)) {
        // Calculate the size of k-state bit array.
        townmap_kstate_size = tmi.num_hor_tiles * tmi.num_vert_tiles / 8;
        if ((tmi.num_hor_tiles * tmi.num_vert_tiles) % 8 != 0) {
            townmap_kstate_size++;
        }

        townmap_kstate = (uint8_t*)MALLOC(townmap_kstate_size);

        // Save the map which k-state was initialized for.
        townmap_kstate_map = map;
    }
}

/**
 * Saves the modified k-state to a file.
 *
 * 0x4BEFB0
 */
bool townmap_kstate_flush(void)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;

    if (townmap_kstate_modified) {
        sprintf(path, "%s\\%s.tmf", "Save\\Current", townmap_name(townmap_kstate_map));
        stream = tig_file_fopen(path, "wb");
        if (stream == NULL) {
            return false;
        }

        if (tig_file_fwrite(townmap_kstate, 1, townmap_kstate_size, stream) != townmap_kstate_size) {
            tig_file_fclose(stream);
            tig_file_remove(path);
            return false;
        }

        tig_file_fclose(stream);
    }

    return true;
}
