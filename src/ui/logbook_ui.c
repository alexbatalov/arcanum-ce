#include "ui/logbook_ui.h"

#include <stdio.h>

#include "game/background.h"
#include "game/bless.h"
#include "game/critter.h"
#include "game/curse.h"
#include "game/description.h"
#include "game/effect.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/location.h"
#include "game/logbook.h"
#include "game/mes.h"
#include "game/player.h"
#include "game/quest.h"
#include "game/reputation.h"
#include "game/rumor.h"
#include "game/snd.h"
#include "game/timeevent.h"
#include "game/ui.h"
#include "ui/intgame.h"
#include "ui/types.h"

enum LogbookUiButton {
    LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT,
    LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT,
    LOGBOOK_UI_BUTTON_COUNT,
};

enum LogbookUiTab {
    // Also includes hidden Bloopers and Quotes.
    LOGBOOK_UI_TAB_RUMORS_AND_NOTES,
    LOGBOOK_UI_TAB_QUESTS,
    LOGBOOK_UI_TAB_REPUTATIONS,
    LOGBOOK_UI_TAB_BLESSINGS_AND_CURSES,
    LOGBOOK_UI_TAB_KILLS_AND_INJURES,
    LOGBOOK_UI_TAB_BACKGROUND,
    LOGBOOK_UI_TAB_KEYS,
    LOGBOOK_UI_TAB_COUNT,
};

static void logbook_ui_create();
static void logbook_ui_destroy();
static bool logbook_ui_message_filter(TigMessage* msg);
static void logbook_ui_draw_panel(int art_num, bool preserve_page);
static void logbook_ui_switch_tab(int tab, bool preserve_page);
static void logbook_ui_turn_page_right();
static void logbook_ui_turn_page_left();
static void logbook_ui_refresh();
static int logbook_ui_draw_page_spread(int start_entry, int max_entry);
static int logbook_ui_draw_entry(int index, TigRect* rect, bool dry_run, bool can_truncate);
static int logbook_ui_draw_text(char* buffer, tig_font_handle_t font, TigRect* rect, bool dry_run, bool can_truncate, bool a6);
static void logbook_ui_load_data();
static void logbook_ui_format_rumor(char* buffer, int index);
static void logbook_ui_format_timestamp(char* buffer, int index);
static void logbook_ui_format_quest(char* buffer, int index);
static void logbook_ui_format_reputation(char* buffer, int index);
static void logbook_ui_format_blessing_or_curse(char* buffer, int index);
static void logbook_ui_format_kill_or_injury(char* buffer, int index);
static void logbook_ui_format_background(char* buffer, int index);
static void logbook_ui_format_key(char* buffer, int index);

// 0x5C33F0
static tig_window_handle_t logbook_ui_window = TIG_WINDOW_HANDLE_INVALID;

// 0x5C33F8
static TigRect logbook_ui_background_rects[2] = {
    { 0, 41, 800, 400 },
    { 150, 11, 501, 365 },
};

// 0x5C3418
static TigRect logbook_ui_portrait_rect = { 25, 24, 89, 89 };

