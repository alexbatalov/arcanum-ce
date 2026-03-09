#include "game/level.h"

#include <stdio.h>

#include "game/background.h"
#include "game/critter.h"
#include "game/mes.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/player.h"
#include "game/skill.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/tech.h"
#include "game/ui.h"

/**
 * Maximum level a character can achieve.
 *
 * This value can be increased, be sure to modify `stat_max_values` as well,
 * and populate `xp_level.mes` accordingly.
 */
#define LEVEL_MAX 51

/**
 * The size of log during auto level up.
 */
#define AUTO_LEVEL_LOG_SIZE 10

/**
 * The number of standard NPC auto-leveling scheme from `gamelevel.mes`.
 */
#define STANDARD_NPC_SCHEME 70

#define LEVEL_SCHEME_STATS_MAX 8
#define LEVEL_SCHEME_BASIC_SKILLS_MAX BASIC_SKILL_COUNT
#define LEVEL_SCHEME_TECH_SKILLS_MAX TECH_SKILL_COUNT
#define LEVEL_SCHEME_SPELLS_MAX COLLEGE_COUNT
#define LEVEL_SCHEME_TECH_MAX TECH_COUNT
#define LEVEL_SCHEME_MISC_MAX 2

typedef enum AutoLevelError {
    AUTO_LEVEL_OK,
    AUTO_LEVEL_INSUFFIICIENT_POINTS,
    AUTO_LEVEL_UPDATE_FAILED,
} AutoLevelError;

typedef int(MiscGetter)(int64_t obj);
typedef int(MiscSetter)(int64_t obj, int value);

static int calculate_bonus_character_points(int old_level, int new_level);
static void update_follower_level(int64_t follower_obj, int old_pc_level, int new_pc_level);
static bool auto_level_apply(int64_t obj, char* str);
static bool auto_level_process_rule(int64_t obj, const char* str);
static AutoLevelError auto_level_stat(int64_t obj, int stat, int value);
static AutoLevelError auto_level_basic_skill(int64_t obj, int skill, int value);
static AutoLevelError auto_level_tech_skill(int64_t obj, int skill, int score);
static AutoLevelError auto_level_spell(int64_t obj, int college, int score);
static AutoLevelError auto_level_tech(int64_t obj, int tech, int degree);
static AutoLevelError auto_level_misc(int64_t obj, int type, int score);
static void record_attribute_change(const char* str, int score);

/**
 * Array of stat tokens used in auto-leveling rule.
 *
 * 0x5B4CE8
 */
static const char* level_scheme_stats[LEVEL_SCHEME_STATS_MAX] = {
    /*     STAT_STRENGTH */ "st",
    /*    STAT_DEXTERITY */ "dx",
    /* STAT_CONSTITUTION */ "cn",
    /*       STAT_BEAUTY */ "be",
    /* STAT_INTELLIGENCE */ "in",
    /*   STAT_PERCEPTION */ "pe",
    /*    STAT_WILLPOWER */ "wp",
    /*     STAT_CHARISMA */ "ch",
};

/**
 * Array of basic skill tokens used in auto-leveling rule.
 *
 * 0x5B4D08
 */
static const char* level_scheme_basic_skills[LEVEL_SCHEME_BASIC_SKILLS_MAX] = {
    /*         BASIC_SKILL_BOW */ "bow",
    /*       BASIC_SKILL_DODGE */ "dodge",
    /*       BASIC_SKILL_MELEE */ "melee",
    /*    BASIC_SKILL_THROWING */ "throwing",
    /*    BASIC_SKILL_BACKSTAB */ "backstab",
    /* BASIC_SKILL_PICK_POCKET */ "pickpocket",
    /*    BASIC_SKILL_PROWLING */ "prowling",
    /*   BASIC_SKILL_SPOT_TRAP */ "spottrap",
    /*    BASIC_SKILL_GAMBLING */ "gambling",
    /*      BASIC_SKILL_HAGGLE */ "haggle",
    /*        BASIC_SKILL_HEAL */ "heal",
    /*  BASIC_SKILL_PERSUATION */ "persuasion",
};

/**
 * Array of tech skill tokens used in auto-leveling rule.
 *
 * 0x5B4D38
 */
static const char* level_scheme_tech_skills[LEVEL_SCHEME_TECH_SKILLS_MAX] = {
    /*       TECH_SKILL_REPAIR */ "repair",
    /*     TECH_SKILL_FIREARMS */ "firearms",
    /*   TECH_SKILL_PICK_LOCKS */ "picklock",
    /* TECH_SKILL_DISARM_TRAPS */ "armtrap",
};

/**
 * Array of spell college tokens used in auto-leveling rule.
 *
 * 0x5B4D48
 */
