#include "game/combat.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/animfx.h"
#include "game/ci.h"
#include "game/criticals.h"
#include "game/critter.h"
#include "game/descriptions.h"
#include "game/effect.h"
#include "game/fate.h"
#include "game/gamelib.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/logbook.h"
#include "game/magictech.h"
#include "game/map.h"
#include "game/material.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/mt_item.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/object_node.h"
#include "game/player.h"
#include "game/random.h"
#include "game/reaction.h"
#include "game/resistance.h"
#include "game/script.h"
#include "game/sfx.h"
#include "game/skill.h"
#include "game/stat.h"
#include "game/tf.h"
#include "game/trap.h"

static void turn_based_changed(void);
static void fast_turn_based_changed(void);
static void combat_create_projectile(CombatContext* combat, int64_t loc, int a3, int a4, tig_art_id_t proj_aid);
static void sub_4B2690(int64_t proj_obj, int64_t a2, int64_t a3, CombatContext* combat, bool a5);
static int combat_weapon_loudness(int64_t weapon_obj);
static void sub_4B2F60(CombatContext* combat);
static void combat_process_melee_attack(CombatContext* combat);
static void combat_process_ranged_attack(CombatContext* combat);
static void combat_critter_toggle_combat_mode(int64_t obj);
static int64_t combat_critter_armor(int64_t critter_obj, int hit_loc);
static bool sub_4B5520(CombatContext* combat);
static void combat_apply_weapon_wear(CombatContext* combat);
static void combat_remove_blood_splotch(int64_t loc);
static void combat_process_crit_hit(CombatContext* combat);
static void combat_process_crit_miss(CombatContext* combat);
static int combat_random_hit_loc(void);
static bool combat_consume_ammo(int64_t weapon_obj, int64_t critter_obj, int cnt, bool destroy);
static void combat_calc_dmg(CombatContext* combat);
static void combat_apply_resistance(CombatContext* combat);
static int combat_play_hit_fx(CombatContext* combat);
static void combat_process_taunts(CombatContext* combat);
static void sub_4B7080(void);
static bool combat_turn_based_start(void);
static void combat_turn_based_end(void);
static bool combat_turn_based_begin_turn(void);
static void combat_recalc_perception_range(void);
static bool sub_4B7580(ObjectNode* object_node);
static void combat_turn_based_subturn_start(void);
static void combat_turn_based_subturn_end(void);
static void combat_turn_based_end_turn(void);
static int combat_move_cost(int64_t source_obj, int64_t target_loc, bool adjacent);
static bool sub_4B7DC0(int64_t obj);
static void sort_combat_list(void);
static void pc_switch_weapon(int64_t pc_obj, int64_t target_obj);

// 0x5B5790
static struct {
    const char* name;
    bool (*func)(int64_t a1, int64_t a2, int64_t a3, int64_t a4);
} stru_5B5790[] = {
    { "Multiplayer", sub_4A6190 },
};

// 0x5B5798
static int hit_loc_penalties[HIT_LOC_COUNT] = {
    /* HIT_LOC_TORSO */ 0,
    /*  HIT_LOC_HEAD */ -50,
    /*   HIT_LOC_ARM */ -30,
    /*   HIT_LOC_LEG */ -30,
};

// 0x5B57A8
static int dword_5B57A8[TRAINING_COUNT] = {
    0,
    10,
    50,
    100,
};

// 0x5B57B8
static int combat_perception_range = 15;

// 0x5B57BC
static int combat_material_damage_reduction_tbl[MATERIAL_COUNT] = {
    /*  STONE */ 10,
    /*  BRICK */ 10,
    /*   WOOD */ 5,
    /*  PLANT */ 0,
    /*  FLESH */ 0,
    /*  METAL */ 10,
    /*  GLASS */ 0,
    /*  CLOTH */ 0,
    /* LIQUID */ 0,
    /*  PAPER */ 0,
    /*    GAS */ 0,
    /*  FORCE */ 10,
    /*   FIRE */ 0,
    /* POWDER */ 0,
};

// 0x5B57FC
static int dword_5B57FC[2] = {
    0,
    10,
};

// 0x5B5804
static int combat_damage_to_resistance_tbl[DAMAGE_TYPE_COUNT] = {
    /*     DAMAGE_TYPE_NORMAL */ RESISTANCE_TYPE_NORMAL,
    /*     DAMAGE_TYPE_POISON */ RESISTANCE_TYPE_POISON,
    /* DAMAGE_TYPE_ELECTRICAL */ RESISTANCE_TYPE_ELECTRICAL,
    /*       DAMAGE_TYPE_FIRE */ RESISTANCE_TYPE_FIRE,
    /*    DAMAGE_TYPE_FATIGUE */ RESISTANCE_TYPE_NORMAL,
};

// 0x5FC178
static mes_file_handle_t combat_mes_file;

// 0x5FC180
static ObjectList combat_critter_list;

// 0x5FC1D8
static bool combat_editor;

// 0x5FC1F8
static AnimFxList combat_eye_candies;

// NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
// compatibility.
//
// 0x5FC224
static int combat_turn_based;

// 0x5FC228
static bool combat_fast_turn_based;

// NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
// compatibility.
//
// 0x5FC22C
static int combat_turn_based_active;

// 0x5FC230
static int combat_turn_based_turn;

// 0x5FC234
static int combat_action_points;

// 0x5FC238
static int64_t qword_5FC238;

// 0x5FC240
static ObjectNode* dword_5FC240;

// 0x5FC244
static int combat_required_action_points;

// 0x5FC248
static int64_t qword_5FC248;

// 0x5FC250
static int dword_5FC250;

// 0x5FC258
static int64_t qword_5FC258;

// 0x5FC260
static int dword_5FC260;

// 0x5FC264
static bool dword_5FC264;

// 0x5FC268
static bool in_combat_reset;

// 0x5FC26C
static bool dword_5FC26C;

// 0x5FC270
static int64_t qword_5FC270;

// 0x5FC1E0
static CombatCallbacks combat_callbacks;

// 0x4B1D50
bool combat_init(GameInitInfo* init_info)
{
    combat_editor = init_info->editor;

    if (!mes_load("mes\\combat.mes", &combat_mes_file)) {
        return false;
    }

    if (!combat_editor) {
        if (!animfx_list_init(&combat_eye_candies)) {
            return false;
        }

        combat_eye_candies.path = "Rules\\CombatEyeCandy.mes";
        combat_eye_candies.capacity = 1;
        if (!animfx_list_load(&combat_eye_candies)) {
            return false;
        }
    }

    settings_register(&settings, TURN_BASED_KEY, "0", turn_based_changed);
    combat_turn_based = settings_get_value(&settings, TURN_BASED_KEY);

    settings_register(&settings, FAST_TURN_BASED_KEY, "0", fast_turn_based_changed);
    combat_fast_turn_based = settings_get_value(&settings, FAST_TURN_BASED_KEY);

    settings_register(&settings, AUTO_ATTACK_KEY, "0", NULL);
    settings_register(&settings, COMBAT_TAUNTS_KEY, "0", NULL);

    return true;
}

// 0x4B1E50
void combat_exit(void)
{
    mes_unload(combat_mes_file);
    combat_turn_based_end();
    if (!combat_editor) {
        animfx_list_exit(&combat_eye_candies);
    }
}

// 0x4B1E80
void combat_reset(void)
{
    in_combat_reset = true;
    combat_turn_based_end();
    in_combat_reset = false;
}

// 0x4B1EA0
bool combat_save(TigFile* stream)
{
    ObjectID oid;

    if (stream == NULL) return false;
    if (tig_file_fwrite(&combat_turn_based, sizeof(combat_turn_based), 1, stream) != 1) return false;
    if (tig_file_fwrite(&combat_turn_based_active, sizeof(combat_turn_based_active), 1, stream) != 1) return false;
    if (tig_file_fwrite(&combat_turn_based_turn, sizeof(combat_turn_based_turn), 1, stream) != 1) return false;

    if (!combat_turn_based_active) {
        return true;
    }

    if (tig_file_fwrite(&combat_action_points, sizeof(combat_action_points), 1, stream) != 1) return false;

    if (qword_5FC238 == OBJ_HANDLE_NULL) {
        oid.type = OID_TYPE_NULL;
    } else {
        oid = obj_get_id(qword_5FC238);
    }
    if (tig_file_fwrite(&oid, sizeof(oid), 1, stream) != 1) return false;

    oid = obj_get_id(dword_5FC240->obj);
    if (tig_file_fwrite(&oid, sizeof(oid), 1, stream) != 1) return false;

    if (tig_file_fwrite(&combat_required_action_points, sizeof(combat_required_action_points), 1, stream) != 1) return false;

    oid = obj_get_id(qword_5FC248);
    if (tig_file_fwrite(&oid, sizeof(oid), 1, stream) != 1) return false;

    return true;
}

// 0x4B2020
bool combat_load(GameLoadInfo* load_info)
{
    int saved_action_points;
    ObjectID oid;
    int64_t obj;
    ObjectNode* node;

    if (load_info->stream == NULL) return false;
    if (tig_file_fread(&combat_turn_based, sizeof(combat_turn_based), 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&combat_turn_based_active, sizeof(combat_turn_based_active), 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&combat_turn_based_turn, sizeof(combat_turn_based_turn), 1, load_info->stream) != 1) return false;

    if (!combat_turn_based_active) {
        return true;
    }

    combat_callbacks.field_4();

    if (tig_file_fread(&combat_action_points, sizeof(combat_action_points), 1, load_info->stream) != 1) return false;
    saved_action_points = combat_action_points;
    combat_turn_based_active = false;

    if (!combat_turn_based_start()) {
        return false;
    }

    combat_action_points = saved_action_points;

    if (tig_file_fread(&oid, sizeof(oid), 1, load_info->stream) != 1) return false;
    qword_5FC238 = obj_pool_perm_lookup(oid);

    if (tig_file_fread(&oid, sizeof(oid), 1, load_info->stream) != 1) return false;
    obj = obj_pool_perm_lookup(oid);
    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    node = combat_critter_list.head;
    while (node != NULL) {
        if (node->obj == obj) {
            break;
        }
        node = node->next;
    }

    if (node == NULL) {
        return false;
    }

    dword_5FC240 = node;

    if (tig_file_fread(&combat_required_action_points, sizeof(combat_required_action_points), 1, load_info->stream) != 1) return false;

    combat_required_action_points = 0;

    if (tig_file_fread(&oid, sizeof(oid), 1, load_info->stream) != 1) return false;
    qword_5FC248 = obj_pool_perm_lookup(oid);
    if (qword_5FC248 == OBJ_HANDLE_NULL) {
        return false;
    }

    combat_callbacks.field_C(combat_action_points);

    return true;
}

// 0x4B2210
void sub_4B2210(int64_t attacker_obj, int64_t target_obj, CombatContext* combat)
{
    int type;

    combat->flags = 0;
    combat->attacker_obj = attacker_obj;

    if (attacker_obj != OBJ_HANDLE_NULL) {
        type = obj_field_int32_get(attacker_obj, OBJ_F_TYPE);
        if (obj_type_is_critter(type)) {
            combat->weapon_obj = combat_critter_weapon(attacker_obj);
        } else {
            combat->weapon_obj = attacker_obj;
        }
    } else {
        combat->weapon_obj = OBJ_HANDLE_NULL;
    }

    combat->skill = item_weapon_skill(combat->weapon_obj);

    if (attacker_obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(type)
        && target_obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(obj_field_int32_get(target_obj, OBJ_F_TYPE))
        && combat->skill == BASIC_SKILL_MELEE
        && critter_can_backstab(attacker_obj, target_obj)) {
        combat->flags |= 0x4000;
        if (ai_can_see(target_obj, attacker_obj) != 0
            && ai_can_hear(target_obj, attacker_obj, LOUDNESS_SILENT) != 0) {
            combat->flags |= 0x8000;
        }
    }

    combat->target_obj = target_obj;
    combat->field_28 = target_obj;

    if (target_obj != OBJ_HANDLE_NULL) {
        combat->target_loc = obj_field_int64_get(target_obj, OBJ_F_LOCATION);
    }

    combat->field_30 = attacker_obj;
    combat->hit_loc = 0;
    combat->dam_flags = 0;
    combat->total_dam = 0;
    combat->dam[DAMAGE_TYPE_NORMAL] = 0;
    combat->dam[DAMAGE_TYPE_POISON] = 0;
    combat->dam[DAMAGE_TYPE_ELECTRICAL] = 0;
    combat->dam[DAMAGE_TYPE_FIRE] = 0;
    combat->dam[DAMAGE_TYPE_FATIGUE] = 0;

    if (combat->skill == BASIC_SKILL_THROWING
        || item_weapon_range(combat->weapon_obj, combat->attacker_obj) > 1) {
        combat->flags |= CF_RANGED;
    }
}

// 0x4B23B0
int64_t combat_critter_weapon(int64_t critter_obj)
{
    return item_wield_get(critter_obj, ITEM_INV_LOC_WEAPON);
}

// 0x4B23D0
void turn_based_changed(void)
{
    int value;

    value = settings_get_value(&settings, TURN_BASED_KEY);
    if (value && tig_net_is_active()) {
        settings_set_value(&settings, TURN_BASED_KEY, 0);
    } else {
        sub_4B6C90(value);
    }
}

// 0x4B2410
void fast_turn_based_changed(void)
{
    combat_fast_turn_based = settings_get_value(&settings, FAST_TURN_BASED_KEY);
}

// 0x4B24F0
void combat_create_projectile(CombatContext* combat, int64_t loc, int a3, int a4, tig_art_id_t proj_aid)
{
    int64_t proj_obj;
    int hit_loc;
    unsigned int weapon_flags;
    unsigned int critter_flags2;

    if (!mp_object_create(BP_PROJECTILE, loc, &proj_obj)) {
        return;
    }

    obj_field_int32_set(proj_obj, OBJ_F_PROJECTILE_FLAGS_COMBAT_DAMAGE, combat->dam_flags);

    if ((combat->flags & 0x100) != 0) {
        hit_loc = combat->dam[DAMAGE_TYPE_NORMAL];
    } else {
        hit_loc = combat->hit_loc;
    }
    obj_field_int32_set(proj_obj, OBJ_F_PROJECTILE_HIT_LOC, hit_loc);

    obj_field_handle_set(proj_obj, OBJ_F_PROJECTILE_PARENT_WEAPON, combat->weapon_obj);

    if (combat->weapon_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
        weapon_flags = obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_FLAGS);
        if ((weapon_flags & OWF_TRANS_PROJECTILE) != 0) {
            obj_field_int32_set(proj_obj, OBJ_F_BLIT_FLAGS, TIG_ART_BLT_BLEND_ADD);
        }
        if ((weapon_flags & OWF_BOOMERANGS) != 0) {
            critter_flags2 = obj_field_int32_get(combat->attacker_obj, OBJ_F_CRITTER_FLAGS2);
            critter_flags2 |= OCF2_USING_BOOMERANG;
            obj_field_int32_set(combat->attacker_obj, OBJ_F_CRITTER_FLAGS2, critter_flags2);

            combat->flags |= 0x1000;
        }
    }

    obj_field_int32_set(proj_obj, OBJ_F_PROJECTILE_FLAGS_COMBAT, combat->flags);

    anim_goal_projectile(combat->attacker_obj,
        proj_obj,
        proj_aid,
        a3,
        a4,
        combat->target_obj,
        combat->target_loc,
        combat->weapon_obj);
}

// 0x4B2650
void sub_4B2650(int64_t a1, int64_t a2, CombatContext* combat)
{
    sub_4B2690(a1, a2, combat != NULL ? combat->field_28 : OBJ_HANDLE_NULL, combat, false);
}

