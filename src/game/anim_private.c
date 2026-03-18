#include "game/anim_private.h"

#include "game/anim.h"
#include "game/critter.h"
#include "game/gamelib.h"
#include "game/item.h"
#include "game/map.h"
#include "game/mp_utils.h"
#include "game/obj_private.h"
#include "game/player.h"
#include "game/random.h"
#include "game/timeevent.h"
#include "game/ui.h"

static bool anim_allocate_this_run_index(AnimID* anim_id);
static bool sub_44D240(int index);
static bool anim_goal_add_func(AnimGoalData* goal_data, AnimID* anim_id, bool a3, unsigned int flags);
static bool anim_subgoal_add_func(AnimID anim_id, AnimGoalData* goal_data);
static bool sub_44E2A0(TimeEvent* timeevent);
static void anim_path_debug(AnimPath* path);
static void anim_goal_data_debug(AnimGoalData* goal_data);
static void anim_run_info_debug(AnimRunInfo* run_info);

// 0x5A164C
const char* off_5A164C[] = {
    "ag_animate",
    "ag_animate_loop",
    "ag_anim_fidget",
    "ag_move_to_tile",
    "ag_run_to_tile",
    "ag_attempt_move",
    "ag_move_to_pause",
    "ag_move_near_tile",
    "ag_move_near_obj",
    "ag_move_straight",
    "ag_attempt_move_straight",
    "ag_open_door",
    "ag_attempt_open_door",
    "ag_unlock_door",
    "ag_jump_window",
    "ag_pickup_item",
    "ag_attempt_pickup",
    "ag_pickpocket",
    "ag_attack",
    "ag_attempt_attack",
    "ag_kill",
    "ag_talk",
    "ag_pick_weapon",
    "ag_chase",
    "ag_follow",
    "ag_flee",
    "ag_throw_spell",
    "ag_attempt_spell",
    "ag_shoot_spell",
    "ag_hit_by_spell",
    "ag_hit_by_weapon",
    "ag_dying",
    "ag_destroy_obj",
    "ag_use_skill_on",
    "ag_attempt_use_skill_on",
    "ag_skill_conceal",
    "ag_projectile",
    "ag_throw_item",
    "ag_use_object",
    "ag_use_item_on_object",
    "ag_use_item_on_object_with_skill",
    "ag_use_item_on_tile",
    "ag_use_item_on_tile_with_skill",
    "ag_knockback",
    "ag_floating",
    "ag_eye_candy",
    "ag_eye_candy_reverse",
    "ag_eye_candy_callback",
    "ag_eye_candy_reverse_callback",
    "ag_close_door",
    "ag_attempt_close_door",
    "ag_animate_reverse",
    "ag_move_away_from_obj",
    "ag_rotate",
    "ag_unconceal",
    "ag_run_near_tile",
    "ag_run_near_obj",
    "ag_animate_stunned",
    "ag_eye_candy_end_callback",
    "ag_eye_candy_reverse_end_callback",
    "ag_animate_kneel_magic_hands",
    "ag_attempt_move_near",
    "ag_knock_down",
    "ag_anim_get_up",
    "ag_attempt_move_straight_knockback",
    "ag_wander",
    "ag_wander_seek_darkness",
    "ag_use_picklock_skill_on",
    "ag_please_move",
    "ag_attempt_spread_out",
    "ag_animate_door_open",
    "ag_animate_door_closed",
    "ag_pend_closing_door",
    "ag_throw_spell_friendly",
    "ag_attempt_spell_friendly",
    "ag_eye_candy_fire_dmg",
    "ag_eye_candy_reverse_fire_dmg",
    "ag_animate_loop_fire_dmg",
    "ag_attempt_move_straight_spell",
    "ag_move_near_obj_combat",
    "ag_attempt_move_near_combat",
    "ag_use_container",
    "ag_throw_spell_w_cast_anim",
    "ag_attempt_spell_w_cast_anim",
    "ag_throw_spell_w_cast_anim_2ndary",
    "ag_back_off_from",
    "ag_attempt_use_pickpocket_skill_on",
};

// 0x5B0530
static const char* off_5B0530[] = {
    "AGDATA_SELF_OBJ",
    "AGDATA_TARGET_OBJ",
    "AGDATA_BLOCK_OBJ",
    "AGDATA_SCRATCH_OBJ",
    "AGDATA_PARENT_OBJ",
    "AGDATA_TARGET_TILE",
    "AGDATA_ORIGINAL_TILE",
    "AGDATA_RANGE_DATA",
    "AGDATA_ANIM_ID",
    "AGDATA_ANIM_ID_PREVIOUS",
    "AGDATA_ANIM_DATA",
    "AGDATA_SPELL_DATA",
    "AGDATA_SKILL_DATA",
    "AGDATA_FLAGS_DATA",
    "AGDATA_SCRATCH_VAL1",
    "AGDATA_SCRATCH_VAL2",
    "AGDATA_SCRATCH_VAL3",
    "AGDATA_SCRATCH_VAL4",
    "AGDATA_SCRATCH_VAL5",
    "AGDATA_SCRATCH_VAL6",
    "AGDATA_SOUND_HANDLE",
};

// 0x5A5978
int dword_5A5978 = -1;

// 0x5A597C
int dword_5A597C[AGDATA_COUNT] = {
    AGDATATYPE_OBJ,
    AGDATATYPE_OBJ,
    AGDATATYPE_OBJ,
    AGDATATYPE_OBJ,
    AGDATATYPE_OBJ,
    AGDATATYPE_LOC,
    AGDATATYPE_LOC,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_INT,
    AGDATATYPE_SOUND,
};

// 0x5B052C
static int dword_5B052C = -1;

// 0x5E3000
static AnimID stru_5E3000;

// 0x5E33F8
static AnimPath stru_5E33F8;

// 0x5E34F4
bool dword_5E34F4;

// 0x5E34F8
void (*dword_5E34F8)(void);

// 0x5E34FC
bool in_anim_load;

// 0x5E3500
int dword_5E3500;

// 0x5E3504
int animNumActiveGoals;

// 0x5E3508
static bool anim_private_editor;

// 0x6876E4
int dword_6876E4;

// 0x687700
AnimRunInfo anim_run_info[216];

// 0x739E40
int dword_739E40;

// 0x739E44
int dword_739E44;

// 0x44C840
void sub_44C840(AnimRunInfo* run_info, AnimGoalNode* goal_node)
{
    ASSERT(run_info != NULL); // pRunInfo != NULL
    ASSERT(goal_node != NULL); // pGoalNode != NULL

    if (goal_node->priority_level >= 2 && !goal_node->field_8 && !sub_44C9A0(run_info)) {
        ASSERT(animNumActiveGoals >= 0); // animNumActiveGoals >= 0

        animNumActiveGoals++;
    }
}

