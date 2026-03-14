#include "game/effect.h"

#include "game/anim.h"
#include "game/critter.h"
#include "game/mes.h"
#include "game/resistance.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/tech.h"

/**
 * Defines the maximum number of effects per effect type.
 */
#define EFFECT_LIST_CAPACITY 400

typedef enum EffectSpecial {
    EFFECT_SPECIAL_MAX_HIT_POINTS,
    EFFECT_SPECIAL_MAX_FATIGUE,
    EFFECT_SPECIAL_REACTION,
    EFFECT_SPECIAL_CRIT_HIT_CHANCE,
    EFFECT_SPECIAL_CRIT_HIT_EFFECT,
    EFFECT_SPECIAL_CRIT_FAIL_CHANCE,
    EFFECT_SPECIAL_CRIT_FAIL_EFFECT,
    EFFECT_SPECIAL_BAD_REACTION_ADJUSTMENT,
    EFFECT_SPECIAL_GOOD_REACTION_ADJUSTMENT,
    EFFECT_SPECIAL_XP_GAIN,
    EFFECT_SPECIAL_COUNT,
} EffectSpecial;

typedef enum EffectOperator {
    EFFECT_OPERATOR_ADD,
    EFFECT_OPERATOR_MULTIPLY,
    EFFECT_OPERATOR_DIVIDE,
    EFFECT_OPERATOR_MIN,
    EFFECT_OPERATOR_MAX,
    EFFECT_OPERATOR_PERCENT,
} EffectOperator;

/**
 * Defines the maximum number of changes a single effect is allowed to have
 * per attribute collection.
 *
 * NOTE: Here is a (misleading) explanation `effect.mes`:
 *
 * "WARNING: Each effect has a constraint of a maximum of *FIVE* changes per
 * *type* of change (so you could have 5 changes to stats *and* 5 changes to
 * skills, but not 6 changes to stats)".
 *
 * This message states that 5 is the max, however the memory layout indicates it
 * has 7 elements.
 *
 * This value can be safely increased.
 */
#define MAX_CHANGE_COUNT 7

/**
 * This structure represents changes that should be made to appropriate
 * attributes when the effect applies.
 *
 * Each effect describes up to `MAX_CHANGE_COUNT` changes made to the attributes
 * of the same group. The attribute IDs depends on the parent collection - it's
 * members of `Stat` enum in `effect_stat_effects`, `BasicSkill` - in
 * `effect_basic_skill_effects` and so on.
 *
 * See:
 *  - `effect_stat_effects`
 *  - `effect_basic_skill_effects`
 *  - `effect_tech_skill_effects`
 *  - `effect_resistance_effects`
 *  - `effect_tech_effects`
 *  - `effect_special_effects`
 */
typedef struct Effect {
    int count;
    int ids[MAX_CHANGE_COUNT];
    int params[MAX_CHANGE_COUNT];
    int operators[MAX_CHANGE_COUNT];
} Effect;

static void effect_parse(int num, char* text);
static void effect_remove_internal(int64_t obj, int index);
static int effect_adjust_func(int64_t obj, int id, int value, Effect* tbl, bool ignore_innate_effects);

/**
 * 0x5B9BA8
 */
static const char* effect_stat_lookup_tbl[STAT_COUNT] = {
    /*             STAT_STRENGTH */ "st",
    /*            STAT_DEXTERITY */ "dx",
    /*         STAT_CONSTITUTION */ "cn",
    /*               STAT_BEAUTY */ "be",
    /*         STAT_INTELLIGENCE */ "in",
    /*           STAT_PERCEPTION */ "pe",
    /*            STAT_WILLPOWER */ "wp",
    /*             STAT_CHARISMA */ "ch",
    /*         STAT_CARRY_WEIGHT */ "carry",
    /*         STAT_DAMAGE_BONUS */ "damage",
    /*        STAT_AC_ADJUSTMENT */ "ac",
    /*                STAT_SPEED */ "speed",
    /*            STAT_HEAL_RATE */ "healrate",
    /*      STAT_POISON_RECOVERY */ "poisonrate",
    /*    STAT_REACTION_MODIFIER */ "beautyreaction",
    /*        STAT_MAX_FOLLOWERS */ "maxfollowers",
    /* STAT_MAGICK_TECH_APTITUDE */ "aptitude",
    /*                STAT_LEVEL */ "level",
    /*    STAT_EXPERIENCE_POINTS */ "xpsUNUSED",
    /*            STAT_ALIGNMENT */ "alignment",
    /*          STAT_FATE_POINTS */ "fateUNUSED",
    /*       STAT_UNSPENT_POINTS */ "unspentUNUSED",
    /*        STAT_MAGICK_POINTS */ "magicpts",
    /*          STAT_TECH_POINTS */ "techpts",
    /*         STAT_POISON_LEVEL */ "poisonlevel",
    /*                  STAT_AGE */ "age",
    /*               STAT_GENDER */ "gender",
    /*                 STAT_RACE */ "race",
};

