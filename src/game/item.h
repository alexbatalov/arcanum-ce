#ifndef ARCANUM_GAME_ITEM_H_
#define ARCANUM_GAME_ITEM_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

#define ITEM_INV_LOC_HELMET 1000
#define ITEM_INV_LOC_RING1 1001
#define ITEM_INV_LOC_RING2 1002
#define ITEM_INV_LOC_MEDALLION 1003
#define ITEM_INV_LOC_WEAPON 1004
#define ITEM_INV_LOC_SHIELD 1005
#define ITEM_INV_LOC_ARMOR 1006
#define ITEM_INV_LOC_GAUNTLET 1007
#define ITEM_INV_LOC_BOOTS 1008

#define FIRST_WEAR_INV_LOC ITEM_INV_LOC_HELMET
#define LAST_WEAR_INV_LOC ITEM_INV_LOC_BOOTS
#define IS_WEAR_INV_LOC(x) ((x) >= FIRST_WEAR_INV_LOC && (x) <= LAST_WEAR_INV_LOC)
#define IS_HOTKEY_INV_LOC(x) ((x) >= 2000 && (x) <= 2009)

typedef enum ItemCannot {
    ITEM_CANNOT_OK,
    ITEM_CANNOT_TOO_HEAVY, // The item is too heavy.
    ITEM_CANNOT_NO_ROOM, // There is no room for that item.
    ITEM_CANNOT_WRONG_TYPE, // The item is the wrong type for that slot.
    ITEM_CANNOT_NO_FREE_HAND, // There is no free hand to use that item.
    ITEM_CANNOT_CRIPPLED, // A crippled arm prevents the wielding of that item.
    ITEM_CANNOT_NOT_USABLE, // The item cannot be used.
    ITEM_CANNOT_BROKEN, // The item is broken.
    ITEM_CANNOT_WRONG_WEARABLE_SIZE, // The clothing or armor is the wrong size and cannot be worn.
    ITEM_CANNOT_NOT_WIELDABLE, // The item cannot be wielded.
    ITEM_CANNOT_WRONG_WEARABLE_GENDER, // The clothing or armor is for the opposite gender and cannot be worn.
    ITEM_CANNOT_NOT_DROPPABLE, // The item cannot be dropped.
    ITEM_CANNOT_HEXED, // The item is hexed and cannot be unwielded.
    ITEM_CANNOT_DUMB, // The scroll requires an Intelligence of 5 or more to use.
    ITEM_CANNOT_PICKUP_MAGIC_ITEMS, // You cannot pick up magickal items.
    ITEM_CANNOT_PICKUP_TECH_ITEMS, // You cannot pick up technological items.
    ITEM_CANNOT_USE_MAGIC_ITEMS, // You cannot use magickal items.
    ITEM_CANNOT_USE_TECH_ITEMS, // You cannot use technological items.
    ITEM_CANNOT_WIELD_MAGIC_ITEMS, // You cannot wield magickal items.
    ITEM_CANNOT_WIELD_TECH_ITEMS, // You cannot wield technological items.
} ItemCannot;

