#include "game/mt_item.h"

#include "game/item.h"
#include "game/magictech.h"
#include "game/obj.h"
#include "game/random.h"

static void handle_item_event(int64_t item_obj, int64_t parent_obj, int64_t extra_obj, unsigned int flags);
static void handle_item_event_func(int64_t item_obj, int64_t parent_obj, int64_t extra_obj, int64_t target_loc, unsigned int flags);
static void handle_item_unwear_drop(int64_t item_obj, int64_t parent_obj, unsigned int flags);

/**
 * 0x5B72AC
 */
const char* mt_item_trig_keys[MT_ITEM_TRIG_COUNT] = {
    /*                         MT_ITEM_TRIG_NONE */ "None",
    /*                MT_ITEM_TRIG_USER_ACTIVATE */ "User_Activate",
    /*                         MT_ITEM_TRIG_WEAR */ "Wear",
    /*                       MT_ITEM_TRIG_UNWEAR */ "Unwear",
    /*                       MT_ITEM_TRIG_PICKUP */ "Pickup",
    /*                         MT_ITEM_TRIG_DROP */ "Drop",
    /*                   MT_ITEM_TRIG_PARENT_HIT */ "Parent_Hit",
    /*               MT_ITEM_TRIG_PARENT_STUNNED */ "Parent_Stunned",
    /*     MT_ITEM_TRIG_PARENT_GOING_UNCONSCIOUS */ "Parent_Going_Unconscious",
    /*                 MT_ITEM_TRIG_PARENT_DYING */ "Parent_Dying",
    /*      MT_ITEM_TRIG_PARENT_HIT_BY_ATK_SPELL */ "Parent_Hit_By_Atk_Spell",
    /*         MT_ITEM_TRIG_PARENT_ATKS_OPPONENT */ "Parent_Atks_Opponent",
    /*         MT_ITEM_TRIG_PARENT_ATKS_LOCATION */ "Parent_Atks_Location",
    /*         MT_ITEM_TRIG_PARENT_DMGS_OPPONENT */ "Parent_Dmgs_Opponent",
    /*  MT_ITEM_TRIG_PARENT_DMGS_OPPONENT_W_ITEM */ "Parent_Dmgs_Opponent_W_Item",
    /*                   MT_ITEM_TRIG_TARGET_HIT */ "Target_Hit",
    /*     MT_ITEM_TRIG_TARGET_GOING_UNCONSCIOUS */ "Target_Going_Unconscious",
    /*                    MT_ITEM_TRIG_ITEM_USED */ "Item_Used",
    /*              MT_ITEM_TRIG_TARGET_ATTACKER */ "Target_Attacker",
    /*       MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON */ "Target_Attacker_Weapon",
    /*        MT_ITEM_TRIG_TARGET_ATTACKER_ARMOR */ "Target_Attacker_Armor",
    /* MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON_MELEE */ "Target_Attacker_Weapon_Melee",
    /*           MT_ITEM_TRIG_RANDOM_CHANCE_RARE */ "Random_Chance_Rare",
    /*       MT_ITEM_TRIG_RANDOM_CHANCE_UNCOMMON */ "Random_Chance_Uncommon",
    /*         MT_ITEM_TRIG_RANDOM_CHANCE_COMMON */ "Random_Chance_Common",
    /*       MT_ITEM_TRIG_RANDOM_CHANCE_FREQUENT */ "Random_Chance_Frequent",
};

/**
 * 0x5B7314
 */
