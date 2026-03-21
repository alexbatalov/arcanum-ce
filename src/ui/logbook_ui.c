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

static void logbook_ui_create(void);
static void logbook_ui_destroy(void);
static bool logbook_ui_message_filter(TigMessage* msg);
static void logbook_ui_draw_panel(int border_art_num, bool preserve_page);
static void logbook_ui_switch_tab(int tab, bool preserve_page);
static void logbook_ui_turn_page_right(void);
static void logbook_ui_turn_page_left(void);
static void logbook_ui_refresh(void);
static int logbook_ui_draw_page_spread(int start_entry, int max_entry);
static int logbook_ui_draw_entry(int index, TigRect* rect, bool dry_run, bool can_truncate);
static int logbook_ui_draw_text(char* buffer, tig_font_handle_t font, TigRect* rect, bool dry_run, bool can_truncate, bool add_line_spacing);
static void logbook_ui_load_data(void);
static void logbook_ui_format_rumor(char* buffer, int index);
static void logbook_ui_format_timestamp(char* buffer, int index);
static void logbook_ui_format_quest(char* buffer, int index);
static void logbook_ui_format_reputation(char* buffer, int index);
static void logbook_ui_format_blessing_or_curse(char* buffer, int index);
static void logbook_ui_format_kill_or_injury(char* buffer, int index);
static void logbook_ui_format_background(char* buffer, int index);
static void logbook_ui_format_key(char* buffer, int index);

/**
 * Handle to the logbook UI window.
 *
 * 0x5C33F0
 */
static tig_window_handle_t logbook_ui_window = TIG_WINDOW_HANDLE_INVALID;

/**
 * 0x5C33F8
 */
static TigRect logbook_ui_background_rects[2] = {
    { 0, 41, 800, 400 },
    { 150, 11, 501, 365 },
};

/**
 * Defines a rect where PC lens is located in the logbook UI.
 *
 * 0x5C3418
 */
static TigRect logbook_ui_pc_lens_rect = { 25, 24, 89, 89 };

/**
 * Defines logbook UI navigation/page-turn buttons.
 *
 * 0x5C3428
 */
static UiButtonInfo logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_COUNT] = {
    { 213, 77, 272, TIG_BUTTON_HANDLE_INVALID },
    { 675, 77, 273, TIG_BUTTON_HANDLE_INVALID },
};

/**
 * Defines logbook UI tab buttons.
 *
 * 0x5C3448
 */
static UiButtonInfo logbook_ui_tab_buttons[LOGBOOK_UI_TAB_COUNT] = {
    /*     LOGBOOK_UI_TAB_RUMORS_AND_NOTES */ { 696, 83, 265, TIG_BUTTON_HANDLE_INVALID },
    /*               LOGBOOK_UI_TAB_QUESTS */ { 696, 129, 267, TIG_BUTTON_HANDLE_INVALID },
    /*          LOGBOOK_UI_TAB_REPUTATIONS */ { 696, 175, 271, TIG_BUTTON_HANDLE_INVALID },
    /* LOGBOOK_UI_TAB_BLESSINGS_AND_CURSES */ { 695, 221, 266, TIG_BUTTON_HANDLE_INVALID },
    /*    LOGBOOK_UI_TAB_KILLS_AND_INJURES */ { 696, 267, 270, TIG_BUTTON_HANDLE_INVALID },
    /*           LOGBOOK_UI_TAB_BACKGROUND */ { 696, 313, 268, TIG_BUTTON_HANDLE_INVALID },
    /*                 LOGBOOK_UI_TAB_KEYS */ { 696, 359, 269, TIG_BUTTON_HANDLE_INVALID },
};

/**
 * Defines text area for the left and right pages.
 *
 * 0x5C34B8
 */
static TigRect logbook_ui_page_content_rects[2] = {
    { 222, 72, 215, 270 },
    { 468, 72, 215, 270 },
};

/**
 * Defines page header area for the left and right pages where the page title
 * is rendered.
 *
 * 0x5C34D8
 */
static TigRect logbook_ui_page_header_rects[2] = {
    { 236, 43, 187, 19 },
    { 482, 43, 187, 19 },
};

/**
 * Defines page footer area for the left and right pages where the page number
 * is rendered.
 *
 * 0x5C34F8
 */
static TigRect logbook_ui_page_footer_rects[2] = {
    { 222, 346, 215, 15 },
    { 468, 346, 215, 15 },
};

/**
 * Height (in pixels) of a single text line. Used as inter-entry spacing so that
 * consecutive entries are visually separated by one blank line.
 *
 * 0x63CBF0
 */
static int logbook_ui_line_height;

/**
 * Per-entry quest state values for the "Quests" tab.
 *
 * 0x63CBF4
 */
static int logbook_ui_quest_states[3000];

/**
 * 0x63FAD8
 */