// 0x5C3428
static UiButtonInfo logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_COUNT] = {
    { 213, 77, 272, TIG_BUTTON_HANDLE_INVALID },
    { 675, 77, 273, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C3448
static UiButtonInfo logbook_ui_tab_buttons[LOGBOOK_UI_TAB_COUNT] = {
    /*     LOGBOOK_UI_TAB_RUMORS_AND_NOTES */ { 696, 83, 265, TIG_BUTTON_HANDLE_INVALID },
    /*               LOGBOOK_UI_TAB_QUESTS */ { 696, 129, 267, TIG_BUTTON_HANDLE_INVALID },
    /*          LOGBOOK_UI_TAB_REPUTATIONS */ { 696, 175, 271, TIG_BUTTON_HANDLE_INVALID },
    /* LOGBOOK_UI_TAB_BLESSINGS_AND_CURSES */ { 695, 221, 266, TIG_BUTTON_HANDLE_INVALID },
    /*    LOGBOOK_UI_TAB_KILLS_AND_INJURES */ { 696, 267, 270, TIG_BUTTON_HANDLE_INVALID },
    /*           LOGBOOK_UI_TAB_BACKGROUND */ { 696, 313, 268, TIG_BUTTON_HANDLE_INVALID },
    /*                 LOGBOOK_UI_TAB_KEYS */ { 696, 359, 269, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C34B8
static TigRect logbook_ui_page_content_rects[2] = {
    { 222, 72, 215, 270 },
    { 468, 72, 215, 270 },
};

// 0x5C34D8
static TigRect logbook_ui_page_title_rects[2] = {
    { 236, 43, 187, 19 },
    { 482, 43, 187, 19 },
};

// 0x5C34F8
static TigRect logbook_ui_page_number_rects[2] = {
    { 222, 346, 215, 15 },
    { 468, 346, 215, 15 },
};

// 0x63CBF0
static int logbook_ui_line_height;

// 0x63CBF4
static int logbook_ui_quest_states[3000];

// 0x63FAD8
static int64_t logbook_ui_obj;

// 0x63FAE0
static tig_font_handle_t logbook_ui_font_quest_completed;

// 0x63FAE4
static int logbook_ui_entry_ids[3000];

// 0x6429C4
static tig_font_handle_t logbook_ui_font_struck;

// 0x6429C8
static tig_font_handle_t logbook_ui_font_page_numbers;

// 0x6429CC
static mes_file_handle_t quotes_mes_file;

// 0x6429D0
static tig_font_handle_t logbook_ui_font_curse;

// 0x6429D8
static DateTime logbook_ui_entry_datetimes[3000];

// 0x648798
static int logbook_ui_current_page;

// 0x64879C
static int logbook_ui_page_spread_starts[100];

// 0x64892C
static tig_font_handle_t logbook_ui_font_quest_failed;

// 0x648930
static tig_font_handle_t logbook_ui_font_header;

// 0x648934
static tig_font_handle_t logbook_ui_font_default;

// 0x64893C
static tig_font_handle_t logbook_ui_font_active;

// 0x648938
static int logbook_ui_entry_count;

// 0x648940
static int logbook_ui_kill_stats[LBK_COUNT];

// 0x648974
static int logbook_ui_last_visible_entry;

// 0x648978
static int logbook_ui_tab;

// 0x64897C
static int logbook_ui_art_num;

// 0x648980
static int logbook_ui_quotes_mode;

// 0x648984
static mes_file_handle_t logbook_ui_mes_file;

// 0x648988
static tig_font_handle_t logbook_ui_entry_fonts[3000];

// 0x64B868
static bool logbook_ui_initialized;

// 0x64B86C
static bool logbook_ui_created;

// 0x53EB10
bool logbook_ui_init(GameInitInfo* init_info)
{
    TigFont font_info;
    int index;

    (void)init_info;

    if (!mes_load("mes\\logbk_ui.mes", &logbook_ui_mes_file)) {
        return false;
    }

    if (!mes_load("mes\\quotes.mes", &quotes_mes_file)) {
        return false;
    }

    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_default);

    font_info.flags = TIG_FONT_CENTERED;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_page_numbers);

    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &logbook_ui_font_struck);

    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 120, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &logbook_ui_font_quest_completed);

    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(150, 0, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &logbook_ui_font_quest_failed);

    font_info.flags = TIG_FONT_CENTERED;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_header);

    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 150);
    tig_font_create(&font_info, &logbook_ui_font_active);

    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(150, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_curse);

    tig_font_push(logbook_ui_font_default);
    font_info.str = "test";
    font_info.width = 0;
    tig_font_measure(&font_info);
    logbook_ui_line_height = font_info.height;
    tig_font_pop();

    logbook_ui_art_num = -1;

    for (index = 0; index < 100; index++) {
        logbook_ui_page_spread_starts[index] = -1;
    }
    logbook_ui_page_spread_starts[0] = 0;

    logbook_ui_tab = LOGBOOK_UI_TAB_RUMORS_AND_NOTES;
    logbook_ui_obj = OBJ_HANDLE_NULL;
    logbook_ui_current_page = 1;
    logbook_ui_initialized = true;

    return true;
}

// 0x53EF40
void logbook_ui_exit()
{
    mes_unload(logbook_ui_mes_file);
    mes_unload(quotes_mes_file);
    tig_font_destroy(logbook_ui_font_default);
    tig_font_destroy(logbook_ui_font_page_numbers);
    tig_font_destroy(logbook_ui_font_struck);
    tig_font_destroy(logbook_ui_font_quest_completed);
    tig_font_destroy(logbook_ui_font_quest_failed);
    tig_font_destroy(logbook_ui_font_header);
    tig_font_destroy(logbook_ui_font_active);
    tig_font_destroy(logbook_ui_font_curse);
    logbook_ui_initialized = false;
}

// 0x53EFD0
void logbook_ui_reset()
{
    int index;

    if (logbook_ui_created) {
        logbook_ui_close();
    }

    logbook_ui_tab = LOGBOOK_UI_TAB_RUMORS_AND_NOTES;

    for (index = 0; index < 100; index++) {
        logbook_ui_page_spread_starts[index] = -1;
    }
    logbook_ui_page_spread_starts[0] = 0;

    logbook_ui_obj = OBJ_HANDLE_NULL;
    logbook_ui_art_num = -1;
    logbook_ui_current_page = 1;
}

// 0x53F020
void logbook_ui_open(int64_t obj)
{
    if (logbook_ui_created) {
        logbook_ui_close();
        return;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (critter_is_dead(player_get_local_pc_obj())) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_LOGBOOK)) {
        return;
    }

    logbook_ui_obj = obj;
    logbook_ui_create();
}

// 0x53F090
void logbook_ui_close()
{
    if (logbook_ui_created
        && intgame_mode_set(INTGAME_MODE_MAIN)) {
        logbook_ui_destroy();
        logbook_ui_obj = OBJ_HANDLE_NULL;
    }
}

