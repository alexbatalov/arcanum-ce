#include "game/obj.h"

#include <inttypes.h>

#include "game/item.h"
#include "game/obj_file.h"
#include "game/obj_find.h"
#include "game/obj_private.h"
#include "game/oname.h"
#include "game/skill.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/tech.h"

#define OBJ_FILE_VERSION 119

// CE: The type of `data` and `transient_properties` is changed to `intptr_t` to
// handle complex fields (virtually everything besides plain integers is stored
// as pointers).
typedef struct Object {
    /* 0000 */ int type;
    /* 0008 */ ObjectID oid;
    /* 0020 */ ObjectID prototype_oid;
    /* 0038 */ int64_t prototype_obj;
    /* 0040 */ int field_40;
    /* 0044 */ int16_t modified;
    /* 0046 */ int16_t num_fields;
    /* 0048 */ int* field_48;
    /* 004C */ int* field_4C;
    /* 0050 */ intptr_t* data;
    /* 0054 */ intptr_t transient_properties[19];
} Object;

typedef bool(ObjectProtoEnumerateFieldsCallback)(Object* object, int fld);

typedef struct ObjectFieldInfo {
    /* 0000 */ int simple_array_idx;
    /* 0004 */ int complex_array_idx;
    /* 0008 */ int change_array_idx;
    /* 000C */ unsigned int mask;
    /* 0010 */ int bit;
    /* 0014 */ int cnt;
    /* 0018 */ int type;
} ObjectFieldInfo;

typedef bool(ObjectInstEnumerateFieldsCallback)(Object* object, int idx, ObjectFieldInfo* field_info);

static void object_field_not_exists(Object* object, int fld);
static void obj_debug_aid(tig_art_id_t aid);
static Object* obj_allocate(int64_t* obj_ptr);
static Object* obj_lock(int64_t obj);
static void obj_unlock(int64_t obj);
static void obj_field_store(Object* object, int fld, void* value_ptr);
static void obj_arrayfield_store(Object* object, int fld, int index, void* value_ptr);
static void obj_field_fetch(Object* object, int fld, void* value_ptr);
static void obj_arrayfield_fetch(Object* object, int fld, int index, void* value);
static void sub_408D60(Object* object, int fld, int* value_ptr);
static void sub_408E70(Object* object, int fld, int value);
static bool sub_408F40(Object* object, int fld, SizeableArray*** ptr, int64_t* proto_handle_ptr);
static void obj_set_defaults(int64_t obj);
static void set_item_defaults(int64_t obj, int subtype);
static bool obj_proto_write_file(TigFile* stream, int64_t obj);
static bool obj_proto_read_file(TigFile* stream, int64_t* obj_ptr, ObjectID oid);
static bool obj_inst_write_file(TigFile* stream, int64_t obj);
static bool obj_inst_read_file(TigFile* stream, int64_t* obj_ptr, ObjectID oid);
static void obj_proto_write_mem(WriteBuffer* wb, int64_t obj);
static bool obj_proto_read_mem(uint8_t* data, int64_t* obj_ptr);
static void obj_inst_write_mem(WriteBuffer* wb, int64_t obj);
static bool obj_inst_read_mem(uint8_t* data, int64_t* obj_ptr);
static bool obj_proto_field_write_file(Object* object, int fld);
static bool obj_proto_field_write_mem(Object* object, int fld);
static bool obj_proto_field_read_file(Object* object, int fld);
static bool obj_proto_field_read_mem(Object* object, int fld);
static bool object_field_write(Object* object, int idx, ObjectFieldInfo* info);
static bool obj_inst_field_write_mem(Object* object, int idx, ObjectFieldInfo* info);
static bool object_field_read(Object* object, int idx, ObjectFieldInfo* info);
static bool obj_inst_field_read_mem(Object* object, int idx, ObjectFieldInfo* info);
static bool object_field_write_if_dif(Object* object, int idx, ObjectFieldInfo* info);
static bool object_field_read_if_dif(Object* object, int idx, ObjectFieldInfo* info);
static void sub_40A400(void);
static void sub_40A740(int fld, int* start_ptr, int* length_ptr);
static int sub_40A790(int a1);
static void sub_40A7B0(void);
static void sub_40A8A0(void);
static void sub_40B8E0(int fld);
static void obj_handle_field_lists_init(void);
static void obj_handle_field_lists_exit(void);
static void object_convert_scalar_handles_to_ids(Object* object);
static void object_convert_array_handles_to_ids(Object* object);
static void object_convert_scalar_ids_to_handles(Object* object);
static void object_convert_array_ids_to_handles(Object* object);
static bool convert_handle_to_id(void* entry, int index);
static bool convert_id_to_handle(void* entry, int index);
static int sub_40C030(ObjectType object_type);
static bool object_field_valid(int type, int fld);
static bool sub_40C560(Object* object, int fld);
static void sub_40C580(Object* object);
static void sub_40C5B0(Object* object);
static void sub_40C5C0(Object* dst, Object* src);
static void sub_40C610(Object* object);
static void sub_40C640(Object* object);
static void sub_40C650(Object* dst, Object* src);
static void sub_40C690(Object* object);
static bool sub_40C6B0(Object* object, int fld);
static bool object_proto_field_dealloc(Object* object, int fld);
static bool object_proto_field_copy(Object* object, int fld);
static bool object_inst_field_copy(Object* object, int storage_idx, ObjectFieldInfo* info);
static void object_transient_field_copy(Object* dst, Object* src, int fld);
static void object_transient_field_dealloc(Object* object, int fld);
static bool object_proto_enumerate_fields(Object* object, ObjectProtoEnumerateFieldsCallback* callback);
static bool object_proto_enumerate_fields_func(Object* obj, int begin, int end, ObjectProtoEnumerateFieldsCallback* callback);
static int sub_40CB40(Object* object, int fld);
static bool object_inst_field_dealloc(Object* object, int storage_idx, ObjectFieldInfo* info);
static bool object_inst_enumerate_overridden_fields(Object* object, ObjectInstEnumerateFieldsCallback* callback);
static bool object_inst_enumerate_overridden_fields_func(Object* object, int begin, int end, ObjectInstEnumerateFieldsCallback* callback);
static bool object_inst_enumerate_available_fields(Object* object, ObjectInstEnumerateFieldsCallback* callback);
static bool object_inst_enumerate_available_fields_func(Object* object, int begin, int end, ObjectInstEnumerateFieldsCallback* callback);
static int sub_40D230(Object* object, int fld);
static void sub_40D2A0(Object* object, int fld);
static bool sub_40D320(Object* object, int fld);
static bool sub_40D350(Object* object, int fld);
static void sub_40D370(Object* object, int fld, bool enabled);
static void sub_40D3A0(Object* object, ObjectFieldInfo* info, int enabled);
static bool sub_40D3D0(Object* object, int fld);
static void sub_40D400(Object* object, int fld, bool enabled);
static void sub_40D450(Object* object, int fld);
static void sub_40D470(Object* object, int fld);
static bool obj_version_write_file(TigFile* stream);
static bool obj_version_read_file(TigFile* stream);
static void obj_version_write_mem(WriteBuffer* wb);
static bool obj_version_read_mem(uint8_t** data);
static bool sub_40D670(Object* object, int a2, ObjectFieldInfo* field_info);
static int64_t obj_get_prototype_handle(Object* object);

// 0x59BE00
static int dword_59BE00[] = {
    OBJ_F_BEGIN,
    OBJ_F_WALL_BEGIN,
    OBJ_F_PORTAL_BEGIN,
    OBJ_F_CONTAINER_BEGIN,
    OBJ_F_SCENERY_BEGIN,
    OBJ_F_PROJECTILE_BEGIN,
    OBJ_F_ITEM_BEGIN,
    OBJ_F_WEAPON_BEGIN,
    OBJ_F_AMMO_BEGIN,
    OBJ_F_ARMOR_BEGIN,
    OBJ_F_GOLD_BEGIN,
    OBJ_F_FOOD_BEGIN,
    OBJ_F_SCROLL_BEGIN,
    OBJ_F_KEY_BEGIN,
    OBJ_F_KEY_RING_BEGIN,
    OBJ_F_WRITTEN_BEGIN,
    OBJ_F_GENERIC_BEGIN,
    OBJ_F_CRITTER_BEGIN,
    OBJ_F_PC_BEGIN,
    OBJ_F_NPC_BEGIN,
    OBJ_F_TRAP_BEGIN,
};

// 0x59BE58
static struct {
    int hp;
    int worth;
} obj_item_defaults[] = {
    { 8, 8 },
    { 5, 1 },
    { 8, 8 },
    { 10, 1 },
    { 2, 2 },
    { 1, 6 },
    { 3, 2 },
    { 3, 2 },
    { 1, 1 },
    { 3, 2 },
};

// 0x59BEA8
const char* object_field_names[] = {
    "obj_f_begin",
    "obj_f_current_aid",
    "obj_f_location",
    "obj_f_offset_x",
    "obj_f_offset_y",
    "obj_f_shadow",
    "obj_f_overlay_fore",
    "obj_f_overlay_back",
    "obj_f_underlay",
    "obj_f_blit_flags",
    "obj_f_blit_color",
    "obj_f_blit_alpha",
    "obj_f_blit_scale",
    "obj_f_light_flags",
    "obj_f_light_aid",
    "obj_f_light_color",
    "obj_f_overlay_light_flags",
    "obj_f_overlay_light_aid",
    "obj_f_overlay_light_color",
    "obj_f_flags",
    "obj_f_spell_flags",
    "obj_f_blocking_mask",
    "obj_f_name",
    "obj_f_description",
    "obj_f_aid",
    "obj_f_destroyed_aid",
    "obj_f_AC",
    "obj_f_hp_pts",
    "obj_f_hp_adj",
    "obj_f_hp_damage",
    "obj_f_material",
    "obj_f_resistance_idx",
    "obj_f_scripts_idx",
    "obj_f_sound_effect",
    "obj_f_category",
    "obj_f_pad_ias_1",
    "obj_f_pad_i64as_1",
    "obj_f_end",
    "obj_f_wall_begin",
    "obj_f_wall_flags",
    "obj_f_wall_pad_i_1",
    "obj_f_wall_pad_i_2",
    "obj_f_wall_pad_ias_1",
    "obj_f_wall_pad_i64as_1",
    "obj_f_wall_end",
    "obj_f_portal_begin",
    "obj_f_portal_flags",
    "obj_f_portal_lock_difficulty",
    "obj_f_portal_key_id",
    "obj_f_portal_notify_npc",
    "obj_f_portal_pad_i_1",
    "obj_f_portal_pad_i_2",
    "obj_f_portal_pad_ias_1",
    "obj_f_portal_pad_i64as_1",
    "obj_f_portal_end",
    "obj_f_container_begin",
    "obj_f_container_flags",
    "obj_f_container_lock_difficulty",
    "obj_f_container_key_id",
    "obj_f_container_inventory_num",
    "obj_f_container_inventory_list_idx",
    "obj_f_container_inventory_source",
    "obj_f_container_notify_npc",
    "obj_f_container_pad_i_1",
    "obj_f_container_pad_i_2",
    "obj_f_container_pad_ias_1",
    "obj_f_container_pad_i64as_1",
    "obj_f_container_end",
    "obj_f_scenery_begin",
    "obj_f_scenery_flags",
    "obj_f_scenery_whos_in_me",
    "obj_f_scenery_respawn_delay",
    "obj_f_scenery_pad_i_2",
    "obj_f_scenery_pad_ias_1",
    "obj_f_scenery_pad_i64as_1",
    "obj_f_scenery_end",
    "obj_f_projectile_begin",
    "obj_f_projectile_flags_combat",
    "obj_f_projectile_flags_combat_damage",
    "obj_f_projectile_hit_loc",
    "obj_f_projectile_parent_weapon",
    "obj_f_projectile_pad_i_1",
    "obj_f_projectile_pad_i_2",
    "obj_f_projectile_pad_ias_1",
    "obj_f_projectile_pad_i64as_1",
    "obj_f_projectile_end",
    "obj_f_item_begin",
    "obj_f_item_flags",
    "obj_f_item_parent",
    "obj_f_item_weight",
    "obj_f_item_magic_weight_adj",
    "obj_f_item_worth",
    "obj_f_item_mana_store",
    "obj_f_item_inv_aid",
    "obj_f_item_inv_location",
    "obj_f_item_use_aid_fragment",
    "obj_f_item_magic_tech_complexity",
    "obj_f_item_discipline",
    "obj_f_item_description_unknown",
    "obj_f_item_description_effects",
    "obj_f_item_spell_1",
    "obj_f_item_spell_2",
    "obj_f_item_spell_3",
    "obj_f_item_spell_4",
    "obj_f_item_spell_5",
    "obj_f_item_spell_mana_store",
    "obj_f_item_ai_action",
    "obj_f_item_pad_i_1",
    "obj_f_item_pad_ias_1",
    "obj_f_item_pad_i64as_1",
    "obj_f_item_end",
    "obj_f_weapon_begin",
    "obj_f_weapon_flags",
    "obj_f_weapon_paper_doll_aid",
    "obj_f_weapon_bonus_to_hit",
    "obj_f_weapon_magic_hit_adj",
    "obj_f_weapon_damage_lower_idx",
    "obj_f_weapon_damage_upper_idx",
    "obj_f_weapon_magic_damage_adj_idx",
    "obj_f_weapon_speed_factor",
    "obj_f_weapon_magic_speed_adj",
    "obj_f_weapon_range",
    "obj_f_weapon_magic_range_adj",
    "obj_f_weapon_min_strength",
    "obj_f_weapon_magic_min_strength_adj",
    "obj_f_weapon_ammo_type",
    "obj_f_weapon_ammo_consumption",
    "obj_f_weapon_missile_aid",
    "obj_f_weapon_visual_effect_aid",
    "obj_f_weapon_crit_hit_chart",
    "obj_f_weapon_magic_crit_hit_chance",
    "obj_f_weapon_magic_crit_hit_effect",
    "obj_f_weapon_crit_miss_chart",
    "obj_f_weapon_magic_crit_miss_chance",
    "obj_f_weapon_magic_crit_miss_effect",
    "obj_f_weapon_pad_i_1",
    "obj_f_weapon_pad_i_2",
    "obj_f_weapon_pad_ias_1",
    "obj_f_weapon_pad_i64as_1",
    "obj_f_weapon_end",
    "obj_f_ammo_begin",
    "obj_f_ammo_flags",
    "obj_f_ammo_quantity",
    "obj_f_ammo_type",
    "obj_f_ammo_pad_i_1",
    "obj_f_ammo_pad_i_2",
    "obj_f_ammo_pad_ias_1",
    "obj_f_ammo_pad_i64as_1",
    "obj_f_ammo_end",
    "obj_f_armor_begin",
    "obj_f_armor_flags",
    "obj_f_armor_paper_doll_aid",
    "obj_f_armor_ac_adj",
    "obj_f_armor_magic_ac_adj",
    "obj_f_armor_resistance_adj_idx",
    "obj_f_armor_magic_resistance_adj_idx",
    "obj_f_armor_silent_move_adj",
    "obj_f_armor_magic_silent_move_adj",
    "obj_f_armor_unarmed_bonus_damage",
    "obj_f_armor_pad_i_2",
    "obj_f_armor_pad_ias_1",
    "obj_f_armor_pad_i64as_1",
    "obj_f_armor_end",
    "obj_f_gold_begin",
    "obj_f_gold_flags",
    "obj_f_gold_quantity",
    "obj_f_gold_pad_i_1",
    "obj_f_gold_pad_i_2",
    "obj_f_gold_pad_ias_1",
    "obj_f_gold_pad_i64as_1",
    "obj_f_gold_end",
    "obj_f_food_begin",
    "obj_f_food_flags",
    "obj_f_food_pad_i_1",
    "obj_f_food_pad_i_2",
    "obj_f_food_pad_ias_1",
    "obj_f_food_pad_i64as_1",
    "obj_f_food_end",
    "obj_f_scroll_begin",
    "obj_f_scroll_flags",
    "obj_f_scroll_pad_i_1",
    "obj_f_scroll_pad_i_2",
    "obj_f_scroll_pad_ias_1",
    "obj_f_scroll_pad_i64as_1",
    "obj_f_scroll_end",
    "obj_f_key_begin",
    "obj_f_key_key_id",
    "obj_f_key_pad_i_1",
    "obj_f_key_pad_i_2",
    "obj_f_key_pad_ias_1",
    "obj_f_key_pad_i64as_1",
    "obj_f_key_end",
    "obj_f_key_ring_begin",
    "obj_f_key_ring_flags",
    "obj_f_key_ring_list_idx",
    "obj_f_key_ring_pad_i_1",
    "obj_f_key_ring_pad_i_2",
    "obj_f_key_ring_pad_ias_1",
    "obj_f_key_ring_pad_i64as_1",
    "obj_f_key_ring_end",
    "obj_f_written_begin",
    "obj_f_written_flags",
    "obj_f_written_subtype",
    "obj_f_written_text_start_line",
    "obj_f_written_text_end_line",
    "obj_f_written_pad_i_1",
    "obj_f_written_pad_i_2",
    "obj_f_written_pad_ias_1",
    "obj_f_written_pad_i64as_1",
    "obj_f_written_end",
    "obj_f_generic_begin",
    "obj_f_generic_flags",
    "obj_f_generic_usage_bonus",
    "obj_f_generic_usage_count_remaining",
    "obj_f_generic_pad_ias_1",
    "obj_f_generic_pad_i64as_1",
    "obj_f_generic_end",
    "obj_f_critter_begin",
    "obj_f_critter_flags",
    "obj_f_critter_flags2",
    "obj_f_critter_stat_base_idx",
    "obj_f_critter_basic_skill_idx",
    "obj_f_critter_tech_skill_idx",
    "obj_f_critter_spell_tech_idx",
    "obj_f_critter_fatigue_pts",
    "obj_f_critter_fatigue_adj",
    "obj_f_critter_fatigue_damage",
    "obj_f_critter_crit_hit_chart",
    "obj_f_critter_effects_idx",
    "obj_f_critter_effect_cause_idx",
    "obj_f_critter_fleeing_from",
    "obj_f_critter_portrait",
    "obj_f_critter_gold",
    "obj_f_critter_arrows",
    "obj_f_critter_bullets",
    "obj_f_critter_power_cells",
    "obj_f_critter_fuel",
    "obj_f_critter_inventory_num",
    "obj_f_critter_inventory_list_idx",
    "obj_f_critter_inventory_source",
    "obj_f_critter_description_unknown",
    "obj_f_critter_follower_idx",
    "obj_f_critter_teleport_dest",
    "obj_f_critter_teleport_map",
    "obj_f_critter_death_time",
    "obj_f_critter_auto_level_scheme",
    "obj_f_critter_pad_i_1",
    "obj_f_critter_pad_i_2",
    "obj_f_critter_pad_i_3",
    "obj_f_critter_pad_ias_1",
    "obj_f_critter_pad_i64as_1",
    "obj_f_critter_end",
    "obj_f_pc_begin",
    "obj_f_pc_flags",
    "obj_f_pc_flags_fate",
    "obj_f_pc_reputation_idx",
    "obj_f_pc_reputation_ts_idx",
    "obj_f_pc_background",
    "obj_f_pc_background_text",
    "obj_f_pc_quest_idx",
    "obj_f_pc_blessing_idx",
    "obj_f_pc_blessing_ts_idx",
    "obj_f_pc_curse_idx",
    "obj_f_pc_curse_ts_idx",
    "obj_f_pc_party_id",
    "obj_f_pc_rumor_idx",
    "obj_f_pc_pad_ias_2",
    "obj_f_pc_schematics_found_idx",
    "obj_f_pc_logbook_ego_idx",
    "obj_f_pc_fog_mask",
    "obj_f_pc_player_name",
    "obj_f_pc_bank_money",
    "obj_f_pc_global_flags",
    "obj_f_pc_global_variables",
    "obj_f_pc_pad_i_1",
    "obj_f_pc_pad_i_2",
    "obj_f_pc_pad_ias_1",
    "obj_f_pc_pad_i64as_1",
    "obj_f_pc_end",
    "obj_f_npc_begin",
    "obj_f_npc_flags",
    "obj_f_npc_leader",
    "obj_f_npc_ai_data",
    "obj_f_npc_combat_focus",
    "obj_f_npc_who_hit_me_last",
    "obj_f_npc_experience_worth",
    "obj_f_npc_experience_pool",
    "obj_f_npc_waypoints_idx",
    "obj_f_npc_waypoint_current",
    "obj_f_npc_standpoint_day",
    "obj_f_npc_standpoint_night",
    "obj_f_npc_origin",
    "obj_f_npc_faction",
    "obj_f_npc_retail_price_multiplier",
    "obj_f_npc_substitute_inventory",
    "obj_f_npc_reaction_base",
    "obj_f_npc_social_class",
    "obj_f_npc_reaction_pc_idx",
    "obj_f_npc_reaction_level_idx",
    "obj_f_npc_reaction_time_idx",
    "obj_f_npc_wait",
    "obj_f_npc_generator_data",
    "obj_f_npc_pad_i_1",
    "obj_f_npc_damage_idx",
    "obj_f_npc_shit_list_idx",
    "obj_f_npc_end",
    "obj_f_trap_begin",
    "obj_f_trap_flags",
    "obj_f_trap_difficulty",
    "obj_f_trap_pad_i_2",
    "obj_f_trap_pad_ias_1",
    "obj_f_trap_pad_i64as_1",
    "obj_f_trap_end",
    "obj_f_total_normal",
    "obj_f_transient_begin",
    "obj_f_render_color",
    "obj_f_render_colors",
    "obj_f_render_palette",
    "obj_f_render_scale",
    "obj_f_render_alpha",
    "obj_f_render_x",
    "obj_f_render_y",
    "obj_f_render_width",
    "obj_f_render_height",
    "obj_f_palette",
    "obj_f_color",
    "obj_f_colors",
    "obj_f_render_flags",
    "obj_f_temp_id",
    "obj_f_light_handle",
    "obj_f_overlay_light_handles",
    "obj_f_shadow_handles",
    "obj_f_internal_flags",
    "obj_f_find_node",
    "obj_f_transient_end",
    "obj_f_type",
    "obj_f_prototype_handle",
    "obj_f_max",
};

