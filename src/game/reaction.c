#include "game/reaction.h"

#include "game/ai.h"
#include "game/critter.h"
#include "game/effect.h"
#include "game/fate.h"
#include "game/magictech.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/obj.h"
#include "game/reputation.h"
#include "game/stat.h"

static int sub_4C0D00(int64_t a1, int64_t a2, unsigned int flags);
static int sub_4C1290(int64_t a1, int64_t a2);
static int reaction_get_base(int64_t obj);
static bool sub_4C12F0(int64_t a1, int64_t a2, bool a3, int* a4);
static void sub_4C1360(int64_t npc_obj, int64_t pc_obj, int value);
static void sub_4C1490(int64_t npc_obj, int64_t pc_obj, int level, int index);
static int sub_4C1500(int64_t npc_obj, int64_t pc_obj, unsigned int flags);
static int sub_4C15A0(int a1);

// 0x5FC88C
static mes_file_handle_t reaction_mes_file;

// 0x5FC890
static char* reaction_names[REACTION_COUNT];

// 0x5B684C
static int dword_5B684C[RACE_COUNT][8] = {
    {
        0,
        0,
        5,
        5,
        0,
        10,
        -5,
        -5,
    },
    {
        -5,
        10,
        -10,
        -5,
        0,
        5,
        -10,
        -15,
    },
    {
        -10,
        -20,
        0,
        0,
        -10,
        10,
        -15,
        -10,
    },
    {
        0,
        0,
        0,
        0,
        0,
        5,
        -5,
        -5,
    },
    {
        0,
        0,
        0,
        0,
        5,
        5,
        -10,
        5,
    },
    {
        0,
        0,
        20,
        15,
        0,
        10,
        -10,
        -10,
    },
    {
        0,
        -20,
        -15,
        -10,
        -10,
        0,
        10,
        0,
    },
    {
        0,
        -20,
        -15,
        -5,
        20,
        10,
        0,
        5,
    },
    {
        -30,
        -30,
        5,
        -20,
        -30,
        -5,
        -30,
        -30,
    },
    {
        -10,
        -30,
        -20,
        -15,
        -20,
        -20,
        0,
        -10,
    },
    {
        -20,
        -30,
        -40,
        -30,
        -20,
        -20,
        10,
        0,
    },
};

// 0x4C0BD0
bool reaction_init(GameInitInfo* init_info)
{
    MesFileEntry mes_file_entry;
    int index;

    (void)init_info;

    if (!mes_load("mes\\reaction.mes", &reaction_mes_file)) {
        return false;
    }

    for (index = 0; index < REACTION_COUNT; index++) {
        mes_file_entry.num = index;
        mes_get_msg(reaction_mes_file, &mes_file_entry);
        reaction_names[index] = mes_file_entry.str;
    }

    return true;
}

// 0x4C0C30
void reaction_exit(void)
{
    mes_unload(reaction_mes_file);
}

// 0x4C0C40
bool reaction_met_before(int64_t npc_obj, int64_t pc_obj)
{
    int v1;

    if (npc_obj == pc_obj) {
        return true;
    }

    // Make sure `pc_obj` is actually PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return false;
    }

    // Make sure `npc_obj` is actually NPC.
    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return false;
    }

    return sub_4C12F0(npc_obj, pc_obj, false, &v1);
}

// 0x4C0CC0
int reaction_get(int64_t npc_obj, int64_t pc_obj)
{
    return sub_4C0D00(npc_obj, pc_obj, 0);
}

// 0x4C0CE0
int sub_4C0CE0(int64_t npc_obj, int64_t pc_obj)
{
    return sub_4C0D00(npc_obj, pc_obj, 1);
}

// 0x4C0D00
int sub_4C0D00(int64_t npc_obj, int64_t pc_obj, unsigned int flags)
{
    int value;
    int64_t mind_controlled_by_obj;

    if (ai_shitlist_has(npc_obj, pc_obj)) {
        return 0;
    }

    // Make sure `pc_obj` is actually PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return 50;
    }

    // Make sure `npc_obj` is actually NPC.
    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return 50;
    }

    value = sub_4C1500(npc_obj, pc_obj, flags) + sub_4C1290(npc_obj, pc_obj);
    value = effect_adjust_reaction(npc_obj, value);

    if (value < 50
        && sub_459040(npc_obj, OSF_MIND_CONTROLLED, &mind_controlled_by_obj)
        && mind_controlled_by_obj == pc_obj) {
        value = 50;
    }

    return value;
}