unsigned int mt_item_trig_values[MT_ITEM_TRIG_COUNT] = {
    MT_ITEM_TRIG_NONE,
    MT_ITEM_TRIG_USER_ACTIVATE,
    MT_ITEM_TRIG_WEAR,
    MT_ITEM_TRIG_UNWEAR,
    MT_ITEM_TRIG_PICKUP,
    MT_ITEM_TRIG_DROP,
    MT_ITEM_TRIG_PARENT_HIT,
    MT_ITEM_TRIG_PARENT_STUNNED,
    MT_ITEM_TRIG_PARENT_GOING_UNCONSCIOUS,
    MT_ITEM_TRIG_PARENT_DYING,
    MT_ITEM_TRIG_PARENT_HIT_BY_ATK_SPELL,
    MT_ITEM_TRIG_PARENT_ATKS_OPPONENT,
    MT_ITEM_TRIG_PARENT_ATKS_LOCATION,
    MT_ITEM_TRIG_PARENT_DMGS_OPPONENT,
    MT_ITEM_TRIG_PARENT_DMGS_OPPONENT_W_ITEM,
    MT_ITEM_TRIG_TARGET_HIT,
    MT_ITEM_TRIG_TARGET_GOING_UNCONSCIOUS,
    MT_ITEM_TRIG_ITEM_USED | MT_ITEM_TRIG_TARGET_ATTACKER, // NOTE: Strange case.
    MT_ITEM_TRIG_TARGET_ATTACKER,
    MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON,
    MT_ITEM_TRIG_TARGET_ATTACKER_ARMOR,
    MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON_MELEE,
    MT_ITEM_TRIG_RANDOM_CHANCE_RARE,
    MT_ITEM_TRIG_RANDOM_CHANCE_UNCOMMON,
    MT_ITEM_TRIG_RANDOM_CHANCE_COMMON,
    MT_ITEM_TRIG_RANDOM_CHANCE_FREQUENT,
};

/**
 * Flag indicating whether the mt item module has been initialized.
 *
 * 0x5FF610
 */
static bool mt_item_initialized;

/**
 * 0x5FF618
 */
static int64_t store_attacker_obj;

/**
 * Called when the game is initialized.
 *
 * 0x4CB720
 */
bool mt_item_init(GameInitInfo* init_info)
{
    (void)init_info;

    mt_item_initialized = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4CB730
 */
void mt_item_exit(void)
{
    if (mt_item_initialized) {
        mt_item_initialized = false;
    }
}

/**
 * Retrieves the trigger flags for a given spell.
 *
 * 0x4CB760
 */
unsigned int mt_item_triggers(int spell)
{
    return spell != 10000 ? magictech_spells[spell].item_triggers : 0;
}

/**
 * Converts item spell to magictech spell.
 *
 * NOTE: The only case where this function is relevant is when converting the
 * special value of `10000`, which seems to indicate no particular spell.
 * However, since all items can be thrown, it is remapped to `16`
 * (`SPELL_STONE_THROW`).
 *
 * 0x4CB790
 */
int mt_item_spell_to_magictech_spell(int spell)
{
    return spell & 0xFF;
}

/**
 * Retrieves the magictech spell for a given item at a specific slot.
 *
 * The `slot` is implied to be in range 0-4, but this is not enforced.
 *
 * 0x4CB7A0
 */
int mt_item_spell(int64_t item_obj, int slot)
{
    int spell;

    if (item_obj == OBJ_HANDLE_NULL) {
        return -1;
    }

    spell = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_1 + slot);
    if (spell == 10000) {
        return -1;
    }

    return spell;
}

/**
 * Notifies the system that an item has been equipped.
 *
 * 0x4CB7D0
 */
void mt_item_notify_wear(int64_t item_obj, int64_t parent_obj)
{
    handle_item_event(item_obj, parent_obj, OBJ_HANDLE_NULL, MT_ITEM_TRIG_WEAR);
}

/**
 * Shortcut for `handle_item_event_func`, specifying no target location.
 *
 * 0x4CB800
 */
void handle_item_event(int64_t item_obj, int64_t parent_obj, int64_t extra_obj, unsigned int flags)
{
    handle_item_event_func(item_obj, parent_obj, extra_obj, 0, flags);
}

/**
 * Processes item triggers.
 *
 * 0x4CB830
 */
