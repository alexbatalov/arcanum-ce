#include "ui/written_ui.h"

#include "game/critter.h"
#include "game/descriptions.h"
#include "game/gsound.h"
#include "game/hrp.h"
#include "game/location.h"
#include "game/mes.h"
#include "game/mp_utils.h"
#include "game/obj.h"
#include "game/player.h"
#include "game/proto.h"
#include "game/snd.h"
#include "game/tech.h"
#include "ui/intgame.h"
#include "ui/types.h"

/**
 * The maximum number of lines in the written message file representing one
 * logical piece of text.
 */
#define TEN 10

/**
 * The maximum number of pages in the book.
 */
#define MAX_BOOK_PAGES 100

/**
 * The number of columns in the newspaper.
 */
#define MAX_NEWSPAPER_COLUMNS 5

/**
 * The first number of the filler article.
 */
#define FIRST_FILLER_ARTICLE 10

typedef enum WrittenMes {
    WRITTEN_MES_BOOK,
    WRITTEN_MES_NOTE,
    WRITTEN_MES_NEWSPAPER,
    WRITTEN_MES_TELEGRAM,
    WRITTEN_MES_PLAQUE,
    WRITTEN_MES_COUNT,
} WrittenMes;

typedef enum WrittenTextAlignment {
    WRITTEN_TEXT_ALIGNMENT_LEFT,
    WRITTEN_TEXT_ALIGNMENT_CENTER,
    WRITTEN_TEXT_ALIGNMENT_RIGHT,
} WrittenTextAlignment;

typedef enum WrittenUiBookButton {
    WRITTEN_UI_BOOK_BUTTON_PREV,
    WRITTEN_UI_BOOK_BUTTON_NEXT,
    WRITTEN_UI_BOOK_BUTTON_COUNT,
} WrittenUiBookButton;

typedef struct WrittenUiElement {
    /* 0000 */ int font_num;
    /* 0004 */ int message_num;
    /* 0008 */ int x;
    /* 000C */ int y;
    /* 0010 */ WrittenTextAlignment alignment;
} WrittenUiElement;

static void written_ui_create();
static void written_ui_destroy();
static bool written_ui_message_filter(TigMessage* msg);
static void written_ui_refresh();
static void written_ui_draw_background(int num, int x, int y);
static void written_ui_draw_element(const char* str, int font_num, int x, int y, WrittenTextAlignment alignment);
static void written_ui_draw_string(const char* str, int font_num, TigRect* rect);
static bool written_ui_draw_paragraph(char* str, int font_num, int centered, TigRect* rect, int* offset_ptr);
static void written_ui_parse(char* str, int* font_num_ptr, int* centered_ptr, char** str_ptr);
static void written_ui_draw_book_side(int side, int* num_ptr, int* offset_ptr);
static int written_ui_draw_page_like(TigRect* rect, int* num_ptr, int* offset_ptr);

/**
 * 0x5CA478
 */
static tig_window_handle_t written_ui_window = TIG_WINDOW_HANDLE_INVALID;

/**
 * Defines the frame of the written UI window (in the original screen
 * coordinate system, that is 800x600).
 *
 * FIX: The height was 399, creating a 1px gap between written UI and interface
 * bar. Besides all background images are 668x400 (accompanied by 132x400
 * sidebar).
 *
 * 0x5CA480
 */
static TigRect written_ui_frame = { 0, 41, 800, 400 };

/**
 * Defines message file names.
 *
 * 0x5CA490
 */
static const char* written_ui_mes_file_names[WRITTEN_MES_COUNT] = {
    /*      WRITTEN_MES_BOOK */ "mes\\gamebook.mes",
    /*      WRITTEN_MES_NOTE */ "mes\\gamenote.mes",
    /* WRITTEN_MES_NEWSPAPER */ "mes\\gamenewspaper.mes",
    /*  WRITTEN_MES_TELEGRAM */ "mes\\gametelegram.mes",
    /*    WRITTEN_MES_PLAQUE */ "mes\\gameplaque.mes",
};

/**
 * Defines a rect where PC lens is located in the written UI.
 *
 * 0x5CA4A8
 */
static TigRect written_ui_lens_rect = { 25, 24, 89, 89 };

/**
 * Defines art numbers of the background images for appropriate UI types.
 *
 * 0x5CA4B8
 */
static int written_ui_backgrounds[WRITTEN_TYPE_COUNT] = {
    /*      WRITTEN_TYPE_BOOK */ 477, // "bookbackground.art"
    /*      WRITTEN_TYPE_NOTE */ 489, // "notebackground.art"
    /* WRITTEN_TYPE_NEWSPAPER */ 487, // "newsbackground.art"
    /*  WRITTEN_TYPE_TELEGRAM */ 491, // "telebackground.art"
    /*     WRITTEN_TYPE_IMAGE */ 0,
    /* WRITTEN_TYPE_SCHEMATIC */ 0,
    /*    WRITTEN_TYPE_PLAQUE */ 633, // "stone_plaque.art"
};

