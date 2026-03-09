#include "game/scroll.h"

#include "game/gamelib.h"
#include "game/gsound.h"
#include "game/location.h"
#include "game/name.h"
#include "game/object.h"
#include "game/player.h"
#include "game/stat.h"
#include "game/tc.h"
#include "game/ui.h"

#define SCROLL_DIAG_X 4
#define SCROLL_DIAG_Y 2

static void scroll_by(int64_t dx, int64_t dy);
static void scroll_origin_changed(int64_t loc);
static void scroll_speed_changed(void);
static bool scroll_cursor_art_set(tig_art_id_t art_id);

/**
 * The minimum time (in milliseconds) between scroll updates.
 *
 * 0x59F050
 */
static unsigned int scroll_fps = 1000;

/**
 * A copy of initialization info.
 *
 * 0x5D1168
 */
static GameInitInfo scroll_init_info;

/**
 * The current scrolling center location.
 *
 * 0x5D1180
 */
static int64_t scroll_center;

/**
 * Scroll speed.
 *
 * 0x5D1188
 */
static ScrollSpeed scroll_speed;

/**
 * Parent window bounds.
 *
 * 0x5D1190
 */
static TigRect scroll_iso_content_rect;

/**
 * Vertical scroll speed (in pixels per update).
 *
 * 0x5D11A0
 */
static int scroll_speed_y;

/**
 * Horizontal scroll speed (in pixels per update).
 *
 * 0x5D11A4
 */
static int scroll_speed_x;

/**
 * Editor view options.
 *
 * 0x5D11A8
 */
static ViewOptions scroll_view_options;

/**
 * Last known listener location for sound positioning.
 *
 * 0x5D11B8
 */
static int64_t scroll_origin;

/**
 * Flag indicating if scrolling is active.
 *
 * 0x5D11C0
 */
static bool is_scrolling;

/**
 * The maximum scroll distance from the scroll center.
 *
 * See `scroll_distance_set` for actual meaning.
 *
 * 0x5D11C4
 */
static int scroll_distance;

/**
 * Custom scroll function callback.
 *
 * 0x5D11C8
 */
static ScrollFunc* scroll_func;

/**
 * Called when the game is initialized.
 *
 * 0x40DF50
 */
