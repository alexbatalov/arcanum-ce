#include "game/script.h"

#include "game/ai.h"
#include "game/anim.h"
#include "game/animfx.h"
#include "game/area.h"
#include "game/bless.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/curse.h"
#include "game/dialog.h"
#include "game/effect.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/level.h"
#include "game/li.h"
#include "game/location.h"
#include "game/mes.h"
#include "game/monstergen.h"
#include "game/mp_utils.h"
#include "game/multiplayer.h"
#include "game/newspaper.h"
#include "game/object.h"
#include "game/player.h"
#include "game/portal.h"
#include "game/quest.h"
#include "game/random.h"
#include "game/reaction.h"
#include "game/reputation.h"
#include "game/rumor.h"
#include "game/script_name.h"
#include "game/scroll.h"
#include "game/sector.h"
#include "game/skill.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/tb.h"
#include "game/teleport.h"
#include "game/trap.h"
#include "game/ui.h"

#define MAX_CACHE_ENTRIES 100
#define MAX_GLOBAL_VARS 2000
#define MAX_GLOBAL_FLAGS 100

#define NEXT -1
#define RETURN_AND_SKIP_DEFAULT -2
#define RETURN_AND_RUN_DEFAULT -3

typedef struct ScriptCacheEntry {
    /* 0000 */ int script_id;
    /* 0008 */ DateTime datetime;
    /* 0010 */ int ref_count;
    /* 0014 */ ScriptFile* file;
} ScriptCacheEntry;

typedef struct ScriptState {
    /* 0000 */ ScriptInvocation* invocation;
    /* 0004 */ int field_4;
    /* 0008 */ int loop_cnt;
    /* 000C */ int field_C;
    /* 0010 */ int script_num;
    /* 0014 */ int field_14;
    /* 0018 */ int64_t current_loop_obj;
    /* 0020 */ int64_t objs[100];
    /* 0340 */ ObjectList objects;
    /* 0398 */ int field_398;
    /* 039C */ int lc_vars[10];
    /* 03C4 */ int field_3C4;
    /* 03C8 */ int64_t lc_objs[10];
} ScriptState;

static int script_execute_condition(ScriptCondition* condition, int line, ScriptState* state);
static int script_execute_action(ScriptAction* action, int line, ScriptState* state);
static bool sub_44AFF0(TimeEvent* timeevent);
static void sub_44B030(ScriptAction* action, ScriptState* state);
static void sub_44B170(ScriptAction* action, ScriptState* state);
static void script_float_line(ScriptAction* action, ScriptState* state);
static void script_print_line(ScriptAction* action, ScriptState* state);
static int script_resolve_focus_obj(ScriptFocusObject type, int index, ScriptState* state, int64_t* objs, ObjectList* l);
static void sub_44B8F0(ScriptFocusObject type, ObjectList* l);
static int64_t script_get_obj(ScriptFocusObject type, int index, ScriptState* state);
static void script_set_obj(ScriptFocusObject type, int index, ScriptState* state, int64_t obj);
static int script_get_value(ScriptValueType type, int index, ScriptState* state);
static void script_set_value(ScriptValueType type, int index, ScriptState* state, int value);
static int sub_44BC60(ScriptState* state);
static bool sub_44C140(Script* scr, unsigned int index, ScriptCondition* entry);
static bool sub_44C1B0(ScriptFile* script_file, unsigned int index, ScriptCondition* entry);
static ScriptFile* script_lock(int script_id);
static void script_unlock(int script_id);
static bool script_file_create(ScriptFile** script_file_ptr);
static bool script_file_destroy(ScriptFile* script_file);
static bool cache_add(int cache_entry_id, int script_id);
static void cache_remove(int cache_entry_id);
static int cache_find(int script_id);
static bool script_file_load_hdr(TigFile* stream, ScriptHeader* hdr);
static bool script_file_load_code(TigFile* stream, ScriptFile* script_file);
static void script_fx_play(int64_t obj, int fx_id);
static void script_fx_stop(int64_t obj, int fx_id);

// 0x5A56FC
static mes_file_handle_t script_story_state_mes_file = MES_FILE_HANDLE_INVALID;

// 0x5A5700
int dword_5A5700[3] = {
    997,
    998,
    999,
};

// 0x5E2FA0
static int* script_global_vars;

// 0x5E2FA4
static int script_story_state;

// 0x5E2FA8
static int* script_global_flags;

// 0x5E2FAC
static IsoInvalidateRectFunc* script_iso_invalidate_rect;

// 0x5E2FB0
static AnimFxList script_eye_candies;

// 0x5E2FDC
static bool script_editor;

// 0x5E2FE0
static IsoRedrawFunc* script_iso_window_draw;

// 0x5E2FE4
static int dword_5E2FE4;

// 0x5E2FE8
static ScriptFloatLineFunc* script_float_line_func;

// 0x5E2FEC
static ScriptStartDialogFunc* script_start_dialog_func;

// 0x5E2FF0
static ScriptCacheEntry* script_cache_entries;

// 0x5E2FF8
static int64_t qword_5E2FF8;

// 0x4446E0
bool script_init(GameInitInfo* init_info)
{
    int index;

    script_editor = init_info->editor;
    script_cache_entries = (ScriptCacheEntry*)CALLOC(MAX_CACHE_ENTRIES, sizeof(ScriptCacheEntry));
    script_global_vars = (int*)CALLOC(MAX_GLOBAL_VARS, sizeof(int));
    script_global_flags = (int*)CALLOC(MAX_GLOBAL_FLAGS, sizeof(int));

    for (index = 0; index < MAX_CACHE_ENTRIES; index++) {
        script_cache_entries[index].script_id = 0;
    }

    script_start_dialog_func = NULL;
    script_float_line_func = NULL;
    script_story_state = 0;

    for (index = 0; index < MAX_GLOBAL_VARS; index++) {
        script_global_vars[index] = 0;
    }

    for (index = 0; index < MAX_GLOBAL_FLAGS; index++) {
        script_global_flags[index] = 0;
    }

    script_iso_invalidate_rect = init_info->invalidate_rect_func;
    script_iso_window_draw = init_info->draw_func;

    if (!script_editor) {
        if (!animfx_list_init(&script_eye_candies)) {
            return false;
        }

        script_eye_candies.path = "Rules\\ScriptEyeCandy.mes";
        script_eye_candies.capacity = 10;

        if (!animfx_list_load(&script_eye_candies)) {
            return false;
        }
    }

    return true;
}

// 0x4447D0
void script_reset()
{
    int index;

    for (index = 0; index < MAX_CACHE_ENTRIES; index++) {
        cache_remove(index);
    }

    script_story_state = 0;

    for (index = 0; index < MAX_GLOBAL_VARS; index++) {
        script_global_vars[index] = 0;
    }

    for (index = 0; index < MAX_GLOBAL_FLAGS; index++) {
        script_global_flags[index] = 0;
    }
}

// 0x444830
void script_exit()
{
    int index;

    for (index = 0; index < MAX_CACHE_ENTRIES; index++) {
        cache_remove(index);
    }

    script_story_state = 0;
    FREE(script_cache_entries);
    FREE(script_global_vars);
    FREE(script_global_flags);

    if (!script_editor) {
        animfx_list_exit(&script_eye_candies);
    }
}

// 0x444890
bool script_mod_load()
{
    mes_load("mes\\storystate.mes", &script_story_state_mes_file);

    return true;
}

// 0x4448B0
void script_mod_unload()
{
    if (script_story_state_mes_file != MES_FILE_HANDLE_INVALID) {
        mes_unload(script_story_state_mes_file);
        script_story_state_mes_file = MES_FILE_HANDLE_INVALID;
    }
}

// 0x4448D0
bool script_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(script_global_vars, sizeof(int) * MAX_GLOBAL_VARS, 1, load_info->stream) != 1) return false;
    if (tig_file_fread(script_global_flags, sizeof(int) * MAX_GLOBAL_FLAGS, 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&script_story_state, sizeof(int), 1, load_info->stream) != 1) return false;

    return true;
}

// 0x444930
bool script_save(TigFile* stream)
{
    if (tig_file_fwrite(script_global_vars, sizeof(int) * MAX_GLOBAL_VARS, 1, stream) != 1) return false;
    if (tig_file_fwrite(script_global_flags, sizeof(int) * MAX_GLOBAL_FLAGS, 1, stream) != 1) return false;
    if (tig_file_fwrite(&script_story_state, sizeof(int), 1, stream) != 1) return false;

    return true;
}

// 0x444990
void script_set_callbacks(ScriptStartDialogFunc* start_dialog_func, ScriptFloatLineFunc* float_line_func)
{
    script_start_dialog_func = start_dialog_func;
    script_float_line_func = float_line_func;
}

// 0x4449B0
bool script_execute(ScriptInvocation* invocation)
{
    ScriptFlags flags;
    int attachee_type;
    bool script_num_changed;
    int saved_script_num;
    ScriptState state;
    ScriptFile* script_file;
    int iter;
    int line;
    int next;
    ScriptCondition statement;
    bool run_default;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        // NOTE: Script at index 1 is not being checked, probably a bug.
        if (invocation->script->num != dword_5A5700[0]
            && invocation->script->num != dword_5A5700[2]) {
            return false;
        }
    }

    if (trap_script_execute(invocation)) {
        return false;
    }

    iter = 0;
    run_default = false;

    if (!script_flags(invocation->script, &flags)) {
        return false;
    }

    script_num_changed = false;

    if (invocation->attachee_obj != OBJ_HANDLE_NULL) {
        attachee_type = obj_field_int32_get(invocation->attachee_obj, OBJ_F_TYPE);
        if (attachee_type == OBJ_TYPE_NPC
            && (obj_field_int32_get(invocation->attachee_obj, OBJ_F_NPC_FLAGS) & ONF_GENERATOR) != 0) {
            return true;
        }
    }

    if (invocation->attachment_point == SAP_DIALOG && attachee_type == OBJ_TYPE_NPC) {
        if (critter_is_dead(invocation->attachee_obj)
            && (flags & SF_DEATH_SPEECH) == 0) {
            return true;
        }

        if (ai_surrendered(invocation->attachee_obj, NULL)
            && (flags & SF_SURRENDER_SPEECH) == 0) {
            return true;
        }

        if ((obj_field_int32_get(invocation->attachee_obj, OBJ_F_SPELL_FLAGS) & OSF_MIND_CONTROLLED) != 0) {
            saved_script_num = invocation->script->num;
            invocation->script->num = 2188;
            script_num_changed = true;
        }
    }

    state.invocation = invocation;
    state.loop_cnt = 0;
    state.script_num = -1;
    state.current_loop_obj = OBJ_HANDLE_NULL;
    memset(state.lc_vars, 0, sizeof(state.lc_vars));
    memset(state.lc_objs, 0, sizeof(state.lc_objs));

    script_file = script_lock(invocation->script->num);
    if (script_file != NULL) {
        line = invocation->line;

        // NOTE: Original code is probably different.
        for (iter = 0; iter < 1000; iter++) {
            if (!sub_44C1B0(script_file, line, &statement)) {
                run_default = false;
                break;
            }

            next = script_execute_condition(&statement, line, &state);
            if (next == NEXT) {
                if (line < script_file->num_entries - 1) {
                    line++;
                } else {
                    next = RETURN_AND_SKIP_DEFAULT;
                }
            } else {
                line = next;
            }

            if (next == RETURN_AND_SKIP_DEFAULT
                || next == RETURN_AND_RUN_DEFAULT
                || next == -4) {
                if ((flags & SF_AUTO_REMOVING) != 0) {
                    invocation->script->num = 0;
                }

                if (next == RETURN_AND_RUN_DEFAULT || next == -4) {
                    run_default = true;
                }
                break;
            }
        }

        script_unlock(invocation->script->num);
    } else {
        run_default = true;
    }

    if (state.script_num != -1) {
        invocation->script->num = state.script_num;
    }

    if (script_num_changed) {
        invocation->script->num = saved_script_num;
    }

    if (state.loop_cnt != 0) {
        sub_44B8F0(state.field_398, &(state.objects));
    }

    return run_default;
}

// 0x444C80
int script_global_var_get(int index)
{
    return script_global_vars[index];
}

