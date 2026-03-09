#include "ui/sleep_ui.h"

#include "game/combat.h"
#include "game/critter.h"
#include "game/gfade.h"
#include "game/hrp.h"
#include "game/magictech.h"
#include "game/mes.h"
#include "game/object.h"
#include "game/player.h"
#include "game/script.h"
#include "game/sector.h"
#include "game/timeevent.h"
#include "game/townmap.h"
#include "game/ui.h"
#include "ui/anim_ui.h"
#include "ui/intgame.h"

typedef enum SleepUiOption {
    SLEEP_UI_OPTION_ONE_HOUR,
    SLEEP_UI_OPTION_TWO_HOURS,
    SLEEP_UI_OPTION_FOUR_HOURS,
    SLEEP_UI_OPTION_EIGHT_HOURS,
    SLEEP_UI_OPTION_ONE_DAY,
    SLEEP_UI_OPTION_UNTIL_MORNING,
    SLEEP_UI_OPTION_UNTIL_EVENING,
    SLEEP_UI_OPTION_UNTIL_HEALED,
    SLEEP_UI_OPTION_COUNT,
} SleepUiOption;

/**
 * As opposed to sleeping, waiting does not recover health, so "Until Healed"
 * option is not availble.
 */
#define SLEEP_UI_WAIT_OPTION_COUNT (SLEEP_UI_OPTION_COUNT - 1)

/**
 * Special duration value indicating the sleeping should last until 7 AM.
 */
#define UNTIL_MORNING_VAL (-1)

/**
 * Special duration value indicating the sleeping should last until 8 PM.
 */
#define UNTIL_EVENING_VAL (-2)

/**
 * Special duration value indicating the sleeping should last until PC is
 * fully healed.
 */
#define UNTIL_HEALED_VAL (-3)

static bool sleep_ui_create(void);
static void sleep_ui_destroy(void);
static bool sleep_ui_message_filter(TigMessage* msg);
static void sleep_ui_fall_asleep(void);
static void sleep_ui_wake_up(void);
static void sleep_ui_notify_leader_sleeping(void);

/**
 * Handle to the parent window.
 *
 * 0x5CB2B8
 */
static tig_window_handle_t sleep_ui_window = TIG_WINDOW_HANDLE_INVALID;

/**
 * Sleep button configurations.
 *
 * 0x5CB2C0
 */
static UiButtonInfo sleep_ui_buttons[SLEEP_UI_OPTION_COUNT] = {
    /*      SLEEP_UI_OPTION_ONE_HOUR */ { 8, 3, 293, TIG_BUTTON_HANDLE_INVALID },
    /*     SLEEP_UI_OPTION_TWO_HOURS */ { 8, 21, 293, TIG_BUTTON_HANDLE_INVALID },
    /*    SLEEP_UI_OPTION_FOUR_HOURS */ { 8, 39, 293, TIG_BUTTON_HANDLE_INVALID },
    /*   SLEEP_UI_OPTION_EIGHT_HOURS */ { 8, 57, 293, TIG_BUTTON_HANDLE_INVALID },
    /*       SLEEP_UI_OPTION_ONE_DAY */ { 8, 75, 293, TIG_BUTTON_HANDLE_INVALID },
    /* SLEEP_UI_OPTION_UNTIL_MORNING */ { 8, 93, 293, TIG_BUTTON_HANDLE_INVALID },
    /* SLEEP_UI_OPTION_UNTIL_EVENING */ { 8, 111, 293, TIG_BUTTON_HANDLE_INVALID },
    /*  SLEEP_UI_OPTION_UNTIL_HEALED */ { 8, 129, 293, TIG_BUTTON_HANDLE_INVALID },
};

/**
 * Array mapping sleep options to their respective durations in hours, with
 * negative values indicate special duration.
 *
 * 0x5CB340
 */
static int sleep_ui_hours[SLEEP_UI_OPTION_COUNT] = {
    /*      SLEEP_UI_OPTION_ONE_HOUR */ 1,
    /*     SLEEP_UI_OPTION_TWO_HOURS */ 2,
    /*    SLEEP_UI_OPTION_FOUR_HOURS */ 4,
    /*   SLEEP_UI_OPTION_EIGHT_HOURS */ 8,
    /*       SLEEP_UI_OPTION_ONE_DAY */ 24,
    /* SLEEP_UI_OPTION_UNTIL_MORNING */ UNTIL_MORNING_VAL,
    /* SLEEP_UI_OPTION_UNTIL_EVENING */ UNTIL_EVENING_VAL,
    /*  SLEEP_UI_OPTION_UNTIL_HEALED */ UNTIL_HEALED_VAL,
};