static int64_t logbook_ui_obj;

/**
 * Green strikethrough font. Used for quests that have been completed by the
 * player's party.
 *
 * 0x63FAE0
 */
static tig_font_handle_t logbook_ui_font_quest_completed;

/**
 * Per-entry data array shared across all tabs. Its interpretation varies by
 * active tab:
 *
 *  - Rumors & Notes: rumor number.
 *  - Quests: quest number.
 *  - Reputations: reputation number.
 *  - Blessings/Curses: blessing or curse ID
 *  - Kills & Injuries: name of the injured critter (only for entries beyond 9).
 *  - Background: element [0] holds the background number, subsequent elements
 *    hold the character offset of the start of each paginated sub-page, the
 *    last element (at `logbook_ui_entry_count` index) serving as a one-past-end
 *     sentinel equal to the total text length.
 *  - Keys: key ID.
 *
 * 0x63FAE4
 */
static int logbook_ui_entry_ids[3000];

/**
 * Black strikethrough font. Used for rumors that have been quelled (proven
 * false) and for injuries that have since healed.
 *
 * 0x6429C4
 */
static tig_font_handle_t logbook_ui_font_struck;

/**
 * Centred black font. Used for page-number labels in the footer areas.
 *
 * 0x6429C8
 */
static tig_font_handle_t logbook_ui_font_page_numbers;

/**
 * "quotes.mes"
 *
 * 0x6429CC
 */
static mes_file_handle_t quotes_mes_file;

/**
 * Plain red font. Used for curse entries in the Blessings & Curses tab.
 *
 * 0x6429D0
 */
static tig_font_handle_t logbook_ui_font_curse;

/**
 * Per-entry timestamp array. For most tabs this records when the event
 * occurred.
 *
 * For the "Kills & Injuries" tab, the `milliseconds` field of each injury
 * entry repurposed to store type of injury, rather than a time value.
 *
 * 0x6429D8
 */
static DateTime logbook_ui_entry_datetimes[3000];

/**
 * Current page number, 1-indexed and always odd (pages are shown two at a
 * time, page 1 is the first spread, page 3 is the second, etc.).
 *
 * 0x648798
 */
static int logbook_ui_current_page;

/**
 * Maps each page-spread index to the index of the first entry displayed on
 * that spread. Index 0 always maps to entry 0 (the beginning of the list).
 * Entries beyond the current spread are invalidated (-1) when a new tab is
 * selected or the logbook is reset.
 *
 * 0x64879C
 */
static int logbook_ui_page_spread_starts[100];

/**
 * Red strikethrough font. Used for quests that were botched or completed by
 * someone other than the player's party.
 *
 * 0x64892C
 */
static tig_font_handle_t logbook_ui_font_quest_failed;

/**
 * Large centred font. Used for the tab-name heading printed above each page.
 *
 * 0x648930
 */
static tig_font_handle_t logbook_ui_font_header;

/**
 * Default plain black font. Used for normal entries and as the base style when
 * no other condition applies.
 *
 * 0x648934
 */
static tig_font_handle_t logbook_ui_font_default;

/**
 * Blue font. Used for active quests and blessings.
 *
 * 0x64893C
 */
static tig_font_handle_t logbook_ui_font_active;

/**
 * Total number of entries loaded for the currently active tab.
 *
 * For the "Background" tab this represents the number of paginated sub-pages.
 *
 * 0x648938
 */
static int logbook_ui_entry_count;

/**
 * Kill and injury statistics array populated by `logbook_get_kills`.
 *
 * 0x648940
 */
static int logbook_ui_kill_stats[LBK_COUNT];

/**
 * Index of the last entry successfully rendered onto the current page spread.
 * Updated by `logbook_ui_draw_page_spread`() and used to determine whether the
 * "next page" button should be visible.
 *
 * 0x648974
 */
static int logbook_ui_last_visible_entry;

/**
 * Currently active tab. Persists across close/open cycles.
 *
 * 0x648978
 */
static int logbook_ui_tab;

/**
 * The number of art in `interface.mes` for border around book spread.
 *
 * 0x64897C
 */
static int logbook_ui_border_art_num;

/**
 * Flag indicating that "Rumors & Notes" tab operates in "Bloopers & Quotes"
 * mode. This hidden easter egg is activated by holding Left Ctrl + Left Alt
 * while opening the logbook.
 *
 * 0x648980
 */
static bool logbook_ui_quotes_mode;

/**
 * "logbk_ui.mes"
 *
 * 0x648984
 */
static mes_file_handle_t logbook_ui_mes_file;

/**
 * Per-entry font handle.
 *
 * 0x648988
 */
static tig_font_handle_t logbook_ui_entry_fonts[3000];

/**
 * Flag indicating whether the logbook UI is initialized.
 *
 * 0x64B868
 */
static bool logbook_ui_initialized;

