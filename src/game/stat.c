#include "game/stat.h"

#include "game/a_name.h"
#include "game/anim.h"
#include "game/background.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/effect.h"
#include "game/item.h"
#include "game/level.h"
#include "game/light.h"
#include "game/location.h"
#include "game/magictech.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/player.h"
#include "game/sector.h"
#include "game/skill.h"
#include "game/spell.h"
#include "game/tech.h"
#include "game/tile.h"
#include "game/timeevent.h"
#include "game/ui.h"

#define STAT_IS_VALID(stat) ((stat) >= 0 && (stat) < STAT_COUNT)
#define STAT_IS_PRIMARY(stat) ((stat) >= 0 && (stat) <= STAT_CHARISMA)
#define STAT_IS_DERIVED(stat) ((stat) >= STAT_CARRY_WEIGHT && (stat) <= STAT_MAGICK_TECH_APTITUDE)

typedef enum PoisonEventType {
    POISON_EVENT_DAMAGE,
    POISON_EVENT_RECOVERY,
} PoisonEventType;

static bool poison_timeevent_check(TimeEvent* timeevent);
static bool poison_timeevent_schedule(int64_t obj, int poison, bool recovery);

/**
 * Minimum values for each stat.
 *
 * 0x5B5194
 */
static int stat_min_values[STAT_COUNT] = {
    /*             STAT_STRENGTH */ 1,
    /*            STAT_DEXTERITY */ 1,
    /*         STAT_CONSTITUTION */ 1,
    /*               STAT_BEAUTY */ 1,
    /*         STAT_INTELLIGENCE */ 1,
    /*           STAT_PERCEPTION */ 1,
    /*            STAT_WILLPOWER */ 1,
    /*             STAT_CHARISMA */ 1,
    /*         STAT_CARRY_WEIGHT */ 300,
    /*         STAT_DAMAGE_BONUS */ -50,
    /*        STAT_AC_ADJUSTMENT */ -9,
    /*                STAT_SPEED */ 1,
    /*            STAT_HEAL_RATE */ 0,
    /*      STAT_POISON_RECOVERY */ 1,
    /*    STAT_REACTION_MODIFIER */ -65,
    /*        STAT_MAX_FOLLOWERS */ 1,
    /* STAT_MAGICK_TECH_APTITUDE */ -100,
    /*                STAT_LEVEL */ 0,
    /*    STAT_EXPERIENCE_POINTS */ 0,
    /*            STAT_ALIGNMENT */ -1000,
    /*          STAT_FATE_POINTS */ 0,
    /*       STAT_UNSPENT_POINTS */ 0,
    /*        STAT_MAGICK_POINTS */ 0,
    /*          STAT_TECH_POINTS */ 0,
    /*         STAT_POISON_LEVEL */ 0,
    /*                  STAT_AGE */ 20,
    /*               STAT_GENDER */ 0,
    /*                 STAT_RACE */ 0,
};

/**
 * Maximum values for each stat.
 *
 * 0x5B5204
 */
static int stat_max_values[STAT_COUNT] = {
    /*             STAT_STRENGTH */ 20,
    /*            STAT_DEXTERITY */ 20,
    /*         STAT_CONSTITUTION */ 20,
    /*               STAT_BEAUTY */ 20,
    /*         STAT_INTELLIGENCE */ 20,
    /*           STAT_PERCEPTION */ 20,
    /*            STAT_WILLPOWER */ 20,
    /*             STAT_CHARISMA */ 20,
    /*         STAT_CARRY_WEIGHT */ 10000,
    /*         STAT_DAMAGE_BONUS */ 50,
    /*        STAT_AC_ADJUSTMENT */ 95,
    /*                STAT_SPEED */ 100,
    /*            STAT_HEAL_RATE */ 6,
    /*      STAT_POISON_RECOVERY */ 20,
    /*    STAT_REACTION_MODIFIER */ 200,
    /*        STAT_MAX_FOLLOWERS */ 7,
    /* STAT_MAGICK_TECH_APTITUDE */ 100,
    /*                STAT_LEVEL */ 51,
    /*    STAT_EXPERIENCE_POINTS */ 2000000000,
    /*            STAT_ALIGNMENT */ 1000,
    /*          STAT_FATE_POINTS */ 100,
    /*       STAT_UNSPENT_POINTS */ 56,
    /*        STAT_MAGICK_POINTS */ 210,
    /*          STAT_TECH_POINTS */ 210,
    /*         STAT_POISON_LEVEL */ 1000,
    /*                  STAT_AGE */ 1000,
    /*               STAT_GENDER */ 1,
    /*                 STAT_RACE */ 11,
};

