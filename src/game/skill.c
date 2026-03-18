#include "game/skill.h"

#include "game/ai.h"
#include "game/anim.h"
#include "game/background.h"
#include "game/critter.h"
#include "game/effect.h"
#include "game/fate.h"
#include "game/gamelib.h"
#include "game/item.h"
#include "game/light.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/player.h"
#include "game/random.h"
#include "game/stat.h"
#include "game/trap.h"

#define FIRST_BASIC_SKILL_NAME_ID 0
#define FIRST_TECH_SKILL_NAME_ID (FIRST_BASIC_SKILL_NAME_ID + BASIC_SKILL_COUNT)
#define FIRST_TRAINING_NAME_ID (FIRST_TECH_SKILL_NAME_ID + TECH_SKILL_COUNT)
#define FIRST_BASIC_SKILL_DESC_ID (FIRST_TRAINING_NAME_ID + TRAINING_COUNT)
#define FIRST_TECH_SKILL_DESC_ID (FIRST_BASIC_SKILL_DESC_ID + BASIC_SKILL_COUNT)

static int basic_skill_vs(int64_t obj, int skill, int64_t other_obj);
static int max_skill_for_stat_level(int stat_level);
static int skill_gambling_random_roll(void);
static bool skill_invocation_check_crit_hit(int roll, int effectiveness, SkillInvocation* skill_invocation);
static bool skill_invocation_check_crit_miss(int roll, int effectiveness, SkillInvocation* skill_invocation);
static bool is_controlled_by_local_pc(int64_t obj);

/**
 * Maps basic skill to it's key stat.
 *
 * 0x5B6F04
 */
static int basic_skill_stats[BASIC_SKILL_COUNT] = {
    /*          BASIC_SKILL_BOW */ STAT_DEXTERITY,
    /*        BASIC_SKILL_DODGE */ STAT_DEXTERITY,
    /*        BASIC_SKILL_MELEE */ STAT_DEXTERITY,
    /*     BASIC_SKILL_THROWING */ STAT_DEXTERITY,
    /*     BASIC_SKILL_BACKSTAB */ STAT_DEXTERITY,
    /*  BASIC_SKILL_PICK_POCKET */ STAT_DEXTERITY,
    /*     BASIC_SKILL_PROWLING */ STAT_PERCEPTION,
    /*    BASIC_SKILL_SPOT_TRAP */ STAT_PERCEPTION,
    /*     BASIC_SKILL_GAMBLING */ STAT_INTELLIGENCE,
    /*       BASIC_SKILL_HAGGLE */ STAT_WILLPOWER,
    /*         BASIC_SKILL_HEAL */ STAT_INTELLIGENCE,
    /*   BASIC_SKILL_PERSUATION */ STAT_CHARISMA,
};

/**
 * Maps tech skill to it's key stat.
 *
 * 0x5B6F34
 */
static int tech_skill_stats[TECH_SKILL_COUNT] = {
    /*       TECH_SKILL_REPAIR */ STAT_INTELLIGENCE,
    /*     TECH_SKILL_FIREARMS */ STAT_PERCEPTION,
    /*   TECH_SKILL_PICK_LOCKS */ STAT_DEXTERITY,
    /* TECH_SKILL_DISARM_TRAPS */ STAT_PERCEPTION,
};

/**
 * Defines the minimum skill level required for each training tier.
 *
 * 0x5B6F44
 */
static int skill_level_training_tbl[TRAINING_COUNT] = {
    /*       TRAINING_NONE */ 0,
    /* TRAINING_APPRENTICE */ 1,
    /*     TRAINING_EXPERT */ 9,
    /*     TRAINING_MASTER */ 18,
};

/**
 * Array defining effective melee skill level requirement to determine melee
 * training tier for monstrous critters.
 *
 * 0x5B6F48
 */
static int dword_5B6F48[TRAINING_COUNT - 1] = {
    /*       TRAINING_NONE */ 1,
    /* TRAINING_APPRENTICE */ 9,
    /*     TRAINING_EXPERT */ 18,
};

/**
 * Array defining character level requirement to determine melee training tier
 * for monstrous critters.
 *
 * 0x5B6F58
 */
static int dword_5B6F58[TRAINING_COUNT - 1] = {
    /*       TRAINING_NONE */ 10,
    /* TRAINING_APPRENTICE */ 20,
    /*     TRAINING_EXPERT */ 30,
};

#define SKILL_CHECK_HIT_LOC 0x0001u
#define SKILL_CHECK_DISTANCE 0x0002u
#define SKILL_CHECK_LIGHT 0x0004u
#define SKILL_CHECK_AWARENESS 0x0008u
#define SKILL_VS_TARGET_PERCEPTION 0x0010u
#define SKILL_VS_TARGET_INTELLIGENCE 0x0020u
#define SKILL_VS_TARGET_SKILL 0x0040u
#define SKILL_USE_EYES 0x0080u
#define SKILL_USE_ARMS 0x0100u
#define SKILL_USE_LEGS 0x0200u
#define SKILL_COMBAT 0x0400u
#define SKILL_CLAMP_AT_95 0x0800u

/**
 * Defines various aspects of skill mechanics.
 *
 * 0x5B6F64
 */
static unsigned int skill_flags[SKILL_COUNT] = {
    /*          SKILL_BOW */ SKILL_COMBAT | SKILL_USE_ARMS | SKILL_USE_EYES | SKILL_CHECK_AWARENESS | SKILL_CHECK_LIGHT | SKILL_CHECK_DISTANCE | SKILL_CHECK_HIT_LOC,
    /*        SKILL_DODGE */ SKILL_CLAMP_AT_95 | SKILL_COMBAT | SKILL_USE_LEGS | SKILL_USE_EYES,
    /*        SKILL_MELEE */ SKILL_COMBAT | SKILL_USE_ARMS | SKILL_USE_EYES | SKILL_CHECK_AWARENESS | SKILL_CHECK_LIGHT | SKILL_CHECK_HIT_LOC,
    /*     SKILL_THROWING */ SKILL_COMBAT | SKILL_USE_ARMS | SKILL_USE_EYES | SKILL_CHECK_AWARENESS | SKILL_CHECK_LIGHT | SKILL_CHECK_DISTANCE | SKILL_CHECK_HIT_LOC,
    /*     SKILL_BACKSTAB */ SKILL_CLAMP_AT_95 | SKILL_USE_EYES,
    /*  SKILL_PICK_POCKET */ SKILL_CLAMP_AT_95 | SKILL_USE_ARMS | SKILL_USE_EYES | SKILL_VS_TARGET_PERCEPTION | SKILL_CHECK_AWARENESS,
    /*     SKILL_PROWLING */ SKILL_USE_LEGS | SKILL_USE_EYES | SKILL_VS_TARGET_PERCEPTION | SKILL_CHECK_AWARENESS,
    /*    SKILL_SPOT_TRAP */ SKILL_CLAMP_AT_95 | SKILL_USE_EYES | SKILL_CHECK_LIGHT,
    /*     SKILL_GAMBLING */ SKILL_CLAMP_AT_95 | SKILL_USE_EYES | SKILL_VS_TARGET_SKILL,
    /*       SKILL_HAGGLE */ SKILL_CLAMP_AT_95,
    /*         SKILL_HEAL */ SKILL_CLAMP_AT_95 | SKILL_USE_ARMS | SKILL_USE_EYES,
    /*   SKILL_PERSUATION */ SKILL_CLAMP_AT_95 | SKILL_VS_TARGET_INTELLIGENCE,
    /*       SKILL_REPAIR */ SKILL_CLAMP_AT_95 | SKILL_USE_ARMS | SKILL_USE_EYES,
    /*     SKILL_FIREARMS */ SKILL_COMBAT | SKILL_USE_ARMS | SKILL_USE_EYES | SKILL_CHECK_AWARENESS | SKILL_CHECK_LIGHT | SKILL_CHECK_DISTANCE | SKILL_CHECK_HIT_LOC,
    /*   SKILL_PICK_LOCKS */ SKILL_CLAMP_AT_95 | SKILL_USE_ARMS | SKILL_USE_EYES | SKILL_CHECK_LIGHT,
    /* SKILL_DISARM_TRAPS */ SKILL_CLAMP_AT_95 | SKILL_USE_ARMS | SKILL_USE_EYES | SKILL_CHECK_LIGHT,
};

/**
 * Defines the maximum skill level allowed for each stat level (1-20).
 *
 * For example, at stat level 12, the maximum effective skill level is capped at
 * 15.
 *
 * 0x5B6FA4
 */
static int max_skill_level_tbl[20] = {
    /*  1 */ 3,
    /*  2 */ 3,
    /*  3 */ 3,
    /*  4 */ 3,
    /*  5 */ 3,
    /*  6 */ 7,
    /*  7 */ 7,
    /*  8 */ 7,
    /*  9 */ 11,
    /* 10 */ 11,
    /* 11 */ 11,
    /* 12 */ 15,
    /* 13 */ 15,
    /* 14 */ 15,
    /* 15 */ 19,
    /* 16 */ 19,
    /* 17 */ 19,
    /* 18 */ 20,
    /* 19 */ 20,
    /* 20 */ 20,
};

/**
 * Lookup keys for basic skills.
 *
 * 0x5B6FF4
 */
const char* basic_skill_lookup_keys_tbl[BASIC_SKILL_COUNT] = {
    /*          BASIC_SKILL_BOW */ "bs_bow",
    /*        BASIC_SKILL_DODGE */ "bs_dodge",
    /*        BASIC_SKILL_MELEE */ "bs_melee",
    /*     BASIC_SKILL_THROWING */ "bs_throwing",
    /*     BASIC_SKILL_BACKSTAB */ "bs_backstab",
    /*  BASIC_SKILL_PICK_POCKET */ "bs_pick_pocket",
    /*     BASIC_SKILL_PROWLING */ "bs_prowling",
    /*    BASIC_SKILL_SPOT_TRAP */ "bs_spot_trap",
    /*     BASIC_SKILL_GAMBLING */ "bs_gambling",
    /*       BASIC_SKILL_HAGGLE */ "bs_haggle",
    /*         BASIC_SKILL_HEAL */ "bs_heal",
    /*   BASIC_SKILL_PERSUATION */ "bs_persuasion",
};

/**
 * Lookup keys for tech skills.
 *
 * 0x5B7024
 */
const char* tech_skill_lookup_keys_tbl[TECH_SKILL_COUNT] = {
    /*       TECH_SKILL_REPAIR */ "ts_repair",
    /*     TECH_SKILL_FIREARMS */ "ts_firearms",
    /*   TECH_SKILL_PICK_LOCKS */ "ts_pick_lock",
    /* TECH_SKILL_DISARM_TRAPS */ "ts_disarm_trap",
};

/**
 * Lookup keys for trainings.
 *
 * 0x5B7034
 */
const char* training_lookup_keys_tbl[TRAINING_COUNT] = {
    /*       TRAINING_NONE */ "st_untrained",
    /* TRAINING_APPRENTICE */ "st_beginner",
    /*     TRAINING_EXPERT */ "st_expert",
    /*     TRAINING_MASTER */ "st_master",
};

/**
 * Array of success rates for competitive basic skill contests.
 *
 * This table is indexed by the difference between source object skill score and
 * target skill score.
 *
 * NOTE: For unknown reason the values from this table are divided by 4. I guess
 * the scale was once different.
 *
 * 0x5B7044
 */
