#ifndef ARCANUM_GAME_SCRIPT_H_
#define ARCANUM_GAME_SCRIPT_H_

#include "game/context.h"
#include "game/timeevent.h"

typedef enum ScriptAttachmentPoint {
    SAP_EXAMINE,
    SAP_USE,
    SAP_DESTROY,
    SAP_UNLOCK,
    SAP_GET,
    SAP_DROP,
    SAP_THROW,
    SAP_HIT,
    SAP_MISS,
    SAP_DIALOG,
    SAP_FIRST_HEARTBEAT,
    SAP_CATCHING_THIEF_PC,
    SAP_DYING,
    SAP_ENTER_COMBAT,
    SAP_EXIT_COMBAT,
    SAP_START_COMBAT,
    SAP_END_COMBAT,
    SAP_BUY_OBJECT,
    SAP_RESURRECT,
    SAP_HEARTBEAT,
    SAP_LEADER_KILLING,
    SAP_INSERT_ITEM,
    SAP_WILL_KOS,
    SAP_TAKING_DAMAGE,
    SAP_WIELD_ON,
    SAP_WIELD_OFF,
    SAP_CRITTER_HITS,
    SAP_NEW_SECTOR,
    SAP_REMOVE_ITEM,
    SAP_LEADER_SLEEPING,
    SAP_BUST,
    SAP_DIALOG_OVERRIDE,
    SAP_TRANSFER,
    SAP_CAUGHT_THIEF,
    SAP_CRITICAL_HIT,
    SAP_CRITICAL_MISS,
} ScriptAttachmentPoint;

typedef enum ScriptFocusObject {
    SFO_TRIGGERER,
    SFO_ATTACHEE,
    SFO_EVERY_FOLLOWER,
    SFO_ANY_FOLLOWER,
    SFO_EVERYONE_IN_PARTY,
    SFO_ANYONE_IN_PARTY,
    SFO_EVERYONE_IN_TEAM,
    SFO_ANYONE_IN_TEAM,
    SFO_EVERYONE_IN_VICINITY,
    SFO_ANYONE_IN_VICINITY,
    SFO_CURRENT_LOOPED_OBJECT,
    SFO_LOCAL_OBJECT,
    SFO_EXTRA_OBJECT,
    SFO_EVERYONE_IN_GROUP,
    SFO_ANYONE_IN_GROUP,
    SFO_EVERY_SCENERY_IN_VICINITY,
    SFO_ANY_SCENERY_IN_VICINITY,
    SFO_EVERY_CONTAINER_IN_VICINITY,
    SFO_ANY_CONTAINER_IN_VICINITY,
    SFO_EVERY_PORTAL_IN_VICINITY,
    SFO_ANY_PORTAL_IN_VICINITY,
    SFO_PLAYER,
    SFO_EVERY_ITEM_IN_VICINITY,
    SFO_ANY_ITEM_IN_VICINITY,
} ScriptFocusObject;

typedef enum ScriptValueType {
    SVT_COUNTER,
    SVT_GL_VAR,
    SVT_LC_VAR,
    SVT_NUMBER,
    SVT_GL_FLAG,
    SVT_PC_VAR,
    SVT_PC_FLAG,
} ScriptValueType;

typedef unsigned int ScriptFlags;

#define SF_NONMAGICAL_TRAP 0x0001u
#define SF_MAGICAL_TRAP 0x0002u
#define SF_AUTO_REMOVING 0x0004u
#define SF_DEATH_SPEECH 0x0008u
#define SF_SURRENDER_SPEECH 0x0010u
#define SF_RADIUS_TWO 0x0020u
#define SF_RADIUS_THREE 0x0040u
#define SF_RADIUS_FIVE 0x0080u
#define SF_TELEPORT_TRIGGER 0x0100u

