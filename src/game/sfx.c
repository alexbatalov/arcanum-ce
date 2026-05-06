#include "game/sfx.h"

#include "game/a_name.h"
#include "game/descriptions.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/material.h"
#include "game/obj.h"
#include "game/player.h"
#include "game/random.h"
#include "game/tile.h"

/**
 * Defines field numbers in `snd_item.mes`.
 *
 * NOTE: These fields do not match `ItemSoundType`.
 */
typedef enum SndItemField {
    SND_ITEM_F_USE,
    SND_ITEM_F_BUSTED,
    SND_ITEM_F_DESTROYED,
    SND_ITEM_F_MISS,
    SND_ITEM_F_HIT_1,
    SND_ITEM_F_HIT_2,
    SND_ITEM_F_CRIT_HIT,
    SND_ITEM_F_OUT_OF_AMMO,
} SndItemField;

/**
 * Defines a reduced set of materials to determine sounds in melee combat.
 *
 * See `sfx_melee_sounds` how item materials are mapped to this reduced set.
 */
typedef enum MeleeMaterialSound {
    MELEE_MAT_SOUND_FLESH,
    MELEE_MAT_SOUND_LIGHT_METAL,
    MELEE_MAT_SOUND_HEAVY_METAL,
    MELEE_MAT_SOUND_STONE,
    MELEE_MAT_SOUND_WOOD,
} MeleeMaterialSound;

/**
 * Defines item materials for picking up/dropping items.
 *
 * See `sfx_item_sounds` how item materials are mapped to this reduced set.
 */
typedef enum ItemMaterialSound {
    ITEM_MAT_SOUND_FLESH,
    ITEM_MAT_SOUND_LIGHT_METAL,
    ITEM_MAT_SOUND_HEAVY_METAL,
    ITEM_MAT_SOUND_STONE,
    ITEM_MAT_SOUND_WOOD,
    ITEM_MAT_SOUND_GLASS,
    ITEM_MAT_SOUND_CLOTH,
    ITEM_MAT_SOUND_PAPER,
} ItemMaterialSound;

/**
 * Maps weapon and target material types to a reduced set of sounds used in
 * melee combat.
 *
 * 0x5BB928
 */
static MeleeMaterialSound sfx_melee_sounds[MATERIAL_COUNT] = {
    /*  MATERIAL_STONE */ MELEE_MAT_SOUND_STONE,
    /*  MATERIAL_BRICK */ MELEE_MAT_SOUND_STONE,
    /*   MATERIAL_WOOD */ MELEE_MAT_SOUND_WOOD,
    /*  MATERIAL_PLANT */ MELEE_MAT_SOUND_WOOD,
    /*  MATERIAL_FLESH */ MELEE_MAT_SOUND_FLESH,
    /*  MATERIAL_METAL */ MELEE_MAT_SOUND_HEAVY_METAL,
    /*  MATERIAL_GLASS */ MELEE_MAT_SOUND_LIGHT_METAL,
    /*  MATERIAL_CLOTH */ MELEE_MAT_SOUND_FLESH,
    /* MATERIAL_LIQUID */ MELEE_MAT_SOUND_FLESH,
    /*  MATERIAL_PAPER */ MELEE_MAT_SOUND_FLESH,
    /*    MATERIAL_GAS */ MELEE_MAT_SOUND_FLESH,
    /*  MATERIAL_FORCE */ MELEE_MAT_SOUND_LIGHT_METAL,
    /*   MATERIAL_FIRE */ MELEE_MAT_SOUND_FLESH,
    /* MATERIAL_POWDER */ MELEE_MAT_SOUND_FLESH,
};

/**
 * Base sound ID when picking up an item.
 */
#define BASE_ITEM_PICKUP_SOUND_ID 5950

/**
 * Special case - sound ID when picking up coins.
 */
#define GOLD_PICKUP_SOUND_ID 5958

/**
 * Base sound ID when dropping an item.
 */
#define BASE_ITEM_DROP_SOUND_ID 5960

/**
 * Special case - sound ID when dropping coins.
 */
#define GOLD_DROP_SOUND_ID 5968

/**
 * Maps item material types to a reduced set of item sound materials.
 *
 * These sounds are only used for pickup/drop actions.
 *
 * 0x5BB960
 */
