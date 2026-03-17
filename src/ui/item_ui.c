#include "ui/item_ui.h"

#include "game/anim.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/item.h"
#include "game/magictech.h"
#include "game/mt_item.h"
#include "game/obj.h"
#include "game/player.h"
#include "game/skill.h"
#include "game/target.h"
#include "game/trap.h"
#include "ui/dialog_ui.h"
#include "ui/intgame.h"
#include "ui/inven_ui.h"
#include "ui/spell_ui.h"

static void sub_5719F0(int64_t item_obj, uint64_t** tgt_ptr);

// 0x5CAC40
static uint64_t qword_5CAC40 = Tgt_Object;

// 0x5CAC48
static uint64_t qword_5CAC48 = Tgt_Obj_Inven;

// 0x5CAC50
static uint64_t qword_5CAC50 = Tgt_Tile | Tgt_Object;

// 0x6810D8
static int64_t qword_6810D8;

// 0x5719D0
bool item_ui_init(GameInitInfo* init_info)
{
    (void)init_info;

    return true;
}

// 0x5719E0
void item_ui_exit(void)
{
}

// 0x5719F0
void sub_5719F0(int64_t item_obj, uint64_t** tgt_ptr)
{
    int mana_store;
    unsigned int flags;
    int spl;

    *tgt_ptr = 0;

    if (item_obj != OBJ_HANDLE_NULL) {
        mana_store = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_MANA_STORE);
        flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
        spl = mt_item_spell(item_obj, 0);
        if (mana_store != 0 || (flags & OIF_IS_MAGICAL) != 0) {
            sub_459F20(spl, tgt_ptr);
        } else if (trap_is_trap_device(item_obj)) {
            *tgt_ptr = &qword_5CAC50;
        } else if (spl == -1) {
            *tgt_ptr = &qword_5CAC40;
        } else {
            sub_459F20(spl, tgt_ptr);
        }
    }
}

// 0x571AA0
void item_ui_activate(int64_t owner_obj, int64_t item_obj)
{
    uint64_t* tgt_ptr;
    TargetDescriptor td;

    if (item_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if ((obj_field_int32_get(owner_obj, OBJ_F_FLAGS) & OF_OFF) != 0
        && !critter_is_dead(owner_obj)
        && !critter_is_unconscious(owner_obj)) {
        return;
    }

    if (intgame_mode_get() == INTGAME_MODE_ITEM) {
        item_ui_deactivate();
    }

    qword_6810D8 = item_obj;

    sub_5719F0(item_obj, &tgt_ptr);

    if ((*tgt_ptr & (Tgt_Obj_Inven & ~Tgt_Object)) == 0) {
        inven_ui_destroy();

        if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
            return;
        }
    }

    // TODO: Check, probably wrong.
    if (*tgt_ptr == Tgt_None
        || ((*tgt_ptr & Tgt_Self) != 0
            && (*tgt_ptr & Tgt_Tile) == 0)
        || *tgt_ptr == Tgt_Obj_Radius) {
        target_descriptor_set_loc(&td, obj_field_int64_get(owner_obj, OBJ_F_LOCATION));
        spell_ui_apply(&td);
        return;
    }

    if (combat_check_action_points(owner_obj, 4)) {
        sub_4F25B0(*tgt_ptr);
        if (intgame_mode_set(INTGAME_MODE_ITEM)) {
            sub_4F25B0(*tgt_ptr);
            if (obj_field_int32_get(qword_6810D8, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) < 0) {
                intgame_item_mode_cursor_set(620);
                intgame_refresh_cursor();
            }

            if ((*tgt_ptr & Tgt_Object) != 0
                && tig_kb_get_modifier(SDL_KMOD_SHIFT)) {
                target_descriptor_set_obj(&td, player_get_local_pc_obj());
                item_ui_apply(&td);
            }
        }
    }
}

// 0x571C80
void item_ui_deactivate(void)
{
    intgame_mode_set(INTGAME_MODE_MAIN);
    qword_6810D8 = OBJ_HANDLE_NULL;
    intgame_item_mode_cursor_set(21);
}