/**
 * Book prev/next buttons.
 *
 * 0x5CA4D8
 */
static UiButtonInfo written_ui_book_buttons[WRITTEN_UI_BOOK_BUTTON_COUNT] = {
    /* WRITTEN_UI_BOOK_BUTTON_PREV */ { 213, 77, 495, TIG_BUTTON_HANDLE_INVALID },
    /* WRITTEN_UI_BOOK_BUTTON_NEXT */ { 675, 77, 496, TIG_BUTTON_HANDLE_INVALID },
};

/**
 * Defines note content frame (in written UI coordinate system).
 *
 * 0x5CA4F8
 */
static TigRect written_ui_note_content_rect = { 349, 49, 215, 300 };

/**
 * Defines telegram content frame (in written UI coordinate system).
 *
 * This rect contains actual telegram text.
 *
 * 0x5CA508
 */
static TigRect written_ui_telegram_content_rect = { 327, 126, 250, 240 };

/**
 * Defines telegram disclaimer frame (in written UI coordinate system).
 *
 * The disclaimer text is message number 1011 ("NO RESPONSIBILITY is assumed
 * by this Company...").
 *
 * 0x5CA518
 */
static TigRect written_ui_telegram_disclaimer_rect = { 335, 90, 234, 20 };

/**
 * Defines plaque content frame (in written UI coordinate system).
 *
 * 0x5CA528
 */
static TigRect written_ui_plaque_content_rect = { 269, 40, 428, 327 };

/**
 * Defines telegram elements.
 *
 * The telegram elements are "headers" on the telegram card. They don't have
 * any special meaning. Their purpose is simply to give the appearance of a
 * real-world telegram.
 *
 * The location of elements is arranged with the telegram background image and
 * should not be changed.
 *
 * 0x5CA538
 */
static WrittenUiElement written_ui_telegram_elements[] = {
    { 481, 1000, 342, 35, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 481, 1001, 342, 41, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 481, 1002, 321, 50, WRITTEN_TEXT_ALIGNMENT_LEFT },
    { 481, 1003, 321, 59, WRITTEN_TEXT_ALIGNMENT_LEFT },
    { 481, 1004, 321, 68, WRITTEN_TEXT_ALIGNMENT_LEFT },
    { 479, 1005, 450, 36, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 479, 1006, 450, 61, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 481, 1007, 558, 35, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 481, 1008, 539, 50, WRITTEN_TEXT_ALIGNMENT_LEFT },
    { 481, 1009, 539, 60, WRITTEN_TEXT_ALIGNMENT_LEFT },
    { 481, 1010, 539, 69, WRITTEN_TEXT_ALIGNMENT_LEFT },
};

/**
 * Defines book page (left and right) content frames (in written UI coordinate
 * system).
 *
 * 0x5CA618
 */
static TigRect written_ui_book_content_rects[2] = {
    { 227, 57, 200, 290 },
    { 476, 57, 200, 290 },
};

/**
 * Defines "The Tarantian" newspaper elements.
 *
 * Just like the other elements, these don't have any special meaining. Their
 * purpose is simply to give the appearance of a real-world newspaper.
 *
 * The location of elements is arranged with the newspaper background image and
 * should not be changed.
 *
 * 0x5CA638
 */
static WrittenUiElement written_ui_newspaper_tarantian_elements[] = {
    { 485, 0, 216, 44, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 484, 1, 453, 41, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 485, 0, 689, 44, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 482, 2, 194, 76, WRITTEN_TEXT_ALIGNMENT_LEFT },
    { 482, 3, 449, 76, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 482, 2, 712, 76, WRITTEN_TEXT_ALIGNMENT_RIGHT },
};

/**
 * Defines "The Vendigroth Times" newspaper elements.
 *
 * Just like the other elements, these don't have any special meaining. Their
 * purpose is simply to give the appearance of a real-world newspaper.
 *
 * The location of elements is arranged with the newspaper background image and
 * should not be changed.
 *
 * 0x5CA6B0
 */
static WrittenUiElement written_ui_vendigroth_times_elements[] = {
    { 485, 0, 216, 44, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 815, 4, 453, 41, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 485, 0, 689, 44, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 482, 2, 194, 76, WRITTEN_TEXT_ALIGNMENT_LEFT },
    { 482, 5, 449, 76, WRITTEN_TEXT_ALIGNMENT_CENTER },
    { 482, 2, 712, 76, WRITTEN_TEXT_ALIGNMENT_RIGHT },
};

/**
 * Defines newspaper content frames.
 *
 * The rect at index 0 represents the newspaper headline. The remaining indexes
 * are newspaper columns.
 *
 * The each content rect is arranged with the newspaper background image and
 * should not be changed.
 *
 * 0x5CA728
 */