// 0x444C90
void script_global_var_set(int index, int value)
{
    if (!multiplayer_is_locked()) {
        PacketScriptFunc pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 124;
        pkt.subtype = SCRIPT_FUNC_SET_GLOBAL_VAR;
        pkt.index = index;
        pkt.value = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    script_global_vars[index] = value;
}

// 0x444CF0
int script_global_flag_get(int index)
{
    return (script_global_flags[index / 32] >> (index % 32)) & 1;
}

// 0x444D20
void script_global_flag_set(int index, int value)
{
    if (!multiplayer_is_locked()) {
        PacketScriptFunc pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 124;
        pkt.subtype = SCRIPT_FUNC_SET_GLOBAL_FLAG;
        pkt.index = index;
        pkt.value = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    script_global_flags[index / 32] &= ~(1 << (index % 32));
    script_global_flags[index / 32] |= value << (index % 32);
}

// 0x444DB0
int script_pc_var_get(int64_t obj, int index)
{
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        return obj_arrayfield_uint32_get(obj, OBJ_F_PC_GLOBAL_VARIABLES, index);
    }

    return 0;
}

// 0x444DF0
void script_pc_var_set(int64_t obj, int index, int value)
{
    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        mp_obj_arrayfield_uint32_set(obj, OBJ_F_PC_GLOBAL_VARIABLES, index, value);
    }
}

// 0x444E30
int script_pc_flag_get(int64_t obj, int index)
{
    int flags;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        flags = obj_arrayfield_uint32_get(obj, OBJ_F_PC_GLOBAL_FLAGS, index / 32);
        return (flags >> (index % 32)) & 1;
    }

    return 0;
}

// 0x444E90
void script_pc_flag_set(int64_t obj, int index, int value)
{
    int flags;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        flags = obj_arrayfield_uint32_get(obj, OBJ_F_PC_GLOBAL_FLAGS, index / 32);
        flags &= ~(1 << (index % 32));
        flags |= value << (index % 32);
        mp_obj_arrayfield_uint32_set(obj, OBJ_F_PC_GLOBAL_FLAGS, index / 32, flags);
    }
}

// 0x444F10
bool script_local_flag_get(int64_t obj, int index, int flag)
{
    Script scr;

    obj_arrayfield_script_get(obj, OBJ_F_SCRIPTS_IDX, index, &scr);

    return scr.num != 0 ? (scr.hdr.flags & (1 << flag)) != 0 : false;
}

// 0x444F60
void script_local_flag_set(int64_t obj, int index, int flag, bool enabled)
{
    Script scr;

    obj_arrayfield_script_get(obj, OBJ_F_SCRIPTS_IDX, index, &scr);

    if (scr.num != 0) {
        if (enabled) {
            scr.hdr.flags |= 1 << flag;
        } else {
            scr.hdr.flags &= ~(1 << flag);
        }

        mp_obj_arrayfield_script_set(obj, OBJ_F_SCRIPTS_IDX, index, &scr);
    }
}

// 0x444FD0
int script_local_counter_get(int64_t obj, int index, int counter)
{
    Script scr;

    obj_arrayfield_script_get(obj, OBJ_F_SCRIPTS_IDX, index, &scr);

    return scr.num != 0 ? (scr.hdr.counters >> (8 * counter)) : 0;
}

// 0x445020
void script_local_counter_set(int64_t obj, int index, int counter, int value)
{
    Script scr;

    obj_arrayfield_script_get(obj, OBJ_F_SCRIPTS_IDX, index, &scr);

    if (scr.num) {
        scr.hdr.counters &= ~(0xFF << (8 * counter));
        scr.hdr.counters |= value << (8 * counter);

        mp_obj_arrayfield_script_set(obj, OBJ_F_SCRIPTS_IDX, index, &scr);
    }
}

// 0x445090
int script_story_state_get()
{
    return script_story_state;
}

