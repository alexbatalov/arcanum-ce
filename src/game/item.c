#include "game/item.h"

#include "game/anim.h"
#include "game/background.h"
#include "game/critter.h"
#include "game/description.h"
#include "game/descriptions.h"
#include "game/invensource.h"
#include "game/magictech.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/mt_ai.h"
#include "game/mt_item.h"
#include "game/multiplayer.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/party.h"
#include "game/player.h"
#include "game/proto.h"
#include "game/random.h"
#include "game/reaction.h"
#include "game/resistance.h"
#include "game/script.h"
#include "game/skill.h"
#include "game/spell.h"
#include "game/stat.h"
#include "game/tf.h"
#include "game/timeevent.h"
#include "game/trap.h"
#include "game/ui.h"

#define FIRST_AMMUNITION_TYPE_ID 0
#define FIRST_ARMOR_COVERAGE_TYPE_ID (FIRST_AMMUNITION_TYPE_ID + TIG_ART_AMMO_TYPE_COUNT)

typedef struct ItemInsertInfo {
    /* 0000 */ int64_t item_obj;
    /* 0008 */ int64_t parent_obj;
    /* 0010 */ int inventory_location;
} ItemInsertInfo;

typedef struct ItemRemoveInfo {
    /* 0000 */ int64_t item_obj;
    /* 0008 */ int64_t parent_obj;
} ItemRemoveInfo;

static bool sub_461CA0(int64_t item_obj, int64_t critter_obj, int inventory_location);
static bool item_check_sell(int64_t item_obj, int64_t seller_pc_obj, int64_t buyer_npc_obj);
static bool item_check_buy(int64_t item_obj, int64_t seller_npc_obj, int64_t buyer_pc_obj);
static bool sub_463240(int64_t critter_obj, int lock_id);
static bool sub_463340(int lock_id, int key_id);
static bool sub_464150(TimeEvent* timeevent);
static int64_t item_gold_obj(int64_t obj);
static int64_t item_find_key_ring(int64_t critter_obj);
static bool item_check_invensource_buy_list(int64_t item_obj, int64_t buyer_npc_obj);
static int sub_465010(int64_t obj);
static tig_art_id_t sub_4650D0(int64_t critter_obj);
static int64_t item_ammo_obj(int64_t obj, int ammo_type);
static bool sub_465AE0(int64_t a1, int64_t a2, tig_art_id_t* art_id_ptr);
static bool sub_466A00(int64_t a1, int64_t key_obj);
static void sub_466A50(int64_t key_obj, int64_t key_ring_obj);
static void sub_466AA0(int64_t critter_obj, int64_t a2);
static void sub_466BD0(int64_t key_ring_obj);
static bool item_insert_success(void* userinfo);
static bool item_insert_failure(void* userinfo);
static bool sub_466EF0(int64_t obj, int64_t loc);
static char* item_error_str(int reason);
static int find_free_inv_loc_horizontal(int64_t item_obj, int64_t parent_obj, int* slots);
static int find_free_inv_loc_vertical(int64_t item_obj, int64_t parent_obj, int* slots);
static void item_equipped(int64_t item_obj, int64_t parent_obj, int inventory_location);
static void item_unequipped(int64_t item_obj, int64_t parent_obj, int inventory_location);
static bool item_force_remove_success(void* userinfo);
static bool item_force_remove_failure(void* userinfo);
static bool sub_467E70();
static void item_recalc_light(int64_t item_obj, int64_t parent_obj);
static bool item_cancel_decay(int64_t obj);
static bool item_decay_timeevent_check(TimeEvent* timeevent);

// 0x5B32A0
static int dword_5B32A0[TIG_ART_AMMO_TYPE_COUNT] = {
    /*  TIG_ART_AMMO_TYPE_ARROW */ OBJ_F_CRITTER_ARROWS,
    /* TIG_ART_AMMO_TYPE_BULLET */ OBJ_F_CRITTER_BULLETS,
    /* TIG_ART_AMMO_TYPE_CHARGE */ OBJ_F_CRITTER_POWER_CELLS,
    /*   TIG_ART_AMMO_TYPE_FUEL */ OBJ_F_CRITTER_FUEL,
};

// 0x5B32B0
static int dword_5B32B0[TIG_ART_AMMO_TYPE_COUNT] = {
    /*  TIG_ART_AMMO_TYPE_ARROW */ BP_ARROW,
    /* TIG_ART_AMMO_TYPE_BULLET */ BP_BULLET,
    /* TIG_ART_AMMO_TYPE_CHARGE */ BP_BATTERY,
    /*   TIG_ART_AMMO_TYPE_FUEL */ BP_FUEL,
};

// 0x5B32C0
static unsigned int item_race_to_armor_size_tbl[RACE_COUNT] = {
    /*     RACE_HUMAN */ OARF_SIZE_MEDIUM,
    /*     RACE_DWARF */ OARF_SIZE_SMALL,
    /*       RACE_ELF */ OARF_SIZE_MEDIUM,
    /*  RACE_HALF_ELF */ OARF_SIZE_MEDIUM,
    /*     RACE_GNOME */ OARF_SIZE_SMALL,
    /*  RACE_HALFLING */ OARF_SIZE_SMALL,
    /*  RACE_HALF_ORC */ OARF_SIZE_MEDIUM,
    /* RACE_HALF_OGRE */ OARF_SIZE_LARGE,
    /*  RACE_DARK_ELF */ OARF_SIZE_MEDIUM,
    /*      RACE_OGRE */ OARF_SIZE_LARGE,
    /*       RACE_ORC */ OARF_SIZE_MEDIUM,
};

// 0x5B32EC
static int dword_5B32EC = 15;

// 0x5B32F0
static int dword_5B32F0[] = {
    12,
    12,
    18,
    15,
    12,
};

// 0x5B3304
static int dword_5B3304 = 24;

// 0x5B3308
static int dword_5B3308[RESISTANCE_TYPE_COUNT] = {
    /*     RESISTANCE_TYPE_NORMAL */ 25,
    /*       RESISTANCE_TYPE_FIRE */ 15,
    /* RESISTANCE_TYPE_ELECTRICAL */ 10,
    /*     RESISTANCE_TYPE_POISON */ 16,
    /*      RESISTANCE_TYPE_MAGIC */ 15,
};

// 0x5E87E0
static char** item_ammunition_type_names;

// 0x5E87E4
static bool item_editor;

// 0x5E87E8
static bool in_rewield;

// 0x5E87F0
static int64_t item_decay_test_obj;

// 0x5E87F8
static char** item_armor_coverage_type_names;

// 0x5E87FC
static mes_file_handle_t item_mes_file;

// 0x5E8800
static int item_decay_process_cnt;

// 0x5E8808
static TigRect item_iso_content_rect;

// 0x5E8818
static int64_t qword_5E8818;

// 0x5E8820
static bool dword_5E8820;

// 0x460E70
bool item_init(GameInitInfo* init_info)
{
    TigWindowData window_data;
    MesFileEntry msg;
    int index;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    item_iso_content_rect.x = 0;
    item_iso_content_rect.y = 0;
    item_iso_content_rect.width = window_data.rect.width;
    item_iso_content_rect.height = window_data.rect.height;

    item_editor = init_info->editor;

    if (!mes_load("mes\\item.mes", &item_mes_file)) {
        return false;
    }

    item_ammunition_type_names = (char**)CALLOC(TIG_ART_AMMO_TYPE_COUNT, sizeof(*item_ammunition_type_names));
    item_armor_coverage_type_names = (char**)CALLOC(TIG_ART_ARMOR_COVERAGE_COUNT, sizeof(*item_armor_coverage_type_names));

    for (index = 0; index < TIG_ART_AMMO_TYPE_COUNT; index++) {
        msg.num = index + FIRST_AMMUNITION_TYPE_ID;
        mes_get_msg(item_mes_file, &msg);
        item_ammunition_type_names[index] = msg.str;
    }

    for (index = 0; index < TIG_ART_ARMOR_COVERAGE_COUNT; index++) {
        msg.num = index + FIRST_ARMOR_COVERAGE_TYPE_ID;
        mes_get_msg(item_mes_file, &msg);
        item_armor_coverage_type_names[index] = msg.str;
    }

    in_rewield = 0;
    dword_5E8820 = 0;
    item_decay_process_cnt = 1;

    return true;
}

// 0x460F90
void item_exit()
{
    FREE(item_ammunition_type_names);
    FREE(item_armor_coverage_type_names);
    mes_unload(item_mes_file);
}

// 0x460FC0
void item_resize(GameResizeInfo* resize_info)
{
    item_iso_content_rect = resize_info->content_rect;
}

// 0x460FF0
void item_generate_inventory(int64_t critter_obj)
{
    int64_t loc;
    int cnt;
    int idx;
    int64_t item_obj;
    int race;
    int gender;
    int64_t proto_obj;

    loc = obj_field_int64_get(critter_obj, OBJ_F_LOCATION);
    sub_463B30(critter_obj, false);
    background_generate_inventory(critter_obj);

    cnt = obj_field_int32_get(critter_obj, OBJ_F_CRITTER_INVENTORY_NUM);
    for (idx = 0; idx < cnt; idx++) {
        item_obj = obj_arrayfield_handle_get(critter_obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, idx);
        if (item_location_get(item_obj) == ITEM_INV_LOC_ARMOR) {
            break;
        }
    }

    if (idx == cnt) {
        race = stat_level_get(critter_obj, STAT_RACE);
        gender = stat_level_get(critter_obj, STAT_GENDER);
        if (gender == GENDER_FEMALE) {
            switch (item_armor_size(race)) {
            case OARF_SIZE_SMALL:
                // "Small Ladies City Dweller Clothes"
                proto_obj = sub_4685A0(BP_SMALL_LADIES_CITY_DWELLER_CLOTHES);
                break;
            case OARF_SIZE_MEDIUM:
                // "Plain Dress"
                proto_obj = sub_4685A0(BP_PLAIN_DRESS);
                break;
            case OARF_SIZE_LARGE:
                // "Small Ladies City Dweller Clothes""
                proto_obj = sub_4685A0(BP_LARGE_LADIES_CITY_DWELLER_CLOTHES);
                break;
            default:
                // Should be unreachable.
                assert(0);
            }
        } else {
            switch (item_armor_size(race)) {
            case OARF_SIZE_SMALL:
                // "Small Nice Suit"
                proto_obj = sub_4685A0(BP_SMALL_NICE_SUIT);
                break;
            case OARF_SIZE_MEDIUM:
                // "Nice Suit"
                proto_obj = sub_4685A0(BP_NICE_SUIT);
                break;
            case OARF_SIZE_LARGE:
                // "Large Nice Suit"
                proto_obj = sub_4685A0(BP_LARGE_NICE_SUIT);
                break;
            default:
                // Should be unreachable.
                assert(0);
            }
        }

        if (object_create(proto_obj, loc, &item_obj)) {
            item_transfer(item_obj, critter_obj);

            if (sub_464D20(item_obj, ITEM_INV_LOC_ARMOR, critter_obj)) {
                object_destroy(item_obj);

                switch (item_armor_size(race)) {
                case OARF_SIZE_SMALL:
                    // "Small Jacket"
                    proto_obj = sub_4685A0(BP_SMALL_JACKET);
                    break;
                case OARF_SIZE_MEDIUM:
                    // "Jacket"
                    proto_obj = sub_4685A0(BP_JACKET);
                    break;
                case OARF_SIZE_LARGE:
                    // "Rags"
                    proto_obj = sub_4685A0(BP_LARGE_JACKET);
                    break;
                default:
                    // Should be unreachable.
                    assert(0);
                }

                if (object_create(proto_obj, loc, &item_obj)) {
                    item_transfer(item_obj, critter_obj);
                }
            }
        }
    }

    if (tech_skill_points_get(critter_obj, TECH_SKILL_PICK_LOCKS) > 0) {
        // "Crude Lockpicks"
        proto_obj = sub_4685A0(BP_CRUDE_LOCKPICKS);
        if (object_create(proto_obj, loc, &item_obj)) {
            item_transfer(item_obj, critter_obj);
        }
    }

    if (basic_skill_points_get(critter_obj, BASIC_SKILL_HEAL) > 0) {
        // "Bandages"
        proto_obj = sub_4685A0(BP_BANDAGES);
        if (object_create(proto_obj, loc, &item_obj)) {
            item_transfer(item_obj, critter_obj);
        }
    }
}

// 0x4612A0
bool item_parent(int64_t item_obj, int64_t* parent_obj_ptr)
{
    if (!item_is_item(item_obj)) {
        tig_debug_printf("Item: item_parent: ERROR: Called on non-Item!\n");
        return false;
    }

    if (parent_obj_ptr != NULL) {
        *parent_obj_ptr = OBJ_HANDLE_NULL;
    }

    if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_INVENTORY) == 0) {
        return false;
    }

    if (parent_obj_ptr != NULL) {
        *parent_obj_ptr = obj_field_handle_get(item_obj, OBJ_F_ITEM_PARENT);
    }

    return true;
}

// 0x461310
bool item_is_item(int64_t obj)
{
    int type;

    if (obj != OBJ_HANDLE_NULL) {
        type = obj_field_int32_get(obj, OBJ_F_TYPE);
        if (obj_type_is_item(type)) {
            return true;
        }
    }

    return false;
}

// 0x461340
int item_inventory_location_get(int64_t obj)
{
    // FIXME: Result is not used.
    item_parent(obj, NULL);
    return obj_field_int32_get(obj, OBJ_F_ITEM_INV_LOCATION);
}

// 0x461370
int item_weight(int64_t item_obj, int64_t owner_obj)
{
    int weight;
    int weight_adj;
    int quantity_fld;

    if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_GOLD) {
        return 0;
    }

    weight = obj_field_int32_get(item_obj, OBJ_F_ITEM_WEIGHT);

    if (owner_obj != OBJ_HANDLE_NULL) {
        weight_adj = obj_field_int32_get(item_obj, OBJ_F_ITEM_MAGIC_WEIGHT_ADJ);
        weight += item_adjust_magic(item_obj, owner_obj, weight_adj);
    }

    if (sub_462410(item_obj, &quantity_fld) != -1) {
        weight *= obj_field_int32_get(item_obj, quantity_fld) / 4;
    }

    return weight;
}

// 0x461410
int item_total_weight(int64_t obj)
{
    int inventory_num_field;
    int inventory_list_idx_field;
    int index;
    int count;
    int weight;
    int64_t item_obj;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_field = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_idx_field = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_field = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_idx_field = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    count = obj_field_int32_get(obj, inventory_num_field);
    weight = 0;
    for (index = 0; index < count; index++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_idx_field, index);
        weight += item_weight(item_obj, obj);
    }

    return weight;
}

// 0x4614A0
int item_effective_power(int64_t item_obj, int64_t parent_obj)
{
    int complexity;
    int effective_power;

    complexity = item_magic_tech_complexity(item_obj);

    if (!obj_type_is_critter(obj_field_int32_get(parent_obj, OBJ_F_TYPE))) {
        return complexity;
    }

    effective_power = (complexity + stat_level_get(parent_obj, STAT_MAGICK_TECH_APTITUDE)) / 2;
    if (complexity < 0) {
        // Item is technological, check if the owner is too much magician to use
        // this item.
        if (effective_power > 0) {
            return 0;
        }

        // Clamp effective technical power (both numbers are negative).
        if (effective_power < complexity) {
            return complexity;
        } else {
            return effective_power;
        }
    } else {
        // Item is magickal, check if the owner is too much technician to use
        // this item.
        if (effective_power < 0) {
            return 0;
        }

        // Clamp effective magical power (both numbers are positive).
        if (effective_power > complexity) {
            return complexity;
        } else {
            return effective_power;
        }
    }
}

// 0x461520
int item_magic_tech_complexity(int64_t item_obj)
{
    return obj_field_int32_get(item_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY);
}

// 0x461540
int item_effective_power_ratio(int64_t item_obj, int64_t owner_obj)
{
    int complexity;

    complexity = item_magic_tech_complexity(item_obj);

    // Check if item is neutral.
    if (complexity == 0) {
        return 100;
    }

    // Calculate effective power ratio.
    return 100 * item_effective_power(item_obj, owner_obj) / complexity;
}

// 0x461590
int item_adjust_magic(int64_t item_obj, int64_t owner_obj, int value)
{
    int complexity;

    complexity = item_magic_tech_complexity(item_obj);
    if (complexity > 0) {
        // The item is magickal, so the magic effect value directly depends on
        // how effectively the owner can utilize its magical properties.
        return value * item_effective_power(item_obj, owner_obj) / complexity;
    } else if (complexity < 0) {
        // Item is technological, so the magic effect value is reduced based on
        // how effectively the owner can use its technological properties. If
        // the owner is 100% effective with the item, the magic effect is
        // reduced to 0.
        return value - value * item_effective_power(item_obj, owner_obj) / complexity;
    } else {
        // The item is neutral, no adjustment is made to the value.
        return value;
    }
}

// 0x461620
int sub_461620(int64_t item_obj, int64_t owner_obj, int64_t a3)
{
    int complexity;
    int aptitude1;
    int aptitude2;

    complexity = item_magic_tech_complexity(item_obj);

    if (!obj_type_is_critter(obj_field_int32_get(owner_obj, OBJ_F_TYPE))) {
        return 0;
    }
    aptitude1 = stat_level_get(owner_obj, STAT_MAGICK_TECH_APTITUDE);

    if (!obj_type_is_critter(obj_field_int32_get(a3, OBJ_F_TYPE))) {
        return 0;
    }
    aptitude2 = stat_level_get(a3, STAT_MAGICK_TECH_APTITUDE);

    if (complexity > 0) {
        if (aptitude2 < 0) {
            if (aptitude1 < 0) {
                return -aptitude2;
            } else {
                return -aptitude2 * (100 - aptitude1) / 100;
            }
        }
    } else if (complexity < 0) {
        if (aptitude2 > 0) {
            if (aptitude1 > 0) {
                return aptitude2;
            } else {
                return aptitude2 * (100 + aptitude1) / 100;
            }
        }
    }

    return 0;
}