// 0x44C8F0
void sub_44C8F0(AnimRunInfo* run_info, AnimGoalNode* goal_node)
{
    ASSERT(run_info != NULL); // pRunInfo != NULL
    ASSERT(goal_node != NULL); // pGoalNode != NULL

    if (goal_node->priority_level >= 2 && !goal_node->field_8 && !sub_44C9A0(run_info)) {
        ASSERT(animNumActiveGoals >= 1); // animNumActiveGoals >= 1

        if (animNumActiveGoals >= 1) {
            animNumActiveGoals--;
        }
    }
}

// 0x44C9A0
bool sub_44C9A0(AnimRunInfo* run_info)
{
    ASSERT(run_info != NULL); // pRunInfo != NULL

    if ((run_info->flags & 0x1) == 0) {
        return false;
    }

    ASSERT(run_info->cur_stack_data); // pRunInfo->pCurStackData != NULL

    if (run_info->cur_stack_data == NULL) {
        ASSERT(run_info->current_goal >= 0); // pRunInfo->current_goal >= 0

        if (run_info->current_goal < 0) {
            return false;
        }

        run_info->cur_stack_data = &(run_info->goals[run_info->current_goal]);
    }

    return (anim_goal_nodes[run_info->cur_stack_data->type]->field_10 & 0x1) != 0
        || (run_info->cur_stack_data->params[AGDATA_FLAGS_DATA].data & 0x80) != 0;
}

// 0x44CA70
bool anim_private_init(GameInitInfo* init_info)
{
    int index;

    anim_private_editor = init_info->editor;

    for (index = 0; index < 216; index++) {
        anim_run_info[index].id.slot_num = index;
        anim_run_info[index].flags = 0;
        anim_run_info[index].path.flags = 1;
        anim_run_info[index].path.field_CC = 200;
        sub_44EBD0(&(anim_run_info[index].path));
    }

    dword_6876E4 = random_between(0, 10024);
    animNumActiveGoals = 0;
    dword_5E3500 = 0;

    return true;
}

// 0x44CAF0
void anim_private_exit(void)
{
    int index;

    for (index = 0; index < 216; index++) {
        anim_run_info[index].flags = 0;
        anim_run_info[index].path.flags = 1;
        sub_44EBE0(&(anim_run_info[index].path));
    }

    animNumActiveGoals = 0;
}

// 0x44CB30
void anim_private_reset(void)
{
    int index;

    for (index = 0; index < 216; index++) {
        anim_run_info[index].flags = 0;
        anim_run_info[index].path.flags = 1;
    }

    animNumActiveGoals = 0;
    dword_5E3500 = 0;
}

// 0x44CB60
bool sub_44CB60(void)
{
    return dword_5A5978 != -1;
}

// 0x44CB70
bool sub_44CB70(TimeEvent* timeevent)
{
    ASSERT(timeevent != NULL); // 2766, "pTimeEvent"

    return timeevent->params[0].integer_value == stru_5E3000.slot_num;
}

// 0x44CBB0
bool anim_goal_restart(AnimID* anim_id)
{
    AnimRunInfo* run_info;
    char str[ANIM_ID_STR_SIZE];
    TimeEvent timeevent;

    ASSERT(anim_id != NULL); // pAnimID != NULL

    if (!anim_id_to_run_info(anim_id, &run_info)) {
        anim_id_to_str(anim_id, str);
        tig_debug_printf("Anim: anim_goal_restart: could not turn animID into a AnimRunInfo %s.\n");
        return false;
    }

    if ((run_info->flags & 0x1) == 0) {
        return false;
    }

    run_info->flags &= ~0x2;
    if (run_info->current_goal == -1) {
        run_info->current_goal = 0;
        run_info->current_state = 0;
        run_info->path_attached_to_stack_index = -1;
    }

    stru_5E3000 = run_info->id;
    timeevent_clear_one_ex(TIMEEVENT_TYPE_ANIM, sub_44CB70);

    timeevent.type = TIMEEVENT_TYPE_ANIM;
    timeevent.params[0].integer_value = run_info->id.slot_num;
    timeevent.params[1].integer_value = run_info->id.field_4;
    timeevent.params[2].integer_value = 2222;
    return timeevent_add_base(&timeevent, &(run_info->next_ping_time));
}

// 0x44CCB0
bool sub_44CCB0(AnimID* anim_id)
{
    int index;
    AnimRunInfo* run_info;
    int subindex;

    ASSERT(anim_id != NULL); // pAnimID != NULL

    for (index = 0; index < 216; index++) {
        if ((anim_run_info[index].flags & 0x1) == 0) {
            break;
        }
    }

    if (index == 216) {
        tig_debug_printf("Anim: WARNING: Ran out of animation slots!\n");
        dword_5E34F4 = 1;
        return false;
    }

    run_info = &(anim_run_info[index]);
    run_info->id.slot_num = index;
    run_info->id.field_4 = dword_6876E4++;
    run_info->id.field_8 = 0;
    run_info->flags = 1;
    run_info->path.maxPathLength = 0;
    *anim_id = run_info->id;

    run_info->anim_obj = 0;
    run_info->cur_stack_data = NULL;
    run_info->next_ping_time.days = 0;
    run_info->next_ping_time.milliseconds = 0;
    run_info->goals[0].params[AGDATA_SELF_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_TARGET_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_BLOCK_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_SCRATCH_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_PARENT_OBJ].obj = OBJ_HANDLE_NULL;

    for (subindex = 0; subindex < 5; subindex++) {
        run_info->goals[0].field_B0[subindex].objid.type = OID_TYPE_NULL;
    }

    dword_5E3500++;

    return true;
}

// 0x44CDA0
bool anim_allocate_this_run_index(AnimID* anim_id)
{
    int slot;
    AnimRunInfo* run_info;
    int idx;

    ASSERT(anim_id->slot_num != -1); // 2886, "!anim_id_none(pAnimID)"

    if (anim_id->slot_num == -1) {
        tig_debug_printf("Anim: WARNING: Animation slots Force Alloc FAILED!\n");
        return false;
    }

    for (slot = 0; slot < 216; slot++) {
        run_info = &(anim_run_info[slot]);
        if (run_info->id.field_4 == anim_id->field_4) {
            if ((run_info->flags & 0x1) != 0
                && !sub_44E2C0(&(run_info->id), PRIORITY_HIGHEST)) {
                tig_debug_printf("Anim: WARNING(uniqueID): Animation slots Force Alloc INTERRUPT FAILED!\n");
                return false;
            }

            anim_id->field_4 = slot;
            anim_id->field_8 = 0;
            break;
        }
    }

    if (slot == 216) {
        if (anim_id->slot_num >= 0
            && anim_id->slot_num < 216
            && (anim_run_info[anim_id->slot_num].flags & 0x1) != 0) {
            for (slot = 0; slot < 216; slot++) {
                run_info = &(anim_run_info[slot]);
                if ((run_info->flags & 0x1) == 0) {
                    anim_id->slot_num = slot;
                    anim_id->field_8 = 0;
                    break;
                }
            }

            if (slot == 216) {
                tig_debug_printf("Anim: anim_allocate_this_run_index: could not allocate a run index, ALL FULL!.\n");
                return false;
            }
        }
    }

    run_info = &(anim_run_info[slot]);
    run_info->id = *anim_id;
    run_info->flags = 0x1;
    run_info->path.maxPathLength = 0;
    run_info->cur_stack_data = NULL;
    run_info->current_goal = -1;
    run_info->goals[0].params[AGDATA_SELF_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_TARGET_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_BLOCK_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_SCRATCH_OBJ].obj = OBJ_HANDLE_NULL;
    run_info->goals[0].params[AGDATA_PARENT_OBJ].obj = OBJ_HANDLE_NULL;

    for (idx = 0; idx < 5; idx++) {
        run_info->goals[0].field_B0[idx].objid.type = OID_TYPE_NULL;
    }

    dword_5E3500++;

    return true;
}