// 0x4450A0
void script_story_state_set(int value)
{
    if (!multiplayer_is_locked()) {
        PacketScriptFunc pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 124;
        pkt.subtype = SCRIPT_FUNC_SET_STORY_STATE;
        pkt.story_state = value;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    if (value > script_story_state) {
        script_story_state = value;
    }
}

// 0x4450F0
char* script_story_state_info(int story_state_num)
{
    MesFileEntry mes_file_entry;

    if (script_story_state_mes_file == MES_FILE_HANDLE_INVALID) {
        return "";
    }

    mes_file_entry.num = story_state_num;
    if (!mes_search(script_story_state_mes_file, &mes_file_entry)) {
        return "";
    }

    mes_get_msg(script_story_state_mes_file, &mes_file_entry);

    return mes_file_entry.str;
}

// 0x445140
bool script_timeevent_process(TimeEvent* timeevent)
{
    Script scr;
    ScriptInvocation invocation;

    scr.num = timeevent->params[0].integer_value;
    script_load_hdr(&scr);

    invocation.script = &scr;
    invocation.triggerer_obj = timeevent->params[2].object_value;
    invocation.attachee_obj = timeevent->params[3].object_value;
    invocation.attachment_point = SAP_USE;
    invocation.extra_obj = OBJ_HANDLE_NULL;
    invocation.line = timeevent->params[1].integer_value;
    script_execute(&invocation);

    return true;
}

// 0x4451B0
int script_execute_condition(ScriptCondition* condition, int line, ScriptState* state)
{
    int rc = -1;
    int value;
    int64_t objs[100];
    ObjectList objects;
    int cnt;
    int index;
    int matched;

    switch (condition->type) {
    case SCT_TRUE:
        rc = script_execute_action(&(condition->action), line, state);
        break;
    case SCT_DAYTIME:
        // it is daytime

        if (game_time_is_day()) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_HAS_GOLD:
        // (obj) has at least (num) gold

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        value = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (item_gold_get(objs[index]) >= value) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_LOCAL_FLAG:
        // local flag (num) is set

        value = script_get_value(condition->op_type[0], condition->op_value[0], state);
        if ((state->invocation->script->hdr.flags & (1 << value)) != 0) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_EQ:
        // (num) == (num)

        if (script_get_value(condition->op_type[0], condition->op_value[0], state) == script_get_value(condition->op_type[1], condition->op_value[1], state)) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_LE:
        // (num) <= (num)

        if (script_get_value(condition->op_type[0], condition->op_value[0], state) <= script_get_value(condition->op_type[1], condition->op_value[1], state)) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_PC_QUEST_STATE: {
        // PC (obj) has quest (num) in state (num)

        int quest;
        int quest_state;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        quest = script_get_value(condition->op_type[1], condition->op_value[1], state);
        quest_state = script_get_value(condition->op_type[2], condition->op_value[2], state);

        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (quest_state_get(objs[index], quest) == quest_state) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_GLOBAL_QUEST_STATE: {
        // {quest (num) in global state (num)}

        int quest;
        int quest_state;

        quest = script_get_value(condition->op_type[0], condition->op_value[0], state);
        quest_state = script_get_value(condition->op_type[1], condition->op_value[1], state);
        if (quest_global_state_get(quest) == quest_state) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_HAS_BLESS: {
        // (obj) has bless (num)

        int bless;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        bless = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (bless_has(objs[index], bless)) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_HAS_CURSE: {
        // (obj) has curse (num)

        int curse;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        curse = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (curse_has(objs[index], curse)) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_MET_PC_BEFORE:
        // npc (obj) has met pc (obj) before

        objs[0] = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        objs[1] = script_get_obj(condition->op_type[1], condition->op_value[1], state);
        if (reaction_met_before(objs[0], objs[1])) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_OBJ_HAS_BAD_ASSOCIATES: {
        // (obj) has bad associates

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (critter_has_bad_associates(objs[index])) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_POLYMORPHED: {
        // (obj) is polymorphed

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_SPELL_FLAGS) & OSF_POLYMORPHED) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_SHRUNK: {
        // (obj) is shrunk

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_SPELL_FLAGS) & OSF_SHRUNK) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_HAS_BODY_SPELL: {
        // (obj) has a body spell

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_SPELL_FLAGS) & (OSF_BODY_OF_AIR | OSF_BODY_OF_EARTH | OSF_BODY_OF_FIRE | OSF_BODY_OF_WATER)) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_INVISIBLE: {
        // (obj) is invisible

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_SPELL_FLAGS) & OSF_INVISIBLE) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_HAS_MIRROR_IMAGE: {
        // (obj) has mirror image

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_SPELL_FLAGS) & OSF_MIRRORED) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_HAS_ITEM_NAMED: {
        // (obj) has item named (num)

        int name;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        name = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (item_find_by_name(objs[index], name) != OBJ_HANDLE_NULL) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_FOLLOWING_PC: {
        // npc (obj) is a follower of pc (obj)

        objs[0] = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        objs[1] = script_get_obj(condition->op_type[1], condition->op_value[1], state);
        if (critter_leader_get(objs[0]) == objs[1]) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_MONSTER_OF_TYPE: {
        // npc (obj) is a monster of specie (num)

        int type;
        tig_art_id_t art_id;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        type = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            art_id = obj_field_int32_get(objs[index], OBJ_F_AID);
            if (tig_art_type(art_id) == TIG_ART_TYPE_MONSTER
                && tig_art_monster_id_specie_get(art_id) == type) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_NAMED: {
        // (obj) is named (num)

        int name;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        name = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_SPELL_FLAGS) & OSF_MIND_CONTROLLED) == 0
                && obj_field_int32_get(objs[index], OBJ_F_NAME) == name) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_WIELDING_ITEM: {
        // (obj) is wielding item named (num)

        int name;
        int inventory_location;
        int64_t item_obj;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        name = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            for (inventory_location = 1000; inventory_location < 1009; inventory_location++) {
                item_obj = item_wield_get(objs[index], inventory_location);
                if (item_obj != OBJ_HANDLE_NULL
                    && obj_field_int32_get(item_obj, OBJ_F_NAME) == name) {
                    matched++;
                    break;
                }
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_DEAD: {
        // (obj) is dead

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (critter_is_dead(objs[index])) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_HAS_MAX_FOLLOWERS: {
        // (obj) has maximum followers

        int max_followers;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (critter_is_pc(objs[index])) {
                max_followers = stat_level_get(objs[index], STAT_MAX_FOLLOWERS);
                if (max_followers == 0 || critter_num_followers(objs[index], true) >= max_followers) {
                    matched++;
                }
            } else {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_CAN_OPEN_CONTAINER:
        // (obj) can open the container (obj)

        objs[0] = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        objs[1] = script_get_obj(condition->op_type[1], condition->op_value[1], state);
        if (ai_attempt_open_container(objs[0], objs[1]) == AI_ATTEMPT_OPEN_CONTAINER_OK) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_OBJ_HAS_SURRENDERED: {
        // (obj) has surrendered

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (obj_field_int32_get(objs[index], OBJ_F_TYPE) == OBJ_TYPE_NPC
                && ai_surrendered(objs[index], NULL)) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_IN_DIALOG: {
        // (obj) is in dialog

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (obj_field_int32_get(objs[index], OBJ_F_TYPE) == OBJ_TYPE_NPC) {
                if (sub_4C1110(objs[index])) {
                    matched++;
                }
            } else {
                if (sub_460460(objs[index])) {
                    matched++;
                }
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_SWITCHED_OFF: {
        // (obj) is switched off

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_FLAGS) & OF_OFF) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_CAN_SEE_OBJ:
        // (obj) can see (obj)

        objs[0] = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        objs[1] = script_get_obj(condition->op_type[1], condition->op_value[1], state);
        if (ai_can_see(objs[0], objs[1]) == 0) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_OBJ_CAN_HEAR_OBJ:
        // (obj) can hear (obj)

        objs[0] = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        objs[1] = script_get_obj(condition->op_type[1], condition->op_value[1], state);
        if (ai_can_hear(objs[0], objs[1], LOUDNESS_NORMAL) == 0) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_OBJ_IS_INVULNERABLE: {
        // (obj) is invulnerable

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if ((obj_field_int32_get(objs[index], OBJ_F_FLAGS) & OF_INVULNERABLE) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_IN_COMBAT: {
        // (obj) is in combat

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (combat_critter_is_combat_mode_active(objs[index])) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_AT_LOCATION: {
        // (obj) is at location X:(num) Y:(num)

        int64_t obj;
        int x;
        int y;

        obj = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        x = script_get_value(condition->op_type[1], condition->op_value[1], state);
        y = script_get_value(condition->op_type[2], condition->op_value[2], state);

        if (location_make(x, y) == obj_field_int64_get(obj, OBJ_F_LOCATION)) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    }
    case SCT_OBJ_HAS_REPUTATION: {
        // (obj) has reputation (num)

        int reputation;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        reputation = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (reputation_has(objs[index], reputation)) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_WITHIN_RANGE: {
        // (obj) is within (num) tiles of location X:(num) Y:(num)

        int64_t obj;
        int range;
        int x;
        int y;

        obj = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        range = script_get_value(condition->op_type[1], condition->op_value[1], state);
        x = script_get_value(condition->op_type[2], condition->op_value[2], state);
        y = script_get_value(condition->op_type[3], condition->op_value[3], state);

        if (location_dist(location_make(x, y), obj_field_int64_get(obj, OBJ_F_LOCATION)) <= range) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    }
    case SCT_OBJ_IS_INFLUENCED_BY_SPELL: {
        // (obj) is under the influence of spell (num)

        int spl;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        spl = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (magictech_is_under_influence_of(objs[index], spl)) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_OPEN: {
        // (obj) is open

        int64_t obj;
        int type;
        bool is_open;

        obj = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        type = obj_field_int32_get(obj, OBJ_F_TYPE);
        if (type == OBJ_TYPE_PORTAL) {
            is_open = portal_is_open(obj);
        } else if (type == OBJ_TYPE_CONTAINER) {
            is_open = tig_art_id_frame_get(obj_field_int32_get(obj, OBJ_F_CURRENT_AID)) != 0;
        } else {
            is_open = false;
        }

        if (is_open) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    }
    case SCT_OBJ_IS_ANIMAL: {
        // (obj) is an animal

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (obj_field_int32_get(objs[index], OBJ_F_TYPE) == OBJ_TYPE_NPC
                && (obj_field_int32_get(objs[index], OBJ_F_CRITTER_FLAGS) & OCF_ANIMAL) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_IS_UNDEAD: {
        // (obj) is undead

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (obj_field_int32_get(objs[index], OBJ_F_TYPE) == OBJ_TYPE_NPC
                && (obj_field_int32_get(objs[index], OBJ_F_CRITTER_FLAGS) & OCF_UNDEAD) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_OBJ_JILTED: {
        // (obj) was jilted by a PC

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (obj_field_int32_get(objs[index], OBJ_F_TYPE) == OBJ_TYPE_NPC
                && (obj_field_int32_get(objs[index], OBJ_F_NPC_FLAGS) & ONF_JILTED) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_RUMOR_KNOWN: {
        // pc (obj) knows rumor (num)
        int64_t obj;
        int rumor;

        obj = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        rumor = script_get_value(condition->op_type[1], condition->op_value[1], state);
        if (rumor_known_get(obj, rumor)) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    }
    case SCT_RUMOR_QUELLED: {
        // rumor (num) has been quelled

        int rumor;

        rumor = script_get_value(condition->op_type[0], condition->op_value[0], state);
        if (rumor_qstate_get(rumor)) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    }
    case SCT_OBJ_IS_BUSTED: {
        // (obj) is busted

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (object_is_busted(objs[index])) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_GLOBAL_FLAG: {
        // global flag (num) is set

        int num;

        num = script_get_value(condition->op_type[0], condition->op_value[0], state);
        if (script_global_var_get(num)) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    }
    case SCT_CAN_OPEN_PORTAL: {
        // (obj) can open the portal (obj) in direction (num)

        int direction;

        objs[0] = script_get_obj(condition->op_type[0], condition->op_value[0], state);
        objs[1] = script_get_obj(condition->op_type[1], condition->op_value[1], state);
        direction = script_get_value(condition->op_type[2], condition->op_value[2], state);

        if (ai_attempt_open_portal(objs[0], objs[1], direction) == AI_ATTEMPT_OPEN_PORTAL_OK) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_SECTOR_IS_BLOCKED: {
        // sector at location X:(num) and Y:(num) is blocked

        int x;
        int y;

        x = script_get_value(condition->op_type[0], condition->op_value[0], state);
        y = script_get_value(condition->op_type[1], condition->op_value[1], state);
        if (sector_is_blocked(sector_id_from_loc(location_make(x, y)))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_MONSTERGEN_DISABLED: {
        // monster generator (num) is disabled

        int num;

        num = script_get_value(condition->op_type[0], condition->op_value[0], state);
        if (monstergen_is_disabled(num)) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    }
    case SCT_IDENTIFIED: {
        // (obj) is identified

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (item_is_identified(objs[index])) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_KNOWS_SPELL: {
        // (obj) knows spell (num)

        int spl;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        spl = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (spell_is_known(objs[index], spl)) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_MASTERED_SPELL_COLLEGE: {
        // (obj) has mastered spell college (num)

        int college;

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        college = script_get_value(condition->op_type[1], condition->op_value[1], state);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (spell_mastery_get(objs[index]) == college) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_ITEMS_ARE_BEING_REWIELDED:
        if (item_in_rewield()) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
        break;
    case SCT_PROWLING: {
        // (obj) is prowling

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (critter_is_concealed(objs[index])) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    case SCT_WAITING_FOR_LEADER: {
        // (obj) is waiting for leader's return

        cnt = script_resolve_focus_obj(condition->op_type[0], condition->op_value[0], state, objs, &objects);
        matched = 0;
        for (index = 0; index < cnt; index++) {
            if (obj_field_int32_get(objs[index], OBJ_F_TYPE) == OBJ_TYPE_NPC
                && (obj_field_int32_get(objs[index], OBJ_F_NPC_FLAGS) & ONF_AI_WAIT_HERE) != 0) {
                matched++;
            }
        }
        sub_44B8F0(condition->op_type[0], &objects);
        if (matched != 0
            && (matched == cnt || sfo_is_any(condition->op_type[0]))) {
            rc = script_execute_action(&(condition->action), line, state);
        } else {
            rc = script_execute_action(&(condition->els), line, state);
        }
    } break;
    default:
        break;
    }

    return rc;
}

// 0x447220
int script_execute_action(ScriptAction* action, int line, ScriptState* state)
{
    ObjectList objects;
    int64_t handles[100];
    CombatContext combat;

    switch (action->type) {
    case SAT_RETURN_AND_SKIP_DEFAULT:
        return RETURN_AND_SKIP_DEFAULT;
    case SAT_RETURN_AND_RUN_DEFAULT:
        return RETURN_AND_RUN_DEFAULT;
    case SAT_GOTO:
        return script_get_value(action->op_type[0], action->op_value[0], state);
    case SAT_DIALOG:
        if (script_start_dialog_func != NULL && player_is_pc_obj(state->invocation->triggerer_obj)) {
            int num;
            Script scr;

            num = script_get_value(action->op_type[0], action->op_value[0], state);
            obj_arrayfield_script_get(state->invocation->attachee_obj, OBJ_F_SCRIPTS_IDX, state->invocation->attachment_point, &scr);

            if (scr.hdr.flags != state->invocation->script->hdr.flags
                || scr.hdr.counters != state->invocation->script->hdr.counters) {
                scr.hdr.flags = state->invocation->script->hdr.counters;
                scr.hdr.counters = state->invocation->script->hdr.counters;
                obj_arrayfield_script_set(state->invocation->attachee_obj, OBJ_F_SCRIPTS_IDX, state->invocation->attachment_point, &scr);
            }

            script_start_dialog_func(state->invocation->triggerer_obj,
                state->invocation->attachee_obj,
                state->invocation->script->num,
                line,
                num);
        }
        return RETURN_AND_SKIP_DEFAULT;
    case SAT_REMOVE_THIS_SCRIPT:
        state->script_num = 0;
        return NEXT;
    case SAT_CHANGE_THIS_SCRIPT_TO_SCRIPT:
        state->script_num = script_get_value(action->op_type[0], action->op_value[0], state);
        return NEXT;
    case SAT_CALL_SCRIPT:
        sub_44B030(action, state);
        return NEXT;
    case SAT_SET_LOCAL_FLAG: {
        int flag = script_get_value(action->op_type[0], action->op_value[0], state);
        state->invocation->script->hdr.flags |= 1 << flag;
        return NEXT;
    }
    case SAT_CLEAR_LOCAL_FLAG: {
        int flag = script_get_value(action->op_type[0], action->op_value[0], state);
        state->invocation->script->hdr.flags &= ~(1 << flag);
        return NEXT;
    }
    case SAT_ASSIGN_NUM:
        script_set_value(action->op_type[0], action->op_value[0], state,
            script_get_value(action->op_type[1], action->op_value[1], state));
        return NEXT;
    case SAT_ADD: {
        int value1 = script_get_value(action->op_type[1], action->op_value[1], state);
        int value2 = script_get_value(action->op_type[2], action->op_value[2], state);
        script_set_value(action->op_type[0],
            action->op_value[0],
            state,
            value1 + value2);
        return NEXT;
    }
    case SAT_SUBTRACT: {
        int value1 = script_get_value(action->op_type[1], action->op_value[1], state);
        int value2 = script_get_value(action->op_type[2], action->op_value[2], state);
        script_set_value(action->op_type[0],
            action->op_value[0],
            state,
            value1 - value2);
        return NEXT;
    }
    case SAT_MULTIPLY: {
        int value1 = script_get_value(action->op_type[1], action->op_value[1], state);
        int value2 = script_get_value(action->op_type[2], action->op_value[2], state);
        script_set_value(action->op_type[0],
            action->op_value[0],
            state,
            value1 * value2);
        return NEXT;
    }
    case SAT_DIVIDE: {
        int value1 = script_get_value(action->op_type[1], action->op_value[1], state);
        int value2 = script_get_value(action->op_type[2], action->op_value[2], state);
        if (value2 != 0) {
            script_set_value(action->op_type[0],
                action->op_value[0],
                state,
                value1 + value2);
        }
        return NEXT;
    }
    case SAT_ASSIGN_OBJ: {
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        script_set_obj(action->op_type[0], action->op_value[0], state, obj);
        return NEXT;
    }
    case SAT_SET_PC_QUEST_STATE: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int quest_num = script_get_value(action->op_type[1], action->op_value[1], state);
        int quest_state = script_get_value(action->op_type[2], action->op_value[2], state);

        for (int idx = 0; idx < cnt; idx++) {
            quest_state_set(handles[idx],
                quest_num,
                quest_state,
                state->invocation->attachee_obj);
        }

        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_SET_QUEST_GLOBAL_STATE: {
        int quest_num = script_get_value(action->op_type[0], action->op_value[0], state);
        int quest_state = script_get_value(action->op_type[1], action->op_value[1], state);
        quest_global_state_set(quest_num, quest_state);
        return NEXT;
    }
    case SAT_LOOP_FOR:
        if (state->loop_cnt != 0) {
            tig_debug_printf("Script: script_execute_action: sat_loop_for: ERROR: Already in a loop!\n");
            return NEXT;
        }

        state->field_4 = 0;
        state->field_C = line + 1;
        state->field_398 = action->op_type[0];
        state->loop_cnt = script_resolve_focus_obj(action->op_type[0],
            action->op_value[0],
            state,
            state->objs,
            &(state->objects));
        if (state->loop_cnt != 0) {
            state->current_loop_obj = state->objs[state->field_4];
            return NEXT;
        }

        sub_44B8F0(state->field_398, &(state->objects));
        return sub_44BC60(state);
    case SAT_LOOP_END:
        if (state->loop_cnt <= 0) {
            tig_debug_printf("Script: script_execute_action: sat_loop_end: ERROR: Not in a loop!\n");
            return NEXT;
        }

        if (++state->field_4 < state->loop_cnt) {
            state->current_loop_obj = state->objs[state->field_4];
            return state->field_C;
        }

        state->loop_cnt = 0;
        sub_44B8F0(state->field_398, &(state->objects));
        return NEXT;
    case SAT_LOOP_BREAK:
        if (state->loop_cnt <= 0) {
            tig_debug_printf("Script: script_execute_action: sat_loop_break: ERROR: Not in a loop!\n");
            return NEXT;
        }

        state->loop_cnt = 0;
        sub_44B8F0(state->field_398, &(state->objects));
        return sub_44BC60(state);
    case SAT_CRITTER_FOLLOW: {
        int64_t follower_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t leader_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        critter_follow(follower_obj, leader_obj, true);
        return NEXT;
    }
    case SAT_CRITTER_DISBAND: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        critter_disband(obj, true);
        return NEXT;
    }
    case SAT_FLOAT_LINE:
        script_float_line(action, state);
        return NEXT;
    case SAT_PRINT_LINE:
        script_print_line(action, state);
        return NEXT;
    case SAT_ADD_BLESSING: {
        int bless = script_get_value(action->op_type[0], action->op_value[0], state);
        int cnt = script_resolve_focus_obj(action->op_type[1], action->op_value[1], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            bless_add(handles[idx], bless);
        }
        sub_44B8F0(action->op_type[1], &objects);
        return NEXT;
    }
    case SAT_REMOVE_BLESSING: {
        int bless = script_get_value(action->op_type[0], action->op_value[0], state);
        int cnt = script_resolve_focus_obj(action->op_type[1], action->op_value[1], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            bless_remove(handles[idx], bless);
        }
        sub_44B8F0(action->op_type[1], &objects);
        return NEXT;
    }
    case SAT_ADD_CURSE: {
        int curse = script_get_value(action->op_type[0], action->op_value[0], state);
        int cnt = script_resolve_focus_obj(action->op_type[1], action->op_value[1], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            curse_add(handles[idx], curse);
        }
        sub_44B8F0(action->op_type[1], &objects);
        return NEXT;
    }
    case SAT_REMOVE_CURSE: {
        int curse = script_get_value(action->op_type[0], action->op_value[0], state);
        int cnt = script_resolve_focus_obj(action->op_type[1], action->op_value[1], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            curse_remove(handles[idx], curse);
        }
        sub_44B8F0(action->op_type[1], &objects);
        return NEXT;
    }
    case SAT_GET_REACTION: {
        int64_t npc_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t pc_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int reaction = reaction_get(npc_obj, pc_obj);
        script_set_value(action->op_type[2], action->op_value[2], state, reaction);
        return NEXT;
    }
    case SAT_SET_REACTION: {
        int64_t npc_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t pc_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int reaction = script_get_value(action->op_type[2], action->op_value[1], state);
        reaction_adj(npc_obj, pc_obj, reaction - reaction_get(npc_obj, pc_obj));
        return NEXT;
    }
    case SAT_ADJUST_REACTION: {
        int64_t npc_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t pc_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int value = script_get_value(action->op_type[2], action->op_value[2], state);
        reaction_adj(npc_obj, pc_obj, value);
        return NEXT;
    }
    case SAT_GET_ARMOR: {
        int64_t item_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int armor = tig_art_critter_id_armor_get(obj_field_int32_get(item_obj, OBJ_F_CURRENT_AID));
        script_set_value(action->op_type[1], action->op_value[1], state, armor);
        return NEXT;
    }
    case SAT_GET_STAT: {
        int stat = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t critter_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int value = stat_level_get(critter_obj, stat);
        if (stat == STAT_INTELLIGENCE
            && value > LOW_INTELLIGENCE
            && critter_is_dumb(critter_obj)) {
            value = 1;
        }
        script_set_value(action->op_type[2], action->op_value[2], state, value);
        return NEXT;
    }
    case SAT_GET_OBJECT_TYPE: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int type = obj_field_int32_get(obj, OBJ_F_TYPE);
        script_set_value(action->op_type[1], action->op_value[1], state, type);
        return NEXT;
    }
    case SAT_ADJUST_GOLD: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int value = script_get_value(action->op_type[1], action->op_value[1], state);
        if (value > 0) {
            item_gold_transfer(OBJ_HANDLE_NULL, obj, value, OBJ_HANDLE_NULL);
        } else {
            item_gold_transfer(obj, OBJ_HANDLE_NULL, -value, OBJ_HANDLE_NULL);
        }
        return NEXT;
    }
    case SAT_ATTACK: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int64_t target_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            ai_attack(target_obj, handles[idx], LOUDNESS_NORMAL, 2);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_RANDOM: {
        int lower = script_get_value(action->op_type[0], action->op_value[0], state);
        int upper = script_get_value(action->op_type[1], action->op_value[1], state);
        int value = random_between(lower, upper);
        script_set_value(action->op_type[2], action->op_value[2], state, value);
        return NEXT;
    }
    case SAT_GET_SOCIAL_CLASS: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int social_class = obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC
            ? obj_field_int32_get(obj, OBJ_F_NPC_SOCIAL_CLASS)
            : 0;
        script_set_value(action->op_type[1], action->op_value[1], state, social_class);
        return NEXT;
    }
    case SAT_GET_ORIGIN: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int origin = critter_origin_get(obj);
        script_set_value(action->op_type[1], action->op_value[1], state, origin);
        return NEXT;
    }
    case SAT_TRANSFORM_ATTACHEE_INTO_BASIC_PROTOTYPE: {
        int64_t parent_obj;
        int64_t loc;
        int obj_type = obj_field_int32_get(state->invocation->attachee_obj, OBJ_F_TYPE);

        if (obj_type_is_item(obj_type)
            && item_parent(state->invocation->attachee_obj, &parent_obj)) {
            loc = obj_field_int64_get(parent_obj, OBJ_F_LOCATION);
        } else {
            parent_obj = OBJ_HANDLE_NULL;
            loc = obj_field_int64_get(state->invocation->attachee_obj, OBJ_F_LOCATION);
        }

        int fatigue_ratio = obj_type_is_critter(obj_type)
            ? ai_critter_fatigue_ratio(state->invocation->attachee_obj)
            : 100;
        int hp_ratio = ai_object_hp_ratio(state->invocation->attachee_obj);
        tig_art_id_t art_id = obj_field_int32_get(state->invocation->attachee_obj, OBJ_F_CURRENT_AID);
        int rotation = tig_art_id_rotation_get(art_id);
        int proto = script_get_value(action->op_type[0], action->op_value[0], state);

        int64_t new_obj;
        if (mp_object_create(proto, loc, &new_obj)) {
            object_destroy(state->invocation->attachee_obj);
            state->invocation->attachee_obj = new_obj;

            int hp = object_hp_max(new_obj);
            int hp_damage = hp * (100 - hp_ratio) / 100;
            if (hp_damage < hp) {
                object_hp_damage_set(new_obj, hp_damage);
            } else {
                object_hp_damage_set(new_obj, hp - 1);
            }

            art_id = obj_field_int32_get(new_obj, OBJ_F_CURRENT_AID);
            art_id = tig_art_id_rotation_set(art_id, rotation);
            sub_4EDCE0(new_obj, art_id);

            obj_type = obj_field_int32_get(new_obj, OBJ_F_TYPE);
            if (obj_type_is_critter(obj_type)) {
                int fatigue_damage = critter_fatigue_max(new_obj) * (100 - fatigue_ratio) / 100;
                critter_fatigue_damage_set(new_obj, fatigue_damage);
            }

            if (obj_type_is_item(obj_type)) {
                item_transfer(new_obj, parent_obj);
            }
        }

        return NEXT;
    }
    case SAT_TRANSFER_ITEM: {
        int name = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t from_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int64_t to_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        int64_t item_obj = item_find_by_name(from_obj, name);
        if (item_obj != OBJ_HANDLE_NULL) {
            item_transfer(item_obj, to_obj);
        }
        return NEXT;
    }
    case SAT_GET_STORY_STATE: {
        int story_state = script_story_state_get();
        script_set_value(action->op_type[0], action->op_value[0], state, story_state);
        return NEXT;
    }
    case SAT_SET_STORY_STATE: {
        int story_state = script_get_value(action->op_type[0], action->op_value[0], state);
        script_story_state_set(story_state);
        return NEXT;
    }
    case SAT_TELEPORT: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int map = script_get_value(action->op_type[1], action->op_value[1], state);
        int x = script_get_value(action->op_type[2], action->op_value[2], state);
        int y = script_get_value(action->op_type[3], action->op_value[3], state);

        TeleportData teleport_data;
        teleport_data.map = map - 4999;
        teleport_data.flags = 0;
        teleport_data.obj = obj;
        teleport_data.loc = location_make(x, y);
        teleport_do(&teleport_data);

        return NEXT;
    }
    case SAT_SET_DAY_STANDPOINT: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int x = script_get_value(action->op_type[1], action->op_value[1], state);
        int y = script_get_value(action->op_type[2], action->op_value[2], state);

        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            mp_obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_DAY, location_make(x, y));
        }

        return NEXT;
    }
    case SAT_SET_NIGHT_STANDPOINT: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int x = script_get_value(action->op_type[1], action->op_value[1], state);
        int y = script_get_value(action->op_type[2], action->op_value[2], state);

        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            mp_obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_NIGHT, location_make(x, y));
        }

        return NEXT;
    }
    case SAT_GET_SKILL: {
        int skill = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int value = IS_TECH_SKILL(skill)
            ? tech_skill_level(obj, GET_TECH_SKILL(skill))
            : basic_skill_level(obj, GET_BASIC_SKILL(skill));
        script_set_value(action->op_type[2], action->op_value[2], state, value);
    }
    case SAT_CAST_SPELL: {
        int64_t source_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int spell = script_get_value(action->op_type[1], action->op_value[1], state);
        int64_t target_obj = script_get_obj(action->op_type[2], action->op_value[2], state);

        MagicTechInvocation mt_invocation;
        magictech_invocation_init(&mt_invocation, source_obj, spell);
        sub_4440E0(target_obj, &(mt_invocation.target_obj));
        magictech_invocation_run(&mt_invocation);
        return NEXT;
    }
    case SAT_MARK_MAP_LOCATION: {
        int area = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t pc_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        area_set_known(pc_obj, area);
        return NEXT;
    }
    case SAT_SET_RUMOR: {
        int rumor = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        rumor_known_set(obj, rumor);
        return NEXT;
    }
    case SAT_QUELL_RUMOR: {
        int rumor = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        if (obj == player_get_local_pc_obj()) {
            rumor_qstate_set(rumor);
        }
        return NEXT;
    }
    case SAT_CREATE_OBJECT: {
        int proto = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t near_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int64_t loc = -1;
        for (int attempt = 0; attempt < 10; attempt++) {
            if (sub_4F4E40(near_obj, random_between(1, 3), &loc)) {
                break;
            }
        }
        if (loc == -1) {
            loc = obj_field_int64_get(near_obj, OBJ_F_LOCATION);
        }

        int64_t new_obj;
        mp_object_create(proto, loc, &new_obj);

        if (obj_field_int32_get(new_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            unsigned int critter_flags = obj_field_int32_get(new_obj, OBJ_F_CRITTER_FLAGS);
            critter_flags |= OCF_ENCOUNTER;
            mp_obj_field_int32_set(new_obj, OBJ_F_CRITTER_FLAGS, critter_flags);
        }

        return NEXT;
    }
    case SAT_SET_LOCK_STATE: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int num = script_get_value(action->op_type[1], action->op_value[1], state);
        mp_object_locked_set(obj, num);
        return NEXT;
    }
    case SAT_CALL_SCRIPT_IN:
    case SAT_CALL_SCRIPT_AT: {
        int num = script_get_value(action->op_type[0], action->op_value[0], state);
        int line = script_get_value(action->op_type[1], action->op_value[1], state);
        int64_t triggerer_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        int64_t attachee_obj = script_get_obj(action->op_type[3], action->op_value[3], state);
        int seconds = script_get_value(action->op_type[4], action->op_value[4], state);

        DateTime datetime;
        if (action->type == SAT_CALL_SCRIPT_AT) {
            datetime = sub_45A7C0();
            datetime.milliseconds += 86400000;
            datetime_sub_milliseconds(&datetime, sub_45AD70());

            int v1 = datetime_seconds_since_reference_date(&datetime) % 86400;
            seconds = seconds % 86400 - v1 + (seconds % 86400 - v1 < 0 ? 86400 : 0);
        }

        TimeEvent timeevent;
        timeevent.type = TIMEEVENT_TYPE_SCRIPT;
        timeevent.params[0].integer_value = num;
        timeevent.params[1].integer_value = line;
        timeevent.params[2].object_value = triggerer_obj;
        timeevent.params[3].object_value = attachee_obj;

        sub_45A950(&datetime, 1000 * seconds);

        if (action->type == SAT_CALL_SCRIPT_IN) {
            datetime.milliseconds *= 8;
        }

        timeevent_add_delay(&timeevent, &datetime);

        return NEXT;
    }
    case SAT_TOGGLE_STATE: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            if ((obj_field_int32_get(handles[idx], OBJ_F_FLAGS) & OF_OFF) != 0) {
                mp_object_flags_unset(handles[idx], OF_OFF);
            } else {
                mp_object_flags_set(handles[idx], OF_OFF);
            }
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_TOGGLE_INVULNERABILITY: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            if ((obj_field_int32_get(handles[idx], OBJ_F_FLAGS) & OF_INVULNERABLE) != 0) {
                mp_object_flags_unset(handles[idx], OF_INVULNERABLE);
            } else {
                mp_object_flags_set(handles[idx], OF_INVULNERABLE);
            }
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_KILL: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            critter_kill(handles[idx]);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_CHANGE_ART_NUM: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int num = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            tig_art_id_t art_id = obj_field_int32_get(handles[idx], OBJ_F_CURRENT_AID);
            art_id = tig_art_num_set(art_id, num);
            art_id = tig_art_id_frame_set(art_id, 0);
            if (tig_art_exists(art_id) == TIG_OK) {
                mp_object_set_current_aid(handles[idx], art_id);
            }
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_DAMAGE: {

        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int damage = script_get_value(action->op_type[1], action->op_value[1], state);
        int type = script_get_value(action->op_type[2], action->op_value[2], state);
        for (int idx = 0; idx < cnt; idx++) {
            sub_4B2210(OBJ_HANDLE_NULL, handles[idx], &combat);
            combat.dam[type] = damage;
            combat_dmg(&combat);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_CAST_SPELL_ON: {
        int spell = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t target_obj = script_get_obj(action->op_type[1], action->op_value[1], state);

        MagicTechInvocation mt_invocation;
        magictech_invocation_init(&mt_invocation, OBJ_HANDLE_NULL, spell);
        sub_4440E0(target_obj, &(mt_invocation.target_obj));
        magictech_invocation_run(&mt_invocation);

        return NEXT;
    }
    case SAT_ACTION_PERFORM_ANIMATION: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int anim = script_get_value(action->op_type[1], action->op_value[1], state);
        anim_goal_animate(obj, anim);
        return NEXT;
    }
    case SAT_GIVE_QUEST_XP: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int quest_level = script_get_value(action->op_type[1], action->op_value[1], state);
        critter_give_xp(obj, quest_get_xp(quest_level));
        return NEXT;
    }
    case SAT_WRITTEN_UI_START_BOOK: {
        int num = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        mp_ui_written_start_type(obj, WRITTEN_TYPE_BOOK, num);
        return NEXT;
    }
    case SAT_WRITTEN_UI_START_IMAGE: {
        int num = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        mp_ui_written_start_type(obj, WRITTEN_TYPE_IMAGE, num);
        return NEXT;
    }
    case SAT_CREATE_ITEM: {
        int proto = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t parent_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int64_t loc = obj_field_int64_get(parent_obj, OBJ_F_LOCATION);
        int64_t new_obj;
        mp_object_create(proto, loc, &new_obj);
        if (obj_type_is_item(obj_field_int32_get(new_obj, OBJ_F_TYPE))) {
            item_transfer(new_obj, parent_obj);
        }
        return NEXT;
    }
    case SAT_ACTION_WAIT_FOR_LEADER: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        ai_npc_wait(obj);
        return NEXT;
    }
    case SAT_DESTROY: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        object_destroy(obj);
        return NEXT;
    }
    case SAT_ACTION_WALK_TO: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int x = script_get_value(action->op_type[1], action->op_value[1], state);
        int y = script_get_value(action->op_type[2], action->op_value[2], state);
        sub_4341C0(obj, location_make(x, y), AG_MOVE_TO_TILE);
        return NEXT;
    }
    case SAT_GET_WEAPON_TYPE: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t weapon_obj = item_wield_get(obj, ITEM_INV_LOC_WEAPON);
        int weapon_type = weapon_obj != OBJ_HANDLE_NULL
            ? tig_art_item_id_subtype_get(obj_field_int32_get(weapon_obj, OBJ_F_ITEM_INV_AID))
            : TIG_ART_WEAPON_TYPE_UNARMED;
        script_set_value(action->op_type[1], action->op_value[1], state, weapon_type);
        return NEXT;
    }
    case SAT_DISTANCE_BETWEEN: {
        int64_t a = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t b = script_get_obj(action->op_type[1], action->op_value[1], state);
        int dist = (int)object_dist(a, b);
        script_set_value(action->op_type[2], action->op_value[2], state, dist);
        return NEXT;
    }
    case SAT_ADD_REPUTATION: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int num = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            reputation_add(handles[idx], num);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_REMOVE_REPUTATION: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int num = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            reputation_remove(handles[idx], num);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_ACTION_RUN_TO: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int x = script_get_value(action->op_type[1], action->op_value[1], state);
        int y = script_get_value(action->op_type[2], action->op_value[2], state);
        sub_434400(obj, location_make(x, y), AG_MOVE_TO_TILE);
        return NEXT;
    }
    case SAT_HEAL_HP: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int value = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            sub_4B2210(OBJ_HANDLE_NULL, handles[idx], &combat);
            combat.dam[DAMAGE_TYPE_NORMAL] = value;
            combat_heal(&combat);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_HEAL_FATIGUE: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int value = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            int fatigue = critter_fatigue_current(handles[idx]);

            int fatigue_damage = critter_fatigue_damage_get(handles[idx]) - value;
            if (fatigue_damage < 0) {
                fatigue_damage = 0;
            }
            mp_critter_fatigue_damage_set(handles[idx], fatigue_damage);

            if (fatigue <= 0 && critter_fatigue_current(handles[idx]) > 0) {
                anim_goal_get_up(handles[idx]);
            }
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_ADD_EFFECT: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int effect = script_get_value(action->op_type[1], action->op_value[1], state);
        int cause = script_get_value(action->op_type[2], action->op_value[2], state);
        for (int idx = 0; idx < cnt; idx++) {
            effect_add(handles[idx], effect, cause);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_REMOVE_EFFECT: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int effect = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            effect_remove_one_typed(handles[idx], effect);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_ACTION_USE_ITEM: {
        int64_t source_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t item_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int64_t target_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        int skill = script_get_value(action->op_type[3], action->op_value[3], state);
        int modifier = script_get_value(action->op_type[4], action->op_value[4], state);
        anim_goal_use_item_on_obj_with_skill(source_obj, item_obj, target_obj, skill, modifier);
        return NEXT;
    }
    case SAT_GET_MAGICTECH_ADJUSTMENT: {
        int value = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t item_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int64_t source_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        int64_t target_obj = script_get_obj(action->op_type[3], action->op_value[3], state);
        int ratio = sub_461620(item_obj, source_obj, target_obj);
        script_set_value(action->op_type[4], action->op_value[4], state, value * (100 - ratio) / 100);
        return NEXT;
    }
    case SAT_CALL_SCRIPT_EX:
        sub_44B170(action, state);
        return NEXT;
    case SAT_PLAY_SOUND: {
        int sound_id = script_get_value(action->op_type[0], action->op_value[0], state);
        mp_gsound_play_sfx(sound_id);
        return NEXT;
    }
    case SAT_PLAY_SOUND_ON: {
        int sound_id = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        mp_gsound_play_sfx_on_obj(sound_id, 1, obj);
        return NEXT;
    }
    case SAT_GET_AREA: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int area = area_of_object(obj);
        script_set_value(action->op_type[1], action->op_value[1], state, area);
        return NEXT;
    }
    case SAT_QUEUE_NEWSPAPER: {
        int newspaper = script_get_value(action->op_type[0], action->op_value[0], state);
        int priority = script_get_value(action->op_type[1], action->op_value[1], state);
        newspaper_enqueue(newspaper, priority);
        return NEXT;
    }
    case SAT_FLOAT_NEWSPAPER_HEADLINE: {
        // FIXME: Probably wrong, takes obj from param 2 (at index 1), but this
        // opcode accepts just 1 param. Check.
        int64_t npc_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        char str[MAX_STRING];
        ui_written_newspaper_headline(newspaper_get(NEWSPAPER_CURRENT), str);
        size_t len = strlen(str);
        if (len > 0) {
            if (str[len] != '.' && str[len] != '?' && str[len] != '!') {
                str[len++] = '.';
            }
            str[len++] = ' ';
            str[len] = '\0';
        }
        int speech_id;
        dialog_copy_npc_newspaper_msg(npc_obj, player_get_local_pc_obj(), &(str[len]), &speech_id);
        script_float_line_func(npc_obj, state->invocation->triggerer_obj, str, speech_id);
        return NEXT;
    }
    case SAT_PLAY_SOUND_SCHEME: {
        int music_scheme_idx = script_get_value(action->op_type[0], action->op_value[0], state);
        int ambient_scheme_idx = script_get_value(action->op_type[1], action->op_value[1], state);
        mp_gsound_play_scheme(music_scheme_idx, ambient_scheme_idx);
        return NEXT;
    }
    case SAT_TOGGLE_OPEN_CLOSED: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        switch (obj_field_int32_get(obj, OBJ_F_TYPE)) {
        case OBJ_TYPE_PORTAL:
            mp_portal_toggle(obj);
            break;
        case OBJ_TYPE_CONTAINER:
            if (tig_art_id_frame_get(obj_field_int32_get(obj, OBJ_F_CURRENT_AID)) == 0) {
                mp_container_open(obj);
            } else {
                mp_container_close(obj);
            }
            break;
        }
        return NEXT;
    }
    case SAT_GET_FACTION: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int faction = critter_faction_get(obj);
        script_set_value(action->op_type[1], action->op_value[1], state, faction);
        return NEXT;
    }
    case SAT_GET_SCROLL_DISTANCE:
        script_set_value(action->op_type[0], action->op_value[0], state, scroll_distance_get());
        return NEXT;
    case SAT_GET_MAGICTECH_ADJUSTMENT_EX: {
        int value = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t item_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int64_t source_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        int effectiveness = item_effective_power_ratio(item_obj, source_obj);
        script_set_value(action->op_type[3], action->op_value[3], state, (value * effectiveness + 50) / 100);
        return NEXT;
    }
    case SAT_RENAME: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int name = script_get_value(action->op_type[1], action->op_value[1], state);
        mp_obj_field_int32_set(obj, OBJ_F_NAME, name);
        return NEXT;
    }
    case SAT_ACTION_BECOME_PRONE: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        for (int idx = 0; idx < cnt; idx++) {
            anim_goal_make_knockdown(handles[idx]);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_SET_WRITTEN_START: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int start = script_get_value(action->op_type[1], action->op_value[1], state);

        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_WRITTEN) {
            mp_obj_field_int32_set(obj, OBJ_F_WRITTEN_TEXT_START_LINE, start);
        }

        return NEXT;
    }
    case SAT_GET_LOCATION: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
        int x = (int)LOCATION_GET_X(loc);
        int y = (int)LOCATION_GET_Y(loc);
        script_set_value(action->op_type[1], action->op_value[1], state, x);
        script_set_value(action->op_type[2], action->op_value[2], state, y);
        return NEXT;
    }
    case SAT_GET_DAY_SINCE_STARTUP: {
        int days = datetime_get_day_since_reference_date();
        script_set_value(action->op_type[0], action->op_value[0], state, days);
        return NEXT;
    }
    case SAT_GET_CURRENT_HOUR: {
        DateTime datetime = sub_45A7C0();
        int hour = datetime_get_hour(&datetime);
        script_set_value(action->op_type[0], action->op_value[0], state, hour);
        return NEXT;
    }
    case SAT_GET_CURRENT_MINUTE: {
        DateTime datetime = sub_45A7C0();
        int minute = datetime_get_minute(&datetime);
        script_set_value(action->op_type[0], action->op_value[0], state, minute);
        return NEXT;
    }
    case SAT_CHANGE_SCRIPT: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int sap = script_get_value(action->op_type[1], action->op_value[1], state);
        int num = script_get_value(action->op_type[2], action->op_value[2], state);
        if (state->invocation->attachee_obj == obj
            && state->invocation->attachment_point == sap) {
            state->script_num = num;
        } else {
            Script scr;
            obj_arrayfield_script_get(obj, OBJ_F_SCRIPTS_IDX, sap, &scr);
            scr.num = num;
            mp_obj_arrayfield_script_set(obj, OBJ_F_SCRIPTS_IDX, sap, &scr);
        }
        return NEXT;
    }
    case SAT_SET_GLOBAL_FLAG: {
        int flag = script_get_value(action->op_type[0], action->op_value[0], state);
        script_global_flag_set(flag, 1);
        return NEXT;
    }
    case SAT_CLEAR_GLOBAL_FLAG: {
        int flag = script_get_value(action->op_type[0], action->op_value[0], state);
        script_global_flag_set(flag, 0);
        return NEXT;
    }
    case SAT_FADE_AND_TELEPORT: {
        if (!tig_net_is_active()) {
            TeleportData teleport_data;
            teleport_data.flags = TELEPORT_FADE_IN | TELEPORT_FADE_OUT;
            teleport_data.fade_out.flags = 0;
            teleport_data.fade_out.duration = 2.0f;
            teleport_data.fade_out.color = tig_color_make(0, 0, 0);
            teleport_data.fade_out.steps = 48;
            teleport_data.fade_in.flags = FADE_IN;
            teleport_data.fade_in.steps = 48;
            teleport_data.fade_in.duration = 2.0f;

            teleport_data.time = script_get_value(action->op_type[0], action->op_value[0], state);
            if (teleport_data.time > 0) {
                teleport_data.flags |= TELEPORT_TIME;
            }

            teleport_data.sound_id = script_get_value(action->op_type[1], action->op_value[1], state);
            if (teleport_data.sound_id > 0) {
                teleport_data.flags |= TELEPORT_SOUND;
            }

            teleport_data.movie1 = script_get_value(action->op_type[2], action->op_value[2], state);
            if (teleport_data.movie1 > 0) {
                teleport_data.flags |= TELEPORT_MOVIE1;
            }

            teleport_data.obj = script_get_obj(action->op_type[3], action->op_value[3], state);

            int map = script_get_value(action->op_type[4], action->op_value[4], state);
            int x = script_get_value(action->op_type[5], action->op_value[5], state);
            int y = script_get_value(action->op_type[6], action->op_value[6], state);

            teleport_data.map = map - 4999;
            teleport_data.loc = location_make(x, y);
            teleport_do(&teleport_data);
        }
        return NEXT;
    }
    case SAT_FADE: {
        if (!tig_net_is_active()) {
            TeleportData teleport_data;
            teleport_data.flags = 0;
            teleport_data.fade_out.flags = 0;
            teleport_data.fade_out.duration = 2.0f;
            teleport_data.fade_out.color = tig_color_make(0, 0, 0);
            teleport_data.fade_out.steps = 48;
            gfade_run(&(teleport_data.fade_out));
            tb_clear();

            teleport_data.time = script_get_value(action->op_type[0], action->op_value[0], state);
            teleport_data.sound_id = script_get_value(action->op_type[1], action->op_value[1], state);
            teleport_data.movie1 = script_get_value(action->op_type[2], action->op_value[2], state);

            if (teleport_data.time > 0) {
                DateTime datetime;
                sub_45A950(&datetime, 1000 * teleport_data.time);
                timeevent_inc_datetime(&datetime);
            }

            if (teleport_data.sound_id > 0) {
                teleport_data.flags |= TELEPORT_SOUND;
            }

            if (teleport_data.movie1 > 0) {
                if (teleport_data.sound_id > 0) {
                    gmovie_play(teleport_data.movie1, 0, teleport_data.sound_id);
                } else {
                    gmovie_play(teleport_data.movie1, 0, 0);
                }
            } else {
                if (teleport_data.sound_id > 0) {
                    gsound_play_sfx(teleport_data.sound_id, 1);
                }
            }

            int delay = script_get_value(action->op_type[3], action->op_value[3], state);
            if (delay != 0) {
                TimeEvent timeevent;
                DateTime datetime;

                sub_4BBC10();

                timeevent.type = TIMEEVENT_TYPE_FADE;
                timeevent.params[0].integer_value = 1;
                timeevent.params[1].integer_value = tig_color_make(0, 0, 0);
                timeevent.params[2].float_value = 2.0f;
                timeevent.params[3].integer_value = 48;

                sub_45A950(&datetime, 1000 * delay);
                timeevent_add_delay(&timeevent, &datetime);
            } else {
                teleport_data.fade_in.flags = FADE_IN;
                teleport_data.fade_in.duration = 2.0f;
                teleport_data.fade_in.steps = 48;
                script_iso_invalidate_rect(NULL);
                script_iso_window_draw();
                tig_window_display();
                gfade_run(&(teleport_data.fade_in));
            }
        }
        return NEXT;
    }
    case SAT_PLAY_SPELL_EYE_CANDY: {
        int fx = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        magictech_fx_add(obj, fx);
        return NEXT;
    }
    case SAT_GET_HOURS_SINCE_STARTUP: {
        DateTime datetime = sub_45A7C0();
        int hours = datetime_get_hour_since_reference_date(&datetime);
        script_set_value(action->op_type[0], action->op_value[0], state, hours);
    }
    case SAT_TOGGLE_SECTOR_BLOCKED: {
        int x = script_get_value(action->op_type[0], action->op_value[0], state);
        int y = script_get_value(action->op_type[1], action->op_value[1], state);
        int64_t loc = location_make(x, y);
        int64_t sector_id = sector_id_from_loc(loc);
        if (sector_is_blocked(sector_id)) {
            mp_sector_block_set(sector_id, false);
        } else {
            mp_sector_block_set(sector_id, true);
        }
        return NEXT;
    }
    case SAT_GET_HIT_POINTS: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        script_set_value(action->op_type[1], action->op_value[1], state, object_hp_current(obj));
        script_set_value(action->op_type[2], action->op_value[2], state, object_hp_max(obj));
        return NEXT;
    }
    case SAT_GET_FATIGUE_POINTS: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        script_set_value(action->op_type[1], action->op_value[1], state, critter_fatigue_current(obj));
        script_set_value(action->op_type[2], action->op_value[2], state, critter_fatigue_max(obj));
        return NEXT;
    }
    case SAT_ACTION_STOP_ATTACKING: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        ai_stop_attacking(obj);
        return NEXT;
    }
    case SAT_TOGGLE_MONSTER_GENERATOR: {
        int gen = script_get_value(action->op_type[0], action->op_value[0], state);
        if (monstergen_is_disabled(gen)) {
            monstergen_enable(gen);
        } else {
            monstergen_disable(gen);
        }
        return NEXT;
    }
    case SAT_GET_ARMOR_COVERAGE: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int coverage = item_armor_coverage(obj);
        script_set_value(action->op_type[1], action->op_value[1], state, coverage);
        return NEXT;
    }
    case SAT_GIVE_SPELL_MASTERY_IN_COLLEGE: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int college = script_get_value(action->op_type[1], action->op_value[1], state);
        mp_spell_mastery_set(obj, college);
        return NEXT;
    }
    case SAT_UNFOG_TOWNMAP: {
        int map = script_get_value(action->op_type[0], action->op_value[0], state);
        mp_townmap_set_known(map, true);
        return NEXT;
    }
    case SAT_START_WRITTEN_UI: {
        int num = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        mp_ui_written_start_type(obj, WRITTEN_TYPE_PLAQUE, num);
        return NEXT;
    }
    case SAT_ACTION_TRY_TO_STEAL_100_COINS: {
        int64_t thief_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int64_t victim_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        if (obj_type_is_critter(obj_field_int32_get(victim_obj, OBJ_F_TYPE))) {
            int64_t gold_obj = obj_field_handle_get(victim_obj, OBJ_F_CRITTER_GOLD);
            if (gold_obj != OBJ_HANDLE_NULL && item_gold_get(gold_obj) >= 100) {
                anim_goal_use_skill_on(thief_obj, victim_obj, gold_obj, SKILL_PICK_POCKET, 0);
            }
        }
        return NEXT;
    }
    case SAT_STOP_SPELL_EYE_CANDY: {
        int fx = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        magictech_fx_remove(obj, fx);
        return NEXT;
    }
    case SAT_GRANT_ONE_FATE_POINT: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            int fate_points = stat_base_get(obj, STAT_FATE_POINTS);
            stat_base_set(obj, STAT_FATE_POINTS, fate_points + 1);
        }
        return NEXT;
    }
    case SAT_CAST_FREE_SPELL: {
        int64_t source_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int spell = script_get_value(action->op_type[1], action->op_value[1], state);
        int64_t target_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        MagicTechInvocation mt_invocation;
        magictech_invocation_init(&mt_invocation, source_obj, spell);
        sub_4440E0(target_obj, &(mt_invocation.target_obj));
        mt_invocation.flags |= MAGICTECH_INVOCATION_FREE;
        magictech_invocation_run(&mt_invocation);
        return NEXT;
    }
    case SAT_SET_PC_QUEST_UNBOTCHED: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int num = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            quest_unbotch(handles[idx], num);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_PLAY_SCRIPT_EYE_CANDY: {
        int num = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        script_fx_play(obj, num);
        return NEXT;
    }
    case SAT_ACTION_CAST_UNRESISTABLE_SPELL: {
        int64_t source_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int spell = script_get_value(action->op_type[1], action->op_value[1], state);
        int64_t target_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        MagicTechInvocation mt_invocation;
        magictech_invocation_init(&mt_invocation, source_obj, spell);
        sub_4440E0(target_obj, &(mt_invocation.target_obj));
        mt_invocation.flags |= MAGICTECH_INVOCATION_UNRESISTABLE;
        magictech_invocation_run(&mt_invocation);
        return NEXT;
    }
    case SAT_ACTION_CAST_FREE_UNRESISTABLE_SPELL: {
        int64_t source_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int spell = script_get_value(action->op_type[1], action->op_value[1], state);
        int64_t target_obj = script_get_obj(action->op_type[2], action->op_value[2], state);
        MagicTechInvocation mt_invocation;
        magictech_invocation_init(&mt_invocation, source_obj, spell);
        sub_4440E0(target_obj, &(mt_invocation.target_obj));
        mt_invocation.flags |= MAGICTECH_INVOCATION_FREE;
        mt_invocation.flags |= MAGICTECH_INVOCATION_UNRESISTABLE;
        magictech_invocation_run(&mt_invocation);
        return NEXT;
    }
    case SAT_TOUCH_ART: {
        tig_art_id_t art_id = script_get_value(action->op_type[0], action->op_value[0], state);
        mp_art_touch(art_id);
        return NEXT;
    }
    case SAT_STOP_SCRIPT_EYE_CANDY: {
        int num = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        script_fx_stop(obj, num);
        return NEXT;
    }
    case SAT_REMOVE_SCRIPT_CALL: {
        dword_5E2FE4 = script_get_value(action->op_type[0], action->op_value[0], state);
        qword_5E2FF8 = script_get_obj(action->op_type[1], action->op_value[1], state);
        timeevent_clear_one_ex(TIMEEVENT_TYPE_SCRIPT, sub_44AFF0);
        return NEXT;
    }
    case SAT_DESTROY_ITEM_NAMED: {
        int name = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int64_t item_obj = item_find_by_name(obj, name);
        if (item_obj != OBJ_HANDLE_NULL) {
            object_destroy(item_obj);
        }
        return NEXT;
    }
    case SAT_TOGGLE_ITEM_INVENTORY_DISPLAY: {
        int64_t item_obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        if (obj_type_is_item(obj_field_int32_get(item_obj, OBJ_F_TYPE))) {
            unsigned int flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
            if ((flags & OIF_NO_DISPLAY) != 0) {
                flags &= ~OIF_NO_DISPLAY;
            } else {
                flags |= OIF_NO_DISPLAY;
            }
            mp_obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, flags);
        }
        return NEXT;
    }
    case SAT_HEAL_POISON: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int value = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            int new_value = stat_base_get(handles[idx], STAT_POISON_LEVEL) - value;
            if (new_value < 0) {
                new_value = 0;
            }
            stat_base_set(handles[idx], STAT_POISON_LEVEL, new_value);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_START_SCHEMATIC_UI: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        if (obj != OBJ_HANDLE_NULL) {
            if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                sub_460B50(obj, obj);
            } else {
                int64_t leader_obj = critter_pc_leader_get(obj);
                if (leader_obj != OBJ_HANDLE_NULL) {
                    sub_460B50(obj, leader_obj);
                }
            }
        }
        return NEXT;
    }
    case SAT_STOP_SPELL: {
        int spell = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        if (obj != OBJ_HANDLE_NULL) {
            magictech_stop_spell(obj, spell);
        }
        return NEXT;
    }
    case SAT_QUEUE_SLIDE: {
        int slide = script_get_value(action->op_type[0], action->op_value[0], state);
        ui_queue_slide(slide);
        return NEXT;
    }
    case SAT_END_GAME_AND_PLAY_SLIDES: {
        ui_end_game();
        if (tig_net_is_active()
            && tig_net_is_host()) {
            PacketScriptFunc pkt;
            pkt.type = 124;
            pkt.subtype = SCRIPT_FUNC_END_GAME;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
        return NEXT;
    }
    case SAT_SET_ROTATION: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int rotation = script_get_value(action->op_type[1], action->op_value[1], state);
        tig_art_id_t art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        art_id = tig_art_id_rotation_set(art_id, rotation);
        sub_4EDCE0(obj, art_id);
        return NEXT;
    }
    case SAT_SET_FACTION: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int faction = script_get_value(action->op_type[1], action->op_value[1], state);
        critter_faction_set(obj, faction);
        return NEXT;
    }
    case SAT_DRAIN_CHARGES: {
        int qty = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        item_ammo_transfer(obj, OBJ_HANDLE_NULL, qty, TIG_ART_AMMO_TYPE_CHARGE, OBJ_HANDLE_NULL);
        return NEXT;
    }
    case SAT_CAST_UNRESISTABLE_SPELL: {
        int spell = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t target_obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        MagicTechInvocation mt_invocation;
        magictech_invocation_init(&mt_invocation, OBJ_HANDLE_NULL, spell);
        sub_4440E0(target_obj, &(mt_invocation.target_obj));
        mt_invocation.flags |= MAGICTECH_INVOCATION_UNRESISTABLE;
        magictech_invocation_run(&mt_invocation);
        return NEXT;
    }
    case SAT_ADJUST_STAT: {
        int stat = script_get_value(action->op_type[0], action->op_value[0], state);
        int64_t obj = script_get_obj(action->op_type[1], action->op_value[1], state);
        int value = script_get_value(action->op_type[2], action->op_value[2], state);
        if (obj != OBJ_HANDLE_NULL) {
            stat_base_set(obj, stat, stat_base_get(obj, stat) + value);
        }
        return NEXT;
    }
    case SAT_APPLY_UNRESISTABLE_DAMAGE: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int damage = script_get_value(action->op_type[1], action->op_value[1], state);
        int type = script_get_value(action->op_type[2], action->op_value[2], state);
        for (int idx = 0; idx < cnt; idx++) {
            sub_4B2210(OBJ_HANDLE_NULL, handles[idx], &combat);
            combat.dam[type] = damage;
            combat.dam_flags |= CDF_IGNORE_RESISTANCE;
            combat_dmg(&combat);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_SET_AUTOLEVEL_SCHEME: {
        int cnt = script_resolve_focus_obj(action->op_type[0], action->op_value[0], state, handles, &objects);
        int scheme = script_get_value(action->op_type[1], action->op_value[1], state);
        for (int idx = 0; idx < cnt; idx++) {
            auto_level_scheme_set(handles[idx], scheme);
        }
        sub_44B8F0(action->op_type[0], &objects);
        return NEXT;
    }
    case SAT_SET_DAY_STANDPOINT_EX: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int x = script_get_value(action->op_type[1], action->op_value[1], state);
        int y = script_get_value(action->op_type[2], action->op_value[2], state);
        int map = script_get_value(action->op_type[3], action->op_value[3], state) - 4999;
        int64_t loc = location_make(x, y);
        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            mp_obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_DAY, loc);
            mp_obj_field_int32_set(obj, OBJ_F_CRITTER_TELEPORT_MAP, map);
        }
        return NEXT;
    }
    case SAT_SET_NIGHT_STANDPOINT_EX: {
        int64_t obj = script_get_obj(action->op_type[0], action->op_value[0], state);
        int x = script_get_value(action->op_type[1], action->op_value[1], state);
        int y = script_get_value(action->op_type[2], action->op_value[2], state);
        int map = script_get_value(action->op_type[3], action->op_value[3], state) - 4999;
        int64_t loc = location_make(x, y);
        if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
            mp_obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_NIGHT, loc);
            mp_obj_field_int32_set(obj, OBJ_F_CRITTER_TELEPORT_MAP, map);
        }
        return NEXT;
    }
    default:
        return NEXT;
    }
}

