#include "ui/follower_ui.h"

#include <stdio.h>

#include "game/broadcast.h"
#include "game/critter.h"
#include "game/hrp.h"
#include "game/mes.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/player.h"
#include "game/portrait.h"
#include "game/stat.h"
#include "game/target.h"
#include "ui/charedit_ui.h"
#include "ui/dialog_ui.h"
#include "ui/intgame.h"
#include "ui/inven_ui.h"
#include "ui/tb_ui.h"

/**
 * Commands available in the follower drop-down context menu.
 *
 * Menu labels for each command is loaded from "mes\follower_ui.mes", keyed by
 * command index.
 */
typedef enum FollowerUiCommand {
    FOLLOWER_UI_COMMAND_WALK,
    FOLLOWER_UI_COMMAND_ATTACK,
    FOLLOWER_UI_COMMAND_STAY_CLOSE,
    FOLLOWER_UI_COMMAND_SPREAD_OUT,
    FOLLOWER_UI_COMMAND_BACK_OFF,
    FOLLOWER_UI_COMMAND_WAIT,
    FOLLOWER_UI_COMMAND_INVENTORY,
    FOLLOWER_UI_COMMAND_CHARACTER_SHEET,
    FOLLOWER_UI_COMMAND_COUNT,
} FollowerUiCommand;

/**
 * The maximum number of followers visible at one time in the panel.
 *
 * TODO: This value should be calculated at runtime to account for available
 * window size.
 */
#define FOLLOWER_UI_SLOTS 6

/**
 * Indices for the non-portrait UI buttons that appear below the slot area.
 */
typedef enum FollowerUiButton {
    FOLLOWER_UI_BUTTON_TOGGLE = FOLLOWER_UI_SLOTS,
    FOLLOWER_UI_BUTTON_SCROLL_UP,
    FOLLOWER_UI_BUTTON_SCROLL_DOWN,
    FOLLOWER_UI_BUTTON_COUNT,
} FollowerUiButton;

/**
 * Initial allocation size for the followers array.
 *
 * The array is grown dynamically if the party exceeds this count.
 */
#define FOLLOWER_UI_FOLLOWERS_INITIAL_CAPACITY 10

/**
 * Amount by which the followers array capacity is increased when it is
 * exhausted.
 */
#define FOLLOWER_UI_FOLLOWERS_CAPACITY_INCREMENT 10

/**
 * Pixel height of each entry row in the drop-down context menu.
 *
 * Used both when creating invisible hit-test buttons and when advancing the
 * draw cursor while rendering text labels.
 */
#define FOLLOWER_UI_DROP_DOWN_MENU_ENTRY_HEIGHT 18

/**
 * Extra width (in pixels beyond the base 800px resolution) beyond which the
 * special buttons (toggle and scrollers) are forced into compact layout.
 */
#define FOLLOWER_UI_WIDESCREEN_EXTRA_WIDTH_THRESHOLD 134

/**
 * HP percentage at or below which a follower portrait receives a red tint
 * overlay as a visual danger warning.
 */
#define FOLLOWER_UI_LOW_HP_THRESHOLD 20

/**
 * Poison level at or above which the poisoned health bar art is used instead
 * of the normal one.
 */
#define FOLLOWER_UI_POISON_LEVEL_THRESHOLD 20

/**
 * "flare12font.art"
 */
#define FOLLOWER_UI_FONT_ART 229

/**
 * "followerarrow_dwn.art"
 */
#define FOLLOWER_UI_ARROW_DOWN_ART 501

/**
 * "followerarrow_up.art"
 */
#define FOLLOWER_UI_ARROW_UP_ART 502

/**
 * "followerframe.art"
 */
#define FOLLOWER_UI_FRAME_ART 503

/**
 * "followerframe_ft.art"
 */
#define FOLLOWER_UI_FATIGUE_BAR_ART 504

/**
 * "followerframe_hp.art"
 */
#define FOLLOWER_UI_HEALTH_BAR_ART 505

/**
 * "follow_scroll_up.art"
 */
#define FOLLOWER_UI_SCROLL_UP_ON_ART 506

/**
 * "follow_scroll_up_off.art"
 */
#define FOLLOWER_UI_SCROLL_UP_OFF_ART 507

/**
 * "follow_scroll_dwn.art"
 */
#define FOLLOWER_UI_SCROLL_DOWN_ON_ART 508

/**
 * "follow_scroll_dwn_off.art"
 */
#define FOLLOWER_UI_SCROLL_DOWN_OFF_ART 509

/**
 * "followerframe_po.art"
 */
#define FOLLOWER_UI_POISON_BAR_ART 616

/**
 * "follwercomandmenu.art"
 */
#define FOLLOWER_UI_DROP_DOWN_MENU_ART 826

static void follower_ui_create(int index);
static bool follower_ui_message_filter(TigMessage* msg);
static void follower_ui_drop_down_menu_create(int index);
static void follower_ui_drop_down_menu_destroy();
static void follower_ui_begin_order_mode(int cmd);
static void follower_ui_draw(tig_window_handle_t window_handle, int num, int x, int y, int src_scale, int dst_scale);
static void follower_ui_toggle();
static void update_toggle_button_visibility();
static void update_scroll_buttons_visibility();
static void follower_ui_drop_down_menu_refresh(int highlighted_cmd);

/**
 * Defines content frames for buttons managed by follower UI.
 *
 * 0x5CA360
 */
