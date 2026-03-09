#ifndef ARCANUM_GAME_CRITTER_H_
#define ARCANUM_GAME_CRITTER_H_

#include "game/context.h"
#include "game/timeevent.h"

typedef enum SocialClass {
    SOCIAL_CLASS_NOBLE,
    SOCIAL_CLASS_PRIEST,
    SOCIAL_CLASS_WIZARD,
    SOCIAL_CLASS_TECHNOLOGIST,
    SOCIAL_CLASS_SHOPKEEPER,
    SOCIAL_CLASS_GUARD,
    SOCIAL_CLASS_CITY_DWELLER,
    SOCIAL_CLASS_VILLAGER,
    SOCIAL_CLASS_BEGGAR,
    SOCIAL_CLASS_THIEF,
    SOCIAL_CLASS_BANDIT,
    MAX_SOCIAL_CLASS,
} SocialClass;

typedef enum EncumbranceLevel {
    ENCUMBRANCE_LEVEL_NONE,
    ENCUMBRANCE_LEVEL_LIGHT,
    ENCUMBRANCE_LEVEL_MODERATE,
    ENCUMBRANCE_LEVEL_MEDIUM,
    ENCUMBRANCE_LEVEL_SIGNIFICANT,
    ENCUMBRANCE_LEVEL_HEAVY,
    ENCUMBRANCE_LEVEL_SEVERE,
    MAX_ENCUMBRANCE_LEVEL,
} EncumbranceLevel;

bool critter_init(GameInitInfo* init_info);
void critter_exit(void);
int critter_social_class_get(int64_t obj);
int critter_social_class_set(int64_t obj, int value);
const char* critter_social_class_name(int social_class);
int critter_faction_get(int64_t obj);
int critter_faction_set(int64_t obj, int value);
bool critter_faction_same(int64_t a, int64_t b);
int critter_origin_get(int64_t obj);
int critter_origin_set(int64_t obj, int value);
bool critter_origin_same(int64_t a, int64_t b);
bool critter_is_pc(int64_t obj);
int critter_fatigue_pts_get(int64_t obj);
int critter_fatigue_pts_set(int64_t obj, int value);
int critter_fatigue_adj_get(int64_t obj);
int critter_fatigue_adj_set(int64_t obj, int value);
int critter_fatigue_damage_get(int64_t obj);
int critter_fatigue_damage_set(int64_t obj, int value);
int critter_fatigue_max(int64_t obj);
int critter_fatigue_current(int64_t obj);
bool critter_is_prone(int64_t obj);
bool critter_is_active(int64_t obj);
bool critter_is_unconscious(int64_t obj);
bool critter_is_sleeping(int64_t obj);
bool critter_is_dead(int64_t obj);
void critter_kill(int64_t obj);
void critter_notify_killed(int64_t victim_obj, int64_t killer_obj, int anim);
void sub_45DC90(int64_t killer_obj, int64_t victim_obj, bool a3);
int64_t critter_pc_leader_get(int64_t obj);
int64_t critter_leader_get(int64_t obj);
void critter_leader_set(int64_t follower_obj, int64_t leader_obj);
bool critter_follow(int64_t follower_obj, int64_t leader_obj, bool force);
bool critter_disband(int64_t obj, bool force);
bool sub_45E180(int64_t obj);
void critter_toggle_on_off(int64_t obj);
bool critter_party_same(int64_t a1, int64_t a2);
int critter_num_followers(int64_t obj, bool exclude_forced_followers);
int64_t critter_follower_prev(int64_t critter_obj);
int64_t critter_follower_next(int64_t critter_obj);
bool critter_fatigue_timeevent_process(TimeEvent* timeevent);
bool critter_fatigue_timeevent_schedule(int64_t obj, int type, int delay);
void critter_resting_heal(int64_t critter_obj, int hours);
bool critter_resting_timeevent_process(TimeEvent* timeevent);
bool critter_resting_timeevent_schedule(int64_t obj);
bool critter_decay_timeevent_process(TimeEvent* timeevent);
bool critter_decay_timeevent_schedule(int64_t obj);
void critter_decay_timeevent_cancel(int64_t obj);
bool critter_npc_combat_focus_wipe_timeevent_process(TimeEvent* timeevent);
bool critter_npc_combat_focus_wipe_schedule(int64_t obj);
bool critter_is_concealed(int64_t obj);
void critter_set_concealed(int64_t obj, bool concealed);
tig_art_id_t critter_conceal_aid(tig_art_id_t art_id);
bool critter_is_facing_to(int64_t a, int64_t b);
bool critter_check_stat(int64_t obj, int stat, int mod);
int critter_xp_worth(int64_t obj);
void critter_give_xp(int64_t obj, int a2);
bool critter_enter_bed(int64_t obj, int64_t bed);
void critter_leave_bed(int64_t obj, int64_t bed);
bool critter_has_bad_associates(int64_t obj);
bool critter_can_open_portals(int64_t obj);
bool critter_can_jump_window(int64_t obj);
void critter_stay_close(int64_t npc_obj);
void critter_spread_out(int64_t npc_obj);
int64_t critter_substitute_inventory_get(int64_t obj);
int critter_teleport_map_get(int64_t obj);
void sub_45F710(int64_t obj);
bool critter_is_monstrous(int64_t critter_obj);
int critter_encumbrance_level_get(int64_t obj);
void critter_encumbrance_level_recalc(int64_t obj, int value);
void critter_encumbrance_recalc_feedback_enable(void);
void critter_encumbrance_recalc_feedback_disable(void);
const char* critter_encumbrance_level_name(int level);
int critter_encumbrance_level_ratio(int level);
int critter_description_get(int64_t a, int64_t b);
bool critter_can_backstab(int64_t obj, int64_t tgt);
tig_art_id_t critter_light_get(int64_t critter_obj, unsigned int* color_ptr);
bool critter_has_dark_sight(int64_t obj);
bool critter_is_dumb(int64_t obj);
void critter_debug_obj(int64_t obj);

#endif /* ARCANUM_GAME_CRITTER_H_ */