bool item_init(GameInitInfo* init_info);
void item_exit();
void item_resize(GameResizeInfo* resize_info);
void item_generate_inventory(int64_t critter_obj);
bool item_parent(int64_t item_obj, int64_t* parent_obj_ptr);
bool item_is_item(int64_t obj);
int item_inventory_location_get(int64_t obj);
int item_weight(int64_t item_obj, int64_t owner_obj);
int item_total_weight(int64_t obj);
int item_effective_power(int64_t item_obj, int64_t parent_obj);
int item_magic_tech_complexity(int64_t item_obj);
int item_effective_power_ratio(int64_t item_obj, int64_t owner_obj);
int item_adjust_magic(int64_t item_obj, int64_t owner_obj, int value);
int sub_461620(int64_t item_obj, int64_t owner_obj, int64_t a3);
int item_aptitude_crit_failure_chance(int64_t item_obj, int64_t owner_obj);
void item_inv_icon_size(int64_t item_id, int* width, int* height);
bool item_transfer(int64_t item_obj, int64_t critter_obj);
bool item_transfer_ex(int64_t item_obj, int64_t critter_obj, int inventory_location);
bool item_drop(int64_t item_obj);
bool item_drop_nearby(int64_t item_obj);
bool item_drop_ex(int64_t item_obj, int distance);
int item_worth(int64_t item_id);
bool sub_461F60(int64_t item_id);
int item_cost(int64_t item_obj, int64_t seller_obj, int64_t buyer_obj, bool a4);
int item_throwing_distance(int64_t item_obj, int64_t critter_obj);
void item_damage_min_max(int64_t item_obj, int damage_type, int* min_damage, int* max_damage);
int sub_462410(int64_t item_id, int* quantity_field_ptr);
int64_t item_find_by_name(int64_t obj, int name);
int64_t sub_462540(int64_t parent_obj, int64_t item_prototype_obj, unsigned int flags);
int64_t item_find_first_of_type(int64_t obj, int type);
int64_t item_find_first_generic(int64_t obj, unsigned int flags);
int64_t item_find_first(int64_t obj);
int item_list_get(int64_t parent_obj, int64_t** items_ptr);
void item_list_free(int64_t* list);
int64_t item_find_first_matching_prototype(int64_t parent_obj, int64_t existing_item_obj);
int64_t sub_462A30(int64_t obj, int64_t a2);
int item_count_items_matching_prototype(int64_t obj, int64_t a2);
int sub_462C30(int64_t a1, int64_t a2);
void item_use_on_obj(int64_t source_obj, int64_t item_obj, int64_t target_obj);
void item_use_on_loc(int64_t obj, int64_t item_obj, int64_t target_loc);
int item_get_keys(int64_t obj, int* key_ids);
bool sub_463370(int64_t obj, int key_id);
bool sub_463540(int64_t container_obj);
void sub_463630(int64_t obj);
void sub_463730(int64_t obj, bool a2);
void sub_463860(int64_t obj, bool a2);
void sub_4639E0(int64_t obj, bool a2);
void sub_463B30(int64_t obj, bool a2);
void sub_463C60(int64_t obj);
void sub_463E20(int64_t obj);
void sub_4640C0(int64_t obj);
bool npc_respawn_timevent_process(TimeEvent* timeevent);
int item_inventory_source(int64_t obj);
bool sub_4642C0(int64_t obj, int64_t item_obj);
bool item_is_identified(int64_t obj);
void item_identify_all(int64_t obj);
void sub_464470(int64_t obj, int* a2, int* a3);
int item_total_attack(int64_t critter_obj);
int item_total_defence(int64_t critter_obj);
int item_gold_get(int64_t obj);
bool item_gold_transfer(int64_t from_obj, int64_t to_obj, int qty, int64_t gold_obj);
int64_t item_gold_create(int amount, int64_t loc);
int64_t item_wield_get(int64_t obj, int inventory_location);
bool item_wield_set(int64_t item_obj, int inventory_location);
bool sub_464C50(int64_t obj, int inventory_location);
bool sub_464C80(int64_t item_obj);
int sub_464D20(int64_t a1, int a2, int64_t a3);
tig_art_id_t sub_465020(int64_t obj);
void item_wield_best(int64_t critter_obj, int inventory_location, int64_t target_obj);
void item_wield_best_all(int64_t critter_obj, int64_t target_obj);
void item_rewield(int64_t obj);
bool item_in_rewield();
int item_location_get(int64_t obj);
int64_t sub_465690(int64_t obj, int inventory_location);
void item_location_set(int64_t obj, int location);
const char* ammunition_type_get_name(int ammo_type);
int item_ammo_quantity_get(int64_t obj, int ammo_type);
bool item_ammo_transfer(int64_t from_obj, int64_t to_obj, int qty, int ammo_type, int64_t ammo_obj);
int64_t item_ammo_create(int quantity, int ammo_type, int64_t obj);
int item_armor_ac_adj(int64_t item_obj, int64_t owner_obj, bool a3);
int item_armor_coverage(int64_t obj);
unsigned int item_armor_size(int race);
int item_weapon_ammo_type(int64_t item_id);
int item_weapon_magic_speed(int64_t item_obj, int64_t owner_obj);
int item_weapon_skill(int64_t obj);
int item_weapon_range(int64_t item_id, int64_t critter_id);
int item_weapon_min_strength(int64_t item_obj, int64_t critter_obj);
void item_weapon_damage(int64_t weapon_obj, int64_t critter_obj, int damage_type, int skill, bool a5, int* min_dam_ptr, int* max_dam_ptr);
int item_weapon_aoe_radius(int64_t obj);
void item_inventory_slots_get(int64_t obj, int* slots);
void item_inventory_slots_set(int64_t item_obj, int inventory_location, int* slots, int value);
bool item_inventory_slots_has_room_for(int64_t item_obj, int64_t parent_obj, int inventory_location, int* slots);
int item_find_free_inv_loc_for_insertion(int64_t item_obj, int64_t parent_obj);
int item_check_insert(int64_t item_obj, int64_t parent_obj, int* inventory_location_ptr);
void item_insert(int64_t item_obj, int64_t parent_obj, int inventory_location);
void sub_466D60(int64_t obj);
int item_check_remove(int64_t obj);
void item_remove(int64_t item_obj);
void sub_466E50(int64_t obj, int64_t loc);
void item_arrange_inventory(int64_t parent_obj, bool vertical);
void item_error_msg(int64_t obj, int reason);
void item_perform_identify_service(int64_t item_obj, int64_t npc_obj, int64_t pc_obj, int cost);
void sub_467520(int64_t obj);
bool item_decay_timeevent_process(TimeEvent* timeevent);
bool item_can_decay(int64_t obj);
void item_force_remove(int64_t item_obj, int64_t parent_obj);
bool item_decay(int64_t obj, int ms);
void item_decay_process_enable();
void item_decay_process_disable();
int item_decay_process_is_enabled();

#endif /* ARCANUM_GAME_ITEM_H_ */