static TigRect written_ui_newspaper_content_rects[MAX_NEWSPAPER_COLUMNS] = {
    { 186, 97, 255, 90 },
    { 188, 196, 120, 175 },
    { 328, 196, 120, 175 },
    { 460, 103, 120, 270 },
    { 592, 103, 120, 270 },
};

/**
 * The number of filler articles for the newspapers.
 *
 * 0x67BC68
 */
static int written_ui_num_filler_newspaper_articles;

/**
 * Current message number for the written UI.
 *
 * 0x67BC6C
 */
static int written_ui_num;

/**
 * Array of book content message number (per page).
 *
 * 0x67BC70
 */
static int written_ui_book_contents[MAX_BOOK_PAGES];

/**
 * Buffer for storing concatenated text content.
 *
 * This buffer is only used for notes and telegrams. The maximum number of lines
 * of notes and telegrams is 10, hence the size.
 *
 * 0x67BE00
 */
static char written_ui_text[TEN * MAX_STRING];

/**
 * Array of text offsets for book pages.
 *
 * 0x680C20
 */
static int written_ui_book_offsets[MAX_BOOK_PAGES];

/**
 * Flag indicating if the newspaper is "The Vendigroth Times".
 *
 * When this flag is set, the written UI type is guaranteed to be
 * `WRITTEN_TYPE_NEWSPAPER`.
 *
 * 0x680DB0
 */
static bool written_ui_is_vendigroth_times;

/**
 * Handle to the current message file (corresponds to the current type).
 *
 * 0x680DB4
 */
static mes_file_handle_t written_ui_mes_file;

/**
 * Array of message file handles for written UI files.
 *
 * 0x680DB8
 */
static mes_file_handle_t written_ui_mes_files[WRITTEN_MES_COUNT];

/**
 * Current type of writtne UI.
 *
 * 0x680DCC
 */
static WrittenType written_ui_type;

/**
 * Current page number of a book.
 *
 * 0x680DD0
 */
static int written_ui_book_cur_page;

/**
 * Last page number of a book.
 *
 * 0x680DD4
 */
static int written_ui_book_max_page;

/**
 * Flag indicating whether the written UI is initialized.
 *
 * 0x680DD8
 */
static bool written_ui_mod_loaded;

/**
 * Flag indicating whether the written UI window is currently created.
 *
 * 0x680DDC
 */
static bool written_ui_created;

/**
 * Called when a module is being loaded.
 *
 * 0x56BAA0
 */
bool written_ui_mod_load()
{
    int index;

    if (written_ui_mod_loaded) {
        return false;
    }

    // Load message files for each written content type.
    for (index = 0; index < WRITTEN_MES_COUNT; index++) {
        mes_load(written_ui_mes_file_names[index], &(written_ui_mes_files[index]));
    }

    written_ui_mod_loaded = true;

    written_ui_is_vendigroth_times = false;

    // Calculate the number of filler newspaper articles.
    if (written_ui_mes_files[WRITTEN_MES_NEWSPAPER] != MES_FILE_HANDLE_INVALID) {
        written_ui_num_filler_newspaper_articles = mes_entries_count_in_range(written_ui_mes_files[WRITTEN_MES_NEWSPAPER], 10, 999);
    } else {
        written_ui_num_filler_newspaper_articles = 0;
    }

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x56BB20
 */
void written_ui_mod_unload()
{
    int index;

    if (!written_ui_mod_loaded) {
        return;
    }

    for (index = 0; index < WRITTEN_MES_COUNT; index++) {
        mes_unload(written_ui_mes_files[index]);
        written_ui_mes_files[index] = MES_FILE_HANDLE_INVALID;
    }

    written_ui_mod_loaded = false;
}

/**
 * Toggles the written UI for a given written object.
 *
 * 0x56BB60
 */
void written_ui_start_obj(int64_t written_obj, int64_t pc_obj)
{
    int subtype;
    int start;

    if (!written_ui_mod_loaded) {
        return;
    }

    // Ensure the player character is not dead.
    if (critter_is_dead(pc_obj)) {
        return;
    }

    // Retrieve type of written object and message number.
    subtype = obj_field_int32_get(written_obj, OBJ_F_WRITTEN_SUBTYPE);
    start = obj_field_int32_get(written_obj, OBJ_F_WRITTEN_TEXT_START_LINE);

    if (subtype == WRITTEN_TYPE_SCHEMATIC) {
        // Special case - schematic does not have special UI, instead they are
        // "learned" by the player. The written object is consumed in this
        // process (unless the player already knows this schematic).
        tech_learn_schematic(pc_obj, written_obj);
    } else {
        // Determine if the newspaper is "The Vendigroth Times" (affects
        // background).
        written_ui_is_vendigroth_times = sub_49B290(written_obj) == BP_VENDIGROTH_NEWSPAPER;

        // Proceed to starting the written UI.
        mp_ui_written_start_type(pc_obj, subtype, start);
    }
}

/**
 * Toggles the written UI.
 *
 * 0x56BC00
 */
void written_ui_start_type(WrittenType written_type, int num)
{
    if (!written_ui_mod_loaded) {
        return;
    }

    // Close written UI if it's currently visible.
    if (written_ui_created) {
        written_ui_close();
        return;
    }

    // Obtain message file corresponding to requested type.
    // NOTE: Original code is slightly different, but does the same thing.
    switch (written_type) {
    case WRITTEN_TYPE_BOOK:
        written_ui_mes_file = written_ui_mes_files[WRITTEN_MES_BOOK];
        break;
    case WRITTEN_TYPE_NOTE:
        written_ui_mes_file = written_ui_mes_files[WRITTEN_MES_NOTE];
        break;
    case WRITTEN_TYPE_NEWSPAPER:
        written_ui_mes_file = written_ui_mes_files[WRITTEN_MES_NEWSPAPER];
        break;
    case WRITTEN_TYPE_TELEGRAM:
        written_ui_mes_file = written_ui_mes_files[WRITTEN_MES_TELEGRAM];
        break;
    case WRITTEN_TYPE_PLAQUE:
        written_ui_mes_file = written_ui_mes_files[WRITTEN_MES_PLAQUE];
        break;
    default:
        written_ui_mes_file = MES_FILE_HANDLE_INVALID;
        break;
    }

    if (written_ui_mes_file == MES_FILE_HANDLE_INVALID) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_WRITTEN)) {
        return;
    }

    written_ui_type = written_type;
    written_ui_num = num;
    written_ui_text[0] = '\0';
    written_ui_create();
}