// 0x461700
int item_aptitude_crit_failure_chance(int64_t item_obj, int64_t owner_obj)
{
    int complexity;
    int aptitude;

    complexity = item_magic_tech_complexity(item_obj);

    // This penalty only applies to technological items (complexity < 0).
    if (complexity >= 0) {
        return 0;
    }

    // Make sure the owner is a critter. Otherwise there is no enough context
    // for proper calculations.
    if (!obj_type_is_critter(obj_field_int32_get(owner_obj, OBJ_F_TYPE))) {
        return 0;
    }

    aptitude = stat_level_get(owner_obj, STAT_MAGICK_TECH_APTITUDE);

    // Check if the owner gravitates towards technology (or at least neutral),
    // in this case there is no penalty.
    if (aptitude <= 0) {
        return 0;
    }

    // Calculate the penalty based the owner's profiency in magick: the better
    // the magician, the higher the chance of a critical failure (up to the
    // techinical complexity of the item itself).
    return -complexity * aptitude / 100;
}

// 0x461780
void item_inv_icon_size(int64_t item_id, int* width, int* height)
{
    tig_art_id_t aid;
    TigArtFrameData art_frame_data;

    aid = obj_field_int32_get(item_id, OBJ_F_ITEM_INV_AID);

    if (tig_art_frame_data(aid, &art_frame_data) == TIG_OK) {
        if (width != NULL) {
            *width = (art_frame_data.width - 1) / 32 + 1;
        }

        if (height != NULL) {
            *height = (art_frame_data.height - 1) / 32 + 1;
        }
    } else {
        if (width != NULL) {
            *width = 1;
        }

        if (height != NULL) {
            *height = 1;
        }
    }
}

// 0x4617F0
bool item_transfer(int64_t item_obj, int64_t critter_obj)
{
    return item_transfer_ex(item_obj, critter_obj, -1);
}

// 0x461810
bool item_transfer_ex(int64_t item_obj, int64_t critter_obj, int inventory_location)
{
    int64_t existing_item_obj;
    bool v1;
    int rc;
    int new_inventory_location;
    int slots[960];

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        Packet28 pkt;

        pkt.type = 28;
        sub_4F0640(item_obj, &(pkt.item_oid));
        sub_4F0640(critter_obj, &(pkt.critter_oid));
        pkt.inventory_location = inventory_location;
        tig_net_send_app_all(&pkt, sizeof(pkt));

        return true;
    }

    if (item_parent(item_obj, NULL)) {
        return sub_461CA0(item_obj, critter_obj, inventory_location);
    }

    if (!object_script_execute(critter_obj, item_obj, OBJ_HANDLE_NULL, SAP_GET, 0)) {
        return false;
    }

    existing_item_obj = OBJ_HANDLE_NULL;
    v1 = false;

    if (inventory_location != -1
        && IS_WEAR_INV_LOC(inventory_location)) {
        existing_item_obj = item_wield_get(critter_obj, inventory_location);
        v1 = true;
    }

    rc = item_check_insert(item_obj, critter_obj, &new_inventory_location);
    if (rc != ITEM_CANNOT_OK
        && (rc != ITEM_CANNOT_NO_ROOM
            || existing_item_obj != OBJ_HANDLE_NULL
            || !v1)) {
        switch (obj_field_int32_get(critter_obj, OBJ_F_TYPE)) {
        case OBJ_TYPE_PC:
            item_error_msg(critter_obj, rc);
            break;
        case OBJ_TYPE_NPC:
            tf_add(critter_obj, TF_TYPE_WHITE, item_error_str(rc));
            break;
        }
        return false;
    }

    object_pickup(item_obj, critter_obj);

    if (inventory_location != -1) {
        if (!v1) {
            item_inventory_slots_get(critter_obj, slots);
            if (item_inventory_slots_has_room_for(item_obj, critter_obj, inventory_location, slots)) {
                item_insert(item_obj, critter_obj, inventory_location);
            } else {
                item_insert(item_obj, critter_obj, new_inventory_location);
            }
        } else {
            if (existing_item_obj == OBJ_HANDLE_NULL) {
                item_insert(item_obj, critter_obj, inventory_location);
            } else {
                item_insert(item_obj, critter_obj, new_inventory_location);
            }
        }
    } else {
        item_insert(item_obj, critter_obj, new_inventory_location);
    }

    item_cancel_decay(item_obj);

    return true;
}

// 0x461A70
bool item_drop(int64_t item_obj)
{
    return item_drop_ex(item_obj, 0);
}

// 0x461A90
bool item_drop_nearby(int64_t item_obj)
{
    return item_drop_ex(item_obj, random_between(2, 4));
}

// 0x461AB0
bool item_drop_ex(int64_t item_obj, int distance)
{
    int64_t parent_obj;
    int reason;
    int obj_type;
    int64_t loc;
    int64_t candidate_loc;
    int attempt;
    unsigned int flags;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        Packet26 pkt;

        pkt.type = 26;
        sub_4F0640(item_obj, &(pkt.oid));
        pkt.field_20 = distance;
        tig_net_send_app_all(&pkt, sizeof(pkt));
        return true;
    }

    item_parent(item_obj, &parent_obj);

    if (object_script_execute(parent_obj, item_obj, OBJ_HANDLE_NULL, SAP_DROP, 0) == 0) {
        return false;
    }

    reason = item_check_remove(item_obj);
    obj_type = obj_field_int32_get(parent_obj, OBJ_F_TYPE);

    if (reason != ITEM_CANNOT_OK) {
        if (obj_type == OBJ_TYPE_PC) {
            item_error_msg(parent_obj, reason);
        }
        return false;
    }

    loc = obj_field_int64_get(parent_obj, OBJ_F_LOCATION);

    if (distance > 0) {
        for (attempt = 0; attempt < 10; attempt++) {
            if (sub_4F4E40(parent_obj, distance, &candidate_loc)) {
                loc = candidate_loc;
                break;
            }
        }
    }

    item_remove(item_obj);
    sub_466E50(item_obj, loc);

    if (obj_type == OBJ_TYPE_NPC) {
        switch (obj_field_int32_get(item_obj, OBJ_F_TYPE)) {
        case OBJ_TYPE_WEAPON:
            flags = obj_field_int32_get(parent_obj, OBJ_F_NPC_FLAGS);
            flags |= ONF_LOOK_FOR_WEAPON;
            obj_field_int32_set(parent_obj, OBJ_F_NPC_FLAGS, flags);
            break;
        case OBJ_TYPE_ARMOR:
            flags = obj_field_int32_get(parent_obj, OBJ_F_NPC_FLAGS);
            flags |= ONF_LOOK_FOR_ARMOR;
            obj_field_int32_set(parent_obj, OBJ_F_NPC_FLAGS, flags);
            break;
        }
    }

    return true;
}

// 0x461CA0
bool sub_461CA0(int64_t item_obj, int64_t critter_obj, int inventory_location)
{
    int64_t parent_obj;
    int64_t existing_item_obj;
    bool v1;
    int rc;
    int new_inventory_location;
    int slots[960];

    if (!item_parent(item_obj, &parent_obj)) {
        return item_transfer_ex(item_obj, critter_obj, inventory_location);
    }

    if (!object_script_execute(parent_obj, critter_obj, item_obj, SAP_TRANSFER, 0)) {
        return false;
    }

    existing_item_obj = OBJ_HANDLE_NULL;
    v1 = false;

    if (inventory_location != -1
        && IS_WEAR_INV_LOC(inventory_location)) {
        existing_item_obj = item_wield_get(critter_obj, inventory_location);
        v1 = true;
    }

    rc = item_check_insert(item_obj, critter_obj, &new_inventory_location);
    if (rc != ITEM_CANNOT_OK
        && (rc != ITEM_CANNOT_NO_ROOM
            || existing_item_obj != OBJ_HANDLE_NULL
            || !v1)) {
        if (obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            item_error_msg(parent_obj, rc);
        }

        if (obj_field_int32_get(critter_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            item_error_msg(critter_obj, rc);
        }

        return false;
    }

    item_remove(item_obj);

    if (inventory_location != -1) {
        if (!v1) {
            item_inventory_slots_get(critter_obj, slots);
            if (item_inventory_slots_has_room_for(item_obj, critter_obj, inventory_location, slots)) {
                item_insert(item_obj, critter_obj, inventory_location);
            } else {
                item_insert(item_obj, critter_obj, new_inventory_location);
            }
        } else {
            if (existing_item_obj == OBJ_HANDLE_NULL) {
                item_insert(item_obj, critter_obj, inventory_location);
            } else {
                item_insert(item_obj, critter_obj, new_inventory_location);
            }
        }
    } else {
        item_insert(item_obj, critter_obj, new_inventory_location);
    }

    item_cancel_decay(item_obj);
    item_decay(item_obj, 172800000);

    return true;
}

// 0x461F20
int item_worth(int64_t item_id)
{
    int worth;

    if (!item_is_identified(item_id)) {
        return 300;
    }

    worth = obj_field_int32_get(item_id, OBJ_F_ITEM_WORTH);
    if (worth < 2) {
        worth = 2;
    }

    return worth;
}

// 0x461F60
bool sub_461F60(int64_t item_id)
{
    return obj_field_int32_get(item_id, OBJ_F_ITEM_WORTH) == 0;
}

// 0x461F80
int item_cost(int64_t item_obj, int64_t seller_obj, int64_t buyer_obj, bool a4)
{
    int worth;
    int mult;
    int cost;
    int haggle;
    int hp_current;
    int hp_max;

    if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_GOLD) {
        if (a4) {
            return item_gold_get(item_obj);
        } else {
            return 0;
        }
    }

    worth = item_worth(item_obj);
    if (player_is_pc_obj(seller_obj)) {
        if (!a4) {
            if (!item_check_sell(item_obj, seller_obj, buyer_obj)) {
                return 0;
            }
        }

        mult = obj_field_int32_get(buyer_obj, OBJ_F_NPC_RETAIL_PRICE_MULTIPLIER);
        if (mult > 100) {
            worth = worth * (3 * (100 - mult) / 8 + 100) / 100;
        }

        haggle = basic_skill_effectiveness(seller_obj, BASIC_SKILL_HAGGLE, item_obj);
        cost = worth * (haggle / 2 + 50) / 100;
        hp_current = object_hp_current(item_obj);
        hp_max = object_hp_max(item_obj);

        if (hp_current < hp_max) {
            cost = cost * hp_current / hp_max;
        }

        if (cost < 0) {
            return 0;
        }

        if (cost == 1) {
            return 2;
        }
    } else {
        if (!a4) {
            if (!item_check_buy(item_obj, seller_obj, buyer_obj)) {
                return 0;
            }
        }

        mult = obj_field_int32_get(seller_obj, OBJ_F_NPC_RETAIL_PRICE_MULTIPLIER);
        haggle = basic_skill_effectiveness(buyer_obj, BASIC_SKILL_HAGGLE, item_obj);
        cost = sub_4C1150(seller_obj, buyer_obj, worth + worth * mult * (100 - haggle) / 10000);

        if (cost <= worth) {
            cost = worth + 1;
        }

        if (cost == 1 || cost == 2) {
            cost = 3;
        }
    }

    return cost;
}

// 0x462170
bool item_check_sell(int64_t item_obj, int64_t seller_pc_obj, int64_t buyer_npc_obj)
{
    if (sub_461F60(item_obj)) {
        return false;
    }

    if (tig_art_item_id_destroyed_get(obj_field_int32_get(item_obj, OBJ_F_CURRENT_AID))) {
        return false;
    }

    if (basic_skill_training_get(seller_pc_obj, BASIC_SKILL_HAGGLE) < TRAINING_EXPERT) {
        if (!object_script_execute(item_obj, buyer_npc_obj, seller_pc_obj, SAP_BUY_OBJECT, 0)) {
            return false;
        }

        if (!item_check_invensource_buy_list(item_obj, buyer_npc_obj)) {
            return false;
        }
    }

    if ((obj_field_int32_get(buyer_npc_obj, OBJ_F_NPC_FLAGS) & ONF_FENCE) == 0) {
        if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_STOLEN) != 0) {
            return false;
        }
    }

    return true;
}

// 0x462230
bool item_check_buy(int64_t item_obj, int64_t seller_npc_obj, int64_t buyer_pc_obj)
{
    (void)seller_npc_obj;

    if (basic_skill_training_get(buyer_pc_obj, BASIC_SKILL_HAGGLE) < TRAINING_MASTER) {
        if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_WONT_SELL) != 0) {
            return false;
        }

        if (IS_WEAR_INV_LOC(item_inventory_location_get(item_obj))) {
            return false;
        }
    }

    return true;
}

// 0x4622A0
int item_throwing_distance(int64_t item_obj, int64_t critter_obj)
{
    int distance;
    int weight;

    distance = 50 * stat_level_get(critter_obj, STAT_STRENGTH);
    if (basic_skill_training_get(critter_obj, BASIC_SKILL_THROWING) >= TRAINING_EXPERT) {
        distance += distance / 2;
    }

    weight = item_weight(item_obj, critter_obj) / 10;
    if (weight < 1) {
        weight = 1;
    }

    distance /= weight;
    distance /= 5;
    if (distance < 1) {
        distance = 1;
    }

    return distance;
}

// 0x462330
void item_damage_min_max(int64_t item_obj, int damage_type, int* min_damage, int* max_damage)
{
    int64_t parent_obj;
    int weight;

    if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON
        && (obj_field_int32_get(item_obj, OBJ_F_WEAPON_FLAGS) & OWF_THROWABLE) != 0) {
        *min_damage = obj_arrayfield_int32_get(item_obj, OBJ_F_WEAPON_DAMAGE_LOWER_IDX, damage_type);
        *max_damage = obj_arrayfield_int32_get(item_obj, OBJ_F_WEAPON_DAMAGE_UPPER_IDX, damage_type);
        return;
    }

    if (damage_type == DAMAGE_TYPE_NORMAL) {
        parent_obj = OBJ_HANDLE_NULL;
        item_parent(item_obj, &parent_obj);

        weight = item_weight(item_obj, parent_obj);

        *min_damage = (weight + 99) / 100;
        *max_damage = (weight + 49) / 50;
        return;
    }

    *min_damage = 0;
    *max_damage = 0;
}

// 0x462410
int sub_462410(int64_t item_id, int* quantity_field_ptr)
{
    // TODO: Rename.
    int rc = -1;
    int quantity_field;
    int ammo_type;

    switch (obj_field_int32_get(item_id, OBJ_F_TYPE)) {
    case OBJ_TYPE_GOLD:
        rc = OBJ_F_CRITTER_GOLD;
        quantity_field = OBJ_F_GOLD_QUANTITY;
        break;
    case OBJ_TYPE_AMMO:
        ammo_type = obj_field_int32_get(item_id, OBJ_F_AMMO_TYPE);
        rc = dword_5B32A0[ammo_type];
        quantity_field = OBJ_F_AMMO_QUANTITY;
        break;
    default:
        // NOTE: Original code leave it uninitialized.
        quantity_field = -1;
        break;
    }

    if (quantity_field_ptr != NULL) {
        *quantity_field_ptr = quantity_field;
    }

    return rc;
}

