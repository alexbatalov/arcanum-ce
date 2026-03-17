#ifndef ARCANUM_UI_FOLLOWER_UI_H_
#define ARCANUM_UI_FOLLOWER_UI_H_

#include "game/context.h"
#include "game/target.h"

bool follower_ui_init(GameInitInfo* init_info);
void follower_ui_exit(void);
void follower_ui_reset(void);
void follower_ui_resize(GameResizeInfo* resize_info);
bool follower_ui_load(GameLoadInfo* load_info);
bool follower_ui_save(TigFile* stream);
void follower_ui_execute_order(TargetDescriptor* td);
void follower_ui_end_order_mode(void);
void follower_ui_update(void);
void follower_ui_update_obj(int64_t obj);
void follower_ui_refresh(void);

#endif /* ARCANUM_UI_FOLLOWER_UI_H_ */