/**
 * Font for rendering sleep option labels.
 *
 * 0x6834A0
 */
static tig_font_handle_t sleep_ui_font;

/**
 * 0x6834A8
 */
static int64_t sleep_ui_obj;

/**
 * Flag indicating whether the sleep UI is in wait mode.
 *
 * A wait mode is activated when the player is in town which has "waitable"
 * setting enabled. Waiting advances time without healing.
 *
 * 0x6834B0
 */
static bool sleep_ui_wait_mode;

/**
 * 0x6834B8
 */
static Ryan sleep_ui_bed_obj_safe;

/**
 * Flag indicating whether the player is currently sleeping.
 *
 * 0x6834E0
 */
static bool is_sleeping;

/**
 * "sleepui.mes"
 *
 * 0x6834E4
 */
static mes_file_handle_t sleep_ui_mes_file;

/**
 * Flag indicating whether the player is sleeping in a bed.
 *
 * 0x6834E8
 */
static bool sleep_ui_using_bed;

/**
 * Flag indicating whether the sleep UI system has been initialized.
 *
 * 0x6834EC
 */
static bool sleep_ui_initialized;

/**
 * Flag indicating whether the sleep UI window is currently created.
 *
 * 0x6834F0
 */
static bool sleep_ui_active;

static IsoInvalidateRectFunc* sleep_ui_invalidate_rect;
static IsoRedrawFunc* sleep_ui_draw;

/**
 * Called when the game is initialized.
 *
 * 0x57B080
 */
bool sleep_ui_init(GameInitInfo* init_info)
{
    TigFont font;

    // CE: Keep invalidate/redraw functions for proper wake up.
    sleep_ui_invalidate_rect = init_info->invalidate_rect_func;
    sleep_ui_draw = init_info->draw_func;

    // Load sleep option labels (required).
    if (!mes_load("mes\\sleepUI.mes", &sleep_ui_mes_file)) {
        return false;
    }

    // Create white font for rendering labels.
    font.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font.art_id));
    font.str = NULL;
    font.color = tig_color_make(255, 255, 255);
    tig_font_create(&font, &sleep_ui_font);

    is_sleeping = false;
    sleep_ui_wait_mode = false;
    sleep_ui_initialized = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x57B140
 */
void sleep_ui_exit(void)
{
    mes_unload(sleep_ui_mes_file);
    tig_font_destroy(sleep_ui_font);
    sleep_ui_initialized = false;
    is_sleeping = false;
    sleep_ui_wait_mode = false;
}

/**
 * Called when the game is being reset.
 *
 * 0x57B170
 */
void sleep_ui_reset(void)
{
    if (sleep_ui_active) {
        sleep_ui_close();
    }
}

/**
 * Toggles the sleep UI.
 *
 * This function shows or hides the sleep UI when the sleep button in the top
 * bar is clicked, or the `S` key is pressed.
 *
 * The `bed_obj` is optional and is populated if the sleep UI is activated when
 * the player clicks and reaches bed.
 *
 * 0x57B180
 */