// 0x44CF20
bool mp_deallocate_run_index(AnimID* anim_id)
{
    AnimRunInfo* run_info;
    char str[ANIM_ID_STR_SIZE];
    int stack_index;

    ASSERT(anim_id != NULL); // pAnimID != NULL

    if (!anim_id_to_run_info(anim_id, &run_info)) {
        anim_id_to_str(anim_id, str);
        tig_debug_printf("Anim: mp_deallocate_run_index: could not turn animID into a AnimRunInfo %s.\n", str);
        return false;
    }

    if ((run_info->flags & 0x1) != 0) {
        if (run_info->goals[0].type == AG_ATTACK
            || run_info->goals[0].type == AG_ATTEMPT_ATTACK) {
            if (player_is_local_pc_obj(run_info->anim_obj)) {
                sub_460280(OBJ_HANDLE_NULL);
            }
        }

        for (stack_index = 0; stack_index <= run_info->current_goal; stack_index++) {
            if (run_info->goals[stack_index].type >= 0 && run_info->goals[stack_index].type < ANIM_GOAL_MAX) {
                sub_44C8F0(run_info, anim_goal_nodes[run_info->goals[stack_index].type]);
            }
        }

        anim_id_init(&(run_info->id));
        run_info->cur_stack_data = NULL;
        run_info->anim_obj = OBJ_HANDLE_NULL;
        run_info->flags = 0;
        run_info->current_goal = -1;
        run_info->path.flags = 1;

        dword_5E3500--;
        sub_423E60("Free Run Index");

        if (animNumActiveGoals == 0) {
            if (dword_5E34F8 != NULL) {
                dword_5E34F8();
            }
        }
    } else {
        anim_id_init(&(run_info->id));
        run_info->cur_stack_data = NULL;
        run_info->anim_obj = 0;
        run_info->flags = 0;
        run_info->current_goal = -1;
        run_info->path.flags = 1;
    }

    return true;
}

// 0x44D0C0
void sub_44D0C0(AnimRunInfo* run_info)
{
    (void)run_info;
}

// 0x44D0D0
void sub_44D0D0(AnimID* anim_id, int a2)
{
    (void)anim_id;
    (void)a2;
}

// 0x44D0E0
bool anim_mp_reap_run_index(AnimID* anim_id)
{
    AnimRunInfo* run_info;
    char str[ANIM_ID_STR_SIZE];

    ASSERT(tig_net_is_active()); // tig_net_is_active()
    ASSERT(anim_id != NULL); // pAnimID != NULL

    if (!anim_id_to_run_info(anim_id, &run_info)) {
        anim_id_to_str(anim_id, str);
        tig_debug_printf("Anim: anim_mp_reap_run_index: could not turn animID into a AnimRunInfo %s.\n", str);
        return false;
    }

    mp_deallocate_run_index(anim_id);

    return true;
}

// 0x44D180
bool anim_free_run_index(AnimID* anim_id)
{
    AnimRunInfo* run_info;
    char str[ANIM_ID_STR_SIZE];

    ASSERT(anim_id != NULL); // pAnimID != NULL

    if (!anim_id_to_run_info(anim_id, &run_info)) {
        anim_id_to_str(anim_id, str);
        tig_debug_printf("Anim: anim_free_run_index: could not turn animID into a AnimRunInfo %s.\n", str);
        return false;
    }

    if (tig_net_is_active()) {
        if (tig_net_is_host()) {
            sub_44D0D0(&(run_info->id), 0);
        }

        anim_mp_reap_run_index(&(run_info->id));

        return true;
    }

    return mp_deallocate_run_index(anim_id);
}

// 0x44D240
bool sub_44D240(int index)
{
    AnimRunInfo* run_info;

    run_info = &(anim_run_info[index]);
    run_info->anim_obj = 0;
    run_info->cur_stack_data = NULL;
    run_info->flags = 0;
    run_info->current_goal = -1;
    run_info->path.flags |= 0x1;

    stru_5E3000 = run_info->id;
    timeevent_clear_one_ex(TIMEEVENT_TYPE_ANIM, sub_44CB70);

    return true;
}

// 0x44D2F0
int anim_find_first(int64_t obj)
{
    int slot;
    AnimRunInfo* run_info;

    for (slot = 0; slot < 216; slot++) {
        run_info = &(anim_run_info[slot]);
        if ((run_info->flags & 0x1) != 0
            && (run_info->flags & 0x2) == 0
            && run_info->current_goal > -1
            && run_info->id.slot_num != -1
            && run_info->anim_obj == obj) {
            return slot;
        }
    }

    return -1;
}

// 0x44D340
int anim_find_next(int prev, int64_t obj)
{
    int slot;
    AnimRunInfo* run_info;

    for (slot = prev + 1; slot < 216; slot++) {
        run_info = &(anim_run_info[slot]);
        if ((run_info->flags & 0x1) != 0
            && (run_info->flags & 0x2) == 0
            && run_info->current_goal > -1
            && run_info->id.slot_num != -1
            && run_info->anim_obj == obj) {
            return slot;
        }
    }

    return -1;
}

