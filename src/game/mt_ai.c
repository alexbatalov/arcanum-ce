#include "game/mt_ai.h"

#include "game/ai.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/item.h"
#include "game/magictech.h"
#include "game/spell.h"
#include "game/stat.h"

#define MAX_MT_AI 40
#define MAX_MT_ENTRIES 200

static bool sub_4CC240(void);
static void sub_4CC270(void);
static int sub_4CC2D0(const void* va, const void* vb);
static int sub_4CC310(int spl);
static bool sub_4CC350(int index, MagicTechAiActionList* list);
static bool sub_4CC380(MagicTechAiActionList* list, const char* name);
static void sub_4CC470(void);
static void mt_ai_clear(void);
static void mt_ai_create_internal(MagicTechAi* magictech_ai, int64_t obj, int action);
static void sub_4CC520(int slot);
static void mt_ai_populate(MagicTechAi* magictech_ai);
static bool sub_4CC6F0(MagicTechAi* magictech_ai);
static void copy_actions(MagicTechAi* src, MagicTechAi* dst);
static bool mt_ai_alloc(MagicTechAi** magictech_ai_ptr);
static void sub_4CC810(MagicTechAi* magictech_ai);
static void magictech_build_ai_action_list(MagicTechAi* magictech_ai);
static void sub_4CC930(MagicTechAi* magictech_ai, int64_t item_obj);
static bool sub_4CCA30(int64_t item_obj, MagicTechAi* magictech_ai);
static void sub_4CCAD0(int spell);
static void sub_4CCB10(int spell, int64_t item_obj);
static void sub_4CCB50(int64_t item_obj);
static void flush_actions(MagicTechAi* magictech_ai, int action);

// 0x5B7558
static int dword_5B7558 = -1;

// 0x5B755C
static const char* off_5B755C[MAGICTECH_AI_ACTION_COUNT] = {
    /*            MAGICTECH_AI_ACTION_FLEE */ "AIFlee",
    /*          MAGICTECH_AI_ACTION_SUMMON */ "AISummon",
    /*       MAGICTECH_AI_ACTION_DEFENSIVE */ "AIDefensive",
    /*       MAGICTECH_AI_ACTION_OFFENSIVE */ "AIOffensive",
    /*      MAGICTECH_AI_ACTION_HEAL_LIGHT */ "AIHealLight",
    /*     MAGICTECH_AI_ACTION_HEAL_MEDIUM */ "AIHealMedium",
    /*      MAGICTECH_AI_ACTION_HEAL_HEAVY */ "AIHealHeavy",
    /*     MAGICTECH_AI_ACTION_CURE_POISON */ "AICurePoison",
    /* MAGICTECH_AI_ACTION_FATIGUE_RECOVER */ "AIFatigueRecover",
    /*       MAGICTECH_AI_ACTION_RESURRECT */ "AIResurrect",
};

// 0x5FF620
static MagicTechAi mt_ai_cache[MAX_MT_AI];

// 0x600A20
static MagicTechAiActionList mt_ai_tmp_lists[MAGICTECH_AI_ACTION_COUNT];

// 0x600A70
static MagicTechAiActionListEntry mt_ai_tmp_entries[MAX_MT_ENTRIES];

// 0x6016F0
static bool mt_ai_initialized;

// 06016F4
static int mt_ai_entries_cnt;

// 0x6016F8
static int mt_ai_active_cnt;

// 0x6016FC
static int mt_id_next_idx;

// 0x4CC1F0
bool mt_ai_init(GameInitInfo* init_info)
{
    (void)init_info;

    sub_4CC470();
    mt_ai_initialized = true;

    if (!sub_4CC240()) {
        return false;
    }

    return true;
}

// 0x4CC210
void mt_ai_reset(void)
{
    mt_ai_clear();
}

// 0x4CC220
void mt_ai_exit(void)
{
    if (mt_ai_initialized) {
        sub_4CC270();
        mt_ai_clear();
        mt_ai_initialized = false;
    }
}

// 0x4CC240
bool sub_4CC240(void)
{
    int action;

    for (action = 0; action < MAGICTECH_AI_ACTION_COUNT; action++) {
        if (!sub_4CC350(action, &(mt_ai_tmp_lists[action]))) {
            return false;
        }
    }

    return true;
}

// 0x4CC270
void sub_4CC270(void)
{
    int action;

    for (action = 0; action < MAGICTECH_AI_ACTION_COUNT; action++) {
        if (mt_ai_tmp_lists[action].entries != NULL) {
            FREE(mt_ai_tmp_lists[action].entries);
            mt_ai_tmp_lists[action].entries = NULL;
        }
    }
}

