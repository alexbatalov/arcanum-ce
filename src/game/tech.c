#include "game/tech.h"

#include "game/effect.h"
#include "game/gamelib.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/object.h"
#include "game/player.h"
#include "game/stat.h"
#include "game/ui.h"

// TODO: Refactor.
#define SEVENTEEN 17

static int tech_degree_set(int64_t obj, int tech, int value);

/**
 * Defines cost (in character points) of picking appropriate tech degree.
 *
 * See `tech_degree_cost_get`.
 *
 * 0x5B5124
 */
static int tech_degree_cost_tbl[DEGREE_COUNT] = {
    /*     DEGREE_LAYMAN */ 0,
    /*     DEGREE_NOVICE */ 1,
    /*  DEGREE_ASSISTANT */ 1,
    /*  DEGREE_ASSOCIATE */ 1,
    /* DEGREE_TECHNICIAN */ 1,
    /*   DEGREE_ENGINEER */ 1,
    /*  DEGREE_PROFESSOR */ 1,
    /*  DEGREE_DOCTORATE */ 1,
};

/**
 * Defines the effective level (in percentage points) of appropriate tech
 * degree.
 *
 * See `tech_degree_level_get`.
 *
 * 0x5B5144
 */
static int tech_degree_level_tbl[DEGREE_COUNT] = {
    /*     DEGREE_LAYMAN */ 0,
    /*     DEGREE_NOVICE */ 10,
    /*  DEGREE_ASSISTANT */ 20,
    /*  DEGREE_ASSOCIATE */ 35,
    /* DEGREE_TECHNICIAN */ 50,
    /*   DEGREE_ENGINEER */ 65,
    /*  DEGREE_PROFESSOR */ 80,
    /*  DEGREE_DOCTORATE */ 100,
};

/**
 * Defines the minimum intelligence required to obtain appropriate tech degree.
 *
 * See `tech_degree_min_intelligence_get`.
 *
 * 0x5B5164
 */
static int tech_degree_min_intelligence_tbl[DEGREE_COUNT] = {
    /*     DEGREE_LAYMAN */ 0,
    /*     DEGREE_NOVICE */ 5,
    /*  DEGREE_ASSISTANT */ 8,
    /*  DEGREE_ASSOCIATE */ 11,
    /* DEGREE_TECHNICIAN */ 13,
    /*   DEGREE_ENGINEER */ 15,
    /*  DEGREE_PROFESSOR */ 17,
    /*  DEGREE_DOCTORATE */ 19,
};

/**
 * 0x5F84A8
 */
static char* tech_degree_names[DEGREE_COUNT];

/**
 * 0x5F84C8
 */
static char* tech_degree_descriptions[TECH_COUNT][DEGREE_COUNT];

/**
 * 0x5F85C8
 */
static mes_file_handle_t tech_mes_file;

/**
 * 0x5F85CC
 */
static char* tech_discipline_descriptions[TECH_COUNT];

/**
 * 0x5F85EC
 */
static char* tech_discipline_names[TECH_COUNT];

/**
 * Called when the game is initialized.
 *
 * 0x4AFCD0
 */