/**
 * Flag indicating whether the logbook UI is currently displayed.
 *
 * 0x64B86C
 */
static bool logbook_ui_created;

/**
 * Called when the game is initialized.
 *
 * 0x53EB10
 */
bool logbook_ui_init(GameInitInfo* init_info)
{
    TigFont font_info;
    int index;

    (void)init_info;

    // Load UI messages (required).
    if (!mes_load("mes\\logbk_ui.mes", &logbook_ui_mes_file)) {
        return false;
    }

    // Load quotes (required).
    if (!mes_load("mes\\quotes.mes", &quotes_mes_file)) {
        return false;
    }

    // Plain black - default body text.
    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_default);

    // Centred black - page numbers in the footer.
    font_info.flags = TIG_FONT_CENTERED;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_page_numbers);

    // Black strikethrough - quelled rumors and healed injuries.
    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &logbook_ui_font_struck);

    // Green strikethrough - quests completed by the player's party.
    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 120, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &logbook_ui_font_quest_completed);

    // Red strikethrough - quests that were botched or completed by others.
    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(150, 0, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &logbook_ui_font_quest_failed);

    // Large centred - tab headings above each page.
    font_info.flags = TIG_FONT_CENTERED;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_header);

    // Blue - active quests and blessings currently in effect.
    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 150);
    tig_font_create(&font_info, &logbook_ui_font_active);

    // Red - active curses currently in effect.
    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(150, 0, 0);
    tig_font_create(&font_info, &logbook_ui_font_curse);

    // Measure a single line of the default font (used as inter-entry spacing).
    tig_font_push(logbook_ui_font_default);
    font_info.str = "test";
    font_info.width = 0;
    tig_font_measure(&font_info);
    logbook_ui_line_height = font_info.height;
    tig_font_pop();

    logbook_ui_border_art_num = -1;

    // Initialize page-spread history: spread 0 starts at entry 0.
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

/**
 * Called when the game shuts down.
 *
 * 0x53EF40
 */
void logbook_ui_exit(void)
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

/**
 * Called when the game is being reset.
 *
 * 0x53EFD0
 */
void logbook_ui_reset(void)
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
    logbook_ui_border_art_num = -1;
    logbook_ui_current_page = 1;
}

/**
 * Toggles the logbook UI.
 *
 * 0x53F020
 */
void logbook_ui_open(int64_t obj)
{
    // Close logbook UI if it's currently visible.
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

/**
 * Closes the logbook window and switches the game back to the main mode.
 *
 * 0x53F090
 */
void logbook_ui_close(void)
{
    if (logbook_ui_created
        && intgame_mode_set(INTGAME_MODE_MAIN)) {
        logbook_ui_destroy();
        logbook_ui_obj = OBJ_HANDLE_NULL;
    }
}

/**
 * Returns `true` if the logbook window is currently open.
 *
 * 0x53F0D0
 */
bool logbook_ui_is_created(void)
{
    return logbook_ui_created;
}

/**
 * Creates the logbook UI window.
 *
 * 0x53F0E0
 */
void logbook_ui_create(void)
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

    // Hidden easter egg: holding Left Ctrl + Left Alt while opening logbook
    // switches the "Rumors & Notes" tab into "Bloopers & Quotes" mode.
    if (tig_kb_get_modifier(SDL_KMOD_LCTRL) && tig_kb_get_modifier(SDL_KMOD_LALT)) {
        logbook_ui_tab = LOGBOOK_UI_TAB_RUMORS_AND_NOTES;
        logbook_ui_current_page = 1;
        logbook_ui_quotes_mode = true;
        gsound_play_sfx(SND_INTERFACE_BLESS, 1);
    }

    // Blit the sidebar ("logbooks_side.art"). This section never changes in the
    // window is therefore never redrawn.
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

    // Create page-turn buttons. Both start hidden, and will be made visible
    // when there is a need for them.
    for (index = 0; index < LOGBOOK_UI_BUTTON_COUNT; index++) {
        intgame_button_create_ex(logbook_ui_window,
            &(logbook_ui_background_rects[0]),
            &(logbook_ui_page_buttons[index]),
            TIG_BUTTON_HIDDEN | TIG_BUTTON_MOMENTARY);
    }

    // Create tab buttons and group them as a radio set so exactly one tab is
    // always selected. The buttons start hidden but all will be made visible
    // a moment later, in `logbook_ui_draw_panel`.
    for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
        intgame_button_create_ex(logbook_ui_window,
            &(logbook_ui_background_rects[0]),
            &(logbook_ui_tab_buttons[index]),
            TIG_BUTTON_HIDDEN | TIG_BUTTON_MOMENTARY);
        button_handles[index] = logbook_ui_tab_buttons[index].button_handle;
    }
    tig_button_radio_group_create(LOGBOOK_UI_TAB_COUNT, button_handles, logbook_ui_tab);

    // Center viewport on the player (so that the lens display proper
    // surroundings).
    location_origin_set(obj_field_int64_get(logbook_ui_obj, OBJ_F_LOCATION));

    // Enable the PC lens.
    pc_lens.window_handle = logbook_ui_window;
    pc_lens.rect = &logbook_ui_pc_lens_rect;
    tig_art_interface_id_create(198, 0, 0, 0, &(pc_lens.art_id));
    intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);

    // Render the main logbook ui and the initial page content.
    logbook_ui_draw_panel(261, true);

    ui_toggle_primary_button(UI_PRIMARY_BUTTON_LOGBOOK, false);
    gsound_play_sfx(SND_INTERFACE_BOOK_OPEN, 1);

    logbook_ui_created = true;
}

