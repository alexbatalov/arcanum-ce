#include "game/magictech.h"

#include <stdio.h>

#include "game/ai.h"
#include "game/anim.h"
#include "game/anim_private.h"
#include "game/animfx.h"
#include "game/antiteleport.h"
#include "game/area.h"
#include "game/combat.h"
#include "game/critter.h"
#include "game/descriptions.h"
#include "game/effect.h"
#include "game/fate.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/light_scheme.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/mt_ai.h"
#include "game/mt_item.h"
#include "game/multiplayer.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/path.h"
#include "game/player.h"
#include "game/proto.h"
#include "game/random.h"
#include "game/reaction.h"
#include "game/resistance.h"
#include "game/script.h"
#include "game/sector.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/target.h"
#include "game/teleport.h"
#include "game/tile.h"
#include "game/tile_script.h"
#include "game/timeevent.h"
#include "game/ui.h"

typedef void(MagicTechProc)();

typedef struct MagicTechSummonTableEntry {
    /* 0000 */ int value;
    /* 0004 */ int basic_prototype;
} MagicTechSummonTableEntry;

typedef struct MagicTechSummonTable {
    /* 0000 */ int cnt;
    /* 0004 */ MagicTechSummonTableEntry* entries;
    /* 0008 */ int type;
} MagicTechSummonTable;

static bool sub_44F3C0(MagicTechRunInfo* run_info, TigFile* stream);
static bool sub_44F620(MagicTechRunInfo* run_info, TigFile* stream);
static bool sub_44FE30(int a1, const char* path, int a3);
static bool sub_44FFA0(int a1, const char* a2, int a3);
static void sub_450090(mes_file_handle_t msg_file, MagicTechInfo* info, int num, int magictech);
static void sub_4501D0(mes_file_handle_t msg_file, MagicTechInfo* info, int num, int magictech);
static void sub_450240();
static bool sub_4507D0(int64_t obj, int magictech);
static int sub_450B90(int64_t obj);
static void sub_450C10(int64_t obj, unsigned int flags);
static void sub_4510F0();
static void MTComponentAGoal_ProcFunc();
static void MTComponentAGoalTerminate_ProcFunc();
static void MTComponentAIRedirect_ProcFunc();
static void MTComponentCast_ProcFunc();
static void MTComponentChargeNBranch_ProcFunc();
static void MTComponentDamage_ProcFunc();
static void MTComponentDestroy_ProcFunc();
static void MTComponentDispel_ProcFunc();
static void sub_451C40(int mt_id, int64_t obj);
static void MTComponentEffect_ProcFunc();
static void MTComponentEyeCandy_ProcFunc();
static void MTComponentHeal_ProcFunc();
static void MTComponentIdentify_ProcFunc();
static void MTComponentInterrupt_ProcFunc();
static void MTComponentObjFlag_ProcFunc();
static void MTComponentMovement_ProcFunc();
static void MTComponentRecharge_ProcFunc();
static void MTComponentSummon_ProcFunc();
static void magictech_pick_proto_from_list(ObjectID* oid, int list);
static void MTComponentTerminate_ProcFunc();
static void MTComponentTestNBranch_ProcFunc();
static void MTComponentTrait_ProcFunc();
static void sub_452CD0(int64_t obj, tig_art_id_t art_id);
static void MTComponentTraitIdx_ProcFunc();
static void MTComponentTrait64_ProcFunc();
static void MTComponentUse_ProcFunc();
static void MTComponentNoop_ProcFunc();
static void MTComponentEnvFlag_ProcFunc();
static bool sub_452F20();
static bool sub_4532F0(int64_t obj, int magictech);
static bool sub_453370(int64_t obj, int magictech, int a3);
static bool sub_453410(int mt_id, int spell, int64_t obj, int* other_mt_id_ptr);
static void sub_4534E0(MagicTechRunInfo* run_info);
static void sub_453630();
static bool sub_453710();
static bool sub_4537B0();
static void sub_453D40();
static void sub_453EE0();
static void sub_453F20(int64_t a1, int64_t a2);
static void sub_453FA0();
static bool sub_4545E0(MagicTechRunInfo* run_info);
static bool sub_454700(int64_t source_loc, int64_t target_loc, int64_t target_obj, int spell);
static void sub_454790(TimeEvent* timeevent, int a2, int a3, DateTime* datetime);
static bool sub_4547F0(TimeEvent* timeevent, DateTime* datetime);
static bool sub_4548D0(TimeEvent* timeevent, DateTime* a2, DateTime* a3);
static int sub_455100(int64_t obj, int fld, unsigned int a3, bool a4);
static bool sub_4551C0(int64_t a1, int64_t a2, int64_t a3);
static void sub_455250(MagicTechRunInfo* run_info, DateTime* datetime);
static void sub_455350(int64_t obj, int64_t target_loc);
static void sub_4554B0(MagicTechRunInfo* run_info, int64_t obj);
static bool sub_455550(S603CB8* a1, MagicTechRunInfo* run_info);
static void sub_455710();
static void magictech_id_new_lock(MagicTechRunInfo** lock_ptr);
static bool sub_455820(MagicTechRunInfo* run_info);
static void magictech_id_free_lock(int mt_id);
static void sub_455960(MagicTechRunInfo* run_info);
static void sub_4559E0(MagicTechRunInfo* run_info);
static void sub_455C30(MagicTechInvocation* mt_invocation);
static bool sub_456430(int64_t a1, int64_t a2, MagicTechInfo* magictech);
static void magictech_preload_art(MagicTechRunInfo* run_info);
static void sub_457030(int mt_id, int action);
static bool sub_4570E0(TimeEvent* timeevent);
static void sub_457270(int mt_id);
static void sub_457530(int mt_id);
static void sub_457580(MagicTechInfo* info, int magictech);
static void magictech_build_aoe_info(MagicTechInfo* info, char* str);
static void sub_4578F0(MagicTechInfo* info, char* str);
static void sub_457B20(MagicTechInfo* info, char* str);
static void sub_457D00(MagicTechInfo* info, char* str);
static void magictech_build_ai_info(MagicTechInfo* info, char* str);
static void magictech_build_effect_info(MagicTechInfo* info, char* str);
static bool magictech_find_first(int64_t obj, int* mt_id_ptr);
static bool magictech_find_next(int64_t obj, int* mt_id_ptr);
static bool sub_459290(int64_t obj, int spell, int* index_ptr);
static void sub_459490(int mt_id);
static bool sub_4594D0(TimeEvent* timeevent);
static bool magictech_recharge_timeevent_schedule(int64_t item_obj, int mana_cost, bool add);
static bool magictech_recharge_timeevent_add(TimeEvent* timeevent);
static void sub_45A480(MagicTechRunInfo* run_info);
static void sub_45A760(int64_t obj, const char* msg);

// 0x596140
static uint64_t qword_596140[] = {
    0x00,
    0x01,
    0x02,
    0x04,
    0x0C,
    0x10,
    0x24,
    0x44,
    0x0C4,
    0x144,
    0x244,
    0x444,
    0x844,
    0x1044,
    0x2044,
    0x4044,
    0x8044,
    0x10004,
    0x20004,
    0x40004,
    0x80004,
    0x100004,
    0x200004,
    0x400004,
    0x800004,
    0x1000004,
    0x2000004,
    0x4000004,
    0x8000004,
    0x10000004,
    0x20000004,
    0x40000044,
    0x80000004,
    0x100000044,
    0x200000044,
    0x400000044,
    0x800000044,
    0x1000000044,
    0x2000000044,
    0x4000000044,
    0x8000000004,
    0x10000000004,
    0x20000000004,
    0x40000000004,
    0x80000000004,
    0x100000000004,
    0x200000000004,
    0x400000000004,
    0x800000000000,
    0x1000000000000,
    0x2800000000000,
    0x4800000000000,
    0x8800000000000,
    0x10800000000000,
    0x20800000000000,
    0x40800000000000,
    0x80800000000000,
    0x100800000000000,
    0x200000000000000,
    0x400000000000044,
    0x800000000000044,
    0x1000000000000004,
    0x2000000000000004,
    0x4000000000000004,
    0x8000000000000000,
};

// 0x5B0BA0
static int dword_5B0BA0 = -1;

// 0x5B0BA4
static int dword_5B0BA4 = -1;

// 0x4513FD
static MagicTechProc* magictech_procs[] = {
    MTComponentNoop_ProcFunc,
    MTComponentAGoal_ProcFunc,
    MTComponentAGoalTerminate_ProcFunc,
    MTComponentAIRedirect_ProcFunc,
    MTComponentCast_ProcFunc,
    MTComponentChargeNBranch_ProcFunc,
    MTComponentDamage_ProcFunc,
    MTComponentDestroy_ProcFunc,
    MTComponentDispel_ProcFunc,
    MTComponentEffect_ProcFunc,
    MTComponentEnvFlag_ProcFunc,
    MTComponentEyeCandy_ProcFunc,
    MTComponentHeal_ProcFunc,
    MTComponentIdentify_ProcFunc,
    MTComponentInterrupt_ProcFunc,
    MTComponentMovement_ProcFunc,
    MTComponentObjFlag_ProcFunc,
    MTComponentRecharge_ProcFunc,
    MTComponentSummon_ProcFunc,
    MTComponentTerminate_ProcFunc,
    MTComponentTestNBranch_ProcFunc,
    MTComponentTrait_ProcFunc,
    MTComponentTraitIdx_ProcFunc,
    MTComponentTrait64_ProcFunc,
    MTComponentUse_ProcFunc,
};

// 0x5B0C0C
static const char* off_5B0C0C[] = {
    "[Begin]",
    "[Maintain]",
    "[End]",
    "[Callback]",
    "[EndCallback]",
};

// 0x5B0C20
static const char* off_5B0C20[] = {
    "neutral",
    "friendly",
    "aggressive",
    "may_be_aggressive",
};

// 0x5B0C30
static MagicTechFlags dword_5B0C30[] = {
    0,
    MAGICTECH_FRIENDLY,
    MAGICTECH_AGGRESSIVE,
    MAGICTECH_FRIENDLY | MAGICTECH_AGGRESSIVE,
};

// 0x5B0C40
static const char* off_5B0C40[] = {
    "anim_goal_animate",
    "anim_goal_knockback",
    "anim_goal_follow",
    "anim_goal_floating",
    "anim_goal_destroy_obj",
    "anim_goal_knock_down",
};

// 0x5B0C58
static int dword_5B0C58[6] = {
    0,
    43,
    24,
    44,
    32,
    62,
};

// 0x5B0C70
static const char* off_5B0C70[] = {
    "Dmg_Normal",
    "Dmg_Poison",
    "Dmg_Electrical",
    "Dmg_Fire",
    "Dmg_Magic",
    "Dmg_Acid",
};

// 0x5B0C88
static const char* off_5B0C88[] = {
    "Remove",
    "Add",
};

// 0x5B0C90
static const char* off_5B0C90[] = {
    "FLAG_OFF",
    "FLAG_ON",
};

// 0x5B0C98
static const char* off_5B0C98[] = {
    "Equal",
    "GreaterThan",
    "GreaterThanOrEqual",
    "LessThan",
    "LessThanOrEqual",
};

// 0x5B0CAC
static const char* off_5B0CAC[] = {
    "art_num",
    "Teleport_Tile",
    "obj_f_critter_fleeing_from",
    "HP_Damage",
};

// 0x5B0CBC
static int dword_5B0CBC[] = {
    OBJ_F_CURRENT_AID,
    OBJ_F_CRITTER_TELEPORT_DEST,
    OBJ_F_CRITTER_FLEEING_FROM,
    OBJ_F_HP_DAMAGE,
};

// 0x5B0CCC
static const char* off_5B0CCC[] = {
    "obj_f_critter_stat_base_idx",
    "obj_f_resistance_idx",
};

// 0x5B0CD4
static int dword_5B0CD4[] = {
    OBJ_F_CRITTER_STAT_BASE_IDX,
    OBJ_F_RESISTANCE_IDX,
};

// 0x5B0CDC
static const char** off_5B0CDC[] = {
    stat_lookup_keys_tbl,
    resistance_lookup_keys_tbl,
};

// 0x5B0CE4
static int dword_5B0CE4[] = {
    STAT_COUNT,
    RESISTANCE_TYPE_COUNT,
};

// 0x5B0CF8
static const char* off_5B0CF8[] = {
    "Target_Tile",
    "Caster_Obj",
    "Caster_Owner_Obj",
};

// 0x5B0D04
static const char* off_5B0D04[] = {
    "Target_Tile",
    "Random_Tile_In_Radius_Self",
    "Teleport_Tile",
    "Teleport_Area_Portal",
};

// 0x5B0D14
static const char* off_5B0D14[] = {
    "None",
    "Full",
    "Resurrect",
    "Reanimate",
    "Death",
    "Stunned",
    "Scars",
    "Scaled",
    "Weap_Dropped",
    "Crippling",
};

// 0x5B0D3C
static unsigned int dword_5B0D3C[] = {
    0,
    CDF_FULL,
    CDF_RESURRECT,
    CDF_REANIMATE | CDF_RESURRECT,
    CDF_DEATH,
    CDF_STUN,
    CDF_SCAR,
    CDF_SCALE,
    CDF_DROP_WEAPON,
    CDF_BLIND | CDF_CRIPPLE_ARM | CDF_CRIPPLE_LEG,
};

// 0x5B0D64
static struct {
    const char* aoe;
    const char* aoe_sf;
    const char* aoe_no_sf;
    const char* radius;
    const char* count;
} off_5B0D64[] = {
    {
        "[Begin]AoE:",
        "[Begin]AoE_SF:",
        "[Begin]AoE_NO_SF:",
        "[Begin]Radius:",
        "[Begin]Count:",
    },
    {
        "[Maintain]AoE:",
        "[Maintain]AoE_SF:",
        "[Maintain]AoE_NO_SF:",
        "[Maintain]Radius:",
        "[Maintain]Count:",
    },
    {
        "[End]AoE:",
        "[End]AoE_SF:",
        "[End]AoE_NO_SF:",
        "[End]Radius:",
        "[End]Count:",
    },
    {
        "[Callback]AoE:",
        "[Callback]AoE_SF:",
        "[Callback]AoE_NO_SF:",
        "[Callback]Radius:",
        "[Callback]Count:",
    },
    {
        "[EndCallback]AoE:",
        "[EndCallback]AoE_SF:",
        "[EndCallback]AoE_NO_SF:",
        "[EndCallback]Radius:",
        "[EndCallback]Count:",
    },
};

// 0x5B0DC8
static int dword_5B0DC8[] = {
    0,
    4,
    5,
    6,
    3,
    7,
};

// 0x5B0DE0
static int dword_5B0DE0[] = {
    6,
    9,
    12,
    15,
    18,
};

// 0x5B0DF8
static MagicTechSummonTableEntry stru_5B0DF8[] = {
    { 20, BP_WOLF },
    { 1, BP_VORPAL_BUNNY },
    { 19, BP_FOREST_APE },
    { 20, BP_GRIZZLY_BEAR },
    { 20, BP_COUGAR },
    { 20, BP_TIGER },
};

// 0x5B0E28
static MagicTechSummonTableEntry stru_5B0E28[] = {
    { 20, BP_BUNNY },
    { 20, BP_SHEEP },
    { 20, BP_CHICKEN },
    { 20, BP_SHEEP },
    { 20, BP_PIG },
    { 20, BP_COW },
};

// 0x5B0E58
static MagicTechSummonTableEntry stru_5B0E58[] = {
    { 99, BP_PATRIARCH_WOLF },
    { 1, BP_VORPAL_BUNNY },
};

// 0x5B0E68
static MagicTechSummonTableEntry stru_5B0E68[] = {
    { 25, BP_FAMILIAR_UNDERLING },
    { 50, BP_FAMILIAR },
    { 75, BP_FAMILIAR_SLASHER },
    { 1000, BP_FAMILIAR_BLOOD_CLAW },
};

// 0x5B0E88
static MagicTechSummonTableEntry stru_5B0E88[] = {
    { 20, BP_WOLF },
    { 40, BP_COUGAR },
    { 55, BP_TIGER },
    { 70, BP_GRIZZLY_BEAR },
    { 95, BP_FOREST_APE },
    { 1000, BP_VORPAL_BUNNY },
};

// 0x5B0EB8
static MagicTechSummonTableEntry stru_5B0EB8[] = {
    { 20, BP_DECAYED_SOLDIER },
    { 60, BP_RAGGED_FIGHTER },
    { 85, BP_SKELETON_WARRIOR },
    { 1000, BP_UNDEAD_CHAMPION },
};

// 0x5B0ED8
static MagicTechSummonTable stru_5B0ED8[6] = {
    { SDL_arraysize(stru_5B0DF8), stru_5B0DF8, 0 },
    { SDL_arraysize(stru_5B0E28), stru_5B0E28, 0 },
    { SDL_arraysize(stru_5B0E58), stru_5B0E58, 0 },
    { SDL_arraysize(stru_5B0E68), stru_5B0E68, 1 },
    { SDL_arraysize(stru_5B0E88), stru_5B0E88, 1 },
    { SDL_arraysize(stru_5B0EB8), stru_5B0EB8, 1 },
};

// 0x5B0F20
static int dword_5B0F20 = -1;

// 0x5BBD70
const char* off_5BBD70[] = {
    "Tgt_None",
    "Tgt_Self",
    "Tgt_Source",
    "Tgt_Object",
    "Tgt_Obj_Self",
    "Tgt_Obj_Radius",
    "Tgt_Obj_T_PC",
    "Tgt_Obj_T_Critter",
    "Tgt_Obj_ST_Critter_Animal",
    "Tgt_Obj_ST_Critter_Dead",
    "Tgt_Obj_ST_Critter_Undead",
    "Tgt_Obj_ST_Critter_Demon",
    "Tgt_Obj_ST_Critter_Mechanical",
    "Tgt_Obj_ST_Critter_Good",
    "Tgt_Obj_ST_Critter_Evil",
    "Tgt_Obj_ST_Critter_Unrevivifiable",
    "Tgt_Obj_ST_Critter_Unresurrectable",
    "Tgt_Obj_T_Portal",
    "Tgt_Obj_T_Container",
    "Tgt_Obj_ST_Openable_Locked",
    "Tgt_Obj_T_Wall",
    "Tgt_Obj_Damaged",
    "Tgt_Obj_Damaged_Poisoned",
    "Tgt_Obj_Poisoned",
    "Tgt_Obj_M_Stone",
    "Tgt_Obj_M_Flesh",
    "Tgt_Obj_Inven",
    "Tgt_Obj_Weight_Below_5",
    "Tgt_Obj_In_Wall",
    "Tgt_Obj_No_Self",
    "Tgt_Obj_No_T_PC",
    "Tgt_Obj_No_ST_Critter_Animal",
    "Tgt_Obj_No_ST_Critter_Dead",
    "Tgt_Obj_No_ST_Critter_Undead",
    "Tgt_Obj_No_ST_Critter_Demon",
    "Tgt_Obj_No_ST_Critter_Mechanical",
    "Tgt_Obj_No_ST_Critter_Good",
    "Tgt_Obj_No_ST_Critter_Evil",
    "Tgt_Obj_No_ST_Critter_Unrevivifiable",
    "Tgt_Obj_No_ST_Critter_Unresurrectable",
    "Tgt_Obj_No_ST_Openable_Locked",
    "Tgt_Obj_No_ST_Magically_Held",
    "Tgt_Obj_No_T_Wall",
    "Tgt_Obj_No_Damaged",
    "Tgt_Obj_No_M_Stone",
    "Tgt_Obj_No_Inven",
    "Tgt_Obj_No_Invulnerable",
    "Tgt_Summoned",
    "Tgt_Tile",
    "Tgt_Tile_Self",
    "Tgt_Tile_Pathable_To",
    "Tgt_Tile_Empty",
    "Tgt_Tile_Empty_Immobiles",
    "Tgt_Tile_No_Empty",
    "Tgt_Tile_Radius",
    "Tgt_Tile_Radius_Wall",
    "Tgt_Tile_Offscreen",
    "Tgt_Tile_Indoor_Or_Outdoor_Match",
    "Tgt_Cone",
    "Tgt_All_Party_Critters",
    "Tgt_Party_Critter",
    "Tgt_Non_Party_Critters",
    "Tgt_Parent_Obj",
    "Tgt_Attacker_Obj",
    "Tgt_List",
};

// 0x5E3510
static bool magictech_editor;

// 0x5E3518
static S603CB8_F50 stru_5E3518;

// 0x5E6D20
static mes_file_handle_t dword_5E6D20;

// 0x5E6D24
static IsoInvalidateRectFunc* dword_5E6D24;

// 0x5E6D28
static S603CB8 stru_5E6D28;

// 0x5E6D90
static int dword_5E6D90;

// 0x5E7568
static AnimFxList spell_eye_candies;

// 0x5E7594
static mes_file_handle_t magictech_spell_mes_file;

// 0x5E7598
static MagicTechInfo* dword_5E7598;

// 0x5E759C
static MagicTechComponentList* dword_5E759C;

// 0x5E75A0
static int dword_5E75A0;

// 0x5E75A8
static bool dword_5E75A8;

// 0x5E75AC
static int dword_5E75AC;

// 0x5E75B0
static int64_t qword_5E75B0;

// 0x5E75B8
static int64_t qword_5E75B8;

// 0x5E75C0
static AnimID stru_5E75C0;

// 0x5E75CC
static int dword_5E75CC;

// 0x5E75D0
static unsigned int dword_5E75D0;

// 0x5E75D4
static int dword_5E75D4;

// 0x5E75D8
static int dword_5E75D8;

// 0x5E75DC
static int dword_5E75DC;

// 0x5E75E0
static int64_t qword_5E75E0;

// 0x5E75E4
static int dword_5E75E4;

// 0x5E75E8
static int dword_5E75E8;

// 0x5E75F0
static MagicTechRunInfo* dword_5E75F0;

// 0x5E75F4
static mes_file_handle_t magictech_mes_file;

// 0x5E75F8
static char** magictech_component_names;

// 0x5E75FC
static bool magictech_cheat_mode;

// 0x5E7600
static int dword_5E7600;

// 0x5E7604
static bool dword_5E7604;

// 0x5E7608
static int magictech_recharge_timeevent_mana_cost;

// 0x5E760C
static int dword_5E760C;

// 0x5E7610
static int64_t magictech_recharge_timeevent_item_obj;

// 0x5E7618
static bool magictech_initialized;

// 0x5E761C
static MagicTechComponentInfo* dword_5E761C;

// 0x5E7620
static MagicTechResistance* dword_5E7620;

// 0x5E7624
static bool dword_5E7624;

// 0x5E7628
static int dword_5E7628;

// 0x5E762C
static int dword_5E762C;

// 0x5E7630
static bool dword_5E7630;

// 0x5E7634
static int dword_5E7634;

// 0x6876D8
MagicTechInfo* magictech_spells;

// 0x6876DC
static int dword_6876DC;

// 0x6876E0
MagicTechRunInfo* magictech_run_info;

// 0x44EF50
bool magictech_init(GameInitInfo* init_info)
{
    MesFileEntry mes_file_entry;
    int index;

    dword_5E6D24 = init_info->invalidate_rect_func;
    magictech_editor = init_info->editor;
    magictech_component_names = (char**)CALLOC(25, sizeof(char*));
    magictech_run_info = (MagicTechRunInfo*)CALLOC(512, sizeof(MagicTechRunInfo));
    sub_455710();

    if (!mes_load("Rules\\magictech.mes", &magictech_mes_file)) {
        FREE(magictech_component_names);
        FREE(magictech_run_info);
        return false;
    }

    if (!mes_load("mes\\spell.mes", &magictech_spell_mes_file)) {
        FREE(magictech_component_names);
        FREE(magictech_run_info);
        return false;
    }

    mes_file_entry.num = 10;
    for (index = 0; index < 25; index++) {
        if (!mes_search(magictech_mes_file, &mes_file_entry)) {
            FREE(magictech_component_names);
            FREE(magictech_run_info);
            return false;
        }

        magictech_component_names[index] = mes_file_entry.str;
        mes_file_entry.num++;
    }

    if (!magictech_editor) {
        if (!animfx_list_init(&spell_eye_candies)) {
            FREE(magictech_component_names);
            FREE(magictech_run_info);
            return false;
        }

        spell_eye_candies.path = "Rules\\SpellEyeCandy.mes";
        spell_eye_candies.capacity = 1344;
        spell_eye_candies.num_fields = 6;
        spell_eye_candies.step = 10;
        spell_eye_candies.num_sound_effects = 6;
        spell_eye_candies.sound_effects = dword_5B0DC8;
        if (!animfx_list_load(&spell_eye_candies)) {
            FREE(magictech_component_names);
            FREE(magictech_run_info);
            return false;
        }
    }

    magictech_spells = (MagicTechInfo*)MALLOC(sizeof(MagicTechInfo) * MT_SPELL_COUNT);

    if (!sub_44FE30(0, "Rules\\spelllist.mes", 1000)) {
        FREE(magictech_component_names);
        FREE(magictech_run_info);
        return false;
    }

    magictech_initialized = true;
    dword_6876DC = 0;

    return true;
}

// 0x44F150
void magictech_reset()
{
    magictech_cheat_mode = false;
}

// 0x44F160
bool magictech_post_init(GameInitInfo* init_info)
{
    (void)init_info;

    if (!sub_44FFA0(0, "Rules\\spelllist.mes", 1000)) {
        FREE(magictech_component_names);
        FREE(magictech_run_info);
        magictech_initialized = false;
        return false;
    }

    return true;
}

// 0x44F1B0
void magictech_exit()
{
    if (magictech_initialized) {
        if (!magictech_editor) {
            if (spell_eye_candies.num_effects) {
                animfx_list_exit(&spell_eye_candies);
            }
        }

        sub_450240();

        mes_unload(magictech_spell_mes_file);
        mes_unload(magictech_mes_file);

        if (magictech_spells != NULL) {
            FREE(magictech_spells);
            magictech_spells = NULL;
        }

        if (magictech_component_names != NULL) {
            FREE(magictech_component_names);
        }

        if (magictech_run_info != NULL) {
            FREE(magictech_run_info);
        }

        magictech_initialized = false;
    }
}

// 0x44F250
bool magictech_post_save(TigFile* stream)
{
    int cnt;
    int index;
    int start;
    int extent;

    if (stream == NULL) {
        return false;
    }

    if (tig_file_fwrite(&dword_6876DC, sizeof(dword_6876DC), 1, stream) != 1) {
        return false;
    }

    cnt = 512;
    if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        return false;
    }

    index = 0;
    while (index < cnt) {
        start = index;
        while (index < cnt && (magictech_run_info[index].field_13C & 0x1) != 0) {
            index++;
        }

        extent = index - start;
        if (extent > 0) {
            if (tig_file_fwrite(&extent, sizeof(extent), 1, stream) != 1) {
                return false;
            }

            while (start < index) {
                if (!sub_44F3C0(&(magictech_run_info[start]), stream)) {
                    return false;
                }
                start++;
            }
        }

        while (index < cnt && (magictech_run_info[index].field_13C & 0x1) == 0) {
            index++;
        }

        extent = index - start;
        if (extent > 0) {
            extent = -extent;
            if (tig_file_fwrite(&extent, sizeof(extent), 1, stream) != 1) {
                return false;
            }
        }
    }

    return true;
}