static int dword_5B7044[19] = {
    /* -9 */ 55, // 13
    /* -8 */ 66, // 16
    /* -7 */ 78, // 19
    /* -6 */ 91, // 22
    /* -5 */ 105, // 26
    /* -4 */ 120, // 30
    /* -3 */ 136, // 34
    /* -2 */ 153, // 38
    /* -1 */ 171, // 42
    /*  0 */ 190, // 47
    /* +1 */ 209, // 52
    /* +2 */ 227, // 56
    /* +3 */ 244, // 61
    /* +4 */ 260, // 65
    /* +5 */ 275, // 68
    /* +6 */ 289, // 72
    /* +7 */ 302, // 75
    /* +8 */ 314, // 78
    /* +9 */ 325, // 81
};

/**
 * Defines the maximum worth of a single item during gambling.
 *
 * This table is indexed by effective gambling level (0-20).
 *
 * 0x5B7090
 */
static int skill_gambling_item_cost_tbl[21] = {
    /*  0 */ 0,
    /*  1 */ 25,
    /*  2 */ 50,
    /*  3 */ 75,
    /*  4 */ 100,
    /*  5 */ 150,
    /*  6 */ 200,
    /*  7 */ 250,
    /*  8 */ 300,
    /*  9 */ 400,
    /* 10 */ 500,
    /* 11 */ 600,
    /* 12 */ 700,
    /* 13 */ 900,
    /* 14 */ 1100,
    /* 15 */ 1300,
    /* 16 */ 1500,
    /* 17 */ 2000,
    /* 18 */ 3000,
    /* 19 */ 4000,
    /* 20 */ 5000,
};

/**
 * Defines penalties for pickpocketing items based on inventory location.
 *
 * For example, when trying to steal the equipped weapon, the difficulty is
 * increased by 70 percentage points.
 *
 * 0x5B70E4
 */
static int dword_5B70E4[9] = {
    /*    HELMET */ 50,
    /*     RING1 */ 30,
    /*     RING2 */ 30,
    /* MEDALLION */ 30,
    /*    WEAPON */ 70,
    /*    SHIELD */ 70,
    /*     ARMOR */ 1000,
    /*  GAUNTLET */ 1000,
    /*     BOOTS */ 1000,
};

/**
 * 0x5FF424
 */
static char* basic_skill_descriptions[BASIC_SKILL_COUNT];

/**
 * 0x5FF454
 */
static char* basic_skill_names[BASIC_SKILL_COUNT];

/**
 * 0x5FF484
 */
static mes_file_handle_t skill_mes_file;

/**
 * 0x5FF488
 */
static char* tech_skill_descriptions[TECH_SKILL_COUNT];

/**
 * 0x5FF498
 */
static char* training_names[TRAINING_COUNT];

/**
 * 0x5FF4E8
 */
static char* tech_skill_names[TECH_SKILL_COUNT];

/**
 * 0x6876A0
 */
static SkillCallbacks skill_callbacks;

/**
 * The next random difficulty roll for gambling skill checks.
 *
 * This value is used to prevent gambling retries by loading the game. It's
 * saved in the save game.
 *
 * 0x6876C4
 */
static int skill_gambling_next_roll;

/**
 * Called when the game is initialized.
 *
 * 0x4C5BD0
 */
bool skill_init(GameInitInfo* init_info)
{
    MesFileEntry mes_file_entry;
    int index;

    (void)init_info;

    settings_register(&settings, FOLLOWER_SKILLS_KEY, "1", NULL);

    // Reset callbacks.
    skill_callbacks.field_0 = NULL;
    skill_callbacks.steal_item_func = NULL;
    skill_callbacks.plant_item_output = NULL;
    skill_callbacks.field_C = NULL;
    skill_callbacks.disarm_trap_func = NULL;
    skill_callbacks.repair_func = NULL;
    skill_callbacks.no_repair_func = NULL;
    skill_callbacks.lock_func = NULL;
    skill_callbacks.no_lock_func = NULL;

    // Load the skill message file.
    if (!mes_load("mes\\skill.mes", &skill_mes_file)) {
        return false;
    }

    // Load basic skill names.
    for (index = 0; index < BASIC_SKILL_COUNT; index++) {
        mes_file_entry.num = index + FIRST_BASIC_SKILL_NAME_ID;
        mes_get_msg(skill_mes_file, &mes_file_entry);
        basic_skill_names[index] = mes_file_entry.str;
    }

    // Load tech skill names.
    for (index = 0; index < TECH_SKILL_COUNT; index++) {
        mes_file_entry.num = index + FIRST_TECH_SKILL_NAME_ID;
        mes_get_msg(skill_mes_file, &mes_file_entry);
        tech_skill_names[index] = mes_file_entry.str;
    }

    // Load skill training names.
    for (index = 0; index < TRAINING_COUNT; index++) {
        mes_file_entry.num = index + FIRST_TRAINING_NAME_ID;
        mes_get_msg(skill_mes_file, &mes_file_entry);
        training_names[index] = mes_file_entry.str;
    }

    // Load basic skill descriptions.
    for (index = 0; index < BASIC_SKILL_COUNT; index++) {
        mes_file_entry.num = index + FIRST_BASIC_SKILL_DESC_ID;
        mes_get_msg(skill_mes_file, &mes_file_entry);
        basic_skill_descriptions[index] = mes_file_entry.str;
    }

    // Load tech skill descriptions.
    for (index = 0; index < TECH_SKILL_COUNT; index++) {
        mes_file_entry.num = index + FIRST_TECH_SKILL_DESC_ID;
        mes_get_msg(skill_mes_file, &mes_file_entry);
        tech_skill_descriptions[index] = mes_file_entry.str;
    }

    // Initialize next gambling roll (used to prevent retries by loading on
    // failure).
    skill_gambling_random_roll();

    return true;
}

/**
 * 0x4C5D50
 */
void skill_set_callbacks(SkillCallbacks* callbacks)
{
    skill_callbacks = *callbacks;
}

/**
 * Called when the game shuts down.
 *
 * 0x4C5DB0
 */
void skill_exit(void)
{
    mes_unload(skill_mes_file);
}

/**
 * Called when the game is being loaded.
 *
 * 0x4C5DC0
 */
bool skill_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(&skill_gambling_next_roll, sizeof(skill_gambling_next_roll), 1, load_info->stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x4C5DE0
 */
bool skill_save(TigFile* stream)
{
    if (tig_file_fwrite(&skill_gambling_next_roll, sizeof(skill_gambling_next_roll), 1, stream) != 1) {
        return false;
    }

    return true;
}

/**
 * Sets default skill values for a critter.
 *
 * 0x4C5E00
 */
void skill_set_defaults(int64_t obj)
{
    int index;

    // Reset basic skill points to 0.
    for (index = 0; index < BASIC_SKILL_COUNT; index++) {
        obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_BASIC_SKILL_IDX, index, 0);
    }

    // Reset tech skill points to 0.
    for (index = 0; index < TECH_SKILL_COUNT; index++) {
        obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_TECH_SKILL_IDX, index, 0);
    }
}

/**
 * Calculates the base skill level of a basic skill.
 *
 * The base skill level is 4 per each skill point.
 *
 * 0x4C5E50
 */
int basic_skill_base(int64_t obj, int bs)
{
    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_BASIC_SKILL_VALID(bs)) {
        return 0;
    }

    return 4 * basic_skill_points_get(obj, bs);
}

/**
 * Retrieves the effective skill level of a basic skill.
 *
 * 0x4C5EB0
 */
int basic_skill_level(int64_t obj, int bs)
{
    int skill_level;
    int key_stat_level;

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_BASIC_SKILL_VALID(bs)) {
        return 0;
    }

    // Special case: monstrous critters have a fixed base level of 20.
    if (bs == BASIC_SKILL_MELEE && critter_is_monstrous(obj)) {
        skill_level = 20;
    } else {
        skill_level = basic_skill_base(obj, bs);
    }

    // Apply effects.
    skill_level = effect_adjust_basic_skill_level(obj, bs, skill_level);

    // Clamp skill level to the valid range (0-20).
    if (skill_level < 0) {
        skill_level = 0;
    } else if (skill_level > 20) {
        skill_level = 20;
    }

    // Cap skill level based on the critter's key stat level.
    key_stat_level = stat_level_get(obj, basic_skill_stat(bs));
    if (skill_level > max_skill_for_stat_level(key_stat_level)) {
        skill_level = max_skill_for_stat_level(key_stat_level);
    }

    return skill_level;
}

/**
 * Determines the maximum skill level allowed to a given key stat value.
 *
 * 0x4C5F70
 */
int max_skill_for_stat_level(int stat_level)
{
    // Clamp stat level to the valid range (1-20).
    if (stat_level < 1) {
        stat_level = 1;
    } else if (stat_level > 20) {
        stat_level = 20;
    }

    return max_skill_level_tbl[stat_level - 1];
}

/**
 * Retrieves the number of skill points allocated to a basic skill.
 *
 * 0x4C5FA0
 */
int basic_skill_points_get(int64_t obj, int bs)
{
    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_BASIC_SKILL_VALID(bs)) {
        return 0;
    }

    // Extract skill points (lower 6 bits) from the skill field.
    return obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_BASIC_SKILL_IDX, bs) & 63;
}

/**
 * Sets the number of skill points allocated to a basic skill.
 *
 * Returns the actual number of skill points set, or 0 if validation fails.
 *
 * 0x4C6000
 */
int basic_skill_points_set(int64_t obj, int bs, int value)
{
    int key_stat_level;
    int current_value;

    // Ensure the value is non-negative.
    if (value < 0) {
        return 0;
    }

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_BASIC_SKILL_VALID(bs)) {
        return 0;
    }

    key_stat_level = stat_level_get(obj, basic_skill_stat(bs));
    current_value = obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_BASIC_SKILL_IDX, bs);

    // Check if the new base skill level exceeds the stat-based maximum. If so,
    // return the current value without changing anything.
    if (4 * value > max_skill_for_stat_level(key_stat_level)) {
        return current_value & 63;
    }

    // Update the skill field with the new points (preserving training bits).
    obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_BASIC_SKILL_IDX, bs, value | (current_value & ~63));

    return value;
}

/**
 * Retrieves the training tier for a basic skill.
 *
 * 0x4C60C0
 */
int basic_skill_training_get(int64_t obj, int bs)
{
    int melee;
    int level;
    int training;

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return TRAINING_NONE;
    }

    // Ensure skill is valid.
    if (!IS_BASIC_SKILL_VALID(bs)) {
        return TRAINING_NONE;
    }

    // Special case: monstrous critters have it's melee training computed from
    // effective skill level and character level.
    if (bs == BASIC_SKILL_MELEE && critter_is_monstrous(obj)) {
        melee = basic_skill_level(obj, BASIC_SKILL_MELEE);
        level = stat_level_get(obj, STAT_LEVEL);

        // Iterate through training tiers, except the last one. Select the
        // highest tier where effective skill level and character level meet
        // thresholds.
        //
        // The thresholds does not have bounds for the Master tier. So if we are
        // not stopped in the middle of the loop, `training` will reach the
        // value of 3 (which is `TRAINING_MASTER`).
        for (training = 0; training < TRAINING_COUNT - 1; training++) {
            if (melee < dword_5B6F48[training]) {
                break;
            }

            if (level < dword_5B6F58[training]) {
                break;
            }
        }

        return training;
    }

    // Extract training from bits 6-7 of the skill field.
    return (obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_BASIC_SKILL_IDX, bs) >> 6) & 3;
}