// 0x53F0D0
bool logbook_ui_is_created()
{
    return logbook_ui_created;
}

// 0x53F0E0
void logbook_ui_create()
{
    TigArtBlitInfo blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int index;
    tig_button_handle_t button_handles[LOGBOOK_UI_TAB_COUNT];
    PcLens pc_lens;

    if (logbook_ui_created) {
        return;
    }

    if (!intgame_big_window_lock(logbook_ui_message_filter, &logbook_ui_window)) {
        tig_debug_printf("logbk_ui_create: ERROR: intgame_big_window_lock failed!\n");
        exit(EXIT_SUCCESS); // FIXME: Should be EXIT_FAILURE.
    }

    if (tig_kb_get_modifier(SDL_KMOD_LCTRL) && tig_kb_get_modifier(SDL_KMOD_LALT)) {
        logbook_ui_tab = LOGBOOK_UI_TAB_RUMORS_AND_NOTES;
        logbook_ui_current_page = 1;
        logbook_ui_quotes_mode = 1;
        gsound_play_sfx(SND_INTERFACE_BLESS, 1);
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = logbook_ui_background_rects[0].width;
    src_rect.height = logbook_ui_background_rects[0].height;

    dst_rect.x = 0;
    dst_rect.y = 0;
    dst_rect.width = src_rect.width;
    dst_rect.height = src_rect.height;

    blit_info.flags = 0;
    tig_art_interface_id_create(264, 0, 0, 0, &(blit_info.art_id));
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(logbook_ui_window, &blit_info);

    for (index = 0; index < LOGBOOK_UI_BUTTON_COUNT; index++) {
        intgame_button_create_ex(logbook_ui_window,
            &(logbook_ui_background_rects[0]),
            &(logbook_ui_page_buttons[index]),
            TIG_BUTTON_HIDDEN | TIG_BUTTON_MOMENTARY);
    }

    for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
        intgame_button_create_ex(logbook_ui_window,
            &(logbook_ui_background_rects[0]),
            &(logbook_ui_tab_buttons[index]),
            TIG_BUTTON_HIDDEN | TIG_BUTTON_MOMENTARY);
        button_handles[index] = logbook_ui_tab_buttons[index].button_handle;
    }
    tig_button_radio_group_create(LOGBOOK_UI_TAB_COUNT, button_handles, logbook_ui_tab);

    location_origin_set(obj_field_int64_get(logbook_ui_obj, OBJ_F_LOCATION));

    pc_lens.window_handle = logbook_ui_window;
    pc_lens.rect = &logbook_ui_portrait_rect;
    tig_art_interface_id_create(198, 0, 0, 0, &(pc_lens.art_id));
    intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);
    logbook_ui_draw_panel(261, true);
    ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, false);
    gsound_play_sfx(SND_INTERFACE_BOOK_OPEN, 1);

    logbook_ui_created = true;
}

// 0x53F2F0
void logbook_ui_destroy()
{
    if (!logbook_ui_created) {
        return;
    }

    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
    intgame_big_window_unlock();
    logbook_ui_window = TIG_WINDOW_HANDLE_INVALID;
    gsound_play_sfx(SND_INTERFACE_BOOK_CLOSE, 1);
    logbook_ui_created = false;

    if (logbook_ui_quotes_mode) {
        logbook_ui_current_page = 1;
    }
    logbook_ui_quotes_mode = false;
}

// 0x53F350
bool logbook_ui_message_filter(TigMessage* msg)
{
    int index;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    if (msg->type == TIG_MESSAGE_MOUSE) {
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP
            && intgame_pc_lens_check_pt(msg->data.mouse.x, msg->data.mouse.y)) {
            logbook_ui_close();
            return true;
        }
        return false;
    }

    if (msg->type == TIG_MESSAGE_BUTTON) {
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_PRESSED:
            for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
                if (logbook_ui_tab_buttons[index].button_handle == msg->data.button.button_handle) {
                    logbook_ui_switch_tab(index, false);
                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_RELEASED:
            if (logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle == msg->data.button.button_handle) {
                logbook_ui_turn_page_left();
                return true;
            }
            if (logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle == msg->data.button.button_handle) {
                logbook_ui_turn_page_right();
                return true;
            }
            return false;
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
                if (logbook_ui_tab_buttons[index].button_handle == msg->data.button.button_handle) {
                    mes_file_entry.num = index;
                    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

                    ui_message.type = UI_MSG_TYPE_FEEDBACK;
                    ui_message.str = mes_file_entry.str;
                    sub_550750(&ui_message);

                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
                if (logbook_ui_tab_buttons[index].button_handle == msg->data.button.button_handle) {
                    sub_550720();
                    return true;
                }
            }
            return false;
        }
        return false;
    }

    return false;
}

