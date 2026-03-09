#include "game/animfx.h"

#include "game/anim.h"
#include "game/gsound.h"
#include "game/light.h"
#include "game/mes.h"
#include "game/object.h"
#include "game/teleport.h"

#define ANIMFX_LIST_CAPACITY 10
#define BLEND_COUNT 7
#define SCALE_COUNT 8

typedef enum EffectType {
    EFFECT_TYPE_TRANSLUCENCY,
    EFFECT_TYPE_TINTING,
    EFFECT_TYPE_COUNT,
} EffectType;

static bool sub_4CD9C0(AnimFxListEntry* entry, int64_t obj);
static void sub_4CDCD0(AnimFxList* list);
static bool animfx_list_load_internal(AnimFxList* list);
static void animfx_build_eye_candy_effect(int index, char* str);
static bool sub_4CE1A0(tig_art_id_t art_id, unsigned int flags, int index, const char* str);
static void sub_4CE2A0(int index);

// 0x5B7658
const char* animfx_play_flags_lookup_tbl_keys[ANIMFX_PLAY_COUNT] = {
    "reverse",
    "stack",
    "destroy",
    "callback",
    "end_callback",
    "random_start",
    "fire_dmg",
    "check_already",
    "no_id",
    "ice_dmg",
};

// 0x5B7680
unsigned int animfx_play_flags_lookup_tbl_values[ANIMFX_PLAY_COUNT] = {
    ANIMFX_PLAY_REVERSE,
    ANIMFX_PLAY_STACK,
    ANIMFX_PLAY_DESTROY,
    ANIMFX_PLAY_CALLBACK,
    ANIMFX_PLAY_END_CALLBACK,
    ANIMFX_PLAY_RANDOM_START,
    ANIMFX_PLAY_FIRE_DMG,
    ANIMFX_PLAY_CHECK_ALREADY,
    ANIMFX_PLAY_NO_ID,
    ANIMFX_PLAY_ICE_DMG,
};

// 0x5B76A8
static const char* off_5B76A8[] = {
    "overlay_f",
    "overlay_b",
    "overlay_fb",
    "underlay",
    "tinting",
    "animates",
    "anim_forever",
    "can_autoscale",
};

// 0x5B76C8
static unsigned int dword_5B76C8[] = {
    ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY,
    ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY,
    ANIMFX_LIST_ENTRY_ANY_OVERLAY,
    ANIMFX_LIST_ENTRY_UNDERLAY,
    ANIMFX_LIST_ENTRY_TINTING,
    ANIMFX_LIST_ENTRY_ANIMATES,
    ANIMFX_LIST_ENTRY_ANIMATES | ANIMFX_LIST_ENTRY_LOOPS,
    ANIMFX_LIST_ENTRY_CAN_AUTOSCALE,
};

static_assert(SDL_arraysize(off_5B76A8) == SDL_arraysize(dword_5B76C8), "wrong size");

// 0x5B76E8
static const char* off_5B76E8[] = {
    "none",
    "add",
    "sub",
    "mult",
    "alphC",
    "alphA",
    "alphS",
};

static_assert(SDL_arraysize(off_5B76E8) == BLEND_COUNT, "wrong size");

// 0x5B7704
static const char* off_5B7704[EFFECT_TYPE_COUNT] = {
    /* EFFECT_TYPE_TRANSLUCENCY */ "Translucency",
    /*      EFFECT_TYPE_TINTING */ "Tinting",
};

// 0x5B770C
static int dword_5B770C[] = {
    50,
    62,
    75,
    87,
    100,
    125,
    150,
    200,
};

static_assert(SDL_arraysize(dword_5B770C) == SCALE_COUNT, "wrong size");

// 0x5B772C
static int dword_5B772C[] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
};

static_assert(SDL_arraysize(dword_5B772C) == SCALE_COUNT, "wrong size");

// 0x601700
static AnimID stru_601700;

// 0x60170C
static AnimFxList* dword_60170C[ANIMFX_LIST_CAPACITY];

// 0x601734
static mes_file_handle_t dword_601734;

// 0x601738
static AnimFxList* dword_601738;

// 0x60173C
static int dword_60173C;

// 0x601740
static bool animfx_initialized;

// 0x4CCCF0
bool animfx_init(GameInitInfo* init_info)
{
    (void)init_info;

    animfx_initialized = true;

    return true;
}

// 0x4CCD00
void animfx_exit(void)
{
    if (animfx_initialized) {
        animfx_initialized = false;
    }
}

// 0x4CCD20
void sub_4CCD20(AnimFxList* list, AnimFxNode* node, int64_t obj, int mt_id, int fx_id)
{
    node->list = list;
    node->obj = obj;
    node->fx_id = fx_id;
    node->parent_obj = OBJ_HANDLE_NULL;
    node->mt_id = mt_id;
    node->animate = false;
    node->max_simultaneous_effects = -1;
    node->art_id_ptr = NULL;
    node->light_art_id_ptr = NULL;
    node->light_color_ptr = NULL;
    node->flags = 0;
    node->sound_id = -1;
    node->rotation = 0;
    node->scale = 4;
}

