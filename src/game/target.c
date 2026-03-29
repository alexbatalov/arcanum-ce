#include "game/target.h"

#include "game/critter.h"
#include "game/material.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/path.h"
#include "game/player.h"
#include "game/random.h"
#include "game/stat.h"
#include "game/tile.h"

static bool sub_4F28A0(int x, int y, TargetDescriptor* td);
static void target_list_add_obj_unless_off(TargetList* list, int64_t obj);
static void target_list_add_obj_unless_destroyed(TargetList* list, int64_t obj);
static void target_list_add_loc(TargetList* list, int64_t loc);

/**
 * Scratch path-finding state reused across targeting operations that require
 * path computation.
 *
 * 0x603B88
 */
static PathCreateInfo target_path_create_info;

// 0x603BB0
static TigRect target_iso_content_rect;

/**
 * Array of directions filled by the pathfinder.
 *
 * 0x603BC4
 */
static int8_t target_path_rotations[200];

/**
 * The current step index while walking `target_path_rotations` after a
 * successful pathfinding call.
 *
 * 0x603C94
 */
static int target_path_step;

/**
 * The number of valid steps computed by the most recent pathfinding call.
 *
 * 0x603C98
 */
static int target_path_length;

/**
 * The starting tile location of the most recently computed path.
 *
 * 0x603CA8
 */
static int64_t target_path_from;

/**
 * The ending tile location of the most recently computed path.
 *
 * 0x603CB0
 */
static int64_t target_path_to;

// 0x603CB8
static TargetContext stru_603CB8;

// 0x603D20
static TargetParams stru_603D20;

// 0x603D38
static bool target_initialized;

/**
 * Stores the targeting flag that caused the most recent targeting attempt
 * rejection.
 *
 * NOTE: For unknown reason this value only includes either damaged or
 * non-damaged flag.
 *
 * 0x603D40
 */
static uint64_t target_last_rejection;

// 0x4F24F0
bool target_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    target_iso_content_rect = window_data.rect;
    target_context_init(&stru_603CB8, &stru_603D20, OBJ_HANDLE_NULL);
    target_initialized = true;

    return true;
}

// 0x4F2560
void target_exit(void)
{
    target_initialized = false;
}

// 0x4F2570
void target_reset(void)
{
    target_last_rejection = Tgt_None;
}

// 0x4F2580
void target_resize(GameResizeInfo* resize_info)
{
    target_iso_content_rect = resize_info->content_rect;
}

/**
 * Sets current targeting flags.
 *
 * 0x4F25B0
 */
void target_flags_set(uint64_t flags)
{
    stru_603D20.tgt = flags;
}

/**
 * Retrieves current targeting flags.
 *
 * 0x4F25D0
 */
uint64_t target_flags_get(void)
{
    return stru_603D20.tgt;
}

// 0x4F25E0
void target_params_init(TargetParams* params)
{
    params->tgt = Tgt_None;
    params->spell_flags = 0;
    params->no_spell_flags = 0;
    params->radius = 0;
    params->count = 0;
}

// 0x4F2600
void target_context_init(TargetContext* ctx, TargetParams* params, int64_t source_obj)
{
    ctx->orig_target_loc = 0;
    ctx->orig_target_obj = OBJ_HANDLE_NULL;
    ctx->field_40 = OBJ_HANDLE_NULL;
    ctx->source_obj = source_obj;
    ctx->self_obj = source_obj;
    ctx->params = params;
    ctx->obj_list = NULL;
    ctx->summoned_obj_list = NULL;
    ctx->targets = NULL;
    if (source_obj != OBJ_HANDLE_NULL) {
        ctx->source_loc = obj_field_int64_get(source_obj, OBJ_F_LOCATION);
    }
    ctx->summoned_obj = OBJ_HANDLE_NULL;
    ctx->target_loc = 0;
    ctx->target_obj = OBJ_HANDLE_NULL;
    ctx->target_spell_flags = 0;

    if (params != NULL) {
        target_params_init(params);
    }
}

// 0x4F2680
bool sub_4F2680(S4F2680* a1)
{
    stru_603CB8.source_obj = a1->field_0;

    if (a1->td->is_loc) {
        stru_603CB8.orig_target_loc = a1->td->loc;
        stru_603CB8.target_loc = a1->td->loc;
        stru_603CB8.target_obj = OBJ_HANDLE_NULL;
        stru_603CB8.orig_target_obj = OBJ_HANDLE_NULL;
    } else {
        if ((obj_field_int32_get(a1->td->obj, OBJ_F_FLAGS) & OF_CLICK_THROUGH) != 0) {
            return false;
        }

        stru_603CB8.orig_target_obj = a1->td->obj;
        stru_603CB8.target_obj = a1->td->obj;
        stru_603CB8.target_loc = OBJ_HANDLE_NULL;
        stru_603CB8.orig_target_loc = OBJ_HANDLE_NULL;
    }

    stru_603CB8.self_obj = a1->field_8;

    if (target_context_evaluate(&stru_603CB8)) {
        if (stru_603D20.tgt) {
            return true;
        }
        return false;
    }

    if (!a1->td->is_loc) {
        if (a1->td->obj != OBJ_HANDLE_NULL) {
            stru_603CB8.target_loc = obj_field_int64_get(a1->td->obj, OBJ_F_LOCATION);
            stru_603CB8.orig_target_loc = stru_603CB8.target_loc;
        }
        if (target_context_evaluate(&stru_603CB8) && stru_603D20.tgt) {
            target_descriptor_set_loc(a1->td, stru_603CB8.target_loc);
            return true;
        }
    }

    return false;
}

/**
 * Sets a `TargetDescriptor` to reference a world location.
 *
 * 0x4F27F0
 */
void target_descriptor_set_loc(TargetDescriptor* td, int64_t loc)
{
    td->loc = loc;
    td->is_loc = 1;
}

/**
 * Sets a `TargetDescriptor` to reference a game object.
 *
 * 0x4F2810
 */
void target_descriptor_set_obj(TargetDescriptor* td, int64_t obj)
{
    td->obj = obj;
    td->is_loc = 0;
}

