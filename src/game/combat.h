#ifndef ARCANUM_GAME_COMBAT_H_
#define ARCANUM_GAME_COMBAT_H_

#include "game/context.h"
#include "game/damage_type.h"
#include "game/timeevent.h"

typedef void(CombatCallbackF0)(int a1);
typedef void(CombatCallbackF4)(void);
typedef void(CombatCallbackF8)(void);
typedef void(CombatCallbackFC)(int a1);
typedef void(CombatCallbackF10)(int64_t obj);

typedef struct CombatCallbacks {
    CombatCallbackF0* field_0;
    CombatCallbackF4* field_4;
    CombatCallbackF8* field_8;
    CombatCallbackFC* field_C;
    CombatCallbackF10* field_10;
} CombatCallbacks;

typedef enum Loudness {
    LOUDNESS_SILENT,
    LOUDNESS_NORMAL,
    LOUDNESS_LOUD,
    LOUDNESS_COUNT,
} Loudness;

typedef enum HitLocation {
    HIT_LOC_TORSO,
    HIT_LOC_HEAD,
    HIT_LOC_ARM,
    HIT_LOC_LEG,
    HIT_LOC_COUNT,
} HitLocation;

// clang-format off
#define CF_AIM                  0x00000001
#define CF_HIT                  0x00000002
#define CF_CRITICAL             0x00000004
#define CF_RANGED               0x00000200
#define CF_WEAPON_WEAR          0x00010000
#define CF_TRAP                 0x00080000
// clang-format on

// clang-format off
#define CDF_FULL                0x00000001
#define CDF_RESURRECT           0x00000002
#define CDF_DEATH               0x00000004
#define CDF_BONUS_DAM_50        0x00000008
#define CDF_BONUS_DAM_100       0x00000010
#define CDF_BONUS_DAM_200       0x00000020
#define CDF_STUN                0x00000040
#define CDF_KNOCKDOWN           0x00000080
#define CDF_KNOCKOUT            0x00000100
#define CDF_BLIND               0x00000200
#define CDF_CRIPPLE_ARM         0x00000400
#define CDF_CRIPPLE_LEG         0x00000800
#define CDF_SCAR                0x00001000
#define CDF_DROP_WEAPON         0x00002000
#define CDF_DAMAGE_WEAPON       0x00004000
#define CDF_DESTROY_WEAPON      0x00008000
#define CDF_EXPLODE_WEAPON      0x00010000
#define CDF_LOST_AMMO           0x00020000
#define CDF_DESTROY_AMMO        0x00040000
#define CDF_DROP_HELMET         0x00080000
#define CDF_DAMAGE_ARMOR        0x00100000
#define CDF_HAVE_DAMAGE         0x00200000
#define CDF_REANIMATE           0x00400000
#define CDF_SCALE               0x00800000
#define CDF_IGNORE_RESISTANCE   0x01000000
// clang-format on

typedef struct CombatContext {
    /* 0000 */ unsigned int flags;
    /* 0004 */ int padding_4;
    /* 0008 */ int64_t attacker_obj;
    /* 0010 */ int64_t weapon_obj;
    /* 0018 */ int skill;
    /* 001C */ int padding_1C;
    /* 0020 */ int64_t target_obj;
    /* 0028 */ int64_t field_28;
    /* 0030 */ int64_t field_30;
    /* 0038 */ int64_t target_loc;
    /* 0040 */ int hit_loc;
    /* 0044 */ int dam[DAMAGE_TYPE_COUNT];
    /* 0058 */ unsigned int dam_flags;
    /* 005C */ int total_dam;
    /* 0060 */ int game_difficulty;
    /* 0064 */ int field_64;
} CombatContext;

// Serializeable.
static_assert(sizeof(CombatContext) == 0x68, "wrong size");

bool combat_init(GameInitInfo* init_info);
void combat_exit(void);
void combat_reset(void);
bool combat_save(TigFile* stream);
bool combat_load(GameLoadInfo* load_info);
void combat_context_setup(int64_t attacker_obj, int64_t weapon_obj, int64_t target_obj, CombatContext* combat);
int64_t combat_critter_weapon(int64_t critter_obj);
void sub_4B2650(int64_t a1, int64_t a2, CombatContext* combat);
bool sub_4B2870(int64_t attacker_obj, int64_t target_obj, int64_t target_loc, int64_t projectile_obj, int range, int64_t cur_loc, int64_t a7);
int sub_4B3170(CombatContext* combat);
void sub_4B3BB0(int64_t attacker_obj, int64_t target_obj, int hit_loc);
void combat_throw(int64_t attacker_obj, int64_t weapon_obj, int64_t target_obj, int64_t target_loc, int hit_loc);
bool combat_critter_is_combat_mode_active(int64_t obj);
bool combat_can_exit_combat_mode(int64_t obj);
void combat_critter_deactivate_combat_mode(int64_t obj);
void combat_critter_activate_combat_mode(int64_t obj);
void sub_4B4320(int64_t obj);
void combat_dmg(CombatContext* combat);
void combat_acid_dmg(CombatContext* combat);
void combat_heal(CombatContext* combat);
int combat_hit_loc_penalty(int hit_loc);
int combat_projectile_rot(int64_t from_loc, int64_t to_loc);
tig_art_id_t combat_projectile_art_id_rotation_set(tig_art_id_t aid, int projectile_rot);
bool combat_set_callbacks(CombatCallbacks* callbacks);
bool combat_is_turn_based(void);
bool sub_4B6C90(bool turn_based);
void combat_turn_based_toggle(void);
bool combat_turn_based_is_active(void);
int64_t combat_turn_based_whos_turn_get(void);
void combat_turn_based_whos_turn_set(int64_t obj);
void sub_4B7010(int64_t obj);
bool combat_tb_timeevent_process(TimeEvent* timeevent);
void combat_turn_based_next_subturn(void);
int combat_action_points_get(void);
bool combat_check_action_points(int64_t obj, int required_action_points);
bool combat_check_move_to(int64_t source_obj, int64_t target_loc);
bool combat_check_attack(int64_t source_obj, int64_t target_obj);
bool combat_check_use_obj(int64_t source_obj, int64_t target_obj);
bool combat_check_pick_item(int64_t source_obj, int64_t target_obj);
bool combat_check_cast_spell(int64_t source_obj);
bool combat_check_use_skill(int64_t source_obj);
int combat_required_action_points_get(void);
int combat_attack_cost(int64_t obj);
void combat_turn_based_end_critter_turn(int64_t obj);
bool combat_consume_action_points(int64_t obj, int action_points);
void combat_turn_based_add_critter(int64_t obj);
bool sub_4B8040(int64_t obj);
int combat_turn_based_turn_get(void);
void combat_recalc_reaction(int64_t obj);
bool combat_set_blinded(int64_t obj);
bool combat_auto_attack_get(int64_t obj);
void combat_auto_attack_set(bool value);
bool combat_taunts_get(void);
void combat_taunts_set(bool value);
bool combat_auto_switch_weapons_get(int64_t obj);
void combat_auto_switch_weapons_set(bool value);

#endif /* ARCANUM_GAME_COMBAT_H_ */