typedef enum ScriptConditionType {
    SCT_TRUE,
    SCT_DAYTIME,
    SCT_HAS_GOLD,
    SCT_LOCAL_FLAG,
    SCT_EQ,
    SCT_LE,
    SCT_PC_QUEST_STATE,
    SCT_GLOBAL_QUEST_STATE,
    SCT_OBJ_HAS_BLESS,
    SCT_OBJ_HAS_CURSE,
    SCT_OBJ_MET_PC_BEFORE,
    SCT_OBJ_HAS_BAD_ASSOCIATES,
    SCT_OBJ_IS_POLYMORPHED,
    SCT_OBJ_IS_SHRUNK,
    SCT_OBJ_HAS_BODY_SPELL,
    SCT_OBJ_IS_INVISIBLE,
    SCT_OBJ_HAS_MIRROR_IMAGE,
    SCT_OBJ_HAS_ITEM_NAMED,
    SCT_OBJ_FOLLOWING_PC,
    SCT_OBJ_IS_MONSTER_OF_TYPE,
    SCT_OBJ_IS_NAMED,
    SCT_OBJ_IS_WIELDING_ITEM,
    SCT_OBJ_IS_DEAD,
    SCT_OBJ_HAS_MAX_FOLLOWERS,
    SCT_OBJ_CAN_OPEN_CONTAINER,
    SCT_OBJ_HAS_SURRENDERED,
    SCT_OBJ_IS_IN_DIALOG,
    SCT_OBJ_IS_SWITCHED_OFF,
    SCT_OBJ_CAN_SEE_OBJ,
    SCT_OBJ_CAN_HEAR_OBJ,
    SCT_OBJ_IS_INVULNERABLE,
    SCT_OBJ_IS_IN_COMBAT,
    SCT_OBJ_IS_AT_LOCATION,
    SCT_OBJ_HAS_REPUTATION,
    SCT_OBJ_WITHIN_RANGE,
    SCT_OBJ_IS_INFLUENCED_BY_SPELL,
    SCT_OBJ_IS_OPEN,
    SCT_OBJ_IS_ANIMAL,
    SCT_OBJ_IS_UNDEAD,
    SCT_OBJ_JILTED,
    SCT_RUMOR_KNOWN,
    SCT_RUMOR_QUELLED,
    SCT_OBJ_IS_BUSTED,
    SCT_GLOBAL_FLAG,
    SCT_CAN_OPEN_PORTAL,
    SCT_SECTOR_IS_BLOCKED,
    SCT_MONSTERGEN_DISABLED,
    SCT_IDENTIFIED,
    SCT_KNOWS_SPELL,
    SCT_MASTERED_SPELL_COLLEGE,
    SCT_ITEMS_ARE_BEING_REWIELDED,
    SCT_PROWLING,
    SCT_WAITING_FOR_LEADER,
} ScriptConditionType;