// 0x44F3C0
bool sub_44F3C0(MagicTechRunInfo* run_info, TigFile* stream)
{
    if (stream == NULL) return false;
    if (tig_file_fwrite(&(run_info->id), sizeof(run_info->id), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(run_info->spell), sizeof(run_info->spell), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(run_info->action), sizeof(run_info->action), 1, stream) != 1) return false;
    if (!mt_obj_node_save_detached(&(run_info->source_obj), stream)) return false;
    if (!mt_obj_node_save_detached(&(run_info->parent_obj), stream)) return false;
    if (!mt_obj_node_save_detached(&(run_info->target_obj), stream)) return false;
    if (!mt_obj_node_save_detached(&(run_info->field_E8), stream)) return false;
    if (!mt_obj_node_save_list(&(run_info->objlist), stream)) return false;
    if (!mt_obj_node_save_list(&(run_info->summoned_obj), stream)) return false;
    if (tig_file_fwrite(&(run_info->field_138), sizeof(run_info->field_138), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(run_info->field_13C), sizeof(run_info->field_13C), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(run_info->trigger), sizeof(run_info->trigger), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(run_info->field_144), sizeof(run_info->field_144), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(run_info->field_148), sizeof(run_info->field_148), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(run_info->field_150), sizeof(run_info->field_150), 1, stream) != 1) return false;

    return true;
}

// 0x44F540
bool magictech_post_load(GameLoadInfo* load_info)
{
    int cnt;
    int index;
    int extent;
    int j;

    if (load_info->stream == NULL) {
        return false;
    }

    if (tig_file_fread(&dword_6876DC, sizeof(dword_6876DC), 1, load_info->stream) != 1) {
        return false;
    }

    if (tig_file_fread(&cnt, sizeof(cnt), 1, load_info->stream) != 1) {
        return false;
    }

    index = 0;
    while (index < cnt) {
        if (tig_file_fread(&extent, sizeof(extent), 1, load_info->stream) != 1) {
            return false;
        }

        if (extent > 0) {
            for (j = 0; j < extent; j++) {
                if (!sub_44F620(&(magictech_run_info[index]), load_info->stream)) {
                    return false;
                }

                index++;
            }
        } else if (extent < 0) {
            index += -extent;
        }
    }

    return true;
}

// 0x44F620
bool sub_44F620(MagicTechRunInfo* run_info, TigFile* stream)
{
    if (stream == NULL) return false;
    if (tig_file_fread(&(run_info->id), sizeof(run_info->id), 1, stream) != 1) return false;
    if (tig_file_fread(&(run_info->spell), sizeof(run_info->spell), 1, stream) != 1) return false;
    if (tig_file_fread(&(run_info->action), sizeof(run_info->action), 1, stream) != 1) return false;
    if (!mt_obj_node_load_detached(&(run_info->source_obj), stream)) return false;
    if (!mt_obj_node_load_detached(&(run_info->parent_obj), stream)) return false;
    if (!mt_obj_node_load_detached(&(run_info->target_obj), stream)) return false;
    if (!mt_obj_node_load_detached(&(run_info->field_E8), stream)) return false;
    if (!mt_obj_node_load_list(&(run_info->objlist), stream)) return false;
    if (!mt_obj_node_load_list(&(run_info->summoned_obj), stream)) return false;
    if (tig_file_fread(&(run_info->field_138), sizeof(run_info->field_138), 1, stream) != 1) return false;
    if (tig_file_fread(&(run_info->field_13C), sizeof(run_info->field_13C), 1, stream) != 1) return false;
    if (tig_file_fread(&(run_info->trigger), sizeof(run_info->trigger), 1, stream) != 1) return false;
    if (tig_file_fread(&(run_info->field_144), sizeof(run_info->field_144), 1, stream) != 1) return false;
    if (tig_file_fread(&(run_info->field_148), sizeof(run_info->field_148), 1, stream) != 1) return false;
    if (tig_file_fread(&(run_info->field_150), sizeof(run_info->field_150), 1, stream) != 1) return false;

    return true;
}

// 0x44F7A0
void magictech_break_nodes_to_map(const char* map)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    bool append;
    int cnt;
    int idx;
    MagicTechObjectNode* node;

    sprintf(path, "save\\current\\maps\\%s\\MT.dat", map);

    if (tig_file_exists(path, NULL)) {
        append = true;
        stream = tig_file_fopen(path, "r+b");
    } else {
        append = false;
        stream = tig_file_fopen(path, "wb");
    }

    if (stream == NULL) {
        // FIXME: Message is misleading, informs about TimeEvent, not magictech.
        tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Couldn't create TimeEvent data file for map!\n");
        return;
    }

    cnt = 0;
    if (!append) {
        if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Writing Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    } else {
        if (tig_file_fseek(stream, 0, SEEK_SET) != 0) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Seeking to start of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Reading Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fseek(stream, 0, SEEK_END) != 0) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Seeking to end of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x01) != 0) {
            if (magictech_run_info[idx].source_obj.obj != OBJ_HANDLE_NULL
                && !teleport_is_teleporting_obj(magictech_run_info[idx].source_obj.obj)) {
                sub_457270(idx);
                continue;
            }

            if (magictech_run_info[idx].parent_obj.obj != magictech_run_info[idx].source_obj.obj
                && magictech_run_info[idx].parent_obj.obj != OBJ_HANDLE_NULL
                && !teleport_is_teleporting_obj(magictech_run_info[idx].parent_obj.obj)) {
                sub_457270(idx);
                continue;
            }

            if (magictech_run_info[idx].target_obj.obj != OBJ_HANDLE_NULL
                && !teleport_is_teleporting_obj(magictech_run_info[idx].target_obj.obj)) {
                sub_457270(idx);
                continue;
            }

            node = magictech_run_info[idx].objlist;
            while (node != NULL) {
                if (node->obj != OBJ_HANDLE_NULL
                    && !teleport_is_teleporting_obj(node->obj)) {
                    sub_457270(idx);
                    break;
                }
                node = node->next;
            }
            if (node != NULL) {
                continue;
            }

            node = magictech_run_info[idx].summoned_obj;
            while (node != NULL) {
                if (node->obj != OBJ_HANDLE_NULL
                    && !teleport_is_teleporting_obj(node->obj)) {
                    sub_457270(idx);
                    break;
                }
                node = node->next;
            }
            if (node != NULL) {
                continue;
            }

            if (!sub_44F3C0(&(magictech_run_info[idx]), stream)) {
                break;
            }

            magictech_id_free_lock(idx);
            cnt++;
        }
    }

    if (idx < 512) {
        tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Failed to save out nodes!\n");
        tig_file_fclose(stream);
        return;
    }

    if (tig_file_fseek(stream, 0, SEEK_SET) != 0
        || tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Writing Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    tig_file_fclose(stream);
}

// 0x44FA70
void magictech_save_nodes_to_map(const char* map)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    bool append;
    int cnt;
    int idx;

    sprintf(path, "save\\current\\maps\\%s\\MT.dat", map);

    if (tig_file_exists(path, NULL)) {
        append = true;
        stream = tig_file_fopen(path, "r+b");
    } else {
        append = false;
        stream = tig_file_fopen(path, "wb");
    }

    if (stream == NULL) {
        // FIXME: Message is misleading, informs about TimeEvent, not magictech.
        tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Couldn't create TimeEvent data file for map!\n");
        return;
    }

    cnt = 0;
    if (!append) {
        if (tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Writing Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    } else {
        if (tig_file_fseek(stream, 0, SEEK_SET) != 0) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Seeking to start of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Reading Header to data file for map!\n");
            tig_file_fclose(stream);
            return;
        }

        if (tig_file_fseek(stream, 0, SEEK_END) != 0) {
            tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Seeking to end of data file for map!\n");
            tig_file_fclose(stream);
            return;
        }
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0) {
            if (!sub_44F3C0(&(magictech_run_info[idx]), stream)) {
                break;
            }
            cnt++;
        }
    }

    if (idx < 512) {
        tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Failed to save out nodes!\n");
        tig_file_fclose(stream);
        return;
    }

    if (tig_file_fseek(stream, 0, SEEK_SET) != 0
        || tig_file_fwrite(&cnt, sizeof(cnt), 1, stream) != 1) {
        tig_debug_printf("MagicTech: magictech_save_nodes_to_map: ERROR: Writing Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    tig_file_fclose(stream);
}

// 0x44FC30
void magictech_load_nodes_from_map(const char* map)
{
    char path[TIG_MAX_PATH];
    TigFile* stream;
    int cnt;
    int index;
    MagicTechRunInfo tmp_run_info;
    MagicTechRunInfo* run_info;
    int mt_id;

    sprintf(path, "save\\current\\maps\\%s\\MT.dat", map);

    if (!tig_file_exists(path, NULL)) {
        return;
    }

    stream = tig_file_fopen(path, "rb");
    if (stream == NULL) {
        // FIXME: Message is misleading, informs about TimeEvent, not magictech.
        tig_debug_printf("MagicTech: magictech_load_nodes_from_map: ERROR: Couldn't open TimeEvent data file for map!\n");
        return;
    }

    if (tig_file_fread(&cnt, sizeof(cnt), 1, stream) != 1) {
        tig_debug_printf("MagicTech: magictech_load_nodes_from_map: ERROR: Reading Header to data file for map!\n");
        tig_file_fclose(stream);
        return;
    }

    tmp_run_info.objlist = NULL;
    tmp_run_info.summoned_obj = NULL;

    for (index = 0; index < cnt; index++) {
        sub_4559E0(&tmp_run_info);
        if (!sub_44F620(&tmp_run_info, stream)) {
            break;
        }

        run_info = &(magictech_run_info[tmp_run_info.id]);
        if ((run_info->field_13C & 0x1) != 0) {
            magictech_id_new_lock(&run_info);
        }

        mt_id = run_info->id;
        *run_info = tmp_run_info;
        run_info->id = mt_id;
        sub_459500(run_info->id);
    }

    tig_file_fclose(stream);

    if (index < cnt) {
        tig_debug_printf("MagicTech: magictech_load_nodes_from_map: ERROR: Failed to load all nodes!\n");
    }

    tig_file_remove(path);
}

// 0x44FDC0
void magictech_get_msg(MesFileEntry* mes_file_entry)
{
    mes_get_msg(magictech_spell_mes_file, mes_file_entry);
}

// 0x44FDE0
char* magictech_spell_name(int num)
{
    MesFileEntry mes_file_entry;

    if (num >= 0 && num < MT_SPELL_COUNT) {
        mes_file_entry.num = num;
        mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
        return mes_file_entry.str;
    } else {
        return NULL;
    }
}

// 0x44FE20
void magictech_cheat_mode_on()
{
    magictech_cheat_mode = true;
}

// 0x44FE30
bool sub_44FE30(int a1, const char* path, int a3)
{
    int magictech = 0;
    int action;

    (void)a1;

    if (!mes_load(path, &dword_5E6D20)) {
        return false;
    }

    for (magictech = 0; magictech < MT_SPELL_COUNT; magictech++) {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            magictech_spells[magictech].components[action].cnt = 0;
            magictech_spells[magictech].components[action].entries = NULL;
        }
    }

    for (magictech = 0; magictech < MT_80; magictech++) {
        sub_450090(dword_5E6D20, &(magictech_spells[magictech]), a3, magictech);
    }

    for (; magictech < MT_140; magictech++) {
        sub_450090(dword_5E6D20, &(magictech_spells[magictech]), a3, magictech);
        magictech_spells[magictech].iq = 0;
    }

    for (; magictech < MT_SPELL_COUNT; magictech++) {
        sub_450090(dword_5E6D20, &(magictech_spells[magictech]), a3 + 2000, magictech);
        magictech_spells[magictech].flags |= MAGICTECH_IS_TECH;
        magictech_spells[magictech].iq = 0;
    }

    // NOTE: Meaningless, never executed.
    for (; magictech < MT_SPELL_COUNT; magictech++) {
        sub_450090(dword_5E6D20, &(magictech_spells[magictech]), a3, magictech);
        magictech_spells[magictech].iq = 0;
    }

    return true;
}

// 0x44FFA0
bool sub_44FFA0(int a1, const char* a2, int a3)
{
    int magictech = 0;

    (void)a1;
    (void)a2;

    while (magictech < MT_80) {
        sub_4501D0(dword_5E6D20, &(magictech_spells[magictech]), a3, magictech);
        magictech++;
    }

    while (magictech < MT_140) {
        sub_4501D0(dword_5E6D20, &(magictech_spells[magictech]), a3, magictech);
        magictech_spells[magictech].iq = 0;
        magictech++;
    }

    while (magictech < MT_SPELL_COUNT) {
        sub_4501D0(dword_5E6D20, &(magictech_spells[magictech]), a3 + 2000, magictech);
        magictech_spells[magictech].flags |= MAGICTECH_IS_TECH;
        magictech_spells[magictech].iq = 0;
        magictech++;
    }

    mes_unload(dword_5E6D20);

    return true;
}

// 0x450090
void sub_450090(mes_file_handle_t msg_file, MagicTechInfo* info, int num, int magictech)
{
    MesFileEntry mes_file_entry;

    dword_5E7628 = magictech;

    sub_457580(info, magictech);

    mes_file_entry.num = magictech;
    mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
    info->name = mes_file_entry.str;

    mes_file_entry.num = num + 50 * magictech;
    mes_get_msg(msg_file, &mes_file_entry);
    magictech_build_aoe_info(info, mes_file_entry.str);

    mes_file_entry.num++;
    mes_get_msg(msg_file, &mes_file_entry);
    sub_4578F0(info, mes_file_entry.str);

    mes_file_entry.num++;
    if (mes_search(msg_file, &mes_file_entry)) {
        mes_get_msg(msg_file, &mes_file_entry);
        sub_457B20(info, mes_file_entry.str);
    }

    mes_file_entry.num++;
    if (mes_search(msg_file, &mes_file_entry)) {
        mes_get_msg(msg_file, &mes_file_entry);
        sub_457D00(info, mes_file_entry.str);
    }

    memset(&(info->ai), 0xFF, sizeof(info->ai));
    info->defensive2 = 0;

    mes_file_entry.num++;
    if (mes_search(msg_file, &mes_file_entry)) {
        mes_get_msg(msg_file, &mes_file_entry);
        magictech_build_ai_info(info, mes_file_entry.str);
    }
}

// 0x4501D0
void sub_4501D0(mes_file_handle_t msg_file, MagicTechInfo* info, int num, int magictech)
{
    MesFileEntry mes_file_entry;
    int index;

    mes_file_entry.num = num + 50 * magictech + 5;
    for (index = 0; index < 44; index++) {
        if (!mes_search(msg_file, &mes_file_entry)) {
            return;
        }

        mes_get_msg(msg_file, &mes_file_entry);
        magictech_build_effect_info(info, mes_file_entry.str);

        mes_file_entry.num++;
    }
}

// 0x450240
void sub_450240()
{
    int index;
    int action;

    for (index = 0; index < MT_SPELL_COUNT; index++) {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            if (magictech_spells[index].components[action].entries != NULL) {
                FREE(magictech_spells[index].components[action].entries);
            }
        }
    }
}

// 0x450280
int magictech_get_range(int magictech)
{
    if (magictech_initialized) {
        return magictech_spells[magictech].range;
    } else {
        return 1;
    }
}

// 0x4502B0
int sub_4502B0(int magictech)
{
    if (magictech_initialized) {
        return magictech_spells[magictech].field_70[0].radius;
    } else {
        return 0;
    }
}

// 0x4502E0
int magictech_min_intelligence(int magictech)
{
    (void)magictech;

    return 5;
}

// 0x4502F0
int magictech_min_willpower(int magictech)
{
    if (magictech_initialized) {
        return magictech_spells[magictech].iq;
    }

    if (magictech >= 0 && magictech < MT_80) {
        return dword_5B0DE0[magictech % 5];
    }

    return dword_5B0DE0[1];
}

// 0x450340
int magictech_get_cost(int magictech)
{
    if (magictech_initialized) {
        return magictech_spells[magictech].cost;
    } else {
        return 1;
    }
}

// 0x450370
bool magictech_is_aggressive(int magictech)
{
    if (magictech_initialized) {
        return (magictech_spells[magictech].flags & MAGICTECH_AGGRESSIVE) != 0;
    } else {
        return false;
    }
}

// 0x4503A0
bool sub_4503A0(int magictech)
{
    if (magictech_initialized) {
        return magictech_spells[magictech].maintenance.period > 0;
    } else {
        return false;
    }
}

// 0x4503E0
MagicTechMaintenanceInfo* magictech_get_maintenance(int magictech)
{
    return &(magictech_spells[magictech].maintenance);
}

// 0x450400
MagicTechDurationInfo* magictech_get_duration(int magictech)
{
    return &(magictech_spells[magictech].duration);
}

// 0x450420
bool sub_450420(int64_t obj, int cost, bool a3, int magictech)
{
    MagicTechInfo* info;
    bool charges_from_cells = false;
    int obj_type;
    int64_t parent_obj;

    if (tig_net_is_active() && !multiplayer_is_locked()) {
        Packet76 pkt;

        if (!tig_net_is_host()) {
            return true;
        }

        pkt.type = 76;
        pkt.oid = obj_get_id(obj);
        pkt.cost = cost;
        pkt.field_24 = a3;
        pkt.magictech = magictech;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    if (magictech != 10000) {
        info = &(magictech_spells[magictech]);
        if (magictech >= 0
            && magictech < MT_SPELL_COUNT
            && info != NULL
            && (info->flags & 0x20) != 0) {
            charges_from_cells = true;
        }
    } else {
        info = NULL;
    }

    if (cost <= 0) {
        return true;
    }

    dword_5E762C = cost;

    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);

    if (charges_from_cells) {
        if (obj_type_is_item(obj_type)) {
            parent_obj = obj_field_handle_get(obj, OBJ_F_ITEM_PARENT);
            cost = 2 * cost - item_adjust_magic(obj, parent_obj, cost);
        } else {
            parent_obj = obj;
        }

        if (item_ammo_quantity_get(parent_obj, TIG_ART_AMMO_TYPE_CHARGE) < cost) {
            return false;
        }

        if (!item_ammo_transfer(parent_obj, OBJ_HANDLE_NULL, cost, TIG_ART_AMMO_TYPE_CHARGE, OBJ_HANDLE_NULL)) {
            return false;
        }

        return true;
    }

    if (obj_type_is_critter(obj_type)
        && (obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & OCF_FATIGUE_LIMITING) != 0) {
        cost /= 2;
        if (cost == 0) {
            cost = 1;
        }
    }

    if (obj_type_is_critter(obj_type)) {
        int inventory_location;
        int64_t item_obj;
        int mana_store;
        int new_mana_store;
        int item_mana_cost;

        for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
            item_obj = item_wield_get(obj, inventory_location);
            if (item_obj != OBJ_HANDLE_NULL) {
                mana_store = obj_field_int32_get(item_obj, OBJ_F_ITEM_MANA_STORE);
                if (mana_store != 0) {
                    if (mana_store >= cost) {
                        item_mana_cost = cost;
                        new_mana_store = mana_store - cost;
                        cost = 0;
                    } else {
                        item_mana_cost = mana_store;
                        new_mana_store = 0;
                        cost -= mana_store;
                    }

                    if (a3) {
                        obj_field_int32_set(item_obj, OBJ_F_ITEM_MANA_STORE, new_mana_store);
                        magictech_recharge_timeevent_schedule(item_obj, item_mana_cost, true);
                        sub_4605D0();

                        if (new_mana_store == 0) {
                            mt_ai_notify_item_exhausted(obj, item_obj);
                        }
                    }
                }
            }
        }

        if (cost != 0) {
            if (critter_fatigue_current(obj) - cost <= -15) {
                return false;
            }

            if (a3) {
                int fatigue_dam = critter_fatigue_damage_get(obj);
                if (critter_fatigue_damage_set(obj, fatigue_dam + cost) == 0) {
                    return false;
                }

                if (info != NULL
                    && (info->maintenance.period != 0 || info->duration.period == 0)
                    && critter_fatigue_current(obj) < 0) {
                    return false;
                }
            }
        }

        return true;
    }

    if (obj_type_is_item(obj_type)) {
        int mana_store;
        unsigned int item_flags;

        mana_store = obj_field_int32_get(obj, OBJ_F_ITEM_SPELL_MANA_STORE);
        if (mana_store == 0) {
            return false;
        }
        if (mana_store > 0) {
            item_flags = obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS);
            item_flags |= OIF_IS_MAGICAL;
            obj_field_int32_set(obj, OBJ_F_ITEM_FLAGS, item_flags);

            if (a3) {
                obj_field_int32_set(obj, OBJ_F_ITEM_SPELL_MANA_STORE, mana_store - 1);
                sub_4605D0();
            }
        }

        return true;
    }

    return true;
}

// 0x4507B0
void sub_4507B0(int64_t obj, int magictech)
{
    sub_4507D0(obj, magictech);
}

// 0x4507D0
bool sub_4507D0(int64_t obj, int magictech)
{
    int cost;

    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    cost = magictech_spells[magictech].cost;
    if (magictech >= 0 && magictech < MT_80
        && spell_mastery_get(obj) == COLLEGE_FROM_SPELL(magictech)) {
        cost /= 2;
        if ((cost & 1) != 0) {
            cost++;
        }
    }

    if (magictech_cheat_mode) {
        cost = 1;
    }

    if ((obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
            || obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC)
        && stat_level_get(obj, STAT_RACE) == RACE_DWARF) {
        cost *= 2;
    }

    return sub_450420(obj, cost, true, magictech);
}

// 0x4508A0
bool magictech_can_charge_spell_fatigue(int64_t obj, int magictech)
{
    int cost;

    if (obj == OBJ_HANDLE_NULL) {
        tig_debug_printf("\tMagicTech: magictech_can_charge_spell_fatigue: Maintain cannot charge!\n");
        return false;
    }

    cost = magictech_spells[magictech].cost;

    if (magictech_cheat_mode) {
        cost = 1;
    }

    if ((obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_PC
            || obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC)
        && stat_level_get(obj, STAT_RACE) == RACE_DWARF) {
        cost *= 2;
    }

    return sub_450420(obj, cost, false, magictech);
}

// 0x450940
bool sub_450940(int mt_id)
{
    MagicTechRunInfo* run_info;
    int cost;

    if (!magictech_id_to_run_info(mt_id, &run_info)) {
        tig_debug_printf("\tMagicTech: Maintain cannot charge!\n");
        return false;
    }

    if ((run_info->field_13C & 0x2) != 0
        || run_info->source_obj.obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if (run_info->action == MAGICTECH_ACTION_BEGIN) {
        cost = magictech_spells[mt_id].cost;
    } else {
        cost = magictech_get_maintenance(mt_id)->cost;
    }

    if (magictech_cheat_mode) {
        cost = 1;
    }

    if ((obj_field_int32_get(run_info->source_obj.obj, OBJ_F_TYPE) == OBJ_TYPE_PC
            || obj_field_int32_get(run_info->source_obj.obj, OBJ_F_TYPE) == OBJ_TYPE_NPC)
        && stat_level_get(run_info->source_obj.obj, STAT_RACE) == RACE_DWARF) {
        cost *= 2;
    }

    return sub_450420(run_info->source_obj.obj, cost, 0, run_info->spell);
}

// 0x450A50
int64_t sub_450A50(int64_t obj)
{
    int type;

    if (obj == OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (type != OBJ_TYPE_PC
        && type != OBJ_TYPE_NPC
        && type >= OBJ_TYPE_WEAPON
        && type <= OBJ_TYPE_GENERIC
        && (obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_INVENTORY) != 0) {
        return obj_field_handle_get(obj, OBJ_F_ITEM_PARENT);
    }

    return obj;
}

// 0x450AC0
int sub_450AC0(int64_t obj)
{
    int index;

    if (obj == OBJ_HANDLE_NULL) {
        return 0;
    }

    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    for (index = 0; index < 512; index++) {
        // NOTE: Unclear, likely no-op.
    }

    return 0;
}

// 0x450B40
int sub_450B40(int64_t obj)
{
    if (obj == OBJ_HANDLE_NULL) {
        return 0;
    }

    if (!obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return 0;
    }

    return sub_450B90(obj) - sub_450AC0(obj);
}

// 0x450B90
int sub_450B90(int64_t obj)
{
    return stat_level_get(obj, STAT_INTELLIGENCE) / 4;
}

// 0x450C10
void sub_450C10(int64_t obj, unsigned int flags)
{
    obj_field_int32_set(obj, OBJ_F_SPELL_FLAGS, obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) | flags);
}

// 0x450C40
void magictech_effect_summon(MagicTechSummonInfo* summon_info)
{
    int64_t proto_obj;
    int64_t obj;

    proto_obj = objp_perm_lookup(summon_info->field_60);

    if (tig_net_is_active()) {
        if (tig_net_is_host()) {
            PacketSummon pkt;

            if (!object_create(proto_obj, summon_info->loc, &obj)) {
                tig_debug_printf("magictech_effect_summon: Error: object create failed!\n");
                exit(EXIT_FAILURE);
            }

            pkt.type = 73;
            pkt.summon_info = *summon_info;
            pkt.summon_info.field_88 = obj_get_id(obj);
            sub_443EB0(pkt.summon_info.field_0.obj, &(pkt.summon_info.field_0.field_8));
            sub_443EB0(pkt.summon_info.field_30.obj, &(pkt.summon_info.field_30.field_8));
            summon_info->field_A0 = qword_5E75B8;
            sub_4F0640(qword_5E75B8, &(pkt.summon_info.field_A8));
            tig_net_send_app_all(&pkt, sizeof(pkt));

            if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_SCENERY) {
                sub_424070(obj, 5, false, false);
                sub_43F710(obj);
            }
        } else {
            if (!multiplayer_is_locked()) {
                tig_debug_println("MP: MagicTech: magictech_effect_summon called without MP set correctly, tell smoret@troikagames.com");
                return;
            }

            object_create_ex(proto_obj, summon_info->loc, summon_info->field_88, &obj);
        }
    } else {
        if (!object_create(proto_obj, summon_info->loc, &obj)) {
            tig_debug_printf("magictech_effect_summon: Error: object create failed!\n");
            exit(EXIT_FAILURE);
        }
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    *summon_info->summoned_obj_ptr = obj;

    sub_450C10(obj, OSF_SUMMONED);

    if ((obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_NPC)) {
        tig_art_id_t art_id;
        int64_t summoner_loc;
        int rot;

        summoner_loc = obj_field_int64_get(summon_info->field_0.obj, OBJ_F_LOCATION);
        rot = location_rot(summon_info->loc, summoner_loc);

        art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
        art_id = tig_art_id_rotation_set(art_id, rot);
        art_id = tig_art_id_palette_set(art_id, summon_info->palette);
        object_set_current_aid(obj, art_id);

        critter_set_concealed(obj, true);
        object_flags_set(obj, OF_DONTDRAW);

        obj_field_int32_set(obj,
            OBJ_F_CRITTER_FLAGS2,
            obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2) | OCF2_NO_PICKPOCKET);

        obj_field_int32_set(obj, OBJ_F_NPC_EXPERIENCE_WORTH, 0);
        obj_field_int32_set(obj, OBJ_F_NPC_EXPERIENCE_POOL, 0);

        if (summon_info->field_C8) {
            obj_field_int32_set(obj, OBJ_F_NPC_FACTION, 0);
        } else {
            if ((!tig_net_is_active()
                    || tig_net_is_host())
                && dword_5E75F0->parent_obj.obj != OBJ_HANDLE_NULL) {
                stat_base_set(obj,
                    STAT_ALIGNMENT,
                    stat_level_get(dword_5E75F0->parent_obj.obj, STAT_ALIGNMENT));
            }

            if (summon_info->field_0.obj != OBJ_HANDLE_NULL
                && obj_field_int32_get(summon_info->field_0.obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
                obj_field_int32_set(obj,
                    OBJ_F_NPC_FACTION,
                    obj_field_int32_get(summon_info->field_0.obj, OBJ_F_NPC_FACTION));
            }
        }
    } else {
        sub_4372B0(obj, summon_info->field_0.obj);
    }

    sub_463630(obj);
}

// 0x451070
void sub_451070(MagicTechRunInfo* run_info)
{
    if (dword_5B0BA4 != -1 && dword_5B0BA4 != run_info->id) {
        tig_debug_printf("\n\nMagicTech: ERROR: Process function is NOT Re-Entrant, Spell: %d (%s)!\n",
            run_info->spell,
            magictech_get_name(run_info->spell));
        return;
    }

    dword_5E75F0 = run_info;
    dword_5E75AC = -1;
    dword_5E75CC = 0;
    dword_5E75D4 = 0;
    dword_5E75D8 = 0;
    dword_5E75DC = 0;
    qword_5E75E0 = 0;
    dword_5E75E8 = run_info->action;
    sub_4510F0();
}

// 0x4510F0
void sub_4510F0()
{
    int idx;
    int comp;

    if (dword_5E75F0->id == -1) {
        return;
    }

    if (tig_net_is_active() && !tig_net_is_host()) {
        return;
    }

    dword_5E7598 = &(magictech_spells[dword_5E75F0->spell]);
    dword_5E7620 = &(dword_5E7598->resistance);
    dword_5E759C = &(dword_5E7598->components[dword_5E75F0->action]);
    dword_5E75F0->field_13C |= 0x04;
    dword_5B0BA4 = dword_5E75F0->id;

    if (dword_5E75F0->action == MAGICTECH_ACTION_BEGIN
        && !sub_456430(dword_5E75F0->parent_obj.obj, dword_5E75F0->target_obj.obj, dword_5E7598)) {
        dword_5B0BA4 = -1;
        if (dword_5E75F0->action < MAGICTECH_ACTION_END) {
            magictech_interrupt_delayed(dword_5E75F0->id);
        }
        return;
    }

    if (!sub_452F20()) {
        dword_5B0BA4 = -1;
        if (dword_5E75F0->action < MAGICTECH_ACTION_END) {
            magictech_interrupt_delayed(dword_5E75F0->id);
        }
        return;
    }

    sub_453630();

    if (!sub_453710()) {
        dword_5B0BA4 = -1;
        if (dword_5E75F0->action < MAGICTECH_ACTION_END) {
            magictech_interrupt_delayed(dword_5E75F0->id);
        }
        return;
    }

    if (dword_5E759C->cnt > 0) {
        stru_5E6D28.field_0 = &(dword_5E7598->field_70[dword_5E75F0->action]);
        sub_4F40B0(&stru_5E6D28);

        if (dword_5E75F0->source_obj.obj != OBJ_HANDLE_NULL
            && obj_field_int32_get(dword_5E75F0->source_obj.obj, OBJ_F_TYPE) == OBJ_TYPE_PC
            && fate_resolve(dword_5E75F0->source_obj.obj, FATE_SPELL_AT_MAXIMUM)) {
            dword_5E75F0->source_obj.aptitude = 100;
            dword_5E75F0->parent_obj.aptitude = 100;
            dword_5E75A8 = true;
        } else {
            dword_5E75A8 = false;
        }

        if (stru_5E3518.cnt == 0) {
            dword_5E75DC = 1;
        }

        for (idx = 0; idx < stru_5E3518.cnt; idx++) {
            stru_5E6D28.field_20 = stru_5E3518.entries[idx].obj;
            stru_5E6D28.field_28 = stru_5E3518.entries[idx].loc;
            if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
                dword_5E75AC = obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_TYPE);
            }

            if (sub_455550(&stru_5E6D28, dword_5E75F0)) {
                if (sub_4537B0()) {
                    for (comp = 0; comp < dword_5E759C->cnt; comp++) {
                        dword_5E761C = &(dword_5E759C->entries[comp]);
                        stru_5E6D28.field_0 = &(dword_5E761C->aoe);

                        if (sub_4F2D20(&stru_5E6D28)) {
                            sub_453D40();

                            if (dword_5E7598->item_triggers == 0
                                || dword_5E761C->item_triggers == 0
                                || ((dword_5E75F0->trigger & dword_5E761C->item_triggers) != 0
                                    && ((dword_5E761C->item_triggers & 0x100) == 0
                                        || (dword_5E75F0->trigger & 0x100) != 0))) {
                                dword_5E75DC = 1;
                                sub_453EE0();
                                magictech_procs[dword_5E761C->type]();

                                if (dword_5E7624) {
                                    comp = dword_5E6D90;
                                    dword_5E7624 = false;
                                }
                            }
                        }
                    }

                    if (dword_5E75F0->source_obj.obj != qword_5E75B0) {
                        qword_5E75B8 = dword_5E75F0->source_obj.obj;
                        dword_5E75F0->source_obj.obj = stru_5E6D28.field_20;
                        stru_5E6D28.field_20 = qword_5E75B8;
                        if (qword_5E75B8 != OBJ_HANDLE_NULL) {
                            dword_5E75AC = obj_field_int32_get(qword_5E75B8, OBJ_F_TYPE);
                        }
                    }
                } else {
                    sub_45A520(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20);
                }
            }
        }
    } else {
        dword_5E75DC = 1;
    }

    dword_5B0BA4 = -1;
    sub_453FA0();
}