// 0x4CCD80
void animfx_preload_art(AnimFxNode* node)
{
    tig_art_id_t art_id;

    dword_601738 = node->list;
    art_id = dword_601738->entries[node->fx_id].eye_candy_art_id;
    tig_art_touch(art_id);
    if ((dword_601738->entries[node->fx_id].flags & ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY) != 0) {
        art_id = tig_art_eye_candy_id_type_set(art_id, 1);
        tig_art_touch(art_id);
    }
}

// 0x4CCDD0
bool animfx_has(AnimFxNode* node)
{
    AnimFxListEntry* entry;
    unsigned int num;
    int index;
    tig_art_id_t art_id;

    dword_601738 = node->list;

    if (node->obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if ((obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return false;
    }

    if (node->fx_id >= dword_601738->num_effects) {
        tig_debug_printf("AnimFX: animfx_id_get: Warning: AnimFXID Out of Range: %d.\n", node->fx_id);
        return false;
    }

    entry = &(dword_601738->entries[node->fx_id]);
    if (entry->eye_candy_art_id == TIG_ART_ID_INVALID) {
        return false;
    }

    if ((node->flags & ANIMFX_PLAY_CHECK_ALREADY) != 0
        && sub_424560(node->obj, entry->eye_candy_art_id, node->mt_id)) {
        return false;
    }

    num = tig_art_num_get(entry->eye_candy_art_id);

    if ((entry->flags & ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY) != 0) {
        for (index = 0; index < 7; index++) {
            art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_FORE, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                return true;
            }
        }
    }

    if ((entry->flags & ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY) != 0) {
        for (index = 0; index < 7; index++) {
            art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_BACK, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                return true;
            }
        }
    }

    if ((entry->flags & ANIMFX_LIST_ENTRY_UNDERLAY) != 0) {
        for (index = 0; index < 4; index++) {
            art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_UNDERLAY, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                return true;
            }
        }
    }

    if (entry->light_art_id != TIG_ART_ID_INVALID) {
        for (index = 0; index < 4; index++) {
            // FIXME: Original code looks odd. Apparently it does not assign
            // light art id to a variable and the check below refer to the
            // `art_id` from earlier loops. In some circumstances that value
            // is set to `num`, which is obviously wrong.
            art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_LIGHT_AID, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                return true;
            }
        }
    }

    return false;
}