/**
 * Closes the written UI.
 *
 * 0x56BC90
 */
void written_ui_close()
{
    if (!written_ui_mod_loaded) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
        return;
    }

    written_ui_destroy();
    written_ui_is_vendigroth_times = false;
}

/**
 * Creates the written UI window.
 *
 * 0x56BCC0
 */
void written_ui_create()
{
    TigWindowData window_data;
    MesFileEntry mes_file_entry;
    PcLens pc_lens;
    int64_t obj;
    int64_t location;
    int index;

    // Skip if UI is already created.
    if (!written_ui_mod_loaded
        || written_ui_created) {
        return;
    }

    // Set up window properties.
    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect = written_ui_frame;
    window_data.message_filter = written_ui_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

    if (tig_window_create(&window_data, &written_ui_window) != TIG_OK) {
        tig_debug_printf("written_ui_create: ERROR: window create failed!\n");
        exit(0);
    }

    // Draw the sidebar.
    written_ui_draw_background(494, 0, 0);

    // Center viewport on the player (so that the lens display proper
    // surroundings).
    obj = player_get_local_pc_obj();
    location = obj_field_int64_get(obj, OBJ_F_LOCATION);
    location_origin_set(location);

    // Enable the PC lens.
    pc_lens.window_handle = written_ui_window;
    pc_lens.rect = &written_ui_lens_rect;
    tig_art_interface_id_create(231, 0, 0, 0, &(pc_lens.art_id));
    intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);

    switch (written_ui_type) {
    case WRITTEN_TYPE_BOOK:
        // Create navigation buttons for books.
        for (index = 0; index < WRITTEN_UI_BOOK_BUTTON_COUNT; index++) {
            intgame_button_create_ex(written_ui_window,
                &written_ui_frame,
                &(written_ui_book_buttons[index]),
                TIG_BUTTON_HIDDEN | TIG_BUTTON_MOMENTARY);
        }

        // Initialize book state.
        written_ui_book_cur_page = 0;
        written_ui_book_max_page = 0;
        written_ui_book_contents[0] = written_ui_num;
        written_ui_book_offsets[0] = 0;
        break;
    case WRITTEN_TYPE_NOTE:
        // Concatenate up to 10 note lines (which must be consecutive).
        for (index = 0; index < TEN; index++) {
            mes_file_entry.num = index + written_ui_num;
            if (!mes_search(written_ui_mes_file, &mes_file_entry)) {
                break;
            }
            strcat(written_ui_text, mes_file_entry.str);
            strcat(written_ui_text, "\n");
        }
        break;
    case WRITTEN_TYPE_TELEGRAM:
        // Concatenate up to 10 telegram lines (which must be consecutive).
        for (index = 0; index < TEN; index++) {
            mes_file_entry.num = index + written_ui_num;
            if (!mes_search(written_ui_mes_file, &mes_file_entry)) {
                break;
            }
            strcat(written_ui_text, mes_file_entry.str);

            // Unlike the note, the telegram lines are joined with the word
            // "STOP", instead of a newline.
            mes_file_entry.num = 1012; // " STOP "
            mes_get_msg(written_ui_mes_files[WRITTEN_MES_TELEGRAM], &mes_file_entry);
            strcat(written_ui_text, mes_file_entry.str);
        }
        break;
    default:
        break;
    }

    // Redraw written UI content.
    written_ui_refresh();

    // Most of the written types are books, so play open book sound effect.
    gsound_play_sfx(SND_INTERFACE_BOOK_OPEN, 1);

    written_ui_created = true;
}

