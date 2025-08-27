#ifndef TIG_ART_H_
#define TIG_ART_H_

#include "tig/color.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIG_ART_ID_INVALID ((tig_art_id_t)(-1))

#define TIG_ART_SYSTEM_MOUSE 0
#define TIG_ART_SYSTEM_BUTTON 1
#define TIG_ART_SYSTEM_UP 2
#define TIG_ART_SYSTEM_DOWN 3
#define TIG_ART_SYSTEM_CANCEL 4
#define TIG_ART_SYSTEM_LENS 5
#define TIG_ART_SYSTEM_X 6
#define TIG_ART_SYSTEM_PLUS 7
#define TIG_ART_SYSTEM_MINUS 8
#define TIG_ART_SYSTEM_BLANK 9
#define TIG_ART_SYSTEM_FONT 10

typedef enum TigArtType {
    TIG_ART_TYPE_TILE,
    TIG_ART_TYPE_WALL,
    TIG_ART_TYPE_CRITTER,
    TIG_ART_TYPE_PORTAL,
    TIG_ART_TYPE_SCENERY,
    TIG_ART_TYPE_INTERFACE,
    TIG_ART_TYPE_ITEM,
    TIG_ART_TYPE_CONTAINER,
    TIG_ART_TYPE_MISC,
    TIG_ART_TYPE_LIGHT,
    TIG_ART_TYPE_ROOF,
    TIG_ART_TYPE_FACADE,
    TIG_ART_TYPE_MONSTER,
    TIG_ART_TYPE_UNIQUE_NPC,
    TIG_ART_TYPE_EYE_CANDY,
} TigArtType;

typedef enum TigArtTileType {
    TIG_ART_TILE_TYPE_INDOOR,
    TIG_ART_TILE_TYPE_OUTDOOR,
} TigArtTileType;

typedef enum TigArtCritterBodyType {
    TIG_ART_CRITTER_BODY_TYPE_HUMAN,
    TIG_ART_CRITTER_BODY_TYPE_DWARF,
    TIG_ART_CRITTER_BODY_TYPE_HALFLING,
    TIG_ART_CRITTER_BODY_TYPE_HALF_OGRE,
    TIG_ART_CRITTER_BODY_TYPE_ELF,
    TIG_ART_CRITTER_BODY_TYPE_COUNT,
} TigArtCritterBodyType;

typedef enum TigArtCritterGender {
    TIG_ART_CRITTER_GENDER_FEMALE,
    TIG_ART_CRITTER_GENDER_MALE,
    TIG_ART_CRITTER_GENDER_COUNT,
} TigArtCritterGender;

typedef enum TigArtPortalType {
    TIG_ART_PORTAL_TYPE_WINDOW,
    TIG_ART_PORTAL_TYPE_DOOR,
} TigArtPortalType;

// NOTE: Originally named `TIG_Art_Scenery_Category` (as mentioned in
// `arcanum3.dat/oemes/scenery.mes`).
typedef enum TigArtSceneryType {
    TIG_ART_SCENERY_TYPE_PROJECTILE,
    TIG_ART_SCENERY_TYPE_TREES_LARGE,
    TIG_ART_SCENERY_TYPE_TREES_MEDIUM,
    TIG_ART_SCENERY_TYPE_TREES_SMALL,
    TIG_ART_SCENERY_TYPE_TREES_DEAD,
    TIG_ART_SCENERY_TYPE_PLANTS,
    TIG_ART_SCENERY_TYPE_METAL_SMALL,
    TIG_ART_SCENERY_TYPE_METAL_MEDIUM,
    TIG_ART_SCENERY_TYPE_METAL_LARGE,
    TIG_ART_SCENERY_TYPE_STONE_SMALL,
    TIG_ART_SCENERY_TYPE_STONE_MEDIUM,
    TIG_ART_SCENERY_TYPE_STONE_LARGE,
    TIG_ART_SCENERY_TYPE_WOODEN_SMALL,
    TIG_ART_SCENERY_TYPE_WOODEN_MEDIUM,
    TIG_ART_SCENERY_TYPE_WOODEN_LARGE,
    TIG_ART_SCENERY_TYPE_MISC_SMALL,
    TIG_ART_SCENERY_TYPE_MISC_MEDIUM,
    TIG_ART_SCENERY_TYPE_MISC_LARGE,
    TIG_ART_SCENERY_TYPE_MACHINERY_LARGE,
    TIG_ART_SCENERY_TYPE_MACHINERY_MEDIUM,
    TIG_ART_SCENERY_TYPE_TELEPORT_FACADES,
    TIG_ART_SCENERY_TYPE_LIGHT_SOURCES,
    TIG_ART_SCENERY_TYPE_BEDS,
} TigArtSceneryType;

