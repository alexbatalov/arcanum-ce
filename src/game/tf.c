#include "game/tf.h"

#include "game/gamelib.h"
#include "game/object.h"

/**
 * The width of text floater rectangle.
 */
#define TEXT_FLOATER_WIDTH 200

/**
 * The number of lines that a text floater box holds.
 */
#define NUMBER_OF_LINES 5

/**
 * Represents a single text floater entry (line).
 */
typedef struct TextFloaterEntry {
    /* 0000 */ int offset;
    /* 0004 */ TigVideoBuffer* video_buffer;
    /* 0008 */ TigRect rect;
    /* 0018 */ uint8_t opacity;
    /* 001C */ struct TextFloaterEntry* next;
} TextFloaterEntry;

/**
 * Represents a text floater "box" associated with a game object.
 */
typedef struct TextFloaterList {
    /* 0000 */ int64_t obj;
    /* 0008 */ TextFloaterEntry* head;
    /* 000C */ struct TextFloaterList* next;
} TextFloaterList;

static void tf_clear(void);
static TextFloaterList* tf_list_create(void);
static void tf_list_destroy(TextFloaterList* list);
static void tf_list_free(TextFloaterList* node);
static void tf_list_get_rect(TextFloaterList* node, TigRect* rect);
static void tf_calc_rect(int64_t loc, int offset_x, int offset_y, TigRect* rect);
static TextFloaterEntry* tf_entry_create(void);
static void tf_entry_free(TextFloaterEntry* entry);
static void tf_entry_destroy(TextFloaterList* list, TextFloaterEntry* entry);
static void tf_entry_get_rect(TextFloaterList* list, TextFloaterEntry* entry, TigRect* entry_rect);
static void tf_entry_get_rect_constrained_to(TigRect* list_rect, TextFloaterEntry* entry, TigRect* entry_rect);
static void tf_entry_recalc_opacity(TextFloaterEntry* entry);
static void tf_level_set_internal(int value);
static void tf_level_changed(void);
static void tf_float_speed_changed(void);

/**
 * Color values (RGB) for text floater types.
 *
 * 0x5B8E6C
 */
static uint8_t tf_colors[TF_TYPE_COUNT][3] = {
    /*  TF_TYPE_WHITE */ { 255, 255, 255 },
    /*    TF_TYPE_RED */ { 255, 0, 0 },
    /*  TF_TYPE_GREEN */ { 0, 255, 0 },
    /*   TF_TYPE_BLUE */ { 64, 64, 255 },
    /* TF_TYPE_YELLOW */ { 255, 255, 0 },
};

/**
 * Line height of text floater entry.
 *
 * 0x602898
 */
static int tf_line_height;

/**
 * A pool of the freed `TextFloaterList` objects for reuse.
 *
 * 0x60289C
 */
static TextFloaterList* tf_free_lists_head;

/**
 * The speed (in pixels per update, which is 50ms) at which text floaters move
 * through a vertical range.
 *
 * 0x6028A0
 */
static int tf_float_speed;

/**
 * Editor view options.
 *
 * 0x6028A8
 */
static ViewOptions text_floater_view_options;

/**
 * Parent window bounds.
 *
 * 0x6028B0
 */
static TigRect tf_iso_content_rect;

/**
 * 0x6028C0
 */
static tig_color_t dword_6028C0;

/**
 * Function pointer to invalidate a rectangle in the parent window.
 *
 * 0x6028C4
 */
static IsoInvalidateRectFunc* tf_iso_invalidate_rect;

/**
 * Background color for text floater.
 *
 * 0x6028C8
 */
static tig_color_t tf_background_color;

/**
 * Bounds of the text floater entry (in it's own coordinate system).
 *
 * 0x6028D0
 */
static TigRect tf_entry_content_rect;

/**
 * Head of the active text floaters.
 *
 * 0x6028E0
 */
TextFloaterList* tf_list_head;

/**
 * Timestamp of the last update.
 *
 * This value is used to throttle updates.
 *
 * 0x6028E4
 */
tig_timestamp_t tf_ping_timestamp;