/**
 * Destroys the written UI window.
 *
 * 0x56BF60
 */
void written_ui_destroy()
{
    if (!written_ui_mod_loaded
        || !written_ui_created) {
        return;
    }

    // Disable the PC lens.
    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);

    if (tig_window_destroy(written_ui_window) == TIG_OK) {
        written_ui_window = TIG_WINDOW_HANDLE_INVALID;
    }

    // Most of the written types are books, so play close book sound effect.
    gsound_play_sfx(SND_INTERFACE_BOOK_CLOSE, 1);

    written_ui_created = false;
}

/**
 * Called when an event occurred.
 *
 * 0x56BFC0
 */
bool written_ui_message_filter(TigMessage* msg)
{
    switch (msg->type) {
    case TIG_MESSAGE_MOUSE:
        // Clicking on the PC lens closes written UI.
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP
            && intgame_pc_lens_check_pt(msg->data.mouse.x, msg->data.mouse.y)) {
            written_ui_close();
            return true;
        }
        break;
    case TIG_MESSAGE_BUTTON:
        if (msg->data.button.state == TIG_BUTTON_STATE_RELEASED) {
            // Handle book navigation button presses.
            if (written_ui_book_buttons[WRITTEN_UI_BOOK_BUTTON_PREV].button_handle == msg->data.button.button_handle) {
                written_ui_book_cur_page -= 2;
                written_ui_refresh();
                gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
                return true;
            }

            if (written_ui_book_buttons[WRITTEN_UI_BOOK_BUTTON_NEXT].button_handle == msg->data.button.button_handle) {
                written_ui_book_cur_page += 2;
                written_ui_refresh();
                gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
                return true;
            }
        }
        break;
    default:
        break;
    }

    return false;
}

/**
 * Refreshes the written UI content.
 *
 * 0x56C050
 */