// 0x4C0DE0
void reaction_adj(int64_t npc_obj, int64_t pc_obj, int value)
{
    int64_t mind_controlled_by_obj;
    int base;
    int adjusted_value;
    unsigned int flags;

    if (!multiplayer_is_locked()) {
        if (!tig_net_is_host()) {
            return;
        }

        mp_reaction_adj(npc_obj, pc_obj, value);
    }

    if (value == 0) {
        return;
    }

    // Make sure `pc_obj` is actually PC.
    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    // Make sure `npc_obj` is actually NPC.
    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return;
    }

    if (sub_459040(npc_obj, OSF_MIND_CONTROLLED, &mind_controlled_by_obj)
        && mind_controlled_by_obj == pc_obj) {
        return;
    }

    if (value < 0
        && stat_is_extraordinary(pc_obj, STAT_CHARISMA)
        && critter_pc_leader_get(npc_obj) == pc_obj) {
        return;
    }

    base = sub_4C1290(npc_obj, pc_obj);
    adjusted_value = effect_adjust_good_bad_reaction(pc_obj, value);
    sub_4C1360(npc_obj, pc_obj, base + adjusted_value);

    if (value < 0 && critter_leader_get(npc_obj) == pc_obj) {
        flags = obj_field_int32_get(npc_obj, OBJ_F_NPC_FLAGS);
        flags |= ONF_CHECK_LEADER;
        obj_field_int32_set(npc_obj, OBJ_F_NPC_FLAGS, flags);

        flags = obj_field_int32_get(npc_obj, OBJ_F_CRITTER_FLAGS2);
        flags |= OCF2_CHECK_REACTION_BAD;
        obj_field_int32_set(npc_obj, OBJ_F_CRITTER_FLAGS2, flags);
    }
}

// 0x4C0F50
void sub_4C0F50(int64_t a1, int64_t a2)
{
    int index;

    for (index = 0; index < 10; index++) {
        if (index != 0
            && obj_arrayfield_handle_get(a1, OBJ_F_NPC_REACTION_PC_IDX, index) == a2) {
            obj_arrayfield_obj_set(a1, OBJ_F_NPC_REACTION_PC_IDX, index, OBJ_HANDLE_NULL);
            break;
        }
    }

    combat_recalc_reaction(a1);
}

// 0x4C0FC0
int reaction_translate(int value)
{
    if (value <= 0) return REACTION_HATRED;
    if (value <= 20) return REACTION_DISLIKE;
    if (value <= 40) return REACTION_SUSPICIOUS;
    if (value <= 60) return REACTION_NEUTRAL;
    if (value <= 80) return REACTION_COURTEOUS;
    if (value <= 100) return REACTION_AMIABLE;
    return REACTION_LOVE;
}

// 0x4C1010
const char* reaction_get_name(int reaction)
{
    return reaction_names[reaction];
}

// 0x4C1020
void sub_4C1020(int64_t npc_obj, int64_t pc_obj)
{
    int v1;

    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return;
    }

    v1 = sub_4C1290(npc_obj, pc_obj);
    if (v1 < 100 && fate_resolve(pc_obj, FATE_FORCE_GOOD_REACTION)) {
        v1 = 100;
    }

    sub_4C1490(npc_obj, pc_obj, v1, 0);
}

