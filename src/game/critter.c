#include "game/critter.h"

#include "game/ai.h"
#include "game/anim.h"
#include "game/background.h"
#include "game/combat.h"
#include "game/effect.h"
#include "game/gamelib.h"
#include "game/item.h"
#include "game/light.h"
#include "game/logbook.h"
#include "game/magictech.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/monstergen.h"
#include "game/mp_utils.h"
#include "game/mt_item.h"
#include "game/multiplayer.h"
#include "game/object.h"
#include "game/object_node.h"
#include "game/party.h"
#include "game/player.h"
#include "game/random.h"
#include "game/reaction.h"
#include "game/reputation.h"
#include "game/script.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/tf.h"
#include "game/ui.h"

typedef enum FatigueEventType {
    FATIGUE_EVENT_RECOVERY,
    FATIGUE_EVENT_DAMAGE,
} FatigueEventType;

static void critter_disband_internal(int64_t obj);
static bool fatigue_timeevent_check(TimeEvent* timeevent);
static bool resting_timeevent_check(TimeEvent* timeevent);
static bool decay_timeevent_check(TimeEvent* timeevent);
static void critter_set_concealed_internal(int64_t obj, bool concealed);

/**
 * 0x5B304C
 */
static int critter_test_fatigue_type = -1;

/**
 * Defines fatigue damage timeevent delay (in milliseconds) per encumbrance
 * level.
 *
 * 0x5B3050
 */
static int fatigue_damage_delay_tbl[MAX_ENCUMBRANCE_LEVEL] = {
    /*        ENCUMBRANCE_LEVEL_NONE */ 0,
    /*       ENCUMBRANCE_LEVEL_LIGHT */ 0,
    /*    ENCUMBRANCE_LEVEL_MODERATE */ 0,
    /*      ENCUMBRANCE_LEVEL_MEDIUM */ 0,
    /* ENCUMBRANCE_LEVEL_SIGNIFICANT */ 600000,
    /*       ENCUMBRANCE_LEVEL_HEAVY */ 60000,
    /*      ENCUMBRANCE_LEVEL_SEVERE */ 5000,
};

/**
 * Defines weight ratio (in percent from the maximum carry weight) per
 * encumbrance level.
 *
 * 0x5B306C
 */
static int encumbrance_level_ratio_tbl[MAX_ENCUMBRANCE_LEVEL] = {
    /*        ENCUMBRANCE_LEVEL_NONE */ 15,
    /*       ENCUMBRANCE_LEVEL_LIGHT */ 30,
    /*    ENCUMBRANCE_LEVEL_MODERATE */ 45,
    /*      ENCUMBRANCE_LEVEL_MEDIUM */ 60,
    /* ENCUMBRANCE_LEVEL_SIGNIFICANT */ 75,
    /*       ENCUMBRANCE_LEVEL_HEAVY */ 90,
    /*      ENCUMBRANCE_LEVEL_SEVERE */ 100,
};

/**
 * "xp_critter.mes"
 *
 * 0x5E8630
 */
static mes_file_handle_t xp_mes_file;

/**
 * 0x5E8634
 */
static char** social_class_names;

/**
 * 0x5E8638
 */
static int critter_encumbrance_recalc_feedback_cnt;

/**
 * Flag indicating whether the critter system is in editor mode.
 *
 * 0x5E863C
 */
static bool critter_editor;

/**
 * "critter.mes"
 *
 * 0x5E8640
 */
static mes_file_handle_t critter_mes_file;

/**
 * 0x5E8648
 */
static int64_t critter_test_obj;

/**
 * 0x5E8650
 */
static int64_t critter_decay_test_obj;

/**
 * Called when the game is initialized.
 *
 * 0x45CF30
 */
bool critter_init(GameInitInfo* init_info)
{
    int index;
    MesFileEntry mes_file_entry;

    critter_editor = init_info->editor;

    // Load experience value table (required).
    if (!mes_load("Rules\\xp_critter.mes", &xp_mes_file)) {
        return false;
    }

    // Load UI messages (required).
    if (!mes_load("mes\\critter.mes", &critter_mes_file)) {
        return false;
    }

    // Allocate and populate social class names array.
    social_class_names = (char**)CALLOC(MAX_SOCIAL_CLASS, sizeof(char*));

    for (index = 0; index < MAX_SOCIAL_CLASS; index++) {
        mes_file_entry.num = index;
        mes_get_msg(critter_mes_file, &mes_file_entry);
        social_class_names[index] = mes_file_entry.str;
    }

    // Enable encumbrance feedback.
    critter_encumbrance_recalc_feedback_cnt = 1;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x45CFE0
 */
void critter_exit()
{
    FREE(social_class_names);
    mes_unload(critter_mes_file);
    mes_unload(xp_mes_file);
}

/**
 * Retrieves the social class of a critter.
 *
 * 0x45D010
 */
int critter_social_class_get(int64_t obj)
{
    return obj_field_int32_get(obj, OBJ_F_NPC_SOCIAL_CLASS);
}

/**
 * Sets the social class of a critter.
 *
 * 0x45D030
 */
int critter_social_class_set(int64_t obj, int value)
{
    // Remove existing effect caused by old social class.
    effect_remove_one_caused_by(obj, EFFECT_CAUSE_CLASS);

    obj_field_int32_set(obj, OBJ_F_NPC_SOCIAL_CLASS, value);

    // Add effect from the new social class.
    effect_add(obj, EFFECT_CLASS_SPECIFIC + value, EFFECT_CAUSE_CLASS);

    return value;
}

/**
 * Retrieves the name of a social class.
 *
 * 0x45D070
 */
const char* critter_social_class_name(int social_class)
{
    return social_class_names[social_class];
}

/**
 * Retrieves the faction of a critter.
 *
 * 0x45D080
 */
int critter_faction_get(int64_t obj)
{
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        return obj_field_int32_get(obj, OBJ_F_NPC_FACTION);
    } else {
        return 0;
    }
}

/**
 * Sets the faction of an NPC critter.
 *
 * 0x45D0C0
 */
int critter_faction_set(int64_t obj, int value)
{
    if (obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        obj_field_int32_set(obj, OBJ_F_NPC_FACTION, value);
    }

    return value;
}

/**
 * Checks if two critters belong to the same faction or group.
 *
 * 0x45D110
 */
bool critter_faction_same(int64_t obj1, int64_t obj2)
{
    int obj_type1;
    int obj_type2;
    int faction;
    int64_t pc_leader;
    int64_t leader_obj1;
    int64_t leader_obj2;

    obj_type1 = obj_field_int32_get(obj1, OBJ_F_TYPE);
    obj_type2 = obj_field_int32_get(obj2, OBJ_F_TYPE);

    // Special case - first object is a PC.
    if (obj_type1 == OBJ_TYPE_PC) {
        if (obj_type2 == OBJ_TYPE_PC) {
            // PCs are not considered in same faction.
            return false;
        }

        // Check if obj1 is the PC leader of obj2.
        if (critter_pc_leader_get(obj2) == obj1) {
            return true;
        }

        faction = critter_faction_get(obj2);
        if (faction == 0) {
            // obj2 does not belong to any specific faction.
            return false;
        }

        return reputation_check_faction(obj1, faction);
    }

    // Special case - second object is a PC.
    if (obj_type2 == OBJ_TYPE_PC) {
        if (critter_pc_leader_get(obj1) == obj2) {
            return true;
        }

        faction = critter_faction_get(obj1);
        if (faction == 0) {
            return false;
        }

        return reputation_check_faction(obj2, faction);
    }

    // Check if both critters follow the same PC.
    pc_leader = critter_pc_leader_get(obj1);
    if (pc_leader != OBJ_HANDLE_NULL && pc_leader == critter_pc_leader_get(obj2)) {
        return true;
    }

    // Check if one is the leader of the other.
    leader_obj1 = critter_leader_get(obj1);
    if (leader_obj1 == obj2) {
        return true;
    }

    leader_obj2 = critter_leader_get(obj2);
    if (leader_obj2 == obj1) {
        return true;
    }

    // Check if both critters follow the same NPC.
    if (leader_obj1 != OBJ_HANDLE_NULL
        && leader_obj1 == leader_obj2) {
        return true;
    }

    faction = critter_faction_get(obj1);
    if (faction == 0) {
        // obj1 does not belong to any specific faction.
        return false;
    }

    if (faction == critter_faction_get(obj2)) {
        return true;
    }

    return false;
}

/**
 * Retrieves the origin of a critter.
 *
 * 0x45D290
 */
int critter_origin_get(int64_t obj)
{
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        return obj_field_int32_get(obj, OBJ_F_NPC_ORIGIN);
    } else {
        return 0;
    }
}

/**
 * Sets the origin of a critter.
 *
 * 0x45D2D0
 */