/**
 * Handle to the parent window.
 *
 * 0x6028E8
 */
static tig_window_handle_t tf_iso_window_handle;

/**
 * 0x6028EC
 */
static int tf_level;

/**
 * A pool of the freed `TextFloaterEntry` objects for reuse.
 *
 * 0x6028F0
 */
static TextFloaterEntry* tf_free_entries_head;

/**
 * Bounds of the text floater list (in it's own coordinate system).
 *
 * 0x6028F8
 */
static TigRect tf_list_content_rect;

// 0x602908
static tig_font_handle_t tf_fonts[TF_TYPE_COUNT];

/**
 * Called when the game is initialized.
 *
 * 0x4D4E20
 */
bool tf_init(GameInitInfo* init_info)
{
    TigWindowData window_data;
    TigFont font_desc;
    TigArtFrameData art_frame_data;
    int index;

    // Retrieve window data to set up content rectangle.
    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    tf_iso_window_handle = init_info->iso_window_handle;
    tf_iso_invalidate_rect = init_info->invalidate_rect_func;

    tf_iso_content_rect.x = 0;
    tf_iso_content_rect.y = 0;
    tf_iso_content_rect.width = window_data.rect.width;
    tf_iso_content_rect.height = window_data.rect.height;

    text_floater_view_options.type = VIEW_TYPE_ISOMETRIC;

    tf_background_color = tig_color_make(0, 0, 255);
    dword_6028C0 = tig_color_make(0, 0, 0);

    // Set up font creation parameters.
    font_desc.flags = TIG_FONT_NO_ALPHA_BLEND | TIG_FONT_SHADOW;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_desc.art_id));

    // Determine line height from font metrics.
    tig_art_frame_data(font_desc.art_id, &art_frame_data);
    tf_line_height = art_frame_data.height + 2;

    // Create fonts for each text floater type with appropriate colors.
    for (index = 0; index < TF_TYPE_COUNT; index++) {
        font_desc.color = tig_color_make(tf_colors[index][0], tf_colors[index][1], tf_colors[index][2]);
        tig_font_create(&font_desc, &(tf_fonts[index]));
    }

    // Set up content rect for a text floater list.
    tf_list_content_rect.x = 0;
    tf_list_content_rect.y = 0;
    tf_list_content_rect.width = TEXT_FLOATER_WIDTH;
    tf_list_content_rect.height = NUMBER_OF_LINES * tf_line_height;

    // Set up content rect for a text floater entry.
    tf_entry_content_rect.x = 0;
    tf_entry_content_rect.y = 0;
    tf_entry_content_rect.width = TEXT_FLOATER_WIDTH;
    tf_entry_content_rect.height = tf_line_height;

    // Register `text floaters` setting and initialize it.
    settings_register(&settings, TEXT_FLOATERS_KEY, "1", tf_level_changed);
    tf_level_changed();

    // Register `float speed` setting and initialize it.
    settings_register(&settings, FLOAT_SPEED_KEY, "2", tf_float_speed_changed);
    tf_float_speed_changed();

    return true;
}

/**
 * Called when the window size has changed.
 *
 * 0x4D5050
 */
void tf_resize(GameResizeInfo* resize_info)
{
    tf_iso_content_rect = resize_info->content_rect;
    tf_iso_window_handle = resize_info->window_handle;
}

/**
 * Called when the game is being reset.
 *
 * 0x4D5090
 */
void tf_reset(void)
{
    tf_clear();
}

/**
 * Called when the game shuts down.
 *
 * 0x4D5DC0
 */
void tf_exit(void)
{
    int index;
    TextFloaterEntry* next_node;
    TextFloaterList* next_list;

    // Clear all active floaters.
    tf_clear();

    // Destroy fonts for each text floater type.
    for (index = 0; index < TF_TYPE_COUNT; index++) {
        tig_font_destroy(tf_fonts[index]);
    }

    // Free all entries in the free pool.
    while (tf_free_entries_head != NULL) {
        next_node = tf_free_entries_head->next;
        tig_video_buffer_destroy(tf_free_entries_head->video_buffer);
        FREE(tf_free_entries_head);
        tf_free_entries_head = next_node;
    }

    // Free all lists in the free pool.
    while (tf_free_lists_head != NULL) {
        next_list = tf_free_lists_head->next;
        FREE(tf_free_lists_head);
        tf_free_lists_head = next_list;
    }

    tf_iso_window_handle = TIG_WINDOW_HANDLE_INVALID;
    tf_iso_invalidate_rect = NULL;
}

