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

typedef struct S603D20 {
    /* 0000 */ uint64_t aoe_flags;
    /* 0008 */ unsigned int aoe_spell_flags;
    /* 000C */ unsigned int aoe_no_spell_flags;
    /* 0010 */ int radius;
    /* 0014 */ int count;
} S603D20;

typedef struct S603CB8_F50_Entry {
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
} S603CB8_F50_Entry;

typedef struct S603CB8_F50 {
    /* 0000 */ int cnt;
    /* 0004 */ int field_4;
    /* 0008 */ S603CB8_F50_Entry entries[256];
} S603CB8_F50;

typedef struct S603CB8 {
    /* 0000 */ S603D20* field_0;
    /* 0004 */ int field_4;
    /* 0008 */ int64_t self_obj;
    /* 0010 */ int64_t source_obj;
    /* 0018 */ int64_t field_18;
    /* 0020 */ int64_t field_20;
    /* 0028 */ int64_t field_28;
    /* 0030 */ int64_t field_30;
    /* 0038 */ int64_t field_38;
    /* 0040 */ int64_t field_40;
    /* 0048 */ int64_t field_48;
    /* 0050 */ S603CB8_F50* field_50;
    /* 0054 */ MagicTechObjectNode** field_54;
    /* 0058 */ MagicTechObjectNode** field_58;
    /* 005C */ int field_5C;
    /* 0060 */ int field_60;
    /* 0064 */ int field_64;
} S603CB8;

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
void sub_4F25B0(uint64_t flags);
uint64_t sub_4F25D0(void);
void sub_4F25E0(S603D20* a1);
void sub_4F2600(S603CB8* a1, S603D20* a2, int64_t a3);
bool sub_4F2680(S4F2680* a1);
int sub_4F2C60(int64_t* obj_ptr);
void target_descriptor_set_loc(TargetDescriptor* td, int64_t loc);
void target_descriptor_set_obj(TargetDescriptor* td, int64_t obj);
bool sub_4F2830(TigMouseMessageData* mouse, TargetDescriptor* td, bool fullscreen);
bool sub_4F2CB0(int x, int y, TargetDescriptor* td, uint64_t tgt, bool fullscreen);
int64_t sub_4F2D10(void);
bool sub_4F2D20(S603CB8* a1);
void sub_4F40B0(S603CB8* a1);
bool sub_4F4E40(int64_t obj, int distance, int64_t* loc_ptr);

#endif /* ARCANUM_GAME_TARGET_H_ */