/**
 * Default values for each stat.
 *
 * 0x5B5274
 */
static int stat_default_values[STAT_COUNT] = {
    /*             STAT_STRENGTH */ 8,
    /*            STAT_DEXTERITY */ 8,
    /*         STAT_CONSTITUTION */ 8,
    /*               STAT_BEAUTY */ 8,
    /*         STAT_INTELLIGENCE */ 8,
    /*           STAT_PERCEPTION */ 8,
    /*            STAT_WILLPOWER */ 8,
    /*             STAT_CHARISMA */ 8,
    /*         STAT_CARRY_WEIGHT */ 0,
    /*         STAT_DAMAGE_BONUS */ 0,
    /*        STAT_AC_ADJUSTMENT */ 0,
    /*                STAT_SPEED */ 0,
    /*            STAT_HEAL_RATE */ 0,
    /*      STAT_POISON_RECOVERY */ 0,
    /*    STAT_REACTION_MODIFIER */ 0,
    /*        STAT_MAX_FOLLOWERS */ 0,
    /* STAT_MAGICK_TECH_APTITUDE */ 0,
    /*                STAT_LEVEL */ 1,
    /*    STAT_EXPERIENCE_POINTS */ 0,
    /*            STAT_ALIGNMENT */ 0,
    /*          STAT_FATE_POINTS */ 0,
    /*       STAT_UNSPENT_POINTS */ 5,
    /*        STAT_MAGICK_POINTS */ 0,
    /*          STAT_TECH_POINTS */ 0,
    /*         STAT_POISON_LEVEL */ 0,
    /*                  STAT_AGE */ 20,
    /*               STAT_GENDER */ 1,
    /*                 STAT_RACE */ 0,
};

/**
 * Defines cost (in character points) of picking appropriate stat level.
 *
 * 0x5B52E4
 */
static int stat_cost_tbl[20] = {
    /*  0 */ 0,
    /*  1 */ 1,
    /*  2 */ 1,
    /*  3 */ 1,
    /*  4 */ 1,
    /*  5 */ 1,
    /*  6 */ 1,
    /*  7 */ 1,
    /*  8 */ 1,
    /*  9 */ 1,
    /* 10 */ 1,
    /* 11 */ 1,
    /* 12 */ 1,
    /* 13 */ 1,
    /* 14 */ 1,
    /* 15 */ 1,
    /* 16 */ 1,
    /* 17 */ 1,
    /* 18 */ 1,
    /* 19 */ 1,
};

/**
 * Map beauty stat value to reaction modifier.
 *
 * 0x5B5334
 */
static int stat_reaction_tbl[20] = {
    /*  0 */ -65,
    /*  1 */ -52,
    /*  2 */ -42,
    /*  3 */ -33,
    /*  4 */ -25,
    /*  5 */ -18,
    /*  6 */ -12,
    /*  7 */ -7,
    /*  8 */ -3,
    /*  9 */ 0,
    /* 10 */ 3,
    /* 11 */ 7,
    /* 12 */ 12,
    /* 13 */ 18,
    /* 14 */ 25,
    /* 15 */ 33,
    /* 16 */ 42,
    /* 17 */ 52,
    /* 18 */ 65,
    /* 19 */ 75,
};

/**
 * Lookup keys for stat names.
 *
 * 0x5B5384
 */