typedef enum TigArtItemDisposition {
    TIG_ART_ITEM_DISPOSITION_GROUND,
    TIG_ART_ITEM_DISPOSITION_INVENTORY,
    TIG_ART_ITEM_DISPOSITION_PAPERDOLL,
    TIG_ART_ITEM_DISPOSITION_SCHEMATIC,
} TigArtItemDisposition;

typedef enum TigArtItemType {
    TIG_ART_ITEM_TYPE_WEAPON,
    TIG_ART_ITEM_TYPE_AMMO,
    TIG_ART_ITEM_TYPE_ARMOR,
    TIG_ART_ITEM_TYPE_GOLD,
    TIG_ART_ITEM_TYPE_FOOD,
    TIG_ART_ITEM_TYPE_SCROLL,
    TIG_ART_ITEM_TYPE_KEY,
    TIG_ART_ITEM_TYPE_KEY_RING,
    TIG_ART_ITEM_TYPE_WRITTEN,
    TIG_ART_ITEM_TYPE_GENERIC,
} TigArtItemType;

// NOTE: Originally named `TIG_Art_Weapon` (as mentioned in
// `arcanum3.dat/art/item_item_ground.mes`).
typedef enum TigArtWeaponType {
    TIG_ART_WEAPON_TYPE_NO_WEAPON,
    TIG_ART_WEAPON_TYPE_UNARMED,
    TIG_ART_WEAPON_TYPE_DAGGER,
    TIG_ART_WEAPON_TYPE_SWORD,
    TIG_ART_WEAPON_TYPE_AXE,
    TIG_ART_WEAPON_TYPE_MACE,
    TIG_ART_WEAPON_TYPE_PISTOL,
    TIG_ART_WEAPON_TYPE_TWO_HANDED_SWORD,
    TIG_ART_WEAPON_TYPE_BOW,
    TIG_ART_WEAPON_TYPE_9,
    TIG_ART_WEAPON_TYPE_RIFLE,
    TIG_ART_WEAPON_TYPE_11,
    TIG_ART_WEAPON_TYPE_12,
    TIG_ART_WEAPON_TYPE_STAFF,
    TIG_ART_WEAPON_TYPE_14,
    TIG_ART_WEAPON_TYPE_COUNT,
} TigArtWeaponType;

typedef enum TigArtAmmoType {
    TIG_ART_AMMO_TYPE_ARROW,
    TIG_ART_AMMO_TYPE_BULLET,
    TIG_ART_AMMO_TYPE_CHARGE,
    TIG_ART_AMMO_TYPE_FUEL,
    TIG_ART_AMMO_TYPE_COUNT,
} TigArtAmmoType;

// NOTE: Originally named `TIG_Art_Armor` (as mentioned in
// `arcanum3.dat/art/item_item_ground.mes`).
typedef enum TigArtArmorType {
    TIG_ART_ARMOR_TYPE_UNDERWEAR,
    TIG_ART_ARMOR_TYPE_VILLAGER,
    TIG_ART_ARMOR_TYPE_LEATHER,
    TIG_ART_ARMOR_TYPE_CHAIN,
    TIG_ART_ARMOR_TYPE_PLATE,
    TIG_ART_ARMOR_TYPE_ROBE,
    TIG_ART_ARMOR_TYPE_PLATE_CLASSIC,
    TIG_ART_ARMOR_TYPE_BARBARIAN,
    TIG_ART_ARMOR_TYPE_CITY_DWELLER,
    TIG_ART_ARMOR_TYPE_COUNT,
} TigArtArmorType;