/**
 * 0x5B9C18
 */
static const char* effect_basic_skill_lookup_tbl[BASIC_SKILL_COUNT] = {
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
 * 0x5B9C48
 */
static const char* effect_tech_skill_lookup_tbl[TECH_SKILL_COUNT] = {
    /*       TECH_SKILL_REPAIR */ "repair",
    /*     TECH_SKILL_FIREARMS */ "firearms",
    /*   TECH_SKILL_PICK_LOCKS */ "picklock",
    /* TECH_SKILL_DISARM_TRAPS */ "armtrap",
};

/**
 * 0x5B9C58
 */
static const char* effect_resistance_lookup_tbl[RESISTANCE_TYPE_COUNT] = {
    /*     RESISTANCE_TYPE_NORMAL */ "resistdamage",
    /*       RESISTANCE_TYPE_FIRE */ "resistfire",
    /* RESISTANCE_TYPE_ELECTRICAL */ "resistelectrical",
    /*     RESISTANCE_TYPE_POISON */ "resistpoison",
    /*      RESISTANCE_TYPE_MAGIC */ "resistmagic",
};

/**
 * 0x5B9C6C
 */
static const char* effect_tech_discipline_lookup_tbl[TECH_COUNT] = {
    /*    TECH_HERBOLOGY */ "expertiseanatomical",
    /*    TECH_CHEMISTRY */ "expertisechemistry",
    /*     TECH_ELECTRIC */ "expertiseelectric",
    /*   TECH_EXPLOSIVES */ "expertiseexplosives",
    /*          TECH_GUN */ "expertisegun_smithy",
    /*   TECH_MECHANICAL */ "expertisemechanical",
    /*       TECH_SMITHY */ "expertisesmithy",
    /* TECH_THERAPEUTICS */ "expertisetherapeutics",
};

/**
 * 0x5B9C8C
 */
static const char* effect_special_attributes_lookup_tbl[EFFECT_SPECIAL_COUNT] = {
    /*           EFFECT_SPECIAL_MAX_HIT_POINTS */ "maxhps",
    /*              EFFECT_SPECIAL_MAX_FATIGUE */ "maxfatigue",
    /*                 EFFECT_SPECIAL_REACTION */ "reaction",
    /*          EFFECT_SPECIAL_CRIT_HIT_CHANCE */ "crithitchance",
    /*          EFFECT_SPECIAL_CRIT_HIT_EFFECT */ "crithiteffect",
    /*         EFFECT_SPECIAL_CRIT_FAIL_CHANCE */ "critfailchance",
    /*         EFFECT_SPECIAL_CRIT_FAIL_EFFECT */ "critfaileffect",
    /*  EFFECT_SPECIAL_BAD_REACTION_ADJUSTMENT */ "badreactionadj",
    /* EFFECT_SPECIAL_GOOD_REACTION_ADJUSTMENT */ "goodreactionadj",
    /*                  EFFECT_SPECIAL_XP_GAIN */ "xpgain",
};

/**
 * 0x5B9CB4
 */
const char* effect_cause_lookup_tbl[EFFECT_CAUSE_COUNT] = {
    /*       EFFECT_CAUSE_RACE */ "Race",
    /* EFFECT_CAUSE_BACKGROUND */ "Background",
    /*      EFFECT_CAUSE_CLASS */ "Class",
    /*      EFFECT_CAUSE_BLESS */ "Bless",
    /*      EFFECT_CAUSE_CURSE */ "Curse",
    /*       EFFECT_CAUSE_ITEM */ "Item",
    /*      EFFECT_CAUSE_SPELL */ "Spell",
    /*     EFFECT_CAUSE_INJURY */ "Injury",
    /*       EFFECT_CAUSE_TECH */ "Tech",
    /*     EFFECT_CAUSE_GENDER */ "Gender",
};

/**
 * Flag indicating whether the effect system is initialized.
 *
 * 0x603ADC
 */
static bool effect_initialized;

/**
 * Array of changes modifying resistances.
 *
 * 0x603AC4
 */
static Effect* effect_resistance_effects;

/**
 * Array of changes modifying critter stats.
 *
 * 0x603AC8
 */
static Effect* effect_stat_effects;

/**
 * Array of changes modifying tech disciplines.
 *
 * 0x603ACC
 */
static Effect* effect_tech_effects;

/**
 * Array of changes modifying tech skill levels.
 *
 * 0x603AD0
 */
static Effect* effect_tech_skill_effects;

/**
 * Array of changes modifying basic skill levels.
 *
 * 0x603AD4
 */
static Effect* effect_basic_skill_effects;

/**
 * Array of changes modifying special attributes.
 *
 * 0x603AD8
 */
static Effect* effect_special_effects;

/**
 * Called when the game is initialized.
 *
 * 0x4E99F0
 */
bool effect_init(GameInitInfo* init_info)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    (void)init_info;

    // Load engine-wide effects message file (required).
    if (!mes_load("rules\\effect.mes", &mes_file)) {
        return false;
    }

    // Allocate memory for typed effect arrays.
    effect_stat_effects = (Effect*)CALLOC(EFFECT_LIST_CAPACITY, sizeof(*effect_stat_effects));
    effect_basic_skill_effects = (Effect*)CALLOC(EFFECT_LIST_CAPACITY, sizeof(*effect_basic_skill_effects));
    effect_tech_skill_effects = (Effect*)CALLOC(EFFECT_LIST_CAPACITY, sizeof(*effect_tech_skill_effects));
    effect_resistance_effects = (Effect*)CALLOC(EFFECT_LIST_CAPACITY, sizeof(*effect_resistance_effects));
    effect_tech_effects = (Effect*)CALLOC(EFFECT_LIST_CAPACITY, sizeof(*effect_tech_effects));
    effect_special_effects = (Effect*)CALLOC(EFFECT_LIST_CAPACITY, sizeof(*effect_special_effects));

    // Parse effects from engine-wide range (50-400).
    for (mes_file_entry.num = 50; mes_file_entry.num < 400; mes_file_entry.num++) {
        if (mes_search(mes_file, &mes_file_entry)) {
            effect_parse(mes_file_entry.num, mes_file_entry.str);
        }
    }

    // Clean up.
    mes_unload(mes_file);

    effect_initialized = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4E9AE0
 */
void effect_exit(void)
{
    FREE(effect_stat_effects);
    FREE(effect_basic_skill_effects);
    FREE(effect_tech_skill_effects);
    FREE(effect_resistance_effects);
    FREE(effect_tech_effects);
    FREE(effect_special_effects);
    effect_initialized = false;
}

/**
 * Called when a module is being loaded.
 *
 * 0x004E9B40
 */
bool effect_mod_load(void)
{
    mes_file_handle_t mes_file;
    MesFileEntry mes_file_entry;

    // Load module-specific effects message file (optional).
    if (mes_load("rules\\gameeffect.mes", &mes_file)) {
        // Parse effects from module-specific range (0-50).
        for (mes_file_entry.num = 0; mes_file_entry.num < 50; mes_file_entry.num++) {
            if (mes_search(mes_file, &mes_file_entry)) {
                effect_parse(mes_file_entry.num, mes_file_entry.str);
            }
        }

        // Clean up.
        mes_unload(mes_file);
    }

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x4E9BB0
 */
void effect_mod_unload(void)
{
    int index;

    // Clear module-specific effects.
    for (index = 0; index < 50; index++) {
        effect_stat_effects[index].count = 0;
        effect_basic_skill_effects[index].count = 0;
        effect_tech_skill_effects[index].count = 0;
        effect_resistance_effects[index].count = 0;
        effect_tech_effects[index].count = 0;
        effect_special_effects[index].count = 0;
    }
}

/**
 * Parses an effect specification from a message file entry into the appropriate
 * effect array.
 *
 * 0x4E9C20
 */
static void effect_parse(int num, char* text)
{
    char* tok;
    Effect* effect_info;
    int type;
    int index;

    tok = strtok(text, " ,");
    while (tok != NULL) {
        // NOTE: Original code is different. Probably uses an inlined routine or
        // a tree of loops.
        effect_info = NULL;

        // The first token designates type of effect by specifying affected
        // attribute. This attribute could be one of stat, basic skill, tech
        // skill, resistance type, tech discipline, or one of the special
        // attributes. Loop through all supported types and check if the
        // modified attribute belongs to the appropriate collection.
        for (type = 0; type < 6 && effect_info == NULL; type++) {
            switch (type) {
            case 0:
                // Check stats.
                for (index = 0; index < STAT_COUNT; index++) {
                    if (SDL_strcasecmp(tok, effect_stat_lookup_tbl[index]) == 0) {
                        effect_info = &(effect_stat_effects[num]);
                        effect_info->ids[effect_info->count] = index;
                        break;
                    }
                }
                break;
            case 1:
                // Check basic skills.
                for (index = 0; index < BASIC_SKILL_COUNT; index++) {
                    if (SDL_strcasecmp(tok, effect_basic_skill_lookup_tbl[index]) == 0) {
                        effect_info = &(effect_basic_skill_effects[num]);
                        effect_info->ids[effect_info->count] = index;
                        break;
                    }
                }
                break;
            case 2:
                // Check tech skills.
                for (index = 0; index < TECH_SKILL_COUNT; index++) {
                    if (SDL_strcasecmp(tok, effect_tech_skill_lookup_tbl[index]) == 0) {
                        effect_info = &(effect_tech_skill_effects[num]);
                        effect_info->ids[effect_info->count] = index;
                        break;
                    }
                }
                break;
            case 3:
                // Check resistance types.
                for (index = 0; index < RESISTANCE_TYPE_COUNT; index++) {
                    if (SDL_strcasecmp(tok, effect_resistance_lookup_tbl[index]) == 0) {
                        effect_info = &(effect_resistance_effects[num]);
                        effect_info->ids[effect_info->count] = index;
                        break;
                    }
                }
                break;
            case 4:
                // Check tech disciplines.
                for (index = 0; index < TECH_COUNT; index++) {
                    if (SDL_strcasecmp(tok, effect_tech_discipline_lookup_tbl[index]) == 0) {
                        effect_info = &(effect_tech_effects[num]);
                        effect_info->ids[effect_info->count] = index;
                        break;
                    }
                }
                break;
            case 5:
                // Check special attributes.
                for (index = 0; index < EFFECT_SPECIAL_COUNT; index++) {
                    if (SDL_strcasecmp(tok, effect_special_attributes_lookup_tbl[index]) == 0) {
                        effect_info = &(effect_special_effects[num]);
                        effect_info->ids[effect_info->count] = index;
                        break;
                    }
                }
                break;
            }
        }

        // Ensure effect type was identified.
        if (effect_info == NULL) {
            tig_debug_printf("Effect: ERROR: Invalid symbol: %s.\n", tok);
            return;
        }

        // Parse operator and associated parameter.
        tok = strtok(NULL, " ,");
        if (SDL_strcasecmp(tok, "min") == 0 || SDL_strcasecmp(tok, "max") == 0) {
            // FIX: Force lowercase.
            if (SDL_tolower(tok[1]) == 'i') {
                effect_info->operators[effect_info->count] = EFFECT_OPERATOR_MIN;
            } else {
                effect_info->operators[effect_info->count] = EFFECT_OPERATOR_MAX;
            }

            tok = strtok(NULL, " ,");
            effect_info->params[effect_info->count] = atoi(tok);
        } else if (tok[0] == '+' || tok[0] == '-') {
            effect_info->operators[effect_info->count] = EFFECT_OPERATOR_ADD;
            effect_info->params[effect_info->count] = atoi(tok);

            if (tok[strlen(tok) - 1] == '%') {
                effect_info->operators[effect_info->count] = EFFECT_OPERATOR_PERCENT;
            }
        } else if (tok[0] == '*') {
            effect_info->operators[effect_info->count] = EFFECT_OPERATOR_MULTIPLY;
            effect_info->params[effect_info->count] = atoi(tok);
        } else if (tok[0] == '/') {
            effect_info->operators[effect_info->count] = EFFECT_OPERATOR_DIVIDE;
            effect_info->params[effect_info->count] = atoi(tok);
        } else {
            tig_debug_printf("Effect: ERROR: Invalid symbol: %s.\n", tok);
        }

        effect_info->count++;
        tok = strtok(NULL, " ,");
    }
}

/**
 * Adds an effect to a critter object.
 *
 * 0x4E9F70
 */
void effect_add(int64_t obj, int effect, int cause)
{
    int strength;
    int encumbrance_level;
    int hp_max;
    int fatigue_max;
    int diff;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    // Retrieve current state.
    strength = stat_level_get(obj, STAT_STRENGTH);
    encumbrance_level = critter_encumbrance_level_get(obj);
    hp_max = object_hp_max(obj);
    fatigue_max = critter_fatigue_max(obj);

    // Add effect and cause to critter's effect arrays.
    obj_arrayfield_uint32_set(obj,
        OBJ_F_CRITTER_EFFECTS_IDX,
        obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECTS_IDX),
        effect);
    obj_arrayfield_uint32_set(obj,
        OBJ_F_CRITTER_EFFECT_CAUSE_IDX,
        obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX),
        cause);

    // Recalculate encumbrance level if strength changed.
    if (strength != stat_level_get(obj, STAT_STRENGTH)) {
        critter_encumbrance_level_recalc(obj, encumbrance_level);
    }

    // Adjust hit points if max HP changed.
    diff = object_hp_max(obj) - hp_max;
    if (diff != 0) {
        object_hp_damage_set(obj, object_hp_damage_get(obj) + diff);
    }

    // Adjust fatigue if max fatigue changed.
    diff = critter_fatigue_max(obj) - fatigue_max;
    if (diff != 0) {
        critter_fatigue_damage_set(obj, critter_fatigue_damage_get(obj) + diff);
    }

    anim_speed_recalc(obj);
}