int critter_origin_set(int64_t obj, int value)
{
    if (obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        obj_field_int32_set(obj, OBJ_F_NPC_ORIGIN, value);
    }

    return value;
}

/**
 * Checks if two critters share the same origin.
 *
 * 0x45D320
 */
bool critter_origin_same(int64_t a, int64_t b)
{
    int origin = critter_origin_get(a);
    return origin != 0 && origin == critter_origin_get(b);
}

/**
 * Checks if a critter is a player character.
 *
 * 0x45D360
 */
bool critter_is_pc(int64_t obj)
{
    return obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC;
}

/**
 * Retrieves the fatigue points of a critter.
 *
 * 0x45D390
 */
int critter_fatigue_pts_get(int64_t obj)
{
    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    return obj_field_int32_get(obj, OBJ_F_CRITTER_FATIGUE_PTS);
}

/**
 * Sets the fatigue points of a critter.
 *
 * 0x45D3E0
 */
int critter_fatigue_pts_set(int64_t obj, int value)
{
    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    if (!multiplayer_is_locked()) {
        Packet51 pkt;

        pkt.type = 51;
        pkt.field_4 = 1;
        pkt.field_8 = obj_get_id(obj);
        pkt.field_20 = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));

        if (!tig_net_is_host()) {
            return value;
        }
    }

    if (value < 0) {
        value = 0;
    }

    obj_field_int32_set(obj, OBJ_F_CRITTER_FATIGUE_PTS, value);

    // Update the UI.
    ui_refresh_fatigue_bar(obj);

    return value;
}

/**
 * Retrieves the fatigue adjustment of a critter.
 *
 * 0x45D4A0
 */
int critter_fatigue_adj_get(int64_t obj)
{
    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    return obj_field_int32_get(obj, OBJ_F_CRITTER_FATIGUE_ADJ);
}

// FIXME: Wrong name, sets `OBJ_F_CRITTER_FATIGUE_PTS`, not `OBJ_F_CRITTER_FATIGUE_ADJ`.
//
// 0x45D4F0
int critter_fatigue_adj_set(int64_t obj, int value)
{
    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    obj_field_int32_set(obj, OBJ_F_CRITTER_FATIGUE_PTS, value);
    ui_refresh_fatigue_bar(obj);

    return value;
}

/**
 * Retrieves the fatigue damage of a critter.
 *
 * 0x45D550
 */
int critter_fatigue_damage_get(int64_t obj)
{
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    return obj_field_int32_get(obj, OBJ_F_CRITTER_FATIGUE_DAMAGE);
}

/**
 * Sets the fatigue damage of a critter.
 *
 * 0x45D5A0
 */
int critter_fatigue_damage_set(int64_t obj, int value)
{
    bool was_unconscious;

    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    if (value < 0) {
        value = 0;
    }

    // Check if critter was unconscious before setting damage.
    was_unconscious = critter_is_unconscious(obj);

    obj_field_int32_set(obj, OBJ_F_CRITTER_FATIGUE_DAMAGE, value);
    ui_refresh_fatigue_bar(obj);

    if (value != 0 && !obj_is_proto(obj)) {
        // Schedule fatigue recovery every 80 game seconds (10 RL seconds).
        critter_fatigue_timeevent_schedule(obj, FATIGUE_EVENT_RECOVERY, 80000);

        // Handle critter becomes unconscious.
        if (!was_unconscious && critter_is_unconscious(obj)) {
            magictech_demaintain_spells(obj);
            mt_item_notify_parent_going_unconscious(OBJ_HANDLE_NULL, obj);
            anim_goal_knockdown(obj);
            combat_turn_based_end_critter_turn(obj);
        }
    }

    return value;
}

/**
 * Calculates the maximum fatigue a critter can have.
 *
 * 0x45D670
 */
int critter_fatigue_max(int64_t obj)
{
    int fatigue_pts;
    int fatigue_adj;
    int level;
    int constitution;
    int willpower;

    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    // Gather stats for fatigue calculation.
    fatigue_pts = critter_fatigue_pts_get(obj);
    fatigue_adj = critter_fatigue_adj_get(obj);
    level = stat_level_get(obj, STAT_LEVEL);
    constitution = stat_level_get(obj, STAT_CONSTITUTION);
    willpower = stat_level_get(obj, STAT_WILLPOWER);

    // PTS * 4 + ADJ + 2 * (LVL + CN) + WP + 4
    return effect_adjust_max_fatigue(obj, fatigue_pts * 4 + fatigue_adj + 2 * (level + constitution) + willpower + 4);
}

/**
 * Calculates the current fatigue of a critter.
 *
 * 0x45D700
 */
int critter_fatigue_current(int64_t obj)
{
    return critter_fatigue_max(obj) - critter_fatigue_damage_get(obj);
}

/**
 * Checks if a critter is prone (knocked down).
 *
 * 0x45D730
 */
bool critter_is_prone(int64_t obj)
{
    tig_art_id_t aid;

    // Check if critter is valid and not destroyed/off.
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & (OF_DESTROYED | OF_OFF)) != 0) {
        return false;
    }

    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return false;
    }

    // Check if the current animation is fall down.
    aid = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    if (tig_art_id_anim_get(aid) != TIG_ART_ANIM_FALL_DOWN) {
        return false;
    }

    return true;
}

/**
 * Checks if a critter is active (not dead, unconscious, stunned, paralyzed, or
 * stoned).
 *
 * 0x45D790
 */
bool critter_is_active(int64_t obj)
{
    return (obj_field_int32_get(obj, OBJ_F_FLAGS) & (OF_DESTROYED | OF_OFF)) == 0
        && !critter_is_dead(obj)
        && !critter_is_unconscious(obj)
        && (obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & (OCF_STUNNED | OCF_PARALYZED)) == 0
        && (obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & OSF_STONED) == 0;
}

/**
 * Checks if a critter is unconscious.
 *
 * 0x45D800
 */
bool critter_is_unconscious(int64_t obj)
{
    return obj == OBJ_HANDLE_NULL
        || !obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))
        || ((obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & OCF_UNDEAD) == 0
            && critter_fatigue_current(obj) <= 0);
}

/**
 * Checks if a critter is sleeping.
 *
 * 0x45D870
 */
bool critter_is_sleeping(int64_t obj)
{
    return obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))
        && (obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & OCF_SLEEPING) != 0;
}

/**
 * Checks if a critter is dead.
 *
 * 0x45D8D0
 */
bool critter_is_dead(int64_t obj)
{
    if (obj != OBJ_HANDLE_NULL) {
        return object_hp_current(obj) <= 0;
    } else {
        return true;
    }
}

/**
 * Kills a critter by dealing 32000 pts of damage.
 *
 * 0x45D900
 */
