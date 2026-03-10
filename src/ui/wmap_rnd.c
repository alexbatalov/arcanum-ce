#include "ui/wmap_rnd.h"

#include "game/area.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/object.h"
#include "game/player.h"
#include "game/proto.h"
#include "game/random.h"
#include "game/sector.h"
#include "game/stat.h"
#include "game/target.h"
#include "game/terrain.h"
#include "game/tile.h"
#include "game/timeevent.h"
#include "game/townmap.h"
#include "ui/sleep_ui.h"
#include "ui/wmap_ui.h"

/**
 * World map terrain types used by the random encounter system.
 */
typedef enum WmapTerrainType {
    WMAP_TERRAIN_TYPE_INVALID,
    WMAP_TERRAIN_TYPE_GREEN_GRASSLANDS,
    WMAP_TERRAIN_TYPE_PLAINS,
    WMAP_TERRAIN_TYPE_SWAMPS,
    WMAP_TERRAIN_TYPE_ELVEN_FOREST,
    WMAP_TERRAIN_TYPE_TROPICAL_JUNGLE,
    WMAP_TERRAIN_TYPE_DESERT,
    WMAP_TERRAIN_TYPE_FOREST,
    WMAP_TERRAIN_TYPE_SNOWY_PLAINS,
    WMAP_TERRAIN_TYPE_VOID,
} WmapTerrainType;

/**
 * A single entry in an encounter chart. Each entry defines a world-map
 * region (a centre location and a radius) and an associated integer value
 * whose meaning depends on which chart the entry belongs to (frequency
 * percentage, power tier, or encounter table index).
 */
typedef struct WmapRndEncounterChartEntry {
    /* 0000 */ int64_t loc;
    /* 0008 */ int64_t radius;
    /* 0010 */ int value;
} WmapRndEncounterChartEntry;

/**
 * A collection of encounter chart entries, sorted by ascending radius so
 * that the smallest (most specific) region matching a location is found
 * first by `wmap_rnd_encounter_chart_lookup`.
 */
typedef struct WmapRndEncounterChart {
    /* 0000 */ WmapRndEncounterChartEntry* entries;
    /* 0004 */ int num_entries;
} WmapRndEncounterChart;

/**
 * A single row in an encounter table, describing one type of monster group
 * that can be spawned. Up to five distinct critter types may be specified.
 * The entry is only eligible if the player's current level falls within
 * [min_level, max_level], the optional global flag is set, and the optional
 * trigger cap has not yet been reached.
 */
typedef struct WmapRndEncounterTableEntry {
    /* 0000 */ int frequency;
    /* 0004 */ int16_t critter_min_cnt[5];
    /* 000E */ int16_t critter_max_cnt[5];
    /* 0018 */ int critter_basic_prototype[5];
    /* 002C */ int16_t min_level;
    /* 002E */ int16_t max_level;
    /* 0030 */ int global_flag_num;
    /* 0034 */ int max_trigger_cnt;
    /* 0038 */ int trigger_cnt;
    /* 003C */ int message_num;
} WmapRndEncounterTableEntry;

/**
 * A collection of encounter table entries for one specific combination of
 * terrain type, power tier, and time of day. The active table is indexed
 * by the formula in `wmap_rnd_encounter_check`.
 */
typedef struct WmapRndEncounterTable {
    /* 0000 */ int num_entries;
    /* 0004 */ WmapRndEncounterTableEntry* entries;
} WmapRndEncounterTable;

/**
 * Minimal per-entry data written to and read from save files. Entries are
 * matched back to their in-memory counterparts by `message_num`.
 *
 * NOTE: Changing this struct will invalidate existing save games and break
 * interoperability with the original game.
 */
typedef struct WmapRndSaveInfo {
    /* 0000 */ int trigger_cnt;
    /* 0004 */ int message_num;
} WmapRndSaveInfo;

// Serializeable.
static_assert(sizeof(WmapRndSaveInfo) == 0x8, "wrong size");

static bool wmap_rnd_terrain_clear(uint16_t tid);
static bool wmap_rnd_mes_count_entries(MesFileEntry* mes_file_entry, int cnt, int* value_ptr);
static void wmap_rnd_encounter_table_init(WmapRndEncounterTable* table);
static void wmap_rnd_encounter_table_entry_init(WmapRndEncounterTableEntry* entry);
static void wmap_rnd_encounter_table_clear();
static void wmap_rnd_encounter_chart_clear();
static bool wmap_rnd_encounter_chart_parse(WmapRndEncounterChart* chart, int num, const char** value_list);
static void wmap_rnd_encounter_chart_entry_parse(char** str, WmapRndEncounterChartEntry* entry);
static void wmap_rnd_encounter_chart_exit(WmapRndEncounterChart* chart);
static void wmap_rnd_encounter_chart_sort(WmapRndEncounterChart* chart);
static int wmap_rnd_encounter_entry_compare(const void* va, const void* vb);
static bool wmap_rnd_parse_critters(char** str, WmapRndEncounterTableEntry* entry);
static bool wmap_rnd_check(int64_t location);
static bool wmap_rnd_encounter_chart_lookup(WmapRndEncounterChart* chart, int64_t loc, int* value_ptr);
static int wmap_rnd_determine_terrain(int64_t loc);
static bool wmap_rnd_encounter_check();
static bool wmap_rnd_encounter_entry_check(WmapRndEncounterTableEntry* entry);
static int wmap_rnd_encounter_total_frequency(WmapRndEncounterTable* table);
static int wmap_rnd_encounter_entry_total_monsters(WmapRndEncounterTableEntry* entry);
static void wmap_rnd_encounter_spawn(WmapRndEncounterTableEntry* entry);
static void wmap_rnd_spawn_position_offset(int a1, int64_t* dx_ptr, int64_t* dy_ptr);
static void wmap_rnd_encounter_build_object(int name, int64_t loc, int64_t* obj_ptr);

/**
 * String tokens used to match power tier names when parsing the power chart
 * from `WMap_Rnd.mes`.
 *
 * 0x5C79A0
 */
static const char* off_5C79A0[] = {
    "none",
    "easy",
    "average",
    "powerful",
};