// 0x4B2690
void sub_4B2690(int64_t proj_obj, int64_t a2, int64_t a3, CombatContext* combat, bool a5)
{
    unsigned int proj_flags;
    int64_t weapon_obj;
    int64_t loc;
    unsigned int critter_flags2;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        return;
    }

    proj_flags = obj_field_int32_get(proj_obj, OBJ_F_PROJECTILE_FLAGS_COMBAT);
    if ((proj_flags & 0x40) != 0) {
        weapon_obj = obj_field_handle_get(proj_obj, OBJ_F_PROJECTILE_PARENT_WEAPON);
        loc = obj_field_int64_get(proj_obj, OBJ_F_LOCATION);
        sub_4EDF20(weapon_obj, loc, 0, 0, false);
        object_flags_unset(weapon_obj, OF_OFF);
        object_destroy(proj_obj);
    } else if ((proj_flags & 0x1000) != 0) {
        if (a5 && (proj_flags & 0x2000) == 0) {
            proj_flags |= 0x2000;
            obj_field_int32_set(proj_obj, OBJ_F_PROJECTILE_FLAGS_COMBAT, proj_flags);
            if (!sub_435A00(proj_obj, obj_field_int64_get(a2, OBJ_F_LOCATION), a3)) {
                sub_4B2690(proj_obj, a2, a3, combat, true);
                return;
            }
        } else {
            critter_flags2 = obj_field_int32_get(a2, OBJ_F_CRITTER_FLAGS2);
            critter_flags2 &= ~OCF2_USING_BOOMERANG;
            obj_field_int32_set(a2, OBJ_F_CRITTER_FLAGS2, critter_flags2);

            object_destroy(proj_obj);
            sub_4A9AD0(a2, a3);
        }
    } else {
        object_destroy(proj_obj);
    }

    if (combat != NULL
        && combat->target_obj != OBJ_HANDLE_NULL) {
        int loudness = combat_weapon_loudness(combat->weapon_obj);
        if ((combat->flags & 0x800) != 0) {
            ai_attack(combat->attacker_obj, combat->target_obj, loudness, 0x01);
        } else {
            ai_attack(combat->attacker_obj, combat->target_obj, loudness, 0);
        }
    }
}

// 0x4B2810
int combat_weapon_loudness(int64_t weapon_obj)
{
    if (weapon_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
        unsigned int weapon_flags = obj_field_int32_get(weapon_obj, OBJ_F_WEAPON_FLAGS);
        if ((weapon_flags & OWF_LOUD) != 0) {
            return LOUDNESS_LOUD;
        }
        if ((weapon_flags & OWF_SILENT) != 0) {
            return LOUDNESS_SILENT;
        }
    }

    return LOUDNESS_NORMAL;
}

// 0x4B2870
bool sub_4B2870(int64_t attacker_obj, int64_t target_obj, int64_t target_loc, int64_t proj_obj, int range, int64_t cur_loc, int64_t a7)
{
    unsigned int proj_flags;
    unsigned int proj_dam_flags;
    int64_t weapon_obj;
    int64_t proj_loc;
    int hit_loc;
    int weapon_obj_type;
    int64_t attacker_loc;
    int64_t attacker_to_target_dist;
    int64_t proj_to_target_dist;
    int scale;
    int64_t block_obj = OBJ_HANDLE_NULL;
    int block_obj_type;
    int dam = 0;

    proj_flags = obj_field_int32_get(proj_obj, OBJ_F_PROJECTILE_FLAGS_COMBAT);
    proj_dam_flags = obj_field_int32_get(proj_obj, OBJ_F_PROJECTILE_FLAGS_COMBAT_DAMAGE);
    weapon_obj = obj_field_handle_get(proj_obj, OBJ_F_PROJECTILE_PARENT_WEAPON);
    proj_loc = obj_field_int64_get(proj_obj, OBJ_F_LOCATION);
    hit_loc = obj_field_int32_get(proj_obj, OBJ_F_PROJECTILE_HIT_LOC);

    if (weapon_obj != OBJ_HANDLE_NULL) {
        weapon_obj_type = obj_field_int32_get(weapon_obj, OBJ_F_TYPE);
    } else {
        weapon_obj_type = -1;
    }

    if (weapon_obj_type != 5) {
        attacker_loc = obj_field_int64_get(attacker_obj, OBJ_F_LOCATION);
        attacker_to_target_dist = location_dist(attacker_loc, target_loc);
        proj_to_target_dist = location_dist(proj_loc, target_loc);
        if (attacker_to_target_dist > 0
            && proj_to_target_dist > 0
            && proj_to_target_dist < attacker_to_target_dist) {
            if (attacker_to_target_dist - proj_to_target_dist * 2 > 0
                && attacker_to_target_dist - proj_to_target_dist != 0) {
                scale = (int)(200 - 150 * (attacker_to_target_dist - proj_to_target_dist) / attacker_to_target_dist);
            } else {
                scale = (int)(200 + 150 * (attacker_to_target_dist - proj_to_target_dist) / attacker_to_target_dist);
            }
        } else {
            scale = 50;
        }

        tig_debug_printf("Scaling to %d\n", scale);
        object_set_blit_scale(proj_obj, scale);
    }

    if (proj_loc == target_loc) {
        if (target_obj == OBJ_HANDLE_NULL
            || (proj_flags & 0x2000) != 0) {
            sub_4B2690(proj_obj, attacker_obj, a7, NULL, true);

            if (weapon_obj != OBJ_HANDLE_NULL) {
                object_script_execute(attacker_obj, weapon_obj, target_obj, SAP_HIT, 0);

                if ((proj_flags & 0x04) != 0
                    && weapon_obj_type == OBJ_TYPE_WEAPON) {
                    object_script_execute(attacker_obj, weapon_obj, target_obj, SAP_CRITICAL_HIT, 0);
                }

                mt_item_notify_parent_attacks_loc(attacker_obj, weapon_obj, target_loc);
            }

            return false;
        }

        unsigned int new_proj_flags = proj_flags;
        new_proj_flags &= ~0x02;
        new_proj_flags |= 0x800 | 0x20;
        obj_field_int32_set(proj_obj, OBJ_F_PROJECTILE_FLAGS_COMBAT, new_proj_flags);
    }

    if ((proj_flags & 0x2000) != 0) {
        return false;
    }

    if ((proj_flags & 0x02) != 0) {
        if (target_obj != OBJ_HANDLE_NULL
            && proj_loc == obj_field_int64_get(target_obj, OBJ_F_LOCATION)) {
            block_obj = target_obj;

            if ((proj_flags & 0x100) != 0) {
                dam = hit_loc;
                hit_loc = HIT_LOC_TORSO;
            }
        }
    } else {
        if ((proj_flags & 0x20) != 0) {
            sub_43FDC0(proj_obj,
                proj_obj,
                location_rot(proj_loc, cur_loc),
                0x08 | 0x04 | 0x01,
                &block_obj,
                &block_obj_type,
                NULL);
        }

        if (block_obj == OBJ_HANDLE_NULL) {
            ObjectList objects;
            ObjectNode* node;

            object_list_location(proj_loc, OBJ_TM_CRITTER, &objects);
            node = objects.head;
            while (node != NULL) {
                if (!critter_is_dead(node->obj)
                    && node->obj != target_obj
                    && node->obj != attacker_obj
                    && random_between(1, 3) == 1) {
                    block_obj = node->obj;
                    break;
                }
                node = node->next;
            }
            object_list_destroy(&objects);
        }

        if (block_obj != OBJ_HANDLE_NULL) {
            proj_dam_flags = 0;

            proj_flags &= ~0x04;
            proj_flags |= 0x800 | 0x02;

            dam = (proj_flags & 0x100) != 0 ? hit_loc : 0;
            hit_loc = HIT_LOC_TORSO;
        }
    }

    if (block_obj != OBJ_HANDLE_NULL) {
        CombatContext combat;

        sub_4B2210(attacker_obj, block_obj, &combat);

        if (target_obj != OBJ_HANDLE_NULL) {
            combat.field_28 = target_obj;
        }

        combat.flags = proj_flags | 0x40000 | CF_RANGED;
        combat.dam_flags = proj_dam_flags;
        combat.dam[DAMAGE_TYPE_NORMAL] = dam;
        combat.hit_loc = hit_loc;
        combat.weapon_obj = weapon_obj;
        combat.skill = item_weapon_skill(weapon_obj);
        combat.flags &= ~0xC000;
        sub_4B2690(proj_obj, attacker_obj, target_obj, &combat, 1);
        sub_4B2F60(&combat);
        combat_process_taunts(&combat);
        mt_item_notify_parent_attacks_loc(attacker_obj,
            weapon_obj,
            obj_field_int64_get(block_obj, OBJ_F_LOCATION));
        return combat_play_hit_fx(&combat);
    }

    int weapon_range;
    if ((proj_flags & 0x100) != 0) {
        weapon_range = 20;
    } else if ((proj_flags & 0x40) != 0) {
        weapon_range = item_throwing_distance(weapon_obj, attacker_obj);
    } else {
        weapon_range = item_weapon_range(weapon_obj, attacker_obj);
    }

    if (range > weapon_range) {
        if (proj_obj != OBJ_HANDLE_NULL) {
            proj_loc = obj_field_int64_get(proj_obj, OBJ_F_LOCATION);
        }

        sub_4B2690(proj_obj, attacker_obj, target_obj, NULL, true);

        if (weapon_obj != OBJ_HANDLE_NULL) {
            object_script_execute(attacker_obj, weapon_obj, target_obj, SAP_MISS, 0);

            if ((proj_flags & 0x04) != 0
                && weapon_obj_type == OBJ_TYPE_WEAPON) {
                object_script_execute(attacker_obj, weapon_obj, target_obj, SAP_CRITICAL_MISS, 0);
            }

            mt_item_notify_parent_attacks_loc(attacker_obj, weapon_obj, proj_loc);
        }
    }

    return false;
}

// 0x4B2F60
void sub_4B2F60(CombatContext* combat)
{
    int sound_id;

    if ((combat->flags & CF_HIT) != 0) {
        if (combat->weapon_obj != OBJ_HANDLE_NULL) {
            object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_HIT, 0);

            if ((combat->flags & CF_CRITICAL) != 0
                && obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
                object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_CRITICAL_HIT, 0);
            }
        }

        if (combat->attacker_obj != OBJ_HANDLE_NULL
            && combat->target_obj != OBJ_HANDLE_NULL) {
            object_script_execute(combat->target_obj, combat->attacker_obj, OBJ_HANDLE_NULL, SAP_CRITTER_HITS, 0);
        }

        combat_calc_dmg(combat);
        combat_dmg(combat);

        if ((combat->flags & CF_CRITICAL) != 0) {
            sound_id = sfx_critter_sound(combat->target_obj, CRITTER_SOUND_CRITICALLY_HIT);
            gsound_play_sfx_on_obj(sound_id, 1, combat->target_obj);

            sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_CRITICAL_HIT);
            gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);
        } else {
            sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_HIT);
            gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);
        }
    } else {
        sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_MISS);
        gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);

        if (combat->target_obj != OBJ_HANDLE_NULL
            && obj_field_int32_get(combat->target_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            ai_process(combat->target_obj);
        }

        if (combat->weapon_obj != OBJ_HANDLE_NULL) {
            object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_MISS, 0);

            if ((combat->flags & CF_CRITICAL) != 0
                && obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
                object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_CRITICAL_MISS, 0);
            }
        }
    }
}

// 0x4B3170
int sub_4B3170(CombatContext* combat)
{
    bool was_in_bed = false;
    int sound_id;
    int aptitude_crit_failure_chance;

    if (combat->target_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(combat->target_obj, OBJ_F_TYPE) == OBJ_TYPE_SCENERY) {
        int64_t critter_obj = obj_field_handle_get(combat->target_obj, OBJ_F_SCENERY_WHOS_IN_ME);
        if (critter_obj != OBJ_HANDLE_NULL) {
            critter_leave_bed(critter_obj, combat->target_obj);

            combat->target_obj = critter_obj;
            combat->field_28 = critter_obj;

            was_in_bed = true;
        }
    }

    if (combat->hit_loc != HIT_LOC_TORSO) {
        combat->flags |= CF_AIM;
    } else {
        combat->hit_loc = combat_random_hit_loc();
    }

    if (!combat_consume_ammo(combat->weapon_obj, combat->attacker_obj, 1, false)) {
        combat->flags |= 0x400;

        sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_OUT_OF_AMMO);
        gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);

        if (obj_field_int32_get(combat->attacker_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            ai_switch_weapon(combat->attacker_obj);
        } else {
            pc_switch_weapon(combat->attacker_obj, combat->target_obj);
        }

        return 0;
    }

    sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_USE);
    gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);

    bool is_melee = true;
    if ((combat->flags & CF_RANGED) != 0
        && (combat->skill == SKILL_THROWING
            || obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_MISSILE_AID) != -1)) {
        is_melee = false;
    }

    if (combat->target_obj != combat->field_28) {
        combat->flags |= 0x800;
    }

    if (is_melee && combat->target_obj != OBJ_HANDLE_NULL) {
        int loudness = combat_weapon_loudness(combat->weapon_obj);
        if ((combat->flags & 0x800) != 0) {
            ai_attack(combat->attacker_obj, combat->target_obj, loudness, 1);
        } else {
            ai_attack(combat->attacker_obj, combat->target_obj, loudness, 0);
        }
    }

    if (combat->weapon_obj != OBJ_HANDLE_NULL && combat->skill != SKILL_THROWING) {
        aptitude_crit_failure_chance = item_aptitude_crit_failure_chance(combat->weapon_obj, combat->attacker_obj);
    } else {
        aptitude_crit_failure_chance = 0;
    }

    if (random_between(1, 100) <= aptitude_crit_failure_chance) {
        combat->flags |= CF_CRITICAL;
    } else if ((combat->flags & 0x100) != 0) {
        combat->flags |= CF_HIT;
    } else {
        if ((combat->flags & CF_RANGED) != 0) {
            int64_t blocking_obj;

            sub_4ADE00(combat->attacker_obj, combat->target_loc, &blocking_obj);
            if (blocking_obj != OBJ_HANDLE_NULL) {
                combat->target_obj = blocking_obj;
            }
        }

        int v3 = 0;
        if (combat->weapon_obj != OBJ_HANDLE_NULL && combat->skill != SKILL_THROWING) {
            v3 = sub_461620(combat->weapon_obj, combat->attacker_obj, combat->target_obj);
        }

        bool cont = true;
        if (random_between(1, 100) <= v3) {
            if (combat->skill == SKILL_MELEE
                && combat->weapon_obj != OBJ_HANDLE_NULL
                && obj_field_int32_get(combat->weapon_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) > 0) {
                combat->flags |= 0x20000;
            } else {
                cont = false;
            }
        }

        if (cont) {
            if (combat->target_obj != OBJ_HANDLE_NULL
                && !obj_type_is_critter(obj_field_int32_get(combat->target_obj, OBJ_F_TYPE))) {
                combat->flags |= CF_HIT;
                cont = false;
            }
        }

        if (cont) {
            SkillInvocation skill_invocation;

            skill_invocation_init(&skill_invocation);
            sub_4440E0(combat->attacker_obj, &(skill_invocation.source));
            sub_4440E0(combat->target_obj, &(skill_invocation.target));
            skill_invocation.target_loc = combat->target_loc;
            sub_4440E0(combat->weapon_obj, &(skill_invocation.item));
            skill_invocation.hit_loc = combat->hit_loc;
            skill_invocation.skill = combat->skill;

            if ((combat->flags & CF_AIM) != 0) {
                skill_invocation.flags |= SKILL_INVOCATION_AIM;
            }

            if ((combat->flags & 0x8000) != 0) {
                skill_invocation.flags |= SKILL_INVOCATION_BACKSTAB;
            }

            if ((combat->flags & 0x20000) != 0) {
                skill_invocation.flags |= SKILL_INVOCATION_NO_MAGIC_ADJ;
            }

            skill_invocation.modifier = 0;

            if (was_in_bed) {
                skill_invocation.modifier -= 30;
            }

            if (!skill_invocation_run(&skill_invocation)) {
                return 0;
            }

            if ((skill_invocation.flags & SKILL_INVOCATION_SUCCESS) != 0) {
                combat->flags |= CF_HIT;
            }

            if ((skill_invocation.flags & SKILL_INVOCATION_CRITICAL) != 0) {
                combat->flags |= CF_CRITICAL;
            }

            if (combat->target_obj != OBJ_HANDLE_NULL
                && (combat->flags & CF_HIT) != 0) {
                skill_invocation_init(&skill_invocation);
                sub_4440E0(combat->target_obj, &(skill_invocation.source));
                skill_invocation.skill = SKILL_DODGE;
                if (!skill_invocation_run(&skill_invocation)) {
                    return 0;
                }

                if ((skill_invocation.flags & SKILL_INVOCATION_SUCCESS) != 0) {
                    MesFileEntry mes_file_entry;

                    mes_file_entry.num = 11; // "Dodge!"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, 0, mes_file_entry.str);

                    combat->flags &= ~(CF_HIT | CF_CRITICAL);

                    if ((skill_invocation.flags & SKILL_INVOCATION_CRITICAL) != 0) {
                        int training = basic_skill_training_get(combat->target_obj, BASIC_SKILL_DODGE);
                        if (random_between(1, 100) <= dword_5B57A8[training]) {
                            combat->flags |= CF_CRITICAL;
                        }
                    }
                }
            }
        }
    }

    if (combat->attacker_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(combat->attacker_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        if (fate_resolve(combat->attacker_obj, FATE_CRIT_HIT)) {
            combat->flags |= CF_HIT | CF_CRITICAL;
        }
    } else if (combat->target_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(combat->target_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        if (fate_resolve(combat->target_obj, FATE_CRIT_MISS)) {
            combat->flags &= ~CF_HIT;
            combat->flags |= CF_CRITICAL;
        }
    }

    if ((combat->flags & CF_HIT) == 0
        && (combat->flags & CF_CRITICAL) != 0) {
        combat_process_crit_miss(combat);
    }

    if ((combat->flags & CF_RANGED) != 0) {
        combat_process_ranged_attack(combat);
    } else {
        combat_process_melee_attack(combat);
    }

    anim_play_weapon_fx(combat, combat->attacker_obj, combat->attacker_obj, ANIM_WEAPON_EYE_CANDY_TYPE_FIRE);

    if (is_melee) {
        combat_process_taunts(combat);
        return combat_play_hit_fx(combat);
    }

    return 0;
}

// 0x4B3770
void combat_process_melee_attack(CombatContext* combat)
{
    int sound_id;
    CombatContext body_of_fire;

    if ((combat->flags & CF_HIT) != 0) {
        if (combat->weapon_obj != OBJ_HANDLE_NULL) {
            object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_HIT, 0);

            if ((combat->flags & CF_CRITICAL) != 0) {
                object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_CRITICAL_HIT, 0);
            }
        }

        if (combat->attacker_obj != OBJ_HANDLE_NULL
            && combat->target_obj != OBJ_HANDLE_NULL) {
            object_script_execute(combat->target_obj, combat->attacker_obj, OBJ_HANDLE_NULL, SAP_CRITTER_HITS, 0);
        }

        combat_calc_dmg(combat);
        combat_dmg(combat);

        if ((combat->flags & CF_CRITICAL) != 0) {
            sound_id = sfx_critter_sound(combat->target_obj, CRITTER_SOUND_CRITICALLY_HIT);
            gsound_play_sfx_on_obj(sound_id, 1, combat->target_obj);

            sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_CRITICAL_HIT);
            gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);
        } else {
            sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_HIT);
            gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);
        }

        if ((obj_field_int32_get(combat->attacker_obj, OBJ_F_SPELL_FLAGS) & OSF_BODY_OF_FIRE) == 0
            && (obj_field_int32_get(combat->target_obj, OBJ_F_SPELL_FLAGS) & OSF_BODY_OF_FIRE) != 0) {
            sub_4B2210(combat->target_obj, combat->attacker_obj, &body_of_fire);
            body_of_fire.dam[DAMAGE_TYPE_FIRE] = 5;
            combat_dmg(&body_of_fire);
        }
    } else {
        sound_id = sfx_item_sound(combat->weapon_obj, combat->attacker_obj, combat->target_obj, WEAPON_SOUND_MISS);
        gsound_play_sfx_on_obj(sound_id, 1, combat->attacker_obj);

        if (obj_field_int32_get(combat->target_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            ai_process(combat->target_obj);
        }

        if (combat->weapon_obj != OBJ_HANDLE_NULL) {
            object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_MISS, 0);

            if ((combat->flags & CF_CRITICAL) != 0) {
                object_script_execute(combat->attacker_obj, combat->weapon_obj, combat->target_obj, SAP_CRITICAL_MISS, 0);
            }
        }
    }
}