typedef enum ScriptActionType {
    SAT_DO_NOTHING,
    SAT_RETURN_AND_SKIP_DEFAULT,
    SAT_RETURN_AND_RUN_DEFAULT,
    SAT_GOTO,
    SAT_DIALOG,
    SAT_REMOVE_THIS_SCRIPT,
    SAT_CHANGE_THIS_SCRIPT_TO_SCRIPT,
    SAT_CALL_SCRIPT,
    SAT_SET_LOCAL_FLAG,
    SAT_CLEAR_LOCAL_FLAG,
    SAT_ASSIGN_NUM,
    SAT_ADD,
    SAT_SUBTRACT,
    SAT_MULTIPLY,
    SAT_DIVIDE,
    SAT_ASSIGN_OBJ,
    SAT_SET_PC_QUEST_STATE,
    SAT_SET_QUEST_GLOBAL_STATE,
    SAT_LOOP_FOR,
    SAT_LOOP_END,
    SAT_LOOP_BREAK,
    SAT_CRITTER_FOLLOW,
    SAT_CRITTER_DISBAND,
    SAT_FLOAT_LINE,
    SAT_PRINT_LINE,
    SAT_ADD_BLESSING,
    SAT_REMOVE_BLESSING,
    SAT_ADD_CURSE,
    SAT_REMOVE_CURSE,
    SAT_GET_REACTION,
    SAT_SET_REACTION,
    SAT_ADJUST_REACTION,
    SAT_GET_ARMOR,
    SAT_GET_STAT,
    SAT_GET_OBJECT_TYPE,
    SAT_ADJUST_GOLD,
    SAT_ATTACK,
    SAT_RANDOM,
    SAT_GET_SOCIAL_CLASS,
    SAT_GET_ORIGIN,
    SAT_TRANSFORM_ATTACHEE_INTO_BASIC_PROTOTYPE,
    SAT_TRANSFER_ITEM,
    SAT_GET_STORY_STATE,
    SAT_SET_STORY_STATE,
    SAT_TELEPORT,
    SAT_SET_DAY_STANDPOINT,
    SAT_SET_NIGHT_STANDPOINT,
    SAT_GET_SKILL,
    SAT_CAST_SPELL,
    SAT_MARK_MAP_LOCATION,
    SAT_SET_RUMOR,
    SAT_QUELL_RUMOR,
    SAT_CREATE_OBJECT,
    SAT_SET_LOCK_STATE,
    SAT_CALL_SCRIPT_IN,
    SAT_CALL_SCRIPT_AT,
    SAT_TOGGLE_STATE,
    SAT_TOGGLE_INVULNERABILITY,
    SAT_KILL,
    SAT_CHANGE_ART_NUM,
    SAT_DAMAGE,
    SAT_CAST_SPELL_ON,
    SAT_ACTION_PERFORM_ANIMATION,
    SAT_GIVE_QUEST_XP,
    SAT_WRITTEN_UI_START_BOOK,
    SAT_WRITTEN_UI_START_IMAGE,
    SAT_CREATE_ITEM,
    SAT_ACTION_WAIT_FOR_LEADER,
    SAT_DESTROY,
    SAT_ACTION_WALK_TO,
    SAT_GET_WEAPON_TYPE,
    SAT_DISTANCE_BETWEEN,
    SAT_ADD_REPUTATION,
    SAT_REMOVE_REPUTATION,
    SAT_ACTION_RUN_TO,
    SAT_HEAL_HP,
    SAT_HEAL_FATIGUE,
    SAT_ADD_EFFECT,
    SAT_REMOVE_EFFECT,
    SAT_ACTION_USE_ITEM,
    SAT_GET_MAGICTECH_ADJUSTMENT,
    SAT_CALL_SCRIPT_EX,
    SAT_PLAY_SOUND,
    SAT_PLAY_SOUND_ON,
    SAT_GET_AREA,
    SAT_QUEUE_NEWSPAPER,
    SAT_FLOAT_NEWSPAPER_HEADLINE,
    SAT_PLAY_SOUND_SCHEME,
    SAT_TOGGLE_OPEN_CLOSED,
    SAT_GET_FACTION,
    SAT_GET_SCROLL_DISTANCE,
    SAT_GET_MAGICTECH_ADJUSTMENT_EX,
    SAT_RENAME,
    SAT_ACTION_BECOME_PRONE,
    SAT_SET_WRITTEN_START,
    SAT_GET_LOCATION,
    SAT_GET_DAY_SINCE_STARTUP,
    SAT_GET_CURRENT_HOUR,
    SAT_GET_CURRENT_MINUTE,
    SAT_CHANGE_SCRIPT,
    SAT_SET_GLOBAL_FLAG,
    SAT_CLEAR_GLOBAL_FLAG,
    SAT_FADE_AND_TELEPORT,
    SAT_FADE,
    SAT_PLAY_SPELL_EYE_CANDY,
    SAT_GET_HOURS_SINCE_STARTUP,
    SAT_TOGGLE_SECTOR_BLOCKED,
    SAT_GET_HIT_POINTS,
    SAT_GET_FATIGUE_POINTS,
    SAT_ACTION_STOP_ATTACKING,
    SAT_TOGGLE_MONSTER_GENERATOR,
    SAT_GET_ARMOR_COVERAGE,
    SAT_GIVE_SPELL_MASTERY_IN_COLLEGE,
    SAT_UNFOG_TOWNMAP,
    SAT_START_WRITTEN_UI,
    SAT_ACTION_TRY_TO_STEAL_100_COINS,
    SAT_STOP_SPELL_EYE_CANDY,
    SAT_GRANT_ONE_FATE_POINT,
    SAT_CAST_FREE_SPELL,
    SAT_SET_PC_QUEST_UNBOTCHED,
    SAT_PLAY_SCRIPT_EYE_CANDY,
    SAT_ACTION_CAST_UNRESISTABLE_SPELL,
    SAT_ACTION_CAST_FREE_UNRESISTABLE_SPELL,
    SAT_TOUCH_ART,
    SAT_STOP_SCRIPT_EYE_CANDY,
    SAT_REMOVE_SCRIPT_CALL,
    SAT_DESTROY_ITEM_NAMED,
    SAT_TOGGLE_ITEM_INVENTORY_DISPLAY,
    SAT_HEAL_POISON,
    SAT_START_SCHEMATIC_UI,
    SAT_STOP_SPELL,
    SAT_QUEUE_SLIDE,
    SAT_END_GAME_AND_PLAY_SLIDES,
    SAT_SET_ROTATION,
    SAT_SET_FACTION,
    SAT_DRAIN_CHARGES,
    SAT_CAST_UNRESISTABLE_SPELL,
    SAT_ADJUST_STAT,
    SAT_APPLY_UNRESISTABLE_DAMAGE,
    SAT_SET_AUTOLEVEL_SCHEME,
    SAT_SET_DAY_STANDPOINT_EX,
    SAT_SET_NIGHT_STANDPOINT_EX,
} ScriptActionType;