// 0x5D10F0
static int* dword_5D10F0;

// 0x5D10F4
static int dword_5D10F4;

// 0x5D10F8
static bool obj_editor;

// 0x5D10FC
static int16_t word_5D10FC;

// 0x5D1100
static int* dword_5D1100;

// 0x5D1104
static ObjectFieldInfo* object_fields;

// 0x5D1108
static Object* dword_5D1108;

// 0x5D110C
static TigFile* dword_5D110C;

// 0x5D1110
static Object* dword_5D1110;

// 0x5D1118
static WriteBuffer* dword_5D1118;

// 0x5D111C
static uint8_t* dword_5D111C;

// 0x5D1120
static int16_t* object_fields_count_per_type;

// 0x5D1124
static bool obj_initialized;

// 0x5D1128
static ObjectField* obj_scalar_handle_fields;

// 0x5D112C
static ObjectField* obj_array_handle_fields;

// 0x5D1130
static int obj_scalar_handle_field_cnt;

// 0x5D1134
static int obj_array_handle_field_cnt;

// 0x405110
bool obj_init(GameInitInfo* init_info)
{
    int index;
    Object object;

    object_fields = (ObjectFieldInfo*)CALLOC(OBJ_F_MAX, sizeof(ObjectFieldInfo));
    dword_5D10F0 = (int*)CALLOC(21, sizeof(int));
    dword_5D1100 = (int*)CALLOC(21, sizeof(int));
    object_fields_count_per_type = (int16_t*)CALLOC(18, sizeof(int16_t));
    obj_editor = init_info->editor;
    bitset_pool_init();
    obj_pool_init(sizeof(Object), obj_editor);
    obj_data_init();
    obj_find_init();
    sub_40A400();
    obj_handle_field_lists_init();

    object.prototype_oid.type = OID_TYPE_BLOCKED;
    for (index = 0; index < 18; index++) {
        object.type = index;
        word_5D10FC = 0;
        dword_5D10F4 = 0;
        object_proto_enumerate_fields(&object, sub_40C560);
        object_fields_count_per_type[index] = word_5D10FC;
    }

    obj_initialized = true;

    return true;
}

// 0x4051F0
void obj_exit(void)
{
    obj_handle_field_lists_exit();
    obj_find_exit();
    obj_pool_exit();
    obj_data_exit();
    bitset_pool_exit();

    FREE(object_fields);
    FREE(dword_5D10F0);
    FREE(dword_5D1100);
    FREE(object_fields_count_per_type);

    obj_initialized = false;
}

// 0x405250
void obj_reset(void)
{
    obj_pool_exit();
    obj_data_exit();
    bitset_pool_exit();
    bitset_pool_init();
    obj_pool_init(sizeof(Object), obj_editor);
    obj_data_init();
}

// 0x405280
bool obj_validate_system(unsigned int flags)
{
    int64_t obj;
    int iter;
    Object* object;
    int obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int idx;

    if (obj_pool_walk_first(&obj, &iter)) {
        do {
            if (!obj_handle_is_valid(obj)) {
                tig_debug_printf("!VS  Walk found invalid handle: %" PRIx64 "\n", obj);
                return false;
            }

            object = obj_lock(obj);
            if (object->oid.type != OID_TYPE_BLOCKED
                && !objid_is_valid(object->oid)) {
                tig_debug_printf("!VS  Permanent id invalid: %" PRIx64 "\n", obj);
                obj_unlock(obj);
                return false;
            }
            obj_type = object->type;
            obj_unlock(obj);

            if (!obj_is_proto(obj)) {
                if (obj_field_int32_get(obj, OBJ_F_TYPE) != obj_type) {
                    tig_debug_println("!VS  Type get failed.");
                    return false;
                }

                switch (obj_type) {
                case OBJ_TYPE_WALL:
                case OBJ_TYPE_PORTAL:
                case OBJ_TYPE_SCENERY:
                case OBJ_TYPE_PROJECTILE:
                case OBJ_TYPE_WEAPON:
                case OBJ_TYPE_AMMO:
                case OBJ_TYPE_ARMOR:
                case OBJ_TYPE_GOLD:
                case OBJ_TYPE_FOOD:
                case OBJ_TYPE_SCROLL:
                case OBJ_TYPE_KEY:
                case OBJ_TYPE_KEY_RING:
                case OBJ_TYPE_WRITTEN:
                case OBJ_TYPE_GENERIC:
                case OBJ_TYPE_TRAP:
                    inventory_num_fld = -1;
                    inventory_list_fld = -1;
                    break;
                case OBJ_TYPE_CONTAINER:
                    inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
                    inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
                    break;
                case OBJ_TYPE_PC:
                case OBJ_TYPE_NPC:
                    inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
                    inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
                    break;
                default:
                    tig_debug_printf("!VS  Object type invalid: %d\n", obj_type);
                    return false;
                }

                if (inventory_num_fld != -1) {
                    cnt = obj_field_int32_get(obj, inventory_num_fld);
                    if (cnt != obj_arrayfield_length_get(obj, inventory_list_fld)) {
                        tig_debug_println("!VS  obj_f_critter_inventory_num doesn't match count for obj_f_critter_inventory_list_idx.");

                        object = obj_lock(obj);
                        obj_field_store(object, inventory_num_fld, &cnt);
                        obj_unlock(obj);
                    }

                    for (idx = 0; idx < cnt; idx++) {
                        bool is_handle = false;
                        bool is_id = false;
                        int64_t item_obj = OBJ_HANDLE_NULL;
                        ObjectID oid;
                        int item_type;

                        object = obj_lock(obj);
                        obj_arrayfield_fetch(object, inventory_list_fld, idx, &oid);
                        obj_unlock(obj);

                        if (oid.type != OID_TYPE_NULL) {
                            if (oid.type == OID_TYPE_HANDLE) {
                                item_obj = oid.d.h;
                                if (!obj_handle_is_valid(item_obj)) {
                                    tig_debug_printf("!VS  Inventory entry is an invalid handle.  handle: %" PRIx64 "\n", oid.d.h);
                                    return false;
                                }

                                is_handle = true;
                            } else {
                                if (!objid_is_valid(oid)) {
                                    // TODO: Unclear what is `a` and `b`.
                                    tig_debug_printf("!VS  Inventory entry is neither a valid handle nor valid id.  a: %" PRIx64 "  b: %" PRIx64 "\n",
                                        oid.d.h,
                                        oid.d.h);
                                }

                                is_id = true;
                            }

                            if ((flags & 0x1) != 0 && !is_handle) {
                                tig_debug_println("!VS  Inventory entry isn't a handle");
                                return false;
                            }

                            if (is_id) {
                                if (objid_is_equal(obj_get_id(obj), oid)) {
                                    tig_debug_println("!VS  Inventory entry has same id as ownder");
                                    return false;
                                }

                                item_obj = obj_pool_perm_lookup(oid);

                                // TODO: Looks wrong, validates `obj` instead of `item_obj`.
                                if (!obj_handle_is_valid(obj)) {
                                    tig_debug_printf("!VS  Inventory entry id resolved to invalid handle.  H: %" PRIx64 "\n", item_obj);
                                }
                            }

                            if (item_obj == obj) {
                                tig_debug_println("!VS  Inventory entry has samehandle as owner");
                                return false;
                            }

                            item_type = obj_field_int32_get(item_obj, OBJ_F_TYPE);
                            if (!obj_type_is_item(item_type)) {
                                tig_debug_printf("!VS  Inventory entry is not an item.  Type is: %d", item_type);
                                return false;
                            }
                        } else {
                            tig_debug_println("!VS  Inventory entry is null.");

                            object = obj_lock(obj);
                            obj_arrayfield_fetch(object, inventory_list_fld, cnt - 1, &oid);
                            obj_arrayfield_store(object, inventory_list_fld, idx, &oid);
                            sub_408E70(object, inventory_list_fld, cnt - 1);
                            cnt--;
                            idx--;
                            obj_field_store(object, inventory_num_fld, &cnt);
                            obj_unlock(obj);
                        }
                    }
                }
            }
        } while (obj_pool_walk_next(&obj, &iter));
    }

    return true;
}

// 0x405790
void sub_405790(int64_t obj)
{
    Object* object;

    object = obj_lock(obj);
    tig_debug_printf("{{ Difs on object w/ aid:");
    obj_debug_aid(obj_field_int32_get(obj, OBJ_F_AID));
    if (object->modified) {
        object_inst_enumerate_overridden_fields(object, sub_40D670);
        obj_unlock(obj);
        tig_debug_println(" }}");
    } else {
        // FIXME: Object not unlocked.
        tig_debug_println("No difs }}");
    }
}

// 0x405800
void obj_create_proto(int type, int64_t* obj_ptr)
{
    int64_t handle;
    Object* object;
    int index;

    object = obj_allocate(&handle);
    object->type = type;
    objid_create_guid(&(object->oid));
    obj_pool_perm_oid_set(object->oid, handle);
    object->prototype_oid.type = OID_TYPE_BLOCKED;
    object->prototype_obj = OBJ_HANDLE_NULL;
    object->field_40 = 0;
    sub_40C610(object);

    for (index = 0; index < 19; index++) {
        object->transient_properties[index] = -1;
    }

    object->num_fields = object_fields_count_per_type[type];
    object->data = (intptr_t*)CALLOC(object->num_fields, sizeof(*object->data));

    dword_5D10F4 = 0;
    object_proto_enumerate_fields(object, sub_40C6B0);

    obj_unlock(handle);

    obj_set_defaults(handle);
    sub_40C690(object);

    *obj_ptr = handle;
}

// 0x4058E0
void obj_create_inst(int64_t proto_obj, int64_t loc, int64_t* obj_ptr)
{
    Object* object;
    int64_t obj;
    Object* prototype;
    unsigned int flags;

    object = obj_allocate(&obj);
    object->prototype_obj = OBJ_HANDLE_NULL;

    prototype = obj_lock(proto_obj);
    object->prototype_oid = prototype->oid;
    object->type = prototype->type;
    object->field_40 = 0;
    obj_unlock(proto_obj);

    object->num_fields = 0;
    object->modified = false;
    sub_40C580(object);

    object->data = NULL;
    memset(object->transient_properties, 0, sizeof(object->transient_properties));

    if (object->type == OBJ_TYPE_PROJECTILE
        || object->type == OBJ_TYPE_CONTAINER
        || obj_type_is_critter(object->type)
        || obj_type_is_item(object->type)
        || !obj_editor) {
        objid_create_guid(&(object->oid));
        obj_pool_perm_oid_set(object->oid, obj);
    } else {
        object->oid.type = OID_TYPE_NULL;
    }

    obj_unlock(obj);

    obj_field_int64_set(obj, OBJ_F_LOCATION, loc);

    if (object->type == OBJ_TYPE_NPC) {
        obj_field_int64_set(obj, OBJ_F_CRITTER_TELEPORT_DEST, loc);
        obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_DAY, loc);
        obj_field_int64_set(obj, OBJ_F_NPC_STANDPOINT_NIGHT, loc);

        flags = obj_field_int32_get(obj, OBJ_F_NPC_FLAGS);
        flags |= ONF_WAYPOINTS_DAY;
        obj_field_int32_set(obj, OBJ_F_NPC_FLAGS, flags);
    }

    *obj_ptr = obj;

    obj_find_add(obj);
}

// 0x405B30
void obj_create_inst_with_oid(int64_t proto_obj, int64_t loc, ObjectID oid, int64_t* obj_ptr)
{
    Object* object;

    obj_create_inst(proto_obj, loc, obj_ptr);

    object = obj_lock(*obj_ptr);
    if (object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_remove(object->oid);
    }

    object->oid = oid;

    obj_pool_perm_oid_set(oid, *obj_ptr);
    obj_unlock(*obj_ptr);
}

// 0x405BC0
bool obj_is_proto(int64_t obj)
{
    Object* object;
    bool ret;

    object = obj_lock(obj);
    ret = object->prototype_oid.type == OID_TYPE_BLOCKED;
    obj_unlock(obj);

    return ret;
}

// 0x405BF0
void obj_deallocate(int64_t obj)
{
    Object* object;
    int fld;

    object = obj_lock(obj);
    if (object->prototype_oid.type != OID_TYPE_BLOCKED) {
        obj_find_remove(obj);
    }

    if (object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_remove(object->oid);
    }

    if (object->prototype_oid.type != OID_TYPE_BLOCKED) {
        object_inst_enumerate_overridden_fields(object, object_inst_field_dealloc);
        sub_40C5B0(object);

        for (fld = OBJ_F_TRANSIENT_BEGIN + 1; fld < OBJ_F_TRANSIENT_END; fld++) {
            object_transient_field_dealloc(object, fld);
        }
    } else {
        dword_5D10F4 = 0;
        object_proto_enumerate_fields(object, object_proto_field_dealloc);
        sub_40C640(object);
    }

    if (object->data != NULL) {
        FREE(object->data);
    }

    obj_unlock(obj);
    obj_pool_deallocate(obj);
}

// 0x405CC0
void sub_405CC0(int64_t obj)
{
    Object* object;
    int fld;

    object = obj_lock(obj);
    if (object->prototype_oid.type != OID_TYPE_BLOCKED) {
        object_inst_enumerate_overridden_fields(object, object_inst_field_dealloc);
        sub_40C5B0(object);

        for (fld = OBJ_F_TRANSIENT_BEGIN + 1; fld < OBJ_F_TRANSIENT_END; fld++) {
            object_transient_field_dealloc(object, fld);
        }
    } else {
        dword_5D10F4 = 0;
        object_proto_enumerate_fields(object, object_proto_field_dealloc);
        sub_40C640(object);
    }

    if (object->data != NULL) {
        FREE(object->data);
    }

    obj_unlock(obj);
    obj_pool_deallocate(obj);
}

// 0x405D60
void sub_405D60(int64_t* new_obj_ptr, int64_t obj)
{
    int64_t new_obj;
    Object* new_object;
    Object* object;
    int fld;

    object = obj_lock(obj);
    new_object = obj_allocate(&new_obj);

    new_object->type = object->type;
    new_object->oid = object->oid;
    new_object->prototype_oid = object->prototype_oid;
    new_object->prototype_obj = object->prototype_obj;
    new_object->field_40 = object->field_40;
    new_object->modified = object->modified;
    new_object->num_fields = object->num_fields;
    new_object->data = (intptr_t*)CALLOC(object->num_fields, sizeof(*new_object->data));

    if (object->prototype_oid.type != OID_TYPE_BLOCKED) {
        sub_40C5C0(new_object, object);

        dword_5D1108 = object;
        object_inst_enumerate_overridden_fields(new_object, object_inst_field_copy);

        memset(new_object->transient_properties, 0, sizeof(new_object->transient_properties));
        for (fld = OBJ_F_TRANSIENT_BEGIN + 1; fld < OBJ_F_TRANSIENT_END; fld++) {
            object_transient_field_copy(new_object, object, fld);
        }
    } else {
        sub_40C650(new_object, object);

        dword_5D10F4 = 0;
        dword_5D1110 = object;
        object_proto_enumerate_fields(new_object, object_proto_field_copy);
    }

    obj_unlock(obj);
    obj_unlock(new_obj);
    sub_464470(new_obj, NULL, NULL);

    *new_obj_ptr = new_obj;
}