/**
 * Called when view settings have changed.
 *
 * 0x4D5130
 */
void tf_update_view(ViewOptions* view_options)
{
    text_floater_view_options = *view_options;
}

/**
 * Called when the map is closed.
 *
 * 0x4D5150
 */
void tf_map_close(void)
{
    tf_clear();
}

/**
 * Sets the visibility level for text floaters.
 *
 * Returns new visibility level.
 *
 * 0x4D5160
 */
int tf_level_set(int value)
{
    tf_level_set_internal(value);
    return tf_level;
}

/**
 * Retrieves the current visibility level for text floaters.
 *
 * 0x4D5180
 */
int tf_level_get(void)
{
    return tf_level;
}

/**
 * Called every frame.
 *
 * 0x4D5190
 */
void tf_ping(tig_timestamp_t timestamp)
{
    TextFloaterList* list;
    TextFloaterList* prev_list;
    TextFloaterEntry* entry;
    TextFloaterEntry* prev_entry;
    TigRect list_rect;
    TigRect entry_rect;
    unsigned int flags;

    // Throttle updates to every 50ms.
    if (tig_timer_between(tf_ping_timestamp, timestamp) < 50) {
        return;
    }

    tf_ping_timestamp = timestamp;

    // Iterate through all text floater lists.
    prev_list = NULL;
    list = tf_list_head;
    while (list != NULL) {
        tf_list_get_rect(list, &list_rect);

        // Process each entry in the current list.
        prev_entry = NULL;
        entry = list->head;
        while (entry != NULL) {
            // Invalidate the current screen area before moving.
            tf_entry_get_rect_constrained_to(&list_rect, entry, &entry_rect);
            if (tig_rect_intersection(&entry_rect, &list_rect, &entry_rect) == TIG_OK) {
                tf_iso_invalidate_rect(&entry_rect);
            }

            // Move the entry upward.
            entry->offset -= tf_float_speed;

            // Check if the entry is visible.
            if (entry->offset >= 0) {
                // Obtain the new entry rectangle, update opacity based on the
                // current offset, and mark this new rect as dirty.
                tf_entry_get_rect_constrained_to(&list_rect, entry, &entry_rect);
                if (tig_rect_intersection(&entry_rect, &list_rect, &entry_rect) == TIG_OK) {
                    tf_entry_recalc_opacity(entry);
                    tf_iso_invalidate_rect(&entry_rect);
                }

                // Continue with the next entry and keep note of previous to
                // maintain linked list on the next iteration.
                prev_entry = entry;
                entry = entry->next;
            } else {
                // The entry is no longer visible, remove it from the list.
                if (prev_entry != NULL) {
                    prev_entry->next = entry->next;
                } else {
                    list->head = entry->next;
                }

                tf_entry_free(entry);

                // Continue with next entry. Current entry is gone, previous
                // entry doesn't change.
                if (prev_entry != NULL) {
                    entry = prev_entry->next;
                } else {
                    entry = list->head;
                }
            }
        }

        // Check if the list is not empty after processing entries.
        if (list->head != NULL) {
            // Continue with next list and keep note of previous to maintain
            // linked list on the next iteration.
            prev_list = list;
            list = list->next;
        } else {
            // The list is empty, remove it and update links.
            if (prev_list != NULL) {
                // TODO: Check if `OF_TEXT_FLOATER` should be cleared here as well.
                prev_list->next = list->next;
                tf_list_destroy(list);
                list = prev_list->next;
            } else {
                // Clear the text floater flag on the associated object.
                flags = obj_field_int32_get(list->obj, OBJ_F_FLAGS);
                flags &= ~OF_TEXT_FLOATER;
                obj_field_int32_set(list->obj, OBJ_F_FLAGS, flags);

                tf_list_head = list->next;
                tf_list_destroy(list);
                list = tf_list_head;
            }
        }
    }
}