// 0x44AFF0
bool sub_44AFF0(TimeEvent* timeevent)
{
    return timeevent->params[0].integer_value == dword_5E2FE4
        && timeevent->params[3].object_value == qword_5E2FF8;
}

// 0x44B030
void sub_44B030(ScriptAction* action, ScriptState* state)
{
    Script scr;
    ScriptInvocation invocation;
    ObjectList triggerers;
    ObjectList attachees;
    int64_t triggerer_objs[100];
    int64_t attachee_objs[100];
    int triggerers_cnt;
    int attachees_cnt;
    int triggerer_idx;
    int attachee_idx;

    scr.num = script_get_value(action->op_type[0], action->op_value[0], state);
    script_load_hdr(&scr);

    invocation.script = &scr;
    invocation.line = script_get_value(action->op_type[1], action->op_value[1], state);
    invocation.attachment_point = state->invocation->attachment_point;

    triggerers_cnt = script_resolve_focus_obj(action->op_type[2],
        action->op_value[2],
        state,
        triggerer_objs,
        &triggerers);
    attachees_cnt = script_resolve_focus_obj(action->op_type[3],
        action->op_value[3],
        state,
        attachee_objs,
        &attachees);

    for (triggerer_idx = 0; triggerer_idx < triggerers_cnt; triggerer_idx++) {
        for (attachee_idx = 0; attachee_idx < attachees_cnt; attachee_idx++) {
            invocation.extra_obj = OBJ_HANDLE_NULL;
            invocation.triggerer_obj = triggerer_objs[triggerer_idx];
            invocation.attachee_obj = attachee_objs[attachee_idx];
            script_execute(&invocation);
        }
    }

    sub_44B8F0(action->op_type[2], &triggerers);
    sub_44B8F0(action->op_type[3], &attachees);
}