/**
 * Removes one instance of a specific effect from a critter.
 *
 * 0x4EA100
 */
void effect_remove_one_typed(int64_t obj, int effect)
{
    int cnt;
    int index;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    // Search for the first matching effect and remove it.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECTS_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECTS_IDX, index) == effect) {
            effect_remove_internal(obj, index);
            break;
        }
    }

    anim_speed_recalc(obj);
}

/**
 * Removes all instances of a specific effect from a critter.
 *
 * 0x4EA200
 */
void effect_remove_all_typed(int64_t obj, int effect)
{
    int cnt;
    int index;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    // Remove all matching effects.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECTS_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECTS_IDX, index) == effect) {
            effect_remove_internal(obj, index);
            index--;
            cnt--;
        }
    }
}

/**
 * Removes one effect caused by a specific cause from a critter.
 *
 * 0x4EA2E0
 */
void effect_remove_one_caused_by(int64_t obj, int cause)
{
    int cnt;
    int index;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    // Search for the first matching effect and remove it.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX, index) == cause) {
            effect_remove_internal(obj, index);
            break;
        }
    }
}

/**
 * Removes all effects caused by a specific cause from a critter.
 *
 * 0x4EA3C0
 */
void effect_remove_all_caused_by(int64_t obj, int cause)
{
    int cnt;
    int index;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    // Remove all matching effects.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX, index) == cause) {
            effect_remove_internal(obj, index);
            index--;
            cnt--;
        }
    }
}