static ItemMaterialSound sfx_item_sounds[MATERIAL_COUNT] = {
    /*  MATERIAL_STONE */ ITEM_MAT_SOUND_STONE,
    /*  MATERIAL_BRICK */ ITEM_MAT_SOUND_STONE,
    /*   MATERIAL_WOOD */ ITEM_MAT_SOUND_WOOD,
    /*  MATERIAL_PLANT */ ITEM_MAT_SOUND_WOOD,
    /*  MATERIAL_FLESH */ ITEM_MAT_SOUND_FLESH,
    /*  MATERIAL_METAL */ ITEM_MAT_SOUND_HEAVY_METAL,
    /*  MATERIAL_GLASS */ ITEM_MAT_SOUND_GLASS,
    /*  MATERIAL_CLOTH */ ITEM_MAT_SOUND_CLOTH,
    /* MATERIAL_LIQUID */ ITEM_MAT_SOUND_FLESH,
    /*  MATERIAL_PAPER */ ITEM_MAT_SOUND_PAPER,
    /*    MATERIAL_GAS */ ITEM_MAT_SOUND_FLESH,
    /*  MATERIAL_FORCE */ ITEM_MAT_SOUND_LIGHT_METAL,
    /*   MATERIAL_FIRE */ ITEM_MAT_SOUND_FLESH,
    /* MATERIAL_POWDER */ ITEM_MAT_SOUND_FLESH,
};

/**
 * Base footstep sound ID for critters wearing plate armor.
 */
#define BASE_PLATE_ARMOR_FOOTSTEP_SOUND_ID 2900

/**
 * Base sound IDs for footstep sounds based on tile type.
 *
 * There are exactly 4 different sounds for each type of type. The exact sound
 * is picked randomly in `sfx_critter_sound`. Dirt, wood, and stone have extra
 * set of sounds for critters wearing chainmail.
 *
 * 0x5BB998
 */
static int sfx_base_footstep_sound_ids[TILE_SOUND_COUNT] = {
    /*  TILE_SOUND_DIRT */ 2904,
    /*  TILE_SOUND_SAND */ 2912,
    /*  TILE_SOUND_SNOW */ 2916,
    /* TILE_SOUND_STONE */ 2920,
    /* TILE_SOUND_WATER */ 2928,
    /*  TILE_SOUND_WOOD */ 2932,
};

/**
 * Determines the sound ID for an item-related action.
 *
 * 0x4F0BF0
 */
