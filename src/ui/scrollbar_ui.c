#include "ui/scrollbar_ui.h"

#include "game/random.h"
#include "game/snd.h"

#define SCROLL_UP_BUTTON_ART_NUM 317
#define SCROLL_DOWN_BUTTON_ART_NUM 318
#define SCROLL_SLIDE_TOP_ART_NUM 238
#define SCROLL_SLIDE_BOTTOM_ART_NUM 240
#define SCROLL_SLIDE_MIDDLE_ART_NUM 787

/**
 * Maximum number of simultaneously active scrollbar controls.
 */
#define MAX_CONTROLS 8

typedef uint32_t ScrollbarUiControlFlags;

#define SB_IN_USE 0x1
#define SB_HIDDEN 0x2

/**
 * Internal state for a scrollbar control.
 */
typedef struct ScrollbarUiControl {
    /* 0000 */ ScrollbarId id;
    /* 0008 */ ScrollbarUiControlFlags flags;
    /* 000C */ ScrollbarUiControlInfo info;
    /* 0050 */ tig_window_handle_t window_handle;
    /* 0054 */ TigWindowData window_data;
    /* 0074 */ tig_button_handle_t button_up;
    /* 0078 */ tig_button_handle_t button_down;
} ScrollbarUiControl;

static void scrollbar_ui_control_draw(int index, int drag_offset);
static void scrollbar_ui_control_reset(ScrollbarUiControl* ctrl);
static void scrollbar_ui_control_apply_info(ScrollbarUiControl* ctrl, ScrollbarUiControlInfo* info);
static bool scrollbar_ui_control_alloc(ScrollbarId* id);
static bool scrollbar_ui_control_get(const ScrollbarId* id, ScrollbarUiControl** ctrl_ptr);
static bool scrollbar_ui_hit_test_thumb(int index, int x, int y);
static bool scrollbar_ui_hit_test_track_above_thumb(int index, int x, int y);
static bool scrollbar_ui_hit_test_track_below_thumb(int index, int x, int y);
static bool scrollbar_ui_hit_test_content_rect(int index, int x, int y);
static float scrollbar_ui_calc_thumb_height(int index);
static int scrollbar_ui_get_thumb_height(int index);
static int scrollbar_ui_get_thumb_top(int index);
static int scrollbar_ui_get_thumb_bottom(int index);
static bool scrollbar_ui_is_value_in_range(int index, int value);
static void scrollbar_ui_redraw_lock_acquire();
static void scrollbar_ui_redraw_lock_release();
static int scrollbar_ui_redraw_lock_get();

/**
 * Source rect for the top cap of the scrollbar thumb graphic.
 *
 * 0x5CBF48
 */
static TigRect scrollbar_ui_thumb_top_src_rect = { 0, 0, 11, 5 };

/**
 * Source rect for the bottom cap of the scrollbar thumb graphic.
 *
 * 0x5CBF58
 */
static TigRect scrollbar_ui_thumb_bottom_src_rect = { 0, 0, 11, 7 };

/**
 * Source rect for the repeating middle tile of the scrollbar thumb.
 *
 * 0x5CBF68
 */
static TigRect scrollbar_ui_thumb_middle_src_rect = { 0, 0, 11, 1 };

/**
 * Index of the scrollbar whose thumb is currently being dragged, or `-1` when
 * no drag is in progress.
 *
 * 0x5CBF78
 */
static int scrollbar_ui_dragging_index = -1;

/**
 * Cached width (in pixels) of the down-arrow button art.
 *
 * 0x684250
 */
static int scrollbar_ui_button_down_width;

/**
 * Art ID for the repeating middle tile of the scrollbar thumb.
 *
 * 0x684254
 */
static tig_art_id_t scrollbar_ui_middle_art_id;

/**
 * Cached height (in pixels) of the up-arrow button art.
 *
 * 0x684258
 */
static int scrollbar_ui_button_up_height;

/**
 * Flag indicating whether the scrollbar UI module has been initialized.
 *
 * 0x68425C
 */
static bool scrollbar_ui_initialized;

/**
 * Art ID for the top cap of the scrollbar thumb.
 *
 * 0x684260
 */
static tig_art_id_t scrollbar_ui_top_art_id;

/**
 * Fixed-size pool of all scrollbar controls.
 *
 * 0x684268
 */
static ScrollbarUiControl scrollbar_ui_controls[MAX_CONTROLS];

/**
 * Art ID for the bottom cap of the scrollbar thumb.
 *
 * 0x684668
 */
static tig_art_id_t scrollbar_ui_bottom_art_id;

/**
 * Cached width (in pixels) of the up-arrow button art.
 *
 * 0x68466C
 */
static int scrollbar_ui_button_up_width;

/**
 * Cached height (in pixels) of the down-arrow button art.
 *
 * 0x684670
 */
static int scrollbar_ui_button_down_height;

/**
 * Nesting counter that suppresses redraws when non-zero.
 *
 * Incremented by `scrollbar_ui_redraw_lock_acquire` and decremented by
 * `scrollbar_ui_redraw_lock_release`. This prevents recursive redraws when the
 * `on_value_changed` callback itself triggers a value change.
 *
 * 0x684674
 */