// 0x462480
int64_t item_find_by_name(int64_t obj, int name)
{
    int obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int index;
    int64_t item_obj;

    if (obj == OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else if (obj_type_is_critter(obj_type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else {
        return OBJ_HANDLE_NULL;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, index);
        if (obj_field_int32_get(item_obj, OBJ_F_NAME) == name) {
            return item_obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x462540
int64_t sub_462540(int64_t parent_obj, int64_t item_prototype_obj, unsigned int flags)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int idx;
    int64_t candidate_obj = OBJ_HANDLE_NULL;
    int64_t item_obj;
    int inventory_location;

    if (obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    cnt = obj_field_int32_get(parent_obj, inventory_num_fld);
    for (idx = 0; idx < cnt; idx++) {
        item_obj = obj_arrayfield_handle_get(parent_obj, inventory_list_fld, idx);
        inventory_location = item_inventory_location_get(item_obj);

        if ((flags & 0x01) != 0) {
            if (IS_WEAR_INV_LOC(inventory_location)) {
                continue;
            }
        }

        if ((flags & 0x02) != 0) {
            if (tig_art_item_id_destroyed_get(obj_field_int32_get(item_obj, OBJ_F_CURRENT_AID)) != 0) {
                continue;
            }
        }

        if (obj_field_handle_get(item_obj, OBJ_F_PROTOTYPE_HANDLE) != item_prototype_obj) {
            continue;
        }

        if ((flags & 0x04) != 0) {
            if (obj_field_int32_get(item_obj, OBJ_F_DESCRIPTION) != obj_field_int32_get(item_prototype_obj, OBJ_F_DESCRIPTION)) {
                continue;
            }
        }

        if (inventory_location < 2000 || inventory_location > 2009) {
            return item_obj;
        }

        candidate_obj = item_obj;
    }

    return candidate_obj;
}

// 0x4626B0
int64_t item_find_first_of_type(int64_t obj, int type)
{
    int obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int index;
    int64_t item_obj;

    if (obj == OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else if (obj_type_is_critter(obj_type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else {
        return OBJ_HANDLE_NULL;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, index);
        if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == type) {
            return item_obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x462760
int64_t item_find_first_generic(int64_t obj, unsigned int flags)
{
    int obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int index;
    int64_t item_obj;

    if (obj == OBJ_HANDLE_NULL) {
        return OBJ_HANDLE_NULL;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else if (obj_type_is_critter(obj_type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else {
        return OBJ_HANDLE_NULL;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, index);
        if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_GENERIC
            && (obj_field_int32_get(item_obj, OBJ_F_GENERIC_FLAGS) & flags) != 0) {
            return item_obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x462820
int64_t item_find_first(int64_t obj)
{
    int inventory_num_fld;
    int inventory_list_fld;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    if (obj_field_int32_get(obj, inventory_num_fld) > 0) {
        return obj_arrayfield_handle_get(obj, inventory_list_fld, 0);
    } else {
        return 0;
    }
}

// 0x462880
int item_list_get(int64_t parent_obj, int64_t** items_ptr)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int idx;
    int cnt;

    *items_ptr = NULL;

    if (obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    cnt = obj_field_int32_get(parent_obj, inventory_num_fld);
    if (cnt > 0) {
        *items_ptr = (int64_t*)MALLOC(sizeof(**items_ptr) * cnt);
        for (idx = 0; idx < cnt; idx++) {
            (*items_ptr)[idx] = obj_arrayfield_handle_get(parent_obj, inventory_list_fld, idx);
        }
    }

    return cnt;
}

// 0x462920
void item_list_free(int64_t* items)
{
    if (items != NULL) {
        FREE(items);
    }
}

// 0x462930
int64_t item_find_first_matching_prototype(int64_t parent_obj, int64_t existing_item_obj)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int index;
    int cnt;
    int64_t prototype_obj;
    int64_t item_obj;

    if (sub_49B290(existing_item_obj) == BP_GENERIC_ITEM_1A) {
        return OBJ_HANDLE_NULL;
    }

    if (obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    cnt = obj_field_int32_get(parent_obj, inventory_num_fld);
    prototype_obj = obj_field_handle_get(existing_item_obj, OBJ_F_PROTOTYPE_HANDLE);

    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(parent_obj, inventory_list_fld, index);
        if (item_obj != existing_item_obj
            && obj_field_handle_get(item_obj, OBJ_F_PROTOTYPE_HANDLE) == prototype_obj) {
            return item_obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x462A30
int64_t sub_462A30(int64_t obj, int64_t a2)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int index;
    int cnt;
    int64_t prototype_obj;
    int64_t item_obj;
    int inventory_location;

    if (sub_49B290(a2) == BP_GENERIC_ITEM_1A) {
        return OBJ_HANDLE_NULL;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    prototype_obj = obj_field_handle_get(a2, OBJ_F_PROTOTYPE_HANDLE);

    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, index);
        if (item_obj != a2
            && obj_field_handle_get(item_obj, OBJ_F_PROTOTYPE_HANDLE) == prototype_obj) {
            inventory_location = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_LOCATION);
            if (IS_HOTKEY_INV_LOC(inventory_location)) {
                return item_obj;
            }
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x462B40
int item_count_items_matching_prototype(int64_t obj, int64_t a2)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int index;
    int cnt;
    int64_t prototype_obj;
    int64_t item_obj;
    int ret = 1;

    if (sub_49B290(a2) == BP_GENERIC_ITEM_1A) {
        return ret;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    prototype_obj = obj_field_handle_get(a2, OBJ_F_PROTOTYPE_HANDLE);

    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, index);
        if (item_obj != a2
            && obj_field_handle_get(item_obj, OBJ_F_PROTOTYPE_HANDLE) == prototype_obj) {
            ret++;
        }
    }

    return ret;
}

// 0x462C30
int sub_462C30(int64_t a1, int64_t a2)
{
    if (a2 == OBJ_HANDLE_NULL
        || (obj_field_int32_get(a2, OBJ_F_ITEM_FLAGS) & OIF_CAN_USE_BOX) == 0) {
        return ITEM_CANNOT_NOT_USABLE;
    }

    return ITEM_CANNOT_OK;
}

// 0x462CC0
void item_use_on_obj(int64_t source_obj, int64_t item_obj, int64_t target_obj)
{
    int64_t parent_obj;
    int item_type;
    S4F2810 v1;

    if (item_obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (tig_net_is_active() && !tig_net_is_host()) {
        if (player_is_local_pc_obj(source_obj)) {
            mp_item_use(source_obj, item_obj, target_obj);
        }
        return;
    }

    item_parent(item_obj, &parent_obj);

    item_type = obj_field_int32_get(item_obj, OBJ_F_TYPE);
    if (item_type == OBJ_TYPE_SCROLL) {
        int spell = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_1);
        int min_intelligence = spell_min_intelligence(spell);
        if (min_intelligence > stat_level_get(source_obj, STAT_INTELLIGENCE)) {
            if (obj_field_int32_get(source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
                item_error_msg(source_obj, ITEM_CANNOT_DUMB);
            }
            return;
        }
    }

    if (target_obj == OBJ_HANDLE_NULL
        && (obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_NEEDS_TARGET) != 0) {
        mp_item_activate(source_obj, item_obj);
        return;
    }

    if (trap_is_trap_device(item_obj)) {
        trap_use_on_obj(source_obj, item_obj, target_obj);
        return;
    }

    if (!object_script_execute(source_obj, item_obj, target_obj, SAP_USE, 0)) {
        combat_consume_action_points(source_obj, 4);
        return;
    }

    if (obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_MANA_STORE) != 0
        || (obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_IS_MAGICAL) != 0) {
        sub_4F2810(&v1, target_obj);
        sub_4605E0(item_obj, &v1, mt_item_spell(item_obj, 0));
        mt_item_notify_used(item_obj, target_obj);

        if (item_type == OBJ_TYPE_FOOD
            || item_type == OBJ_TYPE_SCROLL) {
            sub_4574D0(item_obj);
            object_destroy(item_obj);
        }

        return;
    }

    combat_consume_action_points(source_obj, 4);

    if (item_type == OBJ_TYPE_WRITTEN) {
        ui_written_start_obj(item_obj, source_obj);
        return;
    }

    if (item_type == OBJ_TYPE_GENERIC) {
        unsigned int generic_flags = obj_field_int32_get(item_obj, OBJ_F_GENERIC_FLAGS);

        if ((generic_flags & OGF_IS_LOCKPICK) != 0) {
            if (object_is_lockable(target_obj)) {
                anim_goal_use_item_on_obj_with_skill(source_obj,
                    item_obj,
                    target_obj,
                    SKILL_PICK_LOCKS,
                    0);
            }
        } else if ((generic_flags & OGF_IS_HEALING_ITEM) != 0) {
            anim_goal_use_item_on_obj_with_skill(source_obj,
                item_obj,
                target_obj,
                SKILL_HEAL,
                0);
        }

        return;
    }

    if (item_type == OBJ_TYPE_FOOD) {
        if (obj_field_int32_get(source_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
            MesFileEntry mes_file_entry;
            UiMessage ui_message;

            mes_file_entry.num = 201;
            mes_get_msg(item_mes_file, &mes_file_entry);
            ui_message.type = UI_MSG_TYPE_EXCLAMATION;
            ui_message.str = mes_file_entry.str;
            sub_460630(&ui_message);

            sub_4574D0(item_obj);
            object_destroy(item_obj);
        }

        return;
    }

    if (item_type == OBJ_TYPE_SCROLL) {
        sub_4574D0(item_obj);
        object_destroy(item_obj);
        return;
    }
}

// 0x462FC0
void item_use_on_loc(int64_t obj, int64_t item_obj, int64_t target_loc)
{
    int64_t parent_obj;
    int mana_store;
    int obj_type;
    unsigned int item_flags;
    S4F2810 v1;

    if (item_obj == OBJ_HANDLE_NULL) {
        return;
    }

    item_parent(item_obj, &parent_obj);

    if (target_loc == 0
        && (obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_NEEDS_TARGET) != 0) {
        mp_item_activate(obj, item_obj);
        return;
    }

    if (trap_is_trap_device(item_obj)) {
        trap_use_at_loc(obj, item_obj, target_loc);
        return;
    }

    mana_store = obj_field_int32_get(item_obj, OBJ_F_ITEM_SPELL_MANA_STORE);
    obj_type = obj_field_int32_get(item_obj, OBJ_F_TYPE);
    item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
    if (mana_store != 0 || (item_flags & OIF_IS_MAGICAL) != 0) {
        sub_4F27F0(&v1, target_loc);
        sub_4605E0(item_obj, &v1, mt_item_spell(item_obj, 0));
        mt_item_notify_used(item_obj, OBJ_HANDLE_NULL);
    } else {
        combat_consume_action_points(obj, 4);
        if (obj_type == OBJ_TYPE_WRITTEN) {
            ui_written_start_obj(item_obj, obj);
            return;
        }
    }

    if (obj_type == OBJ_TYPE_FOOD || obj_type == OBJ_TYPE_SCROLL) {
        sub_4574D0(item_obj);
        object_destroy(item_obj);
    }
}

// 0x463110
int item_get_keys(int64_t obj, int* key_ids)
{
    int obj_type;
    int index;
    int cnt;

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(obj_type)) {
        obj = item_find_key_ring(obj);
    } else if (obj_type != OBJ_TYPE_KEY_RING) {
        return 0;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return 0;
    }

    cnt = obj_arrayfield_length_get(obj, OBJ_F_KEY_RING_LIST_IDX);

    if (key_ids != NULL) {
        for (index = 0; index < cnt; index++) {
            key_ids[index] = obj_arrayfield_uint32_get(obj, OBJ_F_KEY_RING_LIST_IDX, index);
        }
    }

    return cnt;
}

// 0x4631A0
int64_t item_find_key_ring(int64_t critter_obj)
{
    int index;
    int cnt;
    int64_t item_obj;
    int64_t key_ring_obj = OBJ_HANDLE_NULL;

    cnt = obj_field_int32_get(critter_obj, OBJ_F_CRITTER_INVENTORY_NUM);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(critter_obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, index);
        if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_KEY_RING) {
            key_ring_obj = item_obj;
            if (obj_arrayfield_length_get(key_ring_obj, OBJ_F_KEY_RING_LIST_IDX) > 0) {
                return key_ring_obj;
            }
        }
    }

    return key_ring_obj;
}

// 0x463240
bool sub_463240(int64_t critter_obj, int lock_id)
{
    int inv_cnt;
    int inv_idx;
    int64_t item_obj;
    int key_id;
    int keyring_cnt;
    int keyring_idx;

    if (lock_id == 0) {
        return false;
    }

    inv_cnt = obj_field_int32_get(critter_obj, OBJ_F_CRITTER_INVENTORY_NUM);
    for (inv_idx = 0; inv_idx < inv_cnt; inv_idx++) {
        item_obj = obj_arrayfield_handle_get(critter_obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, inv_idx);

        switch (obj_field_int32_get(item_obj, OBJ_F_TYPE)) {
        case OBJ_TYPE_KEY:
            key_id = obj_field_int32_get(item_obj, OBJ_F_KEY_KEY_ID);
            if (sub_463340(lock_id, key_id)) {
                return true;
            }
            break;
        case OBJ_TYPE_KEY_RING:
            keyring_cnt = obj_arrayfield_length_get(item_obj, OBJ_F_KEY_RING_LIST_IDX);
            for (keyring_idx = 0; keyring_idx < keyring_cnt; keyring_idx++) {
                key_id = obj_arrayfield_uint32_get(item_obj, OBJ_F_KEY_RING_LIST_IDX, keyring_idx);
                if (sub_463340(lock_id, key_id)) {
                    return true;
                }
            }
            break;
        }
    }

    return false;
}

// 0x463340
bool sub_463340(int lock_id, int key_id)
{
    if (lock_id == 0 || key_id == 0) {
        return false;
    }

    if (lock_id == 1 || key_id == 1) {
        return true;
    }

    return lock_id == key_id;
}

// 0x463370
bool sub_463370(int64_t obj, int key_id)
{
    int64_t leader_obj;
    ObjectList objects;
    ObjectNode* node;
    int64_t party_member_obj;
    int iter;
    int player;

    leader_obj = critter_pc_leader_get(obj);
    if (leader_obj == OBJ_HANDLE_NULL) {
        leader_obj = critter_leader_get(obj);
        if (leader_obj == OBJ_HANDLE_NULL) {
            leader_obj = obj;
        }
    }

    if (sub_463240(leader_obj, key_id)) {
        return true;
    }

    object_list_all_followers(leader_obj, &objects);
    node = objects.head;
    while (node != NULL) {
        if (sub_463240(node->obj, key_id)) {
            break;
        }
        node = node->next;
    }
    object_list_destroy(&objects);

    if (node != NULL) {
        return true;
    }

    if (tig_net_is_active()
        && (tig_net_local_server_get_options() & TIG_NET_SERVER_KEY_SHARING) != 0) {
        if (player_is_pc_obj(obj)) {
            party_member_obj = party_find_first(obj, &iter);
            while (party_member_obj != OBJ_HANDLE_NULL) {
                if (sub_463240(party_member_obj, key_id)) {
                    return true;
                }
                party_member_obj = party_find_next(&iter);
            }
        } else {
            player = party_id_from_obj(obj);
            if (player != -1) {
                object_list_vicinity(obj, OBJ_TM_PC, &objects);
                node = objects.head;
                while (node != NULL) {
                    if (party_id_from_obj(node->obj) == player
                        && sub_463240(node->obj, key_id)) {
                        break;
                    }
                    node = node->next;
                }
                object_list_destroy(&objects);

                if (node != NULL) {
                    return true;
                }
            }
        }
    }

    return false;
}

// 0x463540
bool sub_463540(int64_t container_obj)
{
    // 0x5E8824
    static bool dword_5E8824;

    bool rc = false;
    int cnt;
    int64_t item_obj;
    int64_t loc;

    if (dword_5E8824) {
        return false;
    }

    if (tig_net_is_active()
        && sub_4A5460(container_obj)) {
        return false;
    }

    dword_5E8824 = true;
    if (sub_49B290(container_obj) == BP_JUNK_PILE) {
        cnt = obj_field_int32_get(container_obj, OBJ_F_CONTAINER_INVENTORY_NUM);
        if (cnt == 0) {
            object_destroy(container_obj);
            rc = true;
        } else if (cnt == 1) {
            item_obj = obj_arrayfield_handle_get(container_obj, OBJ_F_CONTAINER_INVENTORY_LIST_IDX, 0);
            item_remove(item_obj);

            loc = obj_field_int64_get(container_obj, OBJ_F_LOCATION);
            object_destroy(container_obj);
            sub_466E50(item_obj, loc);
            rc = true;
        }
    }
    dword_5E8824 = false;

    return rc;
}

// 0x463630
void sub_463630(int64_t obj)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int inventory_list_cnt;
    int idx;
    int64_t item_obj;
    unsigned int flags;

    if (!inventory_fields_from_obj_type(obj_field_int32_get(obj, OBJ_F_TYPE), &inventory_num_fld, &inventory_list_fld)) {
        return;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    inventory_list_cnt = obj_arrayfield_length_get(obj, inventory_list_fld);
    if (cnt != inventory_list_cnt) {
        tig_debug_printf("Inventory array count does not equal associatednum field on inventory marking as summoned.  Array: %d, Field: %d",
            inventory_list_cnt,
            cnt);
    }

    // FIXME: Useless.
    obj_field_int64_get(obj, OBJ_F_LOCATION);

    for (idx = inventory_list_cnt - 1; idx >= 0; idx--) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, idx);

        flags = obj_field_int32_get(item_obj, OBJ_F_SPELL_FLAGS);
        flags |= OSF_SUMMONED;
        obj_field_int32_set(item_obj, OBJ_F_SPELL_FLAGS, flags);

        flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
        flags |= OIF_NO_DROP;
        obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, flags);
    }
}

// 0x463730
void sub_463730(int64_t obj, bool a2)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int inventory_list_cnt;
    int idx;
    int64_t loc;
    int64_t item_obj;

    if (!inventory_fields_from_obj_type(obj_field_int32_get(obj, OBJ_F_TYPE), &inventory_num_fld, &inventory_list_fld)) {
        return;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    inventory_list_cnt = obj_arrayfield_length_get(obj, inventory_list_fld);
    if (cnt != inventory_list_cnt) {
        tig_debug_printf("Inventory array count does not equal associatednum field on poop.  Array: %d, Field: %d",
            inventory_list_cnt,
            cnt);
    }

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);

    for (idx = inventory_list_cnt - 1; idx >= 0; idx--) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, idx);

        item_force_remove(item_obj, obj);

        if (!sub_467E70()) {
            if (a2) {
                sub_466E50(item_obj, loc);
            } else {
                object_drop(item_obj, loc);
            }
        }
    }
}

// 0x463860
void sub_463860(int64_t obj, bool a2)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int inventory_list_cnt;
    int idx;
    int64_t loc;
    int64_t item_obj;
    unsigned int spell_flags;
    unsigned int item_flags;

    if (!inventory_fields_from_obj_type(obj_field_int32_get(obj, OBJ_F_TYPE), &inventory_num_fld, &inventory_list_fld)) {
        return;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    inventory_list_cnt = obj_arrayfield_length_get(obj, inventory_list_fld);
    if (cnt != inventory_list_cnt) {
        tig_debug_printf("Inventory array count does not equal associatednum field on poop.  Array: %d, Field: %d",
            inventory_list_cnt,
            cnt);
    }

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);

    for (idx = inventory_list_cnt - 1; idx >= 0; idx--) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, idx);

        spell_flags = obj_field_int32_get(item_obj, OBJ_F_SPELL_FLAGS);
        item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);

        item_force_remove(item_obj, obj);

        if (!sub_467E70()) {
            if ((spell_flags & OSF_SUMMONED) == 0
                && (item_flags & OIF_NO_DISPLAY) == 0) {
                if (a2) {
                    sub_466E50(item_obj, loc);
                } else {
                    object_drop(item_obj, loc);
                }
            } else {
                object_drop(item_obj, loc);
                object_destroy(item_obj);
            }
        }
    }
}

// 0x4639E0
void sub_4639E0(int64_t obj, bool a2)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int inventory_list_cnt;
    int idx;
    int64_t loc;
    int64_t item_obj;

    if (sub_49B290(obj) == BP_JUNK_PILE) {
        return;
    }

    if (!inventory_fields_from_obj_type(obj_field_int32_get(obj, OBJ_F_TYPE), &inventory_num_fld, &inventory_list_fld)) {
        return;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    inventory_list_cnt = obj_arrayfield_length_get(obj, inventory_list_fld);
    if (cnt != inventory_list_cnt) {
        tig_debug_printf("Inventory array count does not equal associatednum field on poop.  Array: %d, Field: %d",
            inventory_list_cnt,
            cnt);
    }

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);

    for (idx = inventory_list_cnt - 1; idx >= 0; idx--) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, idx);

        if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_INVULNERABLE) != 0) {
            item_force_remove(item_obj, obj);

            if (a2) {
                sub_466E50(item_obj, loc);
            } else {
                object_drop(item_obj, loc);
            }
        }
    }
}

