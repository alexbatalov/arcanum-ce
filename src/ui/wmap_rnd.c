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

typedef struct WmapRndEncounterChartEntry {
    /* 0000 */ int64_t loc;
    /* 0008 */ int64_t radius;
    /* 0010 */ int value;
} WmapRndEncounterChartEntry;

typedef struct WmapRndEncounterChart {
    /* 0000 */ WmapRndEncounterChartEntry* entries;
    /* 0004 */ int num_entries;
} WmapRndEncounterChart;

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

typedef struct WmapRndEncounterTable {
    /* 0000 */ int num_entries;
    /* 0004 */ WmapRndEncounterTableEntry* entries;
} WmapRndEncounterTable;

typedef struct WmapRndSaveInfo {
    /* 0000 */ int trigger_cnt;
    /* 0004 */ int message_num;
} WmapRndSaveInfo;

// Serializeable.
static_assert(sizeof(WmapRndSaveInfo) == 0x8, "wrong size");

static bool wmap_rnd_terrain_clear(uint16_t tid);
static bool sub_558A40(MesFileEntry* mes_file_entry, int cnt, int* value_ptr);
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
static bool sub_558F30(WmapRndEncounterChart* chart, int64_t loc, int* value_ptr);
static int wmap_rnd_determine_terrain(int64_t loc);
static bool wmap_rnd_encounter_check();
static bool wmap_rnd_encounter_entry_check(WmapRndEncounterTableEntry* entry);
static int wmap_rnd_encounter_total_frequency(WmapRndEncounterTable* table);
static int wmap_rnd_encounter_entry_total_monsters(WmapRndEncounterTableEntry* entry);
static void sub_559260(WmapRndEncounterTableEntry* entry);
static void sub_5594E0(int a1, int64_t* dx_ptr, int64_t* dy_ptr);
static void wmap_rnd_encounter_build_object(int name, int64_t loc, int64_t* obj_ptr);

// 0x5C79A0
static const char* off_5C79A0[] = {
    "none",
    "easy",
    "average",
    "powerful",
};

// 0x5C79B0
static const char* off_5C79B0[] = {
    "First:",
    "Second:",
    "Third:",
    "Fourth:",
    "Fifth:",
};