/**
 * Destroys the logbook window and releases all associated resources.
 *
 * 0x53F2F0
 */
void logbook_ui_destroy(void)
{
    if (!logbook_ui_created) {
        return;
    }

    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
    intgame_big_window_unlock();
    logbook_ui_window = TIG_WINDOW_HANDLE_INVALID;
    gsound_play_sfx(SND_INTERFACE_BOOK_CLOSE, 1);
    logbook_ui_created = false;

    // If quotes mode was active, restore the page counter to 1 so the next
    // open (in normal mode) starts from the beginning.
    if (logbook_ui_quotes_mode) {
        logbook_ui_current_page = 1;
    }

    logbook_ui_quotes_mode = false;
}

/**
 * Called when an event occurred.
 *
 * 0x53F350
 */
bool logbook_ui_message_filter(TigMessage* msg)
{
    int index;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    if (msg->type == TIG_MESSAGE_MOUSE) {
        // Clicking on the PC lens closes logbook UI.
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
            // A tab button was pressed: switch to that tab and reset the page
            // position.
            for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
                if (logbook_ui_tab_buttons[index].button_handle == msg->data.button.button_handle) {
                    logbook_ui_switch_tab(index, false);
                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_RELEASED:
            // Page-turn buttons: navigate one spread forward or backward.
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
            // Show the tab's tooltip in the message window while the cursor
            // hovers over it.
            for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
                if (logbook_ui_tab_buttons[index].button_handle == msg->data.button.button_handle) {
                    mes_file_entry.num = index;
                    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

                    ui_message.type = UI_MSG_TYPE_FEEDBACK;
                    ui_message.str = mes_file_entry.str;
                    intgame_message_window_display_msg(&ui_message);

                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            // Clear the message window when the cursor leaves a tab button.
            for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
                if (logbook_ui_tab_buttons[index].button_handle == msg->data.button.button_handle) {
                    intgame_message_window_clear();
                    return true;
                }
            }
            return false;
        }
        return false;
    }

    return false;
}

/**
 * Draws the logbook book spread.
 *
 * `art_num` is the art number of the border around "book" image. The only value
 * used is 261 ("book_quest.art"). There are other art graphics, that have
 * slightly different tint.
 *
 * 0x53F490
 */
void logbook_ui_draw_panel(int border_art_num, bool preserve_page)
{
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;
    tig_button_handle_t selected_button_handle;
    int selected_button_index;
    int index;

    if (logbook_ui_border_art_num != -1) {
        // A border is already drawn. Skip the redraw if the caller is not
        // forcing a refresh and the requested art is the default.
        if (!preserve_page && border_art_num == 261) {
            return;
        }
    } else {
        // The border is not drawn yet.
        border_art_num = 261;
    }

    logbook_ui_border_art_num = border_art_num;

    // Redraw the two-page spread background to clear any previous page content.
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

    // Redraw the specified border around two-page spread.
    tig_art_interface_id_create(logbook_ui_border_art_num, 0, 0, 0, &(blit_info.art_id));
    dst_rect.x = 172;
    dst_rect.y = 23;
    tig_window_blit_art(logbook_ui_window, &blit_info);

    // Reveal all tab buttons (they were hidden at creation time) and determine
    // which one is currently selected so we can restore it after the redraw.
    selected_button_handle = sub_538730(logbook_ui_tab_buttons[0].button_handle);
    selected_button_index = 0;

    for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
        tig_button_show(logbook_ui_tab_buttons[index].button_handle);

        // Find the index of selected tab.
        if (selected_button_handle == logbook_ui_tab_buttons[index].button_handle) {
            selected_button_index = index;
        }
    }

    gsound_play_sfx(SND_INTERFACE_BOOK_SWITCH, 1);

    // Load data and render the entries for the selected tab.
    logbook_ui_switch_tab(selected_button_index, preserve_page);
}

/**
 * Switches the active logbook tab to `tab` and refreshes the displayed pages.
 *
 * When `preserve_page` is false the page-spread history is cleared and the
 * display returns to the very first entry of the new tab. When true the
 * existing page position is kept, which is used when reopening the
 * logbook to restore the previously viewed spread.
 *
 * 0x53F5F0
 */
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

