#include "game/tc.h"

/**
 * The number of lines in the text conversation.
 */
#define TEXT_CONVERSATION_LINES 5

/**
 * Horizontal space before and after the option text (in pixels).
 */
#define TEXT_CONVERSATION_HOR_PADDING 6

/**
 * Vertical space between between lines (in pixels).
 */
#define TEXT_CONVERSATION_VERT_SPACING 3

static void tc_render_internal(TigRectListNode* node);

/**
 * Used to track index of the currently highlighted option.
 *
 * 0x5B7218
 */
static int tc_highlighted_idx = -1;

/**
 * The content area of the text conversation (in window coordinate system).
 *
 * 0x5FF4F8
 */
static TigRect tc_content_rect;

/**
 * The bounds of intermediate video buffers (in it's own coordinate system).
 *
 * 0x5FF508
 */
static TigRect tc_intermediate_video_buffer_rect;

/**
 * Video buffer used as a scratch space for rendering text options.
 *
 * This video buffer is transparent (color keyed).
 *
 * 0x5FF518
 */
static TigVideoBuffer* tc_scratch_video_buffer;

/**
 * Parent window bounds.
 *
 * 0x5FF520
 */
static TigRect tc_iso_window_rect;

/**
 * Font handle for rendering highlighted text option.
 *
 * 0x5FF530
 */
static tig_font_handle_t tc_yellow_font;

/**
 * Function pointer to invalidate a rectangle in the parent window.
 *
 * 0x5FF534
 */
static IsoInvalidateRectFunc* tc_iso_window_invalidate_rect;

/**
 * Font handle for rendering normal text option (i.e. not highlighted).
 *
 * 0x5FF538
 */
static tig_font_handle_t tc_white_font;

/**
 * Temporary video buffer used for rendering a portion of game window, covered
 * by text conversation.
 *
 * 0x5FF53C
 */
static TigVideoBuffer* tc_backdrop_video_buffer;

/**
 * Height of the line in the text conversation.
 *
 * 0x5FF540
 */
static int tc_line_height;

/**
 * Flag indicating some internal state which is never set. Its exact meaning is
 * unclear.
 *
 * 0x5FF544
 */
static bool dword_5FF544;

/**
 * Flag indicating whether the text conversation is in editor mode.
 *
 * 0x5FF548
 */
static bool tc_editor;

/**
 * Font handle for rendering text conversation line in red.
 *
 * The exact purpose is unclear. The flag which controls the use of this font
 * (`dword_5FF544`) is never set.
 *
 * 0x5FF54C
 */
static tig_font_handle_t tc_red_font;

/**
 * Handle to the parent window.
 *
 * 0x5FF550
 */
static tig_window_handle_t tc_iso_window_handle;

/**
 * Array of strings representing text conversation options.
 *
 * 0x5FF554
 */
static const char* tc_options[TEXT_CONVERSATION_LINES];

/**
 * Flag indicating whether the text conversation is currently active and
 * visible.
 *
 * 0x5FF568
 */
static bool tc_active;

/**
 * Called when the game is initialized.
 *
 * 0x4C9280
 */