void written_ui_refresh()
{
    int art_num;
    int num;
    int offset;
    WrittenUiElement* elements;
    int cnt;
    MesFileEntry mes_file_entry;
    int index;
    int font_num;
    int centered;
    char* str;
    TigRect rect;

    // Hide book navigation buttons by default.
    if (written_ui_type == WRITTEN_TYPE_BOOK) {
        tig_button_hide(written_ui_book_buttons[WRITTEN_UI_BOOK_BUTTON_PREV].button_handle);
        tig_button_hide(written_ui_book_buttons[WRITTEN_UI_BOOK_BUTTON_NEXT].button_handle);
    }

    // Determine the background art number.
    if (written_ui_type == WRITTEN_TYPE_IMAGE) {
        art_num = written_ui_num;
    } else if (written_ui_is_vendigroth_times) {
        art_num = 817; // "vendnewsback.art"
    } else {
        art_num = written_ui_backgrounds[written_ui_type];
    }

    // Draw the background.
    written_ui_draw_background(art_num, 132, 0);

    switch (written_ui_type) {
    case WRITTEN_TYPE_BOOK:
        // Draw book pages.
        num = written_ui_book_contents[written_ui_book_cur_page];
        offset = written_ui_book_offsets[written_ui_book_cur_page];
        written_ui_draw_book_side(0, &num, &offset);
        written_ui_draw_book_side(1, &num, &offset);

        // Show navigation buttons based on the current page.
        if (written_ui_book_cur_page > 0) {
            tig_button_show(written_ui_book_buttons[WRITTEN_UI_BOOK_BUTTON_PREV].button_handle);
        }
        if (written_ui_book_cur_page < written_ui_book_max_page - 1 && written_ui_book_max_page < MAX_BOOK_PAGES) {
            tig_button_show(written_ui_book_buttons[WRITTEN_UI_BOOK_BUTTON_NEXT].button_handle);
        }
        break;
    case WRITTEN_TYPE_NOTE:
        // Draw note text.
        written_ui_draw_string(written_ui_text, 497, &written_ui_note_content_rect);
        break;
    case WRITTEN_TYPE_NEWSPAPER:
        // Select newspaper elements for "The Vendigroth Times" vs.
        // "The Tarantian".
        if (written_ui_is_vendigroth_times) {
            elements = written_ui_vendigroth_times_elements;
            cnt = SDL_arraysize(written_ui_vendigroth_times_elements);
        } else {
            elements = written_ui_newspaper_tarantian_elements;
            cnt = SDL_arraysize(written_ui_newspaper_tarantian_elements);
        }

        // Draw newspaper elements.
        for (index = 0; index < cnt; index++) {
            mes_file_entry.num = elements[index].message_num;
            mes_get_msg(written_ui_mes_files[WRITTEN_MES_NEWSPAPER], &mes_file_entry);
            written_ui_draw_element(mes_file_entry.str,
                elements[index].font_num,
                elements[index].x,
                elements[index].y,
                elements[index].alignment);
        }

        // Draw the headline.
        mes_file_entry.num = written_ui_num;
        if (mes_search(written_ui_mes_file, &mes_file_entry)) {
            written_ui_parse(mes_file_entry.str, &font_num, &centered, &str);
            written_ui_draw_paragraph(str, font_num, centered, &(written_ui_newspaper_content_rects[0]), &offset);
        }

        mes_file_entry.num++;
        if (mes_search(written_ui_mes_file, &mes_file_entry)) {
            // Draw the main newspaper article.
            written_ui_parse(mes_file_entry.str, &font_num, &centered, &str);

            // Start with the first column.
            index = 1;
            rect = written_ui_newspaper_content_rects[index];
            while (true) {
                // Draw paragraph until it fit or no more space remains.
                while (!written_ui_draw_paragraph(str, font_num, centered, &rect, &offset)) {
                    // Advance to the next column.
                    index++;
                    if (index >= MAX_NEWSPAPER_COLUMNS) {
                        return;
                    }

                    rect = written_ui_newspaper_content_rects[index];

                    // The paragraph didn't fit into the previous column rect,
                    // the `offset` represents number of consumed characters.
                    str += offset;
                }

                // The paragraph fit into the supplied rect, the `offset` now
                // represents used height.
                rect.height -= offset;
                rect.y += offset;

                // Advance to the next paragraph.
                mes_file_entry.num++;
                if (mes_file_entry.num >= written_ui_num + TEN) {
                    break;
                }

                if (!mes_search(written_ui_mes_file, &mes_file_entry)) {
                    break;
                }

                written_ui_parse(mes_file_entry.str, &font_num, &centered, &str);
            }

            // Check if space remains.
            if (index < MAX_NEWSPAPER_COLUMNS) {
                // Draw filler articles. Note that there is no randomness, the
                // filler articles depend on the newspaper number. This approach
                // guarantees the filler articles are predictable, does not
                // change between reads, save games, and even replays.

                // Select the first filler article.
                mes_file_entry.num = written_ui_num / TEN + FIRST_FILLER_ARTICLE;
                if (mes_file_entry.num >= written_ui_num_filler_newspaper_articles + FIRST_FILLER_ARTICLE) {
                    mes_file_entry.num = FIRST_FILLER_ARTICLE;
                }

                if (mes_search(written_ui_mes_files[WRITTEN_MES_NEWSPAPER], &mes_file_entry)) {
                    written_ui_parse(mes_file_entry.str, &font_num, &centered, &str);

                    // NOTE: The layout approach is exactly the same as above.
                    while (true) {
                        while (!written_ui_draw_paragraph(str, font_num, centered, &rect, &offset)) {
                            index++;
                            if (index >= MAX_NEWSPAPER_COLUMNS) {
                                return;
                            }

                            rect = written_ui_newspaper_content_rects[index];
                            str += offset;
                        }

                        // Advance to the next filler article. Wrap around if
                        // needed.
                        mes_file_entry.num++;
                        if (mes_file_entry.num >= written_ui_num_filler_newspaper_articles + FIRST_FILLER_ARTICLE) {
                            mes_file_entry.num = FIRST_FILLER_ARTICLE;
                        }

                        if (!mes_search(written_ui_mes_files[WRITTEN_MES_NEWSPAPER], &mes_file_entry)) {
                            break;
                        }

                        written_ui_parse(mes_file_entry.str, &font_num, &centered, &str);
                        rect.height -= offset;
                        rect.y += offset;
                    }
                }
            }
        }
        break;
    case WRITTEN_TYPE_TELEGRAM:
        // Draw telegram elements.
        cnt = SDL_arraysize(written_ui_telegram_elements);
        for (index = 0; index < cnt; index++) {
            mes_file_entry.num = written_ui_telegram_elements[index].message_num;
            mes_get_msg(written_ui_mes_files[WRITTEN_MES_TELEGRAM], &mes_file_entry);
            written_ui_draw_element(mes_file_entry.str,
                written_ui_telegram_elements[index].font_num,
                written_ui_telegram_elements[index].x,
                written_ui_telegram_elements[index].y,
                written_ui_telegram_elements[index].alignment);
        }

        // Draw telegram disclaimer.
        mes_file_entry.num = 1011;
        mes_get_msg(written_ui_mes_files[WRITTEN_MES_TELEGRAM], &mes_file_entry);
        written_ui_draw_string(mes_file_entry.str, 481, &written_ui_telegram_disclaimer_rect);

        // Draw telegram text.
        written_ui_draw_string(written_ui_text, 480, &written_ui_telegram_content_rect);
        break;
    case WRITTEN_TYPE_PLAQUE:
        // Draw plaque content (which pretends to be a one-page book).
        num = written_ui_num;
        offset = 0;
        written_ui_draw_page_like(&written_ui_plaque_content_rect, &num, &offset);
        break;
    default:
        break;
    }
}

/**
 * Draws the background image for the written UI.
 *
 * 0x56C590
 */