// 0x44D3B0
bool sub_44D3B0(AnimGoalData* goal_data, int64_t obj, int goal_type, bool a4)
{
    AnimGoalNode* goal_node;

    ASSERT(goal_data != NULL); // pAGoalData != NULL
    ASSERT(goal_type >= 0 && goal_type < ANIM_GOAL_MAX); // (goalType >= 0)&&(goalType < anim_goal_max)

    goal_data->type = goal_type;
    goal_data->params[AGDATA_SELF_OBJ].obj = obj;
    goal_data->params[AGDATA_TARGET_OBJ].obj = OBJ_HANDLE_NULL;
    goal_data->params[AGDATA_BLOCK_OBJ].obj = OBJ_HANDLE_NULL;
    goal_data->params[AGDATA_SCRATCH_OBJ].obj = OBJ_HANDLE_NULL;
    goal_data->params[AGDATA_PARENT_OBJ].obj = OBJ_HANDLE_NULL;
    goal_data->params[AGDATA_TARGET_TILE].loc = 0;
    goal_data->params[AGDATA_ORIGINAL_TILE].loc = 0;
    goal_data->params[AGDATA_RANGE_DATA].data = -1;
    goal_data->params[AGDATA_ANIM_ID].data = -1;
    goal_data->params[AGDATA_ANIM_ID_PREVIOUS].data = -1;
    goal_data->params[AGDATA_ANIM_DATA].data = -1;
    goal_data->params[AGDATA_SPELL_DATA].data = -1;
    goal_data->params[AGDATA_SKILL_DATA].data = -1;
    goal_data->params[AGDATA_FLAGS_DATA].data = 0;
    goal_data->params[AGDATA_SCRATCH_VAL1].data = 0;
    goal_data->params[AGDATA_SCRATCH_VAL2].data = 0;
    goal_data->params[AGDATA_SCRATCH_VAL3].data = 0;
    goal_data->params[AGDATA_SCRATCH_VAL4].data = 0;
    goal_data->params[AGDATA_SCRATCH_VAL5].data = 0;
    goal_data->params[AGDATA_SCRATCH_VAL6].data = 0;
    goal_data->params[AGDATA_SOUND_HANDLE].data = TIG_SOUND_HANDLE_INVALID;

    if (!a4) {
        return true;
    }

    goal_node = anim_goal_nodes[goal_type];

    ASSERT(goal_node != NULL); // pGoalNode != NULL

    return sub_424070(obj, goal_node->priority_level, goal_node->field_8, 1);
}

// 0x44D4E0
bool sub_44D4E0(AnimGoalData* anim_data, int64_t obj, int goal_type)
{
    return sub_44D3B0(anim_data, obj, goal_type, true);
}

// 0x44D500
bool sub_44D500(AnimGoalData* anim_data, int64_t obj, int goal_type)
{
    return sub_44D3B0(anim_data, obj, goal_type, false);
}

// 0x44D520
bool anim_goal_add(AnimGoalData* anim_data, AnimID* anim_id)
{
    return anim_goal_add_ex(anim_data, anim_id, 0);
}

// 0x44D540
bool anim_goal_add_ex(AnimGoalData* anim_data, AnimID* anim_id, unsigned int flags)
{
    Packet5 pkt;
    int index;
    AnimID v1;

    if (anim_private_editor) {
        return false;
    }

    if (!tig_net_is_active()) {
        return anim_goal_add_func(anim_data, anim_id, true, flags);
    }

    if (sub_45B300()
        || (!tig_net_is_host() && !player_is_local_pc_obj(anim_data->params[AGDATA_SELF_OBJ].obj))
        || (!tig_net_is_host() && anim_data->type == AG_DYING)) {
        return true;
    }

    pkt.type = 5;
    pkt.loc = obj_field_int64_get(anim_data->params[AGDATA_SELF_OBJ].obj, OBJ_F_LOCATION);
    pkt.offset_x = obj_field_int32_get(anim_data->params[AGDATA_SELF_OBJ].obj, OBJ_F_OFFSET_X);
    pkt.offset_y = obj_field_int32_get(anim_data->params[AGDATA_SELF_OBJ].obj, OBJ_F_OFFSET_Y);
    pkt.field_8 = sub_45A7C0();
    pkt.field_1A4 = flags;
    anim_id_init(&(pkt.field_198));
    for (index = 0; index < 5; index++) {
        sub_443EB0(anim_data->params[index].obj, &(anim_data->field_B0[index]));
    }

    pkt.field_10 = *anim_data;

    if (tig_net_is_host()) {
        if (!anim_goal_add_func(anim_data, &v1, true, flags)) {
            return false;
        }

        pkt.field_198 = v1;
        if (anim_id != NULL) {
            *anim_id = v1;
        }

        tig_net_send_app_all(&pkt, sizeof(pkt));
        return true;
    } else {
        tig_net_send_app_all(&pkt, sizeof(pkt));
        if (anim_id != NULL) {
            anim_id_init(anim_id);
        }
        return true;
    }
}

