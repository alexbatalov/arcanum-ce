#ifndef GAME_HIGHRES_CONFIG_H_
#define GAME_HIGHRES_CONFIG_H_

#include <stdbool.h>

typedef struct HighResConfig {
    bool loaded;
    int width;
    int height;
    bool windowed;
    bool show_fps;
    int scroll_fps;
    int scroll_dist;
    bool logos;
    bool intro;
    // macOS-only: when true, the game ignores the system safe area on
    // notched MacBooks, drawing under the camera notch / menu bar. When
    // false (the default), the game respects the safe area (rendered area
    // sits below the notch and menu bar). UI elements are calibrated for
    // safe-area-respecting layout, so opting in is the user's call.
    bool ignore_notch;
    // Cross-platform: when true (the default), the game snaps the user's
    // configured (Width, Height) to the screen-aspect-matching pair
    // nearest their values whenever the window will fill the screen. This
    // eliminates letterbox bars without changing the user's intent by
    // more than a few pixels. Set to false to use the configured
    // dimensions exactly (letterbox bars may appear).
    bool aspect_snap;
} HighResConfig;

void highres_config_load(void);
const HighResConfig* highres_config_get(void);

// Rewrites Width/Height in HighRes/config.ini to the given values, leaving
// all other lines (comments, spacing, other keys) intact. Updates the
// in-memory config too. Returns true on success.
bool highres_config_save_resolution(int width, int height);

#endif /* GAME_HIGHRES_CONFIG_H_ */