// 0x4514E0
void MTComponentAGoal_ProcFunc()
{
    int64_t loc;
    int64_t new_loc;
    AnimGoalData goal_data;
    tig_art_id_t art_id;
    int rot;

    switch (dword_5E761C->data.agoal.goal) {
    case AG_KNOCKBACK:
        if (dword_5E75F0->parent_obj.obj != OBJ_HANDLE_NULL) {
            loc = obj_field_int64_get(dword_5E75F0->parent_obj.obj, OBJ_F_LOCATION);
        } else {
            loc = stru_5E6D28.field_18;
        }

        if (loc != 0) {
            new_loc = obj_field_int64_get(stru_5E6D28.field_20, OBJ_F_LOCATION);
            if (loc == new_loc) {
                art_id = obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_CURRENT_AID);
                rot = (tig_art_id_rotation_get(art_id) + 4) % 8;
            } else {
                rot = location_rot(loc, new_loc);
            }

            anim_goal_knockback(stru_5E6D28.field_20, rot, 4, dword_5E75F0->parent_obj.obj);
        }
        break;
    case AG_FOLLOW:
        if (dword_5E75F0->parent_obj.obj != OBJ_HANDLE_NULL) {
            critter_follow(qword_5E75B8, dword_5E75F0->parent_obj.obj, true);
        }
        break;
    case AG_KNOCK_DOWN:
        anim_goal_knockdown(stru_5E6D28.field_20);
        break;
    default:
        if (dword_5E75F0->parent_obj.obj != OBJ_HANDLE_NULL
            && sub_44D500(&goal_data, stru_5E6D28.field_20, dword_5E761C->data.agoal.goal)) {
            goal_data.params[AGDATA_TARGET_OBJ].obj = dword_5E75F0->parent_obj.obj;
            if ((dword_5E761C->data.agoal.subgoal & 1) != 0) {
                // __FILE__: "C:\Troika\Code\Game\GameLibX\MagicTech.c"
                // __LINE__: 3277
                sub_44DBE0(stru_5E75C0, &goal_data, __FILE__, __LINE__);
            } else {
                sub_44D520(&goal_data, &stru_5E75C0);
            }
        }
        break;
    }
}

// 0x4516D0
void MTComponentAGoalTerminate_ProcFunc()
{
    if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
        sub_44E4D0(stru_5E6D28.field_20, dword_5E761C->data.agoal_terminate.goal, -1);
    }
}

// 0x451700
void MTComponentAIRedirect_ProcFunc()
{
    AiRedirect ai_redirect;

    ai_redirect_init(&ai_redirect, stru_5E6D28.field_20, stru_5E6D28.field_20);
    ai_redirect.critter_flags = dword_5E761C->data.ai_redirect.critter_flags;
    ai_redirect.min_iq = dword_5E761C->data.ai_redirect.min_iq;
    ai_redirect_perform(&ai_redirect);
}

// 0x451740
void MTComponentCast_ProcFunc()
{
    MagicTechInvocation mt_invocation;

    magictech_invocation_init(&mt_invocation, OBJ_HANDLE_NULL, dword_5E761C->data.cast.spell);
    sub_4440E0(stru_5E6D28.field_20, &(mt_invocation.target_obj));
    sub_4440E0(dword_5E75F0->parent_obj.obj, &(mt_invocation.parent_obj));
    mt_invocation.flags |= MAGICTECH_INVOCATION_FRIENDLY;
    mt_invocation.target_loc = stru_5E6D28.field_28;

    if (mt_invocation.parent_obj.obj != OBJ_HANDLE_NULL) {
        mt_invocation.loc = obj_field_int64_get(mt_invocation.parent_obj.obj, OBJ_F_LOCATION);
    }

    if (mt_invocation.target_loc == 0) {
        if (mt_invocation.target_obj.obj == OBJ_HANDLE_NULL) {
            return;
        }

        mt_invocation.target_loc = obj_field_int64_get(mt_invocation.target_obj.obj, OBJ_F_LOCATION);
    }

    if (mt_invocation.target_obj.obj != OBJ_HANDLE_NULL || mt_invocation.target_loc != 0) {
        magictech_invocation_run(&mt_invocation);
    }
}

// 0x451850
void MTComponentChargeNBranch_ProcFunc()
{
    if (dword_5E75F0->source_obj.obj != OBJ_HANDLE_NULL
        && !sub_450420(dword_5E75F0->source_obj.obj, dword_5E761C->data.charge_branch.cost, true, dword_5E75F0->spell)) {
        if (dword_5E761C->data.charge_branch.branch != -1) {
            dword_5E7624 = true;
            dword_5E6D90 = SDL_min(dword_5E761C->data.charge_branch.branch, dword_5E759C->cnt);
        } else {
            dword_5E7624 = true;
            dword_5E6D90 = dword_5E759C->cnt;
        }
    }
}

// 0x4518D0
void MTComponentDamage_ProcFunc()
{
    CombatContext combat;
    int dam_min;
    int dam_max;

    if (stru_5E6D28.field_20 == OBJ_HANDLE_NULL) {
        return;
    }

    sub_4B2210(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20, &combat);

    combat.dam_flags |= dword_5E761C->data.damage.damage_flags;

    sub_453F20(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20);

    if (dword_5E75F0->field_E8.obj != OBJ_HANDLE_NULL) {
        combat.field_30 = dword_5E75F0->field_E8.obj;
    }

    dam_min = dword_5E761C->data.damage.damage_min;
    dam_max = dword_5E761C->data.damage.damage_max;

    if ((combat.dam_flags & CDF_SCALE) != 0) {
        if (obj_type_is_critter(dword_5E75F0->parent_obj.type)) {
            int aptitude = dword_5E75F0->parent_obj.aptitude;
            if (aptitude > 0) {
                dam_max = dam_min + aptitude * (dam_max - dam_min) / 100;
                dam_min = dam_max;
            } else {
                dam_max = dam_min;
            }
        }

        combat.dam_flags &= ~CDF_SCALE;
    }

    if ((combat.dam_flags & CDF_FULL) != 0) {
        if (dword_5E761C->data.damage.damage_type == DAMAGE_TYPE_FATIGUE) {
            combat.dam[DAMAGE_TYPE_FATIGUE] = critter_fatigue_current(stru_5E6D28.field_20) + 10;
            combat.dam_flags &= ~CDF_FULL;
        }
    } else {
        if (dword_5E761C->data.damage.damage_type < DAMAGE_TYPE_COUNT) {
            combat.dam[dword_5E761C->data.damage.damage_type] = random_between(dam_min, dam_max);
        } else {
            combat.dam[DAMAGE_TYPE_NORMAL] = random_between(dam_min, dam_max);
        }
    }

    combat.flags |= 0x200 | 0x80;

    if (dword_5E761C->data.damage.damage_type < DAMAGE_TYPE_COUNT) {
        if (dword_5E75F0->field_144 != 0) {
            if ((combat.dam_flags & CDF_DEATH) == 0) {
                int resisted = dword_5E75F0->field_144 * combat.dam[dword_5E761C->data.damage.damage_type] / 100;
                if (resisted == 0) {
                    resisted = 1;
                }
                combat.dam[dword_5E761C->data.damage.damage_type] -= resisted;
                combat_dmg(&combat);
            }
        } else {
            combat_dmg(&combat);
        }
    } else if (dword_5E761C->data.damage.damage_type == DAMAGE_TYPE_COUNT) {
        combat_acid_dmg(&combat);
    }

    dword_5E75D0 = obj_field_int32_get(combat.target_obj, OBJ_F_FLAGS);

    if ((dword_5E75D0 & (OF_DESTROYED | OF_OFF)) == 0
        && (combat.dam_flags & CDF_HAVE_DAMAGE) != 0) {
        anim_play_blood_splotch_fx(combat.target_obj, BLOOD_SPLOTCH_TYPE_NORMAL, dword_5E761C->data.damage.damage_type, &combat);
    }
}

// 0x451AF0
void MTComponentDestroy_ProcFunc()
{
    unsigned int spell_flags;

    if (stru_5E6D28.field_20 == OBJ_HANDLE_NULL) {
        return;
    }

    spell_flags = obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_SPELL_FLAGS);
    if (!dword_5E75F0->field_144 || (spell_flags & OSF_SUMMONED) != 0) {
        object_destroy(stru_5E6D28.field_20);

        if (tig_net_is_active()
            && tig_net_is_host()) {
            PacketObjectDestroy pkt;

            pkt.type = 72;
            pkt.oid = obj_get_id(stru_5E6D28.field_20);
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }
}

// 0x451B90
void MTComponentDispel_ProcFunc()
{
    sub_451BB0(stru_5E6D28.field_20, dword_5E75F0->id);
}

// 0x451BB0
void sub_451BB0(int64_t obj, int mt_id)
{
    if (!multiplayer_is_locked()) {
        Packet74 pkt;

        if (!tig_net_is_host()) {
            return;
        }

        pkt.type = 74;
        pkt.subtype = 1;
        pkt.mt_id = mt_id;
        if (obj != OBJ_HANDLE_NULL) {
            pkt.oid = obj_get_id(obj);
        } else {
            pkt.oid.type = OID_TYPE_NULL;
        }
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    if (obj != OBJ_HANDLE_NULL) {
        sub_451C40(mt_id, obj);
    }
}

// 0x451C40
void sub_451C40(int mt_id, int64_t obj)
{
    int obj_type;
    int index;
    MagicTechRunInfo* run_info;
    MagicTechObjectNode* node;
    unsigned int flags;

    if (mt_id == -1) {
        return;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);

    for (index = 0; index < 512; index++) {
        run_info = &(magictech_run_info[index]);
        if (run_info->source_obj.obj == obj
            && run_info->id != mt_id
            && (magictech_spells[run_info->spell].flags & MAGICTECH_IS_TECH) == 0) {
            magictech_interrupt_delayed(run_info->id);
        }
    }

    for (index = 0; index < 512; index++) {
        run_info = &(magictech_run_info[index]);
        if ((run_info->field_13C & 0x1) != 0) {
            if (run_info->target_obj.obj == obj
                && index != mt_id
                && (magictech_spells[run_info->spell].flags & MAGICTECH_IS_TECH) == 0) {
                magictech_interrupt_delayed(run_info->id);
            }

            node = run_info->summoned_obj;
            while (node != NULL) {
                if (node->obj == obj
                    && index != mt_id
                    && (magictech_spells[run_info->spell].flags & MAGICTECH_IS_TECH) == 0) {
                    magictech_interrupt_delayed(run_info->id);
                }
                node = node->next;
            }

            node = run_info->objlist;
            while (node != NULL) {
                if (node->obj == obj
                    && index != mt_id
                    && (magictech_spells[run_info->spell].flags & MAGICTECH_IS_TECH) == 0) {
                    magictech_interrupt_delayed(run_info->id);
                }
                node = node->next;
            }
        }
    }

    switch (obj_type) {
    case OBJ_TYPE_PORTAL:
        flags = obj_field_int32_get(obj, OBJ_F_PORTAL_FLAGS);
        if ((flags & OPF_MAGICALLY_HELD) != 0) {
            flags &= ~OPF_MAGICALLY_HELD;
            obj_field_int32_set(obj, OBJ_F_PORTAL_FLAGS, flags);
        }
        break;
    case OBJ_TYPE_CONTAINER:
        flags = obj_field_int32_get(obj, OBJ_F_CONTAINER_FLAGS);
        if ((flags & OCOF_MAGICALLY_HELD) != 0) {
            flags &= ~OCOF_MAGICALLY_HELD;
            obj_field_int32_set(obj, OBJ_F_CONTAINER_FLAGS, flags);
        }
        break;
    case OBJ_TYPE_PC:
    case OBJ_TYPE_NPC:
        flags = obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS);
        if ((flags & OSF_STONED) != 0) {
            flags &= ~OSF_STONED;
            obj_field_int32_set(obj, OBJ_F_SPELL_FLAGS, flags);
        }
        if ((flags & OSF_SUMMONED) != 0) {
            object_destroy(obj);
        }

        flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2);
        if ((flags & OCF2_PERMA_POLYMORPH) != 0) {
            sub_452CD0(obj, obj_field_int32_get(obj, OBJ_F_CURRENT_AID));

            flags &= ~OCF2_PERMA_POLYMORPH;
            obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS2, flags);
            effect_remove_one_typed(obj, EFFECT_POLYMORPH);
        }
        break;
    }
}

// 0x451F20
void MTComponentEffect_ProcFunc()
{
    int scale;
    int cnt;

    if (stru_5E6D28.field_20 == OBJ_HANDLE_NULL) {
        return;
    }

    scale = dword_5E761C->data.effect.scaled;
    cnt = dword_5E761C->data.effect.count;

    if (scale > 0) {
        if (obj_type_is_critter(dword_5E75F0->parent_obj.type)) {
            cnt += scale * dword_5E75F0->parent_obj.aptitude / 100;
        }
    }

    if (dword_5E761C->data.effect.add_remove == 0) {
        while (cnt > 0) {
            effect_remove_one_typed(stru_5E6D28.field_20, dword_5E761C->data.effect.num);
            cnt--;
        }
    } else {
        while (cnt > 0) {
            effect_add(stru_5E6D28.field_20,
                dword_5E761C->data.effect.num,
                dword_5E761C->data.effect.cause);
            cnt--;
        }
    }
}

// 0x451FE0
void MTComponentEyeCandy_ProcFunc()
{
    int mt_id;

    if (stru_5E6D28.field_20 == OBJ_HANDLE_NULL) {
        return;
    }

    if (dword_5E761C->data.eye_candy.add_remove == 0) {
        if (!tig_net_is_active()
            || tig_net_is_host()) {
            mt_id = (dword_5E761C->data.eye_candy.flags & 0x100) == 0
                ? dword_5E75F0->id
                : -1;
            animfx_remove(&spell_eye_candies,
                stru_5E6D28.field_20,
                dword_5E761C->data.eye_candy.num + 6 * dword_5E75F0->spell,
                mt_id);

            if (tig_net_is_active()) {
                PacketMagicTechEyeCandy pkt;

                pkt.type = 77;
                pkt.subtype = 0;
                pkt.oid = obj_get_id(stru_5E6D28.field_20);
                pkt.fx_id = dword_5E761C->data.eye_candy.num + 6 * dword_5E75F0->spell;
                pkt.mt_id = mt_id;
                tig_net_send_app_all(&pkt, sizeof(pkt));
            }
        }
    } else {
        AnimFxNode node;

        sub_4CCD20(&spell_eye_candies,
            &node,
            stru_5E6D28.field_20,
            dword_5E75F0->id,
            dword_5E761C->data.eye_candy.num + 6 * dword_5E75F0->spell);

        if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
            node.rotation = tig_art_id_rotation_get(obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_CURRENT_AID));
        }

        node.animate = true;
        node.flags = dword_5E761C->data.eye_candy.flags;
        node.parent_obj = dword_5E75F0->parent_obj.obj;

        if ((dword_5E75F0->field_13C & 0x40) == 0) {
            if (animfx_add(&node)) {
                if ((node.flags & (ANIMFX_PLAY_CALLBACK | ANIMFX_PLAY_END_CALLBACK)) != 0) {
                    dword_5E75CC = 1;
                }
                return;
            }
        }

        if ((node.flags & (ANIMFX_PLAY_CALLBACK | ANIMFX_PLAY_END_CALLBACK)) != 0) {
            dword_5E75D4 = 1;
            if ((node.flags & ANIMFX_PLAY_END_CALLBACK) != 0) {
                dword_5E75D8 = 1;
            }
        }
    }
}

// 0x4521A0
void MTComponentHeal_ProcFunc()
{
    CombatContext combat;
    int heal_min;
    int heal_max;

    if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
        sub_4B2210(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20, &combat);
        combat.dam_flags |= dword_5E761C->data.heal.damage_flags;
        if ((combat.dam_flags & CDF_FULL) == 0) {
            heal_min = dword_5E761C->data.heal.damage_min;
            heal_max = dword_5E761C->data.heal.damage_max;
            if ((combat.dam_flags & CDF_SCALE) != 0
                && obj_type_is_critter(dword_5E75F0->parent_obj.type)) {
                if (dword_5E75F0->parent_obj.aptitude > 0) {
                    heal_max = heal_min + dword_5E75F0->parent_obj.aptitude * (heal_max - heal_min) / 100;
                    heal_min = heal_max;
                } else {
                    heal_max = heal_min;
                }
                combat.dam_flags &= ~CDF_SCALE;
            }

            if (dword_5E761C->data.heal.damage_type == DAMAGE_TYPE_POISON) {
                combat.dam[DAMAGE_TYPE_POISON] = random_between(heal_min, heal_max);
            } else if (dword_5E761C->data.heal.damage_type == DAMAGE_TYPE_FATIGUE) {
                combat.dam[DAMAGE_TYPE_FATIGUE] = random_between(heal_min, heal_max);
            } else {
                combat.dam[DAMAGE_TYPE_NORMAL] = random_between(heal_min, heal_max);
            }
        }

        combat_heal(&combat);
    }
}

// 0x4522A0
void MTComponentIdentify_ProcFunc()
{
    if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
        if (obj_type_is_critter(dword_5E75AC)) {
            sub_4EE4C0(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20);
        } else if (dword_5E75AC == OBJ_TYPE_CONTAINER) {
            mp_ui_show_inven_identify(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20);
        } else {
            sub_4EE3A0(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20);
        }
    }
}

// 0x452300
void MTComponentInterrupt_ProcFunc()
{
    MagicTechInvocation mt_invocation;

    magictech_invocation_init(&mt_invocation, OBJ_HANDLE_NULL, dword_5E761C->data.interrupt.magictech);
    sub_4440E0(stru_5E6D28.field_20, &(mt_invocation.target_obj));
    mt_invocation.target_loc = stru_5E6D28.field_28;
    if (mt_invocation.target_obj.obj != OBJ_HANDLE_NULL || mt_invocation.target_loc != OBJ_HANDLE_NULL) {
        sub_4573D0(&mt_invocation);
    }
}

// 0x452380
void MTComponentObjFlag_ProcFunc()
{
    if (dword_5E761C->data.obj_flag.state == 1) {
        dword_5E75F0->field_138 |= dword_5E761C->data.obj_flag.value;
    } else {
        dword_5E75F0->field_138 &= ~dword_5E761C->data.obj_flag.value;
    }

    magictech_component_obj_flag(stru_5E6D28.field_20,
        stru_5E6D28.self_obj,
        dword_5E761C->data.obj_flag.flags_fld,
        dword_5E761C->data.obj_flag.value,
        dword_5E761C->data.obj_flag.state,
        dword_5E75F0->parent_obj.obj,
        dword_5E75F0->source_obj.obj);

    if (tig_net_is_active()
        && tig_net_is_host()) {
        PacketMagicTechObjFlag pkt;

        sub_4F0640(stru_5E6D28.field_20, &(pkt.field_8));
        sub_4F0640(stru_5E6D28.self_obj, &(pkt.self_oid));
        pkt.fld = dword_5E761C->data.obj_flag.flags_fld;
        pkt.value = dword_5E761C->data.obj_flag.value;
        pkt.state = dword_5E761C->data.obj_flag.state;
        sub_4F0640(dword_5E75F0->parent_obj.obj, &(pkt.parent_oid));
        sub_4F0640(dword_5E75F0->source_obj.obj, &(pkt.source_oid));

        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x4524C0
void MTComponentMovement_ProcFunc()
{
    int64_t loc = 0;

    if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL
        && !dword_5E75F0->field_144) {
        switch (dword_5E761C->data.movement.move_location) {
        case 0:
            loc = dword_5E75F0->target_obj.loc;
            break;
        case 1:
            sub_4F4E40(dword_5E75F0->target_obj.obj, dword_5E761C->data.movement.tile_radius, &loc);
            break;
        case 2:
            if (antiteleport_check(dword_5E75F0->parent_obj.obj, 0)) {
                if (tig_net_is_active()) {
                    TeleportData teleport_data;

                    map_starting_loc_get(&loc);

                    teleport_data.loc = loc;
                    teleport_data.map = map_current_map();
                    teleport_data.flags = TELEPORT_0x0020 | TELEPORT_0x0100;
                    teleport_data.obj = stru_5E6D28.field_20;
                    teleport_do(&teleport_data);
                    return;
                }

                loc = 0;
                ui_wmap_select(dword_5E75F0->source_obj.obj, dword_5E75F0->spell);
            } else {
                MesFileEntry mes_file_entry;

                mes_file_entry.num = 10000;
                mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
                sub_460610(mes_file_entry.str);
            }
            break;
        case 3:
            sub_452650(stru_5E6D28.field_20);
            break;
        }

        if (loc != 0) {
            sub_455350(stru_5E6D28.field_20, loc);
        }
    }
}

// 0x452650
void sub_452650(int64_t obj)
{
    int area = AREA_UNKNOWN;
    int cur_map;
    int start_map;
    int64_t loc;
    MesFileEntry mes_file_entry;
    TeleportData teleport_data;

    cur_map = map_current_map();
    start_map = map_by_type(MAP_TYPE_START_MAP);

    if (cur_map == start_map) {
        area = area_get_nearest_area_in_range(obj_field_int64_get(obj, OBJ_F_LOCATION), true);
    } else {
        if (!tig_net_is_active()) {
            map_get_area(cur_map, &area);
        }
    }

    if (area > 0 && antiteleport_check(stru_5E6D28.field_20, 0)) {
        loc = area_get_location(area);

        if (tig_net_is_active()) {
            sector_flush(0);

            teleport_data.loc = loc;
            teleport_data.map = map_current_map();
            teleport_data.flags = TELEPORT_0x0100 | TELEPORT_0x0020;
            teleport_data.obj = obj;
            teleport_do(&teleport_data);
        } else {
            sector_flush(0);

            teleport_data.loc = loc;
            teleport_data.map = map_by_type(MAP_TYPE_START_MAP);
            teleport_data.flags = TELEPORT_0x0100 | TELEPORT_0x0020;
            teleport_data.obj = obj;
            teleport_do(&teleport_data);
        }
    } else {
        if (player_is_local_pc_obj(obj)) {
            mes_file_entry.num = 1000;
            mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
            sub_460610(mes_file_entry.str);
        }
    }
}

// 0x452800
void MTComponentRecharge_ProcFunc()
{
    sub_454920(stru_5E6D28.field_20,
        dword_5E761C->data.recharge.num,
        dword_5E761C->data.recharge.max);
}

// 0x452830
void MTComponentSummon_ProcFunc()
{
    MagicTechSummonInfo summon_info;

    summon_info.field_0.obj = dword_5E75F0->parent_obj.obj;
    summon_info.field_30.obj = dword_5E75F0->target_obj.obj;
    summon_info.loc = stru_5E6D28.field_28;
    summon_info.summoned_obj_ptr = &qword_5E75B8;
    summon_info.palette = dword_5E761C->data.summon.palette;
    summon_info.field_C8 = dword_5E761C->data.summon.list;

    if (dword_5E761C->data.summon.oid.type != OID_TYPE_NULL) {
        summon_info.field_60 = dword_5E761C->data.summon.oid;
    } else {
        magictech_pick_proto_from_list(&(summon_info.field_60), dword_5E761C->data.summon.list);
    }

    magictech_effect_summon(&summon_info);

    stru_5E6D28.field_48 = qword_5E75B8;
    sub_4554B0(dword_5E75F0, qword_5E75B8);
}

// 0x452900
void magictech_pick_proto_from_list(ObjectID* oid, int list)
{
    int rnd;
    int idx;

    switch (stru_5B0ED8[list].type) {
    case 0:
        rnd = random_between(0, 100);
        for (idx = 0; idx < stru_5B0ED8[list].cnt; idx++) {
            if (rnd < stru_5B0ED8[list].entries[idx].value) {
                break;
            }

            rnd -= stru_5B0ED8[list].entries[idx].value;
        }
        if (idx >= stru_5B0ED8[list].cnt) {
            tig_debug_printf("MagicTech: Error: magictech_pick_proto_from_list: went off end of list!\n");
            idx = 0;
        }
        break;
    case 1:
        for (idx = 0; idx < stru_5B0ED8[list].cnt; idx++) {
            if (dword_5E75F0->parent_obj.aptitude <= stru_5B0ED8[list].entries[idx].value) {
                break;
            }
        }
        break;
    }

    *oid = obj_get_id(sub_4685A0(stru_5B0ED8[list].entries[idx].basic_prototype));
}

// 0x4529D0
void MTComponentTerminate_ProcFunc()
{
    dword_5E7624 = 1;
    dword_5E6D90 = dword_5E759C->cnt;
}

// 0x4529F0
void MTComponentTestNBranch_ProcFunc()
{
    int value;

    if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL && obj_type_is_critter(dword_5E75AC)) {
        value = obj_field_int32_get(stru_5E6D28.field_20, dword_5E761C->data.test_in_branch.field_40);
        switch (dword_5E761C->data.test_in_branch.field_44) {
        case 0:
            if (value == dword_5E761C->data.test_in_branch.field_48) {
                return;
            }
            break;
        case 1:
            if (value > dword_5E761C->data.test_in_branch.field_48) {
                return;
            }
            break;
        case 2:
            if (value >= dword_5E761C->data.test_in_branch.field_48) {
                return;
            }
            break;
        case 3:
            if (value < dword_5E761C->data.test_in_branch.field_48) {
                return;
            }
            break;
        case 4:
            if (value <= dword_5E761C->data.test_in_branch.field_48) {
                return;
            }
            break;
        default:
            tig_debug_printf("MTComponentTestNBranch_ProcFunc: Error: test type out of range!\n");
            break;
        }

        dword_5E7624 = true;
        if (dword_5E761C->data.test_in_branch.field_4C != -1) {
            dword_5E6D90 = dword_5E761C->data.test_in_branch.field_4C;
            if (dword_5E6D90 >= dword_5E759C->cnt) {
                dword_5E6D90 = dword_5E759C->cnt;
            }
        } else {
            dword_5E6D90 = dword_5E759C->cnt;
        }
    }
}