// 0x4CC2A0
int sub_4CC2A0(int spl)
{
    if (mt_ai_initialized) {
        return magictech_spells[spl].defensive2;
    } else {
        return 0;
    }
}

// 0x4CC2D0
int sub_4CC2D0(const void* va, const void* vb)
{
    const MagicTechAiActionListEntry* a = (const MagicTechAiActionListEntry*)va;
    const MagicTechAiActionListEntry* b = (const MagicTechAiActionListEntry*)vb;
    int v1;
    int v2;

    v1 = sub_4CC310(a->spell);
    v2 = sub_4CC310(b->spell);

    if (v1 < v2) {
        return 1;
    } else if (v1 > v2) {
        return -1;
    } else {
        return 0;
    }
}

// 0x4CC310
int sub_4CC310(int spl)
{
    return dword_5B7558 >= 0 && dword_5B7558 < MAGICTECH_AI_ACTION_COUNT && mt_ai_initialized
        ? magictech_spells[spl].ai.values[dword_5B7558]
        : -1;
}

// 0x4CC350
bool sub_4CC350(int action, MagicTechAiActionList* list)
{
    dword_5B7558 = action;
    if (action >= 0 && action < MAGICTECH_AI_ACTION_COUNT) {
        return sub_4CC380(list, off_5B755C[action]);
    } else {
        return false;
    }
}

// 0x4CC380
bool sub_4CC380(MagicTechAiActionList* list, const char* name)
{
    int cnt;
    int spl;

    (void)name;

    cnt = 0;
    for (spl = 0; spl < MT_SPELL_COUNT; spl++) {
        if (sub_4CC310(spl) != -1) {
            cnt++;
        }
    }

    list->cnt = cnt;

    if (cnt != 0) {
        if (list->entries != NULL) {
            FREE(list->entries);
        }

        list->entries = (MagicTechAiActionListEntry*)MALLOC(sizeof(*list->entries) * cnt);

        cnt = 0;
        for (spl = 0; spl < MT_SPELL_COUNT; spl++) {
            if (sub_4CC310(spl) != -1) {
                list->entries[cnt].spell = spl;
                list->entries[cnt].item_obj = OBJ_HANDLE_NULL;
                cnt++;
            }
        }

        qsort(list->entries, list->cnt, sizeof(*list->entries), sub_4CC2D0);
    } else {
        list->entries = NULL;
    }

    return true;
}

// 0x4CC470
void sub_4CC470(void)
{
    int slot;
    int action;
    MagicTechAi* magictech_ai;

    for (slot = 0; slot < MAX_MT_AI; slot++) {
        magictech_ai = &(mt_ai_cache[slot]);
        magictech_ai->obj = OBJ_HANDLE_NULL;
        magictech_ai->field_28 = 0;
        magictech_ai->leader_obj = OBJ_HANDLE_NULL;
        magictech_ai->field_18 = OBJ_HANDLE_NULL;
        magictech_ai->field_2C = 0;

        for (action = 0; action < MAGICTECH_AI_ACTION_COUNT; action++) {
            magictech_ai->actions[action].entries = NULL;
            magictech_ai->actions[action].cnt = 0;
        }
    }

    mt_ai_active_cnt = 0;
    mt_id_next_idx = 0;
}

// 0x4CC4C0
void mt_ai_clear(void)
{
    int slot;

    for (slot = 0; slot < MAX_MT_AI; slot++) {
        sub_4CC520(slot);
    }

    mt_ai_active_cnt = 0;
    mt_id_next_idx = 0;
}

// 0x4CC4F0
void mt_ai_create_internal(MagicTechAi* magictech_ai, int64_t obj, int action)
{
    magictech_ai->obj = obj;
    magictech_ai->action = action;
    magictech_ai->field_18 = OBJ_HANDLE_NULL;
    magictech_ai->field_28 = 0;
    magictech_ai->field_20 = -1;
    magictech_ai->field_2C = 0;
}

// 0x4CC520
void sub_4CC520(int slot)
{
    MagicTechAi* magictech_ai;
    int action;

    magictech_ai = &(mt_ai_cache[slot]);
    magictech_ai->obj = OBJ_HANDLE_NULL;
    magictech_ai->leader_obj = OBJ_HANDLE_NULL;
    magictech_ai->field_28 = 0;
    magictech_ai->field_18 = OBJ_HANDLE_NULL;
    magictech_ai->field_2C = 0;

    for (action = 0; action < MAGICTECH_AI_ACTION_COUNT; action++) {
        if (magictech_ai->actions[action].entries != NULL) {
            FREE(magictech_ai->actions[action].entries);
            magictech_ai->actions[action].entries = NULL;
        }
        magictech_ai->actions[action].cnt = 0;
    }

    mt_ai_active_cnt--;
}