static const char* level_scheme_spells[LEVEL_SCHEME_SPELLS_MAX] = {
    /*        COLLEGE_CONVEYANCE */ "conveyance",
    /*        COLLEGE_DIVINATION */ "divination",
    /*               COLLEGE_AIR */ "air",
    /*             COLLEGE_EARTH */ "earth",
    /*              COLLEGE_FIRE */ "fire",
    /*             COLLEGE_WATER */ "water",
    /*             COLLEGE_FORCE */ "force",
    /*            COLLEGE_MENTAL */ "mental",
    /*              COLLEGE_META */ "meta",
    /*             COLLEGE_MORPH */ "morph",
    /*            COLLEGE_NATURE */ "nature",
    /* COLLEGE_NECROMANTIC_BLACK */ "necro_evil",
    /* COLLEGE_NECROMANTIC_WHITE */ "necro_good",
    /*          COLLEGE_PHANTASM */ "phantasm",
    /*         COLLEGE_SUMMONING */ "summoning",
    /*          COLLEGE_TEMPORAL */ "temporal",
};

/**
 * Array of tech discipline tokens used in auto-leveling rule.
 *
 * 0x5B4D88
 */
static const char* level_scheme_tech[LEVEL_SCHEME_TECH_MAX] = {
    /*    TECH_HERBOLOGY */ "anatomical",
    /*    TECH_CHEMISTRY */ "chemistry",
    /*     TECH_ELECTRIC */ "electric",
    /*   TECH_EXPLOSIVES */ "explosives",
    /*          TECH_GUN */ "gun_smithy",
    /*   TECH_MECHANICAL */ "mechanical",
    /*       TECH_SMITHY */ "smithy",
    /* TECH_THERAPEUTICS */ "therapeutics",
};

/**
 * Array of miscellaneous attribute tokens in auto-leveling rule.
 *
 * 0x5B4DA8
 */
static const char* level_scheme_misc[LEVEL_SCHEME_MISC_MAX] = {
    "maxhps",
    "maxfatigue",
};

/**
 * Array of function pointers to get effective levels for miscellaneous
 * attributes.
 *
 * 0x4A7ABE
 */
static MiscGetter* level_misc_attr_level_getters[LEVEL_SCHEME_MISC_MAX] = {
    object_hp_max,
    critter_fatigue_max,
};

/**
 * Array of function pointers to get base levels for miscellaneous attributes.
 *
 * 0x4A7AF3
 */
static MiscGetter* level_misc_attr_base_getters[LEVEL_SCHEME_MISC_MAX] = {
    object_hp_pts_get,
    critter_fatigue_pts_get,
};

/**
 * Array of function pointers to set base levels for miscellaneous attributes.
 *
 * 0x4A7AFE
 */
static MiscSetter* level_misc_attr_base_setters[LEVEL_SCHEME_MISC_MAX] = {
    object_hp_pts_set,
    critter_fatigue_pts_set,
};

/**
 * Buffers for storing logs related to auto level-up changes.
 *
 * 0x5F0E60
 */
static char auto_level_log_strs[AUTO_LEVEL_LOG_SIZE][MAX_STRING];

/**
 * Array of scores correspoding to auto level-up changes.
 *
 * 0x5F5C80
 */
static int* auto_level_log_values;

/**
 * "gamelevelname.mes"
 *
 * 0x5F5C84
 */
static mes_file_handle_t game_level_name_mes_file;

/**
 * "userlevelname.mes"
 *
 * 0x5F5C88
 */
static mes_file_handle_t user_level_name_mes_file;

/**
 * "gamelevel.mes"
 *
 * 0x5F5C8C
 */
static mes_file_handle_t game_level_mes_file;

/**
 * "userlevel.mes"
 *
 * 0x5F5C90
 */
static mes_file_handle_t user_level_mes_file;

/**
 * Array of experience points required for each level (up to `LEVEL_MAX`).
 *
 * 0x5F5C94
 */
static int* level_xp_tbl;

/**
 * "level.mes"
 *
 * 0x5F5C98
 */
static mes_file_handle_t level_mes_file;

/**
 * Number of auto level-up changes logged.
 *
 * 0x5F5C9C
 */
static int auto_level_log_size;

/**
 * Called when the game is initialized.
 *
 * 0x4A6620
 */
