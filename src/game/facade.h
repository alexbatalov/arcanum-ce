#ifndef ARCANUM_GAME_FACADE_H_
#define ARCANUM_GAME_FACADE_H_

#include "game/context.h"
#include "game/location.h"

bool facade_init(GameInitInfo* init_info);
void facade_exit(void);
void facade_resize(GameResizeInfo* resize_info);
void facade_update_view(ViewOptions* view_options);
void facade_draw(GameDrawInfo* draw_info);
bool sub_4CA6B0(LocRect* loc_rect, int* a2, int* a3);

#endif /* ARCANUM_GAME_FACADE_H_ */