// 0x4CC580
void mt_ai_populate(MagicTechAi* magictech_ai)
{
    MagicTechAi* tmp;

    if (magictech_ai->obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (sub_4CC6F0(magictech_ai)) {
        tmp = magictech_ai;
        if (magictech_ai->actions[magictech_ai->action].cnt > 0) {
            return;
        }
    } else {
        if (!mt_ai_alloc(&tmp)) {
            return;
        }

        tmp->obj = magictech_ai->obj;
        tmp->action = magictech_ai->action;
    }

    tmp->leader_obj = critter_pc_leader_get(magictech_ai->obj);
    tmp->field_18 = magictech_ai->field_18;

    mt_ai_entries_cnt = 0;
    dword_5B7558 = tmp->action;

    if (combat_critter_is_combat_mode_active(tmp->obj)
        && ai_critter_fatigue_ratio(tmp->obj) <= 50) {
        sub_4CC810(tmp);
        if (mt_ai_entries_cnt == 0) {
            magictech_build_ai_action_list(tmp);
        }
    } else {
        magictech_build_ai_action_list(tmp);
        if (mt_ai_entries_cnt == 0) {
            sub_4CC810(tmp);
        }
    }

    if (mt_ai_entries_cnt > 0) {
        flush_actions(tmp, dword_5B7558);
        tmp->field_20 = mt_ai_tmp_entries[0].spell;
        tmp->field_28 = mt_ai_tmp_entries[0].spell;
    }

    copy_actions(tmp, magictech_ai);
}

// 0x4CC6F0
bool sub_4CC6F0(MagicTechAi* magictech_ai)
{
    int slot = 0;

    for (slot = 0; slot < MAX_MT_AI; slot++) {
        if (mt_ai_cache[slot].obj == magictech_ai->obj) {
            copy_actions(&(mt_ai_cache[slot]), magictech_ai);
            return true;
        }
    }

    return false;
}

// 0x4CC740
void copy_actions(MagicTechAi* src, MagicTechAi* dst)
{
    int action;

    for (action = 0; action < MAGICTECH_AI_ACTION_COUNT; action++) {
        dst->actions[action] = src->actions[action];
    }
}

// 0x4CC770
bool mt_ai_alloc(MagicTechAi** magictech_ai_ptr)
{
    int index;

    for (index = 0; index < MAX_MT_AI; index++) {
        if (mt_ai_cache[index].obj == OBJ_HANDLE_NULL) {
            break;
        }
    }

    if (index < MAX_MT_AI) {
        *magictech_ai_ptr = &(mt_ai_cache[index]);
        mt_ai_active_cnt++;
        return true;
    }

    *magictech_ai_ptr = &(mt_ai_cache[mt_id_next_idx]);
    sub_4CC520(mt_id_next_idx);

    if (++mt_id_next_idx >= MAX_MT_AI) {
        mt_id_next_idx = 0;
    }

    mt_ai_active_cnt++;

    return true;
}

// 0x4CC810
void sub_4CC810(MagicTechAi* magictech_ai)
{
    int64_t item_obj;
    int cnt;
    int index;

    item_obj = item_wield_get(magictech_ai->obj, ITEM_INV_LOC_WEAPON);
    if (item_obj != OBJ_HANDLE_NULL
        && (obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_HEXED) != 0) {
        sub_4CC930(magictech_ai, item_obj);
        return;
    }

    cnt = obj_field_int32_get(magictech_ai->obj, OBJ_F_CRITTER_INVENTORY_NUM);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(magictech_ai->obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, index);
        sub_4CC930(magictech_ai, item_obj);
    }
}

// 0x4CC8A0
void magictech_build_ai_action_list(MagicTechAi* magictech_ai)
{
    MagicTechAiActionList* list;
    int index;

    if (magictech_ai->action < 0
        || magictech_ai->action >= MAGICTECH_AI_ACTION_COUNT) {
        tig_debug_printf("MagicTech: magictech_build_ai_action_list: ERROR: AI action type out of range!\n");
        return;
    }

    list = &(mt_ai_tmp_lists[magictech_ai->action]);
    if (list == NULL) {
        // FIXME: Meaningless?
        tig_debug_printf("MagicTech: magictech_build_ai_action_list: ERROR: AI action type out of range!\n");
        return;
    }

    if (stat_level_get(magictech_ai->obj, STAT_MAGICK_POINTS) > 0) {
        for (index = 0; index < list->cnt; index++) {
            if (spell_is_known(magictech_ai->obj, list->entries[index].spell)) {
                sub_4CCAD0(list->entries[index].spell);
            }
        }
    }
}