void sleep_ui_toggle(int64_t bed_obj)
{
    int64_t pc_obj;
    int64_t loc;
    int64_t sec;
    int townmap;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    bool halt;

    pc_obj = player_get_local_pc_obj();

    // Ensure the player is not dead.
    if (critter_is_dead(pc_obj)) {
        return;
    }

    // Prevent sleep UI in network games.
    if (tig_net_is_active()) {
        return;
    }

    // Close sleep UI if it's currently visible.
    if (sleep_ui_active) {
        sleep_ui_close();
        return;
    }

    // Ensure the player is not unconscious.
    if (critter_is_unconscious(pc_obj)) {
        mes_file_entry.num = 11; // "You are unconscious already."
        mes_get_msg(sleep_ui_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_EXCLAMATION;
        ui_message.str = mes_file_entry.str;
        sub_460630(&ui_message);

        sleep_ui_wait_mode = false;
        return;
    }

    // When the player is not using bed for sleeping (which permits sleeping
    // anywhere the bed is located), it must be either in the wilderness area
    // (whos sector does not belong to a town), or the town must be "waitable"
    // (as indicated by townmap system thru `townmap.mes`).
    if (bed_obj == OBJ_HANDLE_NULL) {
        loc = obj_field_int64_get(pc_obj, OBJ_F_LOCATION);
        sec = sector_id_from_loc(loc);
        townmap = townmap_get(sec);
        if (townmap != TOWNMAP_NONE) {
            if (!townmap_is_waitable(townmap)) {
                // The player is in town which does not permit waiting.
                mes_file_entry.num = 20; // "You cannot wait here."
                mes_get_msg(sleep_ui_mes_file, &mes_file_entry);

                ui_message.type = UI_MSG_TYPE_EXCLAMATION;
                ui_message.str = mes_file_entry.str;
                sub_460630(&ui_message);
                return;
            }

            // Indicate sleep UI is in "wait" mode.
            sleep_ui_wait_mode = true;
        } else {
            // The player is not in the town - sleeping is allowed.
        }
    }

    // When the player is in combat mode, attempt to deactivate it.
    if (combat_critter_is_combat_mode_active(pc_obj)) {
        if (!combat_can_exit_combat_mode(pc_obj)) {
            mes_file_entry.num = 9; // "You cannot sleep with enemies near."
            mes_get_msg(sleep_ui_mes_file, &mes_file_entry);

            ui_message.type = UI_MSG_TYPE_EXCLAMATION;
            ui_message.str = mes_file_entry.str;
            sub_460630(&ui_message);
            sleep_ui_wait_mode = false;
            return;
        }
        combat_critter_deactivate_combat_mode(pc_obj);
    }

    if (bed_obj != OBJ_HANDLE_NULL) {
        // Check if bed is usable.
        if (object_script_execute(pc_obj, bed_obj, OBJ_HANDLE_NULL, SAP_USE, 0) == 1) {
            mes_file_entry.num = 8; // "You cannot sleep here. Find an inn and pay for a bed."
            mes_get_msg(sleep_ui_mes_file, &mes_file_entry);

            ui_message.type = UI_MSG_TYPE_EXCLAMATION;
            ui_message.str = mes_file_entry.str;
            sub_460630(&ui_message);
            sleep_ui_wait_mode = false;
            return;
        }

        // Check if bed is empty.
        if (!critter_enter_bed(pc_obj, bed_obj)) {
            mes_file_entry.num = 10; // "That bed is occupied."
            mes_get_msg(sleep_ui_mes_file, &mes_file_entry);

            ui_message.type = UI_MSG_TYPE_EXCLAMATION;
            ui_message.str = mes_file_entry.str;
            sub_460630(&ui_message);
            sleep_ui_wait_mode = false;
            return;
        }
    }

    // NOTE: I'm not sure how to make it pretty without goto.
    halt = true;
    do {
        // TODO: Unclear.
        if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
            break;
        }

        // TODO: Unclear.
        if (!intgame_mode_set(INTGAME_MODE_SLEEP)) {
            break;
        }

        // Set up sleep UI state.
        sleep_ui_obj = pc_obj;

        if (bed_obj != OBJ_HANDLE_NULL) {
            sleep_ui_using_bed = true;
            sub_443EB0(bed_obj, &sleep_ui_bed_obj_safe);
        } else {
            sleep_ui_using_bed = false;
        }

        // Create the sleep UI window.
        if (!sleep_ui_create()) {
            break;
        }

        halt = false;
    } while (0);

    // Clean up if UI set up failed.
    if (halt) {
        if (bed_obj != OBJ_HANDLE_NULL) {
            critter_leave_bed(pc_obj, bed_obj);
        }
        sleep_ui_wait_mode = false;
    }
}

/**
 * Closes the sleep UI.
 *
 * 0x57B450
 */
void sleep_ui_close(void)
{
    int64_t bed_obj;

    if (!sleep_ui_active) {
        return;
    }

    if (intgame_mode_set(INTGAME_MODE_MAIN)) {
        timeevent_clear_all_typed(TIMEEVENT_TYPE_SLEEPING);
        sleep_ui_wake_up();
        sleep_ui_destroy();

        if (sleep_ui_using_bed) {
            if (sub_443F80(&bed_obj, &sleep_ui_bed_obj_safe)
                && bed_obj != OBJ_HANDLE_NULL) {
                critter_leave_bed(sleep_ui_obj, bed_obj);
            }
        }

        // Reset sleep UI state.
        sleep_ui_using_bed = false;
        sleep_ui_obj = OBJ_HANDLE_NULL;
        sleep_ui_wait_mode = false;
    }
}

/**
 * Checks if the sleep UI is currently active.
 *
 * 0x57B4E0
 */
bool sleep_ui_is_active(void)
{
    return sleep_ui_active;
}

/**
 * Creates the sleep UI window.
 *
 * 0x57B4F0
 */