/**
 * Resolves a target from a raw mouse position using current targeting flags
 *
 * Returns `true` and populates target descriptor, `false` otherwise.
 *
 * CE: Signature is slightly changed. Original code accepts raw mouse event
 * instance.
 *
 * 0x4F2830
 */
bool target_pick_at_screen_xy(int x, int y, TargetDescriptor* td, bool fullscreen)
{
    if (x < target_iso_content_rect.x
        || x >= target_iso_content_rect.x + target_iso_content_rect.width
        || y < target_iso_content_rect.y
        || y >= target_iso_content_rect.y + target_iso_content_rect.height) {
        return false;
    }

    x -= target_iso_content_rect.x;
    if (!fullscreen) {
        y -= target_iso_content_rect.y;
    }

    return sub_4F28A0(x, y, td);
}

// 0x4F28A0
bool sub_4F28A0(int x, int y, TargetDescriptor* td)
{
    int64_t pc_obj;
    S4F2680 v1;
    ObjectList party_members;
    ObjectList mp_party_members;
    ObjectList dead_critters;
    ObjectList objects;
    ObjectNode* node;
    int64_t v2;
    int64_t loc;
    bool ret = false;

    pc_obj = player_get_local_pc_obj();

    // Initialize heads to silence compiler warnings.
    party_members.head = NULL;
    mp_party_members.head = NULL;
    dead_critters.head = NULL;
    objects.head = NULL;

    v1.field_0 = pc_obj;
    v1.field_8 = pc_obj;
    v1.td = td;

    if ((stru_603D20.tgt & Tgt_No_Self) != 0) {
        object_flags_set(pc_obj, OF_CLICK_THROUGH);
    }

    if ((stru_603D20.tgt & Tgt_Non_Party) != 0) {
        object_list_all_followers(pc_obj, &party_members);
        node = party_members.head;
        while (node != NULL) {
            object_flags_set(node->obj, OF_CLICK_THROUGH);
            node = node->next;
        }

        if (tig_net_is_host()) {
            object_list_party(pc_obj, &mp_party_members);
            node = mp_party_members.head;
            while (node != NULL) {
                object_flags_set(node->obj, OF_CLICK_THROUGH);
                node = node->next;
            }
        }
    }

    if ((stru_603D20.tgt & Tgt_No_ST_Critter_Dead) != 0) {
        object_list_vicinity(pc_obj, OBJ_TM_PC | OBJ_TM_NPC, &dead_critters);
        node = mp_party_members.head;
        while (node != NULL) {
            if (critter_is_dead(node->obj)) {
                object_flags_set(node->obj, OF_CLICK_THROUGH);
            }
            node = node->next;
        }
    }

    if (sub_43D9F0(x, y, &v2, 0x3)) {
        sub_4F2C60(&v2);
        target_descriptor_set_obj(td, v2);
        if (sub_4F2680(&v1)) {
            ret = true;
        }
    }

    if (!ret) {
        if (location_at(x, y, &loc)) {
            object_list_location(loc, OBJ_TM_ALL & ~OBJ_TM_PROJECTILE, &objects);
            node = objects.head;
            while (node != NULL) {
                v2 = node->obj;
                sub_4F2C60(&v2);
                target_descriptor_set_obj(td, v2);
                if (sub_4F2680(&v1)) {
                    ret = true;
                    break;
                }
                node = node->next;
            }
            object_list_destroy(&objects);

            if (!ret) {
                target_descriptor_set_loc(td, loc);
                if (sub_4F2680(&v1)) {
                    ret = true;
                }
            }
        }
    }

    if ((stru_603D20.tgt & Tgt_No_Self) != 0) {
        object_flags_unset(pc_obj, OF_CLICK_THROUGH);
    }

    if ((stru_603D20.tgt & Tgt_Non_Party) != 0) {
        node = party_members.head;
        while (node != NULL) {
            object_flags_unset(node->obj, OF_CLICK_THROUGH);
            node = node->next;
        }

        if (tig_net_is_host()) {
            node = mp_party_members.head;
            while (node != NULL) {
                object_flags_unset(node->obj, OF_CLICK_THROUGH);
                node = node->next;
            }
        }
    }

    if ((stru_603D20.tgt & Tgt_No_ST_Critter_Dead) != 0) {
        node = mp_party_members.head;
        while (node != NULL) {
            if (critter_is_dead(node->obj)) {
                object_flags_unset(node->obj, OF_CLICK_THROUGH);
            }
            node = node->next;
        }
    }

    if (!ret) {
        target_descriptor_set_obj(td, OBJ_HANDLE_NULL);
        return false;
    }

    return true;
}

// 0x4F2C60
int sub_4F2C60(int64_t* obj_ptr)
{
    int type;
    int64_t whos_in_me_obj;

    type = obj_field_int32_get(*obj_ptr, OBJ_F_TYPE);
    if (type == OBJ_TYPE_SCENERY) {
        whos_in_me_obj = obj_field_handle_get(*obj_ptr, OBJ_F_SCENERY_WHOS_IN_ME);
        if (whos_in_me_obj != OBJ_HANDLE_NULL) {
            *obj_ptr = whos_in_me_obj;
            return OBJ_TYPE_NPC;
        }
    }

    return type;
}

/**
 * Resolves a target from a given mouse coordinates using the specified
 * targeting flags.
 *
 * This function is an extendend variant of `target_pick_at_screen_xy`, it
 * bypasses targeting rect validation, and accepts targeting flags directly.
 *
 * Returns `true` and populates target descriptor, `false` otherwise.
 *
 * 0x4F2CB0
 */
bool target_pick_at_screen_xy_ex(int x, int y, TargetDescriptor* td, uint64_t tgt, bool fullscreen)
{
    uint64_t old_tgt;
    bool rc;

    x -= target_iso_content_rect.x;
    if (!fullscreen) {
        y -= target_iso_content_rect.y;
    }

    old_tgt = target_flags_get();
    target_flags_set(tgt);
    rc = sub_4F28A0(x, y, td);
    target_flags_set(old_tgt);
    return rc;
}

/**
 * Returns the targeting flag that caused the most recent targeting attempt
 * rejection.
 *
 * 0x4F2D10
 */
