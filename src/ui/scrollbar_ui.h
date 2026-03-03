#ifndef ARCANUM_UI_SCROLLBAR_UI_H_
#define ARCANUM_UI_SCROLLBAR_UI_H_

#include "game/context.h"

/**
 * Two-part handle used to identify a scrollbar control.
 */
typedef struct ScrollbarId {
    /* 0000 */ int index;
    /* 0004 */ int global_index;
} ScrollbarId;

/**
 * Callback invoked whenever the scrollbar value changes.
 *
 * Receives the new integer value.
 */
typedef void(ScrollbarUiControlValueChanged)(int value);

/**
 * Callback invoked to repaint the region previously occupied by the scrollbar.
 *
 * Used to restore the background when the scrollbar is hidden or destroyed.
 * Receives the scrollbar rect that needs to be refreshed.
 */
typedef void(ScrollbarUiControlRefresh)(TigRect* rect);

typedef uint32_t ScrollbarUiControlInfoFlags;

#define SB_INFO_VALID 0x0001 /* Marks the info struct as initialised. */
#define SB_INFO_CONTENT_RECT 0x0002 /* `content_rect` is provided. */
#define SB_INFO_MAX_VALUE 0x0004 /* `max_value` is provided. */
#define SB_INFO_MIN_VALUE 0x0008 /* `min_value` is provided. */
#define SB_INFO_LINE_STEP 0x0010 /* `line_step` is provided. */
#define SB_INFO_PAGE_STEP 0x0020 /* `page_step` is provided. */
#define SB_INFO_WHEEL_STEP 0x0040 /* `wheel_step` is provided. */
#define SB_INFO_VALUE 0x0080 /* initial `value` is provided. */
#define SB_INFO_ON_VALUE_CHANGED 0x0100 /* `on_value_changed` callback is provided. */
#define SB_INFO_ON_REFRESH 0x0200 /* `on_refresh` callback is provided. */

/**
 * Scrollbar configuration data.
 *
 * Each meaningful field is paired with a flag bit in `flags`. If the
 * corresponding flag is not set the module applies a built-in default.
 */
typedef struct ScrollbarUiControlInfo {
    /* 0000 */ ScrollbarUiControlInfoFlags flags;
    /* 0004 */ TigRect scrollbar_rect;
    /* 0014 */ TigRect content_rect;
    /* 0024 */ int max_value;
    /* 0028 */ int min_value;
    /* 002C */ int line_step;
    /* 0030 */ int page_step;
    /* 0034 */ int wheel_step;
    /* 0038 */ int value;
    /* 003C */ ScrollbarUiControlValueChanged* on_value_changed;
    /* 0040 */ ScrollbarUiControlRefresh* on_refresh;
} ScrollbarUiControlInfo;

#define SCROLLBAR_MIN_VALUE 0
#define SCROLLBAR_MAX_VALUE 1
#define SCROLLBAR_CURRENT_VALUE 2

bool scrollbar_ui_init(GameInitInfo* init_info);
void scrollbar_ui_exit();
void scrollbar_ui_reset();
bool scrollbar_ui_control_create(ScrollbarId* id, ScrollbarUiControlInfo* info, tig_window_handle_t window_handle);
void scrollbar_ui_control_destroy(ScrollbarId id);
void scrollbar_ui_control_redraw(ScrollbarId id);
bool scrollbar_ui_control_show(ScrollbarId id);
bool scrollbar_ui_control_hide(ScrollbarId id);
bool scrollbar_ui_process_event(TigMessage* msg);
void scrollbar_ui_control_set(ScrollbarId id, int type, int value);
void scrollbar_ui_begin_ignore_events();
void scrollbar_ui_end_ignore_events();

#endif /* ARCANUM_UI_SCROLLBAR_UI_H_ */
