#include "game/highres_config.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL_stdinc.h>

#include <tig/art.h>

static void highres_config_reset(void);
static bool highres_config_load_file(const char* path);
static void highres_config_strip_comments(char* line);
static void highres_config_parse_line(char* line);
static void highres_config_trim(char** start_ptr, char** end_ptr);
static bool highres_config_parse_int(const char* str, int* value_ptr);
static bool highres_config_key_equals(const char* lhs, const char* rhs);

static HighResConfig highres_config;

void highres_config_load(void)
{
    highres_config_reset();

    highres_config_load_file("HighRes/config.ini");
    highres_config_load_file("arcanum-ce.cfg");
}

const HighResConfig* highres_config_get(void)
{
    return &highres_config;
}

void highres_config_reset(void)
{
    highres_config.loaded = false;
    highres_config.width = 800;
    highres_config.height = 600;
    highres_config.windowed = false;
    highres_config.show_fps = false;
    highres_config.scroll_fps = 35;
    highres_config.scroll_dist = 10;
    highres_config.identification_cost = 0;
    highres_config.level_cap = 0;
    highres_config.stats_cap = 0;
    highres_config.speed_cap = 0;
    highres_config.reaction_modifier_cap = 0;
    highres_config.carry_weight_cap = 0;
    highres_config.stolen_gold_cap = 0;
    highres_config.name_length = 0;
    highres_config.logos = true;
    highres_config.intro = true;
    highres_config.disable_combat_music = false;
    highres_config.disable_spell_level_requirement = false;
    highres_config.disable_xp_per_hit = false;
    highres_config.full_xp_per_kill = false;
    highres_config.disable_sprite_mirroring = false;
    highres_config.hide_zero_damage_to_creatures = false;
    highres_config.hide_zero_damage_to_objects = false;
    highres_config.no_character_points_at_start = false;
    highres_config.start_stats_at_one = false;
    highres_config.magical_aptitude_bonus = false;
    highres_config.technological_aptitude_bonus = false;
    highres_config.starting_fate_point = false;
    highres_config.disable_natural_recovery = false;
    highres_config.poison_immunity = false;
    highres_config.disable_prowling_to_hit_bonus = false;
    highres_config.disable_aptitude_to_hit_penalty = false;
    highres_config.technological_cursor = false;
    highres_config.disable_global_lighting_change = false;
    highres_config.free_buying = false;
    highres_config.free_selling = false;
    highres_config.endless_consumables = false;
    highres_config.share_current_skill_training = false;
    highres_config.reduced_int_requirements_for_schemes = false;
    highres_config.inventory_active_for_all_followers = false;
    highres_config.use_any_equipment = false;

    tig_art_mirroring_set_enabled(true);
}

bool highres_config_load_file(const char* path)
{
    FILE* stream;
    char line[512];

    stream = fopen(path, "rt");
    if (stream == NULL) {
        return false;
    }

    highres_config.loaded = true;

    while (fgets(line, sizeof(line), stream) != NULL) {
        highres_config_parse_line(line);
    }

    fclose(stream);
    return true;
}

void highres_config_strip_comments(char* line)
{
    char* comment = strstr(line, "//");
    char* hash = strchr(line, '#');
    char* semicolon = strchr(line, ';');

    if (hash != NULL && (comment == NULL || hash < comment)) {
        comment = hash;
    }

    if (semicolon != NULL && (comment == NULL || semicolon < comment)) {
        comment = semicolon;
    }

    if (comment != NULL) {
        *comment = '\0';
    }
}