// 0x44B170
void sub_44B170(ScriptAction* action, ScriptState* state)
{
    object_script_execute(script_get_obj(action->op_type[3], action->op_value[3], state),
        script_get_obj(action->op_type[0], action->op_value[0], state),
        OBJ_HANDLE_NULL,
        script_get_value(action->op_type[1], action->op_value[1], state),
        script_get_value(action->op_type[2], action->op_value[2], state));
}

// 0x44B1F0
void script_float_line(ScriptAction* action, ScriptState* state)
{
    char path[TIG_MAX_PATH];
    DialogState v1;
    int64_t objs[100];
    ObjectList objects;
    int cnt;
    int index;

    if (script_float_line_func == NULL) {
        return;
    }

    if (state->invocation->script->num == 0) {
        return;
    }

    if (!script_name_build_dlg_name(state->invocation->script->num, path)) {
        return;
    }

    if (!dialog_load(path, &(v1.dlg))) {
        return;
    }

    object_list_vicinity(state->invocation->triggerer_obj, OBJ_TM_PC, &objects);

    if (objects.head == NULL) {
        object_list_destroy(&objects);
        return;
    }

    v1.pc_obj = objects.head->obj;
    object_list_destroy(&objects);
    v1.num = script_get_value(action->op_type[0], action->op_value[0], state);
    v1.script_num = state->invocation->script->num;

    cnt = script_resolve_focus_obj(action->op_type[1], action->op_value[1], state, objs, &objects);
    for (index = 0; index < cnt; index++) {
        v1.npc_obj = objs[index];
        sub_413A30(&v1, true);
        script_float_line_func(objs[index], v1.pc_obj, v1.reply, v1.speech_id);
    }

    sub_44B8F0(action->op_type[1], &objects);
    dialog_unload(v1.dlg);
}