// 0x53F490
void logbook_ui_draw_panel(int art_num, bool preserve_page)
{
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;
    tig_button_handle_t selected_button_handle;
    int selected_button_index;
    int index;

    if (logbook_ui_art_num != -1) {
        if (!preserve_page && art_num == 261) {
            return;
        }
    } else {
        art_num = 261;
    }

    logbook_ui_art_num = art_num;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = logbook_ui_background_rects[0].width;
    src_rect.height = logbook_ui_background_rects[0].height;

    dst_rect.x = 165;
    dst_rect.y = 0;
    dst_rect.width = src_rect.width;
    dst_rect.height = src_rect.height;

    blit_info.flags = 0;
    tig_art_interface_id_create(260, 0, 0, 0, &(blit_info.art_id));
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(logbook_ui_window, &blit_info);

    tig_art_interface_id_create(logbook_ui_art_num, 0, 0, 0, &(blit_info.art_id));
    dst_rect.x = 172;
    dst_rect.y = 23;
    tig_window_blit_art(logbook_ui_window, &blit_info);

    selected_button_handle = sub_538730(logbook_ui_tab_buttons[0].button_handle);
    selected_button_index = 0;

    for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
        tig_button_show(logbook_ui_tab_buttons[index].button_handle);

        if (selected_button_handle == logbook_ui_tab_buttons[index].button_handle) {
            selected_button_index = index;
        }
    }

    gsound_play_sfx(SND_INTERFACE_BOOK_SWITCH, 1);
    logbook_ui_switch_tab(selected_button_index, preserve_page);
}

// 0x53F5F0
void logbook_ui_switch_tab(int tab, bool preserve_page)
{
    int index;

    if (!preserve_page) {
        for (index = 0; index < 100; index++) {
            logbook_ui_page_spread_starts[index] = -1;
        }
        logbook_ui_page_spread_starts[0] = 0;
        logbook_ui_current_page = 1;
        logbook_ui_tab = tab;
    }

    logbook_ui_load_data();
    logbook_ui_refresh();
    gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
}

// 0x53F640
void logbook_ui_turn_page_right()
{
    if (logbook_ui_last_visible_entry < logbook_ui_entry_count - 1
        && (logbook_ui_current_page - 1) / 2 < 100) {
        logbook_ui_current_page += 2;
        logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2] = logbook_ui_last_visible_entry + 1;
        logbook_ui_refresh();
        gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
    }
}

// 0x53F6A0
void logbook_ui_turn_page_left()
{
    if (logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2] > 0) {
        logbook_ui_current_page -= 2;
        logbook_ui_refresh();
        gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
    }
}

// 0x53F6E0
void logbook_ui_refresh()
{
    TigArtBlitInfo blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    MesFileEntry mes_file_entry;
    int index;
    char buffer[80];

    tig_button_hide(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle);
    tig_button_hide(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle);

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = logbook_ui_background_rects[0].width;
    src_rect.height = logbook_ui_background_rects[0].height;

    dst_rect.x = 165;
    dst_rect.y = 0;
    dst_rect.width = src_rect.width;
    dst_rect.height = src_rect.height;

    blit_info.flags = 0;
    tig_art_interface_id_create(260, 0, 0, 0, &(blit_info.art_id));
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;

    tig_window_blit_art(logbook_ui_window, &blit_info);

    if (logbook_ui_entry_count != 0) {
        logbook_ui_last_visible_entry = logbook_ui_draw_page_spread(logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2], logbook_ui_entry_count - 1);
    } else {
        logbook_ui_last_visible_entry = 0;
    }

    if (logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2] > 0) {
        tig_button_show(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle);
    }

    if (logbook_ui_last_visible_entry < logbook_ui_entry_count - 1) {
        tig_button_show(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle);
    }

    mes_file_entry.num = logbook_ui_tab == LOGBOOK_UI_TAB_RUMORS_AND_NOTES && logbook_ui_quotes_mode
        ? 100
        : logbook_ui_tab;
    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

    tig_font_push(logbook_ui_font_header);
    for (index = 0; index < 2; index++) {
        snprintf(buffer, sizeof(buffer), "%c%s%c", '-', mes_file_entry.str, '-');
        tig_window_text_write(logbook_ui_window, buffer, &(logbook_ui_page_title_rects[index]));
    }
    tig_font_pop();

    tig_font_push(logbook_ui_font_page_numbers);
    for (index = 0; index < 2; index++) {
        snprintf(buffer, sizeof(buffer), "%c%d%c", '-', logbook_ui_current_page + index, '-');
        tig_window_text_write(logbook_ui_window, buffer, &(logbook_ui_page_number_rects[index]));
    }
    tig_font_pop();
}