/**
 * String tokens used when parsing the critter slots of an encounter table
 * entry. Each token labels one of the five possible critter groups within
 * a single encounter entry.
 *
 * 0x5C79B0
 */
static const char* off_5C79B0[] = {
    "First:",
    "Second:",
    "Third:",
    "Fourth:",
    "Fifth:",
};

/**
 * Lookup table that maps each engine TERRAIN_TYPE_* value (used as an array
 * index) to the corresponding `WmapTerrainType` used by the encounter system.
 *
 * Terrain types that should never trigger random encounters (water, mountain
 * ranges, scorched earth, etc.) map to WMAP_TERRAIN_TYPE_INVALID.
 *
 * 0x5C79C4
 */
static int wmap_rnd_terrain_type_tbl[TERRAIN_TYPE_COUNT] = {
    /*   TERRAIN_TYPE_GREEN_GRASSLANDS */ WMAP_TERRAIN_TYPE_GREEN_GRASSLANDS,
    /*             TERRAIN_TYPE_SWAMPS */ WMAP_TERRAIN_TYPE_SWAMPS,
    /*              TERRAIN_TYPE_WATER */ WMAP_TERRAIN_TYPE_INVALID,
    /*             TERRAIN_TYPE_PLAINS */ WMAP_TERRAIN_TYPE_PLAINS,
    /*             TERRAIN_TYPE_FOREST */ WMAP_TERRAIN_TYPE_FOREST,
    /*      TERRAIN_TYPE_DESERT_ISLAND */ WMAP_TERRAIN_TYPE_DESERT,
    /*        TERRAIN_TYPE_VOID_PLAINS */ WMAP_TERRAIN_TYPE_VOID,
    /*  TERRAIN_TYPE_BROAD_LEAF_FOREST */ WMAP_TERRAIN_TYPE_PLAINS,
    /*             TERRAIN_TYPE_DESERT */ WMAP_TERRAIN_TYPE_DESERT,
    /*          TERRAIN_TYPE_MOUNTAINS */ WMAP_TERRAIN_TYPE_INVALID,
    /*       TERRAIN_TYPE_ELVEN_FOREST */ WMAP_TERRAIN_TYPE_ELVEN_FOREST,
    /*         TERRAIN_TYPE_DEFORESTED */ WMAP_TERRAIN_TYPE_PLAINS,
    /*        TERRAIN_TYPE_SNOW_PLAINS */ WMAP_TERRAIN_TYPE_SNOWY_PLAINS,
    /*    TERRAIN_TYPE_TROPICAL_JUNGLE */ WMAP_TERRAIN_TYPE_TROPICAL_JUNGLE,
    /*     TERRAIN_TYPE_VOID_MOUNTAINS */ WMAP_TERRAIN_TYPE_INVALID,
    /*     TERRAIN_TYPE_SCORCHED_EARTH */ WMAP_TERRAIN_TYPE_INVALID,
    /*   TERRAIN_TYPE_DESERT_MOUNTAINS */ WMAP_TERRAIN_TYPE_INVALID,
    /*    TERRAIN_TYPE_SNOWY_MOUNTAINS */ WMAP_TERRAIN_TYPE_INVALID,
    /* TERRAIN_TYPE_TROPICAL_MOUNTAINS */ WMAP_TERRAIN_TYPE_INVALID,
};

/**
 * Spatial chart that maps world-map regions to an encounter type index,
 * allowing specific areas to override which encounter table is used.
 * Populated from message entries 30000+ in `WMap_Rnd.mes`.
 *
 * 0x64C728
 */
static WmapRndEncounterChart wmap_rnd_encounter_chart;

/**
 * "wmap_rnd.mes"
 *
 * 0x64C730
 */
static mes_file_handle_t wmap_rnd_mes_file;

/**
 * Encounter frequency percentage for the player's current world-map
 * location, as resolved from `wmap_rnd_frequency_chart`. A random roll of
 * 1-100 must be at or below this value for an encounter to proceed.
 * Multiplied by 5 when the player is resting.
 *
 * 0x64C738
 */
static int wmap_rnd_frequency;

/**
 * `True` if the current in-game time is considered night-time.
 *
 * 0x64C73C
 */
static bool wmap_rnd_night;

/**
 * Encounter power tier for the player's current world-map location,
 * resolved from `wmap_rnd_power_chart`.
 *
 * 0x64C740
 */
static int wmap_rnd_power;

/**
 * Encounter terrain type for the player's current world-map location,
 * derived from the underlying tile's base terrain type via
 * `wmap_rnd_terrain_type_tbl`.
 *
 * 0x64C744
 */
static WmapTerrainType wmap_rnd_terrain_type;

/**
 * Index of the encounter table to use for the current encounter, either
 * resolved from `wmap_rnd_encounter_chart` (location-specific override) or
 * computed by `wmap_rnd_encounter_check` from terrain type, power tier, and
 * time of day. Set to -1 to trigger the computed fallback.
 *
 * 0x64C748
 */
static int wmap_rnd_encounter;

/**
 * Per-slot monster spawn counts resolved during
 * `wmap_rnd_encounter_entry_total_monsters`.
 *
 * Element [i] holds the randomly chosen count for the i-th critter slot of
 * the currently pending encounter entry, to be consumed by
 * `wmap_rnd_encounter_spawn`.
 *
 * 0x64C74C
 */
static int wmap_rnd_critter_spawn_counts[5];

/**
 * World-map tile location of the current encounter, stored in component
 * form for use by `wmap_rnd_encounter_spawn`.
 *
 * 0x64C760
 */
static int64_t wmap_rnd_loc;

/**
 * X coordinate of `wmap_rnd_loc`.
 *
 * 0x64C768
 */
static int64_t wmap_rnd_loc_x;

/**
 * Y coordinate of `wmap_rnd_loc`.
 *
 * 0x64C770
 */
static int64_t wmap_rnd_loc_y;

/**
 * Spawn approach direction value for the current encounter group.
 *
 * 0x64C778
 */
static int wmap_rnd_spawn_direction;

/**
 * Spatial chart that maps world-map regions to an encounter frequency
 * percentage. Populated from message entries 10000+ in `WMap_Rnd.mes`.
 *
 * 0x64C780
 */
static WmapRndEncounterChart wmap_rnd_frequency_chart;

