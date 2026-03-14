#include "game/fate.h"

#include "game/critter.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/stat.h"

static void fate_apply(int64_t obj, int index);

/**
 * Defines bit flags for each fate.
 *
 * The immediate fates are supposed to have 0, they are not stored and applied
 * immediately with `fate_apply`. The activated deferred fates are stored in
 * a single field (`OBJ_F_PC_FLAGS_FATE`) as a bit mask.
 *
 * 0x5BC48C
 */
static unsigned int fate_flags[FATE_COUNT] = {
    /*                 FATE_FULL_HEAL */ 0x0000,
    /*       FATE_FORCE_GOOD_REACTION */ 0x0001,
    /*                  FATE_CRIT_HIT */ 0x0002,
    /*                 FATE_CRIT_MISS */ 0x0004,
    /*       FATE_SAVE_AGAINST_MAGICK */ 0x0008,
    /*          FATE_SPELL_AT_MAXIMUM */ 0x0010,
    /*     FATE_CRIT_SUCCESS_GAMBLING */ 0x0020,
    /*         FATE_CRIT_SUCCESS_HEAL */ 0x0040,
    /*  FATE_CRIT_SUCCESS_PICK_POCKET */ 0x0080,
    /*       FATE_CRIT_SUCCESS_REPAIR */ 0x0100,
    /*   FATE_CRIT_SUCCESS_PICK_LOCKS */ 0x0200,
    /* FATE_CRIT_SUCCESS_DISARM_TRAPS */ 0x0400,
};

/**
 * Function pointer to update UI.
 *
 * This callback is called when fate is successfully applied/resolved.
 *
 * 0x603D48
 */
static FateCallback* fate_callback;

/**
 * Checks if a specific fate is active for a player character.
 *
 * 0x4F5090
 */
bool fate_is_activated(int64_t obj, int fate)
{
    return (obj_field_int32_get(obj, OBJ_F_PC_FLAGS_FATE) & fate_flags[fate]) != 0;
}

/**
 * Activates a fate for a player character.
 *
 * This function consumes one fate point and "enable" the specified fate. The
 * immediate fate (only one exists - `FATE_FULL_HEAL`) is applied immediately.
 * Deferred fate is stored in `OBJ_F_PC_FLAGS_FATE` field until appropriate
 * in-game situation occurs. Until then the deferred fate may be deactivated
 * with `fate_deactivate`.
 *
 * Returns `true` if fate is successfully activated, `false` otherwise.
 *
 * 0x4F50C0
 */
bool fate_activate(int64_t obj, int fate)
{
    unsigned int flags;
    int fate_points;

    flags = obj_field_int32_get(obj, OBJ_F_PC_FLAGS_FATE);

    // Check if the fate is already active.
    if ((fate_flags[fate] & flags) != 0) {
        return false;
    }

    // Check if there are unspent fate points.
    fate_points = stat_base_get(obj, STAT_FATE_POINTS);
    if (fate_points <= 0) {
        return false;
    }

    // Consume one fate point.
    stat_base_set(obj, STAT_FATE_POINTS, fate_points - 1);

    if (fate_flags[fate] != 0) {
        // Update fate's bits.
        obj_field_int32_set(obj, OBJ_F_PC_FLAGS_FATE, flags | fate_flags[fate]);
    } else {
        // Special case - apply fate immediately.
        fate_apply(obj, fate);
    }

    return true;
}

/**
 * Deactivates a fate for a player character, refunding a fate point.
 *
 * Returns `true` if fate is successfully deactivated, `false` otherwise.
 *
 * 0x4F51B0
 */
bool fate_deactivate(int64_t obj, int fate)
{
    unsigned int flags;
    int fate_points;

    flags = obj_field_int32_get(obj, OBJ_F_PC_FLAGS_FATE);

    // Check if fate is active.
    if ((fate_flags[fate] & flags) == 0) {
        return false;
    }

    // Clear the fate's bits.
    obj_field_int32_set(obj, OBJ_F_PC_FLAGS_FATE, flags & ~fate_flags[fate]);

    // Refund one fate point.
    fate_points = stat_base_get(obj, STAT_FATE_POINTS);
    stat_base_set(obj, STAT_FATE_POINTS, fate_points + 1);

    return true;
}

/**
 * Resolves a fate for a player character.
 *
 * 0x4F5270
 */
bool fate_resolve(int64_t obj, int fate)
{
    unsigned int flags;

    flags = obj_field_int32_get(obj, OBJ_F_PC_FLAGS_FATE);

    // Check if the fate is active.
    if ((flags & fate_flags[fate]) == 0) {
        return false;
    }

    // Clear the fate's bits.
    flags &= ~fate_flags[fate];
    obj_field_int32_set(obj, OBJ_F_PC_FLAGS_FATE, flags);

    // Update UI.
    if (fate_callback != NULL) {
        fate_callback(obj, fate);
    }

    return true;
}

/**
 * Applies an immediate fate effect.
 *
 * There is only one immediate fate - `FATE_FULL_HEAL`. All other fates are
 * deferred and are handled with calls to `fate_resolve` when appropriate
 * in-game situation occurs.
 *
 * 0x4F52D0
 */
void fate_apply(int64_t obj, int fate)
{
    if (fate == FATE_FULL_HEAL) {
        // Restore the object's hit points and fatigue points, remove poison.
        object_hp_damage_set(obj, 0);
        stat_base_set(obj, STAT_POISON_LEVEL, 0);
        critter_fatigue_damage_set(obj, 0);
    }

    // Update UI.
    if (fate_callback != NULL) {
        fate_callback(obj, fate);
    }
}

/**
 * Sets the callback function.
 *
 * 0x4F5320
 */
void fate_set_callback(FateCallback* cb)
{
    fate_callback = cb;
}