// 0x452AD0
void MTComponentTrait_ProcFunc()
{
    magictech_process(stru_5E6D28.field_20, &(dword_5E761C->data.trait), dword_5E75AC);
}

// 0x452B00
void magictech_process(int64_t obj, MagicTechComponentTrait* trait, int obj_type)
{
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (!multiplayer_is_locked()) {
        Packet74 pkt;

        pkt.type = 74;
        pkt.subtype = 0;
        pkt.oid = obj_get_id(obj);
        pkt.trait = *trait;
        pkt.obj_type = obj_type;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    if (trait->fld == OBJ_F_CURRENT_AID && obj_type_is_critter(obj_type)) {
        tig_art_id_t art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);

        if (trait->value != -1) {
            int inventory_location;
            int weapon;
            int anim;
            int rot;

            for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
                sub_464C50(obj, inventory_location);
                if (!sub_464C50(obj, inventory_location)) {
                    int64_t item_obj = item_wield_get(obj, inventory_location);
                    if (item_obj != OBJ_HANDLE_NULL) {
                        if (!item_drop(item_obj)) {
                            tig_debug_printf("MagicTech: magictech_process: MTComponentTrait: ERROR: Item_Drop Failed!\n");
                        }
                    }
                }
            }

            weapon = tig_art_critter_id_weapon_get(art_id);
            anim = tig_art_id_anim_get(art_id);
            rot = tig_art_id_rotation_get(art_id);

            if (tig_art_monster_id_create(trait->value, 0, 0, 0, rot, anim, weapon, trait->palette, &art_id) != TIG_OK) {
                tig_debug_printf("MagicTech: magictech_process: MTComponentTrait: ERROR: Monster Art Create Failed!\n");
                exit(EXIT_FAILURE);
            }

            object_set_current_aid(obj, art_id);

            if (trait->value >= TIG_ART_MONSTER_SPECIE_FIRE_ELEMENTAL
                && trait->value <= TIG_ART_MONSTER_SPECIE_AIR_ELEMENTAL) {
                obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS2, OCF2_AUTO_ANIMATES);
                obj_field_int32_set(obj, OBJ_F_BLIT_FLAGS, TIG_ART_BLT_BLEND_ADD);
            }
        } else {
            sub_452CD0(obj, art_id);
        }
    }
}

// 0x452CD0
void sub_452CD0(int64_t obj, tig_art_id_t art_id)
{
    int v1;
    unsigned int flags;
    int rotation;
    int anim;
    tig_art_id_t current_art_id;

    if (tig_art_type(art_id) == TIG_ART_TYPE_MONSTER) {
        v1 = tig_art_monster_id_specie_get(art_id);
        if (v1 >= 26 && v1 <= 28) {
            flags = obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2);
            flags &= ~OCF2_AUTO_ANIMATES;
            obj_field_int32_set(obj, OBJ_F_CRITTER_FLAGS2, flags);

            flags = obj_field_int32_get(obj, OBJ_F_BLIT_FLAGS);
            flags &= ~TIG_ART_BLT_BLEND_ADD;
            obj_field_int32_set(obj, OBJ_F_BLIT_FLAGS, flags);
        }
    }

    rotation = tig_art_id_rotation_get(art_id);
    anim = tig_art_id_anim_get(art_id);

    current_art_id = obj_field_int32_get(obj, OBJ_F_AID);
    current_art_id = tig_art_id_rotation_set(current_art_id, rotation);
    current_art_id = tig_art_id_anim_set(current_art_id, anim);
    current_art_id = tig_art_id_frame_set(current_art_id, 0);
    object_set_current_aid(obj, current_art_id);

    item_wield_best_all(obj, OBJ_HANDLE_NULL);
}

// 0x452D80
void MTComponentTraitIdx_ProcFunc()
{
    int value;

    if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL && obj_type_is_critter(dword_5E75AC)) {
        if (dword_5E761C->data.trait_idx.field_44 == OBJ_F_CRITTER_STAT_BASE_IDX) {
            value = stat_base_get(stru_5E6D28.field_20,
                dword_5E761C->data.trait_idx.field_40);
        } else {
            value = obj_arrayfield_int32_get(stru_5E6D28.field_20,
                dword_5E761C->data.trait_idx.field_44,
                dword_5E761C->data.trait_idx.field_40);
        }

        if (dword_5E761C->data.trait_idx.field_48 == 0) {
            value = value * dword_5E761C->data.trait_idx.field_4C / dword_5E761C->data.trait_idx.field_50 + dword_5E761C->data.trait_idx.field_54;
            if (dword_5E761C->data.trait_idx.field_44 == OBJ_F_CRITTER_STAT_BASE_IDX) {
                stat_base_set(stru_5E6D28.field_20,
                    dword_5E761C->data.trait_idx.field_40,
                    value);
            } else {
                obj_arrayfield_int32_set(stru_5E6D28.field_20,
                    dword_5E761C->data.trait_idx.field_44,
                    dword_5E761C->data.trait_idx.field_40, value);
            }
        }
    }
}

// 0x452E40
void MTComponentTrait64_ProcFunc()
{
    switch (dword_5E761C->data.trait64.field_44) {
    case 0:
        obj_field_int64_set(stru_5E6D28.field_20, dword_5E761C->data.trait64.field_40, stru_5E6D28.field_28);
        obj_field_int32_set(stru_5E6D28.field_20, OBJ_F_CRITTER_TELEPORT_MAP, map_current_map());
        break;
    case 1:
    case 2:
        qword_5E75B8 = dword_5E75F0->parent_obj.obj;
        obj_field_handle_set(stru_5E6D28.field_20, dword_5E761C->data.trait64.field_40, qword_5E75B8);
        break;
    }
}

// 0x452ED0
void MTComponentUse_ProcFunc()
{
    if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
        object_script_execute(stru_5E6D28.field_20,
            stru_5E6D28.field_20,
            stru_5E6D28.field_20,
            SAP_USE,
            0);
    }
}

// 0x452F00
void MTComponentNoop_ProcFunc()
{
}

// 0x452F10
void MTComponentEnvFlag_ProcFunc()
{
    tig_debug_printf("MagicTech: Process: Component: Error:  Invalid Component Type!\n");
}

// 0x452F20
bool sub_452F20()
{
    bool v1;
    bool v2;
    MesFileEntry mes_file_entry;

    v1 = false;
    v2 = (dword_5E75F0->field_13C & 2) == 0;

    if (dword_5E75F0->action == MAGICTECH_ACTION_BEGIN) {
        if (obj_type_is_critter(dword_5E75F0->source_obj.type)) {
            if (critter_is_dead(dword_5E75F0->source_obj.obj)) {
                return false;
            }

            if (critter_is_unconscious(dword_5E75F0->source_obj.obj)) {
                return false;
            }
        }

        if (obj_type_is_critter(dword_5E75F0->parent_obj.type)
            && magictech_is_magic(dword_5E75F0->spell)) {
            if (stat_level_get(dword_5E75F0->parent_obj.obj, STAT_INTELLIGENCE) < magictech_min_intelligence(dword_5E75F0->spell)) {
                if (obj_type_is_critter(dword_5E75F0->source_obj.type)) {
                    v1 = true;
                    v2 = false;
                    mes_file_entry.num = 602; // "You lose your concentration."
                }
            }

            if (stat_level_get(dword_5E75F0->parent_obj.obj, STAT_WILLPOWER) < magictech_min_willpower(dword_5E75F0->spell)) {
                if (obj_type_is_critter(dword_5E75F0->source_obj.type)) {
                    v1 = true;
                    v2 = false;
                    mes_file_entry.num = 602; // "You lose your concentration."
                }
            }

            if (v2) {
                if ((dword_5E7598->flags & 0x100) == 0
                    && !sub_4507D0(dword_5E75F0->source_obj.obj, dword_5E75F0->spell)) {
                    v1 = true;
                    mes_file_entry.num = 600; // "Not enough Energy."
                }
            }

            if (v1) {
                if (player_is_local_pc_obj(dword_5E75F0->parent_obj.obj)) {
                    mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
                    sub_460610(mes_file_entry.str);
                }

                return false;
            }
        }

        qword_5E75B8 = dword_5E75F0->target_obj.obj;

        if (qword_5E75B8 == OBJ_HANDLE_NULL) {
            qword_5E75B8 = dword_5E75F0->source_obj.obj;
        }

        if (qword_5E75B8 != OBJ_HANDLE_NULL) {
            if (dword_5E7598->no_stack) {
                int other_mt_id;
                if (sub_453410(dword_5E75F0->id, dword_5E75F0->spell, qword_5E75B8, &other_mt_id)
                    && dword_5E75F0->id != other_mt_id) {
                    return false;
                }
            }
        }

        sub_4534E0(dword_5E75F0);
        return true;
    }

    if (dword_5E75F0->action == MAGICTECH_ACTION_MAINTAIN) {
        if (obj_type_is_critter(dword_5E75F0->parent_obj.type)) {
            if (stat_level_get(dword_5E75F0->parent_obj.obj, STAT_INTELLIGENCE) < magictech_min_intelligence(dword_5E75F0->spell)) {
                if (obj_type_is_critter(dword_5E75F0->source_obj.type)) {
                    v1 = true;
                    v2 = false;
                    mes_file_entry.num = 602; // "You lose your concentration."
                }
            }

            if (stat_level_get(dword_5E75F0->parent_obj.obj, STAT_WILLPOWER) < magictech_min_willpower(dword_5E75F0->spell)) {
                if (obj_type_is_critter(dword_5E75F0->source_obj.type)) {
                    v1 = true;
                    v2 = false;
                    mes_file_entry.num = 602; // "You lose your concentration."
                }
            }

            if (v2) {
                if (!sub_4532F0(dword_5E75F0->source_obj.obj, dword_5E75F0->spell)) {
                    v1 = true;
                    mes_file_entry.num = 601; // "Maintain terminated."
                }
            }

            if (dword_5E75F0->field_144 != 0) {
                if (!sub_453370(dword_5E75F0->source_obj.obj, dword_5E75F0->spell, dword_5E75F0->field_144)) {
                    v1 = true;
                    mes_file_entry.num = 601; // "Maintain terminated."
                }
            }

            if (v1) {
                if (player_is_local_pc_obj(dword_5E75F0->parent_obj.obj)) {
                    mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
                    sub_460610(mes_file_entry.str);
                    ui_spell_maintain_end(dword_5E75F0->id);
                }

                dword_5E75F0->action = MAGICTECH_ACTION_END;

                stru_5E3518.cnt = 0;
                sub_451070(dword_5E75F0);

                return false;
            }
        }

        return true;
    }

    return true;
}

// 0x4532F0
bool sub_4532F0(int64_t obj, int magictech)
{
    int cost;

    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    cost = magictech_get_maintenance(magictech)->cost;
    if (obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))
        && stat_level_get(obj, STAT_RACE) == RACE_DWARF) {
        cost *= 2;
    }

    return sub_450420(obj, cost, true, magictech);
}

// 0x453370
bool sub_453370(int64_t obj, int magictech, int a3)
{
    int cost;
    int v1;

    if (obj == OBJ_HANDLE_NULL) {
        return true;
    }

    cost = magictech_get_maintenance(magictech)->cost;
    v1 = a3 * cost / 100;
    if (v1 == 0) {
        v1 = 1;
    }
    cost += v1;

    if (obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))
        && stat_level_get(obj, STAT_RACE) == RACE_DWARF) {
        cost *= 2;
    }

    return sub_450420(obj, cost, true, magictech);
}

// 0x453410
bool sub_453410(int mt_id, int spell, int64_t obj, int* other_mt_id_ptr)
{
    int idx;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0) {
            if (magictech_run_info[idx].target_obj.obj == obj
                && magictech_run_info[idx].spell == spell
                && magictech_run_info[idx].id != mt_id) {
                if (other_mt_id_ptr != NULL) {
                    *other_mt_id_ptr = idx;
                }
                return true;
            } else if (magictech_run_info[idx].target_obj.obj == OBJ_HANDLE_NULL
                && (magictech_spells[magictech_run_info[idx].spell].field_70[MAGICTECH_ACTION_BEGIN].flags & Tgt_Self) != 0
                && magictech_run_info[idx].source_obj.obj == obj
                && magictech_run_info[idx].spell == spell
                && magictech_run_info[idx].id != mt_id) {
                if (other_mt_id_ptr != NULL) {
                    *other_mt_id_ptr = idx;
                }
                return true;
            }
        }
    }

    return false;
}

// 0x4534E0
void sub_4534E0(MagicTechRunInfo* run_info)
{
    int index;
    MagicTechInfo* info;
    MagicTechRunInfo* other_run_info;

    if ((run_info->field_13C & 0x1) == 0) {
        return;
    }

    info = &(magictech_spells[run_info->spell]);
    if (info->cancels_sf != 0) {
        for (index = 0; index < 512; index++) {
            other_run_info = &(magictech_run_info[index]);
            if ((other_run_info->field_13C & 0x1) != 0
                && other_run_info->source_obj.obj == run_info->source_obj.obj
                && (magictech_spells[other_run_info->spell].cancels_sf & info->cancels_sf) != 0
                && other_run_info->id != run_info->id) {
                magictech_interrupt(other_run_info->id);
            }
        }
    }

    if (info->cancels_envsf != 0 && magictech_check_env_sf(info->cancels_envsf)) {
        for (index = 0; index < 512; index++) {
            other_run_info = &(magictech_run_info[index]);
            if ((other_run_info->field_13C & 0x1) != 0
                && other_run_info->source_obj.obj == run_info->source_obj.obj
                && (magictech_spells[other_run_info->spell].cancels_envsf & info->cancels_envsf) != 0
                && other_run_info->id != run_info->id) {
                magictech_interrupt(other_run_info->id);
            }
        }
    }
}

// 0x453630
void sub_453630()
{
    sub_4F2600(&stru_5E6D28, 0, dword_5E75F0->source_obj.obj);
    stru_5E6D28.field_50 = &stru_5E3518;
    stru_5E6D28.field_54 = &dword_5E75F0->objlist;
    stru_5E6D28.field_58 = &dword_5E75F0->summoned_obj;
    stru_5E6D28.field_30 = dword_5E75F0->target_obj.obj;
    stru_5E6D28.field_38 = dword_5E75F0->target_obj.loc;
    stru_5E6D28.field_40 = dword_5E75F0->field_E8.obj;
    stru_5E6D28.field_48 = 0;
    stru_5E6D28.self_obj = dword_5E75F0->parent_obj.obj;
    qword_5E75B0 = dword_5E75F0->source_obj.obj;

    if (dword_5E75F0->source_obj.obj == OBJ_HANDLE_NULL) {
        stru_5E6D28.field_18 = dword_5E75F0->source_obj.loc;
    }
}

// 0x453710
bool sub_453710()
{
    MesFileEntry mes_file_entry;

    if (dword_5E75F0->source_obj.obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if (dword_5E75F0->action >= MAGICTECH_ACTION_END) {
        return true;
    }

    stru_5E6D28.field_5C = obj_field_int32_get(dword_5E75F0->source_obj.obj, OBJ_F_SPELL_FLAGS);
    if ((stru_5E6D28.field_5C & OSF_ANTI_MAGIC_SHELL) == 0
        || (dword_5E75F0->field_138 & 0x800) != 0) {
        return true;
    }

    if (player_is_local_pc_obj(dword_5E75F0->parent_obj.obj)) {
        mes_file_entry.num = 602;
        mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
        sub_460610(mes_file_entry.str);
        ui_spell_maintain_end(dword_5E75F0->id);
    }

    return false;
}

// 0x4537B0
bool sub_4537B0()
{
    int v1 = 0;

    if (dword_5E75F0->action != MAGICTECH_ACTION_BEGIN) {
        return true;
    }

    if ((dword_5E7598->flags & MAGICTECH_IS_TECH) != 0) {
        return true;
    }

    if ((dword_5E75F0->field_13C & 0x10) != 0) {
        return true;
    }

    dword_5E75A0 = 0;
    dword_5E75F0->field_144 = 0;

    if (stru_5E6D28.field_20 == OBJ_HANDLE_NULL) {
        return true;
    }

    if (dword_5E75F0->spell == SPELL_DISINTEGRATE
        && obj_type_is_critter(obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_TYPE))
        && (obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_CRITTER_FLAGS2) & OCF2_NO_DISINTEGRATE) != 0) {
        return false;
    }

    if (dword_5E75F0->parent_obj.obj != stru_5E6D28.field_20) {
        if (obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_TYPE) == OBJ_TYPE_PC
            && fate_resolve(stru_5E6D28.field_20, FATE_SAVE_AGAINST_MAGICK)) {
            dword_5E75A0 = 100;
            dword_5E75F0->field_144 = 10;
        } else {
            int resistance = obj_arrayfield_int32_get(stru_5E6D28.field_20, OBJ_F_RESISTANCE_IDX, RESISTANCE_TYPE_MAGIC);
            if (!dword_5E75A8) {
                int aptitude = stat_level_get(stru_5E6D28.field_20, STAT_MAGICK_TECH_APTITUDE);
                if (aptitude < 0) {
                    resistance = 100 - (100 - resistance) * (aptitude + 100) / 100;
                }
            }

            int chance = random_between(1, 100);
            if (chance < resistance) {
                dword_5E75A0 = resistance - chance;
                dword_5E75F0->field_144 = resistance - chance;
            }

            if (resistance > 0
                && (dword_5E7598->flags & MAGICTECH_HAVE_DAMAGE) == 0
                && dword_5E7598->maintenance.period <= 0) {
                if (dword_5E7620->stat != -1) {
                    v1 = dword_5E75F0->field_144 / 10;
                } else {
                    if (chance < resistance) {
                        return false;
                    }
                }
            }
        }
    }

    if (stru_5E6D28.field_20 == OBJ_HANDLE_NULL
        || dword_5E75F0->parent_obj.obj == stru_5E6D28.field_20
        || dword_5E7620->stat == -1
        || !obj_type_is_critter(dword_5E75AC)) {
        return true;
    }

    if (dword_5E7620->stat == STAT_WILLPOWER
        && stat_is_extraordinary(stru_5E6D28.field_20, STAT_WILLPOWER)) {
        if ((dword_5E7598->flags & MAGICTECH_AGGRESSIVE) != 0
            && dword_5E75F0->action != MAGICTECH_ACTION_END) {
            sub_453F20(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20);
        }
        return false;
    }

    int v2 = dword_5E7620->value + stat_level_get(stru_5E6D28.field_20, dword_5E7620->stat) - v1;
    if (v2 > 0) {
        int v3 = random_between(1, 20);
        if (v3 <= v2) {
            dword_5E75F0->field_144 += v2 - v3;
            dword_5E75A0 = dword_5E75F0->field_144;

            if ((dword_5E7598->flags & MAGICTECH_HAVE_DAMAGE) != 0) {
                if (dword_5E75F0->field_144 < 50) {
                    dword_5E75A0 = 50;
                    dword_5E75F0->field_144 = 50;
                }
            } else {
                if (dword_5E7598->maintenance.period == 0) {
                    return false;
                }
            }
        }
    }

    return true;
}

// 0x453B20
int sub_453B20(int64_t attacker_obj, int64_t target_obj, int spell)
{
    MagicTechInfo* info;
    int obj_type;
    int resistance = 0;
    int v2 = 0;

    if (target_obj == OBJ_HANDLE_NULL) {
        return 0;
    }

    obj_type = obj_field_int32_get(target_obj, OBJ_F_TYPE);
    info = &(magictech_spells[spell]);

    if ((info->flags & MAGICTECH_IS_TECH) != 0) {
        if (attacker_obj != target_obj) {
            resistance = obj_arrayfield_int32_get(target_obj, OBJ_F_RESISTANCE_IDX, RESISTANCE_TYPE_MAGIC);
            if (!dword_5E75A8) {
                int aptitude = stat_level_get(target_obj, STAT_MAGICK_TECH_APTITUDE);
                if (aptitude < 0) {
                    resistance = 100 - (100 - resistance) * (aptitude + 100) / 100;
                }
            }
            if (resistance > 0
                && (info->flags & 0x80) == 0
                && info->maintenance.period <= 0) {
                if (info->resistance.stat != -1) {
                    v2 = resistance / 10;
                } else {
                    if (resistance > 40) {
                        return 100;
                    }
                }
            }
        }

        if (attacker_obj != target_obj) {
            if (info->resistance.stat != -1
                && obj_type_is_critter(obj_type)) {
                if (info->resistance.stat == STAT_WILLPOWER
                    && stat_is_extraordinary(target_obj, STAT_WILLPOWER)) {
                    return 100;
                }

                int v3 = info->resistance.value + stat_level_get(target_obj, info->resistance.stat) - v2;
                if (v3 > 0 && random_between(1, 20) <= v3) {
                    if ((info->flags & 0x80) == 0) {
                        if (info->maintenance.period == 0) {
                            resistance = 100;
                        }
                    } else {
                        if (resistance < 50) {
                            resistance = 50;
                        }
                    }
                }
            }
        }
    }

    return resistance;
}

// 0x453CC0
int sub_453CC0(int64_t a1, int64_t item_obj, int64_t a3)
{
    int spell_mana_store;
    int item_flags;
    int spell;

    if (a3 == OBJ_HANDLE_NULL
        || item_obj == OBJ_HANDLE_NULL) {
        return 0;
    }

    spell_mana_store = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_MANA_STORE);
    item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);

    if (spell_mana_store == 0
        || (item_flags & OIF_IS_MAGICAL) == 0) {
        return 0;
    }

    spell = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_1);
    if (spell == -1) {
        return 0;
    }

    return sub_453B20(a1, a3, spell);
}

// 0x453D40
void sub_453D40()
{
    if (dword_5E761C->apply_aoe.flags == 0) {
        return;
    }

    if ((dword_5E761C->apply_aoe.flags & Tgt_Self) != 0) {
        stru_5E6D28.field_20 = dword_5E75F0->parent_obj.obj;
        if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
            dword_5E75AC = obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_TYPE);
        }
    }

    if ((dword_5E761C->apply_aoe.flags & Tgt_Source) != 0) {
        stru_5E6D28.field_20 = dword_5E75F0->source_obj.obj;
        if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
            dword_5E75AC = obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_TYPE);
        }
    }

    if ((dword_5E761C->apply_aoe.flags & 0x400000000000) != 0) {
        stru_5E6D28.field_20 = stru_5E6D28.field_48;
        if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
            dword_5E75AC = obj_field_int32_get(stru_5E6D28.field_20, OBJ_F_TYPE);
        }
    }

    if ((dword_5E761C->apply_aoe.flags & 0x800000000000) != 0) {
        if ((dword_5E761C->apply_aoe.flags & 0x80000000000000) != 0) {
            if (stru_5E6D28.field_20 != OBJ_HANDLE_NULL) {
                stru_5E6D28.field_28 = obj_field_int64_get(stru_5E6D28.field_20, OBJ_F_LOCATION);
            }
        } else {
            if (dword_5E75F0->target_obj.loc != 0) {
                stru_5E6D28.field_28 = dword_5E75F0->target_obj.loc;
            } else {
                stru_5E6D28.field_28 = obj_field_int64_get(dword_5E75F0->target_obj.obj, OBJ_F_LOCATION);
            }
        }
    }
}

// 0x453EE0
void sub_453EE0()
{
    if ((dword_5E7598->flags & MAGICTECH_AGGRESSIVE) != 0
        && stru_5E6D28.field_20 != OBJ_HANDLE_NULL
        && dword_5E75F0->action != MAGICTECH_ACTION_END) {
        sub_453F20(dword_5E75F0->parent_obj.obj, stru_5E6D28.field_20);
    }
}

// 0x453F20
void sub_453F20(int64_t a1, int64_t a2)
{
    if (qword_5E75E0 != a2) {
        qword_5E75E0 = a2;
        ai_attack(a1, a2, 1, 0);

        if (dword_5E75F0->field_E8.obj != OBJ_HANDLE_NULL
            && a1 != dword_5E75F0->field_E8.obj) {
            ai_attack(dword_5E75F0->field_E8.obj, a2, LOUDNESS_NORMAL, 0);
        }
    }
}