// 0x406010
void obj_perm_dup(int64_t* copy_obj_ptr, int64_t existing_obj)
{
    int64_t copy_obj;
    Object* copy_object;
    Object* existing_object;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int64_t existing_item_obj;
    int64_t copy_item_obj;

    existing_object = obj_lock(existing_obj);

    copy_object = obj_allocate(&copy_obj);
    copy_object->type = existing_object->type;
    copy_object->oid = existing_object->oid;

    if (copy_object->oid.type != OID_TYPE_NULL) {
        switch (copy_object->oid.type) {
        case OID_TYPE_GUID:
        case OID_TYPE_P:
            objid_create_guid(&(copy_object->oid));
            obj_pool_perm_oid_set(copy_object->oid, copy_obj);
            break;
        default:
            tig_debug_println("ERROR: Unallowed ID type in obj_perm_dup");
            copy_object->oid.type = OID_TYPE_NULL;
            break;
        }
    }

    copy_object->prototype_oid = existing_object->prototype_oid;
    copy_object->prototype_obj = existing_object->prototype_obj;
    copy_object->field_40 = 0;
    copy_object->num_fields = existing_object->num_fields;
    copy_object->modified = false;
    copy_object->data = (intptr_t*)CALLOC(copy_object->num_fields, sizeof(*copy_object->data));
    sub_40C5C0(copy_object, existing_object);
    memset(copy_object->field_4C, 0, 4 * sub_40C030(copy_object->type));

    dword_5D1108 = existing_object;
    object_inst_enumerate_overridden_fields(copy_object, object_inst_field_copy);
    memset(copy_object->transient_properties, 0, sizeof(copy_object->transient_properties));

    obj_unlock(existing_obj);
    obj_unlock(copy_obj);

    if (copy_object->type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else if (obj_type_is_critter(copy_object->type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = -1;
        inventory_list_fld = -1;
    }

    if (inventory_num_fld != -1) {
        cnt = obj_field_int32_get(copy_obj, inventory_num_fld);
        while (cnt != 0) {
            cnt--;
            existing_item_obj = obj_arrayfield_handle_get(copy_obj, inventory_list_fld, cnt);
            obj_perm_dup(&copy_item_obj, existing_item_obj);
            obj_arrayfield_obj_set(copy_obj, inventory_list_fld, cnt, copy_item_obj);
            obj_field_handle_set(copy_item_obj, OBJ_F_ITEM_PARENT, copy_obj);
        }
    }

    *copy_obj_ptr = copy_obj;

    obj_find_add(copy_obj);
}

// 0x406210
void sub_406210(int64_t* copy, int64_t obj, ObjectID* oids)
{
    Object* object;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int idx;
    int64_t item_obj;

    obj_perm_dup(copy, obj);

    object = obj_lock(*copy);
    if (object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_remove(object->oid);
    }

    obj_pool_perm_oid_set(*oids++, *copy);
    obj_unlock(*copy);

    if (inventory_fields_from_obj_type(obj_field_int32_get(*copy, OBJ_F_TYPE), &inventory_num_fld, &inventory_list_fld)) {
        cnt = obj_field_int32_get(*copy, inventory_num_fld);
        for (idx = cnt - 1; idx >= 0; idx--) {
            item_obj = obj_arrayfield_handle_get(*copy, inventory_list_fld, idx);
            object = obj_lock(item_obj);
            if (object->oid.type != OID_TYPE_NULL) {
                obj_pool_perm_oid_remove(object->oid);
            }
            obj_pool_perm_oid_set(*oids++, item_obj);
            obj_unlock(item_obj);
        }
    }
}

// 0x4063A0
void sub_4063A0(int64_t obj, ObjectID** oids_ptr, int* cnt_ptr)
{
    ObjectID* oids;
    int cnt = 0;
    int idx;
    int inventory_num_fld;
    int inventory_list_fld;
    int64_t item_obj;

    if (inventory_fields_from_obj_type(obj_field_int32_get(obj, OBJ_F_TYPE), &inventory_num_fld, &inventory_list_fld)) {
        cnt = obj_field_int32_get(obj, inventory_num_fld);
    }

    oids = *oids_ptr = (ObjectID*)MALLOC(sizeof(*oids) * (cnt + 1));
    *oids++ = obj_get_id(obj);

    for (idx = cnt - 1; idx >= 0; idx--) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, idx);
        *oids++ = obj_get_id(item_obj);
    }

    *cnt_ptr = cnt + 1;
}

// 0x4064B0
void obj_save_preprocess(int64_t obj)
{
    Object* object;
    unsigned int flags;

    object = obj_lock(obj);
    obj_field_fetch(object, OBJ_F_INTERNAL_FLAGS, &flags);
    if ((flags & 0x1) == 0) {
        object_convert_scalar_handles_to_ids(object);
        object_convert_array_handles_to_ids(object);
        flags |= 0x1;
        obj_field_store(object, OBJ_F_INTERNAL_FLAGS, &flags);
        // NOTE: Probably should be outside of this condition block, otherwise
        // object might remain locked.
        obj_unlock(obj);
    }
}

// 0x406520
void obj_load_postprocess(int64_t obj)
{
    Object* object;
    unsigned int flags;

    object = obj_lock(obj);
    obj_field_fetch(object, OBJ_F_INTERNAL_FLAGS, &flags);
    if ((flags & 0x1) != 0) {
        object_convert_scalar_ids_to_handles(object);
        object_convert_array_ids_to_handles(object);
        flags &= ~0x1;
        obj_field_store(object, OBJ_F_INTERNAL_FLAGS, &flags);
        // NOTE: Probably should be outside of this condition block, otherwise
        // object might remain locked.
        obj_unlock(obj);
    }
}

// 0x406590
bool obj_write(TigFile* stream, int64_t obj)
{
    Object* object;
    bool is_proto;
    bool ret;

    object = obj_lock(obj);
    if (!obj_version_write_file(stream)) {
        // FIXME: Object not unlocked.
        return false;
    }

    is_proto = object->prototype_oid.type == OID_TYPE_BLOCKED;
    obj_unlock(obj);

    if (is_proto) {
        ret = obj_proto_write_file(stream, obj);
    } else {
        ret = obj_inst_write_file(stream, obj);
    }

    return ret;
}

// 0x406600
bool obj_read(TigFile* stream, int64_t* obj_handle_ptr)
{
    ObjectID oid;
    bool ret;

    if (!obj_version_read_file(stream)) {
        return false;
    }

    if (!objf_read(&oid, sizeof(ObjectID), stream)) {
        return false;
    }

    if (oid.type == OID_TYPE_BLOCKED) {
        ret = obj_proto_read_file(stream, obj_handle_ptr, oid);
    } else {
        ret = obj_inst_read_file(stream, obj_handle_ptr, oid);
    }

    return ret;
}

// 0x4066B0
void obj_write_mem(uint8_t** data_ptr, int* size_ptr, int64_t obj)
{
    Object* object;
    WriteBuffer wb;
    bool is_proto;

    object = obj_lock(obj);
    write_buffer_init(&wb);
    obj_version_write_mem(&wb);
    is_proto = object->prototype_oid.type == OID_TYPE_BLOCKED;
    obj_unlock(obj);

    if (is_proto) {
        obj_proto_write_mem(&wb, obj);
    } else {
        obj_inst_write_mem(&wb, obj);
    }

    *data_ptr = wb.base;
    *size_ptr = (int)(wb.curr - wb.base);
}

// 0x406730
bool obj_read_mem(uint8_t* data, int64_t* obj_ptr)
{
    ObjectID oid;
    bool ret;

    if (!obj_version_read_mem(&data)) {
        return false;
    }

    mem_read_advance(&oid, sizeof(oid), &data);
    data -= sizeof(oid);

    if (oid.type == OID_TYPE_BLOCKED) {
        ret = obj_proto_read_mem(data, obj_ptr);
    } else {
        ret = obj_inst_read_mem(data, obj_ptr);
    }

    return ret;
}

// 0x4067C0
int obj_is_modified(int64_t obj)
{
    Object* object;
    bool modified;

    object = obj_lock(obj);
    modified = object->modified;
    obj_unlock(obj);

    return modified;
}

// 0x4067F0
bool obj_dif_write(TigFile* stream, int64_t obj)
{
    Object* object;
    int marker;
    int index;
    int cnt;
    bool written;

    object = obj_lock(obj);
    if (object->modified == 0) {
        // FIXME: Object not unlocked.
        return false;
    }

    // FIXME: Unused.
    obj_field_int32_get(obj, OBJ_F_FLAGS);

    if (!obj_version_write_file(stream)) {
        obj_unlock(obj);
        return false;
    }

    marker = 0x12344321;
    if (!objf_write(&marker, sizeof(marker), stream)) {
        obj_unlock(obj);
        return false;
    }

    if (!objf_write(&(object->oid), sizeof(object->oid), stream)) {
        obj_unlock(obj);
        return false;
    }

    cnt = sub_40C030(object->type);
    for (index = 0; index < cnt; index++) {
        if (!objf_write(&(object->field_4C[index]), sizeof(object->field_4C[0]), stream)) {
            obj_unlock(obj);
            return false;
        }
    }

    dword_5D110C = stream;
    written = object_inst_enumerate_overridden_fields(object, object_field_write_if_dif);
    obj_unlock(obj);

    marker = 0x23455432;
    if (!objf_write(&marker, sizeof(marker), stream)) {
        return false;
    }

    return written;
}

// 0x406930
bool obj_dif_read(TigFile* stream, int64_t obj)
{
    Object* object;
    unsigned int marker;
    ObjectID oid;
    int cnt;
    int idx;

    if ((obj_field_int32_get(obj, OBJ_F_INTERNAL_FLAGS) & 0x1) == 0) {
        tig_debug_println("Error in obj_dif_read:\n  The object in memory that is having difs loaded is not currently storing ids.\n  We are in danger of mixing ids and handles.");
        return false;
    }

    object = obj_lock(obj);

    if (!obj_version_read_file(stream)) {
        tig_debug_println("Error in obj_dif_read:\n  Object version mismatch.");
        obj_unlock(obj);
        return false;
    }

    if (!objf_read(&marker, sizeof(marker), stream)) {
        tig_debug_println("Error in obj_dif_read:\n  Unable to read start marker.");
        obj_unlock(obj);
        return false;
    }

    if (marker != 0x12344321) {
        tig_debug_println("Error in obj_dif_read:\n  Start marker mismatch.");
        obj_unlock(obj);
        return false;
    }

    if (!objf_read(&oid, sizeof(oid), stream)) {
        tig_debug_println("Error in obj_dif_read:\n  Unable to read id.");
        obj_unlock(obj);
        return false;
    }

    if (object->oid.type != OID_TYPE_NULL) {
        if (!objid_is_equal(object->oid, oid)) {
            tig_debug_println("Error in obj_dif_read:\n  Object in memory already has an id, and dif would change the id.");
            obj_unlock(obj);
            return false;
        }
    } else {
        object->oid = oid;

        if (oid.type > 0 && oid.type < 4) {
            obj_pool_perm_oid_set(oid, obj);
        }
    }

    cnt = sub_40C030(object->type);
    for (idx = 0; idx < cnt; idx++) {
        if (!objf_read(&(object->field_4C[idx]), 4, stream)) {
            tig_debug_println("Error in obj_dif_read:\n  Unable to read one of the modified flag vars.");
            obj_unlock(obj);
            return false;
        }
    }

    object->modified = true;

    dword_5D110C = stream;
    if (!object_inst_enumerate_available_fields(object, object_field_read_if_dif)) {
        tig_debug_println("Error in obj_dif_read:\n  Unable to read one of the fields.");
        obj_unlock(obj);
        return false;
    }

    obj_unlock(obj);

    if (!objf_read(&marker, sizeof(marker), stream)) {
        tig_debug_println("Error in obj_dif_read:\n  Unable to read the end marker");
        return false;
    }

    if (marker != 0x23455432) {
        tig_debug_printf("Error in obj_dif_read:\n  End marker mismatch.  File position after read is %X.\n", tig_file_ftell(stream));
        return false;
    }

    obj_find_move(obj);

    return true;
}

// 0x406B80
void sub_406B80(int64_t obj)
{
    Object* object;
    int cnt;
    int index;

    object = obj_lock(obj);
    if (object->modified) {
        cnt = sub_40C030(object->type);
        for (index = 0; index < cnt; index++) {
            object->field_4C[index] = 0;
        }
        object->modified = false;
    }
}

// 0x406CA0
int obj_field_int32_get(int64_t obj, int fld)
{
    Object* object;
    int value;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    if (fld == OBJ_F_TYPE) {
        // FIXME: Object not unlocked.
        return object->type;
    }

    obj_field_fetch(object, fld, &value);
    obj_unlock(obj);

    return value;
}

// 0x406D10
void object_field_not_exists(Object* object, int fld)
{
    tig_debug_printf("Error: Accessing non-existant field [%s : %d] in object type [%d].\n",
        object_field_names[fld],
        fld,
        object->type);
}

// 0x406D40
void obj_field_int32_set(int64_t obj, int fld, int value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_field_store(object, fld, &value);
    obj_unlock(obj);
}

// 0x406DA0
int64_t obj_field_int64_get(int64_t obj, int fld)
{
    Object* object;
    int64_t value;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    obj_field_fetch(object, fld, &value);
    obj_unlock(obj);

    return value;
}

// 0x406E10
void obj_field_int64_set(int64_t obj, int fld, int64_t value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_field_store(object, fld, &value);
    obj_unlock(obj);

    if (fld == OBJ_F_LOCATION) {
        obj_find_move(obj);
    }
}

// 0x406E80
int64_t obj_field_handle_get(int64_t obj, int fld)
{
    Object* object;
    ObjectID oid;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return OBJ_HANDLE_NULL;
    }

    if (fld == OBJ_F_PROTOTYPE_HANDLE) {
        // FIXME: Object not unlocked.
        return obj_get_prototype_handle(object);
    }

    obj_field_fetch(object, fld, &oid);
    obj_unlock(obj);

    if (oid.type == OID_TYPE_NULL || oid.type != OID_TYPE_HANDLE) {
        return OBJ_HANDLE_NULL;
    }

    return oid.d.h;
}

// 0x406F20
void obj_field_handle_set(int64_t obj, int fld, int64_t value)
{
    Object* object;
    ObjectID oid;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    if (value != OBJ_HANDLE_NULL) {
        oid.type = OID_TYPE_HANDLE;
        oid.d.h = value;
    } else {
        oid.type = OID_TYPE_NULL;
    }

    obj_field_store(object, fld, &oid);
    obj_unlock(obj);
}

// 0x406FB0
bool obj_field_obj_get(int64_t obj, int fld, int64_t* value_ptr)
{
    Object* object;
    ObjectID oid;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        *value_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    if (fld == OBJ_F_PROTOTYPE_HANDLE) {
        *value_ptr = obj_get_prototype_handle(object);
        if (!obj_handle_is_valid(*value_ptr)) {
            *value_ptr = OBJ_HANDLE_NULL;
            return false; // FIXME: Object not unlocked.
        }

        return true; // FIXME: Object not unlocked.
    }

    obj_field_fetch(object, fld, &oid);

    if (oid.type == OID_TYPE_NULL) {
        obj_unlock(obj);
        *value_ptr = OBJ_HANDLE_NULL;
        return true;
    }

    if (oid.type == OID_TYPE_HANDLE) {
        if (!obj_handle_is_valid(oid.d.h)) {
            oid.type = OID_TYPE_NULL;
            obj_field_store(object, fld, &oid);
            obj_unlock(obj);
            *value_ptr = OBJ_HANDLE_NULL;
            return false;
        }

        obj_unlock(obj);
        *value_ptr = oid.d.h;
        return true;
    }

    *value_ptr = OBJ_HANDLE_NULL;
    return false;
}

// 0x407100
ObjectID sub_407100(int64_t obj, int fld)
{
    Object* object;
    ObjectID oid;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        oid.type = OID_TYPE_BLOCKED;
        return oid;
    }

    if (fld == OBJ_F_PROTOTYPE_HANDLE) {
        // FIXME: Object not unlocked.
        return object->prototype_oid;
    }

    obj_field_fetch(object, fld, &oid);
    obj_unlock(obj);

    return oid;
}

// 0x4071A0
void obj_field_string_get(int64_t obj, int fld, char** value_ptr)
{
    Object* object;
    int name_num;
    const char* name_str;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        *value_ptr = NULL;
        return;
    }

    if (fld == OBJ_F_NAME) {
        obj_field_fetch(object, OBJ_F_NAME, &name_num);
        name_str = o_name_get(name_num);
        *value_ptr = (char*)MALLOC(strlen(name_str) + 1);
        strcpy(*value_ptr, name_str);
    } else {
        obj_field_fetch(object, fld, value_ptr);
    }

    obj_unlock(obj);
}

// 0x407270
void obj_field_string_set(int64_t obj, int fld, const char* value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_field_store(object, fld, &value);
    obj_unlock(obj);
}

// 0x4072D0
int obj_arrayfield_int32_get(int64_t obj, int fld, int index)
{
    Object* object;
    int value;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    obj_arrayfield_fetch(object, fld, index, &value);
    obj_unlock(obj);

    return value;
}

// 0x407340
void obj_arrayfield_int32_set(int64_t obj, int fld, int index, int value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_store(object, fld, index, &value);
    obj_unlock(obj);
}

// 0x407470
unsigned int obj_arrayfield_uint32_get(int64_t obj, int fld, int index)
{
    Object* object;
    int value;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    obj_arrayfield_fetch(object, fld, index, &value);
    obj_unlock(obj);

    return value;
}

// 0x4074E0
void obj_arrayfield_uint32_set(int64_t obj, int fld, int index, unsigned int value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_store(object, fld, index, &value);
    obj_unlock(obj);
}

// 0x407540
int64_t obj_arrayfield_int64_get(int64_t obj, int fld, int index)
{
    Object* object;
    int64_t value;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    obj_arrayfield_fetch(object, fld, index, &value);
    obj_unlock(obj);

    return value;
}

// 0x4075B0
void obj_arrayfield_int64_set(int64_t obj, int fld, int index, int64_t value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_store(object, fld, index, &value);
    obj_unlock(obj);
}

// 0x407610
int64_t obj_arrayfield_handle_get(int64_t obj, int fld, int index)
{
    Object* object;
    ObjectID oid;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return OBJ_HANDLE_NULL;
    }

    obj_arrayfield_fetch(object, fld, index, &oid);
    obj_unlock(obj);

    if (oid.type == OID_TYPE_NULL || oid.type != OID_TYPE_HANDLE) {
        return OBJ_HANDLE_NULL;
    }

    return oid.d.h;
}

// 0x4076A0
bool obj_arrayfield_obj_get(int64_t obj, int fld, int index, int64_t* value_ptr)
{
    Object* object;
    ObjectID oid;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        *value_ptr = OBJ_HANDLE_NULL;
        return false;
    }

    obj_arrayfield_fetch(object, fld, index, &oid);

    if (oid.type == OID_TYPE_NULL) {
        obj_unlock(obj);
        *value_ptr = OBJ_HANDLE_NULL;
        return true;
    }

    if (oid.type == OID_TYPE_HANDLE) {
        if (!obj_handle_is_valid(oid.d.h)) {
            oid.type = OID_TYPE_NULL;
            obj_arrayfield_store(object, fld, index, &oid);
            obj_unlock(obj);
            *value_ptr = OBJ_HANDLE_NULL;
            return false;
        }

        obj_unlock(obj);
        *value_ptr = oid.d.h;
        return true;
    }

    *value_ptr = OBJ_HANDLE_NULL;
    return false;
}

// 0x4077B0
void obj_arrayfield_obj_set(int64_t obj, int fld, int index, int64_t value)
{
    Object* object;
    ObjectID oid;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    if (value != OBJ_HANDLE_NULL) {
        oid.type = OID_TYPE_HANDLE;
        oid.d.h = value;
    } else {
        oid.type = OID_TYPE_NULL;
    }

    obj_arrayfield_store(object, fld, index, &oid);
    obj_unlock(obj);
}

// 0x407840
void obj_arrayfield_script_get(int64_t obj, int fld, int index, void* value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_fetch(object, fld, index, value);
    obj_unlock(obj);
}

// 0x4078A0
void obj_arrayfield_script_set(int64_t obj, int fld, int index, void* value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_store(object, fld, index, value);
    obj_unlock(obj);
}

// 0x407900
void obj_arrayfield_pc_quest_get(int64_t obj, int fld, int index, void* value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_fetch(object, fld, index, value);
    obj_unlock(obj);
}