int sfx_item_sound(int64_t item_obj, int64_t parent_obj, int64_t target_obj, ItemSoundType type)
{
    int sound_id = -1;
    int sound_effect;
    int material;
    ItemMaterialSound item_mat_sound;
    MeleeMaterialSound target_mat_sound;
    MeleeMaterialSound weapon_mat_sound;
    char path[TIG_MAX_PATH];

    // Retrieve the item's sound effect.
    if (item_obj != OBJ_HANDLE_NULL) {
        sound_effect = obj_field_int32_get(item_obj, OBJ_F_SOUND_EFFECT);
    } else {
        sound_effect = 0;
    }

    // Handle action.
    switch (type) {
    case ITEM_SOUND_PICKUP:
        // `parent_obj` picked up `item_obj`.
        if (item_obj != OBJ_HANDLE_NULL) {
            // Special case - gold/coins.
            if (obj_field_int32_get(item_obj, OBJ_F_ITEM_DESCRIPTION_UNKNOWN) == BP_GOLD) {
                sound_id = GOLD_PICKUP_SOUND_ID;
            } else {
                // Otherwise, determine pick up sound based on item material.
                material = obj_field_int32_get(item_obj, OBJ_F_MATERIAL);
                item_mat_sound = sfx_item_sounds[material];

                // Adjust metal sounds for lightweight items.
                if (item_mat_sound == ITEM_MAT_SOUND_HEAVY_METAL
                    && item_weight(item_obj, parent_obj) <= 2000) {
                    item_mat_sound = ITEM_MAT_SOUND_LIGHT_METAL;
                }

                sound_id = BASE_ITEM_PICKUP_SOUND_ID + item_mat_sound;
            }
        }
        break;
    case ITEM_SOUND_DROP:
        // `parent_obj` dropped `item_obj`.
        if (item_obj != OBJ_HANDLE_NULL) {
            // Special case - gold/coins.
            if (obj_field_int32_get(item_obj, OBJ_F_ITEM_DESCRIPTION_UNKNOWN) == BP_GOLD) {
                sound_id = GOLD_DROP_SOUND_ID;
            } else {
                // Otherwise, determine drop sound based on item material.
                material = obj_field_int32_get(item_obj, OBJ_F_MATERIAL);
                item_mat_sound = sfx_item_sounds[material];

                // Adjust metal sounds for lightweight items.
                if (item_mat_sound == ITEM_MAT_SOUND_HEAVY_METAL
                    && item_weight(item_obj, parent_obj) <= 2000) {
                    item_mat_sound = ITEM_MAT_SOUND_LIGHT_METAL;
                }

                sound_id = BASE_ITEM_DROP_SOUND_ID + item_mat_sound;
            }
        }
        break;
    case WEAPON_SOUND_USE:
        // `parent_obj` used `item_obj` on `target_obj`.
        if (sound_effect != 0) {
            sound_id = sound_effect + SND_ITEM_F_USE;
        }
        break;
    case WEAPON_SOUND_OUT_OF_AMMO:
        // `parent_obj` attempted to fire at `target_obj` with `item_obj` but
        // there were no ammo.
        if (sound_effect != 0) {
            sound_id = sound_effect + SND_ITEM_F_OUT_OF_AMMO;
        }
        break;
    case WEAPON_SOUND_HIT:
    case WEAPON_SOUND_CRITICAL_HIT:
        // `parent_obj` did hit `target_obj` with `item_obj`.
        if (target_obj != OBJ_HANDLE_NULL) {
            // Use item's sound effect if available.
            if (sound_effect != 0) {
                if (type == WEAPON_SOUND_CRITICAL_HIT) {
                    sound_id = sound_effect + SND_ITEM_F_CRIT_HIT;
                } else {
                    // Randomly select between two hit sounds.
                    if (random_between(0, 1) == 1) {
                        sound_id = sound_effect + SND_ITEM_F_HIT_2;
                    } else {
                        sound_id = sound_effect + SND_ITEM_F_HIT_1;
                    }
                }

                // Verify the sound path exists.
                if (gsound_resolve_path(sound_id, path, sizeof(path)) == TIG_OK) {
                    break;
                }
            }

            // If target is a critter and wearing an armor, use that armor to
            // determine what exactly weapon hit.
            if (obj_type_is_critter(obj_field_int32_get(target_obj, OBJ_F_TYPE))) {
                int64_t armor_obj = item_wield_get(target_obj, ITEM_INV_LOC_ARMOR);
                if (armor_obj != OBJ_HANDLE_NULL) {
                    target_obj = armor_obj;
                }
            }

            // Determine target material sound.
            material = obj_field_int32_get(target_obj, OBJ_F_MATERIAL);
            target_mat_sound = sfx_melee_sounds[material];

            // Determine weapon material sound.
            if (item_obj != OBJ_HANDLE_NULL) {
                material = obj_field_int32_get(item_obj, OBJ_F_MATERIAL);
                weapon_mat_sound = sfx_melee_sounds[material];
                if (weapon_mat_sound == MELEE_MAT_SOUND_HEAVY_METAL
                    && item_weight(item_obj, parent_obj) <= 2000) {
                    weapon_mat_sound = MELEE_MAT_SOUND_LIGHT_METAL;
                }
            } else {
                // Unarmed hit - with fists.
                weapon_mat_sound = MELEE_MAT_SOUND_FLESH;
            }

            // Calculate sound ID based on weapon and target materials.
            sound_id = 7000 + weapon_mat_sound * 20 + target_mat_sound * 3 + random_between(0, 1);
        }
        break;
    case WEAPON_SOUND_MISS:
        // `parent_obj` did miss `target_obj` with `item_obj`.
        if (item_obj != OBJ_HANDLE_NULL && sound_effect != 0) {
            sound_id = sound_effect + SND_ITEM_F_MISS;
        } else {
            sound_id = 4010;
        }
        break;
    }

    return sound_id;
}

/**
 * Determines the sound ID for a critter-related action.
 *
 * 0x4F0ED0
 */