static TigRect follower_ui_button_rects[FOLLOWER_UI_BUTTON_COUNT] = {
    /*                         SLOT 0 */ { 11, 50, 40, 49 },
    /*                         SLOT 1 */ { 11, 112, 40, 49 },
    /*                         SLOT 2 */ { 11, 174, 40, 49 },
    /*                         SLOT 3 */ { 11, 236, 40, 49 },
    /*                         SLOT 4 */ { 11, 298, 40, 49 },
    /*                         SLOT 5 */ { 11, 360, 40, 49 },
    /*      FOLLOWER_UI_BUTTON_TOGGLE */ { 0, 428, 67, 13 },
    /*   FOLLOWER_UI_BUTTON_SCROLL_UP */ { 0, 415, 33, 13 },
    /* FOLLOWER_UI_BUTTON_SCROLL_DOWN */ { 33, 415, 34, 13 },
};

/**
 * Positions for the three special buttons in normal interface mode.
 *
 * 0x5CA3F0
 */
static TigRect follower_ui_special_button_rects_normal_mode[FOLLOWER_UI_BUTTON_COUNT - FOLLOWER_UI_SLOTS] = {
    /*      FOLLOWER_UI_BUTTON_TOGGLE */ { 0, 428, 67, 13 },
    /*   FOLLOWER_UI_BUTTON_SCROLL_UP */ { 0, 415, 33, 13 },
    /* FOLLOWER_UI_BUTTON_SCROLL_DOWN */ { 33, 415, 34, 13 },
};

/**
 * Positions for the three special buttons in compact interface mode or when
 * the window is wider than the widescreen threshold.
 *
 * 0x5CA420
 */
static TigRect follower_ui_special_button_rects_compact_mode[FOLLOWER_UI_BUTTON_COUNT - FOLLOWER_UI_SLOTS] = {
    /*      FOLLOWER_UI_BUTTON_TOGGLE */ { 0, 587, 67, 13 },
    /*   FOLLOWER_UI_BUTTON_SCROLL_UP */ { 0, 574, 33, 13 },
    /* FOLLOWER_UI_BUTTON_SCROLL_DOWN */ { 33, 574, 34, 13 },
};

/**
 * Defines content rect for a single drop down menu item. The `y` is advanced by
 * `FOLLOWER_UI_DROP_DOWN_MENU_ENTRY_HEIGHT` for each subsequent command entry.
 *
 * 0x5CA450
 */
static TigRect follower_ui_drop_down_menu_entry_rect = { 10, 3, 118, 15 };

/**
 * Button handles for all follower panel buttons (slots + toggle + scrollers).
 *
 * 0x67BB38
 */
static tig_button_handle_t follower_ui_buttons[FOLLOWER_UI_BUTTON_COUNT];

/**
 * Font handle used to draw a highlighted (hovered) drop-down menu entry.
 * Color: gold (255, 210, 0).
 *
 * 0x67BB5C
 */
static tig_font_handle_t follower_ui_drop_down_menu_item_highlighted_color;

/**
 * Window handles for all follower panel windows (one per button, including
 * the three special buttons).
 *
 * NOTE: Unclear why every button reside in it's own window.
 *
 * 0x67BB60
 */
static tig_window_handle_t follower_ui_windows[FOLLOWER_UI_BUTTON_COUNT];

/**
 * Precomputed screen rects for each follower's drop-down context menu.
 * Each menu opens immediately to the right of its portrait slot.
 * Dimensions are taken from `FOLLOWER_UI_DROP_DOWN_MENU_ART` at init time.
 *
 * NOTE: For unknown reason its size is 8, but only `FOLLOWER_UI_SLOTS` (6)
 * entries are ever initialized or accessed.
 *
 * 0x67BB88
 */
static TigRect follower_ui_drop_down_menu_rects[8];

/**
 * Dynamically allocated array holding followers' safe object handles. Sized to
 * `follower_ui_followers_capacity` entries.
 *
 * 0x67BC08
 */
static FollowerInfo* follower_ui_followers;

/**
 * Index into `follower_ui_followers` of the first follower shown in the
 * visible slot area. Scrolling increments/decrements this value.
 *
 * Persisted across save/load.
 *
 * 0x67BC10
 */
static int follower_ui_top_index;

/**
 * Current allocated capacity of `follower_ui_followers`.
 *
 * 0x67BC14
 */
static int follower_ui_followers_capacity;

/**
 * Font handle used to draw a disabled (grayed-out) drop-down menu entry.
 * Color: gray (128, 128, 128).
 *
 * 0x67BC18
 */
static tig_font_handle_t follower_ui_drop_down_menu_item_disabled_color;

/**
 * Object handle of the player character who issued the last command.
 * Set to the local PC when a follower slot is interacted with.
 *
 * 0x67BC20
 */
static int64_t follower_ui_commander_obj;

/**
 * Font handle used to draw a normal (non-highlighted) drop-down menu entry.
 * Color: white (255, 255, 255).
 *
 * 0x67BC28
 */
static tig_font_handle_t follower_ui_drop_down_menu_item_normal_color;

/**
 * "follower_ui.mes"
 *
 * 0x67BC2C
 */
static mes_file_handle_t follower_ui_mes_file;

/**
 * Button handles for the invisible hit-test buttons within the drop-down
 * context menu, one per `FollowerUiCommand` entry.
 *
 * 0x67BC30
 */
static tig_button_handle_t follower_ui_drop_menu_item_buttons[FOLLOWER_UI_COMMAND_COUNT];

/**
 * Object handle of the follower targeted by the most recent right-click or
 * command interaction.
 *
 * 0x67BC50
 */
static int64_t follower_ui_subordinate_obj;

/**
 * Total number of followers currently in the party.
 *
 * 0x67BC58
 */
static int follower_ui_followers_count;

