#include "game/tb.h"

#include "game/gamelib.h"
#include "game/object.h"

/**
 * The maximum number of text bubbles that can be active simultaneously.
 */
#define MAX_TEXT_BUBBLES 8

/**
 * The maximum width of the text bubble rectangle.
 */
#define TEXT_BUBBLE_WIDTH 200

/**
 * The maximum height of the text bubble rectangle.
 */
#define TEXT_BUBBLE_HEIGHT 200

typedef unsigned int TextBubbleFlags;

/**
 * Flag indicating that a text bubble is currently active.
 */
#define TEXT_BUBBLE_IN_USE 0x0001u

/**
 * Flag indicating that a text bubble is permanent and does not expire
 * automatically.
 */
#define TEXT_BUBBLE_PERMANENT 0x0002u

/**
 * Describes possible positions for text bubbles relative to a game object.
 */
typedef enum TbPosition {
    TB_POS_INVALID = -1,
    TB_POS_TOP,
    TB_POS_TOP_RIGHT,
    TB_POS_RIGHT,
    TB_POS_BOTTOM_RIGHT,
    TB_POS_BOTTOM,
    TB_POS_BOTTOM_LEFT,
    TB_POS_LEFT,
    TB_POS_TOP_LEFT,
    TB_POS_COUNT,
} TbPosition;

/**
 * Represents a text bubble associated with a game object.
 */
typedef struct TextBubble {
    /* 0000 */ TextBubbleFlags flags;
    /* 0008 */ int64_t obj;
    /* 0010 */ tig_timestamp_t timestamp;
    /* 0014 */ int duration; // Duration (in milliseconds) the bubble remains visible.
    /* 0018 */ TigVideoBuffer* video_buffer;
    /* 001C */ TigRect rect;
    /* 002C */ TbPosition pos;
} TextBubble;

static void tb_remove_internal(TextBubble* tb);
static void tb_get_rect(TextBubble* tb, TigRect* rect);
static void tb_calc_rect(TextBubble* tb, int64_t loc, int offset_x, int offset_y, TigRect* rect);
static void tb_text_duration_changed();
static TextBubble* find_text_bubble(int64_t obj);
static TextBubble* find_free_text_bubble(int64_t obj);
static void adjust_text_bubble_rect(TigRect* rect, TbPosition pos);

/**
 * Color values (RGB) for text bubble types.
 *
 * 0x5B8EA0
 */
static uint8_t tb_colors[TB_TYPE_COUNT][3] = {
    /* TB_TYPE_WHITE */ { 255, 255, 255 },
    /*   TB_TYPE_RED */ { 255, 0, 0 },
    /* TB_TYPE_GREEN */ { 0, 255, 0 },
    /*  TB_TYPE_BLUE */ { 0, 0, 255 },
};

/**
 * The maximum text bubble bounds (in it's own coordinate system).
 *
 * 0x5B8EB0
 */
static TigRect tb_content_rect = { 0, 0, TEXT_BUBBLE_WIDTH, TEXT_BUBBLE_HEIGHT };

/**
 * Table defining preferred text bubble positions for nine screen cells. Each
 * row corresponds to a cell, with columns listing position preferences in
 * order.
 *
 * 0x5B8EC0
 */
