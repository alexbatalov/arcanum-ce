#ifndef ARCANUM_GAME_ANIM_PRIVATE_H_
#define ARCANUM_GAME_ANIM_PRIVATE_H_

#include "game/context.h"
#include "game/object.h"
#include "game/timeevent.h"

#define ASSERT(x)                                                                      \
    if (!(x)) {                                                                        \
        tig_debug_printf("ASSERTION: File:%s, Line:%d: %s\n", __FILE__, __LINE__, #x); \
        anim_stats();                                                                  \
    }

typedef enum AnimGoal {
    AG_ANIMATE,
    AG_ANIMATE_LOOP,
    AG_ANIM_FIDGET,
    AG_MOVE_TO_TILE,
    AG_RUN_TO_TILE,
    AG_ATTEMPT_MOVE,
    AG_MOVE_TO_PAUSE,
    AG_MOVE_NEAR_TILE,
    AG_MOVE_NEAR_OBJ,
    AG_MOVE_STRAIGHT,
    AG_ATTEMPT_MOVE_STRAIGHT,
    AG_OPEN_DOOR,
    AG_ATTEMPT_OPEN_DOOR,
    AG_UNLOCK_DOOR,
    AG_JUMP_WINDOW,
    AG_PICKUP_ITEM,
    AG_ATTEMPT_PICKUP,
    AG_PICKPOCKET,
    AG_ATTACK,
    AG_ATTEMPT_ATTACK,
    AG_KILL,
    AG_TALK,
    AG_PICK_WEAPON,
    AG_CHASE,
    AG_FOLLOW,
    AG_FLEE,
    AG_THROW_SPELL,
    AG_ATTEMPT_SPELL,
    AG_SHOOT_SPELL,
    AG_HIT_BY_SPELL,
    AG_HIT_BY_WEAPON,
    AG_DYING,
    AG_DESTROY_OBJ,
    AG_USE_SKILL_ON,
    AG_ATTEMPT_USE_SKILL_ON,
    AG_SKILL_CONCEAL,
    AG_PROJECTILE,
    AG_THROW_ITEM,
    AG_USE_OBJECT,
    AG_USE_ITEM_ON_OBJECT,
    AG_USE_ITEM_ON_OBJECT_WITH_SKILL,
    AG_USE_ITEM_ON_TILE,
    AG_USE_ITEM_ON_TILE_WITH_SKILL,
    AG_KNOCKBACK,
    AG_FLOATING,
    AG_EYE_CANDY,
    AG_EYE_CANDY_REVERSE,
    AG_EYE_CANDY_CALLBACK,
    AG_EYE_CANDY_REVERSE_CALLBACK,
    AG_CLOSE_DOOR,
    AG_ATTEMPT_CLOSE_DOOR,
    AG_ANIMATE_REVERSE,
    AG_MOVE_AWAY_FROM_OBJ,
    AG_ROTATE,
    AG_UNCONCEAL,
    AG_RUN_NEAR_TILE,
    AG_RUN_NEAR_OBJ,
    AG_ANIMATE_STUNNED,
    AG_EYE_CANDY_END_CALLBACK,
    AG_EYE_CANDY_REVERSE_END_CALLBACK,
    AG_ANIMATE_KNEEL_MAGIC_HANDS,
    AG_ATTEMPT_MOVE_NEAR,
    AG_KNOCK_DOWN,
    AG_ANIM_GET_UP,
    AG_ATTEMPT_MOVE_STRAIGHT_KNOCKBACK,
    AG_WANDER,
    AG_WANDER_SEEK_DARKNESS,
    AG_USE_PICKLOCK_SKILL_ON,
    AG_PLEASE_MOVE,
    AG_ATTEMPT_SPREAD_OUT,
    AG_ANIMATE_DOOR_OPEN,
    AG_ANIMATE_DOOR_CLOSED,
    AG_PEND_CLOSING_DOOR,
    AG_THROW_SPELL_FRIENDLY,
    AG_ATTEMPT_SPELL_FRIENDLY,
    AG_EYE_CANDY_FIRE_DMG,
    AG_EYE_CANDY_REVERSE_FIRE_DMG,
    AG_ANIMATE_LOOP_FIRE_DMG,
    AG_ATTEMPT_MOVE_STRAIGHT_SPELL,
    AG_MOVE_NEAR_OBJ_COMBAT,
    AG_ATTEMPT_MOVE_NEAR_COMBAT,
    AG_USE_CONTAINER,
    AG_THROW_SPELL_W_CAST_ANIM,
    AG_ATTEMPT_SPELL_W_CAST_ANIM,
    AG_THROW_SPELL_W_CAST_ANIM_2NDARY,
    AG_BACK_OFF_FROM,
    AG_ATTEMPT_USE_PICKPOCKET_SKILL_ON,
    ANIM_GOAL_MAX,
} AnimGoal;

typedef enum AgDataType {
    AGDATATYPE_INT,
    AGDATATYPE_OBJ,
    AGDATATYPE_LOC,
    AGDATATYPE_SOUND,
} AgDataType;

struct AnimRunInfo;

typedef struct AnimGoalSubNode {
    /* 0000 */ bool (*func)(struct AnimRunInfo* run_info);
    /* 0004 */ int params[2];
    /* 000C */ int field_C;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ int field_18;
    /* 001C */ int field_1C;
} AnimGoalSubNode;

typedef struct AnimGoalNode {
    /* 0000 */ int num_subnodes;
    /* 0004 */ int priority_level;
    /* 0008 */ int field_8;
    /* 000C */ int field_C;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14[3];
    /* 0020 */ AnimGoalSubNode subnodes[15];
} AnimGoalNode;

typedef struct AnimID {
    int slot_num;
    int field_4;
    int field_8;
} AnimID;

// Serializeable.
static_assert(sizeof(AnimID) == 0xC, "wrong size");

typedef struct AGModifyData {
    /* 0000 */ AnimID id;
    /* 000C */ int flags;
    /* 0010 */ int path_flags;
    /* 0014 */ int field_14;
    /* 0018 */ int64_t loc;
    /* 0020 */ int64_t location;
    /* 0028 */ tig_art_id_t current_aid;
    /* 002C */ int path_curr;
} AGModifyData;

// Serializeable.
static_assert(sizeof(AGModifyData) == 0x30, "wrong size");

typedef union AnimRunInfoParam {
    int64_t obj;
    int64_t loc;
    int data;
} AnimRunInfoParam;

typedef enum AgData {
    AGDATA_SELF_OBJ,
    AGDATA_TARGET_OBJ,
    AGDATA_BLOCK_OBJ,
    AGDATA_SCRATCH_OBJ,
    AGDATA_PARENT_OBJ,
    AGDATA_TARGET_TILE,
    AGDATA_ORIGINAL_TILE,
    AGDATA_RANGE_DATA,
    AGDATA_ANIM_ID,
    AGDATA_ANIM_ID_PREVIOUS,
    AGDATA_ANIM_DATA,
    AGDATA_SPELL_DATA,
    AGDATA_SKILL_DATA,
    AGDATA_FLAGS_DATA,
    AGDATA_SCRATCH_VAL1,
    AGDATA_SCRATCH_VAL2,
    AGDATA_SCRATCH_VAL3,
    AGDATA_SCRATCH_VAL4,
    AGDATA_SCRATCH_VAL5,
    AGDATA_SCRATCH_VAL6,
    AGDATA_SOUND_HANDLE,
    AGDATA_COUNT,
} AgData;

#define AGDATA_SELF_TILE 31
#define AGDATA_NULL_OBJ 33
#define AGDATA_FORCE_TARGET_TILE 34

typedef struct AnimGoalData {
    /* 0000 */ int type;
    /* 0004 */ int padding_4;
    /* 0008 */ AnimRunInfoParam params[AGDATA_COUNT];
    /* 00B0 */ Ryan field_B0[5];
} AnimGoalData;

// Serializeable.
static_assert(sizeof(AnimGoalData) == 0x178, "wrong size");

typedef struct AnimPath {
    /* 0000 */ unsigned int flags;
    /* 0004 */ int8_t rotations[200];
    /* 00CC */ int field_CC;
    /* 00D0 */ int baseRot; // TODO: Normalize to snake_case.
    /* 00D4 */ int curr;
    /* 00D8 */ int max;
    /* 00DC */ int subsequence;
    /* 00E0 */ int maxPathLength; // TODO: Normalize to snake_case.
    /* 00E4 */ int absMaxPathLength; // TODO: Normalize to snake_case.
    /* 00E8 */ int64_t field_E8;
    /* 00F0 */ int64_t field_F0;
} AnimPath;

// Serializeable.
static_assert(sizeof(AnimPath) == 0xF8, "wrong size");

typedef struct AnimRunInfo {
    /* 0000 */ AnimID id;
    /* 000C */ unsigned int flags;
    /* 0010 */ int current_state;
    /* 0014 */ int path_attached_to_stack_index;
    /* 0018 */ DateTime next_ping_time;
    /* 0020 */ int64_t anim_obj;
    /* 0028 */ int64_t extra_target_tile;
    /* 0030 */ int current_goal;
    /* 0038 */ AnimGoalData goals[8];
    /* 0BF0 */ AnimGoalData* cur_stack_data;
    /* 0C00 */ AnimPath path;
    /* 0CF8 */ DateTime pause_time;
    /* 0D00 */ AnimRunInfoParam params[3];
    /* 0D18 */ int field_D18;
    /* 0D1C */ int field_D1C;
    /* 0D20 */ int field_D20;
    /* 0D24 */ int field_D24;
    /* 0D28 */ int field_D28;
    /* 0D2C */ int field_D2C;
    /* 0D30 */ int field_D30;
    /* 0D34 */ int field_D34;
} AnimRunInfo;

extern const char* off_5A164C[];
extern int dword_5A5978;
extern int dword_5A597C[AGDATA_COUNT];
extern bool dword_5E34F4;
extern void (*dword_5E34F8)(void);
extern bool in_anim_load;
extern int dword_5E3500;
extern int animNumActiveGoals;
extern int dword_6876E4;
extern int dword_739E40;
extern int dword_739E44;

extern AnimRunInfo anim_run_info[216];

void sub_44C840(AnimRunInfo* run_info, AnimGoalNode* goal_node);
bool sub_44C9A0(AnimRunInfo* run_info);
bool anim_private_init(GameInitInfo* init_info);
void anim_private_exit(void);
void anim_private_reset(void);
bool sub_44CB60(void);
bool anim_goal_restart(AnimID* anim_id);
bool mp_deallocate_run_index(AnimID* anim_id);
void sub_44C8F0(AnimRunInfo* run_info, AnimGoalNode* goal_node);
bool sub_44CCB0(AnimID* anim_id);
void sub_44D0C0(AnimRunInfo* run_info);
int anim_find_first(int64_t obj);
int anim_find_next(int prev, int64_t obj);
bool sub_44D4E0(AnimGoalData* goal_data, int64_t obj, int goal_type);
bool sub_44D500(AnimGoalData* goal_data, int64_t obj, int goal_type);
bool sub_44D520(AnimGoalData* goal_data, AnimID* anim_id);
bool sub_44D540(AnimGoalData* goal_data, AnimID* anim_id, unsigned int flags);
bool sub_44D730(AnimGoalData* goal_data, AnimID* anim_id, bool a3, unsigned int flags);
bool anim_subgoal_add_func(AnimID anim_id, AnimGoalData* goal_data);
bool sub_44DBE0(AnimID anim_id, AnimGoalData* goal_data, const char* file, int line);
bool anim_recover_handles(AnimRunInfo* run_info, AnimGoalSubNode* goal_subnode);
void sub_44E050(int64_t a1, int64_t a2);
void sub_44E0E0(int64_t a1, int64_t a2);
bool sub_44E160(AnimID* anim_id);
bool sub_44E2C0(AnimID* anim_id, int priority);
bool anim_find_first_of_type(int64_t obj, int type, AnimID* anim_id);
bool anim_find_next_of_type(int64_t obj, int type, AnimID* anim_id);
bool sub_44E6F0(int64_t obj, AnimGoalData* goal_data);
bool sub_44E4D0(int64_t a1, int a2, int a3);
bool sub_44E710(int64_t obj, AnimGoalData* goal_data, AnimID* anim_id);
bool anim_is_current_goal_type(int64_t obj, int goal_type, AnimID* anim_id);
bool sub_44E8C0(int64_t obj, AnimID* anim_id);
bool sub_44E940(int64_t a1, AnimID* anim_id, int64_t a2);
bool sub_44EB40(int64_t obj, int64_t from, int64_t to);
void sub_44EBD0(AnimPath* path);
void sub_44EBE0(AnimPath* path);
void sub_44EBF0(AnimRunInfo* run_info);
void anim_run_index_debug(int index);
void anim_stats(void);

#endif /* ARCANUM_GAME_ANIM_PRIVATE_H_ */