const char* stat_lookup_keys_tbl[STAT_COUNT] = {
    /*             STAT_STRENGTH */ "stat_strength",
    /*            STAT_DEXTERITY */ "stat_dexterity",
    /*         STAT_CONSTITUTION */ "stat_constitution",
    /*               STAT_BEAUTY */ "stat_beauty",
    /*         STAT_INTELLIGENCE */ "stat_intelligence",
    /*           STAT_PERCEPTION */ "stat_perception",
    /*            STAT_WILLPOWER */ "stat_willpower",
    /*             STAT_CHARISMA */ "stat_charisma",
    /*         STAT_CARRY_WEIGHT */ "stat_carry_weight",
    /*         STAT_DAMAGE_BONUS */ "stat_damage_bonus",
    /*        STAT_AC_ADJUSTMENT */ "stat_ac_adjustment",
    /*                STAT_SPEED */ "stat_speed",
    /*            STAT_HEAL_RATE */ "stat_heal_rate",
    /*      STAT_POISON_RECOVERY */ "stat_poison_recovery_rate",
    /*    STAT_REACTION_MODIFIER */ "stat_reaction_modifier",
    /*        STAT_MAX_FOLLOWERS */ "stat_max_followers",
    /* STAT_MAGICK_TECH_APTITUDE */ "stat_magic_tech_aptitude",
    /*                STAT_LEVEL */ "stat_level",
    /*    STAT_EXPERIENCE_POINTS */ "stat_experience_points",
    /*            STAT_ALIGNMENT */ "stat_alignment",
    /*          STAT_FATE_POINTS */ "stat_fate_points",
    /*       STAT_UNSPENT_POINTS */ "stat_poison_level", // FIXME: Does not match stat.
    /*        STAT_MAGICK_POINTS */ "stat_age", // FIXME: Does not match stat.
    /*          STAT_TECH_POINTS */ NULL,
    /*         STAT_POISON_LEVEL */ NULL,
    /*                  STAT_AGE */ NULL,
    /*               STAT_GENDER */ NULL,
    /*                 STAT_RACE */ NULL,
};

// 0x5B53F4
static int poison_test_event = -1;

// 0x5F8644
static mes_file_handle_t stat_msg_file;

// 0x5F8610
static char* gender_names[GENDER_COUNT];

// 0x5F8618
static char* race_names[RACE_COUNT];

// 0x5F8648
static char* stat_names[STAT_COUNT];

// 0x5F86B8
static char* stat_short_names[STAT_COUNT];

// 0x5F8728
static int64_t poison_test_obj;

/**
 * Called when the game is initialized.
 *
 * 0x4B0340
 */
bool stat_init(GameInitInfo* init_info)
{
    MesFileEntry mes_file_entry;
    int index;

    (void)init_info;

    // Load the stat message file.
    if (!mes_load("mes\\stat.mes", &stat_msg_file)) {
        return false;
    }

    // Load full stat names.
    for (index = 0; index < STAT_COUNT; index++) {
        mes_file_entry.num = index;
        mes_get_msg(stat_msg_file, &mes_file_entry);
        stat_names[index] = mes_file_entry.str;
    }

    // Load short stat names.
    for (index = 0; index < STAT_COUNT; index++) {
        mes_file_entry.num = index + 500;
        mes_get_msg(stat_msg_file, &mes_file_entry);
        stat_short_names[index] = mes_file_entry.str;
    }

    // Load gender names.
    for (index = 0; index < GENDER_COUNT; index++) {
        mes_file_entry.num = index + 28;
        mes_get_msg(stat_msg_file, &mes_file_entry);
        gender_names[index] = mes_file_entry.str;
    }

    // Load race names.
    for (index = 0; index < RACE_COUNT; index++) {
        mes_file_entry.num = index + 30;
        mes_get_msg(stat_msg_file, &mes_file_entry);
        race_names[index] = mes_file_entry.str;
    }

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4B0440
 */
void stat_exit()
{
    mes_unload(stat_msg_file);
}

/**
 * Set default stat values for a critter.
 *
 * 0x4B0450
 */
void stat_set_defaults(int64_t obj)
{
    int stat;

    for (stat = 0; stat < STAT_COUNT; stat++) {
        obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_STAT_BASE_IDX, stat, stat_default_values[stat]);
    }
}

/**
 * Retrieves the effective stat level for a critter.
 *
 * 0x4B0490
 */
