#ifndef ARCANUM_GAME_AI_H_
#define ARCANUM_GAME_AI_H_

#include "game/context.h"
#include "game/object.h"
#include "game/timeevent.h"

typedef void(AiFloatLineFunc)(int64_t npc_obj, int64_t pc_obj, const char* str, int speech_id);
typedef void(Func5F848C)(int64_t, int);

typedef enum AiDangerSourceType {
    AI_DANGER_SOURCE_TYPE_NONE,
    AI_DANGER_SOURCE_TYPE_COMBAT_FOCUS,
    AI_DANGER_SOURCE_TYPE_FLEE,
    AI_DANGER_SOURCE_TYPE_SURRENDER,
} AiDangerSourceType;

typedef enum AiSpeak {
    AI_SPEAK_OK,
    AI_SPEAK_DEAD,
    AI_SPEAK_COMBAT,
    AI_SPEAK_ANIM,
    AI_SPEAK_REACTION,
    AI_SPEAK_CANNOT,
    AI_SPEAK_SLEEP_UNCONSCIOUS,
} AiSpeak;

typedef enum AiFollow {
    AI_FOLLOW_OK,

    // The NPC will not follow the PC because the PC is too good.
    AI_FOLLOW_TOO_GOOD,

    // The NPC will not follow the PC because the PC is too evil.
    AI_FOLLOW_TOO_BAD,

    // The NPC will not follow the PC because the NPC dislikes the PC.
    AI_FOLLOW_DISLIKE,

    // The NPC will not follow the PC because the NPC is already grouped with
    // another PC who is at least as charismatic.
    AI_FOLLOW_ALREADY_IN_GROUP,

    // The NPC will not follow the PC because the PC is at his charisma limit
    // (which is greater zero).
    AI_FOLLOW_LIMIT_REACHED,

    // The NPC will not follow the PC because the PC's charisma does not allow
    // for followers.
    AI_FOLLOW_NOT_ALLOWED,

    // The NPC is too high level for the PC to be a follower.
    AI_FOLLOW_LOW_LEVEL,
} AiFollow;

typedef enum AiKos {
    AI_KOS_NO,
    AI_KOS_FACTION,
    AI_KOS_REACTION,
    AI_KOS_ALIGNMENT,
    AI_KOS_GUARD,
    AI_KOS_DECOY,
} AiKos;

typedef enum AiProtect {
    AI_PROTECT_NO,
    AI_PROTECT_FACTION,
    AI_PROTECT_ORIGIN,
    AI_PROTECT_GROUP,
    AI_PROTECT_SELF,
} AiProtect;

typedef enum AiUpsetAttacking {
    AI_UPSET_ATTACKING_NONE,
    AI_UPSET_ATTACKING_GOOD,
    AI_UPSET_ATTACKING_ORIGIN,
    AI_UPSET_ATTACKING_FACTION,
    AI_UPSET_ATTACKING_PARTY,
} AiUpsetAttacking;

typedef enum AiAttemptOpenPortal {
    AI_ATTEMPT_OPEN_PORTAL_OK,
    AI_ATTEMPT_OPEN_PORTAL_LOCKED,
    AI_ATTEMPT_OPEN_PORTAL_JAMMED,
    AI_ATTEMPT_OPEN_PORTAL_MAGICALLY_HELD,
    AI_ATTEMPT_OPEN_PORTAL_NOT_ALLOWED,
    AI_ATTEMPT_OPEN_PORTAL_NOT_CRITTER,
} AiAttemptOpenPortal;

typedef enum AiAttemptOpenContainer {
    AI_ATTEMPT_OPEN_CONTAINER_OK,
    AI_ATTEMPT_OPEN_CONTAINER_LOCKED,
    AI_ATTEMPT_OPEN_CONTAINER_JAMMED,
    AI_ATTEMPT_OPEN_CONTAINER_MAGICALLY_HELD,
    AI_ATTEMPT_OPEN_CONTAINER_NOT_ALLOWED,
} AiAttemptOpenContainer;

typedef struct AiRedirect {
    /* 0000 */ int64_t source_obj;
    /* 0008 */ int64_t target_obj;
    /* 0010 */ int min_iq;
    /* 0014 */ unsigned int critter_flags;
} AiRedirect;