// 0x4CCF70
bool animfx_add(AnimFxNode* node)
{
    AnimFxListEntry* entry;
    tig_art_id_t base_eye_candy_art_id;
    tig_art_id_t eye_candy_art_id = TIG_ART_ID_INVALID;
    unsigned int eye_candy_art_num;
    int overlay_fore_index = -1;
    int overlay_back_index = -1;
    int overlay_light_index = -1;
    int scale = 4;
    bool sound_was_set = false;
    bool rc = true;

    dword_601738 = node->list;

    if (node->obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if ((obj_field_int32_get(node->obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return false;
    }

    if (teleport_is_teleporting() && !teleport_is_teleporting_obj(node->obj)) {
        return false;
    }

    if (node->fx_id >= dword_601738->num_effects) {
        tig_debug_printf("AnimFX: animfx_id_get: Warning: (Weapon?) AnimFXID Out of Range: %d.\n", node->fx_id);
        return false;
    }

    entry = &(dword_601738->entries[node->fx_id]);

    if (entry->eye_candy_art_id != TIG_ART_ID_INVALID) {
        int index;
        tig_art_id_t art_id;

        if ((node->flags & ANIMFX_PLAY_CHECK_ALREADY) != 0
            && sub_424560(node->obj, entry->eye_candy_art_id, node->mt_id)) {
            return false;
        }

        if ((entry->flags & ANIMFX_LIST_ENTRY_CAN_AUTOSCALE) != 0) {
            if ((obj_type_is_critter(obj_field_int32_get(node->obj, OBJ_F_TYPE)))) {
                art_id = obj_field_int32_get(node->obj, OBJ_F_CURRENT_AID);
                switch (tig_art_type(art_id)) {
                case TIG_ART_TYPE_CRITTER:
                    switch (tig_art_critter_id_body_type_get(art_id)) {
                    case TIG_ART_CRITTER_BODY_TYPE_HUMAN:
                    case TIG_ART_CRITTER_BODY_TYPE_ELF:
                        scale = 3;
                        break;
                    case TIG_ART_CRITTER_BODY_TYPE_DWARF:
                    case TIG_ART_CRITTER_BODY_TYPE_HALFLING:
                        scale = 1;
                        break;
                    case TIG_ART_CRITTER_BODY_TYPE_HALF_OGRE:
                        scale = 4;
                        break;
                    default:
                        tig_debug_printf("animfx_add: Error: race out of range!\n");
                        scale = 3;
                        break;
                    }
                    break;
                case TIG_ART_TYPE_MONSTER:
                    switch (tig_art_monster_id_specie_get(art_id)) {
                    case TIG_ART_MONSTER_SPECIE_WOLF:
                    case TIG_ART_MONSTER_SPECIE_SPIDER:
                        scale = 1;
                        break;
                    default:
                        scale = 3;
                        break;
                    }
                    break;
                case TIG_ART_TYPE_UNIQUE_NPC:
                    break;
                default:
                    exit(EXIT_FAILURE);
                }
            }
        }

        base_eye_candy_art_id = entry->eye_candy_art_id;
        if (node->rotation != 0) {
            base_eye_candy_art_id = tig_art_id_rotation_set(base_eye_candy_art_id, node->rotation);
        }

        eye_candy_art_num = tig_art_num_get(eye_candy_art_id);

        if ((entry->flags & ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY) != 0) {
            int found = 0;

            for (index = 0; index < 7; index++) {
                art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_FORE, index);
                if (art_id == TIG_ART_ID_INVALID) {
                    break;
                }

                if (node->max_simultaneous_effects > 0
                    && tig_art_num_get(art_id) == eye_candy_art_num) {
                    found++;
                    if (found > node->max_simultaneous_effects) {
                        return false;
                    }
                }
            }

            if (index >= 7) {
                return false;
            }

            eye_candy_art_id = tig_art_eye_candy_id_type_set(base_eye_candy_art_id, 0);
            if (tig_art_exists(eye_candy_art_id) == TIG_OK) {
                if (node->scale == 4) {
                    if ((entry->flags & ANIMFX_LIST_ENTRY_CAN_AUTOSCALE) != 0) {
                        eye_candy_art_id = tig_art_eye_candy_id_scale_set(eye_candy_art_id, scale);
                    }
                } else {
                    eye_candy_art_id = tig_art_eye_candy_id_scale_set(eye_candy_art_id, node->scale);
                }

                if (!node->animate) {
                    object_overlay_set(node->obj, OBJ_F_OVERLAY_FORE, index, eye_candy_art_id);
                }

                overlay_fore_index = index;
            }
        }

        if ((entry->flags & ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY) != 0) {
            int found = 0;

            for (index = 0; index < 7; index++) {
                art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_BACK, index);
                if (art_id == TIG_ART_ID_INVALID) {
                    break;
                }

                if (node->max_simultaneous_effects > 0
                    && tig_art_num_get(art_id) == eye_candy_art_num) {
                    found++;
                    if (found > node->max_simultaneous_effects) {
                        if (!node->animate) {
                            if (overlay_fore_index != -1) {
                                object_overlay_set(node->obj, OBJ_F_OVERLAY_FORE, overlay_fore_index, TIG_ART_ID_INVALID);
                            }
                        }
                        return false;
                    }
                }
            }

            if (index >= 7) {
                if (!node->animate) {
                    if (overlay_fore_index != -1) {
                        object_overlay_set(node->obj, OBJ_F_OVERLAY_FORE, overlay_fore_index, TIG_ART_ID_INVALID);
                    }
                }
                return false;
            }

            eye_candy_art_id = base_eye_candy_art_id;
            if (tig_art_exists(eye_candy_art_id) == TIG_OK) {
                // FIXME: Setting eye candy type after checking existence of
                // base aid. Foreground overlay and underlay perform existence
                // check after type is set.
                eye_candy_art_id = tig_art_eye_candy_id_type_set(eye_candy_art_id, 1);
                if (node->scale == 4) {
                    if ((entry->flags & ANIMFX_LIST_ENTRY_CAN_AUTOSCALE) != 0) {
                        eye_candy_art_id = tig_art_eye_candy_id_scale_set(eye_candy_art_id, scale);
                    }
                } else {
                    eye_candy_art_id = tig_art_eye_candy_id_scale_set(eye_candy_art_id, node->scale);
                }
            }

            if (!node->animate) {
                object_overlay_set(node->obj, OBJ_F_OVERLAY_BACK, index, eye_candy_art_id);
            }

            overlay_back_index = index;
        }

        if ((entry->flags & ANIMFX_LIST_ENTRY_UNDERLAY) != 0) {
            int found = 0;

            for (index = 0; index < 4; index++) {
                art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_UNDERLAY, index);
                if (art_id == TIG_ART_ID_INVALID) {
                    break;
                }

                if (node->max_simultaneous_effects > 0
                    && tig_art_num_get(art_id) == eye_candy_art_num) {
                    found++;
                    if (found > node->max_simultaneous_effects) {
                        if (!node->animate) {
                            if (overlay_fore_index != -1) {
                                object_overlay_set(node->obj, OBJ_F_OVERLAY_FORE, overlay_fore_index, TIG_ART_ID_INVALID);
                            }

                            if (overlay_back_index != -1) {
                                object_overlay_set(node->obj, OBJ_F_OVERLAY_BACK, overlay_back_index, TIG_ART_ID_INVALID);
                            }
                        }
                        return false;
                    }
                }
            }

            if (index >= 4) {
                if (!node->animate) {
                    if (overlay_fore_index != -1) {
                        object_overlay_set(node->obj, OBJ_F_OVERLAY_FORE, overlay_fore_index, TIG_ART_ID_INVALID);
                    }

                    if (overlay_back_index != -1) {
                        object_overlay_set(node->obj, OBJ_F_OVERLAY_BACK, overlay_back_index, TIG_ART_ID_INVALID);
                    }
                }
                return false;
            }

            eye_candy_art_id = tig_art_eye_candy_id_type_set(base_eye_candy_art_id, 2);
            if (tig_art_exists(eye_candy_art_id) == TIG_OK) {
                if (node->scale == 4) {
                    if ((entry->flags & ANIMFX_LIST_ENTRY_CAN_AUTOSCALE) != 0) {
                        eye_candy_art_id = tig_art_eye_candy_id_scale_set(eye_candy_art_id, scale);
                    }
                } else {
                    eye_candy_art_id = tig_art_eye_candy_id_scale_set(eye_candy_art_id, node->scale);
                }
            }

            if (!node->animate) {
                object_overlay_set(node->obj, OBJ_F_UNDERLAY, index, eye_candy_art_id);
            }

            overlay_fore_index = index;
            overlay_back_index = -5;
        }

        if (entry->light_art_id != TIG_ART_ID_INVALID) {
            int found = 0;

            for (index = 0; index < 4; index++) {
                art_id = obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_LIGHT_AID, index);
                if (art_id == TIG_ART_ID_INVALID) {
                    break;
                }

                if (node->max_simultaneous_effects > 0
                    && tig_art_num_get(art_id) == eye_candy_art_num) {
                    found++;
                    if (found > node->max_simultaneous_effects) {
                        if (!node->animate) {
                            if (overlay_fore_index != -1) {
                                object_overlay_set(node->obj, OBJ_F_OVERLAY_FORE, overlay_fore_index, TIG_ART_ID_INVALID);
                            }

                            if (overlay_back_index != -1) {
                                object_overlay_set(node->obj, OBJ_F_OVERLAY_BACK, overlay_back_index, TIG_ART_ID_INVALID);
                            }
                        }
                        return false;
                    }
                }
            }

            if (index >= 4) {
                if (!node->animate) {
                    if (overlay_fore_index != -1) {
                        object_overlay_set(node->obj, OBJ_F_OVERLAY_FORE, overlay_fore_index, TIG_ART_ID_INVALID);
                    }

                    if (overlay_back_index != -1) {
                        object_overlay_set(node->obj, OBJ_F_OVERLAY_BACK, overlay_back_index, TIG_ART_ID_INVALID);
                    }
                }
                return false;
            }

            if (tig_art_exists(entry->light_art_id) == TIG_OK) {
                overlay_light_index = index;
                if (!node->animate) {
                    object_set_overlay_light(node->obj, index, 0x20, entry->light_art_id, entry->light_color);
                }
            }
        }

        node->sound_id = entry->sound;

        if ((entry->flags & ANIMFX_LIST_ENTRY_ANIMATES) != 0
            && node->animate
            && overlay_fore_index != -1) {
            int goal_type;
            AnimGoalData goal_data;

            if ((node->flags & (ANIMFX_PLAY_CALLBACK | ANIMFX_PLAY_END_CALLBACK)) != 0) {
                if ((node->flags & ANIMFX_PLAY_CALLBACK) != 0) {
                    if ((node->flags & ANIMFX_PLAY_REVERSE) != 0) {
                        goal_type = AG_EYE_CANDY_REVERSE_CALLBACK;
                    } else {
                        goal_type = AG_EYE_CANDY_CALLBACK;
                    }
                } else {
                    if ((node->flags & ANIMFX_PLAY_REVERSE) != 0) {
                        goal_type = AG_EYE_CANDY_REVERSE_END_CALLBACK;
                    } else {
                        goal_type = AG_EYE_CANDY_END_CALLBACK;
                    }
                }
            } else {
                if ((node->flags & ANIMFX_PLAY_REVERSE) != 0) {
                    if ((node->flags & ANIMFX_PLAY_FIRE_DMG) != 0) {
                        goal_type = AG_EYE_CANDY_REVERSE_FIRE_DMG;
                    } else {
                        goal_type = AG_EYE_CANDY_REVERSE;
                    }
                } else {
                    if ((node->flags & ANIMFX_PLAY_FIRE_DMG) != 0) {
                        goal_type = AG_EYE_CANDY_FIRE_DMG;
                    } else {
                        goal_type = AG_EYE_CANDY;
                    }
                }
            }

            if (sub_44D500(&goal_data, node->obj, goal_type)) {
                goal_data.params[AGDATA_ANIM_ID].data = eye_candy_art_id;
                if ((node->flags & ANIMFX_PLAY_NO_ID) != 0) {
                    goal_data.params[AGDATA_SPELL_DATA].data = -1;
                } else {
                    goal_data.params[AGDATA_SPELL_DATA].data = node->mt_id;
                }

                goal_data.params[AGDATA_SCRATCH_VAL2].data = overlay_back_index;
                goal_data.params[AGDATA_SCRATCH_VAL1].data = overlay_fore_index;
                goal_data.params[AGDATA_SCRATCH_VAL3].data = overlay_light_index;
                goal_data.params[AGDATA_SCRATCH_VAL4].data = node->fx_id;
                goal_data.params[AGDATA_SKILL_DATA].data = animfx_list_find(dword_601738);
                goal_data.params[AGDATA_SCRATCH_VAL5].data = entry->light_art_id;
                goal_data.params[AGDATA_RANGE_DATA].data = entry->light_color;
                goal_data.params[AGDATA_FLAGS_DATA].data = (entry->flags & ANIMFX_LIST_ENTRY_LOOPS) != 0 ? 0x80 : 0;

                if ((node->flags & ANIMFX_PLAY_RANDOM_START) != 0) {
                    goal_data.params[AGDATA_FLAGS_DATA].data |= 0x800;
                }

                if ((node->flags & ANIMFX_PLAY_ICE_DMG) != 0) {
                    goal_data.params[AGDATA_FLAGS_DATA].data |= 0x4000;
                }

                goal_data.params[AGDATA_PARENT_OBJ].obj = node->parent_obj;

                if (entry->sound != -1) {
                    if ((node->flags & ANIMFX_PLAY_REVERSE) != 0
                        && dword_601738->num_sound_effects > 0) {
                        int sound_id;
                        char path[TIG_MAX_PATH];

                        sound_id = 8 - node->sound_id % dword_601738->step + node->sound_id;
                        if (gsound_resolve_path(sound_id, path) == TIG_OK) {
                            node->sound_id = sound_id;
                        }
                    }

                    sound_was_set = true;
                    goal_data.params[AGDATA_ANIM_ID_PREVIOUS].data = node->sound_id;
                }

                if ((node->flags & ANIMFX_PLAY_STACK) != 0) {
                    // __FILE__: "C:\Troika\Code\Game\GameLibX\AnimFX.c"
                    // __LINE__: 790
                    sub_44DBE0(stru_601700, &goal_data, __FILE__, __LINE__);
                } else {
                    sub_44D520(&goal_data, &stru_601700);
                }

                if ((node->flags & ANIMFX_PLAY_DESTROY) != 0) {
                    if (sub_44D500(&goal_data, node->obj, AG_DESTROY_OBJ)) {

                        // __FILE__: "C:\Troika\Code\Game\GameLibX\AnimFX.c"
                        // __LINE__: 798
                        sub_44DBE0(stru_601700, &goal_data, __FILE__, __LINE__);
                    }
                }
            }
        }
    } else {
        if ((entry->flags & ANIMFX_LIST_ENTRY_TRANSLUCENCY) != 0
            || (entry->flags & ANIMFX_LIST_ENTRY_TINTING) != 0) {
            object_add_flags(node->obj, OF_TRANSLUCENT);
        } else {
            rc = false;
        }
    }

    if (!sound_was_set) {
        if (entry->sound != -1) {
            gsound_play_sfx_on_obj(entry->sound, 1, node->obj);
            node->sound_id = entry->sound;
        }
    }

    if (node->art_id_ptr != NULL) {
        *node->art_id_ptr = eye_candy_art_id;
    }

    if (node->light_art_id_ptr != NULL) {
        *node->light_art_id_ptr = entry->light_art_id;
    }

    if (node->light_color_ptr != NULL) {
        *node->light_color_ptr = entry->light_color;
    }

    node->overlay_fore_index = overlay_fore_index;
    node->overlay_back_index = overlay_back_index;
    node->overlay_light_index = overlay_light_index;

    return rc;
}