static TbPosition tb_placement_tbl[9][TB_POS_COUNT] = {
    { TB_POS_TOP, TB_POS_TOP_LEFT, TB_POS_LEFT, TB_POS_BOTTOM_LEFT, TB_POS_BOTTOM, TB_POS_TOP_RIGHT, TB_POS_RIGHT, TB_POS_BOTTOM_RIGHT },
    { TB_POS_TOP, TB_POS_TOP_RIGHT, TB_POS_TOP_LEFT, TB_POS_RIGHT, TB_POS_LEFT, TB_POS_BOTTOM_RIGHT, TB_POS_BOTTOM_LEFT, TB_POS_BOTTOM },
    { TB_POS_TOP, TB_POS_TOP_RIGHT, TB_POS_RIGHT, TB_POS_BOTTOM_RIGHT, TB_POS_BOTTOM, TB_POS_TOP_LEFT, TB_POS_LEFT, TB_POS_BOTTOM_LEFT },
    { TB_POS_TOP_LEFT, TB_POS_LEFT, TB_POS_BOTTOM_LEFT, TB_POS_TOP, TB_POS_BOTTOM, TB_POS_TOP_RIGHT, TB_POS_BOTTOM_RIGHT, TB_POS_RIGHT },
    { TB_POS_TOP, TB_POS_TOP_RIGHT, TB_POS_RIGHT, TB_POS_BOTTOM_RIGHT, TB_POS_BOTTOM, TB_POS_BOTTOM_LEFT, TB_POS_LEFT, TB_POS_TOP_LEFT },
    { TB_POS_TOP_RIGHT, TB_POS_RIGHT, TB_POS_BOTTOM_RIGHT, TB_POS_TOP, TB_POS_BOTTOM, TB_POS_TOP_LEFT, TB_POS_BOTTOM_LEFT, TB_POS_LEFT },
    { TB_POS_TOP_LEFT, TB_POS_LEFT, TB_POS_BOTTOM_LEFT, TB_POS_BOTTOM, TB_POS_BOTTOM_RIGHT, TB_POS_RIGHT, TB_POS_TOP_RIGHT, TB_POS_TOP },
    { TB_POS_BOTTOM, TB_POS_BOTTOM_RIGHT, TB_POS_BOTTOM_LEFT, TB_POS_RIGHT, TB_POS_LEFT, TB_POS_TOP_RIGHT, TB_POS_TOP_LEFT, TB_POS_TOP },
    { TB_POS_TOP_RIGHT, TB_POS_RIGHT, TB_POS_BOTTOM_RIGHT, TB_POS_BOTTOM, TB_POS_BOTTOM_LEFT, TB_POS_LEFT, TB_POS_TOP_LEFT, TB_POS_TOP },
};

/**
 * Parent window bounds.
 *
 * 0x602920
 */
static TigRect tb_iso_content_rect;

/**
 * Text bubbles.
 *
 * 0x602930
 */
static TextBubble tb_text_bubbles[MAX_TEXT_BUBBLES];

/**
 * Function pointer to invalidate a rectangle in the parent window.
 *
 * 0x602AB0
 */
static IsoInvalidateRectFunc* tb_iso_window_invalidate_rect;

/**
 * Flag indicating whether text bubbles rendering is enabled.
 *
 * 0x602AB4
 */
static bool tb_enabled;

/**
 * The default duration (in milliseconds) that text bubbles remain visible.
 *
 * 0x602AB8
 */
static int tb_text_duration;

/**
 * Handle to the parent window.
 *
 * 0x602ABC
 */
static tig_window_handle_t tb_iso_window_handle;

/**
 * 0x602AC0
 */
static int dword_602AC0;

/**
 * Background color for text bubbles.
 *
 * 0x602AC4
 */
static tig_color_t tb_background_color;

/**
 * Editor view options.
 *
 * 0x602AC8
 */
static ViewOptions tb_view_options;

/**
 * Fonts for text bubble types.
 *
 * 0x602AD0
 */
static tig_font_handle_t tb_fonts[TB_TYPE_COUNT];

/**
 * Called when the game is initialized.
 *
 * 0x4D5B80
 */