// 0x4B39B0
void combat_process_ranged_attack(CombatContext* combat)
{
    bool is_boomerangs = false;
    tig_art_id_t missile_art_id = TIG_ART_ID_INVALID;
    int64_t loc;
    int range;
    int64_t max_range;
    int rotation;
    int num_arrows;
    int arrow;

    if ((combat->flags & 0x100) == 0
        && combat->weapon_obj != OBJ_HANDLE_NULL) {
        if (obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
            missile_art_id = obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_MISSILE_AID);
            is_boomerangs = (obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_FLAGS) & OWF_BOOMERANGS) != 0;
        }
    } else {
        tig_art_scenery_id_create(0, 0, 0, 0, 0, &missile_art_id);
    }

    if (missile_art_id == TIG_ART_ID_INVALID) {
        if (combat->skill == BASIC_SKILL_THROWING) {
            missile_art_id = obj_field_int32_get(combat->weapon_obj, OBJ_F_CURRENT_AID);
        }
    }

    if (missile_art_id == TIG_ART_ID_INVALID) {
        sub_4B2F60(combat);
        return;
    }

    loc = obj_field_int64_get(combat->attacker_obj, OBJ_F_LOCATION);
    if (combat->skill == BASIC_SKILL_THROWING
        && !is_boomerangs
        && (combat->flags & CF_HIT) == 0) {
        range = (20 - basic_skill_level(combat->attacker_obj, BASIC_SKILL_THROWING)) / 5 + 1;
        max_range = location_dist(loc, combat->target_loc);
        if (range > max_range) {
            range = (int)max_range;
        }

        location_in_range(combat->target_loc, random_between(0, 7), range, &(combat->target_loc));
        combat->target_obj = OBJ_HANDLE_NULL;
        combat->flags |= 0x20;
    }

    if (multiplayer_is_locked() || tig_net_is_host()) {
        // FIXME: Unused.
        rotation = tig_art_id_rotation_get(obj_field_int32_get(combat->attacker_obj, OBJ_F_CURRENT_AID));

        num_arrows = 1;
        if (combat->skill == BASIC_SKILL_BOW
            && basic_skill_training_get(combat->attacker_obj, BASIC_SKILL_BOW) >= TRAINING_EXPERT) {
            num_arrows = 2;
        }

        for (arrow = 0; arrow < num_arrows; arrow++) {
            combat_create_projectile(combat, loc, dword_5B57FC[arrow], dword_5B57FC[arrow], missile_art_id);
        }
    }
}

// 0x4B3BB0
void sub_4B3BB0(int64_t attacker_obj, int64_t target_obj, int hit_loc)
{
    CombatContext combat;

    sub_4B2210(attacker_obj, target_obj, &combat);
    combat.hit_loc = hit_loc;
    combat.flags |= CF_WEAPON_WEAR;
    combat.flags |= 0x40000;
    sub_4B3170(&combat);
}

// 0x4B3C00
void combat_throw(int64_t attacker_obj, int64_t weapon_obj, int64_t target_obj, int64_t target_loc, int hit_loc)
{
    int64_t attacker_loc;
    CombatContext combat;

    attacker_loc = obj_field_int64_get(attacker_obj, OBJ_F_LOCATION);
    if (obj_field_int32_get(attacker_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        item_remove(weapon_obj);
    }

    if (object_script_execute(attacker_obj, weapon_obj, target_obj, SAP_THROW, 0)) {
        sub_4B2210(attacker_obj, target_obj, &combat);
        combat.hit_loc = hit_loc;
        combat.weapon_obj = weapon_obj;
        if (target_obj == OBJ_HANDLE_NULL) {
            combat.target_loc = target_loc;
        }
        combat.flags &= ~0xC000;
        combat.flags |= 0x4000;
        combat.flags |= CF_RANGED;
        combat.flags |= 0x40;
        object_flags_set(weapon_obj, OF_OFF);
        object_drop(weapon_obj, attacker_loc);
        sub_4B3170(&combat);
    } else {
        object_drop(weapon_obj, attacker_loc);
        item_transfer(weapon_obj, attacker_obj);
    }
}

// 0x4B3D50
bool combat_critter_is_combat_mode_active(int64_t obj)
{
    if (obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return (obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & OCF_COMBAT_MODE_ACTIVE) != 0;
    } else {
        return false;
    }
}

// 0x4B3D90
bool combat_can_exit_combat_mode(int64_t obj)
{
    bool ret = true;
    ObjectList pcs;
    ObjectNode* pc_node;
    ObjectList npcs;
    ObjectNode* npc_node;
    int64_t combat_focus_obj;

    if (!combat_critter_is_combat_mode_active(obj)) {
        return true;
    }

    if (!critter_is_active(obj)) {
        return true;
    }

    if (anim_is_current_goal_type(obj, AG_ATTEMPT_ATTACK, NULL)) {
        return false;
    }

    object_list_all_followers(obj, &pcs);
    object_list_vicinity(obj, OBJ_TM_NPC, &npcs);

    npc_node = npcs.head;
    while (npc_node != NULL && ret) {
        combat_focus_obj = obj_field_handle_get(npc_node->obj, OBJ_F_NPC_COMBAT_FOCUS);
        if (combat_focus_obj == obj) {
            if (critter_is_active(npc_node->obj) && ai_is_fighting(npc_node->obj)) {
                ret = false;
                break;
            }
        } else {
            pc_node = pcs.head;
            while (pc_node != NULL) {
                if (combat_focus_obj == pc_node->obj
                    && critter_is_active(npc_node->obj)
                    && ai_is_fighting(npc_node->obj)) {
                    ret = false;
                    break;
                }
                pc_node = pc_node->next;
            }
        }
        npc_node = npc_node->next;
    }

    object_list_destroy(&npcs);
    object_list_destroy(&pcs);

    return ret;
}

// 0x4B3F20
void combat_critter_deactivate_combat_mode(int64_t obj)
{
    if (combat_critter_is_combat_mode_active(obj)) {
        combat_critter_toggle_combat_mode(obj);
    }
}

// 0x4B3F50
void combat_critter_activate_combat_mode(int64_t obj)
{
    if (!combat_critter_is_combat_mode_active(obj)) {
        combat_critter_toggle_combat_mode(obj);
    }
}

// 0x4B3F80
void combat_critter_toggle_combat_mode(int64_t obj)
{
    bool v1 = false;
    bool combat_mode_is_active;
    bool is_pc;
    int obj_type;
    bool is_tb;
    tig_art_id_t art_id;
    TigArtAnimData art_anim_data;

    is_pc = player_is_local_pc_obj(obj);
    combat_mode_is_active = combat_critter_is_combat_mode_active(obj);
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);

    if (!obj_type_is_critter(obj_type) || !critter_is_dead(obj)) {
        if ((obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & OSF_STONED) != 0) {
            return;
        }

        if (obj_type_is_critter(obj_type)) {
            unsigned int critter_flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
            if ((critter_flags & OCF_PARALYZED) != 0) {
                return;
            }

            if (combat_mode_is_active) {
                if ((critter_flags & OCF_STUNNED) != 0) {
                    return;
                }
            } else {
                if (!critter_is_active(obj)) {
                    return;
                }
            }
        }
    }

    if (!multiplayer_is_locked()) {
        if (tig_net_is_host() || is_pc) {
            PacketCombatModeSet pkt;

            pkt.type = 19;
            pkt.oid = obj_get_id(obj);
            pkt.active = combat_mode_is_active;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }

        if (!tig_net_is_host()) {
            return;
        }
    }

    is_tb = combat_is_turn_based();

    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);

    if (tig_art_id_anim_get(art_id) == TIG_ART_ANIM_CONCEAL_FIDGET) {
        v1 = true;
        if (!is_pc && !critter_is_dead(obj)) {
            return;
        }
    }

    unsigned int critter_flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
    unsigned int critter_flags2 = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2);

    if (combat_mode_is_active) {
        obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, critter_flags & ~OCF_COMBAT_MODE_ACTIVE);
        ai_target_unlock(obj);

        int anim = tig_art_id_anim_get(art_id);
        if (anim == TIG_ART_ANIM_ATTACK || anim == TIG_ART_ANIM_ATTACK_LOW) {
            art_id = tig_art_id_anim_set(art_id, TIG_ART_ANIM_STAND);
        }

        art_id = tig_art_critter_id_weapon_set(art_id, TIG_ART_WEAPON_TYPE_NO_WEAPON);
        art_id = tig_art_critter_id_shield_set(art_id, 0);

        if (is_pc) {
            if (is_tb) {
                combat_callbacks.field_8();
                combat_turn_based_end();
            }
            gsound_stop_combat_music(obj);
        }

        if ((obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & OSF_INVISIBLE) != 0
            && !player_is_local_pc_obj(obj)) {
            object_flags_set(obj, OF_INVISIBLE);
        }
    } else {
        obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, critter_flags | OCF_COMBAT_MODE_ACTIVE);

        art_id = tig_art_critter_id_weapon_set(art_id, TIG_ART_WEAPON_TYPE_UNARMED);

        if (is_pc && is_tb) {
            sub_40FED0();

            if (!combat_turn_based_start()) {
                return;
            }

            combat_callbacks.field_4();

            if (combat_turn_based_active) {
                combat_turn_based_whos_turn_set(obj);
            }
        }

        gsound_start_combat_music(obj);
    }

    if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
        tig_debug_printf("Combat: combat_critter_toggle_combat_mode: Failed to grab art: %u!\n", art_id);
        obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        return;
    }

    if (tig_art_id_frame_get(art_id) >= art_anim_data.num_frames) {
        art_id = tig_art_id_frame_set(art_id, 0);
    }

    object_set_current_aid(obj, art_id);
    object_set_current_aid(obj, sub_465020(obj));

    if (combat_mode_is_active) {
        if (is_pc) {
            combat_callbacks.field_0(0);
        }

        if (!v1 && anim_is_fidgeting(obj)) {
            sub_424070(obj, 3, 0, 0);
        }
    } else {
        if (is_pc) {
            combat_callbacks.field_0(1);
            sub_4AA580(obj);
        }

        if (!v1 && !is_tb) {
            anim_goal_fidget(obj);
        }
    }

    if ((critter_flags2 & OCF2_COMBAT_TOGGLE_FX) != 0) {
        magictech_fx_add(obj, MAGICTECH_FX_COMBAT_TOGGLE);
    }
}

// 0x4B4320
void sub_4B4320(int64_t obj)
{
    if (obj != OBJ_HANDLE_NULL
        && (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
            || obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC)
        && ((!combat_turn_based_active && combat_critter_is_combat_mode_active(obj))
            || (obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2) & OCF2_AUTO_ANIMATES) != 0)) {
        anim_goal_fidget(obj);
    }
}