// 0x463B30
void sub_463B30(int64_t obj, bool a2)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int inventory_list_cnt;
    int idx;
    int64_t loc;
    int64_t item_obj;

    if (!inventory_fields_from_obj_type(obj_field_int32_get(obj, OBJ_F_TYPE), &inventory_num_fld, &inventory_list_fld)) {
        return;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    inventory_list_cnt = obj_arrayfield_length_get(obj, inventory_list_fld);
    if (cnt != inventory_list_cnt) {
        tig_debug_printf("Inventory array count does not equal associatednum field on delete.  Array: %d, Field: %d",
            inventory_list_cnt,
            cnt);
    }

    loc = obj_field_int64_get(obj, OBJ_F_LOCATION);

    idx = 0;
    cnt = obj_field_int32_get(obj, inventory_num_fld);
    while (cnt > idx) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, idx);

        if (a2 && (obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_PERSISTENT) != 0) {
            idx++;
        } else {
            item_force_remove(item_obj, obj);
            object_drop(item_obj, loc);
            object_destroy(item_obj);
        }

        cnt = obj_field_int32_get(obj, inventory_num_fld);
    }
}

// 0x463C60
void sub_463C60(int64_t obj)
{
    int obj_type;
    TigRect obj_rect;
    ObjectList objects;
    ObjectNode* node;
    bool v1;
    int64_t v2;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (item_inventory_source(obj) == 0) {
        return;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);

    if (obj_type_is_critter(obj_type)) {
        if (critter_is_dead(obj)
            || (obj_type == OBJ_TYPE_NPC
                && critter_pc_leader_get(obj) != OBJ_HANDLE_NULL)) {
            return;
        }

        sub_463E20(obj);
        return;
    }

    if (obj_type == OBJ_TYPE_CONTAINER) {
        if ((obj_field_int32_get(obj, OBJ_F_CONTAINER_FLAGS) & OCOF_INVEN_SPAWN_INDEPENDENT) != 0) {
            sub_463E20(obj);
            return;
        }

        object_get_rect(obj, RECT_FLAG_IGNORE_VIS, &obj_rect);

        if (obj_rect.x >= item_iso_content_rect.x + item_iso_content_rect.width
            || obj_rect.y >= item_iso_content_rect.height + item_iso_content_rect.y
            || item_iso_content_rect.x >= obj_rect.x + obj_rect.width
            || item_iso_content_rect.y >= obj_rect.y + obj_rect.height) {
            v1 = true;
            object_list_vicinity(obj, OBJ_TM_NPC, &objects);
            node = objects.head;
            while (node != NULL) {
                if (critter_substitute_inventory_get(node->obj) == obj) {
                    v2 = node->obj;
                    if (!critter_is_dead(node->obj)
                        && critter_pc_leader_get(node->obj) == OBJ_HANDLE_NULL) {
                        v1 = false;
                    }
                    break;
                }
                node = node->next;
            }
            object_list_destroy(&objects);

            if (!v1) {
                sub_463C60(v2);
                sub_463E20(obj);
            }
        } else {
            sub_4640C0(obj);
        }
    }
}

// 0x463E20
void sub_463E20(int64_t obj)
{
    int obj_type;
    int inventory_source_fld;
    int source_id;
    InvenSourceSet set;
    int64_t loc;
    bool spawn;
    int qty;
    int idx;
    int64_t item_obj;

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        sub_4EFBA0(obj);
        return;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    spawn = false;
    if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_source_fld = OBJ_F_CONTAINER_INVENTORY_SOURCE;
        if ((obj_field_int32_get(obj, OBJ_F_CONTAINER_FLAGS) & OCOF_INVEN_SPAWN_ONCE) != 0) {
            if (item_editor) {
                return;
            }
            spawn = true;
        }
    } else if (obj_type_is_critter(obj_type)) {
        inventory_source_fld = OBJ_F_CRITTER_INVENTORY_SOURCE;
    } else {
        return;
    }

    source_id = obj_field_int32_get(obj, inventory_source_fld);

    sub_4EFAE0(obj, 1);

    if (source_id != 0) {
        loc = obj_field_int64_get(obj, OBJ_F_LOCATION);
        invensource_get_id_list(source_id, &set);

        if (spawn) {
            int seed = (int)loc;
            int64_t pc_obj = player_get_local_pc_obj();

            if (pc_obj != OBJ_HANDLE_NULL) {
                char buffer[80];
                int pos;
                int len;

                seed += stat_level_get(pc_obj, STAT_RACE) * background_get(pc_obj);
                seed += stat_level_get(pc_obj, STAT_GENDER);

                object_examine(pc_obj, pc_obj, buffer);
                len = (int)strlen(buffer);

                for (pos = 1; pos < len; pos += 2) {
                    seed += buffer[pos] * buffer[pos - 1];
                }
            }

            random_seed(seed);
        }

        qty = random_between(set.min_coins, set.max_coins);
        if (qty > 0) {
            item_gold_transfer(OBJ_HANDLE_NULL, obj, qty, OBJ_HANDLE_NULL);
        }

        for (idx = 0; idx < set.cnt; idx++) {
            if (random_between(1, 100) <= set.rate[idx]
                && mp_object_create(set.basic_prototype[idx], loc, &item_obj)) {
                if (!item_transfer(item_obj, obj)) {
                    object_destroy(item_obj);
                }
            }
        }

        if (obj_type == OBJ_TYPE_NPC) {
            item_wield_best_all(obj, OBJ_HANDLE_NULL);
        }

        if (spawn) {
            mp_obj_field_int32_set(obj, inventory_source_fld, 0);

            unsigned int container_flags = obj_field_int32_get(obj, OBJ_F_CONTAINER_FLAGS);
            container_flags &= ~OCOF_INVEN_SPAWN_ONCE;
            mp_obj_field_int32_set(obj, OBJ_F_CONTAINER_FLAGS, container_flags);
        }
    }
}

// 0x4640C0
void sub_4640C0(int64_t obj)
{
    int64_t inven_source_obj;
    TimeEvent timeevent;
    DateTime datetime;
    int ms;

    inven_source_obj = item_inventory_source(obj);
    if (inven_source_obj != OBJ_HANDLE_NULL) {
        qword_5E8818 = obj;
        timeevent_clear_all_ex(TIMEEVENT_TYPE_NPC_RESPAWN, sub_464150);
        timeevent.type = TIMEEVENT_TYPE_NPC_RESPAWN;
        timeevent.params[0].object_value = obj;

        ms = random_between(43200000, 86400000);
        if (tig_net_is_active()) {
            ms /= 4;
        }

        sub_45A950(&datetime, ms);
        timeevent_add_delay(&timeevent, &datetime);
    }
}

// 0x464150
bool sub_464150(TimeEvent* timeevent)
{
    return timeevent != NULL && timeevent->params[0].object_value == qword_5E8818;
}

// 0x464180
bool npc_respawn_timevent_process(TimeEvent* timeevent)
{
    sub_463C60(timeevent->params[0].object_value);
    return true;
}

// 0x4641A0
int item_inventory_source(int64_t obj)
{
    if (obj != OBJ_HANDLE_NULL) {
        switch (obj_field_int32_get(obj, OBJ_F_TYPE)) {
        case OBJ_TYPE_CONTAINER:
            return obj_field_int32_get(obj, OBJ_F_CONTAINER_INVENTORY_SOURCE);
        case OBJ_TYPE_NPC:
            return obj_field_int32_get(obj, OBJ_F_CRITTER_INVENTORY_SOURCE);
        }
    }
    return 0;
}

// 0x464200
bool item_check_invensource_buy_list(int64_t item_obj, int64_t buyer_npc_obj)
{
    int invensource_id;
    InvenSourceSet set;
    int idx;
    int basic_prototype;
    int64_t substitute_inventory_obj;

    invensource_id = item_inventory_source(buyer_npc_obj);
    if (invensource_id != 0) {
        invensource_get_id_list(invensource_id, &set);
        if (set.buy_all) {
            return true;
        }

        basic_prototype = sub_49B290(item_obj);
        for (idx = 0; idx < set.buy_cnt; idx++) {
            if (set.buy_basic_prototype[idx] == basic_prototype) {
                return true;
            }
        }
    }

    substitute_inventory_obj = critter_substitute_inventory_get(buyer_npc_obj);
    if (substitute_inventory_obj != OBJ_HANDLE_NULL) {
        return item_check_invensource_buy_list(item_obj, substitute_inventory_obj);
    }

    return false;
}

// 0x4642C0
bool sub_4642C0(int64_t obj, int64_t item_obj)
{
    int obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int index;
    int cnt;

    if (!obj_handle_is_valid(obj)) {
        return false;
    }

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else if (obj_type_is_critter(obj_type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else {
        return false;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    for (index = 0; index < cnt; index++) {
        if (obj_arrayfield_handle_get(obj, inventory_list_fld, index) == item_obj) {
            return true;
        }
    }

    return false;
}

// 0x464370
bool item_is_identified(int64_t obj)
{
    if (obj == OBJ_HANDLE_NULL
        || !obj_type_is_item(obj_field_int32_get(obj, OBJ_F_TYPE))) {
        return false;
    }

    return obj_field_int32_get(obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) <= 0
        || (obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS) & OIF_IDENTIFIED) != 0;
}

// 0x4643D0
void item_identify_all(int64_t obj)
{
    int obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int index;
    int cnt;
    int64_t item_obj;
    unsigned int flags;

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else if (obj_type_is_critter(obj_type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else {
        return;
    }

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, index);
        flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
        flags |= OIF_IDENTIFIED;
        obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, flags);
    }
}

// 0x464470
void sub_464470(int64_t obj, int* a2, int* a3)
{
    int obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int index;
    int cnt;
    int64_t item_obj;
    int v1 = 0;
    int v2 = 0;

    obj_type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else if (obj_type_is_critter(obj_type)) {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = -1;
    }

    if (inventory_num_fld != -1) {
        cnt = obj_field_int32_get(obj, inventory_num_fld);
        while (cnt > 0) {
            item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, cnt - 1);
            obj_field_handle_set(item_obj, OBJ_F_ITEM_PARENT, obj);

            // TODO: Incomplete.

            if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_INVENTORY) == 0) {
                object_pickup(item_obj, obj);
                v2++;
            }
            cnt--;
        }
    }

    if (a2 != NULL) {
        *a2 += v1;
    }

    if (a3 != NULL) {
        *a3 += v2;
    }
}

// 0x464630
int item_total_attack(int64_t critter_obj)
{
    int64_t weapon_obj;
    int skill;
    int effectiveness;
    int v2;
    int v3;
    DamageType damage_type;
    int min_dam;
    int max_dam;

    weapon_obj = item_wield_get(critter_obj, ITEM_INV_LOC_WEAPON);
    skill = item_weapon_skill(weapon_obj);
    if (IS_TECH_SKILL(skill)) {
        effectiveness = tech_skill_effectiveness(critter_obj, GET_TECH_SKILL(skill), OBJ_HANDLE_NULL);
    } else {
        effectiveness = basic_skill_effectiveness(critter_obj, GET_BASIC_SKILL(skill), OBJ_HANDLE_NULL);
    }

    v3 = effectiveness * dword_5B32EC;
    v2 = dword_5B32EC;

    for (damage_type = 0; damage_type < DAMAGE_TYPE_COUNT; damage_type++) {
        item_weapon_damage(weapon_obj,
            critter_obj,
            damage_type,
            skill,
            false,
            &min_dam,
            &max_dam);

        v3 += dword_5B32F0[damage_type] * ((min_dam + max_dam) / 2);
        v2 += dword_5B32F0[damage_type];
    }

    return v3 / (v2 + 6);
}

// 0x464700
int item_total_defence(int64_t obj)
{
    int v1;
    int v2;
    int resistance_type;

    v1 = dword_5B3304 * object_get_ac(obj, true);
    v2 = dword_5B3304;

    for (resistance_type = 0; resistance_type < RESISTANCE_TYPE_COUNT; resistance_type++) {
        v1 += dword_5B3308[resistance_type] * object_get_resistance(obj, resistance_type, true);
        v2 += dword_5B3308[resistance_type];
    }

    return 2 * (v1 / (v2 + 6));
}

// 0x464780
int item_gold_get(int64_t obj)
{
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_GOLD) {
        obj = item_gold_obj(obj);
    }

    if (obj != OBJ_HANDLE_NULL) {
        return obj_field_int32_get(obj, OBJ_F_GOLD_QUANTITY);
    }

    return 0;
}

// 0x4647D0
int64_t item_gold_obj(int64_t obj)
{
    int type;

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (obj_type_is_critter(type)) {
        return obj_field_handle_get(obj, OBJ_F_CRITTER_GOLD);
    }
    if (type == OBJ_TYPE_CONTAINER) {
        return sub_462540(obj, 9056, 0);
    }

    return OBJ_HANDLE_NULL;
}

// 0x464830
bool item_gold_transfer(int64_t from_obj, int64_t to_obj, int qty, int64_t gold_obj)
{
    int from_qty;
    int to_qty;
    int64_t loc;

    if (qty != 0) {
        if (tig_net_is_active()
            && !tig_net_is_host()) {
            return false;
        }

        if (from_obj != OBJ_HANDLE_NULL) {
            if (gold_obj == OBJ_HANDLE_NULL) {
                gold_obj = item_gold_obj(from_obj);
            }

            if (gold_obj != OBJ_HANDLE_NULL) {
                from_qty = obj_field_int32_get(gold_obj, OBJ_F_GOLD_QUANTITY);
                if (from_qty < qty) {
                    return false;
                }

                if (to_obj != OBJ_HANDLE_NULL) {
                    object_script_execute(to_obj, gold_obj, OBJ_HANDLE_NULL, SAP_INSERT_ITEM, 0);
                }

                if (from_qty == qty) {
                    object_destroy(gold_obj);
                } else {
                    mp_obj_field_int32_set(gold_obj, OBJ_F_GOLD_QUANTITY, from_qty - qty);
                }
            }
        }

        if (to_obj != OBJ_HANDLE_NULL) {
            gold_obj = item_gold_obj(to_obj);
            if (gold_obj != OBJ_HANDLE_NULL) {
                to_qty = obj_field_int32_get(gold_obj, OBJ_F_GOLD_QUANTITY);
                mp_obj_field_int32_set(gold_obj, OBJ_F_GOLD_QUANTITY, to_qty + qty);
            } else {
                loc = obj_field_int64_get(to_obj, OBJ_F_LOCATION);
                gold_obj = item_gold_create(qty, loc);
                item_transfer(gold_obj, to_obj);
            }
        }

        sub_4605D0();
    }

    return true;
}

// 0x464970
int64_t item_gold_create(int amount, int64_t loc)
{
    int64_t gold_obj;

    if (mp_object_create(BP_GOLD, loc, &gold_obj)) {
        mp_obj_field_int32_set(gold_obj, OBJ_F_GOLD_QUANTITY, amount);
    }

    return gold_obj;
}