// 0x407960
void obj_arrayfield_pc_quest_set(int64_t obj, int fld, int index, void* value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_store(object, fld, index, value);
    obj_unlock(obj);
}

// 0x4079C0
int obj_arrayfield_length_get(int64_t obj, int fld)
{
    Object* object;
    int length;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    sub_408D60(object, fld, &length);
    obj_unlock(obj);

    return length;
}

// 0x407A20
void obj_arrayfield_length_set(int64_t obj, int fld, int length)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    sub_408E70(object, fld, length);
    obj_unlock(obj);
}

// 0x407BA0
void obj_arrayfield_pc_rumor_copy_to_flat(int64_t obj, int fld, int cnt, void* data)
{
    Object* object;
    SizeableArray** sa_ptr;
    bool prototype_locked;
    int64_t prototype_obj;

    object = obj_lock(obj);
    if (object_field_valid(object->type, fld)) {
        prototype_locked = sub_408F40(object, fld, &sa_ptr, &prototype_obj);
        sa_array_copy_to_flat(data, sa_ptr, cnt, sizeof(uint64_t));
        if (prototype_locked) {
            obj_unlock(prototype_obj);
        }
    } else {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
    }
}

// 0x407C30
void obj_arrayfield_pc_quest_copy_to_flat(int64_t obj, int fld, int cnt, void* data)
{
    Object* object;
    SizeableArray** sa_ptr;
    bool prototype_locked;
    int64_t prototype_obj;

    object = obj_lock(obj);
    if (object_field_valid(object->type, fld)) {
        prototype_locked = sub_408F40(object, fld, &sa_ptr, &prototype_obj);
        sa_array_copy_to_flat(data, sa_ptr, cnt, sizeof(PcQuestState));
        if (prototype_locked) {
            obj_unlock(prototype_obj);
        }
    } else {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
    }
}

// 0x407D50
void obj_field_reset(int64_t obj, int fld)
{
    Object* object;

    object = obj_lock(obj);
    if (object_field_valid(object->type, fld)) {
        if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
            object_proto_field_dealloc(object, fld);
            sub_40D400(object, fld, true);
            obj_unlock(obj);
        } else if (fld > OBJ_F_TRANSIENT_BEGIN) {
            object_transient_field_dealloc(object, fld);
            obj_unlock(obj);
        } else {
            if (sub_40D320(object, fld)) {
                object_inst_field_dealloc(object, sub_40D230(object, fld), &(object_fields[fld]));
                sub_40D400(object, fld, true);
            } else {
                sub_40D450(object, fld);
                sub_40D370(object, fld, true);
                sub_40D400(object, fld, true);
            }
            object->modified = true;
            obj_unlock(obj);
        }
    } else {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
    }
}

// 0x407EF0
ObjectID obj_get_id(int64_t obj)
{
    ObjectID oid;
    Object* object;

    if (obj_handle_is_valid(obj)) {
        object = obj_lock(obj);
        if (object->oid.type == OID_TYPE_NULL) {
            if (!obj_editor
                && sub_43D940(obj)
                && object_is_static(obj)) {
                objid_id_perm_by_load_order(&(object->oid), obj);
            } else {
                objid_create_guid(&(object->oid));
            }

            obj_pool_perm_oid_set(object->oid, obj);
            object->modified = true;
        }

        oid = object->oid;
        obj_unlock(obj);

        if (!objid_is_valid(oid)) {
            tig_debug_println("! obj_get_id found invalid id");
            tig_debug_printf("! -- object with bad id has base aid of: ");
            obj_debug_aid(obj_field_int32_get(obj, OBJ_F_AID));
            oid.type = OID_TYPE_NULL;
        }
    } else {
        oid = obj_pool_perm_reverse_lookup(obj);
    }

    return oid;
}

// 0x408430
void obj_debug_aid(tig_art_id_t aid)
{
    int type;
    int palette;
    int item_type;
    int item_subtype;
    int item_damage;
    int item_destroyed;
    int item_num;
    int item_disposition;

    if (aid == TIG_ART_ID_INVALID) {
        tig_debug_println("Empty Art ID");
        return;
    }

    type = tig_art_type(aid);

    palette = tig_art_id_palette_get(aid);
    tig_debug_printf("Palette: %d  ", palette);

    switch (type) {
    case TIG_ART_TYPE_TILE:
        tig_debug_printf("List: Tile  ");
        break;
    case TIG_ART_TYPE_WALL:
        tig_debug_printf("List: Wall  ");
        break;
    case TIG_ART_TYPE_CRITTER:
        tig_debug_printf("List: Critter  ");
        break;
    case TIG_ART_TYPE_PORTAL:
        tig_debug_printf("List: Portal  ");
        break;
    case TIG_ART_TYPE_SCENERY:
        tig_debug_printf("List: Scenery  ");
        break;
    case TIG_ART_TYPE_INTERFACE:
        tig_debug_printf("List: Interface  ");
        break;
    case TIG_ART_TYPE_ITEM:
        tig_debug_printf("List: Item  ");
        item_type = tig_art_item_id_type_get(aid);
        item_subtype = tig_art_item_id_subtype_get(aid);
        item_damage = tig_art_id_damaged_get(aid);
        item_destroyed = tig_art_item_id_destroyed_get(aid);
        item_num = tig_art_num_get(aid);
        item_disposition = tig_art_item_id_disposition_get(aid);
        tig_debug_printf("Num: %d  Image: %d  Damage: %d  Destroyed: %d  ",
            item_num,
            item_disposition,
            item_damage,
            item_destroyed);
        switch (item_type) {
        case TIG_ART_ITEM_TYPE_WEAPON:
            tig_debug_printf("Type: Weapon  ");
            switch (item_subtype) {
            case TIG_ART_WEAPON_TYPE_NO_WEAPON:
                tig_debug_printf("Subtype: No Weapon  ");
                break;
            case TIG_ART_WEAPON_TYPE_UNARMED:
                tig_debug_printf("Subtype: Unarmed  ");
                break;
            case TIG_ART_WEAPON_TYPE_DAGGER:
                tig_debug_printf("Subtype: Dagger  ");
                break;
            case TIG_ART_WEAPON_TYPE_SWORD:
                tig_debug_printf("Subtype: Sword  ");
                break;
            case TIG_ART_WEAPON_TYPE_AXE:
                tig_debug_printf("Subtype: Axe  ");
                break;
            case TIG_ART_WEAPON_TYPE_MACE:
                tig_debug_printf("Subtype: Mace  ");
                break;
            case TIG_ART_WEAPON_TYPE_PISTOL:
                tig_debug_printf("Subtype: Pistol  ");
                break;
            case TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD:
                tig_debug_printf("Subtype: Two Handed Sword  ");
                break;
            case TIG_ART_WEAPON_TYPE_BOW:
                tig_debug_printf("Subtype: Bow  ");
                break;
            case TIG_ART_WEAPON_TYPE_9:
                tig_debug_printf("Subtype: Unused Weapon  ");
                break;
            case TIG_ART_WEAPON_TYPE_RIFLE:
                tig_debug_printf("Subtype: Hi Tech Gun  ");
                break;
            case TIG_ART_WEAPON_TYPE_STAFF:
                tig_debug_printf("Subtype: Staff  ");
                break;
            default:
                tig_debug_printf("Subtype: *Invalid*  ");
                break;
            }
            break;
        case TIG_ART_ITEM_TYPE_AMMO:
            tig_debug_printf("Type: Ammo  ");
            break;
        case TIG_ART_ITEM_TYPE_ARMOR:
            tig_debug_printf("Type: Armor  ");
            break;
        case TIG_ART_ITEM_TYPE_GOLD:
            tig_debug_printf("Type: Gold  ");
            break;
        case TIG_ART_ITEM_TYPE_FOOD:
            tig_debug_printf("Type: Food  ");
            break;
        case TIG_ART_ITEM_TYPE_SCROLL:
            tig_debug_printf("Type: Scroll  ");
            break;
        case TIG_ART_ITEM_TYPE_KEY:
            tig_debug_printf("Type: Key  ");
            break;
        case TIG_ART_ITEM_TYPE_KEY_RING:
            tig_debug_printf("Type: Key Ring  ");
            break;
        case TIG_ART_ITEM_TYPE_WRITTEN:
            tig_debug_printf("Type: Written  ");
            break;
        case TIG_ART_ITEM_TYPE_GENERIC:
            tig_debug_printf("Type: Generic  ");
            break;
        default:
            tig_debug_printf("Type: *Invalid*  ");
            break;
        }
        break;
    case TIG_ART_TYPE_CONTAINER:
        tig_debug_printf("List: Container  ");
        break;
    case TIG_ART_TYPE_MISC:
        tig_debug_printf("List: Internal  ");
        break;
    case TIG_ART_TYPE_LIGHT:
        tig_debug_printf("List: Light  ");
        break;
    case TIG_ART_TYPE_ROOF:
        tig_debug_printf("List: Roof  ");
        break;
    case TIG_ART_TYPE_FACADE:
        tig_debug_printf("List: Facade  ");
        break;
    case TIG_ART_TYPE_MONSTER:
        tig_debug_printf("List: Monster  ");
        break;
    case TIG_ART_TYPE_UNIQUE_NPC:
        tig_debug_printf("List: NPC  ");
        break;
    // FIXME: Missing TIG_ART_TYPE_EYE_CANDY.
    default:
        tig_debug_printf("List: *Invalid*  ");
        break;
    }

    tig_debug_printf("\n");
}

// 0x408710
Object* obj_allocate(int64_t* obj_ptr)
{
    return obj_pool_allocate(obj_ptr);
}

// 0x408020
ObjectID sub_408020(int64_t obj, int a2)
{
    Object* object;

    object = obj_lock(obj);
    if (objid_is_valid(object->oid) && object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_remove(object->oid);
    }

    object->oid = sub_4E6540(a2);
    obj_pool_perm_oid_set(object->oid, obj);

    obj_unlock(obj);

    return object->oid;
}

// 0x4082C0
bool obj_inst_first(int64_t* obj_ptr, int* iter_ptr)
{
    int64_t obj;
    int iter;
    Object* object;
    bool is_proto;

    if (obj_pool_walk_first(&obj, &iter)) {
        do {
            object = obj_lock(obj);
            is_proto = object->prototype_oid.type == OID_TYPE_BLOCKED;
            obj_unlock(obj);

            if (!is_proto) {
                *obj_ptr = obj;
                *iter_ptr = iter;
                return true;
            }
        } while (obj_pool_walk_next(&obj, &iter));
    }

    return false;
}

// 0x408390
bool obj_inst_next(int64_t* obj_ptr, int* iter_ptr)
{
    int64_t obj;
    int iter;
    Object* object;
    bool is_proto;

    iter = *iter_ptr;
    while (obj_pool_walk_next(&obj, &iter)) {
        object = obj_lock(obj);
        is_proto = object->prototype_oid.type == OID_TYPE_BLOCKED;
        obj_unlock(obj);

        if (!is_proto) {
            *obj_ptr = obj;
            *iter_ptr = iter;
            return true;
        }
    }

    return false;
}

// 0x408720
Object* obj_lock(int64_t obj)
{
    return obj_pool_lock(obj);
}

// 0x408740
void obj_unlock(int64_t obj)
{
    obj_pool_unlock(obj);
}

// 0x408760
void obj_field_store(Object* object, int fld, void* value_ptr)
{
    ObjDataDescriptor store_op;
    int storage_idx;

    if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
        storage_idx = sub_40CB40(object, fld);
        store_op.ptr = &(object->data[storage_idx]);
        sub_40D400(object, fld, true);
    } else if (fld > OBJ_F_TRANSIENT_BEGIN && fld < OBJ_F_TRANSIENT_END) {
        storage_idx = fld - OBJ_F_TRANSIENT_BEGIN - 1;
        store_op.ptr = &(object->transient_properties[storage_idx]);
    } else {
        if (sub_40D320(object, fld)) {
            storage_idx = sub_40D230(object, fld);
            store_op.ptr = &(object->data[storage_idx]);
            sub_40D400(object, fld, true);
        } else {
            sub_40D450(object, fld);
            sub_40D370(object, fld, true);
            storage_idx = sub_40D230(object, fld);
            store_op.ptr = &(object->data[storage_idx]);
            sub_40D400(object, fld, true);
        }
        object->modified = true;
    }

    store_op.type = object_fields[fld].type;

    switch (store_op.type) {
    case OD_TYPE_INT32:
        store_op.storage.value = *(int*)value_ptr;
        break;
    case OD_TYPE_INT64:
        store_op.storage.value64 = *(int64_t*)value_ptr;
        break;
    case OD_TYPE_STRING:
        store_op.storage.str = *(char**)value_ptr;
        break;
    case OD_TYPE_HANDLE:
        store_op.storage.oid = *(ObjectID*)value_ptr;
        break;
    case OD_TYPE_PTR:
        store_op.storage.ptr = *(intptr_t*)value_ptr;
        break;
    }

    obj_data_store(&store_op);
}

// 0x4088B0
void obj_arrayfield_store(Object* object, int fld, int index, void* value_ptr)
{
    ObjDataDescriptor store_op;
    int storage_idx;

    if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
        storage_idx = sub_40CB40(object, fld);
        store_op.ptr = &(object->data[storage_idx]);
        sub_40D400(object, fld, true);
    } else if (fld > OBJ_F_TRANSIENT_BEGIN && fld < OBJ_F_TRANSIENT_END) {
        storage_idx = fld - OBJ_F_TRANSIENT_BEGIN - 1;
        store_op.ptr = &(object->transient_properties[storage_idx]);
    } else {
        if (!sub_40D350(object, fld)) {
            if (!sub_40D320(object, fld)) {
                sub_40D2A0(object, fld);
            }
        }
        storage_idx = sub_40D230(object, fld);
        store_op.ptr = &(object->data[storage_idx]);
        sub_40D400(object, fld, true);
        object->modified = true;
    }

    store_op.idx = index;
    store_op.type = object_fields[fld].type;

    switch (store_op.type) {
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
        store_op.storage.value = *(int*)value_ptr;
        break;
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
        store_op.storage.value64 = *(uint64_t*)value_ptr;
        break;
    case OD_TYPE_SCRIPT_ARRAY:
        store_op.storage.scr = *(Script*)value_ptr;
        break;
    case OD_TYPE_QUEST_ARRAY:
        store_op.storage.quest = *(PcQuestState*)value_ptr;
        break;
    case OD_TYPE_HANDLE_ARRAY:
        store_op.storage.oid = *(ObjectID*)value_ptr;
        break;
    case OD_TYPE_PTR_ARRAY:
        store_op.storage.ptr = *(intptr_t*)value_ptr;
        break;
    }

    obj_data_store(&store_op);
}

// 0x408A20
void obj_field_fetch(Object* object, int fld, void* value_ptr)
{
    ObjDataDescriptor fetch_op;
    int storage_idx;

    if (fld == OBJ_F_TYPE) {
        *(int*)value_ptr = object->type;
        return;
    }

    fetch_op.type = object_fields[fld].type;
    if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
        storage_idx = sub_40CB40(object, fld);
        fetch_op.ptr = &(object->data[storage_idx]);
        obj_data_fetch(&fetch_op);
    } else if (fld > OBJ_F_TRANSIENT_BEGIN && fld < OBJ_F_TRANSIENT_END) {
        fetch_op.ptr = &(object->transient_properties[fld - OBJ_F_TRANSIENT_BEGIN - 1]);
        obj_data_fetch(&fetch_op);
    } else if (sub_40D320(object, fld)) {
        storage_idx = sub_40D230(object, fld);
        fetch_op.ptr = &(object->data[storage_idx]);
        obj_data_fetch(&fetch_op);
    } else {
        int64_t proto_handle;
        Object* proto;

        proto_handle = obj_get_prototype_handle(object);
        proto = obj_lock(proto_handle);
        storage_idx = sub_40CB40(proto, fld);
        fetch_op.ptr = &(proto->data[storage_idx]);
        obj_data_fetch(&fetch_op);
        obj_unlock(proto_handle);
    }

    switch (fetch_op.type) {
    case OD_TYPE_INT32:
        *(int*)value_ptr = fetch_op.storage.value;
        break;
    case OD_TYPE_INT64:
        *(int64_t*)value_ptr = fetch_op.storage.value64;
        break;
    case OD_TYPE_STRING:
        *(char**)value_ptr = fetch_op.storage.str;
        break;
    case OD_TYPE_HANDLE:
        *(ObjectID*)value_ptr = fetch_op.storage.oid;
        break;
    case OD_TYPE_PTR:
        *(intptr_t*)value_ptr = fetch_op.storage.ptr;
        break;
    }
}

// 0x408BB0
void obj_arrayfield_fetch(Object* object, int fld, int index, void* value)
{
    ObjDataDescriptor fetch_op;
    int storage_idx;

    fetch_op.idx = index;
    fetch_op.type = object_fields[fld].type;

    if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
        storage_idx = sub_40CB40(object, fld);
        fetch_op.ptr = &(object->data[storage_idx]);
        obj_data_fetch(&fetch_op);
    } else if (fld > OBJ_F_TRANSIENT_BEGIN && fld < OBJ_F_TRANSIENT_END) {
        storage_idx = fld - OBJ_F_TRANSIENT_BEGIN - 1;
        fetch_op.ptr = &(object->transient_properties[storage_idx]);
        obj_data_fetch(&fetch_op);
    } else if (sub_40D350(object, fld)) {
        storage_idx = sub_40D230(object, fld);
        fetch_op.ptr = &(object->data[storage_idx]);
        obj_data_fetch(&fetch_op);
    } else {
        int64_t proto_handle;
        Object* proto;

        proto_handle = obj_get_prototype_handle(object);
        proto = obj_lock(proto_handle);
        storage_idx = sub_40CB40(proto, fld);
        fetch_op.ptr = &(proto->data[storage_idx]);
        obj_data_fetch(&fetch_op);
        obj_unlock(proto_handle);
    }

    switch (fetch_op.type) {
    case OD_TYPE_INT32_ARRAY:
    case OD_TYPE_UINT32_ARRAY:
        *(int*)value = fetch_op.storage.value;
        break;
    case OD_TYPE_INT64_ARRAY:
    case OD_TYPE_UINT64_ARRAY:
        *(int64_t*)value = fetch_op.storage.value64;
        break;
    case OD_TYPE_SCRIPT_ARRAY:
        *(Script*)value = fetch_op.storage.scr;
        break;
    case OD_TYPE_QUEST_ARRAY:
        *(PcQuestState*)value = fetch_op.storage.quest;
        break;
    case OD_TYPE_HANDLE_ARRAY:
        *(ObjectID*)value = fetch_op.storage.oid;
        break;
    case OD_TYPE_PTR_ARRAY:
        *(intptr_t*)value = fetch_op.storage.ptr;
        break;
    }
}