// 0x4B4390
void combat_dmg(CombatContext* combat)
{
    int obj_type;
    unsigned int dam_flags;
    int dam;
    MesFileEntry mes_file_entry;
    char str[80];
    unsigned int spell_flags = 0;
    bool weapon_dropped = false;

    if (!multiplayer_is_locked()) {
        if (sub_4B5520(combat) || !tig_net_is_host()) {
            return;
        }
    }

    if (combat->target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    obj_type = obj_field_int32_get(combat->target_obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)) {
        unsigned int critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        spell_flags = obj_field_int32_get(combat->target_obj, OBJ_F_SPELL_FLAGS);

        if ((critter_flags & (OCF_UNDEAD | OCF_MECHANICAL)) != 0) {
            combat->dam[DAMAGE_TYPE_POISON] = 0;
        } else if ((spell_flags & OSF_STONED) != 0) {
            combat->dam[DAMAGE_TYPE_NORMAL] = 0;
            combat->dam[DAMAGE_TYPE_POISON] = 0;
            combat->dam[DAMAGE_TYPE_ELECTRICAL] = 0;
            combat->dam[DAMAGE_TYPE_FIRE] = 0;
            combat->dam[DAMAGE_TYPE_FATIGUE] = 0;
        }
    }

    if (!dword_5FC26C) {
        bool def;

        dword_5FC26C = true;
        def = object_script_execute(combat->attacker_obj, combat->target_obj, OBJ_HANDLE_NULL, SAP_TAKING_DAMAGE, 0);
        dword_5FC26C = false;

        if (!def) {
            return;
        }
    }

    if ((obj_field_int32_get(combat->target_obj, OBJ_F_FLAGS) & (OF_DESTROYED | OF_INVULNERABLE)) != 0) {
        return;
    }

    if (obj_type_is_critter(obj_type)
        && (obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS2) & OCF2_NIGH_INVULNERABLE) != 0
        && (combat->weapon_obj == OBJ_HANDLE_NULL
            || (obj_field_int32_get(combat->weapon_obj, OBJ_F_ITEM_FLAGS) & OIF_UBER) == 0)) {
        combat->dam[DAMAGE_TYPE_NORMAL] = 0;
        combat->dam[DAMAGE_TYPE_POISON] = 0;
        combat->dam[DAMAGE_TYPE_ELECTRICAL] = 0;
        combat->dam[DAMAGE_TYPE_FIRE] = 0;
        combat->dam[DAMAGE_TYPE_FATIGUE] = 0;
        return;
    }

    if (obj_type == OBJ_TYPE_WALL) {
        return;
    }

    if ((combat->flags & CF_CRITICAL) != 0) {
        combat_process_crit_hit(combat);
    }

    combat_apply_resistance(combat);

    dam_flags = combat->dam_flags;
    if ((dam_flags & (CDF_FULL | CDF_DEATH)) != 0) {
        dam = object_hp_current(combat->target_obj);
    } else {
        dam = combat->dam[DAMAGE_TYPE_NORMAL]
            + combat->dam[DAMAGE_TYPE_ELECTRICAL]
            + combat->dam[DAMAGE_TYPE_FIRE];
    }

    if (combat->dam[DAMAGE_TYPE_FIRE] > 0) {
        dam_flags |= CDF_DAMAGE_ARMOR;
    }

    if (combat->weapon_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON
        && (obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_FLAGS) & OWF_DAMAGE_ARMOR) != 0) {
        dam_flags |= CDF_DAMAGE_ARMOR;
    }

    if (player_is_pc_obj(combat->attacker_obj)) {
        if (!tig_net_is_active()
            || tig_net_is_host()) {
            combat->game_difficulty = gamelib_game_difficulty_get();
        }

        switch (combat->game_difficulty) {
        case GAME_DIFFICULTY_EASY:
            dam += dam / 2;
            break;
        case GAME_DIFFICULTY_HARD:
            dam -= dam / 4;
            break;
        }
    }

    if ((dam_flags & CDF_BONUS_DAM_200) != 0) {
        dam *= 3;
    } else if ((dam_flags & CDF_BONUS_DAM_100) != 0) {
        dam *= 2;
    } else if ((dam_flags & CDF_BONUS_DAM_50) != 0) {
        dam += dam / 2;
    }

    if (obj_type_is_critter(obj_type)) {
        // 0x4B475F
        if ((combat->flags & CF_TRAP) != 0) {
            combat->flags |= CF_HIT;
            if (!tig_net_is_active()
                || tig_net_is_host()) {
                combat_play_hit_fx(combat);
            }
        }

        if (critter_is_dead(combat->target_obj)) {
            return;
        }

        if ((combat->flags & CF_CRITICAL) != 0) {
            int inventory_location;
            int64_t item_obj;

            for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
                item_obj = item_wield_get(combat->target_obj, inventory_location);
                if (item_obj != OBJ_HANDLE_NULL
                    && obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_ARMOR) {
                    object_script_execute(combat->attacker_obj, item_obj, combat->target_obj, SAP_TAKING_DAMAGE, 0);
                }
            }
        }

        int tf_type;
        if (obj_type == OBJ_TYPE_PC) {
            tf_type = TF_TYPE_RED;
        } else if (obj_type == OBJ_TYPE_NPC
            && critter_pc_leader_get(combat->target_obj) == player_get_local_pc_obj()) {
            tf_type = TF_TYPE_YELLOW;
        } else {
            tf_type = TF_TYPE_WHITE;
        }

        if ((combat->flags & 0x18) != 0) {
            mes_file_entry.num = 2; // "Critical miss"
            mes_get_msg(combat_mes_file, &mes_file_entry);
            tf_add(combat->attacker_obj, tf_type, mes_file_entry.str);
        } else if ((combat->flags & CF_CRITICAL) != 0) {
            mes_file_entry.num = 12; // "Critical hit"
            mes_get_msg(combat_mes_file, &mes_file_entry);
            tf_add(combat->target_obj, tf_type, mes_file_entry.str);
        }

        if ((dam_flags & CDF_BONUS_DAM_200) != 0) {
            mes_file_entry.num = 15; // "Damage +200%"
            mes_get_msg(combat_mes_file, &mes_file_entry);
            tf_add(combat->target_obj, tf_type, mes_file_entry.str);
        } else if ((dam_flags & CDF_BONUS_DAM_100) != 0) {
            mes_file_entry.num = 14; // "Damage +100%"
            mes_get_msg(combat_mes_file, &mes_file_entry);
            tf_add(combat->target_obj, tf_type, mes_file_entry.str);
        } else if ((dam_flags & CDF_BONUS_DAM_50) != 0) {
            mes_file_entry.num = 13; // "Damage +50%"
            mes_get_msg(combat_mes_file, &mes_file_entry);
            tf_add(combat->target_obj, tf_type, mes_file_entry.str);
        }

        unsigned int critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        if ((dam_flags & CDF_STUN) != 0
            && (critter_flags & OCF_STUNNED) == 0
            && (dam_flags & (CDF_KNOCKDOWN | CDF_KNOCKOUT)) == 0) {
            mt_item_notify_parent_stunned(combat->attacker_obj, combat->target_obj);
            critter_flags |= OCF_STUNNED;

            if ((!tig_net_is_active()
                    || tig_net_is_host())
                && !anim_goal_animate_stunned(combat->target_obj)) {
                critter_flags &= ~OCF_STUNNED;
            }

            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);

            mes_file_entry.num = 16; // "Stunned"
            mes_get_msg(combat_mes_file, &mes_file_entry);
            tf_add(combat->target_obj, tf_type, mes_file_entry.str);
        }

        if ((dam_flags & (CDF_KNOCKDOWN | CDF_KNOCKOUT)) != 0) {
            mt_item_notify_parent_going_unconscious(combat->attacker_obj, combat->target_obj);
            mt_item_notify_target_going_unconscious(combat->attacker_obj, combat->target_obj);
            anim_goal_knockdown(combat->target_obj);

            if ((dam_flags & CDF_KNOCKOUT) != 0) {
                if (!critter_is_unconscious(combat->target_obj)) {
                    int max_fatigue = critter_fatigue_max(combat->target_obj);

                    if (!tig_net_is_active()
                        || tig_net_is_host()) {
                        combat->field_64 = random_between(10, 20);
                    }

                    critter_fatigue_damage_set(combat->target_obj, combat->field_64 + max_fatigue);

                    mes_file_entry.num = 17; // "Unconscious"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);
                }
            } else {
                mes_file_entry.num = 18; // "Knocked down"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);
            }
        }

        if ((dam_flags & CDF_SCAR) != 0) {
            effect_add(combat->target_obj, EFFECT_SCARRING, EFFECT_CAUSE_INJURY);
            logbook_add_injury(combat->target_obj, combat->attacker_obj, LBI_SCARRED);

            mes_file_entry.num = 3; // "Scarred"
            mes_get_msg(combat_mes_file, &mes_file_entry);
            tf_add(combat->target_obj, tf_type, mes_file_entry.str);
        }

        if ((dam_flags & CDF_BLIND) != 0) {
            if ((critter_flags & OCF_BLINDED) == 0) {
                critter_flags |= OCF_BLINDED;
                obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
                logbook_add_injury(combat->target_obj, combat->attacker_obj, LBI_BLINDED);

                mes_file_entry.num = 4; // "Blinded"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);
            }
        }

        if ((dam_flags & CDF_CRIPPLE_ARM) != 0) {
            if ((critter_flags & OCF_CRIPPLED_ARMS_ONE) == 0) {
                critter_flags |= OCF_CRIPPLED_ARMS_ONE;
                obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
                logbook_add_injury(combat->target_obj, combat->attacker_obj, LBI_CRIPPLED_ARM);

                mes_file_entry.num = 5; // "Crippled arm"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);
            } else if ((critter_flags & OCF_CRIPPLED_ARMS_BOTH) == 0
                && random_between(1, 100) <= 50) {
                critter_flags |= OCF_CRIPPLED_ARMS_BOTH;
                obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
                logbook_add_injury(combat->target_obj, combat->attacker_obj, LBI_CRIPPLED_ARM);

                mes_file_entry.num = 5; // "Crippled arm"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);
            }

            int64_t weapon_obj = item_wield_get(combat->target_obj, ITEM_INV_LOC_WEAPON);
            if (weapon_obj != OBJ_HANDLE_NULL
                && sub_464D20(weapon_obj, ITEM_INV_LOC_WEAPON, combat->target_obj)) {
                if (!tig_net_is_active()
                    || tig_net_is_host()) {
                    item_drop_nearby(weapon_obj);
                }

                mes_file_entry.num = 19; // "Weapon dropped"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);

                weapon_dropped = true;
            }

            int64_t shield_obj = item_wield_get(combat->target_obj, ITEM_INV_LOC_SHIELD);
            if (shield_obj != OBJ_HANDLE_NULL
                && sub_464D20(shield_obj, ITEM_INV_LOC_SHIELD, combat->target_obj)) {
                if (!tig_net_is_active()
                    || tig_net_is_host()) {
                    item_drop_nearby(weapon_obj);
                }

                mes_file_entry.num = 20; // "Item dropped"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);
            }
        }

        if ((dam_flags & CDF_CRIPPLE_LEG) != 0) {
            if ((critter_flags & OCF_CRIPPLED_LEGS_BOTH) == 0) {
                critter_flags |= OCF_CRIPPLED_LEGS_BOTH;
                obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
                logbook_add_injury(combat->target_obj, combat->attacker_obj, LBI_CRIPPLED_LEG);

                mes_file_entry.num = 6; // "Crippled leg"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);
            }
        }

        if ((dam_flags & CDF_DAMAGE_ARMOR) != 0) {
            int64_t armor_obj = combat_critter_armor(combat->target_obj, combat->hit_loc);
            if (armor_obj != OBJ_HANDLE_NULL) {
                object_hp_damage_set(armor_obj, object_hp_damage_get(armor_obj) + 10);

                if (object_hp_current(armor_obj) > 0) {
                    mes_file_entry.num = 7; // "Armor damaged"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);
                } else {
                    object_bust(armor_obj, combat->attacker_obj);

                    mes_file_entry.num = 24; // "Armor broken"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);
                }
            }
        }

        if ((dam_flags & CDF_DROP_HELMET) != 0) {
            int64_t helmet_obj = item_wield_get(combat->target_obj, ITEM_INV_LOC_HELMET);
            if (helmet_obj != OBJ_HANDLE_NULL) {
                item_drop_nearby(helmet_obj);

                mes_file_entry.num = 20; // "Item dropped"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);
            }
        }

        int64_t weapon_obj = combat_critter_weapon(combat->target_obj);
        if (weapon_obj != OBJ_HANDLE_NULL) {
            if ((obj_field_int32_get(weapon_obj, OBJ_F_FLAGS) & OF_INVULNERABLE) != 0) {
                dam_flags &= ~(CDF_EXPLODE_WEAPON | CDF_DESTROY_WEAPON | CDF_DAMAGE_WEAPON);
            }

            if ((dam_flags & CDF_DAMAGE_WEAPON) != 0) {
                object_hp_damage_set(weapon_obj, object_hp_damage_get(weapon_obj) + 10);

                if (object_hp_current(weapon_obj) > 0) {
                    mes_file_entry.num = 8; // "Weapon damaged"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);
                } else {
                    object_bust(weapon_obj, combat->attacker_obj);

                    mes_file_entry.num = 23; // "Weapon broken"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);

                    weapon_dropped = true;
                }
            }

            if ((dam_flags & CDF_EXPLODE_WEAPON) != 0) {
                dam_flags |= CDF_DESTROY_WEAPON;
                script_play_explosion_fx(combat->target_obj);
            }

            if ((dam_flags & CDF_DESTROY_WEAPON) != 0) {
                object_destroy(weapon_obj);

                mes_file_entry.num = 22; // "Weapon destroyed"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                tf_add(combat->target_obj, tf_type, mes_file_entry.str);

                weapon_dropped = true;
            } else {
                if ((dam_flags & CDF_DESTROY_AMMO) != 0) {
                    combat_consume_ammo(weapon_obj, combat->target_obj, 0, true);

                    mes_file_entry.num = 9; // "Ammo lost"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);
                } else if ((dam_flags & CDF_LOST_AMMO) != 0) {
                    combat_consume_ammo(weapon_obj, combat->target_obj, 5, false);

                    mes_file_entry.num = 9; // "Ammo lost"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);
                }

                if ((dam_flags & CDF_DROP_WEAPON) != 0) {
                    item_drop_nearby(weapon_obj);

                    mes_file_entry.num = 19; // "Weapon dropped"
                    mes_get_msg(combat_mes_file, &mes_file_entry);
                    tf_add(combat->target_obj, tf_type, mes_file_entry.str);

                    weapon_dropped = true;
                }
            }
        }

        if (dam > 0
            && (obj_field_int32_get(combat->target_obj, OBJ_F_SPELL_FLAGS) & OSF_MIRRORED) != 0) {
            sub_459A20(combat->target_obj);
        }

        int hp_ratio_before;

        if (dam > 0) {
            combat->dam_flags |= CDF_HAVE_DAMAGE;
            hp_ratio_before = ai_object_hp_ratio(combat->target_obj);
        }

        mes_file_entry.num = 1; // "HT"
        mes_get_msg(combat_mes_file, &mes_file_entry);
        sprintf(str, "%s %d", mes_file_entry.str, dam);

        combat->total_dam = dam;

        int hp_dam = object_hp_damage_get(combat->target_obj) + dam;
        if (hp_dam < 0) {
            hp_dam = 0;
        }
        object_hp_damage_set(combat->target_obj, hp_dam);

        if (dam > 0) {
            int64_t leader_obj = critter_pc_leader_get(combat->target_obj);
            if (leader_obj != OBJ_HANDLE_NULL
                && hp_ratio_before >= 20
                && ai_object_hp_ratio(combat->target_obj) < 20) {
                ai_npc_near_death(combat->target_obj, leader_obj);
            }
        }

        if ((critter_flags & OCF_FATIGUE_IMMUNE) == 0) {
            if (combat->dam[DAMAGE_TYPE_FATIGUE] > 0) {
                mes_file_entry.num = 10;
                mes_get_msg(combat_mes_file, &mes_file_entry);

                sprintf(&(str[strlen(str)]),
                    " %d %s",
                    combat->dam[DAMAGE_TYPE_FATIGUE],
                    mes_file_entry.str);

                critter_fatigue_damage_set(combat->target_obj,
                    critter_fatigue_damage_get(combat->target_obj) + combat->dam[DAMAGE_TYPE_FATIGUE]);
            }
        }

        if (tf_level_get() == TF_LEVEL_VERBOSE && str[0] != '\0') {
            tf_add(combat->target_obj, tf_type, str);
        }

        int poison = stat_base_get(combat->target_obj, STAT_POISON_LEVEL) + combat->dam[DAMAGE_TYPE_POISON];
        if (poison < 0) {
            poison = 0;
        }
        stat_base_set(combat->target_obj, STAT_POISON_LEVEL, poison);

        if (combat->dam[DAMAGE_TYPE_POISON] > 0 && tf_level_get() == TF_LEVEL_VERBOSE) {
            mes_file_entry.num = 0;
            mes_get_msg(combat_mes_file, &mes_file_entry);
            sprintf(str, "%s %+d", mes_file_entry.str, combat->dam[DAMAGE_TYPE_POISON]);
            tf_add(combat->target_obj, TF_TYPE_GREEN, str);
        }

        if (combat->field_30 != OBJ_HANDLE_NULL
            && obj_type == OBJ_TYPE_NPC
            && (spell_flags & OSF_STONED) == 0) {
            int remaining_experience = obj_field_int32_get(combat->target_obj, OBJ_F_NPC_EXPERIENCE_POOL);
            if (remaining_experience > 0) {
                int dam_ratio = 100 * dam / object_hp_max(combat->target_obj);
                int awarded_experience = obj_field_int32_get(combat->target_obj, OBJ_F_NPC_EXPERIENCE_WORTH) * dam_ratio / 100;
                if (awarded_experience > remaining_experience) {
                    awarded_experience = remaining_experience;
                }
                obj_field_int32_set(combat->target_obj, OBJ_F_NPC_EXPERIENCE_POOL, remaining_experience - awarded_experience);

                if (obj_field_int32_get(combat->field_30, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                    critter_give_xp(combat->field_30, awarded_experience);
                }
            }
        }

        if (critter_is_dead(combat->target_obj)) {
            int anim;

            if ((dam_flags & CDF_DEATH) != 0 || dam <= 20) {
                anim = 7;
            } else if (combat->hit_loc == HIT_LOC_HEAD) {
                anim = 17;
            } else if (combat->hit_loc == HIT_LOC_LEG) {
                anim = 19;
            } else {
                anim = 18;
            }

            critter_notify_killed(combat->target_obj, combat->field_30, anim);

            if (obj_type == OBJ_TYPE_NPC
                && critter_pc_leader_get(combat->target_obj) == player_get_local_pc_obj()) {
                ui_follower_update();
            }
        } else if (obj_type == OBJ_TYPE_NPC) {
            ai_process(combat->target_obj);

            if (critter_pc_leader_get(combat->target_obj) == player_get_local_pc_obj()) {
                ui_follower_update();
            }
        }

        if (combat->attacker_obj != OBJ_HANDLE_NULL) {
            mt_item_notify_parent_hit(combat->attacker_obj, combat, combat->target_obj);

            if ((combat->flags & 0x40000) != 0) {
                mt_item_notify_parent_dmgs_obj(combat->attacker_obj, combat->weapon_obj, combat->target_obj);
            } else {
                mt_item_notify_parent_dmgs_obj(combat->attacker_obj, OBJ_HANDLE_NULL, combat->target_obj);
            }
        }

        combat_apply_weapon_wear(combat);

        if (weapon_dropped) {
            pc_switch_weapon(combat->target_obj, combat->attacker_obj);
        }
    } else {
        // 0x4B462E
        if (obj_type == OBJ_TYPE_PORTAL
            || obj_type == OBJ_TYPE_CONTAINER) {
            int material = obj_field_int32_get(combat->target_obj, OBJ_F_MATERIAL);
            dam -= combat_material_damage_reduction_tbl[material];
        }

        if (dam < 0) {
            dam = 0;
        } else if (dam > 0) {
            combat->dam_flags |= CDF_HAVE_DAMAGE;
        }

        int hp_dam = object_hp_damage_get(combat->target_obj) + dam;
        if (hp_dam < 0) {
            hp_dam = 0;
        }
        object_hp_damage_set(combat->target_obj, hp_dam);

        if (tf_level_get() == TF_LEVEL_VERBOSE) {
            mes_file_entry.num = 1; // "HT"
            mes_get_msg(combat_mes_file, &mes_file_entry);

            sprintf(str, "%s %d", mes_file_entry.str, dam);
            tf_add(combat->target_obj, TF_TYPE_WHITE, str);
        }

        if (object_hp_current(combat->target_obj) > 0) {
            if (combat->field_30 != OBJ_HANDLE_NULL) {
                ai_notify_portal_container_guards(combat->field_30, combat->target_obj, true, LOUDNESS_LOUD);
                combat_apply_weapon_wear(combat);
            }
        } else {
            if (trap_type(combat->target_obj) != TRAP_TYPE_INVALID) {
                int64_t triggerer_obj = (combat->flags & CF_RANGED) == 0
                    ? combat->attacker_obj
                    : OBJ_HANDLE_NULL;
                trap_invoke(triggerer_obj, combat->target_obj);
            }

            object_bust(combat->target_obj, combat->attacker_obj);

            if (combat->field_30 != OBJ_HANDLE_NULL) {
                ai_notify_portal_container_guards(combat->field_30, combat->target_obj, false, LOUDNESS_LOUD);
            }
        }
    }

    if (!multiplayer_is_locked() && tig_net_is_host()) {
        PacketCombatDmg pkt;

        pkt.type = 20;
        sub_4F0640(combat->attacker_obj, &(pkt.attacker_oid));
        sub_4F0640(combat->weapon_obj, &(pkt.weapon_oid));
        sub_4F0640(combat->target_obj, &(pkt.target_oid));
        sub_4F0640(combat->field_28, &(pkt.field_B8));
        sub_4F0640(combat->field_30, &(pkt.field_D0));
        pkt.combat = *combat;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4B54B0
int64_t combat_critter_armor(int64_t critter_obj, int hit_loc)
{
    switch (hit_loc) {
    case HIT_LOC_HEAD:
        return item_wield_get(critter_obj, ITEM_INV_LOC_HELMET);
    case HIT_LOC_ARM:
        return item_wield_get(critter_obj, ITEM_INV_LOC_GAUNTLET);
    case HIT_LOC_LEG:
        return item_wield_get(critter_obj, ITEM_INV_LOC_BOOTS);
    default:
        return item_wield_get(critter_obj, ITEM_INV_LOC_ARMOR);
    }
}

// 0x4B5520
bool sub_4B5520(CombatContext* combat)
{
    int index;

    for (index = 0; index < sizeof(stru_5B5790) / sizeof(stru_5B5790[0]); index++) {
        if (stru_5B5790[index].func != NULL
            && stru_5B5790[index].func(combat->attacker_obj, combat->target_obj, combat->field_28, combat->field_30)) {
            return true;
        }
    }

    return false;
}

// 0x4B5580
void combat_apply_weapon_wear(CombatContext* combat)
{
    int dam;
    int complexity;
    char str[80];
    MesFileEntry mes_file_entry;

    if (combat->attacker_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (combat->target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if ((combat->flags & CF_WEAPON_WEAR) == 0) {
        return;
    }

    if ((combat->dam_flags & CDF_HAVE_DAMAGE) == 0) {
        return;
    }

    if (combat->skill != SKILL_MELEE) {
        return;
    }

    switch (obj_field_int32_get(combat->target_obj, OBJ_F_MATERIAL)) {
    case MATERIAL_STONE:
        dam = 4;
        break;
    case MATERIAL_BRICK:
        dam = 3;
        break;
    case MATERIAL_WOOD:
        dam = 2;
        break;
    case MATERIAL_PLANT:
        dam = 1;
        break;
    case MATERIAL_METAL:
        dam = 5;
        break;
    case MATERIAL_GLASS:
        dam = combat->weapon_obj != OBJ_HANDLE_NULL ? 0 : 5;
        break;
    case MATERIAL_FIRE:
        dam = 5;
        break;
    default:
        dam = 0;
        break;
    }

    if (combat->weapon_obj != OBJ_HANDLE_NULL) {
        complexity = obj_field_int32_get(combat->weapon_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY);
        if (complexity < 0) {
            complexity = -complexity;
        }

        if (complexity > 100) {
            complexity = 100;
        }

        dam -= dam * complexity / 100;

        switch (tig_art_item_id_subtype_get(obj_field_int32_get(combat->weapon_obj, OBJ_F_ITEM_USE_AID_FRAGMENT))) {
        case TIG_ART_WEAPON_TYPE_AXE:
            dam -= 2;
            break;
        case TIG_ART_WEAPON_TYPE_MACE:
        case TIG_ART_WEAPON_TYPE_STAFF:
            dam -= 1;
            break;
        }
    } else {
        int hp = object_hp_current(combat->attacker_obj);
        if (dam >= hp) {
            dam = hp - 1;
        }
    }

    str[0] = '\0';

    if (dam > 0) {
        if (combat->weapon_obj != OBJ_HANDLE_NULL) {
            // Apply damage to the weapon.
            object_hp_damage_set(combat->weapon_obj,
                object_hp_damage_get(combat->weapon_obj) + dam);

            mes_file_entry.num = 8; // "Weapon damaged"
            if (object_hp_current(combat->weapon_obj) <= 0) {
                object_bust(combat->weapon_obj, combat->attacker_obj);
                mes_file_entry.num = 23; // "Weapon broken"
                pc_switch_weapon(combat->attacker_obj, combat->target_obj);
            }

            mes_get_msg(combat_mes_file, &mes_file_entry);
            strcpy(str, mes_file_entry.str);
        } else {
            if (tf_level_get() == TF_LEVEL_VERBOSE) {
                mes_file_entry.num = 1; // "HT"
                mes_get_msg(combat_mes_file, &mes_file_entry);
                sprintf(str, "%d %s", dam, mes_file_entry.str);
            }

            // Apply damage to the target.
            object_hp_damage_set(combat->attacker_obj,
                object_hp_damage_get(combat->attacker_obj) + dam);
        }

        if (str[0] != '\0') {
            tf_add(combat->attacker_obj, TF_TYPE_RED, str);
        }
    }
}

// 0x4B5810
void combat_acid_dmg(CombatContext* combat)
{
    int64_t target_obj;
    int inventory_location;
    int64_t item_obj;

    target_obj = combat->target_obj;

    if (obj_type_is_critter(obj_field_int32_get(target_obj, OBJ_F_TYPE))) {
        for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
            item_obj = item_wield_get(target_obj, inventory_location);
            if (item_obj != OBJ_HANDLE_NULL) {
                combat->target_obj = item_obj;
                combat_dmg(combat);
            }
        }
    }

    combat->target_obj = target_obj;
    combat->dam[DAMAGE_TYPE_NORMAL] /= 3;
    combat_dmg(combat);
}

// 0x4B58C0
void combat_heal(CombatContext* combat)
{
    int type;
    int index;
    unsigned int critter_flags;
    bool unressurectable = false;
    bool v2 = false;
    tig_art_id_t art_id;

    if (!multiplayer_is_locked()) {
        PacketCombatHeal pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 21;
        sub_4F0640(combat->attacker_obj, &(pkt.attacker_oid));
        sub_4F0640(combat->weapon_obj, &(pkt.weapon_oid));
        sub_4F0640(combat->target_obj, &(pkt.target_oid));
        sub_4F0640(combat->field_28, &(pkt.field_B8));
        sub_4F0640(combat->field_30, &(pkt.field_D0));
        pkt.combat = *combat;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    type = obj_field_int32_get(combat->target_obj, OBJ_F_TYPE);
    if (!obj_type_is_critter(type)) {
        return;
    }

    if ((combat->dam_flags & CDF_SCAR) != 0
        && (combat->dam_flags & CDF_FULL) == 0) {
        for (index = 0; index < 5; index++) {
            effect_remove_one_caused_by(combat->target_obj, EFFECT_CAUSE_INJURY);
        }
    }

    if (critter_is_dead(combat->target_obj)) {
        if ((combat->dam_flags & CDF_RESURRECT) == 0) {
            return;
        }

        critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        if ((critter_flags & OCF_UNREVIVIFIABLE) != 0
            && (combat->dam_flags & CDF_REANIMATE) != 0) {
            unressurectable = true;
        }
        if ((critter_flags & OCF_UNRESSURECTABLE) != 0
            && (combat->dam_flags & CDF_REANIMATE) == 0) {
            unressurectable = true;
        }

        if (!object_script_execute(combat->attacker_obj, combat->target_obj, combat->weapon_obj, SAP_RESURRECT, 0) || unressurectable) {
            magictech_error_unressurectable(combat->target_obj);
            return;
        }

        art_id = obj_field_int32_get(combat->target_obj, OBJ_F_CURRENT_AID);
        art_id = tig_art_id_anim_set(art_id, TIG_ART_ANIM_STAND);
        art_id = tig_art_id_frame_set(art_id, 0);
        object_set_current_aid(combat->target_obj, art_id);

        v2 = true;

        combat_remove_blood_splotch(combat->target_loc);
        ai_stop_fleeing(combat->target_obj);
        object_flags_unset(combat->target_obj, OF_FLAT | OF_NO_BLOCK);

        critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        critter_flags &= ~OCF_STUNNED;
        obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);

        critter_decay_timeevent_cancel(combat->target_obj);

        if (critter_fatigue_damage_get(combat->target_obj) != 10) {
            critter_fatigue_damage_set(combat->target_obj, 10);
        }

        sub_459740(combat->target_obj);

        if (combat->attacker_obj != OBJ_HANDLE_NULL
            && obj_field_int32_get(combat->attacker_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            int reaction_level = reaction_get(combat->target_obj, combat->attacker_obj);
            if (reaction_level > 0 && reaction_level < 70) {
                reaction_adj(combat->target_obj, combat->attacker_obj, 70 - reaction_level);
            }
        }

        if (type == OBJ_TYPE_NPC) {
            int64_t leader_obj = critter_leader_get(combat->target_obj);
            if (leader_obj != OBJ_HANDLE_NULL) {
                if (!critter_follow(combat->target_obj, leader_obj, false)) {
                    critter_leader_set(combat->target_obj, OBJ_HANDLE_NULL);
                }
            }
        }
    }

    if ((combat->dam_flags & CDF_FULL) != 0) {
        combat->dam[DAMAGE_TYPE_NORMAL] = object_hp_damage_get(combat->target_obj);
        if (combat->dam[DAMAGE_TYPE_FATIGUE] > 0) {
            combat->dam[DAMAGE_TYPE_FATIGUE] = critter_fatigue_damage_get(combat->target_obj);
        }

        for (index = 0; index < 5; index++) {
            effect_remove_one_caused_by(combat->target_obj, EFFECT_CAUSE_INJURY);
        }

        critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        if ((critter_flags & OCF_BLINDED) != 0) {
            critter_flags &= ~OCF_BLINDED;
            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }
        if ((critter_flags & OCF_CRIPPLED_ARMS_ONE) != 0) {
            critter_flags &= ~OCF_CRIPPLED_ARMS_ONE;
            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }
        if ((critter_flags & OCF_CRIPPLED_ARMS_BOTH) != 0) {
            critter_flags &= ~OCF_CRIPPLED_ARMS_BOTH;
            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }
        if ((critter_flags & OCF_CRIPPLED_LEGS_BOTH) != 0) {
            critter_flags &= ~OCF_CRIPPLED_LEGS_BOTH;
            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }
    } else {
        critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        if ((combat->dam_flags & CDF_BLIND) != 0
            && (critter_flags & OCF_BLINDED) != 0) {
            critter_flags &= ~OCF_BLINDED;
            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }
        if ((combat->dam_flags & CDF_CRIPPLE_ARM) != 0
            && (critter_flags & (OCF_CRIPPLED_ARMS_ONE | OCF_CRIPPLED_ARMS_BOTH)) != 0) {
            critter_flags &= ~(OCF_CRIPPLED_ARMS_ONE | OCF_CRIPPLED_ARMS_BOTH);
            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }
        if ((combat->dam_flags & CDF_CRIPPLE_LEG) != 0
            && (critter_flags & OCF_CRIPPLED_LEGS_BOTH) != 0) {
            critter_flags &= ~OCF_BLINDED;
            obj_field_int32_set(combat->target_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }
    }

    if (combat->dam[DAMAGE_TYPE_NORMAL] > 0) {
        int new_dam;
        int cur_dam;

        cur_dam = object_hp_damage_get(combat->target_obj);
        if (cur_dam != 0) {
            new_dam = cur_dam - combat->dam[DAMAGE_TYPE_NORMAL];
            if (new_dam < 0) {
                new_dam = 0;
            }
            object_hp_damage_set(combat->target_obj, new_dam);
        }
    }

    if (combat->dam[DAMAGE_TYPE_FATIGUE] > 0) {
        int new_dam;
        int cur_dam;
        int fatigue;

        cur_dam = critter_fatigue_damage_get(combat->target_obj);
        if (cur_dam != 0) {
            fatigue = critter_fatigue_current(combat->target_obj);
            new_dam = cur_dam - combat->dam[DAMAGE_TYPE_FATIGUE];
            if (new_dam < 0) {
                new_dam = 0;
            }
            critter_fatigue_damage_set(combat->target_obj, new_dam);

            if (fatigue <= 0 && critter_fatigue_current(combat->target_obj) > 0) {
                anim_goal_get_up(combat->target_obj);
            }
        }
    }

    if (combat->dam[DAMAGE_TYPE_POISON] > 0) {
        int new_dam;
        int cur_dam;

        cur_dam = stat_base_get(combat->target_obj, STAT_POISON_LEVEL);
        if (cur_dam != 0) {
            new_dam = cur_dam - combat->dam[DAMAGE_TYPE_POISON];
            if (new_dam < 0) {
                new_dam = 0;
            }
            stat_base_set(combat->target_obj, STAT_POISON_LEVEL, new_dam);
        }
    }

    if (v2 && type != OBJ_TYPE_PC) {
        sub_4AD6E0(combat->target_obj);
    }

    if (type == OBJ_TYPE_NPC) {
        if (critter_pc_leader_get(combat->target_obj) == player_get_local_pc_obj()) {
            ui_follower_update();
        }
    }

    combat_recalc_reaction(combat->target_obj);
}

// 0x4B5E90
void combat_remove_blood_splotch(int64_t loc)
{
    ObjectList objects;
    ObjectNode* node;
    tig_art_id_t art_id;
    int64_t obj;

    object_list_location(loc, OBJ_TM_SCENERY, &objects);

    node = objects.head;
    while (node != NULL) {
        art_id = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
        if (tig_art_scenery_id_type_get(art_id) == TIG_ART_SCENERY_TYPE_MISC_SMALL
            && tig_art_num_get(art_id) == 0) {
            obj = node->obj;
            object_list_destroy(&objects);
            critter_decay_timeevent_cancel(obj);
            object_destroy(obj);
            return;
        }
        node = node->next;
    }

    object_list_destroy(&objects);
}

// 0x4B5F30
int combat_hit_loc_penalty(int hit_loc)
{
    return hit_loc_penalties[hit_loc];
}

// 0x4B5F40
void combat_process_crit_hit(CombatContext* combat)
{
    int target_obj_type;
    int attacker_obj_type;
    bool npc_attacks_pc;
    CritHitType crit_hit_type;
    int chance;
    CritBodyType crit_body_type;
    unsigned int critter_flags;
    int64_t helmet_obj;
    int difficulty;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        return;
    }

    if (combat->target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (combat->attacker_obj == OBJ_HANDLE_NULL) {
        return;
    }

    target_obj_type = obj_field_int32_get(combat->target_obj, OBJ_F_TYPE);
    attacker_obj_type = obj_field_int32_get(combat->attacker_obj, OBJ_F_TYPE);

    // Make sure it's a clash between critters.
    if (!obj_type_is_critter(target_obj_type)
        || !obj_type_is_critter(attacker_obj_type)) {
        return;
    }

    npc_attacks_pc = target_obj_type == OBJ_TYPE_PC
        && attacker_obj_type == OBJ_TYPE_NPC;

    crit_body_type = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_CRIT_HIT_CHART);

    if (combat->weapon_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
        crit_hit_type = obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_CRIT_HIT_CHART);
        if ((combat->flags & 0x20000) == 0) {
            chance = item_adjust_magic(combat->weapon_obj,
                combat->attacker_obj,
                obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_MAGIC_CRIT_HIT_EFFECT));
        }
    } else {
        crit_hit_type = CRIT_HIT_TYPE_CRUSHING;
        chance = 0;
    }

    chance = effect_adjust_crit_hit_effect(combat->attacker_obj, chance);

    if ((combat->flags & CF_AIM) != 0) {
        chance += 10;
    }

    if (!npc_attacks_pc) {
        if (random_between(1, 100) <= chance + 10) {
            combat->dam_flags |= CDF_BONUS_DAM_200;
        } else if (random_between(1, 100) <= chance + 30) {
            combat->dam_flags |= CDF_BONUS_DAM_100;
        } else if (random_between(1, 100) <= chance + 60) {
            combat->dam_flags |= CDF_BONUS_DAM_50;
        }
    }

    if (crit_body_type != CRIT_BODY_TYPE_AMORPHOUS) {
        critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        if ((critter_flags & OCF_UNDEAD) == 0) {
            helmet_obj = item_wield_get(combat->target_obj, ITEM_INV_LOC_HELMET);

            difficulty = chance + 5;

            if (crit_hit_type == CRIT_HIT_TYPE_CRUSHING
                || crit_hit_type == CRIT_HIT_TYPE_ELECTRICAL) {
                difficulty += 10;
            }

            if (combat->hit_loc == HIT_LOC_HEAD && helmet_obj == OBJ_HANDLE_NULL) {
                difficulty += 10;
            }

            if (random_between(1, 100) <= difficulty
                && !critter_check_stat(combat->target_obj, STAT_CONSTITUTION, -5)) {
                combat->dam_flags |= CDF_STUN;

                if (target_obj_type != OBJ_TYPE_PC
                    && (critter_flags & OCF_FATIGUE_IMMUNE) == 0
                    && random_between(1, 100) < difficulty
                    && !critter_check_stat(combat->target_obj, STAT_CONSTITUTION, -5)) {
                    combat->dam_flags |= CDF_KNOCKOUT;
                }
            }

            // NOTE: Not sure how to reimplement it without goto.
            do {
                if (crit_body_type != CRIT_BODY_TYPE_SNAKE) {
                    if (crit_body_type != CRIT_BODY_TYPE_AVIAN && combat->hit_loc == HIT_LOC_ARM) {
                        if (random_between(1, 100) <= chance + 1
                            && !critter_check_stat(combat->target_obj, STAT_CONSTITUTION, -5)) {
                            combat->dam_flags |= CDF_CRIPPLE_ARM;
                        }
                        break;
                    }

                    if (combat->hit_loc == HIT_LOC_LEG) {
                        if (random_between(1, 100) <= chance + 1
                            && !critter_check_stat(combat->target_obj, STAT_CONSTITUTION, -5)) {
                            combat->dam_flags |= CDF_CRIPPLE_LEG;
                        }
                        break;
                    }
                }

                if (combat->hit_loc == HIT_LOC_HEAD) {
                    if (crit_hit_type == CRIT_HIT_TYPE_CRUSHING) {
                        if (helmet_obj != OBJ_HANDLE_NULL
                            && random_between(1, 100) <= chance + 5) {
                            combat->dam_flags |= CDF_DROP_HELMET;
                            break;
                        }
                    }

                    if (!npc_attacks_pc
                        && random_between(1, 100) <= chance + (helmet_obj == OBJ_HANDLE_NULL ? 1 : 0)
                        && !critter_check_stat(combat->target_obj, STAT_CONSTITUTION, 0)) {
                        combat->dam_flags |= CDF_BLIND;
                        break;
                    }
                }
            } while (0);

            if (target_obj_type == OBJ_TYPE_PC
                && ((combat->flags & 0x8) == 0
                    || effect_count_effects_of_type(combat->target_obj, EFFECT_SCARRING) <= 0)
                && random_between(1, 100) <= chance + 5) {
                combat->dam_flags |= CDF_SCAR;
            }
        }

        if (crit_body_type == CRIT_BODY_TYPE_BIPED) {
            difficulty = chance + 5;

            if (crit_hit_type == CRIT_HIT_TYPE_CRUSHING) {
                difficulty += 5;
            }

            if (combat->hit_loc == HIT_LOC_LEG) {
                difficulty += 10;
            }

            if (random_between(1, 100) <= difficulty) {
                combat->dam_flags |= CDF_KNOCKDOWN;
            }
        }
    }

    if (combat_critter_weapon(combat->target_obj) != OBJ_HANDLE_NULL) {
        if (random_between(1, 100) <= chance + 10) {
            combat->dam_flags |= CDF_DAMAGE_WEAPON;
        }

        if (!npc_attacks_pc
            && random_between(1, 100) <= chance + 10) {
            combat->dam_flags |= CDF_DROP_WEAPON;
        }
    }

    if (combat_critter_armor(combat->target_obj, combat->hit_loc) != OBJ_HANDLE_NULL
        && random_between(1, 100) <= chance + 10) {
        combat->dam_flags |= CDF_DAMAGE_ARMOR;
    }

    if (combat->dam_flags == 0) {
        combat->dam_flags = CDF_BONUS_DAM_50;
    }
}

// 0x4B6410
void combat_process_crit_miss(CombatContext* combat)
{
    CritMissType crit_miss_type;
    int chance;

    if (combat->weapon_obj != OBJ_HANDLE_NULL) {
        if (obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
            crit_miss_type = obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_CRIT_MISS_CHART);
            if ((combat->flags & 0x20000) == 0) {
                chance = item_adjust_magic(combat->weapon_obj,
                    combat->attacker_obj,
                    obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_MAGIC_CRIT_MISS_EFFECT));
            } else {
                chance = 0;
            }
        } else {
            crit_miss_type = CRIT_MISS_TYPE_BLUDGEON;
            chance = 0;
        }
    } else {
        crit_miss_type = CRIT_MISS_TYPE_HANDS;
        chance = 0;
    }

    chance = effect_adjust_crit_fail_effect(combat->attacker_obj, chance);

    combat->flags &= ~CF_AIM;
    combat->flags &= ~CF_CRITICAL;
    combat->flags |= 0x8;
    combat->flags |= CF_HIT;

    combat->target_obj = combat->attacker_obj;
    combat->target_loc = obj_field_int64_get(combat->target_obj, OBJ_F_LOCATION);

    if (random_between(1, 100) <= 50) {
        combat->flags |= CF_CRITICAL;
        if (crit_miss_type != CRIT_MISS_TYPE_HANDS
            && random_between(1, 100) <= chance + 1) {
            combat->dam_flags |= CDF_DESTROY_WEAPON;
        }

        if ((crit_miss_type == CRIT_MISS_TYPE_EXPLOSIVE
                || crit_miss_type == CRIT_MISS_TYPE_FIRE
                || crit_miss_type == CRIT_MISS_TYPE_ELECTRICAL)
            && random_between(1, 100) <= chance + 1) {
            combat->dam_flags |= CDF_EXPLODE_WEAPON;
        }

        if (item_weapon_ammo_type(combat->weapon_obj) != 10000) {
            if (random_between(1, 100) <= chance + 1) {
                combat->dam_flags |= CDF_DESTROY_AMMO;
            } else if (random_between(1, 100) <= chance + 11) {
                combat->dam_flags |= CDF_LOST_AMMO;
            }
        }

        combat_process_crit_hit(combat);
    }
}

