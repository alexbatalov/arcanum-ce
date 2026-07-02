#include "game/highres_config.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL3/SDL_stdinc.h>

#include <tig/art.h>

static void highres_config_reset(void);
static void highres_config_parse_line(char* line);
static void highres_config_trim(char** start_ptr, char** end_ptr);
static bool highres_config_parse_int(const char* str, int* value_ptr);

static HighResConfig highres_config;

void highres_config_load(void)
{
    FILE* stream;
    char line[512];

    highres_config_reset();

    stream = fopen("HighRes/config.ini", "rt");
    if (stream == NULL) {
        return;
    }

    highres_config.loaded = true;

    while (fgets(line, sizeof(line), stream) != NULL) {
        highres_config_parse_line(line);
    }

    fclose(stream);
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

void highres_config_parse_line(char* line)
{
    char* comment;
    char* sep;
    char* key_start;
    char* key_end;
    char* value_start;
    char* value_end;
    int value;

    comment = strstr(line, "//");
    if (comment != NULL) {
        *comment = '\0';
    }

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

    if (SDL_strcasecmp(key_start, "Width") == 0) {
        if (value >= 800) {
            highres_config.width = value;
        }
    } else if (SDL_strcasecmp(key_start, "Height") == 0) {
        if (value >= 600) {
            highres_config.height = value;
        }
    } else if (SDL_strcasecmp(key_start, "Windowed") == 0) {
        highres_config.windowed = value != 0;
    } else if (SDL_strcasecmp(key_start, "ShowFPS") == 0) {
        highres_config.show_fps = value != 0;
    } else if (SDL_strcasecmp(key_start, "ScrollFPS") == 0) {
        if (value > 0) {
            highres_config.scroll_fps = value;
        }
    } else if (SDL_strcasecmp(key_start, "ScrollDist") == 0) {
        if (value >= 0) {
            highres_config.scroll_dist = value;
        }
    } else if (SDL_strcasecmp(key_start, "IdentificationCost") == 0) {
        if (value >= 0) {
            highres_config.identification_cost = value;
        }
    } else if (SDL_strcasecmp(key_start, "LevelCap") == 0) {
        if (value >= 0) {
            highres_config.level_cap = value;
        }
    } else if (SDL_strcasecmp(key_start, "StatsCap") == 0) {
        if (value >= 0) {
            highres_config.stats_cap = value;
        }
    } else if (SDL_strcasecmp(key_start, "SpeedCap") == 0) {
        if (value >= 0) {
            highres_config.speed_cap = value;
        }
    } else if (SDL_strcasecmp(key_start, "ReactionModifierCap") == 0) {
        if (value >= 0) {
            highres_config.reaction_modifier_cap = value;
        }
    } else if (SDL_strcasecmp(key_start, "CarryWeightCap") == 0) {
        if (value >= 0) {
            highres_config.carry_weight_cap = value;
        }
    } else if (SDL_strcasecmp(key_start, "StolenGoldCap") == 0) {
        if (value >= 0) {
            highres_config.stolen_gold_cap = value;
        }
    } else if (SDL_strcasecmp(key_start, "NameLength") == 0) {
        if (value >= 0) {
            highres_config.name_length = value;
        }
    } else if (SDL_strcasecmp(key_start, "Logos") == 0) {
        highres_config.logos = value != 0;
    } else if (SDL_strcasecmp(key_start, "Intro") == 0) {
        highres_config.intro = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableCombatMusic") == 0) {
        highres_config.disable_combat_music = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableSpellLevelRequirement") == 0) {
        highres_config.disable_spell_level_requirement = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableXPPerHit") == 0) {
        highres_config.disable_xp_per_hit = value != 0;
    } else if (SDL_strcasecmp(key_start, "FullXPPerKill") == 0) {
        highres_config.full_xp_per_kill = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableSpriteMirroring") == 0) {
        highres_config.disable_sprite_mirroring = value != 0;
        tig_art_mirroring_set_enabled(!highres_config.disable_sprite_mirroring);
    } else if (SDL_strcasecmp(key_start, "HideZeroDamageToCreatures") == 0) {
        highres_config.hide_zero_damage_to_creatures = value != 0;
    } else if (SDL_strcasecmp(key_start, "HideZeroDamageToObjects") == 0) {
        highres_config.hide_zero_damage_to_objects = value != 0;
    } else if (SDL_strcasecmp(key_start, "NoCharacterPointsAtStart") == 0) {
        highres_config.no_character_points_at_start = value != 0;
    } else if (SDL_strcasecmp(key_start, "StartStatsAtOne") == 0) {
        highres_config.start_stats_at_one = value != 0;
    } else if (SDL_strcasecmp(key_start, "MagicalAptitudeBonus") == 0) {
        highres_config.magical_aptitude_bonus = value != 0;
    } else if (SDL_strcasecmp(key_start, "TechnologicalAptitudeBonus") == 0) {
        highres_config.technological_aptitude_bonus = value != 0;
    } else if (SDL_strcasecmp(key_start, "StartingFatePoint") == 0) {
        highres_config.starting_fate_point = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableNaturalRecovery") == 0) {
        highres_config.disable_natural_recovery = value != 0;
    } else if (SDL_strcasecmp(key_start, "PoisonImmunity") == 0) {
        highres_config.poison_immunity = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableProwlingToHitBonus") == 0) {
        highres_config.disable_prowling_to_hit_bonus = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableAptitudeToHitPenalty") == 0) {
        highres_config.disable_aptitude_to_hit_penalty = value != 0;
    } else if (SDL_strcasecmp(key_start, "TechnologicalCursor") == 0) {
        highres_config.technological_cursor = value != 0;
    } else if (SDL_strcasecmp(key_start, "DisableGlobalLightingChange") == 0) {
        highres_config.disable_global_lighting_change = value != 0;
    } else if (SDL_strcasecmp(key_start, "FreeBuying") == 0) {
        highres_config.free_buying = value != 0;
    } else if (SDL_strcasecmp(key_start, "FreeSelling") == 0) {
        highres_config.free_selling = value != 0;
    } else if (SDL_strcasecmp(key_start, "EndlessConsumables") == 0) {
        highres_config.endless_consumables = value != 0;
    } else if (SDL_strcasecmp(key_start, "ShareCurrentSkillTraining") == 0) {
        highres_config.share_current_skill_training = value != 0;
    } else if (SDL_strcasecmp(key_start, "ReducedIntRequirementsForSchemes") == 0) {
        highres_config.reduced_int_requirements_for_schemes = value != 0;
    } else if (SDL_strcasecmp(key_start, "InventoryActiveForAllFollowers") == 0) {
        highres_config.inventory_active_for_all_followers = value != 0;
    } else if (SDL_strcasecmp(key_start, "UseAnyEquipment") == 0) {
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