static int scrollbar_ui_redraw_lock;

/**
 * Next global index value to assign when allocating a new scrollbar slot.
 *
 * Initialized to a random value at reset time so that stale handles from a
 * previous session cannot accidentally match a freshly created control.
 *
 * 0x684678
 */
static int scrollbar_ui_next_global_index;

/**
 * Pixel offset of the mouse cursor relative to the top of the thumb at the
 * moment a drag was initiated.
 *
 * 0x68467C
 */
static int scrollbar_ui_drag_base;

/**
 * Pending value change accumulated during a thumb drag.
 *
 * As the mouse moves, whole `line_step` increments are accumulated here
 * rather than immediately written to `info.value`. On mouse-up the pending
 * delta is committed and this variable is reset to zero.
 *
 * 0x684680
 */
static int scrollbar_ui_drag_delta;

/**
 * When non-zero, `scrollbar_ui_process_event` silently ignores all incoming
 * messages. Managed by `scrollbar_ui_begin_ignore_events` and
 * `scrollbar_ui_end_ignore_events`.
 *
 * 0x684684
 */
static bool scrollbar_ui_ignore_events_counter;

/**
 * Called when the game is initialized.
 *
 * 0x580410
 */
bool scrollbar_ui_init(GameInitInfo* init_info)
{
    (void)init_info;

    scrollbar_ui_reset();

    // Load top cap.
    if (tig_art_interface_id_create(SCROLL_SLIDE_TOP_ART_NUM, 0, 0, 0, &scrollbar_ui_top_art_id) != TIG_OK) {
        return false;
    }

    // Load bottom cap.
    if (tig_art_interface_id_create(SCROLL_SLIDE_BOTTOM_ART_NUM, 0, 0, 0, &scrollbar_ui_bottom_art_id) != TIG_OK) {
        return false;
    }

    // Load middle tile.
    if (tig_art_interface_id_create(SCROLL_SLIDE_MIDDLE_ART_NUM, 0, 0, 0, &scrollbar_ui_middle_art_id) != TIG_OK) {
        return false;
    }

    scrollbar_ui_initialized = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x580480
 */
void scrollbar_ui_exit()
{
    scrollbar_ui_initialized = false;
}

/**
 * Called when the game is being reset.
 *
 * 0x580490
 */
void scrollbar_ui_reset()
{
    int index;

    for (index = 0; index < MAX_CONTROLS; index++) {
        scrollbar_ui_control_reset(&(scrollbar_ui_controls[index]));
    }

    scrollbar_ui_next_global_index = random_between(0, 8192);
    scrollbar_ui_dragging_index = -1;
    scrollbar_ui_ignore_events_counter = false;
}

/**
 * Creates a new scrollbar control and registers it in the pool.
 *
 * 0x5804E0
 */
bool scrollbar_ui_control_create(ScrollbarId* id, ScrollbarUiControlInfo* info, tig_window_handle_t window_handle)
{
    ScrollbarUiControl* ctrl;
    TigArtFrameData art_frame_data;
    TigButtonData button_data;

    if ((info->flags & SB_INFO_VALID) == 0
        || !scrollbar_ui_control_alloc(id)
        || !scrollbar_ui_control_get(id, &ctrl)) {
        return false;
    }

    ctrl->id = *id;
    scrollbar_ui_control_apply_info(ctrl, info);

    // Clamp the initial value to the valid range.
    if (ctrl->info.value < ctrl->info.min_value
        || ctrl->info.value > ctrl->info.max_value) {
        ctrl->info.value = ctrl->info.min_value;
    }

    ctrl->window_handle = window_handle;
    tig_window_data(ctrl->window_handle, &(ctrl->window_data));

    // Create the up-arrow button at the top of the track.
    if (tig_art_interface_id_create(SCROLL_UP_BUTTON_ART_NUM, 0, 0, 0, &(button_data.art_id)) != TIG_OK) {
        return false;
    }

    tig_art_frame_data(button_data.art_id, &art_frame_data);
    scrollbar_ui_button_up_height = art_frame_data.height;
    scrollbar_ui_button_up_width = art_frame_data.width;

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = ctrl->window_handle;
    button_data.x = ctrl->info.scrollbar_rect.x + (ctrl->info.scrollbar_rect.width - art_frame_data.width) / 2;
    button_data.y = ctrl->info.scrollbar_rect.y;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_LOW;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_LOW_RELEASE;
    tig_button_create(&button_data, &(ctrl->button_up));

    // Create the down-arrow button at the bottom of the track.
    if (tig_art_interface_id_create(SCROLL_DOWN_BUTTON_ART_NUM, 0, 0, 0, &(button_data.art_id)) != TIG_OK) {
        return false;
    }

    tig_art_frame_data(button_data.art_id, &art_frame_data);
    scrollbar_ui_button_down_height = art_frame_data.height;
    scrollbar_ui_button_down_width = art_frame_data.width;

    button_data.y = ctrl->info.scrollbar_rect.y + ctrl->info.scrollbar_rect.height - art_frame_data.height;
    tig_button_create(&button_data, &(ctrl->button_down));

    // Notify host of the initial value.
    if (ctrl->info.on_value_changed != NULL) {
        ctrl->info.on_value_changed(ctrl->info.value);
    }

    return true;
}

/**
 * Destroys a scrollbar control and returns its pool slot to free use.
 *
 * 0x580690
 */
void scrollbar_ui_control_destroy(ScrollbarId id)
{
    ScrollbarUiControl* ctrl;

    if (!scrollbar_ui_control_get(&id, &ctrl)) {
        return;
    }

    tig_button_destroy(ctrl->button_up);
    tig_button_destroy(ctrl->button_down);

    // If this control was being dragged, cancel the drag.
    if (scrollbar_ui_dragging_index == ctrl->id.index) {
        scrollbar_ui_dragging_index = -1;
    }

    scrollbar_ui_control_reset(ctrl);
}

/**
 * Forces an immediate redraw of a scrollbar control at its current value.
 *
 * 0x5806F0
 */
void scrollbar_ui_control_redraw(ScrollbarId id)
{
    ScrollbarUiControl* ctrl;

    if (!scrollbar_ui_control_get(&id, &ctrl)) {
        return;
    }

    scrollbar_ui_control_draw(ctrl->id.index, 0);
}

/**
 * Makes a previously hidden scrollbar visible.
 *
 * Returns `false` if the control was not found or was already visible.
 *
 * 0x580720
 */
bool scrollbar_ui_control_show(ScrollbarId id)
{
    ScrollbarUiControl* ctrl;

    if (!scrollbar_ui_control_get(&id, &ctrl)) {
        return false;
    }

    if ((ctrl->flags & SB_HIDDEN) == 0) {
        return false;
    }

    ctrl->flags &= ~SB_HIDDEN;

    tig_button_show(ctrl->button_down);
    tig_button_show(ctrl->button_up);
    scrollbar_ui_control_draw(ctrl->id.index, 0);

    return true;
}

/**
 * Hides a scrollbar control.
 *
 * Returns `false` if the control was not found or was already hidden.
 *
 * 0x580780
 */
bool scrollbar_ui_control_hide(ScrollbarId id)
{
    ScrollbarUiControl* ctrl;

    if (!scrollbar_ui_control_get(&id, &ctrl)) {
        return false;
    }

    if ((ctrl->flags & SB_HIDDEN) != 0) {
        return false;
    }

    ctrl->flags |= SB_HIDDEN;

    tig_button_hide(ctrl->button_down);
    tig_button_hide(ctrl->button_up);

    if (ctrl->info.on_refresh != NULL) {
        ctrl->info.on_refresh(&(ctrl->info.scrollbar_rect));
    }

    return true;
}

/**
 * Renders the scrollbar thumb into its parent window.
 *
 * The `dy` is a raw pixel offset applied to the thumb position during dragging.
 *
 * 0x5807F0
 */
void scrollbar_ui_control_draw(int index, int dy)
{
    ScrollbarUiControl* ctrl;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    float thumb_height;
    bool hidden;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = 0;
    src_rect.height = 0;

    // Make sure redraw lock was acquired.
    if (scrollbar_ui_redraw_lock_get() != 0) {
        return;
    }

    ctrl = &(scrollbar_ui_controls[index]);
    if ((ctrl->flags & SB_HIDDEN) != 0) {
        return;
    }

    // Give the host a chance to repaint the scrollbar background first.
    if (ctrl->info.on_refresh != NULL) {
        ctrl->info.on_refresh(&(ctrl->info.scrollbar_rect));
    }

    thumb_height = scrollbar_ui_calc_thumb_height(ctrl->id.index);

    // Compute the Y position of the thumb top edge.
    dst_rect.x = ctrl->info.scrollbar_rect.x;
    if (ctrl->info.value != ctrl->info.max_value || dy != 0) {
        dst_rect.y = scrollbar_ui_get_thumb_top(ctrl->id.index) + dy;
    } else {
        // When at maximum value, snap the thumb to the "down" button.
        dst_rect.y = ctrl->info.scrollbar_rect.y + ctrl->info.scrollbar_rect.height - scrollbar_ui_button_down_height - scrollbar_ui_get_thumb_height(index);
    }

    // Clamp thumb so it stays within the track between two buttons.
    if (dst_rect.y < ctrl->info.scrollbar_rect.y + scrollbar_ui_button_up_height) {
        dst_rect.y = ctrl->info.scrollbar_rect.y + scrollbar_ui_button_up_height;
    } else if (dst_rect.y > ctrl->info.scrollbar_rect.y + ctrl->info.scrollbar_rect.height - scrollbar_ui_button_down_height - scrollbar_ui_get_thumb_height(index)) {
        dst_rect.y = ctrl->info.scrollbar_rect.y + ctrl->info.scrollbar_rect.height - scrollbar_ui_button_down_height - scrollbar_ui_get_thumb_height(index);
    }

    // Draw top cap.
    dst_rect.width = ctrl->info.scrollbar_rect.width;
    dst_rect.height = 5;

    art_blit_info.flags = 0;
    art_blit_info.art_id = scrollbar_ui_top_art_id;
    art_blit_info.src_rect = &scrollbar_ui_thumb_top_src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(ctrl->window_handle, &art_blit_info);

    thumb_height -= 5.0f;

    dst_rect.y += 5;
    dst_rect.width = ctrl->info.scrollbar_rect.width;
    dst_rect.height = 1;

    // Draw repeating middle tile.
    art_blit_info.flags = 0;
    art_blit_info.art_id = scrollbar_ui_middle_art_id;
    art_blit_info.src_rect = &scrollbar_ui_thumb_middle_src_rect;
    art_blit_info.dst_rect = &dst_rect;

    while (thumb_height > 7.0f) {
        tig_window_blit_art(ctrl->window_handle, &art_blit_info);
        dst_rect.y++;
        thumb_height -= 7.0f;
    }

    // Draw bottom cap.
    dst_rect.width = ctrl->info.scrollbar_rect.width;
    dst_rect.height = 7;

    art_blit_info.art_id = scrollbar_ui_bottom_art_id;
    art_blit_info.flags = 0;
    art_blit_info.src_rect = &scrollbar_ui_thumb_bottom_src_rect;
    art_blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(ctrl->window_handle, &art_blit_info);

    // If either arrow button is hidden, manually blit the button art so that
    // the buttons appear to be in place, but not actually buttons.
    tig_button_is_hidden(ctrl->button_up, &hidden);
    if (hidden) {
        dst_rect.x = ctrl->info.scrollbar_rect.x + (ctrl->info.scrollbar_rect.width - scrollbar_ui_button_up_width) / 2;
        dst_rect.y = ctrl->info.scrollbar_rect.y;
        dst_rect.width = scrollbar_ui_button_up_width;
        dst_rect.height = scrollbar_ui_button_up_height;

        src_rect.width = scrollbar_ui_button_up_width;
        src_rect.height = scrollbar_ui_button_up_height;

        tig_art_interface_id_create(SCROLL_UP_BUTTON_ART_NUM, 0, 0, 0, &art_blit_info.art_id);
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(ctrl->window_handle, &art_blit_info);
    }

    tig_button_is_hidden(ctrl->button_down, &hidden);
    if (hidden) {
        dst_rect.x = ctrl->info.scrollbar_rect.x + (ctrl->info.scrollbar_rect.width - scrollbar_ui_button_up_width) / 2;
        dst_rect.y = ctrl->info.scrollbar_rect.y + ctrl->info.scrollbar_rect.height - scrollbar_ui_button_down_height;
        dst_rect.width = scrollbar_ui_button_down_width;
        dst_rect.height = scrollbar_ui_button_down_height;

        src_rect.width = scrollbar_ui_button_down_width;
        src_rect.height = scrollbar_ui_button_down_height;

        tig_art_interface_id_create(SCROLL_DOWN_BUTTON_ART_NUM, 0, 0, 0, &(art_blit_info.art_id));
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(ctrl->window_handle, &art_blit_info);
    }
}

/**
 * Processes a UI message and routes it to the appropriate scrollbar control.
 *
 * Returns `true` if the message was consumed, `false` otherwise.
 *
 * 0x580B10
 */
bool scrollbar_ui_process_event(TigMessage* msg)
{
    int index;
    ScrollbarUiControl* ctrl;

    if (scrollbar_ui_ignore_events_counter) {
        return false;
    }

    switch (msg->type) {
    case TIG_MESSAGE_MOUSE:
        switch (msg->data.mouse.event) {
        case TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN:
            if (scrollbar_ui_dragging_index == -1) {
                for (index = 0; index < 8; index++) {
                    ctrl = &(scrollbar_ui_controls[index]);
                    if ((ctrl->flags & SB_IN_USE) != 0
                        && (ctrl->flags & SB_HIDDEN) == 0) {
                        if (scrollbar_ui_hit_test_thumb(index, msg->data.mouse.x, msg->data.mouse.y)) {
                            // Begin dragging thumb.
                            scrollbar_ui_dragging_index = index;
                            scrollbar_ui_drag_base = msg->data.mouse.y
                                - scrollbar_ui_controls[index].window_data.rect.y
                                - scrollbar_ui_get_thumb_top(index);

                            // Hide arrow buttons during dragging.
                            tig_button_hide(scrollbar_ui_controls[index].button_up);
                            tig_button_hide(scrollbar_ui_controls[index].button_down);

                            return true;
                        }

                        if (scrollbar_ui_hit_test_track_above_thumb(index, msg->data.mouse.x, msg->data.mouse.y)) {
                            // Click in trough between up button and thumb.
                            ctrl->info.value -= ctrl->info.page_step;
                            if (ctrl->info.value < ctrl->info.min_value) {
                                ctrl->info.value = ctrl->info.min_value;
                            }
                            if (ctrl->info.on_value_changed != NULL) {
                                ctrl->info.on_value_changed(ctrl->info.value);
                            }
                            scrollbar_ui_control_redraw(ctrl->id);
                        } else if (scrollbar_ui_hit_test_track_below_thumb(index, msg->data.mouse.x, msg->data.mouse.y)) {
                            // Click in trough between thumb and down button.
                            ctrl->info.value += ctrl->info.page_step;
                            if (ctrl->info.value > ctrl->info.max_value) {
                                ctrl->info.value = ctrl->info.max_value;
                            }
                            if (ctrl->info.on_value_changed != NULL) {
                                ctrl->info.on_value_changed(ctrl->info.value);
                            }
                            scrollbar_ui_control_redraw(ctrl->id);
                        }
                    }
                }
            }
            return false;
        case TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP:
            if (scrollbar_ui_dragging_index != -1) {
                // Commit the accumulated delta to the stored value.
                if (scrollbar_ui_is_value_in_range(scrollbar_ui_dragging_index, scrollbar_ui_controls[scrollbar_ui_dragging_index].info.value) + scrollbar_ui_drag_delta) {
                    scrollbar_ui_controls[scrollbar_ui_dragging_index].info.value += scrollbar_ui_drag_delta;
                }

                scrollbar_ui_drag_delta = 0;
                scrollbar_ui_control_draw(scrollbar_ui_dragging_index, 0);

                // Restore arrow buttons visibility.
                tig_button_show(scrollbar_ui_controls[scrollbar_ui_dragging_index].button_up);
                tig_button_show(scrollbar_ui_controls[scrollbar_ui_dragging_index].button_down);

                scrollbar_ui_dragging_index = -1;
                return true;
            }
            return false;
        case TIG_MESSAGE_MOUSE_MOVE:
            // Handle dragging.
            if (scrollbar_ui_dragging_index != -1) {
                int dy;
                float thumb_height;
                float remainder;

                ctrl = &(scrollbar_ui_controls[scrollbar_ui_dragging_index]);
                dy = msg->data.mouse.y
                    - scrollbar_ui_controls[scrollbar_ui_dragging_index].window_data.rect.y
                    - scrollbar_ui_drag_base
                    - scrollbar_ui_get_thumb_top(scrollbar_ui_dragging_index);
                thumb_height = scrollbar_ui_calc_thumb_height(scrollbar_ui_dragging_index);

                // Subtract the already-pending delta from the raw pixel
                // movement.
                remainder = dy - scrollbar_ui_drag_delta * thumb_height;

                if (remainder > thumb_height && ctrl->info.value + scrollbar_ui_drag_delta < ctrl->info.max_value) {
                    // Accumulate whole `line_step` increments when moving down.
                    do {
                        remainder -= thumb_height;
                        scrollbar_ui_drag_delta += ctrl->info.line_step;
                    } while (remainder > thumb_height && ctrl->info.value + scrollbar_ui_drag_delta < ctrl->info.max_value);
                } else if (remainder < -thumb_height && ctrl->info.value + scrollbar_ui_drag_delta > ctrl->info.min_value) {
                    // Accumulate whole `line_step` decrements when moving up.
                    do {
                        remainder += -thumb_height;
                        scrollbar_ui_drag_delta -= ctrl->info.line_step;
                    } while (remainder < -thumb_height && ctrl->info.value + scrollbar_ui_drag_delta > ctrl->info.min_value);
                } else {
                    // Not enough movement (in px) for a step - just update the
                    // visual position.
                    scrollbar_ui_control_draw(scrollbar_ui_dragging_index, dy);
                    return true;
                }

                // Notify host of the new value.
                if (ctrl->info.on_value_changed != NULL) {
                    if (scrollbar_ui_is_value_in_range(scrollbar_ui_dragging_index, ctrl->info.value + scrollbar_ui_drag_delta)) {
                        scrollbar_ui_redraw_lock_acquire();
                        ctrl->info.on_value_changed(ctrl->info.value + scrollbar_ui_drag_delta);
                        scrollbar_ui_redraw_lock_release();
                    } else {
                        if (ctrl->info.value + scrollbar_ui_drag_delta > ctrl->info.max_value) {
                            ctrl->info.on_value_changed(ctrl->info.max_value);
                            scrollbar_ui_control_draw(scrollbar_ui_dragging_index, ctrl->info.scrollbar_rect.height - ((int)thumb_height * (ctrl->info.value + 1)));
                            return true;
                        }

                        if (ctrl->info.value + scrollbar_ui_drag_delta < ctrl->info.min_value) {
                            ctrl->info.on_value_changed(ctrl->info.min_value);
                            scrollbar_ui_control_draw(scrollbar_ui_dragging_index, -(ctrl->info.value * (int)thumb_height));
                            return true;
                        }
                    }
                }

                scrollbar_ui_control_draw(scrollbar_ui_dragging_index, dy);
                return true;
            }
            return false;
        case TIG_MESSAGE_MOUSE_WHEEL:
            for (index = 0; index < 8; index++) {
                ctrl = &(scrollbar_ui_controls[index]);
                if ((ctrl->flags & SB_IN_USE) != 0
                    && (ctrl->flags & SB_HIDDEN) == 0
                    && scrollbar_ui_hit_test_content_rect(index, msg->data.mouse.x, msg->data.mouse.y)) {
                    if (msg->data.mouse.dy > 0) {
                        // Wheel up.
                        ctrl->info.value -= ctrl->info.wheel_step;
                        if (ctrl->info.value < ctrl->info.min_value) {
                            ctrl->info.value = ctrl->info.min_value;
                        }
                        if (ctrl->info.on_value_changed != NULL) {
                            ctrl->info.on_value_changed(ctrl->info.value);
                        }
                        scrollbar_ui_control_redraw(ctrl->id);
                        return true;
                    } else if (msg->data.mouse.dy < 0) {
                        // Wheel down.
                        ctrl->info.value += ctrl->info.wheel_step;
                        if (ctrl->info.value > ctrl->info.max_value) {
                            ctrl->info.value = ctrl->info.max_value;
                        }
                        if (ctrl->info.on_value_changed != NULL) {
                            ctrl->info.on_value_changed(ctrl->info.value);
                        }
                        scrollbar_ui_control_redraw(ctrl->id);
                        return true;
                    }
                    return false;
                }
            }
            return false;
        default:
            return false;
        }
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_PRESSED:
            for (index = 0; index < 8; index++) {
                ctrl = &(scrollbar_ui_controls[index]);
                if ((ctrl->flags & SB_IN_USE) != 0
                    && (ctrl->flags & SB_HIDDEN) == 0) {
                    // Up-arrow button - scroll one line up.
                    if (msg->data.button.button_handle == ctrl->button_up) {
                        ctrl->info.value -= ctrl->info.line_step;
                        if (ctrl->info.value < ctrl->info.min_value) {
                            ctrl->info.value = ctrl->info.min_value;
                        }
                        if (ctrl->info.on_value_changed != NULL) {
                            ctrl->info.on_value_changed(ctrl->info.value);
                        }
                        scrollbar_ui_control_redraw(ctrl->id);
                        if (scrollbar_ui_dragging_index == ctrl->id.index) {
                            scrollbar_ui_dragging_index = -1;
                        }
                        return true;
                    }

                    // Down-arrow button - scorll one line down.
                    if (msg->data.button.button_handle == ctrl->button_down) {
                        ctrl->info.value += ctrl->info.line_step;
                        if (ctrl->info.value > ctrl->info.max_value) {
                            ctrl->info.value = ctrl->info.max_value;
                        }
                        if (ctrl->info.on_value_changed != NULL) {
                            ctrl->info.on_value_changed(ctrl->info.value);
                        }
                        scrollbar_ui_control_redraw(ctrl->id);
                        if (scrollbar_ui_dragging_index == ctrl->id.index) {
                            scrollbar_ui_dragging_index = -1;
                        }
                        return true;
                    }
                }
            }
            return false;
        default:
            return false;
        }
    default:
        return false;
    }
}