int stat_level_get(int64_t obj, int stat)
{
    int value;
    int64_t loc;
    tig_art_id_t art_id;
    int min_value;
    int max_value;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure stat is valid.
    if (!STAT_IS_VALID(stat)) {
        return 0;
    }

    // Obtain base value.
    value = stat_base_get(obj, stat);

    switch (stat) {
    case STAT_SPEED:
        // Check if Tempus Fugit is active.
        if (magictech_check_env_sf(OSF_TEMPUS_FUGIT)) {
            // Critters affected by Tempus Fugit (usually party members) gets
            // +10 speed, while everyone else gets -10.
            if ((obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & OSF_TEMPUS_FUGIT) != 0) {
                value += 10;
            } else {
                value -= 10;
            }
        }

        // Penalty for crippled legs.
        if ((obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & OCF_CRIPPLED_LEGS_BOTH) != 0) {
            value -= 5;
        }

        break;
    case STAT_MAX_FOLLOWERS:
        // Bonus follower for experts/masters of persuasion.
        if (basic_skill_training_get(obj, BASIC_SKILL_PERSUATION) >= TRAINING_EXPERT) {
            value += 1;
        }
        break;
    case STAT_INTELLIGENCE:
    case STAT_DEXTERITY:
    case STAT_STRENGTH:
    case STAT_WILLPOWER:
        switch (background_get(obj)) {
        case BACKGROUND_AGORAPHOBIC:
            // Indoor:
            // - Intelligence +2
            //
            // Outdoor:
            // - Dexterity -2
            // - Intelligence -2
            // - Willpower -2
            // - Strength +2
            loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
            art_id = tile_art_id_at(loc);
            if (tig_art_tile_id_type_get(art_id) == TIG_ART_TILE_TYPE_INDOOR) {
                if (stat == STAT_INTELLIGENCE) {
                    value += 2;
                }
            } else {
                if (stat == STAT_STRENGTH) {
                    value += 2;
                } else {
                    value -= 2;
                }
            }
            break;
        case BACKGROUND_HYDROPHOBIC:
            // Land:
            // - Persuation +2
            //
            // Water:
            // - Dexterity -2
            // - Intelligence -2
            // - Willpower -2
            // - Strength +2
            //
            // NOTE: Persuation bonus is applied via effects.
            loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
            art_id = tile_art_id_at(loc);
            if (a_name_tile_is_sinkable(art_id)) {
                if (stat == STAT_STRENGTH) {
                    value += 2;
                } else {
                    value -= 2;
                }
            }
            break;
        case BACKGROUND_AFRAID_OF_THE_DARK:
            // Light:
            // - Perception +2
            //
            // Dark:
            // - Dexterity -2
            // - Intelligence -2
            // - Willpower -2
            // - Strength +2
            //
            // NOTE: Perception bonus is applied via effects.
            if (sub_4DCE10(obj) < 128) {
                if (stat == STAT_STRENGTH) {
                    value += 2;
                } else {
                    value -= 2;
                }
            }
            break;
        }

        // Apply poison penalties to strength and dexterity.
        if (stat == STAT_STRENGTH || stat == STAT_DEXTERITY) {
            int poison_penalty = stat_level_get(obj, STAT_POISON_LEVEL) / 100;
            if (poison_penalty > 3) {
                poison_penalty = 3;
            }
            if (poison_penalty > 0) {
                value -= poison_penalty;
            }
        }

        break;
    case STAT_MAGICK_POINTS:
        switch (background_get(obj)) {
        case BACKGROUND_DAY_MAGE:
            // 20% bonus to magickal aptitude during the day, and 20% penalty at
            // night. Each stat is normally within 1-20 range (but upper bound
            // can be modified by race, which is not taken into account), so 20%
            // is 4 points.
            if (game_time_is_day()) {
                value += 4;
            } else {
                value -= 4;
            }
            break;
        case BACKGROUND_NIGHT_MAGE:
            // 20% bonus to magickal aptitude at night, and 20% penalty during
            // the day.
            if (game_time_is_day()) {
                value -= 4;
            } else {
                value += 4;
            }
            break;
        case BACKGROUND_SKY_MAGE:
            // 20% bonus to magickal aptitude in outdoor areas, and 20% penalty
            // when indoors or underground.
            loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
            art_id = tile_art_id_at(loc);
            if (tig_art_tile_id_type_get(art_id) == TIG_ART_TILE_TYPE_INDOOR) {
                value -= 4;
            } else {
                value += 4;
            }
            break;
        case BACKGROUND_NATURE_MAGE:
            // 20% bonus to magickal aptitude when standing on natural surface,
            // and 20% penalty otherwise. The "naturalness" of the tile is set
            // with flags in `tilename.mes`.
            loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
            art_id = tile_art_id_at(loc);
            if (!a_name_tile_is_natural(art_id)) {
                value -= 4;
            } else {
                value += 4;
            }
            break;
        }
        break;
    }

    // Apply effects.
    value = effect_adjust_stat_level(obj, stat, value);

    // Clamp the final value to min/max bounds.
    min_value = stat_level_min(obj, stat);
    max_value = stat_level_max(obj, stat);

    if (value < min_value) {
        value = min_value;
    } else if (value > max_value) {
        value = max_value;
    }

    return value;
}