// TODO: Lots of jumps, rewrite without goto.
//
// 0x453FA0
void sub_453FA0()
{
    bool v0;
    bool v1;
    MagicTechMaintenanceInfo* maintenance;
    MagicTechDurationInfo* duration;
    MesFileEntry mes_file_entry;
    DateTime datetime;
    TimeEvent timeevent;

    v0 = false;
    v1 = true;

    if (dword_5E75D4) {
        if (!dword_5E7630) {
            dword_5E7630 = true;
            if (dword_5E75D8) {
                sub_457030(dword_5E75F0->id, MAGICTECH_ACTION_END_CALLBACK);
            } else {
                sub_457030(dword_5E75F0->id, MAGICTECH_ACTION_CALLBACK);
            }
            dword_5E7630 = false;
            return;
        }
    } else {
        if (dword_5E75F0->action == MAGICTECH_ACTION_END) {
            goto LABEL_25;
        }

        if (dword_5E75F0->action >= MAGICTECH_ACTION_END && dword_5E75F0->action != MAGICTECH_ACTION_CALLBACK) {
        LABEL_69:
            if (dword_5E7598->components[MAGICTECH_ACTION_CALLBACK].cnt == 0
                && dword_5E7598->components[MAGICTECH_ACTION_END_CALLBACK].cnt == 0) {
                v0 = true;
            }

            if (dword_5E75F0->action == MAGICTECH_ACTION_CALLBACK
                && dword_5E7598->components[MAGICTECH_ACTION_END].cnt == 0) {
                v0 = true;
            }

            if (dword_5E75F0->action == MAGICTECH_ACTION_END_CALLBACK) {
                dword_5E75F0->action = MAGICTECH_ACTION_END;
                v0 = false;
            }

            if ((dword_5E75F0->action != MAGICTECH_ACTION_BEGIN
                    || dword_5E7598->components[MAGICTECH_ACTION_CALLBACK].cnt == 0
                    || !dword_5E75CC)
                && v0) {
                dword_5E75F0->action = MAGICTECH_ACTION_END;
                sub_451070(dword_5E75F0);
                return;
            }

            if (!v1) {
                return;
            }

            goto LABEL_25;
        }

        maintenance = magictech_get_maintenance(dword_5E75F0->spell);
        if ((dword_5E75F0->field_13C & 0x10) != 0) {
            v0 = true;
            goto LABEL_69;
        }

        if (maintenance->period > 0) {
            if (dword_5E75DC) {
                if (dword_5E75F0->source_obj.obj != OBJ_HANDLE_NULL) {
                    if (!sub_4545E0(dword_5E75F0)) {
                        if (player_is_pc_obj(dword_5E75F0->parent_obj.obj)) {
                            mes_file_entry.num = 601;
                            mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
                            sub_460610(mes_file_entry.str);
                        }
                        dword_5E75F0->action = MAGICTECH_ACTION_END;
                        sub_451070(dword_5E75F0);
                        return;
                    }

                    sub_454790(&timeevent, dword_5E75F0->id, maintenance->period, &datetime);

                    if (sub_4547F0(&timeevent, &datetime)) {
                    LABEL_25:
                        if (dword_5E75F0->action != MAGICTECH_ACTION_END) {
                            return;
                        }

                        if (dword_5E75CC
                            && dword_5E7598->components[MAGICTECH_ACTION_END_CALLBACK].cnt == 0) {
                            dword_5E75CC = false;
                        }

                        if (dword_5E75F0->action != MAGICTECH_ACTION_END
                            || dword_5E75CC
                            || dword_5E75F0->id == -1) {
                            return;
                        }

                        goto END;
                    }

                    goto LABEL_69;
                }

                if (dword_5E75F0->field_150 == 0) {
                    sub_454790(&timeevent, dword_5E75F0->id, maintenance->period, &datetime);
                    v1 = false;
                    if (dword_5E7598->duration_trigger_count != 0) {
                        dword_5E75F0->field_150 = dword_5E7598->duration_trigger_count;
                    } else {
                        dword_5E75F0->field_150 = 8;
                    }

                LABEL_22:
                    if (sub_4547F0(&timeevent, &datetime)) {
                        if (!v1) {
                            return;
                        }

                        goto LABEL_25;
                    }

                    goto LABEL_69;
                }

                dword_5E75F0->field_150--;
                if (dword_5E75F0->field_150 > 0) {
                    sub_454790(&timeevent, dword_5E75F0->id, maintenance->period, &datetime);
                    goto LABEL_22;
                }
            }
            v0 = true;
        } else {
            duration = magictech_get_duration(dword_5E75F0->spell);
            if (dword_5E75DC && (duration->period != 0 || duration->stat != -1)) {
                timeevent.type = TIMEEVENT_TYPE_MAGICTECH;
                timeevent.params[0].integer_value = dword_5E75F0->id;
                timeevent.params[2].integer_value = 1;

                if (dword_5E75F0->field_150 > 0) {
                    dword_5E75F0->field_150--;
                } else {
                    dword_5E75F0->action = MAGICTECH_ACTION_END;
                    dword_5E75F0->field_13C |= 0x10;
                }

                sub_45A950(&datetime, duration->period);

                if (duration->stat > -1) {
                    if (dword_5E75F0->parent_obj.type != -1) {
                        if (obj_type_is_critter(dword_5E75F0->parent_obj.type)) {
                            int level = duration->level - stat_level_get(dword_5E75F0->parent_obj.obj, duration->stat);
                            if (level < 0) {
                                level = 0;
                            }
                            datetime.milliseconds += level * duration->modifier;
                            v0 = false;
                        }
                    } else {
                        if (dword_5E75F0->target_obj.obj != OBJ_HANDLE_NULL
                            && obj_type_is_critter(dword_5E75F0->target_obj.type)) {
                            int level = duration->level - stat_level_get(dword_5E75F0->target_obj.obj, duration->stat);
                            if (level < 0) {
                                level = 0;
                            }
                            datetime.milliseconds += level * duration->modifier;
                        }

                        v0 = false;
                    }
                }

                if (datetime.milliseconds == 0) {
                    datetime.milliseconds = 1;
                }

                datetime.milliseconds *= 1000;

                sub_455250(dword_5E75F0, &datetime);

                datetime.milliseconds *= 8;
                if (sub_4548D0(&timeevent, &datetime, &dword_5E75F0->field_148)) {
                    return;
                }

                v1 = true;
            } else {
                v0 = true;
            }
        }

        goto LABEL_69;
    }

END:

    if (player_is_pc_obj(dword_5E75F0->parent_obj.obj)) {
        ui_spell_maintain_end(dword_5E75F0->id);
    }

    if (tig_net_is_active() && tig_net_is_host()) {
        PacketPlayerSpellMaintainEnd pkt;

        pkt.type = 61;
        pkt.mt_id = dword_5E75F0->id;
        pkt.player = multiplayer_find_slot_from_obj(dword_5E75F0->parent_obj.obj);

        if (pkt.player != -1) {
            tig_net_send_app_all(&pkt, sizeof(pkt));
        } else {
            tig_debug_println("MP: MagicTech: could not find player for object in SpellMaintainEnd but am handling...");
        }
    }

    magictech_id_free_lock(dword_5E75F0->id);

    if (player_is_pc_obj(dword_5E75F0->target_obj.obj)) {
        sub_4601C0();
    }
}

// 0x4545E0
bool sub_4545E0(MagicTechRunInfo* run_info)
{
    int idx;
    int cnt = 0;

    if (run_info->source_obj.obj == OBJ_HANDLE_NULL) {
        return true;
    }

    // FIXME: Meaningless.
    if (run_info->parent_obj.obj != OBJ_HANDLE_NULL) {
        sub_454700(obj_field_int64_get(run_info->parent_obj.obj, OBJ_F_LOCATION),
            run_info->target_obj.loc,
            run_info->target_obj.obj,
            run_info->spell);
    }

    if (!obj_type_is_critter(obj_field_int32_get(run_info->parent_obj.obj, OBJ_F_TYPE))) {
        return false;
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x01) != 0
            && (magictech_spells[magictech_run_info[idx].spell].flags & MAGICTECH_IS_TECH) == 0
            && magictech_run_info[idx].parent_obj.obj == run_info->parent_obj.obj
            && (magictech_run_info[idx].field_13C & 0x04) != 0
            && magictech_spells[magictech_run_info[idx].spell].maintenance.period > 0) {
            cnt++;
        }
    }

    if (sub_450B90(run_info->parent_obj.obj) < cnt) {
        return false;
    }

    return true;
}

// 0x454700
bool sub_454700(int64_t source_loc, int64_t target_loc, int64_t target_obj, int spell)
{
    if (source_loc == 0) {
        return true;
    }

    if (target_obj != OBJ_HANDLE_NULL) {
        if ((obj_field_int32_get(target_obj, OBJ_F_FLAGS) & OF_INVENTORY) != 0) {
            return true;
        }

        target_loc = obj_field_int64_get(target_obj, OBJ_F_LOCATION);
    }

    if (target_loc == 0) {
        return false;
    }

    return location_dist(source_loc, target_loc) <= magictech_get_range(spell);
}

// 0x454790
void sub_454790(TimeEvent* timeevent, int a2, int a3, DateTime* datetime)
{
    timeevent->type = TIMEEVENT_TYPE_MAGICTECH;
    timeevent->params[0].integer_value = a2;
    timeevent->params[2].integer_value = 1;
    dword_5E75F0->action = MAGICTECH_ACTION_MAINTAIN;
    sub_45A950(datetime, 1000 * a3);
    sub_455250(dword_5E75F0, datetime);
    datetime->milliseconds *= 8;
}

// 0x4547F0
bool sub_4547F0(TimeEvent* timeevent, DateTime* datetime)
{
    int player;

    if (!sub_4548D0(timeevent, datetime, &(dword_5E75F0->field_148))) {
        return false;
    }

    if (player_is_local_pc_obj(dword_5E75F0->parent_obj.obj)) {
        if ((magictech_spells[dword_5E75F0->spell].flags & MAGICTECH_IS_TECH) == 0
            && !ui_spell_maintain_add(dword_5E75F0->id)) {
            dword_5E75F0->action = MAGICTECH_ACTION_END;
        }
    } else {
        player = multiplayer_find_slot_from_obj(dword_5E75F0->parent_obj.obj);
        if (player != -1) {
            PacketPlayerSpellMaintainAdd pkt;

            pkt.type = 60;
            pkt.mt_id = dword_5E75F0->id;
            pkt.player = player;

            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }

    return true;
}

// 0x4548D0
bool sub_4548D0(TimeEvent* timeevent, DateTime* a2, DateTime* a3)
{
    bool v1;

    dword_5B0BA0 = timeevent->params[0].integer_value;
    v1 = timeevent_any(TIMEEVENT_TYPE_MAGICTECH, sub_4570E0);
    dword_5B0BA0 = -1;

    if (v1) {
        return true;
    }

    return timeevent_add_delay_at(timeevent, a2, a3);
}

// 0x454920
bool sub_454920(int64_t obj, int num, int max)
{
    int spell_mana_store;

    if (!multiplayer_is_locked()) {
        if (!tig_net_is_host()) {
            return false;
        }

        Packet74 pkt;
        pkt.type = 74;
        pkt.subtype = 2;
        if (obj != OBJ_HANDLE_NULL) {
            pkt.oid = obj_get_id(obj);
        } else {
            pkt.oid.type = OID_TYPE_NULL;
        }
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (!obj_type_is_item(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return false;
    }

    spell_mana_store = obj_field_int32_get(obj, OBJ_F_ITEM_SPELL_MANA_STORE);
    if (spell_mana_store < max) {
        spell_mana_store += num;
        if (spell_mana_store > max) {
            spell_mana_store = max;
        }
        obj_field_int32_set(obj, OBJ_F_ITEM_SPELL_MANA_STORE, spell_mana_store);
        sub_4605D0();
    }

    return true;
}

// 0x454A10
void magictech_component_obj_flag(int64_t obj, int64_t a2, int fld, int a4, int a5, int64_t a6, int64_t a7)
{
    int obj_type;
    unsigned int flags;
    bool v17 = false;
    bool v16 = true;
    CombatContext combat;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    flags = obj_field_int32_get(obj, fld);

    if (a5 == 1) {
        if (sub_455100(obj, fld, a4, true) > 0) {
            return;
        }

        flags |= a4;

        switch (fld) {
        case OBJ_F_SPELL_FLAGS:
            if ((a4 & (OSF_DETECTING_INVISIBLE | OSF_DETECTING_TRAPS | OSF_DETECTING_ALIGNMENT | OSF_DETECTING_MAGIC)) != 0) {
                if (player_is_local_pc_obj(obj)) {
                    dword_5E6D24(NULL);
                }
            } else if ((a4 & OSF_INVISIBLE) != 0) {
                if (!player_is_local_pc_obj(obj)) {
                    object_flags_set(obj, OF_INVISIBLE);
                }
                object_add_flags(obj, OF_TRANSLUCENT);
            } else if ((a4 & OSF_SHRUNK) != 0) {
                object_add_flags(obj, OF_SHRUNK);
            } else if ((a4 & OSF_WATER_WALKING) != 0) {
                object_flags_set(obj, OF_WATER_WALKING);
            } else if ((a4 & OSF_STONED) != 0) {
                object_flags_set(obj, OF_STONED);
            } else if ((a4 & OSF_ENTANGLED) != 0) {
                sub_44E4D0(obj, AG_MOVE_TO_TILE, -1);
                sub_44E4D0(obj, AG_ATTEMPT_MOVE_NEAR, -1);
            } else if ((a4 & OSF_MIND_CONTROLLED) != 0) {
                if (a6 != OBJ_HANDLE_NULL) {
                    if (obj_type == OBJ_TYPE_NPC) {
                        if (player_is_pc_obj(a6)
                            && (obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & OSF_SUMMONED) == 0) {
                            reaction_adj(obj, a6, -50);
                        }

                        obj_field_int32_set(obj, OBJ_F_SPELL_FLAGS, flags);

                        if (!critter_follow(obj, a6, true)) {
                            tig_debug_printf("magictech_component_obj_flag: Error: critter_follow failed!\n");
                        }

                        ai_set_no_flee(obj);
                    }
                    return;
                }
            } else if ((a4 & OSF_CHARMED) != 0) {
                if (obj_type == OBJ_TYPE_NPC) {
                    if ((obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & OCF_ANIMAL) != 0) {
                        sub_4AA300(obj, a6);
                    } else if (player_is_pc_obj(a6)) {
                        reaction_adj(obj, a6, 30);
                    }

                    obj_field_int32_set(obj,
                        OBJ_F_NPC_FLAGS,
                        obj_field_int32_get(obj, OBJ_F_NPC_FLAGS) | ONF_KOS_OVERRIDE);
                }
            }
            break;
        case OBJ_F_CRITTER_FLAGS:
            if ((a4 & OCF_UNDEAD) != 0) {
                object_flags_set(obj, OF_ANIMATED_DEAD);
            } else if ((a4 & OCF_FLEEING) != 0) {
                ai_flee(obj, a2);
                return;
            } else if ((a4 & OCF_BLINDED) != 0) {
                combat_set_blinded(obj);
                return;
            } else if ((a4 & OCF_PARALYZED) != 0) {
                sub_44E4D0(obj, AG_MOVE_TO_TILE, -1);
                sub_44E4D0(obj, AG_ATTEMPT_MOVE_NEAR, -1);
            }
            break;
        case OBJ_F_CRITTER_FLAGS2:
            break;
        case OBJ_F_FLAGS:
            if ((a4 & OF_DONTDRAW) != 0) {
                object_flags_set(obj, OF_DONTDRAW);

                if (obj_type_is_critter(obj_type)) {
                    obj_field_int32_set(obj,
                        OBJ_F_CRITTER_FLAGS,
                        obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) | OCF_PARALYZED);
                }

                return;
            }
            break;
        }
    } else if (a5 == 0) {
        if (sub_455100(obj, fld, a4, false) > 1) {
            return;
        }

        flags &= ~a4;

        switch (fld) {
        case OBJ_F_SPELL_FLAGS:
            if ((a4 & (OSF_DETECTING_INVISIBLE | OSF_DETECTING_TRAPS | OSF_DETECTING_ALIGNMENT | OSF_DETECTING_MAGIC)) != 0) {
                if (player_is_local_pc_obj(obj)) {
                    dword_5E6D24(NULL);
                }
            } else if ((a4 & OSF_INVISIBLE) != 0) {
                if (!player_is_local_pc_obj(obj)) {
                    object_flags_unset(obj, OF_INVISIBLE);
                }
                object_remove_flags(obj, OF_TRANSLUCENT);
            } else if ((a4 & OSF_SHRUNK) != 0) {
                object_remove_flags(obj, OF_SHRUNK);
            } else if ((a4 & OSF_WATER_WALKING) != 0) {
                object_flags_unset(obj, OF_WATER_WALKING);
                if (tile_is_blocking(obj_field_int64_get(obj, OBJ_F_LOCATION), false)) {
                    sub_4B2210(OBJ_HANDLE_NULL, obj, &combat);
                    combat.dam_flags |= CDF_FULL;
                    combat_dmg(&combat);

                    // FIXME: Meaningless.
                    obj_field_int32_get(combat.target_obj, OBJ_F_FLAGS);
                }
            } else if ((a4 & OSF_STONED) != 0) {
                object_flags_unset(obj, OF_STONED);
            } else if ((a4 & OSF_MIND_CONTROLLED) != 0) {
                if (obj_type == OBJ_TYPE_NPC) {
                    obj_field_int32_set(obj, OBJ_F_SPELL_FLAGS, flags);
                    if (!critter_disband(obj, true)) {
                        tig_debug_printf("magictech_component_obj_flag: Error: critter_disband failed!\n");
                    }
                }
            } else if ((a4 & OSF_CHARMED) != 0) {
                if (obj_type == OBJ_TYPE_NPC) {
                    reaction_adj(obj, a6, -30);
                }
            }
            break;
        case OBJ_F_CRITTER_FLAGS:
            if ((a4 & OCF_UNDEAD) != 0) {
                object_flags_unset(obj, OF_ANIMATED_DEAD);
            } else if ((a4 & OCF_FLEEING) != 0) {
                ai_stop_fleeing(obj);
                return;
            }
            break;
        case OBJ_F_CRITTER_FLAGS2:
            break;
        case OBJ_F_FLAGS:
            if ((a4 & OF_DONTDRAW) != 0) {
                object_flags_unset(obj, OF_DONTDRAW);

                if (obj_type_is_critter(obj_type)) {
                    obj_field_int32_set(obj,
                        OBJ_F_CRITTER_FLAGS,
                        obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS) & ~OCF_PARALYZED);
                }

                if ((obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2) & OCF2_AUTO_ANIMATES) != 0) {
                    anim_goal_unconceal(obj);
                }

                return;
            }
            break;
        case OBJ_F_PORTAL_FLAGS:
            if ((a4 & OPF_LOCKED) != 0) {
                if (!sub_4551C0(a6, a7, obj)) {
                    v16 = false;
                    sub_45A520(a6, obj);
                }

                ai_notify_portal_container_guards(a6, obj, false, LOUDNESS_LOUD);
                v17 = true;

                if (!v16) {
                    sub_460C30(obj);
                    return;
                }
            }
            break;
        case OBJ_F_CONTAINER_FLAGS:
            if ((a4 & OCOF_LOCKED) != 0) {
                if (!sub_4551C0(a6, a7, obj)) {
                    v16 = false;
                    sub_45A520(a6, obj);
                }

                ai_notify_portal_container_guards(a6, obj, false, LOUDNESS_LOUD);
                v17 = true;

                if (!v16) {
                    sub_460C30(obj);
                    return;
                }
            }
            break;
        }
    }

    obj_field_int32_set(obj, fld, flags);

    if (v17) {
        sub_460C30(obj);
    }
}

// 0x455100
int sub_455100(int64_t obj, int fld, unsigned int a3, bool a4)
{
    unsigned int flags;
    int mt_id;
    MagicTechRunInfo* run_info;
    int cnt = 0;

    flags = obj_field_int32_get(obj, fld);

    if (a4) {
        if ((flags & a3) == 0) {
            return 0;
        }
    }

    if (magictech_find_first(obj, &mt_id)) {
        do {
            if (magictech_id_to_run_info(mt_id, &run_info)
                && fld == OBJ_F_SPELL_FLAGS
                && (magictech_spells[run_info->spell].field_114 & a3) != 0) {
                cnt++;
            }
        } while (magictech_find_next(obj, &mt_id));
    }

    return cnt;
}

// 0x4551C0
bool sub_4551C0(int64_t a1, int64_t a2, int64_t a3)
{
    int lock_difficulty;

    if (a1 != OBJ_HANDLE_NULL
        && obj_type_is_critter(obj_field_int32_get(a1, OBJ_F_TYPE))
        && !dword_5E75A8) {
        switch (obj_field_int32_get(a3, OBJ_F_TYPE)) {
        case OBJ_TYPE_PORTAL:
            lock_difficulty = obj_field_int32_get(a3, OBJ_F_PORTAL_LOCK_DIFFICULTY);
            break;
        case OBJ_TYPE_CONTAINER:
            lock_difficulty = obj_field_int32_get(a3, OBJ_F_CONTAINER_LOCK_DIFFICULTY);
            break;
        default:
            return false;
        }

        if (lock_difficulty >= 0 && dword_5E75F0->parent_obj.aptitude < lock_difficulty) {
            return false;
        }
    }

    return true;
}

// 0x455250
void sub_455250(MagicTechRunInfo* run_info, DateTime* datetime)
{
    unsigned int millis;
    int source_aptitude;
    int target_aptitude;
    int delta;

    if (run_info->parent_obj.obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (!obj_type_is_critter(run_info->parent_obj.type)) {
        return;
    }

    millis = datetime->milliseconds;

    if (run_info->spell >= 0
        && run_info->spell < MT_80
        && spell_mastery_get(run_info->parent_obj.obj) == COLLEGE_FROM_SPELL(run_info->spell)) {
        millis *= 2;
    }

    if (run_info->target_obj.obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(run_info->target_obj.type)) {
        source_aptitude = stat_level_get(run_info->parent_obj.obj, STAT_MAGICK_TECH_APTITUDE);
        target_aptitude = stat_level_get(run_info->target_obj.obj, STAT_MAGICK_TECH_APTITUDE);

        if (millis != 0) {
            if (source_aptitude > target_aptitude) {
                delta = source_aptitude - target_aptitude;
            } else {
                delta = target_aptitude - source_aptitude;
            }

            if (delta < 0) {
                millis = 100 * millis / (delta + 100);
            }
        }
    }

    datetime->milliseconds = millis;
}

// 0x455350
void sub_455350(int64_t obj, int64_t target_loc)
{
    int64_t source_loc;
    AnimPath path;
    PathCreateInfo path_create_info;

    source_loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    path.field_CC = sizeof(path.rotations) / sizeof(path.rotations[0]);
    sub_44EBD0(&path);

    path_create_info.to = target_loc;
    path_create_info.from = source_loc;
    path_create_info.obj = obj;
    path_create_info.max_rotations = sizeof(path.rotations) / sizeof(path.rotations[0]);
    path_create_info.rotations = path.rotations;
    path_create_info.flags = PATH_FLAG_0x0010;
    path.max = sub_41F3C0(&path_create_info);

    if (path.max != 0) {
        for (path.curr = 0; path.curr < path.max; path.curr++) {
            if (!location_in_dir(source_loc, path.rotations[path.curr], &source_loc)) {
                break;
            }

            tile_script_exec(source_loc, obj);
        }

        sub_424070(obj, 5, false, true);
        object_move_to_location(obj, source_loc, 0, 0);

        if (player_is_local_pc_obj(obj)) {
            location_origin_set(source_loc);
        }
    }

    sub_44EBE0(&path);
}

// 0x4554B0
void sub_4554B0(MagicTechRunInfo* run_info, int64_t obj)
{
    MagicTechObjectNode* node;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    node = run_info->summoned_obj;

    run_info->summoned_obj = mt_obj_node_create();
    run_info->summoned_obj->next = node;
    run_info->summoned_obj->obj = obj;
    run_info->summoned_obj->type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(run_info->summoned_obj->type)) {
        run_info->summoned_obj->aptitude = stat_level_get(obj, STAT_MAGICK_TECH_APTITUDE);
    }

    sub_443EB0(obj, &(run_info->summoned_obj->field_8));
}

// 0x455550
bool sub_455550(S603CB8* a1, MagicTechRunInfo* run_info)
{
    MesFileEntry mes_file_entry;
    int64_t parent_obj;

    a1->field_60 = 0;
    if (a1->field_20 == OBJ_HANDLE_NULL
        || a1->field_20 == dword_5E75F0->parent_obj.obj) {
        return true;
    }

    a1->field_60 = obj_field_int32_get(a1->field_20, OBJ_F_SPELL_FLAGS);

    if ((a1->field_60 & OSF_ANTI_MAGIC_SHELL) != 0
        && (run_info->field_138 & 0x800) == 0) {
        if (player_is_local_pc_obj(run_info->parent_obj.obj)) {
            mes_file_entry.num = 603; // "The effect is nullified."
            mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
            sub_460610(mes_file_entry.str);
            ui_spell_maintain_end(run_info->id);
        }
        return false;
    }

    if (((a1->field_60 & OSF_FULL_REFLECTION) == 0
            && (run_info->field_13C & 0x08) == 0)
        || (dword_5E7598->flags & MAGICTECH_NO_REFLECT) != 0) {
        return true;
    }

    run_info->field_13C |= 0x08;

    if ((run_info->field_138 & 0x2000) == 0
        && sub_459040(a1->field_20, OSF_FULL_REFLECTION, &parent_obj)) {
        sub_450420(parent_obj, dword_5E762C, true, 10000);
    }

    if (a1->field_20 == run_info->source_obj.obj) {
        return true;
    }

    if ((run_info->field_138 & 0x2000) != 0) {
        return true;
    }

    if ((a1->field_5C & 0x2000) == 0) {
        int64_t source_obj = run_info->source_obj.obj;
        run_info->source_obj.obj = a1->field_20;
        a1->field_20 = source_obj;
        return true;
    }

    if (player_is_local_pc_obj(run_info->parent_obj.obj)) {
        mes_file_entry.num = 602; // "You lose your concentration."
        mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
        sub_460610(mes_file_entry.str);
        ui_spell_maintain_end(run_info->id);
    }

    return false;
}

// 0x455710
void sub_455710()
{
    int index;
    MagicTechRunInfo* run_info;

    for (index = 0; index < 512; index++) {
        run_info = &(magictech_run_info[index]);
        run_info->source_obj.obj = OBJ_HANDLE_NULL;
        run_info->id = -1;
        run_info->field_13C = 0;
    }
}

// 0x455740
void magictech_id_new_lock(MagicTechRunInfo** run_info_ptr)
{
    int index;

    for (index = 0; index < 512; index++) {
        if (magictech_run_info[index].id == -1) {
            magictech_run_info[index].id = index;
            magictech_run_info[index].field_13C = 0x1;
            magictech_run_info[index].action = MAGICTECH_ACTION_BEGIN;
            *run_info_ptr = &(magictech_run_info[index]);
            dword_6876DC++;
            return;
        }
    }

    tig_debug_printf("magictech_id_new_lock: Error: ran out of id's!\n");
    exit(EXIT_FAILURE);
}

// 0x4557C0
bool magictech_id_to_run_info(int mt_id, MagicTechRunInfo** run_info_ptr)
{
    if (mt_id != -1
        && magictech_run_info[mt_id].id != -1
        && sub_455820(&(magictech_run_info[mt_id]))) {
        *run_info_ptr = &(magictech_run_info[mt_id]);
        return true;
    }

    *run_info_ptr = NULL;
    return false;
}

// 0x455820
bool sub_455820(MagicTechRunInfo* run_info)
{
    bool success = true;
    MagicTechObjectNode* node;

    if (!sub_444020(&(run_info->source_obj.obj), &(run_info->source_obj.field_8))) {
        success = false;
    }

    if (!sub_444020(&(run_info->parent_obj.obj), &(run_info->parent_obj.field_8))) {
        success = false;
    }

    node = run_info->objlist;
    while (node != NULL) {
        if (!sub_444020(&(node->obj), &(node->field_8))) {
            success = false;
        }
        node = node->next;
    }

    node = run_info->summoned_obj;
    while (node != NULL) {
        if (!sub_444020(&(node->obj), &(node->field_8))) {
            success = false;
        }
        node = node->next;
    }

    return success;
}

