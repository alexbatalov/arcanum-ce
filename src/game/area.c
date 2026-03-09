#include "game/area.h"

#include "game/location.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/player.h"
#include "game/sector.h"
#include "game/townmap.h"
#include "game/ui.h"

/**
 * Default area detection radius when it's not specified.
 */
#define DEFAULT_RADIUS 5

typedef uint8_t AreaFlags;

#define AREA_KNOWN 0x1u

static int get_nearest_area(int64_t obj);
static int area_from_loc(int64_t loc);

/**
 * The number of areas.
 *
 * 0x5FF5A8
 */
static int num_areas;

/**
 * Array of area locations.
 *
 * 0x5FF5AC
 */
static int64_t* area_locations;

/**
 * Array of area names.
 *
 * 0x5FF5B0
 */
static char** area_names;

/**
 * "gamearea.mes"
 *
 * 0x5FF5B4
 */
static mes_file_handle_t area_gamearea_mes_file;

/**
 * Per-player area flags (in multiplayer mode), tracking known areas.
 *
 * 0x5FF5B8
 */
static AreaFlags* area_flags_per_player[8];

/**
 * Array of area short descriptions.
 *
 * 0x5FF5D8
 */
static char** area_descriptions;

/**
 * Array of x offsets to area worldmap labels.
 *
 * 0x5FF5DC
 */
static int* area_wm_x_offsets;

/**
 * Array of y offsets to area worldmap labels.
 *
 * 0x5FF5E0
 */
static int* area_wm_y_offsets;

/**
 * Array of area detection radiuses.
 *
 * 0x5FF5E4
 */
static int* area_radiuses;

/**
 * Array of area flags, tracking known areas in single-player mode.
 *
 * 0x5FF5E8
 */
static AreaFlags* area_flags;

/**
 * Last area marked known in single-player mode.
 *
 * 0x5FF5EC
 */
static int area_last_known_area;

/**
 * Per-player last area marked known in multiplayer mode.
 *
 * 0x5FF5F0
 */
static int area_last_known_area_per_player[8];

/**
 * Called when the game is initialized.
 *
 * 0x4CA940
 */