/**
 * Sets the minimum value, maximum value, or current value of a scrollbar.
 *
 * `type` must be one of `SCROLLBAR_MIN_VALUE`, `SCROLLBAR_MAX_VALUE`, or
 * `SCROLLBAR_CURRENT_VALUE`. Setting min or max will clamp the current value
 * to the new range if needed. Setting the current value silently resets it to
 * `min_value` if `value` falls outside the range.
 *
 * After the change the `on_value_changed` callback is invoked (even if there
 * was no change).
 *
 * 0x5810D0
 */
void scrollbar_ui_control_set(ScrollbarId id, int type, int value)
{
    ScrollbarUiControl* ctrl;

    if (!scrollbar_ui_control_get(&id, &ctrl)) {
        return;
    }

    switch (type) {
    case SCROLLBAR_MIN_VALUE:
        ctrl->info.min_value = value;
        if (ctrl->info.value < ctrl->info.min_value) {
            ctrl->info.value = ctrl->info.min_value;
        }
        break;
    case SCROLLBAR_MAX_VALUE:
        ctrl->info.max_value = value;
        if (ctrl->info.value > ctrl->info.max_value) {
            ctrl->info.value = ctrl->info.max_value;
        }
        break;
    case SCROLLBAR_CURRENT_VALUE:
        if (value >= ctrl->info.min_value && value <= ctrl->info.max_value) {
            ctrl->info.value = value;
        } else {
            ctrl->info.value = ctrl->info.min_value;
        }
        break;
    default:
        return;
    }

    if (ctrl->info.on_value_changed != NULL) {
        ctrl->info.on_value_changed(ctrl->info.value);
    }

    scrollbar_ui_control_redraw(ctrl->id);
}