typedef enum TigArtArmorCoverage {
    TIG_ART_ARMOR_COVERAGE_TORSO,
    TIG_ART_ARMOR_COVERAGE_SHIELD,
    TIG_ART_ARMOR_COVERAGE_HELMET,
    TIG_ART_ARMOR_COVERAGE_GAUNTLETS,
    TIG_ART_ARMOR_COVERAGE_BOOTS,
    TIG_ART_ARMOR_COVERAGE_RING,
    TIG_ART_ARMOR_COVERAGE_MEDALLION,
    TIG_ART_ARMOR_COVERAGE_COUNT,
} TigArtArmorCoverage;

// NOTE: Originally named `TIG_Art_Container_Type` (as mentioned in
// `arcanum3.dat/oemes/containertype.mes`).
typedef enum TigArtContainerType {
    TIG_ART_CONTAINER_TYPE_JUNK,
    TIG_ART_CONTAINER_TYPE_CHEST,
    TIG_ART_CONTAINER_TYPE_BARREL,
    TIG_ART_CONTAINER_TYPE_BAG,
    TIG_ART_CONTAINER_TYPE_BOX,
    TIG_ART_CONTAINER_TYPE_FURNITURE,
    TIG_ART_CONTAINER_TYPE_BOOKSTAND,
    TIG_ART_CONTAINER_TYPE_MISC,
    TIG_ART_CONTAINER_TYPE_COUNT,
} TigArtContainerType;

typedef enum TigArtEyeCandyType {
    TIG_ART_EYE_CANDY_TYPE_FOREGROUND_OVERLAY,
    TIG_ART_EYE_CANDY_TYPE_BACKGROUND_OVERLAY,
    TIG_ART_EYE_CANDY_TYPE_UNDERLAY,
    TIG_ART_EYE_CANDY_TYPE_COUNT,
} TigArtEyeCandyType;

typedef enum TigArtMonsterSpecie {
    TIG_ART_MONSTER_SPECIE_WOLF,
    TIG_ART_MONSTER_SPECIE_SPIDER,
    TIG_ART_MONSTER_SPECIE_ORC,
    TIG_ART_MONSTER_SPECIE_EARTH_ELEMENTAL,
    TIG_ART_MONSTER_SPECIE_LESSER_DEMON,
    TIG_ART_MONSTER_SPECIE_LESSER_SKELETON,
    TIG_ART_MONSTER_SPECIE_LESSER_ZOMBIE,
    TIG_ART_MONSTER_SPECIE_LESSER_MUMMY,
    TIG_ART_MONSTER_SPECIE_GREATER_DEMON,
    TIG_ART_MONSTER_SPECIE_BUNNY,
    TIG_ART_MONSTER_SPECIE_MECH_SPIDER,
    TIG_ART_MONSTER_SPECIE_AUTOMATON,
    TIG_ART_MONSTER_SPECIE_LIZARD_MAN,
    TIG_ART_MONSTER_SPECIE_PHANTOM_KNIGHT,
    TIG_ART_MONSTER_SPECIE_WERERAT,
    TIG_ART_MONSTER_SPECIE_SNAKE_MAN,
    TIG_ART_MONSTER_SPECIE_APE,
    TIG_ART_MONSTER_SPECIE_BEAR,
    TIG_ART_MONSTER_SPECIE_CHICKEN,
    TIG_ART_MONSTER_SPECIE_COUGAR,
    TIG_ART_MONSTER_SPECIE_SHEEP,
    TIG_ART_MONSTER_SPECIE_TIGER,
    TIG_ART_MONSTER_SPECIE_PIG,
    TIG_ART_MONSTER_SPECIE_COW,
    TIG_ART_MONSTER_SPECIE_WISP,
    TIG_ART_MONSTER_SPECIE_MUTANT_PIG,
    TIG_ART_MONSTER_SPECIE_FIRE_ELEMENTAL,
    TIG_ART_MONSTER_SPECIE_WATER_ELEMENTAL,
    TIG_ART_MONSTER_SPECIE_AIR_ELEMENTAL,
    TIG_ART_MONSTER_SPECIE_DEATH_KNIGHT,
    TIG_ART_MONSTER_SPECIE_SPIDER_QUEEN,
    TIG_ART_MONSTER_SPECIE_WEREWOLF,
    TIG_ART_MONSTER_SPECIE_COUNT,
} TigArtMonsterSpecie;