// 0x408D60
void sub_408D60(Object* object, int fld, int* value_ptr)
{
    ObjDataDescriptor v1;
    int64_t proto_handle;
    Object* proto;

    v1.type = object_fields[fld].type;

    if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
        v1.ptr = &(object->data[sub_40CB40(object, fld)]);
        *value_ptr = obj_data_count_idx(&v1);
    } else {
        if (fld > OBJ_F_TRANSIENT_BEGIN && fld < OBJ_F_TRANSIENT_END) {
            v1.ptr = &(object->transient_properties[fld - OBJ_F_TRANSIENT_BEGIN - 1]);
            *value_ptr = obj_data_count_idx(&v1);
        }

        if (sub_40D320(object, fld)) {
            v1.ptr = &(object->data[sub_40D230(object, fld)]);
            *value_ptr = obj_data_count_idx(&v1);
        } else {
            proto_handle = obj_get_prototype_handle(object);
            proto = obj_lock(proto_handle);
            v1.ptr = &(proto->data[sub_40CB40(proto, fld)]);
            *value_ptr = obj_data_count_idx(&v1);
            obj_unlock(proto_handle);
        }
    }
}

// 0x408E70
void sub_408E70(Object* object, int fld, int value)
{
    ObjDataDescriptor v1;

    if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
        v1.ptr = &(object->data[sub_40CB40(object, fld)]);
        sub_40D400(object, fld, true);
    } else if (fld > OBJ_F_TRANSIENT_BEGIN && fld < OBJ_F_TRANSIENT_END) {
        v1.ptr = &(object->transient_properties[fld - OBJ_F_TRANSIENT_BEGIN - 1]);
    } else {
        if (!sub_40D350(object, fld)) {
            if (!sub_40D320(object, fld)) {
                sub_40D2A0(object, fld);
            }
        }

        v1.ptr = &(object->data[sub_40D230(object, fld)]);
        sub_40D400(object, fld, true);
        object->modified = true;
    }

    v1.type = object_fields[fld].type;
    v1.idx = value;
    obj_data_remove_idx(&v1);
}

// 0x408F40
bool sub_408F40(Object* object, int fld, SizeableArray*** ptr, int64_t* proto_handle_ptr)
{
    Object* proto;

    if (object->prototype_oid.type == OID_TYPE_BLOCKED) {
        *ptr = (SizeableArray**)(&(object->data[sub_40CB40(object, fld)]));
        return false;
    }

    if (fld > OBJ_F_TRANSIENT_BEGIN && fld <= OBJ_F_TRANSIENT_END) {
        *ptr = (SizeableArray**)(&(object->transient_properties[fld - OBJ_F_TRANSIENT_BEGIN - 1]));
        return false;
    }

    if (sub_40D320(object, fld)) {
        *ptr = (SizeableArray**)(&(object->data[sub_40D230(object, fld)]));
        return false;
    }

    *proto_handle_ptr = obj_get_prototype_handle(object);

    proto = obj_lock(*proto_handle_ptr);
    *ptr = (SizeableArray**)(&(proto->data[sub_40CB40(proto, fld)]));

    return true;
}

// 0x409000
void obj_set_defaults(int64_t obj)
{
    int index;
    int type;
    unsigned int flags;
    tig_art_id_t art_id = TIG_ART_ID_INVALID;

    obj_field_int32_set(obj, OBJ_F_SHADOW, -1);
    obj_field_int32_set(obj, OBJ_F_AID, TIG_ART_ID_INVALID);
    obj_field_int32_set(obj, OBJ_F_DESTROYED_AID, TIG_ART_ID_INVALID);
    obj_field_int32_set(obj, OBJ_F_CURRENT_AID, TIG_ART_ID_INVALID);
    obj_field_int32_set(obj, OBJ_F_BLIT_COLOR, tig_color_make(255, 255, 255));
    obj_field_int32_set(obj, OBJ_F_BLIT_ALPHA, 255);
    obj_field_int32_set(obj, OBJ_F_BLIT_SCALE, 100);
    obj_field_int32_set(obj, OBJ_F_LIGHT_AID, TIG_ART_ID_INVALID);
    obj_field_int32_set(obj, OBJ_F_LIGHT_COLOR, tig_color_make(255, 255, 255));

    for (index = 0; index < 7; index++) {
        obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_FORE, index, TIG_ART_ID_INVALID);
        obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_BACK, index, TIG_ART_ID_INVALID);
    }

    for (index = 0; index < 4; index++) {
        obj_arrayfield_uint32_set(obj, OBJ_F_OVERLAY_LIGHT_AID, index, TIG_ART_ID_INVALID);
    }

    for (index = 0; index < 4; index++) {
        obj_arrayfield_uint32_set(obj, OBJ_F_UNDERLAY, index, TIG_ART_ID_INVALID);
    }

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    switch (type) {
    case OBJ_TYPE_WEAPON:
    case OBJ_TYPE_AMMO:
    case OBJ_TYPE_ARMOR:
    case OBJ_TYPE_GOLD:
    case OBJ_TYPE_FOOD:
    case OBJ_TYPE_SCROLL:
    case OBJ_TYPE_KEY:
    case OBJ_TYPE_KEY_RING:
    case OBJ_TYPE_WRITTEN:
    case OBJ_TYPE_GENERIC:
        if (type == OBJ_TYPE_KEY) {
            obj_field_int32_set(obj, OBJ_F_ITEM_WEIGHT, 0);
        } else if (type == OBJ_TYPE_GOLD) {
            obj_field_int32_set(obj, OBJ_F_ITEM_WEIGHT, 1);
        } else {
            obj_field_int32_set(obj, OBJ_F_ITEM_WEIGHT, 10);
        }

        obj_field_int32_set(obj, OBJ_F_ITEM_USE_AID_FRAGMENT, TIG_ART_ID_INVALID);
        obj_field_int32_set(obj, OBJ_F_ITEM_SPELL_1, 10000);
        obj_field_int32_set(obj, OBJ_F_ITEM_SPELL_2, 10000);
        obj_field_int32_set(obj, OBJ_F_ITEM_SPELL_3, 10000);
        obj_field_int32_set(obj, OBJ_F_ITEM_SPELL_4, 10000);
        obj_field_int32_set(obj, OBJ_F_ITEM_SPELL_5, 10000);

        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_NO_BLOCK | OF_SHOOT_THROUGH | OF_SEE_THROUGH | OF_FLAT;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
        break;
    case OBJ_TYPE_PC:
    case OBJ_TYPE_NPC:
        stat_set_defaults(obj);
        skill_set_defaults(obj);
        spell_set_defaults(obj);
        tech_set_defaults(obj);

        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_PROVIDES_COVER | OF_SHOOT_THROUGH | OF_SEE_THROUGH;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
        break;
    }

    switch (type) {
    case OBJ_TYPE_WALL:
        obj_field_int32_set(obj, OBJ_F_HP_PTS, 500);

        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_PROVIDES_COVER;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

        tig_art_wall_id_create(0, 0, 0, 6, 0, 0, &art_id);
        break;
    case OBJ_TYPE_PORTAL:
        obj_field_int32_set(obj, OBJ_F_HP_PTS, 100);

        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_PROVIDES_COVER;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

        tig_art_portal_id_create(0, 1, 0, 0, 6, 0, &art_id);
        break;
    case OBJ_TYPE_CONTAINER:
        obj_field_int32_set(obj, OBJ_F_HP_PTS, 100);

        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_PROVIDES_COVER | OF_SHOOT_THROUGH | OF_SEE_THROUGH;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

        tig_art_container_id_create(0, 1, 0, 0, 0, &art_id);
        break;
    case OBJ_TYPE_SCENERY:
        obj_field_int32_set(obj, OBJ_F_HP_PTS, 100);

        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_PROVIDES_COVER | OF_CLICK_THROUGH | OF_SHOOT_THROUGH | OF_SEE_THROUGH;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

        tig_art_scenery_id_create(0, 0, 0, 0, 0, &art_id);
        break;
    case OBJ_TYPE_PROJECTILE:
        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_NO_BLOCK | OF_SHOOT_THROUGH | OF_SEE_THROUGH;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

        tig_art_scenery_id_create(0, 0, 0, 0, 0, &art_id);
        break;
    case OBJ_TYPE_WEAPON:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_WEAPON);
        obj_field_int32_set(obj, OBJ_F_WEAPON_AMMO_TYPE, 10000);
        obj_field_int32_set(obj, OBJ_F_WEAPON_SPEED_FACTOR, 10);
        obj_arrayfield_int32_set(obj, OBJ_F_WEAPON_DAMAGE_LOWER_IDX, 0, 1);
        obj_arrayfield_int32_set(obj, OBJ_F_WEAPON_DAMAGE_UPPER_IDX, 0, 4);
        tig_art_item_id_create(0, 1, 0, 0, 3, 0, 0, 0, &art_id);
        obj_field_int32_set(obj, OBJ_F_ITEM_INV_AID, art_id);
        tig_art_item_id_create(0, 2, 0, 0, 3, 0, 0, 0, &art_id);
        obj_field_int32_set(obj, OBJ_F_WEAPON_PAPER_DOLL_AID, art_id);
        tig_art_item_id_create(0, 0, 0, 0, 3, 0, 0, 0, &art_id);
        obj_field_int32_set(obj, OBJ_F_ITEM_USE_AID_FRAGMENT, art_id);
        obj_field_int32_set(obj, OBJ_F_WEAPON_MISSILE_AID, TIG_ART_ID_INVALID);
        obj_field_int32_set(obj, OBJ_F_WEAPON_VISUAL_EFFECT_AID, TIG_ART_ID_INVALID);
        break;
    case OBJ_TYPE_AMMO:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_AMMO);
        tig_art_item_id_create(0, 0, 0, 0, 0, 1, 0, 0, &art_id);
        break;
    case OBJ_TYPE_ARMOR:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_ARMOR);
        tig_art_item_id_create(0, 1, 0, 0, 4, 2, 0, 0, &art_id);
        obj_field_int32_set(obj, OBJ_F_ITEM_INV_AID, art_id);
        tig_art_item_id_create(0, 2, 0, 0, 4, 2, 0, 0, &art_id);
        obj_field_int32_set(obj, OBJ_F_ARMOR_PAPER_DOLL_AID, art_id);
        tig_art_item_id_create(0, 0, 0, 0, 4, 2, 0, 0, &art_id);
        obj_field_int32_set(obj, OBJ_F_ITEM_USE_AID_FRAGMENT, art_id);
        obj_field_int32_set(obj, OBJ_F_ARMOR_FLAGS, OARF_SIZE_MEDIUM);
        break;
    case OBJ_TYPE_GOLD:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_GOLD);
        obj_field_int32_set(obj, OBJ_F_GOLD_QUANTITY, 1);
        tig_art_item_id_create(0, 0, 0, 0, 0, 3, 0, 0, &art_id);
        break;
    case OBJ_TYPE_FOOD:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_FOOD);
        tig_art_item_id_create(0, 0, 0, 0, 0, 4, 0, 0, &art_id);
        break;
    case OBJ_TYPE_SCROLL:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_SCROLL);
        tig_art_item_id_create(0, 0, 0, 0, 0, 5, 0, 0, &art_id);
        break;
    case OBJ_TYPE_KEY:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_KEY);
        tig_art_item_id_create(0, 0, 0, 0, 0, 6, 0, 0, &art_id);
        break;
    case OBJ_TYPE_KEY_RING:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_KEY_RING);
        tig_art_item_id_create(0, 0, 0, 0, 0, 7, 0, 0, &art_id);
        break;
    case OBJ_TYPE_WRITTEN:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_WRITTEN);
        tig_art_item_id_create(0, 0, 0, 0, 0, 8, 0, 0, &art_id);
        break;
    case OBJ_TYPE_GENERIC:
        set_item_defaults(obj, TIG_ART_ITEM_TYPE_GENERIC);
        tig_art_item_id_create(0, 0, 0, 0, 0, 9, 0, 0, &art_id);
        break;
    case OBJ_TYPE_PC:
    case OBJ_TYPE_NPC:
        tig_art_critter_id_create(1, 0, 0, 0, 0, 4, 0, 0, 0, &art_id);
        break;
    case OBJ_TYPE_TRAP:
        obj_field_int32_set(obj, OBJ_F_HP_PTS, 100);

        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags |= OF_DONTLIGHT | OF_NO_BLOCK | OF_SHOOT_THROUGH | OF_SEE_THROUGH | OF_FLAT;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

        tig_art_scenery_id_create(0, 0, 0, 0, 0, &art_id);
        break;
    }

    obj_field_int32_set(obj, OBJ_F_AID, art_id);
    obj_field_int32_set(obj, OBJ_F_CURRENT_AID, art_id);
}

// 0x409640
void set_item_defaults(int64_t obj, int subtype)
{
    tig_art_id_t art_id;

    if (tig_art_item_id_create(0, 1, 0, 0, 0, subtype, 0, 0, &art_id) != TIG_OK) {
        art_id = TIG_ART_ID_INVALID;
    }

    obj_field_int32_set(obj, OBJ_F_ITEM_INV_AID, art_id);
    obj_field_int32_set(obj, OBJ_F_ITEM_WORTH, obj_item_defaults[subtype].worth);
    obj_field_int32_set(obj, OBJ_F_HP_PTS, obj_item_defaults[subtype].hp);
}

// 0x4096B0
bool obj_proto_write_file(TigFile* stream, int64_t obj)
{
    Object* object;
    int cnt;

    object = obj_lock(obj);

    if (!objf_write(&(object->prototype_oid), sizeof(object->prototype_oid), stream)) {
        obj_unlock(obj);
        return false;
    }

    if (!objf_write(&(object->oid), sizeof(object->oid), stream)) {
        obj_unlock(obj);
        return false;
    }

    if (!objf_write(&(object->type), sizeof(object->type), stream)) {
        obj_unlock(obj);
        return false;
    }

    cnt = sub_40C030(object->type);
    if (!objf_write(object->field_4C, sizeof(object->field_4C[0]) * cnt, stream)) {
        obj_unlock(obj);
        return false;
    }

    dword_5D10F4 = 0;
    dword_5D110C = stream;
    if (!object_proto_enumerate_fields(object, obj_proto_field_write_file)) {
        obj_unlock(obj);
        return false;
    }

    obj_unlock(obj);
    return true;
}

