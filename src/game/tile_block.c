#include "game/tile_block.h"

#include "game/a_name.h"
#include "game/sector.h"
#include "game/tile.h"

static void tileblock_get_rect_internal(int64_t sec, int tile, TigRect* rect);

/**
 * Art ID for the top-down view tile block graphic.
 *
 * 0x5FC370
 */
static tig_art_id_t tileblock_td_aid;

/**
 * Flag indicating whether the tile block system is in editor mode.
 *
 * 0x5FC374
 */
static bool tileblock_editor;

/**
 * Art ID for the isometric view tile block graphic.
 *
 * 0x5FC378
 */
static tig_art_id_t tileblock_iso_aid;

/**
 * Function pointer to invalidate a rectangle in the parent window.
 *
 * 0x5FC37C
 */
static IsoInvalidateRectFunc* tileblock_invalidate_rect;

/**
 * Flag indicating whether tile block rendering is enabled.
 *
 * 0x5FC380
 */
static bool tileblock_enabled;

/**
 * Editor view options.
 *
 * 0x5FC388
 */
static ViewOptions tileblock_view_options;

/**
 * Handle to the parent window.
 *
 * 0x5FC390
 */
static tig_window_handle_t tileblock_iso_window;

/**
 * Called when the game is initialized.
 *
 * 0x4BAFF0
 */