typedef enum TigArtAnim {
    TIG_ART_ANIM_STAND,
    TIG_ART_ANIM_WALK,
    TIG_ART_ANIM_2,
    TIG_ART_ANIM_STEALTH_WALK,
    TIG_ART_ANIM_4,
    TIG_ART_ANIM_CONCEAL_FIDGET,
    TIG_ART_ANIM_RUN,
    TIG_ART_ANIM_FALL_DOWN,
    TIG_ART_ANIM_GET_UP,
    TIG_ART_ANIM_9,
    TIG_ART_ANIM_10,
    TIG_ART_ANIM_11,
    TIG_ART_ANIM_PICK_POCKET,
    TIG_ART_ANIM_VAULT,
    TIG_ART_ANIM_THROW,
    TIG_ART_ANIM_15,
    TIG_ART_ANIM_16,
    TIG_ART_ANIM_DECAPITATION,
    TIG_ART_ANIM_BLOWN_OUT_CHUNK,
    TIG_ART_ANIM_SEVERED_LEG,
    TIG_ART_ANIM_ATTACK,
    TIG_ART_ANIM_ATTACK_LOW,
    TIG_ART_ANIM_22,
    TIG_ART_ANIM_STUNNED,
    TIG_ART_ANIM_EXPLODE,
} TigArtAnim;

typedef enum TigArtRoofPiece {
    TIG_ART_ROOF_PIECE_NORTH_WEST_OUTSIDE,
    TIG_ART_ROOF_PIECE_WEST,
    TIG_ART_ROOF_PIECE_NORTH,
    TIG_ART_ROOF_PIECE_NORTH_WEST_INSIDE,
    TIG_ART_ROOF_PIECE_SOUTH_WEST_OUTSIDE,
    TIG_ART_ROOF_PIECE_SOUTH_WEST_INSIDE,
    TIG_ART_ROOF_PIECE_NORTH_EAST_INSIDE,
    TIG_ART_ROOF_PIECE_NORTH_EAST_OUTSIDE,
    TIG_ART_ROOF_PIECE_CENTER,
    TIG_ART_ROOF_PIECE_SOUTH_EAST_OUTSIDE,
    TIG_ART_ROOF_PIECE_SOUTH,
    TIG_ART_ROOF_PIECE_EAST,
    TIG_ART_ROOF_PIECE_SOUTH_EAST_INSIDE,
    TIG_ART_ROOF_PIECE_COUNT,
} TigArtRoofPiece;

typedef enum TigArtAnimFlags {
    TIG_ART_0x01 = 0x01,
    TIG_ART_0x02 = 0x02,
    TIG_ART_0x04 = 0x04,
    TIG_ART_0x08 = 0x08,
    TIG_ART_0x10 = 0x10,
    TIG_ART_0x20 = 0x20,
} TigArtAnimFlags;

typedef struct TigArtAnimData {
    TigArtAnimFlags flags;
    int fps;
    int bpp;
    int action_frame;
    int num_frames;
    unsigned int color_key;
    void* palette1;
    void* palette2;
} TigArtAnimData;

typedef struct TigArtFrameData {
    /* 0000 */ int width;
    /* 0004 */ int height;
    /* 0008 */ int hot_x;
    /* 000C */ int hot_y;
    /* 0010 */ int offset_x;
    /* 0014 */ int offset_y;
} TigArtFrameData;

