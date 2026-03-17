#include "ui/anim_ui.h"

#include "game/critter.h"
#include "game/gamelib.h"
#include "game/gfade.h"
#include "game/light_scheme.h"
#include "game/player.h"
#include "game/timeevent.h"
#include "game/ui.h"
#include "ui/combat_ui.h"
#include "ui/compact_ui.h"
#include "ui/gameuilib.h"
#include "ui/intgame.h"
#include "ui/inven_ui.h"
#include "ui/mainmenu_ui.h"
#include "ui/sleep_ui.h"
#include "ui/slide_ui.h"
#include "ui/wmap_ui.h"

static bool sub_57D3B0(TimeEvent* timeevent);
static bool anim_ui_bkg_process_callback(TimeEvent* timeevent);
static bool ambient_lighting_is_enabled(void);
static bool ambient_lighting_process_callback(TimeEvent* timeevent);
static void ambient_lighting_reschedule(void);

// 0x5CB408
static bool ambient_lighting_enabled = true;

// 0x5CB40C
static int dword_5CB40C = -1;

// 0x5CB410
static int dword_5CB410 = -1;

// 0x57D240
bool anim_ui_init(GameInitInfo* init_info)
{
    DateTime datetime;
    TimeEvent timeevent;
    TimeEventFuncs callbacks;

    (void)init_info;

    callbacks.bkg_anim_func = anim_ui_bkg_process_callback;
    callbacks.worldmap_func = wmap_ui_bkg_process_callback;
    callbacks.ambient_lighting_func = ambient_lighting_process_callback;
    callbacks.sleeping_func = sleep_ui_process_callback;
    callbacks.clock_func = intgame_clock_process_callback;
    callbacks.mainmenu_func = mainmenu_ui_process_callback;
    callbacks.mp_ctrl_ui_func = NULL;
    timeevent_set_funcs(&callbacks);

    timeevent.type = TIMEEVENT_TYPE_AMBIENT_LIGHTING;
    sub_45A950(&datetime, 1);
    timeevent_add_delay(&timeevent, &datetime);

    return true;
}

// 0x57D2C0
void anim_ui_exit(void)
{
}

// 0x57D2D0
void anim_ui_reset(void)
{
    DateTime datetime;
    TimeEvent timeevent;

    timeevent.type = TIMEEVENT_TYPE_AMBIENT_LIGHTING;
    sub_45A950(&datetime, 1);
    timeevent_add_delay(&timeevent, &datetime);
}

// 0x57D300
bool anim_ui_save(TigFile* stream)
{
    (void)stream;

    return true;
}

// 0x57D310
bool anim_ui_load(GameLoadInfo* load_info)
{
    DateTime datetime;
    TimeEvent timeevent;

    (void)load_info;

    timeevent_clear_all_typed(TIMEEVENT_TYPE_AMBIENT_LIGHTING);
    timeevent.type = TIMEEVENT_TYPE_AMBIENT_LIGHTING;
    sub_45A950(&datetime, 3600000);
    timeevent_add_delay(&timeevent, &datetime);

    return true;
}

// 0x57D350
void anim_ui_event_add(int type, int param)
{
    anim_ui_event_add_delay(type, param, 50);
}

// 0x57D370
void anim_ui_event_add_delay(int type, int param, int milliseconds)
{
    DateTime datetime;
    TimeEvent timeevent;

    timeevent.type = TIMEEVENT_TYPE_BKG_ANIM;
    timeevent.params[0].integer_value = type;
    timeevent.params[1].integer_value = param;
    sub_45A950(&datetime, milliseconds);
    timeevent_add_delay(&timeevent, &datetime);
}

// 0x57D3B0
bool sub_57D3B0(TimeEvent* timeevent)
{
    return timeevent->params[0].integer_value == dword_5CB40C
        && timeevent->params[1].integer_value == dword_5CB410;
}

// 0x57D3E0
void anim_ui_event_remove(int type, int param)
{
    dword_5CB40C = type;
    dword_5CB410 = param;
    // FIX: Original code uses `type` as timeevent type which is obviously wrong.
    timeevent_clear_one_ex(TIMEEVENT_TYPE_BKG_ANIM, sub_57D3B0);
    dword_5CB40C = -1;
    dword_5CB410 = -1;
}