/**
 * Tells scrollbar UI module to suspend the handling of events.
 *
 * 0x5811A0
 */
void scrollbar_ui_begin_ignore_events()
{
    scrollbar_ui_ignore_events_counter = true;
}

/**
 * Tells scrollbar UI module to resume the handling of events.
 */
// 0x5811B0
void scrollbar_ui_end_ignore_events()
{
    scrollbar_ui_ignore_events_counter = false;
}

/**
 * Resets a control slot to its default invalid state.
 *
 * 0x5811C0
 */
void scrollbar_ui_control_reset(ScrollbarUiControl* ctrl)
{
    ctrl->id.index = -1;
    ctrl->id.index = -1;
    ctrl->flags = 0;
    memset(&(ctrl->info), 0, sizeof(ctrl->info));
    ctrl->window_handle = TIG_WINDOW_HANDLE_INVALID;
    ctrl->button_up = TIG_BUTTON_HANDLE_INVALID;
    ctrl->button_down = TIG_BUTTON_HANDLE_INVALID;
}

/**
 * Copies scrollbar info and fills in defaults for any fields whose
 * corresponding flag bit is not set.
 *
 * 0x5811F0
 */
void scrollbar_ui_control_apply_info(ScrollbarUiControl* ctrl, ScrollbarUiControlInfo* info)
{
    ctrl->info = *info;

    if ((ctrl->info.flags & SB_INFO_CONTENT_RECT) == 0) {
        ctrl->info.content_rect.x = 0;
        ctrl->info.content_rect.y = 0;
        ctrl->info.content_rect.width = 0;
        ctrl->info.content_rect.height = 0;
    }

    if ((ctrl->info.flags & SB_INFO_MAX_VALUE) == 0) {
        ctrl->info.max_value = 10;
    }

    if ((ctrl->info.flags & SB_INFO_MIN_VALUE) == 0) {
        ctrl->info.min_value = 0;
    }

    if ((ctrl->info.flags & SB_INFO_LINE_STEP) == 0) {
        ctrl->info.line_step = 1;
    }

    if ((ctrl->info.flags & SB_INFO_PAGE_STEP) == 0) {
        ctrl->info.page_step = 5;
    }

    if ((ctrl->info.flags & SB_INFO_WHEEL_STEP) == 0) {
        ctrl->info.wheel_step = 3;
    }

    if ((ctrl->info.flags & SB_INFO_VALUE) == 0) {
        ctrl->info.value = 0;
    }

    if ((ctrl->info.flags & SB_INFO_ON_VALUE_CHANGED) == 0) {
        ctrl->info.on_value_changed = NULL;
    }

    if ((ctrl->info.flags & SB_INFO_ON_REFRESH) == 0) {
        ctrl->info.on_refresh = NULL;
    }
}