// 0x4CD7A0
bool sub_4CD7A0(AnimFxNode* node)
{
    AnimFxListEntry* entry;
    int overlay_fore_index = -1;
    int overlay_back_index = -1;
    int overlay_light_index = -1;
    int index;
    tig_art_id_t art_id;

    if (node == NULL) {
        return false;
    }

    if (node->obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (node->art_id_ptr == NULL) {
        return false;
    }

    dword_601738 = node->list;
    entry = &(dword_601738->entries[node->fx_id]);

    if (*node->art_id_ptr != TIG_ART_ID_INVALID) {
        if ((entry->flags & ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY) != 0) {
            for (index = 0; index < 7; index++) {
                if (obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_FORE, index) == TIG_ART_ID_INVALID) {
                    art_id = tig_art_eye_candy_id_type_set(entry->eye_candy_art_id, 0);
                    if (tig_art_exists(art_id) == TIG_OK) {
                        overlay_fore_index = index;
                    }
                    break;
                }
            }
        }

        if ((entry->flags & ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY) != 0) {
            for (index = 0; index < 7; index++) {
                if (obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_BACK, index) == TIG_ART_ID_INVALID) {
                    if (tig_art_exists(entry->eye_candy_art_id) == TIG_OK) {
                        overlay_back_index = index;
                    }
                    break;
                }
            }
        }

        if ((entry->flags & ANIMFX_LIST_ENTRY_UNDERLAY) != 0) {
            for (index = 0; index < 4; index++) {
                if (obj_arrayfield_uint32_get(node->obj, OBJ_F_UNDERLAY, index) == TIG_ART_ID_INVALID) {
                    art_id = tig_art_eye_candy_id_type_set(entry->eye_candy_art_id, 2);
                    if (tig_art_exists(art_id) == TIG_OK) {
                        overlay_fore_index = index;
                        overlay_back_index = -5;
                    }
                    break;
                }
            }
        }

        if (entry->light_art_id != TIG_ART_ID_INVALID) {
            for (index = 0; index < 4; index++) {
                if (obj_arrayfield_uint32_get(node->obj, OBJ_F_OVERLAY_LIGHT_AID, index) == TIG_ART_ID_INVALID) {
                    if (tig_art_exists(entry->light_art_id) == TIG_OK) {
                        overlay_light_index = index;
                    }
                    break;
                }
            }
        }
    }

    node->overlay_fore_index = overlay_fore_index;
    node->overlay_back_index = overlay_back_index;
    node->overlay_light_index = overlay_light_index;

    return true;
}