/**
 * Advances to the next page spread if one exists and the page-spread history
 * array is not full. Records the starting entry of the new spread so the
 * player can navigate back.
 *
 * 0x53F640
 */
void logbook_ui_turn_page_right(void)
{
    if (logbook_ui_last_visible_entry < logbook_ui_entry_count - 1
        && (logbook_ui_current_page - 1) / 2 < 100) {
        logbook_ui_current_page += 2;
        logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2] = logbook_ui_last_visible_entry + 1;
        logbook_ui_refresh();
        gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
    }
}

/**
 * Retreats to the previous page spread if one exists.
 *
 * 0x53F6A0
 */
void logbook_ui_turn_page_left(void)
{
    if (logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2] > 0) {
        logbook_ui_current_page -= 2;
        logbook_ui_refresh();
        gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
    }
}

/**
 * Redraws the entire visible page spread for the current tab and page number.
 *
 * 0x53F6E0
 */
void logbook_ui_refresh(void)
{
    TigArtBlitInfo blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    MesFileEntry mes_file_entry;
    int index;
    char buffer[80];

    // Hide navigation buttons before redrawing. They are re-shown below when
    // appropriate.
    tig_button_hide(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle);
    tig_button_hide(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle);

    // Clear the page area by reblitting the blank page spread background.
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

    // Render entries onto the page spread.
    if (logbook_ui_entry_count != 0) {
        logbook_ui_last_visible_entry = logbook_ui_draw_page_spread(logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2], logbook_ui_entry_count - 1);
    } else {
        logbook_ui_last_visible_entry = 0;
    }

    // Show nagivation buttons where applicable.
    if (logbook_ui_page_spread_starts[(logbook_ui_current_page - 1) / 2] > 0) {
        tig_button_show(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle);
    }

    if (logbook_ui_last_visible_entry < logbook_ui_entry_count - 1) {
        tig_button_show(logbook_ui_page_buttons[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle);
    }

    // Display the tab name in the header of both pages. The "Bloopers & Quotes"
    // easter egg uses mes entry 100 instead of the normal tab index so it can
    // show a distinct heading.
    mes_file_entry.num = logbook_ui_tab == LOGBOOK_UI_TAB_RUMORS_AND_NOTES && logbook_ui_quotes_mode
        ? 100
        : logbook_ui_tab;
    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

    tig_font_push(logbook_ui_font_header);
    for (index = 0; index < 2; index++) {
        snprintf(buffer, sizeof(buffer), "%c%s%c", '-', mes_file_entry.str, '-');
        tig_window_text_write(logbook_ui_window, buffer, &(logbook_ui_page_header_rects[index]));
    }
    tig_font_pop();

    // Write the page numbers (e.g. "-3-" on the left, "-4-" on the right).
    tig_font_push(logbook_ui_font_page_numbers);
    for (index = 0; index < 2; index++) {
        snprintf(buffer, sizeof(buffer), "%c%d%c", '-', logbook_ui_current_page + index, '-');
        tig_window_text_write(logbook_ui_window, buffer, &(logbook_ui_page_footer_rects[index]));
    }
    tig_font_pop();
}

/**
 * Renders entries from `start_entry` up to `max_entry` onto the current page
 * spread, filling the left page first and then overflowing onto the right page.
 * Returns the index of the last entry that was fully rendered.
 *
 * The function renders entries sequentially. When the remaining vertical space
 * on the left page is exhausted it switches to the right page. If an entry
 * still does not fit on the right page (or if both pages are exhausted) the
 * rendering stops.
 *
 * TODO: Review.
 *
 * 0x53F8F0
 */
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

/**
 * Formats and renders a single logbook entry at position `index` into `rect`.
 *
 * 0x53F9E0
 */
int logbook_ui_draw_entry(int index, TigRect* rect, bool dry_run, bool can_truncate)
{
    char buffer[MAX_STRING];
    bool add_line_spacing;

    // Determine whether an extra blank-line gap should follow this entry.
    add_line_spacing = index < logbook_ui_entry_count - 1;
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
        // "Kills & Injuries" entries are fixed-layout.
        add_line_spacing = false;
        break;
    case LOGBOOK_UI_TAB_BACKGROUND:
        logbook_ui_format_background(buffer, index);
        break;
    case LOGBOOK_UI_TAB_KEYS:
        logbook_ui_format_key(buffer, index);
        break;
    }

    return logbook_ui_draw_text(buffer, logbook_ui_entry_fonts[index], rect, dry_run, can_truncate, add_line_spacing);
}

