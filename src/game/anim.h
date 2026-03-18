#ifndef ARCANUM_GAME_ANIM_H_
#define ARCANUM_GAME_ANIM_H_

#include "game/anim_private.h"
#include "game/combat.h"
#include "game/context.h"
#include "game/timeevent.h"

// TODO: Figure out priority meaning.

#define PRIORITY_NONE 0
#define PRIORITY_1 1
#define PRIORITY_2 2
#define PRIORITY_3 3
#define PRIORITY_4 4
#define PRIORITY_5 5
#define PRIORITY_HIGHEST 6

#define ANIM_ID_STR_SIZE 36

typedef enum AnimEyeCandy {
    ANIM_EYE_CANDY_NORMAL_BLOOD_SPLOTCH,
    ANIM_EYE_CANDY_POISON_BLOOD_SPLOTCH,
    ANIM_EYE_CANDY_ELECTRICAL_BLOOD_SPLOTCH,
    ANIM_EYE_CANDY_FIRE_BLOOD_SPLOTCH,
    ANIM_EYE_CANDY_FATIGUE_BLOOD_SPLOTCH,
    ANIM_EYE_CANDY_CRITICAL_SPLOTCH,
    ANIM_EYE_CANDY_UNDEAD_BLOOD_SPLOTCH,
    ANIM_EYE_CANDY_STONED_BLOOD_SPLOTCH,
    ANIM_EYE_CANDY_NORMAL_BLOOD_SPLOTCH_X2,
    ANIM_EYE_CANDY_NORMAL_BLOOD_SPLOTCH_X3,
    ANIM_EYE_CANDY_NORMAL_BLOOD_SPLOTCH_X4,
    ANIM_EYE_CANDY_COUNT,
} AnimEyeCandy;

typedef enum AnimWeaponEyeCandyType {
    ANIM_WEAPON_EYE_CANDY_TYPE_POWER_GATHER,
    ANIM_WEAPON_EYE_CANDY_TYPE_FIRE,
    ANIM_WEAPON_EYE_CANDY_TYPE_PROJECTILE,
    ANIM_WEAPON_EYE_CANDY_TYPE_HIT,
    ANIM_WEAPON_EYE_CANDY_TYPE_SECONDARY_HIT,
    ANIM_WEAPON_EYE_CANDY_TYPE_COUNT,
} AnimWeaponEyeCandyType;

typedef enum BloodSplotchType {
    BLOOD_SPLOTCH_TYPE_NONE,
    BLOOD_SPLOTCH_TYPE_NORMAL,
    BLOOD_SPLOTCH_TYPE_POISON,
    BLOOD_SPLOTCH_TYPE_ELECTRICAL,
    BLOOD_SPLOTCH_TYPE_FIRE,
    BLOOD_SPLOTCH_TYPE_FATIGUE,
    BLOOD_SPLOTCH_TYPE_CRITICAL,
} BloodSplotchType;

extern AnimGoalNode* anim_goal_nodes[];

