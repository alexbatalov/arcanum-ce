#ifndef ARCANUM_GAME_OBJ_H_
#define ARCANUM_GAME_OBJ_H_

#include "game/context.h"
#include "game/obj_flags.h"
#include "game/obj_id.h"

#define OBJ_HANDLE_NULL 0LL

typedef enum ObjectField {
    OBJ_F_BEGIN,
    OBJ_F_CURRENT_AID,
    OBJ_F_LOCATION,
    OBJ_F_OFFSET_X,
    OBJ_F_OFFSET_Y,
    OBJ_F_SHADOW,
    OBJ_F_OVERLAY_FORE,
    OBJ_F_OVERLAY_BACK,
    OBJ_F_UNDERLAY,
    OBJ_F_BLIT_FLAGS,
    OBJ_F_BLIT_COLOR,
    OBJ_F_BLIT_ALPHA,
    OBJ_F_BLIT_SCALE,
    OBJ_F_LIGHT_FLAGS,
    OBJ_F_LIGHT_AID,
    OBJ_F_LIGHT_COLOR,
    OBJ_F_OVERLAY_LIGHT_FLAGS,
    OBJ_F_OVERLAY_LIGHT_AID,
    OBJ_F_OVERLAY_LIGHT_COLOR,
    OBJ_F_FLAGS,
    OBJ_F_SPELL_FLAGS,
    OBJ_F_BLOCKING_MASK,
    OBJ_F_NAME,
    OBJ_F_DESCRIPTION,
    OBJ_F_AID,
    OBJ_F_DESTROYED_AID,
    OBJ_F_AC,
    OBJ_F_HP_PTS,
    OBJ_F_HP_ADJ,
    OBJ_F_HP_DAMAGE,
    OBJ_F_MATERIAL,
    OBJ_F_RESISTANCE_IDX,
    OBJ_F_SCRIPTS_IDX,
    OBJ_F_SOUND_EFFECT,
    OBJ_F_CATEGORY,
    OBJ_F_PAD_IAS_1,
    OBJ_F_PAD_I64AS_1,
    OBJ_F_END,
    OBJ_F_WALL_BEGIN,
    OBJ_F_WALL_FLAGS,
    OBJ_F_WALL_PAD_I_1,
    OBJ_F_WALL_PAD_I_2,
    OBJ_F_WALL_PAD_IAS_1,
    OBJ_F_WALL_PAD_I64AS_1,
    OBJ_F_WALL_END,
    OBJ_F_PORTAL_BEGIN,
    OBJ_F_PORTAL_FLAGS,
    OBJ_F_PORTAL_LOCK_DIFFICULTY,
    OBJ_F_PORTAL_KEY_ID,
    OBJ_F_PORTAL_NOTIFY_NPC,
    OBJ_F_PORTAL_PAD_I_1,
    OBJ_F_PORTAL_PAD_I_2,
    OBJ_F_PORTAL_PAD_IAS_1,
    OBJ_F_PORTAL_PAD_I64AS_1,
    OBJ_F_PORTAL_END,
    OBJ_F_CONTAINER_BEGIN,
    OBJ_F_CONTAINER_FLAGS,
    OBJ_F_CONTAINER_LOCK_DIFFICULTY,
    OBJ_F_CONTAINER_KEY_ID,
    OBJ_F_CONTAINER_INVENTORY_NUM,
    OBJ_F_CONTAINER_INVENTORY_LIST_IDX,
    OBJ_F_CONTAINER_INVENTORY_SOURCE,
    OBJ_F_CONTAINER_NOTIFY_NPC,
    OBJ_F_CONTAINER_PAD_I_1,
    OBJ_F_CONTAINER_PAD_I_2,
    OBJ_F_CONTAINER_PAD_IAS_1,
    OBJ_F_CONTAINER_PAD_I64AS_1,
    OBJ_F_CONTAINER_END,
    OBJ_F_SCENERY_BEGIN,
    OBJ_F_SCENERY_FLAGS,
    OBJ_F_SCENERY_WHOS_IN_ME,
    OBJ_F_SCENERY_RESPAWN_DELAY,
    OBJ_F_SCENERY_PAD_I_2,
    OBJ_F_SCENERY_PAD_IAS_1,
    OBJ_F_SCENERY_PAD_I64AS_1,
    OBJ_F_SCENERY_END,
    OBJ_F_PROJECTILE_BEGIN,
    OBJ_F_PROJECTILE_FLAGS_COMBAT,
    OBJ_F_PROJECTILE_FLAGS_COMBAT_DAMAGE,
    OBJ_F_PROJECTILE_HIT_LOC,
    OBJ_F_PROJECTILE_PARENT_WEAPON,
    OBJ_F_PROJECTILE_PAD_I_1,
    OBJ_F_PROJECTILE_PAD_I_2,
    OBJ_F_PROJECTILE_PAD_IAS_1,
    OBJ_F_PROJECTILE_PAD_I64AS_1,
    OBJ_F_PROJECTILE_END,
    OBJ_F_ITEM_BEGIN,
    OBJ_F_ITEM_FLAGS,
    OBJ_F_ITEM_PARENT,
    OBJ_F_ITEM_WEIGHT,
    OBJ_F_ITEM_MAGIC_WEIGHT_ADJ,
    OBJ_F_ITEM_WORTH,
    OBJ_F_ITEM_MANA_STORE,
    OBJ_F_ITEM_INV_AID,
    OBJ_F_ITEM_INV_LOCATION,
    OBJ_F_ITEM_USE_AID_FRAGMENT,
    OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY,
    OBJ_F_ITEM_DISCIPLINE,
    OBJ_F_ITEM_DESCRIPTION_UNKNOWN,
    OBJ_F_ITEM_DESCRIPTION_EFFECTS,
    OBJ_F_ITEM_SPELL_1,
    OBJ_F_ITEM_SPELL_2,
    OBJ_F_ITEM_SPELL_3,
    OBJ_F_ITEM_SPELL_4,
    OBJ_F_ITEM_SPELL_5,
    OBJ_F_ITEM_SPELL_MANA_STORE,
    OBJ_F_ITEM_AI_ACTION,
    OBJ_F_ITEM_PAD_I_1,
    OBJ_F_ITEM_PAD_IAS_1,
    OBJ_F_ITEM_PAD_I64AS_1,
    OBJ_F_ITEM_END,
    OBJ_F_WEAPON_BEGIN,
    OBJ_F_WEAPON_FLAGS,
    OBJ_F_WEAPON_PAPER_DOLL_AID,
    OBJ_F_WEAPON_BONUS_TO_HIT,
    OBJ_F_WEAPON_MAGIC_HIT_ADJ,
    OBJ_F_WEAPON_DAMAGE_LOWER_IDX,
    OBJ_F_WEAPON_DAMAGE_UPPER_IDX,
    OBJ_F_WEAPON_MAGIC_DAMAGE_ADJ_IDX,
    OBJ_F_WEAPON_SPEED_FACTOR,
    OBJ_F_WEAPON_MAGIC_SPEED_ADJ,
    OBJ_F_WEAPON_RANGE,
    OBJ_F_WEAPON_MAGIC_RANGE_ADJ,
    OBJ_F_WEAPON_MIN_STRENGTH,
    OBJ_F_WEAPON_MAGIC_MIN_STRENGTH_ADJ,
    OBJ_F_WEAPON_AMMO_TYPE,
    OBJ_F_WEAPON_AMMO_CONSUMPTION,
    OBJ_F_WEAPON_MISSILE_AID,
    OBJ_F_WEAPON_VISUAL_EFFECT_AID,
    OBJ_F_WEAPON_CRIT_HIT_CHART,
    OBJ_F_WEAPON_MAGIC_CRIT_HIT_CHANCE,
    OBJ_F_WEAPON_MAGIC_CRIT_HIT_EFFECT,
    OBJ_F_WEAPON_CRIT_MISS_CHART,
    OBJ_F_WEAPON_MAGIC_CRIT_MISS_CHANCE,
    OBJ_F_WEAPON_MAGIC_CRIT_MISS_EFFECT,
    OBJ_F_WEAPON_PAD_I_1,
    OBJ_F_WEAPON_PAD_I_2,
    OBJ_F_WEAPON_PAD_IAS_1,
    OBJ_F_WEAPON_PAD_I64AS_1,
    OBJ_F_WEAPON_END,
    OBJ_F_AMMO_BEGIN,
    OBJ_F_AMMO_FLAGS,
    OBJ_F_AMMO_QUANTITY,
    OBJ_F_AMMO_TYPE,
    OBJ_F_AMMO_PAD_I_1,
    OBJ_F_AMMO_PAD_I_2,
    OBJ_F_AMMO_PAD_IAS_1,
    OBJ_F_AMMO_PAD_I64AS_1,
    OBJ_F_AMMO_END,
    OBJ_F_ARMOR_BEGIN,
    OBJ_F_ARMOR_FLAGS,
    OBJ_F_ARMOR_PAPER_DOLL_AID,
    OBJ_F_ARMOR_AC_ADJ,
    OBJ_F_ARMOR_MAGIC_AC_ADJ,
    OBJ_F_ARMOR_RESISTANCE_ADJ_IDX,
    OBJ_F_ARMOR_MAGIC_RESISTANCE_ADJ_IDX,
    OBJ_F_ARMOR_SILENT_MOVE_ADJ,
    OBJ_F_ARMOR_MAGIC_SILENT_MOVE_ADJ,
    OBJ_F_ARMOR_UNARMED_BONUS_DAMAGE,
    OBJ_F_ARMOR_PAD_I_2,
    OBJ_F_ARMOR_PAD_IAS_1,
    OBJ_F_ARMOR_PAD_I64AS_1,
    OBJ_F_ARMOR_END,
    OBJ_F_GOLD_BEGIN,
    OBJ_F_GOLD_FLAGS,
    OBJ_F_GOLD_QUANTITY,
    OBJ_F_GOLD_PAD_I_1,
    OBJ_F_GOLD_PAD_I_2,
    OBJ_F_GOLD_PAD_IAS_1,
    OBJ_F_GOLD_PAD_I64AS_1,
    OBJ_F_GOLD_END,
    OBJ_F_FOOD_BEGIN,
    OBJ_F_FOOD_FLAGS,
    OBJ_F_FOOD_PAD_I_1,
    OBJ_F_FOOD_PAD_I_2,
    OBJ_F_FOOD_PAD_IAS_1,
    OBJ_F_FOOD_PAD_I64AS_1,
    OBJ_F_FOOD_END,
    OBJ_F_SCROLL_BEGIN,
    OBJ_F_SCROLL_FLAGS,
    OBJ_F_SCROLL_PAD_I_1,
    OBJ_F_SCROLL_PAD_I_2,
    OBJ_F_SCROLL_PAD_IAS_1,
    OBJ_F_SCROLL_PAD_I64AS_1,
    OBJ_F_SCROLL_END,
    OBJ_F_KEY_BEGIN,
    OBJ_F_KEY_KEY_ID,
    OBJ_F_KEY_PAD_I_1,
    OBJ_F_KEY_PAD_I_2,
    OBJ_F_KEY_PAD_IAS_1,
    OBJ_F_KEY_PAD_I64AS_1,
    OBJ_F_KEY_END,
    OBJ_F_KEY_RING_BEGIN,
    OBJ_F_KEY_RING_FLAGS,
    OBJ_F_KEY_RING_LIST_IDX,
    OBJ_F_KEY_RING_PAD_I_1,
    OBJ_F_KEY_RING_PAD_I_2,
    OBJ_F_KEY_RING_PAD_IAS_1,
    OBJ_F_KEY_RING_PAD_I64AS_1,
    OBJ_F_KEY_RING_END,
    OBJ_F_WRITTEN_BEGIN,
    OBJ_F_WRITTEN_FLAGS,
    OBJ_F_WRITTEN_SUBTYPE,
    OBJ_F_WRITTEN_TEXT_START_LINE,
    OBJ_F_WRITTEN_TEXT_END_LINE,
    OBJ_F_WRITTEN_PAD_I_1,
    OBJ_F_WRITTEN_PAD_I_2,
    OBJ_F_WRITTEN_PAD_IAS_1,
    OBJ_F_WRITTEN_PAD_I64AS_1,
    OBJ_F_WRITTEN_END,
    OBJ_F_GENERIC_BEGIN,
    OBJ_F_GENERIC_FLAGS,
    OBJ_F_GENERIC_USAGE_BONUS,
    OBJ_F_GENERIC_USAGE_COUNT_REMAINING,
    OBJ_F_GENERIC_PAD_IAS_1,
    OBJ_F_GENERIC_PAD_I64AS_1,
    OBJ_F_GENERIC_END,
    OBJ_F_CRITTER_BEGIN,
    OBJ_F_CRITTER_FLAGS,
    OBJ_F_CRITTER_FLAGS2,
    OBJ_F_CRITTER_STAT_BASE_IDX,
    OBJ_F_CRITTER_BASIC_SKILL_IDX,
    OBJ_F_CRITTER_TECH_SKILL_IDX,
    OBJ_F_CRITTER_SPELL_TECH_IDX,
    OBJ_F_CRITTER_FATIGUE_PTS,
    OBJ_F_CRITTER_FATIGUE_ADJ,
    OBJ_F_CRITTER_FATIGUE_DAMAGE,
    OBJ_F_CRITTER_CRIT_HIT_CHART,
    OBJ_F_CRITTER_EFFECTS_IDX,
    OBJ_F_CRITTER_EFFECT_CAUSE_IDX,
    OBJ_F_CRITTER_FLEEING_FROM,
    OBJ_F_CRITTER_PORTRAIT,
    OBJ_F_CRITTER_GOLD,
    OBJ_F_CRITTER_ARROWS,
    OBJ_F_CRITTER_BULLETS,
    OBJ_F_CRITTER_POWER_CELLS,
    OBJ_F_CRITTER_FUEL,
    OBJ_F_CRITTER_INVENTORY_NUM,
    OBJ_F_CRITTER_INVENTORY_LIST_IDX,
    OBJ_F_CRITTER_INVENTORY_SOURCE,
    OBJ_F_CRITTER_DESCRIPTION_UNKNOWN,
    OBJ_F_CRITTER_FOLLOWER_IDX,
    OBJ_F_CRITTER_TELEPORT_DEST,
    OBJ_F_CRITTER_TELEPORT_MAP,
    OBJ_F_CRITTER_DEATH_TIME,
    OBJ_F_CRITTER_AUTO_LEVEL_SCHEME,
    OBJ_F_CRITTER_PAD_I_1,
    OBJ_F_CRITTER_PAD_I_2,
    OBJ_F_CRITTER_PAD_I_3,
    OBJ_F_CRITTER_PAD_IAS_1,
    OBJ_F_CRITTER_PAD_I64AS_1,
    OBJ_F_CRITTER_END,
    OBJ_F_PC_BEGIN,
    OBJ_F_PC_FLAGS,
    OBJ_F_PC_FLAGS_FATE,
    OBJ_F_PC_REPUTATION_IDX,
    OBJ_F_PC_REPUTATION_TS_IDX,
    OBJ_F_PC_BACKGROUND,
    OBJ_F_PC_BACKGROUND_TEXT,
    OBJ_F_PC_QUEST_IDX,
    OBJ_F_PC_BLESSING_IDX,
    OBJ_F_PC_BLESSING_TS_IDX,
    OBJ_F_PC_CURSE_IDX,
    OBJ_F_PC_CURSE_TS_IDX,
    OBJ_F_PC_PARTY_ID,
    OBJ_F_PC_RUMOR_IDX,
    OBJ_F_PC_PAD_IAS_2,
    OBJ_F_PC_SCHEMATICS_FOUND_IDX,
    OBJ_F_PC_LOGBOOK_EGO_IDX,
    OBJ_F_PC_FOG_MASK,
    OBJ_F_PC_PLAYER_NAME,
    OBJ_F_PC_BANK_MONEY,
    OBJ_F_PC_GLOBAL_FLAGS,
    OBJ_F_PC_GLOBAL_VARIABLES,
    OBJ_F_PC_PAD_I_1,
    OBJ_F_PC_PAD_I_2,
    OBJ_F_PC_PAD_IAS_1,
    OBJ_F_PC_PAD_I64AS_1,
    OBJ_F_PC_END,
    OBJ_F_NPC_BEGIN,
    OBJ_F_NPC_FLAGS,
    OBJ_F_NPC_LEADER,
    OBJ_F_NPC_AI_DATA,
    OBJ_F_NPC_COMBAT_FOCUS,
    OBJ_F_NPC_WHO_HIT_ME_LAST,
    OBJ_F_NPC_EXPERIENCE_WORTH,
    OBJ_F_NPC_EXPERIENCE_POOL,
    OBJ_F_NPC_WAYPOINTS_IDX,
    OBJ_F_NPC_WAYPOINT_CURRENT,
    OBJ_F_NPC_STANDPOINT_DAY,
    OBJ_F_NPC_STANDPOINT_NIGHT,
    OBJ_F_NPC_ORIGIN,
    OBJ_F_NPC_FACTION,
    OBJ_F_NPC_RETAIL_PRICE_MULTIPLIER,
    OBJ_F_NPC_SUBSTITUTE_INVENTORY,
    OBJ_F_NPC_REACTION_BASE,
    OBJ_F_NPC_SOCIAL_CLASS,
    OBJ_F_NPC_REACTION_PC_IDX,
    OBJ_F_NPC_REACTION_LEVEL_IDX,
    OBJ_F_NPC_REACTION_TIME_IDX,
    OBJ_F_NPC_WAIT,
    OBJ_F_NPC_GENERATOR_DATA,
    OBJ_F_NPC_PAD_I_1,
    OBJ_F_NPC_DAMAGE_IDX,
    OBJ_F_NPC_SHIT_LIST_IDX,
    OBJ_F_NPC_END,
    OBJ_F_TRAP_BEGIN,
    OBJ_F_TRAP_FLAGS,
    OBJ_F_TRAP_DIFFICULTY,
    OBJ_F_TRAP_PAD_I_2,
    OBJ_F_TRAP_PAD_IAS_1,
    OBJ_F_TRAP_PAD_I64AS_1,
    OBJ_F_TRAP_END,
    OBJ_F_TOTAL_NORMAL,
    OBJ_F_TRANSIENT_BEGIN,
    OBJ_F_RENDER_COLOR,
    OBJ_F_RENDER_COLORS,
    OBJ_F_RENDER_PALETTE,
    OBJ_F_RENDER_SCALE,
    OBJ_F_RENDER_ALPHA,
    OBJ_F_RENDER_X,
    OBJ_F_RENDER_Y,
    OBJ_F_RENDER_WIDTH,
    OBJ_F_RENDER_HEIGHT,
    OBJ_F_PALETTE,
    OBJ_F_COLOR,
    OBJ_F_COLORS,
    OBJ_F_RENDER_FLAGS,
    OBJ_F_TEMP_ID,
    OBJ_F_LIGHT_HANDLE,
    OBJ_F_OVERLAY_LIGHT_HANDLES,
    OBJ_F_SHADOW_HANDLES,
    OBJ_F_INTERNAL_FLAGS,
    OBJ_F_FIND_NODE,
    OBJ_F_TRANSIENT_END,
    OBJ_F_TYPE,
    OBJ_F_PROTOTYPE_HANDLE,
    OBJ_F_MAX,
} ObjectField;