/**
 * Renders all active text floaters to the window.
 *
 * 0x4D5310
 */
void tf_draw(GameDrawInfo* draw_info)
{
    TextFloaterList* list;
    TextFloaterEntry* entry;
    TigWindowBlitInfo window_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect list_rect;
    TigRect entry_rect;
    TigRectListNode* rect_node;
    TigRect dirty_list_rect;

    // Ensure text floaters are enabled.
    if (tf_level == TF_LEVEL_NEVER) {
        return;
    }

    // Ensure we're in isometric view. The text floater module is not supposed
    // to work in the editor.
    if (text_floater_view_options.type != VIEW_TYPE_ISOMETRIC) {
        return;
    }

    // Set up blit parameters with alpha blending.
    window_blit_info.type = TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW;
    window_blit_info.vb_blit_flags = TIG_VIDEO_BUFFER_BLIT_BLEND_ALPHA_CONST;
    window_blit_info.src_rect = &src_rect;
    window_blit_info.dst_rect = &dst_rect;
    window_blit_info.dst_window_handle = tf_iso_window_handle;

    // Iterate through all text floater lists.
    for (list = tf_list_head; list != NULL; list = list->next) {
        tf_list_get_rect(list, &list_rect);

        // Iterate through dirty rectangles.
        for (rect_node = *draw_info->rects; rect_node != NULL; rect_node = rect_node->next) {
            // Check for intersection with list rectangle.
            if (tig_rect_intersection(&(rect_node->rect), &list_rect, &dirty_list_rect) != TIG_OK) {
                continue;
            }

            // Iterate through all entries.
            for (entry = list->head; entry != NULL; entry = entry->next) {
                tf_entry_get_rect_constrained_to(&list_rect, entry, &entry_rect);

                // Check for entry intersection with dirty portion of the list
                // rectangle.
                if (tig_rect_intersection(&entry_rect, &dirty_list_rect, &dst_rect) != TIG_OK) {
                    continue;
                }

                // Calculate source rectangle for blitting.
                src_rect.x = dst_rect.x - entry_rect.x;
                src_rect.y = dst_rect.y - entry_rect.y;
                src_rect.width = dst_rect.width;
                src_rect.height = dst_rect.height;

                window_blit_info.src_video_buffer = entry->video_buffer;
                window_blit_info.alpha[0] = entry->opacity;

                // Blit the affected entry rect to the window.
                tig_window_blit(&window_blit_info);
            }
        }
    }
}

/**
 * Adds a new text floater for a game object.
 *
 * 0x4D5450
 */
void tf_add(int64_t obj, int type, const char* str)
{
    TextFloaterList* list;
    TextFloaterList* prev_list;
    TextFloaterEntry* entry;
    TextFloaterEntry* prev_entry;
    TigRect rect;

    // Find or create a text floater list for the object.
    prev_list = NULL;
    list = tf_list_head;
    while (list != NULL) {
        if (list->obj == obj) {
            break;
        }
        prev_list = list;
        list = list->next;
    }

    if (list == NULL) {
        list = tf_list_create();
        list->obj = obj;
    }

    if (prev_list != NULL) {
        prev_list->next = list;
    } else {
        tf_list_head = list;
    }

    // Find the last entry in the list.
    prev_entry = NULL;
    entry = list->head;
    while (entry != NULL) {
        prev_entry = entry;
        entry = entry->next;
    }

    // Create a new entry and position it at the bottom of the list.
    entry = tf_entry_create();
    entry->offset = 4 * tf_line_height;

    if (prev_entry != NULL) {
        prev_entry->next = entry;
        if (prev_entry->offset > 3 * tf_line_height) {
            entry->offset = prev_entry->offset + tf_line_height;
        }
    } else {
        list->head = entry;
    }

    // Reset video buffer with the background color.
    tig_video_buffer_fill(entry->video_buffer, &tf_entry_content_rect, tf_background_color);

    // Render text to the video buffer.
    tig_font_push(tf_fonts[type]);
    tig_font_write(entry->video_buffer, str, &tf_entry_content_rect, &(entry->rect));
    tig_font_pop();

    // Mark the object as having a text floater.
    object_flags_set(obj, OF_TEXT_FLOATER);

    // Update opacity.
    tf_entry_recalc_opacity(entry);

    // Invalidate the screen rect as dirty.
    tf_entry_get_rect(list, entry, &rect);
    tf_iso_invalidate_rect(&rect);
}