/**
 * Allocates a free slot in the control pool and populates `id`.
 *
 * Returns `true` on success, `false` if the pool is full.
 *
 * 0x581280
 */
bool scrollbar_ui_control_alloc(ScrollbarId* id)
{
    int index;

    for (index = 0; index < MAX_CONTROLS; index++) {
        if ((scrollbar_ui_controls[index].flags & SB_IN_USE) == 0) {
            id->index = index;
            id->global_index = scrollbar_ui_next_global_index++;
            scrollbar_ui_controls[index].id = *id;
            scrollbar_ui_controls[index].flags = SB_IN_USE;
            return true;
        }
    }

    return false;
}

/**
 * Retrieves a pointer to `ScrollbarUiControl` from a given `ScrollbarId`.
 *
 * Returns `true` and sets `*ctrl_ptr` on success, `false` if no matching
 * control is found.
 *
 * 0x5812E0
 */
bool scrollbar_ui_control_get(const ScrollbarId* id, ScrollbarUiControl** ctrl_ptr)
{
    int index;

    // Quick lookup using index.
    if (id->index >= 0 && id->index < MAX_CONTROLS
        && scrollbar_ui_controls[id->index].id.global_index == id->global_index
        && (scrollbar_ui_controls[id->index].flags & SB_IN_USE) != 0) {
        *ctrl_ptr = &(scrollbar_ui_controls[id->index]);
        return true;
    }

    // "Slow" linear scan over all slots.
    for (index = 0; index < MAX_CONTROLS; index++) {
        if (scrollbar_ui_controls[index].id.global_index == id->global_index
            && (scrollbar_ui_controls[id->index].flags & SB_IN_USE) != 0) {
            *ctrl_ptr = &(scrollbar_ui_controls[id->index]);
            return true;
        }
    }

    return false;
}