// TODO: Review.
//
// 0x53F8F0
int logbook_ui_draw_page_spread(int start_entry, int max_entry)
{
    TigRect page_rect;
    int entry_idx;
    bool on_right_page;
    bool is_first_on_page;
    int step;
    bool dry_run;
    int entry_height;

    page_rect = logbook_ui_page_content_rects[0];
    entry_idx = start_entry;
    on_right_page = false;
    is_first_on_page = true;
    if (start_entry > max_entry) {
        step = -1;
        dry_run = true;
    } else {
        step = 1;
        dry_run = false;
    }

    // Iterate until we reach one step past the final entry.
    while (entry_idx != step + max_entry) {
        entry_height = logbook_ui_draw_entry(entry_idx, &page_rect, dry_run, is_first_on_page);
        is_first_on_page = false;
        if (entry_height > 0 && entry_height <= page_rect.height) {
            // Entry fits, consume vertical space and move to the next entry.
            page_rect.y += entry_height;
            page_rect.height -= entry_height;
        } else {
            if (on_right_page) {
                // The right page is also full (or the entry is too tall even
                // for a fresh page). If the entry was non-empty, back up so
                // the caller records the correct last-visible index.
                if (entry_height > 0) {
                    entry_idx += step;
                }
                break;
            }

            // Switch to the right page and retry from the same entry.
            page_rect = logbook_ui_page_content_rects[1];
            on_right_page = true;
            is_first_on_page = true;

            // If the entry reported zero height it's "too tall for available
            // space". Back up one step so it is retried against the fresh right
            // page.
            if (entry_height == 0) {
                entry_idx -= step;
            }
        }

        entry_idx += step;
    }

    // Return the last successfully drawn entry index.
    return entry_idx - step;
}

// 0x53F9E0
int logbook_ui_draw_entry(int index, TigRect* rect, bool dry_run, bool can_truncate)
{
    char buffer[2000];
    bool v1;

    v1 = index < logbook_ui_entry_count - 1;
    buffer[0] = '\0';

    switch (logbook_ui_tab) {
    case LOGBOOK_UI_TAB_RUMORS_AND_NOTES:
        logbook_ui_format_rumor(buffer, index);
        break;
    case LOGBOOK_UI_TAB_QUESTS:
        logbook_ui_format_quest(buffer, index);
        break;
    case LOGBOOK_UI_TAB_REPUTATIONS:
        logbook_ui_format_reputation(buffer, index);
        break;
    case LOGBOOK_UI_TAB_BLESSINGS_AND_CURSES:
        logbook_ui_format_blessing_or_curse(buffer, index);
        break;
    case LOGBOOK_UI_TAB_KILLS_AND_INJURES:
        logbook_ui_format_kill_or_injury(buffer, index);
        v1 = false;
        break;
    case LOGBOOK_UI_TAB_BACKGROUND:
        logbook_ui_format_background(buffer, index);
        break;
    case LOGBOOK_UI_TAB_KEYS:
        logbook_ui_format_key(buffer, index);
        break;
    }

    return logbook_ui_draw_text(buffer, logbook_ui_entry_fonts[index], rect, dry_run, can_truncate, v1);
}

// 0x53FAD0
int logbook_ui_draw_text(char* buffer, tig_font_handle_t font, TigRect* rect, bool dry_run, bool can_truncate, bool a6)
{
    TigFont font_info;
    bool warned = false;
    size_t pos;

    if (buffer[0] == '\0') {
        return -1;
    }

    tig_font_push(font);

    font_info.str = buffer;
    font_info.width = rect->width;

    while (true) {
        tig_font_measure(&font_info);
        if (font_info.height <= rect->height) {
            break;
        }

        if (!can_truncate || (pos = strlen(buffer)) == 0) {
            tig_font_pop();
            return 0;
        }

        if (!warned) {
            tig_debug_printf("Had to shorten logbook entry: %s\n", buffer);
        }

        buffer[pos - 1] = '\0';
        warned = true;
    }

    if (!dry_run) {
        tig_window_text_write(logbook_ui_window, buffer, rect);
    }

    tig_font_pop();

    if (a6) {
        return logbook_ui_line_height + font_info.height;
    }

    return font_info.height;
}