/**
 * Counts the number of times the specified effect applied to a critter.
 *
 * 0x4EA4A0
 */
int effect_count_effects_of_type(int64_t obj, int effect)
{
    int effect_count = 0;
    int index;
    int cnt;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECTS_IDX);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECTS_IDX, index) == effect) {
            effect_count++;
        }
    }

    return effect_count;
}

/**
 * Internal helper function to remove an effect at a specific index.
 *
 * 0x4EA520
 */
void effect_remove_internal(int64_t obj, int index)
{
    int strength;
    int encumbrance_level;
    int hp_max;
    int fatigue_max;
    int end;
    unsigned int data;
    int diff;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    // Retrieve current state.
    strength = stat_level_get(obj, STAT_STRENGTH);
    encumbrance_level = critter_encumbrance_level_get(obj);
    hp_max = object_hp_max(obj);
    fatigue_max = critter_fatigue_max(obj);

    // Shift subsequent effects up.
    end = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECTS_IDX) - 1;
    while (index < end) {
        data = obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECTS_IDX, index + 1);
        obj_arrayfield_uint32_set(obj, OBJ_F_CRITTER_EFFECTS_IDX, index, data);

        data = obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX, index + 1);
        obj_arrayfield_uint32_set(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX, index, data);

        index++;
    }

    // Decrease the length of the effect arrays.
    obj_arrayfield_length_set(obj, OBJ_F_CRITTER_EFFECTS_IDX, end);
    obj_arrayfield_length_set(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX, end);

    // Recalculate encumbrance level if strength changed.
    if (strength != stat_level_get(obj, STAT_STRENGTH)) {
        critter_encumbrance_level_recalc(obj, encumbrance_level);
    }

    // Adjust hit points if max HP changed.
    diff = object_hp_max(obj) - hp_max;
    if (diff != 0) {
        object_hp_damage_set(obj, object_hp_damage_get(obj) + diff);
    }

    // Adjust fatigue if max fatigue changed.
    diff = critter_fatigue_max(obj) - fatigue_max;
    if (diff != 0) {
        critter_fatigue_damage_set(obj, critter_fatigue_damage_get(obj) + diff);
    }
}