// 0x4B65A0
int combat_random_hit_loc(void)
{
    int value = random_between(1, 100);
    if (value <= 70) return HIT_LOC_TORSO;
    if (value <= 85) return HIT_LOC_LEG;
    if (value <= 95) return HIT_LOC_ARM;
    return HIT_LOC_HEAD;
}

// 0x4B65D0
bool combat_consume_ammo(int64_t weapon_obj, int64_t critter_obj, int cnt, bool destroy)
{
    int ammo_type;
    int qty;
    int consumption;

    if (obj_field_int32_get(critter_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC
        && critter_pc_leader_get(critter_obj) == OBJ_HANDLE_NULL) {
        return true;
    }

    ammo_type = item_weapon_ammo_type(weapon_obj);
    if (ammo_type == 10000) {
        return true;
    }

    qty = item_ammo_quantity_get(critter_obj, ammo_type);

    if (destroy) {
        item_ammo_transfer(critter_obj, OBJ_HANDLE_NULL, qty, ammo_type, OBJ_HANDLE_NULL);
        return true;
    }

    consumption = cnt * obj_field_int32_get(weapon_obj, OBJ_F_WEAPON_AMMO_CONSUMPTION);
    if (qty >= consumption) {
        item_ammo_transfer(critter_obj, OBJ_HANDLE_NULL, consumption, ammo_type, OBJ_HANDLE_NULL);
        return true;
    }

    return false;
}

// 0x4B6680
void combat_calc_dmg(CombatContext* combat)
{
    unsigned int critter_flags = 0;
    unsigned int critter_flags2 = 0;
    unsigned int spell_flags = 0;
    DamageType damage_type;
    int min_damage;
    int max_damage;
    int damage;

    if ((combat->flags & 0x100) != 0) {
        return;
    }

    if (obj_type_is_critter(obj_field_int32_get(combat->target_obj, OBJ_F_TYPE))) {
        critter_flags = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS);
        critter_flags2 = obj_field_int32_get(combat->target_obj, OBJ_F_CRITTER_FLAGS2);
        spell_flags = obj_field_int32_get(combat->target_obj, OBJ_F_SPELL_FLAGS);
    }

    for (damage_type = 0; damage_type < DAMAGE_TYPE_COUNT; damage_type++) {
        item_weapon_damage(combat->weapon_obj,
            combat->attacker_obj,
            damage_type,
            combat->skill,
            (combat->flags & 0x20000) != 0,
            &min_damage,
            &max_damage);

        if (damage_type == DAMAGE_TYPE_POISON
            && ((critter_flags & (OCF_UNDEAD | OCF_MECHANICAL)) != 0
                || (spell_flags & OSF_STONED) != 0)) {
            damage = 0;
        } else {
            damage = random_between(min_damage, max_damage);

            switch (damage_type) {
            case DAMAGE_TYPE_NORMAL:
                if ((combat->flags & 0x8000) != 0) {
                    damage += basic_skill_level(combat->attacker_obj, BASIC_SKILL_BACKSTAB) * 5;
                } else if ((combat->flags & 0x4000) != 0) {
                    damage += basic_skill_level(combat->attacker_obj, BASIC_SKILL_BACKSTAB);
                }
                break;
            case DAMAGE_TYPE_FATIGUE:
                if ((critter_flags2 & OCF2_FATIGUE_DRAINING) != 0 && damage > 0) {
                    damage *= 4;
                }
                if ((critter_flags & OCF_FATIGUE_LIMITING) != 0 && damage > 0) {
                    damage /= 4;
                }
                break;
            default:
                break;
            }
        }

        combat->dam[damage_type] = damage;
    }

    spell_flags = obj_field_int32_get(combat->attacker_obj, OBJ_F_SPELL_FLAGS);
    if (combat->weapon_obj == OBJ_HANDLE_NULL) {
        if ((spell_flags & OSF_BODY_OF_EARTH) != 0) {
            combat->dam[DAMAGE_TYPE_NORMAL] += 5;
        } else if ((spell_flags & OSF_BODY_OF_FIRE) != 0) {
            combat->dam[DAMAGE_TYPE_FIRE] += 15;
        } else if ((spell_flags & OSF_BODY_OF_WATER) != 0) {
            combat->dam[DAMAGE_TYPE_FATIGUE] += 15;
        } else if ((spell_flags & OSF_HARDENED_HANDS) != 0) {
            combat->dam[DAMAGE_TYPE_NORMAL] += 2;
        }
    }
}

