#include "game/ce_config.h"

#include "game/settings.h"

typedef struct CeConfigDefault {
    const char* section;
    const char* key;
    const char* value;
} CeConfigDefault;

static void ce_config_register_default(Settings* settings, Settings* legacy_settings, const CeConfigDefault* entry);

static const CeConfigDefault ce_config_defaults[] = {
    { "system", "Module", "" },
    { "system", "Profile", "" },
    { "system", "StartupReport", "1" },
    { "system", "StartupReportPath", "restored-ce-startup-report.txt" },

    { "display", "Width", "800" },
    { "display", "Height", "600" },
    { "display", "Windowed", "0" },
    { "display", "ShowFPS", "0" },
    { "display", "ScrollFPS", "35" },
    { "display", "ScrollDist", "10" },
    { "display", "Logos", "1" },
    { "display", "Intro", "1" },

    { "qol", "IdentificationCost", "0" },
    { "qol", "LevelCap", "0" },
    { "qol", "StatsCap", "0" },
    { "qol", "SpeedCap", "0" },
    { "qol", "ReactionModifierCap", "0" },
    { "qol", "CarryWeightCap", "0" },
    { "qol", "StolenGoldCap", "0" },
    { "qol", "NameLength", "0" },
    { "qol", "DisableCombatMusic", "0" },
    { "qol", "DisableSpellLevelRequirement", "0" },
    { "qol", "DisableXPPerHit", "0" },
    { "qol", "FullXPPerKill", "0" },
    { "qol", "DisableSpriteMirroring", "0" },
    { "qol", "HideZeroDamageToCreatures", "0" },
    { "qol", "HideZeroDamageToObjects", "0" },
    { "qol", "NoCharacterPointsAtStart", "0" },
    { "qol", "StartStatsAtOne", "0" },
    { "qol", "MagicalAptitudeBonus", "0" },
    { "qol", "TechnologicalAptitudeBonus", "0" },
    { "qol", "StartingFatePoint", "0" },
    { "qol", "DisableNaturalRecovery", "0" },
    { "qol", "PoisonImmunity", "0" },
    { "qol", "DisableProwlingToHitBonus", "0" },
    { "qol", "DisableAptitudeToHitPenalty", "0" },
    { "qol", "TechnologicalCursor", "0" },
    { "qol", "DisableGlobalLightingChange", "0" },
    { "qol", "FreeBuying", "0" },
    { "qol", "FreeSelling", "0" },
    { "qol", "EndlessConsumables", "0" },
    { "qol", "ShareCurrentSkillTraining", "0" },
    { "qol", "ReducedIntRequirementsForSchemes", "0" },
    { "qol", "InventoryActiveForAllFollowers", "0" },
    { "qol", "UseAnyEquipment", "0" },
};

void ce_config_ensure_defaults(void)
{
    Settings settings;
    Settings legacy_settings;
    int index;

    settings_init(&settings, "arcanum-ce.cfg");
    settings_load(&settings);

    settings_init(&legacy_settings, "HighRes/config.ini");
    settings_load(&legacy_settings);

    settings_set_write_defaults(&settings, true);

    for (index = 0; index < (int)(sizeof(ce_config_defaults) / sizeof(ce_config_defaults[0])); index++) {
        ce_config_register_default(&settings, &legacy_settings, &(ce_config_defaults[index]));
    }

    settings_save(&settings);
    settings_exit(&legacy_settings);
    settings_exit(&settings);
}

void ce_config_register_default(Settings* settings, Settings* legacy_settings, const CeConfigDefault* entry)
{
    const char* legacy_value;

    if (settings_get_str_value_section(settings, entry->section, entry->key) != NULL) {
        return;
    }

    legacy_value = settings_get_str_value(settings, entry->key);
    if (legacy_value == NULL) {
        legacy_value = settings_get_str_value(legacy_settings, entry->key);
    }

    settings_register_section(settings,
        entry->section,
        entry->key,
        legacy_value != NULL ? legacy_value : entry->value,
        NULL);
}