// 0x4649C0
int64_t item_wield_get(int64_t obj, int inventory_location)
{
    int cnt;
    int index;
    int64_t item_obj;
    bool validated = false;

    cnt = obj_field_int32_get(obj, OBJ_F_CRITTER_INVENTORY_NUM);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, index);
        if (item_obj == OBJ_HANDLE_NULL) {
            tig_debug_printf("item_wield_get: ERROR: inv_obj in slot #%d is NULL!\n", index);

            if (!validated) {
                if (!obj_validate_system(1)) {
                    tig_debug_printf("item_wield_get: obj_validate_system: ERROR: Failed to validate!\n");
                }
                validated = true;
                index = -1;
            }

            continue;
        }

        if (!item_is_item(item_obj)) {
            tig_debug_printf("obj with d %d n %d contains object d %d n %d\n",
                obj_field_int32_get(obj, OBJ_F_DESCRIPTION),
                obj_field_int32_get(obj, OBJ_F_NAME),
                obj_field_int32_get(item_obj, OBJ_F_DESCRIPTION),
                obj_field_int32_get(item_obj, OBJ_F_NAME));
        }

        if (item_inventory_location_get(item_obj) == inventory_location) {
            return item_obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x464AE0
bool item_wield_set(int64_t item_obj, int inventory_location)
{
    int64_t owner_obj;
    int64_t cur_item_obj;
    int saved_inventory_location;
    int v1;

    item_parent(item_obj, &owner_obj);

    cur_item_obj = item_wield_get(owner_obj, inventory_location);
    if (cur_item_obj != item_obj) {
        if (sub_464D20(item_obj, inventory_location, owner_obj) != ITEM_CANNOT_OK) {
            return false;
        }

        if (item_check_remove(item_obj) != ITEM_CANNOT_OK) {
            return false;
        }

        if (cur_item_obj != OBJ_HANDLE_NULL) {
            if (item_check_remove(cur_item_obj) != ITEM_CANNOT_OK) {
                return false;
            }

            saved_inventory_location = item_inventory_location_get(item_obj);
            item_remove(item_obj);
            item_remove(cur_item_obj);

            if (item_check_insert(cur_item_obj, owner_obj, &v1) != ITEM_CANNOT_OK) {
                item_insert(item_obj, owner_obj, saved_inventory_location);
                item_insert(cur_item_obj, owner_obj, inventory_location);
                return false;
            }

            item_insert(cur_item_obj, owner_obj, v1);
        } else {
            item_remove(item_obj);
        }

        item_insert(item_obj, owner_obj, inventory_location);

        if (inventory_location == 1003) {
            anim_speed_recalc(item_obj);
        }
    }

    return true;
}

// 0x464C50
bool sub_464C50(int64_t obj, int inventory_location)
{
    int64_t item_obj;

    item_obj = item_wield_get(obj, inventory_location);
    if (item_obj != OBJ_HANDLE_NULL) {
        return sub_464C80(item_obj);
    } else {
        return true;
    }
}

// 0x464C80
bool sub_464C80(int64_t item_obj)
{
    int inventory_location;
    int64_t owner_obj;
    int v1;

    inventory_location = item_inventory_location_get(item_obj);
    if (IS_WEAR_INV_LOC(inventory_location)) {
        if (item_check_remove(item_obj) != ITEM_CANNOT_OK) {
            return false;
        }

        item_parent(item_obj, &owner_obj);
        if (item_check_insert(item_obj, owner_obj, &v1) != ITEM_CANNOT_OK) {
            return false;
        }

        item_remove(item_obj);
        item_insert(item_obj, owner_obj, v1);
    }

    return true;
}

// 0x464D20
int sub_464D20(int64_t item_obj, int inventory_location, int64_t critter_obj)
{
    int item_obj_type;
    unsigned int weapon_flags;
    int item_subtype;
    tig_art_id_t art_id;
    int64_t weapon_obj;
    unsigned int armor_flags;

    if ((obj_field_int32_get(critter_obj, OBJ_F_SPELL_FLAGS) & (OSF_POLYMORPHED | OSF_BODY_OF_WATER | OSF_BODY_OF_FIRE | OSF_BODY_OF_EARTH | OSF_BODY_OF_AIR))) {
        return ITEM_CANNOT_NOT_WIELDABLE;
    }

    if (inventory_location == 1002) {
        inventory_location = 1001;
    }

    if (obj_field_int32_get(item_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) > 0
        && background_get(critter_obj) == BACKGROUND_MAGICK_ALLERGY) {
        return ITEM_CANNOT_WIELD_MAGIC_ITEMS;
    }

    if (tig_art_item_id_destroyed_get(obj_field_int32_get(item_obj, OBJ_F_CURRENT_AID)) != 0) {
        return ITEM_CANNOT_BROKEN;
    }

    if (item_location_get(item_obj) != inventory_location) {
        return ITEM_CANNOT_WRONG_TYPE;
    }

    item_obj_type = obj_field_int32_get(item_obj, OBJ_F_TYPE);

    switch (inventory_location) {
    case ITEM_INV_LOC_WEAPON:
        weapon_flags = obj_field_int32_get(item_obj, OBJ_F_WEAPON_FLAGS);
        if ((weapon_flags & OWF_TWO_HANDED) && (weapon_flags & OWF_HAND_COUNT_FIXED) != 0) {
            if (item_wield_get(critter_obj, ITEM_INV_LOC_SHIELD) != OBJ_HANDLE_NULL) {
                return ITEM_CANNOT_NO_FREE_HAND;
            }

            if ((obj_field_int32_get(critter_obj, OBJ_F_CRITTER_FLAGS) & (OCF_CRIPPLED_ARMS_ONE | OCF_CRIPPLED_ARMS_BOTH)) != 0) {
                return ITEM_CANNOT_CRIPPLED;
            }
        } else {
            if ((obj_field_int32_get(critter_obj, OBJ_F_CRITTER_FLAGS) & OCF_CRIPPLED_ARMS_BOTH) != 0) {
                return ITEM_CANNOT_CRIPPLED;
            }
        }

        item_subtype = tig_art_item_id_subtype_get(obj_field_int32_get(item_obj, OBJ_F_ITEM_USE_AID_FRAGMENT));
        art_id = sub_4650D0(critter_obj);
        art_id = tig_art_critter_id_weapon_set(art_id, item_subtype);
        if (tig_art_critter_id_weapon_get(art_id) != item_subtype) {
            return ITEM_CANNOT_NOT_WIELDABLE;
        }

        if (tig_art_exists(art_id) != TIG_OK) {
            return ITEM_CANNOT_NOT_WIELDABLE;
        }
        break;
    case ITEM_INV_LOC_SHIELD:
        weapon_obj = item_wield_get(critter_obj, ITEM_INV_LOC_WEAPON);
        if (weapon_obj != OBJ_HANDLE_NULL) {
            weapon_flags = obj_field_int32_get(weapon_obj, OBJ_F_WEAPON_FLAGS);
            if ((weapon_flags & OWF_TWO_HANDED) && (weapon_flags & OWF_HAND_COUNT_FIXED) != 0) {
                return ITEM_CANNOT_NO_FREE_HAND;
            }
        }

        if ((obj_field_int32_get(critter_obj, OBJ_F_CRITTER_FLAGS) & OCF_CRIPPLED_ARMS_BOTH) != 0) {
            return ITEM_CANNOT_CRIPPLED;
        }

        if (item_obj_type == OBJ_TYPE_ARMOR) {
            art_id = sub_4650D0(critter_obj);
            art_id = tig_art_critter_id_shield_set(art_id, 1);
            if (tig_art_exists(art_id) != TIG_OK) {
                return ITEM_CANNOT_NOT_WIELDABLE;
            }
        }
        break;
    case ITEM_INV_LOC_ARMOR:
        armor_flags = obj_field_int32_get(item_obj, OBJ_F_ARMOR_FLAGS);
        if ((item_armor_size(stat_level_get(critter_obj, STAT_RACE)) & armor_flags) == 0) {
            return ITEM_CANNOT_WRONG_WEARABLE_SIZE;
        }
        if ((armor_flags & OARF_MALE_ONLY) != 0) {
            if (stat_level_get(critter_obj, STAT_GENDER) != GENDER_MALE) {
                return ITEM_CANNOT_WRONG_WEARABLE_GENDER;
            }
        } else if ((armor_flags & OARF_FEMALE_ONLY)) {
            if (stat_level_get(critter_obj, STAT_GENDER) != GENDER_FEMALE) {
                return ITEM_CANNOT_WRONG_WEARABLE_GENDER;
            }
        }

        art_id = obj_field_int32_get(critter_obj, OBJ_F_CURRENT_AID);
        if (tig_art_type(art_id) == TIG_ART_TYPE_UNIQUE_NPC) {
            return ITEM_CANNOT_NOT_WIELDABLE;
        }

        if (!sub_465AE0(item_obj, critter_obj, &art_id)) {
            return ITEM_CANNOT_NOT_WIELDABLE;
        }

        if (tig_art_exists(art_id) != TIG_OK) {
            return ITEM_CANNOT_NOT_WIELDABLE;
        }
        break;
    }

    return ITEM_CANNOT_OK;
}

// 0x465010
int sub_465010(int64_t obj)
{
    return ITEM_CANNOT_OK;
}

// 0x465020
tig_art_id_t sub_465020(int64_t obj)
{
    int64_t weapon_obj;
    int64_t armor_obj;
    int weapon_type;
    int shield_type;
    tig_art_id_t art_id;

    if (combat_critter_is_combat_mode_active(obj)) {
        weapon_obj = item_wield_get(obj, ITEM_INV_LOC_WEAPON);
        if (weapon_obj != OBJ_HANDLE_NULL) {
            weapon_type = tig_art_item_id_subtype_get(obj_field_int32_get(weapon_obj, OBJ_F_ITEM_USE_AID_FRAGMENT));
            if (weapon_type == TIG_ART_WEAPON_TYPE_NO_WEAPON) {
                tig_debug_printf("Item: ERROR: Item found with invalid item_use_aid_fragment!\n");
                weapon_type = TIG_ART_WEAPON_TYPE_UNARMED;
            }
        } else {
            weapon_type = TIG_ART_WEAPON_TYPE_UNARMED;
        }

        armor_obj = item_wield_get(obj, ITEM_INV_LOC_SHIELD);
        if (armor_obj != OBJ_HANDLE_NULL
            && obj_field_int32_get(armor_obj, OBJ_F_TYPE) == OBJ_TYPE_ARMOR) {
            shield_type = 1;
        } else {
            shield_type = 0;
        }
    } else {
        weapon_type = TIG_ART_WEAPON_TYPE_NO_WEAPON;
        shield_type = 0;
    }

    art_id = obj_field_int32_get(obj, OBJ_F_CURRENT_AID);
    art_id = tig_art_critter_id_weapon_set(art_id, weapon_type);
    art_id = tig_art_critter_id_shield_set(art_id, shield_type);

    return art_id;
}

// 0x4650D0
tig_art_id_t sub_4650D0(int64_t critter_obj)
{
    int64_t item_obj;
    int weapon_type;
    int shield_type;
    tig_art_id_t art_id;

    item_obj = item_wield_get(critter_obj, ITEM_INV_LOC_WEAPON);
    if (item_obj != OBJ_HANDLE_NULL) {
        weapon_type = tig_art_item_id_subtype_get(obj_field_int32_get(item_obj, OBJ_F_ITEM_USE_AID_FRAGMENT));
    } else {
        weapon_type = TIG_ART_WEAPON_TYPE_UNARMED;
    }

    item_obj = item_wield_get(critter_obj, ITEM_INV_LOC_SHIELD);
    if (item_obj != OBJ_HANDLE_NULL
        && obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_ARMOR) {
        shield_type = 1;
    } else {
        shield_type = 0;
    }

    art_id = obj_field_int32_get(critter_obj, OBJ_F_CURRENT_AID);
    art_id = tig_art_id_anim_set(art_id, 20);
    art_id = tig_art_critter_id_weapon_set(art_id, weapon_type);
    art_id = tig_art_critter_id_shield_set(art_id, shield_type);

    return art_id;
}

// 0x465170
void item_wield_best(int64_t critter_obj, int inventory_location, int64_t target_obj)
{
    int cnt;
    int idx;
    int64_t equipped_item_obj;
    int best_item_worth;
    int64_t best_item_obj;
    int64_t dist;
    int best_ranged_weapon_worth;
    int64_t best_ranged_weapon_obj;
    int64_t item_obj;

    cnt = obj_field_int32_get(critter_obj, OBJ_F_CRITTER_INVENTORY_NUM);
    equipped_item_obj = item_wield_get(critter_obj, inventory_location);
    best_item_worth = INT_MIN;
    best_item_obj = equipped_item_obj;

    if (equipped_item_obj != OBJ_HANDLE_NULL) {
        if (sub_464D20(equipped_item_obj, inventory_location, critter_obj) == ITEM_CANNOT_OK
            && inventory_location != ITEM_INV_LOC_WEAPON) {
            best_item_worth = item_worth(equipped_item_obj);
        } else {
            sub_464C50(critter_obj, inventory_location);
            equipped_item_obj = OBJ_HANDLE_NULL;
            best_item_obj = OBJ_HANDLE_NULL;
        }
    }

    best_ranged_weapon_obj = OBJ_HANDLE_NULL;
    if (target_obj != OBJ_HANDLE_NULL) {
        dist = object_dist(critter_obj, target_obj);
        best_ranged_weapon_worth = INT_MIN;
    }

    for (idx = 0; idx < cnt; idx++) {
        item_obj = obj_arrayfield_handle_get(critter_obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, idx);
        if (sub_464D20(item_obj, inventory_location, critter_obj) == ITEM_CANNOT_OK) {
            if (inventory_location == ITEM_INV_LOC_WEAPON) {
                int ammo_type = item_weapon_ammo_type(item_obj);
                if (ammo_type == 10000
                    || obj_field_int32_get(item_obj, OBJ_F_WEAPON_AMMO_CONSUMPTION) <= item_ammo_quantity_get(critter_obj, ammo_type)) {
                    int total_dam = 0;
                    int skill = item_weapon_skill(item_obj);
                    for (DamageType damage_type = 0; damage_type < DAMAGE_TYPE_COUNT; damage_type++) {
                        int min_dam;
                        int max_dam;

                        item_weapon_damage(item_obj, critter_obj, damage_type, skill, false, &min_dam, &max_dam);
                        total_dam = (min_dam + max_dam) / 2;
                    }

                    int effectiveness;
                    if (IS_TECH_SKILL(skill)) {
                        int tech_skill = GET_TECH_SKILL(skill);
                        if (tech_skill_level(critter_obj, tech_skill) != 0) {
                            effectiveness = tech_skill_effectiveness(critter_obj, tech_skill, target_obj);
                        } else {
                            effectiveness = -1;
                        }
                    } else {
                        int basic_skill = GET_BASIC_SKILL(skill);
                        if (skill == BASIC_SKILL_MELEE
                            || basic_skill_level(critter_obj, skill) != 0) {
                            effectiveness = basic_skill_effectiveness(critter_obj, basic_skill, target_obj);
                        } else {
                            effectiveness = -1;
                        }
                    }

                    if (effectiveness == 0) {
                        effectiveness = -1;
                    }

                    int effective_total_dam = total_dam * effectiveness / 100;

                    if (best_item_worth == INT_MIN
                        || effective_total_dam > best_item_worth
                        || (effective_total_dam == best_item_worth
                            && item_obj > best_item_obj)) {
                        best_item_obj = item_obj;
                        best_item_worth = effective_total_dam;
                    }

                    if (target_obj != OBJ_HANDLE_NULL
                        && (best_ranged_weapon_worth == INT_MIN
                            || effective_total_dam > best_ranged_weapon_worth
                            || (effective_total_dam == best_ranged_weapon_worth
                                && item_obj > best_ranged_weapon_obj))
                        && item_weapon_range(item_obj, critter_obj) >= dist) {
                        best_ranged_weapon_obj = item_obj;
                    }
                }
            } else if (!IS_WEAR_INV_LOC(item_inventory_location_get(item_obj))) {
                int worth = item_worth(item_obj);
                if (worth > best_item_worth) {
                    best_item_worth = worth;
                    best_item_obj = item_obj;
                }
            }
        }
    }

    if (best_ranged_weapon_obj != OBJ_HANDLE_NULL) {
        best_item_obj = best_ranged_weapon_obj;
    }

    if (best_item_obj != OBJ_HANDLE_NULL) {
        item_wield_set(best_item_obj, inventory_location);
    } else if (inventory_location == ITEM_INV_LOC_WEAPON) {
        sub_464C50(critter_obj, inventory_location);
    }
}

// 0x4654F0
void item_wield_best_all(int64_t critter_obj, int64_t target_obj)
{
    int inventory_location;

    for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
        item_wield_best(critter_obj, inventory_location, target_obj);
    }
}

// 0x465530
void item_rewield(int64_t obj)
{
    int inventory_location;
    int64_t item_obj;

    for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
        item_obj = item_wield_get(obj, inventory_location);
        if (item_obj != OBJ_HANDLE_NULL) {
            in_rewield = true;
            item_recalc_light(item_obj, obj);
            object_script_execute(obj, item_obj, OBJ_HANDLE_NULL, SAP_WIELD_OFF, 0);
            object_script_execute(obj, item_obj, OBJ_HANDLE_NULL, SAP_WIELD_ON, 0);
            in_rewield = false;
        }
    }
}

// 0x4655C0
bool item_in_rewield()
{
    return in_rewield;
}

// 0x4655D0
int item_location_get(int64_t obj)
{
    int type;

    type = obj_field_int32_get(obj, OBJ_F_TYPE);
    if (type == OBJ_TYPE_ARMOR) {
        switch (item_armor_coverage(obj)) {
        case TIG_ART_ARMOR_COVERAGE_HELMET:
            return ITEM_INV_LOC_HELMET;
        case TIG_ART_ARMOR_COVERAGE_RING:
            return ITEM_INV_LOC_RING1;
        case TIG_ART_ARMOR_COVERAGE_MEDALLION:
            return ITEM_INV_LOC_MEDALLION;
        case TIG_ART_ARMOR_COVERAGE_SHIELD:
            return ITEM_INV_LOC_SHIELD;
        case TIG_ART_ARMOR_COVERAGE_TORSO:
            return ITEM_INV_LOC_ARMOR;
        case TIG_ART_ARMOR_COVERAGE_GAUNTLETS:
            return ITEM_INV_LOC_GAUNTLET;
        case TIG_ART_ARMOR_COVERAGE_BOOTS:
            return ITEM_INV_LOC_BOOTS;
        }
    } else {
        if (type == OBJ_TYPE_WEAPON) {
            return ITEM_INV_LOC_WEAPON;
        }
        if (type == OBJ_TYPE_GENERIC
            && (obj_field_int32_get(obj, OBJ_F_GENERIC_FLAGS) & OGF_USES_TORCH_SHIELD_LOCATION) != 0) {
            return ITEM_INV_LOC_SHIELD;
        }
    }
    return -1;
}