/**
 * Retrieves the base stat value for a critter.
 *
 * The base value is either exact value as was stored with `stat_base_set`, or
 * derived using formulas from primary stats' effective values.
 *
 * NOTE: In case of changing, care should be taken to avoid circular dependency
 * (which will lead to stack overflow).
 *
 * 0x4B0740
 */
int stat_base_get(int64_t obj, int stat)
{
    int value;

    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Ensure stat is valid.
    if (!STAT_IS_VALID(stat)) {
        return 0;
    }

    if (STAT_IS_DERIVED(stat)) {
        // Calculate derived stats.
        switch (stat) {
        case STAT_CARRY_WEIGHT:
            value = 500 * stat_level_get(obj, STAT_STRENGTH);
            break;
        case STAT_DAMAGE_BONUS:
            value = stat_level_get(obj, STAT_STRENGTH) - 10;
            if (value < 0) {
                value /= 2;
            }

            // Extraordinary strength doubles damage bonus.
            if (stat_is_extraordinary(obj, STAT_STRENGTH)) {
                value *= 2;
            }

            break;
        case STAT_AC_ADJUSTMENT:
            value = stat_level_get(obj, STAT_DEXTERITY) - 10;
            break;
        case STAT_SPEED:
            value = stat_level_get(obj, STAT_DEXTERITY);

            // Extraordinary dexterity grants +5 speed.
            if (stat_is_extraordinary(obj, STAT_DEXTERITY)) {
                value += 5;
            }

            break;
        case STAT_HEAL_RATE:
            value = (stat_level_get(obj, STAT_CONSTITUTION) + 1) / 3;
            break;
        case STAT_POISON_RECOVERY:
            value = stat_level_get(obj, STAT_CONSTITUTION);
            break;
        case STAT_REACTION_MODIFIER:
            value = stat_level_get(obj, STAT_BEAUTY);
            if (stat_is_extraordinary(obj, STAT_BEAUTY)) {
                // Extraordinary beauty changes reaction modifier: beauty 20
                // gives 100% + 10% for each point over 20.
                value = 2 * (5 * value - 50);
            } else {
                // Normal beauty - use lookup table ()
                value = stat_reaction_tbl[value - 1];
            }
            break;
        case STAT_MAX_FOLLOWERS:
            value = stat_level_get(obj, STAT_CHARISMA) / 4;
            break;
        case STAT_MAGICK_TECH_APTITUDE:
            value = (50 * stat_level_get(obj, STAT_MAGICK_POINTS) - 55 * stat_level_get(obj, STAT_TECH_POINTS)) / 10;
            value += magictech_get_aptitude_adj(sector_id_from_loc(obj_field_int64_get(obj, OBJ_F_LOCATION)));
            break;
        default:
            // Should be unreachable.
            assert(0);
        }
    } else {
        value = obj_arrayfield_int32_get(obj, OBJ_F_CRITTER_STAT_BASE_IDX, stat);
    }

    return value;
}

/**
 * Sets the base stat value for a critter.
 *
 * 0x4B0980
 */