/**
 * Flag indicating whether the follower UI has been initialized.
 *
 * 0x67BC5C
 */
static bool follower_ui_initialized;

/**
 * Whether the follower UI is currently shown.
 *
 * NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
 * compatibility.
 *
 * 0x67BC60
 */
static int follower_ui_visible;

/**
 * Re-entrancy guard: set to `true` during `follower_ui_update` to prevent
 * `follower_ui_refresh` from triggering a redundant nested update.
 *
 * 0x67BC64
 */
static bool in_update;

/**
 * Window handle for the currently open drop-down context menu, or
 * `TIG_WINDOW_HANDLE_INVALID` when no menu is open.
 *
 * 0x67BC0C
 */
static tig_window_handle_t follower_ui_drop_down_menu_window;

/**
 * Called when the game is initialized.
 *
 * 0x56A4A0
 */
bool follower_ui_init(GameInitInfo* init_info)
{
    int index;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigFont font_desc;

    (void)init_info;

    if (!mes_load("mes\\follower_ui.mes", &follower_ui_mes_file)) {
        return false;
    }

    for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        follower_ui_create(index);
    }

    // Determine the drop-down menu dimensions from the background art.
    tig_art_interface_id_create(FOLLOWER_UI_DROP_DOWN_MENU_ART, 0, 0, 0, &art_id);
    tig_art_frame_data(art_id, &art_frame_data);

    // Each slot's drop-down opens immediately to the right of that slot.
    for (index = 0; index < FOLLOWER_UI_SLOTS; index++) {
        follower_ui_drop_down_menu_rects[index].x = follower_ui_button_rects[index].x + follower_ui_button_rects[index].width;
        follower_ui_drop_down_menu_rects[index].y = follower_ui_button_rects[index].y;
        follower_ui_drop_down_menu_rects[index].width = art_frame_data.width;
        follower_ui_drop_down_menu_rects[index].height = art_frame_data.height;
    }

    follower_ui_followers_capacity = FOLLOWER_UI_FOLLOWERS_INITIAL_CAPACITY;
    follower_ui_followers_count = 0;
    follower_ui_top_index = 0;
    follower_ui_followers = (FollowerInfo*)MALLOC(sizeof(*follower_ui_followers) * follower_ui_followers_capacity);

    // Create white font for normal menu items.
    font_desc.flags = 0;
    tig_art_interface_id_create(FOLLOWER_UI_FONT_ART, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &follower_ui_drop_down_menu_item_normal_color);

    // Create gray font for disabled menu items.
    font_desc.flags = 0;
    tig_art_interface_id_create(FOLLOWER_UI_FONT_ART, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(128, 128, 128);
    tig_font_create(&font_desc, &follower_ui_drop_down_menu_item_disabled_color);

    // Create gold font for highlighted menu items.
    font_desc.flags = 0;
    tig_art_interface_id_create(FOLLOWER_UI_FONT_ART, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 210, 0);
    tig_font_create(&font_desc, &follower_ui_drop_down_menu_item_highlighted_color);

    follower_ui_initialized = true;
    follower_ui_visible = true;
    follower_ui_drop_down_menu_window = TIG_WINDOW_HANDLE_INVALID;

    return true;
}

/**
 * Creates a window-button pair for the given button index.
 *
 * 0x56A6E0
 */
void follower_ui_create(int index)
{
    TigWindowData window_data;
    TigButtonData button_data;

    window_data.flags = TIG_WINDOW_HIDDEN;

    // Only the window hosting toggle button carries the message filter because
    // only this button is visible all the time (whether follower UI itself is
    // visible or hidden).
    if (index == FOLLOWER_UI_BUTTON_TOGGLE) {
        window_data.message_filter = follower_ui_message_filter;
        window_data.flags |= TIG_WINDOW_MESSAGE_FILTER;
    } else {
        window_data.message_filter = NULL;
    }

    window_data.rect = follower_ui_button_rects[index];

    // Adjust window position relative to the original placement in 800x600
    // window.
    switch (index) {
    case FOLLOWER_UI_BUTTON_TOGGLE:
    case FOLLOWER_UI_BUTTON_SCROLL_UP:
    case FOLLOWER_UI_BUTTON_SCROLL_DOWN:
        // Special buttons are anchored bottom-left.
        hrp_apply(&(window_data.rect), GRAVITY_LEFT | GRAVITY_BOTTOM);
        break;
    default:
        // Portrait slots are anchored top-left.
        hrp_apply(&(window_data.rect), GRAVITY_LEFT | GRAVITY_TOP);
        break;
    }

    tig_window_create(&window_data, &(follower_ui_windows[index]));

    // Set up button properties.
    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    button_data.x = 0;
    button_data.y = 0;

    switch (index) {
    case FOLLOWER_UI_BUTTON_TOGGLE:
        tig_art_interface_id_create(FOLLOWER_UI_ARROW_DOWN_ART, 0, 0, 0, &(button_data.art_id));
        break;
    case FOLLOWER_UI_BUTTON_SCROLL_UP:
        tig_art_interface_id_create(FOLLOWER_UI_SCROLL_UP_ON_ART, 0, 0, 0, &(button_data.art_id));
        break;
    case FOLLOWER_UI_BUTTON_SCROLL_DOWN:
        tig_art_interface_id_create(FOLLOWER_UI_SCROLL_DOWN_ON_ART, 0, 0, 0, &(button_data.art_id));
        break;
    default:
        // Portrait slots have no art of their own. Portrait drawing is done
        // manually by `follower_ui_update`.
        button_data.art_id = TIG_ART_ID_INVALID;
        button_data.width = follower_ui_button_rects[index].width;
        button_data.height = follower_ui_button_rects[index].height;
        break;
    }

    button_data.window_handle = follower_ui_windows[index];
    tig_button_create(&button_data, &(follower_ui_buttons[index]));
}