// 0x4558D0
void magictech_id_free_lock(int mt_id)
{
    MagicTechRunInfo* run_info;
    Packet54 pkt;

    run_info = &(magictech_run_info[mt_id]);
    if (run_info->id != -1) {
        dword_5B0BA0 = run_info->id;
        timeevent_clear_one_ex(TIMEEVENT_TYPE_MAGICTECH, sub_4570E0);
        dword_5B0BA0 = -1;

        sub_455960(run_info);
        dword_6876DC--;

        if (tig_net_is_active()
            && tig_net_is_host()) {
            pkt.type = 54;
            pkt.magictech_id = mt_id;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    }
}

// 0x455960
void sub_455960(MagicTechRunInfo* run_info)
{
    MagicTechObjectNode* node;
    MagicTechObjectNode* next;

    if (run_info->id != -1) {
        run_info->id = -1;
        run_info->source_obj.obj = OBJ_HANDLE_NULL;
        run_info->parent_obj.obj = OBJ_HANDLE_NULL;

        node = run_info->objlist;
        while (node != NULL) {
            next = node->next;
            mt_obj_node_destroy(node);
            node = next;
        }
        run_info->objlist = NULL;

        node = run_info->summoned_obj;
        while (node != NULL) {
            next = node->next;
            mt_obj_node_destroy(node);
            node = next;
        }
        run_info->summoned_obj = NULL;

        run_info->field_13C = 0;
    }
}

// 0x4559E0
void sub_4559E0(MagicTechRunInfo* run_info)
{
    if (run_info->id != -1) {
        run_info->id = -1;
        run_info->source_obj.obj = OBJ_HANDLE_NULL;
        run_info->parent_obj.obj = OBJ_HANDLE_NULL;
        run_info->objlist = NULL;
        run_info->summoned_obj = NULL;
        run_info->field_13C = 0;
    }
}

// 0x455A20
void magictech_invocation_init(MagicTechInvocation* mt_invocation, int64_t obj, int spell)
{
    mt_invocation->spell = spell;
    sub_4440E0(obj, &(mt_invocation->source_obj));
    if (obj != OBJ_HANDLE_NULL) {
        mt_invocation->loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
    } else {
        mt_invocation->loc = 0;
    }
    sub_4440E0(OBJ_HANDLE_NULL, &(mt_invocation->target_obj));
    mt_invocation->target_loc = 0;
    sub_4440E0(sub_450A50(obj), &(mt_invocation->parent_obj));
    sub_4440E0(OBJ_HANDLE_NULL, &(mt_invocation->field_A0));
    mt_invocation->trigger = 0;
    mt_invocation->flags = 0;
}

// 0x455AC0
void magictech_invocation_run(MagicTechInvocation* mt_invocation)
{
    if (magictech_editor) {
        return;
    }

    if (tig_net_is_active()) {
        if (tig_net_is_host()) {
            PacketPlayerCastSpell pkt;

            pkt.type = 58;
            pkt.player = multiplayer_find_slot_from_obj(mt_invocation->source_obj.obj);
            sub_4440E0(mt_invocation->source_obj.obj, &(mt_invocation->source_obj));
            sub_4440E0(mt_invocation->parent_obj.obj, &(mt_invocation->parent_obj));
            sub_4440E0(mt_invocation->target_obj.obj, &(mt_invocation->target_obj));
            sub_4440E0(mt_invocation->field_A0.obj, &(mt_invocation->field_A0));
            pkt.invocation = *mt_invocation;
            tig_net_send_app_all(&pkt, sizeof(pkt));
            sub_455C30(mt_invocation);
        } else {
            PacketPlayerRequestCastSpell pkt;

            pkt.type = 57;
            pkt.player = multiplayer_find_slot_from_obj(mt_invocation->source_obj.obj);
            sub_4440E0(mt_invocation->source_obj.obj, &(mt_invocation->source_obj));
            sub_4440E0(mt_invocation->parent_obj.obj, &(mt_invocation->parent_obj));
            sub_4440E0(mt_invocation->target_obj.obj, &(mt_invocation->target_obj));
            sub_4440E0(mt_invocation->field_A0.obj, &(mt_invocation->field_A0));
            pkt.invocation = *mt_invocation;
            tig_net_send_app_all(&pkt, sizeof(pkt));
        }
    } else {
        sub_455C30(mt_invocation);
    }
}

// 0x455C30
void sub_455C30(MagicTechInvocation* mt_invocation)
{
    MagicTechInfo* info;
    MagicTechRunInfo* run_info;
    TimeEvent timeevent;
    DateTime datetime;

    if (magictech_editor) {
        return;
    }

    if (!magictech_invocation_check(mt_invocation)) {
        return;
    }

    info = &(magictech_spells[mt_invocation->spell]);
    magictech_id_new_lock(&run_info);

    run_info->source_obj.obj = mt_invocation->source_obj.obj;
    sub_443EB0(run_info->source_obj.obj, &(run_info->source_obj.field_8));
    if (run_info->source_obj.obj != OBJ_HANDLE_NULL) {
        run_info->source_obj.type = obj_field_int32_get(run_info->source_obj.obj, OBJ_F_TYPE);
        if (obj_type_is_critter(run_info->source_obj.type)) {
            run_info->source_obj.aptitude = stat_level_get(run_info->source_obj.obj, STAT_MAGICK_TECH_APTITUDE);
        }
    } else {
        run_info->source_obj.type = -1;
    }
    run_info->source_obj.loc = mt_invocation->loc;

    run_info->parent_obj.obj = mt_invocation->parent_obj.obj;
    sub_443EB0(run_info->parent_obj.obj, &(run_info->parent_obj.field_8));
    if (run_info->parent_obj.obj != OBJ_HANDLE_NULL) {
        run_info->parent_obj.type = obj_field_int32_get(run_info->parent_obj.obj, OBJ_F_TYPE);
        if (obj_type_is_critter(run_info->parent_obj.type)) {
            run_info->parent_obj.aptitude = stat_level_get(run_info->parent_obj.obj, STAT_MAGICK_TECH_APTITUDE);
        }
    } else {
        run_info->parent_obj.type = -1;
    }
    run_info->parent_obj.loc = mt_invocation->loc;

    run_info->target_obj.obj = mt_invocation->target_obj.obj;
    sub_443EB0(run_info->target_obj.obj, &(run_info->target_obj.field_8));
    if (run_info->target_obj.obj != OBJ_HANDLE_NULL) {
        run_info->target_obj.type = obj_field_int32_get(run_info->target_obj.obj, OBJ_F_TYPE);
        if (obj_type_is_critter(run_info->target_obj.type)) {
            run_info->target_obj.aptitude = stat_level_get(run_info->target_obj.obj, STAT_MAGICK_TECH_APTITUDE);
        }
    } else {
        run_info->target_obj.type = -1;
    }
    run_info->target_obj.loc = mt_invocation->target_loc;

    run_info->field_E8.obj = mt_invocation->field_A0.obj;
    sub_443EB0(run_info->field_E8.obj, &(run_info->field_E8.field_8));
    if (run_info->field_E8.obj != OBJ_HANDLE_NULL) {
        run_info->field_E8.type = obj_field_int32_get(run_info->field_E8.obj, OBJ_F_TYPE);
        if (obj_type_is_critter(run_info->field_E8.type)) {
            run_info->field_E8.aptitude = stat_level_get(run_info->field_E8.obj, STAT_MAGICK_TECH_APTITUDE);
        }
    } else {
        run_info->field_E8.type = -1;
    }

    run_info->spell = mt_invocation->spell;
    run_info->action = MAGICTECH_ACTION_BEGIN;
    run_info->objlist = NULL;
    run_info->summoned_obj = NULL;

    if ((mt_invocation->flags & MAGICTECH_INVOCATION_FREE) != 0) {
        run_info->field_13C |= 0x02;
    }

    if ((mt_invocation->flags & MAGICTECH_INVOCATION_UNRESISTABLE) != 0) {
        run_info->field_13C |= 0x10;
    }

    run_info->field_138 = 0;
    run_info->trigger = mt_invocation->trigger;
    run_info->field_144 = 0;
    run_info->field_150 = info->duration_trigger_count;

    if ((info->flags & MAGICTECH_NO_RESIST) != 0) {
        run_info->field_13C |= 0x10;
    }

    if (run_info->source_obj.loc != 0
        && run_info->parent_obj.obj != OBJ_HANDLE_NULL) {
        tig_art_id_t art_id;
        int64_t loc;
        int rot;

        art_id = obj_field_int32_get(run_info->parent_obj.obj, OBJ_F_CURRENT_AID);
        art_id = tig_art_id_frame_set(art_id, 0);

        loc = run_info->target_obj.obj != OBJ_HANDLE_NULL
            ? obj_field_int64_get(run_info->target_obj.obj, OBJ_F_LOCATION)
            : run_info->target_obj.loc;
        rot = location_rot(run_info->source_obj.loc, loc);
        tig_art_id_rotation_set(art_id, rot);
    }

    magictech_preload_art(run_info);

    if (run_info->parent_obj.obj != OBJ_HANDLE_NULL) {
        int anim;
        AnimGoalData goal_data;
        AnimID anim_id;

        anim = info->casting_anim != -1
            ? AG_THROW_SPELL_W_CAST_ANIM
            : AG_THROW_SPELL;

        if ((mt_invocation->flags & MAGICTECH_INVOCATION_FRIENDLY) != 0) {
            if (sub_44D500(&goal_data, run_info->parent_obj.obj, AG_THROW_SPELL_FRIENDLY)) {
                goal_data.params[AGDATA_SPELL_DATA].data = run_info->id;
                goal_data.params[AGDATA_TARGET_OBJ].obj = run_info->target_obj.obj;
                goal_data.params[AGDATA_TARGET_TILE].obj = run_info->target_obj.loc;
                goal_data.params[AGDATA_ANIM_ID].data = TIG_ART_ID_INVALID;

                if (tig_net_is_active() && !tig_net_is_host()) {
                    return;
                }

                if (sub_44D520(&goal_data, &anim_id)) {
                    turn_on_flags(anim_id, 0x200000, 0);
                    return;
                }
            }
        } else if (anim_is_current_goal_type(run_info->parent_obj.obj, AG_THROW_SPELL, &anim_id)) {
            if (num_goal_subslots_in_use(&anim_id) < 4
                && !combat_turn_based_is_active()
                && sub_44D500(&goal_data, run_info->parent_obj.obj, anim)) {
                goal_data.params[AGDATA_SPELL_DATA].data = run_info->id;
                goal_data.params[AGDATA_TARGET_OBJ].obj = run_info->target_obj.obj;
                goal_data.params[AGDATA_TARGET_TILE].obj = run_info->target_obj.loc;
                goal_data.params[AGDATA_ANIM_ID].data = TIG_ART_ID_INVALID;

                if (info->casting_anim != -1) {
                    tig_art_id_t art_id = obj_field_int32_get(run_info->parent_obj.obj, OBJ_F_CURRENT_AID);
                    art_id = tig_art_id_frame_set(art_id, 0);
                    art_id = tig_art_id_anim_set(art_id, info->casting_anim);
                    goal_data.params[AGDATA_ANIM_ID].data = art_id;
                }

                if (tig_net_is_active() && !tig_net_is_host()) {
                    return;
                }

                // __FILE__: "C:\Troika\Code\Game\GameLibX\MagicTech.c"
                // __LINE__: 6893
                if (sub_44DBE0(anim_id, &goal_data, __FILE__, __LINE__)) {
                    return;
                }
            }
        } else if (sub_44D4E0(&goal_data, run_info->parent_obj.obj, anim)) {
            goal_data.params[AGDATA_SPELL_DATA].data = run_info->id;
            goal_data.params[AGDATA_TARGET_OBJ].obj = run_info->target_obj.obj;
            goal_data.params[AGDATA_TARGET_TILE].obj = run_info->target_obj.loc;
            goal_data.params[AGDATA_ANIM_ID].data = TIG_ART_ID_INVALID;

            if (info->casting_anim != -1) {
                tig_art_id_t art_id = obj_field_int32_get(run_info->parent_obj.obj, OBJ_F_CURRENT_AID);
                art_id = tig_art_id_frame_set(art_id, 0);
                art_id = tig_art_id_anim_set(art_id, info->casting_anim);
                goal_data.params[AGDATA_ANIM_ID].data = art_id;
            }

            if (tig_net_is_active() && !tig_net_is_host()) {
                return;
            }

            if (sub_44D520(&goal_data, &anim_id)) {
                if (info->casting_anim != -1) {
                    if (sub_44D500(&goal_data, run_info->parent_obj.obj, AG_THROW_SPELL_W_CAST_ANIM_2NDARY)) {
                        goal_data.params[AGDATA_SPELL_DATA].data = run_info->id;
                        goal_data.params[AGDATA_TARGET_OBJ].obj = run_info->target_obj.obj;
                        goal_data.params[AGDATA_TARGET_TILE].obj = run_info->target_obj.loc;
                        goal_data.params[AGDATA_ANIM_ID].data = TIG_ART_ID_INVALID;
                        sub_44D520(&goal_data, &anim_id);
                    }
                }
                return;
            }
        }
    } else {
        if (sub_458B70(run_info->id) == -1) {
            sub_456E00(run_info->id);
        }

        timeevent.type = TIMEEVENT_TYPE_MAGICTECH;
        timeevent.params[0].integer_value = run_info->id;
        timeevent.params[2].integer_value = 3;
        sub_45A950(&datetime, 2);
        if (timeevent_add_delay(&timeevent, &datetime)) {
            return;
        }
    }

    magictech_id_free_lock(run_info->id);
}

// 0x456430
bool sub_456430(int64_t a1, int64_t a2, MagicTechInfo* magictech)
{
    if (a1 == OBJ_HANDLE_NULL) {
        return true;
    }

    if ((obj_field_int32_get(a1, OBJ_F_SPELL_FLAGS) & magictech->disallowed_sf) != 0) {
        return false;
    }

    if ((obj_field_int32_get(a2, OBJ_F_SPELL_FLAGS) & magictech->disallowed_tsf) != 0) {
        return false;
    }

    if (obj_type_is_critter(obj_field_int32_get(a2, OBJ_F_TYPE))
        && (obj_field_int32_get(a2, OBJ_F_CRITTER_FLAGS) & magictech->disallowed_tcf) != 0) {
        return false;
    }

    return true;
}

// 0x4564E0
bool magictech_invocation_check(MagicTechInvocation* mt_invocation)
{
    MagicTechInfo* info;
    MesFileEntry mes_file_entry;

    info = &(magictech_spells[mt_invocation->spell]);

    if (mt_invocation->parent_obj.obj != OBJ_HANDLE_NULL
        && combat_turn_based_is_active()
        && combat_turn_based_whos_turn_get() != mt_invocation->parent_obj.obj
        && obj_type_is_critter(obj_field_int32_get(mt_invocation->parent_obj.obj, OBJ_F_TYPE))) {
        return false;
    }

    if (mt_invocation->spell < 0 || mt_invocation->spell >= MT_SPELL_COUNT) {
        tig_debug_printf("MagicTech: Activate: ERROR: NOT A SPELL!\n");
        return false;
    }

    if (mt_invocation->parent_obj.obj != OBJ_HANDLE_NULL) {
        int64_t parent_loc = obj_field_int64_get(mt_invocation->parent_obj.obj, OBJ_F_LOCATION);
        if (!sub_454700(parent_loc, mt_invocation->target_loc, mt_invocation->target_obj.obj, mt_invocation->spell)) {
            return false;
        }
    }

    if (!magictech_check_los(mt_invocation)) {
        return false;
    }

    if (!sub_456430(mt_invocation->parent_obj.obj, mt_invocation->target_obj.obj, info)) {
        return false;
    }

    if (mt_invocation->parent_obj.obj != OBJ_HANDLE_NULL
        && (obj_field_int32_get(mt_invocation->parent_obj.obj, OBJ_F_SPELL_FLAGS) & OSF_BONDS_OF_MAGIC) != 0
        && mt_invocation->parent_obj.obj == mt_invocation->source_obj.obj) {
        return false;
    }

    if (!player_is_pc_obj(mt_invocation->parent_obj.obj)) {
        S603D20 v1;
        S603CB8 v2;
        S603CB8_F50 v3;
        int idx;
        uint64_t* tgt;

        sub_4F25E0(&v1);
        sub_459F20(mt_invocation->spell, &tgt);
        v1.aoe_flags = *tgt;
        v1.radius = 2;

        sub_4F2600(&v2, NULL, mt_invocation->source_obj.obj);
        v2.field_38 = mt_invocation->target_loc;
        v2.field_28 = mt_invocation->target_loc;
        v2.field_18 = mt_invocation->loc;
        v2.field_30 = mt_invocation->target_obj.obj;
        v2.field_20 = mt_invocation->target_obj.obj;
        v2.field_48 = 0;
        v2.field_0 = &v1;

        if (!sub_4F2D20(&v2)) {
            if (mt_invocation->target_obj.obj == OBJ_HANDLE_NULL) {
                return false;
            }

            if ((v1.aoe_flags & 0x800000000000) == 0) {
                return false;
            }

            v2.field_38 = obj_field_int64_get(mt_invocation->target_obj.obj, OBJ_F_LOCATION);
            v2.field_28 = obj_field_int64_get(mt_invocation->target_obj.obj, OBJ_F_LOCATION);
            v2.field_50 = &v3;
            sub_4F40B0(&v2);

            for (idx = 0; idx < v2.field_50->cnt; idx++) {
                if (v2.field_50->entries[idx].loc != 0) {
                    v2.field_28 = v2.field_50->entries[idx].loc;
                    if (sub_4F2D20(&v2)) {
                        break;
                    }
                }
            }

            if (idx >= v2.field_50->cnt) {
                return false;
            }

            mt_invocation->target_loc = v2.field_28;
            mt_invocation->target_obj.obj = OBJ_HANDLE_NULL;
            mt_invocation->target_obj.field_8.objid.type = OID_TYPE_NULL;
        }
    }

    if ((mt_invocation->flags & MAGICTECH_INVOCATION_UNRESISTABLE) == 0) {
        int source_aptitude = mt_invocation->parent_obj.obj != OBJ_HANDLE_NULL
            ? stat_level_get(mt_invocation->parent_obj.obj, STAT_MAGICK_TECH_APTITUDE)
            : 0;
        int target_aptitude = mt_invocation->target_obj.obj != OBJ_HANDLE_NULL
            ? stat_level_get(mt_invocation->target_obj.obj, STAT_MAGICK_TECH_APTITUDE)
            : 0;

        if (mt_invocation->source_obj.obj != OBJ_HANDLE_NULL
            && source_aptitude < 0
            && mt_invocation->parent_obj.obj != mt_invocation->source_obj.obj
            && item_effective_power(mt_invocation->source_obj.obj, mt_invocation->parent_obj.obj) <= 0) {
            sub_45A520(mt_invocation->parent_obj.obj, mt_invocation->target_obj.obj);
            return false;
        }

        if (target_aptitude < 0) {
            if (source_aptitude >= 0) {
                target_aptitude = target_aptitude * (100 - source_aptitude) / 100;
            }
            if (random_between(1, 100) <= target_aptitude) {
                sub_45A520(mt_invocation->parent_obj.obj, mt_invocation->target_obj.obj);
                return false;
            }
        }
    }

    if (mt_invocation->parent_obj.obj != OBJ_HANDLE_NULL
        && obj_type_is_critter(obj_field_int32_get(mt_invocation->parent_obj.obj, OBJ_F_TYPE))
        && mt_invocation->parent_obj.obj == mt_invocation->source_obj.obj
        && stat_level_get(mt_invocation->parent_obj.obj, STAT_INTELLIGENCE) < magictech_min_intelligence(mt_invocation->spell)) {
        return false;
    }

    if (mt_invocation->parent_obj.obj != OBJ_HANDLE_NULL
        && mt_invocation->spell == SPELL_TELEPORTATION
        && !antiteleport_check(mt_invocation->parent_obj.obj, 0)) {
        if (player_is_local_pc_obj(mt_invocation->parent_obj.obj)) {
            mes_file_entry.num = 10000; // "This place seems to block your attempt to teleport."
            mes_get_msg(magictech_spell_mes_file, &mes_file_entry);
            sub_460610(mes_file_entry.str);
        }
        return false;
    }

    return true;
}

// 0x456A10
bool sub_456A10(int64_t a1, int64_t a2, int64_t a3)
{
    int aptitude;

    if (a1 == OBJ_HANDLE_NULL) {
        return true;
    }

    if (obj_field_int32_get(a3, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) <= 0) {
        return true;
    }

    aptitude = stat_level_get(a1, STAT_MAGICK_TECH_APTITUDE);

    // FIXME: Useless.
    if (a2 != OBJ_HANDLE_NULL) {
        stat_level_get(a2, STAT_MAGICK_TECH_APTITUDE);
    }

    if (aptitude > 0) {
        return true;
    }

    if (item_effective_power(a3, a1) > 0) {
        return true;
    }

    return false;
}

// 0x456A90
bool sub_456A90(int mt_id)
{
    MagicTechRunInfo* run_info;
    uint64_t* tgt_ptr;
    uint64_t prev_tgt;
    S4F2810 v2;
    S4F2680 v3;
    bool rc;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        return true;
    }

    if (!magictech_id_to_run_info(mt_id, &run_info)) {
        return false;
    }

    sub_459F20(run_info->spell, &tgt_ptr);

    if (*tgt_ptr == Tgt_None
        || ((*tgt_ptr & Tgt_Self) != 0
            && (*tgt_ptr & Tgt_Tile) == 0)
        || *tgt_ptr == Tgt_Obj_Radius) {
        return true;
    }

    prev_tgt = sub_4F25D0();
    sub_4F25B0(*tgt_ptr);
    if (run_info->target_obj.obj != OBJ_HANDLE_NULL) {
        sub_4F2810(&v2, run_info->target_obj.obj);
    } else {
        sub_4F27F0(&v2, run_info->target_obj.loc);
    }

    v3.field_0 = run_info->source_obj.obj;
    v3.field_8 = run_info->parent_obj.obj;
    v3.field_10 = &v2;
    rc = sub_4F2680(&v3);
    sub_4F25B0(prev_tgt);

    return rc;
}

// 0x456BC0
bool magictech_check_los(MagicTechInvocation* mt_invocation)
{
    int64_t loc;
    int64_t blocking_obj;

    if (mt_invocation->parent_obj.obj == mt_invocation->target_obj.obj || mt_invocation->parent_obj.obj == OBJ_HANDLE_NULL) {
        return true;
    }

    loc = mt_invocation->target_loc;
    if (loc == 0) {
        if (mt_invocation->target_obj.obj != OBJ_HANDLE_NULL) {
            if (obj_type_is_item(obj_field_int32_get(mt_invocation->target_obj.obj, OBJ_F_TYPE))
                && item_parent(mt_invocation->target_obj.obj, NULL)) {
                return true;
            }

            loc = obj_field_int64_get(mt_invocation->target_obj.obj, OBJ_F_LOCATION);
        }
    }

    if (loc != 0) {
        if (sub_4ADE00(mt_invocation->parent_obj.obj, loc, &blocking_obj) >= 100) {
            return false;
        }

        if (blocking_obj != OBJ_HANDLE_NULL
            && (obj_field_int32_get(blocking_obj, OBJ_F_FLAGS) & OF_SHOOT_THROUGH) == 0
            && blocking_obj != mt_invocation->target_obj.obj) {
            return false;
        }
    }

    return true;
}

// 0x456CD0
void magictech_preload_art(MagicTechRunInfo* run_info)
{
    int type;
    AnimFxNode node;

    for (type = 0; type < MAGICTECH_EYE_CANDY_TYPE_COUNT; type++) {
        sub_4CCD20(&spell_eye_candies,
            &node,
            run_info->parent_obj.obj,
            run_info->id,
            6 * run_info->spell + type);
        animfx_preload_art(&node);
    }
}

// 0x456D20
bool sub_456D20(int mt_id, tig_art_id_t* art_id_ptr, tig_art_id_t* light_art_id_ptr, tig_color_t* light_color_ptr, int* a5, int* a6, int* a7, int* a8)
{
    MagicTechRunInfo* run_info;
    AnimFxNode node;

    if (!magictech_id_to_run_info(mt_id, &run_info)) {
        return false;
    }

    sub_4CCD20(&spell_eye_candies,
        &node,
        run_info->parent_obj.obj,
        run_info->id,
        6 * run_info->spell);

    if (node.obj != OBJ_HANDLE_NULL) {
        node.rotation = tig_art_id_rotation_get(obj_field_int32_get(node.obj, OBJ_F_CURRENT_AID));
    }

    node.art_id_ptr = art_id_ptr;
    node.light_art_id_ptr = light_art_id_ptr;
    node.light_color_ptr = light_color_ptr;

    if (!animfx_add(&node)) {
        return false;
    }

    *a5 = node.overlay_fore_index;
    *a6 = node.overlay_back_index;
    *a7 = node.overlay_light_index;
    *a8 = animfx_list_find(&spell_eye_candies);

    return true;
}

// 0x456E00
void sub_456E00(int mt_id)
{
    MagicTechRunInfo* run_info;
    AnimFxNode node;

    if (magictech_id_to_run_info(mt_id, &run_info)) {
        sub_4CCD20(&spell_eye_candies,
            &node,
            run_info->parent_obj.obj,
            run_info->id,
            6 * run_info->spell + MAGICTECH_EYE_CANDY_TYPE_SECONDARY_CASTING);
        node.animate = true;
        animfx_add(&node);
    }
}

// 0x456E60
void magictech_fx_add(int64_t obj, int fx)
{
    AnimFxNode node;

    sub_4CCD20(&spell_eye_candies, &node, obj, -1, fx % 10 + 6 * (fx / 10));
    node.animate = true;
    animfx_add(&node);
}

// 0x456EC0
void magictech_fx_remove(int64_t obj, int fx)
{
    if (tig_net_is_active() && !tig_net_is_host()) {
        return;
    }

    animfx_remove(&spell_eye_candies, obj, fx % 10 + 6 * (fx / 10), -1);

    if (tig_net_is_active()) {
        PacketMagicTechEyeCandy pkt;

        pkt.type = 77;
        pkt.subtype = 0;
        pkt.oid = obj_get_id(obj);
        pkt.fx_id = fx % 10 + 6 * (fx / 10);
        pkt.mt_id = -1;
        tig_net_send_app_all(&pkt, sizeof(pkt));
    }
}

// 0x456F70
void sub_456F70(int mt_id)
{
    MagicTechRunInfo* run_info;

    if (magictech_id_to_run_info(mt_id, &run_info)) {
        run_info->action = MAGICTECH_ACTION_BEGIN;
        sub_451070(run_info);
    }
}

// 0x456FA0
void sub_456FA0(int mt_id, unsigned int flags)
{
    if (mt_id != -1
        && (!tig_net_is_active()
            || tig_net_is_host())) {
        if ((flags & 0x1) != 0) {
            sub_4507D0(magictech_run_info[mt_id].source_obj.obj,
                magictech_run_info[mt_id].spell);
        }
        magictech_id_free_lock(mt_id);
    }
}

// 0x457000
void sub_457000(int mt_id, int action)
{
    MagicTechRunInfo* run_info;

    if (magictech_id_to_run_info(mt_id, &run_info)) {
        run_info->action = action;
        sub_451070(run_info);
    }
}

// 0x457030
void sub_457030(int mt_id, int action)
{
    MagicTechRunInfo* run_info;

    if (magictech_id_to_run_info(mt_id, &run_info)) {
        run_info->action = action;
        sub_457060(run_info);
    }
}

// 0x457060
void sub_457060(MagicTechRunInfo* run_info)
{
    if (dword_5B0BA4 != -1 && dword_5B0BA4 != run_info->id) {
        tig_debug_printf("\n\nMagicTech: ERROR: Process function is NOT Re-Entrant, Spell: %d (%s)!\n",
            run_info->spell,
            magictech_get_name(run_info->spell));
        return;
    }

    dword_5E75F0 = run_info;
    dword_5E75AC = -1;
    dword_5E75CC = 0;
    dword_5E75D4 = 1;
    dword_5E75DC = 0;
    qword_5E75E0 = 0;
    dword_5E75E8 = run_info->action;
    sub_4510F0();
}

// 0x4570E0
bool sub_4570E0(TimeEvent* timeevent)
{
    return timeevent->params[0].integer_value == dword_5B0BA0
        && timeevent->params[2].integer_value == 1;
}