/**
 * Sets the training tier for a basic skill.
 *
 * 0x4C6170
 */
int basic_skill_training_set(int64_t obj, int bs, int training)
{
    int skill_value;
    int current_training;

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return TRAINING_NONE;
    }

    // Ensure skill is valid.
    if (!IS_BASIC_SKILL_VALID(bs)) {
        return TRAINING_NONE;
    }

    // Ensure training tier is valid.
    if (!IS_TRAINING_VALID(training)) {
        return TRAINING_NONE;
    }

    // Get the current skill field value and extract training tier.
    skill_value = obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_BASIC_SKILL_IDX, bs);
    current_training = (skill_value >> 6) & 3;

    // Enforce sequential training: new tier must be one step above current or
    // equal.
    if (training > current_training && current_training != training - 1) {
        return current_training;
    }

    // Check if the skill level meets the minimum required for the new tier.
    if (basic_skill_level(obj, bs) < training_min_skill_level_required(training)) {
        return current_training;
    }

    // Update the skill field with the new training tier (preserving skill
    // points bits).
    obj_arrayfield_int32_set(obj,
        OBJ_F_CRITTER_BASIC_SKILL_IDX,
        bs,
        (skill_value & ~(3 << 6)) | (training << 6));

    // Handle "Educator" background.
    background_educate_followers(obj);

    return training;
}

/**
 * Retrieves the name of a basic skill.
 *
 * 0x4C62B0
 */
char* basic_skill_name(int bs)
{
    return basic_skill_names[bs];
}

/**
 * Retrieves the description of a basic skill.
 *
 * 0x4C62C0
 */
char* basic_skill_description(int bs)
{
    return basic_skill_descriptions[bs];
}

/**
 * Retrieves the cost (in coins) of skill-as-a-service.
 *
 * Called during dialog to determine amount of money NPC will ask to use his/her
 * skill ("u:" opcode).
 *
 * 0x4C62D0
 */
int basic_skill_money(int bs, int skill_level, int training)
{
    (void)bs;

    return (training + 1) * (skill_level + 2);
}

/**
 * Determines success rate of performing a basic skill.
 *
 * 0x4C62E0
 */
int basic_skill_effectiveness(int64_t obj, int bs, int64_t target_obj)
{
    int value;
    int level;
    int game_difficulty;

    // Check if the skill involves a competitive check against the target.
    if ((skill_flags[bs] & (SKILL_VS_TARGET_SKILL | SKILL_VS_TARGET_INTELLIGENCE | SKILL_VS_TARGET_PERCEPTION)) != 0) {
        value = basic_skill_vs(obj, bs, target_obj);
    } else {
        // Calculate success rate based on skill level for non-competitive
        // skills.
        level = basic_skill_level(obj, bs);
        switch (bs) {
        case BASIC_SKILL_BOW:
            value = 5 * level + 25;
            break;
        case BASIC_SKILL_DODGE:
            value = 5 * level;
            break;
        case BASIC_SKILL_MELEE:
            value = 5 * level + 25;
            break;
        case BASIC_SKILL_THROWING:
            value = 7 * level + 25;
            break;
        case BASIC_SKILL_PROWLING:
            if (level != 0) {
                value = 5 * level + 25;
            } else {
                value = 0;
            }
            break;
        case BASIC_SKILL_SPOT_TRAP:
            value = 4 * level;
            break;
        case BASIC_SKILL_HAGGLE:
            value = 4 * level + 10;
            break;
        case BASIC_SKILL_HEAL:
            value = 5 * level;
            break;
        default:
            value = 0;
            break;
        }
    }

    // Extraordinary intelligence grants +10% to the success rate of every
    // skill.
    if (stat_is_extraordinary(obj, STAT_INTELLIGENCE)) {
        value += 10;
    }

    // For PC - adjust effectiveness based on game difficulty.
    if (obj == player_get_local_pc_obj()) {
        game_difficulty = gamelib_game_difficulty_get();
        switch (game_difficulty) {
        case GAME_DIFFICULTY_EASY:
            // Easy: +50% to success rate.
            value += value / 2;
            break;
        case GAME_DIFFICULTY_HARD:
            // Hard: -25% to success rate.
            value -= value / 4;
            break;
        }
    }

    // Clamp effectiveness at 95% for certain skills.
    if ((skill_flags[bs] & SKILL_CLAMP_AT_95) != 0) {
        if (value > 95) {
            value = 95;
        }
    }

    // Ensure success rate is non-negative.
    if (value < 0) {
        value = 0;
    }

    return value;
}

/**
 * Determines the success rate for a competitive skill check.
 *
 * 0x4C6410
 */
int basic_skill_vs(int64_t obj, int skill, int64_t other_obj)
{
    int skill_level;
    int difficulty;
    int delta;

    // If no target is provided, assume 100% success rate (no opposition).
    if (other_obj == OBJ_HANDLE_NULL) {
        return 100;
    }

    skill_level = basic_skill_level(obj, skill);

    // Determine the target's opposing stat or skill based on skill's
    // configuration.
    if ((skill_flags[skill] & SKILL_VS_TARGET_PERCEPTION) != 0) {
        difficulty = stat_level_get(other_obj, STAT_PERCEPTION);
    } else if ((skill_flags[skill] & SKILL_VS_TARGET_INTELLIGENCE) != 0) {
        difficulty = stat_level_get(other_obj, STAT_INTELLIGENCE);
    } else {
        difficulty = basic_skill_level(other_obj, skill);
    }

    // Calculate the delta of how much source object is "better" than the target
    // object. The skill level and difficulty are normally in 0-20 scale, so...
    delta = skill_level - difficulty;

    // ...the difference of 10 and above means source object is way superior
    // than the target (say Persuasion 17 vs. Intelligence 7), resulting in
    // 100% success rate...
    if (delta >= 10) {
        return 100;
    }

    // ...and the difference of -10 and below is exactly the opposite - source
    // object has no chance to withstand target, resulting in 0% success rate.
    if (delta <= -10) {
        return 0;
    }

    // Otherwise, use the lookup table to determine success rate, scaled by 1/4.
    return dword_5B7044[delta + 9] / 4;
}

/**
 * Returns the cost (in character points) to increment a basic skill point by 1.
 *
 * 0x4C64B0
 */
int basic_skill_cost_inc(int64_t obj, int bs)
{
    (void)obj;
    (void)bs;

    return 1;
}

/**
 * Returns the character points that will be redemeed by decrementing a basic
 * skill point by 1.
 *
 * 0x4C64C0
 */
int basic_skill_cost_dec(int64_t obj, int bs)
{
    (void)obj;
    (void)bs;

    return 1;
}

/**
 * Retrieves the key stat associated with a basic skill.
 *
 * 0x4C64D0
 */
int basic_skill_stat(int bs)
{
    return basic_skill_stats[bs];
}

/**
 * Determines the minimum key stat level required to achieve a given skill
 * level.
 *
 * 0x4C64E0
 */
int basic_skill_min_stat_level_required(int skill_level)
{
    int stat_level;

    for (stat_level = 0; stat_level < 20; stat_level++) {
        if (max_skill_for_stat_level(stat_level + 1) >= skill_level) {
            return stat_level + 1;
        }
    }

    return 0;
}

/**
 * 0x4C6510
 */
int sub_4C6510(int64_t obj)
{
    (void)obj;

    return 100;
}

/**
 * Determines the maximum worth of a single item that the critter can gamble
 * for.
 *
 * 0x4C6520
 */
int skill_gambling_max_item_cost(int64_t obj)
{
    int gambling;
    int amount;

    gambling = basic_skill_level(obj, BASIC_SKILL_GAMBLING);
    amount = skill_gambling_item_cost_tbl[gambling];

    // Gambling Apprentice (and better) allows gambling for more expensive
    // items.
    if (basic_skill_training_get(obj, BASIC_SKILL_GAMBLING) >= TRAINING_APPRENTICE) {
        amount *= 2;
    }

    return amount;
}

/**
 * Retrieves the next random difficulty roll for gambling.
 *
 * 0x4C6560
 */
int skill_gambling_random_roll(void)
{
    int value;

    value = skill_gambling_next_roll;

    // Store the next roll.
    skill_gambling_next_roll = random_between(1, 100);

    return value;
}

/**
 * Calculates the base skill level of a tech skill.
 *
 * The base skill level is 4 per each skill point.
 *
 * 0x4C6580
 */
int tech_skill_base(int64_t obj, int ts)
{
    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_TECH_SKILL_VALID(ts)) {
        return 0;
    }

    return 4 * tech_skill_points_get(obj, ts);
}

/**
 * Retrieves the effective skill level of a tech skill.
 *
 * 0x4C65E0
 */
int tech_skill_level(int64_t obj, int ts)
{
    int skill_level;
    int key_stat_level;

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_TECH_SKILL_VALID(ts)) {
        return 0;
    }

    skill_level = tech_skill_base(obj, ts);

    // Apply effects.
    skill_level = effect_adjust_tech_skill_level(obj, ts, skill_level);

    // Clamp skill level to the valid range (0-20).
    if (skill_level < 0) {
        skill_level = 0;
    } else if (skill_level > 20) {
        skill_level = 20;
    }

    // Cap skill level based on the critter's key stat level.
    key_stat_level = stat_level_get(obj, tech_skill_stat(ts));
    if (skill_level > max_skill_for_stat_level(key_stat_level)) {
        skill_level = max_skill_for_stat_level(key_stat_level);
    }

    return skill_level;
}

/**
 * Retrieves the number of skill points allocated to a tech skill.
 *
 * 0x4C6680
 */
int tech_skill_points_get(int64_t obj, int ts)
{
    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_TECH_SKILL_VALID(ts)) {
        return 0;
    }

    // Extract skill points (lower 6 bits) from the skill field.
    return obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_TECH_SKILL_IDX, ts) & 63;
}

/**
 * Sets the number of skill points allocated to a tech skill.
 *
 * Returns the actual number of skill points set, or 0 if validation fails.
 *
 * 0x4C66E0
 */
int tech_skill_points_set(int64_t obj, int ts, int value)
{
    int key_stat_level;
    int current_value;
    int tech_points;

    // Ensure the value is non-negative.
    if (value < 0) {
        return 0;
    }

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure skill is valid.
    if (!IS_TECH_SKILL_VALID(ts)) {
        return 0;
    }

    key_stat_level = stat_level_get(obj, tech_skill_stat(ts));
    current_value = obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_TECH_SKILL_IDX, ts);

    // Check if the new base skill level exceeds the stat-based maximum. If so,
    // return the current value without changing anything.
    if (4 * value > max_skill_for_stat_level(key_stat_level)) {
        return current_value & 63;
    }

    // Update the skill field with the new points (preserving training bits).
    obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_TECH_SKILL_IDX, ts, value | (current_value & ~63));

    // Update tech aptitude.
    tech_points = stat_base_get(obj, STAT_TECH_POINTS);
    tech_points += value - (current_value & 63);
    stat_base_set(obj, STAT_TECH_POINTS, tech_points);

    return value;
}

/**
 * Retrieves the training tier for a tech skill.
 *
 * 0x4C67F0
 */
int tech_skill_training_get(int64_t obj, int ts)
{
    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return TRAINING_NONE;
    }

    // Ensure skill is valid.
    if (!IS_TECH_SKILL_VALID(ts)) {
        return TRAINING_NONE;
    }

    // Extract training from bits 6-7 of the skill field.
    return (obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_TECH_SKILL_IDX, ts) >> 6) & 3;
}