/**
 * Returns `true` if the given screen coordinate lies within the thumb area of
 * scrollbar.
 *
 * 0x581360
 */
bool scrollbar_ui_hit_test_thumb(int index, int x, int y)
{
    ScrollbarUiControl* ctrl;
    TigWindowData window_data;

    ctrl = &(scrollbar_ui_controls[index]);
    tig_window_data(ctrl->window_handle, &window_data);

    return x - window_data.rect.x >= ctrl->info.scrollbar_rect.x
        && x - window_data.rect.x <= ctrl->info.scrollbar_rect.x + ctrl->info.scrollbar_rect.width
        && y - window_data.rect.y >= scrollbar_ui_get_thumb_top(index)
        && y - window_data.rect.y <= scrollbar_ui_get_thumb_bottom(index);
}

/**
 * Returns `true` if the given screen coordinate lies in the trough above the
 * thumb of scrollbar.
 *
 * Clicking in this area produces a "page-up".
 *
 * 0x5813E0
 */
bool scrollbar_ui_hit_test_track_above_thumb(int index, int x, int y)
{
    ScrollbarUiControl* ctrl;
    TigWindowData window_data;

    ctrl = &(scrollbar_ui_controls[index]);
    tig_window_data(ctrl->window_handle, &window_data);

    return x - window_data.rect.x >= ctrl->info.scrollbar_rect.x
        && x - window_data.rect.x <= ctrl->info.scrollbar_rect.x + ctrl->info.scrollbar_rect.width
        && y - window_data.rect.y >= ctrl->info.content_rect.y + scrollbar_ui_button_up_height
        && y - window_data.rect.y <= scrollbar_ui_get_thumb_top(index);
}