int stat_base_set(int64_t obj, int stat, int value)
{
    int obj_type;
    int min_value;
    int max_value;
    int before;
    int after;
    int encumbrance_level;

    // Make sure obj is a critter.
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(obj_type)) {
        return 0;
    }

    // Make sure stat is valid.
    if (!STAT_IS_VALID(stat)) {
        return false;
    }

    if (tig_net_is_active()
        && !multiplayer_is_locked()) {
        SetBaseStatPacket pkt;

        pkt.type = 50;
        pkt.stat = stat;
        pkt.value = value;
        pkt.oid = obj_get_id(obj);

        if (!tig_net_is_host()) {
            if (player_is_local_pc_obj(obj)
                && STAT_IS_PRIMARY(stat)
                && abs(stat_base_get(obj, stat) - value) == 1) {
                tig_net_send_app_all(&pkt, sizeof(pkt));
            }
            return true;
        }

        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    // We cannot modify derived stats for obvious reasons. If we're trying to do
    // it silently ignore this request and simply return it's current value.
    if (STAT_IS_DERIVED(stat)) {
        return stat_base_get(obj, stat);
    }

    // Grab valid range for given stat and clamp value to that range.
    min_value = stat_level_min(obj, stat);
    max_value = stat_level_max(obj, stat);
    if (value < min_value) {
        value = min_value;
    } else if (value > max_value) {
        value = max_value;
    }

    // Obtain current base stat value.
    before = stat_base_get(obj, stat);

    // Temporarily change the base value, obtain new effective level, and revert
    // it back.
    obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_STAT_BASE_IDX, stat, value);
    after = stat_level_get(obj, stat);
    obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_STAT_BASE_IDX, stat, before);

    // Check if reducing the stat would break skill, spell, or tech
    // requirements.
    if (value < before) {
        if (!skill_check_stat(obj, stat, after)) {
            return before;
        }

        switch (stat) {
        case STAT_INTELLIGENCE:
            if (!spell_check_intelligence(obj, after)) {
                return before;
            }
            if (!tech_check_intelligence(obj, after)) {
                return before;
            }
            break;
        case STAT_WILLPOWER:
            if (!spell_check_willpower(obj, after)) {
                return before;
            }
            break;
        }
    }

    // Handle stat-specific side effects.
    switch (stat) {
    case STAT_STRENGTH:
        // Grab encumbrance level before change.
        encumbrance_level = critter_encumbrance_level_get(obj);
        break;
    case STAT_POISON_LEVEL:
        // Extraordinary constitution grants poison immunity.
        if (stat_is_extraordinary(obj, STAT_CONSTITUTION)) {
            value = 0;
        }
        break;
    case STAT_GENDER:
        // Remove gender-specific effect.
        effect_remove_one_caused_by(obj, EFFECT_CAUSE_GENDER);
        break;
    case STAT_RACE:
        // Remove race-specific effect.
        effect_remove_one_caused_by(obj, EFFECT_CAUSE_RACE);
        break;
    }

    // Set the new base stat value.
    obj_arrayfield_int32_set(obj, OBJ_F_CRITTER_STAT_BASE_IDX, stat, value);

    switch (stat) {
    case STAT_STRENGTH:
        // Recalculate encumbrance level.
        critter_encumbrance_level_recalc(obj, encumbrance_level);
        break;
    case STAT_INTELLIGENCE:
        // Recalculate maintained spell slots.
        if (player_is_local_pc_obj(obj)) {
            ui_spell_maintain_refresh();
        }
        break;
    case STAT_SPEED:
        // Recalculate animation speed.
        anim_speed_recalc(obj);
        break;
    case STAT_EXPERIENCE_POINTS:
        // Recalculate critter level.
        level_recalc(obj);

        // Refresh expierence bars.
        ui_charedit_refresh();
        break;
    case STAT_ALIGNMENT:
        // PC alignment have been changed, make followers recheck their attitude
        // towards PC (and probably leave the party).
        if (before != value && obj_type == OBJ_TYPE_PC) {
            ObjectList followers;
            ObjectNode* node;
            unsigned int flags;

            object_list_followers(obj, &followers);
            node = followers.head;
            while (node != NULL) {
                flags = obj_field_int32_get(node->obj, OBJ_F_NPC_FLAGS);
                flags |= ONF_CHECK_LEADER;
                obj_field_int32_set(node->obj, OBJ_F_NPC_FLAGS, flags);

                flags = obj_field_int32_get(node->obj, OBJ_F_CRITTER_FLAGS2);
                if (value >= before) {
                    flags |= OCF2_CHECK_ALIGN_GOOD;
                } else {
                    flags |= OCF2_CHECK_ALIGN_BAD;
                }
                obj_field_int32_set(node->obj, OBJ_F_CRITTER_FLAGS2, flags);

                node = node->next;
            }
            object_list_destroy(&followers);
        }

        // Update UI.
        sub_4601C0();
        break;
    case STAT_MAGICK_POINTS:
    case STAT_TECH_POINTS:
        // Recalc item effects affected by magic/tech aptitude.
        item_rewield(obj);
        break;
    case STAT_POISON_LEVEL:
        if (value > 0) {
            // Schedule poison events - both damage and recovery - if required.
            poison_timeevent_schedule(obj, value, true);
        }

        // Update health bar.
        ui_refresh_health_bar(obj);
        break;
    case STAT_GENDER:
        if (value == GENDER_FEMALE) {
            // Add female effects:
            // - Strength -1
            // - Constitution +1
            effect_add(obj, EFFECT_FEMALE, EFFECT_CAUSE_GENDER);
        }
        break;
    case STAT_RACE:
        // Add racial effects.
        effect_add(obj, EFFECT_RACE_SPECIFIC + value, EFFECT_CAUSE_RACE);
        break;
    }

    return value;
}