typedef enum ObjectType {
    OBJ_TYPE_WALL,
    OBJ_TYPE_PORTAL,
    OBJ_TYPE_CONTAINER,
    OBJ_TYPE_SCENERY,
    OBJ_TYPE_PROJECTILE,
    OBJ_TYPE_WEAPON,
    OBJ_TYPE_AMMO,
    OBJ_TYPE_ARMOR,
    OBJ_TYPE_GOLD,
    OBJ_TYPE_FOOD,
    OBJ_TYPE_SCROLL,
    OBJ_TYPE_KEY,
    OBJ_TYPE_KEY_RING,
    OBJ_TYPE_WRITTEN,
    OBJ_TYPE_GENERIC,
    OBJ_TYPE_PC,
    OBJ_TYPE_NPC,
    OBJ_TYPE_TRAP,
    OBJ_TYPE_MONSTER,
    OBJ_TYPE_UNIQUE_NPC,
    OBJ_TYPE_COUNT,
} ObjectType;

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

typedef bool(ObjEnumerateCallback)(Object* object, int fld);

bool obj_init(GameInitInfo* init_info);
void obj_exit(void);
void obj_reset(void);
bool obj_validate_system(unsigned int flags);
void sub_405790(int64_t obj);
void obj_create_proto(int type, int64_t* obj_ptr);
void obj_create_inst(int64_t proto_obj, int64_t loc, int64_t* obj_ptr);
void sub_405B30(int64_t proto_obj, int64_t loc, ObjectID oid, int64_t* obj_ptr);
void sub_408D60(Object* object, int fld, int* value_ptr);
void sub_408E70(Object* object, int fld, int value);
bool obj_is_proto(int64_t obj);
void obj_deallocate(int64_t obj);
void sub_405CC0(int64_t obj);
void sub_405D60(int64_t* new_obj_ptr, int64_t obj);
void obj_perm_dup(int64_t* copy_obj_ptr, int64_t existing_obj);
void sub_406210(int64_t* copy, int64_t obj, ObjectID* oids);
void sub_4063A0(int64_t obj, ObjectID** oids_ptr, int* cnt_ptr);
void sub_4064B0(int64_t obj);
void sub_406520(int64_t obj);
bool obj_write(TigFile* stream, int64_t obj);
bool obj_read(TigFile* stream, int64_t* obj_ptr);
void obj_write_mem(uint8_t** data_ptr, int* size_ptr, int64_t obj);
bool obj_read_mem(uint8_t* data, int64_t* obj_ptr);
int obj_is_modified(int64_t obj);
bool obj_dif_write(TigFile* stream, int64_t obj);
bool obj_dif_read(TigFile* stream, int64_t obj);
void sub_406B80(int64_t obj);
int obj_field_int32_get(int64_t obj, int field);
void obj_field_int32_set(int64_t obj, int field, int value);
int64_t obj_field_int64_get(int64_t obj, int field);
void obj_field_int64_set(int64_t obj, int fld, int64_t value);
int64_t obj_field_handle_get(int64_t obj, int fld);
void obj_field_handle_set(int64_t obj, int fld, int64_t value);
bool obj_field_obj_get(int64_t obj, int fld, int64_t* value_ptr);
ObjectID sub_407100(int64_t obj, int fld);
void obj_field_string_get(int64_t obj, int fld, char** value_ptr);
void obj_field_string_set(int64_t obj, int fld, const char* value);
int obj_arrayfield_int32_get(int64_t obj, int fld, int index);
void obj_arrayfield_int32_set(int64_t obj, int fld, int index, int value);
unsigned int obj_arrayfield_uint32_get(int64_t obj, int fld, int index);
void obj_arrayfield_uint32_set(int64_t obj, int fld, int index, unsigned int value);
int64_t obj_arrayfield_int64_get(int64_t obj, int fld, int index);
void obj_arrayfield_int64_set(int64_t obj, int fld, int index, int64_t value);
int64_t obj_arrayfield_handle_get(int64_t obj, int fld, int index);
bool obj_arrayfield_obj_get(int64_t obj, int fld, int index, int64_t* value_ptr);
void obj_arrayfield_obj_set(int64_t obj, int fld, int index, int64_t value);
void obj_arrayfield_script_get(int64_t obj, int fld, int index, void* value);
void obj_arrayfield_script_set(int64_t obj, int fld, int index, void* value);
void obj_arrayfield_pc_quest_get(int64_t obj, int fld, int index, void* value);
void obj_arrayfield_pc_quest_set(int64_t obj, int fld, int index, void* value);
int obj_arrayfield_length_get(int64_t obj, int fld);
void obj_arrayfield_length_set(int64_t obj, int fld, int length);
void obj_arrayfield_pc_rumor_copy_to_flat(int64_t obj, int fld, int cnt, void* data);
void obj_arrayfield_pc_quest_copy_to_flat(int64_t obj, int fld, int cnt, void* data);
void obj_field_reset(int64_t obj, int fld);
ObjectID obj_get_id(int64_t obj);
ObjectID sub_408020(int64_t obj, int a2);
bool obj_inst_first(int64_t* obj_ptr, int* iter_ptr);
bool obj_inst_next(int64_t* obj_ptr, int* iter_ptr);
Object* obj_lock(int64_t obj);
void obj_unlock(int64_t obj);
int sub_40C030(ObjectType object_type);
bool object_field_valid(int type, int fld);
bool obj_enumerate_fields(Object* object, ObjEnumerateCallback* callback);
int64_t obj_get_prototype_handle(Object* object);

void* obj_field_ptr_get(int64_t obj, int field);
void obj_field_ptr_set(int64_t obj, int field, void* value);
void* obj_arrayfield_ptr_get(int64_t obj, int fld, int index);
void obj_arrayfield_ptr_set(int64_t obj, int fld, int index, void* value);

// NOTE: Seen in some assertions in `anim.c`.
static inline bool obj_type_is_critter(int type)
{
    return type == OBJ_TYPE_PC || type == OBJ_TYPE_NPC;
}

// If `obj_type_is_critter` exists, `obj_type_is_item` should also be there.
static inline bool obj_type_is_item(int type)
{
    return type >= OBJ_TYPE_WEAPON && type <= OBJ_TYPE_GENERIC;
}

// NOTE: Convenience.
static inline bool inventory_fields_from_obj_type(int type, int* inventory_num_fld, int* inventory_list_idx_fld)
{
    if (type == OBJ_TYPE_CONTAINER) {
        *inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        *inventory_list_idx_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
        return true;
    }

    if (obj_type_is_critter(type)) {
        *inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        *inventory_list_idx_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
        return true;
    }

    return false;
}

#endif /* ARCANUM_GAME_OBJ_H_ */
