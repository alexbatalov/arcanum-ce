#include "game/tile_script.h"

#include "game/sector.h"
#include "game/tile.h"
#include "game/tile_script_list.h"

typedef struct TileScript {
    /* 0000 */ int64_t loc;
    /* 0008 */ Script scr;
    /* 0014 */ int padding_14;
} TileScript;

static_assert(sizeof(TileScript) == 0x18, "wrong size");

static bool tile_script_get(int64_t loc, TileScript* tile_script);
static bool tile_script_set(TileScript* tile_script);
static bool tile_script_remove(int64_t loc);
static void tile_script_get_rect(int64_t sector_id, int tile, TigRect* rect);

/**
 * Art ID for the isometric view tile script graphic.
 *
 * 0x5FC868
 */
static tig_art_id_t tile_script_iso_art_id;

/**
 * Flag indicating whether the tile script system is in editor mode.
 *
 * 0x5FC86C
 */
static bool tile_script_editor;

/**
 * Function pointer to invalidate a rectangle in the parent window.
 *
 * 0x5FC870
 */
static IsoInvalidateRectFunc* tile_script_iso_invalidate_rect;

/**
 * Flag indicating whether the tile script system has been initialized.
 *
 * 0x5FC874
 */
static bool tile_script_initialized;

/**
 * Flag indicating whether tile script rendering is enabled.
 *
 * 0x5FC878
 */
static bool tile_script_enabled;

/**
 * Handle to the parent window.
 *
 * 0x5FC87C
 */
static tig_window_handle_t tile_script_iso_window_handle;

/**
 * Editor view options.
 *
 * 0x5FC880
 */
static ViewOptions tile_script_view_options;

/**
 * Art ID for the top-down view tile script graphic.
 *
 * 0x5FC888
 */
static tig_art_id_t tile_script_td_art_id;

/**
 * Called when the game is initialized.
 *
 * 0x4C0530
 */
