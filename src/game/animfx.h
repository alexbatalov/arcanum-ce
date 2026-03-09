#ifndef ARCANUM_GAME_ANIMFX_H_
#define ARCANUM_GAME_ANIMFX_H_

#include "game/context.h"

typedef enum AnimFxListEntryFlags {
    ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY = 0x01,
    ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY = 0x02,
    ANIMFX_LIST_ENTRY_UNDERLAY = 0x04,
    ANIMFX_LIST_ENTRY_TRANSLUCENCY = 0x08,
    ANIMFX_LIST_ENTRY_TINTING = 0x10,
    ANIMFX_LIST_ENTRY_ANIMATES = 0x20,
    ANIMFX_LIST_ENTRY_LOOPS = 0x40,
    ANIMFX_LIST_ENTRY_CAN_AUTOSCALE = 0x80,
    ANIMFX_LIST_ENTRY_ANY_OVERLAY = ANIMFX_LIST_ENTRY_FOREGROUND_OVERLAY | ANIMFX_LIST_ENTRY_BACKGROUND_OVERLAY,
} AnimFxListEntryFlags;

typedef struct AnimFxListEntry {
    /* 0000 */ tig_art_id_t eye_candy_art_id;
    /* 0004 */ unsigned int flags;
    /* 0008 */ int sound;
    /* 000C */ tig_art_id_t light_art_id;
    /* 0010 */ unsigned int light_color;
    /* 0014 */ int projectile_speed;
} AnimFxListEntry;

typedef enum AnimFxListFlags {
    ANIMFX_LIST_LOADED = 0x1,
} AnimFxListFlags;

typedef struct AnimFxList {
    /* 0000 */ const char* path;
    /* 0004 */ unsigned int flags;
    /* 0008 */ int initial;
    /* 000C */ int num_fields;
    /* 0010 */ int step;
    /* 0014 */ int num_effects;
    /* 0018 */ int capacity;
    /* 001C */ int size;
    /* 0020 */ int num_sound_effects;
    /* 0024 */ int* sound_effects;
    /* 0028 */ AnimFxListEntry* entries;
} AnimFxList;

typedef uint32_t AnimFxNodeFlags;

#define ANIMFX_PLAY_COUNT 10

#define ANIMFX_PLAY_REVERSE 0x0001u
#define ANIMFX_PLAY_STACK 0x0002u
#define ANIMFX_PLAY_DESTROY 0x0004u
#define ANIMFX_PLAY_CALLBACK 0x0008u
#define ANIMFX_PLAY_END_CALLBACK 0x0010u
#define ANIMFX_PLAY_RANDOM_START 0x0020u
#define ANIMFX_PLAY_FIRE_DMG 0x0040u
#define ANIMFX_PLAY_CHECK_ALREADY 0x0080u
#define ANIMFX_PLAY_NO_ID 0x0100u
#define ANIMFX_PLAY_ICE_DMG 0x0240u

typedef struct AnimFxNode {
    /* 0000 */ AnimFxList* list;
    /* 0008 */ int64_t obj;
    /* 0010 */ int64_t parent_obj;
    /* 0018 */ int fx_id;
    /* 001C */ bool animate;
    /* 0020 */ int max_simultaneous_effects;
    /* 0024 */ AnimFxNodeFlags flags;
    /* 0028 */ int mt_id;
    /* 002C */ tig_art_id_t* art_id_ptr;
    /* 0030 */ tig_art_id_t* light_art_id_ptr;
    /* 0034 */ tig_color_t* light_color_ptr;
    /* 0038 */ int overlay_fore_index;
    /* 003C */ int overlay_back_index;
    /* 0040 */ int overlay_light_index;
    /* 0044 */ int sound_id;
    /* 0048 */ int rotation;
    /* 004C */ int scale;
} AnimFxNode;

extern const char* animfx_play_flags_lookup_tbl_keys[ANIMFX_PLAY_COUNT];
extern unsigned int animfx_play_flags_lookup_tbl_values[ANIMFX_PLAY_COUNT];

bool animfx_init(GameInitInfo* init_info);
void animfx_exit(void);
void sub_4CCD20(AnimFxList* list, AnimFxNode* node, int64_t obj, int mt_id, int fx_id);
void animfx_preload_art(AnimFxNode* node);
bool animfx_has(AnimFxNode* node);
bool animfx_add(AnimFxNode* node);
bool sub_4CD7A0(AnimFxNode* node);
void animfx_remove(AnimFxList* list, int64_t obj, int fx_id, int mt_id);
bool animfx_list_init(AnimFxList* list);
bool animfx_list_load(AnimFxList* list);
int animfx_list_find(AnimFxList* list);
AnimFxList* animfx_list_get(int index);
void animfx_list_exit(AnimFxList* list);
bool animfx_id_get(AnimFxList* list, int index, AnimFxListEntry** entry);

#endif /* ARCANUM_GAME_ANIMFX_H_ */