/**
 * Called when the game shuts down.
 *
 * 0x56A820
 */
void follower_ui_exit()
{
    int index;

    mes_unload(follower_ui_mes_file);

    // There is no need to destroy buttons individually - destroying host
    // window also destroys all buttons.
    for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        tig_window_destroy(follower_ui_windows[index]);
    }

    FREE(follower_ui_followers);

    if (follower_ui_drop_down_menu_window != TIG_WINDOW_HANDLE_INVALID) {
        follower_ui_drop_down_menu_destroy();
    }

    follower_ui_initialized = false;

    // FIX: Memory leak.
    tig_font_destroy(follower_ui_drop_down_menu_item_normal_color);
    tig_font_destroy(follower_ui_drop_down_menu_item_disabled_color);
    tig_font_destroy(follower_ui_drop_down_menu_item_highlighted_color);
}

/**
 * Called when the game is being reset.
 *
 * 0x56A880
 */
void follower_ui_reset()
{
    int index;

    follower_ui_visible = true;
    follower_ui_followers_count = 0;
    follower_ui_top_index = 0;

    for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        tig_window_hide(follower_ui_windows[index]);
    }

    if (follower_ui_drop_down_menu_window != TIG_WINDOW_HANDLE_INVALID) {
        follower_ui_drop_down_menu_destroy();
    }
}

/**
 * Called when the window size has changed.
 *
 * 0x56A8D0
 */
void follower_ui_resize(GameResizeInfo* resize_info)
{
    TigRect* rects;
    int index;

    // Pick placement rects for the three special buttons.
    rects = intgame_is_compact_interface()
        ? follower_ui_special_button_rects_compact_mode
        : follower_ui_special_button_rects_normal_mode;

    // Force compact layout for wide-screen resolutions regardless of mode.
    if (resize_info->window_rect.width - 800 > FOLLOWER_UI_WIDESCREEN_EXTRA_WIDTH_THRESHOLD) {
        rects = follower_ui_special_button_rects_compact_mode;
    }

    // Update the rect table entries for the three special buttons.
    for (index = FOLLOWER_UI_SLOTS; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        follower_ui_button_rects[index] = rects[index - FOLLOWER_UI_SLOTS];
    }

    // Rebuild only the special button windows.
    for (index = FOLLOWER_UI_SLOTS; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        tig_window_destroy(follower_ui_windows[index]);
    }

    for (index = FOLLOWER_UI_SLOTS; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        follower_ui_create(index);
    }

    follower_ui_refresh();

    // Toggle twice to force a redraw with the new layout while leaving
    // `follower_ui_visible` unchanged.
    follower_ui_toggle();
    follower_ui_toggle();
}

/**
 * Called when the game is being loaded.
 *
 * 0x56A940
 */
bool follower_ui_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(&follower_ui_visible, sizeof(follower_ui_visible), 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&follower_ui_top_index, sizeof(follower_ui_top_index), 1, load_info->stream) != 1) return false;

    follower_ui_refresh();

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x56A990
 */
bool follower_ui_save(TigFile* stream)
{
    if (tig_file_fwrite(&follower_ui_visible, sizeof(follower_ui_visible), 1, stream) != 1) return false;
    if (tig_file_fwrite(&follower_ui_top_index, sizeof(follower_ui_top_index), 1, stream) != 1) return false;

    return true;
}

/**
 * Called when an event occurred.
 *
 * 0x56A9D0
 */