// 0x44D730
bool anim_goal_add_func(AnimGoalData* goal_data, AnimID* anim_id, bool a3, unsigned int flags)
{
    AnimID new_anim_id;
    AnimRunInfo* run_info;
    int idx;
    TimeEvent timeevent;
    DateTime datetime;

    ASSERT(goal_data != NULL); // 3527, "pGoalRegData != NULL"
    ASSERT(goal_data->type < ANIM_GOAL_MAX); // 3528, "pGoalRegData->goal_type < anim_goal_max"

    if (in_anim_load) {
        return false;
    }

    if ((obj_field_int32_get(goal_data->params[AGDATA_SELF_OBJ].obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return false;
    }

    if (a3) {
        if (!sub_44CCB0(&new_anim_id)) {
            return false;
        }

        if (anim_id != NULL) {
            *anim_id = new_anim_id;
        }
    } else {
        ASSERT(anim_id != NULL); // 3551, "pAnimID != NULL"

        new_anim_id = *anim_id;
        if (!anim_allocate_this_run_index(&new_anim_id)) {
            return false;
        }
    }

    if (new_anim_id.slot_num == -1) {
        if (anim_id != NULL) {
            anim_id->slot_num = -1;
            anim_id->field_4 = new_anim_id.field_4;
            anim_id->field_8 = new_anim_id.field_8;
        }
        return false;
    }

    run_info = &(anim_run_info[new_anim_id.slot_num]);
    run_info->current_goal = 0;
    run_info->current_state = 0;
    run_info->path_attached_to_stack_index = -1;
    run_info->anim_obj = goal_data->params[AGDATA_SELF_OBJ].obj;
    run_info->flags |= flags;
    run_info->goals[0] = *goal_data;
    run_info->cur_stack_data = &(run_info->goals[0]);
    for (idx = 0; idx < 5; idx++) {
        sub_443EB0(run_info->goals[0].params[idx].obj, &(run_info->goals[0].field_B0[idx]));
    }
    sub_44C840(run_info, anim_goal_nodes[run_info->goals[0].type]);
    sub_423E60("GoalAdd");
    if ((goal_data->type == AG_ATTACK
            || goal_data->type == AG_ATTEMPT_ATTACK)
        && player_is_local_pc_obj(run_info->anim_obj)) {
        sub_460280(run_info->goals[0].params[AGDATA_TARGET_OBJ].obj);
    }

    timeevent.type = TIMEEVENT_TYPE_ANIM;
    timeevent.params[0].integer_value = new_anim_id.slot_num;
    timeevent.params[1].integer_value = new_anim_id.field_4;
    timeevent.params[2].integer_value = 3333;
    sub_45A950(&datetime, 5);

    if (!combat_turn_based_is_active() || combat_turn_based_whos_turn_get() == run_info->anim_obj) {
        return timeevent_add_delay(&timeevent, &datetime);
    } else {
        return timeevent_add_immediate(&timeevent);
    }
}

// 0x44D9B0
bool anim_subgoal_add_func(AnimID anim_id, AnimGoalData* goal_data)
{
    AnimRunInfo* run_info;
    char str[ANIM_ID_STR_SIZE];
    int idx;

    ASSERT(goal_data != NULL); // 3655, "pGoalRegData != NULL"
    ASSERT(goal_data->type < ANIM_GOAL_MAX); // 3656, "pGoalRegData->goal_type < anim_goal_max"

    if (anim_id.slot_num == -1) {
        if (!tig_net_is_active()
            || tig_net_is_host()) {
            return false;
        }

        return anim_goal_add_func(goal_data, &anim_id, false, 0);
    }

    if (!anim_id_to_run_info(&anim_id, &run_info)) {
        anim_id_to_str(&anim_id, str);
        tig_debug_printf("Anim: anim_subgoal_add_func: could not turn animID into a AnimRunInfo %s.\n", str);
        return false;
    }

    if (!tig_net_is_active()) {
        if (run_info->current_goal >= 7) {
            return false;
        }
    }

    ASSERT(run_info->current_goal < 7); // 3675, "pRunInfo->current_goal < (ANIM_MAX_GOAL_STACK_SIZE - 1)"
    ASSERT(run_info->current_goal >= 0); // 3676, "pRunInfo->current_goal >= 0"

    run_info->current_goal++;

    for (idx = run_info->current_goal; idx > 0; idx--) {
        run_info->goals[idx] = run_info->goals[idx - 1];
    }

    run_info->goals[0] = *goal_data;
    for (idx = 0; idx < 5; idx++) {
        sub_443EB0(run_info->goals[0].params[idx].obj, &(run_info->goals[0].field_B0[idx]));
    }

    if (run_info->path_attached_to_stack_index != -1) {
        run_info->path_attached_to_stack_index++;
    }

    sub_44C840(run_info, anim_goal_nodes[goal_data->type]);
    sub_423E60("SubGoal Add");

    return true;
}

// 0x44DBE0
bool anim_subgoal_add(AnimID anim_id, AnimGoalData* goal_data, const char* file, int line)
{
    Packet7 pkt;
    int64_t obj;
    int idx;

    (void)file;
    (void)line;

    if (!tig_net_is_active()) {
        return anim_subgoal_add_func(anim_id, goal_data);
    }

    if (!tig_net_is_host()
        && !player_is_local_pc_obj(goal_data->params[AGDATA_SELF_OBJ].obj)) {
        return true;
    }

    obj = goal_data->params[AGDATA_SELF_OBJ].obj;

    pkt.type = 7;
    pkt.anim_id = anim_id;
    pkt.loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    pkt.offset_x = obj_field_int32_get(obj, OBJ_F_OFFSET_X);
    pkt.offset_y = obj_field_int32_get(obj, OBJ_F_OFFSET_Y);

    for (idx = 0; idx < 5; idx++) {
        sub_443EB0(goal_data->params[idx].obj, &(goal_data->field_B0[idx]));
    }

    pkt.goal_data = *goal_data;

    if (!tig_net_is_host()) {
        tig_net_send_app_all(&pkt, sizeof(pkt));
        return false;
    }

    if (!anim_subgoal_add_func(anim_id, goal_data)) {
        return false;
    }

    tig_net_send_app_all(&pkt, sizeof(pkt));
    return true;
}

// 0x44DD80
bool anim_recover_handles(AnimRunInfo* run_info, AnimGoalSubNode* goal_subnode)
{
    int idx;
    int64_t obj;

    for (idx = 0; idx < 5; idx++) {
        obj = run_info->cur_stack_data->params[idx].obj;
        if (obj != OBJ_HANDLE_NULL) {
            if (map_is_clearing_objects()) {
                for (idx = 0; idx < 5; idx++) {
                    run_info->cur_stack_data->params[idx].obj = OBJ_HANDLE_NULL;
                }
                run_info->anim_obj = OBJ_HANDLE_NULL;
                return false;
            }

            if (!obj_handle_is_valid(obj)) {
                if (!sub_443F80(&obj, &(run_info->cur_stack_data->field_B0[idx]))) {
                    tig_debug_printf("Anim: ERROR: Object validate recovery FAILED!\n");
                    run_info->cur_stack_data->params[idx].obj = OBJ_HANDLE_NULL;
                    ASSERT(0); // 3808, "0"
                    return false;
                }

                run_info->cur_stack_data->params[idx].obj = obj;
            }
        }
    }

    run_info->anim_obj = run_info->cur_stack_data->params[AGDATA_SELF_OBJ].obj;

    if (goal_subnode != NULL) {
        for (idx = 0; idx < 2; idx++) {
            int param = goal_subnode->params[idx];
            if (param < 0) {
                run_info->params[idx].data = 0;
            } else if (param < AGDATA_COUNT) {
                switch (dword_5A597C[param]) {
                case AGDATATYPE_OBJ:
                    run_info->params[idx].obj = run_info->cur_stack_data->params[param].obj;
                    break;
                case AGDATATYPE_LOC:
                    run_info->params[idx].loc = run_info->cur_stack_data->params[param].loc;
                    break;
                default:
                    run_info->params[idx].data = run_info->cur_stack_data->params[param].data;
                    break;
                }
            } else {
                switch (param) {
                case AGDATA_SELF_TILE:
                    run_info->params[idx].obj = obj_field_int64_get(run_info->cur_stack_data->params[AGDATA_SELF_OBJ].obj, OBJ_F_LOCATION);
                    break;
                case 32: {
                    int64_t parent_obj;
                    int64_t item_obj = run_info->cur_stack_data->params[AGDATA_TARGET_OBJ].obj;
                    if (obj_type_is_item(obj_field_int32_get(item_obj, OBJ_F_TYPE))
                        && item_parent(item_obj, &parent_obj)) {
                        run_info->params[idx].loc = obj_field_int64_get(parent_obj, OBJ_F_LOCATION);
                        obj_field_int64_set(item_obj, OBJ_F_LOCATION, run_info->params[idx].loc);
                    } else {
                        run_info->params[idx].loc = obj_field_int64_get(item_obj, OBJ_F_LOCATION);
                    }
                    break;
                }
                case AGDATA_NULL_OBJ:
                    run_info->params[idx].obj = OBJ_HANDLE_NULL;
                    break;
                case AGDATA_FORCE_TARGET_TILE:
                    if (run_info->cur_stack_data->params[AGDATA_TARGET_TILE].loc != 0) {
                        run_info->params[idx].loc = run_info->cur_stack_data->params[AGDATA_TARGET_TILE].loc;
                    } else {
                        if (run_info->cur_stack_data->params[AGDATA_TARGET_OBJ].obj != OBJ_HANDLE_NULL) {
                            run_info->cur_stack_data->params[AGDATA_TARGET_TILE].loc = obj_field_int64_get(run_info->cur_stack_data->params[AGDATA_TARGET_OBJ].obj, OBJ_F_LOCATION);
                        }

                        run_info->params[idx].loc = run_info->cur_stack_data->params[AGDATA_TARGET_TILE].loc;
                    }
                    break;
                }
            }
        }

        run_info->params[2].data = goal_subnode->field_C;
    }

    return true;
}

// 0x44E050
void sub_44E050(int64_t a1, int64_t a2)
{
    AnimID anim_id;
    AnimRunInfo* run_info;

    if (sub_423300(a1, &anim_id)) {
        run_info = &(anim_run_info[anim_id.slot_num]);
        if (run_info->goals[0].type == AG_ATTACK
            || run_info->goals[1].type == AG_ATTEMPT_ATTACK) {
            if (run_info->goals[0].params[AGDATA_TARGET_OBJ].obj == a2
                || critter_pc_leader_get(run_info->goals[0].params[AGDATA_TARGET_OBJ].obj) == a2) {
                sub_44E2C0(&anim_id, 5);
            }
        }
    }
}

// 0x44E0E0
void sub_44E0E0(int64_t a1, int64_t a2)
{
    AnimID anim_id;
    AnimRunInfo* run_info;

    if (sub_423300(a1, &anim_id)) {
        run_info = &(anim_run_info[anim_id.slot_num]);
        if (run_info->goals[0].type == AG_ATTACK
            || run_info->goals[1].type == AG_ATTEMPT_ATTACK) {
            if (run_info->goals[0].params[AGDATA_TARGET_OBJ].obj == a2) {
                sub_44E2C0(&anim_id, 5);
            }
        }
    }
}

// 0x44E160
bool sub_44E160(AnimID* anim_id)
{
    AnimRunInfo* run_info;
    int idx;
    AnimGoalNode* goal_node;

    ASSERT(anim_id != NULL); // 3979, "pAnimID != NULL"
    ASSERT(anim_id->slot_num < 216); // 3980, "pAnimID->slotNum < ANIM_MAX_CURRENT_ANIMS"

    if (!anim_id_to_run_info(anim_id, &run_info)) {
        return false;
    }

    if ((run_info->flags & 0x1) == 0) {
        return false;
    }

    run_info->flags |= 0x8002;

    if (anim_id->slot_num == dword_5A5978) {
        return true;
    }

    dword_5B052C = anim_id->slot_num;
    timeevent_clear_all_ex(TIMEEVENT_TYPE_ANIM, sub_44E2A0);

    if (run_info->current_goal != -1) {
        if (run_info->cur_stack_data == NULL) {
            run_info->cur_stack_data = &(run_info->goals[run_info->current_goal]);
        }

        for (idx = run_info->current_goal; idx >= 0; idx--) {
            goal_node = anim_goal_nodes[run_info->goals[idx].type];
            if (goal_node->subnodes[14].func != NULL) {
                if (anim_recover_handles(run_info, &(goal_node->subnodes[14]))) {
                    goal_node->subnodes[14].func(run_info);
                }
            }
        }
    }

    return anim_free_run_index(anim_id);
}

// 0x44E2A0
bool sub_44E2A0(TimeEvent* timeevent)
{
    return timeevent->params[0].integer_value == dword_5B052C;
}

// 0x44E2C0
bool sub_44E2C0(AnimID* anim_id, int priority)
{
    AnimRunInfo* run_info;
    bool in_reset;
    AnimGoalNode* goal_node;
    int idx;
    bool freed;

    ASSERT(anim_id != NULL); // 4034, "pAnimID != NULL"
    ASSERT(anim_id->slot_num < 216); // 4035, "pAnimID->slotNum < ANIM_MAX_CURRENT_ANIMS"

    if (!anim_id_to_run_info(anim_id, &run_info)) {
        return false;
    }

    in_reset = gamelib_in_reset();

    if ((run_info->flags & 0x01) == 0) {
        return false;
    }

    if (!in_reset) {
        if (run_info->current_goal != -1) {
            goal_node = anim_goal_nodes[run_info->goals[run_info->current_goal].type];

            ASSERT(goal_node != NULL); // 4070, "pGoalNode != NULL"

            if (priority < PRIORITY_HIGHEST) {
                if (goal_node->field_8) {
                    return true;
                }

                if (goal_node->priority_level == 5) {
                    return false;
                }
            }

            if (goal_node->priority_level == 3) {
                if (priority < 3) {
                    return false;
                }
            } else if (goal_node->priority_level == 2) {
                if (priority < 2) {
                    return false;
                }
            } else if (priority <= goal_node->priority_level) {
                return false;
            }
        }
    }

    run_info->flags |= 0x02;

    if (anim_id->slot_num == dword_5A5978) {
        return true;
    }

    dword_5B052C = anim_id->slot_num;
    timeevent_clear_all_ex(TIMEEVENT_TYPE_ANIM, sub_44E2A0);

    if (run_info->current_goal != -1) {
        if (run_info->cur_stack_data == NULL) {
            run_info->cur_stack_data = &(run_info->goals[run_info->current_goal]);
        }

        for (idx = run_info->current_goal; idx >= 0; idx--) {
            goal_node = anim_goal_nodes[run_info->goals[idx].type];
            if (!in_reset) {
                if (goal_node->subnodes[14].func != NULL) {
                    if (anim_recover_handles(run_info, &(goal_node->subnodes[14]))) {
                        goal_node->subnodes[14].func(run_info);
                    }
                }
            }
        }
    }

    freed = anim_free_run_index(anim_id);
    sub_423E60("Interrupt Slot");

    return freed;
}

// 0x44E4D0
bool sub_44E4D0(int64_t obj, int goal_type, int a3)
{
    int priority;
    AnimGoalNode* goal_node;
    int prev_slot = -1;
    int slot;
    AnimRunInfo* run_info;

    if (a3 != -1) {
        goal_node = anim_goal_nodes[a3];

        ASSERT(goal_node != NULL); // 4156, "pGoalNode != NULL"

        priority = goal_node->priority_level;

        ASSERT(priority >= PRIORITY_NONE && priority <= PRIORITY_HIGHEST); // 4166, "(priorityLevel >= priorityNone)&&(priorityLevel <= priorityHighest)"

        if (goal_node->field_8) {
            priority = PRIORITY_NONE;
        }
    } else {
        priority = PRIORITY_HIGHEST;
    }

    slot = anim_find_first(obj);
    while (slot != -1 && slot != prev_slot) {
        prev_slot = slot;

        run_info = &(anim_run_info[slot]);
        if (run_info->goals[0].type == goal_type) {
            if (!sub_44E2C0(&(run_info->id), priority)) {
                return false;
            }
        } else if (run_info->cur_stack_data->type == goal_type) {
            if (!sub_44E2C0(&(run_info->id), priority)) {
                return false;
            }
        }

        slot = anim_find_next(slot, obj);
    }

    return true;
}

// 0x44E5F0
bool anim_find_first_of_type(int64_t obj, int type, AnimID* anim_id)
{
    AnimID tmp_anim_id;

    if (anim_id == NULL) {
        anim_id = &tmp_anim_id;
    }

    anim_id_init(anim_id);
    return anim_find_next_of_type(obj, type, anim_id);
}

// 0x44E650
bool anim_find_next_of_type(int64_t obj, int type, AnimID* anim_id)
{
    int prev_slot = -1;
    int slot;
    AnimRunInfo* run_info;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    slot = anim_id != NULL ? anim_id->slot_num + 1 : anim_find_first(obj);
    while (slot != -1 && slot != prev_slot) {
        prev_slot = slot;
        run_info = &(anim_run_info[slot]);
        if (run_info->goals[0].type == type && run_info->id.slot_num != -1) {
            if (anim_id != NULL) {
                *anim_id = run_info->id;
            }
            return true;
        }

        slot = anim_find_next(slot, obj);
    }

    return false;
}

// 0x44E6F0
bool sub_44E6F0(int64_t obj, AnimGoalData* goal_data)
{
    return sub_44E710(obj, goal_data, NULL);
}

// 0x44E710
bool sub_44E710(int64_t obj, AnimGoalData* goal_data, AnimID* anim_id)
{
    int prev_slot;
    int slot;
    AnimRunInfo* run_info;
    AnimGoalNode* goal_node;
    int goal_type;
    int idx;
    int param;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    slot = anim_find_first(obj);
    while (slot != -1) {
        prev_slot = slot;

        run_info = &(anim_run_info[slot]);

        if (goal_data->type != -1) {
            // NOTE: Original code is slightly different but does the same
            // thing.
            goal_node = anim_goal_nodes[goal_data->type];

            for (idx = -1; idx < 3; idx++) {
                goal_type = idx == -1
                    ? goal_data->type
                    : goal_node->field_14[idx];
                if (goal_type == -1) {
                    break;
                }

                if (goal_type == run_info->goals[0].type) {
                    for (param = 0; param < 5; param++) {
                        if (goal_data->params[param].obj != run_info->goals[0].params[param].obj) {
                            break;
                        }
                    }

                    if (param >= 5) {
                        if (anim_id != NULL) {
                            *anim_id = run_info->id;
                        }

                        return true;
                    }
                }
            }
        }

        slot = anim_find_next(slot, obj);

        ASSERT(slot != prev_slot); // 4293, "animRunIndex != lastAnimRunIndex"

        if (slot == prev_slot) {
            return false;
        }
    }

    return false;
}

// 0x44E830
bool anim_is_current_goal_type(int64_t obj, int goal_type, AnimID* anim_id)
{
    int slot;
    int idx;
    AnimRunInfo* run_info;
    AnimGoalNode* goal_node;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    slot = anim_find_first(obj);
    if (slot == -1) {
        return false;
    }

    run_info = &(anim_run_info[slot]);

    if (goal_type == -1) {
        return false;
    }

    if (goal_type == run_info->goals[0].type) {
        if (anim_id != NULL) {
            *anim_id = run_info->id;
        }

        return true;
    }

    goal_node = anim_goal_nodes[goal_type];

    for (idx = 0; idx < 3; idx++) {
        if (goal_node->field_14[idx] == -1) {
            break;
        }

        if (goal_node->field_14[idx] == run_info->goals[0].type) {
            if (anim_id != NULL) {
                *anim_id = run_info->id;
            }

            return true;
        }
    }

    return false;
}

// 0x44E8C0
bool sub_44E8C0(int64_t obj, AnimID* anim_id)
{
    int slot;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    slot = anim_find_first(obj);
    while (slot != -1) {
        if (!anim_goal_nodes[anim_run_info[slot].goals[0].type]->field_C) {
            if (anim_id != NULL) {
                *anim_id = anim_run_info[slot].id;
            }
            return true;
        }
        slot = anim_find_next(slot, obj);
    }

    return false;
}

// 0x44E940
bool sub_44E940(int64_t obj, AnimID* anim_id, int64_t a2)
{
    int slot;
    int goal_type;
    AnimRunInfo* run_info;
    AnimGoalNode* goal_node;
    int idx;
    int64_t leader_obj;
    int64_t target_leader_obj;
    ObjectList objects;
    ObjectNode* node;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    slot = anim_find_first(obj);
    if (slot == -1) {
        return false;
    }

    run_info = &(anim_run_info[slot]);

    goal_type = AG_ATTACK;
    goal_node = anim_goal_nodes[goal_type];

    for (idx = -1; idx < 3; idx++) {
        if (idx != -1) {
            goal_type = goal_node->field_14[idx];
        }

        if (goal_type == -1) {
            break;
        }

        if (goal_type == run_info->goals[0].type) {
            if (a2 == OBJ_HANDLE_NULL) {
                if (anim_id != NULL) {
                    *anim_id = run_info->id;
                }
                return true;
            }

            leader_obj = critter_pc_leader_get(a2);
            if (leader_obj == OBJ_HANDLE_NULL) {
                leader_obj = a2;
            }

            target_leader_obj = critter_pc_leader_get(run_info->goals[0].params[AGDATA_TARGET_OBJ].obj);
            if (target_leader_obj == OBJ_HANDLE_NULL) {
                target_leader_obj = run_info->goals[0].params[AGDATA_TARGET_OBJ].obj;
            }

            if (leader_obj == target_leader_obj) {
                if (anim_id != NULL) {
                    *anim_id = run_info->id;
                }
                return true;
            }

            if (tig_net_is_active()
                && obj_field_int32_get(a2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                object_list_party(a2, &objects);
                node = objects.head;
                while (node != NULL) {
                    if (node->obj == run_info->goals[0].params[AGDATA_TARGET_OBJ].obj) {
                        break;
                    }
                    node = node->next;
                }
                object_list_destroy(&objects);

                if (node != NULL) {
                    if (anim_id != NULL) {
                        *anim_id = run_info->id;
                    }
                    return true;
                }
            }

            return false;
        }
    }

    return false;
}

// 0x44EAD0
bool sub_44EAD0(int index)
{
    return anim_goal_nodes[index]->field_8 == 1;
}

// 0x44EB40
bool sub_44EB40(int64_t obj, int64_t from, int64_t to)
{
    ASSERT(obj != OBJ_HANDLE_NULL); // 4489, "obj != OBJ_HANDLE_NULL"

    stru_5E33F8.flags = 1;
    stru_5E33F8.field_CC = 200;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (to == 0 || to == -1) {
        return false;
    }

    if (!sub_426560(obj, from, to, &stru_5E33F8, 0)) {
        return false;
    }

    return true;
}

// 0x44EBD0
void sub_44EBD0(AnimPath* path)
{
    (void)path;
}

// 0x44EBE0
void sub_44EBE0(AnimPath* path)
{
    (void)path;
}

// 0x44EBF0
void sub_44EBF0(AnimRunInfo* run_info)
{
    ASSERT(run_info != NULL); // pRunInfo != NULL

    if ((run_info->path.flags & 0x1) != 0) {
        run_info->flags |= 0x10000;
    }
}

// 0x44EC30
void anim_path_debug(AnimPath* path)
{
    int idx;

    tig_debug_printf("    flags(0x%0X): %s\n", path->flags, "");
    tig_debug_printf("    path: ");

    for (idx = 0; idx < path->max; idx++) {
        switch (path->rotations[idx]) {
        case 0:
            tig_debug_printf("U  ");
            break;
        case 1:
            tig_debug_printf("UR ");
            break;
        case 2:
            tig_debug_printf("R  ");
            break;
        case 3:
            tig_debug_printf("DR ");
            break;
        case 4:
            tig_debug_printf("D  ");
            break;
        case 5:
            tig_debug_printf("DL ");
            break;
        case 6:
            tig_debug_printf("L  ");
            break;
        case 7:
            tig_debug_printf("UL ");
            break;
        default:
            tig_debug_printf("?(%d)? ", path->rotations[idx]);
            break;
        }
    }

    tig_debug_printf("    baseRot: %d\n", path->baseRot);
    tig_debug_printf("    curr: %d, max: %d\n", path->curr, path->max);
    tig_debug_printf("    subsequence: %d\n", path->subsequence);
    tig_debug_printf("    maxPathLength: %d, absMaxPathLength: %d\n", path->maxPathLength, path->absMaxPathLength);
}

// 0x44ED90
void anim_goal_data_debug(AnimGoalData* goal_data)
{
    int idx;
    char str[256];

    tig_debug_printf("    goal_type: %s(%d)\n", off_5A164C[goal_data->type], goal_data->type);

    for (idx = 0; idx < AGDATA_COUNT; idx++) {
        if (idx < 5) {
            if (goal_data->params[idx].obj != OBJ_HANDLE_NULL) {
                if (obj_handle_is_valid(goal_data->params[idx].obj)) {
                    object_examine(goal_data->params[idx].obj, goal_data->params[idx].obj, str);
                } else {
                    strcpy(str, "INVALID_OBJ_HANDLE");
                }
                tig_debug_printf("    params[ %s ] = %s\n", off_5B0530[idx], str);
            }
        } else if (idx == AGDATA_TARGET_TILE) {
            if (goal_data->params[idx].loc != 0) {
                tig_debug_printf("    params[ %s ] = X:%d, Y:%d\n",
                    off_5B0530[idx],
                    (int)LOCATION_GET_X(goal_data->params[idx].loc),
                    (int)LOCATION_GET_Y(goal_data->params[idx].loc));
            }
        } else {
            if (goal_data->params[idx].data != 0
                && goal_data->params[idx].data != -1) {
                tig_debug_printf("    params[ %s ] = %d\n",
                    off_5B0530[idx],
                    goal_data->params[idx].data);
            }
        }
    }
}

// 0x44EEB0
void anim_run_info_debug(AnimRunInfo* run_info)
{
    char str[ANIM_ID_STR_SIZE];
    char temp[256];
    int goal;

    if (run_info == NULL) {
        tig_debug_printf("Anim: anim_run_info_debug: input is NULL\n");
        return;
    }

    anim_id_to_str(&(run_info->id), str);
    tig_debug_printf("BEGIN animID:%s\n", str);
    tig_debug_printf("flags(0x%0X): %s\n", run_info->flags, "");
    tig_debug_printf("current_state: %d\n", run_info->current_state);
    tig_debug_printf("pathAttachedToStackIndex: %d\n", run_info->path_attached_to_stack_index);
    tig_debug_printf("nextPingTime: %d ticks, %d days\n", run_info->next_ping_time.milliseconds, run_info->next_ping_time.days);

    if (run_info->anim_obj != OBJ_HANDLE_NULL && obj_handle_is_valid(run_info->anim_obj)) {
        int64_t loc = obj_field_int64_get(run_info->anim_obj, OBJ_F_LOCATION);
        object_examine(run_info->anim_obj, run_info->anim_obj, temp);
        tig_debug_printf("animObj: %s @(x:%d,y:%d)\n",
            temp,
            (int)location_get_x(loc),
            (int)location_get_y(loc));
    } else {
        tig_debug_printf("animObj: OBJ_HANDLE_NULL\n");
    }

    tig_debug_printf("extraTargetTile: X:%d, Y:%d\n",
        (int)location_get_x(run_info->extra_target_tile),
        (int)location_get_y(run_info->extra_target_tile));

    tig_debug_printf("current_goal: %d\n", run_info->current_goal);

    if (run_info->cur_stack_data != NULL) {
        tig_debug_printf("goal_stack[ %d ] == curStackPtr -->\n", run_info->current_goal);
        anim_goal_data_debug(run_info->cur_stack_data);
    } else {
        tig_debug_printf("ERROR: pCurStackData == NULL!\n");
    }

    for (goal = run_info->current_goal - 1; goal >= 0; goal--) {
        tig_debug_printf("goal_stack[ %d ] -->\n", goal);
        anim_goal_data_debug(&(run_info->goals[goal]));
    }

    tig_debug_printf("PATH:\n");
    anim_path_debug(&(run_info->path));

    tig_debug_printf("pauseTime: %d ticks %d days\n",
        run_info->pause_time.milliseconds,
        run_info->pause_time.days);

    // TODO: Incomplete - params.

    tig_debug_printf("END animID:%s\n", str);
}

// 0x44EEC0
void anim_run_index_debug(int index)
{
    anim_run_info_debug(&(anim_run_info[index]));
}

// 0x44EEE0
void anim_stats(void)
{
    int index;

    tig_debug_printf("Currently Existing Animations\n");
    tig_debug_printf("------------------------------------------------\n");

    for (index = 0; index < 216; index++) {
        if (anim_run_info[index].flags != 0) {
            tig_debug_printf("In Slot %d:\n", index);
            anim_run_info_debug(&(anim_run_info[index]));
            tig_debug_printf("------------------------------------------------\n");
        }
    }

    tig_debug_printf("Done.\n");
    tig_debug_printf("------------------------------------------------\n");
}