bool tb_init(GameInitInfo* init_info)
{
    TigWindowData window_data;
    TigVideoBufferCreateInfo vb_create_info;
    int idx;
    TigFont font;

    // Retrieve window data to set up content rectangle.
    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    tb_iso_window_handle = init_info->iso_window_handle;
    tb_iso_window_invalidate_rect = init_info->invalidate_rect_func;

    tb_iso_content_rect.x = 0;
    tb_iso_content_rect.y = 0;
    tb_iso_content_rect.width = window_data.rect.width;
    tb_iso_content_rect.height = window_data.rect.height;

    tb_view_options.type = VIEW_TYPE_ISOMETRIC;

    tb_enabled = true;
    tb_background_color = tig_color_make(0, 0, 255);

    // Set up video buffer creation parameters.
    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY | TIG_VIDEO_BUFFER_CREATE_COLOR_KEY;
    vb_create_info.width = TEXT_BUBBLE_WIDTH;
    vb_create_info.height = TEXT_BUBBLE_HEIGHT;
    vb_create_info.background_color = tb_background_color;
    vb_create_info.color_key = tb_background_color;

    dword_602AC0 = tig_color_make(0, 0, 0);

    // Create video buffers for each text bubble.
    for (idx = 0; idx < MAX_TEXT_BUBBLES; idx++) {
        if (tig_video_buffer_create(&vb_create_info, &(tb_text_bubbles[idx].video_buffer)) != TIG_OK) {
            // Clean up previously allocated buffers.
            while (--idx >= 0) {
                tig_video_buffer_destroy(tb_text_bubbles[idx].video_buffer);
            }

            // Something went wrong.
            return false;
        }
    }

    // Set up font creation parameters.
    font.flags = TIG_FONT_NO_ALPHA_BLEND | TIG_FONT_CENTERED | TIG_FONT_SHADOW;
    tig_art_interface_id_create(229, 0, 0, 0, &(font.art_id));

    // Create fonts for each text bubble type with appropriate colors.
    for (idx = 0; idx < TB_TYPE_COUNT; idx++) {
        font.color = tig_color_make(tb_colors[idx][0], tb_colors[idx][1], tb_colors[idx][2]);
        tig_font_create(&font, &(tb_fonts[idx]));
    }

    // Register `text duration` setting and initialize it.
    settings_register(&settings, TEXT_DURATION_KEY, "6", tb_text_duration_changed);
    tb_text_duration_changed();

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4D5DB0
 */
void tb_reset()
{
    tb_clear();
}

/**
 * Called when the game shuts down.
 *
 * 0x4D5DC0
 */
void tb_exit()
{
    int idx;

    // Clear all active text bubbles.
    tb_clear();

    // Destroy fonts for each text bubble type.
    for (idx = 0; idx < TB_TYPE_COUNT; idx++) {
        tig_font_destroy(tb_fonts[idx]);
    }

    // Destroy video buffers for each text bubble.
    for (idx = 0; idx < MAX_TEXT_BUBBLES; idx++) {
        tig_video_buffer_destroy(tb_text_bubbles[idx].video_buffer);
    }

    tb_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    tb_iso_window_invalidate_rect = NULL;
}

/**
 * Called when the window size has changed.
 *
 * 0x4D5E20
 */
void tb_resize(GameResizeInfo* resize_info)
{
    tb_iso_content_rect = resize_info->content_rect;
    tb_iso_window_handle = resize_info->window_handle;
}

/**
 * Called when view settings have changed.
 *
 * 0x4D5E60
 */
void tb_update_view(ViewOptions* view_options)
{
    tb_view_options = *view_options;
}

/**
 * Called when the map is closed.
 *
 * 0x4D5E80
 */
void tb_map_close()
{
    tb_clear();
}

/**
 * Toggles the visibility of text bubbles.
 *
 * 0x4D5E90
 */
void tb_toggle()
{
    tb_enabled = !tb_enabled;
}

/**
 * Called every frame.
 *
 * 0x4D5EB0
 */
void tb_ping(tig_timestamp_t timestamp)
{
    int idx;
    TigRect rect;

    for (idx = 0; idx < MAX_TEXT_BUBBLES; idx++) {
        // Check if the bubble has expired.
        if ((tb_text_bubbles[idx].flags & TEXT_BUBBLE_IN_USE) != 0
            && (tb_text_bubbles[idx].flags & TEXT_BUBBLE_PERMANENT) == 0
            && tig_timer_between(tb_text_bubbles[idx].timestamp, timestamp) > tb_text_bubbles[idx].duration) {
            // Retrive and invalidate text bubble screen rectangle.
            tb_get_rect(&(tb_text_bubbles[idx]), &rect);
            tb_iso_window_invalidate_rect(&rect);

            // Destroy the bubble.
            tb_remove_internal(&(tb_text_bubbles[idx]));
        }
    }
}

/**
 * Renders all active text bubbles to the window.
 *
 * 0x4D5F10
 */
void tb_draw(GameDrawInfo* draw_info)
{
    int idx;
    TigRect tb_rect;
    TigRectListNode* node;
    TigRect dst_rect;
    TigRect src_rect;

    // Ensure text bubble rendering is enabled.
    if (!tb_enabled) {
        return;
    }

    // Ensure we're in isometric view. The text bubble module is not supposed
    // to work in the editor.
    if (tb_view_options.type != VIEW_TYPE_ISOMETRIC) {
        return;
    }

    // Iterate through active text bubbles.
    for (idx = 0; idx < MAX_TEXT_BUBBLES; idx++) {
        if ((tb_text_bubbles[idx].flags & TEXT_BUBBLE_IN_USE) != 0) {
            // Get the bubble's screen rect.
            tb_get_rect(&(tb_text_bubbles[idx]), &tb_rect);

            // Iterate through dirty rects to check if text bubble needs to be
            // rendered at all.
            node = *draw_info->rects;
            while (node != NULL) {
                if (tig_rect_intersection(&tb_rect, &(node->rect), &dst_rect) == TIG_OK) {
                    // Calculate source rect.
                    src_rect.x = dst_rect.x + tb_text_bubbles[idx].rect.x - tb_rect.x;
                    src_rect.y = dst_rect.y + tb_text_bubbles[idx].rect.y - tb_rect.y;
                    src_rect.width = dst_rect.width;
                    src_rect.height = dst_rect.height;

                    // Copy the affected portion of text bubble's video buffer
                    // onto the window.
                    tig_window_copy_from_vbuffer(tb_iso_window_handle,
                        &dst_rect,
                        tb_text_bubbles[idx].video_buffer,
                        &src_rect);
                }
                node = node->next;
            }
        }
    }
}

/**
 * Adds a new text bubble for a game object.
 *
 * 0x4D5FE0
 */
void tb_add(int64_t obj, int type, const char* str)
{
    TextBubble* tb;
    TigRect dirty_rect;

    // Find or allocate a text bubble for the object.
    tb = find_free_text_bubble(obj);
    if (tb == NULL) {
        return;
    }

    // Reset video buffer with the background color.
    tig_video_buffer_fill(tb->video_buffer, &tb_content_rect, tb_background_color);

    // Render text to the video buffer.
    tig_font_push(tb_fonts[type]);
    tig_font_write(tb->video_buffer, str, &tb_content_rect, &dirty_rect);
    tig_font_pop();

    // Set up the text bubble properties.
    tb->timestamp = gamelib_ping_time;
    tb->duration = tb_text_duration;
    tb->flags = TEXT_BUBBLE_IN_USE;
    tb->obj = obj;
    tb->rect = dirty_rect;
    tb->pos = TB_POS_INVALID;

    // Mark the object as having a text bubble.
    object_flags_set(obj, OF_TEXT);

    // Invalidate the screen rect as dirty.
    tb_get_rect(tb, &dirty_rect);
    tb_iso_window_invalidate_rect(&dirty_rect);
}

/**
 * Sets the expiration time for a text bubble.
 *
 * Does nothing if the object have no associated text bubble.
 *
 * 0x4D6160
 */
void tb_expire_in(int64_t obj, int seconds)
{
    TextBubble* tb;

    // Find the text bubble for the object.
    tb = find_text_bubble(obj);
    if (tb == NULL) {
        return;
    }

    if (seconds == TB_EXPIRE_NEVER) {
        // Special case - mark text bubble as permanent.
        tb->flags |= TEXT_BUBBLE_PERMANENT;
    } else {
        // Update duration.
        tb->flags &= ~TEXT_BUBBLE_PERMANENT;
        tb->timestamp = gamelib_ping_time;
        tb->duration = (seconds >= 0) ? 1000 * seconds : tb_text_duration;
    }
}

/**
 * Called when the game object's position is about to change.
 *
 * 0x4D6210
 */
void tb_notify_moved(int64_t obj, int64_t loc, int offset_x, int offset_y)
{
    TextBubble* tb;
    TigRect rect;
    TigRect new_rect;

    // Find the text bubble for the object.
    tb = find_text_bubble(obj);
    if (tb == NULL) {
        return;
    }

    // Retrieve current screen rect (based on the current object's position).
    tb_get_rect(tb, &rect);

    // Calculate the new screen rect.
    tb_calc_rect(tb, loc, offset_x, offset_y, &new_rect);

    // Invalidate the combined area.
    tig_rect_union(&rect, &new_rect, &rect);
    tb_iso_window_invalidate_rect(&rect);
}

/**
 * Removes a text bubble associated with a game object.
 *
 * 0x4D62B0
 */
void tb_remove(int64_t obj)
{
    TextBubble* tb;
    unsigned int flags;

    // Find and remove the text bubble if it exists.
    tb = find_text_bubble(obj);
    if (tb != NULL) {
        tb_remove_internal(tb);
    } else {
        // Clear the text flag on the object to maintain consistency.
        flags = obj_field_int32_get(obj, OBJ_F_FLAGS);
        flags &= ~OF_TEXT;
        obj_field_int32_set(obj, OBJ_F_FLAGS, flags);
    }
}

/**
 * Clears all active text bubbles.
 *
 * 0x4D6320
 */
void tb_clear()
{
    int idx;

    for (idx = 0; idx < MAX_TEXT_BUBBLES; idx++) {
        if ((tb_text_bubbles[idx].flags & TEXT_BUBBLE_IN_USE) != 0) {
            tb_remove_internal(&(tb_text_bubbles[idx]));
        }
    }
}

/**
 * Internal function to remove a text bubble and clean up its resources.
 *
 * 0x4D6350
 */
void tb_remove_internal(TextBubble* tb)
{
    TigRect rect;
    unsigned int flags;

    // Invalidate the bubble's screen area.
    tb_get_rect(tb, &rect);
    tb_iso_window_invalidate_rect(&rect);

    // Clear the text flag on the associated object.
    flags = obj_field_int32_get(tb->obj, OBJ_F_FLAGS);
    flags &= ~OF_TEXT;
    obj_field_int32_set(tb->obj, OBJ_F_FLAGS, flags);

    // Reset the text bubble's properties.
    tb->flags = 0;
    tb->obj = OBJ_HANDLE_NULL;
}

/**
 * Retrieves the screen rectangle for a text bubble.
 *
 * 0x4D63B0
 */
void tb_get_rect(TextBubble* tb, TigRect* rect)
{
    int64_t loc;
    int offset_x;
    int offset_y;

    // Retrieve the object's position and offsets.
    loc = obj_field_int64_get(tb->obj, OBJ_F_LOCATION);
    offset_x = obj_field_int32_get(tb->obj, OBJ_F_OFFSET_X);
    offset_y = obj_field_int32_get(tb->obj, OBJ_F_OFFSET_Y);

    // Calculate the screen rectangle.
    tb_calc_rect(tb, loc, offset_x, offset_y, rect);
}

/**
 * Computes the text bubble's screen rectangle based on object location and
 * offsets.
 *
 * 0x4D6410
 */
void tb_calc_rect(TextBubble* tb, int64_t loc, int offset_x, int offset_y, TigRect* rect)
{
    int64_t x;
    int64_t y;
    int cell;
    int attempt;

    // Retrieve screen coordinates of the location.
    location_xy(loc, &x, &y);

    // Center it and offset by the given values.
    x += offset_x + 40;
    y += offset_y + 20;

    // Check for coordinate overflow and return an empty rectangle if invalid.
    if (x < INT_MIN
        || x > INT_MAX
        || y < INT_MIN
        || y > INT_MAX) {
        rect->x = 0;
        rect->y = 0;
        rect->width = 0;
        rect->height = 0;
        return;
    }

    // Set the rectangle's size based on the bubble's content.
    rect->width = tb->rect.width;
    rect->height = tb->rect.height;

    // Determine the bubble's position if not already set.
    if (tb->pos == TB_POS_INVALID) {
        tb->pos = TB_POS_TOP;

        // Calculate the screen cell based on coordinates.
        cell = 0;
        if (x >= 380) {
            if (x >= 420) {
                cell += 2;
            } else {
                cell += 1;
            }
        }

        if (y >= 190) {
            if (y >= 290) {
                cell += 6;
            } else {
                cell += 3;
            }
        }

        // Try different positions until one fits within the content rectangle.
        for (attempt = 0; attempt < TB_POS_COUNT; attempt++) {
            rect->x = (int)x;
            rect->y = (int)y;

            // Adjust the rectangle based on the current position attempt.
            adjust_text_bubble_rect(rect, tb_placement_tbl[cell][attempt]);

            // Check if the adjusted rectangle fits within the content area.
            if (rect->x >= tb_iso_content_rect.x
                && rect->y >= tb_iso_content_rect.y
                && rect->x + rect->width <= tb_iso_content_rect.x + tb_iso_content_rect.width
                && rect->y + rect->height <= tb_iso_content_rect.y + tb_iso_content_rect.height) {
                tb->pos = tb_placement_tbl[cell][attempt];
                break;
            }
        }
    }

    // Set the base position and apply the final adjustment.
    rect->x = (int)x;
    rect->y = (int)y;

    adjust_text_bubble_rect(rect, tb->pos);
}

/**
 * Called when `text duration` setting is changed.
 *
 * 0x4D67F0
 */
void tb_text_duration_changed()
{
    int index;

    // Calculate the new duration based on the setting (scaled to milliseconds).
    tb_text_duration = settings_get_value(&settings, TEXT_DURATION_KEY) * 1000;

    // Update the duration of all non-permanent active bubbles.
    for (index = 0; index < MAX_TEXT_BUBBLES; index++) {
        if ((tb_text_bubbles[index].flags & TEXT_BUBBLE_IN_USE) != 0
            && (tb_text_bubbles[index].flags & TEXT_BUBBLE_PERMANENT) == 0) {
            // FIX: There is an error in the original code which additionally
            // multiplies this value by 1000 (effectively scaling it to
            // microseconds).
            tb_text_bubbles[index].duration = tb_text_duration;
        }
    }
}

/**
 * Finds an existing text bubble associated with a given object.
 *
 * Returns `NULL` if object does not have an active text bubble.
 */
TextBubble* find_text_bubble(int64_t obj)
{
    int idx;

    for (idx = 0; idx < MAX_TEXT_BUBBLES; idx++) {
        if ((tb_text_bubbles[idx].flags & TEXT_BUBBLE_IN_USE) != 0
            && tb_text_bubbles[idx].obj == obj) {
            return &(tb_text_bubbles[idx]);
        }
    }

    return NULL;
}

/**
 * Finds a free text bubble slot or reuses an existing one for a given object.
 *
 * If no free slot exists, replaces the oldest non-permanent bubble or an
 * existing bubble for the same object.
 *
 * Returns `NULL` if no slot is available.
 */
TextBubble* find_free_text_bubble(int64_t obj)
{
    int idx;
    int idx_to_remove = -1;
    int idx_to_use = -1;

    for (idx = 0; idx < MAX_TEXT_BUBBLES; idx++) {
        if ((tb_text_bubbles[idx].flags & TEXT_BUBBLE_IN_USE) != 0) {
            // If the object already has a bubble, reuse it. This prevents one
            // object to have several text bubbles.
            if (tb_text_bubbles[idx].obj != OBJ_HANDLE_NULL
                && tb_text_bubbles[idx].obj == obj) {
                idx_to_remove = idx;
                idx_to_use = -1;
                break;
            }

            // Track the oldest non-permanent bubble as a fallback for
            // replacement.
            if (idx_to_remove == -1
                || ((tb_text_bubbles[idx].flags & TEXT_BUBBLE_PERMANENT) == 0
                    && tb_text_bubbles[idx].timestamp < tb_text_bubbles[idx_to_remove].timestamp)) {
                idx_to_remove = idx;
            }
        } else {
            // Mark an unused slot for use. This is still a candidate because
            // we have to check all slots in case the specified object already
            // have an active bubble.
            idx_to_use = idx;
        }
    }

    // If no free slot was found, replace an existing bubble.
    if (idx_to_use == -1) {
        if (idx_to_remove == -1) {
            return NULL;
        }

        // Remove the selected bubble to free its slot.
        tb_remove_internal(&(tb_text_bubbles[idx_to_remove]));
        idx_to_use = idx_to_remove;
    }

    return &(tb_text_bubbles[idx_to_use]);
}

/**
 * Adjusts a text bubble's rect based on its position relative to the object.
 */
void adjust_text_bubble_rect(TigRect* rect, TbPosition pos)
{
    switch (pos) {
    case TB_POS_TOP:
        rect->x -= rect->width / 2;
        rect->y -= rect->height + 100;
        break;
    case TB_POS_TOP_RIGHT:
        rect->x += 40;
        rect->y -= rect->height / 2 + 55;
        break;
    case TB_POS_RIGHT:
        rect->x += 80;
        rect->y -= rect->height / 2;
        break;
    case TB_POS_BOTTOM_RIGHT:
        rect->x += 40;
        rect->y += rect->height / 2 + 20;
        break;
    case TB_POS_BOTTOM:
        rect->x -= rect->width / 2;
        rect->y += 10;
        break;
    case TB_POS_BOTTOM_LEFT:
        rect->x -= rect->width + 40;
        rect->y += rect->height / 2 + 20;
        break;
    case TB_POS_LEFT:
        rect->x -= rect->width + 80;
        rect->y -= rect->height / 2;
        break;
    case TB_POS_TOP_LEFT:
        rect->x -= rect->width + 40;
        rect->y -= rect->height / 2 + 55;
        break;
    default:
        break;
    }
}