// 0x457100
void sub_457100()
{
    sub_455710();
}

// 0x457110
void magictech_interrupt(int mt_id)
{
    MagicTechRunInfo* run_info;
    MagicTechInfo* info;

    if (!magictech_id_to_run_info(mt_id, &run_info)) {
        return;
    }

    info = &(magictech_spells[run_info->spell]);

    if (run_info->action == MAGICTECH_ACTION_MAINTAIN) {
        if (info->components[MAGICTECH_ACTION_END_CALLBACK].cnt > 0) {
            run_info->action = MAGICTECH_ACTION_END;
        } else if (info->components[MAGICTECH_ACTION_CALLBACK].cnt > 0) {
            run_info->action = MAGICTECH_ACTION_CALLBACK;
        } else {
            run_info->action = MAGICTECH_ACTION_END;
        }
    } else {
        if (info->components[MAGICTECH_ACTION_END_CALLBACK].cnt > 0) {
            run_info->action = MAGICTECH_ACTION_END_CALLBACK;
        } else if (info->components[MAGICTECH_ACTION_CALLBACK].cnt > 0) {
            run_info->action = MAGICTECH_ACTION_CALLBACK;
        } else {
            run_info->action = MAGICTECH_ACTION_END;
        }
    }

    stru_5E3518.cnt = 0;
    if (!dword_5E7604) {
        dword_5E7604 = true;
        dword_5B0BA0 = mt_id;
        timeevent_clear_one_ex(TIMEEVENT_TYPE_MAGICTECH, sub_4570E0);
        dword_5E7604 = false;
    }

    sub_451070(run_info);
}

// 0x4571E0
void magictech_interrupt_delayed(int mt_id)
{
    MagicTechRunInfo* run_info;
    TimeEvent timeevent;
    DateTime datetime;

    ui_spell_maintain_end(mt_id);

    if (magictech_id_to_run_info(mt_id, &run_info)
        && run_info->action == MAGICTECH_ACTION_BEGIN
        && (run_info->field_13C & 0x04) == 0) {
        sub_456FA0(mt_id, 1);
        return;
    }

    timeevent.type = TIMEEVENT_TYPE_MAGICTECH;
    timeevent.params[0].integer_value = mt_id;
    timeevent.params[2].integer_value = 2;
    sub_45A950(&datetime, 1);

    if (!timeevent_add_delay(&timeevent, &datetime)) {
        tig_debug_printf("magictech_interrupt_delayed: Error: failed to queue timeevent!\n");
    }
}

// 0x457270
void sub_457270(int mt_id)
{
    MagicTechRunInfo* run_info;

    if (magictech_id_to_run_info(mt_id, &run_info)) {
        run_info->action = MAGICTECH_ACTION_END;
        stru_5E3518.cnt = 0;
        if (!dword_5E7604) {
            dword_5E7604 = true;
            dword_5B0BA0 = mt_id;
            timeevent_clear_one_ex(TIMEEVENT_TYPE_MAGICTECH, sub_4570E0);
            dword_5E7604 = false;
        }
        run_info->field_13C |= 0x40;
        sub_451070(run_info);
    }
}

// 0x4573D0
void sub_4573D0(MagicTechInvocation* mt_invocation)
{
    int idx;
    MagicTechRunInfo* run_info;

    if (mt_invocation->source_obj.obj) {
        for (idx = 0; idx < 512; idx++) {
            if (magictech_id_to_run_info(idx, &run_info)
                && run_info->parent_obj.obj == mt_invocation->parent_obj.obj
                && run_info->target_obj.obj == mt_invocation->target_obj.obj
                && run_info->spell == mt_invocation->spell) {
                magictech_interrupt(run_info->id);
                break;
            }
        }
    }
}

// 0x457450
void magictech_demaintain_spells(int64_t obj)
{
    int index;
    MagicTechInfo* info;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    for (index = 0; index < 512; index++) {
        if (magictech_run_info[index].source_obj.obj == obj) {
            info = &(magictech_spells[magictech_run_info[index].spell]);
            if ((info->flags & MAGICTECH_IS_TECH) == 0
                && (info->item_triggers == 0 || info->maintenance.period > 0)) {
                magictech_interrupt_delayed(magictech_run_info[index].id);
            }
        }
    }
}

// 0x4574D0
void sub_4574D0(int64_t obj)
{
    int index;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    for (index = 0; index < 512; index++) {
        if (magictech_run_info[index].parent_obj.obj == obj
            || magictech_run_info[index].source_obj.obj == obj) {
            sub_457530(magictech_run_info[index].id);
        }
    }
}

// 0x457530
void sub_457530(int mt_id)
{
    MagicTechRunInfo* run_info;

    if (magictech_id_to_run_info(mt_id, &run_info)) {
        run_info->source_obj.obj = OBJ_HANDLE_NULL;
        sub_443EB0(OBJ_HANDLE_NULL, &(run_info->source_obj.field_8));
        run_info->source_obj.type = -1;
    }
}

// 0x457580
void sub_457580(MagicTechInfo* info, int magictech)
{
    int index;

    if (dword_5E7600 != 0) {
        info->iq = 0;
    } else {
        info->iq = dword_5B0DE0[magictech % 5];
    }

    info->cost = 0;
    info->flags = 0;
    info->item_triggers = 0;
    info->maintenance.cost = 0;
    info->maintenance.period = 0;
    info->duration.period = 0;
    info->duration.stat = -1;
    info->duration.level = 0;
    info->duration.modifier = 1;
    info->duration_trigger_count = 0;
    info->range = 99;
    info->resistance.stat = -1;
    info->resistance.value = 0;
    info->missile = -1;
    info->casting_anim = -1;
    info->no_stack = 0;
    info->field_114 = 0;
    info->cancels_sf = 0;
    info->disallowed_sf = 0;
    info->disallowed_tsf = 0;
    info->disallowed_tcf = 0;
    info->cancels_envsf = 0;

    for (index = 0; index < MAGICTECH_ACTION_COUNT; index++) {
        info->pairs[index].caster = -1;
        info->pairs[index].target = -1;

        info->field_70[index].flags = 0;
        info->field_70[index].spell_flags = 0;
        info->field_70[index].no_spell_flags = 0;
        info->field_70[index].radius = 0;
        info->field_70[index].count = -1;

        info->components[index].cnt = 0;
        info->components[index].entries = NULL;
    }
}