typedef unsigned int TigArtBlitFlags;

#define TIG_ART_BLT_FLIP_X 0x00000001u
#define TIG_ART_BLT_FLIP_Y 0x00000002u

// Forces to use base art palette.
//
// Mutually exclusive with `TIG_ART_BLT_PALETTE_OVERRIDE`.
#define TIG_ART_BLT_PALETTE_ORIGINAL 0x00000004u

// Forces to use custom palette specified in `TigArtBlitInfo::palette`.
//
// Mutually exclusive with `TIG_ART_BLT_PALETTE_ORIGINAL`.
#define TIG_ART_BLT_PALETTE_OVERRIDE 0x00000008u

// Sum the components of source (art) and destination (video buffer).
#define TIG_ART_BLT_BLEND_ADD 0x00000010u

// Subtract the components of source (art) from destination (video buffer).
#define TIG_ART_BLT_BLEND_SUB 0x00000020u

// Multiply the components of source (art) and destination (video buffer).
#define TIG_ART_BLT_BLEND_MUL 0x00000040u

#define TIG_ART_BLT_BLEND_ALPHA_AVG 0x00000080u

#define TIG_ART_BLT_BLEND_ALPHA_CONST 0x00000100u

#define TIG_ART_BLT_BLEND_ALPHA_SRC 0x00000200u

// Blends source (art) with destination (video buffer) by applying linearly
// interpolated alpha mask along horizontal axis.
//
// Use `TigArtBlitInfo::alpha[0]` to specify start alpha value (at left edge)
// and `TigArtBlitInfo::alpha[1]` to specify end alpha value (right edge) where
// `0` is fully transparent (results in src color), and `255` is fully opaque
// (results in dst color).
#define TIG_ART_BLT_BLEND_ALPHA_LERP_X 0x00000400u

// Blends source (art) with destination (video buffer) by applying linearly
// interpolated alpha mask along vertical axis.
//
// Use `TigArtBlitInfo::alpha[0]` to specify start alpha value (at top edge)
// and `TigArtBlitInfo::alpha[3]` to specify end alpha value (at bottom edge)
// where `0` is fully transparent (results in src color), and `255` is fully
// opaque (results in dst color).
#define TIG_ART_BLT_BLEND_ALPHA_LERP_Y 0x00000800u

// Blends source (art) with destination (video buffer) by applying linearly
// interpolated alpha mask in both directions.
//
//  - `TigArtBlitInfo::alpha[0]` - top-left corner alpha,
//  - `TigArtBlitInfo::alpha[1]` - top-right corner alpha,
//  - `TigArtBlitInfo::alpha[2]` - bottom-right corner alpha,
//  - `TigArtBlitInfo::alpha[3]` - bottom-left corner alpha.
#define TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH 0x00001000u

#define TIG_ART_BLT_BLEND_COLOR_CONST 0x00002000u

#define TIG_ART_BLT_BLEND_COLOR_ARRAY 0x00004000u

#define TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S 0x00008000u

#define TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D 0x00010000u

#define TIG_ART_BLT_BLEND_COLOR_LERP 0x00020000u

#define TIG_ART_BLT_SCRATCH_VALID 0x01000000u

#define TIG_ART_BLT_BLEND_COLOR_ANY (TIG_ART_BLT_BLEND_COLOR_LERP \
    | TIG_ART_BLT_BLEND_COLOR_ARRAY                               \
    | TIG_ART_BLT_BLEND_COLOR_CONST)