/**
 * Returns `true` if the given screen coordinate lies in the trough below the
 * thumb of scrollbar.
 *
 * Clicking in this area produces a "page-down".
 *
 * 0x581460
 */
bool scrollbar_ui_hit_test_track_below_thumb(int index, int x, int y)
{
    ScrollbarUiControl* ctrl;
    TigWindowData window_data;

    ctrl = &(scrollbar_ui_controls[index]);
    tig_window_data(ctrl->window_handle, &window_data);

    return x - window_data.rect.x >= ctrl->info.scrollbar_rect.x
        && x - window_data.rect.x <= ctrl->info.scrollbar_rect.x + ctrl->info.scrollbar_rect.width
        && y - window_data.rect.y >= scrollbar_ui_get_thumb_bottom(index)
        && y - window_data.rect.y <= ctrl->info.content_rect.y + ctrl->info.content_rect.height - scrollbar_ui_button_down_height;
}

/**
 * Returns `true` if the given screen coordinate lies within the content area
 * associated with scrollbar control `index`.
 *
 * Used to decide whether a mouse-wheel event should be routed to this
 * scrollbar.
 *
 * 0x5814E0
 */
bool scrollbar_ui_hit_test_content_rect(int index, int x, int y)
{
    ScrollbarUiControl* ctrl;
    TigWindowData window_data;

    ctrl = &(scrollbar_ui_controls[index]);
    tig_window_data(ctrl->window_handle, &window_data);

    return x - window_data.rect.x >= ctrl->info.content_rect.x
        && x - window_data.rect.x <= ctrl->info.content_rect.x + ctrl->info.content_rect.width
        && y - window_data.rect.y >= ctrl->info.content_rect.y
        && y - window_data.rect.y <= ctrl->info.content_rect.y + ctrl->info.content_rect.height;
}