// 0x4097B0
bool obj_proto_read_file(TigFile* stream, int64_t* obj_ptr, ObjectID oid)
{
    int64_t obj;
    Object* object;
    int size;

    object = obj_allocate(&obj);
    object->prototype_oid = oid;
    object->prototype_obj = OBJ_HANDLE_NULL;

    if (!objf_read(&(object->oid), sizeof(object->oid), stream)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    if (!objf_read(&(object->type), sizeof(object->type), stream)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    size = 4 * sub_40C030(object->type);
    object->field_4C = MALLOC(size);
    if (!objf_read(object->field_4C, size, stream)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    object->field_40 = 0;
    object->modified = false;
    object->num_fields = object_fields_count_per_type[object->type];
    object->data = (intptr_t*)CALLOC(object->num_fields, sizeof(*object->data));

    dword_5D10F4 = 0;
    dword_5D110C = stream;
    if (!object_proto_enumerate_fields(object, obj_proto_field_read_file)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    memset(object->transient_properties, -1, sizeof(object->transient_properties));

    if (object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_set(object->oid, obj);
    }

    obj_unlock(obj);
    *obj_ptr = obj;

    return true;
}

// 0x409980
bool obj_inst_write_file(TigFile* stream, int64_t obj)
{
    Object* object;
    int cnt;

    object = obj_lock(obj);

    if (!objf_write(&(object->prototype_oid), sizeof(object->prototype_oid), stream)) {
        obj_unlock(obj);
        return false;
    }

    if (!objf_write(&(object->oid), sizeof(object->oid), stream)) {
        obj_unlock(obj);
        return false;
    }

    if (!objf_write(&(object->type), sizeof(object->type), stream)) {
        obj_unlock(obj);
        return false;
    }

    if (!objf_write(&(object->num_fields), sizeof(object->num_fields), stream)) {
        obj_unlock(obj);
        return false;
    }

    cnt = sub_40C030(object->type);
    if (!objf_write(object->field_48, sizeof(object->field_48[0]) * cnt, stream)) {
        obj_unlock(obj);
        return false;
    }

    dword_5D110C = stream;
    if (!object_inst_enumerate_overridden_fields(object, object_field_write)) {
        obj_unlock(obj);
        return false;
    }

    obj_unlock(obj);
    return true;
}

// 0x409AA0
bool obj_inst_read_file(TigFile* stream, int64_t* obj_ptr, ObjectID oid)
{
    int64_t obj;
    Object* object;

    object = obj_allocate(&obj);
    object->prototype_oid = oid;
    object->prototype_obj = OBJ_HANDLE_NULL;

    if (!objf_read(&(object->oid), sizeof(object->oid), stream)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    if (!objf_read(&(object->type), sizeof(object->type), stream)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    if (!objf_read(&(object->num_fields), sizeof(object->num_fields), stream)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    object->field_40 = 0;
    object->modified = false;
    sub_40C580(object);

    object->data = (intptr_t*)CALLOC(object->num_fields, sizeof(*object->data));

    if (!objf_read(object->field_48, sizeof(*object->field_48) * sub_40C030(object->type), stream)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    dword_5D110C = stream;
    if (!object_inst_enumerate_overridden_fields(object, object_field_read)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    memset(object->transient_properties, 0, sizeof(object->transient_properties));

    if (object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_set(object->oid, obj);
    }

    obj_unlock(obj);

    obj_field_int32_set(obj, OBJ_F_INTERNAL_FLAGS, 0x1);

    *obj_ptr = obj;
    obj_find_add(obj);

    return true;
}

// 0x409CB0
void obj_proto_write_mem(WriteBuffer* wb, int64_t obj)
{
    Object* object;
    int cnt;

    object = obj_lock(obj);
    write_buffer_append(&(object->prototype_oid), sizeof(object->prototype_oid), wb);
    write_buffer_append(&(object->oid), sizeof(object->oid), wb);
    write_buffer_append(&(object->type), sizeof(object->type), wb);
    cnt = sub_40C030(object->type);
    write_buffer_append(object->field_4C, sizeof(object->field_4C[0]) * cnt, wb);
    dword_5D10F4 = 0;
    dword_5D1118 = wb;
    object_proto_enumerate_fields(object, obj_proto_field_write_mem);
    obj_unlock(obj);
}

// 0x409D30
bool obj_proto_read_mem(uint8_t* data, int64_t* obj_ptr)
{
    int64_t obj;
    Object* object;
    int size;

    object = obj_allocate(&obj);
    mem_read_advance(&(object->prototype_oid), sizeof(object->prototype_oid), &data);
    object->prototype_obj = OBJ_HANDLE_NULL;
    mem_read_advance(&(object->oid), sizeof(object->oid), &data);
    mem_read_advance(&(object->type), sizeof(object->type), &data);

    size = sizeof(*object->field_4C) * sub_40C030(object->type);
    object->field_4C = MALLOC(size);
    mem_read_advance(object->field_4C, size, &data);

    object->field_40 = 0;
    object->modified = false;
    object->num_fields = object_fields_count_per_type[object->type];
    object->data = (intptr_t*)CALLOC(object->num_fields, sizeof(*object->data));

    dword_5D10F4 = 0;
    dword_5D111C = data;
    if (!object_proto_enumerate_fields(object, obj_proto_field_read_mem)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    memset(object->transient_properties, -1, sizeof(object->transient_properties));

    if (object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_set(object->oid, obj);
    }

    obj_unlock(obj);
    *obj_ptr = obj;

    return true;
}

// 0x409E80
void obj_inst_write_mem(WriteBuffer* wb, int64_t obj)
{
    Object* object;
    int cnt;

    object = obj_lock(obj);
    write_buffer_append(&(object->prototype_oid), sizeof(object->prototype_oid), wb);
    write_buffer_append(&(object->oid), sizeof(object->oid), wb);
    write_buffer_append(&(object->type), sizeof(object->type), wb);
    write_buffer_append(&(object->num_fields), sizeof(object->num_fields), wb);
    cnt = sub_40C030(object->type);
    write_buffer_append(object->field_48, sizeof(object->field_48[0]) * cnt, wb);
    dword_5D1118 = wb;

    object_inst_enumerate_overridden_fields(object, obj_inst_field_write_mem);
    obj_unlock(obj);
}

// 0x409F10
bool obj_inst_read_mem(uint8_t* data, int64_t* obj_ptr)
{
    Object* object;
    int64_t obj;

    object = obj_allocate(&obj);
    mem_read_advance(&(object->prototype_oid), sizeof(object->prototype_oid), &data);
    object->prototype_obj = OBJ_HANDLE_NULL;
    mem_read_advance(&(object->oid), sizeof(object->oid), &data);
    mem_read_advance(&(object->type), sizeof(object->type), &data);
    mem_read_advance(&(object->num_fields), sizeof(object->num_fields), &data);

    object->field_40 = 0;
    object->modified = false;
    sub_40C580(object);

    object->data = (intptr_t*)CALLOC(object->num_fields, sizeof(*object->data));
    mem_read_advance(object->field_48, 4 * sub_40C030(object->type), &data);

    dword_5D111C = data;
    if (!object_inst_enumerate_overridden_fields(object, obj_inst_field_read_mem)) {
        obj_unlock(obj);
        obj_pool_deallocate(obj);
        return false;
    }

    memset(object->transient_properties, 0, sizeof(object->transient_properties));

    if (object->oid.type != OID_TYPE_NULL) {
        obj_pool_perm_oid_set(object->oid, obj);
    }

    obj_unlock(obj);

    obj_field_int32_set(obj, OBJ_F_INTERNAL_FLAGS, 0x1);

    *obj_ptr = obj;
    obj_find_add(obj);

    return true;
}

// 0x40A070
bool obj_proto_field_write_file(Object* object, int fld)
{
    ObjDataDescriptor v1;

    v1.type = object_fields[fld].type;
    v1.ptr = &(object->data[dword_5D10F4]);
    if (!obj_data_write_file(&v1, dword_5D110C)) {
        return false;
    }

    dword_5D10F4++;

    return true;
}

// 0x40A0E0
bool obj_proto_field_write_mem(Object* object, int fld)
{
    ObjDataDescriptor v1;

    v1.type = object_fields[fld].type;
    v1.ptr = &(object->data[dword_5D10F4]);
    obj_data_write_mem(&v1, dword_5D1118);
    dword_5D10F4++;

    return true;
}

// 0x40A140
bool obj_proto_field_read_file(Object* object, int fld)
{
    ObjDataDescriptor v1;

    v1.type = object_fields[fld].type;
    v1.ptr = &(object->data[dword_5D10F4]);
    if (!obj_data_read_file_slow(&v1, dword_5D110C)) {
        return false;
    }

    dword_5D10F4++;

    return true;
}

// 0x40A1B0
bool obj_proto_field_read_mem(Object* object, int fld)
{
    ObjDataDescriptor v1;

    v1.type = object_fields[fld].type;
    v1.ptr = &(object->data[dword_5D10F4]);
    obj_data_read_mem(&v1, &dword_5D111C);
    dword_5D10F4++;

    return true;
}

// 0x40A210
bool object_field_write(Object* object, int idx, ObjectFieldInfo* info)
{
    ObjDataDescriptor v1;

    v1.type = info->type;
    v1.ptr = &(object->data[idx]);
    if (!obj_data_write_file(&v1, dword_5D110C)) {
        return false;
    }

    return true;
}

// 0x40A250
bool obj_inst_field_write_mem(Object* object, int idx, ObjectFieldInfo* info)
{
    ObjDataDescriptor v1;

    v1.type = info->type;
    v1.ptr = &(object->data[idx]);
    obj_data_write_mem(&v1, dword_5D1118);

    return true;
}

// 0x40A290
bool object_field_read(Object* object, int idx, ObjectFieldInfo* info)
{
    ObjDataDescriptor v1;

    v1.type = info->type;
    v1.ptr = &(object->data[idx]);
    if (!obj_data_read_file_fast(&v1, dword_5D110C)) {
        return false;
    }

    return true;
}

// 0x40A2D0
bool obj_inst_field_read_mem(Object* object, int idx, ObjectFieldInfo* info)
{
    ObjDataDescriptor v1;

    v1.type = info->type;
    v1.ptr = &(object->data[idx]);
    obj_data_read_mem(&v1, &dword_5D111C);

    return true;
}

// 0x40A310
bool object_field_write_if_dif(Object* object, int idx, ObjectFieldInfo* info)
{
    if ((object->field_4C[info->change_array_idx] & info->mask) == 0) {
        return true;
    }

    if (!object_field_write(object, idx, info)) {
        tig_debug_printf("object_field_write failed in object_field_write_if_dif trying to write field #%d\n", info - object_fields);
        return false;
    }

    return true;
}

// 0x40A370
bool object_field_read_if_dif(Object* object, int idx, ObjectFieldInfo* info)
{
    if ((object->field_4C[info->change_array_idx] & info->mask) == 0) {
        return true;
    }

    if ((object->field_48[info->change_array_idx] & info->mask) == 0) {
        sub_40D470(object, idx);
        sub_40D3A0(object, info, true);
    }

    if (!object_field_read(object, idx, info)) {
        tig_debug_printf("object_field_read failed in object_field_read_if_dif trying to read field #%d\n", info - object_fields);
        return false;
    }

    return true;
}

// 0x40A400
void sub_40A400(void)
{
    int fld;
    ObjectFieldInfo* info;
    int v1;
    int v2;
    int v3;
    int idx;
    int v5;
    int v6;
    int v7;
    int v8;
    int v9;
    int v10;

    sub_40A8A0();
    sub_40A7B0();

    for (fld = OBJ_F_BEGIN; fld < OBJ_F_TOTAL_NORMAL; fld++) {
        info = &(object_fields[fld]);
        if (info->type == OD_TYPE_BEGIN) {
            sub_40B8E0(fld);
            info->complex_array_idx = -1;
            info->change_array_idx = -1;
            info->mask = 0;
            info->bit = 0;
            info->simple_array_idx = -1;

            switch (fld) {
            case OBJ_F_BEGIN:
                v1 = 0;
                v2 = 0;
                break;
            case OBJ_F_WALL_BEGIN:
            case OBJ_F_PORTAL_BEGIN:
            case OBJ_F_CONTAINER_BEGIN:
            case OBJ_F_SCENERY_BEGIN:
            case OBJ_F_PROJECTILE_BEGIN:
            case OBJ_F_ITEM_BEGIN:
            case OBJ_F_CRITTER_BEGIN:
            case OBJ_F_TRAP_BEGIN:
                v1 = v5;
                v2 = v6;
                break;
            case OBJ_F_WEAPON_BEGIN:
            case OBJ_F_AMMO_BEGIN:
            case OBJ_F_ARMOR_BEGIN:
            case OBJ_F_GOLD_BEGIN:
            case OBJ_F_FOOD_BEGIN:
            case OBJ_F_SCROLL_BEGIN:
            case OBJ_F_KEY_BEGIN:
            case OBJ_F_KEY_RING_BEGIN:
            case OBJ_F_WRITTEN_BEGIN:
            case OBJ_F_GENERIC_BEGIN:
                v1 = v7;
                v2 = v8;
                break;
            case OBJ_F_PC_BEGIN:
            case OBJ_F_NPC_BEGIN:
                v1 = v9;
                v2 = v10;
                break;
            }

            dword_5D1100[sub_40A790(fld)] = v2;
        } else if (info->type == OD_TYPE_END) {
            info->complex_array_idx = -1;
            info->change_array_idx = -1;
            info->mask = 0;
            info->bit = 0;
            info->simple_array_idx = -1;

            switch (fld) {
            case OBJ_F_END:
                v5 = v1;
                v6 = v2;
                break;
            case OBJ_F_ITEM_END:
                v7 = v1;
                v8 = v2;
                break;
            case OBJ_F_CRITTER_END:
                v9 = v1;
                v10 = v2;
                break;
            }
        } else {
            sub_40A740(fld, &v3, &idx);
            info->simple_array_idx = v2++;
            info->change_array_idx = idx / 32 + dword_5D10F0[sub_40A790(v3)];
            info->bit = idx % 32;
            info->mask = 1u << info->bit;

            switch (info->type) {
            case OD_TYPE_INT32:
            case OD_TYPE_INT64:
            case OD_TYPE_STRING:
            case OD_TYPE_HANDLE:
                info->complex_array_idx = -1;
                break;
            case OD_TYPE_INT32_ARRAY:
            case OD_TYPE_INT64_ARRAY:
            case OD_TYPE_UINT32_ARRAY:
            case OD_TYPE_UINT64_ARRAY:
            case OD_TYPE_SCRIPT_ARRAY:
            case OD_TYPE_QUEST_ARRAY:
            case OD_TYPE_HANDLE_ARRAY:
                info->complex_array_idx = v1++;
                break;
            }
        }
    }

    for (fld = OBJ_F_TOTAL_NORMAL; fld < OBJ_F_MAX; fld++) {
        info = &(object_fields[fld]);
        info->complex_array_idx = -1;
        info->change_array_idx = -1;
        info->mask = 0;
        info->bit = 0;
    }
}

// 0x40A740
void sub_40A740(int fld, int* start_ptr, int* length_ptr)
{
    int index;

    for (index = 20; index >= 0; index--) {
        if (dword_59BE00[index] < fld) {
            break;
        }
    }

    *start_ptr = dword_59BE00[index];
    *length_ptr = 0;
    if (*start_ptr + 1 < fld) {
        index = fld - (*start_ptr + 1);
        while (index != 0) {
            (*length_ptr)++;
            index--;
        }
    }
}

// 0x40A790
int sub_40A790(int a1)
{
    int index;

    for (index = 0; index < 21; index++) {
        if (dword_59BE00[index] == a1) {
            return index;
        }
    }
    return 0;
}

// 0x40A7B0
void sub_40A7B0(void)
{
    int fld;

    for (fld = OBJ_F_BEGIN; fld <= OBJ_F_TOTAL_NORMAL; fld++) {
        object_fields[fld].cnt = object_fields[fld].type > OD_TYPE_INVALID && object_fields[fld].type <= OD_TYPE_END ? 0 : 1;
    }

    for (fld = OBJ_F_TOTAL_NORMAL; fld < OBJ_F_MAX; fld++) {
        object_fields[fld].cnt = 0;
    }

    object_fields[OBJ_F_RESISTANCE_IDX].cnt = 5;
    object_fields[OBJ_F_WEAPON_DAMAGE_LOWER_IDX].cnt = 5;
    object_fields[OBJ_F_WEAPON_DAMAGE_UPPER_IDX].cnt = 5;
    object_fields[OBJ_F_WEAPON_MAGIC_DAMAGE_ADJ_IDX].cnt = 5;
    object_fields[OBJ_F_ARMOR_RESISTANCE_ADJ_IDX].cnt = 5;
    object_fields[OBJ_F_ARMOR_MAGIC_RESISTANCE_ADJ_IDX].cnt = 5;
    object_fields[OBJ_F_CRITTER_STAT_BASE_IDX].cnt = STAT_COUNT;
    object_fields[OBJ_F_CRITTER_BASIC_SKILL_IDX].cnt = BASIC_SKILL_COUNT;
    object_fields[OBJ_F_CRITTER_TECH_SKILL_IDX].cnt = TECH_SKILL_COUNT;
    object_fields[OBJ_F_RENDER_ALPHA].cnt = 4;
    object_fields[OBJ_F_SHADOW_HANDLES].cnt = 5;
}

// 0x40A8A0
void sub_40A8A0(void)
{
    object_fields[OBJ_F_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_CURRENT_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_LOCATION].type = OD_TYPE_INT64;
    object_fields[OBJ_F_OFFSET_X].type = OD_TYPE_INT32;
    object_fields[OBJ_F_OFFSET_Y].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SHADOW].type = OD_TYPE_INT32;
    object_fields[OBJ_F_OVERLAY_FORE].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_OVERLAY_BACK].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_UNDERLAY].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_BLIT_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_BLIT_COLOR].type = OD_TYPE_INT32;
    object_fields[OBJ_F_BLIT_ALPHA].type = OD_TYPE_INT32;
    object_fields[OBJ_F_BLIT_SCALE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_LIGHT_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_LIGHT_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_LIGHT_COLOR].type = OD_TYPE_INT32;
    object_fields[OBJ_F_OVERLAY_LIGHT_FLAGS].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_OVERLAY_LIGHT_AID].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_OVERLAY_LIGHT_COLOR].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SPELL_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_BLOCKING_MASK].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NAME].type = OD_TYPE_INT32;
    object_fields[OBJ_F_DESCRIPTION].type = OD_TYPE_INT32;
    object_fields[OBJ_F_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_DESTROYED_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_AC].type = OD_TYPE_INT32;
    object_fields[OBJ_F_HP_PTS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_HP_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_MATERIAL].type = OD_TYPE_INT32;
    object_fields[OBJ_F_HP_DAMAGE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RESISTANCE_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_SCRIPTS_IDX].type = OD_TYPE_SCRIPT_ARRAY;
    object_fields[OBJ_F_SOUND_EFFECT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CATEGORY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_END].type = OD_TYPE_END;
    object_fields[OBJ_F_WALL_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_WALL_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WALL_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WALL_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WALL_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_WALL_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_WALL_END].type = OD_TYPE_END;
    object_fields[OBJ_F_PORTAL_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_PORTAL_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PORTAL_LOCK_DIFFICULTY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PORTAL_KEY_ID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PORTAL_NOTIFY_NPC].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PORTAL_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PORTAL_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PORTAL_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PORTAL_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_PORTAL_END].type = OD_TYPE_END;
    object_fields[OBJ_F_CONTAINER_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_CONTAINER_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_LOCK_DIFFICULTY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_KEY_ID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_INVENTORY_NUM].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_INVENTORY_LIST_IDX].type = OD_TYPE_HANDLE_ARRAY;
    object_fields[OBJ_F_CONTAINER_INVENTORY_SOURCE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_NOTIFY_NPC].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CONTAINER_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_CONTAINER_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_CONTAINER_END].type = OD_TYPE_END;
    object_fields[OBJ_F_SCENERY_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_SCENERY_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SCENERY_WHOS_IN_ME].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_SCENERY_RESPAWN_DELAY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SCENERY_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SCENERY_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_SCENERY_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_SCENERY_END].type = OD_TYPE_END;
    object_fields[OBJ_F_PROJECTILE_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_PROJECTILE_FLAGS_COMBAT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PROJECTILE_FLAGS_COMBAT_DAMAGE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PROJECTILE_HIT_LOC].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PROJECTILE_PARENT_WEAPON].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_PROJECTILE_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PROJECTILE_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PROJECTILE_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PROJECTILE_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_PROJECTILE_END].type = OD_TYPE_END;
    object_fields[OBJ_F_ITEM_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_ITEM_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_PARENT].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_ITEM_WEIGHT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_MAGIC_WEIGHT_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_WORTH].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_MANA_STORE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_INV_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_INV_LOCATION].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_USE_AID_FRAGMENT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_DISCIPLINE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_DESCRIPTION_UNKNOWN].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_DESCRIPTION_EFFECTS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_SPELL_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_SPELL_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_SPELL_3].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_SPELL_4].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_SPELL_5].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_SPELL_MANA_STORE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_AI_ACTION].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ITEM_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_ITEM_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_ITEM_END].type = OD_TYPE_END;
    object_fields[OBJ_F_WEAPON_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_WEAPON_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_PAPER_DOLL_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_BONUS_TO_HIT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_HIT_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_DAMAGE_LOWER_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_WEAPON_DAMAGE_UPPER_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_WEAPON_MAGIC_DAMAGE_ADJ_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_WEAPON_SPEED_FACTOR].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_SPEED_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_RANGE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_RANGE_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MIN_STRENGTH].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_MIN_STRENGTH_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_AMMO_TYPE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_AMMO_CONSUMPTION].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MISSILE_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_VISUAL_EFFECT_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_CRIT_HIT_CHART].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_CRIT_HIT_CHANCE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_CRIT_HIT_EFFECT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_CRIT_MISS_CHART].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_CRIT_MISS_CHANCE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_MAGIC_CRIT_MISS_EFFECT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WEAPON_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_WEAPON_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_WEAPON_END].type = OD_TYPE_END;
    object_fields[OBJ_F_AMMO_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_AMMO_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_AMMO_QUANTITY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_AMMO_TYPE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_AMMO_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_AMMO_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_AMMO_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_AMMO_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_AMMO_END].type = OD_TYPE_END;
    object_fields[OBJ_F_ARMOR_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_ARMOR_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_PAPER_DOLL_AID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_AC_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_MAGIC_AC_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_RESISTANCE_ADJ_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_ARMOR_MAGIC_RESISTANCE_ADJ_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_ARMOR_SILENT_MOVE_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_MAGIC_SILENT_MOVE_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_UNARMED_BONUS_DAMAGE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_ARMOR_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_ARMOR_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_ARMOR_END].type = OD_TYPE_END;
    object_fields[OBJ_F_GOLD_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_GOLD_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_GOLD_QUANTITY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_GOLD_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_GOLD_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_GOLD_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_GOLD_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_GOLD_END].type = OD_TYPE_END;
    object_fields[OBJ_F_FOOD_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_FOOD_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_FOOD_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_FOOD_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_FOOD_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_FOOD_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_FOOD_END].type = OD_TYPE_END;
    object_fields[OBJ_F_SCROLL_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_SCROLL_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SCROLL_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SCROLL_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_SCROLL_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_SCROLL_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_SCROLL_END].type = OD_TYPE_END;
    object_fields[OBJ_F_KEY_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_KEY_KEY_ID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_KEY_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_KEY_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_KEY_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_KEY_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_KEY_END].type = OD_TYPE_END;
    object_fields[OBJ_F_KEY_RING_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_KEY_RING_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_KEY_RING_LIST_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_KEY_RING_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_KEY_RING_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_KEY_RING_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_KEY_RING_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_KEY_RING_END].type = OD_TYPE_END;
    object_fields[OBJ_F_WRITTEN_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_WRITTEN_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WRITTEN_SUBTYPE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WRITTEN_TEXT_START_LINE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WRITTEN_TEXT_END_LINE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WRITTEN_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WRITTEN_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_WRITTEN_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_WRITTEN_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_WRITTEN_END].type = OD_TYPE_END;
    object_fields[OBJ_F_GENERIC_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_GENERIC_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_GENERIC_USAGE_BONUS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_GENERIC_USAGE_COUNT_REMAINING].type = OD_TYPE_INT32;
    object_fields[OBJ_F_GENERIC_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_GENERIC_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_GENERIC_END].type = OD_TYPE_END;
    object_fields[OBJ_F_CRITTER_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_CRITTER_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_FLAGS2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_STAT_BASE_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_CRITTER_BASIC_SKILL_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_CRITTER_TECH_SKILL_IDX].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_CRITTER_SPELL_TECH_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_CRITTER_FATIGUE_PTS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_FATIGUE_ADJ].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_FATIGUE_DAMAGE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_CRIT_HIT_CHART].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_EFFECTS_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_CRITTER_EFFECT_CAUSE_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_CRITTER_FLEEING_FROM].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_CRITTER_PORTRAIT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_GOLD].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_CRITTER_ARROWS].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_CRITTER_BULLETS].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_CRITTER_POWER_CELLS].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_CRITTER_FUEL].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_CRITTER_INVENTORY_NUM].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_INVENTORY_LIST_IDX].type = OD_TYPE_HANDLE_ARRAY;
    object_fields[OBJ_F_CRITTER_INVENTORY_SOURCE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_DESCRIPTION_UNKNOWN].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_FOLLOWER_IDX].type = OD_TYPE_HANDLE_ARRAY;
    object_fields[OBJ_F_CRITTER_TELEPORT_DEST].type = OD_TYPE_INT64;
    object_fields[OBJ_F_CRITTER_TELEPORT_MAP].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_DEATH_TIME].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_AUTO_LEVEL_SCHEME].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_PAD_I_3].type = OD_TYPE_INT32;
    object_fields[OBJ_F_CRITTER_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_CRITTER_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_CRITTER_END].type = OD_TYPE_END;
    object_fields[OBJ_F_PC_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_PC_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_FLAGS_FATE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_REPUTATION_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_BACKGROUND].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_BACKGROUND_TEXT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_REPUTATION_TS_IDX].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_PC_BACKGROUND].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_QUEST_IDX].type = OD_TYPE_QUEST_ARRAY;
    object_fields[OBJ_F_PC_BLESSING_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_BLESSING_TS_IDX].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_PC_CURSE_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_CURSE_TS_IDX].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_PC_PARTY_ID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_RUMOR_IDX].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_PC_PAD_IAS_2].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_SCHEMATICS_FOUND_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_LOGBOOK_EGO_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_FOG_MASK].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_PLAYER_NAME].type = OD_TYPE_STRING;
    object_fields[OBJ_F_PC_BANK_MONEY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_GLOBAL_FLAGS].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_GLOBAL_VARIABLES].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PC_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_PC_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_PC_END].type = OD_TYPE_END;
    object_fields[OBJ_F_NPC_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_NPC_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_LEADER].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_NPC_AI_DATA].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_COMBAT_FOCUS].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_NPC_WHO_HIT_ME_LAST].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_NPC_EXPERIENCE_WORTH].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_EXPERIENCE_POOL].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_WAYPOINTS_IDX].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_NPC_WAYPOINT_CURRENT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_STANDPOINT_DAY].type = OD_TYPE_INT64;
    object_fields[OBJ_F_NPC_STANDPOINT_NIGHT].type = OD_TYPE_INT64;
    object_fields[OBJ_F_NPC_ORIGIN].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_FACTION].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_RETAIL_PRICE_MULTIPLIER].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_SUBSTITUTE_INVENTORY].type = OD_TYPE_HANDLE;
    object_fields[OBJ_F_NPC_REACTION_BASE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_SOCIAL_CLASS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_REACTION_PC_IDX].type = OD_TYPE_HANDLE_ARRAY;
    object_fields[OBJ_F_NPC_REACTION_LEVEL_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_NPC_REACTION_TIME_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_NPC_WAIT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_GENERATOR_DATA].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_PAD_I_1].type = OD_TYPE_INT32;
    object_fields[OBJ_F_NPC_DAMAGE_IDX].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_NPC_SHIT_LIST_IDX].type = OD_TYPE_HANDLE_ARRAY;
    object_fields[OBJ_F_NPC_END].type = OD_TYPE_END;
    object_fields[OBJ_F_TRAP_BEGIN].type = OD_TYPE_BEGIN;
    object_fields[OBJ_F_TRAP_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_TRAP_DIFFICULTY].type = OD_TYPE_INT32;
    object_fields[OBJ_F_TRAP_PAD_I_2].type = OD_TYPE_INT32;
    object_fields[OBJ_F_TRAP_PAD_IAS_1].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_TRAP_PAD_I64AS_1].type = OD_TYPE_UINT64_ARRAY;
    object_fields[OBJ_F_TRAP_END].type = OD_TYPE_END;
    object_fields[OBJ_F_TOTAL_NORMAL].type = OD_TYPE_INVALID;
    object_fields[OBJ_F_RENDER_COLOR].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_COLORS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_PALETTE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_SCALE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_ALPHA].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_RENDER_X].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_Y].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_WIDTH].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_HEIGHT].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PALETTE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_COLOR].type = OD_TYPE_INT32;
    object_fields[OBJ_F_COLORS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_RENDER_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_TEMP_ID].type = OD_TYPE_INT32;
    object_fields[OBJ_F_LIGHT_HANDLE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_OVERLAY_LIGHT_HANDLES].type = OD_TYPE_UINT32_ARRAY;
    object_fields[OBJ_F_SHADOW_HANDLES].type = OD_TYPE_INT32_ARRAY;
    object_fields[OBJ_F_INTERNAL_FLAGS].type = OD_TYPE_INT32;
    object_fields[OBJ_F_FIND_NODE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_TYPE].type = OD_TYPE_INT32;
    object_fields[OBJ_F_PROTOTYPE_HANDLE].type = OD_TYPE_HANDLE;

    // CE: Special case - designate pointer properties.
    object_fields[OBJ_F_RENDER_COLORS].type = OD_TYPE_PTR;
    object_fields[OBJ_F_RENDER_PALETTE].type = OD_TYPE_PTR;
    object_fields[OBJ_F_PALETTE].type = OD_TYPE_PTR;
    object_fields[OBJ_F_COLORS].type = OD_TYPE_PTR;
    object_fields[OBJ_F_LIGHT_HANDLE].type = OD_TYPE_PTR;
    object_fields[OBJ_F_OVERLAY_LIGHT_HANDLES].type = OD_TYPE_PTR_ARRAY;
    object_fields[OBJ_F_SHADOW_HANDLES].type = OD_TYPE_PTR_ARRAY;
    object_fields[OBJ_F_FIND_NODE].type = OD_TYPE_PTR;
}

