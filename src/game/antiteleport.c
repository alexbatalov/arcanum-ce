#include "game/antiteleport.h"

#include <tig/tig.h>

#include "game/area.h"
#include "game/location.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/obj.h"

typedef struct MapEntry {
    int map;
} MapEntry;

typedef struct AntiTeleportMapList {
    MapEntry* entries;
    int cnt;
} AntiTeleportMapList;

typedef struct RegionEntry {
    int64_t location;
    int64_t radius;
} RegionEntry;

typedef struct AntiteleportRegionList {
    RegionEntry* entries;
    int cnt;
} AntiteleportRegionList;

static void antiteleport_clear(void);
static void antiteleport_region_list_init(AntiteleportRegionList* region_list);
static bool antiteleport_region_list_load(mes_file_handle_t mes_file, AntiteleportRegionList* region_list, int num);
static void antiteleport_map_list_init(AntiTeleportMapList* map_list);
static bool antiteleport_map_list_load(mes_file_handle_t mes_file, AntiTeleportMapList* map_list, int num);

/**
 * List of maps where teleportation is restricted.
 *
 * 0x5FC490
 */
static AntiTeleportMapList antiteleport_map_list;

/**
 * List of regions where teleportation is restricted.
 *
 * 0x5FC498
 */
static AntiteleportRegionList antiteleport_region_list;

/**
 * Flag indicating whether the anti-teleport system is initialized.
 *
 * 0x5FC4A0
 */
static bool antiteleport_initialized;

/**
 * Flag indicating whether the anti-teleport system is disabled.
 *
 * This flag is enabled when "antiteleport.mes" is malformed.
 *
 * 0x5FC4A4
 */
static bool antiteleport_disabled;

/**
 * Flag indicating whether the anti-teleport module data has been loaded.
 *
 * 0x5FC4A8
 */
static bool antiteleport_mod_loaded;

/**
 * Called when the game is initialized.
 *
 * 0x4BD980
 */