// 0x5C79C4
static int dword_5C79C4[TERRAIN_TYPE_COUNT] = {
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

// 0x64C728
static WmapRndEncounterChart wmap_rnd_encounter_chart;

// 0x64C730
static mes_file_handle_t wmap_rnd_mes_file;

// 0x64C738
static int wmap_rnd_frequency;

// 0x64C73C
static bool wmap_rnd_night;

// 0x64C740
static int wmap_rnd_power;

// 0x64C744
static WmapTerrainType wmap_rnd_terrain_type;

// 0x64C748
static int wmap_rnd_encounter;

// 0x64C74C
static int dword_64C74C[5];

// 0x64C760
static int64_t wmap_rnd_loc;

// 0x64C768
static int64_t wmap_rnd_loc_x;

// 0x64C770
static int64_t wmap_rnd_loc_y;

// 0x64C778
static int dword_64C778;

// 0x64C780
static WmapRndEncounterChart wmap_rnd_frequency_chart;

// 0x64C788
static WmapRndEncounterChart wmap_rnd_power_chart;

// 0x64C790
static bool wmap_rnd_initialized;

// 0x64C794
static bool wmap_rnd_disabled;

// 0x64C798
static WmapRndEncounterTable* wmap_rnd_encounter_tables;

// 0x64C79C
static int wmap_rnd_num_encounter_tables;

// 0x5581B0
bool wmap_rnd_init(GameInitInfo* init_info)
{
    (void)init_info;

    return true;
}

// 0x5581C0
void wmap_rnd_exit()
{
}

// 0x5581D0
void wmap_rnd_reset()
{
}

// 0x5581E0
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

    if (!wmap_rnd_encounter_chart_parse(&wmap_rnd_frequency_chart, 10000, NULL)) {
        tig_debug_println("Disabling random encounters because of bad message file.");
        mes_unload(wmap_rnd_mes_file);
        wmap_rnd_initialized = true;
        return true;
    }

    if (!wmap_rnd_encounter_chart_parse(&wmap_rnd_power_chart, 20000, off_5C79A0)) {
        tig_debug_println("Disabling random encounters because of bad message file.");
        wmap_rnd_encounter_chart_exit(&wmap_rnd_frequency_chart);
        mes_unload(wmap_rnd_mes_file);
        wmap_rnd_initialized = true;
        return true;
    }

    if (!wmap_rnd_encounter_chart_parse(&wmap_rnd_encounter_chart, 30000, NULL)) {
        tig_debug_println("Disabling random encounters because of bad message file.");
        wmap_rnd_encounter_chart_exit(&wmap_rnd_frequency_chart);
        wmap_rnd_encounter_chart_exit(&wmap_rnd_power_chart);
        mes_unload(wmap_rnd_mes_file);
        wmap_rnd_initialized = true;
        return true;
    }

    mes_file_entry.num = 49999;
    mes_get_msg(wmap_rnd_mes_file, &mes_file_entry);

    num_tables = atoi(mes_file_entry.str) + 54;
    wmap_rnd_encounter_tables = MALLOC(sizeof(*wmap_rnd_encounter_tables) * num_tables);

    for (table_idx = 0; table_idx < num_tables; table_idx++) {
        table = &(wmap_rnd_encounter_tables[table_idx]);
        wmap_rnd_encounter_table_init(table);

        mes_file_entry.num = 50000 + 100 * table_idx;
        if (!sub_558A40(&mes_file_entry, 100, &num_entries)) {
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
            entry->message_num = mes_file_entry.num;

            mes_get_msg(wmap_rnd_mes_file, &mes_file_entry);
            str = mes_file_entry.str;

            tig_str_parse_value(&str, &(entry->frequency));

            if (!wmap_rnd_parse_critters(&str, entry)) {
                tig_debug_printf("Error: Random encounter table has no: prototype at line: %d.\n", mes_file_entry.num);
                tig_debug_println("Disabling random encounters because of bad message file.");
                wmap_rnd_encounter_table_clear();
                wmap_rnd_encounter_chart_clear();
                mes_unload(wmap_rnd_mes_file);
                wmap_rnd_initialized = true;
                return true;
            }

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

            if (tig_str_parse_named_value(&str, "TriggerCount:", &value)) {
                entry->max_trigger_cnt = value;
            }
        }
    }

    mes_unload(wmap_rnd_mes_file);
    wmap_rnd_initialized = true;
    return true;
}

// 0x558700
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

// 0x558730
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
        for (table_idx = 0; table_idx < wmap_rnd_num_encounter_tables; table_idx++) {
            num_entries += wmap_rnd_encounter_tables[table_idx].num_entries;
        }

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

    if (tig_file_fwrite(&num_entries, sizeof(num_entries), 1, stream) != 1) {
        tig_debug_println("Error writing random encounter save data.");
        if (save_info != NULL) {
            FREE(save_info);
        }
        return false;
    }

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

// 0x558880
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

// 0x5589D0
void wmap_rnd_disable()
{
    wmap_rnd_disabled = true;
}

// 0x5589E0
bool wmap_rnd_terrain_clear(uint16_t tid)
{
    int v1;
    int v2;

    v1 = sub_4E8DC0(tid);
    v2 = sub_4E8DD0(tid);
    if (!TERRAIN_TYPE_IS_VALID(v1) || !TERRAIN_TYPE_IS_VALID(v2)) {
        tig_debug_println("Error:  Unknown terrain encountered in wmap_rnd_terrain_clear");
        return false;
    }

    if (dword_5C79C4[v1] == WMAP_TERRAIN_TYPE_INVALID
        || dword_5C79C4[v2] == WMAP_TERRAIN_TYPE_INVALID) {
        return false;
    }

    return true;
}