// 0x571CB0
void item_ui_apply(TargetDescriptor* td)
{
    int64_t pc_obj;
    SkillInvocation skill_invocation;
    int spell_mana_store = 0;
    unsigned int item_flags = 0;
    int range = -1;
    int64_t item_obj;
    char str[1000];

    pc_obj = player_get_local_pc_obj();

    if (critter_is_dead(pc_obj)) {
        return;
    }

    if (critter_is_unconscious(pc_obj)) {
        return;
    }

    if (obj_field_int32_get(qword_6810D8, OBJ_F_TYPE) == OBJ_TYPE_GENERIC
        && !tig_kb_get_modifier(SDL_KMOD_LCTRL)
        && (obj_field_int32_get(qword_6810D8, OBJ_F_GENERIC_FLAGS) & OGF_IS_LOCKPICK) != 0) {
        skill_invocation_init(&skill_invocation);
        sub_4440E0(pc_obj, &(skill_invocation.source));
        if (!td->is_loc) {
            sub_4440E0(td->obj, &(skill_invocation.target));
        }
        skill_invocation.skill = SKILL_PICK_LOCKS;
        skill_invocation.item.obj = item_find_first_generic(skill_invocation.source.obj, OGF_IS_LOCKPICK);
        skill_pick_best_follower(&skill_invocation);

        if (pc_obj == skill_invocation.source.obj) {
            item_obj = qword_6810D8;
        } else {
            item_obj = item_find_first_generic(skill_invocation.source.obj, OGF_IS_LOCKPICK);
            dialog_copy_npc_let_me_handle_msg(skill_invocation.source.obj, pc_obj, str);
            dialog_ui_float_line(skill_invocation.source.obj, pc_obj, str, 0);
            if (item_obj == OBJ_HANDLE_NULL) {
                anim_goal_use_item_on_obj_with_skill(skill_invocation.source.obj,
                    OBJ_HANDLE_NULL,
                    skill_invocation.target.obj,
                    SKILL_PICK_LOCKS,
                    0);
                item_ui_deactivate();
                return;
            }
        }
    } else {
        item_obj = qword_6810D8;
    }

    if (item_obj != OBJ_HANDLE_NULL) {
        spell_mana_store = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_MANA_STORE);
        item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
    }

    if (td->is_loc) {
        if (spell_mana_store != 0 || (item_flags & OIF_IS_MAGICAL) != 0) {
            item_use_on_loc(pc_obj, item_obj, td->loc);
        } else if (trap_is_trap_device(item_obj)) {
            anim_goal_use_item_on_loc(pc_obj, td->loc, item_obj, 0);
        } else {
            anim_goal_throw_item(pc_obj, item_obj, td->loc);
        }
    } else {
        if (spell_mana_store != 0 || (item_flags & OIF_IS_MAGICAL) != 0) {
            range = magictech_get_range(obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_1));
            if (range < object_dist(pc_obj, td->obj)) {
                range = -1;
            }
        }

        // TODO: Too many conditions merged with previous block, probably wrong.
        if ((spell_mana_store != 0
                && range != -1
                && (item_flags & OIF_NO_RANGED_USE) == 0)
            || ((item_flags & OIF_IS_MAGICAL) == 0
                && range == -1
                && (item_flags & OIF_NO_RANGED_USE) == 0)
            || ((item_flags & OIF_IS_MAGICAL) != 0
                && range != -1
                && (item_flags & OIF_NO_RANGED_USE) == 0)) {
            item_use_on_obj(pc_obj, item_obj, td->obj);
            item_ui_deactivate();
            return;
        }

        anim_goal_use_item_on_obj(pc_obj, td->obj, item_obj, 0);
        if (tig_kb_get_modifier(SDL_KMOD_LSHIFT)) {
            sub_436C80();
            item_ui_deactivate();
            return;
        }

        if (tig_kb_get_modifier(SDL_KMOD_LCTRL)
            || !tig_kb_get_modifier(SDL_KMOD_NUM)
            || (get_always_run(pc_obj) && !tig_kb_get_modifier(SDL_KMOD_LCTRL))) {
            sub_436C20();
            item_ui_deactivate();
            return;
        }
    }

    item_ui_deactivate();
}