typedef enum AiNpcWitnessPcCritical {
    AI_NPC_WITNESS_PC_CRITICAL_HIT,
    AI_NPC_WITNESS_PC_CRITICAL_MISS,
} AiNpcWitnessPcCritical;

typedef enum AiUseSkill {
    AI_USE_SKILL_OK,
    AI_USE_SKILL_BAD_SOURCE,
    AI_USE_SKILL_BAD_TARGET,
} AiUseSkill;

typedef enum AiActionType {
    AI_ACTION_TYPE_UNKNOWN,
    AI_ACTION_TYPE_CAST_SPELL,
    AI_ACTION_TYPE_USE_ITEM,
    AI_ACTION_TYPE_USE_SKILL,
} AiActionType;

bool ai_init(GameInitInfo* init_info);
void ai_exit(void);
bool ai_mod_load(void);
void ai_mod_unload(void);
void ai_set_callbacks(Func5F848C* a1, AiFloatLineFunc* float_line_func);
void ai_process(int64_t obj);
void ai_redirect_init(AiRedirect* ai_redirect, int64_t source_obj, int64_t target_obj);
void ai_redirect_perform(AiRedirect* ai_redirect);
void ai_attack(int64_t source_obj, int64_t target_obj, int loudness, unsigned int flags);
void sub_4A9AD0(int64_t attacker_obj, int64_t target_obj);
void ai_notify_explosion_dynamite(int64_t pc_obj);
void ai_notify_killed(int64_t victim_obj, int64_t killer_obj);
void sub_4AA300(int64_t a1, int64_t a2);
void ai_stop_attacking(int64_t obj);
void sub_4AA580(int64_t obj);
void ai_npc_wait(int64_t obj);
void ai_npc_unwait(int64_t obj, bool force);
bool ai_npc_wait_here_timeevent_process(TimeEvent* timeevent);
bool ai_is_fighting(int64_t obj);
int ai_object_hp_ratio(int64_t obj);
int ai_critter_fatigue_ratio(int64_t obj);
int64_t sub_4ABBC0(int64_t obj);
bool ai_timeevent_process(TimeEvent* timeevent);
void sub_4AD6E0(int64_t obj);
void ai_timeevent_clear(int64_t obj);
int ai_can_speak(int64_t npc_obj, int64_t pc_obj, bool a3);
int ai_check_follow(int64_t npc_obj, int64_t pc_obj, bool ignore_charisma_limits);
void sub_4AD790(int64_t obj, int a2);
int sub_4ADE00(int64_t source_obj, int64_t target_loc, int64_t* block_obj_ptr);
void ai_switch_weapon(int64_t obj);
void ai_calc_party_size_and_level(int64_t obj, int* cnt_ptr, int* lvl_ptr);
int ai_check_kos(int64_t source_obj, int64_t target_obj);
void ai_objects_in_radius(int64_t obj, int radius, ObjectList* objects, unsigned int flags);
int ai_check_use_skill(int64_t source_obj, int64_t target_obj, int64_t item_obj, int skill);
void ai_npc_witness_pc_critical(int64_t pc_obj, int type);
void ai_npc_near_death(int64_t npc_obj, int64_t pc_obj);
bool ai_critter_can_open_portals(int64_t obj);
int ai_attempt_open_portal(int64_t obj, int64_t portal_obj, int dir);
int ai_attempt_open_container(int64_t obj, int64_t container_obj);
void ai_notify_portal_container_guards(int64_t critter_obj, int64_t target_obj, bool a3, int loudness);
void ai_flee(int64_t obj, int64_t danger_obj);
void ai_stop_fleeing(int64_t obj);
void ai_set_no_flee(int64_t obj);
bool ai_surrendered(int64_t obj, int64_t* danger_source_ptr);
int ai_can_see(int64_t source_obj, int64_t target_obj);
int ai_can_hear(int64_t a1, int64_t a2, int loudness);
void ai_npc_fighting_toggle(void);
bool ai_shitlist_has(int64_t npc_obj, int64_t shit_obj);
int ai_max_dialog_distance(int64_t obj);
void ai_target_lock(int64_t obj, int64_t tgt);
void ai_target_unlock(int64_t obj);

#endif /* ARCANUM_GAME_AI_H_ */