bool follower_ui_message_filter(TigMessage* msg)
{
    int64_t pc_obj;
    int index;
    MesFileEntry mes_file_entry;
    char str[MAX_STRING];
    Broadcast bcast;
    S4F2810 v1;

    pc_obj = player_get_local_pc_obj();
    if (sub_567400(pc_obj)) {
        return false;
    }

    if (follower_ui_drop_down_menu_window != TIG_WINDOW_HANDLE_INVALID) {
        // A drop-down context menu is open — route events to it.
        switch (msg->type) {
        case TIG_MESSAGE_BUTTON:
            switch (msg->data.button.state) {
            case TIG_BUTTON_STATE_MOUSE_INSIDE:
                // Highlight the command entry the cursor is over.
                for (index = 0; index < FOLLOWER_UI_COMMAND_COUNT; index++) {
                    if (msg->data.button.button_handle == follower_ui_drop_menu_item_buttons[index]) {
                        follower_ui_drop_down_menu_refresh(index);
                        return true;
                    }
                }
                return false;
            case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
                // Remove highlighting when cursor leaves a command entry.
                for (index = 0; index < FOLLOWER_UI_COMMAND_COUNT; index++) {
                    if (msg->data.button.button_handle == follower_ui_drop_menu_item_buttons[index]) {
                        follower_ui_drop_down_menu_refresh(-1);
                        return true;
                    }
                }
                return false;
            case TIG_BUTTON_STATE_RELEASED:
                // Execute the clicked command, then close the menu.
                for (index = 0; index < FOLLOWER_UI_COMMAND_COUNT; index++) {
                    if (msg->data.button.button_handle == follower_ui_drop_menu_item_buttons[index]) {
                        follower_ui_drop_down_menu_destroy();
                        switch (index) {
                        case FOLLOWER_UI_COMMAND_WALK:
                        case FOLLOWER_UI_COMMAND_ATTACK:
                            // Enter targeting mode. The player needs to click
                            // a tile or object to complete the order.
                            follower_ui_begin_order_mode(index);
                            break;
                        case FOLLOWER_UI_COMMAND_INVENTORY:
                            // Open barter inventory if permitted.
                            if (inven_ui_can_open_inven(follower_ui_commander_obj, follower_ui_subordinate_obj)) {
                                inven_ui_open(follower_ui_commander_obj, follower_ui_subordinate_obj, INVEN_UI_MODE_BARTER);
                            }
                            break;
                        case FOLLOWER_UI_COMMAND_CHARACTER_SHEET:
                            charedit_open(follower_ui_subordinate_obj, CHAREDIT_MODE_PASSIVE);
                            break;
                        case FOLLOWER_UI_COMMAND_WAIT:
                            // Mind-controlled followers cannot be ordered to wait.
                            if ((obj_field_int32_get(follower_ui_subordinate_obj, OBJ_F_SPELL_FLAGS) & OSF_MIND_CONTROLLED) != 0) {
                                return true;
                            }
                            // FALLTHROUGH
                        default:
                            // Stay Close, Spread Out, Back Off, and Wait (when
                            // not mind-controlled) are issued as broadcast
                            // messages.
                            mes_file_entry.num = index;
                            if (mes_search(follower_ui_mes_file, &mes_file_entry)) {
                                object_examine(follower_ui_subordinate_obj, follower_ui_commander_obj, str);
                                sprintf(bcast.str, "%s %s", str, mes_file_entry.str);
                                broadcast_msg(follower_ui_commander_obj, &bcast);
                            }
                            break;
                        }
                        return true;
                    }
                }
                return false;
            }
            break;
        case TIG_MESSAGE_MOUSE:
            switch (msg->data.mouse.event) {
            case TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP:
            case TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP:
                // Any click outside a command button closes the menu.
                follower_ui_drop_down_menu_destroy();
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }

        return false;
    }

    // No drop-down open — handle normal panel interactions.
    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            // Show the hovered follower's name/description in the message
            // window.
            for (index = 0; index < FOLLOWER_UI_SLOTS; index++) {
                if (msg->data.button.button_handle == follower_ui_buttons[index]) {
                    follower_ui_commander_obj = player_get_local_pc_obj();
                    sub_444130(&follower_ui_followers[follower_ui_top_index + index]);
                    follower_ui_subordinate_obj = follower_ui_followers[follower_ui_top_index + index].obj;
                    if (follower_ui_subordinate_obj != OBJ_HANDLE_NULL) {
                        sub_57CD60(follower_ui_commander_obj, follower_ui_subordinate_obj, str);
                        sub_553BE0(follower_ui_commander_obj, follower_ui_subordinate_obj, str);
                        object_hover_obj_set(follower_ui_subordinate_obj);
                    }
                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            // Clear the message window when the cursor leaves any button.
            for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
                if (msg->data.button.button_handle == follower_ui_buttons[index]) {
                    sub_550720();
                    object_hover_obj_set(OBJ_HANDLE_NULL);
                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_RELEASED:
            if (msg->data.button.button_handle == follower_ui_buttons[FOLLOWER_UI_BUTTON_TOGGLE]) {
                follower_ui_toggle();
                return true;
            }

            if (msg->data.button.button_handle == follower_ui_buttons[FOLLOWER_UI_BUTTON_SCROLL_UP]) {
                if (follower_ui_top_index > 0) {
                    follower_ui_top_index--;
                    update_scroll_buttons_visibility();
                    follower_ui_update();
                }
                return true;
            }

            if (msg->data.button.button_handle == follower_ui_buttons[FOLLOWER_UI_BUTTON_SCROLL_DOWN]) {
                if (follower_ui_top_index < follower_ui_followers_count - FOLLOWER_UI_SLOTS) {
                    follower_ui_top_index++;
                    update_scroll_buttons_visibility();
                    follower_ui_update();
                }
                return true;
            }

            // Left-click on a portrait slot.
            for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
                if (msg->data.button.button_handle == follower_ui_buttons[index]) {
                    sub_444130(&follower_ui_followers[follower_ui_top_index + index]);
                    sub_4F2810(&v1, follower_ui_followers[follower_ui_top_index + index].obj);
                    sub_54EA80(&v1);
                    return true;
                }
            }
            return false;
        }
        return false;
    case TIG_MESSAGE_MOUSE:
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP && follower_ui_visible) {
            tig_window_handle_t window_handle;

            // Determine which portrait slot (if any) was right-clicked.
            for (index = 0; index < FOLLOWER_UI_SLOTS; index++) {
                if (follower_ui_top_index + index >= follower_ui_followers_count) {
                    break;
                }

                if (msg->data.mouse.x >= follower_ui_button_rects[index].x
                    && msg->data.mouse.y >= follower_ui_button_rects[index].y
                    && msg->data.mouse.x < follower_ui_button_rects[index].x + follower_ui_button_rects[index].width
                    && msg->data.mouse.y < follower_ui_button_rects[index].y + follower_ui_button_rects[index].height) {
                    // Confirm the top-most window at the cursor position is
                    // the expected slot window before opening the menu.
                    if (tig_window_get_at_position(msg->data.mouse.x, msg->data.mouse.y, &window_handle) == TIG_OK
                        && window_handle == follower_ui_windows[index]) {
                        follower_ui_drop_down_menu_create(index);
                    }
                    break;
                }
            }
        }
        return false;
    default:
        return false;
    }
}

/**
 * Creates the drop-down context menu for the given follower slot.
 *
 * 0x56AFD0
 */
