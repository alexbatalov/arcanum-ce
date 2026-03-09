#ifndef ARCANUM_GAME_SKILL_H_
#define ARCANUM_GAME_SKILL_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/object.h"

typedef enum BasicSkill {
    BASIC_SKILL_BOW,
    BASIC_SKILL_DODGE,
    BASIC_SKILL_MELEE,
    BASIC_SKILL_THROWING,
    BASIC_SKILL_BACKSTAB,
    BASIC_SKILL_PICK_POCKET,
    BASIC_SKILL_PROWLING,
    BASIC_SKILL_SPOT_TRAP,
    BASIC_SKILL_GAMBLING,
    BASIC_SKILL_HAGGLE,
    BASIC_SKILL_HEAL,
    BASIC_SKILL_PERSUATION,
    BASIC_SKILL_COUNT,
} BasicSkill;

#define IS_BASIC_SKILL_VALID(bs) ((bs) >= 0 && (bs) < BASIC_SKILL_COUNT)

typedef enum TechSkill {
    TECH_SKILL_REPAIR,
    TECH_SKILL_FIREARMS,
    TECH_SKILL_PICK_LOCKS,
    TECH_SKILL_DISARM_TRAPS,
    TECH_SKILL_COUNT,
} TechSkill;

#define IS_TECH_SKILL_VALID(ts) ((ts) >= 0 && (ts) < TECH_SKILL_COUNT)

typedef enum Skill {
    SKILL_BOW,
    SKILL_DODGE,
    SKILL_MELEE,
    SKILL_THROWING,
    SKILL_BACKSTAB,
    SKILL_PICK_POCKET,
    SKILL_PROWLING,
    SKILL_SPOT_TRAP,
    SKILL_GAMBLING,
    SKILL_HAGGLE,
    SKILL_HEAL,
    SKILL_PERSUATION,
    SKILL_REPAIR,
    SKILL_FIREARMS,
    SKILL_PICK_LOCKS,
    SKILL_DISARM_TRAPS,
    SKILL_COUNT,
} Skill;

#define IS_TECH_SKILL(sk) ((sk) >= BASIC_SKILL_COUNT)
#define GET_BASIC_SKILL(sk) (sk)
#define GET_TECH_SKILL(sk) ((sk) - BASIC_SKILL_COUNT)

typedef enum Training {
    TRAINING_NONE,
    TRAINING_APPRENTICE,
    TRAINING_EXPERT,
    TRAINING_MASTER,
    TRAINING_COUNT,
} Training;

#define IS_TRAINING_VALID(tr) ((tr) >= 0 && (tr) < TRAINING_COUNT)

typedef bool(SkillCallbacksF0)(int64_t source_obj, int64_t target_obj, bool success);
typedef bool(SkillStealItemFunc)(int64_t source_obj, int64_t target_obj, int64_t item_obj, bool success);
typedef bool(SkillPlantItemFunc)(int64_t source_obj, int64_t target_obj, int64_t item_obj, bool success);
typedef bool(SkillCallbacksFC)(int64_t source_obj, int64_t target_obj, bool success);
typedef bool(SkillDisarmTrapFunc)(int64_t source_obj, int64_t target_obj, bool success);
typedef bool(SkillRepairFunc)(int64_t source_obj, int64_t target_obj, bool success);
typedef bool(SkillNoRepairFunc)(int64_t source_obj, int64_t target_obj, bool success);
typedef bool(SkillLockFunc)(int64_t source_obj, int64_t target_obj, bool success);
typedef bool(SkillNoLockFunc)(int64_t source_obj);

typedef struct SkillCallbacks {
    SkillCallbacksF0* field_0;
    SkillStealItemFunc* steal_item_func;
    SkillPlantItemFunc* plant_item_output;
    SkillCallbacksFC* field_C;
    SkillDisarmTrapFunc* disarm_trap_func;
    SkillRepairFunc* repair_func;
    SkillNoRepairFunc* no_repair_func;
    SkillLockFunc* lock_func;
    SkillNoLockFunc* no_lock_func;
} SkillCallbacks;

// clang-format off
#define SKILL_INVOCATION_SUCCESS            0x00000001u
#define SKILL_INVOCATION_0x02               0x00000002u
#define SKILL_INVOCATION_0x04               0x00000004u
#define SKILL_INVOCATION_AIM                0x00000008u
#define SKILL_INVOCATION_CRITICAL           0x00000010u
#define SKILL_INVOCATION_PENALTY_MSR        0x00000040u
#define SKILL_INVOCATION_PENALTY_RANGE      0x00000080u
#define SKILL_INVOCATION_PENALTY_PERCEPTION 0x00000100u
#define SKILL_INVOCATION_PENALTY_COVER      0x00000200u
#define SKILL_INVOCATION_PENALTY_LIGHT      0x00000400u
#define SKILL_INVOCATION_PENALTY_INJURY     0x00000800u
#define SKILL_INVOCATION_FORCED             0x00001000u
#define SKILL_INVOCATION_CHECK_HEARING      0x00002000u
#define SKILL_INVOCATION_CHECK_SEEING       0x00004000u
#define SKILL_INVOCATION_BACKSTAB           0x00008000u
#define SKILL_INVOCATION_NO_MAGIC_ADJ       0x00010000u
#define SKILL_INVOCATION_BLOCKED_SHOT       0x00020000u
#define SKILL_INVOCATION_MAGIC_TECH_PENALTY 0x00040000u
// clang-format on