/**
 * Sets the training tier for a tech skill.
 *
 * 0x4C6850
 */
int tech_skill_training_set(int64_t obj, int ts, int training)
{
    int skill_value;
    int current_training;

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return TRAINING_NONE;
    }

    // Ensure skill is valid.
    if (!IS_TECH_SKILL_VALID(ts)) {
        return TRAINING_NONE;
    }

    // Ensure training tier is valid.
    if (!IS_TRAINING_VALID(training)) {
        return TRAINING_NONE;
    }

    // Get the current skill field value and extract training tier.
    skill_value = obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_TECH_SKILL_IDX, ts);
    current_training = (skill_value >> 6) & 3;

    // Enforce sequential training: new tier must be one step above current or
    // equal.
    if (training > current_training && current_training != training - 1) {
        return current_training;
    }

    // Check if the skill level meets the minimum required for the new tier.
    if (tech_skill_level(obj, ts) < training_min_skill_level_required(training)) {
        return current_training;
    }

    // Update the skill field with the new training tier (preserving skill
    // points bits).
    obj_arrayfield_int32_set(obj,
        OBJ_F_CRITTER_TECH_SKILL_IDX,
        ts,
        (skill_value & ~(3 << 6)) | (training << 6));

    // Handle "Educator" background.
    background_educate_followers(obj);

    return training;
}

/**
 * Retrieves the name of a tech skill.
 *
 * 0x4C69A0
 */
char* tech_skill_name(int ts)
{
    return tech_skill_names[ts];
}

/**
 * Retrieves the description of a tech skill.
 *
 * 0x4C69B0
 */
char* tech_skill_description(int ts)
{
    return tech_skill_descriptions[ts];
}

/**
 * Retrieves the minimum skill level required for a training tier.
 *
 * 0x4C69C0
 */
int training_min_skill_level_required(int training)
{
    return skill_level_training_tbl[training];
}

/**
 * Retrieves the name of a training tier.
 *
 * 0x4C69D0
 */
char* training_name(int training)
{
    return training_names[training];
}

/**
 * Retrieves the cost (in coins) of skill-as-a-service.
 *
 * Called during dialog to determine amount of money NPC will ask to use his/her
 * skill ("u:" opcode).
 *
 * 0x4C69E0
 */
int tech_skill_money(int ts, int skill_level, int training)
{
    (void)ts;

    return (training + 1) * (skill_level + 2);
}

/**
 * Determines success rate of performing a tech skill.
 *
 * 0x4C69F0
 */
int tech_skill_effectiveness(int64_t obj, int ts, int64_t target_obj)
{
    int value;
    int level;
    int game_difficulty;

    // Calculate success rate based on skill level.
    level = tech_skill_level(obj, ts);
    switch (ts) {
    case TECH_SKILL_REPAIR:
        value = 4 * level;
        break;
    case TECH_SKILL_FIREARMS:
        value = 5 * level + 25;
        if (target_obj != OBJ_HANDLE_NULL
            && (obj_field_int32_get(target_obj, OBJ_F_SPELL_FLAGS) & OSF_MAGNETIC_INVERSION) != 0) {
            value -= 20;
        }
        break;
    case TECH_SKILL_PICK_LOCKS:
        value = 6 * level;
        break;
    case TECH_SKILL_DISARM_TRAPS:
        if (level != 0) {
            value = 4 * level + 20;
        } else {
            value = 0;
        }
        break;
    default:
        value = 0;
        break;
    }

    // Extraordinary intelligence grants +10% to the success rate of every
    // skill.
    if (stat_is_extraordinary(obj, STAT_INTELLIGENCE)) {
        value += 10;
    }

    // For PC - adjust effectiveness based on game difficulty.
    if (obj == player_get_local_pc_obj()) {
        game_difficulty = gamelib_game_difficulty_get();
        switch (game_difficulty) {
        case GAME_DIFFICULTY_EASY:
            // Easy: +50% to success rate.
            value += value / 2;
            break;
        case GAME_DIFFICULTY_HARD:
            // Hard: -25% to success rate.
            value -= value / 4;
            break;
        }
    }

    // Clamp effectiveness at 95% for certain skills.
    if ((skill_flags[ts + BASIC_SKILL_COUNT] & SKILL_CLAMP_AT_95) != 0) {
        if (value > 95) {
            value = 95;
        }
    }

    // Ensure success rate is non-negative.
    if (value < 0) {
        value = 0;
    }

    return value;
}

/**
 * Returns the cost (in character points) to increment a tech skill point by 1.
 *
 * 0x4C6AF0
 */
int tech_skill_cost_inc(int64_t obj, int ts)
{
    (void)obj;
    (void)ts;

    return 1;
}

/**
 * Returns the character points that will be redemeed by decrementing a tech
 * skill point by 1.
 *
 * 0x4C6B00
 */
int tech_skill_cost_dec(int64_t obj, int ts)
{
    (void)obj;
    (void)ts;

    return 1;
}

/**
 * Retrieves the key stat associated with a tech skill.
 *
 * 0x4C6B10
 */
int tech_skill_stat(int ts)
{
    return tech_skill_stats[ts];
}

/**
 * Determines the minimum key stat level required to achieve a given skill
 * level.
 *
 * 0x4C6B20
 */
int tech_skill_min_stat_level_required(int skill_level)
{
    int stat_level;

    for (stat_level = 0; stat_level < 20; stat_level++) {
        if (max_skill_for_stat_level(stat_level + 1) >= skill_level) {
            return stat_level + 1;
        }
    }

    return 0;
}

/**
 * Checks if changing a critter's stat to the new value won't break requirements
 * for already allocated skills.
 *
 * This function is used to re-validate skill requirements that depend on the
 * specified stat when it's value is about to decrease.
 *
 * Returns `true` if all skills' requirements are met, `false` otherwise.
 *
 * 0x4C6B50
 */
bool skill_check_stat(int64_t obj, int stat, int value)
{
    int skill;

    for (skill = 0; skill < BASIC_SKILL_COUNT; skill++) {
        if (basic_skill_stats[skill] == stat) {
            if (basic_skill_base(obj, skill) > max_skill_for_stat_level(value)) {
                return false;
            }
        }
    }

    for (skill = 0; skill < TECH_SKILL_COUNT; skill++) {
        if (tech_skill_stats[skill] == stat) {
            if (tech_skill_base(obj, skill) > max_skill_for_stat_level(value)) {
                return false;
            }
        }
    }

    return true;
}

/**
 * Initiates a skill usage animation with specified parameters.
 *
 * 0x4C6F90
 */
bool skill_use(int64_t obj, int skill, int64_t target_obj, unsigned int flags)
{
    return anim_goal_use_skill_on(obj, target_obj, OBJ_HANDLE_NULL, skill, flags);
}

/**
 * Initiates a pickpocket skill usage animation (stealing mode).
 *
 * 0x4C6FD0
 */
bool skill_steal_item(int64_t obj, int64_t target_obj, int64_t item_obj)
{
    return anim_goal_use_skill_on(obj, target_obj, item_obj, SKILL_PICK_POCKET, 0);
}

/**
 * Initiates a pickpocket skill usage animation (planting mode).
 *
 * 0x4C7010
 */
bool skill_plant_item(int64_t obj, int64_t target_obj, int64_t item_obj)
{
    return anim_goal_use_skill_on(obj, target_obj, item_obj, SKILL_PICK_POCKET, 1);
}

/**
 * Initiates a disarm trap skill usage animation.
 *
 * NOTE: This function is never used.
 *
 * 0x4C7050
 */
bool skill_disarm_trap(int64_t obj, int a2, int64_t target_obj)
{
    return anim_goal_use_skill_on(obj, target_obj, OBJ_HANDLE_NULL, SKILL_DISARM_TRAPS, 2 << a2);
}

/**
 * Initializes a `SkillInvocation` structure to default values.
 *
 * 0x4C7090
 */
void skill_invocation_init(SkillInvocation* skill_invocation)
{
    skill_invocation->flags = 0;
    sub_4440E0(OBJ_HANDLE_NULL, &(skill_invocation->item));
    sub_4440E0(OBJ_HANDLE_NULL, &(skill_invocation->source));
    sub_4440E0(OBJ_HANDLE_NULL, &(skill_invocation->target));
    skill_invocation->target_loc = 0;
    skill_invocation->modifier = 0;
    skill_invocation->skill = -1;
}

/**
 * Recovers object handles in a specified `SkillInvocation` structure.
 *
 * 0x4C7120
 */
bool skill_invocation_recover(SkillInvocation* skill_invocation)
{
    return sub_444130(&(skill_invocation->source))
        && sub_444130(&(skill_invocation->target))
        && sub_444130(&(skill_invocation->item));
}

/**
 * Executes a skill invocation.
 *
 * Returns `true` if invocation was processed, `false` otherwise. The only case
 * when `false` is returned is a multiplayer game when the client (not host)
 * attempts to run invocation.
 *
 * The outcome of the invocation is done by checking `SKILL_INVOCATION_SUCCESS`
 * and `SKILL_INVOCATION_CRITICAL` flags.
 *
 * 0x4C7160
 */