/**
 * Called when the game object's position is about to change.
 *
 * 0x4D5570
 */
void tf_notify_moved(int64_t obj, int64_t loc, int offset_x, int offset_y)
{
    TextFloaterList* list;
    TextFloaterEntry* entry;
    TigRect old_list_rect;
    TigRect new_list_rect;
    TigRect entry_rect;

    // Find the text floater list for the object.
    list = tf_list_head;
    while (list != NULL) {
        if (list->obj == obj) {
            break;
        }
        list = list->next;
    }

    if (list == NULL) {
        return;
    }

    // Retrieve current screen rect (based on the current object's position).
    tf_list_get_rect(list, &old_list_rect);

    // Calculate the new screen rect.
    tf_calc_rect(loc, offset_x, offset_y, &new_list_rect);

    // Invalidate screen areas for each entry in both positions.
    entry = list->head;
    while (entry != NULL) {
        tf_entry_get_rect_constrained_to(&old_list_rect, entry, &entry_rect);
        tf_iso_invalidate_rect(&entry_rect);

        tf_entry_get_rect_constrained_to(&new_list_rect, entry, &entry_rect);
        tf_iso_invalidate_rect(&entry_rect);

        entry = entry->next;
    }
}

/**
 * Called then the game object is killed.
 *
 * 0x4D5620
 */
void tf_notify_killed(int64_t obj)
{
    TextFloaterList* list;
    TextFloaterEntry* entry;
    TextFloaterEntry* prev_entry;
    TextFloaterEntry* next_entry;

    // Check if the object has active text floaters.
    if ((obj_field_int32_get(obj, OBJ_F_FLAGS) & OF_TEXT_FLOATER) == 0) {
        return;
    }

    // Find the text floater list for the object.
    list = tf_list_head;
    while (list != NULL) {
        if (list->obj == obj) {
            break;
        }
        list = list->next;
    }

    if (list == NULL) {
        return;
    }

    // Find the first queued entry outside the list content rect (this is the
    // first entry which is not yet visible).
    prev_entry = NULL;
    entry = list->head;
    while (entry != NULL) {
        if (entry->offset >= tf_list_content_rect.height) {
            break;
        }
        prev_entry = entry;
        entry = entry->next;
    }

    if (entry == NULL) {
        // There are no enqueued entries. Let the currently visible entries
        // fade to nothing as usual (in `tf_ping`).
        return;
    }

    if (prev_entry != NULL) {
        // Detach subsequent entries from the last visible entry.
        prev_entry->next = NULL;

        // Destroy all detached nodes.
        while (entry != NULL) {
            next_entry = entry->next;
            tf_entry_free(entry);
            entry = next_entry;
        }
    } else {
        // Somehow the list is not empty, but don't have any currently visible
        // entry.
        tf_list_destroy(list);
    }
}

/**
 * Removes all text floaters associated with a game object.
 *
 * 0x4D56C0
 */
void tf_remove(int64_t obj)
{
    unsigned int flags;
    TextFloaterList* prev;
    TextFloaterList* node;
    TigRect rect;

    flags = obj_field_int32_get(obj, OBJ_F_FLAGS);

    // Check if the object has active text floaters.
    if ((flags & OF_TEXT_FLOATER) == 0) {
        return;
    }

    // Clear the text floater flag.
    flags &= ~OF_TEXT_FLOATER;
    obj_field_int32_set(obj, OBJ_F_FLAGS, flags);

    // Find the text floater list for the object.
    prev = NULL;
    node = tf_list_head;
    while (node != NULL) {
        if (node->obj == obj) {
            break;
        }
        prev = node;
        node = node->next;
    }

    if (node == NULL) {
        return;
    }

    // Mark appropriate screen area as dirty.
    tf_list_get_rect(node, &rect);
    tf_iso_invalidate_rect(&rect);

    // Update links.
    if (prev != NULL) {
        prev->next = node->next;
    } else {
        tf_list_head = node->next;
    }

    // Free list.
    tf_list_destroy(node);
}