uint64_t target_last_rejection_get(void)
{
    return target_last_rejection;
}

/**
 * Tests whether the candidate described by `ctx` satisfies all targeting
 * constraints.
 *
 * Returns `true` if the candidate passes all applicable checks, `false`
 * otherwise.
 *
 * 0x4F2D20
 */
bool target_context_evaluate(TargetContext* ctx)
{
    uint64_t tgt;
    ObjectList objects;
    ObjectNode* node;
    int obj_type;
    unsigned int flags;
    unsigned int spell_flags;
    unsigned int critter_flags;

    tgt = ctx->params->tgt;

    if (tgt == Tgt_None) {
        // No constraints: everything is a valid target.
        return true;
    }

    // Object validation pass.
    if ((tgt & (Tgt_Obj_Radius | Tgt_Object | Tgt_Self)) != 0) {
        if (ctx->target_obj != OBJ_HANDLE_NULL) {
            obj_type = obj_field_int32_get(ctx->target_obj, OBJ_F_TYPE);
            flags = obj_field_int32_get(ctx->target_obj, OBJ_F_FLAGS);

            // Reject objects that are destroyed or switched off (unless the
            // objects is a critter marked with `OCF2_SAFE_OFF`).
            if ((flags & (OF_DESTROYED | OF_OFF)) != 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }

                if ((obj_field_int32_get(ctx->target_obj, OBJ_F_CRITTER_FLAGS2) & OCF2_SAFE_OFF) == 0) {
                    return false;
                }
            }

            // Reject objects that are drawn (unless they are sleeping
            // critters).
            if ((flags & OF_DONTDRAW) != 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }

                if (!critter_is_sleeping(ctx->target_obj)) {
                    return false;
                }
            }

            spell_flags = obj_field_int32_get(ctx->target_obj, OBJ_F_SPELL_FLAGS);

            // Enforce required spell effects to be present on target.
            if ((spell_flags & ctx->params->spell_flags) != ctx->params->spell_flags) {
                return false;
            }

            // Enforce forbidden spell effects not to be present on target.
            if ((spell_flags & ctx->params->no_spell_flags) != 0) {
                return false;
            }

            if ((tgt & 0x9) != 0) {
                if ((tgt & 0x10000000) == 0
                    && ctx->target_obj != ctx->self_obj) {
                    return false;
                }
            } else {
                if ((tgt & 0x10000000) != 0) {
                    if (ctx->target_obj == ctx->self_obj) {
                        return false;
                    }
                    if (ctx->target_obj == ctx->source_obj) {
                        return false;
                    }
                }
            }

            if ((tgt & Tgt_Source) != 0
                && (tgt & 0x10000000) == 0
                && ctx->target_obj != ctx->source_obj) {
                return false;
            }

            if ((tgt & 0x40) != 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }

                if ((tgt & 0x20) != 0
                    && obj_type != OBJ_TYPE_PC) {
                    return false;
                }

                if ((tgt & 0x20000000) != 0
                    && obj_type == OBJ_TYPE_PC) {
                    return false;
                }

                if ((tgt & 0x100) != 0) {
                    if (!critter_is_dead(ctx->target_obj)) {
                        return false;
                    }
                } else if ((tgt & 0x80000000) != 0) {
                    if (critter_is_dead(ctx->target_obj)) {
                        return false;
                    }
                }

                critter_flags = obj_field_int32_get(ctx->target_obj, OBJ_F_CRITTER_FLAGS);
                if ((tgt & 0x80) != 0) {
                    if ((critter_flags & OCF_ANIMAL) == 0) {
                        return false;
                    }
                } else if ((tgt & 0x40000000) != 0) {
                    if ((critter_flags & OCF_ANIMAL) != 0) {
                        return false;
                    }
                }

                if ((tgt & 0x200) != 0
                    && (critter_flags & OCF_UNDEAD) == 0) {
                    return false;
                }

                if ((tgt & 0x400) != 0
                    && (critter_flags & OCF_DEMON) == 0) {
                    return false;
                }

                if ((tgt & 0x800) != 0
                    && (critter_flags & OCF_MECHANICAL) == 0) {
                    return false;
                }

                if ((tgt & 0x1000) != 0) {
                    if (stat_level_get(ctx->target_obj, STAT_ALIGNMENT) < 0) {
                        return false;
                    }
                } else if ((tgt & 0x2000) != 0) {
                    if (stat_level_get(ctx->target_obj, STAT_ALIGNMENT) >= 0) {
                        return false;
                    }
                }

                if ((tgt & 0x4000) != 0
                    && (critter_flags & OCF_UNREVIVIFIABLE) == 0) {
                    return false;
                }

                if ((tgt & 0x8000) != 0
                    && (critter_flags & OCF_UNRESSURECTABLE) == 0) {
                    return false;
                }

                if ((tgt & 0x100000000) != 0
                    && (critter_flags & OCF_UNDEAD) != 0) {
                    return false;
                }

                if ((tgt & 0x200000000) != 0
                    && (critter_flags & OCF_DEMON) != 0) {
                    return false;
                }

                if ((tgt & 0x400000000) != 0
                    && (critter_flags & OCF_MECHANICAL) != 0) {
                    return false;
                }

                if ((tgt & 0x800000000) != 0) {
                    if (stat_level_get(ctx->target_obj, STAT_ALIGNMENT) >= 0) {
                        return false;
                    }
                } else if ((tgt & 0x1000000000) != 0) {
                    if (stat_level_get(ctx->target_obj, STAT_ALIGNMENT) < 0) {
                        return false;
                    }
                }

                if ((tgt & 0x2000000000) != 0
                    && (critter_flags & OCF_UNREVIVIFIABLE) != 0) {
                    return false;
                }

                if ((tgt & 0x4000000000) != 0
                    && (critter_flags & OCF_UNRESSURECTABLE) != 0) {
                    return false;
                }

                if ((tgt & 0x400000) != 0) {
                    if (stat_level_get(ctx->target_obj, STAT_POISON_LEVEL) <= 0) {
                        return false;
                    }
                }

                if ((tgt & 0xC00000000000000) != 0) {
                    if (critter_pc_leader_get(ctx->target_obj) == OBJ_HANDLE_NULL
                        && !player_is_pc_obj(ctx->target_obj)) {
                        return false;
                    }
                } else if ((tgt & 0x1000000000000000) != 0) {
                    // TODO: Looks the same as the code below, probably one of
                    // it should be inverted, check.
                    if (ctx->self_obj != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(ctx->target_obj);
                        v2 = critter_pc_leader_get(ctx->self_obj);
                        if (v1 == ctx->self_obj
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == ctx->target_obj) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(ctx->self_obj, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == ctx->target_obj) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }

                    if (ctx->field_40 != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(ctx->target_obj);
                        v2 = critter_pc_leader_get(ctx->field_40);
                        if (v1 == ctx->field_40
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == ctx->target_obj
                            || ctx->target_obj == ctx->field_40) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(ctx->field_40, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == ctx->target_obj) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }
                }
            } else if ((tgt & 0x10000) != 0) {
                if (obj_type != OBJ_TYPE_PORTAL) {
                    // TODO: Check, looks odd.
                    if ((tgt & 0x20000) == 0) {
                        return false;
                    }
                    if (obj_type != OBJ_TYPE_CONTAINER) {
                        return false;
                    }
                }
            } else if ((tgt & 0x20000) != 0) {
                if (obj_type != OBJ_TYPE_CONTAINER) {
                    return false;
                }
            } else if ((tgt & 0x80000) != 0) {
                if (obj_type != OBJ_TYPE_WALL) {
                    return false;
                }
            } else if ((tgt & 0x20000000000) != 0) {
                if (obj_type == OBJ_TYPE_WALL) {
                    return false;
                }
            }

            if ((tgt & 0x40) == 0) {
                if ((tgt & 0x80000000) != 0
                    && critter_is_dead(ctx->target_obj)) {
                    return false;
                }

                if ((tgt & 0x1000000000000000) != 0
                    && obj_type_is_critter(obj_type)) {
                    // TODO: Looks the same as the code above, probably one of
                    // it should be inverted, check.
                    if (ctx->self_obj != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(ctx->target_obj);
                        v2 = critter_pc_leader_get(ctx->self_obj);
                        if (v1 == ctx->self_obj
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == ctx->target_obj) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(ctx->self_obj, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == ctx->target_obj) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }

                    if (ctx->field_40 != OBJ_HANDLE_NULL) {
                        int64_t v1;
                        int64_t v2;

                        v1 = critter_pc_leader_get(ctx->target_obj);
                        v2 = critter_pc_leader_get(ctx->field_40);
                        if (v1 == ctx->field_40
                            || (v1 == v2 && v1 != OBJ_HANDLE_NULL)
                            || v2 == ctx->target_obj
                            || ctx->target_obj == ctx->field_40) {
                            return false;
                        }

                        if (tig_net_is_active()
                            && obj_field_int32_get(v2, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                            object_list_party(ctx->field_40, &objects);
                            node = objects.head;
                            while (node != NULL) {
                                if (node->obj == v1
                                    || node->obj == ctx->target_obj) {
                                    break;
                                }
                                node = node->next;
                            }
                            object_list_destroy(&objects);

                            if (node != NULL) {
                                return false;
                            }
                        }
                    }
                }
            }

            if ((tgt & 0x100000) != 0) {
                if (object_hp_damage_get(ctx->target_obj) <= 0
                    && (!obj_type_is_critter(obj_type)
                        || (obj_field_int32_get(ctx->target_obj, OBJ_F_CRITTER_FLAGS) & OCF_INJURED) == 0)) {
                    target_last_rejection = 0x100000;
                    return false;
                }
            } else if ((tgt & 0x40000000000) != 0) {
                if (object_hp_damage_get(ctx->target_obj) > 0
                    && (!obj_type_is_critter(obj_type)
                        || (obj_field_int32_get(ctx->target_obj, OBJ_F_CRITTER_FLAGS) & OCF_INJURED) != 0)) {
                    target_last_rejection = 0x40000000000;
                    return false;
                }
            }

            if ((tgt & 0x40000) != 0) {
                if (obj_type == OBJ_TYPE_PORTAL) {
                    if ((obj_field_int32_get(ctx->target_obj, OBJ_F_PORTAL_FLAGS) & OPF_LOCKED) == 0) {
                        return false;
                    }
                } else if (obj_type == OBJ_TYPE_CONTAINER) {
                    if ((obj_field_int32_get(ctx->target_obj, OBJ_F_CONTAINER_FLAGS) & OCOF_LOCKED) == 0) {
                        return false;
                    }
                }
            } else if ((tgt & 0x8000000000) != 0) {
                if (obj_type == OBJ_TYPE_PORTAL) {
                    if ((obj_field_int32_get(ctx->target_obj, OBJ_F_PORTAL_FLAGS) & OPF_LOCKED) != 0) {
                        return false;
                    }
                } else if (obj_type == OBJ_TYPE_CONTAINER) {
                    if ((obj_field_int32_get(ctx->target_obj, OBJ_F_CONTAINER_FLAGS) & OCOF_LOCKED) != 0) {
                        return false;
                    }
                }
            }

            if ((tgt & 0x10000000000) != 0) {
                if (obj_type == OBJ_TYPE_PORTAL) {
                    if ((obj_field_int32_get(ctx->target_obj, OBJ_F_PORTAL_FLAGS) & OPF_MAGICALLY_HELD) != 0) {
                        return false;
                    }
                } else if (obj_type == OBJ_TYPE_CONTAINER) {
                    if ((obj_field_int32_get(ctx->target_obj, OBJ_F_CONTAINER_FLAGS) & OCOF_MAGICALLY_HELD) != 0) {
                        return false;
                    }
                }
            }

            if ((tgt & 0x200000) != 0
                && object_hp_damage_get(ctx->target_obj) <= 0) {
                if (!obj_type_is_critter(obj_type)) {
                    return false;
                }
                if (stat_level_get(ctx->target_obj, STAT_POISON_LEVEL) <= 0) {
                    return false;
                }
            }

            if ((tgt & 0x1000000) != 0
                && obj_field_int32_get(ctx->target_obj, OBJ_F_MATERIAL) != MATERIAL_FLESH) {
                return false;
            }

            if ((tgt & 0x10) != 0
                && ctx->target_obj != OBJ_HANDLE_NULL) {
                int64_t loc1;
                int64_t loc2;

                loc1 = ctx->orig_target_loc;
                if (loc1 == 0) {
                    if (ctx->source_obj == OBJ_HANDLE_NULL) {
                        return false;
                    }

                    loc1 = obj_field_int64_get(ctx->source_obj, OBJ_F_LOCATION);
                }

                loc2 = obj_field_int64_get(ctx->target_obj, OBJ_F_LOCATION);
                if (location_dist(loc1, loc2) > ctx->params->radius) {
                    return false;
                }
            }

            if ((tgt & 0x400000000000) != 0
                && (spell_flags & OSF_SUMMONED) == 0) {
                return false;
            }

            if ((tgt & 0x100002000000) != 0) {
                if ((tgt & 0x2000000) != 0) {
                    if ((flags & OF_INVENTORY) == 0) {
                        return false;
                    }
                } else if ((tgt & 0x100000000000) != 0) {
                    if ((flags & OF_INVENTORY) != 0) {
                        return false;
                    }
                }
            }

            if ((tgt & 0x200000000000) != 0
                && (flags & OF_INVULNERABLE) != 0) {
                return false;
            }

            if ((tgt & 0x4000000) != 0) {
                return obj_type_is_item(obj_type)
                    ? obj_field_int32_get(ctx->target_obj, OBJ_F_ITEM_WEIGHT) < 5
                    : false;
            }

            return true;
        }

        if ((tgt & 0x8000800000000000) == 0) {
            return false;
        }
    }

    if ((tgt & 0x800000000000) == 0) {
        return true;
    }

    if (ctx->target_loc == 0) {
        return (tgt & 0x8000000000000000) != 0;
    }

    if ((tgt & 0x1000000000000) != 0) {
        if (ctx->source_obj == OBJ_HANDLE_NULL
            || ctx->target_loc != obj_field_int64_get(ctx->target_obj, OBJ_F_LOCATION)) {
            return false;
        }
    }

    if ((tgt & 0x20000) != 0
        && ctx->source_obj != OBJ_HANDLE_NULL) {
        target_path_create_info.obj = ctx->source_obj;
        target_path_create_info.from = obj_field_int64_get(ctx->source_obj, OBJ_F_LOCATION);
        target_path_create_info.to = ctx->target_loc;
        target_path_create_info.max_rotations = sizeof(target_path_rotations);
        target_path_create_info.rotations = target_path_rotations;
        target_path_create_info.flags = PATH_FLAG_0x0010;

        target_path_length = path_make(&target_path_create_info);
        target_path_from = target_path_create_info.from;
        target_path_to = target_path_create_info.to;

        if (target_path_length == 0) {
            return false;
        }
    }

    if ((tgt & 0x4000000000000) != 0) {
        bool v51 = true;

        if (tile_is_blocking(ctx->target_loc, false)) {
            return false;
        }

        object_list_location(ctx->target_loc, OBJ_TM_ALL & ~OBJ_TM_PROJECTILE, &objects);
        node = objects.head;
        while (node != NULL) {
            if (node->obj != OBJ_HANDLE_NULL) {
                if (!obj_type_is_item(obj_field_int32_get(node->obj, OBJ_F_TYPE))
                    && (obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_NO_BLOCK) == 0) {
                    v51 = false;
                    break;
                }
            }
            node = node->next;
        }
        object_list_destroy(&objects);

        if (!v51) {
            return false;
        }
    }

    if ((tgt & 0x8000000000000) != 0) {
        bool v54 = true;

        if (tile_is_blocking(ctx->target_loc, false)) {
            return false;
        }

        object_list_location(ctx->target_loc, OBJ_TM_TRAP | OBJ_TM_SCENERY | OBJ_TM_PORTAL | OBJ_TM_WALL, &objects);
        node = objects.head;
        while (node != NULL) {
            if (node->obj != OBJ_HANDLE_NULL) {
                if (!obj_type_is_item(obj_field_int32_get(node->obj, OBJ_F_TYPE))
                    && (obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_NO_BLOCK) == 0) {
                    v54 = false;
                    break;
                }
            }
            node = node->next;
        }
        object_list_destroy(&objects);

        if (!v54) {
            return false;
        }
    }

    if ((tgt & 0x20000000000000) != 0
        // TODO: Sames args looks wrong, check.
        && location_dist(ctx->orig_target_loc, ctx->orig_target_loc) > ctx->params->radius) {
        return false;
    }

    if ((tgt & 0x100000000000000) != 0
        && ctx->source_loc != 0
        && ctx->target_loc != 0) {
        if (tig_art_tile_id_type_get(tile_art_id_at(ctx->target_loc)) != tig_art_tile_id_type_get(tile_art_id_at(ctx->source_loc))) {
            return false;
        }
    }

    return true;
}