// 0x465690
int64_t sub_465690(int64_t obj, int inventory_location)
{
    int cnt;
    int index;
    int64_t item_obj;

    cnt = obj_field_int32_get(obj, OBJ_F_CRITTER_INVENTORY_NUM);
    for (index = 0; index < cnt; index++) {
        item_obj = obj_arrayfield_handle_get(obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, index);
        if (!item_is_item(item_obj)) {
            tig_debug_printf("obj with d %d n %d contains object d %d n %d\n",
                obj_field_int32_get(obj, OBJ_F_DESCRIPTION),
                obj_field_int32_get(obj, OBJ_F_NAME),
                obj_field_int32_get(item_obj, OBJ_F_DESCRIPTION),
                obj_field_int32_get(item_obj, OBJ_F_NAME));
        }

        if (item_inventory_location_get(item_obj) == inventory_location) {
            return item_obj;
        }
    }

    return OBJ_HANDLE_NULL;
}

// 0x465760
void item_location_set(int64_t obj, int location)
{
    int64_t owner_obj;

    item_parent(obj, &owner_obj);
    item_remove(obj);
    item_insert(obj, owner_obj, location);
}

// 0x4657D0
const char* ammunition_type_get_name(int ammo_type)
{
    return item_ammunition_type_names[ammo_type];
}

// 0x4657E0
size_t ammunition_type_get_name_length(int ammo_type)
{
    return strlen(item_ammunition_type_names[ammo_type]);
}

// 0x465820
int item_ammo_quantity_get(int64_t obj, int ammo_type)
{
    int64_t ammo_obj;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_AMMO) {
        ammo_obj = obj;
    } else {
        ammo_obj = item_ammo_obj(obj, ammo_type);
    }

    if (ammo_obj != OBJ_HANDLE_NULL) {
        return obj_field_int32_get(ammo_obj, OBJ_F_AMMO_QUANTITY);
    } else {
        return 0;
    }
}

// 0x465870
int64_t item_ammo_obj(int64_t obj, int ammo_type)
{
    switch (obj_field_int32_get(obj, OBJ_F_TYPE)) {
    case OBJ_TYPE_PC:
    case OBJ_TYPE_NPC:
        return obj_field_handle_get(obj, dword_5B32A0[ammo_type]);
    case OBJ_TYPE_CONTAINER:
        return sub_462540(obj, dword_5B32B0[ammo_type], 0);
    default:
        return OBJ_HANDLE_NULL;
    }
}

// 0x4658E0
bool item_ammo_transfer(int64_t from_obj, int64_t to_obj, int qty, int ammo_type, int64_t ammo_obj)
{
    int remaining_qty;
    int64_t to_ammo_obj;

    if (qty == 0) {
        return true;
    }

    if (tig_net_is_active()
        && !tig_net_is_host()) {
        return false;
    }

    if (from_obj != OBJ_HANDLE_NULL) {
        if (ammo_obj == OBJ_HANDLE_NULL) {
            ammo_obj = item_ammo_obj(from_obj, ammo_type);
        }

        if (ammo_obj != OBJ_HANDLE_NULL) {
            remaining_qty = obj_field_int32_get(ammo_obj, OBJ_F_AMMO_QUANTITY);
            if (remaining_qty < qty) {
                return false;
            }

            if (to_obj != OBJ_HANDLE_NULL) {
                object_script_execute(to_obj, ammo_obj, OBJ_HANDLE_NULL, SAP_INSERT_ITEM, 0);
            }

            if (remaining_qty == qty) {
                object_destroy(ammo_obj);
            } else {
                mp_obj_field_int32_set(ammo_obj, OBJ_F_AMMO_QUANTITY, remaining_qty - qty);
            }
        }
    }

    if (to_obj != OBJ_HANDLE_NULL) {
        to_ammo_obj = item_ammo_obj(to_obj, ammo_type);
        if (to_ammo_obj != OBJ_HANDLE_NULL) {
            remaining_qty = obj_field_int32_get(ammo_obj, OBJ_F_AMMO_QUANTITY);
            mp_obj_field_int32_set(ammo_obj, OBJ_F_AMMO_QUANTITY, remaining_qty + qty);
        } else {
            to_ammo_obj = item_ammo_create(qty,
                ammo_type,
                obj_field_int64_get(to_obj, OBJ_F_LOCATION));
            item_transfer(to_ammo_obj, to_obj);
        }
    }

    sub_466D60(from_obj);
    sub_466D60(to_obj);

    return true;
}

// 0x465A40
int64_t item_ammo_create(int quantity, int ammo_type, int64_t loc)
{
    int64_t ammo_obj;

    if (mp_object_create(dword_5B32B0[ammo_type], loc, &ammo_obj)) {
        mp_obj_field_int32_set(ammo_obj, OBJ_F_AMMO_QUANTITY, quantity);
    }

    return ammo_obj;
}

// 0x465A90
const char* armor_coverage_type_get_name(int coverage_type)
{
    return item_armor_coverage_type_names[coverage_type];
}

// 0x465AA0
size_t armor_coverage_type_get_name_length(int coverage_type)
{
    return strlen(item_armor_coverage_type_names[coverage_type]);
}

// 0x465AE0
bool sub_465AE0(int64_t a1, int64_t a2, tig_art_id_t* art_id_ptr)
{
    tig_art_id_t current_aid;
    tig_art_id_t use_aid_fragment;
    int subtype;
    tig_art_id_t aid;

    if (a1 != OBJ_HANDLE_NULL) {
        current_aid = obj_field_int32_get(a2, OBJ_F_CURRENT_AID);
        use_aid_fragment = obj_field_int32_get(a1, OBJ_F_ITEM_USE_AID_FRAGMENT);
        subtype = tig_art_item_id_subtype_get(use_aid_fragment);
        aid = tig_art_critter_id_armor_set(current_aid, subtype);
        if (tig_art_critter_id_armor_get(aid) != subtype) {
            return false;
        }

        *art_id_ptr = tig_art_id_palette_set(aid, tig_art_id_palette_get(use_aid_fragment));
        return true;
    }

    current_aid = obj_field_int32_get(a2, OBJ_F_CURRENT_AID);
    use_aid_fragment = obj_field_int32_get(a2, OBJ_F_AID);
    aid = tig_art_critter_id_armor_set(current_aid, TIG_ART_ARMOR_TYPE_UNDERWEAR);
    *art_id_ptr = tig_art_id_palette_set(aid, tig_art_id_palette_get(use_aid_fragment));
    return true;
}

// 0x465BA0
int item_armor_ac_adj(int64_t item_obj, int64_t owner_obj, bool a3)
{
    int ac_adj;
    int magic_ac_adj;

    if (obj_field_int32_get(item_obj, OBJ_F_TYPE) != OBJ_TYPE_ARMOR) {
        return 0;
    }

    ac_adj = obj_field_int32_get(item_obj, OBJ_F_ARMOR_AC_ADJ);

    if (!a3
        || -obj_field_int32_get(item_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) > 0
        || item_is_identified(item_obj)) {
        magic_ac_adj = obj_field_int32_get(item_obj, OBJ_F_ARMOR_MAGIC_AC_ADJ);
        if (owner_obj != OBJ_HANDLE_NULL) {
            magic_ac_adj = item_adjust_magic(item_obj, owner_obj, magic_ac_adj);
        }
        ac_adj += magic_ac_adj;
    }

    return ac_adj;
}

// 0x465C30
int item_armor_coverage(int64_t obj)
{
    tig_art_id_t inv_aid;

    if (obj == OBJ_HANDLE_NULL
        || !item_is_item(obj)
        || obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_ARMOR) {
        return -1;
    }

    inv_aid = obj_field_int32_get(obj, OBJ_F_ITEM_INV_AID);
    return tig_art_item_id_armor_coverage_get(inv_aid);
}

// 0x465C90
unsigned int item_armor_size(int race)
{
    return item_race_to_armor_size_tbl[race];
}

// 0x465CA0
int item_weapon_ammo_type(int64_t item_id)
{
    if (item_id == OBJ_HANDLE_NULL
        || obj_field_int32_get(item_id, OBJ_F_TYPE) != OBJ_TYPE_WEAPON) {
        return 10000;
    }

    return obj_field_int32_get(item_id, OBJ_F_WEAPON_AMMO_TYPE);
}

// 0x465CF0
int item_weapon_magic_speed(int64_t item_obj, int64_t owner_obj)
{
    int speed_adj;
    int skill;
    int training;

    if (item_obj == OBJ_HANDLE_NULL) {
        return 10;
    }

    speed_adj = obj_field_int32_get(item_obj, OBJ_F_WEAPON_MAGIC_SPEED_ADJ);

    if (owner_obj != OBJ_HANDLE_NULL) {
        speed_adj = item_adjust_magic(item_obj, owner_obj, speed_adj);
        skill = item_weapon_skill(item_obj);
        if (IS_TECH_SKILL(skill)) {
            training = tech_skill_training_get(owner_obj, GET_TECH_SKILL(skill));
        } else {
            training = basic_skill_training_get(owner_obj, GET_BASIC_SKILL(skill));
        }
        if (training >= TRAINING_APPRENTICE) {
            speed_adj += 5;
        }
    }

    return speed_adj + obj_field_int32_get(item_obj, OBJ_F_WEAPON_SPEED_FACTOR);
}

// 0x465D80
int item_weapon_skill(int64_t obj)
{
    int tech_complexity;
    int ammo_type;

    if (obj == OBJ_HANDLE_NULL) {
        return SKILL_MELEE;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_WEAPON) {
        return SKILL_THROWING;
    }

    if ((obj_field_int32_get(obj, OBJ_F_WEAPON_FLAGS) & OWF_BOOMERANGS) != 0) {
        return SKILL_THROWING;
    }

    tech_complexity = -obj_field_int32_get(obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY);
    ammo_type = item_weapon_ammo_type(obj);
    if (tech_complexity > 0
        && obj_field_int32_get(obj, OBJ_F_WEAPON_RANGE) >= 3
        && (ammo_type == TIG_ART_AMMO_TYPE_BULLET
            || ammo_type == TIG_ART_AMMO_TYPE_FUEL
            || ammo_type == TIG_ART_AMMO_TYPE_CHARGE)) {
        return SKILL_FIREARMS;
    }

    if (ammo_type == TIG_ART_AMMO_TYPE_ARROW) {
        return SKILL_BOW;
    }

    return SKILL_MELEE;
}

// 0x465E30
int item_weapon_range(int64_t item_id, int64_t critter_id)
{
    int magic_range_adj;

    if (item_id == OBJ_HANDLE_NULL
        || obj_field_int32_get(item_id, OBJ_F_TYPE) != OBJ_TYPE_WEAPON) {
        return 1;
    }

    magic_range_adj = obj_field_int32_get(item_id, OBJ_F_WEAPON_MAGIC_RANGE_ADJ);
    return obj_field_int32_get(item_id, OBJ_F_WEAPON_RANGE) + item_adjust_magic(item_id, critter_id, magic_range_adj);
}

// 0x465E90
int item_weapon_min_strength(int64_t item_obj, int64_t critter_obj)
{
    int adj;
    int min_strength;
    unsigned int flags;
    int64_t main_weapon_obj;

    if (item_obj == OBJ_HANDLE_NULL) {
        return 1;
    }

    if (obj_field_int32_get(item_obj, OBJ_F_TYPE) != OBJ_TYPE_WEAPON) {
        return 1;
    }

    adj = obj_field_int32_get(item_obj, OBJ_F_WEAPON_MAGIC_MIN_STRENGTH_ADJ);
    if (critter_obj != OBJ_HANDLE_NULL) {
        adj = item_adjust_magic(item_obj, critter_obj, adj);
    }

    min_strength = obj_field_int32_get(item_obj, OBJ_F_WEAPON_MIN_STRENGTH) + adj;
    flags = obj_field_int32_get(item_obj, OBJ_F_WEAPON_FLAGS);
    if (critter_obj != OBJ_HANDLE_NULL
        && (flags & OWF_HAND_COUNT_FIXED) == 0) {
        main_weapon_obj = item_wield_get(critter_obj, ITEM_INV_LOC_SHIELD);
        if ((flags & OWF_TWO_HANDED) != 0) {
            if (main_weapon_obj != OBJ_HANDLE_NULL) {
                min_strength += 4;
            }
        } else {
            if (main_weapon_obj == OBJ_HANDLE_NULL) {
                min_strength -= 2;
            }
        }
    }

    return min_strength;
}

// TODO: Lots of jumps, check.
//
// 0x465F70
void item_weapon_damage(int64_t weapon_obj, int64_t critter_obj, int damage_type, int skill, bool a5, int* min_dam_ptr, int* max_dam_ptr)
{
    int min_dam;
    int max_dam;
    int massive_dam;
    int bonus_dam;
    int v1;
    int64_t gauntlet_obj;
    int unarmed_dam;

    if (skill == SKILL_MELEE) {
        bonus_dam = stat_level_get(critter_obj, STAT_DAMAGE_BONUS);
        v1 = sub_4C6510(critter_obj);
    }

    if (weapon_obj != OBJ_HANDLE_NULL) {
        if (skill == SKILL_THROWING) {
            item_damage_min_max(weapon_obj, damage_type, &min_dam, &max_dam);
        } else {
            min_dam = obj_arrayfield_int32_get(weapon_obj, OBJ_F_WEAPON_DAMAGE_LOWER_IDX, damage_type);
            max_dam = obj_arrayfield_int32_get(weapon_obj, OBJ_F_WEAPON_DAMAGE_UPPER_IDX, damage_type);
        }
    } else {
        unarmed_dam = 0;

        if (skill == SKILL_MELEE) {
            gauntlet_obj = item_wield_get(critter_obj, ITEM_INV_LOC_GAUNTLET);
            if (gauntlet_obj != OBJ_HANDLE_NULL) {
                unarmed_dam = obj_field_int32_get(gauntlet_obj, OBJ_F_ARMOR_UNARMED_BONUS_DAMAGE);
            }
        }

        if (critter_is_monstrous(critter_obj)) {
            min_dam = obj_arrayfield_uint32_get(critter_obj, OBJ_F_NPC_DAMAGE_IDX, 2 * damage_type);
            max_dam = obj_arrayfield_uint32_get(critter_obj, OBJ_F_NPC_DAMAGE_IDX, 2 * damage_type + 1);
        } else {
            if (damage_type != DAMAGE_TYPE_NORMAL && damage_type != DAMAGE_TYPE_FATIGUE) {
                *min_dam_ptr = 0;
                *max_dam_ptr = 0;
                return;
            }

            max_dam = unarmed_dam + 5;
            min_dam = unarmed_dam - 25;

            if (min_dam < 1) {
                min_dam = 1;
            }
        }
    }

    if (skill == SKILL_MELEE) {
        if (v1 != 100) {
            max_dam = min_dam + (v1 * (max_dam - min_dam) + 50) / 100;
        }
    }

    if (skill == SKILL_MELEE && weapon_obj == OBJ_HANDLE_NULL) {
        massive_dam = 2 * max_dam;
    } else {
        massive_dam = 3 * max_dam;
    }

    if ((damage_type == DAMAGE_TYPE_NORMAL || damage_type == DAMAGE_TYPE_FATIGUE)
        && skill == SKILL_MELEE) {
        if (min_dam <= 0 || min_dam + bonus_dam > 0) {
            min_dam += bonus_dam;
        } else {
            min_dam = 1;
        }

        if (max_dam <= 0 || max_dam + bonus_dam > 0) {
            max_dam += bonus_dam;
        } else {
            max_dam = 1;
        }
    }

    if (weapon_obj != OBJ_HANDLE_NULL && !a5) {
        if (obj_field_int32_get(weapon_obj, OBJ_F_TYPE) == OBJ_TYPE_WEAPON) {
            int adj = obj_arrayfield_int32_get(weapon_obj, OBJ_F_WEAPON_MAGIC_DAMAGE_ADJ_IDX, damage_type);
            adj = item_adjust_magic(weapon_obj, critter_obj, adj);
            min_dam += adj;
            max_dam += adj;
        }
    }

    if (min_dam < 0) {
        min_dam = 0;
    }

    if (min_dam > massive_dam) {
        min_dam = massive_dam;
    }

    if (max_dam < 0) {
        max_dam = 0;
    }

    if (max_dam > massive_dam) {
        max_dam = massive_dam;
    }

    *min_dam_ptr = min_dam;
    *max_dam_ptr = max_dam;
}

// 0x466230
int item_weapon_aoe_radius(int64_t obj)
{
    if (obj_field_int32_get(obj, OBJ_F_ITEM_SPELL_1) != 10000) {
        return 0;
    }

    return sub_4502B0(mt_item_spell_to_magictech_spell(10000));
}

// 0x466260
void item_inventory_slots_get(int64_t obj, int* slots)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int capacity;
    int cnt;
    int idx;
    int64_t item_obj;
    int inventory_location;

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
        capacity = 960;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
        capacity = 120;
    }

    memset(slots, 0, sizeof(*slots) * capacity);

    cnt = obj_field_int32_get(obj, inventory_num_fld);
    for (idx = 0; idx < cnt; idx++) {
        item_obj = obj_arrayfield_handle_get(obj, inventory_list_fld, idx);
        inventory_location = item_inventory_location_get(item_obj);
        item_inventory_slots_set(item_obj, inventory_location, slots, idx + 1);
    }
}

// 0x466310
void item_inventory_slots_set(int64_t item_obj, int inventory_location, int* slots, int value)
{
    int x;
    int y;
    int width;
    int height;

    if (IS_WEAR_INV_LOC(inventory_location)) {
        return;
    }

    if (IS_HOTKEY_INV_LOC(inventory_location)) {
        return;
    }

    item_inv_icon_size(item_obj, &width, &height);

    slots += inventory_location;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            slots[y * 10 + x] = value;
        }
    }
}