void follower_ui_drop_down_menu_create(int index)
{
    TigWindowData window_data;
    TigButtonData button_data;
    int cmd;

    follower_ui_commander_obj = player_get_local_pc_obj();

    sub_444130(&(follower_ui_followers[follower_ui_top_index + index]));
    follower_ui_subordinate_obj = follower_ui_followers[follower_ui_top_index + index].obj;

    window_data.flags = 0;
    window_data.rect = follower_ui_drop_down_menu_rects[index];
    hrp_apply(&(window_data.rect), GRAVITY_LEFT | GRAVITY_TOP);

    tig_window_create(&window_data, &follower_ui_drop_down_menu_window);

    // Draw the initial unhighlighted menu (no command hovered yet).
    follower_ui_drop_down_menu_refresh(-1);

    // Create an invisible hit-test button for each command entry, stacked
    // vertically at `FOLLOWER_UI_DROP_DOWN_MENU_ENTRY_HEIGHT` intervals.
    button_data.x = follower_ui_drop_down_menu_entry_rect.x;
    button_data.y = follower_ui_drop_down_menu_entry_rect.y;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    button_data.art_id = TIG_ART_ID_INVALID;
    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.width = follower_ui_drop_down_menu_entry_rect.width;
    button_data.height = follower_ui_drop_down_menu_entry_rect.height;
    button_data.window_handle = follower_ui_drop_down_menu_window;

    for (cmd = 0; cmd < FOLLOWER_UI_COMMAND_COUNT; cmd++) {
        tig_button_create(&button_data, &(follower_ui_drop_menu_item_buttons[cmd]));
        button_data.y += FOLLOWER_UI_DROP_DOWN_MENU_ENTRY_HEIGHT;
    }
}

/**
 * Closes and destroys the drop-down context menu, if one is open.
 *
 * 0x56B0F0
 */
void follower_ui_drop_down_menu_destroy()
{
    if (follower_ui_drop_down_menu_window != TIG_WINDOW_HANDLE_INVALID) {
        tig_window_destroy(follower_ui_drop_down_menu_window);
        follower_ui_drop_down_menu_window = TIG_WINDOW_HANDLE_INVALID;
    }
}

/**
 * Enters follower order mode, placing the game in targeting mode so the
 * player can click a destination tile or target object to complete the order.
 *
 * For `FOLLOWER_UI_COMMAND_WALK` the target must be a walkable tile.
 * For `FOLLOWER_UI_COMMAND_ATTACK`, holding Alt allows targeting any object
 * (including friendlies), while the default restricts to non-party,
 * non-dead critters.
 *
 * 0x56B110
 */
void follower_ui_begin_order_mode(int cmd)
{
    uint64_t tgt;

    intgame_mode_set(INTGAME_MODE_FOLLOWER);

    if (cmd == FOLLOWER_UI_COMMAND_WALK) {
        // The target must be a walkable tile.
        tgt = Tgt_Tile;
    } else {
        if (tig_kb_get_modifier(SDL_KMOD_ALT)) {
            // Unsafe targeting mode - any object (except self and walls).
            tgt = Tgt_Obj_No_Self | Tgt_Obj_No_T_Wall;
        } else {
            // Safe targeting mode - only valid critter enemies, no dead, no
            // self, no walls.
            tgt = Tgt_Obj_No_ST_Critter_Dead | Tgt_Obj_No_Self | Tgt_Obj_No_T_Wall | Tgt_Non_Party_Critters;
        }
    }

    sub_4F25B0(tgt);
}

/**
 * Called by the `intgame` system when the player completes a targeting action
 * while in follower order mode (after `follower_ui_begin_order_mode`).
 *
 * Determines whether the player clicked a tile (walk order) or an object
 * (attack order), looks up the appropriate broadcast message, and sends it to
 * the follower.
 *
 * 0x56B180
 */
void follower_ui_execute_order(S4F2810* a1)
{
    int num;
    Broadcast bcast;
    MesFileEntry mes_file_entry;
    char str[MAX_STRING];

    follower_ui_end_order_mode();

    if (!critter_is_dead(follower_ui_commander_obj) && !critter_is_unconscious(follower_ui_commander_obj)) {
        if (a1->is_loc) {
            // Walk
            num = FOLLOWER_UI_COMMAND_WALK;
            bcast.loc = a1->loc;
        } else {
            // Attack
            num = FOLLOWER_UI_COMMAND_ATTACK;
            bcast.loc = obj_field_int64_get(a1->obj, OBJ_F_LOCATION);
        }

        mes_file_entry.num = num;
        if (mes_search(follower_ui_mes_file, &mes_file_entry)) {
            object_examine(follower_ui_subordinate_obj, follower_ui_commander_obj, str);
            sprintf(bcast.str, "%s %s", str, mes_file_entry.str);
            broadcast_msg(follower_ui_commander_obj, &bcast);
        }
    }
}

/**
 * Exits follower order mode and returns the game to normal intgame mode.
 *
 * 0x56B280
 */
void follower_ui_end_order_mode()
{
    intgame_mode_set(INTGAME_MODE_MAIN);
}

/**
 * Redraws all visible follower portrait slots.
 *
 * 0x56B290
 */