/**
 * Adjusts a critter's stat level based on applied effects.
 *
 * 0x4EA690
 */
int effect_adjust_stat_level(int64_t obj, int stat, int value)
{
    return effect_adjust_func(obj, stat, value, effect_stat_effects, false);
}

/**
 * The main function to adjust a value based on effects in a specified table.
 *
 * 0x4EA6C0
 */
int effect_adjust_func(int64_t obj, int id, int value, Effect* tbl, bool ignore_innate_effects)
{
    int cnt;
    int index;
    int effect;
    int cause;
    Effect* meta;
    int change;
    int adds = 0;
    int mults = 1;
    int divs = 1;
    int min = -1;
    int max = -1;
    int percents = 0;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Retrieve the number of effects applied to the critter.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECTS_IDX);
    if (cnt <= 0) {
        return value;
    }

    // Process each effect.
    for (index = 0; index < cnt; index++) {
        effect = obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECTS_IDX, index);

        // Filter out innate effects.
        if (ignore_innate_effects) {
            cause = obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECT_CAUSE_IDX, index);
            switch (cause) {
            case EFFECT_CAUSE_RACE:
            case EFFECT_CAUSE_BACKGROUND:
            case EFFECT_CAUSE_CLASS:
            case EFFECT_CAUSE_GENDER:
                continue;
            }
        }

        // Get the effect metadata.
        meta = &(tbl[effect]);

        // Loop through all bonuses and penalties granted by the effect and
        // stack them in specific variables associated with each change type.
        for (change = 0; change < meta->count; change++) {
            if (meta->ids[change] == id) {
                switch (meta->operators[change]) {
                case EFFECT_OPERATOR_ADD:
                    adds += meta->params[change];
                    break;
                case EFFECT_OPERATOR_MULTIPLY:
                    mults *= meta->params[change];
                    break;
                case EFFECT_OPERATOR_DIVIDE:
                    divs *= meta->params[change];
                    break;
                case EFFECT_OPERATOR_MIN:
                    if (min == -1 || min < meta->params[change]) {
                        min = meta->params[change];
                    }
                    break;
                case EFFECT_OPERATOR_MAX:
                    if (max == -1 || max > meta->params[change]) {
                        max = meta->params[change];
                    }
                    break;
                case EFFECT_OPERATOR_PERCENT:
                    percents += meta->params[change];
                    break;
                }
            }
        }
    }

    // 1. Percentage.
    if (percents != 0) {
        value = value * (percents + 100) / 100;
    }

    // 2. Multiply.
    if (mults != 1) {
        value *= mults;
    }

    // 3. Divide.
    if (divs != 1) {
        value /= divs;
    }

    // 4. Sum.
    if (adds != 0) {
        value += adds;
    }

    // 5. Min.
    if (min != -1 && value < min) {
        value = min;
    }

    // 6. Max
    if (max != -1 && value > max) {
        value = max;
    }

    return value;
}