// 0x466390
bool item_inventory_slots_has_room_for(int64_t item_obj, int64_t parent_obj, int inventory_location, int* slots)
{
    int num_rows;
    int x;
    int y;
    int width;
    int height;
    int64_t existing_item_obj;

    if (IS_WEAR_INV_LOC(inventory_location)) {
        return false;
    }

    if (IS_HOTKEY_INV_LOC(inventory_location)) {
        return false;
    }

    num_rows = obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER ? 96 : 12;

    if (sub_462410(item_obj, NULL) != -1) {
        existing_item_obj = item_find_first_matching_prototype(parent_obj, item_obj);
        if (existing_item_obj != OBJ_HANDLE_NULL
            && existing_item_obj != item_obj) {
            return true;
        }
    }

    item_inv_icon_size(item_obj, &width, &height);

    if (inventory_location % 10 + width > 10) {
        return false;
    }

    if (inventory_location / 10 + height > num_rows) {
        return false;
    }

    slots += inventory_location;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (slots[y * 10 + x] != 0) {
                return false;
            }
        }
    }

    return true;
}

// 0x4664C0
int item_find_free_inv_loc_for_insertion(int64_t item_obj, int64_t parent_obj)
{
    int slots[960];

    item_inventory_slots_get(parent_obj, slots);
    return find_free_inv_loc_horizontal(item_obj, parent_obj, slots);
}

// 0x466510
int item_check_insert(int64_t item_obj, int64_t parent_obj, int* inventory_location_ptr)
{
    int inventory_location;

    inventory_location = -1;
    if (obj_type_is_critter(obj_field_int32_get(parent_obj, OBJ_F_TYPE))) {
        if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_KEY
            && item_find_key_ring(parent_obj) != OBJ_HANDLE_NULL) {
            inventory_location = 0;
        } else {
            if (item_total_weight(parent_obj) + item_weight(item_obj, parent_obj) > stat_level_get(parent_obj, STAT_CARRY_WEIGHT)) {
                return ITEM_CANNOT_TOO_HEAVY;
            }
        }

        if (obj_field_int32_get(item_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY) < 0
            && background_get(parent_obj) == BACKGROUND_TECHNOPHOBIA) {
            return ITEM_CANNOT_PICKUP_TECH_ITEMS;
        }
    }

    if (sub_462410(item_obj, NULL) != -1
        && item_find_first_matching_prototype(parent_obj, item_obj) != OBJ_HANDLE_NULL) {
        inventory_location = 0;
    } else {
        if (inventory_location == -1) {
            inventory_location = item_find_free_inv_loc_for_insertion(item_obj, parent_obj);
            if (inventory_location == -1) {
                return ITEM_CANNOT_NO_ROOM;
            }
        }
    }

    if (inventory_location_ptr != NULL) {
        *inventory_location_ptr = inventory_location;
    }

    return ITEM_CANNOT_OK;
}

// 0x466640
void item_insert(int64_t item_obj, int64_t parent_obj, int inventory_location)
{
    char name[256];
    ItemInsertInfo* insert_info;
    unsigned int item_flags;
    int parent_obj_type;
    int item_obj_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int stack_fld;
    int qty_fld;
    int encumbrance_level;
    int64_t existing_item_obj;
    int existing_qty;
    int qty;
    int cnt;

    if (tig_net_is_active()) {
        if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_MP_INSERTED) != 0) {
            object_examine(item_obj, item_obj, name);
            tig_debug_printf("MP: Item: cannot item_insert( %s ) it is already flagged OIF_MP_INSERTED.\n", name);
            return;
        }

        if (!multiplayer_is_locked()) {
            insert_info = (ItemInsertInfo*)MALLOC(sizeof(*insert_info));
            insert_info->item_obj = item_obj;
            insert_info->parent_obj = parent_obj;
            insert_info->inventory_location = inventory_location;
            sub_4A3230(obj_get_id(item_obj),
                item_insert_success,
                insert_info,
                item_insert_failure,
                insert_info);

            item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
            item_flags |= OIF_MP_INSERTED;
            obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, item_flags);
        }
    }

    if (inventory_location == -1) {
        tig_debug_printf("Item: item_insert: ERROR: Attempt to insert an item at Location -1!\n");
    }

    parent_obj_type = obj_field_int32_get(parent_obj, OBJ_F_TYPE);
    item_obj_type = obj_field_int32_get(item_obj, OBJ_F_TYPE);

    if (parent_obj_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        encumbrance_level = critter_encumbrance_level_get(parent_obj);

        if (item_obj_type == OBJ_TYPE_KEY_RING) {
            sub_466AA0(parent_obj, item_obj);
        }

        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    stack_fld = sub_462410(item_obj, &qty_fld);
    if (stack_fld != -1) {
        existing_item_obj = item_find_first_matching_prototype(parent_obj, item_obj);
        if (existing_item_obj != OBJ_HANDLE_NULL) {
            qty = obj_field_int32_get(item_obj, qty_fld);
            existing_qty = obj_field_int32_get(existing_item_obj, qty_fld);
            obj_field_int32_set(existing_item_obj, qty_fld, existing_qty + qty);

            object_drop(item_obj, obj_field_int64_get(parent_obj, OBJ_F_LOCATION));
            sub_466D60(parent_obj);
            object_script_execute(parent_obj, item_obj, OBJ_HANDLE_NULL, SAP_INSERT_ITEM, 0);
            object_destroy(item_obj);
            return;
        }

        if (obj_type_is_critter(parent_obj_type)) {
            obj_field_handle_set(parent_obj, stack_fld, item_obj);
        }
    }

    cnt = obj_field_int32_get(parent_obj, inventory_num_fld);
    obj_field_int32_set(parent_obj, inventory_num_fld, cnt + 1);

    obj_arrayfield_obj_set(parent_obj, inventory_list_fld, cnt, item_obj);

    obj_field_int32_set(item_obj, OBJ_F_ITEM_INV_LOCATION, inventory_location);
    obj_field_handle_set(item_obj, OBJ_F_ITEM_PARENT, parent_obj);

    mt_item_notify_pickup(item_obj, parent_obj);

    if (IS_WEAR_INV_LOC(inventory_location)) {
        item_equipped(item_obj, parent_obj, inventory_location);
    }

    if (parent_obj_type != OBJ_TYPE_CONTAINER) {
        critter_encumbrance_level_recalc(parent_obj, encumbrance_level);
    }

    if (parent_obj_type == OBJ_TYPE_NPC) {
        unsigned int npc_flags = obj_field_int32_get(parent_obj, OBJ_F_NPC_FLAGS);
        npc_flags |= ONF_CHECK_WIELD;
        obj_field_int32_set(parent_obj, OBJ_F_NPC_FLAGS, npc_flags);
    }

    sub_466D60(parent_obj);
    object_script_execute(parent_obj, item_obj, OBJ_HANDLE_NULL, SAP_INSERT_ITEM, 0);
    mt_ai_notify_inventory_changed(parent_obj);

    if (player_is_local_pc_obj(parent_obj)) {
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_INVENTORY, true);

        if (IS_HOTKEY_INV_LOC(inventory_location)) {
            sub_460590(item_obj, inventory_location);
        }

        ui_notify_item_inserted_or_removed(item_obj, false, inventory_location);
    }

    if (parent_obj_type != OBJ_TYPE_CONTAINER
        && item_obj_type == OBJ_TYPE_KEY) {
        sub_466A00(parent_obj, item_obj);
    }
}

// 0x466A00
bool sub_466A00(int64_t a1, int64_t key_obj)
{
    int64_t key_ring_obj;

    key_ring_obj = item_find_key_ring(a1);
    if (key_ring_obj == OBJ_HANDLE_NULL) {
        return false;
    }

    sub_466A50(key_obj, key_ring_obj);
    sub_466BD0(key_ring_obj);

    return true;
}

// 0x466A50
void sub_466A50(int64_t key_obj, int64_t key_ring_obj)
{
    int index;
    int key_id;

    index = obj_arrayfield_length_get(key_ring_obj, OBJ_F_KEY_RING_LIST_IDX);
    key_id = obj_field_int32_get(key_obj, OBJ_F_KEY_KEY_ID);
    obj_arrayfield_uint32_set(key_ring_obj,
        OBJ_F_KEY_RING_LIST_IDX,
        index,
        key_id);
    object_destroy(key_obj);
}

// 0x466AA0
void sub_466AA0(int64_t critter_obj, int64_t a2)
{
    int64_t key_ring_obj;
    int cnt;
    int64_t item_obj;
    int index;
    int key_id;

    key_ring_obj = item_find_key_ring(critter_obj);
    if (key_ring_obj != OBJ_HANDLE_NULL) {
        index = obj_arrayfield_length_get(key_ring_obj, OBJ_F_KEY_RING_LIST_IDX);
        cnt = obj_arrayfield_length_get(a2, OBJ_F_KEY_RING_LIST_IDX);
        while (cnt > 0) {
            key_id = obj_arrayfield_uint32_get(a2, OBJ_F_KEY_RING_LIST_IDX, cnt - 1);
            obj_arrayfield_length_set(a2, OBJ_F_KEY_RING_LIST_IDX, cnt - 1);
            obj_arrayfield_uint32_set(key_ring_obj, OBJ_F_KEY_RING_LIST_IDX, index++, key_id);
            cnt--;
        }

        sub_466BD0(key_ring_obj);
        sub_466BD0(a2);
    } else {
        cnt = obj_field_int32_get(critter_obj, OBJ_F_CRITTER_INVENTORY_NUM);
        while (cnt > 0) {
            item_obj = obj_arrayfield_handle_get(critter_obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, cnt - 1);
            if (obj_field_int32_get(item_obj, OBJ_F_TYPE) == OBJ_TYPE_KEY) {
                sub_466A50(item_obj, a2);
            }
            cnt--;
        }
        sub_466BD0(a2);
    }
}

// 0x466BD0
void sub_466BD0(int64_t key_ring_obj)
{
    tig_art_id_t aid;

    if (obj_arrayfield_length_get(key_ring_obj, OBJ_F_KEY_RING_LIST_IDX) != 0) {
        tig_art_item_id_create(0, 1, 0, 0, 0, 7, 0, 0, &aid);
    } else {
        tig_art_item_id_create(1, 1, 0, 0, 0, 7, 0, 0, &aid);
    }

    obj_field_int32_set(key_ring_obj, OBJ_F_ITEM_INV_AID, aid);
}

// 0x466C40
bool item_insert_success(void* userinfo)
{
    ItemInsertInfo* item_insert_info = (ItemInsertInfo*)userinfo;
    Packet24 pkt;
    unsigned int item_flags;

    if (item_insert_info != NULL) {
        pkt.type = 24;
        sub_4F0640(item_insert_info->item_obj, &(pkt.item_oid));
        sub_4F0640(item_insert_info->parent_obj, &(pkt.parent_oid));
        item_insert_info->inventory_location = item_insert_info->inventory_location;

        multiplayer_lock();
        item_insert(item_insert_info->item_obj, item_insert_info->parent_obj, item_insert_info->inventory_location);
        item_flags = obj_field_int32_get(item_insert_info->item_obj, OBJ_F_ITEM_FLAGS);
        item_flags |= OIF_MP_INSERTED;
        obj_field_int32_set(item_insert_info->item_obj, OBJ_F_ITEM_FLAGS, item_flags);
        multiplayer_unlock();

        tig_net_send_app_all(&pkt, sizeof(pkt));

        FREE(item_insert_info);
    }

    return true;
}

// 0x466CF0
bool item_insert_failure(void* userinfo)
{
    ItemInsertInfo* item_insert_info = (ItemInsertInfo*)userinfo;
    char name[256];
    unsigned int flags;

    object_examine(item_insert_info->item_obj, item_insert_info->item_obj, name);
    tig_debug_printf("MP: Item: item_insert_failure: removing from %s OIF_MP_INSERTED.\n", name);
    flags = obj_field_int32_get(item_insert_info->item_obj, OBJ_F_ITEM_FLAGS);
    flags &= ~OIF_MP_INSERTED;
    obj_field_int32_set(item_insert_info->item_obj, OBJ_F_ITEM_FLAGS, flags);
    FREE(item_insert_info);

    return true;
}

// 0x466D60
void sub_466D60(int64_t obj)
{
    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    anim_speed_recalc(obj);

    if (player_is_local_pc_obj(obj)) {
        sub_4605D0();
    }

    ui_update_inven(obj);
}

// 0x466DA0
int item_check_remove(int64_t obj)
{
    int inventory_location;

    if ((obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS) & OIF_NO_DROP) != 0) {
        return ITEM_CANNOT_NOT_DROPPABLE;
    }

    inventory_location = item_inventory_location_get(obj);
    if (IS_WEAR_INV_LOC(inventory_location)) {
        return sub_465010(obj);
    }

    return ITEM_CANNOT_OK;
}

// 0x466E00
void item_remove(int64_t item_obj)
{
    int64_t parent_obj;

    if (!item_parent(item_obj, &parent_obj)) {
        tig_debug_printf("Warning: item_remove called on item that doesn't think it has a parent.");
        return;
    }

    item_force_remove(item_obj, parent_obj);
}

// 0x466E50
void sub_466E50(int64_t obj, int64_t loc)
{
    if (tig_net_is_active()
        && !tig_net_is_host()) {
        Packet27 pkt;

        pkt.type = 27;
        sub_4F0640(obj, &(pkt.oid));
        pkt.loc = loc;
        tig_net_send_app_all(&pkt, sizeof(pkt));
        return;
    }

    if (!sub_466EF0(obj, loc)) {
        object_drop(obj, loc);
    }

    item_decay(obj, 172800000);
}

// 0x466EF0
bool sub_466EF0(int64_t obj, int64_t loc)
{
    ObjectList objects;
    ObjectNode* node;
    int inventory_location;
    int64_t new_container_obj;
    bool rc = false;

    object_list_location(loc, OBJ_TM_CONTAINER, &objects);
    node = objects.head;
    while (node != NULL) {
        if (sub_49B290(node->obj) == BP_JUNK_PILE) {
            inventory_location = item_find_free_inv_loc_for_insertion(obj, node->obj);
            if (inventory_location != -1) {
                item_insert(obj, node->obj, inventory_location);
                object_list_destroy(&objects);
                return true;
            }
        }
        node = node->next;
    }
    object_list_destroy(&objects);

    new_container_obj = OBJ_HANDLE_NULL;
    object_list_location(loc, OBJ_TM_ITEM, &objects);
    node = objects.head;
    while (node != NULL) {
        if (new_container_obj == OBJ_HANDLE_NULL) {
            if (!mp_object_create(BP_JUNK_PILE, loc, &new_container_obj)) {
                if (!tig_net_is_active()
                    || tig_net_is_host()) {
                    rc = false;
                } else {
                    rc = true;
                }
                break;
            }
        }

        item_transfer(node->obj, new_container_obj);

        node = node->next;
    }
    object_list_destroy(&objects);

    if (new_container_obj != OBJ_HANDLE_NULL) {
        inventory_location = item_find_free_inv_loc_for_insertion(obj, new_container_obj);
        if (inventory_location != -1) {
            item_insert(obj, new_container_obj, inventory_location);
            return true;
        }
    }

    return rc;
}

// 0x4670A0
void item_arrange_inventory(int64_t parent_obj, bool vertical)
{
    int inventory_num_fld;
    int inventory_list_fld;
    int cnt;
    int idx;
    int64_t item_obj;
    int64_t items[960];
    int slots[960];
    int inventory_locations[960];
    int inventory_location;

    if (tig_net_is_active() && !tig_net_is_host()) {
        mp_item_arrange_inventory(parent_obj, vertical);
        return;
    }

    if (obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
    }

    cnt = obj_field_int32_get(parent_obj, inventory_num_fld);
    if (cnt == 0) {
        return;
    }

    for (idx = 0; idx < cnt; idx++) {
        items[idx] = obj_arrayfield_handle_get(parent_obj, inventory_list_fld, idx);
    }

    for (idx = 0; idx < cnt; idx++) {
        inventory_location = item_inventory_location_get(items[idx]);
        if (IS_WEAR_INV_LOC(inventory_location)
            || IS_HOTKEY_INV_LOC(inventory_location)) {
            item_obj = items[cnt - 1];
            items[cnt - 1] = items[idx];
            items[idx] = item_obj;
            cnt--;
            idx--;
        }
    }

    if (cnt == 0) {
        return;
    }

    for (idx = 1; idx < cnt; idx++) {
        int prev_width;
        int prev_height;
        int width;
        int height;

        item_inv_icon_size(items[idx - 1], &prev_width, &prev_height);
        item_inv_icon_size(items[idx], &width, &height);

        if (height > prev_height
            || (height == prev_height && width > prev_width)) {
            item_obj = items[idx - 1];
            items[idx - 1] = items[idx];
            items[idx] = item_obj;
        }
    }

    memset(slots, 0, sizeof(slots));

    for (idx = 0; idx < cnt; idx++) {
        inventory_locations[idx] = vertical
            ? find_free_inv_loc_vertical(items[idx], parent_obj, slots)
            : find_free_inv_loc_horizontal(items[idx], parent_obj, slots);
        if (inventory_locations[idx] == -1) {
            return;
        }

        item_inventory_slots_set(items[idx], inventory_locations[idx], slots, 1);
    }

    for (idx = 0; idx < cnt; idx++) {
        mp_obj_field_int32_set(items[idx], OBJ_F_ITEM_INV_LOCATION, inventory_locations[idx]);
    }

    mp_ui_update_inven(parent_obj);
}

