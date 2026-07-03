#ifndef ARCANUM_GAME_CE_OVERLAY_H_
#define ARCANUM_GAME_CE_OVERLAY_H_

#include <stdbool.h>

bool ce_overlay_mount(void);
bool ce_overlay_is_mounted(void);
const char* ce_overlay_root_path(void);

#endif /* ARCANUM_GAME_CE_OVERLAY_H_ */