// 0x44B390
void script_print_line(ScriptAction* action, ScriptState* state)
{
    char path[TIG_MAX_PATH];
    DialogState v1;
    UiMessage ui_message;
    int player;

    if (state->invocation->script->num == 0) {
        return;
    }

    if (!script_name_build_dlg_name(state->invocation->script->num, path)) {
        return;
    }

    if (!dialog_load(path, &(v1.dlg))) {
        return;
    }

    if (state->invocation->attachee_obj == OBJ_HANDLE_NULL) {
        return;
    }

    v1.npc_obj = state->invocation->attachee_obj;
    v1.pc_obj = state->invocation->triggerer_obj;
    v1.num = script_get_value(action->op_type[0], action->op_value[0], state);
    v1.script_num = state->invocation->script->num;
    sub_413A30(&v1, true);

    ui_message.type = script_get_value(action->op_type[1], action->op_value[1], state);
    ui_message.str = v1.reply;
    ui_message.field_8 = 0;

    // TODO: Refactor.
    if (tig_net_is_active()) {
        player = multiplayer_find_slot_from_obj(state->invocation->triggerer_obj);
        if (player == -1) {
            dialog_unload(v1.dlg);
            return;
        }

        if (player != 0) {
            sub_4EDA60(&ui_message, player, 0);
            dialog_unload(v1.dlg);
            return;
        }
    } else {
        sub_460630(&ui_message);
        dialog_unload(v1.dlg);
    }
}