bool tc_init(GameInitInfo* init_info)
{
    TigWindowData window_data;
    TigVideoBufferCreateInfo vb_create_info;
    tig_art_id_t art_id;
    TigFont font;

    tc_editor = init_info->editor;

    // Make sure we're not in the editor.
    if (tc_editor) {
        return true;
    }

    // Retrieve window data to set up content rectangle.
    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    tc_iso_window_handle = init_info->iso_window_handle;
    tc_iso_window_invalidate_rect = init_info->invalidate_rect_func;

    tc_iso_window_rect = window_data.rect;

    // Set up the intermediate video buffers' rect.
    tc_intermediate_video_buffer_rect.x = 0;
    tc_intermediate_video_buffer_rect.y = 0;
    tc_intermediate_video_buffer_rect.width = window_data.rect.width;
    tc_intermediate_video_buffer_rect.height = 100;

    // Create the scratch video buffer for rendering text (color keyed for
    // transparency).
    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_COLOR_KEY | TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
    vb_create_info.width = tc_intermediate_video_buffer_rect.width;
    vb_create_info.height = tc_intermediate_video_buffer_rect.height;
    vb_create_info.color_key = tig_color_make(0, 0, 0);
    vb_create_info.background_color = vb_create_info.color_key;
    if (tig_video_buffer_create(&vb_create_info, &tc_scratch_video_buffer) != TIG_OK) {
        return false;
    }

    // Create the backdrop video buffer for rendering the tinted background.
    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
    vb_create_info.width = tc_intermediate_video_buffer_rect.width;
    vb_create_info.height = tc_intermediate_video_buffer_rect.height;
    vb_create_info.background_color = 0;
    if (tig_video_buffer_create(&vb_create_info, &tc_backdrop_video_buffer) != TIG_OK) {
        tig_video_buffer_destroy(tc_scratch_video_buffer);
        return false;
    }

    // Set up font creation parameters.
    tig_art_interface_id_create(229, 0, 0, 0, &art_id);
    font.flags = TIG_FONT_SHADOW;
    font.art_id = art_id;

    // Create white font for normal text.
    font.color = tig_color_make(255, 255, 255);
    tig_font_create(&font, &tc_white_font);

    // Create yellow font for highlighted text.
    font.color = tig_color_make(255, 255, 0);
    tig_font_create(&font, &tc_yellow_font);

    // Create red font, the purpose is unclear. This font is only used instead
    // of the yellow font, but appropriate variable `dword_5FF544` is never set.
    font.color = tig_color_make(255, 0, 0);
    tig_font_create(&font, &tc_red_font);

    // Calculate the line height based on the available vertical space.
    tc_line_height = (tc_intermediate_video_buffer_rect.height - ((TEXT_CONVERSATION_LINES + 1) * TEXT_CONVERSATION_VERT_SPACING)) / TEXT_CONVERSATION_LINES;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4C9540
 */
void tc_exit(void)
{
    // Make sure we're not in the editor.
    if (tc_editor) {
        return;
    }

    // Destroy fonts.
    tig_font_destroy(tc_red_font);
    tig_font_destroy(tc_yellow_font);
    tig_font_destroy(tc_white_font);

    // Destroy intermediate buffers.
    tig_video_buffer_destroy(tc_backdrop_video_buffer);
    tig_video_buffer_destroy(tc_scratch_video_buffer);

    tc_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    tc_iso_window_invalidate_rect = NULL;
}

/**
 * Called when the window size has changed.
 *
 * 0x4C95A0
 */
void tc_resize(GameResizeInfo* resize_info)
{
    bool was_active;

    // Preserve the active state to restore it after resizing.
    was_active = tc_active;
    if (was_active) {
        tc_hide();
    }

    // Update window properties.
    tc_iso_window_handle = resize_info->window_handle;
    tc_iso_window_rect = resize_info->window_rect;

    // Restore the active state if it was active before resizing.
    if (was_active) {
        tc_show();
    }
}

/**
 * Called to draw contents to the game window.
 *
 * 0x4C95F0
 */
void tc_draw(GameDrawInfo* draw_info)
{
    // Make sure we're not in the editor.
    if (tc_editor) {
        return;
    }

    // Make sure text conversation is currently active.
    if (!tc_active) {
        return;
    }

    tc_render_internal(*draw_info->rects);
}

/**
 * Called when game isometric window is scrolled (either with mouse or
 * keyboard).
 *
 * 0x4C9620
 */
void tc_scroll(int dx, int dy)
{
    TigRect rect;

    // Make sure we're not in the editor.
    if (tc_editor) {
        return;
    }

    // Make sure text conversation is currently active.
    if (!tc_active) {
        return;
    }

    rect = tc_content_rect;

    // Expand affected rect based on horizontal scroll.
    if (dx < 0) {
        rect.x += dx;
        rect.width -= dx;
    } else if (dx > 0) {
        rect.width += dx;
    }

    // Expand affected rect based on vertical scroll.
    if (dy < 0) {
        rect.y += dy;
        rect.height -= dy;
    } else if (dy > 0) {
        rect.height += dy;
    }

    // Invalidate the affected rect (which now includes portions affected by
    // scrolling) to trigger a redraw.
    tc_iso_window_invalidate_rect(&rect);
}

/**
 * Shows the text conversation.
 *
 * This function does nothing if conversation is already visible.
 *
 * 0x4C96C0
 */
void tc_show(void)
{
    // Make sure we're not in the editor.
    if (tc_editor) {
        return;
    }

    // Make sure text conversation is not currently active.
    if (tc_active) {
        return;
    }

    // Mark as visible and trigger a redraw.
    tc_active = true;
    tc_iso_window_invalidate_rect(&tc_content_rect);
}

/**
 * Hides the text conversation.
 *
 * This function does nothing if conversation is not currently visible.
 *
 * 0x4C96F0
 */
void tc_hide(void)
{
    // Make sure we're not in the editor.
    if (tc_editor) {
        return;
    }

    // Make sure text conversation is currently active.
    if (!tc_active) {
        return;
    }

    tc_active = false;
    tc_iso_window_invalidate_rect(&tc_content_rect);
}

/**
 * Clears the text conversation area.
 *
 * 0x4C9720
 */
void tc_clear(bool compact)
{
    int idx;

    // Make sure we're not in the editor.
    if (tc_editor) {
        return;
    }

    // Clear scratch (text) video buffer with black color (which is also a key
    // color, effectively making it transparent).
    tig_video_buffer_fill(tc_scratch_video_buffer,
        &tc_intermediate_video_buffer_rect,
        tig_color_make(0, 0, 0));

    // Reset all option strings.
    for (idx = 0; idx < TEXT_CONVERSATION_LINES; idx++) {
        tc_options[idx] = NULL;
    }

    // Invalidate current content rect (if any).
    tc_iso_window_invalidate_rect(&tc_content_rect);

    // Reset highlight state and some internal flag (which is unused).
    tc_highlighted_idx = -1;
    dword_5FF544 = false;

    // Set default content size and center it horizontally.
    tc_content_rect.width = 400;
    tc_content_rect.height = 100;
    tc_content_rect.x = (tc_iso_window_rect.width - tc_content_rect.width) / 2;

    // Adjust vertical position depending on normal vs. compact mode.
    if (compact) {
        tc_content_rect.y = tc_iso_window_rect.height - tc_content_rect.height - 37;
    } else {
        tc_content_rect.y = tc_iso_window_rect.height - tc_content_rect.height - 159;
    }
}

/**
 * Sets a text conversation option at the specified index.
 *
 * 0x4C9810
 */
void tc_set_option(int index, const char* str)
{
    TigRect rect;
    TigRect dirty_rect;
    TigMouseState mouse_state;
    bool highlighted;

    // Make sure we're not in the editor.
    if (tc_editor) {
        return;
    }

    // Store the option string. Note that we don't create a copy here.
    tc_options[index] = str;

    rect.x = TEXT_CONVERSATION_HOR_PADDING;
    rect.y = index * (tc_line_height + TEXT_CONVERSATION_VERT_SPACING) + TEXT_CONVERSATION_VERT_SPACING;
    rect.width = tc_intermediate_video_buffer_rect.width - TEXT_CONVERSATION_HOR_PADDING * 2;
    rect.height = tc_line_height;

    // Clear scratch (text) video buffer with black color (which is also a key
    // color, effectively making it transparent).
    tig_video_buffer_fill(tc_scratch_video_buffer,
        &rect,
        tig_color_make(0, 0, 0));

    highlighted = false;

    // Check of the mouse is over the option's rect and pick an appropriate
    // font.
    //
    // TODO: Probably missing horizontal rect constraints.
    if (tig_mouse_get_state(&mouse_state) == TIG_OK
        && mouse_state.x - tc_iso_window_rect.x >= tc_content_rect.x
        && mouse_state.y - tc_iso_window_rect.y >= tc_content_rect.y
        && mouse_state.x - tc_iso_window_rect.x < tc_content_rect.x + tc_content_rect.width
        && mouse_state.y - tc_iso_window_rect.y < tc_content_rect.y + tc_content_rect.height
        && mouse_state.y - tc_iso_window_rect.y - tc_content_rect.y >= rect.y
        && mouse_state.y - tc_iso_window_rect.y - tc_content_rect.y < rect.y + rect.height) {
        highlighted = true;

        if (dword_5FF544) {
            // NOTE: Since `dword_5FF544` is never set, this code path is dead.
            tig_font_push(tc_red_font);
        } else {
            // Use yellow to highlight option.
            tig_font_push(tc_yellow_font);
        }
    } else {
        // Use white font for non-highlighted options.
        tig_font_push(tc_white_font);
    }

    // Render the option text to the scratch (text) video buffer.
    tig_font_write(tc_scratch_video_buffer, str, &rect, &dirty_rect);
    tig_font_pop();

    // Update content rect's width and reposition it if the text in this option
    // is wider than the current width.
    if (tc_content_rect.width < dirty_rect.width + TEXT_CONVERSATION_HOR_PADDING * 2) {
        tc_content_rect.width = dirty_rect.width + TEXT_CONVERSATION_HOR_PADDING * 2;
        tc_content_rect.x = (tc_iso_window_rect.width - tc_content_rect.width) / 2;
    }

    // Invalidate the content rect if conversation is currently visible.
    if (tc_active) {
        tc_iso_window_invalidate_rect(&tc_content_rect);
    }

    if (highlighted && tc_highlighted_idx != index) {
        // Refresh the previously highlighted option to remove its highlight.
        if (tc_highlighted_idx != -1) {
            if (tc_options[tc_highlighted_idx] != NULL) {
                tc_set_option(tc_highlighted_idx, tc_options[tc_highlighted_idx]);
            }
        }
        tc_highlighted_idx = index;
    }
}

/**
 * Returns the index of text conversation option being pressed.
 *
 * 0x4C9A10
 */
int tc_handle_message(TigMessage* msg)
{
    int index;

    // Make sure we're not in the editor.
    if (tc_editor) {
        return -1;
    }

    // Make sure text conversation is currently active.
    if (!tc_active) {
        return -1;
    }

    // Only process mouse messages.
    if (msg->type != TIG_MESSAGE_MOUSE) {
        return -1;
    }

    // Check if the mouse is within the content rectangle.
    if (msg->data.mouse.x < tc_iso_window_rect.x + tc_content_rect.x
        || msg->data.mouse.y < tc_iso_window_rect.y + tc_content_rect.y
        || msg->data.mouse.x >= tc_iso_window_rect.x + tc_content_rect.x + tc_content_rect.width
        || msg->data.mouse.y >= tc_iso_window_rect.y + tc_content_rect.y + tc_content_rect.height) {
        // Mouse cursor is outside of the text conversation area, clear the
        // highlighted option.
        if (tc_highlighted_idx != -1) {
            tc_set_option(tc_highlighted_idx, tc_options[tc_highlighted_idx]);
            tc_highlighted_idx = -1;
        }
        return -1;
    }

    switch (msg->data.mouse.event) {
    case TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN:
        if (msg->data.mouse.repeat) {
            return -1;
        }

        // Calculate the option index based on the mouse's vertical position.
        index = (msg->data.mouse.y - (tc_iso_window_rect.y + tc_content_rect.y)) / (tc_intermediate_video_buffer_rect.height / TEXT_CONVERSATION_LINES);
        if (tc_options[index] != NULL) {
            return index;
        }
        return -1;
    case TIG_MESSAGE_MOUSE_MOVE:
        // Calculate the option index based on the mouse's vertical position.
        index = (msg->data.mouse.y - (tc_iso_window_rect.y + tc_content_rect.y)) / (tc_intermediate_video_buffer_rect.height / TEXT_CONVERSATION_LINES);

        // Clear the previous highlight if mouse moved to a different option.
        if (tc_highlighted_idx != index && tc_highlighted_idx != -1) {
            tc_set_option(tc_highlighted_idx, tc_options[tc_highlighted_idx]);
        }

        // Highlight the new option if it exists.
        if (tc_options[index] != NULL) {
            tc_set_option(index, tc_options[index]);
            tc_highlighted_idx = index;
        } else {
            tc_highlighted_idx = -1;
        }
        return -1;
    default:
        return -1;
    }
}

/**
 * Measures the width required to render a string and return overflow in pixels.
 *
 * This function is used to validate that a particular dialog option will fit
 * into text conversation area (remember there is no wrapping).
 *
 * Return `0` if the is no overflow.
 *
 * 0x4C9B90
 */
int tc_check_size(const char* str)
{
    TigFont font;
    int width;

    font.width = 0;
    font.str = str;

    tig_font_push(tc_white_font);
    tig_font_measure(&font);
    tig_font_pop();

    width = font.width - tc_intermediate_video_buffer_rect.width + TEXT_CONVERSATION_HOR_PADDING * 2;
    if (width < 0) {
        width = 0;
    }

    return width;
}

/**
 * Renders the text conversation.
 *
 * `node` is a head of list of dirty rects.
 *
 * 0x4C9BE0
 */
void tc_render_internal(TigRectListNode* node)
{
    TigRect src_rect;
    TigRect dst_rect;
    TigWindowBlitInfo win_to_vb_blt;
    TigWindowBlitInfo vb_to_win_blt;
    TigRectListNode* tmp_head;
    TigRectListNode* tmp_node;

    // Set up blit operation to copy from window to backdrop buffer.
    win_to_vb_blt.type = TIG_WINDOW_BLT_WINDOW_TO_VIDEO_BUFFER;
    win_to_vb_blt.vb_blit_flags = 0;
    win_to_vb_blt.src_window_handle = tc_iso_window_handle;
    win_to_vb_blt.src_rect = &src_rect;
    win_to_vb_blt.dst_video_buffer = tc_backdrop_video_buffer;
    win_to_vb_blt.dst_rect = &dst_rect;

    // Set up blit operation to copy from backdrop buffer to window.
    vb_to_win_blt.type = TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW;
    vb_to_win_blt.vb_blit_flags = 0;
    vb_to_win_blt.src_video_buffer = tc_backdrop_video_buffer;
    vb_to_win_blt.src_rect = &dst_rect;
    vb_to_win_blt.dst_window_handle = tc_iso_window_handle;
    vb_to_win_blt.dst_rect = &src_rect;

    tmp_head = NULL;

    while (node != NULL) {
        if (tig_rect_intersection(&tc_content_rect, &(node->rect), &src_rect) == TIG_OK) {
            // Calculate the destination rect relative to the content rect.
            dst_rect.x = src_rect.x - tc_content_rect.x;
            dst_rect.y = src_rect.y - tc_content_rect.y;
            dst_rect.width = src_rect.width;
            dst_rect.height = src_rect.height;

            // Copy the window content to the backdrop buffer, tint it, and copy
            // it back.
            tig_window_blit(&win_to_vb_blt);
            tig_video_buffer_tint(tc_backdrop_video_buffer,
                &dst_rect,
                tig_color_make(18, 18, 18),
                TIG_VIDEO_BUFFER_TINT_MODE_SUB);
            tig_window_blit(&vb_to_win_blt);

            // Store the affected rect in a new list for optimized text
            // rendering.
            tmp_node = tig_rect_node_create();
            tmp_node->rect = dst_rect;
            tmp_node->next = tmp_head;
            tmp_head = tmp_node;
        }
        node = node->next;
    }

    // Switch to render from scratch (text) buffer to window.
    vb_to_win_blt.src_video_buffer = tc_scratch_video_buffer;

    while (tmp_head != NULL) {
        vb_to_win_blt.src_rect = &(tmp_head->rect);

        // Adjust the source rect to window coordinates.
        src_rect = tmp_head->rect;
        src_rect.x += tc_content_rect.x;
        src_rect.y += tc_content_rect.y;
        tig_window_blit(&vb_to_win_blt);

        // Free this node.
        tmp_node = tmp_head->next;
        tig_rect_node_destroy(tmp_head);
        tmp_head = tmp_node;
    }
}