typedef void(ScriptStartDialogFunc)(int64_t pc_obj, int64_t npc_obj, int script_num, int script_line, int num);
typedef void(ScriptFloatLineFunc)(int64_t npc_obj, int64_t pc_obj, const char* str, int speech_id);

typedef struct ScriptAction {
    int type;
    uint8_t op_type[8];
    int op_value[8];
} ScriptAction;

// Serializeable.
static_assert(sizeof(ScriptAction) == 0x2C, "wrong size");

typedef struct ScriptCondition {
    int type;
    uint8_t op_type[8];
    int op_value[8];
    ScriptAction action;
    ScriptAction els;
} ScriptCondition;

// Serializeable.
static_assert(sizeof(ScriptCondition) == 0x84, "wrong size");

typedef struct ScriptFile {
    /* 0000 */ char description[40];
    /* 0028 */ ScriptFlags flags;
    /* 002C */ int num_entries;
    /* 0030 */ int max_entries;
    /* 0034 */ ScriptCondition* entries;
} ScriptFile;

typedef struct ScriptHeader {
    unsigned int flags;
    // TODO: Refactor to uint8_t[4].
    unsigned int counters;
} ScriptHeader;

// Serializeable.
static_assert(sizeof(ScriptHeader) == 0x8, "wrong size");

typedef struct Script {
    ScriptHeader hdr;
    int num;
} Script;

// Serializeable.
static_assert(sizeof(Script) == 0xC, "wrong size");

typedef struct ScriptInvocation {
    /* 0000 */ Script* script;
    /* 0004 */ int line;
    /* 0008 */ int64_t triggerer_obj;
    /* 0010 */ int64_t attachee_obj;
    /* 0018 */ int64_t extra_obj;
    /* 0020 */ int attachment_point;
} ScriptInvocation;

extern int dword_5A5700[3];

bool script_init(GameInitInfo* init_info);
void script_reset();
void script_exit();
bool script_mod_load();
void script_mod_unload();
bool script_load(GameLoadInfo* load_info);
bool script_save(TigFile* stream);
void script_set_callbacks(ScriptStartDialogFunc* start_dialog_func, ScriptFloatLineFunc* float_line_func);
bool script_execute(ScriptInvocation* invocation);
int script_global_var_get(int index);
void script_global_var_set(int index, int value);
int script_global_flag_get(int index);
void script_global_flag_set(int index, int value);
int script_pc_var_get(int64_t obj, int index);
void script_pc_var_set(int64_t obj, int index, int value);
int script_pc_flag_get(int64_t obj, int index);
void script_pc_flag_set(int64_t obj, int index, int value);
bool script_local_flag_get(int64_t obj, int index, int flag);
void script_local_flag_set(int64_t obj, int index, int flag, bool enabled);
int script_local_counter_get(int64_t obj, int index, int counter);
void script_local_counter_set(int64_t obj, int index, int counter, int value);
int script_story_state_get();
void script_story_state_set(int value);
char* script_story_state_info(int story_state_num);
bool script_timeevent_process(TimeEvent* timeevent);
bool script_load_hdr(Script* scr);
bool script_flags(Script* scr, ScriptFlags* flags_ptr);
void script_play_explosion_fx(int64_t obj);

static inline bool sfo_is_any(uint8_t type)
{
    return type == SFO_ANY_FOLLOWER
        || type == SFO_ANYONE_IN_PARTY
        || type == SFO_ANYONE_IN_TEAM
        || type == SFO_ANYONE_IN_GROUP
        || type == SFO_ANYONE_IN_VICINITY
        || type == SFO_ANY_SCENERY_IN_VICINITY
        || type == SFO_ANY_CONTAINER_IN_VICINITY
        || type == SFO_ANY_PORTAL_IN_VICINITY
        || type == SFO_ANY_ITEM_IN_VICINITY;
}

#endif /* ARCANUM_GAME_SCRIPT_H_ */