bool level_init(GameInitInfo* init_info)
{
    mes_file_handle_t xp_level_mes_file;
    MesFileEntry msg;
    int index;

    (void)init_info;

    // Load experience points from file (required).
    if (!mes_load("rules\\xp_level.mes", &xp_level_mes_file)) {
        return false;
    }

    level_xp_tbl = (int*)CALLOC(LEVEL_MAX, sizeof(*level_xp_tbl));

    // NOTE: Unclear why log strs have static storage duration, but associated
    // values are dynamically allocated on heap.
    auto_level_log_values = (int*)CALLOC(AUTO_LEVEL_LOG_SIZE, sizeof(*auto_level_log_values));

    // Populate experience points table for each level.
    for (index = 0; index < LEVEL_MAX; index++) {
        msg.num = index + 1;
        if (mes_search(xp_level_mes_file, &msg)) {
            level_xp_tbl[index] = atoi(msg.str);
        } else {
            level_xp_tbl[index] = 0;
        }
    }

    mes_unload(xp_level_mes_file);

    // Level 1 must always be 0 experience points, no matter what was specified
    // in the file. This requirement is also mentioned in `xp_level.mes` itself.
    level_xp_tbl[0] = 0;

    // Load system-defined advancement scheme rules (required).
    if (!mes_load("rules\\gamelevel.mes", &game_level_mes_file)) {
        FREE(level_xp_tbl);
        FREE(auto_level_log_values);
        return false;
    }

    // Load user-defined advancement scheme rules (required).
    if (!mes_load("rules\\userlevel.mes", &user_level_mes_file)) {
        mes_unload(game_level_mes_file);
        FREE(level_xp_tbl);
        FREE(auto_level_log_values);
        return false;
    }

    // Load system-defined advancement scheme names (required).
    if (!mes_load("mes\\gamelevelname.mes", &game_level_name_mes_file)) {
        mes_unload(user_level_mes_file);
        mes_unload(game_level_mes_file);
        FREE(level_xp_tbl);
        FREE(auto_level_log_values);
        return false;
    }

    // Load user-defined advancement scheme names (required).
    if (!mes_load("mes\\userlevelname.mes", &user_level_name_mes_file)) {
        mes_unload(game_level_name_mes_file);
        mes_unload(user_level_mes_file);
        mes_unload(game_level_mes_file);
        FREE(level_xp_tbl);
        FREE(auto_level_log_values);
        return false;
    }

    // Load UI messages (required).
    if (!mes_load("mes\\level.mes", &level_mes_file)) {
        mes_unload(user_level_name_mes_file);
        mes_unload(game_level_name_mes_file);
        mes_unload(user_level_mes_file);
        mes_unload(game_level_mes_file);
        FREE(level_xp_tbl);
        FREE(auto_level_log_values);
        return false;
    }

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4A6850
 */
void level_exit(void)
{
    mes_unload(game_level_mes_file);
    mes_unload(user_level_mes_file);
    mes_unload(game_level_name_mes_file);
    mes_unload(user_level_name_mes_file);
    mes_unload(level_mes_file);
    FREE(level_xp_tbl);
    FREE(auto_level_log_values);
}

/**
 * Calculates the experience points needed to reach the next level.
 *
 * The `obj` is implied to be a critter, but this is not enforced.
 *
 * 0x4A68B0
 */
int level_experience_points_to_next_level(int64_t obj)
{
    int level;

    level = stat_level_get(obj, STAT_LEVEL);

    // Clamp at maximum.
    if (level >= LEVEL_MAX) {
        return 999999;
    }

    return level_xp_tbl[level] - stat_level_get(obj, STAT_EXPERIENCE_POINTS);
}

/**
 * Calculates the percentage of experience points earned toward the next level.
 *
 * The `obj` is implied to be a critter, but this is not enforced.
 *
 * The returned value is scaled to 0-1000 range. For example, advancing from
 * level 2 (2100 xp) to level 3 (4600 xp) requires 2500 xp, at 4000 xp a PC
 * still needs 600 xp for the next level, which resolves to 76% (returned as
 * 760).
 *
 * 0x4A6900
 */
int level_progress_towards_next_level(int64_t obj)
{
    int level;
    int xp;

    level = stat_level_get(obj, STAT_LEVEL);

    // Clamp at maximum.
    if (level >= LEVEL_MAX) {
        return 0;
    }

    if (level != 0 && level_xp_tbl[level] != 0) {
        xp = level_experience_points_to_next_level(obj);
        if (xp >= 0) {
            return 1000 - 1000 * xp / (level_xp_tbl[level] - level_xp_tbl[level - 1]);
        }
    }

    // Something's wrong.
    return 999;
}

/**
 * Calculates the number of characters points for advancing from `old_level` to
 * `new_level`.
 *
 * The standard advacement scheme is one point for every level, plus one extra
 * point per every 5 levels.
 *
 * NOTE: This math could be easily changed, but doing so would definitely break
 * game balance even further. Be sure to watch
 * [The Arcanum One-Point Fiasco](https://www.youtube.com/watch?v=keTG-5WPccU)
 * by Tim Cain, before doing anything here.
 *
 * In case some one is brave enough to change it, be sure to update stats,
 * spells, skills, and tech costs in relevant modules.
 *
 * 0x4A6980
 */
int calculate_bonus_character_points(int old_level, int new_level)
{
    int points = 0;
    int level;

    for (level = old_level + 1; level <= new_level; level++) {
        points++;

        // Extra point every 5 levels.
        if ((level % 5) == 0) {
            points++;
        }
    }

    return points;
}

/**
 * Recalculates a player character's level based on their experience points.
 *
 * 0x4A69C0
 */
void level_recalc(int64_t pc_obj)
{
    UiMessage ui_message;
    int old_level;
    int cur_level;
    int max_level;
    int exp_to_next_level;
    ObjectList objects;
    ObjectNode* node;
    char str[MAX_STRING];
    int unspent_points;
    MesFileEntry mes_file_entry;

    // Make sure `pc_obj` is PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    // Get current and maximum levels.
    cur_level = stat_base_get(pc_obj, STAT_LEVEL);
    max_level = stat_level_max(pc_obj, STAT_LEVEL);
    if (cur_level >= max_level) {
        return;
    }

    // Check if PC got enough experience points for leveling up.
    exp_to_next_level = level_experience_points_to_next_level(pc_obj);
    if (exp_to_next_level > 0) {
        return;
    }

    // Set up UI message for level-up notification.
    ui_message.type = UI_MSG_TYPE_LEVEL;
    ui_message.str = str;

    old_level = cur_level;

    // Process level-ups until no more are possible or max level is reached.
    while (cur_level < max_level && exp_to_next_level <= 0) {
        cur_level++;

        if (cur_level == max_level) {
            // Max level reached - cap experience at the maximum level's requirement.
            stat_base_set(pc_obj, STAT_EXPERIENCE_POINTS, level_xp_tbl[cur_level - 1]);
        } else {
            // Update level.
            stat_base_set(pc_obj, STAT_LEVEL, cur_level);

            // Grant character points for level up.
            unspent_points = stat_base_get(pc_obj, STAT_UNSPENT_POINTS);
            unspent_points += calculate_bonus_character_points(cur_level - 1, cur_level);
            stat_base_set(pc_obj, STAT_UNSPENT_POINTS, unspent_points);

            ui_message.field_8 = cur_level;

            if (auto_level_scheme_get(pc_obj) != 0) {
                // Apply auto-leveling scheme.
                ui_message.field_C = -1;
                if (auto_level_apply(pc_obj, str)) {
                    mes_file_entry.num = 1; // "Your auto-level scheme has completed."
                    mes_get_msg(level_mes_file, &mes_file_entry);
                    strcat(str, " ");
                    strcat(str, mes_file_entry.str);
                }
            } else {
                // No auto-leveling scheme.
                ui_message.field_C = unspent_points;
                str[0] = '\0';
            }

            // Notify UI for local player characters.
            if (player_is_local_pc_obj(pc_obj)) {
                sub_460630(&ui_message);
                ui_toggle_primary_button(UI_PRIMARY_BUTTON_CHAR, true);
                ui_refresh_health_bar(pc_obj);
                ui_refresh_fatigue_bar(pc_obj);
            }

            exp_to_next_level = level_experience_points_to_next_level(pc_obj);
        }
    }

    // Level up current followers.
    object_list_all_followers(pc_obj, &objects);
    node = objects.head;
    while (node != NULL) {
        update_follower_level(node->obj, old_level, cur_level);
        node = node->next;
    }
    object_list_destroy(&objects);

    // Level up nearby followers there were ordered to wait.
    object_list_vicinity(pc_obj, OBJ_TM_NPC, &objects);
    node = objects.head;
    while (node != NULL) {
        if (!critter_is_dead(node->obj)
            && (obj_field_int32_get(node->obj, OBJ_F_NPC_FLAGS) & ONF_AI_WAIT_HERE) != 0
            && critter_pc_leader_get(node->obj) == pc_obj) {
            update_follower_level(node->obj, old_level, cur_level);
        }
        node = node->next;
    }
    object_list_destroy(&objects);

    // Handle "Educator" background.
    background_educate_followers(pc_obj);
}

/**
 * Updates a NPC follower's level based on the PC's level progression.
 *
 * 0x4A6CB0
 */
void update_follower_level(int64_t follower_obj, int old_pc_level, int new_pc_level)
{
    int curr_level;
    int max_level;
    int new_level;
    int bonus_points;
    int curr_points;

    // Get current and maximum levels.
    curr_level = stat_base_get(follower_obj, STAT_LEVEL);
    max_level = stat_level_max(follower_obj, STAT_LEVEL);
    if (curr_level >= max_level) {
        return;
    }

    if (curr_level <= old_pc_level) {
        new_level = new_pc_level + curr_level - old_pc_level;
    } else {
        new_level = new_pc_level;
    }

    // Cap the new level to avoid exceeding maximum level.
    if (new_level >= max_level) {
        new_level = max_level - 1;
    }

    if (new_level > curr_level) {
        // Update level.
        stat_base_set(follower_obj, STAT_LEVEL, new_level);

        // Grant character points for level up.
        bonus_points = calculate_bonus_character_points(curr_level, new_level);
        curr_points = stat_base_get(follower_obj, STAT_UNSPENT_POINTS);
        stat_base_set(follower_obj, STAT_UNSPENT_POINTS, bonus_points + curr_points);

        // Apply auto-leveling scheme.
        auto_level_apply(follower_obj, NULL);
    }
}

/**
 * Retrieves the auto-leveling scheme for a critter.
 *
 * 0x4A6D40
 */
int auto_level_scheme_get(int64_t obj)
{
    // Ensure object is a critter.
    if (obj == OBJ_HANDLE_NULL
        || !obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return -1;
    }

    return obj_field_int32_get(obj, OBJ_F_CRITTER_AUTO_LEVEL_SCHEME);
}

/**
 * Sets the auto-leveling scheme for a critter.
 *
 * 0x4A6D90
 */
int auto_level_scheme_set(int64_t obj, int value)
{
    // Ensure object is a critter.
    if (obj == OBJ_HANDLE_NULL
        || !obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return -1;
    }

    obj_field_int32_set(obj, OBJ_F_CRITTER_AUTO_LEVEL_SCHEME, value);

    return value;
}

/**
 * Retrieves the name of an auto-leveling scheme.
 *
 * 0x4A6DE0
 */
const char* auto_level_scheme_name(int scheme)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    // Select system-defined vs. user-defined message file.
    if (scheme == 0) {
        mes_file = game_level_name_mes_file;
    } else if (scheme >= 1 && scheme < 200) {
        if (scheme < 50) {
            mes_file = user_level_name_mes_file;
        } else {
            mes_file = game_level_name_mes_file;
        }
    } else {
        return NULL;
    }

    // Look up the scheme name in the message file.
    mes_file_entry.num = scheme;
    if (!mes_search(mes_file, &mes_file_entry)) {
        return NULL;
    }

    return mes_file_entry.str;
}

