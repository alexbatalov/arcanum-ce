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
static void sub_53F490(int a1, int a2);
static void sub_53F5F0(int a1, int a2);
static void sub_53F640();
static void sub_53F6A0();
static void sub_53F6E0();
static int sub_53F8F0(int a1, int a2);
static int sub_53F9E0(int index, TigRect* rect, bool a3, bool a4);
static int sub_53FAD0(char* buffer, tig_font_handle_t font, TigRect* rect, bool a4, bool a5, bool a6);
static void sub_53FBB0();
static void sub_540310(char* buffer, int index);
static void sub_5403C0(char* buffer, int index);
static void sub_540470(char* buffer, int index);
static void sub_540510(char* buffer, int index);
static void sub_540550(char* buffer, int index);
static void sub_5405C0(char* buffer, int index);
static void sub_540760(char* buffer, int index);
static void sub_5407B0(char* buffer, int index);

// 0x5C33F0
static tig_window_handle_t logbook_ui_window = TIG_WINDOW_HANDLE_INVALID;

// 0x5C33F8
static TigRect stru_5C33F8[2] = {
    { 0, 41, 800, 400 },
    { 150, 11, 501, 365 },
};

// 0x5C3418
static TigRect stru_5C3418 = { 25, 24, 89, 89 };

// 0x5C3428
static UiButtonInfo stru_5C3428[LOGBOOK_UI_BUTTON_COUNT] = {
    { 213, 77, 272, TIG_BUTTON_HANDLE_INVALID },
    { 675, 77, 273, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C3448
static UiButtonInfo stru_5C3448[LOGBOOK_UI_TAB_COUNT] = {
    { 696, 83, 265, TIG_BUTTON_HANDLE_INVALID },
    { 696, 129, 267, TIG_BUTTON_HANDLE_INVALID },
    { 696, 175, 271, TIG_BUTTON_HANDLE_INVALID },
    { 695, 221, 266, TIG_BUTTON_HANDLE_INVALID },
    { 696, 267, 270, TIG_BUTTON_HANDLE_INVALID },
    { 696, 313, 268, TIG_BUTTON_HANDLE_INVALID },
    { 696, 359, 269, TIG_BUTTON_HANDLE_INVALID },
};

// 0x5C34B8
static TigRect stru_5C34B8 = { 222, 72, 215, 270 };

// 0x5C34C8
static TigRect stru_5C34C8 = { 468, 72, 215, 270 };

// 0x5C34D8
static TigRect stru_5C34D8[2] = {
    { 236, 43, 187, 19 },
    { 482, 43, 187, 19 },
};

// 0x5C34F8
static TigRect stru_5C34F8[2] = {
    { 222, 346, 215, 15 },
    { 468, 346, 215, 15 },
};

// 0x63CBF0
static int dword_63CBF0;

// 0x63CBF4
static int dword_63CBF4[3000];

// 0x63FAD8
static int64_t logbook_ui_obj;

// 0x63FAE0
static tig_font_handle_t dword_63FAE0;

// 0x63FAE4
static int dword_63FAE4[3000];

// 0x6429C4
static tig_font_handle_t dword_6429C4;

// 0x6429C8
static tig_font_handle_t dword_6429C8;

// 0x6429CC
static mes_file_handle_t quotes_mes_file;

// 0x6429D0
static tig_font_handle_t dword_6429D0;

// 0x6429D8
static DateTime stru_6429D8[3000];

// 0x648798
static int dword_648798;

// 0x64879C
static int dword_64879C[100];

// 0x64892C
static tig_font_handle_t dword_64892C;

// 0x648930
static tig_font_handle_t dword_648930;

// 0x648934
static tig_font_handle_t dword_648934;

// 0x64893C
static tig_font_handle_t dword_64893C;

// 0x648938
static int dword_648938;

// 0x648940
static int dword_648940[LBK_COUNT];

// 0x648974
static int dword_648974;

// 0x648978
static int logbook_ui_tab;

// 0x64897C
static int dword_64897C;

// 0x648980
static int dword_648980;

// 0x648984
static mes_file_handle_t logbook_ui_mes_file;

// 0x648988
static tig_font_handle_t dword_648988[3000];

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
    tig_font_create(&font_info, &dword_648934);

    font_info.flags = TIG_FONT_CENTERED;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &dword_6429C8);

    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &dword_6429C4);

    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 120, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &dword_63FAE0);

    font_info.flags = TIG_FONT_STRIKE_THROUGH;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(150, 0, 0);
    font_info.strike_through_color = font_info.color;
    tig_font_create(&font_info, &dword_64892C);

    font_info.flags = TIG_FONT_CENTERED;
    tig_art_interface_id_create(27, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 0);
    tig_font_create(&font_info, &dword_648930);

    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(0, 0, 150);
    tig_font_create(&font_info, &dword_64893C);

    font_info.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_info.art_id));
    font_info.str = NULL;
    font_info.color = tig_color_make(150, 0, 0);
    tig_font_create(&font_info, &dword_6429D0);

    tig_font_push(dword_648934);
    font_info.str = "test";
    font_info.width = 0;
    tig_font_measure(&font_info);
    dword_63CBF0 = font_info.height;
    tig_font_pop();

    dword_64897C = -1;

    for (index = 0; index < 100; index++) {
        dword_64879C[index] = -1;
    }
    dword_64879C[0] = 0;

    logbook_ui_tab = LOGBOOK_UI_TAB_RUMORS_AND_NOTES;
    logbook_ui_obj = OBJ_HANDLE_NULL;
    dword_648798 = 1;
    logbook_ui_initialized = true;

    return true;
}