bool tech_init(GameInitInfo* init_info)
{
    MesFileEntry mes_file_entry;
    int tech;
    int degree;

    (void)init_info;

    // Load the tech message file (required).
    if (!mes_load("mes\\tech.mes", &tech_mes_file)) {
        return false;
    }

    // Load discipline names.
    for (tech = 0; tech < TECH_COUNT; tech++) {
        mes_file_entry.num = tech;
        mes_get_msg(tech_mes_file, &mes_file_entry);
        tech_discipline_names[tech] = mes_file_entry.str;
    }

    // Load degree names.
    for (degree = 0; degree < DEGREE_COUNT; degree++) {
        mes_file_entry.num = degree + TECH_COUNT;
        mes_get_msg(tech_mes_file, &mes_file_entry);
        tech_degree_names[degree] = mes_file_entry.str;
    }

    // Load discipline descriptions.
    for (tech = 0; tech < TECH_COUNT; tech++) {
        mes_file_entry.num = tech + TECH_COUNT + DEGREE_COUNT;
        mes_get_msg(tech_mes_file, &mes_file_entry);
        tech_discipline_descriptions[tech] = mes_file_entry.str;
    }

    // Load discipline-degree descriptions matrix.
    //
    // NOTE: Original code is slightly different. It does not use nested loop,
    // instead it simply runs 64 iterations, implying `tech_degree_descriptions`
    // is one-dimensional array. Making it two-dimensional slightly increase
    // code readability.
    for (tech = 0; tech < TECH_COUNT; tech++) {
        for (degree = 0; degree < DEGREE_COUNT; degree++) {
            mes_file_entry.num = tech * DEGREE_COUNT + degree + 24;
            mes_get_msg(tech_mes_file, &mes_file_entry);
            tech_degree_descriptions[tech][degree] = mes_file_entry.str;
        }
    }

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4AFDD0
 */
void tech_exit(void)
{
    mes_unload(tech_mes_file);
}

/**
 * Set default tech values for a critter.
 *
 * 0x4AFDE0
 */
void tech_set_defaults(int64_t obj)
{
    int tech;

    for (tech = 0; tech < TECH_COUNT; tech++) {
        obj_arrayfield_uint32_set(obj, OBJ_F_CRITTER_SPELL_TECH_IDX, tech + SEVENTEEN, 0);
    }
}

/**
 * Retrieves the name of a tech discipline.
 *
 * 0x4AFE10
 */
char* tech_discipline_name_get(int tech)
{
    return tech_discipline_names[tech];
}

/**
 * Retrieves the description of a tech discipline.
 *
 * 0x4AFE20
 */
char* tech_discipline_description_get(int tech)
{
    return tech_discipline_descriptions[tech];
}

/**
 * Retrieves the name of a degree.
 *
 * 0x4AFE30
 */
char* tech_degree_name_get(int degree)
{
    return tech_degree_names[degree];
}

/**
 * Retrieves the description of a degree in a given tech.
 *
 * 0x4AFE40
 */
char* tech_degree_description_get(int degree, int tech)
{
    return tech_degree_descriptions[tech][degree];
}

/**
 * Retrieves the current degree of a tech discipline for a critter.
 *
 * 0x4AFE60
 */
int tech_degree_get(int64_t obj, int tech)
{
    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    return obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_SPELL_TECH_IDX, SEVENTEEN + tech);
}

/**
 * Increments the degree of a tech discipline for a critter.
 *
 * 0x4AFEC0
 */
int tech_degree_inc(int64_t obj, int tech)
{
    int degree;
    int cost;
    int tech_points;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Get the current degree.
    degree = tech_degree_get(obj, tech);

    if (tig_net_is_active()
        && !tig_net_is_host()
        && !multiplayer_is_locked()) {
        return degree;
    }

    // Check if the degree can be incremented.
    if (degree + 1 >= DEGREE_COUNT) {
        return degree;
    }

    // Check if the critter has sufficient intelligence for the next degree.
    if (tech_degree_min_intelligence_get(degree + 1) > stat_level_get(obj, STAT_INTELLIGENCE)) {
        return degree;
    }

    // Increase tech points.
    cost = tech_degree_cost_get(degree + 1);
    tech_points = stat_base_get(obj, STAT_TECH_POINTS);
    stat_base_set(obj, STAT_TECH_POINTS, tech_points + cost);

    return tech_degree_set(obj, tech, degree + 1);
}

/**
 * Sets the degree of a tech discipline for an object.
 *
 * 0x4AFF90
 */
int tech_degree_set(int64_t obj, int tech, int value)
{
    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    mp_obj_arrayfield_uint32_set(obj, OBJ_F_CRITTER_SPELL_TECH_IDX, SEVENTEEN + tech, value);

    return value;
}

/**
 * Decrements the degree of a tech discipline for an object.
 *
 * 0x4AFFF0
 */
int tech_degree_dec(int64_t obj, int tech)
{
    int degree;
    int cost;
    int tech_points;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Get the current degree.
    degree = tech_degree_get(obj, tech);

    if (tig_net_is_active()
        && !tig_net_is_host()
        && !multiplayer_is_locked()) {
        return degree;
    }

    // Check if the degree can be decremented.
    if (degree <= 0) {
        return degree;
    }

    // Reduce tech points.
    cost = tech_degree_cost_get(degree);
    tech_points = stat_base_get(obj, STAT_TECH_POINTS);
    stat_base_set(obj, STAT_TECH_POINTS, tech_points - cost);

    return tech_degree_set(obj, tech, degree - 1);
}