/**
 * Retrieves the rule string for an auto-leveling scheme.
 *
 * 0x4A6E40
 */
const char* auto_level_scheme_rule(int scheme)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    if (scheme == 0 || scheme < 1 || scheme >= 200) {
        return NULL;
    }

    // Select system-defined vs. user-defined message file.
    if (scheme < 50) {
        mes_file = user_level_mes_file;
    } else {
        mes_file = game_level_mes_file;
    }

    // Look up the rule string in the message file.
    mes_file_entry.num = scheme;
    if (!mes_search(mes_file, &mes_file_entry)) {
        return NULL;
    }

    return mes_file_entry.str;
}

/**
 * Sets a critters's level, resetting stats and applying bonus points.
 *
 * 0x4A6F00
 */
void level_set(int64_t obj, int level)
{
    int gender;
    int race;
    int alignment;
    int age;
    int unspent_points;
    int bonus;

    // Clamp level to valid range.
    if (level < 0) {
        level = 0;
    } else if (level > LEVEL_MAX) {
        level = LEVEL_MAX;
    }

    // Preserve character's "natural" attributes (that are independent of level).
    gender = stat_base_get(obj, STAT_GENDER);
    race = stat_base_get(obj, STAT_RACE);
    alignment = stat_base_get(obj, STAT_ALIGNMENT);
    age = stat_base_get(obj, STAT_AGE);

    // Reset all stats, skills, spells, and tech to defaults.
    stat_set_defaults(obj);
    skill_set_defaults(obj);
    spell_set_defaults(obj);
    tech_set_defaults(obj);

    // Restore "natural" attributes.
    stat_base_set(obj, STAT_GENDER, gender);
    stat_base_set(obj, STAT_RACE, race);
    stat_base_set(obj, STAT_ALIGNMENT, alignment);
    stat_base_set(obj, STAT_AGE, age);

    // Set the required level.
    stat_base_set(obj, STAT_LEVEL, level);

    object_hp_pts_set(obj, 0);
    critter_fatigue_pts_set(obj, 0);

    if (level >= 1) {
        // Set experience points to match level.
        stat_base_set(obj, STAT_EXPERIENCE_POINTS, level_xp_tbl[level - 1]);

        // Grant character points for level up.
        unspent_points = stat_base_get(obj, STAT_UNSPENT_POINTS);
        bonus = calculate_bonus_character_points(1, level);
        stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points + bonus);
    }

    // Apply auto-leveling scheme.
    auto_level_apply(obj, NULL);

    // NOTE: I'm not sure if damage can appear here somehow.
    object_hp_damage_set(obj, 0);
}

