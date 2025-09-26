#ifndef ARCANUM_GAME_OBJECT_H_
#define ARCANUM_GAME_OBJECT_H_

#include "game/context.h"
#include "game/location.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/object_node.h"
#include "game/timeevent.h"

#define OBJ_TM_WALL 0x00000001
#define OBJ_TM_PORTAL 0x00000002
#define OBJ_TM_CONTAINER 0x00000004
#define OBJ_TM_SCENERY 0x00000008
#define OBJ_TM_PROJECTILE 0x00000010
#define OBJ_TM_WEAPON 0x00000020
#define OBJ_TM_AMMO 0x00000040
#define OBJ_TM_ARMOR 0x00000080
#define OBJ_TM_GOLD 0x00000100
#define OBJ_TM_FOOD 0x00000200
#define OBJ_TM_SCROLL 0x00000400
#define OBJ_TM_KEY 0x00000800
#define OBJ_TM_KEY_RING 0x00001000
#define OBJ_TM_WRITTEN 0x00002000
#define OBJ_TM_GENERIC 0x00004000
#define OBJ_TM_PC 0x00008000
#define OBJ_TM_NPC 0x00010000
#define OBJ_TM_TRAP 0x00020000

#define OBJ_TM_ITEM (OBJ_TM_WEAPON \
    | OBJ_TM_AMMO                  \
    | OBJ_TM_ARMOR                 \
    | OBJ_TM_GOLD                  \
    | OBJ_TM_FOOD                  \
    | OBJ_TM_SCROLL                \
    | OBJ_TM_KEY                   \
    | OBJ_TM_KEY_RING              \
    | OBJ_TM_WRITTEN               \
    | OBJ_TM_GENERIC)

#define OBJ_TM_CRITTER (OBJ_TM_PC | OBJ_TM_NPC)

#define OBJ_TM_ALL (OBJ_TM_WALL \
    | OBJ_TM_PORTAL             \
    | OBJ_TM_CONTAINER          \
    | OBJ_TM_SCENERY            \
    | OBJ_TM_PROJECTILE         \
    | OBJ_TM_ITEM               \
    | OBJ_TM_PC                 \
    | OBJ_TM_NPC                \
    | OBJ_TM_TRAP)

#define RECT_FLAG_NONE 0x00000000
#define RECT_FLAG_SHADOWS 0x00000001
#define RECT_FLAG_OVERLAYS 0x00000002
#define RECT_FLAG_UNDERLAYS 0x00000004
#define RECT_FLAG_IGNORE_VIS 0x00000008
#define RECT_FLAG_ALL_LAYERS (RECT_FLAG_OVERLAYS | RECT_FLAG_UNDERLAYS)
#define RECT_FLAG_FULL (RECT_FLAG_SHADOWS | RECT_FLAG_OVERLAYS | RECT_FLAG_UNDERLAYS)

// TODO: Better name.
typedef struct Ryan {
    /* 0000 */ ObjectID objid;
    /* 0018 */ int64_t location;
    /* 0020 */ int map;
    /* 0024 */ int padding_24;
} Ryan;

// Serializeable.
static_assert(sizeof(Ryan) == 0x28, "wrong size");

// TODO: Better name.
typedef struct FollowerInfo {
    /* 0000 */ int64_t obj;
    /* 0008 */ Ryan field_8;
} FollowerInfo;

// Serializeable.
static_assert(sizeof(FollowerInfo) == 0x30, "wrong size");

typedef struct ObjectList {
    /* 0000 */ int num_sectors;
    /* 0008 */ int64_t sectors[9];
    /* 0050 */ ObjectNode* head;
} ObjectList;

extern int dword_5E2E68;
extern int dword_5E2E6C;
extern bool dword_5E2E94;
extern tig_art_id_t object_hover_underlay_art_id;
extern tig_color_t object_hover_color;
extern tig_art_id_t object_hover_overlay_art_id;
extern Ryan stru_5E2F60;
extern int64_t object_hover_obj;