// 0x4673B0
char* item_error_str(int reason)
{
    MesFileEntry mes_file_entry;

    if (reason <= 0) {
        return NULL;
    }

    mes_file_entry.num = reason + 100;
    mes_get_msg(item_mes_file, &mes_file_entry);

    return mes_file_entry.str;
}

// 0x4673F0
void item_error_msg(int64_t obj, int reason)
{
    UiMessage ui_message;
    char* str;

    if (!player_is_local_pc_obj(obj)) {
        return;
    }

    str = item_error_str(reason);
    if (str == NULL) {
        return;
    }

    ui_message.type = UI_MSG_TYPE_EXCLAMATION;
    ui_message.str = str;
    sub_460630(&ui_message);
}

// 0x467440
void item_perform_identify_service(int64_t item_obj, int64_t npc_obj, int64_t pc_obj, int cost)
{
    unsigned int flags;

    if (tig_net_is_active() && !tig_net_is_host()) {
        PacketPerformIdentifyService pkt;

        pkt.type = 125;
        sub_4F0640(item_obj, &(pkt.item_oid));
        sub_4F0640(npc_obj, &(pkt.npc_oid));
        sub_4F0640(pc_obj, &(pkt.pc_oid));
        pkt.cost = cost;
        tig_net_send_app_all(&pkt, sizeof(pkt));
        return;
    }

    flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
    flags |= OIF_IDENTIFIED;
    obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, flags);

    item_gold_transfer(pc_obj, npc_obj, cost, OBJ_HANDLE_NULL);

    sub_4EE3A0(pc_obj, item_obj);
}

// 0x467520
void sub_467520(int64_t obj)
{
    int cnt;
    int index;
    int64_t item_obj;
    int inventory_location;

    if (tig_net_is_active()) {
        cnt = obj_field_int32_get(obj, OBJ_F_CRITTER_INVENTORY_NUM);
        for (index = cnt - 1; index >= 0; index--) {
            item_obj = obj_arrayfield_handle_get(obj, OBJ_F_CRITTER_INVENTORY_LIST_IDX, index);
            inventory_location = item_inventory_location_get(item_obj);
            if (IS_HOTKEY_INV_LOC(inventory_location)) {
                item_drop(item_obj);
                item_transfer(item_obj, obj);
            }
        }
    }
}

// 0x4675A0
int find_free_inv_loc_horizontal(int64_t item_obj, int64_t parent_obj, int* slots)
{
    int width;
    int height;
    int rows;
    int max_x;
    int max_y;
    int x;
    int y;
    int nx;
    int ny;
    bool occupied;

    item_inv_icon_size(item_obj, &width, &height);

    if (obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        rows = 96;
    } else {
        rows = 12;
    }

    max_x = 10 - width;
    max_y = rows - height;

    for (y = 0; y <= max_y; y++) {
        for (x = 0; x <= max_x; x++) {
            occupied = false;
            for (ny = 0; ny < height; ny++) {
                for (nx = 0; nx < width; nx++) {
                    if (slots[x + y * 10 + nx + ny * 10] != 0) {
                        occupied = true;
                        break;
                    }
                }

                if (occupied) {
                    break;
                }
            }

            if (!occupied) {
                return x + y * 10;
            }
        }
    }

    return -1;
}

// 0x4676A0
int find_free_inv_loc_vertical(int64_t item_obj, int64_t parent_obj, int* slots)
{
    int width;
    int height;
    int rows;
    int max_x;
    int max_y;
    int x;
    int y;
    int nx;
    int ny;
    bool occupied;

    item_inv_icon_size(item_obj, &width, &height);

    if (obj_field_int32_get(parent_obj, OBJ_F_TYPE) == OBJ_TYPE_CONTAINER) {
        rows = 96;
    } else {
        rows = 12;
    }

    max_x = 10 - width;
    max_y = rows - height;

    for (x = 0; x <= max_x; x++) {
        for (y = 0; y <= max_y; y++) {
            occupied = false;
            for (ny = 0; ny < height; ny++) {
                for (nx = 0; nx < width; nx++) {
                    if (slots[x + y * 10 + nx + ny * 10] != 0) {
                        occupied = true;
                        break;
                    }
                }

                if (occupied) {
                    break;
                }
            }

            if (!occupied) {
                return x + y * 10;
            }
        }
    }

    return -1;
}

// 0x4677B0
void item_equipped(int64_t item_obj, int64_t parent_obj, int inventory_location)
{
    tig_art_id_t aid;

    switch (inventory_location) {
    case ITEM_INV_LOC_WEAPON:
    case ITEM_INV_LOC_SHIELD:
        aid = sub_465020(parent_obj);
        object_set_current_aid(parent_obj, aid);

        if (player_is_local_pc_obj(parent_obj)) {
            sub_4605D0();
        }
        break;
    case ITEM_INV_LOC_ARMOR:
        if (sub_465AE0(item_obj, parent_obj, &aid)) {
            object_set_current_aid(parent_obj, aid);
        }
        break;
    }

    mt_item_notify_wear(item_obj, parent_obj);
    item_recalc_light(item_obj, parent_obj);
    object_script_execute(parent_obj, item_obj, OBJ_HANDLE_NULL, SAP_WIELD_ON, 0);
}

// 0x467860
void item_force_remove(int64_t item_obj, int64_t parent_obj)
{
    bool is_pc = false;
    char name[256];
    unsigned int item_flags;
    int64_t actual_parent_obj;
    int parent_type;
    int inventory_num_fld;
    int inventory_list_fld;
    int stack_fld;
    int encubmrance_level;
    int cnt;
    int idx;
    int inventory_location;

    dword_5E8820 = false;

    if (tig_net_is_active()) {
        if ((obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_MP_INSERTED) == 0) {
            if ((obj_field_int32_get(item_obj, OBJ_F_FLAGS) & OF_INVENTORY) == 0) {
                object_examine(item_obj, item_obj, name);
                tig_debug_printf("MP: Item: cannot item_force_remove( %s ) it is not flagged OIF_MP_INSERTED.\n", name);
                return;
            }

            item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
            item_flags |= OIF_MP_INSERTED;
            obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, item_flags);
        }

        if (!multiplayer_is_locked()) {
            ItemRemoveInfo* remove_info = (ItemRemoveInfo*)MALLOC(sizeof(*remove_info));
            remove_info->item_obj = item_obj;
            remove_info->parent_obj = parent_obj;
            sub_4A3230(obj_get_id(item_obj),
                item_force_remove_success,
                remove_info,
                item_force_remove_failure,
                remove_info);

            item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
            item_flags &= ~OIF_MP_INSERTED;
            obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, item_flags);

            return;
        }

        item_flags = obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS);
        item_flags &= ~OIF_MP_INSERTED;
        obj_field_int32_set(item_obj, OBJ_F_ITEM_FLAGS, item_flags);
    }

    if (item_parent(item_obj, &actual_parent_obj)) {
        if (actual_parent_obj != parent_obj) {
            dword_5E8820 = true;
            tig_debug_printf("Warning: item_force_remove called on item with different parent.\n");
        }
        is_pc = player_is_local_pc_obj(parent_obj);
    } else {
        dword_5E8820 = true;
        tig_debug_printf("Warning: item_force_remove called on item that doesn't think it has a parent.\n");
    }

    magictech_demaintain_spells(item_obj);

    parent_type = obj_field_int32_get(parent_obj, OBJ_F_TYPE);

    if (parent_type == OBJ_TYPE_CONTAINER) {
        inventory_num_fld = OBJ_F_CONTAINER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CONTAINER_INVENTORY_LIST_IDX;
    } else {
        stack_fld = sub_462410(item_obj, NULL);
        if (stack_fld != -1) {
            obj_field_handle_set(parent_obj, stack_fld, OBJ_HANDLE_NULL);
        }

        inventory_num_fld = OBJ_F_CRITTER_INVENTORY_NUM;
        inventory_list_fld = OBJ_F_CRITTER_INVENTORY_LIST_IDX;
        encubmrance_level = critter_encumbrance_level_get(parent_obj);
    }

    cnt = obj_field_int32_get(parent_obj, inventory_num_fld);
    for (idx = 0; idx < cnt; idx++) {
        if (item_obj == obj_arrayfield_handle_get(parent_obj, inventory_list_fld, idx)) {
            break;
        }
    }

    if (idx >= cnt) {
        dword_5E8820 = true;
        tig_debug_printf("Item: item_force_remove: ERROR: Couldn't match object in parent!\n");
        return;
    }

    inventory_location = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_LOCATION);

    cnt--;
    while (idx < cnt) {
        int64_t tmp_item_obj = obj_arrayfield_handle_get(parent_obj, inventory_list_fld, idx + 1);
        obj_arrayfield_obj_set(parent_obj, inventory_list_fld, idx, tmp_item_obj);
        idx++;
    }

    obj_arrayfield_length_set(parent_obj, inventory_list_fld, cnt);
    obj_field_int32_set(parent_obj, inventory_num_fld, cnt);

    if (!dword_5E8820) {
        obj_field_int32_set(item_obj, OBJ_F_ITEM_INV_LOCATION, -1);
    }

    if (IS_WEAR_INV_LOC(inventory_location)) {
        item_unequipped(item_obj, parent_obj, inventory_location);
    } else if (IS_HOTKEY_INV_LOC(inventory_location)) {
        if (is_pc) {
            sub_460540(inventory_location - 2000);
        }
    }

    if (is_pc) {
        ui_notify_item_inserted_or_removed(item_obj, true, inventory_location);
    }

    mt_ai_notify_inventory_changed(parent_obj);
    mt_item_notify_drop(item_obj, parent_obj);

    if (parent_type == OBJ_TYPE_CONTAINER) {
        if (!item_editor && item_decay_process_cnt > 0) {
            sub_463540(parent_obj);
        }
    } else {
        critter_encumbrance_level_recalc(parent_obj, encubmrance_level);
        if (parent_type == OBJ_TYPE_NPC) {
            unsigned int npc_flags = obj_field_int32_get(parent_obj, OBJ_F_NPC_FLAGS);
            npc_flags |= ONF_CHECK_WIELD;
            obj_field_int32_set(parent_obj, OBJ_F_NPC_FLAGS, npc_flags);
        }
    }

    sub_466D60(parent_obj);
    object_script_execute(parent_obj, item_obj, OBJ_HANDLE_NULL, SAP_REMOVE_ITEM, 0);

    if (is_pc) {
        ui_toggle_primary_button(UI_PRIMARY_BUTTON_INVENTORY, true);
    }
}

// 0x467CB0
void item_unequipped(int64_t item_obj, int64_t parent_obj, int inventory_location)
{
    tig_art_id_t aid;

    switch (inventory_location) {
    case ITEM_INV_LOC_WEAPON:
    case ITEM_INV_LOC_SHIELD:
        aid = sub_465020(parent_obj);
        object_set_current_aid(parent_obj, aid);

        if (player_is_local_pc_obj(parent_obj)) {
            sub_4605D0();
        }
        break;
    case ITEM_INV_LOC_ARMOR:
        if (sub_465AE0(OBJ_HANDLE_NULL, parent_obj, &aid)) {
            object_set_current_aid(parent_obj, aid);
        }
        break;
    }

    mt_item_notify_unwear(item_obj, parent_obj);
    item_recalc_light(item_obj, parent_obj);
    object_script_execute(parent_obj, item_obj, OBJ_HANDLE_NULL, SAP_WIELD_OFF, 0);
}

// 0x467D60
bool item_force_remove_success(void* userinfo)
{
    ItemRemoveInfo* item_remove_info = (ItemRemoveInfo*)userinfo;
    Packet25 pkt;
    unsigned int item_flags;

    if (item_remove_info != NULL) {
        pkt.type = 25;
        sub_4F0640(item_remove_info->item_obj, &(pkt.item_oid));
        sub_4F0640(item_remove_info->parent_obj, &(pkt.parent_oid));

        multiplayer_lock();
        item_force_remove(item_remove_info->item_obj, item_remove_info->parent_obj);
        item_flags = obj_field_int32_get(item_remove_info->item_obj, OBJ_F_ITEM_FLAGS);
        item_flags &= ~OIF_MP_INSERTED;
        obj_field_int32_set(item_remove_info->item_obj, OBJ_F_ITEM_FLAGS, item_flags);
        multiplayer_unlock();

        tig_net_send_app_all(&pkt, sizeof(pkt));

        FREE(item_remove_info);
    }

    return true;
}

// 0x467E00
bool item_force_remove_failure(void* userinfo)
{
    ItemRemoveInfo* item_remove_info = (ItemRemoveInfo*)userinfo;
    char name[256];
    unsigned int flags;

    object_examine(item_remove_info->item_obj, item_remove_info->item_obj, name);
    tig_debug_printf("MP: Item: item_force_remove_failure: adding to %s OIF_MP_INSERTED.\n", name);
    flags = obj_field_int32_get(item_remove_info->item_obj, OBJ_F_ITEM_FLAGS);
    flags |= OIF_MP_INSERTED;
    obj_field_int32_set(item_remove_info->item_obj, OBJ_F_ITEM_FLAGS, flags);
    FREE(item_remove_info);

    return true;
}

// 0x467E70
bool sub_467E70()
{
    bool ret;

    ret = dword_5E8820;
    dword_5E8820 = false;

    return ret;
}

// 0x467E80
void item_recalc_light(int64_t item_obj, int64_t parent_obj)
{
    int inventory_location;
    int64_t inven_item_obj;
    unsigned int light_flags;
    unsigned int light_size;
    unsigned int critter_flags;
    int effectiveness;
    tig_art_id_t aid;
    tig_color_t color;

    if (item_obj != OBJ_HANDLE_NULL
        && (obj_field_int32_get(item_obj, OBJ_F_ITEM_FLAGS) & OIF_LIGHT_ANY) == 0) {
        return;
    }

    light_flags = 0;
    for (inventory_location = FIRST_WEAR_INV_LOC; inventory_location <= LAST_WEAR_INV_LOC; inventory_location++) {
        inven_item_obj = item_wield_get(parent_obj, inventory_location);
        if (inven_item_obj != OBJ_HANDLE_NULL) {
            light_size = obj_field_int32_get(inven_item_obj, OBJ_F_ITEM_FLAGS) & OIF_LIGHT_ANY;
            if (light_size != 0) {
                effectiveness = item_effective_power_ratio(inven_item_obj, parent_obj);
                if (effectiveness <= 25) {
                    light_size >>= 3;
                } else if (effectiveness <= 50) {
                    light_size >>= 2;
                } else if (effectiveness <= 75) {
                    light_size >>= 1;
                }
                light_flags |= light_size & OIF_LIGHT_ANY;
            }
        }
    }

    critter_flags = obj_field_int32_get(parent_obj, OBJ_F_CRITTER_FLAGS);
    critter_flags &= ~OCF_LIGHT_ANY;

    if ((light_flags & OIF_LIGHT_XLARGE) != 0) {
        critter_flags |= OCF_LIGHT_XLARGE;
    }

    if ((light_flags & OIF_LIGHT_LARGE) != 0) {
        critter_flags |= OCF_LIGHT_LARGE;
    }

    if ((light_flags & OIF_LIGHT_MEDIUM) != 0) {
        critter_flags |= OCF_LIGHT_MEDIUM;
    }

    if ((light_flags & OIF_LIGHT_SMALL) != 0) {
        critter_flags |= OCF_LIGHT_SMALL;
    }

    obj_field_int32_set(parent_obj, OBJ_F_CRITTER_FLAGS, critter_flags);

    aid = critter_light_get(parent_obj, &color);
    object_set_light(parent_obj, 0, aid, color);
}

// 0x467FC0
bool item_decay_timeevent_process(TimeEvent* timeevent)
{
    int64_t obj;

    obj = timeevent->params[0].object_value;
    if (item_decay_process_cnt > 0) {
        if (item_can_decay(obj)) {
            object_destroy(obj);
        }
    } else {
        item_decay(obj, 60000);
    }

    return true;
}

// 0x468010
bool item_can_decay(int64_t obj)
{
    int64_t owner_obj;

    if (obj == OBJ_HANDLE_NULL) {
        return false;
    }

    if (item_editor) {
        return false;
    }

    if ((obj_field_int32_get(obj, OBJ_F_ITEM_FLAGS) & OIF_NO_DECAY) != 0) {
        return false;
    }

    return !item_parent(obj, &owner_obj)
        || sub_49B290(owner_obj) == BP_JUNK_PILE;
}

// 0x468090
bool item_decay(int64_t obj, int ms)
{
    TimeEvent timeevent;
    DateTime datetime;

    if (!item_can_decay(obj)) {
        return false;
    }

    item_decay_test_obj = obj;
    timeevent_clear_all_ex(TIMEEVENT_TYPE_ITEM_DECAY, item_decay_timeevent_check);

    timeevent.type = TIMEEVENT_TYPE_ITEM_DECAY;
    timeevent.params[0].object_value = obj;
    timeevent.params[1].integer_value = sub_45A7F0();
    sub_45A950(&datetime, ms);
    return timeevent_add_delay(&timeevent, &datetime);
}

// 0x468110
bool item_cancel_decay(int64_t obj)
{
    if (item_can_decay(obj)) {
        return false;
    }

    item_decay_test_obj = obj;
    timeevent_clear_all_ex(TIMEEVENT_TYPE_ITEM_DECAY, item_decay_timeevent_check);

    return true;
}

// 0x468150
bool item_decay_timeevent_check(TimeEvent* timeevent)
{
    return timeevent != NULL && timeevent->params[0].object_value == item_decay_test_obj;
}

// 0x468180
void item_decay_process_enable()
{
    item_decay_process_cnt++;
}

// 0x468190
void item_decay_process_disable()
{
    item_decay_process_cnt--;
}

// 0x4681A0
int item_decay_process_is_enabled()
{
    return item_decay_process_cnt;
}