/**
 * Applies an auto-leveling scheme to an object and builds a description string.
 *
 * Returns `true` if auto-leveling scheme is completed, `false` otherwise.
 *
 * 0x4A7030
 */
bool auto_level_apply(int64_t obj, char* str)
{
    int scheme;
    MesFileEntry mes_file_entry;
    int type;
    char buffer[MAX_STRING];
    const char* rule;
    int index;
    bool ret;

    // Get the auto-leveling scheme for the object.
    scheme = auto_level_scheme_get(obj);
    if (scheme == 0) {
        return false;
    }

    // Initialize description string if provided.
    if (str != NULL) {
        mes_file_entry.num = 2; // "Scheme bought: "
        mes_get_msg(level_mes_file, &mes_file_entry);
        strcpy(str, mes_file_entry.str);
        auto_level_log_size = 0;
    } else {
        auto_level_log_size = -1;
    }

    // Check if the object is a PC with alternate data (in multiplayer).
    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (type == OBJ_TYPE_PC
        && (obj_field_int32_get(obj, OBJ_F_PC_FLAGS) & OPCF_USE_ALT_DATA) != 0) {
        if (!multiplayer_level_scheme_rule(obj, buffer)) {
            return false;
        }
        rule = buffer;
    } else {
        // A PC in single-player mode or a NPC - retrieve the standard rule for
        // the scheme.
        rule = auto_level_scheme_rule(scheme);
    }

    // Validate the rule string.
    if (rule == NULL) {
        if (str != NULL) {
            mes_file_entry.num = 4; // "Missing scheme!"
            mes_get_msg(level_mes_file, &mes_file_entry);
            strcat(str, mes_file_entry.str);
        }
        return false;
    }

    ret = auto_level_process_rule(obj, rule);
    if (ret && type == OBJ_TYPE_NPC) {
        // Specific auto-level scheme has completed. Fallback to the default NPC
        // scheme.
        rule = auto_level_scheme_rule(STANDARD_NPC_SCHEME);
        if (rule != NULL) {
            ret = auto_level_process_rule(obj, rule);
        }
    }

    if (auto_level_log_size == 0) {
        mes_file_entry.num = 3; // "No purchases."
        mes_get_msg(level_mes_file, &mes_file_entry);
        strcat(str, mes_file_entry.str);
    } else if (auto_level_log_size > 0) {
        // Append each change to the description string.
        for (index = 0; index < auto_level_log_size; index++) {
            strcat(str, auto_level_log_strs[index]);
            if (auto_level_log_values[index] > 0) {
                sprintf(&(str[strlen(str)]), " %d", auto_level_log_values[index]);
            } else if (auto_level_log_values[index] < 0) {
                // Special case - negative value indicates a tech degree.
                strcat(str, " ");
                strcat(str, tech_degree_name_get(-auto_level_log_values[index]));
            }
            strcat(str, ", ");
        }

        // Replace final comma with a period for proper formatting.
        str[strlen(str) - 2] = '.';
    }

    return ret;
}