bool object_init(GameInitInfo* init_info);
void object_resize(GameResizeInfo* resize_info);
void object_reset();
void object_exit();
void object_ping(tig_timestamp_t timestamp);
void object_update_view(ViewOptions* view_options);
void object_map_close();
bool object_type_is_enabled(int obj_type);
void object_type_toggle(int obj_type);
void object_draw(GameDrawInfo* draw_info);
void object_hover_obj_set(int64_t obj);
int64_t object_hover_obj_get();
void sub_43C690(GameDrawInfo* draw_info);
void object_invalidate_rect(TigRect* rect);
void object_flush();
bool object_create(int64_t proto_obj, int64_t loc, int64_t* obj_ptr);
bool object_create_ex(int64_t proto_obj, int64_t loc, ObjectID oid, int64_t* obj_ptr);
bool object_duplicate(int64_t proto_obj, int64_t loc, int64_t* obj_ptr);
bool object_duplicate_ex(int64_t proto_obj, int64_t loc, ObjectID* oids, int64_t* obj_ptr);
void object_destroy(int64_t obj);
void sub_43CF70(int64_t obj);
void object_delete(int64_t obj);
void object_flags_set(int64_t obj, unsigned int flags);
void object_flags_unset(int64_t obj, unsigned int flags);
int object_hp_pts_get(int64_t obj);
int object_hp_pts_set(int64_t obj, int value);
int object_hp_adj_get(int64_t obj);
int object_hp_adj_set(int64_t obj, int value);
int object_hp_damage_get(int64_t obj);
int object_hp_damage_set(int64_t obj, int value);
int object_hp_max(int64_t obj);
int object_hp_current(int64_t obj);
int object_get_resistance(int64_t obj, int resistance_type, bool a2);
int object_get_ac(int64_t obj, bool a2);
bool sub_43D940(int64_t obj);
bool object_is_static(int64_t obj);
bool sub_43D9F0(int x, int y, int64_t* obj_ptr, unsigned int flags);
void object_get_rect(int64_t obj, unsigned int flags, TigRect* rect);
void object_move_to_location(int64_t obj, int64_t loc, int offset_x, int offset_y);
bool object_teleported_timeevent_process(TimeEvent* timeevent);
void object_set_offset(int64_t obj, int offset_x, int offset_y);
void object_set_current_aid(int64_t obj, tig_art_id_t aid);
void object_set_light(int64_t obj, unsigned int flags, tig_art_id_t aid, tig_color_t color);
void object_overlay_set(int64_t obj, int fld, int index, tig_art_id_t aid);
bool object_scenery_respawn_timeevent_process(TimeEvent* timeevent);
void object_set_overlay_light(int64_t obj, int index, unsigned int flags, tig_art_id_t aid, tig_color_t color);
void object_set_blit_scale(int64_t obj, int value);
void object_add_flags(int64_t obj, unsigned int flags);
void object_remove_flags(int64_t obj, unsigned int flags);
void sub_43F030(int64_t obj);
void object_cycle_palette(int64_t obj);
void object_bust(int64_t obj, int64_t triggerer_obj);
bool object_is_busted(int64_t obj);
void sub_43F710(int64_t obj);
void object_inc_current_aid(int64_t obj);
void object_dec_current_aid(int64_t obj);
void object_eye_candy_aid_inc(int64_t obj, int fld, int index);
void object_eye_candy_aid_dec(int64_t obj, int fld, int index);
void object_overlay_light_frame_set_first(int64_t obj, int index);
void object_overlay_light_frame_set_last(int64_t obj, int index);
void object_overlay_light_frame_inc(int64_t obj, int index);
void object_overlay_light_frame_dec(int64_t obj, int index);
void object_cycle_rotation(int64_t obj);
bool sub_43FD70(int64_t a1, int64_t a2, int rotation, unsigned int flags, int* a5);
int sub_43FDC0(int64_t a1, int64_t a2, int rotation, unsigned int flags, int64_t* a5, int* a6, int* a7);
bool sub_440700(int64_t obj, int64_t loc, int rot, unsigned int flags, int64_t* block_obj_ptr);
void object_list_location(int64_t loc, unsigned int flags, ObjectList* objects);
void object_list_rect(LocRect* loc_rect, unsigned int flags, ObjectList* objects);
void object_list_vicinity(int64_t obj, unsigned int flags, ObjectList* objects);
void object_list_destroy(ObjectList* objects);
bool object_list_remove(ObjectList* objects, int64_t obj);
void object_list_followers(int64_t obj, ObjectList* objects);
void object_list_all_followers(int64_t obj, ObjectList* objects);
void object_list_party(int64_t obj, ObjectList* objects);
void object_list_team(int64_t obj, ObjectList* objects);
void object_list_copy(ObjectList* dst, ObjectList* src);
void object_drop(int64_t obj, int64_t loc);
void object_pickup(int64_t item_obj, int64_t parent_obj);
bool object_script_execute(int64_t triggerer_obj, int64_t attachee_obj, int64_t extra_obj, int a4, int a5);
int64_t object_dist(int64_t a, int64_t b);
int object_rot(int64_t a, int64_t b);
void object_examine(int64_t obj, int64_t pc_obj, char* buffer);
void object_set_gender_and_race(int64_t obj, int body_type, int gender, int race);
bool object_is_lockable(int64_t obj);
bool object_locked_get(int64_t obj);
bool object_locked_set(int64_t obj, bool locked);
bool object_lock_timeevent_process(TimeEvent* timeevent);
bool object_jammed_set(int64_t obj, bool jammed);
void sub_441FC0(int64_t obj, int a2);
void object_blit_flags_set(unsigned int flags);
int object_blit_flags_get();
void sub_442050(uint8_t** data_ptr, int* size_ptr, int64_t obj);
bool sub_4420D0(uint8_t* data, int64_t* obj_ptr, int64_t loc);
void sub_442520(int64_t obj);
void sub_443770(int64_t obj);
bool sub_4437E0(TigRect* rect);
bool object_save_obj_handle_safe(int64_t* obj_ptr, Ryan* a2, TigFile* stream);
bool object_load_obj_handle_safe(int64_t* obj_ptr, Ryan* a2, TigFile* stream);
void sub_443EB0(int64_t obj, Ryan* a2);
bool sub_443F80(int64_t* obj_ptr, Ryan* a2);
bool sub_444020(int64_t* obj_ptr, Ryan* a2);
void sub_4440E0(int64_t obj, FollowerInfo* a2);
bool sub_444110(FollowerInfo* a1);
bool sub_444130(FollowerInfo* a1);
int64_t get_fire_at_location(int64_t loc);
void sub_4445A0(int64_t a1, int64_t a2);

#endif /* ARCANUM_GAME_OBJECT_H_ */