// 0x558A40
bool sub_558A40(MesFileEntry* mes_file_entry, int cnt, int* value_ptr)
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

// 0x558AC0
void wmap_rnd_encounter_table_init(WmapRndEncounterTable* table)
{
    table->num_entries = 0;
}

// 0x558AD0
void wmap_rnd_encounter_table_entry_init(WmapRndEncounterTableEntry* entry)
{
    entry->critter_basic_prototype[0] = 0;
    entry->min_level = 0;
    entry->max_level = 32000;
    entry->global_flag_num = -1;
    entry->max_trigger_cnt = -1;
    entry->trigger_cnt = 0;
}

// 0x558AF0
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

// 0x558B50
void wmap_rnd_encounter_chart_clear()
{
    wmap_rnd_encounter_chart_exit(&wmap_rnd_frequency_chart);
    wmap_rnd_encounter_chart_exit(&wmap_rnd_power_chart);
    wmap_rnd_encounter_chart_exit(&wmap_rnd_encounter_chart);
}

// 0x558B80
bool wmap_rnd_encounter_chart_parse(WmapRndEncounterChart* chart, int num, const char** value_list)
{
    MesFileEntry mes_file_entry;
    int cnt;
    int index;
    char* str;

    mes_file_entry.num = num;
    if (!sub_558A40(&mes_file_entry, 10000, &cnt)) {
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
        wmap_rnd_encounter_chart_entry_parse(&str, &(chart->entries[index]));

        if (value_list != NULL) {
            SDL_strlwr(str);
            tig_str_match_str_to_list(&str, value_list, 4, &(chart->entries[index].value));
        } else {
            tig_str_parse_value(&str, &(chart->entries[index].value));
        }
    }

    wmap_rnd_encounter_chart_sort(chart);

    return true;
}

// 0x558C90
void wmap_rnd_encounter_chart_entry_parse(char** str, WmapRndEncounterChartEntry* entry)
{
    int64_t x;
    int64_t y;

    tig_str_parse_value_64(str, &x);
    tig_str_parse_value_64(str, &y);
    entry->loc = LOCATION_MAKE(x, y);
    tig_str_parse_value_64(str, &(entry->radius));
}

// 0x558CE0
void wmap_rnd_encounter_chart_exit(WmapRndEncounterChart* chart)
{
    if (chart->num_entries != 0) {
        FREE(chart->entries);
        chart->num_entries = 0;
    }
}

// 0x558D00
void wmap_rnd_encounter_chart_sort(WmapRndEncounterChart* chart)
{
    qsort(chart->entries, chart->num_entries, sizeof(*chart->entries), wmap_rnd_encounter_entry_compare);
}

// 0x558D20
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

// 0x558D40
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

// 0x558DE0
bool wmap_rnd_check(int64_t loc)
{
    int hour;

    if (wmap_rnd_disabled) {
        return false;
    }

    if (!wmap_rnd_initialized) {
        return false;
    }

    wmap_rnd_loc = loc;
    wmap_rnd_loc_x = LOCATION_GET_X(loc);
    wmap_rnd_loc_y = LOCATION_GET_Y(loc);

    if (!sub_558F30(&wmap_rnd_frequency_chart, loc, &wmap_rnd_frequency)) {
        wmap_rnd_frequency = 5;
    }

    if (sleep_ui_is_active()) {
        wmap_rnd_frequency *= 5;
    }

    if (random_between(1, 100) > wmap_rnd_frequency) {
        return false;
    }

    if (sub_558F30(&wmap_rnd_power_chart, loc, &wmap_rnd_power)) {
        if (wmap_rnd_power == 0) {
            return false;
        }
    } else {
        wmap_rnd_power = 2;
    }

    hour = datetime_current_hour();
    wmap_rnd_night = hour < 6 || hour >= 18;

    if (townmap_get(sector_id_from_loc(loc))) {
        return false;
    }

    wmap_rnd_terrain_type = wmap_rnd_determine_terrain(loc);
    if (wmap_rnd_terrain_type == WMAP_TERRAIN_TYPE_INVALID) {
        return false;
    }

    if (!sub_558F30(&wmap_rnd_encounter_chart, loc, &wmap_rnd_encounter)) {
        wmap_rnd_encounter = -1;
    }

    if (!wmap_rnd_encounter_check()) {
        return false;
    }

    return true;
}