// 0x4C10A0
void sub_4C10A0(int64_t npc_obj, int64_t pc_obj)
{
    int v1;

    if (obj_field_int32_get(pc_obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    if (obj_field_int32_get(npc_obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return;
    }

    v1 = sub_4C1290(npc_obj, pc_obj);
    sub_4C1490(npc_obj, 0, v1, 0);
    sub_4C1360(npc_obj, pc_obj, v1);
}

// 0x4C1110
int64_t sub_4C1110(int64_t npc)
{
    if (obj_field_int32_get(npc, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return 0;
    }

    return obj_arrayfield_handle_get(npc, OBJ_F_NPC_REACTION_PC_IDX, 0);
}

// TODO: Figure out the math.
//
// 0x4C1150
int sub_4C1150(int64_t a1, int64_t a2, int a3)
{
    int v1;
    int v2;

    v1 = reaction_get(a1, a2);
    if (v1 > 100) {
        v1 = 100;
    } else {
        if (v1 < 0) {
            v2 = 200;
            return a3 * v2 / 100;
        }

        if (v1 < 50) {
            v2 = 2 * (100 - v1);
            return a3 * v2 / 100;
        }
    }

    v2 = 120 - 2 * v1 / 5;
    return a3 * v2 / 100;
}

// 0x4C11D0
void sub_4C11D0(int64_t a1, int64_t a2, int a3)
{
    int v1;

    if (a3 < 0) {
        v1 = a3 / 100;
        if (v1 == 0) {
            v1 = -2;
        } else if (v1 < -30) {
            v1 = -30;
        }
    } else if (a3 > 50
        && reaction_translate(reaction_get(a1, a2)) < REACTION_AMIABLE) {
        if (a3 < 200) {
            v1 = a3 / 40;
        } else if (a3 < 1000) {
            v1 = (a3 - 200) / 100 + 5;
        } else {
            v1 = 15;
        }
    } else {
        v1 = 0;
    }

    if (v1 != 0) {
        reaction_adj(a1, a2, v1);
    }
}

// 0x4C1290
int sub_4C1290(int64_t npc_obj, int64_t pc_obj)
{
    int v1;

    if (sub_4C12F0(npc_obj, pc_obj, 1, &v1)) {
        return v1;
    }

    return reaction_get_base(npc_obj);
}

// 0x4C12D0
int reaction_get_base(int64_t obj)
{
    return obj_field_int32_get(obj, OBJ_F_NPC_REACTION_BASE);
}

// 0x4C12F0
bool sub_4C12F0(int64_t npc_obj, int64_t pc_obj, bool a3, int* a4)
{
    int index;

    for (index = 0; index < 10; index++) {
        if ((index != 0 || a3)
            && pc_obj == obj_arrayfield_handle_get(npc_obj, OBJ_F_NPC_REACTION_PC_IDX, index)) {
            *a4 = obj_arrayfield_uint32_get(npc_obj, OBJ_F_NPC_REACTION_LEVEL_IDX, index);
            return true;
        }
    }

    return false;
}

// 0x4C1360
void sub_4C1360(int64_t npc_obj, int64_t pc_obj, int value)
{
    int index;
    int candidate;
    int64_t obj;
    int v1;
    int reaction_level;
    int v2;
    int v3;

    // NOTE: Silence compiler warning.
    v3 = -1;

    candidate = -1;
    for (index = 0; index < 10; index++) {
        obj = obj_arrayfield_handle_get(npc_obj, OBJ_F_NPC_REACTION_PC_IDX, index);
        if (obj == pc_obj) {
            sub_4C1490(npc_obj, pc_obj, value, index);
            return;
        }

        if (obj == OBJ_HANDLE_NULL
            && index != 0
            && candidate == -1) {
            candidate = index;
        }
    }

    if (candidate != -1) {
        sub_4C1490(npc_obj, pc_obj, value, candidate);
        return;
    }

    // FIXME: Unused.
    datetime_current_second();

    v1 = sub_4C15A0(value);
    for (index = 0; index < 10; index++) {
        if (index != 0) {
            reaction_level = obj_arrayfield_uint32_get(npc_obj, OBJ_F_NPC_REACTION_LEVEL_IDX, index);

            // FIXME: Unused.
            obj_arrayfield_uint32_get(npc_obj, OBJ_F_NPC_REACTION_TIME_IDX, index);

            v2 = sub_4C15A0(reaction_level);
            if (v2 < v1 && (candidate == -1 || v2 < v3)) {
                candidate = index;
                v3 = v2;
            }
        }
    }

    if (candidate != -1) {
        sub_4C1490(npc_obj, pc_obj, value, candidate);
        return;
    }
}

// 0x4C1490
void sub_4C1490(int64_t npc_obj, int64_t pc_obj, int level, int index)
{
    sub_4F0070(npc_obj, OBJ_F_NPC_REACTION_PC_IDX, index, pc_obj);
    mp_obj_arrayfield_uint32_set(npc_obj, OBJ_F_NPC_REACTION_LEVEL_IDX, index, level);
    mp_obj_arrayfield_uint32_set(npc_obj, OBJ_F_NPC_REACTION_TIME_IDX, index, datetime_current_second());
    ui_refresh_health_bar(npc_obj);
    combat_recalc_reaction(npc_obj);
}

// 0x4C1500
int sub_4C1500(int64_t npc_obj, int64_t pc_obj, unsigned int flags)
{
    int v1 = 0;
    int modifier;
    int npc_race;
    int pc_race;

    if ((obj_field_int32_get(npc_obj, OBJ_F_NPC_FLAGS) & ONF_ALOOF) == 0
        && !critter_is_monstrous(npc_obj)) {
        modifier = stat_level_get(pc_obj, STAT_REACTION_MODIFIER);
        npc_race = stat_level_get(npc_obj, STAT_RACE);
        pc_race = stat_level_get(pc_obj, STAT_RACE);

        v1 = dword_5B684C[npc_race][pc_race] + modifier;
        if ((flags & 0x1) != 0) {
            if (v1 < -49) {
                v1 = -49;
            }
        }
    }

    v1 += reputation_reaction_adj(pc_obj, npc_obj);

    return v1;
}

// 0x4C15A0
int sub_4C15A0(int a1)
{
    // FIXME: Unused.
    datetime_current_second();

    if (a1 > 50) {
        return a1 - 50;
    } else {
        return 50 - a1;
    }
}