// 0x4CD940
void animfx_remove(AnimFxList* list, int64_t obj, int fx_id, int mt_id)
{
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    dword_601738 = list;

    if (fx_id >= list->num_effects) {
        tig_debug_printf("AnimFX: animfx_id_get: Warning: Weapon AnimFXID Out of Range: %d.\n", fx_id);
        return;
    }

    if (list->entries[fx_id].eye_candy_art_id == TIG_ART_ID_INVALID) {
        tig_debug_printf("AnimFX: animfx_remove: Note: Attempting to remove EMPTY Art!\n");
        return;
    }

    if ((list->entries[fx_id].flags & ANIMFX_LIST_ENTRY_ANIMATES) != 0) {
        sub_4243E0(obj, list->entries[fx_id].eye_candy_art_id, mt_id);
    } else {
        sub_4CD9C0(&(list->entries[fx_id]), obj);
    }
}

// 0x4CD9C0
bool sub_4CD9C0(AnimFxListEntry* entry, int64_t obj)
{
    unsigned int num;
    int index;
    tig_art_id_t art_id;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_DESTROYED) != 0) {
        return false;
    }

    if (entry->eye_candy_art_id == TIG_ART_ID_INVALID) {
        return false;
    }

    num = tig_art_num_get(entry->eye_candy_art_id);

    if ((entry->flags & ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY) != 0) {
        for (index = 0; index < 7; index++) {
            art_id = obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_FORE, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                object_overlay_set(obj, OBJ_F_OVERLAY_FORE, index, TIG_ART_ID_INVALID);
                break;
            }
        }
    }

    if ((entry->flags & ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY) != 0) {
        for (index = 0; index < 7; index++) {
            art_id = obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_BACK, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                object_overlay_set(obj, OBJ_F_OVERLAY_BACK, index, TIG_ART_ID_INVALID);
                break;
            }
        }
    }

    if ((entry->flags & ANIMFX_LIST_ENTRY_UNDERLAY) != 0) {
        for (index = 0; index < 4; index++) {
            art_id = obj_arrayfield_uint32_get(obj, OBJ_F_UNDERLAY, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                object_overlay_set(obj, OBJ_F_UNDERLAY, index, TIG_ART_ID_INVALID);
                break;
            }
        }
    }

    if (entry->light_art_id != TIG_ART_ID_INVALID) {
        for (index = 0; index < 4; index++) {
            // FIXME: Original code looks odd. Apparently it does not assign
            // light art id to a variable and the check below refer to the
            // `art_id` from earlier loops. In some circumstances that value
            // is set to memory address of `entry`, which is obviously wrong.
            art_id = obj_arrayfield_uint32_get(obj, OBJ_F_OVERLAY_LIGHT_AID, index);
            if (art_id == TIG_ART_ID_INVALID) {
                break;
            }

            if (tig_art_num_get(art_id) == num) {
                return true;
            }
        }
    }

    return false;
}