/**
 * Checks if a primary stat value is extraordinary (20 or higher).
 *
 * 0x4B0EE0
 */
bool stat_is_extraordinary(int64_t obj, int stat)
{
    // Ensure the object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return false;
    }

    // Ensure stat is valid.
    if (!STAT_IS_VALID(stat)) {
        return false;
    }

    // Only primary stats can be considered extraordinary.
    if (!STAT_IS_PRIMARY(stat)) {
        return false;
    }

    if (stat_level_get(obj, stat) < 20) {
        return false;
    }

    return true;
}

/**
 * Calculates the cost of increasing a stat to a given value.
 *
 * 0x4B0F50
 */
int stat_cost(int value)
{
    if (value < 1) {
        value = 1;
    } else if (value > 20) {
        value = 20;
    }

    return stat_cost_tbl[value - 1];
}

/**
 * Retrieves the full name of a stat.
 *
 * 0x4B0F80
 */
const char* stat_name(int stat)
{
    return stat_names[stat];
}

/**
 * Retrieves the short name of a stat.
 *
 * 0x4B0F90
 */
const char* stat_short_name(int stat)
{
    return stat_short_names[stat];
}

/**
 * Retrieves the name of a gender.
 *
 * 0x4B0FA0
 */
const char* gender_name(int gender)
{
    return gender_names[gender];
}

/**
 * Retrieves the name of a race.
 *
 * 0x4B0FB0
 */
const char* race_name(int race)
{
    return race_names[race];
}

/**
 * Retrieves the minimum value for a stat.
 *
 * 0x4B0FC0
 */
int stat_level_min(int64_t obj, int stat)
{
    (void)obj;

    return stat_min_values[stat];
}

/**
 * Retrieves the maximum value for a stat.
 *
 * The maximum value of some stats is adjusted for critter's race.
 *
 * 0x4B0FD0
 */
int stat_level_max(int64_t obj, int stat)
{
    int race;

    if (obj != OBJ_HANDLE_NULL) {
        race = stat_base_get(obj, STAT_RACE);
    } else {
        race = RACE_HUMAN;
    }

    switch (race) {
    case RACE_DWARF:
    case RACE_HALF_ORC:
        if (stat == STAT_STRENGTH || stat == STAT_CONSTITUTION) {
            return 21;
        }
        break;
    case RACE_ELF:
    case RACE_DARK_ELF:
        if (stat == STAT_DEXTERITY || stat == STAT_BEAUTY || stat == STAT_WILLPOWER) {
            return 21;
        }
        break;
    case RACE_HALF_ELF:
        if (stat == STAT_DEXTERITY) {
            return 21;
        }
        break;
    case RACE_GNOME:
        if (stat == STAT_WILLPOWER) {
            return 22;
        }
        break;
    case RACE_HALFLING:
        if (stat == STAT_DEXTERITY) {
            return 22;
        }
        break;
    case RACE_HALF_OGRE:
        if (stat == STAT_STRENGTH) {
            return 24;
        }
        break;
    case RACE_OGRE:
        if (stat == STAT_STRENGTH) {
            return 26;
        }
        break;
    case RACE_ORC:
        if (stat == STAT_STRENGTH || stat == STAT_CONSTITUTION) {
            return 22;
        }
        break;
    }

    return stat_max_values[stat];
}

/**
 * Attempts to set a stat to a target effective level by incrementing it's base
 * value.
 *
 * 0x4B10A0
 */
bool stat_level_set(int64_t obj, int stat, int value)
{
    int max_value;
    int base_value;
    int iter = 1;

    max_value = stat_level_max(obj, stat);

    base_value = stat_base_get(obj, stat);
    stat_base_set(obj, stat, base_value + 1);

    while (value > stat_level_get(obj, stat)) {
        if (stat_base_get(obj, stat) == base_value) {
            break;
        }

        if (stat_level_get(obj, stat) >= max_value) {
            break;
        }

        if (iter >= 100) {
            break;
        }

        iter++;

        base_value = stat_base_get(obj, stat);
        stat_base_set(obj, stat, base_value + 1);
    }

    return value <= stat_level_get(obj, stat);
}