/**
 * Clears all active text floaters.
 *
 * 0x4D5780
 */
void tf_clear(void)
{
    TextFloaterList* list;
    TextFloaterList* next;
    TigRect rect;

    list = tf_list_head;
    while (list != NULL) {
        next = list->next;

        tf_list_get_rect(list, &rect);
        tf_iso_invalidate_rect(&rect);

        tf_list_destroy(list);
        list = next;
    }
    tf_list_head = NULL;
}

/**
 * Creates a new text floater list, reusing from the free pool if available.
 *
 * 0x4D57E0
 */
TextFloaterList* tf_list_create(void)
{
    TextFloaterList* list;

    if (tf_free_lists_head != NULL) {
        list = tf_free_lists_head;
        tf_free_lists_head = list->next;
    } else {
        list = (TextFloaterList*)MALLOC(sizeof(*list));
    }

    list->obj = OBJ_HANDLE_NULL;
    list->head = NULL;
    list->next = NULL;

    return list;
}

/**
 * Destroys a text floater list and its entries.
 *
 * 0x4D5820
 */
void tf_list_destroy(TextFloaterList* list)
{
    TextFloaterEntry* curr;
    TextFloaterEntry* next;

    // Destroy all entries in the list.
    curr = list->head;
    while (curr != NULL) {
        next = curr->next;
        tf_entry_destroy(list, curr);
        curr = next;
    }

    // Free the list itself.
    tf_list_free(list);
}

/**
 * Frees a text floater list by adding it to the free pool.
 *
 * 0x4D5850
 */
void tf_list_free(TextFloaterList* node)
{
    node->next = tf_free_lists_head;
    tf_free_lists_head = node;
}

/**
 * Retrieves the screen rectangle for a text floater list.
 *
 * 0x4D5870
 */
void tf_list_get_rect(TextFloaterList* node, TigRect* rect)
{
    int64_t loc;
    int offset_x;
    int offset_y;

    // Retrieve the object's position and offsets.
    loc = obj_field_int64_get(node->obj, OBJ_F_LOCATION);
    offset_x = obj_field_int32_get(node->obj, OBJ_F_OFFSET_X);
    offset_y = obj_field_int32_get(node->obj, OBJ_F_OFFSET_Y);

    // Calculate the screen rectangle.
    tf_calc_rect(loc, offset_x, offset_y, rect);
}

/**
 * Computes the text floater's screen rectangle based on object location and
 * offsets.
 *
 * 0x4D58C0
 */
void tf_calc_rect(int64_t loc, int offset_x, int offset_y, TigRect* rect)
{
    int64_t sx;
    int64_t sy;

    // Retrieve screen coordinates of the location.
    location_xy(loc, &sx, &sy);

    // Center it and offset by the given values.
    sx += offset_x + 40;
    sy += offset_y + 20;

    // Center horizontally.
    sx -= TEXT_FLOATER_WIDTH / 2;

    // Adjust vertical position.
    //
    // NOTE: The 90 pixels is probably the average height of critter sprites.
    // This adjustment makes text floater box end up right above critter head.
    sy -= NUMBER_OF_LINES * tf_line_height + 90;

    rect->x = (int)sx;
    rect->y = (int)sy;
    rect->width = tf_list_content_rect.width;
    rect->height = tf_list_content_rect.height;
}

/**
 * Creates a new text floater entry with a video buffer.
 *
 * 0x4D5950
 */