void handle_item_event_func(int64_t item_obj, int64_t parent_obj, int64_t extra_obj, int64_t target_loc, unsigned int flags)
{
    int slot;
    int spell;
    unsigned int trig;
    MagicTechInvocation mt_invocation;

    for (slot = 0; slot < 5; slot++) {
        spell = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_1 + slot);
        if (spell == 10000) {
            break;
        }

        trig = mt_item_triggers(spell);

        // Check if the spell's triggers match the event flags.
        if ((trig & flags) == 0) {
            continue;
        }

        // Check random chance triggers.
        if ((trig & MT_ITEM_TRIG_RANDOM_CHANCE_ANY) != 0) {
            if ((trig & MT_ITEM_TRIG_RANDOM_CHANCE_RARE) != 0) {
                // Rare - 10%.
                if (random_between(1, 100) > 10) {
                    continue;
                }
            } else if ((trig & MT_ITEM_TRIG_RANDOM_CHANCE_UNCOMMON) != 0) {
                // Uncommon - 20%.
                if (random_between(1, 100) > 20) {
                    continue;
                }
            } else if ((trig & MT_ITEM_TRIG_RANDOM_CHANCE_COMMON) != 0) {
                // Common - 40%.
                if (random_between(1, 100) > 40) {
                    continue;
                }
            } else if ((trig & MT_ITEM_TRIG_RANDOM_CHANCE_FREQUENT) != 0) {
                // Frequent - 66%.
                if (random_between(1, 100) > 66) {
                    continue;
                }
            }
        }

        magictech_invocation_init(&mt_invocation, item_obj, mt_item_spell_to_magictech_spell(spell));

        if ((trig & MT_ITEM_TRIG_TARGET_ATTACKER_ANY) != 0) {
            if ((trig & MT_ITEM_TRIG_TARGET_ATTACKER) != 0) {
                sub_4440E0(extra_obj, &(mt_invocation.target_obj));
            } else if ((trig & MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON) != 0) {
                if (extra_obj != OBJ_HANDLE_NULL) {
                    sub_4440E0(item_wield_get(extra_obj, ITEM_INV_LOC_WEAPON), &(mt_invocation.target_obj));
                } else {
                    sub_4440E0(OBJ_HANDLE_NULL, &(mt_invocation.target_obj));
                }
            } else if ((trig & MT_ITEM_TRIG_TARGET_ATTACKER_ARMOR) != 0) {
                if (extra_obj != OBJ_HANDLE_NULL) {
                    sub_4440E0(item_wield_get(extra_obj, ITEM_INV_LOC_ARMOR), &(mt_invocation.target_obj));
                } else {
                    sub_4440E0(OBJ_HANDLE_NULL, &(mt_invocation.target_obj));
                }
            } else {
                sub_4440E0(OBJ_HANDLE_NULL, &(mt_invocation.target_obj));
            }
        } else if ((trig & MT_ITEM_TRIG_PARENT_ATKS_LOCATION) != 0) {
            mt_invocation.target_loc = target_loc;
        } else {
            sub_4440E0(parent_obj, &(mt_invocation.target_obj));
        }

        mt_invocation.trigger = flags;
        mt_invocation.flags |= MAGICTECH_INVOCATION_FRIENDLY;

        if (store_attacker_obj != OBJ_HANDLE_NULL) {
            sub_4440E0(store_attacker_obj, &(mt_invocation.attacker_obj));
        }

        if (mt_invocation.target_obj.obj != OBJ_HANDLE_NULL
            || mt_invocation.target_loc != 0) {
            magictech_invocation_run(&mt_invocation);
        }
    }

    store_attacker_obj = OBJ_HANDLE_NULL;
}

/**
 * Notifies the system that an item has been picked up.
 *
 * 0x4CBAA0
 */
void mt_item_notify_pickup(int64_t item_obj, int64_t parent_obj)
{
    handle_item_event(item_obj, parent_obj, OBJ_HANDLE_NULL, MT_ITEM_TRIG_PICKUP);
}

/**
 * Notifies the system that the parent object has been hit.
 *
 * 0x4CBAD0
 */