bool area_init(GameInitInfo* init_info)
{
    (void)init_info;

    num_areas = 0;

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4CA950
 */
void area_reset(void)
{
    int area;
    int player;

    // Clear local player area flags.
    for (area = 0; area < num_areas; area++) {
        area_flags[area] = 0;
    }

    // Clear per-player area flags for all players.
    for (player = 0; player < 8; player++) {
        for (area = 0; area < num_areas; area++) {
            area_flags_per_player[player][area] = 0;
        }
    }
}

/**
 * Called when the game shuts down.
 *
 * 0x4CA9A0
 */
void area_exit(void)
{
}

/**
 * Called when a module is being loaded.
 *
 * 0x4CA9B0
 */
bool area_mod_load(void)
{
    MesFileEntry mes_file_entry;
    int index;
    char* str;
    int64_t x;
    int64_t y;
    char* pch;
    int radius;

    num_areas = 0;

    // Load module-specific areas message file (optional).
    if (!mes_load("mes\\gamearea.mes", &area_gamearea_mes_file)) {
        return true;
    }

    // Count the number of areas.
    mes_file_entry.num = 0;
    mes_get_msg(area_gamearea_mes_file, &mes_file_entry);
    do {
        num_areas++;
    } while (mes_find_next(area_gamearea_mes_file, &mes_file_entry));

    // Allocate arrays for area data.
    area_names = (char**)MALLOC(sizeof(*area_names) * num_areas);
    area_descriptions = (char**)MALLOC(sizeof(*area_descriptions) * num_areas);
    area_locations = (int64_t*)MALLOC(sizeof(*area_locations) * num_areas);
    area_flags = (uint8_t*)MALLOC(sizeof(*area_flags) * num_areas);

    for (index = 0; index < 8; index++) {
        area_flags_per_player[index] = (uint8_t*)MALLOC(sizeof(*area_flags_per_player[index]) * num_areas);
    }

    area_radiuses = (int*)MALLOC(sizeof(*area_radiuses) * num_areas);
    area_wm_x_offsets = (int*)MALLOC(sizeof(*area_wm_x_offsets) * num_areas);
    area_wm_y_offsets = (int*)MALLOC(sizeof(*area_wm_y_offsets) * num_areas);

    // Parse each area entry.
    index = 0;
    mes_file_entry.num = 0;
    mes_get_msg(area_gamearea_mes_file, &mes_file_entry);
    tig_str_parse_set_separator(',');

    do {
        str = mes_file_entry.str;

        // The first pair are coordinates of the placement of the area on the
        // world map.
        tig_str_parse_value_64(&str, &x);
        tig_str_parse_value_64(&str, &y);
        area_locations[index] = location_make(x, y);

        // The next pair are offsets (in pixels) of the area label on the
        // world map.
        area_wm_x_offsets[index] = 0;
        area_wm_y_offsets[index] = 0;
        tig_str_parse_value(&str, &(area_wm_x_offsets[index]));
        tig_str_parse_value(&str, &(area_wm_y_offsets[index]));

        // Parse the name and short description, separated by '/'.
        pch = strchr(mes_file_entry.str, '/');
        if (pch != NULL) {
            area_names[index] = pch + 1;

            pch = strchr(pch + 1, '/');
            if (pch != NULL) {
                // Clamp area name.
                *pch++ = '\0';
                area_descriptions[index] = pch;

                area_flags[index] = 0;
                area_radiuses[index] = DEFAULT_RADIUS * 64;

                // Parse radius if present.
                pch = strchr(pch, '/');
                if (pch != NULL) {
                    // Clamp area short description.
                    *pch++ = '\0';

                    if (tig_str_parse_named_value(&pch, "Radius:", &radius)) {
                        if (index > 0) {
                            // Convert radius in sectors to radius in tiles.
                            area_radiuses[index] = radius * 64;
                        } else if (index == 0) {
                            // Special case - radius of unknown area is always
                            // 0.
                            area_radiuses[AREA_UNKNOWN] = 0;
                        } else {
                            // FIXME: Unreachable, `index` is never negative.
                            area_radiuses[index] = -1;
                        }
                    }
                }
            } else {
                tig_debug_printf("Area: area_init: ERROR: Line %d has invalid description data!\n", index);
            }
        } else {
            tig_debug_printf("Area: area_init: ERROR: Line %d has invalid location data!\n", index);
        }

        index++;
    } while (mes_find_next(area_gamearea_mes_file, &mes_file_entry));

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4CACB0
 */
void area_mod_unload(void)
{
    int player;

    if (area_gamearea_mes_file != MES_FILE_HANDLE_INVALID) {
        FREE(area_names);
        FREE(area_descriptions);
        FREE(area_locations);
        FREE(area_flags);

        for (player = 0; player < 8; player++) {
            FREE(area_flags_per_player[player]);
        }

        FREE(area_radiuses);
        FREE(area_wm_x_offsets);
        FREE(area_wm_y_offsets);

        mes_unload(area_gamearea_mes_file);
        area_gamearea_mes_file = MES_FILE_HANDLE_INVALID;
    }
}

/**
 * Called when the game is being loaded.
 *
 * 0x4CAD50
 */
bool area_load(GameLoadInfo* load_info)
{
    int player;

    if (tig_file_fread(area_flags, num_areas, 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&area_last_known_area, sizeof(area_last_known_area), 1, load_info->stream) != 1) return false;

    for (player = 0; player < 8; player++) {
        if (tig_file_fread(area_flags_per_player[player], num_areas, 1, load_info->stream) != 1) return false;
        if (tig_file_fread(&(area_last_known_area_per_player[player]), sizeof(area_last_known_area_per_player[0]), 1, load_info->stream) != 1) return false;
    }

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x4CADF0
 */
bool area_save(TigFile* stream)
{
    int player;

    if (tig_file_fwrite(area_flags, num_areas, 1, stream) != 1) return false;
    if (tig_file_fwrite(&area_last_known_area, sizeof(area_last_known_area), 1, stream) != 1) return false;

    for (player = 0; player < 8; player++) {
        if (tig_file_fwrite(area_flags_per_player[player], num_areas, 1, stream) != 1) return false;
        if (tig_file_fwrite(&(area_last_known_area_per_player[player]), sizeof(area_last_known_area_per_player[0]), 1, stream) != 1) return false;
    }

    return true;
}

/**
 * Returns the total number of areas.
 *
 * 0x4CAE80
 */
int area_count(void)
{
    return num_areas;
}

/**
 * Retrieves the name of a specific area.
 *
 * 0x4CAE90
 */
char* area_get_name(int area)
{
    return area >= 0 && area < num_areas
        ? area_names[area]
        : NULL;
}

/**
 * Retrieves the description of a specific area.
 *
 * 0x4CAEB0
 */
char* area_get_description(int area)
{
    return area >= 0 && area < num_areas
        ? area_descriptions[area]
        : NULL;
}

/**
 * Retrieves the location of a specific area.
 *
 * 0x4CAED0
 */
int64_t area_get_location(int area)
{
    return area >= 0 && area < num_areas
        ? area_locations[area]
        : 0;
}

/**
 * Retrieves the world map label offsets for a specific area.
 *
 * 0x4CAF00
 */
void area_get_wm_offset(int area, int* x, int* y)
{
    // FIXME: No bound checking guard seen in previous functions.
    *x = area_wm_x_offsets[area];
    *y = area_wm_y_offsets[area];
}

/**
 * Checks if an area is known to a player character.
 *
 * 0x4CAF50
 */
bool area_is_known(int64_t pc_obj, int area)
{
    int player;

    // Validate the object and ensure it's a player character.
    if (pc_obj == OBJ_HANDLE_NULL
        || obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    if (tig_net_is_active()) {
        player = multiplayer_find_slot_from_obj(pc_obj);
        if (player != -1) {
            return (area_flags_per_player[player][area] & AREA_KNOWN) != 0;
        }
    } else if (pc_obj == player_get_local_pc_obj()) {
        return (area_flags[area] & AREA_KNOWN) != 0;
    }

    return false;
}

/**
 * Marks an area as known to a player character.
 *
 * 0x4CAFD0
 */
bool area_set_known(int64_t pc_obj, int area)
{
    PacketAreaKnownSet pkt;
    int player;

    // If the area is already known, no action is needed.
    if (area_is_known(pc_obj, area)) {
        return true;
    }

    // Validate the object and ensure it's a player character.
    if (pc_obj == OBJ_HANDLE_NULL
        || obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    if (tig_net_is_active()) {
        if (!multiplayer_is_locked()) {
            if (tig_net_is_host()) {
                pkt.type = 101;
                pkt.oid = obj_get_id(pc_obj);
                pkt.area = area;
                tig_net_send_app_all(&pkt, sizeof(pkt));

                player = multiplayer_find_slot_from_obj(pc_obj);
                if (player == -1) {
                    return false;
                }

                area_flags_per_player[player][area] |= AREA_KNOWN;
                area_last_known_area_per_player[player] = area;
                mp_ui_toggle_primary_button(UI_PRIMARY_BUTTON_WORLDMAP, true, player);
            }
            return true;
        }
    } else if (pc_obj == player_get_local_pc_obj()) {
        area_flags[area] |= AREA_KNOWN;
        area_last_known_area = area;
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_WORLDMAP, true);
        return true;
    }

    return false;
}

/**
 * Retrieves the last known area for a player character.
 *
 * 0x4CB100
 */
int area_get_last_known_area(int64_t pc_obj)
{
    int player;

    // Validate the object and ensure it's a player character.
    if (pc_obj == OBJ_HANDLE_NULL
        || obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return AREA_UNKNOWN;
    }

    if (tig_net_is_active()) {
        player = multiplayer_find_slot_from_obj(pc_obj);
        if (player != -1) {
            return area_last_known_area_per_player[player];
        }
    } else if (pc_obj == player_get_local_pc_obj()) {
        return area_last_known_area;
    }

    return AREA_UNKNOWN;
}

/**
 * Resets the last known area for a player character.
 *
 * 0x4CB160
 */
void area_reset_last_known_area(int64_t pc_obj)
{
    PacketAreaResetLastKnown pkt;
    int player;

    // Validate the object and ensure it's a player character.
    if (pc_obj == OBJ_HANDLE_NULL
        || obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    if (tig_net_is_active()) {
        if (!multiplayer_is_locked()) {
            if (!tig_net_is_host()) {
                return;
            }

            pkt.type = 102;
            pkt.oid = obj_get_id(pc_obj);
            tig_net_send_app_all(&pkt, sizeof(pkt));

            player = multiplayer_find_slot_from_obj(pc_obj);
            if (player != -1) {
                area_last_known_area_per_player[player] = AREA_UNKNOWN;
            }
        }
    } else if (pc_obj == player_get_local_pc_obj()) {
        area_last_known_area = AREA_UNKNOWN;
    }
}

/**
 * Finds the nearest area to a given location.
 *
 * 0x4CB220
 */
int get_nearest_area(int64_t loc)
{
    int area;
    int nearest_area;
    int64_t nearest_dist;
    int64_t distance;

    // Initialize with the unknown area as a base.
    nearest_area = AREA_UNKNOWN;
    nearest_dist = location_dist(area_get_location(nearest_area), loc);

    // Compare distances to all other areas.
    for (area = 1; area < num_areas; area++) {
        distance = location_dist(area_get_location(area), loc);
        if (distance < nearest_dist) {
            nearest_area = area;
            nearest_dist = distance;
        }
    }

    return nearest_area;
}

/**
 * Finds the nearest known area to a location within a range for a player
 * character.
 *
 * 0x4CB2A0
 */
int area_get_nearest_known_area(int64_t loc, int64_t pc_obj, int64_t range)
{
    int area;
    int nearest_area;
    int64_t nearest_dist;
    int64_t dist;

    // Validate the object and ensure it's a player character.
    if (pc_obj == OBJ_HANDLE_NULL
        || obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return AREA_UNKNOWN;
    }

    // Initialize with an invalid distance to ensure at least one area is
    // considered.
    nearest_area = AREA_UNKNOWN;
    nearest_dist = range + 1;

    // Check all areas for known status and distance.
    for (area = 1; area < num_areas; area++) {
        dist = location_dist(area_get_location(area), loc);
        if (dist < nearest_dist) {
            if (area_is_known(pc_obj, area)) {
                nearest_area = area;
                nearest_dist = dist;
            }
        }
    }

    return nearest_area;
}

/**
 * NOTE: Original code is different.
 *
 * 0x4CB4D0
 */
int area_get_nearest_area_in_range(int64_t loc, bool safe)
{
    int area;
    int nearest_area;
    int64_t nearest_dist;
    int64_t dist;
    int64_t radius;

    // Initialize with an invalid distance to ensure at least one area is
    // considered.
    nearest_area = AREA_UNKNOWN;
    nearest_dist = 99999;

    // Check all areas for distance.
    for (area = 1; area < num_areas; area++) {
        radius = area_radiuses[area];
        if (safe && radius <= 0) {
            radius = 64;
        }

        dist = location_dist(area_get_location(area), loc);
        if (dist < radius
            && dist < nearest_dist) {
            nearest_area = area;
            nearest_dist = dist;
        }
    }

    return nearest_area;
}

/**
 * Determines the area from a specified location.
 *
 * 0x4CB630
 */
int area_from_loc(int64_t loc)
{
    int loc_townmap;
    int area;
    int area_townmap;

    // Retrieve the townmap for the sector containing the specified location.
    loc_townmap = townmap_get(sector_id_from_loc(loc));
    if (loc_townmap == TOWNMAP_NONE) {
        return AREA_UNKNOWN;
    }

    // Find the nearest area to the specified location.
    area = get_nearest_area(loc);
    if (area == AREA_UNKNOWN) {
        return AREA_UNKNOWN;
    }

    // Retrieve the townmap from the sector containing area's origin location.
    area_townmap = townmap_get(sector_id_from_loc(area_get_location(area)));
    if (area_townmap == TOWNMAP_NONE) {
        return AREA_UNKNOWN;
    }

    // Ensure both townmap IDs match.
    if (loc_townmap != area_townmap) {
        return AREA_UNKNOWN;
    }

    return area;
}

/**
 * Retrieves the area associated with an object's current map.
 *
 * The `obj` is implied to be a player character, but this is not enforced.
 *
 * 0x4CB6A0
 */
int area_of_object(int64_t obj)
{
    int map;
    int area;

    // Validate the object.
    if (obj == OBJ_HANDLE_NULL) {
        return AREA_UNKNOWN;
    }

    map = map_current_map();

    // Retrieve area from the current map.
    if (!map_get_area(map, &area)) {
        return AREA_UNKNOWN;
    }

    // Special case - the primary map usually does not have associated area.
    // In this case attempt to obtain area from PC's location.
    if (area == AREA_UNKNOWN
        && map == map_by_type(MAP_TYPE_START_MAP)) {
        area = area_from_loc(obj_field_int64_get(obj, OBJ_F_LOCATION));
    }

    return area;
}