// 0x53FBB0
void logbook_ui_load_data()
{
    int index;

    if (logbook_ui_tab == LOGBOOK_UI_TAB_RUMORS_AND_NOTES) {
        RumorLogbookEntry rumors[MAX_RUMORS]; // NOTE: Forces `alloca(72000)`.

        logbook_ui_entry_count = rumor_get_logbook_data(logbook_ui_obj, rumors);

        if (logbook_ui_quotes_mode) {
            logbook_ui_entry_count = mes_num_entries(quotes_mes_file);
            for (index = 0; index < logbook_ui_entry_count; index++) {
                logbook_ui_entry_fonts[index] = logbook_ui_font_default;
            }
        } else {
            for (index = 0; index < logbook_ui_entry_count; index++) {
                logbook_ui_entry_ids[index] = rumors[index].num;
                logbook_ui_entry_datetimes[index] = rumors[index].datetime;
                if (rumors[index].quelled) {
                    logbook_ui_entry_fonts[index] = logbook_ui_font_struck;
                } else {
                    logbook_ui_entry_fonts[index] = logbook_ui_font_default;
                }
            }
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_QUESTS) {
        QuestLogbookEntry quests[2000]; // NOTE: Forces `alloca(48000)`.

        logbook_ui_entry_count = quest_get_logbook_data(logbook_ui_obj, quests);

        for (index = 0; index < logbook_ui_entry_count; index++) {
            logbook_ui_entry_ids[index] = quests[index].num;
            logbook_ui_entry_datetimes[index] = quests[index].datetime;
            logbook_ui_quest_states[index] = quests[index].state;

            switch (logbook_ui_quest_states[index]) {
            case QUEST_STATE_COMPLETED:
                logbook_ui_entry_fonts[index] = logbook_ui_font_quest_completed;
                break;
            case QUEST_STATE_OTHER_COMPLETED:
            case QUEST_STATE_BOTCHED:
                logbook_ui_entry_fonts[index] = logbook_ui_font_quest_failed;
                break;
            case QUEST_STATE_ACCEPTED:
            case QUEST_STATE_ACHIEVED:
                logbook_ui_entry_fonts[index] = logbook_ui_font_active;
                break;
            default:
                logbook_ui_entry_fonts[index] = logbook_ui_font_default;
            }
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_REPUTATIONS) {
        ReputationLogbookEntry reps[2000]; // NOTE: Forces `alloca(32000)`.

        logbook_ui_entry_count = reputation_get_logbook_data(logbook_ui_obj, reps);

        for (index = 0; index < logbook_ui_entry_count; index++) {
            logbook_ui_entry_ids[index] = reps[index].reputation;
            logbook_ui_entry_datetimes[index] = reps[index].datetime;

            logbook_ui_entry_fonts[index] = logbook_ui_font_default;
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_BLESSINGS_AND_CURSES) {
        CurseLogbookEntry curses[100];
        BlessLogbookEntry blessings[100];
        int num_blessings;
        int num_curses;
        int bless_idx;
        int curse_idx;
        int idx;

        num_blessings = bless_get_logbook_data(logbook_ui_obj, blessings);
        num_curses = curse_get_logbook_data(logbook_ui_obj, curses);
        logbook_ui_entry_count = num_blessings + num_curses;

        bless_idx = 0;
        curse_idx = 0;
        for (idx = 0; idx < logbook_ui_entry_count; idx++) {
            if (bless_idx < num_blessings
                && (curse_idx == num_curses || datetime_compare(&(blessings[bless_idx].datetime), &(curses[curse_idx].datetime)))) {
                logbook_ui_entry_ids[idx] = blessings[bless_idx].id;
                logbook_ui_entry_datetimes[idx] = blessings[bless_idx].datetime;
                logbook_ui_entry_fonts[idx] = logbook_ui_font_active;
                bless_idx++;
            } else {
                logbook_ui_entry_ids[idx] = curses[curse_idx].id;
                logbook_ui_entry_datetimes[idx] = curses[curse_idx].datetime;
                logbook_ui_entry_fonts[idx] = logbook_ui_font_curse;
                curse_idx++;
            }
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_KILLS_AND_INJURES) {
        int desc;
        int injury;
        unsigned int flags;
        int cnt;

        logbook_get_kills(logbook_ui_obj, logbook_ui_kill_stats);

        for (index = 0; index < 9; index++) {
            logbook_ui_entry_fonts[index] = logbook_ui_font_default;
        }

        logbook_ui_entry_count = 9;

        index = logbook_find_first_injury(logbook_ui_obj, &desc, &injury);
        while (index != 0) {
            logbook_ui_entry_ids[logbook_ui_entry_count] = desc;
            logbook_ui_entry_datetimes[logbook_ui_entry_count].milliseconds = injury;
            logbook_ui_entry_fonts[logbook_ui_entry_count] = logbook_ui_font_struck;
            logbook_ui_entry_count++;
            index = logbook_find_next_injury(logbook_ui_obj, index, &desc, &injury);
        }

        flags = obj_field_int32_get(logbook_ui_obj, OBJ_F_CRITTER_FLAGS);
        if ((flags & OCF_BLINDED) != 0) {
            for (index = logbook_ui_entry_count - 1; index >= 9; index--) {
                if (logbook_ui_entry_datetimes[index].milliseconds == LBI_BLINDED) {
                    logbook_ui_entry_fonts[index] = logbook_ui_font_default;
                    break;
                }
            }
        }
        if ((flags & OCF_CRIPPLED_LEGS_BOTH) != 0) {
            for (index = logbook_ui_entry_count - 1; index >= 9; index--) {
                if (logbook_ui_entry_datetimes[index].milliseconds == LBI_CRIPPLED_LEG) {
                    logbook_ui_entry_fonts[index] = logbook_ui_font_default;
                    break;
                }
            }
        }
        if ((flags & OCF_CRIPPLED_ARMS_BOTH) != 0) {
            for (index = logbook_ui_entry_count - 1; index >= 9; index--) {
                if (logbook_ui_entry_datetimes[index].milliseconds == LBI_CRIPPLED_ARM) {
                    logbook_ui_entry_fonts[index] = logbook_ui_font_default;
                    break;
                }
            }
        }
        if ((flags & OCF_CRIPPLED_ARMS_ONE) != 0) {
            for (index = logbook_ui_entry_count - 1; index >= 9; index--) {
                if (logbook_ui_entry_datetimes[index].milliseconds == LBI_CRIPPLED_ARM) {
                    logbook_ui_entry_fonts[index] = logbook_ui_font_default;
                    break;
                }
            }
        }
        cnt = effect_count_effects_of_type(logbook_ui_obj, EFFECT_SCARRING);
        if (cnt > 0) {
            for (index = logbook_ui_entry_count - 1; index >= 9; index--) {
                if (logbook_ui_entry_datetimes[index].milliseconds == LBI_SCARRED) {
                    logbook_ui_entry_fonts[index] = logbook_ui_font_default;
                    break;
                }
            }
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_BACKGROUND) {
        char str[MAX_STRING];
        char* curr;
        size_t pos;
        size_t end;
        size_t truncate_pos;
        size_t prev_truncate_pos;
        TigFont font_desc;
        char ch;

        logbook_ui_entry_count = 1;
        logbook_ui_entry_ids[0] = background_text_get(logbook_ui_obj);
        logbook_ui_entry_fonts[0] = logbook_ui_font_default;

        strcpy(str, background_description_get_body(logbook_ui_entry_ids[0]));
        end = strlen(str);

        tig_font_push(logbook_ui_entry_fonts[0]);
        font_desc.str = str;
        font_desc.width = logbook_ui_page_content_rects[0].width;
        tig_font_measure(&font_desc);

        prev_truncate_pos = 0;
        curr = str;
        while (font_desc.height > logbook_ui_page_content_rects[0].height) {
            truncate_pos = 0;

            for (pos = 0; pos < end; pos++) {
                ch = curr[pos];
                if (ch == ' ' || ch == '\n') {
                    curr[pos] = '\0';
                    tig_font_measure(&font_desc);
                    curr[pos] = ch;

                    if (font_desc.height > logbook_ui_page_content_rects[0].height) {
                        break;
                    }

                    truncate_pos = pos + 1;
                }
            }

            if (truncate_pos == 0) {
                break;
            }

            prev_truncate_pos += truncate_pos;

            logbook_ui_entry_fonts[logbook_ui_entry_count] = logbook_ui_entry_fonts[0];
            logbook_ui_entry_ids[logbook_ui_entry_count] = (int)prev_truncate_pos;
            logbook_ui_entry_count++;

            end -= truncate_pos;
            font_desc.str = &(curr[truncate_pos]);
            tig_font_measure(&font_desc);
        }

        logbook_ui_entry_ids[logbook_ui_entry_count] = (int)(prev_truncate_pos + end);
        tig_font_pop();

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_KEYS) {
        logbook_ui_entry_count = item_get_keys(logbook_ui_obj, logbook_ui_entry_ids);
        if (logbook_ui_entry_count > 0) {
            for (index = 0; index < logbook_ui_entry_count; index++) {
                logbook_ui_entry_fonts[index] = logbook_ui_font_default;
            }
        }
        return;
    }
}

// 0x540310
void logbook_ui_format_rumor(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    size_t pos;

    if (logbook_ui_quotes_mode) {
        mes_file_entry.num = index;
        if (mes_search(quotes_mes_file, &mes_file_entry)) {
            strcpy(buffer, mes_file_entry.str);
        } else {
            buffer[0] = '\0';
        }
    } else {
        logbook_ui_format_timestamp(buffer, index);

        pos = strlen(buffer);
        buffer[pos] = '\n';

        rumor_copy_logbook_str(logbook_ui_obj, logbook_ui_entry_ids[index], &(buffer[pos + 1]));
    }
}

// 0x5403C0
void logbook_ui_format_timestamp(char* buffer, int index)
{
    DateTime* datetime;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    char am_pm;
    MesFileEntry mes_file_entry;

    datetime = &(logbook_ui_entry_datetimes[index]);
    month = datetime_get_month(datetime);
    day = datetime_get_day(datetime);
    year = datetime_get_year(datetime);
    hour = datetime_get_hour(datetime);
    minute = datetime_get_minute(datetime);

    // Convert 24-hour time to 12-hour with AM/PM indicator.
    if (hour < 12) {
        am_pm = 'a';
        if (hour == 0) {
            hour = 12;
        }
    } else {
        am_pm = 'p';
        if (hour > 12) {
            hour -= 12;
        }
    }

    mes_file_entry.num = month + 6;
    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

    sprintf(buffer, "%s %d, %d  %d:%.2d%cm    ",
        mes_file_entry.str,
        day,
        year,
        hour,
        minute,
        am_pm);
}

// 0x540470
void logbook_ui_format_quest(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    size_t pos;

    logbook_ui_format_timestamp(buffer, index);
    mes_file_entry.num = logbook_ui_quest_states[index] + 19;
    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);
    strcat(buffer, mes_file_entry.str);

    pos = strlen(buffer);
    buffer[pos] = '\n';

    quest_copy_description(logbook_ui_obj, logbook_ui_entry_ids[index], &(buffer[pos + 1]));
}

// 0x540510
void logbook_ui_format_reputation(char* buffer, int index)
{
    size_t pos;

    logbook_ui_format_timestamp(buffer, index);
    pos = strlen(buffer);
    buffer[pos] = '\n';

    reputation_name(logbook_ui_entry_ids[index], &(buffer[pos + 1]));
}

// 0x540550
void logbook_ui_format_blessing_or_curse(char* buffer, int index)
{
    size_t pos;

    logbook_ui_format_timestamp(buffer, index);
    pos = strlen(buffer);
    buffer[pos] = '\n';

    if (logbook_ui_entry_fonts[index] == logbook_ui_font_active) {
        bless_copy_name(logbook_ui_entry_ids[index], &(buffer[pos + 1]));
    } else {
        curse_copy_name(logbook_ui_entry_ids[index], &(buffer[pos + 1]));
    }
}

// 0x5405C0
void logbook_ui_format_kill_or_injury(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    const char* desc_str;
    char tmp[80];
    int desc;

    if (index < 7) {
        mes_file_entry.num = 26 + index;
        mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

        // NOTE: Original code is slightly different but does the same thing.
        if (index == 0) {
            SDL_itoa(logbook_ui_kill_stats[LBK_TOTAL_KILLS], tmp, 10);
            sprintf(buffer, "%s: %s", mes_file_entry.str, tmp);
        } else {
            switch (index) {
            case 1:
                desc = logbook_ui_kill_stats[LBK_MOST_POWERFUL_NAME];
                break;
            case 2:
                desc = logbook_ui_kill_stats[LBK_LEAST_POWERFUL_NAME];
                break;
            case 3:
                desc = logbook_ui_kill_stats[LBK_MOST_GOOD_NAME];
                break;
            case 4:
                desc = logbook_ui_kill_stats[LBK_MOST_EVIL_NAME];
                break;
            case 5:
                desc = logbook_ui_kill_stats[LBK_MOST_MAGICAL_NAME];
                break;
            case 6:
                desc = logbook_ui_kill_stats[LBK_MOST_TECH_NAME];
                break;
            default:
                // Should be unreachable.
                assert(0);
            }

            if (desc > 0) {
                desc_str = description_get(desc);
            } else {
                desc_str = NULL;
            }

            if (desc_str != NULL) {
                sprintf(buffer, "%s: %s", mes_file_entry.str, desc_str);
            } else {
                sprintf(buffer, "%s: -----", mes_file_entry.str);
            }
        }
    } else if (index == 7) {
        buffer[0] = '\0';
    } else if (index == 8) {
        mes_file_entry.num = 33;
        mes_get_msg(logbook_ui_mes_file, &mes_file_entry);
        strcpy(buffer, mes_file_entry.str);
    } else {
        mes_file_entry.num = logbook_ui_entry_datetimes[index].milliseconds + 34;
        mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

        desc_str = description_get(logbook_ui_entry_ids[index]);
        if (desc_str != NULL) {
            sprintf(buffer, "%s %s", mes_file_entry.str, desc_str);
        }
    }
}

// 0x540760
void logbook_ui_format_background(char* buffer, int index)
{
    const char* body;
    int start;
    int end;

    body = background_description_get_body(logbook_ui_entry_ids[0]);
    if (index != 0) {
        start = logbook_ui_entry_ids[index];
    } else {
        start = 0;
    }

    end = logbook_ui_entry_ids[index + 1] - start;
    strncpy(buffer, &(body[start]), end);
    buffer[end] = '\0';
}

// 0x5407B0
void logbook_ui_format_key(char* buffer, int index)
{
    strcpy(buffer, key_description_get(logbook_ui_entry_ids[index]));
}

// 0x5407F0
void logbook_ui_check()
{
    int index;
    char buffer[2000];
    size_t pos;
    TigRect rect;

    logbook_ui_entry_datetimes[0] = sub_45A7C0();

    for (index = 0; index < 3000; index++) {
        rect = logbook_ui_page_content_rects[0];

        logbook_ui_format_timestamp(buffer, 0);
        pos = strlen(buffer);
        buffer[pos] = '\n';

        rumor_copy_logbook_normal_str(index, &(buffer[pos + 1]));
        if (buffer[pos + 1] != '\0') {
            tig_debug_printf("Checking rumor %d\n", index);
            logbook_ui_draw_text(buffer, logbook_ui_font_default, &rect, 1, 1, 1);
        }

        rect = logbook_ui_page_content_rects[0];

        logbook_ui_format_timestamp(buffer, 0);
        pos = strlen(buffer);
        buffer[pos] = '\n';
        rumor_copy_logbook_dumb_str(index, &(buffer[pos + 1]));
        if (buffer[pos + 1] != '\0') {
            tig_debug_printf("Checking dumb rumor %d\n", index);
            logbook_ui_draw_text(buffer, logbook_ui_font_default, &rect, 1, 1, 1);
        }
    }
}