/**
 * Processes an auto-leveling rule string to apply stat/skill changes.
 *
 * Returns `true` if auto-leveling rule is completed, `false` otherwise.
 *
 * 0x4A7340
 */
bool auto_level_process_rule(int64_t obj, const char* str)
{
    char buffer[MAX_STRING];
    char* tok;
    int score;
    int index;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        return false;
    }

    // Copy the rule string to a modifiable buffer.
    strcpy(buffer, str);

    for (tok = strtok(buffer, " "); tok != NULL; tok = strtok(NULL, " ")) {
        score = atoi(strtok(NULL, ","));

        // Check if the token matches a stat.
        for (index = 0; index < LEVEL_SCHEME_STATS_MAX; index++) {
            if (SDL_strcasecmp(tok, level_scheme_stats[index]) == 0) {
                if (auto_level_stat(obj, index, score) == AUTO_LEVEL_INSUFFIICIENT_POINTS) {
                    return false;
                }
                break;
            }
        }
        if (index < LEVEL_SCHEME_STATS_MAX) continue;

        // Check if the token matches a basic skill.
        for (index = 0; index < LEVEL_SCHEME_BASIC_SKILLS_MAX; index++) {
            if (SDL_strcasecmp(tok, level_scheme_basic_skills[index]) == 0) {
                if (auto_level_basic_skill(obj, index, score) == AUTO_LEVEL_INSUFFIICIENT_POINTS) {
                    return false;
                }
                break;
            }
        }
        if (index < LEVEL_SCHEME_BASIC_SKILLS_MAX) continue;

        // Check if the token matches a tech skill.
        for (index = 0; index < LEVEL_SCHEME_TECH_SKILLS_MAX; index++) {
            if (SDL_strcasecmp(tok, level_scheme_tech_skills[index]) == 0) {
                if (auto_level_tech_skill(obj, index, score) == AUTO_LEVEL_INSUFFIICIENT_POINTS) {
                    return false;
                }
                break;
            }
        }
        if (index < LEVEL_SCHEME_TECH_SKILLS_MAX) continue;

        // Check if the token matches a spell college.
        for (index = 0; index < LEVEL_SCHEME_SPELLS_MAX; index++) {
            if (SDL_strcasecmp(tok, level_scheme_spells[index]) == 0) {
                if (auto_level_spell(obj, index, score) == AUTO_LEVEL_INSUFFIICIENT_POINTS) {
                    return false;
                }
                break;
            }
        }
        if (index < LEVEL_SCHEME_SPELLS_MAX) continue;

        // Check if the token matches a tech discipline.
        for (index = 0; index < LEVEL_SCHEME_TECH_MAX; index++) {
            if (SDL_strcasecmp(tok, level_scheme_tech[index]) == 0) {
                if (auto_level_tech(obj, index, score) == AUTO_LEVEL_INSUFFIICIENT_POINTS) {
                    return false;
                }
                break;
            }
        }
        if (index < LEVEL_SCHEME_TECH_MAX) continue;

        // Check if the token matches a miscellaneous attribute.
        for (index = 0; index < LEVEL_SCHEME_MISC_MAX; index++) {
            if (SDL_strcasecmp(tok, level_scheme_misc[index]) == 0) {
                if (auto_level_misc(obj, index, score) == AUTO_LEVEL_INSUFFIICIENT_POINTS) {
                    return false;
                }
                break;
            }
        }
        if (index < LEVEL_SCHEME_MISC_MAX) continue;

        // Unknown token.
        tig_debug_printf("Error processing auto level scheme, cannot find match for: %s\n", tok);
        return false;
    }

    return true;
}