bool sleep_ui_create(void)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigWindowData window_data;
    TigRect rect;
    TigRect label_rect;
    int cnt;
    int index;
    MesFileEntry mes_file_entry;

    // Skip if UI is already created.
    if (sleep_ui_active) {
        return false;
    }

    // "sleep_base.art"
    if (tig_art_interface_id_create(565, 0, 0, 0, &art_id) != TIG_OK) {
        return false;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    // Set up window properties.
    rect.x = 573;
    rect.y = 41;
    rect.width = art_frame_data.width;
    rect.height = art_frame_data.height;

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.message_filter = sleep_ui_message_filter;
    window_data.rect = rect;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_TOP);

    if (tig_window_create(&window_data, &sleep_ui_window) != TIG_OK) {
        tig_debug_printf("sleep_ui_create: ERROR: window create failed!\n");
        exit(EXIT_FAILURE); // FIX: Proper exit code.
    }

    // Draw background.
    rect.x = 0;
    rect.y = 0;

    art_blit_info.art_id = art_id;
    art_blit_info.flags = 0;
    art_blit_info.src_rect = &rect;
    art_blit_info.dst_rect = &rect;
    tig_window_blit_art(sleep_ui_window, &art_blit_info);

    tig_font_push(sleep_ui_font);

    // Set up initial label rect.
    label_rect.x = 36;
    label_rect.y = 2;
    label_rect.width = rect.width - 36;
    label_rect.height = 18;

    // Write sleep option labels from the message file.
    cnt = sleep_ui_wait_mode ? SLEEP_UI_WAIT_OPTION_COUNT : SLEEP_UI_OPTION_COUNT;
    for (index = 0; index < cnt; index++) {
        // Waiting labels use "Advance" while non-waitable labels use "Sleep",
        // i.e. "Advance Time One Hour" vs. "Sleep For One Hour".
        mes_file_entry.num = sleep_ui_wait_mode ? index + 12 : index;
        mes_get_msg(sleep_ui_mes_file, &mes_file_entry);
        tig_window_text_write(sleep_ui_window, mes_file_entry.str, &label_rect);
        label_rect.y += label_rect.height;
    }

    tig_font_pop();

    // Create sleep option buttons.
    for (index = 0; index < cnt; index++) {
        intgame_button_create_ex(sleep_ui_window, &rect, &(sleep_ui_buttons[index]), true);
    }

    sleep_ui_active = true;

    return true;
}

/**
 * Destroys the sleep UI window.
 *
 * 0x57B6E0
 */
void sleep_ui_destroy(void)
{
    if (!sleep_ui_active) {
        return;
    }

    tig_window_destroy(sleep_ui_window);
    sleep_ui_active = false;
}

/**
 * Called when an event occurred.
 *
 * 0x57B710
 */
bool sleep_ui_message_filter(TigMessage* msg)
{
    int cnt;
    int index;
    TimeEvent timeevent;
    DateTime datetime;

    // Only handle button messages.
    if (msg->type != TIG_MESSAGE_BUTTON) {
        return false;
    }

    // Only handle button release events.
    if (msg->data.button.state != TIG_BUTTON_STATE_RELEASED) {
        return false;
    }

    // Find the index of the clicked button (which corresponds to sleep option).
    cnt = sleep_ui_wait_mode ? SLEEP_UI_WAIT_OPTION_COUNT : SLEEP_UI_OPTION_COUNT;
    for (index = 0; index < cnt; index++) {
        if (msg->data.button.button_handle == sleep_ui_buttons[index].button_handle) {
            break;
        }
    }

    if (index >= cnt) {
        return false;
    }

    // Clear existing sleep events.
    timeevent_clear_all_typed(TIMEEVENT_TYPE_SLEEPING);

    // Set up a new sleeping event.
    timeevent.type = TIMEEVENT_TYPE_SLEEPING;
    if (sleep_ui_hours[index] >= 0) {
        // Non-negative values indicate duration in hours.
        timeevent.params[0].integer_value = 3600000 * sleep_ui_hours[index] / 3600000;
    } else {
        // Negative values indicate special duration, keep it as-is.
        timeevent.params[0].integer_value = sleep_ui_hours[index];
    }

    // Schedule the first sleep event in 50 ms.
    sub_45A950(&datetime, 50);
    timeevent_add_delay(&timeevent, &datetime);

    // Fade out.
    sleep_ui_fall_asleep();

    // Let followers know that PC goes sleeping.
    sleep_ui_notify_leader_sleeping();

    // End all spells PC is currently maintaining.
    magictech_demaintain_spells(sleep_ui_obj);

    return true;
}