#define SKILL_INVOCATION_PENALTY_MASK (SKILL_INVOCATION_PENALTY_MSR \
    | SKILL_INVOCATION_PENALTY_RANGE                                \
    | SKILL_INVOCATION_PENALTY_PERCEPTION                           \
    | SKILL_INVOCATION_PENALTY_COVER                                \
    | SKILL_INVOCATION_PENALTY_LIGHT                                \
    | SKILL_INVOCATION_PENALTY_INJURY                               \
    | SKILL_INVOCATION_BLOCKED_SHOT                                 \
    | SKILL_INVOCATION_MAGIC_TECH_PENALTY)

typedef struct SkillInvocation {
    /* 0000 */ FollowerInfo source;
    /* 0030 */ FollowerInfo target;
    /* 0060 */ int64_t target_loc;
    /* 0068 */ FollowerInfo item;
    /* 0098 */ unsigned int flags;
    /* 009C */ int skill;
    /* 00A0 */ int modifier;
    /* 00A4 */ int hit_loc;
} SkillInvocation;

extern const char* basic_skill_lookup_keys_tbl[BASIC_SKILL_COUNT];
extern const char* tech_skill_lookup_keys_tbl[TECH_SKILL_COUNT];
extern const char* training_lookup_keys_tbl[TRAINING_COUNT];

bool skill_init(GameInitInfo* init_info);
void skill_set_callbacks(SkillCallbacks* callbacks);
void skill_exit(void);
bool skill_load(GameLoadInfo* load_info);
bool skill_save(TigFile* stream);
void skill_set_defaults(int64_t obj);
int basic_skill_base(int64_t obj, int bs);
int basic_skill_level(int64_t obj, int bs);
int basic_skill_points_get(int64_t obj, int bs);
int basic_skill_points_set(int64_t obj, int bs, int value);
int basic_skill_training_get(int64_t obj, int bs);
int basic_skill_training_set(int64_t obj, int bs, int training);
char* basic_skill_name(int bs);
char* basic_skill_description(int bs);
int basic_skill_money(int bs, int skill_level, int training);
int basic_skill_effectiveness(int64_t obj, int bs, int64_t target_obj);
int basic_skill_cost_inc(int64_t obj, int bs);
int basic_skill_cost_dec(int64_t obj, int bs);
int basic_skill_stat(int bs);
int basic_skill_min_stat_level_required(int skill_level);
int sub_4C6510(int64_t obj);
int skill_gambling_max_item_cost(int64_t obj);
int tech_skill_base(int64_t obj, int ts);
int tech_skill_level(int64_t obj, int ts);
int tech_skill_points_get(int64_t obj, int ts);
int tech_skill_points_set(int64_t obj, int ts, int value);
int tech_skill_training_get(int64_t obj, int ts);
int tech_skill_training_set(int64_t obj, int ts, int training);
char* tech_skill_name(int ts);
char* tech_skill_description(int ts);
int training_min_skill_level_required(int training);
char* training_name(int training);
int tech_skill_money(int ts, int skill_level, int training);
int tech_skill_effectiveness(int64_t obj, int ts, int64_t target_obj);
int tech_skill_cost_inc(int64_t obj, int ts);
int tech_skill_cost_dec(int64_t obj, int ts);
int tech_skill_stat(int ts);
int tech_skill_min_stat_level_required(int skill_level);
bool skill_check_stat(int64_t obj, int stat, int value);
bool skill_use(int64_t obj, int skill, int64_t target_obj, unsigned int flags);
bool skill_steal_item(int64_t obj, int64_t target_obj, int64_t item_obj);
bool skill_plant_item(int64_t obj, int64_t target_obj, int64_t item_obj);
bool skill_disarm_trap(int64_t obj, int a2, int64_t target_obj);
void skill_invocation_init(SkillInvocation* skill_invocation);
bool skill_invocation_recover(SkillInvocation* skill_invocation);
bool skill_invocation_run(SkillInvocation* skill_invocation);
int skill_invocation_difficulty(SkillInvocation* skill_invocation);
void skill_perform_repair_service(int64_t item_obj, int64_t npc_obj, int64_t pc_obj, int cost);
bool get_follower_skills(int64_t obj);
void set_follower_skills(bool enabled);
void skill_pick_best_follower(SkillInvocation* skill_invocation);
int64_t skill_supplementary_item(int64_t obj, int skill);

#endif /* ARCANUM_GAME_SKILL_H_ */
