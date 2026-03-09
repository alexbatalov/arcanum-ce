#include "game/jumppoint.h"

#include <stdio.h>

#include "game/location.h"

static void jumppoint_invalidate(int jumppoint);
static void jumppoint_get_rect(int jumppoint, TigRect* rect);
static bool jumppoint_read_all(TigFile* stream);
static bool jumppoint_write_all(TigFile* stream);

/**
 * Save (current) path for jumppoint file.
 *
 * 0x603450
 */
static char jumppoint_save_path[TIG_MAX_PATH];

/**
 * Jumppoints.
 *
 * 0x603554
 */
static JumpPoint* jumppoints;

/**
 * Art ID for the top-down view jumppoint graphic.
 *
 * 0x603558
 */
static tig_art_id_t jumppoint_td_art_id;

/**
 * Function pointer to invalidate a rectangle in the parent window.
 *
 * 0x60355C
 */
static IsoInvalidateRectFunc* jumppoint_iso_invalidate_rect;

/**
 * Base (template) path for jumppoint file.
 *
 * 0x603560
 */
static char jumppoint_base_path[TIG_MAX_PATH];

/**
 * Editor view options.
 *
 * 0x603668
 */
static ViewOptions jumppoint_view_options;

/**
 * Flag indicating whether jumppoint rendering is enabled.
 *
 * 0x603670
 */
static bool jumppoint_enabled;

/**
 * Handle to the parent window.
 *
 * 0x603674
 */
static tig_window_handle_t jumppoint_iso_window_handle;

/**
 * Art ID for the isometric view jumppoint graphic.
 *
 * 0x603678
 */
static tig_art_id_t jumppoint_iso_art_id;

/**
 * Flag indicating whether jumppoint data has been modified since the last save.
 *
 * 0x60367C
 */
static bool jumppoint_modified;

/**
 * Flag indicating whether the jumppoint module has been initialized.
 *
 * 0x603680
 */
static bool jumppoint_initialized;

/**
 * Number of jumppoints currently stored in the jumppoints array.
 *
 * 0x603684
 */
static int jumppoints_count;

/**
 * Flag indicating whether the module is in editor mode.
 *
 * 0x603688
 */
static bool jumppoint_editor;

/**
 * Called when the game is initialized.
 *
 * 0x4E2FB0
 */