/**
 * Called by timeevent scheduler.
 *
 * 0x57B7F0
 */
bool sleep_ui_process_callback(TimeEvent* timeevent)
{
    DateTime datetime;
    TimeEvent next_timeevent;
    ObjectList objects;
    ObjectNode* node;
    int hours;

    // Advance game time by one hour.
    sub_45A950(&datetime, 3600000);
    timeevent_inc_datetime(&datetime);

    // Apply healing if not in wait mode.
    if (!sleep_ui_wait_mode) {
        // Sleeping in bed has healing ratio x2.
        hours = sleep_ui_using_bed ? 2 : 1;

        // Heal player.
        critter_resting_heal(sleep_ui_obj, hours);

        // Heal all followers.
        object_list_all_followers(sleep_ui_obj, &objects);
        node = objects.head;
        while (node != NULL) {
            critter_resting_heal(node->obj, hours);
            node = node->next;
        }
        object_list_destroy(&objects);
    }

    if (critter_is_dead(sleep_ui_obj)) {
        // Unfortunately, the player died while in sleep. A wake-up is needed to
        // remove the fading effect, only to fade out again to the death screen.
        sleep_ui_wake_up();
        return true;
    }

    // Prepare next sleep timeevent.
    next_timeevent.type = TIMEEVENT_TYPE_SLEEPING;

    // Handle fixed-duration sleep continuation.
    if (timeevent->params[0].integer_value > 1) {
        // Decrement remaining number of hours to sleep.
        next_timeevent.params[0].integer_value = timeevent->params[0].integer_value - 1;

        // Schedule next event in 200ms.
        sub_45A950(&datetime, 200);
        timeevent_add_delay(&next_timeevent, &datetime);

        return true;
    }

    // Handle special duration sleep conditions.
    if ((timeevent->params[0].integer_value == UNTIL_MORNING_VAL
            && datetime_current_hour() != 7)
        || (timeevent->params[0].integer_value == UNTIL_EVENING_VAL
            && datetime_current_hour() != 20)
        || (timeevent->params[0].integer_value == UNTIL_HEALED_VAL
            && object_hp_damage_get(sleep_ui_obj) > 0)) {
        // Pass special duration value as-is.
        next_timeevent.params[0].integer_value = timeevent->params[0].integer_value;

        // Schedule next event in 200ms.
        sub_45A950(&datetime, 200);
        timeevent_add_delay(&next_timeevent, &datetime);
        return true;
    }

    // If we reached this point, either the sleep time has ended or the sleep
    // conditions are met. Continue the game.
    sleep_ui_wake_up();
    return true;
}

/**
 * Starts sleeping with a fade-out effect.
 *
 * 0x57B9E0
 */
void sleep_ui_fall_asleep(void)
{
    FadeData fade_data;

    if (is_sleeping) {
        return;
    }

    // Set up and run fade-out effect.
    fade_data.flags = 0;
    fade_data.duration = 2.0f;
    fade_data.steps = 48;
    fade_data.color = tig_color_make(0, 0, 0);
    gfade_run(&fade_data);

    // Disable ambient lighting updates.
    ambient_lighting_disable();

    is_sleeping = true;
}

/**
 * Ends sleeping with a fade-in effect and closes the UI.
 *
 * 0x57BA70
 */
void sleep_ui_wake_up(void)
{
    FadeData fade_data;

    if (!is_sleeping) {
        return;
    }

    // Close the sleep UI.
    is_sleeping = false;
    sleep_ui_close();

    // Re-enable ambient lighting updates.
    ambient_lighting_enable();

    // CE: Update iso content while window is still faded. Once the fade is
    // removed, the world will appear updated, particularly ambient lighting.
    sleep_ui_invalidate_rect(NULL);
    sleep_ui_draw();
    tig_window_display();

    // Set up and run fade-in effect.
    fade_data.flags = FADE_IN;
    fade_data.duration = 2.0f;
    fade_data.steps = 48;
    gfade_run(&fade_data);
}

/**
 * Notifies all followers that the leader is sleeping.
 *
 * 0x57BAC0
 */
void sleep_ui_notify_leader_sleeping(void)
{
    ObjectList objects;
    ObjectNode* node;

    object_list_followers(sleep_ui_obj, &objects);

    node = objects.head;
    while (node != NULL) {
        object_script_execute(sleep_ui_obj, node->obj, OBJ_HANDLE_NULL, SAP_LEADER_SLEEPING, 0);
        node = node->next;
    }

    object_list_destroy(&objects);
}
