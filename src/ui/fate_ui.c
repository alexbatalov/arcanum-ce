#include "ui/fate_ui.h"

#include "game/critter.h"
#include "game/fate.h"
#include "game/hrp.h"
#include "game/mes.h"
#include "ui/charedit_ui.h"
#include "ui/intgame.h"
#include "ui/types.h"

static void fate_ui_create();
static void fate_ui_destroy();
static bool fate_ui_message_filter(TigMessage* msg);
static void fate_ui_handle_fate_resolved(int64_t obj, int fate);

/**
 * Handle to the parent window.
 *
 * 0x5CAAE0
 */
static tig_window_handle_t fate_ui_window = TIG_WINDOW_HANDLE_INVALID;

/**
 * Fate button configurations.
 *
 * 0x5CAAE8
 */
static UiButtonInfo fate_ui_buttons[FATE_COUNT] = {
    /*                 FATE_FULL_HEAL */ { 223, 4, 293, TIG_BUTTON_HANDLE_INVALID },
    /*       FATE_FORCE_GOOD_REACTION */ { 223, 22, 293, TIG_BUTTON_HANDLE_INVALID },
    /*                  FATE_CRIT_HIT */ { 223, 40, 293, TIG_BUTTON_HANDLE_INVALID },
    /*                 FATE_CRIT_MISS */ { 223, 58, 293, TIG_BUTTON_HANDLE_INVALID },
    /*       FATE_SAVE_AGAINST_MAGICK */ { 223, 76, 293, TIG_BUTTON_HANDLE_INVALID },
    /*          FATE_SPELL_AT_MAXIMUM */ { 223, 94, 293, TIG_BUTTON_HANDLE_INVALID },
    /*     FATE_CRIT_SUCCESS_GAMBLING */ { 223, 112, 293, TIG_BUTTON_HANDLE_INVALID },
    /*         FATE_CRIT_SUCCESS_HEAL */ { 223, 130, 293, TIG_BUTTON_HANDLE_INVALID },
    /*  FATE_CRIT_SUCCESS_PICK_POCKET */ { 223, 148, 293, TIG_BUTTON_HANDLE_INVALID },
    /*       FATE_CRIT_SUCCESS_REPAIR */ { 223, 166, 293, TIG_BUTTON_HANDLE_INVALID },
    /*   FATE_CRIT_SUCCESS_PICK_LOCKS */ { 223, 184, 293, TIG_BUTTON_HANDLE_INVALID },
    /* FATE_CRIT_SUCCESS_DISARM_TRAPS */ { 223, 202, 293, TIG_BUTTON_HANDLE_INVALID },
};

/**
 * Font for rendering fate labels.
 *
 * 0x680ED0
 */
static tig_font_handle_t fate_ui_font;

/**
 * 0x680ED8
 */
static int64_t fate_ui_obj;

/**
 * "fate_ui.mes"
 *
 * 0x680EE0
 */
static mes_file_handle_t fate_ui_mes;

/**
 * Flag indicating whether the fate UI system is initialized.
 *
 * 0x680EE4
 */
static bool fate_ui_initialized;

/**
 * Flag indicating whether the fate UI window is created.
 *
 * 0x680EE8
 */
static bool fate_ui_created;

/**
 * Called when the game is initialized.
 *
 * 0x56FAE0
 */
bool fate_ui_init(GameInitInfo* init_info)
{
    TigFont font_info;

    (void)init_info;

    // Load fate button labels (required).
    if (!mes_load("mes\\fate_ui.mes", &fate_ui_mes)) {
        return false;
    }

    // Create white font for rendering labels.
    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_info, &fate_ui_font);

    // Bind fate change handler.
    fate_set_callback(fate_ui_handle_fate_resolved);

    fate_ui_initialized = true;

    return true;
}

/**
 * Called when the game is being reset.
 *
 * 0x56FBB0
 */
void fate_ui_reset()
{
    if (fate_ui_created) {
        fate_ui_close();
    }
}

/**
 * Called when the game shuts down.
 *
 * 0x56FBC0
 */
void fate_ui_exit()
{
    mes_unload(fate_ui_mes);
    tig_font_destroy(fate_ui_font);
    fate_ui_initialized = false;
}

/**
 * Toggles the fate UI.
 *
 * This function shows or hides the fate UI when the fate button in the top bar
 * is clicked or the `W` key is pressed.
 *
 * The `obj` is mandatory for presenting fate UI and should be the local PC.
 *
 * 0x56FBF0
 */
void fate_ui_toggle(int64_t obj)
{
    // Close fate UI if it's currently visible.
    if (fate_ui_created) {
        fate_ui_close();
        return;
    }

    // Make sure PC object is specified and it's not dead.
    if (obj == OBJ_HANDLE_NULL
        || critter_is_dead(obj)) {
        return;
    }

    fate_ui_obj = obj;

    // Proceed to create the UI.
    fate_ui_create();
}

/**
 * Closes the fate UI.
 *
 * 0x56FC40
 */