void critter_kill(int64_t obj)
{
    CombatContext combat;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (!multiplayer_is_locked()) {
        Packet104 pkt;

        if (!tig_net_is_active()) {
            return;
        }

        pkt.type = 104;
        pkt.oid = obj_get_id(obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    multiplayer_lock();

    sub_4B2210(OBJ_HANDLE_NULL, obj, &combat);
    combat.dam_flags |= CDF_DEATH;
    combat_dmg(&combat);

    object_hp_damage_set(obj, 32000);

    // Handle editor mode specific actions.
    if (critter_editor) {
        tig_art_id_t art_id;
        TigArtAnimData art_anim_data;

        art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        art_id = tig_art_id_anim_set(art_id, TIG_ART_ANIM_FALL_DOWN);
        if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
            art_id = tig_art_id_frame_set(art_id, art_anim_data.num_frames - 1);
        }
        object_set_current_aid(obj, art_id);
        object_flags_set(obj, OF_FLAT | OF_NO_BLOCK);
        sub_432D90(obj);
    }

    multiplayer_unlock();
}

/**
 * Notifies the system when a critter is killed.
 *
 * 0x45DA20
 */
void critter_notify_killed(int64_t victim_obj, int64_t killer_obj, int anim)
{
    int64_t leader_obj;
    int64_t pc_killer_obj;
    tig_art_id_t art_id;

    if (victim_obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Notify text floater system of kill.
    tf_notify_killed(victim_obj);

    // Execute death script.
    if (!object_script_execute(killer_obj, victim_obj, OBJ_HANDLE_NULL, SAP_DYING, 0)) {
        return;
    }

    if ((obj_field_int32_get(victim_obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return;
    }

    combat_critter_deactivate_combat_mode(victim_obj);
    obj_field_int32_set(victim_obj, OBJ_F_CRITTER_DEATH_TIME, datetime_current_second());
    sub_459740(victim_obj);
    combat_recalc_reaction(victim_obj);

    if (obj_field_int32_get(victim_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        critter_npc_combat_focus_wipe_schedule(victim_obj);
        obj_field_handle_set(victim_obj, OBJ_F_NPC_COMBAT_FOCUS, killer_obj);
        ai_notify_killed(victim_obj, killer_obj);

        // Handle PC killing NPC.
        if (killer_obj != OBJ_HANDLE_NULL) {
            if (obj_field_int32_get(killer_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                pc_killer_obj = killer_obj;
            } else {
                pc_killer_obj = critter_pc_leader_get(killer_obj);
            }

            if (pc_killer_obj != OBJ_HANDLE_NULL) {
                ObjectList followers;
                ObjectNode* node;

                // 20% of the experience cost for killing.
                critter_give_xp(pc_killer_obj, 20 * obj_field_int32_get(victim_obj, OBJ_F_NPC_EXPERIENCE_WORTH) / 100);
                obj_field_int32_set(victim_obj, OBJ_F_NPC_EXPERIENCE_WORTH, 0);

                // Adjust alignment.
                sub_45DC90(pc_killer_obj, victim_obj, true);

                // Record kill.
                logbook_add_kill(pc_killer_obj, victim_obj);

                // Notify followers of leader's kill.
                object_list_followers(pc_killer_obj, &followers);
                node = followers.head;
                while (node != NULL) {
                    object_script_execute(victim_obj, node->obj, pc_killer_obj, SAP_LEADER_KILLING, 0);
                    node = node->next;
                }
                object_list_destroy(&followers);
            }
        }

        // Remove victim from the group.
        leader_obj = critter_leader_get(victim_obj);
        if (leader_obj != OBJ_HANDLE_NULL) {
            critter_disband(victim_obj, true);
        }
        critter_leader_set(victim_obj, leader_obj);

        // Notify monstergen system of kill.
        monstergen_notify_killed(victim_obj);
    } else {
        // Move items from hotkey bar to inventory.
        sub_467520(victim_obj);
    }

    // Remove weapon from death animation.
    art_id = obj_field_int32_get(victim_obj, OBJ_F_CURRENT_AID);
    if (tig_art_critter_id_weapon_get(art_id) != TIG_ART_WEAPON_TYPE_NO_WEAPON) {
        art_id = tig_art_critter_id_weapon_set(art_id, TIG_ART_WEAPON_TYPE_NO_WEAPON);
        object_set_current_aid(victim_obj, art_id);
    }

    // Play dying animation.
    anim_goal_dying(victim_obj, anim);

    // Schedule body decay.
    if ((obj_field_int32_get(victim_obj, OBJ_F_CRITTER_FLAGS2) & OCF2_NO_DECAY) == 0) {
        critter_decay_timeevent_schedule(victim_obj);
    }

    object_hp_damage_set(victim_obj, 32000);

    // Notify magictech items worn by the victim that the owner is dying.
    if (killer_obj != OBJ_HANDLE_NULL) {
        mt_item_notify_parent_dying(killer_obj, victim_obj);
    }
}

/**
 * Adjusts alignment based on killing another critter.
 *
 * 0x45DC90
 */
void sub_45DC90(int64_t killer_obj, int64_t victim_obj, bool a3)
{
    int64_t summoner_obj;
    int alignment1;
    int alignment2;
    int adj;
    int v1;
    int alignment;

    if (!sub_459040(victim_obj, OSF_SUMMONED, &summoner_obj) || summoner_obj != killer_obj) {
        alignment2 = stat_level_get(victim_obj, STAT_ALIGNMENT);
        if (alignment2 < 0) {
            alignment1 = stat_level_get(killer_obj, STAT_ALIGNMENT);
            if (alignment1 > alignment2) {
                alignment2 = -alignment2;
                adj = 100 - alignment2;
            } else {
                adj = alignment2 - 100;
            }
        } else {
            alignment2 = -alignment2;
            adj = -100 - alignment2;
        }

        v1 = alignment2 / 20;
        if (!a3) {
            v1 /= 2;
        }

        // TODO: Check.
        if (v1 < 0) {
            alignment = stat_base_get(killer_obj, STAT_ALIGNMENT);
            if (adj >= 0) {
                if (alignment + v1 > adj) {
                    if (alignment > adj) {
                        stat_base_set(killer_obj, STAT_ALIGNMENT, alignment);
                    } else {
                        stat_base_set(killer_obj, STAT_ALIGNMENT, adj);
                    }
                } else {
                    stat_base_set(killer_obj, STAT_ALIGNMENT, alignment + v1);
                }
            } else {
                if (alignment + v1 < adj) {
                    if (alignment < adj) {
                        stat_base_set(killer_obj, STAT_ALIGNMENT, alignment);
                    } else {
                        stat_base_set(killer_obj, STAT_ALIGNMENT, adj);
                    }
                } else {
                    stat_base_set(killer_obj, STAT_ALIGNMENT, alignment + v1);
                }
            }
        }
    }
}

/**
 * Retrieves the PC leader of a critter.
 *
 * 0x45DDA0
 */
int64_t critter_pc_leader_get(int64_t obj)
{
    int64_t leader_obj = obj;

    // Only NPCs have leaders.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return OBJ_HANDLE_NULL;
    }

    // Traverse leader hierarchy until a PC is found.
    do {
        leader_obj = critter_leader_get(leader_obj);
    } while (leader_obj != OBJ_HANDLE_NULL && obj_field_int32_get(leader_obj, OBJ_F_TYPE) != OBJ_TYPE_PC);

    return leader_obj;
}

/**
 * Retrieves the immediate leader of an NPC critter.
 *
 * 0x45DE00
 */
int64_t critter_leader_get(int64_t obj)
{
    int64_t leader_obj;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        obj_field_obj_get(obj, OBJ_F_NPC_LEADER, &leader_obj);
        return leader_obj;
    } else {
        return OBJ_HANDLE_NULL;
    }
}

/**
 * Sets the leader of an NPC critter.
 *
 * 0x45DE50
 */
void critter_leader_set(int64_t follower_obj, int64_t leader_obj)
{
    if (obj_field_int32_get(follower_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        mp_obj_field_obj_set(follower_obj, OBJ_F_NPC_LEADER, leader_obj);
    }
}

/**
 * Makes an NPC critter follow a leader.
 *
 * 0x45DE90
 */
bool critter_follow(int64_t follower_obj, int64_t leader_obj, bool force)
{
    unsigned int flags;

    if (tig_net_is_active() && !tig_net_is_host()) {
        return true;
    }

    // Validate follower is an NPC.
    if (obj_field_int32_get(follower_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return false;
    }

    // Check NPC can/will follow a leader (unless forced).
    if (!force) {
        if (ai_check_follow(follower_obj, leader_obj, false) != AI_FOLLOW_OK) {
            return false;
        }
    }

    // Remove existing leader if present.
    if (critter_leader_get(follower_obj) != OBJ_HANDLE_NULL) {
        critter_disband(follower_obj, true);
    }

    // Add follower to leader's follower list.
    sub_4F0070(leader_obj,
        OBJ_F_CRITTER_FOLLOWER_IDX,
        obj_arrayfield_length_get(leader_obj, OBJ_F_CRITTER_FOLLOWER_IDX),
        follower_obj);
    critter_leader_set(follower_obj, leader_obj);

    // Update NPC flags.
    flags = obj_field_int32_get(follower_obj, OBJ_F_NPC_FLAGS);
    flags &= ~ONF_JILTED;
    if (force) {
        flags |= ONF_FORCED_FOLLOWER;
    }
    mp_obj_field_int32_set(follower_obj, OBJ_F_NPC_FLAGS, flags);

    // Update follower state.
    anim_speed_recalc(follower_obj);
    mp_ui_follower_refresh();

    // Conceal new follower if leader is concealed.
    critter_set_concealed(follower_obj, critter_is_concealed(leader_obj));

    // Synchronize spell flags if leader is maintaining Tempus Fugit.
    if ((obj_field_int32_get(leader_obj, OBJ_F_SPELL_FLAGS) & OSF_TEMPUS_FUGIT) != 0) {
        flags = obj_field_int32_get(follower_obj, OBJ_F_SPELL_FLAGS);
        flags |= OSF_TEMPUS_FUGIT;
        obj_field_int32_set(follower_obj, OBJ_F_SPELL_FLAGS, flags);
    }

    return true;
}

/**
 * Disbands an NPC critter from its leader.
 *
 * 0x45DFC0
 */
bool critter_disband(int64_t obj, bool force)
{
    int64_t leader_obj;
    int64_t v1;

    leader_obj = critter_leader_get(obj);
    if (leader_obj != OBJ_HANDLE_NULL) {
        // Check conditions preventing disband unless forced.
        if (!force) {
            // Extraordinary charisma grants 100% loyalty: "Followers will never
            // flee from your side and will only leave if you ask them to, never
            // of their own accord".
            if (stat_is_extraordinary(leader_obj, STAT_CHARISMA)) {
                return false;
            }

            // Mind-controlled critters cannot voluntarily leave their master.
            if (sub_459040(obj, OSF_MIND_CONTROLLED, &v1)) {
                return false;
            }
        }

        // Remove "Wait Here" flag from the NPC.
        ai_npc_unwait(obj, force);

        // Handle disbanding.
        critter_disband_internal(obj);
    }

    return true;
}

/**
 * Removes a critter from its leader's follower list.
 *
 * 0x45E040
 */
void critter_disband_internal(int64_t obj)
{
    int64_t leader_obj;
    int64_t follower_obj;
    int cnt;
    int idx;
    unsigned int flags;

    // Retrieve current leader.
    leader_obj = critter_leader_get(obj);
    if (leader_obj == OBJ_HANDLE_NULL) {
        return;
    }

    sub_459EA0(obj);

    // Find and remove the critter from the leader's follower list.
    cnt = obj_arrayfield_length_get(leader_obj, OBJ_F_CRITTER_FOLLOWER_IDX);
    if (cnt > 0) {
        // Find index of `obj` in the followers array.
        for (idx = 0; idx < cnt; idx++) {
            follower_obj = obj_arrayfield_handle_get(leader_obj, OBJ_F_CRITTER_FOLLOWER_IDX, idx);
            if (follower_obj == obj) {
                break;
            }
        }

        // If we're not at the end of the array, move subsequent items up.
        if (idx < cnt - 1) {
            while (idx < cnt - 1) {
                follower_obj = obj_arrayfield_handle_get(leader_obj, OBJ_F_CRITTER_FOLLOWER_IDX, idx + 1);
                sub_4F0070(leader_obj, OBJ_F_CRITTER_FOLLOWER_IDX, idx, follower_obj);
                idx++;
            }
        }

        // Shrink the array.
        sub_4F0570(leader_obj, OBJ_F_CRITTER_FOLLOWER_IDX, cnt - 1);
    }

    critter_leader_set(obj, OBJ_HANDLE_NULL);

    // Update animations.
    sub_424070(obj, PRIORITY_5, false, true);
    anim_speed_recalc(obj);

    // Remove forced follower flag if present.
    flags = obj_field_int32_get(obj, OBJ_F_NPC_FLAGS);
    if ((flags & ONF_FORCED_FOLLOWER) != 0) {
        flags &= ~ONF_FORCED_FOLLOWER;
        mp_obj_field_int32_set(obj, OBJ_F_NPC_FLAGS, flags);
    }

    mp_ui_follower_refresh();

    // Reveal critter if concealed.
    if (critter_is_concealed(obj)) {
        critter_set_concealed(obj, false);
    }
}

/**
 * Removes a critter from its group or party.
 *
 * 0x45E180
 */
bool sub_45E180(int64_t obj)
{
    int type;

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    switch (type) {
    case OBJ_TYPE_PC:
        party_remove(obj);
        return true;
    case OBJ_TYPE_NPC:
        ai_npc_unwait(obj, true);
        critter_disband_internal(obj);
        return true;
    default:
        return false;
    }
}

/**
 * Toggles the on/off state of a critter and its followers.
 *
 * 0x45E1E0
 */
void critter_toggle_on_off(int64_t obj)
{
    void (*func)(int64_t obj, unsigned int flags);
    unsigned int critter_flags2;
    unsigned int spell_flags;
    ObjectList objects;
    ObjectNode* node;

    // Determine whether to set or unset the OFF flag.
    func = (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_OFF) != 0
        ? object_flags_unset
        : object_flags_set;

    // Toggle the critter's state.
    func(obj, OF_OFF);

    // Update safe off flag.
    critter_flags2 = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2);
    if (func == object_flags_set) {
        critter_flags2 |= OCF2_SAFE_OFF;
    } else {
        critter_flags2 &= ~OCF2_SAFE_OFF;
    }
    obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS2, critter_flags2);

    // Apply to all followers.
    object_list_all_followers(obj, &objects);
    node = objects.head;
    while (node != NULL) {
        spell_flags = obj_field_int32_get(node->obj, OBJ_F_SPELL_FLAGS);
        if ((spell_flags & OSF_SUMMONED) == 0
            || (spell_flags & OSF_FAMILIAR) != 0) {
            func(node->obj, OF_OFF);

            critter_flags2 = obj_field_int32_get(node->obj, OBJ_F_CRITTER_FLAGS2);
            if (func == object_flags_set) {
                critter_flags2 |= OCF2_SAFE_OFF;
            } else {
                critter_flags2 &= ~OCF2_SAFE_OFF;
            }
            obj_field_int32_set(node->obj, OBJ_F_CRITTER_FLAGS2, critter_flags2);
        }
        node = node->next;
    }
    object_list_destroy(&objects);
}

/**
 * Checks if two critters are in the same party.
 *
 * 0x45E2E0
 */
bool critter_party_same(int64_t a, int64_t b)
{
    int64_t pc_leader_a = OBJ_HANDLE_NULL;
    int64_t pc_leader_b = OBJ_HANDLE_NULL;
    int party_id_a;
    int party_id_b;

    // Retrieve party ID of `a`.
    party_id_a = party_id_from_obj(a);
    if (party_id_a == -1) {
        if (obj_field_int32_get(a, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            pc_leader_a = a;
        } else {
            pc_leader_a = critter_pc_leader_get(a);
            if (pc_leader_a != OBJ_HANDLE_NULL) {
                party_id_a = party_id_from_obj(pc_leader_a);
            }
        }
    }

    // Retrieve party ID of `b`.
    party_id_b = party_id_from_obj(b);
    if (party_id_b == -1) {
        if (obj_field_int32_get(b, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            pc_leader_b = b;
        } else {
            pc_leader_b = critter_pc_leader_get(b);
            if (pc_leader_b != OBJ_HANDLE_NULL) {
                party_id_b = party_id_from_obj(pc_leader_b);
            }
        }
    }

    return (party_id_a == party_id_b && party_id_a != -1)
        || (pc_leader_a == pc_leader_b && pc_leader_a != OBJ_HANDLE_NULL);
}

/**
 * Counts the number of followers for a critter.
 *
 * 0x45E3F0
 */
int critter_num_followers(int64_t obj, bool exclude_forced_followers)
{
    int cnt;
    int all_followers_cnt;
    int index;
    int64_t follower_obj;

    // Get total number of followers.
    cnt = obj_arrayfield_length_get(obj, OBJ_F_CRITTER_FOLLOWER_IDX);

    // Adjust count if excluding forced followers.
    if (exclude_forced_followers) {
        all_followers_cnt = cnt;
        for (index = 0; index < all_followers_cnt; index++) {
            follower_obj = obj_arrayfield_handle_get(obj, OBJ_F_CRITTER_FOLLOWER_IDX, index);
            if ((obj_field_int32_get(follower_obj, OBJ_F_NPC_FLAGS) & ONF_FORCED_FOLLOWER) != 0) {
                cnt--;
            }
        }
    }

    return cnt;
}

/**
 * Recursively counts followers, including followers of followers.
 *
 * 0x45E460
 */
int sub_45E460(int64_t critter_obj, bool exclude_forced_followers)
{
    int cnt;
    int all_followers_cnt;
    int index;
    int64_t follower_obj;

    // Get direct followers.
    cnt = obj_arrayfield_length_get(critter_obj, OBJ_F_CRITTER_FOLLOWER_IDX);

    // Recursively count followers of followers.
    all_followers_cnt = cnt;
    for (index = 0; index < all_followers_cnt; index++) {
        follower_obj = obj_arrayfield_handle_get(critter_obj, OBJ_F_CRITTER_FOLLOWER_IDX, index);
        if (exclude_forced_followers
            && (obj_field_int32_get(follower_obj, OBJ_F_NPC_FLAGS) & ONF_FORCED_FOLLOWER) != 0) {
            cnt--;
        } else {
            cnt += sub_45E460(follower_obj, exclude_forced_followers);
        }
    }

    return cnt;
}

/**
 * Retrieves the previous follower in a leader's follower list.
 *
 * 0x45E4F0
 */
int64_t critter_follower_prev(int64_t critter_obj)
{
    int64_t prev_obj;
    int64_t prev_candidate_obj;
    int64_t leader_obj;
    ObjectList followers;
    ObjectNode* node;

    prev_obj = critter_obj;

    // Get leader and check followers.
    leader_obj = critter_leader_get(critter_obj);
    if (leader_obj != OBJ_HANDLE_NULL) {
        prev_candidate_obj = OBJ_HANDLE_NULL;

        object_list_followers(leader_obj, &followers);
        node = followers.head;
        while (node != NULL) {
            if (node->obj == critter_obj) {
                break;
            }
            prev_candidate_obj = node->obj;
            node = node->next;
        }

        // Determine previous follower or wrap around.
        if (node != NULL) {
            if (prev_candidate_obj != OBJ_HANDLE_NULL) {
                prev_obj = prev_candidate_obj;
            } else {
                while (node != NULL) {
                    prev_candidate_obj = node->obj;
                    node = node->next;
                }
                prev_obj = prev_candidate_obj;
            }
        }

        object_list_destroy(&followers);
    }

    return prev_obj;
}

/**
 * Retrieves the next follower in a leader's follower list.
 *
 * 0x45E590
 */
int64_t critter_follower_next(int64_t critter_obj)
{
    int64_t next_obj;
    int64_t leader_obj;
    ObjectList followers;
    ObjectNode* node;

    next_obj = critter_obj;

    // Get leader and check followers.
    leader_obj = critter_leader_get(critter_obj);
    if (leader_obj != OBJ_HANDLE_NULL) {
        object_list_followers(leader_obj, &followers);
        node = followers.head;
        while (node != NULL) {
            if (node->obj == critter_obj) {
                break;
            }
            node = node->next;
        }

        // Determine next follower or wrap around.
        if (node != NULL) {
            if (node->next != NULL) {
                next_obj = node->next->obj;
            } else {
                next_obj = followers.head->obj;
            }
        }

        object_list_destroy(&followers);
    }

    return next_obj;
}

/**
 * Called when a fatigue timeevent occurs.
 *
 * 0x45E610
 */
bool critter_fatigue_timeevent_process(TimeEvent* timeevent)
{
    int64_t critter_obj;
    int type;
    int v3;
    int fatigue;
    int dam;
    int encumbrance_level;

    if (tig_net_is_active()) {
        if (!tig_net_is_host()) {
            return true;
        }
    }

    // Extract event parameters.
    type = timeevent->params[0].integer_value;
    critter_obj = timeevent->params[1].object_value;

    if (critter_obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if (critter_is_dead(critter_obj)) {
        return true;
    }

    // TODO: Figure out math.
    v3 = sub_45A820(timeevent->params[2].integer_value) / 8 / 10;
    if (v3 < 1) {
        v3 = 1;
    }

    dam = critter_fatigue_damage_get(critter_obj);

    // Handle event type.
    switch (type) {
    case FATIGUE_EVENT_RECOVERY:
        if (dam > 0) {
            // Recover fatigue based on heal rate.
            dam -= v3 * stat_level_get(critter_obj, STAT_HEAL_RATE);
            fatigue = critter_fatigue_current(critter_obj);
            if (dam < 0) {
                dam = 0;
            }
            critter_fatigue_damage_set(critter_obj, dam);

            if (tig_net_is_active() && tig_net_is_host()) {
                PacketCritterFatigueDamageSet pkt;

                pkt.type = 35;
                pkt.oid = obj_get_id(critter_obj);
                pkt.dam = dam;
                tig_net_send_app_all(&pkt, sizeof(pkt));
            }

            // Play get-up animation if recovered from unconsciousness.
            if (fatigue <= 0 && critter_fatigue_current(critter_obj) > 0) {
                anim_goal_get_up(critter_obj);
            }
        }
        break;
    case FATIGUE_EVENT_DAMAGE:
        // Apply fatigue damage if sufficient fatigue remains.
        fatigue = critter_fatigue_current(critter_obj);
        if (fatigue >= 5) {
            if (fatigue - v3 < 5) {
                v3 = 1;
            }
            dam += v3;
            critter_fatigue_damage_set(critter_obj, dam);
        }

        if (tig_net_is_active() && tig_net_is_host()) {
            PacketCritterFatigueDamageSet pkt;

            pkt.type = 35;
            pkt.oid = obj_get_id(critter_obj);
            pkt.dam = dam;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }

        // Schedule next fatigue damage event if applicable.
        encumbrance_level = critter_encumbrance_level_get(critter_obj);
        if (fatigue_damage_delay_tbl[encumbrance_level] != 0) {
            critter_fatigue_timeevent_schedule(critter_obj, FATIGUE_EVENT_DAMAGE, fatigue_damage_delay_tbl[encumbrance_level]);
        }
        break;
    }

    return true;
}

/**
 * Schedules a fatigue-related time event for a critter.
 *
 * 0x45E820
 */
bool critter_fatigue_timeevent_schedule(int64_t obj, int type, int delay)
{
    TimeEvent timeevent;
    DateTime datetime;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        return true;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if (obj_is_proto(obj)) {
        return true;
    }

    // Check for existing event to avoid duplicates.
    critter_test_obj = obj;
    critter_test_fatigue_type = type;
    if (timeevent_any(TIMEEVENT_TYPE_FATIGUE, fatigue_timeevent_check)) {
        return true;
    }

    // Schedule the event.
    timeevent.type = TIMEEVENT_TYPE_FATIGUE;
    timeevent.params[0].integer_value = type;
    timeevent.params[1].object_value = obj;
    timeevent.params[2].integer_value = sub_45A7F0();
    sub_45A950(&datetime, delay);
    return timeevent_add_delay(&timeevent, &datetime);
}

/**
 * 0x45E8D0
 */
bool fatigue_timeevent_check(TimeEvent* timeevent)
{
    return timeevent->params[0].integer_value == critter_test_fatigue_type
        && timeevent->params[1].object_value == critter_test_obj;
}

/**
 * Heals a critter's HP and fatigue during rest.
 *
 * 0x45E910
 */
void critter_resting_heal(int64_t critter_obj, int hours)
{
    int dam;
    int heal_rate;

    if (tig_net_is_active() && !tig_net_is_host()) {
        return;
    }

    if (critter_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (critter_is_dead(critter_obj)) {
        return;
    }

    // Heal HP based on heal rate and the number of hours elapsed.
    dam = object_hp_damage_get(critter_obj);
    heal_rate = stat_level_get(critter_obj, STAT_HEAL_RATE);
    if (dam > 0) {
        dam -= heal_rate * hours;
        if (dam < 0) {
            dam = 0;
        }
        object_hp_damage_set(critter_obj, dam);
    }

    // Heal fatigue based on heal rate and the number of hours elapsed.
    dam = critter_fatigue_damage_get(critter_obj);
    if (dam > 0) {
        dam -= 3 * heal_rate * hours;
        if (dam < 0) {
            dam = 0;
        }
        critter_fatigue_damage_set(critter_obj, dam);
    }

    if (tig_net_is_active()) {
        PacketCritterRestingHeal pkt;

        pkt.type = 34;
        pkt.hours = hours;
        pkt.oid = obj_get_id(critter_obj);
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

/**
 * Called when a resting timeevent occurs.
 *
 * 0x45EA00
 */
bool critter_resting_timeevent_process(TimeEvent* timeevent)
{
    int64_t obj;
    int hours;

    if (timeevent == NULL) {
        return true;
    }

    obj = timeevent->params[0].object_value;
    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    hours = sub_45A820(timeevent->params[1].integer_value) / 3600;
    if (hours < 1) {
        hours = 1;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC
        && critter_pc_leader_get(obj) == OBJ_HANDLE_NULL) {
        critter_resting_heal(obj, hours);
    }

    return true;
}

/**
 * 0x45EA80
 */
bool resting_timeevent_check(TimeEvent* timeevent)
{
    return timeevent != NULL && timeevent->params[0].object_value == critter_test_obj;
}

/**
 * Schedules a resting time event for a critter.
 *
 * 0x45EAB0
 */
bool critter_resting_timeevent_schedule(int64_t obj)
{
    TimeEvent timeevent;
    DateTime datetime;

    if (tig_net_is_active() && !tig_net_is_host()) {
        return true;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if (obj_is_proto(obj)) {
        return true;
    }

    // Check for existing event.
    critter_test_obj = obj;
    if (timeevent_any(TIMEEVENT_TYPE_RESTING, resting_timeevent_check)) {
        return true;
    }

    // Schedule the event.
    timeevent.type = TIMEEVENT_TYPE_RESTING;
    timeevent.params[0].object_value = obj;
    timeevent.params[1].integer_value = sub_45A7F0();
    sub_45A950(&datetime, 3600000);
    return timeevent_add_delay(&timeevent, &datetime);
}

/**
 * Called when a decay timeevent occurs.
 *
 * 0x45EB50
 */
bool critter_decay_timeevent_process(TimeEvent* timeevent)
{
    int64_t obj = timeevent->params[0].object_value;

    if (obj == OBJ_HANDLE_NULL) {
        tig_debug_printf("Critter: Error: Attempt to decay a NULL object!\n");
        return true;
    }

    // Process decay for critters.
    if (obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        if (!critter_is_dead(obj)) {
            tig_debug_printf("Critter: critter_decay_timeevent_process: ERROR: Attempt to Decay a LIVE critter!\n");
            return false;
        }

        sub_463860(obj, true);
    }

    object_destroy(obj);

    return true;
}

/**
 * Schedules a decay timeevent for a dead critter or object.
 *
 * 0x45EBE0
 */
bool critter_decay_timeevent_schedule(int64_t obj)
{
    int type;
    TimeEvent timeevent;
    DateTime datetime;

    if (obj == OBJ_HANDLE_NULL
        || obj_is_proto(obj)) {
        return false;
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);

    // Schedule decay event.
    timeevent.type = TIMEEVENT_TYPE_DECAY_DEAD_BODIE;
    timeevent.params[0].object_value = obj;
    timeevent.params[1].integer_value = sub_45A7F0();
    if (obj_type_is_critter(type)) {
        // 1 day for critters.
        sub_45A950(&datetime, 86400000);
    } else {
        // 2 days for non-critters.
        sub_45A950(&datetime, 172800000);
    }
    return timeevent_add_delay(&timeevent, &datetime);
}

/**
 * Cancels a decay timeevent for an object.
 *
 * 0x45EC80
 */
void critter_decay_timeevent_cancel(int64_t obj)
{
    critter_decay_test_obj = obj;
    timeevent_clear_one_ex(TIMEEVENT_TYPE_DECAY_DEAD_BODIE, decay_timeevent_check);
}

/**
 * 0x45ECB0
 */
bool decay_timeevent_check(TimeEvent* timeevent)
{
    return timeevent->params[0].object_value == critter_decay_test_obj;
}

/**
 * Called when a combat focus wipe timeevent occurs.
 *
 * 0x45ECE0
 */
bool critter_npc_combat_focus_wipe_timeevent_process(TimeEvent* timeevent)
{
    int64_t npc_obj;
    int64_t combat_focus_obj;

    npc_obj = timeevent->params[0].object_value;
    if (npc_obj == OBJ_HANDLE_NULL
        || !critter_is_dead(npc_obj)) {
        return true;
    }

    // Get current combat focus.
    obj_field_obj_get(npc_obj, OBJ_F_NPC_COMBAT_FOCUS, &combat_focus_obj);
    if (combat_focus_obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if (object_dist(npc_obj, combat_focus_obj) >= 30) {
        // Target is too far.
        obj_field_handle_set(npc_obj, OBJ_F_NPC_COMBAT_FOCUS, OBJ_HANDLE_NULL);
    } else {
        // Reschedule.
        critter_npc_combat_focus_wipe_schedule(npc_obj);
    }

    return true;
}

/**
 * Schedules a combat focus wipe timeevent for an NPC.
 *
 * 0x45ED70
 */
bool critter_npc_combat_focus_wipe_schedule(int64_t npc_obj)
{
    TimeEvent timeevent;
    DateTime datetime;

    if (npc_obj == OBJ_HANDLE_NULL
        || obj_is_proto(npc_obj)) {
        return false;
    }

    // Schedule the event.
    timeevent.type = TIMEEVENT_TYPE_COMBAT_FOCUS_WIPE;
    timeevent.params[0].object_value = npc_obj;
    timeevent.params[1].integer_value = sub_45A7F0();
    sub_45A950(&datetime, 600000); // 10 minutes.

    return timeevent_add_delay(&timeevent, &datetime);
}

/**
 * Checks if a critter is concealed.
 *
 * 0x45EDE0
 */
bool critter_is_concealed(int64_t obj)
{
    return obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))
        && (obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & OCF_IS_CONCEALED) != 0;
}

/**
 * Sets the concealed state of a critter and its followers.
 *
 * 0x45EE30
 */
void critter_set_concealed(int64_t obj, bool concealed)
{
    ObjectList objects;
    ObjectNode* node;

    // Set critter concealed.
    critter_set_concealed_internal(obj, concealed);

    // Apply to all followers.
    object_list_all_followers(obj, &objects);
    node = objects.head;
    while (node != NULL) {
        critter_set_concealed_internal(node->obj, concealed);
        node = node->next;
    }
    object_list_destroy(&objects);
}

/**
 * Internal helper to set the concealed state of a critter.
 *
 * 0x45EE90
 */
void critter_set_concealed_internal(int64_t obj, bool concealed)
{
    tig_art_id_t art_id;
    tig_art_id_t new_art_id;
    unsigned int flags;

    if (!multiplayer_is_locked()) {
        PacketCritterConcealSet pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 33;
        sub_4440E0(obj, &(pkt.field_8));
        pkt.concealed = concealed;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    sub_424070(obj, PRIORITY_5, false, false);

    // Update animation based on concealed state.
    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    if (concealed) {
        new_art_id = critter_conceal_aid(art_id);
        if (new_art_id == art_id) {
            // Critter is already concealed, no need to do anything.
            return;
        }
    } else {
        new_art_id = tig_art_id_anim_set(art_id, 0);
        new_art_id = tig_art_id_frame_set(new_art_id, 0);
    }

    // Update concealed flags.
    flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
    if (concealed) {
        flags |= OCF_IS_CONCEALED | OCF_MOVING_SILENTLY;
    } else {
        flags &= ~(OCF_IS_CONCEALED | OCF_MOVING_SILENTLY);
    }
    obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, flags);

    // Apply new animation if active.
    if (critter_is_active(obj)) {
        object_set_current_aid(obj, new_art_id);
        sub_43F030(obj);
    }

    // Trigger fidget animation if revealed in combat.
    if (!concealed) {
        if (combat_critter_is_combat_mode_active(obj)) {
            anim_goal_fidget(obj);
        }
    }
}

/**
 * Generates a concealed animation ID for a critter.
 *
 * 0x45EFA0
 */
tig_art_id_t critter_conceal_aid(tig_art_id_t art_id)
{
    tig_art_id_t new_art_id;
    TigArtAnimData art_anim_data;

    new_art_id = tig_art_id_anim_set(art_id, TIG_ART_ANIM_CONCEAL_FIDGET);
    if (tig_art_exists(new_art_id) == TIG_OK
        && tig_art_anim_data(new_art_id, &art_anim_data) == TIG_OK) {
        return tig_art_id_frame_set(new_art_id, art_anim_data.num_frames - 1);
    } else {
        return art_id;
    }
}

/**
 * Checks if a critter is facing another object.
 *
 * 0x45EFF0
 */
bool critter_is_facing_to(int64_t a, int64_t b)
{
    int rot;
    int facing;
    int delta;

    // Calculate rotation and facing difference.
    rot = object_rot(a, b);
    facing = tig_art_id_rotation_get(obj_field_int32_get(a, OBJ_F_CURRENT_AID));
    delta = (rot - facing + 8) % 8;

    // Allow facing within a small angle range.
    return delta == 0
        || delta == 1
        || delta == 7
        || delta == 2
        || delta == 6;
}

/**
 * Checks if a critter's stat meets a threshold with a random check.
 *
 * 0x45F060
 */
bool critter_check_stat(int64_t obj, int stat, int mod)
{
    int value;
    int lower;
    int upper;

    value = stat_level_get(obj, stat) + mod;
    upper = stat_level_max(obj, stat);
    lower = stat_level_min(obj, stat);

    return random_between(lower, upper) <= value;
}

/**
 * Retrieves the XP worth of an NPC critter.
 *
 * 0x45F0B0
 */
int critter_xp_worth(int64_t obj)
{
    MesFileEntry mes_file_entry;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return 0;
    }

    // Look up XP based on level.
    mes_file_entry.num = stat_level_get(obj, STAT_LEVEL);
    if (!mes_search(xp_mes_file, &mes_file_entry)) {
        return 0;
    }

    return atoi(mes_file_entry.str);
}

/**
 * Awards XP to a PC and its party members.
 *
 * 0x45F110
 */
void critter_give_xp(int64_t obj, int xp_gain)
{
    int player;
    int cnt;
    ObjectList objects;
    ObjectNode* node;
    int xp;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    if (xp_gain == 0) {
        return;
    }

    // Adjust reward based on game difficulty.
    switch (gamelib_game_difficulty_get()) {
    case GAME_DIFFICULTY_EASY:
        xp_gain += xp_gain / 2;
        break;
    case GAME_DIFFICULTY_HARD:
        xp_gain -= xp_gain / 2;
        break;
    }

    if (tig_net_is_active()) {
        if (!tig_net_is_host()) {
            return;
        }

        player = party_id_from_obj(obj);
    } else {
        player = -1;
    }

    if (player != -1) {
        // Count nearby living party members.
        cnt = 0;
        object_list_vicinity(obj, OBJ_TM_PC, &objects);
        node = objects.head;
        while (node != NULL) {
            if (!critter_is_dead(node->obj)
                && node->obj != OBJ_HANDLE_NULL
                && party_id_from_obj(node->obj) == player) {
                cnt++;
            }
            node = node->next;
        }

        // Distribute XP among party members.
        xp_gain /= cnt;
        if (xp_gain == 0) {
            xp_gain = 1;
        }

        node = objects.head;
        while (node != NULL) {
            if (!critter_is_dead(node->obj)
                && node->obj != OBJ_HANDLE_NULL
                && party_id_from_obj(node->obj) == player) {
                xp = stat_base_get(node->obj, STAT_EXPERIENCE_POINTS);
                xp += effect_adjust_xp_gain(node->obj, xp_gain);
                stat_base_set(node->obj, STAT_EXPERIENCE_POINTS, xp);
            }
            node = node->next;
        }

        object_list_destroy(&objects);
        sub_4EDDE0(OBJ_HANDLE_NULL);
    } else {
        // Single-player game - just give all reward to the local PC.
        if (!critter_is_dead(obj)) {
            xp = stat_base_get(obj, STAT_EXPERIENCE_POINTS);
            xp += effect_adjust_xp_gain(obj, xp_gain);
            stat_base_set(obj, STAT_EXPERIENCE_POINTS, xp);
            sub_4EDDE0(obj);
        }
    }
}

/**
 * Makes a critter enter a bed for sleeping.
 *
 * 0x45F2D0
 */
bool critter_enter_bed(int64_t obj, int64_t bed)
{
    unsigned int flags;
    int64_t obj_location;
    int64_t bed_location;
    int64_t location;

    // Check if bed is occupied.
    if (obj_field_handle_get(bed, OBJ_F_SCENERY_WHOS_IN_ME) != OBJ_HANDLE_NULL) {
        return false;
    }

    // Set sleeping state.
    flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
    flags |= OCF_SLEEPING;
    obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, flags);

    // Assign critter to bed and update bed state.
    obj_field_handle_set(bed, OBJ_F_SCENERY_WHOS_IN_ME, obj);
    object_inc_current_aid(bed);

    // Hide critter.
    object_flags_set(obj, OF_DONTDRAW);

    // Move critter to bed's location if possible.
    bed_location = obj_field_int64_get(bed, OBJ_F_LOCATION);
    obj_location = obj_field_int64_get(bed, OBJ_F_LOCATION);
    if (bed_location == obj_location
        && location_in_dir(obj_location, 4, &location)) {
        object_move_to_location(obj, location, 0, 0);
    }

    return true;
}

/**
 * Makes a critter leave a bed.
 *
 * 0x45F3A0
 */
void critter_leave_bed(int64_t obj, int64_t bed)
{
    unsigned int flags;

    // Verify critter is in the bed.
    if (obj_field_handle_get(bed, OBJ_F_SCENERY_WHOS_IN_ME) != obj) {
        tig_debug_printf("Someone is trying to make a critter leave a bed that he isn't in!\n");
        return;
    }

    // Unhide critter.
    object_flags_unset(obj, OF_DONTDRAW);

    // Clear sleeping state.
    flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
    flags &= ~OCF_SLEEPING;
    obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, flags);

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        flags = obj_field_int32_get(obj, OBJ_F_NPC_FLAGS);
        flags &= ~ONF_WAYPOINTS_BED;
        obj_field_int32_set(obj, OBJ_F_NPC_FLAGS, flags);
    }

    // Clear bed occupancy and update bed animation.
    obj_field_handle_set(bed, OBJ_F_SCENERY_WHOS_IN_ME, OBJ_HANDLE_NULL);
    object_dec_current_aid(bed);
}

/**
 * NOTE: Unclear.
 *
 * 0x45F460
 */
bool critter_has_bad_associates(int64_t obj)
{
    ObjectList objects;
    ObjectNode* node;
    unsigned int flags;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        obj = critter_pc_leader_get(obj);
        if (obj == OBJ_HANDLE_NULL) {
            return false;
        }
    }

    object_list_all_followers(obj, &objects);
    node = objects.head;
    while (node != NULL) {
        if ((obj_field_int32_get(node->obj, OBJ_F_SPELL_FLAGS) & OSF_SUMMONED) != 0
            && (obj_field_int32_get(node->obj, OBJ_F_NPC_FLAGS) & ONF_FAMILIAR) == 0
            && (obj_field_int32_get(node->obj, OBJ_F_CRITTER_FLAGS) & OCF_ANIMAL) == 0) {
            flags = obj_field_int32_get(node->obj, OBJ_F_FLAGS);
            if ((flags & (OF_OFF | OF_DONTDRAW)) == 0) {
                break;
            }
            tig_debug_printf("critter_has_bad_associates: WARNING: Critter found in party that is turned OFF/DONT_DRAW: oFlags: %d!\n", flags);
        }
        node = node->next;
    }

    object_list_destroy(&objects);

    return node != NULL;
}

/**
 * Determines if a critter can open doors and windows.
 *
 * 0x45F550
 */
bool critter_can_open_portals(int64_t obj)
{
    return ai_critter_can_open_portals(obj);
}

/**
 * Determines if a critter can jump through windows.
 *
 * 0x45F570
 */
bool critter_can_jump_window(int64_t obj)
{
    tig_art_id_t art_id;
    int type;

    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    type = tig_art_type(art_id);
    switch (type) {
    case TIG_ART_TYPE_CRITTER:
        return true;
    case TIG_ART_TYPE_MONSTER:
        return tig_art_monster_id_specie_get(art_id) == TIG_ART_MONSTER_SPECIE_ORC;
    default:
        return false;
    }
}

/**
 * Orders a critter to stay close.
 *
 * 0x45F5C0
 */
void critter_stay_close(int64_t npc_obj)
{
    unsigned int npc_flags;

    npc_flags = obj_field_int32_get(npc_obj, OBJ_F_NPC_FLAGS);
    if ((npc_flags & ONF_AI_SPREAD_OUT) != 0) {
        npc_flags &= ~ONF_AI_SPREAD_OUT;
        obj_field_int32_set(npc_obj, OBJ_F_NPC_FLAGS, npc_flags);
    }
}

/**
 * Orders a critter to spread out.
 *
 * 0x45F600
 */
void critter_spread_out(int64_t npc_obj)
{
    unsigned int npc_flags;

    npc_flags = obj_field_int32_get(npc_obj, OBJ_F_NPC_FLAGS);
    if ((npc_flags & ONF_AI_SPREAD_OUT) == 0) {
        npc_flags |= ONF_AI_SPREAD_OUT;
        obj_field_int32_set(npc_obj, OBJ_F_NPC_FLAGS, npc_flags);

        // Play spread out animation.
        anim_goal_attempt_spread_out(npc_obj, critter_leader_get(npc_obj));
    }
}

/**
 * Retrieves the substitute inventory object for a critter.
 *
 * 0x45F650
 */
int64_t critter_substitute_inventory_get(int64_t obj)
{
    int64_t substitute_inventory_obj;

    if (obj == OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    // Only applicable to NPCs.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return OBJ_HANDLE_NULL;
    }

    if (critter_leader_get(obj) != OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    // Retrieve the substitute inventory handle.
    substitute_inventory_obj = obj_field_handle_get(obj, OBJ_F_NPC_SUBSTITUTE_INVENTORY);
    if (substitute_inventory_obj == OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    // Make sure it's not too far.
    if (object_dist(obj, substitute_inventory_obj) > 20) {
        return OBJ_HANDLE_NULL;
    }

    return substitute_inventory_obj;
}

/**
 * Retrieves the teleport map for a critter, initializing it if unset.
 *
 * 0x45F6D0
 */
int critter_teleport_map_get(int64_t obj)
{
    int map;

    map = obj_field_int32_get(obj, OBJ_F_CRITTER_TELEPORT_MAP);
    if (map == 0) {
        map = map_current_map();
        obj_field_int32_set(obj, OBJ_F_CRITTER_TELEPORT_MAP, map);
    }

    return map;
}

// 0x45F710
void sub_45F710(int64_t obj)
{
    critter_teleport_map_get(obj);
}

/**
 * Checks if a critter is monster-like (mechanical, monster, animal, or undead).
 *
 * 0x45F730
 */
bool critter_is_monstrous(int64_t critter_obj)
{
    unsigned int critter_flags;

    if (critter_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(critter_obj, OBJ_F_TYPE))) {
        return false;
    }

    // Check for monstrous flags.
    critter_flags = obj_field_int32_get(critter_obj, OBJ_F_CRITTER_FLAGS);
    if ((critter_flags & (OCF_MECHANICAL | OCF_MONSTER | OCF_ANIMAL | OCF_UNDEAD)) == 0) {
        return false;
    }

    return true;
}

/**
 * Calculates the encumbrance level of a critter based on carried weight.
 *
 * 0x45F790
 */
int critter_encumbrance_level_get(int64_t obj)
{
    int current_weight;
    int max_weight;
    int encumbrance_level;

    // Check if object is a critter.
    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return ENCUMBRANCE_LEVEL_SEVERE;
    }

    // Get current and maximum carry weight.
    current_weight = item_total_weight(obj);
    max_weight = stat_level_get(obj, STAT_CARRY_WEIGHT);

    // Calculate encumbrance level based on weight ratio.
    for (encumbrance_level = 0; encumbrance_level < MAX_ENCUMBRANCE_LEVEL; encumbrance_level++) {
        if (current_weight < max_weight * critter_encumbrance_level_ratio(encumbrance_level) / 100) {
            return encumbrance_level;
        }
    }

    return ENCUMBRANCE_LEVEL_SEVERE;
}

/**
 * Recalculates and applies encumbrance effects if the level has changed.
 *
 * 0x45F820
 */
void critter_encumbrance_level_recalc(int64_t obj, int prev_encumbrance_level)
{
    int encumbrance_level;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    int lvl;

    encumbrance_level = critter_encumbrance_level_get(obj);
    if (encumbrance_level == prev_encumbrance_level) {
        return;
    }

    // Remove all existing encumbrance effects.
    effect_remove_all_typed(obj, EFFECT_ENCUMBRANCE);

    // Add one effect per each encumbrance level.
    for (lvl = 0; lvl < encumbrance_level; lvl++) {
        effect_add(obj, EFFECT_ENCUMBRANCE, EFFECT_CAUSE_ITEM);
    }

    // Special case - severe encumbrance adds two extra penalties.
    if (encumbrance_level == ENCUMBRANCE_LEVEL_SEVERE) {
        effect_add(obj, EFFECT_ENCUMBRANCE, EFFECT_CAUSE_ITEM);
        effect_add(obj, EFFECT_ENCUMBRANCE, EFFECT_CAUSE_ITEM);
    }

    // Recalculate animation speed based on new effects.
    anim_speed_recalc(obj);

    // Provide UI feedback if enabled and for local PC.
    if (critter_encumbrance_recalc_feedback_cnt > 0 && player_is_local_pc_obj(obj)) {
        mes_file_entry.num = 11 + encumbrance_level;
        mes_get_msg(critter_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_EXCLAMATION;
        ui_message.str = mes_file_entry.str;
        sub_460630(&ui_message);
    }

    // Schedule fatigue damage event.
    if (fatigue_damage_delay_tbl[encumbrance_level] != 0) {
        critter_fatigue_timeevent_schedule(obj, FATIGUE_EVENT_DAMAGE, fatigue_damage_delay_tbl[encumbrance_level]);
    }
}

/**
 * Enables encumbrance level change feedback.
 *
 * 0x45F910
 */
void critter_encumbrance_recalc_feedback_enable()
{
    critter_encumbrance_recalc_feedback_cnt++;
}

/**
 * Disables encumbrance level change feedback.
 *
 * 0x45F920
 */
void critter_encumbrance_recalc_feedback_disable()
{
    critter_encumbrance_recalc_feedback_cnt--;
}

/**
 * Retrieves the name of an encumbrance level.
 *
 * 0x45F930
 */
const char* critter_encumbrance_level_name(int level)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = level + 18;
    mes_get_msg(critter_mes_file, &mes_file_entry);

    return mes_file_entry.str;
}

/**
 * Retrieves the ratio for an encumbrance level.
 *
 * 0x45F960
 */
int critter_encumbrance_level_ratio(int level)
{
    return encumbrance_level_ratio_tbl[level];
}

/**
 * Retrieves the description ID for a critter when `b` is looking on `a`.
 *
 * 0x45F970
 */
int critter_description_get(int64_t a, int64_t b)
{
    if (a == b || (b != OBJ_HANDLE_NULL && reaction_met_before(a, b))) {
        return obj_field_int32_get(a, OBJ_F_DESCRIPTION);
    } else {
        return obj_field_int32_get(a, OBJ_F_CRITTER_DESCRIPTION_UNKNOWN);
    }
}

/**
 * Checks if a critter can perform a backstab on a target.
 *
 * 0x45F9D0
 */
bool critter_can_backstab(int64_t source_obj, int64_t target_obj)
{
    int64_t weapon_obj;
    int weapon_type;

    // At least one level of training in Backstab is required.
    if (basic_skill_training_get(source_obj, BASIC_SKILL_BACKSTAB) == 0) {
        return false;
    }

    // Target must not be facing the attacker.
    if (critter_is_facing_to(target_obj, source_obj)) {
        return false;
    }

    // Get the equipped weapon.
    weapon_obj = combat_critter_weapon(source_obj);
    if (weapon_obj == OBJ_HANDLE_NULL) {
        // Cannot backstab with bare hands.
        return false;
    }

    // Determine weapon type.
    weapon_type = tig_art_item_id_subtype_get(obj_field_int32_get(weapon_obj, OBJ_F_ITEM_USE_AID_FRAGMENT));
    if (weapon_type == TIG_ART_WEAPON_TYPE_DAGGER) {
        // Daggers are always allowed.
        return true;
    }

    // The expert of backstabbing may backstab with swords (including two-handed
    // swords) and axes.
    if (basic_skill_training_get(source_obj, BASIC_SKILL_BACKSTAB) >= TRAINING_EXPERT
        && (weapon_type == TIG_ART_WEAPON_TYPE_SWORD
            || weapon_type == TIG_ART_WEAPON_TYPE_AXE
            || weapon_type == TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD)) {
        return true;
    }

    return false;
}

/**
 * Retrieves the light art ID and color for a critter based on its light flags.
 *
 * 0x45FA70
 */
tig_art_id_t critter_light_get(int64_t critter_obj, unsigned int* color_ptr)
{
    tig_art_id_t art_id;
    unsigned int critter_flags;

    critter_flags = obj_field_int32_get(critter_obj, OBJ_F_CRITTER_FLAGS);
    if ((critter_flags & OCF_LIGHT_XLARGE) != 0) {
        // Extra large light (pale aqua).
        tig_art_light_id_create(1, 0, 0, 0, &art_id);
        light_build_color(244, 255, 255, color_ptr);
    } else if ((critter_flags & OCF_LIGHT_LARGE) != 0) {
        // Large light (peach puff).
        tig_art_light_id_create(1, 0, 0, 0, &art_id);
        light_build_color(255, 214, 172, color_ptr);
    } else if ((critter_flags & OCF_LIGHT_MEDIUM) != 0) {
        // Medium light (coral).
        tig_art_light_id_create(1, 0, 0, 0, &art_id);
        light_build_color(236, 138, 85, color_ptr);
    } else if ((critter_flags & OCF_LIGHT_SMALL) != 0) {
        // Small light (terracotta).
        tig_art_light_id_create(1, 0, 0, 0, &art_id);
        light_build_color(167, 88, 58, color_ptr);
    } else {
        // No light.
        art_id = TIG_ART_ID_INVALID;
        light_build_color(0, 0, 0, color_ptr);
    }

    return art_id;
}

/**
 * Checks if a critter has dark sight ability.
 *
 * 0x45FB90
 */
bool critter_has_dark_sight(int64_t critter_obj)
{
    if (critter_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(critter_obj, OBJ_F_TYPE))) {
        return false;
    }

    // Check background for dark sight.
    if (background_get(critter_obj) == BACKGROUND_DARK_SIGHT) {
        return true;
    }

    // Check critter flag for dark sight.
    if ((obj_field_int32_get(critter_obj, OBJ_F_CRITTER_FLAGS2) & OCF2_DARK_SIGHT) != 0) {
        return true;
    }

    return false;
}

/**
 * Checks if a critter is dumb.
 *
 * 0x45FC00
 */
bool critter_is_dumb(int64_t critter_obj)
{
    int background;

    if (critter_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    // Ensure it's a critter.
    if (!obj_type_is_critter(obj_field_int32_get(critter_obj, OBJ_F_TYPE))) {
        return false;
    }

    // Check intelligence.
    if (stat_level_get(critter_obj, STAT_INTELLIGENCE) <= LOW_INTELLIGENCE) {
        return true;
    }

    // Check for dumb-implying backgrounds.
    background = background_get(critter_obj);
    if (background == BACKGROUND_IDIOT_SAVANT
        || background == BACKGROUND_FRANKENSTEIN_MONSTER
        || background == BACKGROUND_BRIDE_OF_FRANKENSTEIN) {
        return true;
    }

    return false;
}

/**
 * Prints debug information on a critter.
 *
 * 0x45FC70
 */
void critter_debug_obj(int64_t obj)
{
    char name[1000];
    int stat;
    int level;
    int base;

    // Check for valid critter.
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return;
    }

    tig_debug_println("\n\n--------------------------------------");
    tig_debug_println("Critter Debug Obj:\n\n");
    object_examine(obj, obj, name);
    tig_debug_printf("Name: %s\n", name);
    tig_debug_println("Stats:  Level (Base)");

    for (stat = 0; stat < STAT_COUNT; stat++) {
        level = stat_level_get(obj, stat);
        base = stat_base_get(obj, stat);
        tig_debug_printf("\t[%s]:\t%d (%d)\n",
            stat_name(stat),
            level,
            base);
    }

    // Print effects.
    effect_debug_obj(obj);
}