// 0x40B8E0
void sub_40B8E0(int fld)
{
    switch (fld) {
    case OBJ_F_BEGIN:
        dword_5D10F0[sub_40A790(fld)] = 0;
        break;
    case OBJ_F_WALL_BEGIN:
    case OBJ_F_PORTAL_BEGIN:
    case OBJ_F_CONTAINER_BEGIN:
    case OBJ_F_SCENERY_BEGIN:
    case OBJ_F_PROJECTILE_BEGIN:
    case OBJ_F_ITEM_BEGIN:
    case OBJ_F_CRITTER_BEGIN:
    case OBJ_F_TRAP_BEGIN:
        dword_5D10F0[sub_40A790(fld)] = object_fields[OBJ_F_PAD_I64AS_1].change_array_idx + 1;
        break;
    case OBJ_F_WEAPON_BEGIN:
    case OBJ_F_AMMO_BEGIN:
    case OBJ_F_ARMOR_BEGIN:
    case OBJ_F_GOLD_BEGIN:
    case OBJ_F_FOOD_BEGIN:
    case OBJ_F_SCROLL_BEGIN:
    case OBJ_F_KEY_BEGIN:
    case OBJ_F_KEY_RING_BEGIN:
    case OBJ_F_WRITTEN_BEGIN:
    case OBJ_F_GENERIC_BEGIN:
        dword_5D10F0[sub_40A790(fld)] = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx + 1;
        break;
    case OBJ_F_PC_BEGIN:
    case OBJ_F_NPC_BEGIN:
        dword_5D10F0[sub_40A790(fld)] = object_fields[OBJ_F_CRITTER_PAD_I64AS_1].change_array_idx + 1;
        break;
    }
}

// 0x40BAC0
void obj_handle_field_lists_init(void)
{
    int fld;

    obj_scalar_handle_field_cnt = 0;
    obj_array_handle_field_cnt = 0;

    for (fld = 0; fld < OBJ_F_TOTAL_NORMAL; fld++) {
        if (object_fields[fld].type == OD_TYPE_HANDLE) {
            obj_scalar_handle_field_cnt++;
        } else if (object_fields[fld].type == OD_TYPE_HANDLE_ARRAY) {
            obj_array_handle_field_cnt++;
        }
    }

    if (obj_scalar_handle_field_cnt != 0) {
        obj_scalar_handle_fields = (ObjectField*)MALLOC(sizeof(*obj_scalar_handle_fields) * obj_scalar_handle_field_cnt);
    }

    if (obj_array_handle_field_cnt != 0) {
        obj_array_handle_fields = (ObjectField*)MALLOC(sizeof(*obj_array_handle_fields) * obj_array_handle_field_cnt);
    }

    obj_scalar_handle_field_cnt = 0;
    obj_array_handle_field_cnt = 0;

    for (fld = 0; fld < OBJ_F_TOTAL_NORMAL; fld++) {
        if (object_fields[fld].type == OD_TYPE_HANDLE) {
            obj_scalar_handle_fields[obj_scalar_handle_field_cnt++] = fld;
        } else if (object_fields[fld].type == OD_TYPE_HANDLE_ARRAY) {
            obj_array_handle_fields[obj_array_handle_field_cnt++] = fld;
        }
    }
}

// 0x40BBB0
void obj_handle_field_lists_exit(void)
{
    if (obj_scalar_handle_fields != NULL) {
        FREE(obj_scalar_handle_fields);
        obj_scalar_handle_fields = NULL;
    }

    if (obj_array_handle_fields != NULL) {
        FREE(obj_array_handle_fields);
        obj_array_handle_fields = NULL;
    }
}

// 0x40BBF0
void object_convert_scalar_handles_to_ids(Object* object)
{
    int idx;
    int fld;
    ObjectID oid;
    unsigned int flags;

    for (idx = 0; idx < obj_scalar_handle_field_cnt; idx++) {
        fld = obj_scalar_handle_fields[idx];
        if (object_field_valid(object->type, fld)
            && sub_40D320(object, fld)) {
            obj_field_fetch(object, fld, &oid);
            if (oid.type != OID_TYPE_NULL) {
                if (oid.type == OID_TYPE_HANDLE) {
                    if (obj_handle_is_valid(oid.d.h)) {
                        flags = obj_field_int32_get(oid.d.h, OBJ_F_FLAGS);
                        if ((flags & OF_DESTROYED) == 0
                            || (flags & OF_EXTINCT) != 0) {
                            oid = obj_get_id(oid.d.h);
                            obj_field_store(object, fld, &oid);
                        } else {
                            oid.type = OID_TYPE_NULL;
                            obj_field_store(object, fld, &oid);
                        }
                    } else {
                        oid = obj_pool_perm_reverse_lookup(oid.d.h);
                        obj_field_store(object, fld, &oid);
                    }
                } else {
                    tig_debug_println("Found something other than a handle converting fields to ids.");
                    oid.type = OID_TYPE_NULL;
                    obj_field_store(object, fld, &oid);
                }
            }
        }
    }
}

// 0x40BD20
void object_convert_array_handles_to_ids(Object* object)
{
    int idx;
    int fld;
    SizeableArray** sa_ptr;
    int64_t prototype_obj;

    for (idx = 0; idx < obj_array_handle_field_cnt; idx++) {
        fld = obj_array_handle_fields[idx];
        if (object_field_valid(object->type, fld)
            && sub_40D320(object, fld)) {
            sub_408F40(object, fld, &sa_ptr, &prototype_obj);
            if (*sa_ptr != NULL) {
                if (!sa_enumerate(sa_ptr, convert_handle_to_id)) {
                    tig_debug_printf("Error converting array of object handles to ids (oft_as_count: %d)", idx);
                }
            }
        }
    }
}

// 0x40BDB0
void object_convert_scalar_ids_to_handles(Object* object)
{
    int idx;
    int fld;
    ObjectID oid;
    int64_t obj;

    for (idx = 0; idx < obj_scalar_handle_field_cnt; idx++) {
        fld = obj_scalar_handle_fields[idx];
        if (object_field_valid(object->type, fld)
            && sub_40D320(object, fld)) {
            obj_field_fetch(object, fld, &oid);
            if (oid.type != OID_TYPE_NULL) {
                obj = obj_pool_perm_lookup(oid);
                if (obj != OBJ_HANDLE_NULL) {
                    oid.type = OID_TYPE_HANDLE;
                    oid.d.h = obj;
                } else {
                    oid.type = OID_TYPE_NULL;
                }
                obj_field_store(object, fld, &oid);
            }
        }
    }
}

// 0x40BE70
void object_convert_array_ids_to_handles(Object* object)
{
    int idx;
    int fld;
    SizeableArray** sa_ptr;
    int64_t prototype_obj;

    for (idx = 0; idx < obj_array_handle_field_cnt; idx++) {
        fld = obj_array_handle_fields[idx];
        if (object_field_valid(object->type, fld)
            && sub_40D320(object, fld)) {
            sub_408F40(object, fld, &sa_ptr, &prototype_obj);
            if (*sa_ptr != NULL) {
                if (!sa_enumerate(sa_ptr, convert_id_to_handle)) {
                    tig_debug_printf("Can't convert sizeable array of objects to handles (oft_as_count: %d)\n", idx);
                }
            }
        }
    }
}

// 0x40BF00
bool convert_handle_to_id(void* entry, int index)
{
    ObjectID oid;
    unsigned int flags;

    (void)index;

    oid = *(ObjectID*)entry;

    if (oid.type != OID_TYPE_NULL) {
        if (obj_handle_is_valid(oid.d.h)) {
            flags = obj_field_int32_get(oid.d.h, OBJ_F_FLAGS);
            if ((flags & OF_DESTROYED) != 0
                && (flags & OF_EXTINCT) == 0) {
                oid.type = OID_TYPE_NULL;
                *(ObjectID*)entry = oid;
                return true;
            }

            oid = obj_get_id(oid.d.h);
        } else {
            oid = obj_pool_perm_reverse_lookup(oid.d.h);
        }

        *(ObjectID*)entry = oid;

        return oid.type != OID_TYPE_NULL;
    }

    return true;
}

// 0x40BFC0
bool convert_id_to_handle(void* entry, int index)
{
    ObjectID oid;
    int64_t obj;

    (void)index;

    memcpy(&oid, entry, sizeof(oid));

    if (oid.type != OID_TYPE_NULL) {
        obj = obj_pool_perm_lookup(oid);
        if (obj != OBJ_HANDLE_NULL) {
            oid.type = OID_TYPE_HANDLE;
            oid.d.h = obj;
        } else {
            oid.type = OID_TYPE_NULL;
        }

        memcpy(entry, &oid, sizeof(oid));
    }

    return true;
}

