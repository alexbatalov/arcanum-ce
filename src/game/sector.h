#ifndef ARCANUM_GAME_SECTOR_H_
#define ARCANUM_GAME_SECTOR_H_

#include "game/context.h"
#include "game/sector_block_list.h"
#include "game/sector_light_list.h"
#include "game/sector_object_list.h"
#include "game/sector_roof_list.h"
#include "game/sector_script_list.h"
#include "game/sector_sound_list.h"
#include "game/sector_tile_list.h"
#include "game/tile_script_list.h"
#include "game/timeevent.h"

typedef uint32_t SectorFlags;

#define SECTOR_TOWNMAP_CHANGED 0x00000001u
#define SECTOR_APTITUDE_ADJ_CHANGED 0x00000002u
#define SECTOR_LIGHT_SCHEME_CHANGED 0x00000004u
#define SECTOR_IS_NEW 0x80000000u

#define SECTOR_CHANGED_MASK (SECTOR_TOWNMAP_CHANGED | SECTOR_APTITUDE_ADJ_CHANGED | SECTOR_LIGHT_SCHEME_CHANGED)

typedef struct Sector {
    /* 0000 */ SectorFlags flags;
    /* 0008 */ int64_t id;
    /* 0010 */ DateTime datetime;
    /* 0018 */ SectorLightList lights;
    /* 0020 */ SectorTileList tiles;
    /* 4224 */ SectorRoofList roofs;
    /* 4628 */ TileScriptList tile_scripts;
    /* 4630 */ SectorScriptList sector_scripts;
    /* 4640 */ int townmap_info;
    /* 4644 */ int aptitude_adj;
    /* 4648 */ int light_scheme;
    /* 464C */ SectorSoundList sounds;
    /* 4658 */ SectorBlockList blocks;
    /* 485C */ SectorObjectList objects;
} Sector;

typedef bool(SectorEnumerateFunc)(Sector* sector);
typedef bool(SectorLockFunc)(const char* path);

bool sector_init(GameInitInfo* init_info);
void sector_reset(void);
void sector_exit(void);
void sector_resize(GameResizeInfo* resize_info);
void sector_map_close(void);
void sector_update_view(ViewOptions* view_options);
void sub_4CF360(SectorLockFunc* func);
void sector_grid_toggle(void);
void sector_draw(GameDrawInfo* draw_info);
bool sector_limits_set(int64_t x, int64_t y);
void sector_limits_get(int64_t* x, int64_t* y);
int64_t sector_id_from_loc(int64_t loc);
int64_t sector_loc_from_id(int64_t sector_id);
int sector_rot(int64_t a, int64_t b);
bool sector_in_dir(int64_t sec, int dir, int64_t* new_sec_ptr);
bool sub_4D0090(LocRect* rect, SomeSectorStuff* a2);
SectorListNode* sector_list_create(LocRect* loc_rect);
void sector_list_destroy(SectorListNode* node);
bool sector_map_name_set(const char* base_path, const char* save_path);
bool sector_exists(uint64_t id);
bool sector_loaded(int64_t id);
bool sector_lock(int64_t id, Sector** sector_ptr);
bool sector_unlock(int64_t id);
void sector_cache_reset(void);
void sector_flush(unsigned int flags);
bool sector_check_demo_limits(int64_t id);
void sector_art_cache_register(tig_art_id_t art_id);
bool sector_is_blocked(int64_t sec);
void sector_block_set(int64_t sec, bool blocked);
void sector_block_init(void);
bool sector_block_load(const char* base_map_name, const char* current_map_name);
void sector_art_cache_enable(void);
void sector_art_cache_disable(void);
void sector_enumerate(SectorEnumerateFunc* func);
bool sector_history_init(GameInitInfo* init_info);
void sector_history_reset(void);
void sector_history_exit(void);
bool sector_history_save(TigFile* stream);
bool sector_history_load(GameLoadInfo* load_info);

#define SECTOR_X(a) ((a) & 0x3FFFFFF)
#define SECTOR_Y(a) (((a) >> 26) & 0x3FFFFFF)
#define SECTOR_MAKE(a, b) ((a) | ((b) << 26))

#endif /* ARCANUM_GAME_SECTOR_H_ */