// 0x4CDB30
bool animfx_list_init(AnimFxList* list)
{
    if (list == NULL) {
        return false;
    }

    list->path = NULL;
    list->flags = 0;
    list->initial = 0;
    list->num_fields = 1;
    list->step = 1;
    list->num_effects = 0;
    list->capacity = 0;
    list->size = 0;
    list->entries = NULL;
    list->num_sound_effects = 0;
    list->sound_effects = 0;

    return true;
}

// 0x4CDB70
bool animfx_list_load(AnimFxList* list)
{
    bool success;

    if (dword_60173C >= ANIMFX_LIST_CAPACITY) {
        tig_debug_printf("AnimFX: animfx_list_load: ERROR: Attempt to load too many FX lists!\n");
        // FIXME: Should be `EXIT_FAILURE`.
        exit(EXIT_SUCCESS);
    }

    dword_601738 = list;

    if (!mes_load(list->path, &dword_601734)) {
        return false;
    }

    sub_4CDCD0(list);

    if (list->capacity > 0) {
        list->entries = (AnimFxListEntry*)MALLOC(sizeof(AnimFxListEntry) * list->capacity);
        list->size = list->capacity;
    }

    success = false;
    if (animfx_list_load_internal(list)) {
        success = true;
        list->flags |= ANIMFX_LIST_LOADED;
        dword_60170C[dword_60173C++] = list;
    }

    mes_unload(dword_601734);
    dword_601738 = NULL;

    return success;
}