void written_ui_draw_background(int num, int x, int y)
{
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = written_ui_frame.width;
    src_rect.height = written_ui_frame.height;

    dst_rect.x = x;
    dst_rect.y = y;
    dst_rect.width = src_rect.width;
    dst_rect.height = src_rect.height;

    blit_info.flags = 0;
    tig_art_interface_id_create(num, 0, 0, 0, &(blit_info.art_id));
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;

    tig_window_blit_art(written_ui_window, &blit_info);
}

/**
 * Draws a single text element with specified alignment.
 *
 * 0x56C630
 */
void written_ui_draw_element(const char* str, int font_num, int x, int y, WrittenTextAlignment alignment)
{
    TigFont font_info;
    tig_font_handle_t font_handle;
    TigRect text_rect;

    // Set up font properties.
    font_info.flags = 0;
    tig_art_interface_id_create(font_num, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);

    // Create a new font and make it active.
    tig_font_create(&font_info, &font_handle);
    tig_font_push(font_handle);

    // Measure text to determine placement.
    font_info.str = str;
    font_info.width = 0;
    tig_font_measure(&font_info);

    // Adjust x-coordinate based on alignment.
    switch (alignment) {
    case WRITTEN_TEXT_ALIGNMENT_LEFT:
        text_rect.x = x;
        break;
    case WRITTEN_TEXT_ALIGNMENT_CENTER:
        text_rect.x = x - font_info.width / 2;
        break;
    case WRITTEN_TEXT_ALIGNMENT_RIGHT:
        text_rect.x = x - font_info.width;
        break;
    }

    text_rect.y = y;
    text_rect.width = font_info.width;
    text_rect.height = font_info.height;
    tig_window_text_write(written_ui_window, str, &text_rect);

    // Deactivate the font and destroy it.
    tig_font_pop();
    tig_font_destroy(font_handle);
}

/**
 * Draws a text string within a specified rect.
 *
 * 0x56C750
 */
void written_ui_draw_string(const char* str, int font_num, TigRect* rect)
{
    TigFont font_info;
    tig_font_handle_t font_handle;

    // Set up font properties.
    font_info.flags = 0;
    tig_art_interface_id_create(font_num, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);

    // Create a new font and make it active.
    tig_font_create(&font_info, &font_handle);
    tig_font_push(font_handle);

    tig_window_text_write(written_ui_window, str, rect);

    // Deactivate the font and destroy it.
    tig_font_pop();
    tig_font_destroy(font_handle);
}

/**
 * Draws a paragraph of text.
 *
 * Returns `true` if paragraph fitted into the available rect. In this case
 * `offset_ptr` is set to the height that was required by the paragraph.
 *
 * When the paragraph didn't fit in to the available rect, returns `false`. In
 * this case `offset_ptr` indicates number characters drawn which is also the
 * start position for the next iteration.
 *
 * 0x56C800
 */
bool written_ui_draw_paragraph(char* str, int font_num, int centered, TigRect* rect, int* offset_ptr)
{
    TigFont font_desc;
    tig_font_handle_t font;
    bool rc;

    // Set up font properties.
    font_desc.flags = centered == 1 ? TIG_FONT_CENTERED : 0;
    tig_art_interface_id_create(font_num, 0, 0, 0, &(font_desc.art_id));

    if (written_ui_type == WRITTEN_TYPE_PLAQUE) {
        font_desc.color = tig_color_make(255, 255, 255);
    } else {
        font_desc.color = tig_color_make(0, 0, 0);
    }

    font_desc.str = NULL;

    // Create a new font and make it active.
    tig_font_create(&font_desc, &font);
    tig_font_push(font);

    // Measure text to determine placement.
    font_desc.str = str;
    font_desc.width = rect->width;
    tig_font_measure(&font_desc);

    // Check if the required height for the paragraph exceeds available height.
    if (font_desc.height > rect->height) {
        size_t truncate_pos = 0;
        size_t end = strlen(str);
        size_t pos;
        char ch;

        // Find the last space or newline for truncation.
        for (pos = 0; pos < end; pos++) {
            ch = str[pos];
            if (ch == ' ' || ch == '\n') {
                str[pos] = '\0';
                tig_font_measure(&font_desc);
                str[pos] = ch;

                if (font_desc.height > rect->height) {
                    break;
                }

                truncate_pos = pos;
            }
        }

        // Draw truncated text if a suitable position was found.
        if (truncate_pos > 0) {
            ch = str[truncate_pos];
            str[truncate_pos] = '\0';
            tig_window_text_write(written_ui_window, str, rect);
            str[truncate_pos] = ch;
        }

        // The paragraph didn't fit into the available rect.
        *offset_ptr = (int)truncate_pos;
        rc = false;
    } else {
        // The paragraph fitted into the available rect.
        tig_window_text_write(written_ui_window, str, rect);
        *offset_ptr = font_desc.height;
        rc = true;
    }

    // Deactivate the font and destroy it.
    tig_font_pop();
    tig_font_destroy(font);

    return rc;
}