/**
 * Adds an object to the target list, unless the object is destroyed or switched
 * off.
 *
 * 0x4F3F10
 */
void target_list_add_obj_unless_off(TargetList* list, int64_t obj)
{
    int idx;

    // Reject null handles.
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Reject objects that are destroyed or switched off (unless the object is
    // a critter marked with `OCF2_SAFE_OFF`).
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & (OF_DESTROYED | OF_OFF)) != 0) {
        if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
            return;
        }

        if ((obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2) & OCF2_SAFE_OFF) == 0) {
            return;
        }
    }

    // Check if this object is already on the list.
    for (idx = 0; idx < list->cnt; idx++) {
        if (list->entries[idx].obj == obj) {
            return;
        }
    }

    // Append object to the list.
    list->entries[list->cnt].obj = obj;
    list->entries[list->cnt].loc = 0;
    list->cnt++;
}

/**
 * Adds an object to the target list, unless the object is destroyed.
 *
 * 0x4F3FD0
 */
void target_list_add_obj_unless_destroyed(TargetList* list, int64_t obj)
{
    int idx;

    // Reject null handles.
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Reject objects that are destroyed.
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return;
    }

    // Check if this object is already on the list.
    for (idx = 0; idx < list->cnt; idx++) {
        if (list->entries[idx].obj == obj) {
            return;
        }
    }

    // Append object to the list.
    list->entries[list->cnt].obj = obj;
    list->entries[list->cnt].loc = 0;
    list->cnt++;
}