// 0x4CDC30
int animfx_list_find(AnimFxList* list)
{
    int index;

    for (index = 0; index < ANIMFX_LIST_CAPACITY; index++) {
        if (dword_60170C[index] == list) {
            return index;
        }
    }

    return -1;
}

// 0x4CDC50
AnimFxList* animfx_list_get(int index)
{
    if (index < ANIMFX_LIST_CAPACITY) {
        return dword_60170C[index];
    } else {
        return NULL;
    }
}

// 0x4CDC70
void animfx_list_exit(AnimFxList* list)
{
    int index;

    if ((list->flags & ANIMFX_LIST_LOADED) != 0) {
        for (index = 0; index < ANIMFX_LIST_CAPACITY; index++) {
            if (dword_60170C[index] == list) {
                dword_60170C[index] = NULL;
                dword_60173C--;
                break;
            }
        }

        FREE(list->entries);
        list->entries = NULL;

        list->flags &= ~ANIMFX_LIST_LOADED;
    }
}

// 0x4CDCD0
void sub_4CDCD0(AnimFxList* list)
{
    int count = 0;
    MesFileEntry mes_file_entry;

    mes_file_entry.num = list->initial;

    while (mes_search(dword_601734, &mes_file_entry)) {
        mes_file_entry.num += list->step;
        count += list->num_fields;
    }

    list->capacity = count;
}

// 0x4CDD30
bool animfx_list_load_internal(AnimFxList* list)
{
    MesFileEntry mes_file_entry;
    int index;

    if (list->num_effects != 0) {
        return false;
    }

    mes_file_entry.num = list->initial;

    while (list->num_effects < list->size) {
        for (index = 0; index < list->num_fields; index++) {
            sub_4CE2A0(list->num_effects);

            if (mes_search(dword_601734, &mes_file_entry)) {
                mes_get_msg(dword_601734, &mes_file_entry);
                animfx_build_eye_candy_effect(list->num_effects, mes_file_entry.str);
            }

            mes_file_entry.num++;
            list->num_effects++;
        }

        mes_file_entry.num += list->step - list->num_fields;
    }

    return true;
}

// 0x4CDDF0
void animfx_build_eye_candy_effect(int index, char* str)
{
    if (str == NULL) {
        return;
    }

    char* curr = str;
    tig_str_parse_set_separator(',');

    AnimFxListEntry* entry = &(dword_601738->entries[index]);
    entry->eye_candy_art_id = TIG_ART_ID_INVALID;
    entry->sound = -1;
    entry->light_art_id = TIG_ART_ID_INVALID;
    entry->flags = 0;
    entry->light_color = 0;

    if (curr[0] == '\0') {
        entry->eye_candy_art_id = TIG_ART_ID_INVALID;
        return;
    }

    int art;
    int palette;
    int scale;
    int blend;
    int sound;
    int art_effect;
    if (tig_str_parse_named_value(&curr, "Art:", &art)) {
        tig_str_parse_named_value(&curr, "Palette:", &palette);

        int scale_key;
        if (tig_str_parse_named_value(&curr, "Scale:", &scale_key)) {
            int scale_index;
            for (scale_index = 0; scale_index < SCALE_COUNT; scale_index++) {
                if (dword_5B770C[scale_index] == scale_key) {
                    break;
                }
            }

            if (scale_index < SCALE_COUNT) {
                scale = dword_5B772C[scale_index];
            } else {
                tig_debug_printf("animfx_build_eye_candy_effect: Error: scale out of range!\n");
                scale = dword_5B772C[0];
            }

            entry->flags |= ANIMFX_LIST_ENTRY_CAN_AUTOSCALE;
        }

        tig_str_match_named_str_to_list(&curr, "Blend:", off_5B76E8, BLEND_COUNT, &blend);

        if (tig_str_parse_named_value(&curr, "Sound:", &sound)) {
            entry->sound = sound;
        } else {
            entry->sound = -1;

            if (dword_601738->num_sound_effects > 0) {
                int base_index = index % dword_601738->num_fields;
                if (base_index != 0) {
                    int base_sound = dword_601738->entries[index - base_index].sound;
                    if (base_sound != -1) {
                        entry->sound = dword_601738->sound_effects[base_index] + base_sound;

                        char path[TIG_MAX_PATH];
                        if (gsound_resolve_path(entry->sound, path) != TIG_OK) {
                            entry->sound = -1;
                        }
                    }
                }
            }
        }

        tig_str_parse_named_flag_list(&curr, "Flags:", off_5B76A8, dword_5B76C8, sizeof(off_5B76A8) / sizeof(off_5B76A8[0]), &(entry->flags));

        // TODO: Find constants.
        int type = ((entry->flags & ANIMFX_LIST_ENTRY_ANY_OVERLAY) != 0)
            ? 0
            : 2;

        if (blend != 0) {
            entry->flags |= ANIMFX_LIST_ENTRY_TRANSLUCENCY;
        }

        tig_art_id_t eye_candy_art_id;
        if (tig_art_eye_candy_id_create(art, 0, 0, (entry->flags & ANIMFX_LIST_ENTRY_TRANSLUCENCY) != 0 ? 1 : 0, type, palette, scale, &eye_candy_art_id) == TIG_OK) {
            entry->eye_candy_art_id = eye_candy_art_id;
            if (!sub_4CE1A0(entry->eye_candy_art_id, entry->flags, index, str)) {
                entry->eye_candy_art_id = TIG_ART_ID_INVALID;
                entry->flags = 0;
            }
        } else {
            tig_debug_printf("AnimFX: ERROR: Build Eye Candy FAILED Due to invalid ART ID: [%s], %u, Art Num: %d: Foreground: Index: %d, Str: [%s]!!!\n",
                dword_601738->path,
                eye_candy_art_id,
                tig_art_num_get(eye_candy_art_id),
                index,
                curr);
            entry->eye_candy_art_id = TIG_ART_ID_INVALID;
            entry->flags = 0;
        }
    } else if (tig_str_parse_named_value(&curr, "Sound:", &sound)) {
        entry->sound = sound;
    } else if (tig_str_match_named_str_to_list(&curr, "ArtEffect:", off_5B7704, EFFECT_TYPE_COUNT, &art_effect)) {
        switch (art_effect) {
        case EFFECT_TYPE_TRANSLUCENCY:
            entry->flags |= ANIMFX_LIST_ENTRY_TRANSLUCENCY;
            break;
        case EFFECT_TYPE_TINTING:
            entry->flags |= ANIMFX_LIST_ENTRY_TINTING;
            break;
        }
    }

    int light;
    if (tig_str_parse_named_value(&curr, "Light:", &light)) {
        tig_art_id_t light_art_id;
        tig_art_light_id_create(light, 0, 0, 0, &light_art_id);
        entry->light_art_id = light_art_id;

        int red;
        int green;
        int blue;
        if (tig_str_parse_named_complex_value3(&curr, "Light Color:", '@', &red, &green, &blue)) {
            light_build_color((uint8_t)red, (uint8_t)green, (uint8_t)blue, &(entry->light_color));
        } else {
            light_build_color(255, 255, 255, &(entry->light_color));
        }
    } else {
        entry->light_art_id = TIG_ART_ID_INVALID;
    }

    int projectile_speed;
    if (tig_str_parse_named_value(&curr, "Proj Speed:", &projectile_speed)) {
        entry->projectile_speed = projectile_speed;
    }
}