void fate_ui_close()
{
    if (fate_ui_created) {
        fate_ui_destroy();
        fate_ui_obj = OBJ_HANDLE_NULL;
    }
}

/**
 * Creates the fate UI window.
 *
 * 0x56FC70
 */
void fate_ui_create()
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigRect window_rect;
    TigRect label_rect;
    TigWindowData window_data;
    TigArtBlitInfo blit_info;
    MesFileEntry mes_file_entry;
    int fate;

    // Skip if UI is already created.
    if (fate_ui_created) {
        return;
    }

    if (tig_art_interface_id_create(292, 0, 0, 0, &art_id) != TIG_OK) {
        return;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    // Set up window properties.
    window_rect.x = 0;
    window_rect.y = 41;
    window_rect.width = art_frame_data.width;
    window_rect.height = art_frame_data.height;

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect = window_rect;
    window_data.message_filter = fate_ui_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_TOP);

    if (tig_window_create(&window_data, &fate_ui_window) != TIG_OK) {
        tig_debug_printf("fate_ui_create: ERROR: window create failed!\n");
        exit(EXIT_FAILURE); // FIX: Proper exit code.
    }

    // Draw background.
    window_rect.x = 0;
    window_rect.y = 0;

    blit_info.flags = 0;
    blit_info.art_id = art_id;
    blit_info.src_rect = &window_rect;
    blit_info.dst_rect = &window_rect;
    tig_window_blit_art(fate_ui_window, &blit_info);

    tig_font_push(fate_ui_font);

    // Set up initial label rect.
    label_rect.x = 11;
    label_rect.y = 3;
    label_rect.width = window_rect.width - 11;
    label_rect.height = 18;

    // Write fate button label from the message file.
    for (fate = 0; fate < FATE_COUNT; fate++) {
        mes_file_entry.num = fate;
        mes_get_msg(fate_ui_mes, &mes_file_entry);
        tig_window_text_write(fate_ui_window, mes_file_entry.str, &label_rect);
        label_rect.y += label_rect.height;
    }

    tig_font_pop();

    // Create fate buttons.
    for (fate = 0; fate < FATE_COUNT; fate++) {
        intgame_button_create_ex(fate_ui_window,
            &window_rect,
            &(fate_ui_buttons[fate]),
            fate_is_activated(fate_ui_obj, fate)
                ? TIG_BUTTON_ON
                : TIG_BUTTON_TOGGLE);
    }

    fate_ui_created = true;
}

/**
 * Destroys the fate UI window.
 *
 * 0x56FE40
 */
void fate_ui_destroy()
{
    if (fate_ui_created) {
        if (tig_window_destroy(fate_ui_window) == TIG_OK) {
            fate_ui_window = TIG_WINDOW_HANDLE_INVALID;
        }
        fate_ui_created = false;
    }
}

/**
 * Called when an event occurred.
 *
 * 0x56FE70
 */
bool fate_ui_message_filter(TigMessage* msg)
{
    int fate;

    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        for (fate = 0; fate < FATE_COUNT; fate++) {
            if (fate_ui_buttons[fate].button_handle == msg->data.button.button_handle) {
                if (msg->data.button.state == TIG_BUTTON_STATE_PRESSED) {
                    // Button is pressed - attempt to activate appropriate fate.
                    if (fate_activate(fate_ui_obj, fate)) {
                        // Refresh UI.
                        iso_interface_refresh();
                    } else {
                        // Fate was not activated - simulate button release by
                        // programatically changing state so that appropriate
                        // art is displayed.
                        tig_button_state_change(fate_ui_buttons[fate].button_handle, TIG_BUTTON_STATE_RELEASED);

                        // Since the user just pressed this button the mouse
                        // cursor is within button bounds - mark it as
                        // highlighted.
                        tig_button_state_change(fate_ui_buttons[fate].button_handle, TIG_BUTTON_STATE_MOUSE_INSIDE);
                    }
                } else if (msg->data.button.state == TIG_BUTTON_STATE_RELEASED) {
                    // Button is released - attempt to deactivate appropriate
                    // fate.
                    if (fate_deactivate(fate_ui_obj, fate)) {
                        // Refresh UI.
                        iso_interface_refresh();
                    }
                }

                return true;
            }
        }
        break;
    case TIG_MESSAGE_KEYBOARD:
        // Pressing space closes fate UI.
        if (msg->data.keyboard.scancode == SDL_SCANCODE_SPACE
            && msg->data.keyboard.pressed) {
            fate_ui_close();
            return true;
        }
        break;
    default:
        break;
    }

    return false;
}

/**
 * Called when fate has been resolved.
 *
 * 0x56FF40
 */
void fate_ui_handle_fate_resolved(int64_t obj, int fate)
{
    if (fate_ui_obj == obj) {
        tig_button_state_change(fate_ui_buttons[fate].button_handle, TIG_BUTTON_STATE_RELEASED);
    }

    charedit_refresh();
}
