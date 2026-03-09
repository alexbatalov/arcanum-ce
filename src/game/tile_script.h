#ifndef ARCANUM_GAME_TILE_SCRIPT_H_
#define ARCANUM_GAME_TILE_SCRIPT_H_

#include "game/context.h"

bool tile_script_init(GameInitInfo* init_info);
void tile_script_reset(void);
void tile_script_exit(void);
void tile_script_resize(GameResizeInfo* resize_info);
void tile_script_update_view(ViewOptions* view_options);
bool tile_script_is_enabled(void);
void tile_script_toggle(void);
void tile_script_draw(GameDrawInfo* draw_info);
void tile_script_move(int64_t old_loc, int64_t new_loc);
void tile_script_exec(int64_t loc, int64_t triggerer_obj);

#endif /* ARCANUM_GAME_TILE_SCRIPT_H_ */
