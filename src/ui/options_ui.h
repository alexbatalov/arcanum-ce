#ifndef ARCANUM_UI_OPTIONS_UI_H_
#define ARCANUM_UI_OPTIONS_UI_H_

#include "game/context.h"

typedef enum OptionsUiTab {
    OPTIONS_UI_TAB_GAME,
    OPTIONS_UI_TAB_VIDEO,
    OPTIONS_UI_TAB_AUDIO,
    OPTIONS_UI_TAB_COUNT,
} OptionsUiTab;

void options_ui_start(OptionsUiTab tab, tig_window_handle_t window_handle, bool in_play);
bool options_ui_load_module(void);
void options_ui_close(void);
bool options_ui_handle_button_pressed(tig_button_handle_t button_handle);

#endif /* ARCANUM_UI_OPTIONS_UI_H_ */