// 0x558F30
bool sub_558F30(WmapRndEncounterChart* chart, int64_t loc, int* value_ptr)
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

// 0x558FB0
int wmap_rnd_determine_terrain(int64_t loc)
{
    uint64_t sec;
    uint16_t tid;
    int terrain_base_type;

    sec = sector_id_from_loc(loc);
    tid = sub_4E87F0(sec);
    if (!wmap_rnd_terrain_clear(tid)) {
        return 0;
    }

    terrain_base_type = sub_4E8DC0(tid);
    if (terrain_base_type < 0 || terrain_base_type >= 19) {
        tig_debug_println("Error:  Unknown terrain base encountered inwmap_rnd_determine_terrain");
        return 0;
    }

    return dword_5C79C4[terrain_base_type];
}

// 0x559010
bool wmap_rnd_encounter_check()
{
    int v1;
    WmapRndEncounterTable* table;
    int frequency;
    int index;
    int v2 = -1;

    if (wmap_rnd_encounter == -1) {
        wmap_rnd_encounter = wmap_rnd_night + 2 * wmap_rnd_power + 6 * wmap_rnd_terrain_type - 8;
    }

    tig_debug_printf("WMap_Rnd: Random Encounter Fired: Table: %d.\n", wmap_rnd_encounter);

    if (wmap_rnd_encounter >= wmap_rnd_num_encounter_tables) {
        tig_debug_printf("WmapRnd: wmap_rnd_encounter_check: ERROR: Table %d was out of bounds!\n", wmap_rnd_encounter);
        return false;
    }

    table = &(wmap_rnd_encounter_tables[wmap_rnd_encounter]);
    v1 = wmap_rnd_encounter_total_frequency(table);
    if (v1 <= 0) {
        return false;
    }

    frequency = random_between(1, v1);

    for (index = 0; index < table->num_entries; index++) {
        if (wmap_rnd_encounter_entry_check(&(table->entries[index]))) {
            v2 = index;
            if (frequency < table->entries[index].frequency) {
                break;
            }
            frequency -= table->entries[index].frequency;
        }
    }

    if (index >= table->num_entries) {
        if (v2 == -1) {
            return false;
        }
        index = v2;
    }

    table->entries[index].trigger_cnt++;

    if (wmap_rnd_encounter_entry_total_monsters(&(table->entries[index])) == 0) {
        tig_debug_println("WmapRnd: Total monsters is zero.");
        return false;
    }

    sub_559260(&(table->entries[index]));

    return true;
}