bool tileblock_init(GameInitInfo* init_info)
{
    tileblock_iso_window = init_info->iso_window_handle;
    tileblock_invalidate_rect = init_info->invalidate_rect_func;
    tileblock_editor = init_info->editor;
    tileblock_view_options.type = VIEW_TYPE_ISOMETRIC;

    // Initialize art IDs for isometric and top-down views. These only make
    // sense in the editor.
    tig_art_interface_id_create(614, 0, 0, 0, &tileblock_iso_aid);
    tig_art_interface_id_create(615, 0, 0, 0, &tileblock_td_aid);

    tileblock_enabled = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x4BB050
 */
void tileblock_exit(void)
{
}

/**
 * Called when view settings have changed.
 *
 * 0x4BB060
 */
void tileblock_update_view(ViewOptions* view_info)
{
    tileblock_view_options = *view_info;
}

/**
 * Called when the window size has changed.
 *
 * 0x4BB080
 */
void tileblock_resize(GameResizeInfo* resize_info)
{
    tileblock_iso_window = resize_info->window_handle;
}

/**
 * Checks if tile block rendering is enabled.
 *
 * The game does not render tile blocks, this function only makes sense in the
 * editor.
 *
 * 0x4BB090
 */
bool tileblock_is_enabled(void)
{
    return tileblock_enabled;
}

/**
 * Toggles the rendering of tile blocks.
 *
 * The game does not render tile blocks, this function only makes sense in the
 * editor.
 *
 * 0x4BB0A0
 */
void tileblock_toggle(void)
{
    tileblock_enabled = !tileblock_enabled;
}

/**
 * Renders all tile blocks to the window based on the current view options.
 *
 * 0x4BB0C0
 */
void tileblock_draw(GameDrawInfo* draw_info)
{
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigArtFrameData art_frame_data;
    SectorListNode* sector_node;
    Sector* sector;
    int tile;
    tig_art_id_t aid;
    TigRect tb_rect;
    bool tb_rect_calculated;
    TigRectListNode* rect_node;
    ObjectNode* obj_node;
    int obj_type;
    bool blocked;

    // Skip rendering if tile blocks are disabled.
    if (!tileblock_enabled) {
        return;
    }

    // Select the appropriate art ID based on the view type.
    switch (tileblock_view_options.type) {
    case VIEW_TYPE_ISOMETRIC:
        art_blit_info.art_id = tileblock_iso_aid;
        break;
    case VIEW_TYPE_TOP_DOWN:
        art_blit_info.art_id = tileblock_td_aid;
        tig_art_frame_data(tileblock_td_aid, &art_frame_data);
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
    art_blit_info.flags = TIG_ART_BLT_BLEND_COLOR_CONST;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    // Iterate through all affected sectors.
    sector_node = draw_info->sectors;
    while (sector_node != NULL) {
        if (sector_lock(sector_node->sec, &sector)) {
            // Iterate through all sector tiles (64x64).
            for (tile = 0; tile < 4096; tile++) {
                tb_rect_calculated = false;

                // Check generated blocking.
                //
                // NOTE: Here is good explanation of generated blocking from
                // the WorldEd manual:
                //
                // "Generated blocking tiles are ones that are built into
                // certain facades and tilesets. Generated blocking tiles cannot
                // be edited. They will always be blocked. An example of a
                // generated blocking tile is the deep water tileset. If you
                // select deep water and spray it down in the editor window, you
                // will notice a dark red-hatched tilesize piece of art appear
                // on top of the water. This is indicating that the player will
                // not be able to walk on these tiles. Generated blocked tiles
                // are also very common on facades and scenery.""
                blocked = false;

                // Retrieve and check tile art ID - it's guaranteed to be either
                // tile or facade. The tile blocking configuration is specified
                // in `tilename.mes` with `/b` flag. The facade blocking
                // configuration is stored in the appropriate `facwalk` file.
                aid = sector->tiles.art_ids[tile];
                if ((tig_art_type(aid) == TIG_ART_TYPE_FACADE
                        && !tig_art_facade_id_walkable_get(aid))
                    || a_name_tile_is_blocking(aid)) {
                    blocked = true;
                }

                if (!blocked) {
                    // Check if there is at least one object on this tile that
                    // does not have `OF_NO_BLOCK` flag.
                    //
                    // Walls are always considered blocking and portals are
                    // non-blocking. There is no point in marking them
                    // specifically since their "walkability" is obvious.
                    obj_node = sector->objects.heads[tile];
                    while (obj_node != NULL) {
                        obj_type = obj_field_int32_get(obj_node->obj, OBJ_F_TYPE);
                        if (obj_type != OBJ_TYPE_WALL
                            && obj_type != OBJ_TYPE_PORTAL
                            && (obj_field_int32_get(obj_node->obj, OBJ_F_FLAGS) & OF_NO_BLOCK) == 0) {
                            blocked = true;
                            break;
                        }
                        obj_node = obj_node->next;
                    }
                }

                if (blocked) {
                    // Use darker red for generated blocking tiles.
                    art_blit_info.color = tig_color_make(128, 0, 0);

                    // Retrieve screen rect of the tile.
                    tileblock_get_rect_internal(sector_node->sec, tile, &tb_rect);
                    tb_rect_calculated = true;

                    // Iterate through dirty rects to check if tile block needs
                    // to be rendered at all.
                    rect_node = *draw_info->rects;
                    while (rect_node != NULL) {
                        if (tig_rect_intersection(&tb_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                            // Adjust source rectangle for isometric view.
                            if (tileblock_view_options.type != VIEW_TYPE_TOP_DOWN) {
                                src_rect.x = dst_rect.x - tb_rect.x;
                                src_rect.width = dst_rect.width;
                                src_rect.y = dst_rect.y - tb_rect.y;
                                src_rect.height = dst_rect.height;
                            }

                            // Blit the affected tile block rect to the window.
                            tig_window_blit_art(tileblock_iso_window, &art_blit_info);
                        }
                        rect_node = rect_node->next;
                    }
                }

                // Check placed blocking. The configuration is stored in bit
                // array.
                if ((sector->blocks.mask[tile / 32] & (1 << (tile % 32))) != 0) {
                    // Use bright red for placed blocking tiles.
                    art_blit_info.color = tig_color_make(255, 0, 0);

                    // Reuse tile block rect if it was already calculated.
                    if (!tb_rect_calculated) {
                        tileblock_get_rect_internal(sector_node->sec, tile, &tb_rect);
                    }

                    // Iterate through dirty rects to check if tile block needs
                    // to be rendered at all.
                    rect_node = *draw_info->rects;
                    while (rect_node != NULL) {
                        if (tig_rect_intersection(&tb_rect, &(rect_node->rect), &dst_rect) == TIG_OK) {
                            // Adjust source rectangle for isometric view.
                            if (tileblock_view_options.type != VIEW_TYPE_TOP_DOWN) {
                                src_rect.x = dst_rect.x - tb_rect.x;
                                src_rect.width = dst_rect.width;
                                src_rect.y = dst_rect.y - tb_rect.y;
                                src_rect.height = dst_rect.height;
                            }

                            // Blit the affected tile block rect to the window.
                            tig_window_blit_art(tileblock_iso_window, &art_blit_info);
                        }
                        rect_node = rect_node->next;
                    }
                }
            }

            sector_unlock(sector_node->sec);
        }
        sector_node = sector_node->next;
    }
}

/**
 * Checks if a tile is explicitly blocked.
 *
 * 0x4BB410
 */
bool tileblock_is_tile_blocked(int64_t loc)
{
    int64_t sec;
    Sector* sector;
    int tile;
    bool blocked;

    sec = sector_id_from_loc(loc);
    if (!sector_lock(sec, &sector)) {
        return false;
    }

    tile = tile_id_from_loc(loc);
    blocked = (sector->blocks.mask[tile / 32] & (1u << (tile % 32))) != 0;
    sector_unlock(sec);

    return blocked;
}

/**
 * Sets or clears the blocked state of a tile.
 *
 * 0x4BB490
 */
void tileblock_set_tile_blocked(int64_t loc, bool blocked)
{
    int64_t sec;
    Sector* sector;
    int tile;
    TigRect rect;

    sec = sector_id_from_loc(loc);
    tile = tile_id_from_loc(loc);
    if (sector_lock(sec, &sector)) {
        if (blocked) {
            sector->blocks.mask[tile / 32] |= 1 << (tile % 32);
        } else {
            sector->blocks.mask[tile / 32] &= ~(1 << (tile % 32));
        }
        sector->blocks.modified = true;
        sector_unlock(sec);
    }

    // Mark the tile block screen area as dirty.
    tileblock_get_rect_internal(sec, tile, &rect);
    tileblock_invalidate_rect(&rect);
}

/**
 * Computes the screen rect for a tile script based on its location and view
 * options.
 *
 * 0x4BB550
 */
void tileblock_get_rect(int64_t loc, TigRect* rect)
{
    tileblock_get_rect_internal(sector_id_from_loc(loc),
        tile_id_from_loc(loc),
        rect);
}

/**
 * Internal helper function that computes screen rect from sector/tile pair
 * rather than the location.
 *
 * 0x4BB590
 */
void tileblock_get_rect_internal(int64_t sec, int tile, TigRect* rect)
{
    int64_t loc;
    int64_t x;
    int64_t y;
    TigArtFrameData art_frame_data;

    // Retrieve position on screen.
    loc = location_make(SECTOR_X(sec) + TILE_X(tile),
        SECTOR_Y(sec) + TILE_Y(tile));
    location_xy(loc, &x, &y);

    if (x > INT_MIN && x < INT_MAX
        && y > INT_MIN && y < INT_MAX) {
        // Handle top-down view: use zoom level as rect size.
        if (tileblock_view_options.type == VIEW_TYPE_TOP_DOWN) {
            rect->x = (int)x;
            rect->y = (int)y;
            rect->width = tileblock_view_options.zoom;
            rect->height = tileblock_view_options.zoom;
            return;
        }

        // Handle isometric view: retrieve art data for rect size and position
        // adjustment.
        if (tig_art_frame_data(tileblock_iso_aid, &art_frame_data) == TIG_OK) {
            rect->x = (int)x - art_frame_data.hot_x + 40;
            rect->y = (int)y - ((40 - art_frame_data.height) / 2) - art_frame_data.hot_y + 40;
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