void mt_item_notify_parent_hit(int64_t attacker_obj, CombatContext* combat, int64_t target_obj)
{
    unsigned int flags;
    int type;
    int index;
    int64_t item_obj;

    if (target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    flags = MT_ITEM_TRIG_PARENT_HIT;

    // Add melee weapon event.
    if ((combat->flags & 0x40000) != 0
        && (combat->weapon_obj == OBJ_HANDLE_NULL
            || item_weapon_range(combat->weapon_obj, attacker_obj) <= 1)) {
        flags |= MT_ITEM_TRIG_TARGET_ATTACKER_WEAPON_MELEE;
    }

    // Process only if the target is a critter.
    type = obj_field_int32_get(target_obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(type)) {
        return;
    }

    // Iterate through equipment slots and fire an event.
    for (index = 0; index < 9; index++) {
        item_obj = item_wield_get(target_obj, 1000 + index);
        if (item_obj != OBJ_HANDLE_NULL) {
            handle_item_event(item_obj, target_obj, attacker_obj, flags);
        }
    }
}

/**
 * Notifies the system that the parent object has been stunned.
 *
 * 0x4CBB80
 */
void mt_item_notify_parent_stunned(int64_t attacker_obj, int64_t target_obj)
{
    int type;
    int index;
    int64_t item_obj;

    if (target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Process only if the target is a critter.
    type = obj_field_int32_get(target_obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(type)) {
        return;
    }

    // Iterate through equipment slots and fire an event.
    for (index = 0; index < 9; index++) {
        item_obj = item_wield_get(target_obj, 1000 + index);
        if (item_obj != OBJ_HANDLE_NULL) {
            handle_item_event(item_obj, target_obj, attacker_obj, MT_ITEM_TRIG_PARENT_STUNNED);
        }
    }
}

/**
 * Notifies the system that the parent object is going unconscious.
 *
 * 0x4CBBF0
 */
void mt_item_notify_parent_going_unconscious(int64_t attacker_obj, int64_t target_obj)
{
    int type;
    int index;
    int64_t item_obj;

    if (target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Process only if the target is a critter.
    type = obj_field_int32_get(target_obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(type)) {
        return;
    }

    // Iterate through equipment slots and fire an event.
    for (index = 0; index < 9; index++) {
        item_obj = item_wield_get(target_obj, 1000 + index);
        if (item_obj != OBJ_HANDLE_NULL) {
            handle_item_event(item_obj, target_obj, attacker_obj, MT_ITEM_TRIG_PARENT_GOING_UNCONSCIOUS);
        }
    }
}

/**
 * Notifies the system that the parent object has died.
 *
 * 0x4CBC60
 */
void mt_item_notify_parent_dying(int64_t attacker_obj, int64_t target_obj)
{
    int type;
    int index;
    int64_t item_obj;

    // Process only if the target is a critter.
    type = obj_field_int32_get(target_obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(type)) {
        return;
    }

    // Iterate through equipment slots and fire an event.
    for (index = 0; index < 9; index++) {
        item_obj = item_wield_get(target_obj, 1000 + index);
        if (item_obj != OBJ_HANDLE_NULL) {
            handle_item_event(item_obj, target_obj, attacker_obj, MT_ITEM_TRIG_PARENT_DYING);
        }
    }
}

/**
 * Notifies the system that the parent object is attacking another object.
 *
 * 0x4CBD40
 */
void mt_item_notify_parent_attacks_obj(int64_t attacker_obj, int64_t target_obj)
{
    int type;
    int index;
    int64_t item_obj;

    // Process only if the attacker is a critter.
    type = obj_field_int32_get(attacker_obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(type)) {
        return;
    }

    // Iterate through equipment slots and fire an event.
    store_attacker_obj = attacker_obj;
    for (index = 0; index < 9; index++) {
        item_obj = item_wield_get(attacker_obj, 1000 + index);
        if (item_obj != OBJ_HANDLE_NULL) {
            handle_item_event(item_obj, attacker_obj, target_obj, MT_ITEM_TRIG_PARENT_ATKS_OPPONENT);
        }
    }
}

/**
 * Notifies the system that the parent object is attacking a location.
 *
 * 0x4CBDB0
 */
void mt_item_notify_parent_attacks_loc(int64_t attacker_obj, int64_t weapon_obj, int64_t target_loc)
{
    if (weapon_obj == OBJ_HANDLE_NULL) {
        return;
    }

    store_attacker_obj = attacker_obj;
    handle_item_event_func(weapon_obj, attacker_obj, OBJ_HANDLE_NULL, target_loc, MT_ITEM_TRIG_PARENT_ATKS_LOCATION);
}

/**
 * Notifies the system that the target object is going unconscious.
 *
 * 0x4CBE00
 */
void mt_item_notify_target_going_unconscious(int64_t attacker_obj, int64_t target_obj)
{
    int type;
    int index;
    int64_t item_obj;

    if (attacker_obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Process only if the attacker is a critter.
    type = obj_field_int32_get(attacker_obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(type)) {
        return;
    }

    // Iterate through equipment slots and fire an event.
    for (index = 0; index < 9; index++) {
        item_obj = item_wield_get(attacker_obj, 1000 + index);
        if (item_obj != OBJ_HANDLE_NULL) {
            handle_item_event(item_obj, attacker_obj, target_obj, MT_ITEM_TRIG_TARGET_GOING_UNCONSCIOUS);
        }
    }
}

/**
 * Notifies the system that the parent object has damaged another object.
 *
 * 0x4CBE70
 */
void mt_item_notify_parent_dmgs_obj(int64_t attacker_obj, int64_t weapon_obj, int64_t target_obj)
{
    int type;
    int index;
    int64_t item_obj;

    type = obj_field_int32_get(attacker_obj, OBJ_F_TYPE);
    if (obj_type_is_critter(type)) {
        // Iterate through equipment slots and fire an event.
        for (index = 0; index < 9; index++) {
            item_obj = item_wield_get(attacker_obj, 1000 + index);
            if (item_obj != OBJ_HANDLE_NULL) {
                handle_item_event(item_obj, attacker_obj, target_obj, MT_ITEM_TRIG_PARENT_DMGS_OPPONENT);
                handle_item_event(item_obj, attacker_obj, target_obj, MT_ITEM_TRIG_TARGET_HIT);
                if (item_obj == weapon_obj) {
                    handle_item_event(item_obj, attacker_obj, target_obj, MT_ITEM_TRIG_PARENT_DMGS_OPPONENT_W_ITEM);
                }
            }
        }
    } else if (obj_type_is_item(type)) {
        handle_item_event(attacker_obj, target_obj, attacker_obj, MT_ITEM_TRIG_TARGET_HIT);
    }
}

/**
 * Notifies the system that an item has been used.
 *
 * 0x4CBF70
 */
void mt_item_notify_used(int64_t item_obj, int64_t target_obj)
{
    int type;
    int64_t parent_obj;

    if (item_obj == OBJ_HANDLE_NULL) {
        return;
    }

    type = obj_field_int32_get(item_obj, OBJ_F_TYPE);
    if (obj_type_is_item(type)) {
        parent_obj = obj_field_handle_get(item_obj, OBJ_F_ITEM_PARENT);
        handle_item_event(item_obj, parent_obj, target_obj, MT_ITEM_TRIG_ITEM_USED);
    }
}

/**
 * Notifies the system that an item has been unequipped.
 *
 * 0x4CBFC0
 */
void mt_item_notify_unwear(int64_t item_obj, int64_t parent_obj)
{
    handle_item_unwear_drop(item_obj, parent_obj, MT_ITEM_TRIG_UNWEAR);
}

/**
 * Internal helper to process unwear or drop triggers for an item.
 *
 * 0x4CBFF0
 */
void handle_item_unwear_drop(int64_t item_obj, int64_t parent_obj, unsigned int flags)
{
    int slot;
    int spell;
    MagicTechInvocation mt_invocation;

    if (item_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (parent_obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Iterate through spell slots.
    for (slot = 0; slot < 5; slot++) {
        spell = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_1 + slot);
        if (spell == 10000) {
            break;
        }

        if ((mt_item_triggers(spell) & flags) == 0) {
            continue;
        }

        // Remove current effects.
        magictech_invocation_init(&mt_invocation, item_obj, mt_item_spell_to_magictech_spell(spell));
        sub_4440E0(parent_obj, &(mt_invocation.target_obj));
        sub_4573D0(&mt_invocation);

        // Add new effects.
        magictech_invocation_init(&mt_invocation, item_obj, mt_item_spell_to_magictech_spell(spell));
        sub_4440E0(parent_obj, &(mt_invocation.target_obj));
        mt_invocation.trigger = flags;
        if (mt_invocation.target_obj.obj != OBJ_HANDLE_NULL) {
            magictech_invocation_run(&mt_invocation);
        }
    }
}

/**
 * Notifies the system that an item has been dropped.
 *
 * 0x4CC130
 */
void mt_item_notify_drop(int64_t item_obj, int64_t parent_obj)
{
    handle_item_unwear_drop(item_obj, parent_obj, MT_ITEM_TRIG_DROP);
}

/**
 * Determines if an item is valid for AI actions.
 *
 * NOTE: The code looks unclear and likely contains remnants from earlier
 * implementations.
 *
 * 0x4CC160
 */
bool mt_item_valid_ai_action(int64_t item_obj)
{
    int spell;

    // FIXME: Unused, probably should validate if item_obj is of item type.
    obj_field_int32_get(item_obj, OBJ_F_TYPE);

    if (obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_MANA_STORE) == 0) {
        return false;
    }

    spell = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_1);
    if (spell == 10000) {
        return true;
    }

    if (magictech_spells[spell].item_triggers != 0) {
        return true;
    }

    if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_VALID_AI_ACTION) == 0) {
        return false;
    }

    // FIXME: Unused.
    obj_field_int32_get(item_obj, OBJ_F_ITEM_AI_ACTION);

    return true;
}