int sfx_critter_sound(int64_t obj, CritterSoundType type)
{
    int64_t pc_obj;
    tig_art_id_t art_id;
    int armor_type;
    int sound_id;
    int64_t loc;
    tig_art_id_t tile_art_id;
    int tile_sound_type;

    // Validate the object and ensure it's a critter.
    if (obj == OBJ_HANDLE_NULL
        || !obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return -1;
    }

    // Ensure the critter is the local player character.
    pc_obj = player_get_local_pc_obj();
    if (pc_obj == OBJ_HANDLE_NULL) {
        return -1;
    }

    // Check if the critter is disabled.
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_OFF) != 0) {
        return -1;
    }

    // Simple case - non-footstep sounds.
    if (type != CRITTER_SOUND_FOOTSTEPS) {
        return obj_field_int32_get(obj, OBJ_F_SOUND_EFFECT) + type;
    }

    // Handle footstep sounds based on armor worn and tile type.
    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    armor_type = tig_art_critter_id_armor_get(art_id);

    // Special case - plate armor have the same set of sounds on any surface.
    if (armor_type == TIG_ART_ARMOR_TYPE_PLATE
        || armor_type == TIG_ART_ARMOR_TYPE_PLATE_CLASSIC) {
        sound_id = BASE_PLATE_ARMOR_FOOTSTEP_SOUND_ID;
    } else {
        // Otherwise, determine footsteps sound based on tile type.
        loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
        tile_art_id = tile_art_id_at(loc);
        tile_sound_type = a_name_tile_sound(tile_art_id);
        sound_id = sfx_base_footstep_sound_ids[tile_sound_type];

        // Adjust for chainmail armor the specific tile types.
        if (armor_type == TIG_ART_ARMOR_TYPE_CHAIN
            && (tile_sound_type == TILE_SOUND_WOOD
                || tile_sound_type == TILE_SOUND_STONE
                || tile_sound_type == TILE_SOUND_DIRT)) {
            sound_id += 4;
        }
    }

    // Add some randomness (each set of footsteps is guaranteed to have 4
    // items).
    sound_id += random_between(0, 3);

    return sound_id;
}

/**
 * Determines the sound ID for a container-related action.
 *
 * 0x4F0FD0
 */
int sfx_container_sound(int64_t obj, ContainerSoundType type)
{
    int sound_effect;

    // Validate the object and ensure it's a container.
    if (obj == OBJ_HANDLE_NULL
        || obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_CONTAINER) {
        return -1;
    }

    // Retrieve base sound ID.
    sound_effect = obj_field_int32_get(obj, OBJ_F_SOUND_EFFECT);

    return sound_effect + type;
}

/**
 * Determines the sound ID for a portal-related action.
 *
 * 0x4F1010
 */
int sfx_portal_sound(int64_t obj, PortalSoundType type)
{
    int sound_effect;

    // Validate the object and ensure it's a portal.
    if (obj == OBJ_HANDLE_NULL
        || obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PORTAL) {
        return -1;
    }

    // Retrieve base sound ID.
    sound_effect = obj_field_int32_get(obj, OBJ_F_SOUND_EFFECT);

    return sound_effect + type;
}

/**
 * Determines the sound ID for miscellaneous object actions.
 *
 * 0x4F1050
 */
int sfx_misc_sound(int64_t obj, MiscSoundType type)
{
    int obj_type;
    int sound_effect;
    int sound_id = -1;
    char path[TIG_MAX_PATH];

    // Validate the object.
    if (obj == OBJ_HANDLE_NULL) {
        return -1;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)
        || obj_type == OBJ_TYPE_CONTAINER
        || obj_type == OBJ_TYPE_PORTAL
        || (obj_type_is_item(obj_type) && obj_type != OBJ_TYPE_WEAPON)) {
        // Invalid object type, no sound effect.
        sound_effect = 0;
    } else {
        // Scenery (mostly) and weapons - retrieve base sound effect.
        sound_effect = obj_field_int32_get(obj, OBJ_F_SOUND_EFFECT);
    }

    switch (type) {
    case MISC_SOUND_BUSTING:
        if (sound_effect == 0) {
            return -1;
        }

        // For weapons sound effect is the base number into `snd_item.mes` (so
        // sound ID is at field offset 1), otherwise sound effect is the base
        // number into `snd_misc.mes` (ID is at field offset 0).
        sound_id = obj_type == OBJ_TYPE_WEAPON
            ? sound_effect + SND_ITEM_F_BUSTED
            : sound_effect + MISC_SOUND_BUSTING;
        if (gsound_resolve_path(sound_id, path, sizeof(path)) != TIG_OK) {
            return -1;
        }
        break;
    case MISC_SOUND_DESTROYING:
        if (sound_effect == 0) {
            return -1;
        }

        // For weapons sound effect is the base number into `snd_item.mes`
        // (sound ID is at field offset 2), otherwise sound effect is the base
        // number into `snd_misc.mes` (ID is at field offset 1).
        sound_id = obj_type == OBJ_TYPE_WEAPON
            ? sound_effect + SND_ITEM_F_DESTROYED
            : sound_effect + MISC_SOUND_DESTROYING;
        if (gsound_resolve_path(sound_id, path, sizeof(path)) != TIG_OK) {
            return -1;
        }
        break;
    case MISC_SOUND_ANIMATING:
        if (sound_effect == 0) {
            return -1;
        }

        // All objects (scenery mostly) - sound effect is the base number into
        // `snd_misc.mes` (sound ID is at field offset 2).
        sound_id = sound_effect + MISC_SOUND_ANIMATING;
        break;
    }

    return sound_id;
}