/**
 * Adjusts a critter's stat level based on applied effects.
 *
 * This function ignores effects caused by innate reasons (race, background,
 * class, and gender).
 *
 * 0x4EA930
 */
int effect_adjust_stat_level_no_innate(int64_t obj, int stat, int value)
{
    return effect_adjust_func(obj, stat, value, effect_stat_effects, true);
}

/**
 * Adjusts a critter's basic skill level based on applied effects.
 *
 * 0x4EA960
 */
int effect_adjust_basic_skill_level(int64_t obj, int skill, int value)
{
    return effect_adjust_func(obj, skill, value, effect_basic_skill_effects, false);
}

/**
 * Adjusts a critter's tech skill level based on applied effects.
 *
 * 0x4EA990
 */
int effect_adjust_tech_skill_level(int64_t obj, int skill, int value)
{
    return effect_adjust_func(obj, skill, value, effect_tech_skill_effects, false);
}

/**
 * Adjusts a critter's resistance level based on applied effects.
 *
 * 0x4EA9C0
 */
int effect_adjust_resistance(int64_t obj, int resistance, int value)
{
    return effect_adjust_func(obj, resistance, value, effect_resistance_effects, false);
}

/**
 * Adjusts a critter's tech effective level based on applied effects.
 *
 * 0x4EA9F0
 */