#define TIG_ART_BLT_BLEND_ALPHA_ANY (TIG_ART_BLT_BLEND_ALPHA_STIPPLE_D \
    | TIG_ART_BLT_BLEND_ALPHA_STIPPLE_S                                \
    | TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH                                \
    | TIG_ART_BLT_BLEND_ALPHA_LERP_Y                                   \
    | TIG_ART_BLT_BLEND_ALPHA_LERP_X                                   \
    | TIG_ART_BLT_BLEND_ALPHA_SRC                                      \
    | TIG_ART_BLT_BLEND_ALPHA_CONST                                    \
    | TIG_ART_BLT_BLEND_ALPHA_AVG                                      \
    | TIG_ART_BLT_BLEND_MUL                                            \
    | TIG_ART_BLT_BLEND_SUB                                            \
    | TIG_ART_BLT_BLEND_ADD)

#define TIG_ART_BLT_BLEND_ANY (TIG_ART_BLT_BLEND_COLOR_ANY \
    | TIG_ART_BLT_BLEND_ALPHA_ANY)

#define TIG_ART_BLT_BLEND_ALPHA_LERP_ANY (TIG_ART_BLT_BLEND_ALPHA_LERP_X \
    | TIG_ART_BLT_BLEND_ALPHA_LERP_Y                                     \
    | TIG_ART_BLT_BLEND_ALPHA_LERP_BOTH)

typedef struct TigArtBlitInfo {
    /* 0000 */ TigArtBlitFlags flags;
    /* 0004 */ tig_art_id_t art_id;
    /* 0008 */ TigRect* src_rect;
    /* 000C */ TigPalette palette;
    /* 0010 */ tig_color_t color;
    /* 0014 */ uint32_t* field_14;
    /* 0018 */ TigRect* field_18;
    /* 001C */ uint8_t alpha[4];
    /* 0020 */ TigVideoBuffer* dst_video_buffer;
    /* 0024 */ TigRect* dst_rect;
    /* 0028 */ TigVideoBuffer* scratch_video_buffer;
} TigArtBlitInfo;

typedef bool(TigArtBlitPaletteAdjustCallback)(tig_art_id_t art_id, TigPaletteModifyInfo* modify_info);