bool scroll_init(GameInitInfo* init_info)
{
    TigWindowData window_data;

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    scroll_iso_content_rect.width = window_data.rect.width;
    scroll_iso_content_rect.height = window_data.rect.height;
    scroll_iso_content_rect.y = 0;
    scroll_iso_content_rect.x = 0;

    // Keep a copy of initialization info for later use. All props are being
    // used, so there is no point splitting it into individual variables.
    scroll_init_info = *init_info;

    scroll_view_options.type = VIEW_TYPE_ISOMETRIC;

    // Set the default scroll speed.
    scroll_speed = SCROLL_SPEED_NORMAL;
    scroll_speed_changed();

    location_origin_significant_change_callback_set(scroll_origin_changed);

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x40E000
 */
void scroll_exit(void)
{
}

/**
 * Called when the game is being reset.
 *
 * 0x40E010
 */
void scroll_reset(void)
{
    scroll_func = NULL;
}

/**
 * Called when the window size has changed.
 *
 * 0x40E020
 */
void scroll_resize(GameResizeInfo* resize_info)
{
    scroll_iso_content_rect = resize_info->content_rect;
    scroll_init_info.iso_window_handle = resize_info->window_handle;
}

/**
 * Called when view settings have changed.
 *
 * 0x40E060
 */
void scroll_update_view(ViewOptions* view_options)
{
    scroll_view_options = *view_options;
    scroll_speed_changed();
}

/**
 * Sets the scroll speed.
 *
 * 0x40E080
 */
void scroll_speed_set(ScrollSpeed value)
{
    scroll_speed = value;
    scroll_speed_changed();
}

/**
 * Retrieves the scroll speed.
 *
 * 0x40E090
 */
ScrollSpeed scroll_speed_get(void)
{
    return scroll_speed;
}

// 0x40E0A0
void scroll_start(int direction)
{
    // 0x5D117C
    static unsigned int scroll_ping_time;

    int dx;
    int dy;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    int distance;
    int64_t center_x;
    int64_t center_y;
    int viewport_center_x;
    int viewport_center_y;
    int hor;
    int vert;
    bool blocked;
    int64_t viewport_center_loc;
    int rot;

    // In non-editor mode, enforce scroll rate limit.
    if (!scroll_init_info.editor) {
        if ((unsigned int)tig_timer_between(scroll_ping_time, gamelib_ping_time) < scroll_fps) {
            return;
        }

        scroll_ping_time = gamelib_ping_time;
    }

    // Delegate to custom scroll function if set.
    if (scroll_func != NULL) {
        scroll_func(direction);
        return;
    }

    dx = 0;
    dy = 0;

    // Calculate scroll deltas based on direction.
    switch (direction) {
    case SCROLL_DIRECTION_UP:
        dy = scroll_speed_y;
        break;
    case SCROLL_DIRECTION_UP_RIGHT:
        dx = -(scroll_speed_x + SCROLL_DIAG_X);
        dy = scroll_speed_y + SCROLL_DIAG_Y;
        break;
    case SCROLL_DIRECTION_RIGHT:
        dx = -scroll_speed_x;
        break;
    case SCROLL_DIRECTION_DOWN_RIGHT:
        dx = -(scroll_speed_x + SCROLL_DIAG_X);
        dy = -(scroll_speed_y + SCROLL_DIAG_Y);
        break;
    case SCROLL_DIRECTION_DOWN:
        dy = -scroll_speed_y;
        break;
    case SCROLL_DIRECTION_DOWN_LEFT:
        dx = scroll_speed_x + SCROLL_DIAG_X;
        dy = -(scroll_speed_y + SCROLL_DIAG_Y);
        break;
    case SCROLL_DIRECTION_LEFT:
        dx = scroll_speed_x;
        break;
    case SCROLL_DIRECTION_UP_LEFT:
        dx = scroll_speed_x + SCROLL_DIAG_X;
        dy = scroll_speed_y + SCROLL_DIAG_Y;
        break;
    default:
        break;
    }

    // In editor mode, perform the scroll immediately.
    if (scroll_init_info.editor) {
        scroll_by(dx, dy);
        return;
    }

    // Begin continuous scrolling.
    is_scrolling = true;

    // Retrieve the effective scroll distance.
    distance = scroll_distance_get();
    if (distance == 0) {
        // No distance limit.
        tig_art_interface_id_create(direction + 679, 0, 0, 0, &art_id);
        scroll_cursor_art_set(art_id);
        scroll_by(dx, dy);
        return;
    }

    // Get the current scroll center coordinates (adjusted to center of tile)
    location_xy(scroll_center, &center_x, &center_y);
    center_x += 40;
    center_y += 20;

    // Calculate viewport center.
    viewport_center_x = scroll_iso_content_rect.width / 2;
    viewport_center_y = scroll_iso_content_rect.height / 2;

    // Calculate horizontal and vertical distance (in pixels) from the scroll
    // center.
    hor = abs(viewport_center_x - dx - (int)center_x);
    vert = abs(viewport_center_y - dy - (int)center_y);

    // Check if scrolling is within perception-based limits.
    if (hor < 80 * distance && vert < 40 * distance) {
        tig_art_interface_id_create(direction + 679, 0, 0, 0, &art_id);
        scroll_cursor_art_set(art_id);
        scroll_by(dx, dy);
        return;
    }

    location_at(viewport_center_x, viewport_center_y, &viewport_center_loc);

    rot = location_rot(viewport_center_loc, scroll_center);
    if (rot == (direction - 1) % 8
        || rot == (direction + 1) % 8
        || rot == direction) {
        tig_art_interface_id_create(direction + 679, 0, 0, 0, &art_id);
        scroll_cursor_art_set(art_id);
        scroll_by(dx, dy);
        return;
    }

    blocked = false;

    // Adjust direction if horizontal distance exceeds scroll distance limit.
    if (hor >= 80 * distance) {
        switch (direction) {
        case SCROLL_DIRECTION_UP_RIGHT:
            direction = SCROLL_DIRECTION_UP;
            dx += scroll_speed_x + SCROLL_DIAG_X;
            break;
        case SCROLL_DIRECTION_RIGHT:
        case SCROLL_DIRECTION_LEFT:
            blocked = true;
            break;
        case SCROLL_DIRECTION_DOWN_RIGHT:
            direction = SCROLL_DIRECTION_DOWN;
            dx += scroll_speed_x + SCROLL_DIAG_X;
            break;
        case SCROLL_DIRECTION_DOWN_LEFT:
            direction = SCROLL_DIRECTION_DOWN;
            dx -= scroll_speed_x + SCROLL_DIAG_X;
            break;
        case SCROLL_DIRECTION_UP_LEFT:
            direction = SCROLL_DIRECTION_UP;
            dx -= scroll_speed_x + SCROLL_DIAG_X;
            break;
        }
    }

    // Adjust direction if vertical distance exceeds scroll distance limit.
    if (vert >= 40 * distance) {
        switch (direction) {
        case SCROLL_DIRECTION_UP:
        case SCROLL_DIRECTION_DOWN:
            blocked = true;
            break;
        case SCROLL_DIRECTION_UP_RIGHT:
            direction = SCROLL_DIRECTION_RIGHT;
            dy -= scroll_speed_y + SCROLL_DIAG_Y;
            break;
        case SCROLL_DIRECTION_DOWN_RIGHT:
            direction = SCROLL_DIRECTION_RIGHT;
            dy += scroll_speed_y + SCROLL_DIAG_Y;
            break;
        case SCROLL_DIRECTION_DOWN_LEFT:
            direction = SCROLL_DIRECTION_LEFT;
            dy += scroll_speed_y + SCROLL_DIAG_Y;
            break;
        case SCROLL_DIRECTION_UP_LEFT:
            direction = SCROLL_DIRECTION_LEFT;
            dy -= scroll_speed_y + SCROLL_DIAG_Y;
            break;
        }
    }

    // Perform scroll unless blocked.
    if (!blocked) {
        tig_art_interface_id_create(direction + 679, 0, 0, 0, &art_id);
        scroll_cursor_art_set(art_id);
        scroll_by(dx, dy);
        return;
    }

    // Scrolling is blocked. Update the mouse cursor and set appropriate offsets
    // so it appears sticked to the relevant edge.
    tig_art_interface_id_create(678, 0, 0, 0, &art_id);
    if (scroll_cursor_art_set(art_id)
        && tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
        switch (direction) {
        case SCROLL_DIRECTION_UP:
            tig_mouse_cursor_set_offset(art_frame_data.width / 2, 0);
            break;
        case SCROLL_DIRECTION_UP_RIGHT:
            tig_mouse_cursor_set_offset(art_frame_data.width - 1, 0);
            break;
        case SCROLL_DIRECTION_RIGHT:
            tig_mouse_cursor_set_offset(art_frame_data.width - 1, art_frame_data.height / 2);
            break;
        case SCROLL_DIRECTION_DOWN_RIGHT:
            tig_mouse_cursor_set_offset(art_frame_data.width - 1, art_frame_data.height - 1);
            break;
        case SCROLL_DIRECTION_DOWN:
            tig_mouse_cursor_set_offset(art_frame_data.width / 2, art_frame_data.height - 1);
            break;
        case SCROLL_DIRECTION_DOWN_LEFT:
            tig_mouse_cursor_set_offset(art_frame_data.width / 2, art_frame_data.height - 1);
            break;
        case SCROLL_DIRECTION_LEFT:
            tig_mouse_cursor_set_offset(0, art_frame_data.height / 2);
            break;
        case SCROLL_DIRECTION_UP_LEFT:
            tig_mouse_cursor_set_offset(0, 0);
            break;
        }
    }
}

/**
 * Ends scrolling and resets the cursor.
 *
 * 0x40E610
 */
void scroll_stop(void)
{
    if (is_scrolling) {
        ui_refresh_cursor();
        is_scrolling = false;
    }
}

/**
 * Scrolls the game view by the specified offsets.
 *
 * 0x40E630
 */
void scroll_by(int64_t dx, int64_t dy)
{
    int64_t old_origin_x;
    int64_t old_origin_y;
    int64_t new_origin_x;
    int64_t new_origin_y;
    TigRect rect;

    // Redraw the view to prepare for scrolling.
    scroll_init_info.draw_func();

    // Update the view origin and check for actual movement.
    location_origin_get(&old_origin_x, &old_origin_y);
    location_origin_scroll(dx, dy);
    location_origin_get(&new_origin_x, &new_origin_y);

    // Exit if no actual movement occurred (at map edges).
    if (old_origin_x == new_origin_x && old_origin_y == new_origin_y) {
        return;
    }

    // Calculate effective scroll offsets.
    dx = new_origin_x - old_origin_x;
    dy = new_origin_y - old_origin_y;

    // Scroll the window content.
    tig_window_scroll(scroll_init_info.iso_window_handle, (int)dx, (int)dy);

    // Invalidate newly revealed areas (horizontal scroll).
    if (dx > 0) {
        rect = scroll_iso_content_rect;
        rect.width = (int)dx;
        scroll_init_info.invalidate_rect_func(&rect);
    } else if (dx < 0) {
        rect = scroll_iso_content_rect;
        rect.x = rect.width + (int)dx;
        rect.width = -((int)dx);
        scroll_init_info.invalidate_rect_func(&rect);
    }

    // Force redraw when scrolling in both directions.
    if (dx != 0 && dy != 0) {
        scroll_init_info.draw_func();
    }

    // Invalidate newly revealed areas (vertical scroll).
    if (dy < 0) {
        rect = scroll_iso_content_rect;
        rect.y += rect.height + (int)dy;
        rect.height = -(int)dy;
        scroll_init_info.invalidate_rect_func(&rect);
    } else if (dy > 0) {
        rect = scroll_iso_content_rect;
        rect.height = (int)dy;
        scroll_init_info.invalidate_rect_func(&rect);
    }

    // Notify text conversation system of the scroll, so it can update it's
    // dimming overlay.
    tc_scroll((int)dx, (int)dy);

    if (!scroll_init_info.editor) {
        int64_t loc;

        // Update the sound listener position.
        location_at(scroll_iso_content_rect.width / 2, scroll_iso_content_rect.height / 2, &loc);
        if (loc != scroll_origin) {
            gsound_listener_set(loc);
            scroll_origin = loc;
        }
    }
}

/**
 * Sets the scroll frame rate.
 *
 * 0x40E890
 */
void scroll_fps_set(int fps)
{
    // FIX: Make sure `fps` is positive.
    if (fps <= 0) {
        return;
    }

    scroll_fps = 1000 / fps;
}

/**
 * Sets the maximum scroll distance.
 *
 * The value of `0` indicates that there is no scrolling limit. Any other value
 * indicates that scrolling limit is in effect, subject to player's perception
 * level.
 *
 * 0x40E8A0
 */
void scroll_distance_set(int distance)
{
    scroll_distance = distance;
}

/**
 * Retrieves the effective scrolling distance (in tiles), factoring in player
 * perception.
 *
 * The maximum scrolling distance is 13 tiles from the scrolling center
 * location.
 *
 * Returns `0` if there is no scrolling distance limit.
 *
 * 0x40E8B0
 */
int scroll_distance_get(void)
{
    int64_t pc_obj;

    if (scroll_distance == 0) {
        return 0;
    }

    pc_obj = player_get_local_pc_obj();
    if (pc_obj == OBJ_HANDLE_NULL) {
        return 0;
    }

    return stat_level_get(pc_obj, STAT_PERCEPTION) / 2 + 3;
}

/**
 * Sets the center location which is used to determine maximum allowed scrolling
 * distance.
 *
 * 0x40E8E0
 */
void scroll_set_center(int64_t location)
{
    scroll_center = location;
}

/**
 * Sets a custom scroll function to override default behaviour.
 *
 * 0x40E900
 */
void scroll_set_scroll_func(ScrollFunc* func)
{
    scroll_func = func;
}

/**
 * Called when location origin suddenly changes.
 *
 * 0x40E910
 */
void scroll_origin_changed(int64_t loc)
{
    if (!scroll_init_info.editor) {
        // Update listener location.
        gsound_listener_set(loc);
        scroll_origin = loc;
    }
}

/**
 * Internal helper to update scroll speeds based on view options and zoom level.
 *
 * 0x40E940
 */
void scroll_speed_changed(void)
{
    if (scroll_view_options.type == VIEW_TYPE_ISOMETRIC) {
        switch (scroll_speed) {
        case SCROLL_SPEED_SLOW:
            scroll_speed_x = 8;
            scroll_speed_y = 4;
            break;
        case SCROLL_SPEED_NORMAL:
            scroll_speed_x = 20;
            scroll_speed_y = 10;
            break;
        case SCROLL_SPEED_FAST:
            scroll_speed_x = 28;
            scroll_speed_y = 14;
            break;
        case SCROLL_SPEED_VERY_FAST:
            scroll_speed_x = 56;
            scroll_speed_y = 28;
            break;
        }
    } else {
        switch (scroll_speed) {
        case SCROLL_SPEED_SLOW:
            scroll_speed_x = scroll_view_options.zoom / 2;
            scroll_speed_y = scroll_view_options.zoom / 4;
            break;
        case SCROLL_SPEED_NORMAL:
            scroll_speed_x = scroll_view_options.zoom;
            scroll_speed_y = scroll_view_options.zoom / 2;
            break;
        case SCROLL_SPEED_FAST:
            scroll_speed_x = scroll_view_options.zoom * 2;
            scroll_speed_y = scroll_view_options.zoom;
            break;
        case SCROLL_SPEED_VERY_FAST:
            scroll_speed_x = scroll_view_options.zoom * 4;
            scroll_speed_y = scroll_view_options.zoom * 2;
            break;
        }
    }
}

/**
 * Internal helper to set the mouse cursor art.
 *
 * 0x40EA50
 */
bool scroll_cursor_art_set(tig_art_id_t art_id)
{
    // Check if the desired cursor is already set.
    if (name_normalize_aid(art_id) == name_normalize_aid(tig_mouse_cursor_get_art_id())) {
        return true;
    }

    // Attempt to set the new cursor art.
    if (tig_mouse_cursor_set_art_id(art_id) == TIG_OK) {
        return true;
    }

    return false;
}
