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
    /* 0040 */ int64_t attacker_obj;
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

#define Tgt_No_Self 0x10000000
#define Tgt_Non_Party 0x1000000000000000
#define Tgt_No_ST_Critter_Dead 0x80000000
#define Tgt_Summoned_No_Obj 0x400000000000
#define Tgt_Parent 0x2000000000000000
#define Tgt_All_Party_Critters_Naked 0x400000000000000
#define Tgt_Tile_Offscreen_Naked 0x80000000000000
#define Tgt_Obj_T_Critter_Naked 0x40
#define Tgt_Obj_T_Portal_Naked 0x10000
#define Tgt_Obj_T_Container_Naked 0x20000
#define Tgt_Obj_T_Wall_Naked 0x80000
#define Tgt_Tile_Radius_Naked 0x20000000000000
#define Tgt_Tile_Radius_Wall_Naked 0x40000000000000
#define Tgt_Damaged_Poisoned 0x200000

#define Tgt_None 0x00
#define Tgt_Self 0x01
#define Tgt_Source 0x02
#define Tgt_Object 0x04
#define Tgt_Obj_Self 0x0C
#define Tgt_Obj_Radius 0x10
#define Tgt_Obj_T_PC 0x24
#define Tgt_Obj_T_Critter 0x44
#define Tgt_Obj_ST_Critter_Animal 0x0C4
#define Tgt_Obj_ST_Critter_Dead 0x144
#define Tgt_Obj_ST_Critter_Undead 0x244
#define Tgt_Obj_ST_Critter_Demon 0x444
#define Tgt_Obj_ST_Critter_Mechanical 0x844
#define Tgt_Obj_ST_Critter_Good 0x1044
#define Tgt_Obj_ST_Critter_Evil 0x2044
#define Tgt_Obj_ST_Critter_Unrevivifiable 0x4044
#define Tgt_Obj_ST_Critter_Unresurrectable 0x8044
#define Tgt_Obj_T_Portal 0x10004
#define Tgt_Obj_T_Container 0x20004
#define Tgt_Obj_ST_Openable_Locked 0x40004
#define Tgt_Obj_T_Wall 0x80004
#define Tgt_Obj_Damaged 0x100004
#define Tgt_Obj_Damaged_Poisoned 0x200004
#define Tgt_Obj_Poisoned 0x400004
#define Tgt_Obj_M_Stone 0x800004
#define Tgt_Obj_M_Flesh 0x1000004
#define Tgt_Obj_Inven 0x2000004
#define Tgt_Obj_Weight_Below_5 0x4000004
#define Tgt_Obj_In_Wall 0x8000004
#define Tgt_Obj_No_Self 0x10000004
#define Tgt_Obj_No_T_PC 0x20000004
#define Tgt_Obj_No_ST_Critter_Animal 0x40000044
#define Tgt_Obj_No_ST_Critter_Dead 0x80000004
#define Tgt_Obj_No_ST_Critter_Undead 0x100000044
#define Tgt_Obj_No_ST_Critter_Demon 0x200000044
#define Tgt_Obj_No_ST_Critter_Mechanical 0x400000044
#define Tgt_Obj_No_ST_Critter_Good 0x800000044
#define Tgt_Obj_No_ST_Critter_Evil 0x1000000044
#define Tgt_Obj_No_ST_Critter_Unrevivifiable 0x2000000044
#define Tgt_Obj_No_ST_Critter_Unresurrectable 0x4000000044
#define Tgt_Obj_No_ST_Openable_Locked 0x8000000004
#define Tgt_Obj_No_ST_Magically_Held 0x10000000004
#define Tgt_Obj_No_T_Wall 0x20000000004
#define Tgt_Obj_No_Damaged 0x40000000004
#define Tgt_Obj_No_M_Stone 0x80000000004
#define Tgt_Obj_No_Inven 0x100000000004
#define Tgt_Obj_No_Invulnerable 0x200000000004
#define Tgt_Summoned 0x400000000004
#define Tgt_Tile 0x800000000000
#define Tgt_Tile_Self 0x1000000000000
#define Tgt_Tile_Pathable_To 0x2800000000000
#define Tgt_Tile_Empty 0x4800000000000
#define Tgt_Tile_Empty_Immobiles 0x8800000000000
#define Tgt_Tile_No_Empty 0x10800000000000
#define Tgt_Tile_Radius 0x20800000000000
#define Tgt_Tile_Radius_Wall 0x40800000000000
#define Tgt_Tile_Offscreen 0x80800000000000
#define Tgt_Tile_Indoor_Or_Outdoor_Match 0x100800000000000
#define Tgt_Cone 0x200000000000000
#define Tgt_All_Party_Critters 0x400000000000044
#define Tgt_Party_Critter 0x800000000000044
#define Tgt_Non_Party_Critters 0x1000000000000004
#define Tgt_Parent_Obj 0x2000000000000004
#define Tgt_Attacker_Obj 0x4000000000000004
#define Tgt_List 0x8000000000000000

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