// 0x559150
bool wmap_rnd_encounter_entry_check(WmapRndEncounterTableEntry* entry)
{
    int64_t pc_obj;
    int level;

    pc_obj = player_get_local_pc_obj();
    if (pc_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    level = stat_level_get(pc_obj, STAT_LEVEL);
    if (level < entry->min_level
        || level > entry->max_level) {
        return false;
    }

    if (entry->global_flag_num != -1 && !script_global_flag_get(entry->global_flag_num)) {
        return false;
    }

    if (entry->max_trigger_cnt > -1 && entry->max_trigger_cnt <= entry->trigger_cnt) {
        return false;
    }

    return true;
}

// 0x5591B0
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

// 0x559200
int wmap_rnd_encounter_entry_total_monsters(WmapRndEncounterTableEntry* entry)
{
    int monsters = 0;
    int index;

    for (index = 0; index < 5; index++) {
        if (entry->critter_basic_prototype[index] == 0) {
            break;
        }

        dword_64C74C[index] = entry->critter_min_cnt[index] == entry->critter_max_cnt[index]
            ? entry->critter_min_cnt[index]
            : random_between(entry->critter_min_cnt[index], entry->critter_max_cnt[index]);
        monsters += dword_64C74C[index];
    }

    return monsters;
}

// 0x559260
void sub_559260(WmapRndEncounterTableEntry* entry)
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

    if (random_between(1, 100) < 51) {
        // TODO: Check.
        origin = LOCATION_MAKE(wmap_rnd_loc_x - 6, wmap_rnd_loc_y);
        dword_64C778 = 6;
    } else {
        // TODO: Check.
        origin = LOCATION_MAKE(wmap_rnd_loc_x, wmap_rnd_loc_y - 6);
        dword_64C778 = 2;
    }

    for (index = 0; index < 5; index++) {
        if (entry->critter_basic_prototype[index] == 0) {
            break;
        }

        for (k = 0; k < dword_64C74C[index]; k++) {
            // TODO: Check.
            dx = LOCATION_GET_X(origin);
            dy = LOCATION_GET_Y(origin);
            sub_5594E0(k, &dx, &dy);
            loc = LOCATION_MAKE(dx, dy);
            wmap_rnd_encounter_build_object(entry->critter_basic_prototype[index], loc, &obj);
            if (tile_is_blocking(loc, 0)) {
                pc_obj = player_get_local_pc_obj();
                if (!sub_4F4E40(pc_obj, 6, &loc) || tile_is_blocking(loc, 0)) {
                    object_destroy(obj);
                    obj = OBJ_HANDLE_NULL;
                } else {
                    object_move_to_location(obj, loc, 0, 0);
                }
            }

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

            if (obj != OBJ_HANDLE_NULL) {
                rot = location_rot(loc, wmap_rnd_loc);
                art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
                art_id = tig_art_id_rotation_set(art_id, rot);
                object_set_current_aid(obj, art_id);
            }
        }
    }
}

// TODO: Check.
//
// 0x5594E0
void sub_5594E0(int a1, int64_t* dx_ptr, int64_t* dy_ptr)
{
    int v1;

    if (a1 != 0) {
        v1 = (a1 - 1) / 3 + 1;
        *dx_ptr = LOCATION_MAKE(LOCATION_GET_X(*dx_ptr) + 1, LOCATION_GET_Y(*dx_ptr) + 1);

        v1 = a1 / 3;
        *dy_ptr = LOCATION_MAKE(LOCATION_GET_X(*dy_ptr) + v1, LOCATION_GET_Y(*dy_ptr) + v1);
    } else {
        *dy_ptr = LOCATION_MAKE(LOCATION_GET_X(*dy_ptr) + 1, LOCATION_GET_Y(*dy_ptr) + 1);
    }
}

// 0x559550
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

// 0x5595B0
bool wmap_rnd_timeevent_process(TimeEvent* timeevent)
{
    int64_t loc;

    (void)timeevent;

    wmap_rnd_schedule();

    if (map_current_map() == map_by_type(MAP_TYPE_START_MAP)
        && area_of_object(player_get_local_pc_obj()) == AREA_UNKNOWN) {
        wmap_ui_get_current_location(&loc);
        if (wmap_rnd_check(loc)) {
            if (wmap_ui_is_created()) {
                wmap_ui_close();
                wmap_ui_encounter_start();
            } else if (sleep_ui_is_active()) {
                sleep_ui_close();
            }
        }
    }

    return true;
}

// 0x559640
void wmap_rnd_schedule()
{
    TimeEvent timeevent;
    DateTime datetime;

    timeevent.type = TIMEEVENT_TYPE_RANDOM_ENCOUNTER;
    sub_45A950(&datetime, 60000 * random_between(300, 700));
    timeevent_add_delay(&timeevent, &datetime);
}