// 0x4B6860
void combat_apply_resistance(CombatContext* combat)
{
    int damage_type;
    int resistance;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        return;
    }

    if (combat->target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if ((combat->dam_flags & CDF_IGNORE_RESISTANCE) != 0) {
        return;
    }

    if (combat->weapon_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(combat->weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON
        && (obj_field_int32_get(combat->weapon_obj, OBJ_F_WEAPON_FLAGS) & OWF_IGNORE_RESISTANCE) != 0) {
        return;
    }

    for (damage_type = 0; damage_type < DAMAGE_TYPE_COUNT; damage_type++) {
        resistance = object_get_resistance(combat->target_obj, combat_damage_to_resistance_tbl[damage_type], false);
        if (damage_type == DAMAGE_TYPE_FATIGUE) {
            resistance = 3 * resistance / 4;
        }
        if (resistance > 0) {
            combat->dam[damage_type] -= resistance * combat->dam[damage_type] / 100;
        }
    }
}

// 0x4B6930
int combat_play_hit_fx(CombatContext* combat)
{
    int blood_splotch_type = BLOOD_SPLOTCH_TYPE_NONE;
    int max_damage = 0;
    int max_damage_type = 0;
    int damage_type;

    if (combat->target_obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(obj_field_int32_get(combat->target_obj, OBJ_F_TYPE))
        && (combat->flags & CF_HIT) != 0) {
        if ((combat->flags & CF_CRITICAL) != 0) {
            blood_splotch_type = BLOOD_SPLOTCH_TYPE_CRITICAL;
        } else {
            for (damage_type = 0; damage_type < 4; damage_type++) {
                if (combat->dam[damage_type] > max_damage) {
                    max_damage = combat->dam[damage_type];
                    max_damage_type = damage_type;
                }
            }

            blood_splotch_type = max_damage_type + 1;

            // FIXME: Unreachable.
            if (max_damage_type == -1) {
                return blood_splotch_type;
            }
        }

        if (!anim_play_weapon_fx(combat, combat->weapon_obj, combat->target_obj, ANIM_WEAPON_EYE_CANDY_TYPE_HIT)) {
            if (max_damage > 0) {
                if ((combat->flags & 0x80) == 0) {
                    anim_play_blood_splotch_fx(combat->target_obj, blood_splotch_type, DAMAGE_TYPE_NORMAL, combat);
                }
            } else {
                magictech_anim_play_hit_fx(combat->target_obj, combat);
            }
        }
    }

    return blood_splotch_type;
}

// 0x4B6A00
int combat_projectile_rot(int64_t from_loc, int64_t to_loc)
{
    int64_t x1;
    int64_t y1;
    int64_t x2;
    int64_t y2;
    double v1;
    int extended_rotation;

    if (from_loc == to_loc) {
        return 0;
    }

    location_xy(from_loc, &x1, &y1);
    location_xy(to_loc, &x2, &y2);

    v1 = atan2((double)(y2 - y1), (double)(x2 - x1));
    if (v1 < 0.0) {
        v1 += M_PI * 2.0;
    }

    extended_rotation = (int)(v1 * 32.0 / (M_PI * 2.0));
    if (extended_rotation < 0 || extended_rotation >= 32) {
        extended_rotation = 0;
    }

    extended_rotation += 8;
    if (extended_rotation >= 32) {
        extended_rotation -= 32;
    }

    return extended_rotation;
}

// 0x4B6B10
tig_art_id_t combat_projectile_art_id_rotation_set(tig_art_id_t aid, int projectile_rot)
{
    if (tig_art_type(aid) == TIG_ART_TYPE_ITEM) {
        aid = tig_art_id_rotation_set(aid, projectile_rot / 4);
    } else {
        aid = tig_art_num_set(aid, tig_art_num_get(aid) + projectile_rot / 8);
        aid = tig_art_id_rotation_set(aid, projectile_rot % 8);
    }

    return aid;
}

// 0x4B6B90
void combat_process_taunts(CombatContext* combat)
{
    int64_t pc_obj;

    // FIXME: Unused.
    pc_obj = player_get_local_pc_obj();

    if (!combat_taunts_get()) {
        return;
    }

    if (obj_field_int32_get(combat->attacker_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    if (combat->target_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (critter_pc_leader_get(combat->target_obj) == combat->attacker_obj) {
        return;
    }

    if ((combat->flags & CF_HIT) == 0) {
        return;
    }

    if (combat->target_obj == combat->field_28) {
        if ((combat->flags & CF_CRITICAL) != 0) {
            ai_npc_witness_pc_critical(combat->attacker_obj, AI_NPC_WITNESS_PC_CRITICAL_HIT);
        }
    } else {
        if ((combat->flags & 0x08) != 0) {
            ai_npc_witness_pc_critical(combat->attacker_obj, AI_NPC_WITNESS_PC_CRITICAL_MISS);
        }
    }
}

// 0x4B6C40
bool combat_set_callbacks(CombatCallbacks* callbacks)
{
    combat_callbacks = *callbacks;

    return true;
}

// 0x4B6C80
bool combat_is_turn_based(void)
{
    return combat_turn_based;
}

// 0x4B6C90
bool sub_4B6C90(bool turn_based)
{
    int64_t pc;

    if (tig_net_is_active() && turn_based) {
        return false;
    }

    if (combat_turn_based == turn_based) {
        return true;
    }

    pc = player_get_local_pc_obj();
    if (!sub_424070(pc, 3, 0, 1)) {
        return false;
    }

    if (!combat_critter_is_combat_mode_active(pc)) {
        combat_turn_based = turn_based;
        return true;
    }

    if (!turn_based) {
        combat_callbacks.field_8();
        combat_turn_based_end();
        combat_turn_based = turn_based;
        return true;
    }

    combat_callbacks.field_4();
    if (combat_turn_based_start()) {
        combat_turn_based = turn_based;
        return true;
    }

    combat_callbacks.field_8();
    return 1;
}

// 0x4B6D20
void combat_turn_based_toggle(void)
{
    if (combat_is_turn_based()) {
        settings_set_value(&settings, TURN_BASED_KEY, 0);
    } else {
        if (critter_is_active(player_get_local_pc_obj())) {
            settings_set_value(&settings, TURN_BASED_KEY, 1);
        }
    }
}

// 0x4B6D70
bool combat_turn_based_is_active(void)
{
    return combat_turn_based_active;
}

// 0x4B6D80
int64_t combat_turn_based_whos_turn_get(void)
{
    if (dword_5FC240 != NULL) {
        return dword_5FC240->obj;
    } else {
        return OBJ_HANDLE_NULL;
    }
}

// 0x4B6DA0
void combat_debug(int64_t obj, const char* msg)
{
    char* name = NULL;

    if (obj == OBJ_HANDLE_NULL) {
        obj = qword_5FC270;
    }

    if (obj != OBJ_HANDLE_NULL) {
        if (obj_handle_is_valid(obj)) {
            if (player_is_pc_obj(obj)) {
                obj_field_string_get(obj, OBJ_F_PC_PLAYER_NAME, &name);
            } else {
                obj_field_string_get(obj, OBJ_F_NAME, &name);
            }
        } else {
            qword_5FC270 = OBJ_HANDLE_NULL;
        }
    }

    tig_debug_printf("Combat: TB: DBG: %s: %s, Idx: %d, APs Left: %d\n",
        msg,
        name != NULL ? name : " ",
        dword_5FC250,
        combat_action_points);

    if (name != NULL) {
        FREE(name);
    }
}

// 0x4B6E70
void combat_turn_based_whos_turn_set(int64_t obj)
{
    if (!combat_turn_based_active) {
        return;
    }

    combat_debug(obj, "Whos Turn Set");

    if (!player_is_local_pc_obj(qword_5FC248)) {
        sub_424070(qword_5FC248, 3, 0, 1);
    }

    if (dword_5FC240->obj != obj) {
        dword_5FC240 = combat_critter_list.head;
        while (dword_5FC240 != NULL) {
            if (dword_5FC240->obj == obj) {
                break;
            }
            dword_5FC240 = dword_5FC240->next;
        }

        if (dword_5FC240 == NULL) {
            tig_debug_printf("Combat: TB: Note: Couldn't change to 'Who's' Turn, inserting them in list.\n");
            combat_turn_based_add_critter(obj);
            dword_5FC240 = combat_critter_list.head;
            while (dword_5FC240 != NULL) {
                if (dword_5FC240->obj == obj) {
                    break;
                }
                dword_5FC240 = dword_5FC240->next;
            }

            if (dword_5FC240 == NULL) {
                tig_debug_printf("Combat: TB: ERROR: Couldn't change to 'Who's' Turn AFTER inserting them in list...DISABLING TB-COMBAT!\n");
                settings_set_value(&settings, TURN_BASED_KEY, 0);
                return;
            }
        }
    }

    combat_action_points = stat_level_get(obj, STAT_SPEED);
    if (combat_action_points < 5) {
        combat_action_points = 5;
    }

    tig_debug_printf("Combat: TB: Action Points Available: %d.\n", combat_action_points);

    if (player_is_local_pc_obj(obj)) {
        combat_callbacks.field_C(combat_action_points);

        if (!sub_4BB900()) {
            sub_4BB8E0();
        }
    } else {
        combat_callbacks.field_C(0);
        sub_4BB910();
        sub_4B7010(obj);

        if (sub_4BB900()) {
            sub_4BB8F0();
        }
    }

    qword_5FC248 = obj;

    if (combat_callbacks.field_10 != NULL) {
        combat_callbacks.field_10(obj);
    }
}

// 0x4B7010
void sub_4B7010(int64_t obj)
{
    if (obj == dword_5FC240->obj
        && !player_is_pc_obj(obj)
        && !gamelib_in_load()) {
        if (object_script_execute(obj, obj, OBJ_HANDLE_NULL, SAP_HEARTBEAT, 0) == 1) {
            ai_process(obj);
        }

        if (!sub_423300(obj, NULL)) {
            combat_action_points = 0;
            sub_4B7080();
        }
    }
}

// 0x4B7080
void sub_4B7080(void)
{
    DateTime datetime;
    TimeEvent timeevent;

    if (!combat_turn_based_active) {
        return;
    }

    combat_debug(OBJ_HANDLE_NULL, "TB Anims Complete");

    if (gamelib_in_reset()) {
        return;
    }

    if (dword_5FC240 != NULL) {
        if (player_is_pc_obj(dword_5FC240->obj) && combat_action_points > 0) {
            return;
        }

        if (dword_5FC240->obj == qword_5FC258) {
            if (combat_action_points == dword_5FC260) {
                dword_5FC264 = true;
            }
            dword_5FC260 = combat_action_points;
        } else {
            qword_5FC258 = dword_5FC240->obj;
        }
    }

    if (!sub_45C0E0(TIMEEVENT_TYPE_TB_COMBAT)) {
        timeevent.type = TIMEEVENT_TYPE_TB_COMBAT;
        sub_45A950(&datetime, 2);
        timeevent_add_delay(&timeevent, &datetime);
    }
}

// 0x4B7150
bool combat_tb_timeevent_process(TimeEvent* timeevent)
{
    (void)timeevent;

    combat_debug(OBJ_HANDLE_NULL, "TimeEvent Process");

    if (combat_turn_based_active) {
        if (dword_5FC240 != NULL
            && player_is_pc_obj(dword_5FC240->obj)
            && combat_action_points > 0) {
            return true;
        }

        if (dword_5FC264) {
            dword_5FC264 = false;
            combat_consume_action_points(dword_5FC240->obj, combat_action_points);
        }

        if (combat_action_points_get() <= 0) {
            combat_turn_based_next_subturn();
        }
    }

    return true;
}

// 0x4B71E0
bool combat_turn_based_start(void)
{
    int64_t loc;
    LocRect loc_rect;
    ObjectNode* node;

    combat_debug(OBJ_HANDLE_NULL, "TB Start");

    if (combat_turn_based_active) {
        return true;
    }

    combat_turn_based_turn = 0;

    if (!anim_goal_interrupt_all_for_tb_combat()) {
        tig_debug_printf("Combat: TB_Start: Anim-Goal-Interrupt FAILED!\n");
    }

    dword_5FC250 = 0;

    sub_423FE0(sub_4B7080);

    loc = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    combat_recalc_perception_range();

    loc_rect.x1 = LOCATION_GET_X(loc) - combat_perception_range;
    loc_rect.x2 = LOCATION_GET_X(loc) + combat_perception_range;
    loc_rect.y1 = LOCATION_GET_Y(loc) - combat_perception_range;
    loc_rect.y2 = LOCATION_GET_Y(loc) + combat_perception_range;
    object_list_rect(&loc_rect, OBJ_TM_NPC | OBJ_TM_PC, &combat_critter_list);
    sort_combat_list();

    node = combat_critter_list.head;
    while (node != NULL) {
        sub_424070(node->obj, 3, 0, 1);
        node = node->next;
    }

    combat_turn_based_active = true;

    return combat_turn_based_begin_turn();
}

// 0x4B7300
void combat_recalc_perception_range(void)
{
    combat_perception_range = stat_level_get(player_get_local_pc_obj(), STAT_PERCEPTION) / 2 + 5;
    if (combat_perception_range < 10) {
        combat_perception_range = 10;
    }
}

// 0x4B7330
void combat_turn_based_end(void)
{
    ObjectNode* node;

    combat_debug(OBJ_HANDLE_NULL, "TB End");

    if (combat_turn_based_active) {
        combat_turn_based_active = false;

        sub_423FE0(NULL);

        if (!in_combat_reset) {
            node = combat_critter_list.head;
            while (node != NULL) {
                animfx_remove(&combat_eye_candies, node->obj, 0, -1);
                node = node->next;
            }
        }

        object_list_destroy(&combat_critter_list);
    }
}

// 0x4B73A0
bool combat_turn_based_begin_turn(void)
{
    int64_t pc_obj;
    int64_t pc_loc;
    LocRect loc_rect;
    ObjectList objects;

    dword_5FC250 = 0;
    pc_obj = player_get_local_pc_obj();
    combat_turn_based_turn++;
    pc_loc = obj_field_int64_get(pc_obj, OBJ_F_LOCATION);
    combat_recalc_perception_range();

    loc_rect.x1 = LOCATION_GET_X(pc_loc) - combat_perception_range;
    loc_rect.y1 = LOCATION_GET_Y(pc_loc) - combat_perception_range;
    loc_rect.x2 = LOCATION_GET_X(pc_loc) + combat_perception_range;
    loc_rect.y2 = LOCATION_GET_Y(pc_loc) + combat_perception_range;
    object_list_rect(&loc_rect, OBJ_TM_PC | OBJ_TM_NPC, &objects);
    object_list_copy(&combat_critter_list, &objects);
    object_list_destroy(&objects);

    sort_combat_list();

    dword_5FC250 = 0;
    dword_5FC240 = combat_critter_list.head;
    while (sub_4B7580(dword_5FC240) && dword_5FC240 != NULL) {
        dword_5FC250++;
        dword_5FC240 = dword_5FC240->next;
    }

    if (dword_5FC240 == NULL) {
        if (critter_is_active(pc_obj)) {
            dword_5FC250 = 0;
            dword_5FC240 = combat_critter_list.head;
            while (dword_5FC240 != NULL && dword_5FC240->obj != pc_obj) {
                dword_5FC240 = dword_5FC240->next;
                dword_5FC250++;
            }

            if (dword_5FC240 == OBJ_HANDLE_NULL) {
                tig_debug_printf("Combat: combat_turn_based_begin_turn: ERROR: Couldn't start TB Combat Turn due to no Active Critters!\n");
                combat_turn_based_end();
                return false;
            }
        }
    }

    combat_debug(dword_5FC240 != NULL ? dword_5FC240->obj : OBJ_HANDLE_NULL, "TB Begin Turn");
    qword_5FC258 = OBJ_HANDLE_NULL;
    dword_5FC260 = 0;
    dword_5FC264 = 0;
    combat_turn_based_subturn_start();

    return true;
}

// 0x4B7580
bool sub_4B7580(ObjectNode* object_node)
{
    if (dword_5FC240 == NULL) {
        return true;
    }

    if (!critter_is_active(object_node->obj)) {
        sub_427000(object_node->obj);
        return true;
    }

    if (sub_4B7DC0(object_node->obj)) {
        return true;
    }

    return false;
}

// 0x4B75D0
void combat_turn_based_subturn_start(void)
{
    if (dword_5FC240 != NULL) {
        combat_debug(dword_5FC240->obj, "SubTurn Start");
        combat_turn_based_whos_turn_set(dword_5FC240->obj);
        combat_check_action_points(dword_5FC240->obj, 0);
    } else {
        tig_debug_printf("Combat: combat_turn_based_subturn_start: ERROR: Couldn't start TB Combat Turn due to no Active Critters!\n");
        combat_turn_based_end();
    }
}

// 0x4B7630
void combat_turn_based_subturn_end(void)
{
    combat_debug(dword_5FC240->obj, "SubTurn End");
    if (player_is_local_pc_obj(dword_5FC240->obj)) {
        combat_callbacks.field_C(0);
    } else {
        object_script_execute(dword_5FC240->obj, dword_5FC240->obj, OBJ_HANDLE_NULL, SAP_END_COMBAT, 0);
    }
}

// 0x4B7690
void combat_turn_based_next_subturn(void)
{
    combat_debug(dword_5FC240->obj, "Next SubTurn");

    dword_5FC250++;
    combat_turn_based_subturn_end();

    dword_5FC240 = dword_5FC240->next;
    if (dword_5FC240 == NULL) {
        combat_turn_based_end_turn();
        return;
    }

    if (sub_4B7580(dword_5FC240)) {
        while (dword_5FC240 != NULL) {
            dword_5FC240 = dword_5FC240->next;
            dword_5FC250++;
            if (dword_5FC240 == NULL) {
                break;
            }
            if (!sub_4B7580(dword_5FC240)) {
                break;
            }
        }
        if (dword_5FC240 == NULL) {
            combat_turn_based_end_turn();
        }
    }

    if (dword_5FC240 != NULL) {
        combat_turn_based_subturn_start();
        return;
    }

    tig_debug_printf("Combat: combat_turn_based_next_subturn: ERROR: Couldn't start TB Combat Turn due to no Active Critters!\n");
    combat_turn_based_end();
}

// 0x4B7740
void combat_turn_based_end_turn(void)
{
    DateTime datetime;

    combat_debug(OBJ_HANDLE_NULL, "TB End Turn");
    sub_45A950(&datetime, 1000);
    timeevent_inc_datetime(&datetime);
    combat_turn_based_begin_turn();
}

// 0x4B7780
int combat_action_points_get(void)
{
    return combat_action_points;
}

// 0x4B7790
bool combat_check_action_points(int64_t obj, int required_action_points)
{
    bool is_pc;

    if (!combat_turn_based_active) {
        return true;
    }

    if (dword_5FC240->obj != obj) {
        return true;
    }

    combat_required_action_points = required_action_points;

    is_pc = player_is_local_pc_obj(obj);
    if (is_pc) {
        combat_callbacks.field_C(combat_action_points);
    }

    if (combat_action_points >= required_action_points) {
        return true;
    }

    if (combat_action_points > 0
        && is_pc
        && critter_fatigue_current(obj) > 1) {
        return true;
    }

    return false;
}

// 0x4B7830
bool combat_check_move_to(int64_t source_obj, int64_t target_loc)
{
    int type;

    if (!combat_turn_based_active) {
        return combat_check_action_points(source_obj, 0);
    }

    type = obj_field_int32_get(source_obj, OBJ_F_TYPE);
    if (obj_type_is_item(type)
        && (obj_field_int32_get(source_obj, OBJ_F_FLAGS) & OF_INVENTORY) != 0
        && obj_field_handle_get(source_obj, OBJ_F_ITEM_PARENT) != OBJ_HANDLE_NULL) {
        return combat_check_action_points(source_obj, 0);
    }

    return combat_check_action_points(source_obj, combat_move_cost(source_obj, target_loc, false));
}

// 0x4B78D0
bool combat_check_attack(int64_t source_obj, int64_t target_obj)
{
    int action_points;
    int64_t weapon_obj;
    int64_t source_loc;
    int64_t target_loc;

    if (!combat_turn_based_active) {
        return combat_check_action_points(source_obj, 0);
    }

    action_points = combat_attack_cost(source_obj);

    weapon_obj = item_wield_get(source_obj, ITEM_INV_LOC_WEAPON);
    if (weapon_obj == OBJ_HANDLE_NULL
        || item_weapon_range(weapon_obj, source_obj) <= 1) {
        source_loc = obj_field_int64_get(source_obj, OBJ_F_LOCATION);
        target_loc = obj_field_int64_get(target_obj, OBJ_F_LOCATION);
        if (location_dist(source_loc, target_loc) > 1) {
            action_points += combat_move_cost(source_obj, target_loc, true);
        }
    }

    return combat_check_action_points(source_obj, action_points);
}

// 0x4B79A0
bool combat_check_use_obj(int64_t source_obj, int64_t target_obj)
{
    int64_t source_loc;
    int64_t target_loc;
    int action_points;

    if (!combat_turn_based_active) {
        return combat_check_action_points(source_obj, 0);
    }

    action_points = 2;

    source_loc = obj_field_int64_get(source_obj, OBJ_F_LOCATION);
    target_loc = obj_field_int64_get(target_obj, OBJ_F_LOCATION);
    if (source_loc != target_loc) {
        action_points += combat_move_cost(source_obj, target_loc, true);
    }

    return combat_check_action_points(source_obj, action_points);
}

// 0x4B7A20
bool combat_check_pick_item(int64_t source_obj, int64_t target_obj)
{
    int64_t source_loc;
    int64_t target_loc;
    int action_points;

    if (!combat_turn_based_active) {
        return combat_check_action_points(source_obj, 0);
    }

    action_points = 1;

    source_loc = obj_field_int64_get(source_obj, OBJ_F_LOCATION);
    target_loc = obj_field_int64_get(target_obj, OBJ_F_LOCATION);
    if (source_loc != target_loc) {
        action_points += combat_move_cost(source_obj, target_loc, false);
    }

    return combat_check_action_points(source_obj, action_points);
}

// 0x4B7AA0
bool combat_check_cast_spell(int64_t source_obj)
{
    if (!combat_turn_based_active) {
        return combat_check_action_points(source_obj, 0);
    }

    return combat_check_action_points(source_obj, 4);
}

// 0x4B7AE0
bool combat_check_use_skill(int64_t source_obj)
{
    if (!combat_turn_based_active) {
        return combat_check_action_points(source_obj, 0);
    }

    return combat_check_action_points(source_obj, 4);
}

// 0x4B7BA0
int combat_move_cost(int64_t source_obj, int64_t target_loc, bool adjacent)
{
    int mult;

    mult = 2;
    if (obj_field_int32_get(source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && get_always_run(source_obj)) {
        mult = 1;
    }

    if (adjacent) {
        return mult * sub_426250(source_obj, target_loc);
    } else {
        return mult * sub_4261E0(source_obj, target_loc);
    }
}

// 0x4B7C20
int combat_required_action_points_get(void)
{
    return combat_required_action_points;
}

// 0x4B7C30
int combat_attack_cost(int64_t obj)
{
    int64_t weapon_obj;
    int speed;
    int action_points;

    weapon_obj = item_wield_get(obj, ITEM_INV_LOC_WEAPON);
    speed = item_weapon_magic_speed(weapon_obj, obj);
    if (speed > 24) {
        action_points = 1;
    } else if (speed > 20) {
        action_points = 2;
    } else {
        action_points = 8 - speed / 3;
        if (action_points < 1) {
            action_points = 1;
        }
    }

    return action_points;
}

// 0x4B7C90
void combat_turn_based_end_critter_turn(int64_t obj)
{
    if (combat_turn_based_active
        && dword_5FC240->obj == obj) {
        combat_action_points = 0;
        combat_turn_based_next_subturn();
    }
}

// 0x4B7CD0
bool combat_consume_action_points(int64_t obj, int action_points)
{
    bool is_pc;
    CombatContext combat;

    if (!combat_turn_based_active) {
        return true;
    }

    if (dword_5FC240->obj != obj) {
        return true;
    }

    is_pc = player_is_local_pc_obj(obj);
    if (combat_action_points >= action_points) {
        combat_action_points -= action_points;

        if (is_pc) {
            combat_callbacks.field_C(combat_action_points);
        }

        return true;
    }

    if (combat_action_points > 0
        && is_pc
        && critter_fatigue_current(obj) > 1) {
        sub_4B2210(OBJ_HANDLE_NULL, obj, &combat);
        combat.flags |= 0x80;
        combat.dam[DAMAGE_TYPE_FATIGUE] = 2;
        combat_dmg(&combat);

        combat_action_points = 0;
        combat_callbacks.field_C(0);

        return true;
    }

    combat_action_points = 0;
    combat_turn_based_next_subturn();

    return false;
}

// 0x4B7DC0
bool sub_4B7DC0(int64_t obj)
{
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DONTDRAW) != 0) {
        return !critter_is_sleeping(obj);
    } else {
        return false;
    }
}

// 0x4B7E00
void combat_turn_based_add_critter(int64_t obj)
{
    ObjectNode* prev = NULL;
    ObjectNode* curr;

    if (!combat_turn_based_active) {
        return;
    }

    curr = combat_critter_list.head;
    while (curr != NULL && curr->obj != obj) {
        prev = curr;
        curr = curr->next;
    }

    if (curr != NULL) {
        // Already added.
        return;
    }

    if (sub_4B7DC0(obj)) {
        tig_debug_printf("Combat: combat_turn_based_add_critter: WARNING: Attempt to add critter that is OF_DONTDRAW!\n");
    }

    curr = object_node_create();
    curr->obj = obj;
    curr->next = NULL;
    combat_debug(obj, "Adding Critter to List");

    if (prev != NULL) {
        prev->next = curr;
    } else {
        tig_debug_printf("Combat: combat_turn_based_add_critter: ERROR: Base list is EMPTY!\n");
        combat_critter_list.head = curr;
    }

    sort_combat_list();
}

// 0x4B7EB0
void sort_combat_list(void)
{
    ObjectNode* node;
    ObjectNode* tail;
    ObjectNode* prev;
    ObjectNode* tmp;
    int64_t leader_obj;
    bool process;
    char* name;
    int index;

    combat_debug(OBJ_HANDLE_NULL, "Sorting List");

    tail = NULL;
    prev = NULL;
    node = combat_critter_list.head;
    if (node != NULL) {
        do {
            process = true;
            if (obj_field_int32_get(node->obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
                leader_obj = critter_pc_leader_get(node->obj);
                if (leader_obj == OBJ_HANDLE_NULL
                    || (obj_field_int32_get(node->obj, OBJ_F_TYPE) != OBJ_TYPE_PC)) {
                    process = false;
                }
            }

            if (process) {
                if (prev != NULL) {
                    prev->next = node->next;
                } else {
                    combat_critter_list.head = node->next;
                }

                if (tail != NULL) {
                    tmp = tail;
                    while (tmp->next != NULL) {
                        tmp = tmp->next;
                    }
                    tmp->next = node;
                } else {
                    tail = node;
                }

                node->next = NULL;
                if (prev != NULL) {
                    node = prev->next;
                } else {
                    node = combat_critter_list.head;
                }
            } else {
                prev = node;
                node = node->next;
            }
        } while (node != NULL);

        if (tail != NULL) {
            if (prev != NULL) {
                prev->next = tail;
            } else {
                combat_critter_list.head = tail;
            }
        }
    }

    node = combat_critter_list.head;
    while (node != NULL) {
        combat_recalc_reaction(node->obj);
        node = node->next;
    }

    index = 0;
    node = combat_critter_list.head;
    while (node != NULL) {
        name = NULL;
        if (obj_handle_is_valid(node->obj)) {
            if (player_is_pc_obj(node->obj)) {
                obj_field_string_get(node->obj, OBJ_F_PC_PLAYER_NAME, &name);
            } else {
                obj_field_string_get(node->obj, OBJ_F_NAME, &name);
            }
        }

        tig_debug_printf("Combat: TB: DBG: List[%d]: %s\n",
            index,
            name != NULL ? name : " ");

        // NOTE: Original code is slightly different and buggy - it attempts
        // to release `name` depending on a flag which is not reset on every
        // iteration.
        if (name != NULL) {
            FREE(name);
        }

        node = node->next;
        index++;
    }
}

// 0x4B8040
bool sub_4B8040(int64_t obj)
{
    if (!combat_turn_based_active) {
        return false;
    }

    if (combat_turn_based_whos_turn_get() != obj) {
        return false;
    }

    if (combat_fast_turn_based) {
        return true;
    }

    if (player_is_pc_obj(obj)) {
        return false;
    }

    if (combat_critter_is_combat_mode_active(obj)) {
        return false;
    }

    if (anim_is_current_goal_type(obj, AG_ATTACK, NULL)) {
        return false;
    }

    if ((obj_field_int32_get(obj, OBJ_F_NPC_FLAGS) & ONF_BACKING_OFF) != 0) {
        return false;
    }

    return true;
}

// 0x4B80D0
int combat_turn_based_turn_get(void)
{
    return combat_turn_based_turn;
}

// 0x4B80E0
void combat_recalc_reaction(int64_t obj)
{
    // 0x5B5818
    static unsigned int dword_5B5818[7] = {
        OCF2_REACTION_0,
        OCF2_REACTION_1,
        OCF2_REACTION_2,
        OCF2_REACTION_3,
        OCF2_REACTION_4,
        OCF2_REACTION_5,
        OCF2_REACTION_6,
    };

    int64_t pc_obj;
    int reaction_level;
    int reaction_type;
    unsigned int flags;
    AnimFxNode node;

    if (!combat_turn_based_active) {
        return;
    }

    pc_obj = player_get_local_pc_obj();
    reaction_level = reaction_get(obj, pc_obj);
    reaction_type = reaction_translate(reaction_level);

    flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2);
    flags &= ~(OCF2_REACTION_0 | OCF2_REACTION_1 | OCF2_REACTION_2 | OCF2_REACTION_3 | OCF2_REACTION_4 | OCF2_REACTION_5 | OCF2_REACTION_6);
    flags |= dword_5B5818[reaction_type];
    obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS2, flags);

    if (critter_is_dead(obj) || sub_4B7DC0(obj)) {
        animfx_remove(&combat_eye_candies, obj, 0, -1);
    } else {
        sub_4CCD20(&combat_eye_candies, &node, obj, -1, 0);
        if (!animfx_has(&node)) {
            animfx_add(&node);
        }
    }
}

// 0x4B81B0
bool combat_set_blinded(int64_t obj)
{
    unsigned int flags;
    int tf_type;
    MesFileEntry mes_file_entry;

    flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS);
    if ((flags & OCF_BLINDED) != 0) {
        return false;
    }

    flags |= OCF_BLINDED;
    obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS, flags);

    tf_type = obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        ? TF_TYPE_RED
        : TF_TYPE_WHITE;

    mes_file_entry.num = 4; // "Blinded"
    mes_get_msg(combat_mes_file, &mes_file_entry);
    tf_add(obj, tf_type, mes_file_entry.str);

    return true;
}