/**
 * Adds a world location to the target list (unless it's already in the list).
 *
 * 0x4F4050
 */
void target_list_add_loc(TargetList* list, int64_t loc)
{
    int idx;

    // Check if this location is already on the list.
    for (idx = 0; idx < list->cnt; idx++) {
        if (list->entries[idx].loc == loc) {
            return;
        }
    }

    // Append location to the list.
    list->entries[list->cnt].obj = OBJ_HANDLE_NULL;
    list->entries[list->cnt].loc = loc;
    list->cnt++;
}

/**
 * Collects all valid targets into `ctx->targets`.
 *
 * 0x4F40B0
 */
void target_context_build_list(TargetContext* ctx)
{
    TargetList* targets;
    TargetParams* target_params;
    TargetContext tmp_target_ctx;
    TargetParams tmp_target_params;
    ObjectList objects;
    ObjectNode* obj_node;
    MagicTechObjectNode* mt_obj_node;
    int idx;
    int64_t origin;

    targets = ctx->targets;
    if (targets == NULL) {
        return;
    }

    targets->cnt = 0;
    target_params = ctx->params;

    if ((target_params->tgt & Tgt_Self) != 0
        && (target_params->tgt & Tgt_No_Self) == 0) {
        target_list_add_obj_unless_off(targets, ctx->self_obj);
    }

    if ((target_params->tgt & Tgt_Source) != 0
        && (target_params->tgt & Tgt_No_Self) == 0) {
        target_list_add_obj_unless_off(targets, ctx->source_obj);
    }

    if ((target_params->tgt & Tgt_Object) != 0 && ctx->orig_target_obj != OBJ_HANDLE_NULL) {
        target_list_add_obj_unless_destroyed(targets, ctx->orig_target_obj);
    }

    if ((target_params->tgt & Tgt_Summoned_No_Obj) != 0) {
        if (ctx->summoned_obj != OBJ_HANDLE_NULL) {
            target_list_add_obj_unless_destroyed(targets, ctx->summoned_obj);
        }

        if (ctx->summoned_obj_list != NULL) {
            mt_obj_node = *ctx->summoned_obj_list;
            while (mt_obj_node != NULL) {
                target_list_add_obj_unless_destroyed(targets, mt_obj_node->obj);
                mt_obj_node = mt_obj_node->next;
            }
        }
    }

    if ((target_params->tgt & Tgt_Tile) != 0 && ctx->orig_target_loc != 0) {
        target_list_add_loc(targets, ctx->orig_target_loc);
    }

    if ((target_params->tgt & Tgt_Tile_Self) != 0 && ctx->source_obj != OBJ_HANDLE_NULL) {
        target_list_add_loc(targets, obj_field_int64_get(ctx->source_obj, OBJ_F_LOCATION));
    }

    if ((target_params->tgt & Tgt_Obj_Radius) != 0) {
        LocRect loc_rect;
        unsigned int obj_type_mask;
        bool all;

        target_context_init(&tmp_target_ctx, &tmp_target_params, ctx->source_obj);
        origin = ctx->orig_target_loc;
        tmp_target_ctx.orig_target_loc = ctx->orig_target_loc;
        tmp_target_ctx.field_40 = ctx->field_40;

        tmp_target_params.tgt = target_params->tgt & ~Tgt_Tile;
        tmp_target_params.spell_flags = target_params->spell_flags;
        tmp_target_params.no_spell_flags = target_params->no_spell_flags;
        tmp_target_params.radius = target_params->radius;
        tmp_target_params.count = target_params->count;

        if (ctx->orig_target_obj != OBJ_HANDLE_NULL) {
            origin = obj_field_int64_get(ctx->orig_target_obj, OBJ_F_LOCATION);
        }

        loc_rect.x1 = location_get_x(origin) - target_params->radius;
        loc_rect.y1 = location_get_y(origin) - target_params->radius;
        loc_rect.x2 = location_get_x(origin) + target_params->radius;
        loc_rect.y2 = location_get_y(origin) + target_params->radius;

        obj_type_mask = OBJ_TM_SCENERY;
        all = false;

        if ((target_params->tgt & Tgt_Obj_T_Critter_Naked) != 0) {
            obj_type_mask |= OBJ_TM_CRITTER;
        }

        if ((target_params->tgt & Tgt_Obj_T_Portal_Naked) != 0) {
            obj_type_mask |= OBJ_TM_PORTAL;
        }

        if ((target_params->tgt & Tgt_Obj_T_Container_Naked) != 0) {
            obj_type_mask |= OBJ_TM_CONTAINER;
        }

        if ((target_params->tgt & Tgt_Obj_T_Wall_Naked) != 0) {
            obj_type_mask |= OBJ_TM_WALL;
        }

        if (obj_type_mask == OBJ_TM_SCENERY
            && (target_params->tgt & (Tgt_Obj_Radius | Tgt_Object)) != 0) {
            obj_type_mask = OBJ_TM_ALL & ~OBJ_TM_PROJECTILE;
            all = true;
        }

        object_list_rect(&loc_rect, obj_type_mask, &objects);
        obj_node = objects.head;
        while (obj_node != NULL) {
            int64_t tmp_obj = obj_node->obj;
            if (sub_4F2C60(&tmp_obj) != OBJ_TYPE_SCENERY || all) {
                tmp_target_ctx.target_obj = tmp_obj;
                if (target_context_evaluate(&tmp_target_ctx)) {
                    target_list_add_obj_unless_off(targets, tmp_obj);

                    if (tmp_target_params.count > 0) {
                        if (--tmp_target_params.count == 0) {
                            break;
                        }
                    }
                }
            }
            obj_node = obj_node->next;
        }
        object_list_destroy(&objects);
    }

    if ((target_params->tgt & Tgt_Tile_Radius_Naked) != 0) {
        int x;
        int y;
        bool done;

        target_context_init(&tmp_target_ctx, &tmp_target_params, ctx->source_obj);
        tmp_target_params.tgt = target_params->tgt & ~Tgt_Object;
        tmp_target_params.radius = target_params->radius;
        tmp_target_params.count = target_params->count;

        origin = ctx->orig_target_loc;
        if (origin == 0) {
            origin = obj_field_int64_get(ctx->orig_target_obj, OBJ_F_LOCATION);
        }

        done = false;
        for (y = -target_params->radius; y <= target_params->radius; y++) {
            for (x = -target_params->radius; x <= target_params->radius; x++) {
                tmp_target_ctx.target_loc = location_make(location_get_x(origin) + x, location_get_y(origin) + y);
                if (target_context_evaluate(&tmp_target_ctx)) {
                    target_list_add_loc(targets, tmp_target_ctx.target_loc);

                    if (tmp_target_params.count > 0) {
                        if (--tmp_target_params.count == 0) {
                            // To break out of the outer loop.
                            done = true;
                            break;
                        }
                    }
                }
            }

            // NOTE: There is a bug in the original code related to tracking
            // count. It checks for `count > 0 && count == 0` to break out of
            // this outer loop, which is obviously wrong.
            if (done) {
                break;
            }
        }
    }

    if ((target_params->tgt & Tgt_Tile_Radius_Wall_Naked) != 0) {
        int rot;
        int sx;
        int sy;
        int range;
        int dx1;
        int dy1;
        int dx2;
        int dy2;

        target_context_init(&tmp_target_ctx, &tmp_target_params, ctx->source_obj);
        tmp_target_ctx.field_40 = ctx->field_40;
        tmp_target_params.tgt = target_params->tgt & ~Tgt_Object;
        tmp_target_params.radius = target_params->radius;
        tmp_target_params.count = target_params->count;

        origin = ctx->orig_target_loc;
        if (origin == 0) {
            origin = obj_field_int64_get(ctx->orig_target_obj, OBJ_F_LOCATION);
        }

        if (ctx->source_loc != 0 && ctx->orig_target_loc != 0) {
            rot = location_rot(ctx->source_loc, ctx->orig_target_loc);
        } else {
            rot = 0;
        }

        switch (rot) {
        case 1:
            sx = 0;
            sy = -1;
            break;
        case 2:
            sx = 0;
            sy = -1;
            break;
        case 3:
            sx = -1;
            sy = 0;
            break;
        case 4:
            sx = -1;
            sy = 1;
            break;
        case 5:
            sx = 0;
            sy = 1;
            break;
        case 6:
            sx = 1;
            sy = 1;
            break;
        case 7:
            sx = 1;
            sy = 0;
            break;
        default:
            sx = 1;
            sy = -1;
            break;
        }

        dx1 = 0;
        dy1 = 0;
        dx2 = 0;
        dy2 = 0;
        for (range = 0; range < target_params->radius; range++) {
            tmp_target_ctx.target_loc = location_make(location_get_x(origin) + dx1, location_get_y(origin) + dy1);
            if (target_context_evaluate(&tmp_target_ctx)) {
                target_list_add_loc(targets, tmp_target_ctx.target_loc);
                if (tmp_target_params.count > 0) {
                    if (--tmp_target_params.count == 0) {
                        break;
                    }
                }
            }

            tmp_target_ctx.target_loc = location_make(location_get_x(origin) + dx2, location_get_y(origin) + dy2);
            if (target_context_evaluate(&tmp_target_ctx)) {
                target_list_add_loc(targets, tmp_target_ctx.target_loc);
                if (tmp_target_params.count > 0) {
                    if (--tmp_target_params.count == 0) {
                        break;
                    }
                }
            }

            dx1 += sx;
            dy1 += sy;
            dx2 -= sx;
            dy2 -= sy;
        }
    }

    if ((target_params->tgt & Tgt_Tile_Offscreen_Naked) != 0 && ctx->target_obj != OBJ_HANDLE_NULL) {
        target_list_add_loc(targets, obj_field_int64_get(ctx->target_obj, OBJ_F_LOCATION));
    }

    // FIXME: The code below does not look like implementation of cone.
    if ((target_params->tgt & Tgt_Cone) != 0) {
        target_context_init(&tmp_target_ctx, &tmp_target_params, ctx->source_obj);
        tmp_target_ctx.field_40 = ctx->field_40;
        tmp_target_params.tgt = target_params->tgt & ~Tgt_Object;
        tmp_target_params.radius = target_params->radius;
        tmp_target_params.count = target_params->count;

        if (ctx->source_obj != OBJ_HANDLE_NULL) {
            origin = ctx->orig_target_loc;
            if (origin == 0) {
                origin = obj_field_int64_get(ctx->source_obj, OBJ_F_LOCATION);
            }

            if ((target_params->tgt & Tgt_Self) != 0) {
                unsigned int obj_type_mask;
                bool all;
                int x;
                int y;
                int64_t loc;
                bool done;

                obj_type_mask = OBJ_TM_SCENERY;
                all = false;

                if ((target_params->tgt & Tgt_Obj_T_Critter_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_CRITTER;
                }

                if ((target_params->tgt & Tgt_Obj_T_Portal_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_PORTAL;
                }

                if ((target_params->tgt & Tgt_Obj_T_Container_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_CONTAINER;
                }

                if ((target_params->tgt & Tgt_Obj_T_Wall_Naked) != 0) {
                    obj_type_mask |= OBJ_TM_WALL;
                }

                if (obj_type_mask == OBJ_TM_SCENERY
                    && (target_params->tgt & Tgt_Object) != 0) {
                    obj_type_mask = OBJ_TM_ALL & ~OBJ_TM_PROJECTILE;
                    all = true;
                }

                done = false;
                for (y = -target_params->radius; y <= target_params->radius; y++) {
                    for (x = -target_params->radius; x <= target_params->radius; x++) {
                        loc = location_make(location_get_x(origin) + x, location_get_y(origin) + y);
                        object_list_location(loc, obj_type_mask, &objects);
                        obj_node = objects.head;
                        while (obj_node != NULL) {
                            int64_t tmp_obj = obj_node->obj;
                            if (sub_4F2C60(&tmp_obj) != OBJ_TYPE_SCENERY
                                || all) {
                                tmp_target_ctx.target_obj = tmp_obj;
                                if (target_context_evaluate(&tmp_target_ctx)) {
                                    target_list_add_obj_unless_off(targets, tmp_obj);
                                    if (tmp_target_params.count > 0) {
                                        if (--tmp_target_params.count == 0) {
                                            done = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            obj_node = obj_node->next;
                        }
                        object_list_destroy(&objects);
                    }

                    // NOTE: There is a bug in the original code related to tracking
                    // count. It checks for `count > 0 && count == 0` to break out of
                    // this outer loop, which is obviously wrong.
                    if (done) {
                        break;
                    }
                }
            } else {
                int x;
                int y;

                for (y = -target_params->radius; y <= target_params->radius; y++) {
                    for (x = -target_params->radius; x <= target_params->radius; x++) {
                        tmp_target_ctx.target_loc = location_make(location_get_x(origin) + x, location_get_y(origin) + y);
                        if (target_context_evaluate(&tmp_target_ctx)) {
                            target_list_add_loc(targets, tmp_target_ctx.target_loc);

                            // FIXME: Missing count tracking.
                        }
                    }
                }
            }
        }
    }

    if ((target_params->tgt & Tgt_List) != 0 && ctx->obj_list != NULL) {
        mt_obj_node = *ctx->obj_list;
        if (mt_obj_node != NULL) {
            while (mt_obj_node != NULL) {
                target_list_add_obj_unless_destroyed(targets, mt_obj_node->obj);
                mt_obj_node = mt_obj_node->next;
            }
        } else {
            for (idx = 0; idx < targets->cnt; idx++) {
                // FIXME: Leaking node when object handle is null.
                mt_obj_node = mt_obj_node_create();
                if (targets->entries[idx].obj != OBJ_HANDLE_NULL) {
                    mt_obj_node->obj = targets->entries[idx].obj;
                    mt_obj_node->next = *ctx->obj_list;
                    sub_443EB0(mt_obj_node->obj, &(mt_obj_node->field_8));
                    if (mt_obj_node->obj != OBJ_HANDLE_NULL) {
                        mt_obj_node->type = obj_field_int32_get(mt_obj_node->obj, OBJ_F_TYPE);
                        if (obj_type_is_critter(mt_obj_node->type)) {
                            mt_obj_node->aptitude = stat_level_get(mt_obj_node->obj, STAT_MAGICK_TECH_APTITUDE);
                        }
                    }
                    *ctx->obj_list = mt_obj_node;
                }
            }
        }
    }

    if ((target_params->tgt & Tgt_All_Party_Critters_Naked) != 0 && ctx->source_obj != OBJ_HANDLE_NULL) {
        object_list_team(ctx->source_obj, &objects);
        obj_node = objects.head;
        while (obj_node != NULL) {
            target_list_add_obj_unless_destroyed(targets, obj_node->obj);
            obj_node = obj_node->next;
        }
        object_list_destroy(&objects);

        if (tig_net_is_active()
            && obj_field_int32_get(ctx->source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            object_list_party(ctx->source_obj, &objects);
            obj_node = objects.head;
            while (obj_node != NULL) {
                target_list_add_obj_unless_destroyed(targets, obj_node->obj);
                obj_node = obj_node->next;
            }
            object_list_destroy(&objects);
        }
    }

    if ((target_params->tgt & Tgt_Parent) != 0
        && obj_type_is_item(obj_field_int32_get(ctx->source_obj, OBJ_F_TYPE))) {
        int64_t parent_obj = obj_field_handle_get(ctx->source_obj, OBJ_F_ITEM_PARENT);
        if (parent_obj != OBJ_HANDLE_NULL) {
            target_list_add_obj_unless_destroyed(targets, parent_obj);
        }
    }
}

/**
 * Finds a valid landing tile for `obj` at approximately `distance` steps away
 * in a random direction.
 *
 * Returns `true` if a valid location was found, `false` otherwise.
 *
 * 0x4F4E40
 */
bool target_find_displacement_loc(int64_t obj, int distance, int64_t* loc_ptr)
{
    int64_t from;
    int64_t to;
    int obj_type;
    int rotation;
    int idx;
    ObjectList critters;
    ObjectNode* node;

    from = obj_field_int64_get(obj, OBJ_F_LOCATION);
    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    *loc_ptr = 0;
    to = from;

    // Pick random direction.
    rotation = random_between(0, 8);

    // "Walk" distance steps in that direction.
    for (idx = 0; idx < distance; idx++) {
        if (!location_in_dir(to, rotation, &to)) {
            return false;
        }
    }

    // Setup path info.
    target_path_create_info.obj = obj;
    target_path_create_info.from = from;
    target_path_create_info.to = to;
    target_path_create_info.max_rotations = sizeof(target_path_rotations);
    target_path_create_info.rotations = target_path_rotations;
    target_path_create_info.flags = PATH_FLAG_0x0010;

    target_path_length = path_make(&target_path_create_info);
    target_path_to = target_path_create_info.to;
    target_path_from = target_path_create_info.from;

    if ((target_path_length > 0 || target_path_create_info.field_24 > 0)
        && target_path_length < distance + 2) {
        if (target_path_create_info.field_24 == 0) {
            target_path_create_info.field_24 = target_path_length;
        }

        // Start with current location.
        to = from;

        // Advance along the path.
        target_path_step = 0;
        while (target_path_step < target_path_create_info.field_24) {
            if (obj_type_is_critter(obj_type)) {
                // Check if there are no critters on candidate loc.
                object_list_location(to, OBJ_TM_CRITTER, &critters);
                node = critters.head;
                object_list_destroy(&critters);
                if (node == NULL) {
                    // No critters, location found.
                    break;
                }
            }

            // Retrieve next location.
            if (!location_in_dir(to, target_path_rotations[target_path_step], &to)) {
                return false;
            }

            target_path_step++;
        }

        *loc_ptr = to;
        return true;
    }

    // Cannot plot path, return current location as result.
    *loc_ptr = from;
    return true;
}