TextFloaterEntry* tf_entry_create(void)
{
    TextFloaterEntry* entry;
    TigVideoBufferCreateInfo vb_create_info;

    // Reuse from free pool if available.
    if (tf_free_entries_head != NULL) {
        entry = tf_free_entries_head;
        tf_free_entries_head = entry->next;

        entry->next = NULL;

        return entry;
    }

    // Allocate new entry.
    entry = (TextFloaterEntry*)MALLOC(sizeof(*entry));
    entry->next = NULL;

    // Create a video buffer.
    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_COLOR_KEY | TIG_VIDEO_BUFFER_CREATE_VIDEO_MEMORY;
    vb_create_info.width = TEXT_FLOATER_WIDTH;
    vb_create_info.height = tf_line_height;
    vb_create_info.background_color = tf_background_color;
    vb_create_info.color_key = tf_background_color;
    tig_video_buffer_create(&vb_create_info, &(entry->video_buffer));

    return entry;
}

/**
 * Frees a text floater entry by adding it to the free pool.
 *
 * 0x4D59D0
 */
void tf_entry_free(TextFloaterEntry* entry)
{
    entry->next = tf_free_entries_head;
    tf_free_entries_head = entry;
}

/**
 * Destroys a text floater entry, freeing its resources.
 *
 * 0x4D59F0
 */
void tf_entry_destroy(TextFloaterList* list, TextFloaterEntry* entry)
{
    TigRect rect;

    tf_entry_get_rect(list, entry, &rect);
    tf_iso_invalidate_rect(&rect);

    tf_entry_free(entry);
}

/**
 * Retrieves the screen rectangle for a text floater entry.
 *
 * 0x4D5A30
 */
void tf_entry_get_rect(TextFloaterList* list, TextFloaterEntry* entry, TigRect* entry_rect)
{
    TigRect rect;

    tf_list_get_rect(list, &rect);
    tf_entry_get_rect_constrained_to(&rect, entry, entry_rect);
}

/**
 * Calculates the screen rectangle for a text floater entry relative to its
 * list.
 *
 * 0x4D5A60
 */
void tf_entry_get_rect_constrained_to(TigRect* list_rect, TextFloaterEntry* entry, TigRect* entry_rect)
{
    entry_rect->x = list_rect->x + (list_rect->width - entry->rect.width) / 2;
    entry_rect->y = list_rect->y + entry->offset;
    entry_rect->width = entry->rect.width;
    entry_rect->height = entry->rect.height;
}

/**
 * Updates the opacity of a text floater entry based on its vertical position.
 *
 * This function implements a fade-in/fade-out effect as the entry moves through
 * a vertical range. The opacity is 255 (opaque) in the middle range, fading
 * to 0 (transparent) at the edges.
 *
 * 0x4D5AA0
 */
void tf_entry_recalc_opacity(TextFloaterEntry* entry)
{
    int mid;
    int pos;

    // Calculate the middle point and current position.
    mid = tf_line_height / 2;
    pos = entry->offset + mid;
    if (pos > ((NUMBER_OF_LINES / 2) + 1) * tf_line_height) {
        pos = NUMBER_OF_LINES * tf_line_height - pos;
    }

    // Set opacity based on position within the vertical range.
    if (pos > (NUMBER_OF_LINES / 2) * tf_line_height) {
        entry->opacity = 255;
    } else if (pos > mid) {
        entry->opacity = (uint8_t)((float)(pos - mid) / ((NUMBER_OF_LINES / 2) * tf_line_height - mid) * 255.0f);
    } else {
        entry->opacity = 0;
    }
}

/**
 * Internal helper function to set text floaters level of details.
 *
 * This function does nothing if the specified value is out of range.
 *
 * 0x4D5B10
 */
void tf_level_set_internal(int value)
{
    if (!(value >= 0 && value < TF_LEVEL_COUNT)) {
        return;
    }

    settings_set_value(&settings, TEXT_FLOATERS_KEY, value);
}

/**
 * Called when `text floaters` setting is changed.
 *
 * 0x4D5B40
 */
void tf_level_changed(void)
{
    tf_level = settings_get_value(&settings, TEXT_FLOATERS_KEY);
}

/**
 * Called when `float speed` setting is changed.
 *
 * 0x4D5B60
 */
void tf_float_speed_changed(void)
{
    tf_float_speed = settings_get_value(&settings, FLOAT_SPEED_KEY) + 1;
}