/**
 * Increments a stat for a critter.
 *
 * 0x4A75E0
 */
AutoLevelError auto_level_stat(int64_t obj, int stat, int value)
{
    int current_value;
    int cost;
    int unspent_points;
    int new_value;

    current_value = stat_base_get(obj, stat);
    while (current_value < value) {
        // Calculate cost to increment the stat.
        cost = stat_cost(current_value + 1);
        unspent_points = stat_level_get(obj, STAT_UNSPENT_POINTS);
        if (cost > unspent_points) {
            return AUTO_LEVEL_INSUFFIICIENT_POINTS;
        }

        // Attempt to increment the stat and validate the result.
        stat_base_set(obj, stat, current_value + 1);
        new_value = stat_base_get(obj, stat);
        if (new_value < current_value + 1) {
            return AUTO_LEVEL_UPDATE_FAILED;
        }

        // Consume character points.
        stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points - cost);

        current_value = stat_level_get(obj, stat);

        // Log change.
        record_attribute_change(stat_name(stat), stat_level_get(obj, stat));
    }

    return AUTO_LEVEL_OK;
}

/**
 * Increments a basic skill for a critter.
 *
 * 0x4A76B0
 */
AutoLevelError auto_level_basic_skill(int64_t obj, int skill, int value)
{
    int current_value;
    int cost;
    int unspent_points;
    int new_value;
    int stat;
    AutoLevelError rc;

    current_value = basic_skill_level(obj, skill);
    while (current_value < value) {
        // Calculate cost to increment the skill.
        cost = basic_skill_cost_inc(obj, skill);
        unspent_points = stat_level_get(obj, STAT_UNSPENT_POINTS);
        if (cost > unspent_points) {
            return AUTO_LEVEL_INSUFFIICIENT_POINTS;
        }

        // Attempt to increment the skill.
        new_value = basic_skill_points_get(obj, skill) + cost;
        if (basic_skill_points_set(obj, skill, new_value) == new_value) {
            // Consume character points.
            stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points - cost);
            current_value = basic_skill_level(obj, skill);

            // Log change.
            record_attribute_change(basic_skill_name(skill), new_value);
        } else {
            // If skill increment fails, try boosting the associated stat.
            stat = basic_skill_stat(skill);
            rc = auto_level_stat(obj, stat, stat_base_get(obj, stat) + 1);
            if (rc != AUTO_LEVEL_OK) {
                return rc;
            }
        }
    }

    return AUTO_LEVEL_OK;
}

/**
 * Increments a tech skill for a critter.
 *
 * 0x4A77A0
 */
AutoLevelError auto_level_tech_skill(int64_t obj, int skill, int score)
{
    int level;
    int cost;
    int unspent_points;
    int new_level;
    int stat;
    AutoLevelError rc;

    level = tech_skill_level(obj, skill);
    while (level < score) {
        // Calculate cost to increment the skill.
        cost = tech_skill_cost_inc(obj, skill);
        unspent_points = stat_level_get(obj, STAT_UNSPENT_POINTS);
        if (cost > unspent_points) {
            return AUTO_LEVEL_INSUFFIICIENT_POINTS;
        }

        // Attempt to increment the skill.
        new_level = tech_skill_points_get(obj, skill) + cost;
        if (tech_skill_points_set(obj, skill, new_level) == new_level) {
            // Consume character points.
            stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points - cost);
            level = tech_skill_level(obj, skill);

            // Log change.
            record_attribute_change(tech_skill_name(skill), new_level);
        } else {
            // If skill increment fails, try boosting the associated stat.
            stat = tech_skill_stat(skill);
            rc = auto_level_stat(obj, stat, stat_base_get(obj, stat) + 1);
            if (rc != AUTO_LEVEL_OK) {
                return rc;
            }
        }
    }

    return AUTO_LEVEL_OK;
}

/**
 * Increments a spell college level for a critter.
 *
 * 0x4A7890
 */