// 0x4CC930
void sub_4CC930(MagicTechAi* magictech_ai, int64_t item_obj)
{
    int index;
    int spell;

    // FIXME: Unused.
    obj_field_int32_get(item_obj, OBJ_F_TYPE);

    if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_MANA_STORE)) != 0
        && sub_4CCA30(item_obj, magictech_ai)) {
        for (index = 0; index < 5; index++) {
            spell = obj_field_int32_get(item_obj, 100 + index);
            if (spell != 10000
                && magictech_spells[spell].ai.values[dword_5B7558] != -1
                && sub_456A10(magictech_ai->obj, magictech_ai->field_18, item_obj)) {
                sub_4CCB10(spell, item_obj);
            }
        }
    }

    if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_VALID_AI_ACTION) != 0
        && obj_field_int32_get(item_obj, OBJ_F_ITEM_AI_ACTION) == magictech_ai->action) {
        sub_4CCB50(item_obj);
    }
}

// 0x4CCA30
bool sub_4CCA30(int64_t item_obj, MagicTechAi* magictech_ai)
{
    int type;

    if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_IDENTIFIED) == 0
        && magictech_ai->leader_obj != OBJ_HANDLE_NULL) {
        type = tig_art_item_id_type_get(obj_field_int32_get(item_obj, OBJ_F_CURRENT_AID));
        if (type != TIG_ART_ITEM_TYPE_FOOD && type != TIG_ART_ITEM_TYPE_SCROLL) {
            return false;
        }
    }

    return true;
}

// 0x4CCA90
void mt_ai_create(MagicTechAi* magictech_ai, int64_t obj, int action)
{
    if (obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        mt_ai_create_internal(magictech_ai, obj, action);
        mt_ai_populate(magictech_ai);
    }
}

// 0x4CCAD0
void sub_4CCAD0(int spell)
{
    if (mt_ai_entries_cnt < MAX_MT_ENTRIES) {
        mt_ai_tmp_entries[mt_ai_entries_cnt].spell = spell;
        mt_ai_tmp_entries[mt_ai_entries_cnt].item_obj = OBJ_HANDLE_NULL;
        mt_ai_entries_cnt++;
    }
}

// 0x4CCB10
void sub_4CCB10(int spell, int64_t item_obj)
{
    if (mt_ai_entries_cnt < MAX_MT_ENTRIES) {
        mt_ai_tmp_entries[mt_ai_entries_cnt].spell = spell;
        mt_ai_tmp_entries[mt_ai_entries_cnt].item_obj = item_obj;
        mt_ai_entries_cnt++;
    }
}

// 0x4CCB50
void sub_4CCB50(int64_t item_obj)
{
    if (mt_ai_entries_cnt < MAX_MT_ENTRIES) {
        mt_ai_tmp_entries[mt_ai_entries_cnt].spell = -1;
        mt_ai_tmp_entries[mt_ai_entries_cnt].item_obj = item_obj;
        mt_ai_entries_cnt++;
    }
}

// 0x4CCB90
void flush_actions(MagicTechAi* magictech_ai, int action)
{
    magictech_ai->actions[action].cnt = mt_ai_entries_cnt;
    if (magictech_ai->actions[action].entries != NULL) {
        FREE(magictech_ai->actions[action].entries);
    }

    magictech_ai->actions[action].entries = (MagicTechAiActionListEntry*)MALLOC(sizeof(MagicTechAiActionListEntry) * mt_ai_entries_cnt);
    memcpy(magictech_ai->actions[action].entries, mt_ai_tmp_entries, sizeof(MagicTechAiActionListEntry) * mt_ai_entries_cnt);
}

// 0x4CCBF0
void mt_ai_destroy(MagicTechAi* magictech_ai)
{
    (void)magictech_ai;
}

// 0x4CCC00
void mt_ai_notify_inventory_changed(int64_t obj)
{
    int slot;

    for (slot = 0; slot < MAX_MT_AI; slot++) {
        if (mt_ai_cache[slot].obj == obj) {
            sub_4CC520(slot);
        }
    }
}

// 0x4CCC40
void mt_ai_notify_item_exhausted(int64_t obj, int64_t item_obj)
{
    int slot;
    int action;
    int idx;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    for (slot = 0; slot < MAX_MT_AI; slot++) {
        if (mt_ai_cache[slot].obj == obj) {
            break;
        }
    }

    if (slot >= MAX_MT_AI) {
        return;
    }

    for (action = 0; action < MAGICTECH_AI_ACTION_COUNT; action++) {
        for (idx = 0; idx < mt_ai_cache[slot].actions[action].cnt; idx++) {
            if (mt_ai_cache[slot].actions[action].entries[idx].item_obj == item_obj) {
                sub_4CC520(slot);
                return;
            }
        }
    }
}