void follower_ui_update()
{
    int64_t pc_obj;
    int64_t follower_obj;
    tig_color_t color;
    TigRect rect;
    int index;
    int portrait;
    int hp_percent;
    int fatigue_percent;

    if (!follower_ui_visible) {
        return;
    }

    in_update = true;

    pc_obj = player_get_local_pc_obj();
    color = tig_color_make(255, 0, 0);

    // Portrait area within the slot window used for the tint overlay.
    rect.x = 4;
    rect.y = 4;
    rect.width = 32;
    rect.height = 32;

    for (index = 0; index < FOLLOWER_UI_SLOTS; index++) {
        if (follower_ui_top_index + index >= follower_ui_followers_count) {
            break;
        }

        sub_444130(&(follower_ui_followers[follower_ui_top_index + index]));

        follower_obj = follower_ui_followers[follower_ui_top_index + index].obj;
        if (follower_obj == OBJ_HANDLE_NULL) {
            // Follower is no longer valid — rebuild the list and stop.
            follower_ui_refresh();
            break;
        }

        // Draw slot background.
        follower_ui_draw(follower_ui_windows[index], FOLLOWER_UI_FRAME_ART, 0, 0, 100, 100);

        // Retrieve and draw portrait.
        if (intgame_examine_portrait(pc_obj, follower_obj, &portrait)) {
            // The `portrait` is a critter portrait number from `portrait.mes`.
            portrait_draw_32x32(follower_obj,
                portrait,
                follower_ui_windows[index],
                4,
                4);
        } else {
            // The `portrait` is an art number from `interface.mes`.
            follower_ui_draw(follower_ui_windows[index], portrait, 4, 4, 100, 50);
        }

        // Draw health bar.
        hp_percent = 100 * object_hp_current(follower_obj) / object_hp_max(follower_obj);
        if (stat_level_get(follower_obj, STAT_POISON_LEVEL) > FOLLOWER_UI_POISON_LEVEL_THRESHOLD) {
            follower_ui_draw(follower_ui_windows[index], FOLLOWER_UI_POISON_BAR_ART, 3, 39, hp_percent, 100);
        } else {
            follower_ui_draw(follower_ui_windows[index], FOLLOWER_UI_HEALTH_BAR_ART, 3, 39, hp_percent, 100);
        }

        // Apply red tint over portrait when HP is critically low.
        if (hp_percent < FOLLOWER_UI_LOW_HP_THRESHOLD) {
            tig_window_tint(follower_ui_windows[index], &rect, color, 0);
        }

        // Draw fatigue bar.
        fatigue_percent = 100 * critter_fatigue_current(follower_obj) / critter_fatigue_max(follower_obj);
        follower_ui_draw(follower_ui_windows[index], FOLLOWER_UI_FATIGUE_BAR_ART, 3, 44, fatigue_percent, 100);
    }

    in_update = false;
}

/**
 * Called when a critter's health or fatigue value has changed.
 *
 * 0x56B4D0
 */
void follower_ui_update_obj(int64_t obj)
{
    if (follower_ui_visible) {
        if (critter_pc_leader_get(obj) == player_get_local_pc_obj()) {
            follower_ui_update();
        }
    }
}

/**
 * Blits a portion of an interface art asset into a window, with independent
 * horizontal scaling for source crop and destination size.
 *
 * `src_scale` controls how much of the art's width is sampled (1-100%),
 * used to show health/fatigue bars as partial fills. `dst_scale` controls the
 * size of the rendered destination rect relative to the (already-cropped)
 * source.
 *
 * 0x56B510
 */
void follower_ui_draw(tig_window_handle_t window_handle, int num, int x, int y, int src_scale, int dst_scale)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;

    if (src_scale <= 0) {
        return;
    }

    if (src_scale > 100) {
        src_scale = 100;
    }

    if (dst_scale <= 0) {
        return;
    }

    tig_art_interface_id_create(num, 0, 0, 0, &art_id);
    tig_art_frame_data(art_id, &art_frame_data);

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = src_scale * art_frame_data.width / 100;
    // NOTE: Why height is not scaled?
    src_rect.height = art_frame_data.height;

    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.width = dst_scale * src_rect.width / 100;
    dst_rect.height = dst_scale * src_rect.height / 100;

    blit_info.flags = 0;
    blit_info.art_id = art_id;
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(window_handle, &blit_info);
}

/**
 * Toggles the follower UI.
 *
 * 0x56B620
 */
void follower_ui_toggle()
{
    int index;
    tig_art_id_t art_id;

    follower_ui_visible = !follower_ui_visible;
    if (follower_ui_visible) {
        // Show all active slots.
        for (index = 0; index < FOLLOWER_UI_SLOTS && index < follower_ui_followers_count; index++) {
            tig_window_show(follower_ui_windows[index]);
        }

        tig_art_interface_id_create(FOLLOWER_UI_ARROW_DOWN_ART, 0, 0, 0, &art_id);
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_TOGGLE], art_id);
        update_toggle_button_visibility();
        update_scroll_buttons_visibility();
        follower_ui_update();
    } else {
        // Hide all buttons.
        for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
            tig_window_hide(follower_ui_windows[index]);
        }

        tig_art_interface_id_create(FOLLOWER_UI_ARROW_UP_ART, 0, 0, 0, &art_id);
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_TOGGLE], art_id);
        update_toggle_button_visibility();
        update_scroll_buttons_visibility();
    }
}

/**
 * Rebuilds the follower list from the current party state and refreshes all
 * UI windows accordingly.
 *
 * 0x56B6F0
 */