/**
 * Retrieves the cost of advancing to a specific tech degree.
 *
 * 0x4B00A0
 */
int tech_degree_cost_get(int degree)
{
    return tech_degree_cost_tbl[degree];
}

/**
 * Calculates the effective tech level for a discipline, considering
 * intelligence and effects.
 *
 * 0x4B00B0
 */
int tech_degree_level_get(int64_t obj, int tech)
{
    int degree;
    int intelligence;

    // Get the current degree and intelligence.
    degree = tech_degree_get(obj, tech);

    // Reduce degree if intelligence is below the minimum required.
    intelligence = stat_level_get(obj, STAT_INTELLIGENCE);
    while (intelligence < tech_degree_min_intelligence_get(degree)) {
        degree--;
    }

    // Apply effects.
    return effect_adjust_tech_level(obj, tech, tech_degree_level_tbl[degree]);
}

/**
 * Retrieves the minimum intelligence required for a tech degree.
 *
 * 0x4B0110
 */
int tech_degree_min_intelligence_get(int degree)
{
    return tech_degree_min_intelligence_tbl[degree];
}

/**
 * Grants a player character schematic from a written object.
 *
 * 0x4B0120
 */
void tech_learn_schematic(int64_t pc_obj, int64_t written_obj)
{
    int schematic;
    int cnt;
    int index;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    int player;

    // Get the schematic ID from the written object.
    schematic = obj_field_int32_get(written_obj, OBJ_F_WRITTEN_TEXT_START_LINE);

    // Check if the schematic is already known.
    cnt = obj_arrayfield_length_get(pc_obj, OBJ_F_PC_SCHEMATICS_FOUND_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(pc_obj, OBJ_F_PC_SCHEMATICS_FOUND_IDX, index) == schematic) {
            break;
        }
    }

    if (index >= cnt) {
        // Add the schematic to the player's known schematics.
        mp_obj_arrayfield_uint32_set(pc_obj, OBJ_F_PC_SCHEMATICS_FOUND_IDX, index, schematic);

        // Destroy the written object.
        object_destroy(written_obj);

        // Set up success message.
        mes_file_entry.num = 89; // "You have learned a new schematic!"
        mes_get_msg(tech_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_SCHEMATIC;
        ui_message.str = mes_file_entry.str;
        ui_message.field_8 = schematic;

        if (tig_net_is_active()) {
            player = multiplayer_find_slot_from_obj(pc_obj);
            if (player == -1) {
                return;
            }

            if (player != 0) {
                sub_4EDA60(&ui_message, player, 0);
                return;
            }
        }
    } else {
        // Set up failure message.
        mes_file_entry.num = 88; // "You already know this schematic."
        mes_get_msg(tech_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_SCHEMATIC;
        ui_message.str = mes_file_entry.str;
        ui_message.field_8 = schematic;

        if (tig_net_is_active()) {
            player = multiplayer_find_slot_from_obj(pc_obj);
            if (player == -1) {
                return;
            }

            if (player != 0) {
                sub_4EDA60(&ui_message, player, 0);
                return;
            }
        }
    }

    sub_460630(&ui_message);
}

/**
 * Checks if a critter's intelligence is sufficient for all obtained tech
 * degrees.
 *
 * This function is used to re-validate tech degree requirements when
 * intelligence is about to decrease.
 *
 * Returns `true` if all known obtained tech degrees' intelligence requirements
 * are met, `false` otherwise.
 *
 * 0x4B02B0
 */
bool tech_check_intelligence(int64_t obj, int intelligence)
{
    int tech;
    int degree;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return true;
    }

    // Check each tech discipline's degree against the intelligence.
    for (tech = 0; tech < TECH_COUNT; tech++) {
        degree = tech_degree_get(obj, tech);
        if (tech_degree_min_intelligence_get(degree) > intelligence) {
            return false;
        }
    }

    return true;
}

/**
 * Retrieves the schematic learned in a given tech at the given degree.
 *
 * 0x4B0320
 */
int tech_schematic_get(int tech, int degree)
{
    return 10 * degree + 200 * tech + 1990;
}
