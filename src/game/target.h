#ifndef ARCANUM_GAME_TARGET_H_
#define ARCANUM_GAME_TARGET_H_

#include "game/context.h"
#include "game/mt_obj_node.h"

/**
 * Represents a target that can reference either a game object or a world
 * location. The `is_loc` flag distinguishes which union member is active.
 */
typedef struct TargetDescriptor {
    union {
        /* 0000 */ int64_t obj;
        /* 0000 */ int64_t loc;
    };
    /* 0008 */ int is_loc;
    /* 000C */ int padding_C;
} TargetDescriptor;

// Serializeable.
static_assert(sizeof(TargetDescriptor) == 0x10, "wrong size");

typedef struct TargetParams {
    /* 0000 */ uint64_t tgt;
    /* 0008 */ unsigned int spell_flags;
    /* 000C */ unsigned int no_spell_flags;
    /* 0010 */ int radius;
    /* 0014 */ int count;
} TargetParams;

/**
 * A single entry in a collected target list. Stores either the object handle or
 * the world location of a resolved target, but not both.
 *
 * NOTE: For unknown reason the majority of fields are not used. `obj` and `loc`
 * are separate fields instead of relying on `TargetDescriptor`. I would say
 * targeting system underwent some serious refactorings back in a day, with some
 * things left behind.
 */
typedef struct TargetListEntry {
    /* 0000 */ int64_t obj;
    /* 0008 */ int64_t loc;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ int field_18;
    /* 001C */ int field_1C;
    /* 0020 */ int field_20;
    /* 0024 */ int field_24;
    /* 0028 */ int field_28;
    /* 002C */ int field_2C;
    /* 0030 */ int field_30;
    /* 0034 */ int field_34;
} TargetListEntry;

/**
 * A list of collected targeting results. Holds up to 256 entries, although this
 * limit is never validated in the code.
 */
typedef struct TargetList {
    /* 0000 */ int cnt;
    /* 0004 */ int padding_4;
    /* 0008 */ TargetListEntry entries[256];
} TargetList;

/**
 * The targeting evaluation context. Holds all inputs and intermediate state
 * used when testing whether a candidate object or location is a valid target.
 *
 * `source_obj` and `source_loc` are the object and location that activate
 * targeting.
 *
 * `target_obj` and `target_loc` are the candidate target object and location
 * currently being evaluated.
 *
 * `orig_target_obj` and `orig_target_loc` are used during building of the list
 * of target candidates to preserve the original target intent during nested
 * evaluations.
 *
 * `summoned_obj` is probably some kind of leftover, which was later replaced
 * with `summoned_obj_list`. The latter is guaranteed to have a list item with
 * `summoned_obj` included.
 *
 * `source_spell_flags` and `target_spell_flags` are used during magictech
 * action resolution. Strictly speaking, they do not belong to targeting.
 */
typedef struct TargetContext {
    /* 0000 */ TargetParams* params;
    /* 0004 */ int padding_4;
    /* 0008 */ int64_t self_obj;
    /* 0010 */ int64_t source_obj;
    /* 0018 */ int64_t source_loc;
    /* 0020 */ int64_t target_obj;
    /* 0028 */ int64_t target_loc;
    /* 0030 */ int64_t orig_target_obj;
    /* 0038 */ int64_t orig_target_loc;
    /* 0040 */ int64_t field_40;
    /* 0048 */ int64_t summoned_obj;
    /* 0050 */ TargetList* targets;
    /* 0054 */ MagicTechObjectNode** obj_list;
    /* 0058 */ MagicTechObjectNode** summoned_obj_list;
    /* 005C */ ObjectSpellFlags source_spell_flags;
    /* 0060 */ ObjectSpellFlags target_spell_flags;
    /* 0064 */ int padding_64;
} TargetContext;

typedef struct S4F2680 {
    /* 0000 */ int64_t field_0;
    /* 0008 */ int64_t field_8;
    /* 0010 */ TargetDescriptor* td;
    /* 0014 */ int field_14;
} S4F2680;

// clang-format off