bool skill_invocation_run(SkillInvocation* skill_invocation)
{
    int64_t source_obj;
    int64_t target_obj;
    int64_t target_loc;
    int64_t item_obj;
    int skill;
    int basic_skill;
    int tech_skill;
    int training;
    int effectiveness;
    int difficulty;
    int rnd_difficulty_roll;
    int crit_roll;
    bool is_success;
    bool is_critical;
    bool is_fate = false;

    source_obj = skill_invocation->source.obj;
    target_obj = skill_invocation->target.obj;
    target_loc = skill_invocation->target_loc;
    item_obj = skill_invocation->item.obj;
    skill = skill_invocation->skill;

    // Determine skill details - basic vs. tech skill, success rate, training.
    if (IS_TECH_SKILL(skill)) {
        basic_skill = -1;
        tech_skill = GET_TECH_SKILL(skill);
        effectiveness = tech_skill_effectiveness(source_obj, tech_skill, target_obj);
        training = tech_skill_training_get(source_obj, tech_skill);
    } else {
        basic_skill = GET_BASIC_SKILL(skill);
        tech_skill = -1;
        effectiveness = basic_skill_effectiveness(source_obj, basic_skill, target_obj);
        training = basic_skill_training_get(source_obj, basic_skill);
    }

    // Calculate the situational difficulty and add difficulty modifier from
    // invocation details.
    difficulty = skill_invocation_difficulty(skill_invocation) + skill_invocation->modifier;

    // Add some randomness for difficulty. Gambling is a special case - the roll
    // is still random, but it's saved as a part of the game to prevent retries
    // with loading.
    rnd_difficulty_roll = basic_skill == BASIC_SKILL_GAMBLING
        ? skill_gambling_random_roll()
        : random_between(1, 100);

    // Determine if roll is successful - the skill succeds if it's success rate
    // is greater than difficulty plus random modifier, or the invocation is a
    // forced success.
    is_success = difficulty + rnd_difficulty_roll <= effectiveness
        || (skill_invocation->flags & SKILL_INVOCATION_FORCED) != 0;

    // Roll for critical hit/critical miss.
    crit_roll = random_between(1, 100);
    if (is_success) {
        // Determine if a hit is critical.
        is_critical = skill_invocation_check_crit_hit(crit_roll, effectiveness, skill_invocation);
    } else {
        // Determine if a miss is critical. Special case - master of melee
        // combat cannot critically fail.
        if (basic_skill == BASIC_SKILL_MELEE && training >= TRAINING_MASTER) {
            is_critical = false;
        } else {
            is_critical = skill_invocation_check_crit_miss(crit_roll, effectiveness, skill_invocation);
        }
    }

    // For PC - check if fate affects the outcome.
    if (obj_field_int32_get(source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        if (basic_skill == BASIC_SKILL_GAMBLING
            && fate_resolve(source_obj, FATE_CRIT_SUCCESS_GAMBLING)) {
            is_success = true;
            is_critical = true;
        } else if (basic_skill == BASIC_SKILL_HEAL
            && fate_resolve(source_obj, FATE_CRIT_SUCCESS_HEAL)) {
            is_success = true;
            is_critical = true;
        } else if (basic_skill == BASIC_SKILL_PICK_POCKET
            && fate_resolve(source_obj, FATE_CRIT_SUCCESS_PICK_POCKET)) {
            is_success = true;
            is_critical = true;
        } else if (tech_skill == TECH_SKILL_REPAIR
            && fate_resolve(source_obj, FATE_CRIT_SUCCESS_REPAIR)) {
            is_success = true;
            is_critical = true;
            is_fate = true;
        } else if (tech_skill == TECH_SKILL_PICK_LOCKS
            && fate_resolve(source_obj, FATE_CRIT_SUCCESS_PICK_LOCKS)) {
            is_success = true;
            is_critical = true;
        } else if (tech_skill == TECH_SKILL_DISARM_TRAPS
            && fate_resolve(source_obj, FATE_CRIT_SUCCESS_DISARM_TRAPS)) {
            is_success = true;
            is_critical = true;
        }
    }

    // Process skill-specific mechanics.
    switch (skill) {
    case SKILL_DODGE:
        // Dodge automatically fails if the source object is incapacitated.
        if (critter_is_unconscious(source_obj)
            || (obj_field_int32_get(source_obj, OBJ_F_SPELL_FLAGS) & OSF_STONED) != 0
            || (obj_field_int32_get(source_obj, OBJ_F_CRITTER_FLAGS) & (OCF_PARALYZED | OCF_STUNNED)) != 0) {
            is_success = false;
            is_critical = false;
        }
        break;
    case SKILL_PICK_POCKET: {
        if ((skill_invocation->flags & SKILL_INVOCATION_0x02) == 0) {
            // Steal item from the target object.
            if (is_success) {
                int64_t parent_obj;

                // Verify the item belongs to the target object.
                if (item_parent(item_obj, &parent_obj) && parent_obj == target_obj) {
                    bool moved;

                    switch (obj_field_int32_get(item_obj, OBJ_F_TYPE)) {
                    case OBJ_TYPE_GOLD: {
                        // Transfer up to 100 gold coins.
                        int qty = item_gold_get(item_obj);
                        if (qty > 100) {
                            qty = 100;
                        }

                        moved = item_gold_transfer(target_obj, source_obj, qty, item_obj);
                        break;
                    }
                    case OBJ_TYPE_AMMO: {
                        // Transfer up to 20 ammo units.
                        int ammo_type = obj_field_int32_get(item_obj, OBJ_F_AMMO_TYPE);
                        int qty = item_ammo_quantity_get(item_obj, ammo_type);
                        if (qty > 20) {
                            qty = 20;
                        }
                        moved = item_ammo_transfer(target_obj, source_obj, qty, ammo_type, item_obj);
                        break;
                    }
                    default:
                        // Transfer item.
                        moved = item_transfer(item_obj, source_obj);
                        break;
                    }

                    if (moved) {
                        // Adjust alignment.
                        sub_45DC90(source_obj, target_obj, false);

                        // Mark item as stolen.
                        unsigned int critter_flags2 = obj_field_int32_get(target_obj, OBJ_F_CRITTER_FLAGS2);
                        critter_flags2 |= OCF2_ITEM_STOLEN;
                        obj_field_int32_set(target_obj, OBJ_F_CRITTER_FLAGS2, critter_flags2);
                    } else {
                        is_success = false;
                        is_critical = false;
                    }
                } else {
                    is_success = false;
                    is_critical = false;
                }
            } else {
                // Failed to steal item - trigger detection. Critical failures
                // are always detected. Non-critical failures are not detected
                // if the thief is at least apprentice of pick pocketing.
                // Otherwise the target needs to perform a successful perception
                // roll in order to catch a thief.
                if (is_critical
                    || (training == TRAINING_NONE && critter_check_stat(target_obj, STAT_PERCEPTION, 0))) {
                    object_script_execute(target_obj, source_obj, item_obj, SAP_CAUGHT_THIEF, 0);
                    if (object_script_execute(source_obj, target_obj, item_obj, SAP_CATCHING_THIEF_PC, 0)) {
                        if (obj_field_int32_get(target_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC
                            || critter_leader_get(target_obj) != source_obj) {
                            ai_attack(source_obj, target_obj, LOUDNESS_NORMAL, 0);
                        }
                    }
                }
            }

            // Notify UI of pickpocketing attempt.
            if (obj_field_int32_get(source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                if (skill_callbacks.steal_item_func != NULL) {
                    skill_callbacks.steal_item_func(source_obj, target_obj, item_obj, is_success);
                }
            }
        } else {
            // Planting an item on the target.
            if (is_success) {
                int64_t parent_obj;

                // Verify the item belongs to the source object.
                if (item_parent(item_obj, &parent_obj) && parent_obj == source_obj) {
                    bool moved;

                    switch (obj_field_int32_get(item_obj, OBJ_F_TYPE)) {
                    case OBJ_TYPE_GOLD: {
                        // Transfer up to 100 gold coins.
                        int qty = item_gold_get(item_obj);
                        if (qty > 100) {
                            qty = 100;
                        }
                        moved = item_gold_transfer(source_obj, target_obj, qty, item_obj);
                        break;
                    }
                    case OBJ_TYPE_AMMO: {
                        // Transfer up to 20 ammo units.
                        int ammo_type = obj_field_int32_get(item_obj, OBJ_F_AMMO_TYPE);
                        int qty = item_ammo_quantity_get(item_obj, ammo_type);
                        if (qty > 20) {
                            qty = 20;
                        }
                        moved = item_ammo_transfer(source_obj, target_obj, qty, ammo_type, item_obj);
                        break;
                    }
                    default:
                        // Transefer item.
                        moved = item_transfer(item_obj, target_obj);
                        break;
                    }

                    if (!moved) {
                        is_success = false;
                        is_critical = false;
                    }
                }
            } else {
                // Failed to plant item - trigger detection. The master of pick
                // pocketing cannot be caught when planting an item. Other than
                // that, the rules for being caught are the same as with
                // stealing an item (see above).
                if (training < TRAINING_MASTER
                    && (is_critical
                        || (training == TRAINING_NONE && critter_check_stat(target_obj, STAT_PERCEPTION, 0)))) {
                    object_script_execute(target_obj, source_obj, item_obj, SAP_CAUGHT_THIEF, 0);
                    if (object_script_execute(source_obj, target_obj, item_obj, SAP_CATCHING_THIEF_PC, 0)) {
                        if (obj_field_int32_get(target_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC
                            || critter_leader_get(target_obj) != source_obj) {
                            ai_attack(source_obj, target_obj, LOUDNESS_NORMAL, 0);
                        }
                    }
                }
            }

            // Notify UI of planting attempt.
            if (obj_field_int32_get(source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                if (skill_callbacks.plant_item_output != NULL) {
                    skill_callbacks.plant_item_output(source_obj, target_obj, item_obj, is_success);
                }
            }
        }
        break;
    }
    case SKILL_SPOT_TRAP:
        // The master of spotting traps gets a second chance (unless the first
        // attempt was a critical failure).
        if (!is_success
            && !is_critical
            && training >= TRAINING_MASTER
            && difficulty + random_between(1, 100) <= effectiveness) {
            is_success = true;
        }

        // Expert training is required to spot magical traps.
        if (is_success
            && training < TRAINING_EXPERT
            && trap_type(target_obj) == TRAP_TYPE_MAGICAL) {
            is_success = false;
            is_critical = false;
        }
        break;
    case SKILL_HEAL: {
        CombatContext combat;

        // Initialize combat invocation for healing.
        sub_4B2210(source_obj, target_obj, &combat);

        if (is_success) {
            int heal;

            // The base amount of healing is effective level of heal skill.
            heal = basic_skill_level(source_obj, BASIC_SKILL_HEAL);

            // Apprentice of healing have a bonus of 50%.
            if (training != TRAINING_NONE) {
                heal = 3 * heal / 2;
            }

            // Make sure there will be at least some healing.
            if (heal < 1) {
                heal = 1;
            }

            // Check if it's a critical success (for the master of healing
            // any success is a critical success).
            if (training == TRAINING_MASTER || is_critical) {
                // Critical success - the amount of heal is maximized...
                combat.dam[DAMAGE_TYPE_NORMAL] = heal;

                // ...and it heals crippling injury.
                effect_remove_one_caused_by(target_obj, EFFECT_CAUSE_INJURY);

                unsigned int critter_flags = obj_field_int32_get(target_obj, OBJ_F_CRITTER_FLAGS);
                if ((critter_flags & OCF_BLINDED) != 0) {
                    critter_flags &= ~OCF_BLINDED;
                } else if ((critter_flags & OCF_CRIPPLED_ARMS_BOTH) != 0) {
                    critter_flags &= ~OCF_CRIPPLED_ARMS_BOTH;
                } else if ((critter_flags & OCF_CRIPPLED_ARMS_ONE) != 0) {
                    critter_flags &= ~OCF_CRIPPLED_ARMS_ONE;
                } else if ((critter_flags & OCF_CRIPPLED_LEGS_BOTH) != 0) {
                    critter_flags &= ~OCF_CRIPPLED_LEGS_BOTH;
                }
                obj_field_int32_set(target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
            } else {
                // Not a critical - the amount of healing is random between
                // 1 and a calculated max.
                combat.dam[DAMAGE_TYPE_NORMAL] = random_between(1, heal);
            }

            // Apply healing.
            combat_heal(&combat);
        } else {
            // Failure to heal. The expert of healing converts critical failure
            // to be mere failure.
            if (is_critical && training >= TRAINING_EXPERT) {
                is_critical = false;
            }
        }

        // Handle healing item usage (e.g., bandages).
        if (item_obj != OBJ_HANDLE_NULL
            && obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_GENERIC
            && (obj_field_int32_get(item_obj, OBJ_F_GENERIC_FLAGS) & OGF_IS_HEALING_ITEM) != 0) {
            int charges = obj_field_int32_get(item_obj, OBJ_F_GENERIC_USAGE_COUNT_REMAINING);
            if (!is_success && is_critical) {
                // Critical failure consumes 5 charges.
                charges -= 5;
            } else {
                // Mere failure consume 1 charge.
                charges -= 1;
            }
            if (charges > 0) {
                // Some charges remain - update appropriate field.
                obj_field_int32_set(item_obj, OBJ_F_GENERIC_USAGE_COUNT_REMAINING, charges);
            } else {
                // No charges left - destroy the item.
                object_destroy(item_obj);
            }
        }
        break;
    }
    case SKILL_REPAIR: {
        int durability_dam_by_training[3];

        // Initialize durability damage table. Master of repair is a special
        // case and is handled differently (see below).
        durability_dam_by_training[TRAINING_NONE] = 10;
        durability_dam_by_training[TRAINING_APPRENTICE] = 5;
        durability_dam_by_training[TRAINING_EXPERT] = 1;

        // Check if the target is repairable.
        int hp_dam = object_hp_damage_get(target_obj);
        if (hp_dam <= 0
            && (tig_art_item_id_destroyed_get(obj_field_int32_get(target_obj, OBJ_F_CURRENT_AID)) == 0
                || training != TRAINING_MASTER)) {
            // Notify UI there is nothing to repair.
            if (player_is_local_pc_obj(source_obj)) {
                if (skill_callbacks.no_repair_func != NULL) {
                    skill_callbacks.no_repair_func(source_obj, target_obj, is_success);
                }
            }
        } else {
            int repair;
            int durability_dam;

            // Calculate the base amount of repair. For fate-driven success the
            // repair restores full HP. Otherwise its proportional to the
            // success rate.
            repair = is_fate
                ? object_hp_max(target_obj)
                : effectiveness * object_hp_max(target_obj) / 100;

            // NOTE: This condition is slightly unclear, I'm not sure it cannot
            // be below 0.
            if (repair <= 0) {
                if (effectiveness > 0 || (skill_invocation->flags & SKILL_INVOCATION_FORCED) != 0) {
                    // Provide minimal healing given success rate is non-zero or
                    // it was a forced success.
                    repair = 1;
                } else {
                    // No actual repairing taking place, indicate failure.
                    repair = 0;
                    is_success = false;
                }
            }

            // Adjust HP damage repaired, make sure it doesn't drop below 0 (as
            // it doesn't make sense).
            hp_dam -= repair;
            if (hp_dam < 0) {
                hp_dam = 0;
            }

            object_hp_damage_set(target_obj, hp_dam);

            // Calculate damage to durability.
            if (training == TRAINING_MASTER) {
                if (tig_art_item_id_destroyed_get(obj_field_int32_get(target_obj, OBJ_F_CURRENT_AID)) != 0) {
                    // Master of repair can fix broken items at the cost of 5%
                    // of max HP.
                    durability_dam = 5;

                    // Remove destroyed flag from art id.
                    tig_art_id_t art_id = obj_field_int32_get(target_obj, OBJ_F_CURRENT_AID);
                    art_id = tig_art_item_id_destroyed_set(art_id, 0);
                    object_set_current_aid(target_obj, art_id);

                    // Such fix is an automatic success.
                    is_success = true;
                } else {
                    // Master of repair have 1% reduction of max HP in case of
                    // critical failure, and no reduction otherwise (mere
                    // failure, success, or critical success).
                    if (is_critical && !is_success) {
                        durability_dam = 1;
                    } else {
                        durability_dam = 0;
                    }
                }
            } else {
                // Use lookup table to determine reduction rate based on
                // training.
                durability_dam = durability_dam_by_training[training];
            }

            // Apply durability reduction.
            if (durability_dam > 0) {
                int dam = durability_dam * object_hp_max(target_obj) / 100;
                object_hp_adj_set(target_obj, object_hp_adj_get(target_obj) - dam);
            }

            // Notify UI of repair attempt.
            if (player_is_local_pc_obj(source_obj)) {
                if (skill_callbacks.repair_func != NULL) {
                    skill_callbacks.repair_func(source_obj, target_obj, is_success);
                }
            }
        }
        break;
    }
    case SKILL_PICK_LOCKS: {
        if (object_locked_get(target_obj)) {
            // Attempt to unlock.
            if (is_success) {
                object_locked_set(target_obj, false);

                // Notify guards.
                ai_notify_portal_container_guards(source_obj, target_obj, false, LOUDNESS_SILENT);
            } else {
                // Critical failure jam the lock.
                if (is_critical) {
                    object_jammed_set(target_obj, true);
                }

                // Notify guards.
                ai_notify_portal_container_guards(source_obj, target_obj, true, LOUDNESS_SILENT);
            }

            // Notify UI of pick lock attempt.
            if (is_controlled_by_local_pc(source_obj)) {
                if (skill_callbacks.lock_func != NULL) {
                    skill_callbacks.lock_func(source_obj, target_obj, is_success);
                }
            }
        } else {
            // Atempt to lock.
            if (is_success) {
                object_locked_set(target_obj, true);

                // Notify UI of pick lock attempt.
                if (player_is_local_pc_obj(source_obj)) {
                    if (skill_callbacks.lock_func != NULL) {
                        skill_callbacks.lock_func(source_obj, target_obj, is_success);
                    }
                }
            }
        }
        break;
    }
    case SKILL_DISARM_TRAPS: {
        if ((skill_invocation->flags & SKILL_INVOCATION_0x04) != 0) {
            // The master of disarming traps gets a second chance (unless the
            // first attempt was a critical failure).
            if (!is_success
                && !is_critical
                && training >= TRAINING_MASTER
                && difficulty + random_between(1, 100) <= effectiveness) {
                is_success = true;
            }

            // Process trap disarming.
            trap_handle_disarm(source_obj, target_obj, &is_success, &is_critical);

            // Notify UI of disarm trap attempt.
            if (is_controlled_by_local_pc(source_obj)) {
                if (skill_callbacks.disarm_trap_func != NULL) {
                    skill_callbacks.disarm_trap_func(source_obj, target_obj, is_success);
                }
            }
        } else {
            // TODO: Unclear.
            if (player_is_local_pc_obj(source_obj)) {
                if (skill_callbacks.field_C != NULL) {
                    skill_callbacks.field_C(source_obj, target_obj, is_success);
                }
            }
        }
        break;
    }
    default:
        break;
    }

    // Update invocation flags depending on the outcome.
    if (is_success) {
        skill_invocation->flags |= SKILL_INVOCATION_SUCCESS;
    } else {
        skill_invocation->flags &= ~SKILL_INVOCATION_SUCCESS;
    }

    if (is_critical) {
        skill_invocation->flags |= SKILL_INVOCATION_CRITICAL;
    } else {
        skill_invocation->flags &= ~SKILL_INVOCATION_CRITICAL;
    }

    return true;
}

/**
 * Determines if a skill invocation results in a critical hit based on a random
 * roll and success rate.
 *
 * Returns `true` if it's a critical hit, or `false` otherwise.
 *
 * 0x4C81A0
 */
bool skill_invocation_check_crit_hit(int roll, int effectiveness, SkillInvocation* skill_invocation)
{
    int divisor;
    int chance;

    // Determine the base critical hit divisor based on skill type. Combat
    // skills (e.g., bow, melee) have a higher divisor, making critical hits
    // less frequent, while non-combat skills use a lower divisor, increasing
    // critical hit likelihood.
    divisor = (skill_flags[skill_invocation->skill] & SKILL_COMBAT) != 0 ? 20 : 2;

    // Calculate the base critical hit chance by dividing effectiveness by the
    // divisor. For example, a bow with 80% success rate would have 4% chance to
    // become critical.
    chance = effectiveness / divisor;

    // Adjust for aimed shots based on the hit location penalty. For example,
    // aiming at head have -50% to-hit penalty, which translates to a bonus of
    // 10 percentage points.
    if ((skill_invocation->flags & SKILL_INVOCATION_AIM) != 0) {
        chance += combat_hit_loc_penalty(skill_invocation->hit_loc) / -5;
    }

    // Apply magical critical hit adjustment from the weapon (unless explicitly
    // disabled by appropriate flag).
    if (skill_invocation->item.obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(skill_invocation->item.obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON
        && (skill_invocation->flags & SKILL_INVOCATION_NO_MAGIC_ADJ) == 0) {
        chance += item_adjust_magic(skill_invocation->item.obj,
            skill_invocation->source.obj,
            obj_field_int32_get(skill_invocation->item.obj, OBJ_F_WEAPON_MAGIC_CRIT_HIT_CHANCE));
    }

    // Apply backstab-specific mechanics.
    if ((skill_invocation->flags & SKILL_INVOCATION_BACKSTAB) != 0) {
        // The backstab bonus to critical hit is the difference between doubled
        // backstab effective level (range 0-40) and target level (1-51). This
        // effectively means that even with the maxed backstab you'll get a
        // penalty trying to backstab high-level critters.
        chance += 2 * basic_skill_level(skill_invocation->source.obj, BASIC_SKILL_BACKSTAB);
        chance -= stat_level_get(skill_invocation->target.obj, STAT_LEVEL);

        // The master of backstabbing "gains enourmous increase to his chance of
        // scoring a critical success" (from the manual).
        if (basic_skill_training_get(skill_invocation->source.obj, BASIC_SKILL_BACKSTAB) == TRAINING_MASTER) {
            chance += 20;
        }
    }

    // Apply effects.
    chance = effect_adjust_crit_hit_chance(skill_invocation->source.obj, chance);

    // Compare the roll (1-100) to the final critical hit chance to determine if
    // a critical hit occurs.
    return roll <= chance;
}

/**
 * Determines if a skill invocation results in a critical miss based on a random
 * roll and success rate.
 *
 * Returns `true` if it's a critical miss, or `false` otherwise.
 *
 * 0x4C82E0
 */
bool skill_invocation_check_crit_miss(int roll, int effectiveness, SkillInvocation* skill_invocation)
{
    int divisor;
    int chance;
    int hp_curr;
    int hp_max;

    // Determine the base critical miss divisor based on skill type. Combat
    // skills (e.g., bow, melee) have a higher divisor, making critical misses
    // less frequent, while non-combat skills use a lower divisor, increasing
    // critical miss likelihood.
    divisor = (skill_flags[skill_invocation->skill] & SKILL_COMBAT) != 0 ? 7 : 2;

    // Calculate the base critical miss chance by dividing effectiveness by the
    // divisor. For example, a bow with 80% success rate would have 2% chance to
    // become critical (2.86% to be precise).
    chance = (100 - effectiveness) / divisor;

    if (skill_invocation->item.obj != OBJ_HANDLE_NULL) {
        // Adjust for item condition - damaged items increase the critical miss
        // chance proportional to the percentage of damage.
        hp_curr = object_hp_current(skill_invocation->item.obj);
        hp_max = object_hp_max(skill_invocation->item.obj);
        if (hp_curr < hp_max) {
            chance += 100 * (hp_max - hp_curr) / (hp_max * divisor);
        }

        // Apply magical critical miss adjustment from the weapon (unless
        // explicitly disabled by appropriate flag).
        if (obj_field_int32_get(skill_invocation->item.obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON
            && (skill_invocation->flags & SKILL_INVOCATION_NO_MAGIC_ADJ) == 0) {
            chance += item_adjust_magic(skill_invocation->item.obj,
                skill_invocation->source.obj,
                obj_field_int32_get(skill_invocation->item.obj, OBJ_F_WEAPON_MAGIC_CRIT_MISS_CHANCE));
        }
    }

    // Apply effects.
    chance = effect_adjust_crit_fail_chance(skill_invocation->source.obj, chance);

    // The minimal critical miss chance is 2%.
    if (chance < 2) {
        chance = 2;
    }

    // Compare the roll (1-100) to the final critical miss chance to determine
    // if a critical miss occurs.
    return roll <= chance;
}

/**
 * Determines if the specified object is either a local player character, or
 * controlled by local PC.
 *
 * This function is used to check if UI updates are needed.
 *
 * 0x4C83E0
 */
bool is_controlled_by_local_pc(int64_t obj)
{
    int64_t leader_obj;

    if (player_is_local_pc_obj(obj)) {
        return true;
    }

    leader_obj = critter_leader_get(obj);
    if (leader_obj != OBJ_HANDLE_NULL && player_is_local_pc_obj(leader_obj)) {
        return true;
    }

    return false;
}

/**
 * Calculates the difficulty score for performing a skill invocation.
 *
 * 0x4C8430
 */
int skill_invocation_difficulty(SkillInvocation* skill_invocation)
{
    int64_t source_obj;
    int64_t target_obj;
    int64_t item_obj;
    int skill;
    int basic_skill;
    int tech_skill;
    int effectiveness;
    int training;
    int difficulty;
    int target_type;

    source_obj = skill_invocation->source.obj;
    target_obj = skill_invocation->target.obj;
    item_obj = skill_invocation->item.obj;
    skill = skill_invocation->skill;

    // Determine concrete skills, calculate effectiveness and training level.
    if (IS_TECH_SKILL(skill)) {
        basic_skill = -1;
        tech_skill = GET_TECH_SKILL(skill);
        effectiveness = tech_skill_effectiveness(source_obj, tech_skill, target_obj);
        training = tech_skill_training_get(source_obj, tech_skill);
    } else {
        basic_skill = GET_BASIC_SKILL(skill);
        tech_skill = -1;
        effectiveness = basic_skill_effectiveness(source_obj, basic_skill, target_obj);
        training = basic_skill_training_get(source_obj, basic_skill);
    }

    // Initialize difficulty with the base modifier from the skill invocation.
    difficulty = skill_invocation->modifier;

    // Set the target location if a target object is provided.
    if (target_obj != OBJ_HANDLE_NULL) {
        skill_invocation->target_loc = obj_field_int64_get(target_obj, OBJ_F_LOCATION);
    }

    if (item_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
        int to_hit;
        int strength_difference;

        to_hit = obj_field_int32_get(item_obj, OBJ_F_WEAPON_BONUS_TO_HIT);

        // Apply magical "to hit" adjustment from the weapon (unless explicitly
        // disabled by appropriate flag).
        if ((skill_invocation->flags & SKILL_INVOCATION_NO_MAGIC_ADJ) == 0) {
            to_hit += item_adjust_magic(item_obj,
                source_obj,
                obj_field_int32_get(item_obj, OBJ_F_WEAPON_MAGIC_HIT_ADJ));
        }

        // Reduce difficulty by the weapon's to-hit bonus.
        difficulty -= to_hit;

        // Apply a penalty if the source's strength is below the weapon's
        // minimum requirement.
        strength_difference = stat_level_get(source_obj, STAT_STRENGTH) - item_weapon_min_strength(item_obj, source_obj);
        if (strength_difference < 0) {
            difficulty += -strength_difference * 5;
            skill_invocation->flags |= SKILL_INVOCATION_PENALTY_MSR;
        }
    }

    // Determine the target type if a target object is provided.
    if (target_obj != OBJ_HANDLE_NULL) {
        target_type = obj_field_int32_get(target_obj, OBJ_F_TYPE);
    }

    // Apply awareness-based modifiers for skills that require detecting the
    // target's state.
    if ((skill_flags[skill] & SKILL_CHECK_AWARENESS) != 0
        && target_obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(target_type)) {
        unsigned int critter_flags = obj_field_int32_get(target_obj, OBJ_F_CRITTER_FLAGS);

        // Reduce the difficulty for incapacitated targets.
        if ((critter_flags & OCF_PARALYZED) != 0) {
            difficulty -= 50;
        } else if (critter_is_unconscious(target_obj)) {
            difficulty -= 50;
        } else if (tig_art_id_anim_get(obj_field_int32_get(target_obj, OBJ_F_CURRENT_AID)) == TIG_ART_ANIM_FALL_DOWN
            || (critter_flags & OCF_STUNNED) != 0) {
            difficulty -= 30;
        } else if ((critter_flags & OCF_SLEEPING) != 0) {
            difficulty -= 50;
        } else if (basic_skill != BASIC_SKILL_PROWLING
            && !combat_critter_is_combat_mode_active(target_obj)
            && ai_can_see(target_obj, source_obj) != 0
            && ai_can_hear(target_obj, source_obj, 1) != 0) {
            // Target is not in combat mode and does not hear or see source.
            difficulty -= 30;
        }
    }

    // Apply hit location modifiers for skills that involve targeting specific
    // body parts.
    if ((skill_flags[skill] & SKILL_CHECK_HIT_LOC) != 0 && target_obj != OBJ_HANDLE_NULL) {
        // Check if called shot is being made.
        if ((skill_invocation->flags & SKILL_INVOCATION_AIM) != 0) {
            int penalty = combat_hit_loc_penalty(skill_invocation->hit_loc);

            // The expert in firearms make called shots with 2/3 of the penalty.
            if (tech_skill == TECH_SKILL_FIREARMS && training >= TRAINING_EXPERT) {
                penalty = 2 * penalty / 3;
            }

            // Penalty is already negative, so subtracting penalty actually
            // increases difficulty.
            difficulty -= penalty;
        }

        // Increase difficulty by target's armor class (a backstab apprentice
        // bypasses).
        if ((skill_invocation->flags & SKILL_INVOCATION_BACKSTAB) == 0
            || basic_skill_training_get(source_obj, BASIC_SKILL_BACKSTAB) == TRAINING_NONE) {
            difficulty += effectiveness * (object_get_ac(target_obj, false) / 2) / 100;
        }
    }

    // Apply distance-based modifiers for skills that depend on range.
    if ((skill_flags[skill] & SKILL_CHECK_DISTANCE) != 0) {
        int64_t dist = location_dist(skill_invocation->target_loc, obj_field_int64_get(source_obj, OBJ_F_LOCATION));

        // Check if objects are way to far away from each other.
        if (dist > INT_MAX) {
            return 1000000;
        }

        // Check if the target is within the item's range (for weapons or
        // throwing).
        if (item_obj != OBJ_HANDLE_NULL) {
            int range;

            // Obtain weapon/throwable item range.
            if (basic_skill == BASIC_SKILL_THROWING) {
                range = item_throwing_distance(item_obj, source_obj);
            } else {
                range = obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON
                    ? item_weapon_range(item_obj, source_obj)
                    : 10;
            }

            if (dist > range) {
                // Out-of-range target is impossible to hit.
                difficulty += 1000000;
                skill_invocation->flags |= SKILL_INVOCATION_PENALTY_RANGE;
            }
        }

        // Add long distance penalty. The safe distance is half of the source's
        // perception (thus maxed at 10). The penalty is 5 percentage points per
        // each additional tile. The masters of bow, throwing, and firearms are
        // unaffected.
        if ((basic_skill != BASIC_SKILL_BOW
                && basic_skill != BASIC_SKILL_THROWING
                && tech_skill != TECH_SKILL_FIREARMS)
            || training < TRAINING_MASTER) {
            int extra_dist = (int)dist - stat_level_get(source_obj, STAT_PERCEPTION) / 2;
            if (extra_dist > 0) {
                difficulty += 5 * extra_dist;
                skill_invocation->flags |= SKILL_INVOCATION_PENALTY_PERCEPTION;
            }
        }

        // Check for obstructions between source and target.
        int64_t blocking_obj;
        int v2 = sub_4ADE00(source_obj, skill_invocation->target_loc, &blocking_obj);
        if (blocking_obj != OBJ_HANDLE_NULL) {
            // Target is blocked.
            difficulty += 1000000;
            skill_invocation->flags |= SKILL_INVOCATION_BLOCKED_SHOT;
        } else if (v2 > 0) {
            // TODO: Unclear.
            difficulty += v2;
            skill_invocation->flags |= SKILL_INVOCATION_PENALTY_COVER;
        }
    }

    unsigned int critter_flags = obj_field_int32_get(source_obj, OBJ_F_CRITTER_FLAGS);

    // Calculate lighting penalty.
    if ((critter_flags & OCF_UNDEAD) == 0
        && ((skill_flags[skill] & SKILL_CHECK_LIGHT) != 0
            || (skill_invocation->flags & SKILL_INVOCATION_CHECK_SEEING) != 0)) {
        bool affected = true;

        // Expert of melee combat/pick locks is unaffected by light penalties.
        if ((basic_skill == BASIC_SKILL_MELEE
                || tech_skill == TECH_SKILL_PICK_LOCKS)
            && training >= TRAINING_EXPERT) {
            affected = false;
        }

        if ((((skill_invocation->flags & SKILL_INVOCATION_CHECK_SEEING) == 0
                 && basic_skill != BASIC_SKILL_SPOT_TRAP
                 && tech_skill != TECH_SKILL_DISARM_TRAPS)
                || training < TRAINING_APPRENTICE)
            && affected) {
            int lum;
            if ((skill_invocation->flags & SKILL_INVOCATION_CHECK_SEEING) != 0) {
                // Take light level of the source object.
                lum = sub_4DCE10(source_obj) & 0xFF;
            } else if (target_obj != OBJ_HANDLE_NULL) {
                // Take light level of the target object.
                lum = sub_4DCE10(target_obj) & 0xFF;
            } else {
                // Take luminance of a target location.
                lum = sub_4D9240(skill_invocation->target_loc, 0, 0) & 0xFF;
            }

            // Calculate lighting penalty based on darkness (where 255 being
            // bright light, and 0 is a complete darkness).
            int penalty = 30 * (255 - lum) / 255;

            // Adjust penalty based on critter's light emission.
            if ((critter_flags & OCF_LIGHT_XLARGE) != 0) {
                penalty /= 4;
            } else if ((critter_flags & OCF_LIGHT_LARGE) != 0) {
                penalty /= 3;
            } else if ((critter_flags & OCF_LIGHT_MEDIUM) != 0) {
                penalty /= 2;
            } else if ((critter_flags & OCF_LIGHT_SMALL) != 0) {
                penalty = 2 * penalty / 3;
            }

            // Adjust for dark sight.
            if ((skill_invocation->flags & SKILL_INVOCATION_CHECK_SEEING) != 0) {
                if (!critter_has_dark_sight(target_obj)) {
                    penalty = 30 - penalty;
                }
            } else {
                if (critter_has_dark_sight(source_obj)) {
                    penalty = 30 - penalty;
                }
            }

            if (penalty > 0) {
                difficulty += penalty;
                skill_invocation->flags |= SKILL_INVOCATION_PENALTY_LIGHT;
            }
        }
    }

    // Apply injury-based modifiers for skills requiring vision.
    if ((skill_flags[skill] & SKILL_USE_EYES) != 0
        && (critter_flags & OCF_BLINDED) != 0) {
        difficulty += 30;
        skill_invocation->flags |= SKILL_INVOCATION_PENALTY_INJURY;
    }

    // Apply injury-based modifiers for skills requiring arms.
    if ((skill_flags[skill] & SKILL_USE_ARMS) != 0) {
        if ((critter_flags & OCF_CRIPPLED_ARMS_BOTH) != 0) {
            difficulty += 50;
            skill_invocation->flags |= SKILL_INVOCATION_PENALTY_INJURY;
        } else if ((critter_flags & OCF_CRIPPLED_ARMS_ONE) != 0) {
            difficulty += 20;
            skill_invocation->flags |= SKILL_INVOCATION_PENALTY_INJURY;
        }
    }

    // Apply injury-based modifiers for skills requiring legs.
    if ((skill_flags[skill] & SKILL_USE_LEGS) != 0) {
        if ((critter_flags & OCF_CRIPPLED_LEGS_BOTH) != 0) {
            difficulty += 30;
            skill_invocation->flags |= SKILL_INVOCATION_PENALTY_INJURY;
        }
    }

    // Apply skill-specific modifiers.
    switch (skill) {
    case SKILL_PICK_POCKET: {
        int inventory_location;
        int width;
        int height;
        int penalty;

        // Apply penalties based on the item's inventory location (worn items
        // are harder to steal)
        inventory_location = item_inventory_location_get(item_obj);
        if (IS_WEAR_INV_LOC(inventory_location)) {
            difficulty += dword_5B70E4[inventory_location - FIRST_WEAR_INV_LOC];
        }

        // Apply penalty based on the item's size (larger items are harder to
        // steal).
        item_inv_icon_size(item_obj, &width, &height);

        // The penalty is 5 percentage points per square.
        penalty = 5 * width * height;

        // The expert of pick pocketing have item size penalty halved.
        if (training >= TRAINING_EXPERT) {
            penalty /= 2;
        }

        difficulty += penalty;
        break;
    }
    case SKILL_PROWLING: {
        if ((skill_invocation->flags & SKILL_INVOCATION_CHECK_SEEING) != 0) {
            // Check for blocking objects when hiding from a target.
            int64_t blocking_obj;
            int v6 = sub_4ADE00(target_obj,
                obj_field_int64_get(source_obj, OBJ_F_LOCATION),
                &blocking_obj);
            if (blocking_obj != OBJ_HANDLE_NULL) {
                // There is a blocking object between source and target,
                // automatic success.
                difficulty = -100;
                break;
            }

            // TODO: Unclear.
            if (training < TRAINING_EXPERT && v6 == 0) {
                v6 = -50;
            }

            difficulty -= v6;
        } else if ((skill_invocation->flags & SKILL_INVOCATION_CHECK_HEARING) != 0) {
            // Apply armor-based modifier for silent movement.
            for (int inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
                int64_t inv_item_obj = item_wield_get(source_obj, inventory_location);
                if (inv_item_obj != OBJ_HANDLE_NULL
                    && obj_field_int32_get(inv_item_obj, OBJ_F_TYPE) == OBJ_TYPE_ARMOR) {
                    int penalty = obj_field_int32_get(inv_item_obj, OBJ_F_ARMOR_SILENT_MOVE_ADJ);
                    penalty += item_adjust_magic(inv_item_obj,
                        source_obj,
                        obj_field_int32_get(inv_item_obj, OBJ_F_ARMOR_MAGIC_SILENT_MOVE_ADJ));

                    // The apprentice of prowling have armor penalty halved.
                    if (training >= TRAINING_APPRENTICE) {
                        penalty /= 2;
                    }

                    difficulty -= penalty;
                }
            }
        }
        break;
    }
    case SKILL_HEAL:
        // Apply bonus from healing items to reduce difficulty.
        if (item_obj != OBJ_HANDLE_NULL
            && obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_GENERIC
            && (obj_field_int32_get(item_obj, OBJ_F_GENERIC_FLAGS) & OGF_IS_HEALING_ITEM) != 0) {
            difficulty -= obj_field_int32_get(item_obj, OBJ_F_GENERIC_USAGE_BONUS);
        }
        break;
    case SKILL_PICK_LOCKS:
        if (target_obj != OBJ_HANDLE_NULL
            && (target_type == OBJ_TYPE_PORTAL || target_type == OBJ_TYPE_CONTAINER)) {
            // Apply bonus from lock pick items to reduce difficulty.
            if (item_obj != OBJ_HANDLE_NULL
                && obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_GENERIC
                && (obj_field_int32_get(item_obj, OBJ_F_GENERIC_FLAGS) & OGF_IS_LOCKPICK) != 0) {
                difficulty -= obj_field_int32_get(item_obj, OBJ_F_GENERIC_USAGE_BONUS);
            }

            // Apply penalty from lock difficulty.
            int lock_difficulty;
            if (target_type == OBJ_TYPE_PORTAL) {
                lock_difficulty = obj_field_int32_get(target_obj, OBJ_F_PORTAL_LOCK_DIFFICULTY);
            } else {
                lock_difficulty = obj_field_int32_get(target_obj, OBJ_F_CONTAINER_LOCK_DIFFICULTY);
            }

            // The master of lock picking have lock's difficulty penalty halved.
            if (training >= TRAINING_MASTER && lock_difficulty > 0) {
                lock_difficulty /= 2;
            }

            difficulty += lock_difficulty;
        } else {
            // Invalid target type.
            difficulty += 1000000;
        }
        break;
    case SKILL_DISARM_TRAPS:
        // Apply trap difficulty.
        if (target_obj != OBJ_HANDLE_NULL
            && target_type == OBJ_TYPE_TRAP) {
            difficulty += obj_field_int32_get(target_obj, OBJ_F_TRAP_DIFFICULTY);
        }
        break;
    }

    return difficulty;
}

/**
 * Performs a repair service by an NPC on an item for a PC, handling skill
 * invocation and payment.
 *
 * 0x4C8E60
 */
void skill_perform_repair_service(int64_t item_obj, int64_t npc_obj, int64_t pc_obj, int cost)
{
    SkillInvocation skill_invocation;

    // Set up and run repair skill invocation with force success flag set.
    skill_invocation_init(&skill_invocation);
    skill_invocation.flags |= SKILL_INVOCATION_FORCED;
    skill_invocation.skill = SKILL_REPAIR;
    sub_4440E0(npc_obj, &(skill_invocation.source));
    sub_4440E0(item_obj, &(skill_invocation.target));
    skill_invocation_run(&skill_invocation);

    // Transfer the specified amount of money from the PC to the NPC as payment
    // for the service.
    item_gold_transfer(pc_obj, npc_obj, cost, OBJ_HANDLE_NULL);

    // Update examine item UI.
    sub_4EE3A0(pc_obj, item_obj);
}

/**
 * Determines if follower skills are enabled for a given PC.
 *
 * The `obj` is only meaningful in the multiplayer games.
 *
 * 0x4C8FA0
 */
bool get_follower_skills(int64_t obj)
{
    // In single-player mode simply retrieve the value from settings.
    return settings_get_value(&settings, FOLLOWER_SKILLS_KEY);
}

/**
 * Sets whether to use follower skills enabled or not.
 *
 * 0x4C8FF0
 */
void set_follower_skills(bool enabled)
{
    // In single-player mode simply set the value in settings.
    settings_set_value(&settings, FOLLOWER_SKILLS_KEY, enabled);
}

/**
 * Selects the follower with the highest effective skill level for a given skill
 * invocation.
 *
 * This function evaluates all followers of a PC to determine which one has the
 * highest effective skill level for the specified skill invcation. The source
 * object (the one who will actually "do the job") is changed in place.
 *
 * 0x4C9050
 */
void skill_pick_best_follower(SkillInvocation* skill_invocation)
{
    ObjectList objects;
    ObjectNode* node;
    int effectiveness;
    int best_effectiveness;
    int64_t best_follower_obj;

    // Ensure the source is a PC.
    if (obj_field_int32_get(skill_invocation->source.obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    // Check if follower skills are enabled.
    if (!get_follower_skills(skill_invocation->source.obj)) {
        return;
    }

    if (IS_TECH_SKILL(skill_invocation->skill)) {
        // Calculate the PC's effectiveness for the specified tech skill.
        best_effectiveness = tech_skill_effectiveness(skill_invocation->source.obj, GET_TECH_SKILL(skill_invocation->skill), skill_invocation->target.obj) - skill_invocation_difficulty(skill_invocation);
        best_follower_obj = skill_invocation->source.obj;

        // Iterate through all followers to find the one with the highest
        // effectiveness.
        object_list_all_followers(best_follower_obj, &objects);
        node = objects.head;
        while (node != NULL) {
            // Temporarily set the follower to be the source object of the
            // invocation. This is needed to calculate invocation's difficulty.
            sub_4440E0(node->obj, &(skill_invocation->source));
            effectiveness = tech_skill_effectiveness(skill_invocation->source.obj, GET_TECH_SKILL(skill_invocation->skill), skill_invocation->target.obj) - skill_invocation_difficulty(skill_invocation);

            // Update the best follower if this one has higher effectiveness.
            if (effectiveness > best_effectiveness) {
                best_effectiveness = effectiveness;
                best_follower_obj = node->obj;
            }

            node = node->next;
        }

        // FIX: Original code leaks `objects`.
        object_list_destroy(&objects);
    } else {
        // Calculate the PC's effectiveness for the specified basic skill.
        best_effectiveness = basic_skill_effectiveness(skill_invocation->source.obj, GET_BASIC_SKILL(skill_invocation->skill), skill_invocation->target.obj) - skill_invocation_difficulty(skill_invocation);
        best_follower_obj = skill_invocation->source.obj;

        // Iterate through all followers to find the one with the highest
        // effectiveness.
        object_list_all_followers(best_follower_obj, &objects);
        node = objects.head;
        while (node != NULL) {
            // Temporarily set the follower to be the source object of the
            // invocation. This is needed to calculate invocation's difficulty.
            sub_4440E0(node->obj, &(skill_invocation->source));
            effectiveness = basic_skill_effectiveness(skill_invocation->source.obj, GET_BASIC_SKILL(skill_invocation->skill), skill_invocation->target.obj) - skill_invocation_difficulty(skill_invocation);

            // Update the best follower if this one has higher effectiveness.
            if (effectiveness > best_effectiveness) {
                best_effectiveness = effectiveness;
                best_follower_obj = node->obj;
            }

            node = node->next;
        }

        // FIX: Original code leaks `objects`.
        object_list_destroy(&objects);
    }

    // Update the skill invocation's source to the best follower.
    sub_4440E0(best_follower_obj, &(skill_invocation->source));
}

/**
 * Retrieves a supplementary item for a specific skill from a critter's
 * inventory.
 *
 * 0x4C91F0
 */
int64_t skill_supplementary_item(int64_t obj, int skill)
{
    int64_t item_obj = OBJ_HANDLE_NULL;
    int64_t substitute_inventory_obj;

    switch (skill) {
    case SKILL_HEAL:
        // Search the critter's inventory for healing item.
        item_obj = item_find_first_generic(obj, OGF_IS_HEALING_ITEM);
        if (item_obj == OBJ_HANDLE_NULL) {
            // Check the substitute inventory.
            substitute_inventory_obj = critter_substitute_inventory_get(obj);
            if (substitute_inventory_obj != OBJ_HANDLE_NULL) {
                item_obj = item_find_first_generic(substitute_inventory_obj, OGF_IS_HEALING_ITEM);
            }
        }
        break;
    case SKILL_PICK_LOCKS:
        // Search the critter's inventory for lock picks.
        item_obj = item_find_first_generic(obj, OGF_IS_LOCKPICK);
        if (item_obj == OBJ_HANDLE_NULL) {
            // Check the substitute inventory.
            substitute_inventory_obj = critter_substitute_inventory_get(obj);
            if (substitute_inventory_obj != OBJ_HANDLE_NULL) {
                item_obj = item_find_first_generic(substitute_inventory_obj, OGF_IS_LOCKPICK);
            }
        }
        break;
    }

    return item_obj;
}