bool jumppoint_init(GameInitInfo* init_info)
{
    jumppoint_iso_window_handle = init_info->iso_window_handle;
    jumppoint_iso_invalidate_rect = init_info->invalidate_rect_func;
    jumppoint_editor = init_info->editor;
    jumppoint_view_options.type = VIEW_TYPE_ISOMETRIC;

    // Initialize art IDs for isometric and top-down views. These only make
    // sense in the editor.
    tig_art_interface_id_create(350, 0, 0, 0, &jumppoint_iso_art_id);
    tig_art_interface_id_create(351, 0, 0, 0, &jumppoint_td_art_id);

    jumppoint_initialized = true;
    jumppoint_modified = false;
    jumppoint_enabled = true;

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4E3020
 */
void jumppoint_reset(void)
{
    jumppoint_map_close();
}

/**
 * Called when the game shuts down.
 *
 * 0x4E3030
 */
void jumppoint_exit(void)
{
    jumppoint_map_close();
    jumppoint_initialized = false;
}

/**
 * Called when the window size has changed.
 *
 * 0x4E3040
 */
void jumppoint_resize(GameResizeInfo* resize_info)
{
    jumppoint_iso_window_handle = resize_info->window_handle;
}

/**
 * Initializes jumppoint data for a new map.
 *
 * 0x4E3050
 */
bool jumppoint_map_new(MapNewInfo* new_map_info)
{
    // Clear existing jumppoint data.
    jumppoint_map_close();

    // Prepare file paths.
    sprintf(jumppoint_base_path, "%s\\map.jmp", new_map_info->base_path);
    sprintf(jumppoint_save_path, "%s\\map.jmp", new_map_info->save_path);

    // Mark data as modified so that subsequent flush creates empty `map.jmp`.
    jumppoint_modified = true;

    return jumppoint_flush();
}

/**
 * Loads jumppoint data.
 *
 * 0x4E30A0
 */
bool jumppoint_open(const char* base_path, const char* save_path)
{
    TigFile* stream;

    // Clear existing jumppoint data.
    jumppoint_map_close();

    // Prepare file paths.
    sprintf(jumppoint_base_path, "%s\\map.jmp", base_path);
    sprintf(jumppoint_save_path, "%s\\map.jmp", save_path);

    // Attempt to open the save game path.
    stream = tig_file_fopen(jumppoint_save_path, "rb");
    if (stream == NULL) {
        // Fallback to base (template) path
        stream = tig_file_fopen(jumppoint_base_path, "rb");
        if (stream == NULL) {
            tig_debug_printf("Jumppoint file doesn't exist [%s]\n", jumppoint_save_path);
            return false;
        }
    }

    // Read jumppoint data from the file.
    if (!jumppoint_read_all(stream)) {
        // FIX: Resource leak.
        tig_file_fclose(stream);
        return false;
    }

    jumppoint_modified = false;
    tig_file_fclose(stream);

    return true;
}

/**
 * Called when the map is closed.
 *
 * 0x4E3140
 */
void jumppoint_map_close(void)
{
    // Free the jumppoints array.
    if (jumppoints != NULL) {
        FREE(jumppoints);
        jumppoints = NULL;
    }

    jumppoints_count = 0;

    // In editor mode, mark the entire window as dirty.
    if (jumppoint_editor) {
        jumppoint_iso_invalidate_rect(NULL);
    }

    // Reset the modified flag.
    jumppoint_modified = false;
}

/**
 * Saves modified jumppoint data to the save path.
 *
 * 0x4E3270
 */
bool jumppoint_flush(void)
{
    TigFile* stream;

    // Skip saving if no modifications have been made.
    if (jumppoint_modified) {
        // Open the save file for writing.
        stream = tig_file_fopen(jumppoint_save_path, "wb");
        if (stream == NULL) {
            return false;
        }

        // Write jump point data to the file.
        if (!jumppoint_write_all(stream)) {
            tig_file_fclose(stream);
            return false;
        }

        tig_file_fclose(stream);
        jumppoint_modified = false;
    }

    return true;
}

/**
 * Called when view settings have changed.
 *
 * 0x4E32D0
 */
void jumppoint_update_view(ViewOptions* view_options)
{
    jumppoint_view_options = *view_options;
}

/**
 * Checks if jump point rendering is enabled.
 *
 * The game does not render jumppoints, this function only makes sense in the
 * editor.
 *
 * 0x4E32F0
 */
bool jumppoint_is_enabled(void)
{
    return jumppoint_enabled;
}

/**
 * Toggles the rendering of jump points.
 *
 * The game does not render jumppoints, this function only makes sense in the
 * editor.
 *
 * 0x4E3300
 */
void jumppoint_toggle(void)
{
    jumppoint_enabled = !jumppoint_enabled;
}

/**
 * Renders all jump points to the window based on the current view options.
 *
 * 0x4E3320
 */
void jumppoint_draw(GameDrawInfo* draw_info)
{
    TigArtBlitInfo art_blit_info;
    TigArtFrameData art_frame_data;
    TigRect src_rect;
    TigRect dst_rect;
    int index;
    TigRect jp_rect;
    TigRectListNode* rect_node;

    // Skip rendering if jumppoints are disabled.
    if (!jumppoint_enabled) {
        return;
    }

    // Select the appropriate art ID based on the view type.
    switch (jumppoint_view_options.type) {
    case VIEW_TYPE_ISOMETRIC:
        art_blit_info.art_id = jumppoint_iso_art_id;
        break;
    case VIEW_TYPE_TOP_DOWN:
        art_blit_info.art_id = jumppoint_td_art_id;
        tig_art_frame_data(jumppoint_td_art_id, &art_frame_data);
        src_rect.y = 0;
        src_rect.x = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;
        break;
    default:
        // Unknown view type.
        return;
    }

    // Set up art blit info for rendering.
    art_blit_info.flags = 0;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    // Iterate through all jumppoints to render them.
    for (index = 0; index < jumppoints_count; index++) {
        // Get the screen rect for the current jumppoint.
        jumppoint_get_rect(index, &jp_rect);

        // Iterate through dirty rects to check if jumppoint needs to be
        // rendered at all.
        rect_node = *draw_info->rects;
        while (rect_node != NULL) {
            if (tig_rect_intersection(&jp_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                // Adjust source rectangle for isometric view.
                if (jumppoint_view_options.type != VIEW_TYPE_TOP_DOWN) {
                    src_rect.x = dst_rect.x - jp_rect.x;
                    src_rect.y = dst_rect.y - jp_rect.y;
                    src_rect.width = dst_rect.width;
                    src_rect.height = dst_rect.height;
                }

                // Blit the affected jumppoint rect to the window.
                tig_window_blit_art(jumppoint_iso_window_handle, &art_blit_info);
            }
            rect_node = rect_node->next;
        }
    }
}

/**
 * Retrieves jumppoint for a given location.
 *
 * Returns `true` if a jumppoint is found at the location, `false` otherwise.
 *
 * 0x4E3450
 */
bool jumppoint_get(int64_t loc, JumpPoint* jumppoint)
{
    int index;

    for (index = 0; index < jumppoints_count; index++) {
        if (jumppoints[index].loc == loc) {
            if (jumppoint != NULL) {
                *jumppoint = jumppoints[index];
            }
            return true;
        }
    }

    return false;
}

/**
 * Adds a new jumppoint or updates an existing one.
 *
 * Returns `true` if the jumppoint is added or updated.
 *
 * 0x4E34B0
 */
bool jumppoint_add(JumpPoint* jumppoint, bool update)
{
    int index;

    // Check if the jumppoint already exists at the same location.
    for (index = 0; index < jumppoints_count; index++) {
        if (jumppoints[index].loc == jumppoint->loc) {
            // Jumppoint found, check if we should update.
            if (!update) {
                return false;
            }
            break;
        }
    }

    // If no existing jump point was found, expand the array.
    if (index == jumppoints_count) {
        jumppoints_count++;
        jumppoints = (JumpPoint*)REALLOC(jumppoints, sizeof(*jumppoints) * jumppoints_count);
    }

    jumppoints[index] = *jumppoint;

    jumppoint_invalidate(index);

    // Mark as modified for saving.
    jumppoint_modified = true;

    return true;
}

/**
 * Removes a jumppoint at the specified location.
 *
 * Returns `true` if a jumppoint was removed, `false` if no jumppoint was found
 * at the speicifed location.
 *
 * 0x4E3540
 */
bool jumppoint_remove(int64_t loc)
{
    int index;

    // Find the index of jumppoint to remove.
    for (index = 0; index < jumppoints_count; index++) {
        if (jumppoints[index].loc == loc) {
            break;
        }
    }

    if (index >= jumppoints_count) {
        return false;
    }

    // Mark the jumppoint screen area as dirty.
    jumppoint_invalidate(index);

    // Shift remaining entries to fill the gap.
    memmove(&(jumppoints[index]),
        &(jumppoints[index + 1]),
        sizeof(*jumppoints) * (jumppoints_count - index - 1));

    // Decrease the count and shrink the array.
    jumppoints_count--;
    jumppoints = (JumpPoint*)REALLOC(jumppoints, sizeof(*jumppoints) * jumppoints_count);

    // Mark as modified for saving.
    jumppoint_modified = true;

    return true;
}

/**
 * Moves a jumppoint from one location to another.
 *
 * Returns `true` if the jumppoint was moved, `false` if no jumppoint was found
 * at the specified location, or the new location already has other jumppoint.
 *
 * 0x4E35E0
 */
bool jumppoint_move(int64_t from, int64_t to)
{
    int index;
    int found = -1;

    // No action needed if locations are the same.
    if (from == to) {
        return false;
    }

    // Find the index of jumppoint to move.
    for (index = 0; index < jumppoints_count; index++) {
        if (jumppoints[index].loc == from) {
            found = index;
        } else if (jumppoints[index].loc == to) {
            // New location is already occupied.
            return false;
        }
    }

    if (found == -1) {
        return false;
    }

    // Mark old screen area as dirty.
    jumppoint_invalidate(found);

    jumppoints[found].loc = to;

    // Mark new screen area as dirty.
    jumppoint_invalidate(found);

    // Mark data as modified for saving.
    jumppoint_modified = true;

    return true;
}

/**
 * Checks if there are no jumppoints.
 *
 * 0x4E3690
 */
bool jumppoint_is_empty(void)
{
    return jumppoints_count == 0;
}

/**
 * Invalidates the screen rect of a jumppoint at the specified index.
 *
 * 0x4E36A0
 */
void jumppoint_invalidate(int jumppoint)
{
    TigRect rect;

    jumppoint_get_rect(jumppoint, &rect);
    jumppoint_iso_invalidate_rect(&rect);
}

/**
 * Computes the screen rect for a jumppoint based on its location and view
 * options.
 *
 * 0x4E36D0
 */
void jumppoint_get_rect(int jumppoint, TigRect* rect)
{
    int64_t x;
    int64_t y;
    TigArtFrameData art_frame_data;

    location_xy(jumppoints[jumppoint].loc, &x, &y);

    if (x >= INT_MIN && x < INT_MAX
        && y >= INT_MIN && y < INT_MAX) {
        // Handle top-down view: use zoom level as rect size.
        if (jumppoint_view_options.type == VIEW_TYPE_TOP_DOWN) {
            rect->x = (int)x;
            rect->y = (int)y;
            rect->width = jumppoint_view_options.zoom;
            rect->height = jumppoint_view_options.zoom;
            return;
        }

        // Handle isometric view: retrieve art data for rect size and position
        // adjustment.
        if (tig_art_frame_data(jumppoint_iso_art_id, &art_frame_data) == TIG_OK) {
            rect->x = (int)x - art_frame_data.hot_x + 40;
            rect->y = (int)y - (40 - art_frame_data.height) / 2 - art_frame_data.hot_y + 40;
            rect->width = art_frame_data.width;
            rect->height = art_frame_data.height;
            return;
        }
    }

    // Fallback - return an empty rect for invalid coordinates or failed art
    // data retrieval.
    rect->x = 0;
    rect->y = 0;
    rect->width = 0;
    rect->height = 0;
}

/**
 * Reads all jumppoint data from a file stream.
 *
 * Returns `true` if reading succeeds, `false` otherwise.
 *
 * 0x4E3800
 */
bool jumppoint_read_all(TigFile* stream)
{
    int index;

    // Read the number of jump points.
    if (tig_file_fread(&jumppoints_count, sizeof(jumppoints_count), 1, stream) != 1) {
        return false;
    }

    if (jumppoints_count != 0) {
        jumppoints = (JumpPoint*)MALLOC(sizeof(*jumppoints) * jumppoints_count);

        // Read jumppoint one by one.
        for (index = 0; index < jumppoints_count; index++) {
            if (tig_file_fread(&(jumppoints[index]), sizeof(*jumppoints), 1, stream) != 1) {
                // Clear partially loaded data on failure.
                jumppoint_map_close();
                return false;
            }
        }
    }

    return true;
}

/**
 * Writes all jump point data to a file stream.
 *
 * Returns `true` if writing succeeds, `false` otherwise.
 *
 * 0x4E3890
 */
bool jumppoint_write_all(TigFile* stream)
{
    int index;

    if (tig_file_fwrite(&jumppoints_count, sizeof(jumppoints_count), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < jumppoints_count; index++) {
        if (tig_file_fwrite(&(jumppoints[index]), sizeof(*jumppoints), 1, stream) != 1) {
            return false;
        }
    }

    return true;
}
