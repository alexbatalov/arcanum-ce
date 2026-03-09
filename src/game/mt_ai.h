#ifndef ARCANUM_GAME_MT_AI_H_
#define ARCANUM_GAME_MT_AI_H_

#include "game/context.h"

typedef enum MagicTechAiAction {
    MAGICTECH_AI_ACTION_FLEE,
    MAGICTECH_AI_ACTION_SUMMON,
    MAGICTECH_AI_ACTION_DEFENSIVE,
    MAGICTECH_AI_ACTION_OFFENSIVE,
    MAGICTECH_AI_ACTION_HEAL_LIGHT,
    MAGICTECH_AI_ACTION_HEAL_MEDIUM,
    MAGICTECH_AI_ACTION_HEAL_HEAVY,
    MAGICTECH_AI_ACTION_CURE_POISON,
    MAGICTECH_AI_ACTION_FATIGUE_RECOVER,
    MAGICTECH_AI_ACTION_RESURRECT,
    MAGICTECH_AI_ACTION_COUNT,
} MagicTechAiAction;

typedef struct MagicTechAiActionListEntry {
    /* 0000 */ int spell;
    /* 0008 */ int64_t item_obj;
} MagicTechAiActionListEntry;

typedef struct MagicTechAiActionList {
    /* 0000 */ int cnt;
    /* 0004 */ MagicTechAiActionListEntry* entries;
} MagicTechAiActionList;

typedef struct MagicTechAi {
    /* 0000 */ int64_t obj;
    /* 0008 */ int64_t leader_obj;
    /* 0010 */ int action;
    /* 0018 */ int64_t field_18;
    /* 0020 */ int field_20;
    /* 0024 */ int field_24;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
    /* 0030 */ MagicTechAiActionList actions[MAGICTECH_AI_ACTION_COUNT];
} MagicTechAi;

bool mt_ai_init(GameInitInfo* init_info);
void mt_ai_reset(void);
void mt_ai_exit(void);
int sub_4CC2A0(int spl);
void mt_ai_create(MagicTechAi* magictech_ai, int64_t obj, int action);
void mt_ai_destroy(MagicTechAi* magictech_ai);
void mt_ai_notify_inventory_changed(int64_t obj);
void mt_ai_notify_item_exhausted(int64_t obj, int64_t item_obj);

#endif /* ARCANUM_GAME_MT_AI_H_ */