bool antiteleport_init(GameInitInfo* init_info)
{
    (void)init_info;

    // NOTE: This precondition is somewhat odd. Antiteleport can be disabled
    // only from within this very system. This handler is supposed to run only
    // one time before any other call to system-specific API. Which means it
    // will never be true.
    if (antiteleport_disabled) {
        return true;
    }

    antiteleport_region_list_init(&antiteleport_region_list);
    antiteleport_map_list_init(&antiteleport_map_list);

    antiteleport_initialized = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4BD9C0
 */
void antiteleport_exit(void)
{
    if (antiteleport_disabled) {
        return;
    }

    antiteleport_clear();

    antiteleport_initialized = false;
}

/**
 * Called when a module is being loaded.
 *
 * 0x4BD9E0
 */
bool antiteleport_mod_load(void)
{
    mes_file_handle_t antiteleport_mes_file;

    if (antiteleport_disabled) {
        return true;
    }

    // Load "antiteleport.mes" (might be absent).
    if (!mes_load("Rules\\AntiTeleport.mes", &antiteleport_mes_file)) {
        // FIXME: Typo in function name.
        tig_debug_printf("AntiTeleport: antiteleport_init: Note: No Message File Detected.\n");
        return true;
    }

    tig_str_parse_set_separator(',');

    // Load anti-teleport regions.
    if (!antiteleport_region_list_load(antiteleport_mes_file, &antiteleport_region_list, 100)) {
        tig_debug_println("Disabling Anti-Teleport Regions because of bad message file (Region List).");
        mes_unload(antiteleport_mes_file);
        antiteleport_disabled = true;
        return true;
    }

    // Load anti-teleport maps.
    if (!antiteleport_map_list_load(antiteleport_mes_file, &antiteleport_map_list, 1000)) {
        tig_debug_println("Disabling Anti-Teleport Regions because of bad message file (Map List).");
        mes_unload(antiteleport_mes_file);
        antiteleport_disabled = true;
        return true;
    }

    mes_unload(antiteleport_mes_file);
    antiteleport_mod_loaded = true;

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4BDAB0
 */
void antiteleport_mod_unload(void)
{
    antiteleport_clear();
    antiteleport_mod_loaded = false;
}

/**
 * Frees anti-teleport lists.
 *
 * 0x4BDAC0
 */
void antiteleport_clear(void)
{
    if (antiteleport_region_list.entries != NULL) {
        FREE(antiteleport_region_list.entries);
        antiteleport_region_list.entries = NULL;
        antiteleport_region_list.cnt = 0;
    }

    if (antiteleport_map_list.entries != NULL) {
        FREE(antiteleport_map_list.entries);
        antiteleport_map_list.entries = NULL;
        antiteleport_map_list.cnt = 0;
    }
}

/**
 * Initializes the anti-teleport region list.
 *
 * 0x4BDB10
 */
void antiteleport_region_list_init(AntiteleportRegionList* region_list)
{
    region_list->entries = NULL;
    region_list->cnt = 0;
}

/**
 * Loads anti-teleport regions from a message file.
 *
 * 0x4BDB30
 */
bool antiteleport_region_list_load(mes_file_handle_t mes_file, AntiteleportRegionList* region_list, int num)
{
    MesFileEntry mes_file_entry;
    int cnt;
    RegionEntry* region_entry;
    char* str;
    int64_t x;
    int64_t y;
    int radius;

    // Count the number of region entries in the message file.
    cnt = 0;
    mes_file_entry.num = num;
    if (mes_search(mes_file, &mes_file_entry)) {
        do {
            mes_file_entry.num++;
            cnt++;
        } while (mes_search(mes_file, &mes_file_entry));
    }

    if (cnt != 0) {
        region_list->entries = (RegionEntry*)MALLOC(sizeof(RegionEntry) * cnt);

        // Load each region entry.
        mes_file_entry.num = num;
        while (cnt > 0) {
            region_entry = &(region_list->entries[region_list->cnt]);

            mes_get_msg(mes_file, &mes_file_entry);
            str = mes_file_entry.str;

            // Parse x/y coordinates.
            tig_str_parse_value_64(&str, &x);
            tig_str_parse_value_64(&str, &y);
            region_entry->location = LOCATION_MAKE(x, y);

            // Parse radius, scale it from number of sectors to number of tiles.
            tig_str_parse_value(&str, &radius);
            region_entry->radius = (int64_t)radius * 64;

            mes_file_entry.num++;
            region_list->cnt++;
            cnt--;
        }
    }

    return true;
}

/**
 * Initializes the anti-teleport map list.
 *
 * 0x4BDC40
 */
void antiteleport_map_list_init(AntiTeleportMapList* map_list)
{
    map_list->entries = NULL;
    map_list->cnt = 0;
}

/**
 * Loads anti-teleport maps from a message file.
 *
 * 0x4BDC60
 */
bool antiteleport_map_list_load(mes_file_handle_t mes_file, AntiTeleportMapList* map_list, int num)
{
    MesFileEntry mes_file_entry;
    int cnt;
    MapEntry* map_entry;
    char* str;
    int map;

    // Count the number of map entries in the message file.
    cnt = 0;
    mes_file_entry.num = num;
    if (mes_search(mes_file, &mes_file_entry)) {
        do {
            mes_file_entry.num++;
            cnt++;
        } while (mes_search(mes_file, &mes_file_entry));
    }

    if (cnt != 0) {
        map_list->entries = (MapEntry*)MALLOC(sizeof(MapEntry) * cnt);

        // Load each map entry.
        mes_file_entry.num = num;
        while (cnt > 0) {
            map_entry = &(map_list->entries[map_list->cnt]);
            mes_get_msg(mes_file, &mes_file_entry);
            str = mes_file_entry.str;
            tig_str_parse_value(&str, &map);

            // Convert map number to map ID.
            if (map > 5000) {
                map -= 4999;
            } else {
                map += 1;
            }
            map_entry->map = map;

            mes_file_entry.num++;
            map_list->cnt++;
            cnt--;
        }
    }

    return true;
}

/**
 * Checks if a given object can teleport from from it's current location to a
 * specified destination.
 *
 * Returns `true` if teleportation is allowed, `false` otherwise.
 *
 * 0x4BDD30
 */
bool antiteleport_check(int64_t obj, int64_t to_loc)
{
    int map;
    int index;
    int area;
    int64_t loc;
    int64_t dist;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    // Ensure anti-teleport lists have been loaded.
    if (!antiteleport_mod_loaded) {
        return true;
    }

    map = map_current_map();

    // Check if the current map is on the restricted map list.
    for (index = 0; index < antiteleport_map_list.cnt; index++) {
        if (map == antiteleport_map_list.entries[index].map) {
            return false;
        }
    }

    // Determine the source location we're teleporting from.
    if (map == map_by_type(MAP_TYPE_START_MAP)) {
        // For main map the source location is the current world location.
        loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    } else if (map_get_area(map, &area)) {
        // For specific maps (caves, castles, etc) the source location is the
        // area's center point as specified in "gamearea.mes".
        loc = area_get_location(area);
    } else {
        // The maps without designated area (The Void) do not allow
        // teleportation at all.
        return false;
    }

    // Check if the source location is within the bounds of any restricted
    // regions.
    for (index = 0; index < antiteleport_region_list.cnt; index++) {
        dist = location_dist(loc, antiteleport_region_list.entries[index].location);
        if (dist < antiteleport_region_list.entries[index].radius) {
            return false;
        }
    }

    // Check if the destination location is within the bounds of any restricted
    // regions (if provided).
    if (to_loc != 0) {
        for (index = 0; index < antiteleport_region_list.cnt; index++) {
            dist = location_dist(to_loc, antiteleport_region_list.entries[index].location);
            if (dist < antiteleport_region_list.entries[index].radius) {
                return false;
            }
        }
    }

    // Teleportation is allowed.
    return true;
}