// 0x40C030
int sub_40C030(ObjectType object_type)
{
    int v1;

    switch (object_type) {
    case OBJ_TYPE_WALL:
        v1 = object_fields[OBJ_F_WALL_PAD_I64AS_1].change_array_idx;
        break;
    case OBJ_TYPE_PORTAL:
        v1 = object_fields[OBJ_F_PORTAL_PAD_I64AS_1].change_array_idx;
        break;
    case OBJ_TYPE_CONTAINER:
        v1 = object_fields[OBJ_F_CONTAINER_PAD_I64AS_1].change_array_idx;
        break;
    case OBJ_TYPE_SCENERY:
        v1 = object_fields[OBJ_F_SCENERY_PAD_I64AS_1].change_array_idx;
        break;
    case OBJ_TYPE_PROJECTILE:
        v1 = object_fields[OBJ_F_PROJECTILE_PAD_I64AS_1].change_array_idx;
        break;
    case OBJ_TYPE_WEAPON:
        v1 = object_fields[OBJ_F_WEAPON_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_AMMO:
        v1 = object_fields[OBJ_F_AMMO_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_ARMOR:
        v1 = object_fields[OBJ_F_ARMOR_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_GOLD:
        v1 = object_fields[OBJ_F_GOLD_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_FOOD:
        v1 = object_fields[OBJ_F_FOOD_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_SCROLL:
        v1 = object_fields[OBJ_F_SCROLL_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_KEY:
        v1 = object_fields[OBJ_F_KEY_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_KEY_RING:
        v1 = object_fields[OBJ_F_KEY_RING_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_WRITTEN:
        v1 = object_fields[OBJ_F_WRITTEN_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_GENERIC:
        v1 = object_fields[OBJ_F_GENERIC_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_ITEM_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_PC:
        v1 = object_fields[OBJ_F_PC_PAD_I64AS_1].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_CRITTER_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_NPC:
        // NOTE: Probably wrong.
        v1 = object_fields[OBJ_F_NPC_SHIT_LIST_IDX].change_array_idx;
        if (v1 == -1) {
            v1 = object_fields[OBJ_F_CRITTER_PAD_I64AS_1].change_array_idx;
        }
        break;
    case OBJ_TYPE_TRAP:
        v1 = object_fields[OBJ_F_TRAP_PAD_I64AS_1].change_array_idx;
        break;
    default:
        // NOTE: Original code returns `object_type`.
        assert(0);
    }

    if (v1 == -1) {
        v1 = object_fields[OBJ_F_PAD_I64AS_1].change_array_idx;
    }

    return v1 + 1;
}

// 0x40C260
bool object_field_valid(int type, int fld)
{
    if (object_fields[fld].type <= 2) {
        return false;
    }

    if ((fld > OBJ_F_BEGIN && fld < OBJ_F_END)
        || (fld > OBJ_F_TRANSIENT_BEGIN && fld < OBJ_F_TRANSIENT_END)
        || fld == OBJ_F_TYPE
        || fld == OBJ_F_PROTOTYPE_HANDLE) {
        return true;
    }

    switch (type) {
    case OBJ_TYPE_WALL:
        return fld > OBJ_F_WALL_BEGIN && fld < OBJ_F_WALL_END;
    case OBJ_TYPE_PORTAL:
        return fld > OBJ_F_PORTAL_BEGIN && fld < OBJ_F_PORTAL_END;
    case OBJ_TYPE_CONTAINER:
        return fld > OBJ_F_CONTAINER_BEGIN && fld < OBJ_F_CONTAINER_END;
    case OBJ_TYPE_SCENERY:
        return fld > OBJ_F_SCENERY_BEGIN && fld < OBJ_F_SCENERY_END;
    case OBJ_TYPE_PROJECTILE:
        return fld > OBJ_F_PROJECTILE_BEGIN && fld < OBJ_F_PROJECTILE_END;
    case OBJ_TYPE_WEAPON:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_WEAPON_BEGIN && fld < OBJ_F_WEAPON_END);
    case OBJ_TYPE_AMMO:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_AMMO_BEGIN && fld < OBJ_F_AMMO_END);
    case OBJ_TYPE_ARMOR:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_ARMOR_BEGIN && fld < OBJ_F_ARMOR_END);
    case OBJ_TYPE_GOLD:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_GOLD_BEGIN && fld < OBJ_F_GOLD_END);
    case OBJ_TYPE_FOOD:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_FOOD_BEGIN && fld < OBJ_F_FOOD_END);
    case OBJ_TYPE_SCROLL:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_SCROLL_BEGIN && fld < OBJ_F_SCROLL_END);
    case OBJ_TYPE_KEY:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_KEY_BEGIN && fld < OBJ_F_KEY_END);
    case OBJ_TYPE_KEY_RING:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_KEY_RING_BEGIN && fld < OBJ_F_KEY_RING_END);
    case OBJ_TYPE_WRITTEN:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_WRITTEN_BEGIN && fld < OBJ_F_WRITTEN_END);
    case OBJ_TYPE_GENERIC:
        return (fld > OBJ_F_ITEM_BEGIN && fld < OBJ_F_ITEM_END)
            || (fld > OBJ_F_GENERIC_BEGIN && fld < OBJ_F_GENERIC_END);
    case OBJ_TYPE_PC:
        return (fld > OBJ_F_CRITTER_BEGIN && fld < OBJ_F_CRITTER_END)
            || (fld > OBJ_F_PC_BEGIN && fld < OBJ_F_PC_END);
    case OBJ_TYPE_NPC:
        return (fld > OBJ_F_CRITTER_BEGIN && fld < OBJ_F_CRITTER_END)
            || (fld > OBJ_F_NPC_BEGIN && fld < OBJ_F_NPC_END);
    case OBJ_TYPE_TRAP:
        return fld > OBJ_F_TRAP_BEGIN && fld < OBJ_F_TRAP_END;
    }

    return false;
}

// 0x40C560
bool sub_40C560(Object* object, int fld)
{
    (void)object;
    (void)fld;

    word_5D10FC++;
    dword_5D10F4++;

    return true;
}

// 0x40C580
void sub_40C580(Object* object)
{
    int cnt;

    cnt = sub_40C030(object->type);
    object->field_48 = (int*)CALLOC(sizeof(int) * 2 * cnt, 1);
    object->field_4C = &(object->field_48[cnt]);
}

// 0x40C5B0
void sub_40C5B0(Object* object)
{
    FREE(object->field_48);
}

// 0x40C5C0
void sub_40C5C0(Object* dst, Object* src)
{
    int cnt;

    cnt = sub_40C030(dst->type);
    dst->field_48 = (int*)MALLOC(sizeof(int) * 2 * cnt);
    dst->field_4C = &(dst->field_48[cnt]);
    memcpy(dst->field_48, src->field_48, sizeof(int) * 2 * cnt);
}

// 0x40C610
void sub_40C610(Object* object)
{
    int cnt;

    cnt = sub_40C030(object->type);
    object->field_4C = (int*)CALLOC(sizeof(int) * cnt, 1);
}

// 0x40C640
void sub_40C640(Object* object)
{
    FREE(object->field_4C);
}

// 0x40C650
void sub_40C650(Object* dst, Object* src)
{
    int cnt;

    cnt = sub_40C030(dst->type);
    dst->field_4C = (int*)MALLOC(sizeof(int) * cnt);

    memcpy(dst->field_4C, src->field_4C, sizeof(int) * cnt);
}

// 0x40C690
void sub_40C690(Object* object)
{
    int cnt;

    cnt = sub_40C030(object->type);
    memset(object->field_4C, 0, sizeof(int) * cnt);
}

// 0x40C6B0
bool sub_40C6B0(Object* object, int fld)
{
    (void)fld;

    object->data[dword_5D10F4++] = 0;

    return true;
}

// 0x40C6E0
bool object_proto_field_dealloc(Object* object, int fld)
{
    ObjDataDescriptor free_op;

    free_op.type = object_fields[fld].type;
    free_op.ptr = &(object->data[dword_5D10F4]);
    obj_data_reset(&free_op);
    dword_5D10F4++;

    return true;
}

// 0x40C730
bool object_proto_field_copy(Object* object, int fld)
{
    ObjDataDescriptor copy_op;

    copy_op.type = object_fields[fld].type;
    copy_op.ptr = &(dword_5D1110->data[dword_5D10F4]);
    obj_data_copy(&copy_op, &(object->data[dword_5D10F4]));
    dword_5D10F4++;

    return true;
}

// 0x40C7A0
bool object_inst_field_copy(Object* object, int storage_idx, ObjectFieldInfo* info)
{
    ObjDataDescriptor copy_op;

    copy_op.type = info->type;
    copy_op.ptr = &(dword_5D1108->data[storage_idx]);
    obj_data_copy(&copy_op, &(object->data[storage_idx]));

    return true;
}

// 0x40C7F0
void object_transient_field_copy(Object* dst, Object* src, int fld)
{
    ObjDataDescriptor copy_op;

    copy_op.type = object_fields[fld].type;
    copy_op.ptr = &(src->transient_properties[fld - OBJ_F_TRANSIENT_BEGIN - 1]);
    obj_data_copy(&copy_op, &(dst->transient_properties[fld - OBJ_F_TRANSIENT_BEGIN - 1]));
}

// 0x40C840
void object_transient_field_dealloc(Object* object, int fld)
{
    ObjDataDescriptor free_op;

    free_op.type = object_fields[fld].type;
    free_op.ptr = &(object->transient_properties[fld - OBJ_F_TRANSIENT_BEGIN - 1]);
    obj_data_reset(&free_op);
}

// 0x40C880
bool object_proto_enumerate_fields(Object* object, ObjectProtoEnumerateFieldsCallback* callback)
{
    if (!object_proto_enumerate_fields_func(object, OBJ_F_BEGIN, OBJ_F_END, callback)) {
        return false;
    }

    switch (object->type) {
    case OBJ_TYPE_WALL:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_WALL_BEGIN, OBJ_F_WALL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PORTAL:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_PORTAL_BEGIN, OBJ_F_PORTAL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_CONTAINER:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_CONTAINER_BEGIN, OBJ_F_CONTAINER_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_SCENERY:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_SCENERY_BEGIN, OBJ_F_SCENERY_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PROJECTILE:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_PROJECTILE_BEGIN, OBJ_F_PROJECTILE_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_WEAPON:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_WEAPON_BEGIN, OBJ_F_WEAPON_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_AMMO:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_AMMO_BEGIN, OBJ_F_AMMO_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_ARMOR:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ARMOR_BEGIN, OBJ_F_ARMOR_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_GOLD:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_GOLD_BEGIN, OBJ_F_GOLD_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_FOOD:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_FOOD_BEGIN, OBJ_F_FOOD_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_SCROLL:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_SCROLL_BEGIN, OBJ_F_SCROLL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_KEY:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_KEY_BEGIN, OBJ_F_KEY_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_KEY_RING:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_KEY_RING_BEGIN, OBJ_F_KEY_RING_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_WRITTEN:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_WRITTEN_BEGIN, OBJ_F_WRITTEN_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_GENERIC:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_GENERIC_BEGIN, OBJ_F_GENERIC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PC:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_CRITTER_BEGIN, OBJ_F_CRITTER_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_PC_BEGIN, OBJ_F_PC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_NPC:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_CRITTER_BEGIN, OBJ_F_CRITTER_END, callback)) {
            return false;
        }
        if (!object_proto_enumerate_fields_func(object, OBJ_F_NPC_BEGIN, OBJ_F_NPC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_TRAP:
        if (!object_proto_enumerate_fields_func(object, OBJ_F_TRAP_BEGIN, OBJ_F_TRAP_END, callback)) {
            return false;
        }
        break;
    }

    return true;
}

// 0x40CB00
bool object_proto_enumerate_fields_func(Object* obj, int begin, int end, ObjectProtoEnumerateFieldsCallback* callback)
{
    int index;

    // +1 to skip `OBJ_F_{TYPE}_BEGIN`.
    for (index = begin + 1; index < end; index++) {
        if (!callback(obj, index)) {
            return false;
        }
    }

    return true;
}

// 0x40CB40
int sub_40CB40(Object* object, int fld)
{
    (void)object;

    return object_fields[fld].simple_array_idx;
}

// 0x40CB60
bool object_inst_field_dealloc(Object* object, int storage_idx, ObjectFieldInfo* info)
{
    ObjDataDescriptor free_op;

    free_op.type = info->type;
    free_op.ptr = &(object->data[storage_idx]);
    obj_data_reset(&free_op);

    return true;
}

// 0x40CBA0
bool object_inst_enumerate_overridden_fields(Object* object, ObjectInstEnumerateFieldsCallback* callback)
{
    if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_BEGIN, OBJ_F_END, callback)) {
        return false;
    }

    switch (object->type) {
    case OBJ_TYPE_WALL:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_WALL_BEGIN, OBJ_F_WALL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PORTAL:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_PORTAL_BEGIN, OBJ_F_PORTAL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_CONTAINER:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_CONTAINER_BEGIN, OBJ_F_CONTAINER_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_SCENERY:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_SCENERY_BEGIN, OBJ_F_SCENERY_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PROJECTILE:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_PROJECTILE_BEGIN, OBJ_F_PROJECTILE_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_WEAPON:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_WEAPON_BEGIN, OBJ_F_WEAPON_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_AMMO:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_AMMO_BEGIN, OBJ_F_AMMO_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_ARMOR:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ARMOR_BEGIN, OBJ_F_ARMOR_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_GOLD:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_GOLD_BEGIN, OBJ_F_GOLD_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_FOOD:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_FOOD_BEGIN, OBJ_F_FOOD_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_SCROLL:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_SCROLL_BEGIN, OBJ_F_SCROLL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_KEY:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_KEY_BEGIN, OBJ_F_KEY_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_KEY_RING:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_KEY_RING_BEGIN, OBJ_F_KEY_RING_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_WRITTEN:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_WRITTEN_BEGIN, OBJ_F_WRITTEN_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_GENERIC:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_GENERIC_BEGIN, OBJ_F_GENERIC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PC:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_CRITTER_BEGIN, OBJ_F_CRITTER_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_PC_BEGIN, OBJ_F_PC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_NPC:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_CRITTER_BEGIN, OBJ_F_CRITTER_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_NPC_BEGIN, OBJ_F_NPC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_TRAP:
        if (!object_inst_enumerate_overridden_fields_func(object, OBJ_F_TRAP_BEGIN, OBJ_F_TRAP_END, callback)) {
            return false;
        }
        break;
    }

    return true;
}

// 0x40CE20
bool object_inst_enumerate_overridden_fields_func(Object* object, int begin, int end, ObjectInstEnumerateFieldsCallback* callback)
{
    int v1 = 0;
    int index;
    int fld;

    for (index = 0; index < object_fields[begin + 1].change_array_idx; index++) {
        v1 += bitset_count_bits(object->field_48[index], 32);
    }

    v1 += bitset_count_bits(object->field_48[index], object_fields[begin + 1].bit);

    for (fld = begin + 1; fld < end; fld++) {
        if ((object->field_48[object_fields[fld].change_array_idx] & object_fields[fld].mask) != 0) {
            if (!callback(object, v1, &(object_fields[fld]))) {
                return false;
            }

            if ((object->field_48[object_fields[fld].change_array_idx] & object_fields[fld].mask) != 0) {
                v1++;
            }
        }
    }

    return true;
}

// 0x40CEF0
bool object_inst_enumerate_available_fields(Object* object, ObjectInstEnumerateFieldsCallback* callback)
{
    if (!object_inst_enumerate_available_fields_func(object, OBJ_F_BEGIN, OBJ_F_END, callback)) {
        return false;
    }

    switch (object->type) {
    case OBJ_TYPE_WALL:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_WALL_BEGIN, OBJ_F_WALL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PORTAL:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_PORTAL_BEGIN, OBJ_F_PORTAL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_CONTAINER:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_CONTAINER_BEGIN, OBJ_F_CONTAINER_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_SCENERY:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_SCENERY_BEGIN, OBJ_F_SCENERY_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PROJECTILE:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_PROJECTILE_BEGIN, OBJ_F_PROJECTILE_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_WEAPON:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_WEAPON_BEGIN, OBJ_F_WEAPON_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_AMMO:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_AMMO_BEGIN, OBJ_F_AMMO_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_ARMOR:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ARMOR_BEGIN, OBJ_F_ARMOR_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_GOLD:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_GOLD_BEGIN, OBJ_F_GOLD_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_FOOD:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_FOOD_BEGIN, OBJ_F_FOOD_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_SCROLL:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_SCROLL_BEGIN, OBJ_F_SCROLL_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_KEY:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_KEY_BEGIN, OBJ_F_KEY_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_KEY_RING:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_KEY_RING_BEGIN, OBJ_F_KEY_RING_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_WRITTEN:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_WRITTEN_BEGIN, OBJ_F_WRITTEN_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_GENERIC:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_ITEM_BEGIN, OBJ_F_ITEM_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_GENERIC_BEGIN, OBJ_F_GENERIC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_PC:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_CRITTER_BEGIN, OBJ_F_CRITTER_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_PC_BEGIN, OBJ_F_PC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_NPC:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_CRITTER_BEGIN, OBJ_F_CRITTER_END, callback)) {
            return false;
        }
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_NPC_BEGIN, OBJ_F_NPC_END, callback)) {
            return false;
        }
        break;
    case OBJ_TYPE_TRAP:
        if (!object_inst_enumerate_available_fields_func(object, OBJ_F_TRAP_BEGIN, OBJ_F_TRAP_END, callback)) {
            return false;
        }
        break;
    }

    return true;
}

// 0x40D170
bool object_inst_enumerate_available_fields_func(Object* object, int begin, int end, ObjectInstEnumerateFieldsCallback* callback)
{
    int v1 = 0;
    int index;
    int fld;

    for (index = 0; index < object_fields[begin + 1].change_array_idx; index++) {
        v1 += bitset_count_bits(object->field_48[index], 32);
    }

    v1 += bitset_count_bits(object->field_48[index], object_fields[begin + 1].bit);

    for (fld = begin + 1; fld < end; fld++) {
        if (!callback(object, v1, &(object_fields[fld]))) {
            return false;
        }

        if ((object->field_48[object_fields[fld].change_array_idx] & object_fields[fld].mask) != 0) {
            v1++;
        }
    }

    return true;
}

// 0x40D230
int sub_40D230(Object* object, int fld)
{
    int v1 = 0;
    int index;

    for (index = 0; index < object_fields[fld].change_array_idx; index++) {
        v1 += bitset_count_bits(object->field_48[index], 32);
    }

    v1 += bitset_count_bits(object->field_48[index], object_fields[fld].bit);

    return v1;
}

// 0x40D2A0
void sub_40D2A0(Object* object, int fld)
{
    Object* prototype;
    ObjDataDescriptor v1;
    int v2;
    int v3;

    sub_40D450(object, fld);
    sub_40D370(object, fld, true);

    prototype = obj_lock(obj_get_prototype_handle(object));
    v2 = sub_40CB40(prototype, fld);
    v1.type = object_fields[fld].type;
    v1.ptr = &(prototype->data[v2]);
    v3 = sub_40D230(object, fld);
    obj_data_copy(&v1, &(object->data[v3]));
}

// 0x40D320
bool sub_40D320(Object* object, int fld)
{
    return (object->field_48[object_fields[fld].change_array_idx] & object_fields[fld].mask) != 0;
}

// 0x40D350
bool sub_40D350(Object* object, int fld)
{
    return sub_40D320(object, fld) != 0;
}

// 0x40D370
void sub_40D370(Object* object, int fld, bool enabled)
{
    sub_40D3A0(object, &(object_fields[fld]), enabled);
}

// 0x40D3A0
void sub_40D3A0(Object* object, ObjectFieldInfo* info, int enabled)
{
    if (enabled) {
        object->field_48[info->change_array_idx] |= info->mask;
    } else {
        object->field_48[info->change_array_idx] &= ~info->mask;
    }
}

// 0x40D3D0
bool sub_40D3D0(Object* object, int fld)
{
    return (object->field_4C[object_fields[fld].change_array_idx] & object_fields[fld].mask) != 0;
}

// 0x40D400
void sub_40D400(Object* object, int fld, bool enabled)
{
    if (enabled) {
        object->field_4C[object_fields[fld].change_array_idx] |= object_fields[fld].mask;
    } else {
        object->field_4C[object_fields[fld].change_array_idx] &= ~object_fields[fld].mask;
    }
}

// 0x40D450
void sub_40D450(Object* object, int fld)
{
    sub_40D470(object, sub_40D230(object, fld));
}

// 0x40D470
void sub_40D470(Object* object, int fld)
{
    int index;

    object->num_fields++;
    object->data = (intptr_t*)REALLOC(object->data, sizeof(*object->data) * object->num_fields);

    for (index = object->num_fields - 1; index > fld; index--) {
        object->data[index] = object->data[index - 1];
    }

    object->data[fld] = 0;
}

// 0x40D4D0
void sub_40D4D0(Object* object, int fld)
{
    int v1;
    ObjDataDescriptor v2;
    int index;

    v1 = sub_40D230(object, fld);
    v2.type = object_fields[fld].type;
    v2.ptr = &(object->data[v1]);
    obj_data_reset(&v2);

    for (index = v1; index < object->num_fields - 1; index++) {
        object->data[index] = object->data[index + 1];
    }

    object->num_fields--;
    object->data = (intptr_t*)REALLOC(object->data, sizeof(*object->data) * object->num_fields);
}

// 0x40D560
bool obj_version_write_file(TigFile* stream)
{
    int version = OBJ_FILE_VERSION;
    return objf_write(&version, sizeof(version), stream);
}

// 0x40D590
bool obj_version_read_file(TigFile* stream)
{
    int version;

    if (!objf_read(&version, sizeof(version), stream)) {
        return false;
    }

    if (version != OBJ_FILE_VERSION) {
        tig_debug_printf("Object file format version mismatch (read: %d, expected: %d).\n", version, OBJ_FILE_VERSION);
        return false;
    }

    return true;
}

// 0x40D5D0
void obj_version_write_mem(WriteBuffer* wb)
{
    int version = OBJ_FILE_VERSION;
    write_buffer_append(&version, sizeof(version), wb);
}

// 0x40D5F0
bool obj_version_read_mem(uint8_t** data)
{
    int version;

    mem_read_advance(&version, sizeof(version), data);
    if (version != OBJ_FILE_VERSION) {
        tig_debug_printf("Object file format version mismatch: (read: %d, expected: %d).\n", version, OBJ_FILE_VERSION);
    }

    return true;
}

// 0x40D630
int64_t obj_get_prototype_handle(Object* object)
{
    if (object->prototype_obj == OBJ_HANDLE_NULL) {
        object->prototype_obj = obj_pool_perm_lookup(object->prototype_oid);
    }

    return object->prototype_obj;
}

// 0x40D670
bool sub_40D670(Object* object, int a2, ObjectFieldInfo* field_info)
{
    (void)a2;

    if ((object->field_4C[field_info->change_array_idx] & field_info->mask) != 0) {
        tig_debug_printf("\t #%d", field_info - object_fields);
    }
    return true;
}

void* obj_field_ptr_get(int64_t obj, int fld)
{
    Object* object;
    void* value;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    obj_field_fetch(object, fld, &value);
    obj_unlock(obj);

    return value;
}

void obj_field_ptr_set(int64_t obj, int fld, void* value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_field_store(object, fld, &value);
    obj_unlock(obj);
}

void* obj_arrayfield_ptr_get(int64_t obj, int fld, int index)
{
    Object* object;
    void* value;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return 0;
    }

    obj_arrayfield_fetch(object, fld, index, &value);
    obj_unlock(obj);

    return value;
}

void obj_arrayfield_ptr_set(int64_t obj, int fld, int index, void* value)
{
    Object* object;

    object = obj_lock(obj);
    if (!object_field_valid(object->type, fld)) {
        object_field_not_exists(object, fld);
        obj_unlock(obj);
        return;
    }

    obj_arrayfield_store(object, fld, index, &value);
    obj_unlock(obj);
}