// 0x44B4E0
int script_resolve_focus_obj(ScriptFocusObject type, int index, ScriptState* state, int64_t* objs, ObjectList* l)
{
    ObjectNode* node;
    int cnt = 0;

    switch (type) {
    case SFO_TRIGGERER:
        objs[cnt++] = state->invocation->triggerer_obj;
        break;
    case SFO_ATTACHEE:
        objs[cnt++] = state->invocation->attachee_obj;
        break;
    case SFO_EVERY_FOLLOWER:
    case SFO_ANY_FOLLOWER:
        object_list_followers(state->invocation->triggerer_obj, l);

        node = l->head;
        while (node != NULL && cnt < 100) {
            objs[cnt++] = node->obj;
            node = node->next;
        }
        break;
    case SFO_EVERYONE_IN_PARTY:
    case SFO_ANYONE_IN_PARTY:
        object_list_party(state->invocation->triggerer_obj, l);

        node = l->head;
        while (node != NULL && cnt < 100) {
            objs[cnt++] = node->obj;
            node = node->next;
        }
        break;
    case SFO_EVERYONE_IN_TEAM:
    case SFO_ANYONE_IN_TEAM:
        object_list_team(state->invocation->triggerer_obj, l);

        node = l->head;
        while (node != NULL && cnt < 100) {
            objs[cnt++] = node->obj;
            node = node->next;
        }
        break;
    case SFO_EVERYONE_IN_VICINITY:
    case SFO_ANYONE_IN_VICINITY:
        if (state->invocation->attachee_obj != OBJ_HANDLE_NULL) {
            object_list_vicinity(state->invocation->attachee_obj, OBJ_TM_PC | OBJ_TM_NPC, l);

            node = l->head;
            while (node != NULL && cnt < 100) {
                objs[cnt++] = node->obj;
                node = node->next;
            }
        } else {
            if (l != NULL) {
                l->head = NULL;
                l->num_sectors = 0;
            }
        }
        break;
    case SFO_CURRENT_LOOPED_OBJECT:
        objs[cnt++] = state->current_loop_obj;
        break;
    case SFO_LOCAL_OBJECT:
        objs[cnt++] = state->lc_objs[index];
        break;
    case SFO_EXTRA_OBJECT:
        objs[cnt++] = state->invocation->extra_obj;
        break;
    case SFO_EVERYONE_IN_GROUP:
    case SFO_ANYONE_IN_GROUP:
        object_list_followers(state->invocation->triggerer_obj, l);

        node = l->head;
        while (node != NULL && cnt < 100) {
            objs[cnt++] = node->obj;
            node = node->next;
        }

        // FIXME: Possible overflow when cnt == 100.
        objs[cnt++] = state->invocation->triggerer_obj;
        break;
    case SFO_EVERY_SCENERY_IN_VICINITY:
    case SFO_ANY_SCENERY_IN_VICINITY:
        if (state->invocation->attachee_obj != OBJ_HANDLE_NULL) {
            object_list_vicinity(state->invocation->attachee_obj, OBJ_TM_SCENERY, l);

            node = l->head;
            while (node != NULL && cnt < 100) {
                objs[cnt++] = node->obj;
                node = node->next;
            }
        } else {
            if (l != NULL) {
                l->head = NULL;
                l->num_sectors = 0;
            }
        }
        break;
    case SFO_EVERY_CONTAINER_IN_VICINITY:
    case SFO_ANY_CONTAINER_IN_VICINITY:
        if (state->invocation->attachee_obj != OBJ_HANDLE_NULL) {
            object_list_vicinity(state->invocation->attachee_obj, OBJ_TM_CONTAINER, l);

            node = l->head;
            while (node != NULL && cnt < 100) {
                objs[cnt++] = node->obj;
                node = node->next;
            }
        } else {
            if (l != NULL) {
                l->head = NULL;
                l->num_sectors = 0;
            }
        }
        break;
    case SFO_EVERY_PORTAL_IN_VICINITY:
    case SFO_ANY_PORTAL_IN_VICINITY:
        if (state->invocation->attachee_obj != OBJ_HANDLE_NULL) {
            object_list_vicinity(state->invocation->attachee_obj, OBJ_TM_PORTAL, l);

            node = l->head;
            while (node != NULL && cnt < 100) {
                objs[cnt++] = node->obj;
                node = node->next;
            }
        } else {
            if (l != NULL) {
                l->head = NULL;
                l->num_sectors = 0;
            }
        }
        break;
    case SFO_PLAYER:
        objs[cnt++] = player_get_local_pc_obj();
        break;
    case SFO_EVERY_ITEM_IN_VICINITY:
    case SFO_ANY_ITEM_IN_VICINITY:
        if (state->invocation->attachee_obj != OBJ_HANDLE_NULL) {
            object_list_vicinity(state->invocation->attachee_obj, 0x7FE0, l);

            node = l->head;
            while (node != NULL && cnt < 100) {
                objs[cnt++] = node->obj;
                node = node->next;
            }
        } else {
            if (l != NULL) {
                l->head = NULL;
                l->num_sectors = 0;
            }
        }
        break;
    }

    return cnt;
}