void highres_config_parse_line(char* line)
{
    char* sep;
    char* key_start;
    char* key_end;
    char* value_start;
    char* value_end;
    int value;

    highres_config_strip_comments(line);

    key_start = line;
    key_end = key_start + strlen(key_start);
    highres_config_trim(&key_start, &key_end);
    *key_end = '\0';

    if (*key_start == '\0') {
        return;
    }

    sep = strchr(key_start, '=');
    if (sep == NULL) {
        return;
    }

    *sep = '\0';

    key_end = sep;
    highres_config_trim(&key_start, &key_end);
    *key_end = '\0';

    value_start = sep + 1;
    value_end = value_start + strlen(value_start);
    highres_config_trim(&value_start, &value_end);
    *value_end = '\0';

    if (!highres_config_parse_int(value_start, &value)) {
        return;
    }

    if (highres_config_key_equals(key_start, "Width")) {
        if (value >= 800) {
            highres_config.width = value;
        }
    } else if (highres_config_key_equals(key_start, "Height")) {
        if (value >= 600) {
            highres_config.height = value;
        }
    } else if (highres_config_key_equals(key_start, "Windowed")) {
        highres_config.windowed = value != 0;
    } else if (highres_config_key_equals(key_start, "ShowFPS")) {
        highres_config.show_fps = value != 0;
    } else if (highres_config_key_equals(key_start, "ScrollFPS")) {
        if (value > 0) {
            highres_config.scroll_fps = value;
        }
    } else if (highres_config_key_equals(key_start, "ScrollDist")) {
        if (value >= 0) {
            highres_config.scroll_dist = value;
        }
    } else if (highres_config_key_equals(key_start, "IdentificationCost")) {
        if (value >= 0) {
            highres_config.identification_cost = value;
        }
    } else if (highres_config_key_equals(key_start, "LevelCap")) {
        if (value >= 0) {
            highres_config.level_cap = value;
        }
    } else if (highres_config_key_equals(key_start, "StatsCap")) {
        if (value >= 0) {
            highres_config.stats_cap = value;
        }
    } else if (highres_config_key_equals(key_start, "SpeedCap")) {
        if (value >= 0) {
            highres_config.speed_cap = value;
        }
    } else if (highres_config_key_equals(key_start, "ReactionModifierCap")) {
        if (value >= 0) {
            highres_config.reaction_modifier_cap = value;
        }
    } else if (highres_config_key_equals(key_start, "CarryWeightCap")) {
        if (value >= 0) {
            highres_config.carry_weight_cap = value;
        }
    } else if (highres_config_key_equals(key_start, "StolenGoldCap")) {
        if (value >= 0) {
            highres_config.stolen_gold_cap = value;
        }
    } else if (highres_config_key_equals(key_start, "NameLength")) {
        if (value >= 0) {
            highres_config.name_length = value;
        }
    } else if (highres_config_key_equals(key_start, "Logos")) {
        highres_config.logos = value != 0;
    } else if (highres_config_key_equals(key_start, "Intro")) {
        highres_config.intro = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableCombatMusic")) {
        highres_config.disable_combat_music = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableSpellLevelRequirement")) {
        highres_config.disable_spell_level_requirement = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableXPPerHit")) {
        highres_config.disable_xp_per_hit = value != 0;
    } else if (highres_config_key_equals(key_start, "FullXPPerKill")) {
        highres_config.full_xp_per_kill = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableSpriteMirroring")) {
        highres_config.disable_sprite_mirroring = value != 0;
        tig_art_mirroring_set_enabled(!highres_config.disable_sprite_mirroring);
    } else if (highres_config_key_equals(key_start, "HideZeroDamageToCreatures")) {
        highres_config.hide_zero_damage_to_creatures = value != 0;
    } else if (highres_config_key_equals(key_start, "HideZeroDamageToObjects")) {
        highres_config.hide_zero_damage_to_objects = value != 0;
    } else if (highres_config_key_equals(key_start, "NoCharacterPointsAtStart")) {
        highres_config.no_character_points_at_start = value != 0;
    } else if (highres_config_key_equals(key_start, "StartStatsAtOne")) {
        highres_config.start_stats_at_one = value != 0;
    } else if (highres_config_key_equals(key_start, "MagicalAptitudeBonus")) {
        highres_config.magical_aptitude_bonus = value != 0;
    } else if (highres_config_key_equals(key_start, "TechnologicalAptitudeBonus")) {
        highres_config.technological_aptitude_bonus = value != 0;
    } else if (highres_config_key_equals(key_start, "StartingFatePoint")) {
        highres_config.starting_fate_point = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableNaturalRecovery")) {
        highres_config.disable_natural_recovery = value != 0;
    } else if (highres_config_key_equals(key_start, "PoisonImmunity")) {
        highres_config.poison_immunity = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableProwlingToHitBonus")) {
        highres_config.disable_prowling_to_hit_bonus = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableAptitudeToHitPenalty")) {
        highres_config.disable_aptitude_to_hit_penalty = value != 0;
    } else if (highres_config_key_equals(key_start, "TechnologicalCursor")) {
        highres_config.technological_cursor = value != 0;
    } else if (highres_config_key_equals(key_start, "DisableGlobalLightingChange")) {
        highres_config.disable_global_lighting_change = value != 0;
    } else if (highres_config_key_equals(key_start, "FreeBuying")) {
        highres_config.free_buying = value != 0;
    } else if (highres_config_key_equals(key_start, "FreeSelling")) {
        highres_config.free_selling = value != 0;
    } else if (highres_config_key_equals(key_start, "EndlessConsumables")) {
        highres_config.endless_consumables = value != 0;
    } else if (highres_config_key_equals(key_start, "ShareCurrentSkillTraining")) {
        highres_config.share_current_skill_training = value != 0;
    } else if (highres_config_key_equals(key_start, "ReducedIntRequirementsForSchemes")) {
        highres_config.reduced_int_requirements_for_schemes = value != 0;
    } else if (highres_config_key_equals(key_start, "InventoryActiveForAllFollowers")) {
        highres_config.inventory_active_for_all_followers = value != 0;
    } else if (highres_config_key_equals(key_start, "UseAnyEquipment")) {
        highres_config.use_any_equipment = value != 0;
    }
}

void highres_config_trim(char** start_ptr, char** end_ptr)
{
    while (*start_ptr < *end_ptr && isspace((unsigned char)**start_ptr)) {
        (*start_ptr)++;
    }

    while (*end_ptr > *start_ptr && isspace((unsigned char)*((*end_ptr) - 1))) {
        (*end_ptr)--;
    }
}

bool highres_config_parse_int(const char* str, int* value_ptr)
{
    char* end;
    long value;

    if (*str == '\0') {
        return false;
    }

    value = strtol(str, &end, 10);
    if (end == str) {
        return false;
    }

    while (*end != '\0') {
        if (!isspace((unsigned char)*end)) {
            return false;
        }

        end++;
    }

    *value_ptr = (int)value;
    return true;
}

bool highres_config_key_equals(const char* lhs, const char* rhs)
{
    while (*lhs != '\0' || *rhs != '\0') {
        while (*lhs == '_' || *lhs == '-') {
            lhs++;
        }

        while (*rhs == '_' || *rhs == '-') {
            rhs++;
        }

        if (tolower((unsigned char)*lhs) != tolower((unsigned char)*rhs)) {
            return false;
        }

        if (*lhs == '\0') {
            return true;
        }

        lhs++;
        rhs++;
    }

    return true;
}