/**
 * Calculates the ideal thumb height (in pixels).
 *
 * The thumb height is proportional to the ratio of `line_step` to the total
 * value range, scaled to the available track height (track height minus the
 * two arrow button heights).
 *
 * 0x581550
 */
float scrollbar_ui_calc_thumb_height(int index)
{
    ScrollbarUiControl* ctrl;

    ctrl = &(scrollbar_ui_controls[index]);
    return (float)(ctrl->info.scrollbar_rect.height - scrollbar_ui_button_down_height - scrollbar_ui_button_up_height)
        / (float)(ctrl->info.line_step + ctrl->info.max_value - ctrl->info.min_value)
        * (float)ctrl->info.line_step;
}

/**
 * Returns the thumb height in whole pixels, clamped to a minimum of 12 px.
 *
 * 0x5815A0
 */
int scrollbar_ui_get_thumb_height(int index)
{
    float thumb_height;

    thumb_height = scrollbar_ui_calc_thumb_height(index);
    if (thumb_height > 12.0f) {
        return (int)thumb_height;
    }
    return 12;
}

/**
 * Returns the Y coordinate of the top edge of the thumb (in window coordinate
 * system).
 *
 * 0x5815D0
 */
int scrollbar_ui_get_thumb_top(int index)
{
    return scrollbar_ui_button_up_height
        + scrollbar_ui_controls[index].info.scrollbar_rect.y
        + (int)(scrollbar_ui_calc_thumb_height(index)
            * (float)(scrollbar_ui_controls[index].info.value - scrollbar_ui_controls[index].info.min_value)
            / (float)(scrollbar_ui_controls[index].info.line_step));
}

/**
 * Returns the Y coordinate of the bottom edge of the thumb (in window
 * coordinate system).
 *
 * 0x581660
 */
int scrollbar_ui_get_thumb_bottom(int index)
{
    return scrollbar_ui_get_thumb_top(index) + scrollbar_ui_get_thumb_height(index);
}

/**
 * Returns `true` if `value` lies within the valid range of scrollbar.
 *
 * 0x5816A0
 */
bool scrollbar_ui_is_value_in_range(int index, int value)
{
    return value >= scrollbar_ui_controls[index].info.min_value
        && value <= scrollbar_ui_controls[index].info.max_value;
}

/**
 * Increments the redraw lock counter, suppressing redraws while non-zero.
 *
 * 0x5816D0
 */
void scrollbar_ui_redraw_lock_acquire()
{
    scrollbar_ui_redraw_lock++;
}

/**
 * Decrements the redraw lock counter, re-enabling redraws when it reaches 0.
 *
 * 0x5816E0
 */
void scrollbar_ui_redraw_lock_release()
{
    scrollbar_ui_redraw_lock--;
}

/**
 * Returns the current redraw lock counter value.
 *
 * 0x5816F0
 */
int scrollbar_ui_redraw_lock_get()
{
    return scrollbar_ui_redraw_lock;
}
