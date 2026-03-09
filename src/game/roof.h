#ifndef ARCANUM_GAME_ROOF_H_
#define ARCANUM_GAME_ROOF_H_

#include "game/context.h"

bool roof_init(GameInitInfo* init_info);
void roof_exit();
void roof_resize(GameResizeInfo* resize_info);
void roof_update_view(ViewOptions* view_options);
void roof_toggle();
void roof_draw(GameDrawInfo* draw_info);
int64_t roof_normalize_loc(int64_t loc);
bool roof_hit_test(int x, int y);
void roof_recalc(int64_t loc);
void roof_fill_off(int64_t loc);
void roof_fill_on(int64_t loc);
void roof_fade_on(int64_t loc);
void roof_fade_off(int64_t loc);
bool roof_is_faded(int64_t loc);
bool roof_is_covered_xy(int64_t x, int64_t y, bool check_faded);
bool roof_is_covered_loc(int64_t loc, bool check_faded);
void roof_blit_flags_set(unsigned int flags);
unsigned int roof_blit_flags_get();

#endif /* ARCANUM_GAME_ROOF_H_ */