AutoLevelError auto_level_spell(int64_t obj, int college, int score)
{
    int current_value;
    int spl;
    int cost;
    int unspent_points;
    int intelligence;
    int willpower;
    AutoLevelError rc;

    current_value = spell_college_level_get(obj, college);
    while (current_value < score) {
        // Calculate spell ID and cost.
        spl = current_value + 5 * college;
        cost = spell_cost(spl);
        unspent_points = stat_level_get(obj, STAT_UNSPENT_POINTS);
        if (cost > unspent_points) {
            return AUTO_LEVEL_INSUFFIICIENT_POINTS;
        }

        // Attempt to add the spell.
        if (spell_add(obj, spl, false)) {
            // Consume character points.
            stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points - cost);
            current_value++;
        } else {
            // Check if level requirements are unmet.
            if (spell_min_level(spl) > stat_level_get(obj, STAT_LEVEL)) {
                return AUTO_LEVEL_UPDATE_FAILED;
            }

            // Try boosting intelligence or willpower to meet requirements.
            if (spell_min_intelligence(spl) > stat_level_get(obj, STAT_INTELLIGENCE)) {
                intelligence = stat_base_get(obj, STAT_INTELLIGENCE);
                rc = auto_level_stat(obj, STAT_INTELLIGENCE, intelligence + 1);
            } else {
                willpower = stat_base_get(obj, STAT_WILLPOWER);
                rc = auto_level_stat(obj, STAT_WILLPOWER, willpower + 1);
            }

            if (rc != AUTO_LEVEL_OK) {
                return rc;
            }
        }
    }

    return AUTO_LEVEL_OK;
}

/**
 * Increments a technology degree for a critter.
 *
 * 0x4A79C0
 */
AutoLevelError auto_level_tech(int64_t obj, int tech, int degree)
{
    int current_degree;
    int cost;
    int unspent_points;
    int new_degree;
    int intelligence;
    AutoLevelError rc;

    current_degree = tech_degree_get(obj, tech);
    while (current_degree < degree) {
        // Calculate cost to increment the technology degree.
        cost = tech_degree_cost_get(current_degree + 1);
        unspent_points = stat_level_get(obj, STAT_UNSPENT_POINTS);
        if (cost > unspent_points) {
            return AUTO_LEVEL_INSUFFIICIENT_POINTS;
        }

        // Attempt to increment the technology degree.
        new_degree = tech_degree_inc(obj, tech);
        if (new_degree != current_degree) {
            // Consume character points.
            stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points - cost);
            current_degree = new_degree;

            // Log change.
            record_attribute_change(tech_discipline_name_get(tech), -new_degree);
        } else {
            // If increment fails, try boosting intelligence.
            intelligence = stat_base_get(obj, STAT_INTELLIGENCE);
            rc = auto_level_stat(obj, STAT_INTELLIGENCE, intelligence + 1);
            if (rc != AUTO_LEVEL_OK) {
                return rc;
            }
        }
    }

    return AUTO_LEVEL_OK;
}

/**
 * Increments a misc attribute for a critter.
 *
 * 0x4A7AA0
 */
AutoLevelError auto_level_misc(int64_t obj, int type, int score)
{
    int current_value;
    int unspent_points;
    int new_value;
    MesFileEntry mes_file_entry;

    current_value = level_misc_attr_level_getters[type](obj);
    while (current_value < score) {
        // Check for sufficient unspent points.
        unspent_points = stat_base_get(obj, STAT_UNSPENT_POINTS);
        if (unspent_points < 1) {
            return AUTO_LEVEL_INSUFFIICIENT_POINTS;
        }

        // Increment the attribute using the appropriate setter.
        level_misc_attr_base_setters[type](obj, level_misc_attr_base_getters[type](obj) + 1);
        new_value = level_misc_attr_level_getters[type](obj);
        if (new_value < current_value + 1) {
            return AUTO_LEVEL_UPDATE_FAILED;
        }

        // Consume character points.
        stat_base_set(obj, STAT_UNSPENT_POINTS, unspent_points - 1);

        // Log change.
        mes_file_entry.num = 5 + type;
        mes_get_msg(level_mes_file, &mes_file_entry);
        record_attribute_change(mes_file_entry.str, new_value);

        current_value = new_value;
    }

    return AUTO_LEVEL_OK;
}

/**
 * Saves attribute change in the log.
 *
 * 0x4A7B90
 */
void record_attribute_change(const char* str, int score)
{
    int index;

    if (auto_level_log_size == -1) {
        return;
    }

    // Check for existing entry to update.
    for (index = 0; index < auto_level_log_size; index++) {
        if (strcmp(auto_level_log_strs[index], str) == 0) {
            auto_level_log_values[index] = score;
            return;
        }
    }

    // Add new entry if log is not full.
    if (auto_level_log_size < AUTO_LEVEL_LOG_SIZE) {
        strcpy(auto_level_log_strs[auto_level_log_size], str);
        auto_level_log_values[auto_level_log_size] = score;
        auto_level_log_size++;
    }
}