// 0x4B8230
bool combat_auto_attack_get(int64_t obj)
{
    int player;

    if (combat_turn_based_is_active()) {
        return false;
    }

    if (!tig_net_is_active()) {
        return settings_get_value(&settings, AUTO_ATTACK_KEY);
    }

    player = multiplayer_find_slot_from_obj(obj);
    if (player == -1) {
        return false;
    }

    return (multiplayer_flags_get(player) & MULTIPLAYER_AUTO_ATTACK) != 0;
}

// 0x4B8280
void combat_auto_attack_set(bool value)
{
    int player;

    settings_set_value(&settings, AUTO_ATTACK_KEY, value);

    if (tig_net_is_active()) {
        player = multiplayer_find_slot_from_obj(player_get_local_pc_obj());
        if (player != -1) {
            if (value) {
                multiplayer_flags_set(player, MULTIPLAYER_AUTO_ATTACK);
            } else {
                multiplayer_flags_unset(player, MULTIPLAYER_AUTO_ATTACK);
            }
        }
    }
}

// 0x4B82E0
bool combat_taunts_get(void)
{
    if (!tig_net_is_active()) {
        return settings_get_value(&settings, COMBAT_TAUNTS_KEY);
    } else {
        return false;
    }
}

// 0x4B8300
void combat_taunts_set(bool value)
{
    if (!tig_net_is_active()) {
        settings_set_value(&settings, COMBAT_TAUNTS_KEY, value);
    }
}