void follower_ui_refresh()
{
    int index;
    int64_t pc_obj;
    ObjectList followers;
    ObjectNode* node;

    if (!follower_ui_initialized) {
        return;
    }

    // Hide all buttons.
    for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        tig_window_hide(follower_ui_windows[index]);
    }

    // Count current followers.
    follower_ui_followers_count = 0;
    pc_obj = player_get_local_pc_obj();
    object_list_followers(pc_obj, &followers);
    node = followers.head;
    while (node != NULL) {
        follower_ui_followers_count++;
        node = node->next;
    }

    // Grow the followers array if needed.
    if (follower_ui_followers_count > follower_ui_followers_capacity) {
        follower_ui_followers_capacity = follower_ui_followers_count + FOLLOWER_UI_FOLLOWERS_CAPACITY_INCREMENT;
        follower_ui_followers = (FollowerInfo*)REALLOC(follower_ui_followers, sizeof(*follower_ui_followers) * follower_ui_followers_capacity);
    }

    // Retrieve followers' safe object handles.
    index = 0;
    node = followers.head;
    while (node != NULL) {
        sub_4440E0(node->obj, &(follower_ui_followers[index++]));
        node = node->next;
    }
    follower_ui_followers_count = index;

    object_list_destroy(&followers);

    if (follower_ui_visible) {
        // Show all active slots.
        for (index = 0; index < FOLLOWER_UI_SLOTS && index < follower_ui_followers_count; index++) {
            tig_window_show(follower_ui_windows[index]);
        }
    }

    // Clamp the scroll position so no empty slots are shown.
    if (follower_ui_followers_count > FOLLOWER_UI_SLOTS) {
        if (follower_ui_top_index > follower_ui_followers_count - FOLLOWER_UI_SLOTS) {
            follower_ui_top_index = follower_ui_followers_count - FOLLOWER_UI_SLOTS;
        }
    } else {
        follower_ui_top_index = 0;
    }

    update_toggle_button_visibility();
    update_scroll_buttons_visibility();

    // Avoid a redundant nested redraw if we are already inside
    // `follower_ui_update`.
    if (!in_update) {
        follower_ui_update();
    }
}

/**
 * Shows the toggle button if there are any followers, hides otherwise.
 *
 * 0x56B850
 */
void update_toggle_button_visibility()
{
    if (follower_ui_followers_count > 0) {
        tig_window_show(follower_ui_windows[FOLLOWER_UI_BUTTON_TOGGLE]);
    } else {
        tig_window_hide(follower_ui_windows[FOLLOWER_UI_BUTTON_TOGGLE]);
    }
}

/**
 * Updates scroll buttons visibility and art to reflect the current scroll
 * position.
 *
 * Scroll buttons are only shown when there are more followers than visible
 * slots AND the panel is visible. Each button shows a greyed "off" art when
 * scrolling further in that direction is not possible.
 *
 * 0x56B880
 */
void update_scroll_buttons_visibility()
{
    tig_art_id_t art_id;

    if (follower_ui_followers_count > FOLLOWER_UI_SLOTS && follower_ui_visible) {
        // Scroll-up button is active if not already at the top.
        if (follower_ui_top_index != 0) {
            tig_art_interface_id_create(FOLLOWER_UI_SCROLL_UP_ON_ART, 0, 0, 0, &art_id);
        } else {
            tig_art_interface_id_create(FOLLOWER_UI_SCROLL_UP_OFF_ART, 0, 0, 0, &art_id);
        }
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_SCROLL_UP], art_id);
        tig_window_show(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_UP]);

        // Scroll-down button is active if not already at the bottom.
        if (follower_ui_top_index != follower_ui_followers_count - FOLLOWER_UI_SLOTS) {
            tig_art_interface_id_create(FOLLOWER_UI_SCROLL_DOWN_ON_ART, 0, 0, 0, &art_id);
        } else {
            tig_art_interface_id_create(FOLLOWER_UI_SCROLL_DOWN_OFF_ART, 0, 0, 0, &art_id);
        }
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_SCROLL_DOWN], art_id);
        tig_window_show(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_DOWN]);
    } else {
        // Either all portraits fit into existing slots, or the follower UI is
        // hidden.
        tig_window_hide(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_UP]);
        tig_window_hide(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_DOWN]);
    }
}

/**
 * Redraws the drop-down context menu contents.
 *
 * 0x56B970
 */
void follower_ui_drop_down_menu_refresh(int highlighted_cmd)
{
    TigRect rect;
    int cmd;
    MesFileEntry mes_file_entry;
    bool enabled;

    follower_ui_draw(follower_ui_drop_down_menu_window, FOLLOWER_UI_DROP_DOWN_MENU_ART, 0, 0, 100, 100);

    rect = follower_ui_drop_down_menu_entry_rect;
    for (cmd = 0; cmd < FOLLOWER_UI_COMMAND_COUNT; cmd++) {
        mes_file_entry.num = cmd;
        if (mes_search(follower_ui_mes_file, &mes_file_entry)) {
            // Determine whether this command is currently available.
            switch (cmd) {
            case FOLLOWER_UI_COMMAND_INVENTORY:
                enabled = inven_ui_can_open_inven(follower_ui_commander_obj, follower_ui_subordinate_obj);
                break;
            case FOLLOWER_UI_COMMAND_WAIT:
                enabled = (obj_field_int32_get(follower_ui_subordinate_obj, OBJ_F_SPELL_FLAGS) & OSF_MIND_CONTROLLED) == 0;
                break;
            default:
                enabled = true;
                break;
            }

            if (enabled) {
                if (cmd == highlighted_cmd) {
                    tig_font_push(follower_ui_drop_down_menu_item_highlighted_color);
                } else {
                    tig_font_push(follower_ui_drop_down_menu_item_normal_color);
                }
            } else {
                tig_font_push(follower_ui_drop_down_menu_item_disabled_color);
            }

            tig_window_text_write(follower_ui_drop_down_menu_window, mes_file_entry.str, &rect);
            tig_font_pop();
        }

        rect.y += FOLLOWER_UI_DROP_DOWN_MENU_ENTRY_HEIGHT;
    }
}