bool anim_init(GameInitInfo* init_info);
void anim_exit(void);
void anim_reset(void);
bool anim_id_to_run_info(AnimID* anim_id, AnimRunInfo** run_info_ptr);
void anim_break_nodes_to_map(const char* map);
void anim_save_nodes_to_map(const char* map);
void anim_load_nodes_from_map(const char* map);
bool anim_id_is_equal(AnimID* a, AnimID* b);
void anim_id_init(AnimID* anim_id);
void anim_id_to_str(AnimID* anim_id, char* buffer);
bool anim_save(TigFile* stream);
bool anim_load(GameLoadInfo* load_info);
void anim_debug_enable(void);
bool sub_423300(int64_t obj, AnimID* anim_id);
int sub_4233D0(int64_t obj);
bool anim_is_idle(int64_t obj);
bool anim_is_fidgeting(int64_t obj);
bool anim_timeevent_process(TimeEvent* timeevent);
void sub_423E60(const char* msg);
void anim_catch_up_enable(void);
void anim_catch_up_disable(void);
void sub_423FE0(void (*func)(void));
bool sub_423FF0(int64_t obj);
bool sub_424070(int64_t obj, int priority_level, bool a3, bool a4);
bool anim_goal_interrupt_all_goals(void);
bool anim_goal_interrupt_all_goals_of_priority(int priority_level);
bool anim_goal_interrupt_all_for_tb_combat(void);
bool anim_eye_candy_interrupt(int64_t obj, tig_art_id_t eye_candy_id, int mt_id);
bool anim_eye_candy_is_active(int64_t obj, tig_art_id_t eye_candy_id, int mt_id);
int sub_4261E0(int64_t a1, int64_t a2);
int sub_426250(int64_t a1, int64_t a2);
bool sub_426560(int64_t obj, int64_t from, int64_t to, AnimPath* path, unsigned int flags);
bool sub_427000(int64_t obj);
void anim_goal_reset_position_mp(AnimID* anim_id, int64_t obj, int64_t loc, tig_art_id_t art_id, unsigned int flags, int offset_x, int offset_y);
bool anim_fidget_timeevent_process(TimeEvent* timeevent);
void sub_430460(void);
void sub_430490(int64_t obj, int offset_x, int offset_y);
bool get_always_run(int64_t obj);
void set_always_run(bool value);
bool anim_goal_animate(int64_t obj, int anim);
void sub_432D90(int64_t obj);
void anim_play_blood_splotch_fx(int64_t obj, int blood_splotch_type, int damage_type, CombatContext* combat);
void anim_lag_icon_add(int64_t obj);
void anim_lag_icon_remove(int64_t obj);
bool anim_goal_rotate(int64_t obj, int rot);
bool anim_goal_animate_loop(int64_t obj);
bool anim_goal_move_to_tile(int64_t obj, int64_t loc);
bool sub_433A00(int64_t obj, int64_t loc, bool a3);
bool anim_goal_run_to_tile(int64_t obj, int64_t loc);
bool sub_434030(int64_t obj, int64_t loc);
bool sub_4341C0(int64_t source_obj, int64_t target_loc, int range);
bool sub_434400(int64_t source_obj, int64_t target_loc, int range);
bool anim_goal_follow_obj(int64_t source_obj, int64_t target_obj);
bool sub_4348E0(int64_t obj, int action_points);
bool anim_goal_flee(int64_t obj, int64_t flee_from);
bool anim_goal_attack(int64_t attacker_obj, int64_t target_obj);
bool anim_goal_attack_ex(int64_t attacker_obj, int64_t target_obj, int sound_id);
bool anim_goal_get_up(int64_t obj);
bool anim_goal_knockback(int64_t target_obj, int rot, int range, int64_t source_obj);
bool anim_goal_throw_item(int64_t obj, int64_t item_obj, int64_t target_loc);
bool anim_goal_dying(int64_t obj, int anim);
bool anim_goal_use_skill_on(int64_t obj, int64_t target_obj, int64_t item_obj, int skill, unsigned int flags);
bool anim_goal_use_item_on_obj_with_skill(int64_t obj, int64_t item_obj, int64_t target_obj, int skill, int modifier);
bool anim_goal_use_item_on_obj(int64_t obj, int64_t target_obj, int64_t item_obj, unsigned int flags);
bool anim_goal_use_item_on_loc(int64_t obj, int64_t target_loc, int64_t item_obj, unsigned int flags);
bool anim_goal_pickup_item(int64_t obj, int64_t item_obj);
bool anim_goal_animate_stunned(int64_t obj);
bool anim_goal_projectile(int64_t source_obj, int64_t missile_obj, tig_art_id_t missile_art_id, int a4, int a5, int64_t target_obj, int64_t target_loc, int64_t weapon_obj);
bool sub_435A00(int64_t proj_obj, int64_t a2, int64_t a3);
bool anim_goal_knockdown(int64_t obj);
bool anim_goal_make_knockdown(int64_t obj);
bool anim_goal_fidget(int64_t critter_obj);
bool sub_435CE0(int64_t obj);
bool anim_goal_unconceal(int64_t critter_obj);
bool anim_goal_wander(int64_t obj, int64_t tether_loc, int radius);
bool anim_goal_wander_seek_darkness(int64_t obj, int64_t tether_loc, int radius);
bool anim_goal_please_move(int64_t obj, int64_t target_obj);
void sub_4364D0(int64_t obj);
bool anim_goal_attempt_spread_out(int64_t a1, int64_t a2);
void turn_on_flags(AnimID anim_id, unsigned int flags1, unsigned int flags2);
void turn_on_running(AnimID anim_id);
void sub_436C20(void);
void sub_436C50(AnimID anim_id);
void sub_436C80(void);
void sub_436CF0(void);
void sub_436D20(unsigned int flags1, unsigned int flags2);
void sub_436ED0(AnimID anim_id);
void anim_speed_recalc(int64_t obj);
bool sub_4372B0(int64_t a1, int64_t a2);
int num_goal_subslots_in_use(AnimID* anim_id);
bool is_anim_forever(AnimID* anim_id);
bool anim_play_weapon_fx(CombatContext* combat, int64_t source_obj, int64_t target_obj, AnimWeaponEyeCandyType which);
void sub_437980(void);
bool sub_437CF0(int a1, int a2, int a3);

#endif /* ARCANUM_GAME_ANIM_H_ */