// 0x57D410
bool anim_ui_bkg_process_callback(TimeEvent* timeevent)
{
    FadeData fade_data;

    switch (timeevent->params[0].integer_value) {
    case ANIM_UI_EVENT_TYPE_UPDATE_HEALTH_BAR:
        intgame_draw_bar(INTGAME_BAR_HEALTH);
        break;
    case ANIM_UI_EVENT_TYPE_UPDATE_FATIGUE_BAR:
        intgame_draw_bar(INTGAME_BAR_FATIGUE);
        break;
    case ANIM_UI_EVENT_TYPE_2:
    case ANIM_UI_EVENT_TYPE_3:
    case ANIM_UI_EVENT_TYPE_4:
    case ANIM_UI_EVENT_TYPE_5:
    case ANIM_UI_EVENT_TYPE_6:
    case ANIM_UI_EVENT_TYPE_7:
        break;
    case ANIM_UI_EVENT_TYPE_ROTATE_INTERFACE:
        iso_interface_window_set_animated(timeevent->params[1].integer_value);
        break;
    case ANIM_UI_EVENT_TYPE_END_DEATH:
        if (critter_is_dead(player_get_local_pc_obj())) {
            if (inven_ui_drag_item_obj_get() != OBJ_HANDLE_NULL) {
                sub_575770();
            }
            if (mainmenu_ui_is_active()) {
                sub_5412D0();
                anim_ui_event_add_delay(ANIM_UI_EVENT_TYPE_END_DEATH, -1, 300);
            } else {
                slide_ui_start(SLIDE_UI_TYPE_DEATH);

                tig_debug_printf("DEATH: Resetting game!\n");
                gamelib_reset();
                gameuilib_reset();
                mainmenu_ui_start(MM_TYPE_DEFAULT);

                fade_data.flags = FADE_IN;
                fade_data.duration = 2.0f;
                fade_data.steps = 48;
                gfade_run(&fade_data);
            }
        }
        break;
    case ANIM_UI_EVENT_TYPE_END_GAME:
        if (inven_ui_drag_item_obj_get() != OBJ_HANDLE_NULL) {
            sub_575770();
        }
        if (mainmenu_ui_is_active()) {
            sub_5412D0();
            anim_ui_event_add_delay(ANIM_UI_EVENT_TYPE_END_GAME, -1, 300);
        } else {
            slide_ui_start(SLIDE_UI_TYPE_END_GAME);

            tig_debug_printf("EndGame: Resetting game!\n");
            gamelib_reset();
            gameuilib_reset();
            mainmenu_ui_start(MM_TYPE_DEFAULT);

            fade_data.flags = FADE_IN;
            fade_data.duration = 2.0f;
            fade_data.steps = 48;
            gfade_run(&fade_data);
        }
        break;
    case ANIM_UI_EVENT_TYPE_REFRESH_COMBAT_UI:
        combat_ui_refresh();
        break;
    case ANIM_UI_EVENT_TYPE_END_RANDOM_ENCOUNTER:
        wmap_ui_encounter_end();
        break;
    case ANIM_UI_EVENT_TYPE_HIDE_COMPACT_UI:
        compact_ui_message_window_hide();
        break;
    default:
        tig_debug_printf("AnimUI: anim_ui_bkg_process_callback: ERROR: Failed to match event type!\n");
        break;
    }

    return true;
}

// 0x57D620
void ambient_lighting_enable(void)
{
    if (!ambient_lighting_enabled) {
        ambient_lighting_enabled = true;
        ambient_lighting_reschedule();
    }
}

// 0x57D640
void ambient_lighting_disable(void)
{
    ambient_lighting_enabled = false;
}

// 0x57D650
bool ambient_lighting_is_enabled(void)
{
    return ambient_lighting_enabled;
}

// 0x57D660
bool ambient_lighting_process_callback(TimeEvent* timeevent)
{
    int hour;
    DateTime datetime;
    TimeEvent next_timeevent;

    (void)timeevent;

    hour = datetime_current_hour();
    if (ambient_lighting_is_enabled()) {
        light_scheme_set_hour(hour);
    }

    timeevent_clear_all_typed(TIMEEVENT_TYPE_AMBIENT_LIGHTING);
    next_timeevent.type = TIMEEVENT_TYPE_AMBIENT_LIGHTING;
    sub_45A950(&datetime, 3600000);
    timeevent_add_delay(&next_timeevent, &datetime);

    return true;
}

// 0x57D6C0
void ambient_lighting_reschedule(void)
{
    int hour;
    DateTime datetime;
    TimeEvent timeevent;

    hour = datetime_current_hour();
    light_scheme_set_hour(hour);
    timeevent_clear_all_typed(TIMEEVENT_TYPE_AMBIENT_LIGHTING);
    timeevent.type = TIMEEVENT_TYPE_AMBIENT_LIGHTING;
    sub_45A950(&datetime, 3600000);
    timeevent_add_delay(&timeevent, &datetime);
}