/**
 * Called when a poison timeevent occurs.
 *
 * 0x4B1170
 */
bool stat_poison_timeevent_process(TimeEvent* timeevent)
{
    int type;
    int64_t obj;
    int poison;
    int damage;
    DateTime datetime;
    TimeEvent next_timeevent;
    CombatContext combat;

    type = timeevent->params[0].integer_value;
    obj = timeevent->params[1].object_value;

    poison = stat_base_get(obj, STAT_POISON_LEVEL);
    switch (type) {
    case POISON_EVENT_DAMAGE:
        if (poison > 0) {
            if (!tig_net_is_active()
                || (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_OFF) == 0) {
                sub_4B2210(OBJ_HANDLE_NULL, obj, &combat);

                // Determine damage based on poison severity.
                if (poison >= 550) {
                    damage = 3;
                } else if (poison >= 200) {
                    damage = 2;
                } else {
                    damage = 1;
                }

                combat.dam[DAMAGE_TYPE_NORMAL] = damage;
                combat.flags |= 0x80;
                combat_dmg(&combat);
            }

            // Add blood splotch effect if applicable.
            if ((combat.dam_flags & CDF_HAVE_DAMAGE) != 0) {
                anim_play_blood_splotch_fx(combat.target_obj, BLOOD_SPLOTCH_TYPE_POISON, DAMAGE_TYPE_POISON, &combat);
            }

            // Schedule next poison event.
            poison_timeevent_schedule(obj, poison, false);

            // Update health bar.
            if (!tig_net_is_active()
                || (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_OFF) == 0) {
                ui_refresh_health_bar(obj);
            }
        }
        break;
    case POISON_EVENT_RECOVERY:
        if (poison > 0) {
            if (!tig_net_is_active()
                || (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_OFF) == 0) {
                // Reduce poison level based on recovery stat.
                poison -= stat_level_get(obj, STAT_POISON_RECOVERY);
                if (poison < 0) {
                    poison = 0;
                }
                stat_base_set(obj, STAT_POISON_LEVEL, poison);
            }

            // Schedule next recovery event if poison remains.
            if (poison > 0) {
                next_timeevent.type = TIMEEVENT_TYPE_POISON;
                next_timeevent.params[0].integer_value = POISON_EVENT_RECOVERY;
                next_timeevent.params[1].object_value = obj;
                next_timeevent.params[2].integer_value = sub_45A7F0();
                sub_45A950(&datetime, 120000);
                if (!timeevent_add_delay(&next_timeevent, &datetime)) {
                    return false;
                }
            }
        }
        break;
    }

    return true;
}

// 0x4B1310
bool poison_timeevent_check(TimeEvent* timeevent)
{
    return timeevent->params[1].object_value == poison_test_obj
        && timeevent->params[0].integer_value == poison_test_event;
}

// 0x4B1350
bool poison_timeevent_schedule(int64_t obj, int poison, bool recovery)
{
    DateTime datetime;
    TimeEvent timeevent;

    (void)poison;

    // Setup damage event.
    timeevent.type = TIMEEVENT_TYPE_POISON;
    timeevent.params[0].integer_value = POISON_EVENT_DAMAGE;
    timeevent.params[1].object_value = obj;
    timeevent.params[2].integer_value = sub_45A7F0();

    // Check if poison damage event is not already scheduled.
    poison_test_obj = obj;
    poison_test_event = POISON_EVENT_DAMAGE;
    if (!timeevent_any(TIMEEVENT_TYPE_POISON, poison_timeevent_check)) {
        // Schedule damage event in 15 seconds.
        sub_45A950(&datetime, 15000);
        if (!timeevent_add_delay(&timeevent, &datetime)) {
            return false;
        }
    }

    if (recovery) {
        // Check is poison recovery event event is not already scheduled.
        poison_test_obj = obj;
        poison_test_event = POISON_EVENT_RECOVERY;
        if (!timeevent_any(TIMEEVENT_TYPE_POISON, poison_timeevent_check)) {
            // Set event type to recovery.
            timeevent.params[0].integer_value = POISON_EVENT_RECOVERY;

            // Schedule recovery event in 120 seconds.
            sub_45A950(&datetime, 120000);
            if (!timeevent_add_delay(&timeevent, &datetime)) {
                return false;
            }
        }
    }

    // Update health bar.
    ui_refresh_health_bar(obj);

    return true;
}