/**
 * Spatial chart that maps world-map regions to an encounter power tier.
 * Populated from message entries 20000+ in `WMap_Rnd.mes`.
 *
 * 0x64C788
 */
static WmapRndEncounterChart wmap_rnd_power_chart;

/**
 * Flag indicating whether the random encounter system has been initialized.
 *
 * 0x64C790
 */
static bool wmap_rnd_initialized;

/**
 * Flag indicating whether the random encounters are completely disabled.
 *
 * NOTE: Looks like there is no way to reenable them back. Random encounters
 * can be disabled with a command line switch.
 *
 * 0x64C794
 */
static bool wmap_rnd_disabled;

/**
 * Array of encounter tables, one per unique combination of terrain type,
 * power tier, and time of day. Indexed by the formula in
 * `wmap_rnd_encounter_check`.
 *
 * 0x64C798
 */
static WmapRndEncounterTable* wmap_rnd_encounter_tables;

/**
 * Number of encounter tables currently allocated in
 * `wmap_rnd_encounter_tables`.
 *
 * 0x64C79C
 */
static int wmap_rnd_num_encounter_tables;

/**
 * Called when the game is initialized.
 *
 * 0x5581B0
 */
bool wmap_rnd_init(GameInitInfo* init_info)
{
    (void)init_info;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x5581C0
 */
void wmap_rnd_exit()
{
}

/**
 * Called when the game is being reset.
 *
 * 0x5581D0
 */
void wmap_rnd_reset()
{
}

/**
 * Called when a module is being loaded.
 *
 * 0x5581E0
 */
bool wmap_rnd_mod_load()
{
    MesFileEntry mes_file_entry;
    int num_tables;
    int table_idx;
    WmapRndEncounterTable* table;
    int num_entries;
    int entry_idx;
    WmapRndEncounterTableEntry* entry;
    char* str;
    int value;

    if (wmap_rnd_disabled) {
        return true;
    }

    if (wmap_rnd_initialized) {
        return true;
    }

    if (!mes_load("Rules\\WMap_Rnd.mes", &wmap_rnd_mes_file)) {
        tig_debug_printf("wmap_rnd_mod_load: ERROR: failed to load msg file: Rules\\WMap_Rnd.mes!\n");
        exit(EXIT_FAILURE);
    }

    tig_str_parse_set_separator(',');

    // Parse the frequency chart (entries 10000+).
    if (!wmap_rnd_encounter_chart_parse(&wmap_rnd_frequency_chart, 10000, NULL)) {
        tig_debug_println("Disabling random encounters because of bad message file.");
        mes_unload(wmap_rnd_mes_file);
        wmap_rnd_initialized = true;
        return true;
    }

    // Parse the power chart (entries 20000+). Power tier names are matched
    // against wmap_rnd_power_level_names to produce integer values 0-3.
    if (!wmap_rnd_encounter_chart_parse(&wmap_rnd_power_chart, 20000, off_5C79A0)) {
        tig_debug_println("Disabling random encounters because of bad message file.");
        wmap_rnd_encounter_chart_exit(&wmap_rnd_frequency_chart);
        mes_unload(wmap_rnd_mes_file);
        wmap_rnd_initialized = true;
        return true;
    }

    // Parse the encounter type override chart (entries 30000+).
    if (!wmap_rnd_encounter_chart_parse(&wmap_rnd_encounter_chart, 30000, NULL)) {
        tig_debug_println("Disabling random encounters because of bad message file.");
        wmap_rnd_encounter_chart_exit(&wmap_rnd_frequency_chart);
        wmap_rnd_encounter_chart_exit(&wmap_rnd_power_chart);
        mes_unload(wmap_rnd_mes_file);
        wmap_rnd_initialized = true;
        return true;
    }

    // Entry 49999 holds the number of user-defined encounter tables.
    mes_file_entry.num = 49999;
    mes_get_msg(wmap_rnd_mes_file, &mes_file_entry);

    // The 54 is a fixed set of tables covering the nine encounter-eligible
    // terrain types × three power tiers × two time-of-day variants.
    num_tables = atoi(mes_file_entry.str) + 54;
    wmap_rnd_encounter_tables = MALLOC(sizeof(*wmap_rnd_encounter_tables) * num_tables);

    for (table_idx = 0; table_idx < num_tables; table_idx++) {
        table = &(wmap_rnd_encounter_tables[table_idx]);
        wmap_rnd_encounter_table_init(table);

        // Each table occupies up to 100 consecutive message entries. Count how
        // many are actually present.
        mes_file_entry.num = 50000 + 100 * table_idx;
        if (!wmap_rnd_mes_count_entries(&mes_file_entry, 100, &num_entries)) {
            tig_debug_println("Disabling random encounters because of bad message file.");
            wmap_rnd_encounter_table_clear();
            wmap_rnd_encounter_chart_clear();
            mes_unload(wmap_rnd_mes_file);
            wmap_rnd_initialized = true;
            return true;
        }

        mes_file_entry.num = 50000 + 100 * table_idx;

        table->num_entries = num_entries;
        table->entries = (WmapRndEncounterTableEntry*)MALLOC(sizeof(WmapRndEncounterTableEntry) * num_entries);
        wmap_rnd_num_encounter_tables++;

        for (entry_idx = 0; entry_idx < num_entries; entry_idx++) {
            entry = &(table->entries[entry_idx]);
            wmap_rnd_encounter_table_entry_init(entry);

            if (!mes_search(wmap_rnd_mes_file, &mes_file_entry)) {
                tig_debug_printf("Error:  Random encounter table is discontinuous at line %d.\n", mes_file_entry.num);
                tig_debug_println("Disabling random encounters because of bad message file.");
                wmap_rnd_encounter_table_clear();
                wmap_rnd_encounter_chart_clear();
                mes_unload(wmap_rnd_mes_file);
                wmap_rnd_initialized = true;
                return true;
            }

            // Store the message number as a stable key for save/load matching.
            entry->message_num = mes_file_entry.num;

            mes_get_msg(wmap_rnd_mes_file, &mes_file_entry);
            str = mes_file_entry.str;

            // First field: relative frequency weight for weighted random
            // selection.
            tig_str_parse_value(&str, &(entry->frequency));

            // Subsequent fields: up to five critter groups (prototype + count
            // range).
            if (!wmap_rnd_parse_critters(&str, entry)) {
                tig_debug_printf("Error: Random encounter table has no: prototype at line: %d.\n", mes_file_entry.num);
                tig_debug_println("Disabling random encounters because of bad message file.");
                wmap_rnd_encounter_table_clear();
                wmap_rnd_encounter_chart_clear();
                mes_unload(wmap_rnd_mes_file);
                wmap_rnd_initialized = true;
                return true;
            }

            // Minimum player level required for this entry to be eligible.
            if (tig_str_parse_named_value(&str, "MinLevel:", &value)) {
                if (value < 0 || value > 32000) {
                    tig_debug_printf("WmapRnd: Init: ERROR: MinLevel Value Wrong: Line: %d.\n", mes_file_entry.num);
                    tig_debug_println("Disabling random encounters because of bad message file.");
                    wmap_rnd_encounter_table_clear();
                    wmap_rnd_encounter_chart_clear();
                    mes_unload(wmap_rnd_mes_file);
                    wmap_rnd_initialized = true;
                    return true;
                }
                entry->min_level = (int16_t)value;
            }

            // Maximum player level required for this entry to be eligible.
            // TODO: Check, probably should be "MaxLevel:".
            if (tig_str_parse_named_value(&str, "MinLevel:", &value)) {
                if (value < 0 || value > 32000) {
                    tig_debug_printf("WmapRnd: Init: ERROR: MaxLevel Value Wrong: Line: %d.\n", mes_file_entry.num);
                    tig_debug_println("Disabling random encounters because of bad message file.");
                    wmap_rnd_encounter_table_clear();
                    wmap_rnd_encounter_chart_clear();
                    mes_unload(wmap_rnd_mes_file);
                    wmap_rnd_initialized = true;
                    return true;
                }
                entry->max_level = (int16_t)value;
            }

            // Global flag that must be set for this entry to be eligible.
            if (tig_str_parse_named_value(&str, "GlobalFlag:", &value)) {
                if (value > 3200) {
                    tig_debug_printf("WmapRnd: Init: ERROR: globalFlagNum Value Wrong: Line: %d.\n", mes_file_entry.num);
                    tig_debug_println("Disabling random encounters because of bad message file.");
                    wmap_rnd_encounter_table_clear();
                    wmap_rnd_encounter_chart_clear();
                    mes_unload(wmap_rnd_mes_file);
                    wmap_rnd_initialized = true;
                    return true;
                }
                entry->global_flag_num = value;
            }

            // Maximum number of times this entry may fire across the whole
            // playthrough.
            if (tig_str_parse_named_value(&str, "TriggerCount:", &value)) {
                entry->max_trigger_cnt = value;
            }
        }
    }

    mes_unload(wmap_rnd_mes_file);
    wmap_rnd_initialized = true;
    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x558700
 */
void wmap_rnd_mod_unload()
{
    if (wmap_rnd_disabled) {
        return;
    }

    if (wmap_rnd_initialized) {
        wmap_rnd_encounter_table_clear();
        wmap_rnd_encounter_chart_clear();
        wmap_rnd_initialized = false;
    }
}

/**
 * Called when the game is being saved.
 *
 * 0x558730
 */
bool wmap_rnd_save(TigFile* stream)
{
    int table_idx;
    int entry_idx;
    int save_info_idx;
    int num_entries;
    WmapRndSaveInfo* save_info;
    WmapRndEncounterTable* table;
    WmapRndEncounterTableEntry* entry;

    if (stream == NULL) {
        return false;
    }

    num_entries = 0;
    save_info = NULL;
    if (!wmap_rnd_disabled
        && wmap_rnd_initialized
        && wmap_rnd_encounter_tables != NULL) {
        // Count the total number of entries across all tables.
        for (table_idx = 0; table_idx < wmap_rnd_num_encounter_tables; table_idx++) {
            num_entries += wmap_rnd_encounter_tables[table_idx].num_entries;
        }

        // Collect save info from table/entries into a flat array.
        if (num_entries != 0) {
            save_info = (WmapRndSaveInfo*)MALLOC(sizeof(*save_info) * num_entries);
            save_info_idx = 0;
            for (table_idx = 0; table_idx < wmap_rnd_num_encounter_tables; table_idx++) {
                table = &(wmap_rnd_encounter_tables[table_idx]);
                for (entry_idx = 0; entry_idx < table->num_entries; entry_idx++) {
                    entry = &(table->entries[entry_idx]);
                    save_info[save_info_idx].trigger_cnt = entry->trigger_cnt;
                    save_info[save_info_idx].message_num = entry->message_num;
                }
            }
        }
    }

    // Write entry count...
    if (tig_file_fwrite(&num_entries, sizeof(num_entries), 1, stream) != 1) {
        tig_debug_println("Error writing random encounter save data.");
        if (save_info != NULL) {
            FREE(save_info);
        }
        return false;
    }

    // ...followed by the records themselves.
    if (save_info != NULL) {
        if (tig_file_fwrite(save_info, sizeof(*save_info) * num_entries, 1, stream) != 1) {
            tig_debug_println("Error writing random encounter save data.");
            FREE(save_info);
            return false;
        }

        FREE(save_info);
    }

    return true;
}

/**
 * Called when the game is being loaded.
 *
 * 0x558880
 */
bool wmap_rnd_load(GameLoadInfo* load_info)
{
    int table_idx;
    int entry_idx;
    int save_info_idx;
    int num_entries;
    WmapRndSaveInfo* save_info;
    WmapRndEncounterTable* table;
    WmapRndEncounterTableEntry* entry;

    if (load_info->stream == NULL) {
        return false;
    }

    if (tig_file_fread(&num_entries, sizeof(num_entries), 1, load_info->stream) != 1) {
        tig_debug_println("Error reading random encounter save data.");
        return false;
    }

    if (num_entries != 0) {
        save_info = (WmapRndSaveInfo*)MALLOC(sizeof(*save_info) * num_entries);
        if (tig_file_fread(save_info, sizeof(*save_info) * num_entries, 1, load_info->stream) != 1) {
            tig_debug_println("Error reading random encounter save data.");
            FREE(save_info);
            return false;
        }

        // Restore trigger counts only when the module is fully operational.
        // Silently skip if random encounters were disabled or not yet loaded.
        if (!wmap_rnd_disabled
            && wmap_rnd_initialized
            && wmap_rnd_num_encounter_tables != 0
            && wmap_rnd_encounter_tables != NULL) {
            for (table_idx = 0; table_idx < wmap_rnd_num_encounter_tables; table_idx++) {
                table = &(wmap_rnd_encounter_tables[table_idx]);
                for (entry_idx = 0; entry_idx < table->num_entries; entry_idx++) {
                    entry = &(table->entries[entry_idx]);
                    for (save_info_idx = 0; save_info_idx < num_entries; save_info_idx++) {
                        if (entry->message_num == save_info[save_info_idx].message_num) {
                            entry->trigger_cnt = save_info[save_info_idx].trigger_cnt;
                            break;
                        }
                    }
                }
            }
        }

        FREE(save_info);
    }

    return true;
}

/**
 * Disables the random encounters.
 *
 * 0x5589D0
 */
void wmap_rnd_disable()
{
    wmap_rnd_disabled = true;
}

/**
 * Returns `true` if the specified terrain id is suitable for a random
 * encounter.
 *
 * 0x5589E0
 */
bool wmap_rnd_terrain_clear(uint16_t tid)
{
    int base_terrain_type;
    int comp_terrain_type;

    base_terrain_type = sub_4E8DC0(tid);
    comp_terrain_type = sub_4E8DD0(tid);
    if (!TERRAIN_TYPE_IS_VALID(base_terrain_type) || !TERRAIN_TYPE_IS_VALID(comp_terrain_type)) {
        tig_debug_println("Error:  Unknown terrain encountered in wmap_rnd_terrain_clear");
        return false;
    }

    if (wmap_rnd_terrain_type_tbl[base_terrain_type] == WMAP_TERRAIN_TYPE_INVALID
        || wmap_rnd_terrain_type_tbl[comp_terrain_type] == WMAP_TERRAIN_TYPE_INVALID) {
        return false;
    }

    return true;
}

/**
 * Counts the number of consecutive message entries present in the given range.
 * On success the count is written to `*value_ptr` and `true` is returned.
 * Returns `false` if a duplicate entry number is detected.
 *
 * 0x558A40
 */
bool wmap_rnd_mes_count_entries(MesFileEntry* mes_file_entry, int cnt, int* value_ptr)
{
    int end;
    int index = 0;
    int v1 = -1;

    end = mes_file_entry->num + cnt;

    mes_get_msg(wmap_rnd_mes_file, mes_file_entry);

    while (mes_file_entry->num != v1) {
        if (mes_file_entry->num >= end) {
            *value_ptr = index;
            return true;
        }

        index++;
        if (!mes_find_next(wmap_rnd_mes_file, mes_file_entry)) {
            *value_ptr = index;
            return true;
        }
    }

    tig_debug_printf("Error:  Line %d is duplicated in random encounter tables.\n", v1);
    return false;
}

/**
 * Initializes an encounter table to an empty state.
 *
 * 0x558AC0
 */
void wmap_rnd_encounter_table_init(WmapRndEncounterTable* table)
{
    table->num_entries = 0;
}

/**
 * Initializes an encounter table entry.
 *
 * 0x558AD0
 */
void wmap_rnd_encounter_table_entry_init(WmapRndEncounterTableEntry* entry)
{
    entry->critter_basic_prototype[0] = 0;
    entry->min_level = 0;
    entry->max_level = 32000;
    entry->global_flag_num = -1;
    entry->max_trigger_cnt = -1;
    entry->trigger_cnt = 0;
}

/**
 * Frees all encounter tables and their entries.
 *
 * 0x558AF0
 */
void wmap_rnd_encounter_table_clear()
{
    int index;

    if (wmap_rnd_encounter_tables != NULL) {
        for (index = 0; index < wmap_rnd_num_encounter_tables; index++) {
            FREE(wmap_rnd_encounter_tables[index].entries);
        }
        FREE(wmap_rnd_encounter_tables);
        wmap_rnd_encounter_tables = NULL;
        wmap_rnd_num_encounter_tables = 0;
    }
}

/**
 * Frees all three spatial encounter charts (frequency, power, and encounter
 * type override).
 *
 * 0x558B50
 */
void wmap_rnd_encounter_chart_clear()
{
    wmap_rnd_encounter_chart_exit(&wmap_rnd_frequency_chart);
    wmap_rnd_encounter_chart_exit(&wmap_rnd_power_chart);
    wmap_rnd_encounter_chart_exit(&wmap_rnd_encounter_chart);
}

/**
 * Parses a spatial encounter chart from the message file, starting at entry
 * `num`. Up to 10000 consecutive entries are read. If `value_list` is
 * non-NULL each entry's value field is resolved by matching it against the
 * list of strings; otherwise it is parsed as a plain integer.
 *
 * On success the chart's entries are sorted by ascending radius and true is
 * returned. Returns false if the chart is empty or the message file is
 * malformed.
 *
 * 0x558B80
 */
bool wmap_rnd_encounter_chart_parse(WmapRndEncounterChart* chart, int num, const char** value_list)
{
    MesFileEntry mes_file_entry;
    int cnt;
    int index;
    char* str;

    mes_file_entry.num = num;
    if (!wmap_rnd_mes_count_entries(&mes_file_entry, 10000, &cnt)) {
        return false;
    }

    chart->num_entries = cnt;
    if (chart->num_entries == 0) {
        tig_debug_println("Warning:  Empty random encouter chart.");
        return false;
    }

    chart->entries = (WmapRndEncounterChartEntry*)MALLOC(sizeof(*chart->entries) * chart->num_entries);
    for (index = 0; index < chart->num_entries; index++) {
        mes_file_entry.num = num + index;
        mes_get_msg(wmap_rnd_mes_file, &mes_file_entry);

        str = mes_file_entry.str;

        // Parse the location and radius common to all chart types.
        wmap_rnd_encounter_chart_entry_parse(&str, &(chart->entries[index]));

        // Parse the value field, either a named string token or a plain
        // integer.
        if (value_list != NULL) {
            SDL_strlwr(str);
            tig_str_match_str_to_list(&str, value_list, 4, &(chart->entries[index].value));
        } else {
            tig_str_parse_value(&str, &(chart->entries[index].value));
        }
    }

    // Sort by ascending radius so that the smallest (most specific) matching
    // region is found first in wmap_rnd_encounter_chart_lookup.
    wmap_rnd_encounter_chart_sort(chart);

    return true;
}

/**
 * Parses the location (x, y) and radius fields of a single chart entry from
 * the current position in the message string `*str`, advancing the pointer
 * past the parsed tokens.
 *
 * 0x558C90
 */
void wmap_rnd_encounter_chart_entry_parse(char** str, WmapRndEncounterChartEntry* entry)
{
    int64_t x;
    int64_t y;

    tig_str_parse_value_64(str, &x);
    tig_str_parse_value_64(str, &y);
    entry->loc = LOCATION_MAKE(x, y);
    tig_str_parse_value_64(str, &(entry->radius));
}

/**
 * Frees the entries array of a chart and resets its count to zero.
 *
 * 0x558CE0
 */
void wmap_rnd_encounter_chart_exit(WmapRndEncounterChart* chart)
{
    if (chart->num_entries != 0) {
        FREE(chart->entries);
        chart->num_entries = 0;
    }
}

/**
 * Sorts the entries of a chart in ascending order of radius.
 *
 * 0x558D00
 */
void wmap_rnd_encounter_chart_sort(WmapRndEncounterChart* chart)
{
    qsort(chart->entries, chart->num_entries, sizeof(*chart->entries), wmap_rnd_encounter_entry_compare);
}

/**
 * Comparator that orders encounter chart entries by ascending radius.
 *
 * 0x558D20
 */
int wmap_rnd_encounter_entry_compare(const void* va, const void* vb)
{
    const WmapRndEncounterChartEntry* a = (const WmapRndEncounterChartEntry*)va;
    const WmapRndEncounterChartEntry* b = (const WmapRndEncounterChartEntry*)vb;

    if (a->radius < b->radius) {
        return -1;
    } else if (a->radius > b->radius) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Parses up to five critter group definitions from the message string `*str`
 * into the given encounter table entry. Each group is introduced by its slot
 * keyword ("First:", "Second:", etc.) followed by a prototype ID and a
 * min-max count range. Parsing stops at the first missing slot keyword.
 *
 * Returns `true` if at least one critter group was successfully parsed,
 * `false` if none were found.
 *
 * 0x558D40
 */
bool wmap_rnd_parse_critters(char** str, WmapRndEncounterTableEntry* entry)
{
    int index;
    int bp;
    int lower;
    int upper;

    for (index = 0; index < 5; index++) {
        if (!tig_str_parse_named_value(str, off_5C79B0[index], &bp)) {
            entry->critter_basic_prototype[index] = 0;
            return index != 0;
        }

        entry->critter_basic_prototype[index] = bp;

        tig_str_parse_range(str, &lower, &upper);
        entry->critter_min_cnt[index] = (int16_t)lower;
        entry->critter_max_cnt[index] = (int16_t)upper;
    }

    return true;
}

/**
 * Entry point for the random encounter roll. Called from the world map time
 * event handler with the player's current world-map tile location.
 *
 * Returns `true` if an encounter was triggered (monsters have been spawned),
 * `false` otherwise.
 *
 * 0x558DE0
 */
bool wmap_rnd_check(int64_t loc)
{
    int hour;

    if (wmap_rnd_disabled) {
        return false;
    }

    if (!wmap_rnd_initialized) {
        return false;
    }

    // Cache location components for later use by the spawn routines.
    wmap_rnd_loc = loc;
    wmap_rnd_loc_x = LOCATION_GET_X(loc);
    wmap_rnd_loc_y = LOCATION_GET_Y(loc);

    // Look up encounter frequency for this location. Default to 5% if not
    // covered by any chart entry.
    if (!wmap_rnd_encounter_chart_lookup(&wmap_rnd_frequency_chart, loc, &wmap_rnd_frequency)) {
        wmap_rnd_frequency = 5;
    }

    // When the player is resting, random encounters are five times more likely.
    if (sleep_ui_is_active()) {
        wmap_rnd_frequency *= 5;
    }

    if (random_between(1, 100) > wmap_rnd_frequency) {
        return false;
    }

    // Look up power tier.
    if (wmap_rnd_encounter_chart_lookup(&wmap_rnd_power_chart, loc, &wmap_rnd_power)) {
        if (wmap_rnd_power == 0) {
            // Power "none" means no encounters are possible in this area.
            return false;
        }
    } else {
        wmap_rnd_power = 2;
    }

    // Determine day vs. night encounters.
    hour = datetime_current_hour();
    wmap_rnd_night = hour < 6 || hour >= 18;

    // Town sectors suppress random encounters entirely.
    if (townmap_get(sector_id_from_loc(loc)) != TOWNMAP_NONE) {
        return false;
    }

    // Determine the encounter terrain type from the tile's base terrain.
    wmap_rnd_terrain_type = wmap_rnd_determine_terrain(loc);
    if (wmap_rnd_terrain_type == WMAP_TERRAIN_TYPE_INVALID) {
        return false;
    }

    // Optionally override which encounter table is used for this location.
    // -1 triggers the computed fallback in `wmap_rnd_encounter_check`.
    if (!wmap_rnd_encounter_chart_lookup(&wmap_rnd_encounter_chart, loc, &wmap_rnd_encounter)) {
        wmap_rnd_encounter = -1;
    }

    if (!wmap_rnd_encounter_check()) {
        return false;
    }

    return true;
}

/**
 * Searches `chart` for the first entry whose circular region contains `loc`
 * (i.e. distance from `loc` to the entry's centre is at most the entry's
 * radius). Because the chart is sorted by ascending radius, the first match
 * is always the most specific (smallest) region.
 *
 * On success writes the matched entry's value to `*value_ptr` and returns
 * `true`. Returns `false` if no entry matches.
 *
 * 0x558F30
 */
bool wmap_rnd_encounter_chart_lookup(WmapRndEncounterChart* chart, int64_t loc, int* value_ptr)
{
    int index;

    for (index = 0; index < chart->num_entries; index++) {
        if (location_dist(chart->entries[index].loc, loc) <= chart->entries[index].radius) {
            *value_ptr = chart->entries[index].value;
            return true;
        }
    }

    return false;
}

/**
 * Resolves the `WmapTerrainType` for a world-map tile location by reading its
 * base terrain type from the sector's tile and mapping it through
 * `wmap_rnd_terrain_type_tbl`.
 *
 * Returns `WMAP_TERRAIN_TYPE_INVALID` if the tile is impassable terrain or if
 * an unrecognised terrain base type is encountered.
 *
 * 0x558FB0
 */
int wmap_rnd_determine_terrain(int64_t loc)
{
    uint64_t sec;
    uint16_t tid;
    int terrain_base_type;

    sec = sector_id_from_loc(loc);
    tid = sub_4E87F0(sec);
    if (!wmap_rnd_terrain_clear(tid)) {
        return WMAP_TERRAIN_TYPE_INVALID;
    }

    terrain_base_type = sub_4E8DC0(tid);
    if (terrain_base_type < 0 || terrain_base_type >= 19) {
        tig_debug_println("Error:  Unknown terrain base encountered inwmap_rnd_determine_terrain");
        return WMAP_TERRAIN_TYPE_INVALID;
    }

    return wmap_rnd_terrain_type_tbl[terrain_base_type];
}

/**
 * Selects a specific encounter table entry using a weighted random draw and
 * triggers the monster spawn.
 *
 * Returns `true` if an encounter was successfully set up, `false` if the table
 * is empty, out of bounds, or all entries are ineligible.
 *
 * 0x559010
 */
bool wmap_rnd_encounter_check()
{
    int total_frequency;
    WmapRndEncounterTable* table;
    int frequency;
    int index;
    int last_eligible = -1;

    // Compute the default table index from the current terrain/power/time
    // combination if no location-specific override was provided.
    if (wmap_rnd_encounter == -1) {
        wmap_rnd_encounter = wmap_rnd_night + 2 * wmap_rnd_power + 6 * wmap_rnd_terrain_type - 8;
    }

    tig_debug_printf("WMap_Rnd: Random Encounter Fired: Table: %d.\n", wmap_rnd_encounter);

    if (wmap_rnd_encounter >= wmap_rnd_num_encounter_tables) {
        tig_debug_printf("WmapRnd: wmap_rnd_encounter_check: ERROR: Table %d was out of bounds!\n", wmap_rnd_encounter);
        return false;
    }

    table = &(wmap_rnd_encounter_tables[wmap_rnd_encounter]);
    total_frequency = wmap_rnd_encounter_total_frequency(table);
    if (total_frequency <= 0) {
        return false;
    }

    // Weighted random selection - subtract each eligible entry's frequency from
    // the roll until it goes negative, then use that entry.
    frequency = random_between(1, total_frequency);

    for (index = 0; index < table->num_entries; index++) {
        if (wmap_rnd_encounter_entry_check(&(table->entries[index]))) {
            last_eligible = index;
            if (frequency < table->entries[index].frequency) {
                break;
            }
            frequency -= table->entries[index].frequency;
        }
    }

    // If the loop exhausted all entries (rounding edge case), fall back to the
    // last eligible entry seen.
    if (index >= table->num_entries) {
        if (last_eligible == -1) {
            return false;
        }
        index = last_eligible;
    }

    table->entries[index].trigger_cnt++;

    if (wmap_rnd_encounter_entry_total_monsters(&(table->entries[index])) == 0) {
        tig_debug_println("WmapRnd: Total monsters is zero.");
        return false;
    }

    wmap_rnd_encounter_spawn(&(table->entries[index]));

    return true;
}

/**
 * Checks whether a single encounter table entry is currently eligible to be
 * selected.
 *
 * An entry is ineligible if:
 *   - The player's level is outside [min_level, max_level].
 *   - A required global flag is not set.
 *   - The entry has already fired its maximum number of times.
 *
 * 0x559150
 */
bool wmap_rnd_encounter_entry_check(WmapRndEncounterTableEntry* entry)
{
    int64_t pc_obj;
    int level;

    pc_obj = player_get_local_pc_obj();
    if (pc_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    // Check player's level.
    level = stat_level_get(pc_obj, STAT_LEVEL);
    if (level < entry->min_level
        || level > entry->max_level) {
        return false;
    }

    // Check if the required global flag is not set.
    if (entry->global_flag_num != -1 && !script_global_flag_get(entry->global_flag_num)) {
        return false;
    }

    // Check if the entry has already fired its maximum number of times.
    if (entry->max_trigger_cnt > -1 && entry->max_trigger_cnt <= entry->trigger_cnt) {
        return false;
    }

    return true;
}

/**
 * Computes the sum of the frequency weights of all currently eligible
 * entries in a table. Used as the upper bound for the weighted random draw
 * in `wmap_rnd_encounter_check`.
 *
 * 0x5591B0
 */
int wmap_rnd_encounter_total_frequency(WmapRndEncounterTable* table)
{
    int frequency = 0;
    int index;

    for (index = 0; index < table->num_entries; index++) {
        if (wmap_rnd_encounter_entry_check(&(table->entries[index]))) {
            frequency += table->entries[index].frequency;
        }
    }

    return frequency;
}

/**
 * Randomises and accumulates the per-slot monster counts for an encounter
 * entry into `wmap_rnd_critter_spawn_counts`, then returns their total.
 *
 * For each critter slot with a non-zero prototype, the spawn count is either
 * the fixed min value (when min equals max) or a random value in [min, max].
 * Returns 0 if no critter prototypes are defined.
 *
 * 0x559200
 */
int wmap_rnd_encounter_entry_total_monsters(WmapRndEncounterTableEntry* entry)
{
    int monsters = 0;
    int index;

    for (index = 0; index < 5; index++) {
        if (entry->critter_basic_prototype[index] == 0) {
            break;
        }

        wmap_rnd_critter_spawn_counts[index] = entry->critter_min_cnt[index] == entry->critter_max_cnt[index]
            ? entry->critter_min_cnt[index]
            : random_between(entry->critter_min_cnt[index], entry->critter_max_cnt[index]);
        monsters += wmap_rnd_critter_spawn_counts[index];
    }

    return monsters;
}

/**
 * Spawns all monsters for a selected encounter entry around the player's
 * current world-map location.
 *
 * 0x559260
 */
void wmap_rnd_encounter_spawn(WmapRndEncounterTableEntry* entry)
{
    int index;
    int k;
    ObjectList objects;
    ObjectNode* node;
    int64_t obj;
    int64_t origin;
    int64_t loc;
    int64_t dx;
    int64_t dy;
    int64_t pc_obj;
    int rot;
    tig_art_id_t art_id;

    // Randomly choose whether monsters approach from the left or from above.
    if (random_between(1, 100) < 51) {
        // TODO: Check.
        origin = LOCATION_MAKE(wmap_rnd_loc_x - 6, wmap_rnd_loc_y);
        wmap_rnd_spawn_direction = 6;
    } else {
        // TODO: Check.
        origin = LOCATION_MAKE(wmap_rnd_loc_x, wmap_rnd_loc_y - 6);
        wmap_rnd_spawn_direction = 2;
    }

    for (index = 0; index < 5; index++) {
        if (entry->critter_basic_prototype[index] == 0) {
            break;
        }

        for (k = 0; k < wmap_rnd_critter_spawn_counts[index]; k++) {
            // Calculate the spawn position for this individual monster.
            // TODO: Check.
            dx = LOCATION_GET_X(origin);
            dy = LOCATION_GET_Y(origin);
            wmap_rnd_spawn_position_offset(k, &dx, &dy);

            loc = LOCATION_MAKE(dx, dy);
            wmap_rnd_encounter_build_object(entry->critter_basic_prototype[index], loc, &obj);

            // If the ideal spawn tile is blocked, try to find a nearby clear
            // tile (range 6). Destroy the monster if none can be found.
            if (tile_is_blocking(loc, 0)) {
                pc_obj = player_get_local_pc_obj();
                if (!sub_4F4E40(pc_obj, 6, &loc) || tile_is_blocking(loc, false)) {
                    object_destroy(obj);
                    obj = OBJ_HANDLE_NULL;
                } else {
                    object_move_to_location(obj, loc, 0, 0);
                }
            }

            // Destroy the monster if another PC or NPC already occupies the
            // chosen tile.
            object_list_location(loc, OBJ_TM_PC | OBJ_TM_NPC, &objects);
            node = objects.head;
            while (node != NULL) {
                if (node->obj != obj) {
                    object_destroy(obj);
                    obj = OBJ_HANDLE_NULL;
                    break;
                }
                node = node->next;
            }
            object_list_destroy(&objects);

            // Orient the spawned monster to face toward the player's location.
            if (obj != OBJ_HANDLE_NULL) {
                rot = location_rot(loc, wmap_rnd_loc);
                art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
                art_id = tig_art_id_rotation_set(art_id, rot);
                object_set_current_aid(obj, art_id);
            }
        }
    }
}

/**
 * Computes the tile offset for the `index`-th monster within a spawn group,
 * advancing `*dx_ptr` and `*dy_ptr` to spread monsters across adjacent tiles
 * rather than stacking them all on the same tile.
 *
 * TODO: Check.
 *
 * 0x5594E0
 */
void wmap_rnd_spawn_position_offset(int index, int64_t* dx_ptr, int64_t* dy_ptr)
{
    int v1;

    if (index != 0) {
        v1 = (index - 1) / 3 + 1;
        *dx_ptr = LOCATION_MAKE(LOCATION_GET_X(*dx_ptr) + 1, LOCATION_GET_Y(*dx_ptr) + 1);

        v1 = index / 3;
        *dy_ptr = LOCATION_MAKE(LOCATION_GET_X(*dy_ptr) + v1, LOCATION_GET_Y(*dy_ptr) + v1);
    } else {
        *dy_ptr = LOCATION_MAKE(LOCATION_GET_X(*dy_ptr) + 1, LOCATION_GET_Y(*dy_ptr) + 1);
    }
}

/**
 * Creates a world-map encounter object of the given prototype at `loc` and
 * returns a handle to it via `*obj_ptr`.
 *
 * `name` is the basic prototype identifier passed to the proto lookup
 * function to obtain the full prototype object before instantiation.
 *
 * 0x559550
 */
void wmap_rnd_encounter_build_object(int name, int64_t loc, int64_t* obj_ptr)
{
    int64_t proto_obj;
    int64_t obj;

    proto_obj = sub_4685A0(name);
    if (!object_create(proto_obj, loc, &obj)) {
        tig_debug_printf("wmap_rnd_encounter_build_object: ERROR: object_create failed!\n");
        exit(EXIT_FAILURE);
    }

    if (obj_ptr != NULL) {
        *obj_ptr = obj;
    }
}

/**
 * Called when a random encounter timeevent occurs.
 *
 * 0x5595B0
 */
bool wmap_rnd_timeevent_process(TimeEvent* timeevent)
{
    int64_t loc;

    (void)timeevent;

    // Reschedule before processing so the next event is always queued even
    // if this one triggers an encounter.
    wmap_rnd_schedule();

    // Random encounters are only possible while navigating the Arcanum worldmap
    // (that is, not dungeons, sewers, castles, Void, etc.), and not in the
    // "civilized" areas.
    if (map_current_map() == map_by_type(MAP_TYPE_START_MAP)
        && area_of_object(player_get_local_pc_obj()) == AREA_UNKNOWN) {
        // Retrieve player's world location.
        wmap_ui_get_current_location(&loc);

        // Check if encounter is triggered.
        if (wmap_rnd_check(loc)) {
            if (wmap_ui_is_created()) {
                // The player
                wmap_ui_close();
                wmap_ui_encounter_start();
            } else if (sleep_ui_is_active()) {
                sleep_ui_close();
            }
        }
    }

    return true;
}

/**
 * Schedules the next random encounter time event to fire after a random
 * delay of 300 to 700 in-game minutes.
 *
 * 0x559640
 */
void wmap_rnd_schedule()
{
    TimeEvent timeevent;
    DateTime datetime;

    timeevent.type = TIMEEVENT_TYPE_RANDOM_ENCOUNTER;
    sub_45A950(&datetime, 60000 * random_between(300, 700));
    timeevent_add_delay(&timeevent, &datetime);
}