// 0x4CE1A0
bool sub_4CE1A0(tig_art_id_t art_id, unsigned int flags, int index, const char* str)
{
    if ((flags & ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY) != 0) {
        art_id = tig_art_eye_candy_id_type_set(art_id, 0);
        if (tig_art_exists(art_id) != TIG_OK) {
            tig_debug_printf("AnimFX: ERROR: Build Eye Candy FAILED Due to invalid ART ID: [%s], %u, Art Num: %d: Foreground: Index: %d, Str: [%s]!!!\n",
                dword_601738->path,
                art_id,
                tig_art_num_get(art_id),
                index,
                str);
            return false;
        }
    }

    if ((flags & ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY) != 0) {
        art_id = tig_art_eye_candy_id_type_set(art_id, 1);
        if (tig_art_exists(art_id) != TIG_OK) {
            tig_debug_printf("AnimFX: ERROR: Build Eye Candy FAILED Due to invalid ART ID: [%s], %u, Art Num: %d: Background: Index: %d, Str: [%s]!!!\n",
                dword_601738->path,
                art_id,
                tig_art_num_get(art_id),
                index,
                str);
            return false;
        }
    }

    if ((flags & ANIMFX_LIST_ENTRY_UNDERLAY) != 0) {
        art_id = tig_art_eye_candy_id_type_set(art_id, 2);
        if (tig_art_exists(art_id) != TIG_OK) {
            tig_debug_printf("AnimFX: ERROR: Build Eye Candy FAILED Due to invalid ART ID: [%s], %u, Art Num: %d: Underlay: Index: %d, Str: [%s]!!!\n",
                dword_601738->path,
                art_id,
                tig_art_num_get(art_id),
                index,
                str);
            return false;
        }
    }

    return true;
}

// 0x4CE2A0
void sub_4CE2A0(int index)
{
    AnimFxListEntry* entry;

    if (index >= dword_601738->size) {
        dword_601738->size = index;
        dword_601738->entries = (AnimFxListEntry*)REALLOC(dword_601738->entries, sizeof(AnimFxListEntry) * index);
    }

    entry = &(dword_601738->entries[index]);
    entry->flags = 0;
    entry->eye_candy_art_id = TIG_ART_ID_INVALID;
    entry->sound = -1;
    entry->light_art_id = TIG_ART_ID_INVALID;
    entry->projectile_speed = 0;
}

// 0x4CE300
bool animfx_id_get(AnimFxList* list, int index, AnimFxListEntry** entry)
{
    dword_601738 = list;

    if (entry != NULL) {
        if (index < list->num_effects) {
            *entry = &(list->entries[index]);
            return true;
        }

        tig_debug_printf("AnimFX: animfx_id_get: Warning: Weapon AnimFXID Out of Range: %d.\n", index);
    }

    return false;
}