// 0x457650
void magictech_build_aoe_info(MagicTechInfo* info, char* str)
{
    uint64_t aoe_flags;
    int action;
    unsigned int flags;
    int value;

    tig_str_parse_set_separator(',');

    if (!tig_str_parse_named_flag_list_64(&str, "AoE:", off_5BBD70, qword_596140, 65, &aoe_flags)) {
        tig_debug_printf("magictech_build_aoe_info: Error: failed match target AoE strings!\n");
    }

    for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
        if (dword_5E7628 == 2
            || dword_5E7628 == 9
            || dword_5E7628 == 1) {
            info->field_70[action].flags = aoe_flags;
        } else {
            info->field_70[action].flags = aoe_flags;
            info->field_70[action].flags |= 0x200000000000;
        }
    }

    if (tig_str_parse_named_flag_list_direct(&str, "AoE_SF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            info->field_70[action].spell_flags = flags;
        }
    } else {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            info->field_70[action].spell_flags = 0;
        }
    }

    if (tig_str_parse_named_flag_list_direct(&str, "AoE_NO_SF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            info->field_70[action].no_spell_flags = flags;
        }
    } else {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            info->field_70[action].no_spell_flags = 0;
        }
    }

    if (tig_str_parse_named_value(&str, "Radius:", &value)) {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            info->field_70[action].radius = value;
        }
    }

    if (tig_str_parse_named_value(&str, "Count:", &value)) {
        for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
            info->field_70[action].count = value;
        }
    }

    for (action = 0; action < MAGICTECH_ACTION_COUNT; action++) {
        if (tig_str_parse_named_flag_list_64(&str, off_5B0D64[action].aoe, off_5BBD70, qword_596140, 65, &aoe_flags)) {
            info->field_70[action].flags |= aoe_flags;
        }

        if (tig_str_parse_named_flag_list_direct(&str, off_5B0D64[action].aoe_sf, obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
            info->field_70[action].spell_flags |= flags;
        }

        if (tig_str_parse_named_flag_list_direct(&str, off_5B0D64[action].aoe_no_sf, obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
            info->field_70[action].no_spell_flags |= flags;
        }

        if (tig_str_parse_named_value(&str, off_5B0D64[action].radius, &value)) {
            info->field_70[action].radius = value;
        }

        if (tig_str_parse_named_value(&str, off_5B0D64[action].count, &value)) {
            info->field_70[action].count = value;
        }
    }
}

// 0x4578F0
void sub_4578F0(MagicTechInfo* info, char* str)
{
    char* curr = str;
    int value1;
    int value2;

    tig_str_parse_set_separator(',');

    if (tig_str_parse_named_value(&curr, "IQ:", &value1)) {
        info->iq = value1;
    }

    if (tig_str_parse_named_value(&curr, "Cost:", &value1)) {
        info->cost = value1;
    }

    if (tig_str_parse_named_complex_value(&curr, "Maintain:", '@', &value1, &value2)) {
        info->maintenance.cost = value1;
        info->maintenance.period = value2;
    }

    if (tig_str_parse_named_complex_value(&curr, "Duration:", '@', &value1, &value2)) {
        info->duration.period = value1;
        info->duration.stat = value2;

        if (info->duration.stat != -1) {
            if (tig_str_parse_named_complex_value(&curr, "DurationStatInfo:", '@', &value1, &value2)) {
                info->duration.level = value1;
                info->duration.modifier = value2;
            }
        }
    }

    if (tig_str_parse_named_value(&curr, "DurationTriggerCount:", &value1)) {
        info->duration_trigger_count = value1;
    }

    if (tig_str_parse_named_complex_str_value(&curr, "Resist:", '@', stat_lookup_keys_tbl, STAT_COUNT, &value1, &value2)) {
        info->resistance.stat = value1;
        info->resistance.value = value2;
    }

    if (tig_str_parse_named_value(&curr, "Range:", &value1)) {
        info->range = value1;
    }

    if (tig_str_parse_named_value(&curr, "ChargesFromCells:", &value1)) {
        if (value1 != 0) {
            info->flags |= 0x20;
        }
    }

    if (tig_str_parse_named_value(&curr, "ChargeBeginCost:", &value1)) {
        if (value1 == 0) {
            info->flags |= 0x0100;
        }
    }

    if (tig_str_match_named_str_to_list(&curr, "Info:", off_5B0C20, 4, &value1)) {
        info->flags |= dword_5B0C30[value1];
    }

    if (!tig_str_parse_named_value(&curr, "Disabled:", &value1) || value1 == 0) {
        info->flags |= MAGICTECH_IS_ENABLED;
    }
}

// 0x457B20
void sub_457B20(MagicTechInfo* info, char* str)
{
    char* curr = str;
    int value;

    tig_str_parse_set_separator(',');

    if (tig_str_parse_named_value(&curr, "CastingAnim:", &value)) {
        info->casting_anim = value;
    }

    if (tig_str_parse_named_value(&curr, "Missile:", &value)) {
        info->missile = value;
    }

    if (tig_str_parse_named_value(&curr, "[Begin]Caster:", &value)) {
        info->pairs[MAGICTECH_ACTION_BEGIN].caster = value;
    }

    if (tig_str_parse_named_value(&curr, "[Begin]Target:", &value)) {
        info->pairs[MAGICTECH_ACTION_BEGIN].target = value;
    }

    if (tig_str_parse_named_value(&curr, "[Maintain]Caster:", &value)) {
        info->pairs[MAGICTECH_ACTION_MAINTAIN].caster = value;
    }

    if (tig_str_parse_named_value(&curr, "[Maintain]Target:", &value)) {
        info->pairs[MAGICTECH_ACTION_MAINTAIN].target = value;
    }

    if (tig_str_parse_named_value(&curr, "[End]Caster:", &value)) {
        info->pairs[MAGICTECH_ACTION_END].caster = value;
    }

    if (tig_str_parse_named_value(&curr, "[End]Target:", &value)) {
        info->pairs[MAGICTECH_ACTION_END].target = value;
    }

    if (tig_str_parse_named_value(&curr, "[Callback]Caster:", &value)) {
        info->pairs[MAGICTECH_ACTION_CALLBACK].caster = value;
    }

    if (tig_str_parse_named_value(&curr, "[Callback]Target:", &value)) {
        info->pairs[MAGICTECH_ACTION_CALLBACK].target = value;
    }

    if (tig_str_parse_named_value(&curr, "[EndCallback]Caster:", &value)) {
        info->pairs[MAGICTECH_ACTION_END_CALLBACK].caster = value;
    }

    if (tig_str_parse_named_value(&curr, "[EndCallback]Target:", &value)) {
        info->pairs[MAGICTECH_ACTION_END_CALLBACK].target = value;
    }

    if (tig_str_parse_named_value(&curr, "Is_Tech:", &value)) {
        if (value != 0) {
            info->flags |= MAGICTECH_IS_TECH;
        }
    }
}

// 0x457D00
void sub_457D00(MagicTechInfo* info, char* str)
{
    char* curr = str;
    unsigned int flags;
    int value;

    tig_str_parse_set_separator(',');

    if (tig_str_parse_named_value(&curr, "No_Stack:", &value)) {
        info->no_stack = value;
    }

    if (tig_str_parse_named_flag_list_direct(&curr, "Cancels_SF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        info->cancels_sf = flags;
    }

    if (tig_str_parse_named_flag_list_direct(&curr, "Cancels_EnvSF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        info->cancels_envsf = flags;
    }

    if (tig_str_parse_named_flag_list_direct(&curr, "Disallowed_SF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        info->disallowed_sf = flags;
    }

    if (tig_str_parse_named_flag_list_direct(&curr, "Disallowed_TSF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        info->disallowed_tsf = flags;
    }

    if (tig_str_parse_named_flag_list_direct(&curr, "Disallowed_TCF:", obj_flags_tbl[OFS_CRITTER_FLAGS], obj_flags_size_tbl[OFS_CRITTER_FLAGS], &flags)) {
        info->disallowed_tcf = flags;
    }

    if (tig_str_parse_named_flag_list(&curr, "ItemTriggers:", mt_item_trig_keys, mt_item_trig_values, MT_ITEM_TRIG_COUNT, &flags)) {
        info->item_triggers = flags;
    }
}

// 0x457E70
void magictech_build_ai_info(MagicTechInfo* info, char* str)
{
    char* curr = str;
    int value1;
    int value2;

    tig_str_parse_set_separator(',');

    if (tig_str_parse_named_value(&curr, "AI_Flee:", &value1)) {
        info->ai.flee = value1;
    }

    if (tig_str_parse_named_value(&curr, "AI_Summon:", &value1)) {
        info->ai.summon = value1;
    }

    if (tig_str_parse_named_complex_value(&curr, "AI_Defensive:", '@', &value1, &value2)) {
        info->ai.defensive1 = value1;
        info->defensive2 = value2;
    }

    if (tig_str_parse_named_value(&curr, "AI_Offensive:", &value1)) {
        info->ai.offensive = value1;
    }

    if (tig_str_parse_named_value(&curr, "AI_HealingLight:", &value1)) {
        info->ai.healing_light = value1;
    }

    if (tig_str_parse_named_value(&curr, "AI_HealingMedium:", &value1)) {
        info->ai.healing_medium = value1;
    }

    if (tig_str_parse_named_value(&curr, "AI_HealingHeavy:", &value1)) {
        info->ai.healing_heavy = value1;
    }

    if (tig_str_parse_named_value(&curr, "AI_CurePoison:", &value1)) {
        info->ai.cure_poison = value1;
    }

    if (tig_str_parse_named_value(&curr, "AI_FatigueRecover:", &value1)) {
        info->ai.fatigue_recover = value1;
    }

    if (tig_str_parse_named_value(&curr, "AI_Resurrect:", &value1)) {
        info->ai.resurrect = value1;
    }

    if (tig_str_parse_named_value(&curr, "No_Resist:", &value1)) {
        info->flags |= MAGICTECH_NO_RESIST;
    }

    if (tig_str_parse_named_value(&curr, "No_Reflect:", &value1)) {
        info->flags |= MAGICTECH_NO_REFLECT;
    }
}

// 0x458060
void magictech_build_effect_info(MagicTechInfo* info, char* str)
{
    int action;
    MagicTechComponentList* component_list;
    MagicTechComponentInfo* component_info;
    uint64_t aoe;
    unsigned int flags;
    int value;

    tig_str_parse_set_separator(',');

    tig_str_match_str_to_list(&str, off_5B0C0C, MAGICTECH_ACTION_COUNT, &action);

    component_list = &(info->components[action]);
    if (component_list->entries != NULL) {
        component_list->entries = REALLOC(component_list->entries, sizeof(*component_list->entries) * (component_list->cnt + 1));
        if (component_list->entries == NULL) {
            tig_debug_printf("magictech_build_effect_info: Error: failed realloc component list!\n");
            exit(EXIT_FAILURE);
        }
    } else {
        component_list->entries = MALLOC(sizeof(*component_list->entries));
    }

    component_info = &(component_list->entries[component_list->cnt++]);
    component_info->aoe.flags = 0;
    component_info->aoe.spell_flags = 0;
    component_info->aoe.no_spell_flags = 0;
    component_info->aoe.radius = 0;
    component_info->aoe.count = -1;

    if (tig_str_parse_named_flag_list_64(&str, "AoE:", off_5BBD70, qword_596140, 65, &aoe)) {
        component_info->aoe.flags = aoe;
    }

    if (tig_str_parse_named_flag_list_direct(&str, "AoE_SF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        component_info->aoe.spell_flags |= flags;
    }

    if (tig_str_parse_named_flag_list_direct(&str, "AoE_NO_SF:", obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &flags)) {
        component_info->aoe.no_spell_flags |= flags;
    }

    if (tig_str_parse_named_value(&str, "Radius:", &value)) {
        component_info->aoe.radius = value;
    } else {
        component_info->aoe.radius = info->field_70[action].radius;
    }

    if (tig_str_parse_named_value(&str, "Count:", &value)) {
        component_info->aoe.count = value;
    }

    component_info->apply_aoe.flags = 0;
    component_info->apply_aoe.spell_flags = 0;
    component_info->apply_aoe.no_spell_flags = 0;
    component_info->apply_aoe.radius = 0;
    component_info->apply_aoe.count = -1;

    tig_str_parse_named_flag_list_64(&str, "Apply_AoE:", off_5BBD70, qword_596140, 65, &(component_info->apply_aoe.flags));

    if (tig_str_parse_named_flag_list(&str, "ItemTriggers:", mt_item_trig_keys, mt_item_trig_values, 26, &flags)) {
        component_info->item_triggers = flags;
    } else {
        component_info->item_triggers = 0;
    }

    if (tig_str_match_named_str_to_list(&str, "Type:", magictech_component_names, 25, &(component_info->type))) {
        switch (component_info->type) {
        case MTC_AGOAL:
            tig_str_match_str_to_list(&str, off_5B0C40, 6, &value);
            component_info->data.agoal.goal = dword_5B0C58[value];
            if (tig_str_parse_named_value(&str, "SubGoal:", &value)) {
                component_info->data.agoal.subgoal = value;
            } else {
                component_info->data.agoal.subgoal = 0;
            }
            break;
        case MTC_AGOALTERMINATE:
            tig_str_match_str_to_list(&str, off_5B0C40, 6, &value);
            component_info->data.agoal_terminate.goal = dword_5B0C58[value];
            break;
        case MTC_AIREDIRECT:
            tig_str_match_str_to_list(&str, obj_flags_ocf, 32, &value);
            component_info->data.ai_redirect.critter_flags = 1 << value;
            if (tig_str_parse_named_value(&str, "MinIQ:", &value)) {
                component_info->data.ai_redirect.min_iq = value;
            } else {
                component_info->data.ai_redirect.min_iq = -1;
            }
            break;
        case MTC_CAST:
            if (tig_str_parse_named_value(&str, "Spell:", &value)) {
                component_info->data.cast.spell = value;
            } else {
                component_info->data.cast.spell = 10000;
                tig_debug_printf("MagicTech: ERROR: Cast Component has no spell!\n");
            }
            break;
        case MTC_CHARGENBRANCH:
            component_info->data.charge_branch.branch = -1;
            component_info->data.charge_branch.cost = 0;
            tig_str_parse_named_value(&str, "Cost:", &(component_info->data.charge_branch.cost));
            if (tig_str_parse_named_value(&str, "Branch:", &value)) {
                component_info->data.charge_branch.branch = value;
            }
            break;
        case MTC_DAMAGE:
            tig_str_match_named_str_to_list(&str, "DmgType:", off_5B0C70, 6, &(component_info->data.damage.damage_type));
            tig_str_parse_named_range(&str, "Dmg:", &(component_info->data.damage.damage_min), &(component_info->data.damage.damage_max));
            component_info->data.damage.damage_flags = 0;
            tig_str_parse_named_flag_list(&str, "Dmg_Flags:", off_5B0D14, dword_5B0D3C, 10, &(component_info->data.damage.damage_flags));
            info->flags |= MAGICTECH_HAVE_DAMAGE;
            break;
        case MTC_EFFECT:
            tig_str_parse_value(&str, &(component_info->data.effect.num));
            tig_str_match_str_to_list(&str, off_5B0C88, 2, &(component_info->data.effect.add_remove));
            if (tig_str_parse_named_value(&str, "Count:", &value)) {
                component_info->data.effect.count = value;
            } else {
                component_info->data.effect.count = 1;
            }

            if (tig_str_match_named_str_to_list(&str, "Cause:", effect_cause_lookup_tbl, 10, &value)) {
                component_info->data.effect.cause = value;
            } else {
                component_info->data.effect.cause = 6;
            }

            if (tig_str_parse_named_value(&str, "Scaled:", &value)) {
                component_info->data.effect.scaled = value;
            } else {
                component_info->data.effect.scaled = 0;
            }
            break;
        case MTC_ENVFLAG:
            tig_str_match_str_to_list(&str, obj_flags_tbl[OFS_SPELL_FLAGS], obj_flags_size_tbl[OFS_SPELL_FLAGS], &value);
            component_info->data.env_flags.flags = 1 << value;
            tig_str_match_str_to_list(&str, off_5B0C90, 2, &(component_info->data.env_flags.state));
            break;
        case MTC_EYECANDY:
            tig_str_parse_value(&str, &(component_info->data.eye_candy.num));
            tig_str_match_str_to_list(&str, off_5B0C88, 2, &(component_info->data.eye_candy.add_remove));
            tig_str_parse_named_flag_list(&str,
                "Play:",
                animfx_play_flags_lookup_tbl_keys,
                animfx_play_flags_lookup_tbl_values,
                ANIMFX_PLAY_COUNT,
                &(component_info->data.eye_candy.flags));
            break;
        case MTC_HEAL:
            tig_str_match_named_str_to_list(&str, "DmgType:", off_5B0C70, 6, &(component_info->data.heal.damage_type));
            tig_str_parse_named_range(&str, "Dmg:", &(component_info->data.heal.damage_min), &(component_info->data.heal.damage_max));
            component_info->data.heal.damage_flags = 0;
            tig_str_parse_named_flag_list(&str, "Dmg_Flags:", off_5B0D14, dword_5B0D3C, 10, &(component_info->data.heal.damage_flags));
            break;
        case MTC_INTERRUPT:
            if (tig_str_parse_named_value(&str, "MagicTech:", &value)) {
                component_info->data.interrupt.magictech = value;
            } else {
                component_info->data.interrupt.magictech = 10000;
                tig_debug_printf("MagicTech: ERROR: Interrupt Component has no spell!\n");
            }
            break;
        case MTC_MOVEMENT:
            if (tig_str_match_named_str_to_list(&str, "Move_Location:", off_5B0D04, 4, &(component_info->data.movement.move_location))
                && component_info->data.movement.move_location == 1) {
                tig_str_parse_named_value(&str, "Tile_Radius:", &(component_info->data.movement.tile_radius));
            }
            break;
        case MTC_OBJFLAG:
            tig_str_match_str_to_list(&str, obj_flags_fields_lookup_tbl_keys, OFS_COUNT, &value);
            component_info->data.obj_flag.flags_fld = obj_flags_fields_lookup_tbl_values[value];
            tig_str_match_str_to_list(&str, obj_flags_tbl[value], obj_flags_size_tbl[value], &value);
            component_info->data.obj_flag.value = 1 << value;
            tig_str_match_str_to_list(&str, off_5B0C90, 2, &(component_info->data.obj_flag.state));
            if (component_info->data.obj_flag.flags_fld == OBJ_F_SPELL_FLAGS
                && component_info->data.obj_flag.state) {
                info->field_114 |= component_info->data.obj_flag.value;
            }
            break;
        case MTC_RECHARGE:
            tig_str_parse_value(&str, &(component_info->data.recharge.num));
            if (!tig_str_parse_named_value(&str, "Max:", &(component_info->data.recharge.max))) {
                component_info->data.recharge.max = 9999;
            }
            break;
        case MTC_SUMMON:
            if (!tig_str_parse_named_value(&str, "Proto:", &value)) {
                tig_debug_printf("magictech_build_effect_info: MTComponentSummon: Error: failed to find proto #!\n");
            }

            if (value > 0) {
                component_info->data.summon.oid = obj_get_id(sub_4685A0(value));
            } else {
                component_info->data.summon.oid.type = OID_TYPE_NULL;
            }

            if (tig_str_parse_named_value(&str, "Clear_Faction:", &value)) {
                component_info->data.summon.clear_faction = value;
            } else {
                component_info->data.summon.clear_faction = 0;
            }

            if (tig_str_parse_named_value(&str, "Palette:", &value)) {
                component_info->data.summon.palette = value;
            } else {
                component_info->data.summon.palette = 0;
            }

            if (tig_str_parse_named_value(&str, "List:", &value)) {
                component_info->data.summon.list = value;
            } else {
                component_info->data.summon.list = -1;
            }
            break;
        case MTC_TESTNBRANCH:
            component_info->data.test_in_branch.field_48 = 0;
            component_info->data.test_in_branch.field_4C = -1;
            tig_str_match_str_to_list(&str, off_5B0CAC, 4, &value);
            component_info->data.test_in_branch.field_40 = dword_5B0CBC[value];
            tig_str_match_str_to_list(&str, off_5B0C98, 5, &value);
            component_info->data.test_in_branch.field_44 = value;
            if (tig_str_parse_named_value(&str, "TestVal:", &value)) {
                component_info->data.test_in_branch.field_48 = value;
            }
            if (tig_str_parse_named_value(&str, "Branch:", &value)) {
                component_info->data.test_in_branch.field_4C = value;
            }
            break;
        case MTC_TRAIT:
            tig_str_match_str_to_list(&str, off_5B0CAC, 4, &value);
            component_info->data.trait.fld = dword_5B0CBC[value];
            component_info->data.trait.field_4 = 0;
            tig_str_parse_value(&str, &value);
            component_info->data.trait.value = value;
            component_info->data.trait.field_C = 1;
            component_info->data.trait.field_8 = 1;
            tig_str_parse_named_value(&str, "Palette:", &(component_info->data.trait.palette));
            break;
        case MTC_TRAITIDX:
            tig_str_match_str_to_list(&str, off_5B0CCC, 2, &value);
            component_info->data.trait_idx.field_44 = dword_5B0CD4[value];
            tig_str_match_str_to_list(&str, off_5B0CDC[value], dword_5B0CE4[value], &value);
            component_info->data.trait_idx.field_40 = value;
            component_info->data.trait_idx.field_48 = 0;
            tig_str_parse_value(&str, &value);
            component_info->data.trait_idx.field_54 = value;
            component_info->data.trait_idx.field_50 = 1;
            component_info->data.trait_idx.field_4C = 1;
            break;
        case MTC_TRAIT64:
            tig_str_match_str_to_list(&str, off_5B0CAC, 4, &value);
            component_info->data.trait64.field_40 = dword_5B0CBC[value];
            tig_str_match_str_to_list(&str, off_5B0CF8, 3, &value);
            component_info->data.trait64.field_44 = value;
            break;
        }
    }
}

// 0x458A80
bool magictech_check_env_sf(unsigned int flags)
{
    int64_t pc_obj;

    pc_obj = player_get_local_pc_obj();
    if (pc_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if ((obj_field_int32_get(pc_obj, OBJ_F_SPELL_FLAGS) & flags) == flags) {
        return true;
    }

    if (flags == OSF_DETECTING_INVISIBLE
        && stat_is_extraordinary(pc_obj, STAT_PERCEPTION)) {
        return true;
    }

    return false;
}

// 0x458AE0
tig_art_id_t sub_458AE0(int mt_id)
{
    MagicTechRunInfo* run_info;
    tig_art_id_t art_id;

    if (!magictech_id_to_run_info(mt_id, &run_info)) {
        return TIG_ART_ID_INVALID;
    }

    if (tig_art_interface_id_create(spell_icon(run_info->spell), 0, 0, 0, &art_id) != TIG_OK) {
        return TIG_ART_ID_INVALID;
    }

    return art_id;
}

// 0x458B60
const char* magictech_get_name(int magictech)
{
    return spell_name(magictech);
}

// 0x458B70
tig_art_id_t sub_458B70(int mt_id)
{
    MagicTechRunInfo* run_info;
    AnimFxListEntry* v2;
    tig_art_id_t eye_candy_art_id = TIG_ART_ID_INVALID;
    tig_art_id_t obj_art_id;
    int rotation;

    if (magictech_id_to_run_info(mt_id, &run_info)
        && animfx_id_get(&spell_eye_candies, run_info->spell * 6 + MAGICTECH_EYE_CANDY_TYPE_PROJECTILE, &v2)
        && run_info->parent_obj.obj != OBJ_HANDLE_NULL) {
        eye_candy_art_id = v2->eye_candy_art_id;
        if (eye_candy_art_id != TIG_ART_ID_INVALID) {
            obj_art_id = obj_field_int32_get(run_info->parent_obj.obj, OBJ_F_CURRENT_AID);
            rotation = tig_art_id_rotation_get(obj_art_id);
            eye_candy_art_id = tig_art_id_rotation_set(eye_candy_art_id, rotation);
        }
    }

    return eye_candy_art_id;
}

// 0x458C00
void sub_458C00(int spell, int64_t obj)
{
    MagicTechRunInfo* run_info;
    AnimFxListEntry* fx_info;

    if (obj == OBJ_HANDLE_NULL
        && magictech_id_to_run_info(spell, &run_info)
        && animfx_id_get(&spell_eye_candies, run_info->spell * 6 + MAGICTECH_EYE_CANDY_TYPE_PROJECTILE, &fx_info)) {
        if (tig_art_exists(fx_info->light_art_id) == TIG_OK) {
            object_set_light(obj, 0x20, fx_info->light_art_id, fx_info->light_color);
        }

        if (fx_info->sound != -1) {
            gsound_play_sfx_on_obj(fx_info->sound, 1, obj);
        }
    }
}

// 0x458CA0
int sub_458CA0(int mt_id)
{
    MagicTechRunInfo* run_info;
    AnimFxListEntry* fx_info;

    if (magictech_id_to_run_info(mt_id, &run_info)
        && animfx_id_get(&spell_eye_candies, run_info->spell * 6 + MAGICTECH_EYE_CANDY_TYPE_PROJECTILE, &fx_info)) {
        return fx_info->projectile_speed;
    } else {
        return 0;
    }
}

// 0x458CF0
bool magictech_find_first(int64_t obj, int* mt_id_ptr)
{
    int idx;
    MagicTechObjectNode* node;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0) {
            if (magictech_run_info[idx].target_obj.obj == obj) {
                if (mt_id_ptr != NULL) {
                    *mt_id_ptr = idx;
                }
                return true;
            }

            node = magictech_run_info[idx].summoned_obj;
            while (node != NULL) {
                if (node->obj == obj) {
                    if (mt_id_ptr != NULL) {
                        *mt_id_ptr = idx;
                    }
                    return true;
                }
                node = node->next;
            }

            node = magictech_run_info[idx].objlist;
            while (node != NULL) {
                if (node->obj == obj) {
                    if (mt_id_ptr != NULL) {
                        *mt_id_ptr = idx;
                    }
                    return true;
                }
                node = node->next;
            }
        }
    }

    return false;
}

// 0x458D90
bool magictech_find_next(int64_t obj, int* mt_id_ptr)
{
    int idx;
    MagicTechObjectNode* node;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    for (idx = *mt_id_ptr + 1; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0) {
            if (magictech_run_info[idx].target_obj.obj == obj) {
                if (mt_id_ptr != NULL) {
                    *mt_id_ptr = idx;
                }
                return true;
            }

            node = magictech_run_info[idx].summoned_obj;
            while (node != NULL) {
                if (node->obj == obj) {
                    if (mt_id_ptr != NULL) {
                        *mt_id_ptr = idx;
                    }
                    return true;
                }
                node = node->next;
            }

            node = magictech_run_info[idx].objlist;
            while (node != NULL) {
                if (node->obj == obj) {
                    if (mt_id_ptr != NULL) {
                        *mt_id_ptr = idx;
                    }
                    return true;
                }
                node = node->next;
            }
        }
    }

    return false;
}

// 0x459040
bool sub_459040(int64_t obj, unsigned int flags, int64_t* parent_obj_ptr)
{
    int idx;
    MagicTechObjectNode* node;

    if (parent_obj_ptr == NULL) {
        return false;
    }

    if (obj == OBJ_HANDLE_NULL) {
        *parent_obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    if ((obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & flags) == 0) {
        *parent_obj_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    for (idx = 0; idx < 512; idx++) {
        if (magictech_run_info[idx].source_obj.obj != OBJ_HANDLE_NULL) {
            if (magictech_run_info[idx].target_obj.obj == obj
                && (magictech_run_info[idx].field_138 & flags) == flags) {
                *parent_obj_ptr = magictech_run_info[idx].parent_obj.obj;
                return true;
            }

            node = magictech_run_info[idx].summoned_obj;
            while (node != NULL) {
                if (node->obj == obj
                    && (magictech_run_info[idx].field_138 & flags) == flags) {
                    *parent_obj_ptr = magictech_run_info[idx].parent_obj.obj;
                    return true;
                }
                node = node->next;
            }

            node = magictech_run_info[idx].objlist;
            while (node != NULL) {
                if (node->obj == obj
                    && (magictech_run_info[idx].field_138 & flags) == flags) {
                    *parent_obj_ptr = magictech_run_info[idx].parent_obj.obj;
                    return true;
                }
                node = node->next;
            }
        }
    }

    *parent_obj_ptr = OBJ_HANDLE_NULL;
    return false;
}

// 0x459170
bool sub_459170(int64_t obj, unsigned int flags, int* index_ptr)
{
    int idx;
    MagicTechObjectNode* node;

    if (index_ptr == NULL) {
        return false;
    }

    if (obj == OBJ_HANDLE_NULL) {
        *index_ptr = -1;
        return false;
    }

    if ((obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & flags) == 0) {
        *index_ptr = -1;
        return false;
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0) {
            if (magictech_run_info[idx].target_obj.obj == obj
                && (magictech_run_info[idx].field_138 & flags) == flags) {
                *index_ptr = idx;
                return true;
            }

            node = magictech_run_info[idx].summoned_obj;
            while (node != NULL) {
                if (node->obj == obj
                    && (magictech_run_info[idx].field_138 & flags) == flags) {
                    *index_ptr = idx;
                    return true;
                }
                node = node->next;
            }

            node = magictech_run_info[idx].objlist;
            while (node != NULL) {
                if (node->obj == obj
                    && (magictech_run_info[idx].field_138 & flags) == flags) {
                    *index_ptr = idx;
                    return true;
                }
                node = node->next;
            }
        }
    }

    *index_ptr = -1;
    return false;
}

// 0x459290
bool sub_459290(int64_t obj, int spell, int* index_ptr)
{
    int idx;
    MagicTechObjectNode* node;

    if (index_ptr == NULL) {
        return false;
    }

    if (obj == OBJ_HANDLE_NULL) {
        *index_ptr = -1;
        return false;
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0) {
            if (magictech_run_info[idx].target_obj.obj == obj
                && magictech_run_info[idx].spell == spell) {
                *index_ptr = idx;
                return true;
            }

            node = magictech_run_info[idx].summoned_obj;
            while (node != NULL) {
                if (node->obj == obj
                    && magictech_run_info[idx].spell == spell) {
                    *index_ptr = idx;
                    return true;
                }
                node = node->next;
            }

            node = magictech_run_info[idx].objlist;
            while (node != NULL) {
                if (node->obj == obj
                    && magictech_run_info[idx].spell == spell) {
                    *index_ptr = idx;
                    return true;
                }
                node = node->next;
            }
        }
    }

    *index_ptr = -1;
    return false;
}

// 0x459380
bool magictech_is_under_influence_of(int64_t obj, int magictech)
{
    int mt_id;
    MagicTechRunInfo* run_info;

    if (magictech_find_first(obj, &mt_id)) {
        do {
            if (magictech_id_to_run_info(mt_id, &run_info) && run_info->spell == magictech) {
                return true;
            }
        } while (magictech_find_next(obj, &mt_id));
    }

    return false;
}

// 0x4593F0
bool magictech_stop_spell(int64_t obj, int magictech)
{
    int mt_id;

    if (!sub_459290(obj, magictech, &mt_id)) {
        return false;
    }

    magictech_interrupt(mt_id);

    return true;
}

// 0x459430
bool magictech_timeevent_process(TimeEvent* timeevent)
{
    MagicTechRunInfo* run_info;

    switch (timeevent->params[2].integer_value) {
    case 1:
        sub_459490(timeevent->params[0].integer_value);
        break;
    case 2:
        magictech_interrupt(timeevent->params[0].integer_value);
        break;
    case 3:
        if (magictech_id_to_run_info(timeevent->params[0].integer_value, &run_info)) {
            sub_451070(run_info);
        }
        break;
    }

    return true;
}

// 0x459490
void sub_459490(int mt_id)
{
    MagicTechRunInfo* run_info;

    if (magictech_id_to_run_info(mt_id, &run_info)) {
        if (run_info->action == MAGICTECH_ACTION_BEGIN) {
            run_info->action = MAGICTECH_ACTION_MAINTAIN;
        }
        sub_451070(run_info);
    }
}

// 0x4594D0
bool sub_4594D0(TimeEvent* timeevent)
{
    return timeevent->params[0].integer_value == dword_5B0F20
        && timeevent->params[2].integer_value == dword_5E7634;
}

// 0x459500
bool sub_459500(int index)
{
    MagicTechRunInfo* run_info;
    TimeEvent timeevent;

    if (index == -1
        || !magictech_id_to_run_info(index, &run_info)
        || (run_info->field_13C & 0x1) == 0) {
        return false;
    }

    dword_5B0F20 = index;
    dword_5E7634 = 1;
    timeevent_clear_one_ex(TIMEEVENT_TYPE_MAGICTECH, sub_4594D0);

    timeevent.type = TIMEEVENT_TYPE_MAGICTECH;
    timeevent.params[0].integer_value = run_info->id;
    timeevent.params[2].integer_value = 1;
    return timeevent_add_base(&timeevent, &(run_info->field_148));
}

// 0x459590
bool magictech_recharge_timeevent_schedule(int64_t item_obj, int mana_cost, bool add)
{
    DateTime datetime;
    TimeEvent timeevent;

    magictech_recharge_timeevent_mana_cost = mana_cost;
    magictech_recharge_timeevent_item_obj = item_obj;
    dword_5E760C = sub_45A7F0();

    if (add) {
        if (timeevent_any(TIMEEVENT_TYPE_RECHARGE_MAGIC_ITEM, magictech_recharge_timeevent_add)) {
            return true;
        }
    }

    timeevent.type = TIMEEVENT_TYPE_RECHARGE_MAGIC_ITEM;
    timeevent.params[0].object_value = item_obj;
    timeevent.params[1].integer_value = dword_5E760C;
    timeevent.params[2].integer_value = magictech_recharge_timeevent_mana_cost;
    sub_45A950(&datetime, 60000);
    datetime.milliseconds *= 8;
    return timeevent_add_delay(&timeevent, &datetime);
}

// 0x459640
bool magictech_recharge_timeevent_add(TimeEvent* timeevent)
{
    if (timeevent->params[0].object_value != magictech_recharge_timeevent_item_obj) {
        return false;
    }

    timeevent->params[2].integer_value += magictech_recharge_timeevent_mana_cost;

    return true;
}

// 0x459680
bool magictech_recharge_timeevent_process(TimeEvent* timeevent)
{
    int64_t obj;
    int v1;
    int v2;
    int v3;
    int mana_store;
    int v4;
    int64_t parent_obj;
    int diff;

    obj = timeevent->params[0].object_value;
    v1 = timeevent->params[1].integer_value;
    v2 = timeevent->params[2].integer_value;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    v3 = sub_45A820(v1) / 120;
    if (v3 < 1) {
        v3 = 1;
    }

    mana_store = obj_field_int32_get(obj, OBJ_F_ITEM_MANA_STORE);
    v4 = 2 * v3;
    if (v4 > v2) {
        v4 = v2;
    }

    diff = mana_store - v4;
    obj_field_int32_set(obj, OBJ_F_ITEM_MANA_STORE, mana_store + v4);

    parent_obj = obj_field_handle_get(obj, OBJ_F_ITEM_PARENT);
    if (parent_obj != OBJ_HANDLE_NULL && player_is_local_pc_obj(parent_obj)) {
        sub_4605D0();
    }

    if (diff > 0) {
        magictech_recharge_timeevent_schedule(obj, diff, false);
    }

    return true;
}

// 0x459740
void sub_459740(int64_t obj)
{
    int idx;
    MagicTechObjectNode* node;

    if (!magictech_initialized) {
        return;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (light_scheme_is_changing()) {
        return;
    }

    if ((obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & OSF_SUMMONED) != 0) {
        if (!map_is_clearing_objects()) {
            sub_463860(obj, true);
        }

        for (idx = 0; idx < 512; idx++) {
            if ((magictech_run_info[idx].field_13C & 0x1) != 0
                && idx != dword_5B0BA4) {
                node = magictech_run_info[idx].summoned_obj;
                while (node != NULL) {
                    if (node->obj == obj) {
                        magictech_interrupt_delayed(magictech_run_info[idx].id);
                        break;
                    }
                    node = node->next;
                }
            }
        }
    }

    for (idx = 0; idx < 512; idx++) {
        if (idx != dword_5B0BA4) {
            if (magictech_run_info[idx].parent_obj.obj != OBJ_HANDLE_NULL
                && magictech_run_info[idx].parent_obj.obj == obj) {
                magictech_interrupt_delayed(magictech_run_info[idx].id);
            } else if (magictech_run_info[idx].source_obj.obj != OBJ_HANDLE_NULL
                && magictech_run_info[idx].source_obj.obj == obj) {
                sub_457530(magictech_run_info[idx].id);
            }
        }
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0
            && idx != dword_5B0BA4
            && magictech_run_info[idx].target_obj.obj == obj) {
            magictech_interrupt_delayed(magictech_run_info[idx].id);
        }
    }
}

// 0x4598D0
void sub_4598D0(int64_t obj)
{
    int idx;
    MagicTechObjectNode* node;

    if (!magictech_initialized) {
        return;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if ((obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS) & OSF_SUMMONED) != 0) {
        sub_463730(obj, true);

        for (idx = 0; idx < 512; idx++) {
            if ((magictech_run_info[idx].field_13C & 0x1) != 0
                && idx != dword_5B0BA4) {
                node = magictech_run_info[idx].summoned_obj;
                while (node != NULL) {
                    if (node->obj == obj) {
                        sub_457270(magictech_run_info[idx].id);
                        break;
                    }
                    node = node->next;
                }
            }
        }
    }

    for (idx = 0; idx < 512; idx++) {
        if (idx != dword_5B0BA4) {
            if (magictech_run_info[idx].parent_obj.obj == obj) {
                sub_457270(magictech_run_info[idx].id);
            }
        }
    }

    for (idx = 0; idx < 512; idx++) {
        if ((magictech_run_info[idx].field_13C & 0x1) != 0
            && idx != dword_5B0BA4
            && magictech_run_info[idx].target_obj.obj == obj) {
            sub_457270(magictech_run_info[idx].id);
        }
    }
}

// 0x459A20
void sub_459A20(int64_t obj)
{
}

// 0x459A30
void magictech_anim_play_hit_fx(int64_t obj, CombatContext* combat)
{
    unsigned int spell_flags;
    AnimFxNode node;
    int magictech;

    spell_flags = obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS);

    if ((spell_flags & OSF_SHIELDED) != 0) {
        if (sub_459170(obj, OSF_SHIELDED, &magictech)) {
            sub_4CCD20(&spell_eye_candies,
                &node,
                obj,
                magictech_run_info[magictech].id,
                6 * magictech_run_info[magictech].spell + MAGICTECH_EYE_CANDY_TYPE_DAMAGE);
            node.animate = true;
            animfx_add(&node);
        } else {
            tig_debug_printf("MagicTech: magictech_anim_play_hit_fx: Failed to match spell from flags!\n");
        }
    }

    if ((spell_flags & OSF_MAGNETIC_INVERSION) != 0) {
        if (sub_459170(obj, OSF_MAGNETIC_INVERSION, &magictech)) {
            sub_4CCD20(&spell_eye_candies,
                &node,
                obj,
                magictech_run_info[magictech].id,
                6 * magictech_run_info[magictech].spell + MAGICTECH_EYE_CANDY_TYPE_DAMAGE);
            node.animate = true;
            animfx_add(&node);
        } else {
            tig_debug_printf("MagicTech: magictech_anim_play_hit_fx: Failed to match spell from flags!\n");
            sub_4CCD20(&spell_eye_candies, &node, obj, -1, 1097);
            node.animate = true;
            animfx_add(&node);
        }
    }

    if ((spell_flags & OSF_ENTANGLED) != 0) {
        if (sub_459170(obj, OSF_ENTANGLED, &magictech)) {
            if (!sub_459C10(obj, magictech)) {
                sub_45A520(combat->attacker_obj, obj);
                magictech_interrupt_delayed(magictech);
            }
        }
    }

    if (sub_49B290(obj) == BP_KERGHAN_3 && !combat->total_dam) {
        sub_4CCD20(&spell_eye_candies, &node, obj, -1, 1259);
        node.animate = true;
        animfx_add(&node);
    }
}

// 0x459C10
bool sub_459C10(int64_t obj, int mt_id)
{
    MagicTechRunInfo* run_info;
    MagicTechInfo* info;
    MagicTechMaintenanceInfo* maintenance;
    int resistance;
    int roll;
    int v2;
    int v3 = 0;
    int v4;

    if (!magictech_id_to_run_info(mt_id, &run_info)) {
        return true;
    }

    info = &(magictech_spells[run_info->spell]);
    maintenance = magictech_get_maintenance(run_info->spell);
    if ((info->flags & MAGICTECH_IS_TECH) != 0
        || (run_info->field_13C & 0x10) != 0
        || obj == OBJ_HANDLE_NULL) {
        return true;
    }

    if (run_info->spell == SPELL_DISINTEGRATE
        && obj_type_is_critter(obj_field_int32_get(obj, OBJ_F_TYPE))
        && (obj_field_int32_get(obj, OBJ_F_CRITTER_FLAGS2) & OCF2_NO_DISINTEGRATE) != 0) {
        return false;
    }

    if (run_info->parent_obj.obj != obj
        && (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC
            || !fate_resolve(obj, FATE_SAVE_AGAINST_MAGICK))) {
        resistance = obj_arrayfield_int32_get(obj, OBJ_F_RESISTANCE_IDX, RESISTANCE_TYPE_MAGIC);
        if (!dword_5E75A8) {
            int aptitude = stat_level_get(obj, STAT_MAGICK_TECH_APTITUDE);
            if (aptitude < 0) {
                resistance = 100 - (aptitude + 100) * (100 - resistance) / 100;
            }
        }

        roll = random_between(1, 100);
        v2 = roll < resistance ? resistance - roll : 0;
        if (resistance > 0 && (info->flags & 0x80) == 0 && maintenance->period <= 0) {
            if (info->resistance.stat != -1) {
                v3 = v2 / 10;
            } else {
                if (roll < resistance) {
                    return 0;
                }
            }
        }
    }

    if (run_info->parent_obj.obj == obj) {
        return true;
    }

    if (info->resistance.stat != -1
        && obj_type_is_critter(dword_5E75AC)) {
        if (info->resistance.stat == STAT_WILLPOWER
            && stat_is_extraordinary(obj, STAT_WILLPOWER)) {
            if ((dword_5E7598->flags & MAGICTECH_AGGRESSIVE) != 0
                && run_info->action != MAGICTECH_ACTION_END) {
                sub_453F20(dword_5E75F0->parent_obj.obj, obj);
            }
            return false;
        } else {
            v4 = info->resistance.value + stat_level_get(obj, info->resistance.stat) - v3;
            return v4 <= 0 || random_between(1, 20) > v4;
        }
    }

    return true;
}

// 0x459EA0
void sub_459EA0(int64_t obj)
{
    unsigned int flags;
    int64_t leader_obj;

    if (obj != OBJ_HANDLE_NULL) {
        flags = obj_field_int32_get(obj, OBJ_F_SPELL_FLAGS);
        if ((flags & OSF_TEMPUS_FUGIT) != 0) {
            flags &= ~OSF_TEMPUS_FUGIT;
        }
        mp_obj_field_int32_set(obj, OBJ_F_SPELL_FLAGS, flags);

        if ((obj_field_int32_get(obj, OBJ_F_NPC_FLAGS) & ONF_FAMILIAR) != 0) {
            leader_obj = critter_leader_get(obj);
            if (leader_obj != OBJ_HANDLE_NULL) {
                flags = obj_field_int32_get(leader_obj, OBJ_F_SPELL_FLAGS);
                flags &= ~OSF_FAMILIAR;
                mp_obj_field_int32_set(leader_obj, OBJ_F_SPELL_FLAGS, flags);
            }
        }
    }
}

// 0x459F20
bool sub_459F20(int magictech, uint64_t** a2)
{
    // TODO: Unclear.
    *a2 = &(magictech_spells[magictech].field_70[0].flags);
    return true;
}

// 0x459F50
void sub_459F50()
{
    mt_ai_reset();
}

// 0x459F60
bool magictech_is_enabled(int magictech)
{
    return (magictech_spells[magictech].flags & MAGICTECH_IS_ENABLED) != 0;
}

// 0x459F90
bool magictech_is_magic(int magictech)
{
    return (magictech_spells[magictech].flags & MAGICTECH_IS_TECH) == 0;
}

// 0x459FC0
bool magictech_is_tech(int magictech)
{
    return (magictech_spells[magictech].flags & MAGICTECH_IS_TECH) != 0;
}

// 0x459FF0
bool sub_459FF0(int mt_id)
{
    MagicTechRunInfo* run_info;

    if (!magictech_id_to_run_info(mt_id, &run_info)) {
        return false;
    }

    return run_info->spell >= 0 && run_info->spell < MT_SPELL_COUNT;
}

// 0x45A030
bool sub_45A030(int magictech)
{
    return magictech_spells[magictech].item_triggers != 0;
}

// 0x45A060
int magictech_get_aptitude_adj(int64_t sector_id)
{
    int aptitude_adj;
    Sector* sector;

    aptitude_adj = 0;
    if (sector_lock(sector_id, &sector)) {
        aptitude_adj = sector->aptitude_adj;
        sector_unlock(sector_id);
    }

    return aptitude_adj;
}

// 0x45A480
void sub_45A480(MagicTechRunInfo* run_info)
{
    MagicTechObjectNode* node;

    while (run_info->objlist != NULL) {
        node = run_info->objlist;
        run_info->objlist = node->next;
        mt_obj_node_destroy(node);
    }

    while (run_info->summoned_obj != NULL) {
        node = run_info->summoned_obj;
        run_info->summoned_obj = node->next;
        mt_obj_node_destroy(node);
    }

    FREE(run_info);
}

// 0x45A4F0
void sub_45A4F0(int64_t obj, int fx_id, int mt_id)
{
    animfx_remove(&spell_eye_candies, obj, fx_id, mt_id);
}

// 0x45A520
void sub_45A520(int64_t a1, int64_t a2)
{
    (void)a1;

    magictech_fx_add(a2, MAGICTECH_FX_RESURRECT);
}

// 0x45A540
void magictech_error_unressurectable(int64_t obj)
{
    MesFileEntry mes_file_entry;

    mes_file_entry.num = 605; // "This life cannot be replenished."
    magictech_get_msg(&mes_file_entry);
    sub_460610(mes_file_entry.str);

    magictech_fx_add(obj, MAGICTECH_FX_RESURRECT);
}

// 0x45A580
bool sub_45A580(int64_t a1, int64_t a2)
{
    if ((obj_field_int32_get(a2, OBJ_F_SPELL_FLAGS) & OSF_INVISIBLE) == 0
        || (obj_field_int32_get(a1, OBJ_F_SPELL_FLAGS) & OSF_DETECTING_INVISIBLE) != 0) {
        return true;
    }

    if ((obj_field_int32_get(a2, OBJ_F_FLAGS) & OF_INVISIBLE) != 0) {
        return false;
    }

    if ((obj_field_int32_get(a2, OBJ_F_TYPE) != OBJ_TYPE_PC)) {
        return true;
    }

    if (anim_is_current_goal_type(a2, AG_ATTEMPT_ATTACK, NULL)) {
        return true;
    }

    if (anim_is_current_goal_type(a2, AG_ATTEMPT_SPELL, NULL)) {
        return true;
    }

    if (combat_critter_is_combat_mode_active(a2)) {
        return true;
    }

    return false;
}

// 0x45A620
void magictech_debug_lists()
{
    int index;
    MagicTechRunInfo* run_info;
    MagicTechObjectNode* node;

    tig_debug_printf("\n\nMagicTech DEBUG Lists:\n");
    tig_debug_printf("----------------------\n\n");

    for (index = 0; index < 512; index++) {
        run_info = &(magictech_run_info[index]);
        if ((run_info->field_13C & 0x1) != 0) {
            tig_debug_printf("mtID: [%d], Spell: %s(%d)\n",
                index,
                magictech_spell_name(run_info->spell),
                run_info->spell);
            tig_debug_printf("\tAction: %s\n", off_5B0C0C[run_info->action]);
            sub_45A760(run_info->parent_obj.obj, "Parent");
            sub_45A760(run_info->source_obj.obj, "Source");
            sub_45A760(run_info->target_obj.obj, "Target");
            node = run_info->summoned_obj;
            while (node != NULL) {
                sub_45A760(node->obj, "SummonedObj");
                node = node->next;
            }
            node = run_info->objlist;
            while (node != NULL) {
                sub_45A760(node->obj, "ObjList");
                node = node->next;
            }
            tig_debug_printf("\n");
        }
    }

    tig_debug_printf("Local PC Has %d Maintained Spells\n",
        sub_450AC0(player_get_local_pc_obj()));
    tig_debug_printf("\n\n");
}

// 0x45A760
void sub_45A760(int64_t obj, const char* msg)
{
    // 0x5E6D94
    static char buffer[2000];

    if (obj != OBJ_HANDLE_NULL) {
        object_examine(obj, obj, buffer);
        tig_debug_printf("\t%s: %s(%I64d), Loc: [%I64d x %I64d]\n",
            msg,
            buffer,
            obj,
            obj_field_int64_get(obj, OBJ_F_LOCATION),
            obj_field_int64_get(obj, OBJ_F_LOCATION));
    }
}