/**
 * Parses a message string to extract font number, justification, and actual
 * string.
 *
 * 0x56C9F0
 */
void written_ui_parse(char* str, int* font_num_ptr, int* centered_ptr, char** str_ptr)
{
    *font_num_ptr = atoi(str);

    // Move past the number.
    *str_ptr = str;
    while (SDL_isdigit(**str_ptr)) {
        (*str_ptr)++;
    }

    // Check for justificiation letter. We're only interested in centering with
    // 'c', and 'l' (for "left") being the default.
    *centered_ptr = **str_ptr == 'c' || **str_ptr == 'C';

    // Move past the justification letter.
    if (**str_ptr != '\0') {
        (*str_ptr)++;
    }

    // Skip any whitespace.
    if (SDL_isspace(**str_ptr)) {
        (*str_ptr)++;
    }
}

/**
 * Draws one page of a book.
 *
 * 0x56CAA0
 */
void written_ui_draw_book_side(int side, int* num_ptr, int* offset_ptr)
{
    TigRect rect;
    int offset;
    MesFileEntry mes_file_entry;

    // Retrieve content frame for the book side.
    rect = written_ui_book_content_rects[side];

    // Draw the side text.
    offset = written_ui_draw_page_like(&rect, num_ptr, offset_ptr);
    if (offset == -1) {
        // -1 indicates error.
        return;
    }

    *offset_ptr += offset;

    // Check if the next page exists.
    mes_file_entry.num = *num_ptr;
    if (mes_file_entry.num >= written_ui_num + TEN
        || !mes_search(written_ui_mes_file, &mes_file_entry)) {
        return;
    }

    // Set message number/start position for the next page.
    written_ui_book_contents[written_ui_book_cur_page + side + 1] = mes_file_entry.num;
    written_ui_book_offsets[written_ui_book_cur_page + side + 1] = *offset_ptr;

    // Increase max number of pages if needed.
    if (written_ui_book_max_page == written_ui_book_cur_page + side) {
        written_ui_book_max_page++;
    }
}

/**
 * Draws a page-like content for books or plaques.
 *
 * 0x56CB60
 */
int written_ui_draw_page_like(TigRect* rect, int* num_ptr, int* offset_ptr)
{
    MesFileEntry mes_file_entry;
    int font_num;
    int centered;
    char* str;
    TigRect tmp_rect;
    int offset;

    // Check if the start message number is valid.
    mes_file_entry.num = *num_ptr;
    if (mes_file_entry.num >= written_ui_num + TEN
        || !mes_search(written_ui_mes_file, &mes_file_entry)) {
        // Indicate error.
        return -1;
    }

    // Retrieve font, justification, and actual string.
    written_ui_parse(mes_file_entry.str, &font_num, &centered, &str);

    offset = *offset_ptr;
    str += *offset_ptr;
    tmp_rect = *rect;

    // Draw the remaining of the start message.
    if (written_ui_draw_paragraph(str, font_num, centered, &tmp_rect, offset_ptr)) {
        offset = 0;

        do {
            // The parapgraph is fully drawn, `offset_ptr` is the height
            // occupied. Move `y` and reduce remaining `height`.
            tmp_rect.y += *offset_ptr;
            tmp_rect.height -= *offset_ptr;

            // Check if the next message number is valid.
            mes_file_entry.num++;
            if (mes_file_entry.num >= written_ui_num + TEN
                || !mes_search(written_ui_mes_file, &mes_file_entry)) {
                break;
            }

            // Retrieve font, justification, and actual string.
            written_ui_parse(mes_file_entry.str, &font_num, &centered, &str);
        } while (written_ui_draw_paragraph(str, font_num, centered, &tmp_rect, offset_ptr));
    }

    *num_ptr = mes_file_entry.num;
    return offset;
}

/**
 * Retrieves a newspaper headline.
 *
 * NOTE: The code is slightly reorganized.
 *
 * 0x56CCA0
 */
void written_ui_newspaper_headline(int num, char* str)
{
    MesFileEntry mes_file_entry;
    int font_num;
    int centered;
    char* tmp;
    char* pch;

    str[0] = '\0';

    if (!written_ui_mod_loaded) {
        return;
    }

    if (written_ui_mes_files[WRITTEN_MES_NEWSPAPER] == MES_FILE_HANDLE_INVALID) {
        return;
    }

    // Search for the headline message.
    mes_file_entry.num = num;
    if (!mes_search(written_ui_mes_files[WRITTEN_MES_NEWSPAPER], &mes_file_entry)) {
        return;
    }

    // Retrieve font, justification, and actual string.
    written_ui_parse(mes_file_entry.str, &font_num, &centered, &tmp);

    strcpy(str, tmp);

    // Replace newlines with spaces.
    pch = str;
    while (*pch != '\0') {
        if (*pch == '\n') {
            *pch = ' ';
        }
        pch++;
    }
}