// 0x4B8330
bool combat_auto_switch_weapons_get(int64_t obj)
{
    int player;

    if (!tig_net_is_active()) {
        return settings_get_value(&settings, AUTO_SWITCH_WEAPON_KEY);
    }

    player = multiplayer_find_slot_from_obj(obj);
    if (player == -1) {
        return false;
    }

    return (multiplayer_flags_get(player) & MULTIPLAYER_AUTO_SWITCH_WEAPONS) != 0;
}

// 0x4B8380
void combat_auto_switch_weapons_set(bool value)
{
    int player;

    settings_set_value(&settings, AUTO_SWITCH_WEAPON_KEY, value);

    if (tig_net_is_active()) {
        player = multiplayer_find_slot_from_obj(player_get_local_pc_obj());
        if (player != -1) {
            if (value) {
                multiplayer_flags_set(player, MULTIPLAYER_AUTO_SWITCH_WEAPONS);
            } else {
                multiplayer_flags_unset(player, MULTIPLAYER_AUTO_SWITCH_WEAPONS);
            }
        }
    }
}

// 0x4B83E0
void pc_switch_weapon(int64_t pc_obj, int64_t target_obj)
{
    if (pc_obj != OBJ_HANDLE_NULL
        && (!tig_net_is_active() || tig_net_is_host())
        && obj_field_int32_get(pc_obj, OBJ_F_TYPE) == OBJ_TYPE_PC
        && combat_auto_switch_weapons_get(pc_obj)) {
        item_wield_best(pc_obj, ITEM_INV_LOC_WEAPON, target_obj);
    }
}