bool tile_script_init(GameInitInfo* init_info)
{
    tile_script_iso_window_handle = init_info->iso_window_handle;
    tile_script_iso_invalidate_rect = init_info->invalidate_rect_func;
    tile_script_editor = init_info->editor;
    tile_script_view_options.type = VIEW_TYPE_ISOMETRIC;

    // Initialize art IDs for isometric and top-down views. These only make
    // sense in the editor.
    tig_art_interface_id_create(475, 0, 0, 0, &tile_script_iso_art_id);
    tig_art_interface_id_create(476, 0, 0, 0, &tile_script_td_art_id);

    tile_script_initialized = true;
    tile_script_enabled = true;

    tile_script_node_init();

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x4C05B0
 */
void tile_script_reset(void)
{
}

/**
 * Called when the game shuts down.
 *
 * 0x4C05C0
 */
void tile_script_exit(void)
{
    tile_script_initialized = false;

    tile_script_node_exit();
}

/**
 * Called when the window size has changed.
 *
 * 0x4C05D0
 */
void tile_script_resize(GameResizeInfo* resize_info)
{
    tile_script_iso_window_handle = resize_info->window_handle;
}

/**
 * Called when view settings have changed.
 *
 * 0x4C05E0
 */
void tile_script_update_view(ViewOptions* view_options)
{
    tile_script_view_options = *view_options;
}

/**
 * Checks if tile scripts rendering is enabled.
 *
 * The game does not render tile scripts, this function only makes sense in the
 * editor.
 *
 * 0x4C0600
 */
bool tile_script_is_enabled(void)
{
    return tile_script_enabled;
}

/**
 * Toggles the rendering of tile scripts.
 *
 * The game does not render tile scripts, this function only makes sense in the
 * editor.
 *
 * 0x4C0610
 */
void tile_script_toggle(void)
{
    tile_script_enabled = !tile_script_enabled;
}

/**
 * Renders all tile scripts to the window based on the current view options.
 *
 * 0x4C0630
 */
void tile_script_draw(GameDrawInfo* draw_info)
{
    TigArtBlitInfo art_blit_info;
    TigArtFrameData art_frame_data;
    TigRect src_rect;
    TigRect dst_rect;
    SectorListNode* sector_node;
    Sector* sector;
    TileScriptListNode* tile_script_node;
    TigRect tile_rect;
    TigRectListNode* rect_node;

    // Skip rendering if tile scripts rendering is disabled.
    if (!tile_script_enabled) {
        return;
    }

    // Select the appropriate art ID based on the view type.
    switch (tile_script_view_options.type) {
    case VIEW_TYPE_ISOMETRIC:
        art_blit_info.art_id = tile_script_iso_art_id;
        break;
    case VIEW_TYPE_TOP_DOWN:
        art_blit_info.art_id = tile_script_td_art_id;
        tig_art_frame_data(art_blit_info.art_id, &art_frame_data);
        src_rect.x = 0;
        src_rect.y = 0;
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

    // Iterate through all affected sectors.
    sector_node = draw_info->sectors;
    while (sector_node != NULL) {
        if (sector_lock(sector_node->sec, &sector)) {
            // Iterate though all tile scripts in the sector.
            tile_script_node = sector->tile_scripts.head;
            while (tile_script_node != NULL) {
                // Get the screen rect for the current tile script.
                tile_script_get_rect(sector_node->sec, tile_script_node->id, &tile_rect);

                // Iterate throught dirty rects to check if tile script needs to
                // be rendered at all.
                rect_node = *draw_info->rects;
                while (rect_node != NULL) {
                    if (tig_rect_intersection(&tile_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                        // Adjust source rectangle for isometric view.
                        if (tile_script_view_options.type != VIEW_TYPE_TOP_DOWN) {
                            src_rect.x = dst_rect.x - tile_rect.x;
                            src_rect.y = dst_rect.y - tile_rect.y;
                            src_rect.width = dst_rect.width;
                            src_rect.height = dst_rect.height;
                        }

                        // Blit the affected tile script rect to the window.
                        tig_window_blit_art(tile_script_iso_window_handle, &art_blit_info);
                    }
                    rect_node = rect_node->next;
                }
                tile_script_node = tile_script_node->next;
            }
            sector_unlock(sector_node->sec);
        }
        sector_node = sector_node->next;
    }
}

/**
 * Retrieves tile script for a given location.
 *
 * 0x4C07B0
 */
bool tile_script_get(int64_t loc, TileScript* tile_script)
{
    int64_t sec;
    Sector* sector;
    int tile;
    bool rc;

    sec = sector_id_from_loc(loc);
    if (!sector_lock(sec, &sector)) {
        return false;
    }

    tile = tile_id_from_loc(loc);
    rc = tile_script_list_get(&(sector->tile_scripts), tile, &(tile_script->scr));
    tile_script->loc = loc;
    sector_unlock(sec);

    return rc;
}

/**
 * Sets tile script for a given location.
 *
 * 0x4C0830
 */
bool tile_script_set(TileScript* tile_script)
{
    int tile;
    int64_t sec;
    Sector* sector;
    TigRect dirty_rect;

    tile = tile_id_from_loc(tile_script->loc);
    sec = sector_id_from_loc(tile_script->loc);
    if (!sector_lock(sec, &sector)) {
        return false;
    }

    // In the editor setting tile script number to 0 removes it entirely. In
    // the game the scripts are allowed to remove themselves (opcode 5,
    // `SAT_REMOVE_THIS_SCRIPT`), in the case the script number is zeroed (which
    // indicates "no script")
    if (tile_script_editor && tile_script->scr.num == 0) {
        tile_script_list_remove(&(sector->tile_scripts), tile);
    } else {
        tile_script_list_set(&(sector->tile_scripts), tile, &(tile_script->scr));
    }

    sector_unlock(sec);

    // Mark the tile script screen area as dirty.
    tile_script_get_rect(sec, tile, &dirty_rect);
    tile_script_iso_invalidate_rect(&dirty_rect);

    return true;
}

/**
 * Moves a tile script from one location to another.
 *
 * This function only makes sense in the editor.
 *
 * 0x4C08E0
 */
void tile_script_move(int64_t old_loc, int64_t new_loc)
{
    TileScript tile_script;

    if (!tile_script_get(old_loc, &tile_script)) {
        return;
    }

    tile_script_remove(old_loc);

    tile_script.loc = new_loc;
    tile_script_set(&tile_script);
}

/**
 * Removes a tile script at the specified location.
 *
 * This function only makes sense in the editor.
 *
 * 0x4C0930
 */
bool tile_script_remove(int64_t loc)
{
    TileScript tile_script;

    if (!tile_script_get(loc, &tile_script)) {
        return false;
    }

    // See `tile_script_set` how setting script number works in the game vs.
    // editor.
    tile_script.scr.num = 0;
    tile_script_set(&tile_script);

    return true;
}

/**
 * Executes a tile script at a given location.
 *
 * 0x4C0980
 */
void tile_script_exec(int64_t loc, int64_t triggerer_obj)
{
    TileScript tile_script;
    Script scr;
    ScriptInvocation invocation;

    if (!tile_script_get(loc, &tile_script)) {
        return;
    }

    scr = tile_script.scr;

    // Set up script call.
    invocation.script = &scr;
    invocation.attachee_obj = OBJ_HANDLE_NULL;
    invocation.extra_obj = OBJ_HANDLE_NULL;
    invocation.line = 0;
    invocation.attachment_point = SAP_USE;
    invocation.triggerer_obj = triggerer_obj;
    script_execute(&invocation);

    // Update the script if modified.
    if (tile_script.scr.num != scr.num
        || tile_script.scr.hdr.flags != scr.hdr.flags
        || tile_script.scr.hdr.counters != scr.hdr.counters) {
        // TODO: Looks odd. Test how "remove this script" opcode works in this
        // case using script 3071 (teleport to Kerghan room).
        tile_script_set(&tile_script);
    }
}

/**
 * Computes the screen rect for a tile script based on its location and view
 * options.
 *
 * 0x4C0A40
 */
void tile_script_get_rect(int64_t sector_id, int tile, TigRect* rect)
{
    int64_t loc;
    int64_t x;
    int64_t y;
    TigArtFrameData art_frame_data;

    // Retrieve position on screen.
    loc = location_make((SECTOR_X(sector_id) << 6) | TILE_X(tile),
        (SECTOR_Y(sector_id) << 6) | TILE_Y(tile));
    location_xy(loc, &x, &y);

    if (x >= INT_MIN && x < INT_MAX
        && y >= INT_MIN && y < INT_MAX) {
        // Handle top-down view: use zoom level as rect size.
        if (tile_script_view_options.type == VIEW_TYPE_TOP_DOWN) {
            rect->x = (int)x;
            rect->y = (int)y;
            rect->width = tile_script_view_options.zoom;
            rect->height = tile_script_view_options.zoom;
            return;
        }

        // Handle isometric view: retrieve art data for rect size and position
        // adjustment.
        if (tig_art_frame_data(tile_script_iso_art_id, &art_frame_data) == TIG_OK) {
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