int effect_adjust_tech_level(int64_t obj, int tech, int value)
{
    return effect_adjust_func(obj, tech, value, effect_tech_effects, false);
}

/**
 * Adjusts a critter's maximum hit points based on applied effects.
 *
 * 0x4EAA20
 */
int effect_adjust_max_hit_points(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_MAX_HIT_POINTS, value, effect_special_effects, false);
}

/**
 * Adjusts a critter's maximum fatigue based on applied effects.
 *
 * 0x4EAA50
 */
int effect_adjust_max_fatigue(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_MAX_FATIGUE, value, effect_special_effects, false);
}

/**
 * Adjusts a critter's reaction based on applied effects.
 *
 * 0x4EAA80
 */
int effect_adjust_reaction(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_REACTION, value, effect_special_effects, false);
}

/**
 * Adjusts a critter's good/bad reaction adjustment based on applied effects.
 *
 * 0x4EAAB0
 */
int effect_adjust_good_bad_reaction(int64_t obj, int value)
{
    if (value < 0) {
        return effect_adjust_func(obj, EFFECT_SPECIAL_BAD_REACTION_ADJUSTMENT, value, effect_special_effects, false);
    } else if (value > 0) {
        return effect_adjust_func(obj, EFFECT_SPECIAL_GOOD_REACTION_ADJUSTMENT, value, effect_special_effects, false);
    } else {
        return 0;
    }
}

/**
 * Adjusts a critter's critical hit chance based on applied effects.
 *
 * 0x4EAB00
 */
int effect_adjust_crit_hit_chance(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_CRIT_HIT_CHANCE, value, effect_special_effects, false);
}

/**
 * Adjusts a critter's critical hit effect based on applied effects.
 *
 * 0x4EAB30
 */
int effect_adjust_crit_hit_effect(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_CRIT_HIT_EFFECT, value, effect_special_effects, false);
}

/**
 * Adjusts a critter's critical fail chance based on applied effects.
 *
 * 0x4EAB60
 */
int effect_adjust_crit_fail_chance(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_CRIT_FAIL_CHANCE, value, effect_special_effects, false);
}

/**
 * Adjusts a critter's critical fail effect based on applied effects.
 *
 * 0x4EAB90
 */
int effect_adjust_crit_fail_effect(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_CRIT_FAIL_EFFECT, value, effect_special_effects, false);
}

/**
 * Adjusts a critter's XP gain based on applied effects.
 *
 * 0x4EABC0
 */
int effect_adjust_xp_gain(int64_t obj, int value)
{
    return effect_adjust_func(obj, EFFECT_SPECIAL_XP_GAIN, value, effect_special_effects, false);
}

/**
 * Prints the effects applied to a critter objects.
 *
 * 0x4EABF0
 */
void effect_debug_obj(int64_t obj)
{
    int cnt;
    int index;

    // Ensure object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    // Retrieve the number of effects.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_EFFECTS_IDX);

    tig_debug_println("\tEffect Debug Obj:\n");
    tig_debug_printf("\tEffect Count: %d\n", cnt);

    // Log effect IDs.
    for (index = 0; index < cnt; index++) {
        tig_debug_printf("\t\tEffect %d: ID: %d\n",
            index,
            obj_arrayfield_uint32_get(obj, OBJ_F_CRITTER_EFFECTS_IDX, index));
    }
}