/**
 * Measures or renders `buffer` using `font` inside `rect`.
 *
 * If the text's measured height exceeds the available height, the behaviour
 * is controlled with `can_truncate`. When truncation is allowed, characters
 * are stripped from the end one at a time until it fits and logs a warning. If
 * truncation is not allowed, the function returns `0` immediately.
 *
 * Returns the height of the rendered (or measured) text. If `add_line_spacing`
 * is `true`, additional line heighgt is added to the return value so the caller
 * reserves space for a blank-line gap after this entry. Returns -1 if the
 * buffer is empty.
 *
 * 0x53FAD0
 */
int logbook_ui_draw_text(char* buffer, tig_font_handle_t font, TigRect* rect, bool dry_run, bool can_truncate, bool add_line_spacing)
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

    // Measure the text, trimming it character by character if it is too tall
    // and truncation is permitted.
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

    if (add_line_spacing) {
        // Return height + one blank line so the next entry is visually
        // separated from this one.
        return logbook_ui_line_height + font_info.height;
    }

    return font_info.height;
}

/**
 * Populates the internal arrays for the currently active tab by querying the
 * relevant game system. Also assigns the appropriate display font to each entry
 * based on its state.
 *
 * 0x53FBB0
 */
void logbook_ui_load_data(void)
{
    int index;

    if (logbook_ui_tab == LOGBOOK_UI_TAB_RUMORS_AND_NOTES) {
        RumorLogbookEntry rumors[MAX_RUMORS]; // NOTE: Forces `alloca(72000)`.

        // Retrieve rumors data.
        logbook_ui_entry_count = rumor_get_logbook_data(logbook_ui_obj, rumors);

        if (logbook_ui_quotes_mode) {
            // In the "Bloopers & Quotes" mode, ignore the rumors, and instead
            // load original dev quotes. Entries array is not used.
            logbook_ui_entry_count = mes_num_entries(quotes_mes_file);
            for (index = 0; index < logbook_ui_entry_count; index++) {
                logbook_ui_entry_fonts[index] = logbook_ui_font_default;
            }
        } else {
            for (index = 0; index < logbook_ui_entry_count; index++) {
                logbook_ui_entry_ids[index] = rumors[index].num;
                logbook_ui_entry_datetimes[index] = rumors[index].datetime;

                // Quelled rumors are struck through to indicate they were
                // proven false.
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

        // Retrieve quests data.
        logbook_ui_entry_count = quest_get_logbook_data(logbook_ui_obj, quests);

        for (index = 0; index < logbook_ui_entry_count; index++) {
            logbook_ui_entry_ids[index] = quests[index].num;
            logbook_ui_entry_datetimes[index] = quests[index].datetime;
            logbook_ui_quest_states[index] = quests[index].state;

            // Font reflects the quest's resolution state.
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
                break;
            }
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_REPUTATIONS) {
        ReputationLogbookEntry reps[2000]; // NOTE: Forces `alloca(32000)`.

        // Retrieve reputations data.
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

        // Retrieve blessings and curses data.
        num_blessings = bless_get_logbook_data(logbook_ui_obj, blessings);
        num_curses = curse_get_logbook_data(logbook_ui_obj, curses);
        logbook_ui_entry_count = num_blessings + num_curses;

        // Merge the two sets in chronological order (earliest first).
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

        // Retrieve kill statistics.
        logbook_get_kills(logbook_ui_obj, logbook_ui_kill_stats);

        // Kill stats use default fonts.
        for (index = 0; index < 9; index++) {
            logbook_ui_entry_fonts[index] = logbook_ui_font_default;
        }

        logbook_ui_entry_count = 9;

        // Add injury history:
        //  - Entry ID is the name of critter who inflicted the injury
        //  - The milliseconds of time data is the type of injury
        //
        // All injuries starts striked out (see below).
        index = logbook_find_first_injury(logbook_ui_obj, &desc, &injury);
        while (index != 0) {
            logbook_ui_entry_ids[logbook_ui_entry_count] = desc;
            logbook_ui_entry_datetimes[logbook_ui_entry_count].milliseconds = injury;
            logbook_ui_entry_fonts[logbook_ui_entry_count] = logbook_ui_font_struck;
            logbook_ui_entry_count++;
            index = logbook_find_next_injury(logbook_ui_obj, index, &desc, &injury);
        }

        // For any injury type that is currently active on the critter, change
        // the font of the most recent matching entry from struck-through back
        // to the default (i.e. remove the strikethrough for an injury that has
        // not yet healed).
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

        // Special case - scarring is also in the logbook ego array, but it is
        // managed as effect, rather than being stored in the flags. Actual
        // count of scarring effects does not matter.
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
        size_t remaining_len;
        size_t truncate_pos;
        size_t prev_truncate_pos;
        TigFont font_desc;
        char ch;

        logbook_ui_entry_count = 1;

        // Special case - entry at index 0 is the number of background, font at
        // index 0 will be used for rendering all background text.
        logbook_ui_entry_ids[0] = background_text_get(logbook_ui_obj);
        logbook_ui_entry_fonts[0] = logbook_ui_font_default;

        strcpy(str, background_description_get_body(logbook_ui_entry_ids[0]));
        remaining_len = strlen(str);

        // Measure how many characters fit on one page and split the text at
        // word boundaries until the remainder fits.
        tig_font_push(logbook_ui_entry_fonts[0]);
        font_desc.str = str;
        font_desc.width = logbook_ui_page_content_rects[0].width;
        tig_font_measure(&font_desc);

        prev_truncate_pos = 0;
        curr = str;
        while (font_desc.height > logbook_ui_page_content_rects[0].height) {
            truncate_pos = 0;

            // Scan forward to find the last word boundary that still fits on
            // the page.
            for (pos = 0; pos < remaining_len; pos++) {
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

            // If no word boundary was found (single word longer than a page)
            // stop splitting. The oversized text will be rendered with whatever
            // clipping the rendering system provides.
            if (truncate_pos == 0) {
                break;
            }

            prev_truncate_pos += truncate_pos;

            // Record this page split: store the character offset of the new
            // page's start. The font is the same for all background sub-pages
            // (stored at font index 0).
            logbook_ui_entry_fonts[logbook_ui_entry_count] = logbook_ui_entry_fonts[0];
            logbook_ui_entry_ids[logbook_ui_entry_count] = (int)prev_truncate_pos;
            logbook_ui_entry_count++;

            // Advance curr to the start of the next page and re-measure.
            remaining_len -= truncate_pos;
            font_desc.str = &(curr[truncate_pos]);
            tig_font_measure(&font_desc);
        }

        // Write the one-past-end sentinel so
        // `logbook_ui_format_background_page` can determine the length of the
        // final page's text.
        logbook_ui_entry_ids[logbook_ui_entry_count] = (int)(prev_truncate_pos + remaining_len);
        tig_font_pop();

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_KEYS) {
        // Retrieve keys in possession.
        logbook_ui_entry_count = item_get_keys(logbook_ui_obj, logbook_ui_entry_ids);
        if (logbook_ui_entry_count > 0) {
            for (index = 0; index < logbook_ui_entry_count; index++) {
                logbook_ui_entry_fonts[index] = logbook_ui_font_default;
            }
        }
        return;
    }
}

/**
 * Formats a single "Rumors & Notes" entry into `buffer`.
 *
 * 0x540310
 */
void logbook_ui_format_rumor(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    size_t pos;

    if (logbook_ui_quotes_mode) {
        // Special case - in "Bloopers & Quotes" mode, the entries array is
        // not used used. The index itself is a number of message in
        // `quotes.mes` file.
        mes_file_entry.num = index;
        if (mes_search(quotes_mes_file, &mes_file_entry)) {
            strcpy(buffer, mes_file_entry.str);
        } else {
            buffer[0] = '\0';
        }
    } else {
        // Start with timestamp.
        logbook_ui_format_timestamp(buffer, index);

        // Jump to next line.
        pos = strlen(buffer);
        buffer[pos] = '\n';

        rumor_copy_logbook_str(logbook_ui_obj, logbook_ui_entry_ids[index], &(buffer[pos + 1]));
    }
}

/**
 * Formats the date/time header for the entry at `index` into `buffer`.
 *
 * 0x5403C0
 */
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

    // Retrieve localized month name.
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

/**
 * Formats a single "Quests" entry into `buffer`.
 *
 * 0x540470
 */
void logbook_ui_format_quest(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    size_t pos;

    // Start with timestamp.
    logbook_ui_format_timestamp(buffer, index);

    // Retrieve and append quest state.
    mes_file_entry.num = logbook_ui_quest_states[index] + 19;
    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);
    strcat(buffer, mes_file_entry.str);

    // Jump to next line.
    pos = strlen(buffer);
    buffer[pos] = '\n';

    quest_copy_description(logbook_ui_obj, logbook_ui_entry_ids[index], &(buffer[pos + 1]));
}

/**
 * Formats a single "Reputations" entry into `buffer`.
 *
 * 0x540510
 */
void logbook_ui_format_reputation(char* buffer, int index)
{
    size_t pos;

    // Start with timestamp.
    logbook_ui_format_timestamp(buffer, index);

    // Jump to next line.
    pos = strlen(buffer);
    buffer[pos] = '\n';

    reputation_name(logbook_ui_entry_ids[index], &(buffer[pos + 1]));
}

/**
 * Formats a single "Blessings & Curses" entry into `buffer`.
 *
 * 0x540550
 */
void logbook_ui_format_blessing_or_curse(char* buffer, int index)
{
    size_t pos;

    // Start with timestamp.
    logbook_ui_format_timestamp(buffer, index);

    // Jump to next line.
    pos = strlen(buffer);
    buffer[pos] = '\n';

    // Special case - blessing vs. curses are determined using font.
    if (logbook_ui_entry_fonts[index] == logbook_ui_font_active) {
        bless_copy_name(logbook_ui_entry_ids[index], &(buffer[pos + 1]));
    } else {
        curse_copy_name(logbook_ui_entry_ids[index], &(buffer[pos + 1]));
    }
}

/**
 * Formats a single "Kills & Injuries" entry into `buffer`.
 *
 * 0x5405C0
 */
void logbook_ui_format_kill_or_injury(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    const char* desc_str;
    char tmp[80];
    int desc;

    if (index < 7) {
        // Retrieve stat label.
        mes_file_entry.num = 26 + index;
        mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

        // NOTE: Original code is slightly different but does the same thing.
        if (index == 0) {
            // Index 0 - "Total kills".
            SDL_itoa(logbook_ui_kill_stats[LBK_TOTAL_KILLS], tmp, 10);
            sprintf(buffer, "%s: %s", mes_file_entry.str, tmp);
        } else {
            // Indices 1-6: name of tehe most notable critter in each category.
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
                // No critter for this category yet.
                sprintf(buffer, "%s: -----", mes_file_entry.str);
            }
        }
    } else if (index == 7) {
        // Separator.
        buffer[0] = '\0';
    } else if (index == 8) {
        // "Injury History"
        mes_file_entry.num = 33;
        mes_get_msg(logbook_ui_mes_file, &mes_file_entry);
        strcpy(buffer, mes_file_entry.str);
    } else {
        // Special case - time array used to store injury type.
        mes_file_entry.num = 34 + logbook_ui_entry_datetimes[index].milliseconds;
        mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

        // Entries array stores name of critter who inflicted this injury.
        desc_str = description_get(logbook_ui_entry_ids[index]);
        if (desc_str != NULL) {
            sprintf(buffer, "%s %s", mes_file_entry.str, desc_str);
        }
    }
}

/**
 * Formats one paginated sub-page of the "Background" tab text into `buffer`.
 *
 * 0x540760
 */
void logbook_ui_format_background(char* buffer, int index)
{
    const char* body;
    int start;
    int end;

    // Entry at index 0 is special - it contains background number.
    body = background_description_get_body(logbook_ui_entry_ids[0]);

    if (index != 0) {
        // All other entries are position indices where appropriate sub-page
        // starts.
        start = logbook_ui_entry_ids[index];
    } else {
        // The first sub-page always starts at the beginning.
        start = 0;
    }

    // End of sub-page is start of next page.
    end = logbook_ui_entry_ids[index + 1] - start;

    strncpy(buffer, &(body[start]), end);
    buffer[end] = '\0';
}

/**
 * Formats a single "Keyring Contents" entry into `buffer` by copying the key's
 * name.
 *
 * 0x5407B0
 */
void logbook_ui_format_key(char* buffer, int index)
{
    strcpy(buffer, key_description_get(logbook_ui_entry_ids[index]));
}

/**
 * Debug/QA utility that iterates over every possible rumors and attempts to lay
 * out both the "normal" and "dumb" variant of each rumor using the current page
 * geometry. Any rumor that would overflow the page triggers a truncation
 * warning to console.
 *
 * This function is not called during normal gameplay. The command line switch
 * `-logcheck` actives when the game is about to start (see `main`).
 *
 * 0x5407F0
 */
void logbook_ui_check(void)
{
    int index;
    char buffer[MAX_STRING];
    size_t pos;
    TigRect rect;

    // Retrieve current in-game time.
    logbook_ui_entry_datetimes[0] = sub_45A7C0();

    // Note that the maximum number of rumors is 2000, but this check was
    // probably either future-proof, or was completed before rumor limit was
    // introduced.
    for (index = 0; index < 3000; index++) {
        // Checking normal intelligence.
        rect = logbook_ui_page_content_rects[0];

        logbook_ui_format_timestamp(buffer, 0);
        pos = strlen(buffer);
        buffer[pos] = '\n';

        rumor_copy_logbook_normal_str(index, &(buffer[pos + 1]));
        if (buffer[pos + 1] != '\0') {
            tig_debug_printf("Checking rumor %d\n", index);
            logbook_ui_draw_text(buffer, logbook_ui_font_default, &rect, true, true, true);
        }

        // Checking low intelligence.
        rect = logbook_ui_page_content_rects[0];

        logbook_ui_format_timestamp(buffer, 0);
        pos = strlen(buffer);
        buffer[pos] = '\n';

        rumor_copy_logbook_dumb_str(index, &(buffer[pos + 1]));
        if (buffer[pos + 1] != '\0') {
            tig_debug_printf("Checking dumb rumor %d\n", index);
            logbook_ui_draw_text(buffer, logbook_ui_font_default, &rect, true, true, true);
        }
    }
}