int tig_art_init(TigInitInfo* init_info);
void tig_art_exit();
void tig_art_ping();
int tig_art_misc_id_create(unsigned int num, unsigned int palette, tig_art_id_t* art_id);
void tig_art_flush();
int tig_art_exists(tig_art_id_t art_id);
int tig_art_touch(tig_art_id_t art_id);
void sub_5022B0(TigArtBlitPaletteAdjustCallback* callback);
TigArtBlitPaletteAdjustCallback* sub_5022C0();
void sub_5022D0();
int tig_art_blit(TigArtBlitInfo* blit_info);
int tig_art_type(tig_art_id_t art_id);
unsigned int tig_art_num_get(tig_art_id_t art_id);
tig_art_id_t tig_art_num_set(tig_art_id_t art_id, unsigned int value);
unsigned int sub_502830(tig_art_id_t art_id);
int tig_art_id_rotation_get(tig_art_id_t art_id);
tig_art_id_t tig_art_id_rotation_inc(tig_art_id_t art_id);
tig_art_id_t tig_art_id_rotation_set(tig_art_id_t, int rotation);
int tig_art_id_frame_get(tig_art_id_t art_id);
tig_art_id_t tig_art_id_frame_inc(tig_art_id_t art_id);
tig_art_id_t tig_art_id_frame_dec(tig_art_id_t art_id);
tig_art_id_t tig_art_id_frame_set(tig_art_id_t art_id, int value);
tig_art_id_t tig_art_id_flags_set(tig_art_id_t art_id, int value);
int tig_art_id_palette_get(tig_art_id_t art_id);
tig_art_id_t tig_art_id_palette_set(tig_art_id_t art_id, int value);
int sub_502E00(tig_art_id_t art_id);
int sub_502E50(tig_art_id_t art_id, int x, int y, unsigned int* color_ptr);
int sub_502FD0(tig_art_id_t art_id, int x, int y);
int tig_art_anim_data(tig_art_id_t art_id, TigArtAnimData* data);
int tig_art_frame_data(tig_art_id_t art_id, TigArtFrameData* data);
int sub_503340(tig_art_id_t art_id, uint8_t* dst, int pitch);
int sub_5033E0(tig_art_id_t art_id, int a2, int a3);
int tig_art_size(tig_art_id_t art_id, int* width_ptr, int* height_ptr);
int tig_art_tile_id_create(unsigned int a1, unsigned int a2, unsigned int a3, unsigned int a4, unsigned int a5, unsigned int a6, unsigned int a7, unsigned int a8, tig_art_id_t* art_id_ptr);
int tig_art_tile_id_num1_get(tig_art_id_t art_id);
int tig_art_tile_id_num2_get(tig_art_id_t art_id);
int sub_503700(tig_art_id_t art_id);
tig_art_id_t sub_503740(tig_art_id_t art_id, int value);
int sub_5037B0(tig_art_id_t art_id);
tig_art_id_t sub_503800(tig_art_id_t art_id, int value);
int tig_art_tile_id_type_get(tig_art_id_t art_id);
int tig_art_tile_id_flippable_get(tig_art_id_t art_id);
int tig_art_tile_id_flippable1_get(tig_art_id_t art_id);
int tig_art_tile_id_flippable2_get(tig_art_id_t art_id);
int tig_art_wall_id_create(unsigned int num, int p_piece, int variation, int rotation, unsigned int palette, int damaged, tig_art_id_t* art_id_ptr);
int tig_art_wall_id_p_piece_get(tig_art_id_t art_id);
tig_art_id_t tig_art_wall_id_p_piece_set(tig_art_id_t art_id, int value);
int tig_art_wall_id_num_get(tig_art_id_t art_id);
int tig_art_wall_id_variation_get(tig_art_id_t art_id);
tig_art_id_t tig_art_wall_id_variation_set(tig_art_id_t art_id, int value);
int tig_art_id_damaged_get(tig_art_id_t art_id);
tig_art_id_t tig_art_id_damaged_set(tig_art_id_t art_id, int value);
int tig_art_critter_id_create(unsigned int gender, int body_type, int armor, unsigned int shield, unsigned int frame, int rotation, int anim, int weapon, unsigned int palette, tig_art_id_t* art_id_ptr);
int tig_art_monster_id_create(int specie, int armor, unsigned int shield, unsigned int frame, int rotation, int anim, int weapon, unsigned int palette, tig_art_id_t* art_id_ptr);
int tig_art_unique_npc_id_create(int num, unsigned int shield, unsigned int frame, int rotation, int anim, int weapon, unsigned int palette, tig_art_id_t* art_id_ptr);
int tig_art_id_anim_get(tig_art_id_t art_id);
tig_art_id_t tig_art_id_anim_set(tig_art_id_t art_id, int value);
int tig_art_critter_id_body_type_get(tig_art_id_t art_id);
tig_art_id_t tig_art_critter_id_body_type_set(tig_art_id_t art_id, int value);
int tig_art_monster_id_specie_get(tig_art_id_t art_id);
int sub_503F50(int a1);
int sub_503F60(tig_art_id_t art_id);
int tig_art_critter_id_gender_get(tig_art_id_t art_id);
tig_art_id_t tig_art_critter_id_gender_set(tig_art_id_t art_id, int value);
int tig_art_critter_id_armor_get(tig_art_id_t art_id);
tig_art_id_t tig_art_critter_id_armor_set(tig_art_id_t art_id, int value);
int tig_art_critter_id_weapon_get(tig_art_id_t art_id);
tig_art_id_t tig_art_critter_id_weapon_set(tig_art_id_t art_id, int value);
int tig_art_critter_id_shield_get(tig_art_id_t art_id);
tig_art_id_t tig_art_critter_id_shield_set(tig_art_id_t art_id, int value);
int tig_art_portal_id_create(unsigned int num, int type, int a3, unsigned int frame, int a5, unsigned int a6, tig_art_id_t* art_id_ptr);
int tig_art_portal_id_type_get(tig_art_id_t art_id);
int tig_art_scenery_id_create(unsigned int num, int type, unsigned int frame, int rotation, unsigned int palette, tig_art_id_t* art_id_ptr);
int tig_art_scenery_id_type_get(tig_art_id_t art_id);
int tig_art_interface_id_create(unsigned int num, unsigned int frame, unsigned char a3, unsigned int a4, tig_art_id_t* art_id_ptr);
int sub_504390(tig_art_id_t art_id);
int tig_art_item_id_create(int num, int disposition, int damaged, int destroyed, int subtype, int type, int armor_coverage, unsigned int palette, tig_art_id_t* art_id_ptr);
int tig_art_item_id_disposition_get(tig_art_id_t art_id);
tig_art_id_t tig_art_item_id_disposition_set(tig_art_id_t art_id, int value);
int tig_art_item_id_subtype_get(tig_art_id_t art_id);
tig_art_id_t tig_art_item_id_subtype_set(tig_art_id_t art_id, int value);
int tig_art_item_id_type_get(tig_art_id_t art_id);
int tig_art_item_id_armor_coverage_get(tig_art_id_t art_id);
int tig_art_item_id_destroyed_get(tig_art_id_t art_id);
tig_art_id_t tig_art_item_id_destroyed_set(tig_art_id_t art_id, int value);
int tig_art_container_id_create(unsigned int num, int type, unsigned int frame, int rotation, unsigned int a5, tig_art_id_t* art_id_ptr);
int tig_art_container_id_type_get(tig_art_id_t art_id);
int tig_art_light_id_create(unsigned int num, unsigned int frame, unsigned int rotation, int a4, tig_art_id_t* art_id_ptr);
int sub_504700(tig_art_id_t art_id);
tig_art_id_t sub_504730(tig_art_id_t art_id, int rotation);
int sub_504790(tig_art_id_t art_id);
int tig_art_roof_id_create(unsigned int num, int piece, unsigned int fill, unsigned int fade, tig_art_id_t* art_id_ptr);
int tig_art_roof_id_piece_get(tig_art_id_t art_id);
tig_art_id_t tig_art_roof_id_piece_set(tig_art_id_t art_id, int piece);
unsigned int tig_art_roof_id_fill_get(tig_art_id_t art_id);
tig_art_id_t tig_art_roof_id_fill_set(tig_art_id_t art_id, unsigned int value);
unsigned int tig_art_roof_id_fade_get(tig_art_id_t art_id);
tig_art_id_t tig_art_roof_id_fade_set(tig_art_id_t art_id, unsigned int value);
int tig_art_facade_id_create(unsigned int num, unsigned int tile_num, unsigned int type, unsigned int flippable, unsigned int frame, unsigned int walkable, tig_art_id_t* art_id_ptr);
int tig_art_facade_id_num_get(tig_art_id_t art_id);
int tig_art_facade_id_frame_get(tig_art_id_t art_id);
int tig_art_facade_id_walkable_get(tig_art_id_t art_id);
int tig_art_eye_candy_id_create(unsigned int num, unsigned int frame, int a3, int translucency, int type, unsigned int palette, int scale, tig_art_id_t* art_id_ptr);
int tig_art_eye_candy_id_type_get(tig_art_id_t art_id);
tig_art_id_t tig_art_eye_candy_id_type_set(tig_art_id_t art_id, int value);
int tig_art_eye_candy_id_translucency_get(tig_art_id_t art_id);
tig_art_id_t tig_art_eye_candy_id_translucency_set(tig_art_id_t art_id, int value);
int tig_art_eye_candy_id_scale_get(tig_art_id_t art_id);
tig_art_id_t tig_art_eye_candy_id_scale_set(tig_art_id_t art_id, int value);
int tig_art_id_flags_get(tig_art_id_t art_id);
void sub_505000(tig_art_id_t art_id, TigPalette src_palette, TigPalette dst_palette);
void tig_art_cache_set_video_memory_fullness(int fullness);
tig_art_id_t tig_art_id_reset(tig_art_id_t art_id);

#ifdef __cplusplus
}
#endif

#endif /* TIG_ART_H_ */