// 0x53EF40
void logbook_ui_exit()
{
    mes_unload(logbook_ui_mes_file);
    mes_unload(quotes_mes_file);
    tig_font_destroy(dword_648934);
    tig_font_destroy(dword_6429C8);
    tig_font_destroy(dword_6429C4);
    tig_font_destroy(dword_63FAE0);
    tig_font_destroy(dword_64892C);
    tig_font_destroy(dword_648930);
    tig_font_destroy(dword_64893C);
    tig_font_destroy(dword_6429D0);
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
        dword_64879C[index] = -1;
    }
    dword_64879C[0] = 0;

    logbook_ui_obj = OBJ_HANDLE_NULL;
    dword_64897C = -1;
    dword_648798 = 1;
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
        dword_648798 = 1;
        dword_648980 = 1;
        gsound_play_sfx(SND_INTERFACE_BLESS, 1);
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = stru_5C33F8[0].width;
    src_rect.height = stru_5C33F8[0].height;

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
            &(stru_5C33F8[0]),
            &(stru_5C3428[index]),
            TIG_BUTTON_HIDDEN | TIG_BUTTON_MOMENTARY);
    }

    for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
        intgame_button_create_ex(logbook_ui_window,
            &(stru_5C33F8[0]),
            &(stru_5C3448[index]),
            TIG_BUTTON_HIDDEN | TIG_BUTTON_MOMENTARY);
        button_handles[index] = stru_5C3448[index].button_handle;
    }
    tig_button_radio_group_create(LOGBOOK_UI_TAB_COUNT, button_handles, logbook_ui_tab);

    location_origin_set(obj_field_int64_get(logbook_ui_obj, OBJ_F_LOCATION));

    pc_lens.window_handle = logbook_ui_window;
    pc_lens.rect = &stru_5C3418;
    tig_art_interface_id_create(198, 0, 0, 0, &(pc_lens.art_id));
    intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);
    sub_53F490(261, 1);
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

    if (dword_648980) {
        dword_648798 = 1;
    }
    dword_648980 = false;
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
                if (stru_5C3448[index].button_handle == msg->data.button.button_handle) {
                    sub_53F5F0(index, 0);
                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_RELEASED:
            if (stru_5C3428[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle == msg->data.button.button_handle) {
                sub_53F6A0();
                return true;
            }
            if (stru_5C3428[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle == msg->data.button.button_handle) {
                sub_53F640();
                return true;
            }
            return false;
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
                if (stru_5C3448[index].button_handle == msg->data.button.button_handle) {
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
                if (stru_5C3448[index].button_handle == msg->data.button.button_handle) {
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
void sub_53F490(int a1, int a2)
{
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;
    tig_button_handle_t selected_button_handle;
    int selected_button_index;
    int index;

    if (dword_64897C != -1) {
        if (!a2 && a1 == 261) {
            return;
        }
    } else {
        a1 = 261;
    }

    dword_64897C = a1;

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = stru_5C33F8[0].width;
    src_rect.height = stru_5C33F8[0].height;

    dst_rect.x = 165;
    dst_rect.y = 0;
    dst_rect.width = src_rect.width;
    dst_rect.height = src_rect.height;

    blit_info.flags = 0;
    tig_art_interface_id_create(260, 0, 0, 0, &(blit_info.art_id));
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;
    tig_window_blit_art(logbook_ui_window, &blit_info);

    tig_art_interface_id_create(dword_64897C, 0, 0, 0, &(blit_info.art_id));
    dst_rect.x = 172;
    dst_rect.y = 23;
    tig_window_blit_art(logbook_ui_window, &blit_info);

    selected_button_handle = sub_538730(stru_5C3448[0].button_handle);
    selected_button_index = 0;

    for (index = 0; index < LOGBOOK_UI_TAB_COUNT; index++) {
        tig_button_show(stru_5C3448[index].button_handle);

        if (selected_button_handle == stru_5C3448[index].button_handle) {
            selected_button_index = index;
        }
    }

    gsound_play_sfx(SND_INTERFACE_BOOK_SWITCH, 1);
    sub_53F5F0(selected_button_index, a2);
}

// 0x53F5F0
void sub_53F5F0(int tab, int a2)
{
    int index;

    if (a2 == 0) {
        for (index = 0; index < 100; index++) {
            dword_64879C[index] = -1;
        }
        dword_64879C[0] = 0;
        dword_648798 = 1;
        logbook_ui_tab = tab;
    }

    sub_53FBB0();
    sub_53F6E0();
    gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
}

// 0x53F640
void sub_53F640()
{
    if (dword_648974 < dword_648938 - 1
        && (dword_648798 - 1) / 2 < 100) {
        dword_648798 += 2;
        dword_64879C[(dword_648798 - 1) / 2] = dword_648974 + 1;
        sub_53F6E0();
        gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
    }
}

// 0x53F6A0
void sub_53F6A0()
{
    if (dword_64879C[(dword_648798 - 1) / 2] > 0) {
        dword_648798 -= 2;
        sub_53F6E0();
        gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
    }
}

// 0x53F6E0
void sub_53F6E0()
{
    TigArtBlitInfo blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    MesFileEntry mes_file_entry;
    int index;
    char buffer[80];

    tig_button_hide(stru_5C3428[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle);
    tig_button_hide(stru_5C3428[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle);

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = stru_5C33F8[0].width;
    src_rect.height = stru_5C33F8[0].height;

    dst_rect.x = 165;
    dst_rect.y = 0;
    dst_rect.width = src_rect.width;
    dst_rect.height = src_rect.height;

    blit_info.flags = 0;
    tig_art_interface_id_create(260, 0, 0, 0, &(blit_info.art_id));
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;

    tig_window_blit_art(logbook_ui_window, &blit_info);

    if (dword_648938 != 0) {
        dword_648974 = sub_53F8F0(dword_64879C[(dword_648798 - 1) / 2], dword_648938 - 1);
    } else {
        dword_648974 = 0;
    }

    if (dword_64879C[(dword_648798 - 1) / 2] > 0) {
        tig_button_show(stru_5C3428[LOGBOOK_UI_BUTTON_TURN_PAGE_LEFT].button_handle);
    }

    if (dword_648974 < dword_648938 - 1) {
        tig_button_show(stru_5C3428[LOGBOOK_UI_BUTTON_TURN_PAGE_RIGHT].button_handle);
    }

    mes_file_entry.num = logbook_ui_tab == LOGBOOK_UI_TAB_RUMORS_AND_NOTES && dword_648980
        ? 100
        : logbook_ui_tab;
    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

    tig_font_push(dword_648930);
    for (index = 0; index < 2; index++) {
        snprintf(buffer, sizeof(buffer), "%c%s%c", '-', mes_file_entry.str, '-');
        tig_window_text_write(logbook_ui_window, buffer, &(stru_5C34D8[index]));
    }
    tig_font_pop();

    tig_font_push(dword_6429C8);
    for (index = 0; index < 2; index++) {
        snprintf(buffer, sizeof(buffer), "%c%d%c", '-', dword_648798 + index, '-');
        tig_window_text_write(logbook_ui_window, buffer, &(stru_5C34F8[index]));
    }
    tig_font_pop();
}

// TODO: Review.
//
// 0x53F8F0
int sub_53F8F0(int a1, int a2)
{
    TigRect v9;
    int v2;
    bool v3;
    bool v4;
    int v5;
    bool v6;
    int v7;

    v9 = stru_5C34B8;
    v2 = a1;
    v3 = false;
    v4 = true;
    if (a1 > a2) {
        v5 = -1;
        v6 = true;
    } else {
        v5 = 1;
        v6 = false;
    }

    while (v2 != v5 + a2) {
        v7 = sub_53F9E0(v2, &v9, v6, v4);
        v4 = false;
        if (v7 > 0 && v7 <= v9.height) {
            v9.y += v7;
            v9.height -= v7;
        } else {
            if (v3) {
                if (v7 > 0) {
                    v2 += v5;
                }
                break;
            }

            v9 = stru_5C34C8;
            v3 = true;
            v4 = true;
            if (v7 == 0) {
                v2 -= v5;
            }
        }

        v2 += v5;
    }

    return v2 - v5;
}

// 0x53F9E0
int sub_53F9E0(int index, TigRect* rect, bool a3, bool a4)
{
    char buffer[2000];
    bool v1;

    v1 = index < dword_648938 - 1;
    buffer[0] = '\0';

    switch (logbook_ui_tab) {
    case LOGBOOK_UI_TAB_RUMORS_AND_NOTES:
        sub_540310(buffer, index);
        break;
    case LOGBOOK_UI_TAB_QUESTS:
        sub_540470(buffer, index);
        break;
    case LOGBOOK_UI_TAB_REPUTATIONS:
        sub_540510(buffer, index);
        break;
    case LOGBOOK_UI_TAB_BLESSINGS_AND_CURSES:
        sub_540550(buffer, index);
        break;
    case LOGBOOK_UI_TAB_KILLS_AND_INJURES:
        sub_5405C0(buffer, index);
        v1 = false;
        break;
    case LOGBOOK_UI_TAB_BACKGROUND:
        sub_540760(buffer, index);
        break;
    case LOGBOOK_UI_TAB_KEYS:
        sub_5407B0(buffer, index);
        break;
    }

    return sub_53FAD0(buffer, dword_648988[index], rect, a3, a4, v1);
}

// 0x53FAD0
int sub_53FAD0(char* buffer, tig_font_handle_t font, TigRect* rect, bool a4, bool a5, bool a6)
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

        if (!a5 || (pos = strlen(buffer)) == 0) {
            tig_font_pop();
            return 0;
        }

        if (!warned) {
            tig_debug_printf("Had to shorten logbook entry: %s\n", buffer);
        }

        buffer[pos - 1] = '\0';
        warned = true;
    }

    if (!a4) {
        tig_window_text_write(logbook_ui_window, buffer, rect);
    }

    if (a6) {
        tig_font_pop();
        return dword_63CBF0 + font_info.height;
    }

    tig_font_pop();
    return font_info.height;
}

// 0x53FBB0
void sub_53FBB0()
{
    int index;

    if (logbook_ui_tab == LOGBOOK_UI_TAB_RUMORS_AND_NOTES) {
        RumorLogbookEntry rumors[MAX_RUMORS]; // NOTE: Forces `alloca(72000)`.

        dword_648938 = rumor_get_logbook_data(logbook_ui_obj, rumors);

        if (dword_648980) {
            dword_648938 = mes_num_entries(quotes_mes_file);
            for (index = 0; index < dword_648938; index++) {
                dword_648988[index] = dword_648934;
            }
        } else {
            for (index = 0; index < dword_648938; index++) {
                dword_63FAE4[index] = rumors[index].num;
                stru_6429D8[index] = rumors[index].datetime;
                if (rumors[index].quelled) {
                    dword_648988[index] = dword_6429C4;
                } else {
                    dword_648988[index] = dword_648934;
                }
            }
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_QUESTS) {
        QuestLogbookEntry quests[2000]; // NOTE: Forces `alloca(48000)`.

        dword_648938 = quest_get_logbook_data(logbook_ui_obj, quests);

        for (index = 0; index < dword_648938; index++) {
            dword_63FAE4[index] = quests[index].num;
            stru_6429D8[index] = quests[index].datetime;
            dword_63CBF4[index] = quests[index].state;

            switch (dword_63CBF4[index]) {
            case QUEST_STATE_COMPLETED:
                dword_648988[index] = dword_63FAE0;
                break;
            case QUEST_STATE_OTHER_COMPLETED:
            case QUEST_STATE_BOTCHED:
                dword_648988[index] = dword_64892C;
                break;
            case QUEST_STATE_ACCEPTED:
            case QUEST_STATE_ACHIEVED:
                dword_648988[index] = dword_64893C;
                break;
            default:
                dword_648988[index] = dword_648934;
            }
        }

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_REPUTATIONS) {
        ReputationLogbookEntry reps[2000]; // NOTE: Forces `alloca(32000)`.

        dword_648938 = reputation_get_logbook_data(logbook_ui_obj, reps);

        for (index = 0; index < dword_648938; index++) {
            dword_63FAE4[index] = reps[index].reputation;
            stru_6429D8[index] = reps[index].datetime;

            dword_648988[index] = dword_648934;
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
        dword_648938 = num_blessings + num_curses;

        bless_idx = 0;
        curse_idx = 0;
        for (idx = 0; idx < dword_648938; idx++) {
            if (bless_idx < num_blessings
                && (curse_idx == num_curses || datetime_compare(&(blessings[bless_idx].datetime), &(curses[curse_idx].datetime)))) {
                dword_63FAE4[idx] = blessings[bless_idx].id;
                stru_6429D8[idx] = blessings[bless_idx].datetime;
                dword_648988[idx] = dword_64893C;
                bless_idx++;
            } else {
                dword_63FAE4[idx] = curses[curse_idx].id;
                stru_6429D8[idx] = curses[curse_idx].datetime;
                dword_648988[idx] = dword_6429D0;
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

        logbook_get_kills(logbook_ui_obj, dword_648940);

        for (index = 0; index < 9; index++) {
            dword_648988[index] = dword_648934;
        }

        dword_648938 = 9;

        index = logbook_find_first_injury(logbook_ui_obj, &desc, &injury);
        while (index != 0) {
            dword_63FAE4[dword_648938] = desc;
            stru_6429D8[dword_648938].milliseconds = injury;
            dword_648988[dword_648938] = dword_6429C4;
            dword_648938++;
            index = logbook_find_next_injury(logbook_ui_obj, index, &desc, &injury);
        }

        flags = obj_field_int32_get(logbook_ui_obj, OBJ_F_CRITTER_FLAGS);
        if ((flags & OCF_BLINDED) != 0) {
            for (index = dword_648938 - 1; index >= 9; index--) {
                if (stru_6429D8[index].milliseconds == LBI_BLINDED) {
                    dword_648988[index] = dword_648934;
                    break;
                }
            }
        }
        if ((flags & OCF_CRIPPLED_LEGS_BOTH) != 0) {
            for (index = dword_648938 - 1; index >= 9; index--) {
                if (stru_6429D8[index].milliseconds == LBI_CRIPPLED_LEG) {
                    dword_648988[index] = dword_648934;
                    break;
                }
            }
        }
        if ((flags & OCF_CRIPPLED_ARMS_BOTH) != 0) {
            for (index = dword_648938 - 1; index >= 9; index--) {
                if (stru_6429D8[index].milliseconds == LBI_CRIPPLED_ARM) {
                    dword_648988[index] = dword_648934;
                    break;
                }
            }
        }
        if ((flags & OCF_CRIPPLED_ARMS_ONE) != 0) {
            for (index = dword_648938 - 1; index >= 9; index--) {
                if (stru_6429D8[index].milliseconds == LBI_CRIPPLED_ARM) {
                    dword_648988[index] = dword_648934;
                    break;
                }
            }
        }
        cnt = effect_count_effects_of_type(logbook_ui_obj, EFFECT_SCARRING);
        if (cnt > 0) {
            for (index = dword_648938 - 1; index >= 9; index--) {
                if (stru_6429D8[index].milliseconds == LBI_SCARRED) {
                    dword_648988[index] = dword_648934;
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

        dword_648938 = 1;
        dword_63FAE4[0] = background_text_get(logbook_ui_obj);
        dword_648988[0] = dword_648934;

        strcpy(str, background_description_get_body(dword_63FAE4[0]));
        end = strlen(str);

        tig_font_push(dword_648988[0]);
        font_desc.str = str;
        font_desc.width = stru_5C34B8.width;
        tig_font_measure(&font_desc);

        prev_truncate_pos = 0;
        curr = str;
        while (font_desc.height > stru_5C34B8.height) {
            truncate_pos = 0;

            for (pos = 0; pos < end; pos++) {
                ch = curr[pos];
                if (ch == ' ' || ch == '\n') {
                    curr[pos] = '\0';
                    tig_font_measure(&font_desc);
                    curr[pos] = ch;

                    if (font_desc.height > stru_5C34B8.height) {
                        break;
                    }

                    truncate_pos = pos + 1;
                }
            }

            if (truncate_pos == 0) {
                break;
            }

            prev_truncate_pos += truncate_pos;

            dword_648988[dword_648938] = dword_648988[0];
            dword_63FAE4[dword_648938] = prev_truncate_pos;
            dword_648938++;

            end -= truncate_pos;
            font_desc.str = &(curr[truncate_pos]);
            tig_font_measure(&font_desc);
        }

        dword_63FAE4[dword_648938] = prev_truncate_pos + end;
        tig_font_pop();

        return;
    }

    if (logbook_ui_tab == LOGBOOK_UI_TAB_KEYS) {
        dword_648938 = item_get_keys(logbook_ui_obj, dword_63FAE4);
        if (dword_648938 > 0) {
            for (index = 0; index < dword_648938; index++) {
                dword_648988[index] = dword_648934;
            }
        }
        return;
    }
}

// 0x540310
void sub_540310(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    size_t pos;

    if (dword_648980) {
        mes_file_entry.num = index;
        if (mes_search(quotes_mes_file, &mes_file_entry)) {
            strcpy(buffer, mes_file_entry.str);
        } else {
            buffer[0] = '\0';
        }
    } else {
        sub_5403C0(buffer, index);

        pos = strlen(buffer);
        buffer[pos] = '\n';

        rumor_copy_logbook_str(logbook_ui_obj, dword_63FAE4[index], &(buffer[pos + 1]));
    }
}

// 0x5403C0
void sub_5403C0(char* buffer, int index)
{
    DateTime* datetime;
    int year;
    int month;
    int day;
    int hour;
    int minute;
    char am_pm;
    MesFileEntry mes_file_entry;

    datetime = &(stru_6429D8[index]);
    month = datetime_get_month(datetime);
    day = datetime_get_day(datetime);
    year = datetime_get_year(datetime);
    hour = datetime_get_hour(datetime);
    minute = datetime_get_minute(datetime);

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
void sub_540470(char* buffer, int index)
{
    MesFileEntry mes_file_entry;
    size_t pos;

    sub_5403C0(buffer, index);
    mes_file_entry.num = dword_63CBF4[index] + 19;
    mes_get_msg(logbook_ui_mes_file, &mes_file_entry);
    strcat(buffer, mes_file_entry.str);

    pos = strlen(buffer);
    buffer[pos] = '\n';

    quest_copy_description(logbook_ui_obj, dword_63FAE4[index], &(buffer[pos + 1]));
}

// 0x540510
void sub_540510(char* buffer, int index)
{
    size_t pos;

    sub_5403C0(buffer, index);
    pos = strlen(buffer);
    buffer[pos] = '\n';

    reputation_name(dword_63FAE4[index], &(buffer[pos + 1]));
}

// 0x540550
void sub_540550(char* buffer, int index)
{
    size_t pos;

    sub_5403C0(buffer, index);
    pos = strlen(buffer);
    buffer[pos] = '\n';

    if (dword_648988[index] == dword_64893C) {
        bless_copy_name(dword_63FAE4[index], &(buffer[pos + 1]));
    } else {
        curse_copy_name(dword_63FAE4[index], &(buffer[pos + 1]));
    }
}

// 0x5405C0
void sub_5405C0(char* buffer, int index)
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
            SDL_itoa(dword_648940[LBK_TOTAL_KILLS], tmp, 10);
            sprintf(buffer, "%s: %s", mes_file_entry.str, tmp);
        } else {
            switch (index) {
            case 1:
                desc = dword_648940[LBK_MOST_POWERFUL_NAME];
                break;
            case 2:
                desc = dword_648940[LBK_LEAST_POWERFUL_NAME];
                break;
            case 3:
                desc = dword_648940[LBK_MOST_GOOD_NAME];
                break;
            case 4:
                desc = dword_648940[LBK_MOST_EVIL_NAME];
                break;
            case 5:
                desc = dword_648940[LBK_MOST_MAGICAL_NAME];
                break;
            case 6:
                desc = dword_648940[LBK_MOST_TECH_NAME];
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
        mes_file_entry.num = stru_6429D8[index].milliseconds + 34;
        mes_get_msg(logbook_ui_mes_file, &mes_file_entry);

        desc_str = description_get(dword_63FAE4[index]);
        if (desc_str != NULL) {
            sprintf(buffer, "%s %s", mes_file_entry.str, desc_str);
        }
    }
}

// 0x540760
void sub_540760(char* buffer, int index)
{
    const char* body;
    int start;
    int end;

    body = background_description_get_body(dword_63FAE4[0]);
    if (index != 0) {
        start = dword_63FAE4[index];
    } else {
        start = 0;
    }

    end = dword_63FAE4[index + 1] - start;
    strncpy(buffer, &(body[start]), end);
    buffer[end] = '\0';
}

// 0x5407B0
void sub_5407B0(char* buffer, int index)
{
    strcpy(buffer, key_description_get(dword_63FAE4[index]));
}

// 0x5407F0
void logbook_check()
{
    int index;
    char buffer[2000];
    size_t pos;
    TigRect rect;

    stru_6429D8[0] = sub_45A7C0();

    for (index = 0; index < 3000; index++) {
        rect = stru_5C34B8;

        sub_5403C0(buffer, 0);
        pos = strlen(buffer);
        buffer[pos] = '\n';

        rumor_copy_logbook_normal_str(index, &(buffer[pos + 1]));
        if (buffer[pos + 1] != '\0') {
            tig_debug_printf("Checking rumor %d\n", index);
            sub_53FAD0(buffer, dword_648934, &rect, 1, 1, 1);
        }

        rect = stru_5C34B8;

        sub_5403C0(buffer, 0);
        pos = strlen(buffer);
        buffer[pos] = '\n';
        rumor_copy_logbook_dumb_str(index, &(buffer[pos + 1]));
        if (buffer[pos + 1] != '\0') {
            tig_debug_printf("Checking dumb rumor %d\n", index);
            sub_53FAD0(buffer, dword_648934, &rect, 1, 1, 1);
        }
    }
}