// 0x44B8F0
void sub_44B8F0(ScriptFocusObject type, ObjectList* l)
{
    switch (type) {
    case SFO_EVERY_FOLLOWER:
    case SFO_ANY_FOLLOWER:
    case SFO_EVERYONE_IN_PARTY:
    case SFO_ANYONE_IN_PARTY:
    case SFO_EVERYONE_IN_TEAM:
    case SFO_ANYONE_IN_TEAM:
    case SFO_EVERYONE_IN_VICINITY:
    case SFO_ANYONE_IN_VICINITY:
    case SFO_EVERYONE_IN_GROUP:
    case SFO_ANYONE_IN_GROUP:
    case SFO_EVERY_SCENERY_IN_VICINITY:
    case SFO_ANY_SCENERY_IN_VICINITY:
    case SFO_EVERY_CONTAINER_IN_VICINITY:
    case SFO_ANY_CONTAINER_IN_VICINITY:
    case SFO_EVERY_PORTAL_IN_VICINITY:
    case SFO_ANY_PORTAL_IN_VICINITY:
    case SFO_EVERY_ITEM_IN_VICINITY:
    case SFO_ANY_ITEM_IN_VICINITY:
        if (l->head != NULL) {
            object_list_destroy(l);
        }
        break;
    }
}

// 0x44B960
int64_t script_get_obj(ScriptFocusObject type, int index, ScriptState* state)
{
    ObjectList l;
    int64_t objs[100];

    script_resolve_focus_obj(type, index, state, objs, &l);
    sub_44B8F0(type, &l);

    return objs[0];
}

// 0x44B9B0
void script_set_obj(ScriptFocusObject type, int index, ScriptState* state, int64_t obj)
{
    switch (type) {
    case SFO_TRIGGERER:
        state->invocation->triggerer_obj = obj;
        break;
    case SFO_ATTACHEE:
        state->invocation->attachee_obj = obj;
        break;
    case SFO_CURRENT_LOOPED_OBJECT:
        state->current_loop_obj = obj;
        break;
    case SFO_LOCAL_OBJECT:
        state->lc_objs[index] = obj;
        break;
    case SFO_EXTRA_OBJECT:
        state->invocation->extra_obj = obj;
        break;
    }
}

// 0x44BA70
int script_get_value(ScriptValueType type, int index, ScriptState* state)
{
    int64_t obj;

    switch (type) {
    case SVT_COUNTER:
        return (state->invocation->script->hdr.counters >> (8 * index)) & 0xFF;
    case SVT_GL_VAR:
        return script_global_var_get(index);
    case SVT_LC_VAR:
        return state->lc_vars[index];
    case SVT_NUMBER:
        return index;
    case SVT_GL_FLAG:
        return script_global_flag_get(index);
    case SVT_PC_VAR:
        obj = script_get_obj(index >> 16, 0, state);
        return script_pc_var_get(obj, index & 0xFFFF);
    case SVT_PC_FLAG:
        obj = script_get_obj(index >> 16, 0, state);
        return script_pc_flag_get(obj, index & 0xFFFF);
    }

    return index;
}

// 0x44BB50
void script_set_value(ScriptValueType type, int index, ScriptState* state, int value)
{
    int64_t obj;

    switch (type) {
    case SVT_COUNTER:
        state->invocation->script->hdr.counters &= ~(0xFF << (8 * index));
        state->invocation->script->hdr.counters |= value << (8 * index);
        break;
    case SVT_GL_VAR:
        script_global_var_set(index, value);
        break;
    case SVT_LC_VAR:
        state->lc_vars[index] = value;
        break;
    case SVT_GL_FLAG:
        script_global_flag_set(index, value);
        break;
    case SVT_PC_VAR:
        obj = script_get_obj(index >> 16, 0, state);
        script_pc_var_set(obj, index & 0xFFFF, value);
        break;
    case SVT_PC_FLAG:
        obj = script_get_obj(index >> 16, 0, state);
        script_pc_flag_set(obj, index & 0xFFFF, value);
        break;
    }
}

// 0x44BC60
int sub_44BC60(ScriptState* state)
{
    unsigned int index;
    ScriptCondition condition;

    index = state->field_C;
    while (sub_44C140(state->invocation->script, index, &condition)) {
        if (condition.type == 0 && condition.action.type == 19) {
            break;
        }
        index++;
    }

    return index + 1;
}

// 0x44BCC0
bool script_load_hdr(Script* scr)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    bool rc;

    if (!script_name_build_scr_name(scr->num, path)) {
        return false;
    }

    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        return false;
    }

    rc = script_file_load_hdr(stream, &(scr->hdr));
    tig_file_fclose(stream);

    return rc;
}

// 0x44C140
bool sub_44C140(Script* scr, unsigned int index, ScriptCondition* entry)
{
    ScriptFile* script_file;
    bool success;

    script_file = script_lock(scr->num);
    if (script_file != NULL) {
        success = sub_44C1B0(script_file, index, entry);
        script_unlock(scr->num);
        return success;
    }

    return false;
}

// 0x44C1B0
bool sub_44C1B0(ScriptFile* script_file, unsigned int index, ScriptCondition* entry)
{
    // NOTE: Unsigned math.
    if (index < (unsigned int)script_file->num_entries) {
        *entry = script_file->entries[index];
        return true;
    }

    return false;
}

// 0x44C310
bool script_flags(Script* scr, ScriptFlags* flags_ptr)
{
    ScriptFile* script_file;

    script_file = script_lock(scr->num);
    if (script_file != NULL) {
        *flags_ptr = script_file->flags;
        script_unlock(scr->num);
        return true;
    }

    return false;
}

// 0x44C390
ScriptFile* script_lock(int script_id)
{
    int cache_entry_id;

    if (script_editor) {
        // FIX: Original code returns `script_id` which is obviously wrong.
        return NULL;
    }

    cache_entry_id = cache_find(script_id);
    if (script_cache_entries[cache_entry_id].script_id == script_id) {
        script_cache_entries[cache_entry_id].ref_count++;
        script_cache_entries[cache_entry_id].datetime = sub_45A7C0();
        return script_cache_entries[cache_entry_id].file;
    }

    if (script_cache_entries[cache_entry_id].script_id != 0) {
        cache_remove(cache_entry_id);
    }

    if (cache_add(cache_entry_id, script_id)) {
        script_cache_entries[cache_entry_id].ref_count++;
        script_cache_entries[cache_entry_id].datetime = sub_45A7C0();
        return script_cache_entries[cache_entry_id].file;
    }

    return NULL;
}

// 0x44C450
void script_unlock(int script_id)
{
    int cache_entry_id;

    if (!script_editor) {
        return;
    }

    cache_entry_id = cache_find(script_id);
    script_cache_entries[cache_entry_id].ref_count--;
}

// 0x44C480
bool script_file_create(ScriptFile** script_file_ptr)
{
    ScriptFile* script_file;

    script_file = (ScriptFile*)MALLOC(sizeof(ScriptFile));
    script_file->description[0] = '\0';
    script_file->flags = 0;
    script_file->num_entries = 0;
    script_file->max_entries = 0;
    script_file->entries = NULL;
    *script_file_ptr = script_file;

    return true;
}

// 0x44C4B0
bool script_file_destroy(ScriptFile* script_file)
{
    if (script_file->max_entries > 0) {
        FREE(script_file->entries);
    }

    FREE(script_file);

    return true;
}

// 0x44C4E0
bool cache_add(int cache_entry_id, int script_id)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    ScriptHeader hdr;

    if (!script_name_build_scr_name(script_id, path)) {
        tig_debug_printf("Script: cache_add: ERROR: Failed to build script name: %d!\n", script_id);
        return false;
    }

    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        tig_debug_printf("Script: cache_add: ERROR: Failed to load script file: %d!\n", script_id);
        return false;
    }

    if (!script_file_load_hdr(stream, &hdr)) {
        tig_debug_printf("Script: cache_add: ERROR: Failed to load script variables: %d!\n", script_id);
        // FIXME: Leaking stream.
        return false;
    }

    if (!script_file_create(&(script_cache_entries[cache_entry_id].file))) {
        tig_debug_printf("Script: cache_add: ERROR: Failed to build script code: %d!\n", script_id);
        // FIXME: Leaking stream.
        return false;
    }

    if (!script_file_load_code(stream, script_cache_entries[cache_entry_id].file)) {
        tig_debug_printf("Script: cache_add: ERROR: Failed to load script code: %d!\n", script_id);
        // FIXME: Leaking stream.
        return false;
    }

    tig_file_fclose(stream);

    script_cache_entries[cache_entry_id].script_id = script_id;
    script_cache_entries[cache_entry_id].ref_count = 0;

    return true;
}

// 0x44C630
void cache_remove(int cache_entry_id)
{
    if (script_cache_entries[cache_entry_id].script_id) {
        script_file_destroy(script_cache_entries[cache_entry_id].file);
        script_cache_entries[cache_entry_id].script_id = 0;
    }
}

// 0x44C670
int cache_find(int script_id)
{
    int idx;
    int candidate = -1;
    int best_candidate = -1;

    for (idx = 0; idx < MAX_CACHE_ENTRIES; idx++) {
        if (script_cache_entries[idx].script_id == script_id) {
            return idx;
        }

        if (script_cache_entries[idx].script_id != 0) {
            if (script_cache_entries[idx].ref_count == 0) {
                if (candidate == -1) {
                    candidate = idx;
                } else {
                    if (datetime_compare(&(script_cache_entries[candidate].datetime), &(script_cache_entries[idx].datetime)) > 0) {
                        candidate = idx;
                    }
                }
            }
        } else {
            if (best_candidate == -1) {
                best_candidate = idx;
            }
        }
    }

    if (best_candidate != -1) {
        return best_candidate;
    }

    if (candidate != -1) {
        return candidate;
    }

    tig_debug_printf("cache_find: Error: failed to find cache item!\n");
    exit(EXIT_SUCCESS); // FIXME: Should be `EXIT_FAILURE`.
}

// 0x44C710
bool script_file_load_hdr(TigFile* stream, ScriptHeader* hdr)
{
    if (tig_file_fread(hdr, sizeof(*hdr), 1, stream) != 1) return false;

    return true;
}

// 0x44C730
bool script_file_load_code(TigFile* stream, ScriptFile* script_file)
{
    // CE: Read data field-by-field fixing `ScriptFile::entries` alignment/size
    // discrepancy on x64.
    if (tig_file_fread(script_file->description, sizeof(script_file->description), 1, stream) != 1
        || tig_file_fread(&(script_file->flags), sizeof(script_file->flags), 1, stream) != 1
        || tig_file_fread(&(script_file->num_entries), sizeof(script_file->num_entries), 1, stream) != 1
        || tig_file_fread(&(script_file->max_entries), sizeof(script_file->max_entries), 1, stream) != 1
        || tig_file_fseek(stream, 4, SEEK_CUR) != 0) {
        return false;
    }

    if (script_file->num_entries > 0) {
        script_file->entries = (ScriptCondition*)CALLOC(script_file->max_entries, sizeof(ScriptCondition));
        if (tig_file_fread(script_file->entries, sizeof(ScriptCondition), script_file->num_entries, stream) != script_file->num_entries) {
            // FIXME: Leaks entries.
            return false;
        }
    } else {
        script_file->entries = NULL;
    }

    return true;
}

// 0x44C7A0
void script_fx_play(int64_t obj, int fx_id)
{
    AnimFxNode fx;

    sub_4CCD20(&script_eye_candies, &fx, obj, -1, fx_id);
    fx.rotation = tig_art_id_rotation_get(obj_field_int32_get(obj, OBJ_F_CURRENT_AID));
    fx.animate = true;
    animfx_add(&fx);
}

// 0x44C800
void script_fx_stop(int64_t obj, int fx_id)
{
    animfx_remove(&script_eye_candies, obj, fx_id, -1);
}

// 0x44C820
void script_play_explosion_fx(int64_t obj)
{
    script_fx_play(obj, 55);
}
