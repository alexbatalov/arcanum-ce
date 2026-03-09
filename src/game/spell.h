#ifndef ARCANUM_GAME_SPELL_H_
#define ARCANUM_GAME_SPELL_H_

#include "game/context.h"

typedef enum College {
    COLLEGE_CONVEYANCE,
    COLLEGE_DIVINATION,
    COLLEGE_AIR,
    COLLEGE_EARTH,
    COLLEGE_FIRE,
    COLLEGE_WATER,
    COLLEGE_FORCE,
    COLLEGE_MENTAL,
    COLLEGE_META,
    COLLEGE_MORPH,
    COLLEGE_NATURE,
    COLLEGE_NECROMANTIC_BLACK,
    COLLEGE_NECROMANTIC_WHITE,
    COLLEGE_PHANTASM,
    COLLEGE_SUMMONING,
    COLLEGE_TEMPORAL,
    COLLEGE_COUNT,
} College;

#define SPELL_MASTERY_IDX COLLEGE_COUNT

typedef enum Spell {
    SPELL_DISARM,
    SPELL_UNLOCKING_CANTRIP,
    SPELL_UNSEEN_FORCE,
    SPELL_SPATIAL_DISTORTION,
    SPELL_TELEPORTATION,
    SPELL_SENSE_ALIGNMENT,
    SPELL_SEE_CONTENTS,
    SPELL_READ_AURA,
    SPELL_SENSE_HIDDEN,
    SPELL_DIVINE_MAGICK,
    SPELL_VITALITY_OF_AIR,
    SPELL_POISON_VAPOURS,
    SPELL_CALL_WINDS,
    SPELL_BODY_OF_AIR,
    SPELL_CALL_AIR_ELEMENTAL,
    SPELL_STRENGTH_OF_EARTH,
    SPELL_STONE_THROW,
    SPELL_WALL_OF_STONE,
    SPELL_BODY_OF_STONE,
    SPELL_CALL_EARTH_ELEMENTAL,
    SPELL_AGILITY_OF_FIRE,
    SPELL_WALL_OF_FIRE,
    SPELL_FIREFLASH,
    SPELL_BODY_OF_FIRE,
    SPELL_CALL_FIRE_ELEMENTAL,
    SPELL_PURITY_OF_WATER,
    SPELL_CALL_FOG,
    SPELL_SQUALL_OF_ICE,
    SPELL_BODY_OF_WATER,
    SPELL_CALL_WATER_ELEMENTAL,
    SPELL_SHIELD_OF_PROTECTION,
    SPELL_JOLT,
    SPELL_WALL_OF_FORCE,
    SPELL_BOLT_OF_LIGHTNING,
    SPELL_DISINTEGRATE,
    SPELL_CHARM,
    SPELL_STUN,
    SPELL_DRAIN_WILL,
    SPELL_NIGHTMARE,
    SPELL_DOMINATE_WILL,
    SPELL_RESIST_MAGICK,
    SPELL_DISPERSE_MAGICK,
    SPELL_DWEOMER_SHIELD,
    SPELL_BONDS_OF_MAGICK,
    SPELL_REFLECTION_SHIELD,
    SPELL_HARDENED_HANDS,
    SPELL_WEAKEN,
    SPELL_SHRINK,
    SPELL_FLESH_TO_STONE,
    SPELL_POLYMORPH,
    SPELL_CHARM_BEAST,
    SPELL_ENTANGLE,
    SPELL_CONTROL_BEAST,
    SPELL_SUCCOUR_BEAST,
    SPELL_REGENERATE,
    SPELL_HARM,
    SPELL_CONJURE_SPIRIT,
    SPELL_SUMMON_UNDEAD,
    SPELL_CREATE_UNDEAD,
    SPELL_QUENCH_LIFE,
    SPELL_MINOR_HEALING,
    SPELL_HALT_POISON,
    SPELL_MAJOR_HEALING,
    SPELL_SANCTUARY,
    SPELL_RESURRECT,
    SPELL_ILLUMINATE,
    SPELL_FLASH,
    SPELL_BLUR_SIGHT,
    SPELL_PHANTASMAL_FIEND,
    SPELL_INVISIBILITY,
    SPELL_PLAGUE_OF_INSECTS,
    SPELL_ORCISH_CHAMPION,
    SPELL_GUARDIAN_OGRE,
    SPELL_HELLGATE,
    SPELL_FAMILIAR,
    SPELL_MAGELOCK,
    SPELL_CONGEAL_TIME,
    SPELL_HASTEN,
    SPELL_STASIS,
    SPELL_TEMPUS_FUGIT,
    SPELL_COUNT,
} Spell;

bool spell_init(GameInitInfo* init_info);
void spell_exit(void);
void spell_set_defaults(int64_t obj);
int spell_icon(int spell);
size_t spell_name_length(int spell);
char* spell_name(int spell);
char* spell_description(int spell);
int spell_cost(int spell);
int spell_cast_cost(int spell, int64_t obj);
int spell_maintain_cost(int spell, int64_t obj, int* period_ptr);
int spell_money(int spell);
int spell_min_intelligence(int spell);
int spell_min_willpower(int spell);
int spell_min_level(int spell);
bool spell_add(int64_t obj, int spell, bool force);
bool spell_is_known(int64_t obj, int spell);
bool spell_remove(int64_t obj, int spell);
char* spell_college_name(int college);
char* spell_college_description(int college);
int spell_college_small_icon(int college);
int spell_college_large_icon(int college);
int spell_college_level_get(int64_t obj, int college);
bool spell_college_is_known(int64_t obj, int college);
int spell_college_level_set(int64_t obj, int college, int level);
bool spell_check_intelligence(int64_t obj, int intelligence);
bool spell_check_willpower(int64_t obj, int willpower);
bool spell_can_become_master_of_college(int64_t obj, int college);
int spell_mastery_get(int64_t obj);
void spell_mastery_set(int64_t obj, int college);

#define MAX_SPELL_LEVEL 5
#define COLLEGE_FROM_SPELL(spell) ((spell) / MAX_SPELL_LEVEL)
#define LEVEL_FROM_SPELL(spell) ((spell) % MAX_SPELL_LEVEL)

#endif /* ARCANUM_GAME_SPELL_H_ */