#define TGT_NONE                                    SDL_UINT64_C(0x0000000000000000)
#define TGT_SELF                                    SDL_UINT64_C(0x0000000000000001)
#define TGT_SOURCE                                  SDL_UINT64_C(0x0000000000000002)
#define TGT_OBJECT                                  SDL_UINT64_C(0x0000000000000004)
#define TGT_OBJ_SELF                                SDL_UINT64_C(0x0000000000000008)
#define TGT_OBJ_RADIUS                              SDL_UINT64_C(0x0000000000000010)
#define TGT_OBJ_T_PC                                SDL_UINT64_C(0x0000000000000020)
#define TGT_OBJ_T_CRITTER                           SDL_UINT64_C(0x0000000000000040)
#define TGT_OBJ_ST_CRITTER_ANIMAL                   SDL_UINT64_C(0x0000000000000080)
#define TGT_OBJ_ST_CRITTER_DEAD                     SDL_UINT64_C(0x0000000000000100)
#define TGT_OBJ_ST_CRITTER_UNDEAD                   SDL_UINT64_C(0x0000000000000200)
#define TGT_OBJ_ST_CRITTER_DEMON                    SDL_UINT64_C(0x0000000000000400)
#define TGT_OBJ_ST_CRITTER_MECHANICAL               SDL_UINT64_C(0x0000000000000800)
#define TGT_OBJ_ST_CRITTER_GOOD                     SDL_UINT64_C(0x0000000000001000)
#define TGT_OBJ_ST_CRITTER_EVIL                     SDL_UINT64_C(0x0000000000002000)
#define TGT_OBJ_ST_CRITTER_UNREVIVIFIABLE           SDL_UINT64_C(0x0000000000004000)
#define TGT_OBJ_ST_CRITTER_UNRESURRECTABLE          SDL_UINT64_C(0x0000000000008000)
#define TGT_OBJ_T_PORTAL                            SDL_UINT64_C(0x0000000000010000)
#define TGT_OBJ_T_CONTAINER                         SDL_UINT64_C(0x0000000000020000)
#define TGT_OBJ_ST_OPENABLE_LOCKED                  SDL_UINT64_C(0x0000000000040000)
#define TGT_OBJ_T_WALL                              SDL_UINT64_C(0x0000000000080000)
#define TGT_OBJ_DAMAGED                             SDL_UINT64_C(0x0000000000100000)
#define TGT_OBJ_DAMAGED_POISONED                    SDL_UINT64_C(0x0000000000200000)
#define TGT_OBJ_POISONED                            SDL_UINT64_C(0x0000000000400000)
#define TGT_OBJ_M_STONE                             SDL_UINT64_C(0x0000000000800000)
#define TGT_OBJ_M_FLESH                             SDL_UINT64_C(0x0000000001000000)
#define TGT_OBJ_INVEN                               SDL_UINT64_C(0x0000000002000000)
#define TGT_OBJ_WEIGHT_BELOW_5                      SDL_UINT64_C(0x0000000004000000)
#define TGT_OBJ_IN_WALL                             SDL_UINT64_C(0x0000000008000000)
#define TGT_OBJ_NO_SELF                             SDL_UINT64_C(0x0000000010000000)
#define TGT_OBJ_NO_T_PC                             SDL_UINT64_C(0x0000000020000000)
#define TGT_OBJ_NO_ST_CRITTER_ANIMAL                SDL_UINT64_C(0x0000000040000000)
#define TGT_OBJ_NO_ST_CRITTER_DEAD                  SDL_UINT64_C(0x0000000080000000)
#define TGT_OBJ_NO_ST_CRITTER_UNDEAD                SDL_UINT64_C(0x0000000100000000)
#define TGT_OBJ_NO_ST_CRITTER_DEMON                 SDL_UINT64_C(0x0000000200000000)
#define TGT_OBJ_NO_ST_CRITTER_MECHANICAL            SDL_UINT64_C(0x0000000400000000)
#define TGT_OBJ_NO_ST_CRITTER_GOOD                  SDL_UINT64_C(0x0000000800000000)
#define TGT_OBJ_NO_ST_CRITTER_EVIL                  SDL_UINT64_C(0x0000001000000000)
#define TGT_OBJ_NO_ST_CRITTER_UNREVIVIFIABLE        SDL_UINT64_C(0x0000002000000000)
#define TGT_OBJ_NO_ST_CRITTER_UNRESURRECTABLE       SDL_UINT64_C(0x0000004000000000)
#define TGT_OBJ_NO_ST_OPENABLE_LOCKED               SDL_UINT64_C(0x0000008000000000)
#define TGT_OBJ_NO_ST_MAGICALLY_HELD                SDL_UINT64_C(0x0000010000000000)
#define TGT_OBJ_NO_T_WALL                           SDL_UINT64_C(0x0000020000000000)
#define TGT_OBJ_NO_DAMAGED                          SDL_UINT64_C(0x0000040000000000)
#define TGT_OBJ_NO_M_STONE                          SDL_UINT64_C(0x0000080000000000)
#define TGT_OBJ_NO_INVEN                            SDL_UINT64_C(0x0000100000000000)
#define TGT_OBJ_NO_INVULNERABLE                     SDL_UINT64_C(0x0000200000000000)
#define TGT_SUMMONED                                SDL_UINT64_C(0x0000400000000000)
#define TGT_TILE                                    SDL_UINT64_C(0x0000800000000000)
#define TGT_TILE_SELF                               SDL_UINT64_C(0x0001000000000000)
#define TGT_TILE_PATHABLE_TO                        SDL_UINT64_C(0x0002000000000000)
#define TGT_TILE_EMPTY                              SDL_UINT64_C(0x0004000000000000)
#define TGT_TILE_EMPTY_IMMOBILES                    SDL_UINT64_C(0x0008000000000000)
#define TGT_TILE_NO_EMPTY                           SDL_UINT64_C(0x0010000000000000)
#define TGT_TILE_RADIUS                             SDL_UINT64_C(0x0020000000000000)
#define TGT_TILE_RADIUS_WALL                        SDL_UINT64_C(0x0040000000000000)
#define TGT_TILE_OFFSCREEN                          SDL_UINT64_C(0x0080000000000000)
#define TGT_TILE_INDOOR_OR_OUTDOOR_MATCH            SDL_UINT64_C(0x0100000000000000)
#define TGT_CONE                                    SDL_UINT64_C(0x0200000000000000)
#define TGT_ALL_PARTY_CRITTERS                      SDL_UINT64_C(0x0400000000000000)
#define TGT_PARTY_CRITTER                           SDL_UINT64_C(0x0800000000000000)
#define TGT_NON_PARTY_CRITTERS                      SDL_UINT64_C(0x1000000000000000)
#define TGT_PARENT_OBJ                              SDL_UINT64_C(0x2000000000000000)
#define TGT_ATTACKER_OBJ                            SDL_UINT64_C(0x4000000000000000)
#define TGT_LIST                                    SDL_UINT64_C(0x8000000000000000)

// clang-format on

bool target_init(GameInitInfo* init_info);
void target_exit(void);
void target_reset(void);
void target_resize(GameResizeInfo* resize_info);
void target_flags_set(uint64_t flags);
uint64_t target_flags_get(void);
void target_params_init(TargetParams* params);
void target_context_init(TargetContext* ctx, TargetParams* params, int64_t source_obj);
bool sub_4F2680(S4F2680* a1);
int sub_4F2C60(int64_t* obj_ptr);
void target_descriptor_set_loc(TargetDescriptor* td, int64_t loc);
void target_descriptor_set_obj(TargetDescriptor* td, int64_t obj);
bool target_pick_at_screen_xy(int x, int y, TargetDescriptor* td, bool fullscreen);
bool target_pick_at_screen_xy_ex(int x, int y, TargetDescriptor* td, uint64_t tgt, bool fullscreen);
uint64_t target_last_rejection_get(void);
bool target_context_evaluate(TargetContext* ctx);
void target_context_build_list(TargetContext* ctx);
bool target_find_displacement_loc(int64_t obj, int distance, int64_t* loc_ptr);

#endif /* ARCANUM_GAME_TARGET_H_ */
