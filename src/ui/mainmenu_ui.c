#include "ui/mainmenu_ui.h"

#include <stdio.h>

#include "game/area.h"
#include "game/background.h"
#include "game/critter.h"
#include "game/descriptions.h"
#include "game/gamelib.h"
#include "game/gfade.h"
#include "game/gmovie.h"
#include "game/gsound.h"
#include "game/hrp.h"
#include "game/item.h"
#include "game/map.h"
#include "game/mes.h"
#include "game/obj.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/player.h"
#include "game/portrait.h"
#include "game/proto.h"
#include "game/reaction.h"
#include "game/script.h"
#include "game/snd.h"
#include "game/stat.h"
#include "game/teleport.h"
#include "game/timeevent.h"
#include "ui/broadcast_ui.h"
#include "ui/charedit_ui.h"
#include "ui/fate_ui.h"
#include "ui/gameuilib.h"
#include "ui/intgame.h"
#include "ui/inven_ui.h"
#include "ui/iso.h"
#include "ui/logbook_ui.h"
#include "ui/options_ui.h"
#include "ui/schematic_ui.h"
#include "ui/scrollbar_ui.h"
#include "ui/sleep_ui.h"
#include "ui/slide_ui.h"
#include "ui/spell_ui.h"
#include "ui/textedit_ui.h"
#include "ui/wmap_rnd.h"
#include "ui/wmap_ui.h"

typedef enum MainMenuUiNewCharHoverMode {
    MMUI_NEW_CHAR_HOVER_MODE_BACKGROUND,
    MMUI_NEW_CHAR_HOVER_MODE_GENDER,
    MMUI_NEW_CHAR_HOVER_MODE_RACE,
} MainMenuUiNewCharHoverMode;

typedef struct S64B870 {
    /* 0000 */ tig_art_id_t art_id;
    /* 0004 */ int max_frame;
    /* 0008 */ int fps;
    /* 000C */ int x;
    /* 0010 */ int y;
} S64B870;

static void sub_5412E0(bool a1);
static TigWindowModalDialogChoice mainmenu_ui_confirm(int num);
static void sub_541830(char* dst, const char* src);
static void sub_5418A0(char* str, TigRect* rect, tig_font_handle_t font, unsigned int flags);
static void mainmenu_ui_create_mainmenu();
static void mainmenu_ui_draw_version();
static bool mainmenu_ui_press_mainmenu_in_play(tig_button_handle_t button_handle);
static bool mainmenu_ui_press_mainmenu_in_play_locked(tig_button_handle_t button_handle);
static void mainmenu_ui_create_options();
static void sub_541D40();
static void mainmenu_ui_destroy_options();
static bool mainmenu_ui_press_options(tig_button_handle_t button_handle);
static void sub_541E20(int a1);
static void mainmenu_ui_load_game_create();
static void sub_542200();
static void mainmenu_ui_load_game_destroy();
static void sub_542280(int a1);
static void sub_5422A0(TigRect* rect);
static bool mainmenu_ui_load_game_execute(int btn);
static bool mainmenu_ui_load_game_button_pressed(tig_button_handle_t button_handle);
static bool mainmenu_ui_load_game_button_released(tig_button_handle_t button_handle);
static void mainmenu_ui_load_game_mouse_up(int a1, int a2);
static void sub_542560();
static void mainmenu_ui_load_game_refresh(TigRect* rect);
static void sub_542D00(char* str, TigRect* rect, tig_font_handle_t font);
static void sub_542DF0(char* str, TigRect* rect, tig_font_handle_t font);
static void sub_542EA0(char* str, TigRect* rect, tig_font_handle_t font);
static void mmUITextWriteCenteredToArray(const char* str, TigRect* rects, int cnt, tig_font_handle_t font);
static char* sub_543040(int index);
static void sub_543060();
static void sub_5430D0();
static bool mainmenu_ui_load_game_handle_delete();
static bool sub_5432B0(const char* name);
static void mainmenu_ui_save_game_create();
static void mainmenu_ui_save_game_destroy();
static bool mainmenu_ui_save_game_execute(int btn);
static bool mainmenu_ui_save_game_button_pressed(tig_button_handle_t button_handle);
static bool mainmenu_ui_save_game_button_released(tig_button_handle_t button_handle);
static void mainmenu_ui_save_game_mouse_up(int x, int y);
static void mainmenu_ui_save_game_refresh(TigRect* rect);
static void sub_544100(const char* str, TigRect* rect, tig_font_handle_t font);
static void sub_544210();
static void sub_544250();
static void sub_544290();
static bool mainmenu_ui_save_game_handle_delete();
static void mainmenu_ui_last_save_create();
static void mainmenu_ui_intro_create();
static void mainmenu_ui_credits_create();
static void mainmenu_ui_last_save_refresh(TigRect* rect);
static void mainmenu_ui_create_single_player();
static void mainmenu_ui_pick_new_or_pregen_create();
static void mainmenu_ui_new_char_create();
static void mainmenu_ui_new_char_refresh(TigRect* rect);
static void mmUISharedCharRefreshFunc(int64_t obj, TigRect* rect);
static bool mainmenu_ui_new_char_button_released(tig_button_handle_t button_handle);
static bool mainmenu_ui_new_char_next_background(int64_t obj, int* background_ptr);
static bool mainmenu_ui_new_char_prev_background(int64_t obj, int* background_ptr);
static bool mainmenu_ui_new_char_prev_gender(int64_t obj);
static bool mainmenu_ui_new_char_set_gender(int64_t obj, int gender);
static bool mainmenu_ui_new_char_next_gender(int64_t obj);
static bool mainmenu_ui_new_char_prev_race(int64_t obj);
static void mainmenu_ui_new_char_set_race(int64_t obj, int race);
static bool mainmenu_ui_new_char_next_race(int64_t obj);
static bool mainmenu_ui_new_char_button_hover(tig_button_handle_t button_handle);
static bool mainmenu_ui_new_char_button_leave(tig_button_handle_t button_handle);
static void mainmenu_ui_new_char_mouse_idle(int x, int y);
static bool mainmenu_ui_new_char_execute(int btn);
static void mainmenu_ui_pregen_char_create();
static void mainmenu_ui_pregen_char_refresh(TigRect* rect);
static bool mainmenu_ui_pregen_char_button_released(tig_button_handle_t button_handle);
static bool mainmenu_ui_pregen_char_execute(int btn);
static void mainmenu_ui_charedit_create();
static void mainmenu_ui_charedit_destroy();
static bool mainmenu_ui_charedit_button_released(tig_button_handle_t button_handle);
static void mainmenu_ui_charedit_refresh(TigRect* rect);
static void mainmenu_ui_shop_create();
static void mainmenu_ui_shop_destroy();
static bool mainmenu_ui_shop_button_released(tig_button_handle_t button_handle);
static void mainmenu_ui_shop_refresh(TigRect* rect);
static bool main_menu_button_create(MainMenuButtonInfo* info, int width, int height);
static bool main_menu_button_create_ex(MainMenuButtonInfo* info, int width, int height, unsigned int flags);
static void mainmenu_ui_refresh_text(tig_window_handle_t window_handle, const char* str, TigRect* rect, unsigned int flags);
static void sub_546DD0();
static void mainmenu_ui_create_shared_radio_buttons();
static bool sub_546EE0(TigMessage* msg);
static void mainmenu_ui_refresh_button_text(int btn, unsigned int flags);
static void sub_547EF0();
static void sub_5480C0(int a1);
static void mmUIWinRefreshScrollBar();
static void sub_548FF0(int a1);
static void sub_549450();
static void sub_5494C0(TextEdit* textedit);
static void sub_549540(TextEdit* textedit);
static void mainmenu_ui_feedback(int num);
static void mainmenu_fonts_init();
static void mainmenu_fonts_exit();
static void sub_549A80();

// 0x5993D0
static int mainmenu_font_nums[MM_FONT_COUNT] = {
    229,
    26,
    27,
    841,
};

// 0x5993E0
static int mainmenu_font_colors[MM_COLOR_COUNT][3] = {
    { 255, 255, 255 },
    { 255, 0, 0 },
    { 0, 0, 255 },
    { 255, 210, 0 },
    { 32, 15, 0 },
    { 0, 255, 0 },
    { 150, 0, 150 },
    { 60, 160, 255 },
    { 255, 128, 0 },
    { 128, 128, 128 },
};

// 0x5C3618
int dword_5C3618 = -1;

// 0x5C361C
static int mainmenu_ui_pregen_char_cnt = 9;

// 0x5C3620
static bool dword_5C3620 = true;

// 0x5C3624
static tig_window_handle_t dword_5C3624 = TIG_WINDOW_HANDLE_INVALID;

// 0x5C3628
static TigRect stru_5C3628 = { 0, 0, 800, 600 };

// 0x5C3638
static TigRect stru_5C3638 = { 0, 0, 800, 600 };

// 0x5C3648
static TigRect stru_5C3648 = { 0, 41, 800, 400 };

// 0x5C3658
static tig_window_handle_t dword_5C3658 = TIG_WINDOW_HANDLE_INVALID;

// 0x5C3660
static TigRect stru_5C3660 = { 0, 0, 800, 41 };

// 0x5C3670
static tig_window_handle_t dword_5C3670[3] = {
    TIG_WINDOW_HANDLE_INVALID,
    TIG_WINDOW_HANDLE_INVALID,
    TIG_WINDOW_HANDLE_INVALID,
};

// 0x5C3680
static TigRect stru_5C3680[3] = {
    { 200, 441, 400, 47 },
    { 0, 441, 200, 159 },
    { 600, 441, 200, 159 },
};

// 0x5C36B0
static bool stru_5C36B0[6][2] = {
    /*        MM_TYPE_DEFAULT */ { false, true },
    /*              MM_TYPE_1 */ { false, true },
    /*        MM_TYPE_IN_PLAY */ { true, false },
    /* MM_TYPE_IN_PLAY_LOCKED */ { true, false },
    /*        MM_TYPE_OPTIONS */ { true, false },
    /*              MM_TYPE_5 */ { false, false },
};

// 0x5C36E0
static MainMenuButtonInfo mainmenu_ui_mainmenu_in_play_buttons[] = {
    { 410, 143, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_SAVE_GAME, 0, 0, { 0 }, -1 },
    { 410, 193, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_LOAD_GAME, 0, 0, { 0 }, -1 },
    { 410, 243, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_OPTIONS, 0, 0, { 0 }, -1 },
    { 410, 293, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 410, 343, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
};

// 0x5C37D0
static MainMenuButtonInfo mainmenu_ui_mainmenu_in_play_locked_buttons[] = {
    { 410, 243, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_OPTIONS, 0, 0, { 0 }, -1 },
    { 410, 293, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 410, 343, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
};

// 0x5C3860
static MainMenuButtonInfo stru_5C3860[] = {
    { 410, 143, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_SAVE_GAME, 0, 0, { 0 }, -1 },
    { 410, 243, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_OPTIONS, 0, 0, { 0 }, -1 },
    { 410, 293, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 410, 343, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
};

// 0x5C3AB0
static MainMenuWindowInfo stru_5C3AB0 = {
    2,
    NULL,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    -1,
    0,
    NULL,
    0,
    0,
    0,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    1,
};

// 0x5C3B48
static MainMenuWindowInfo stru_5C3B48 = {
    2,
    NULL,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    -1,
    0,
    NULL,
    0,
    0,
    0,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    2,
};

// 0x5C3BE0
static MainMenuWindowInfo mainmenu_ui_intro_info = {
    2,
    mainmenu_ui_intro_create,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    -1,
    0,
    NULL,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    5,
};

// 0x5C3C78
static MainMenuWindowInfo mainmenu_ui_credits_window_info = {
    2,
    mainmenu_ui_credits_create,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    -1,
    0,
    NULL,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    5,
};

// 0x5C3FB4
static mes_file_handle_t mainmenu_ui_mainmenu_mes_file = MES_FILE_HANDLE_INVALID;

// NOTE: Write-only. It's impossible to understand its meaning.
//
// 0x5C3FB8
static int dword_5C3FB8 = -1;

// 0x5C3FC0
static TigRect stru_5C3FC0 = { 260, 293, 60, 20 };

// 0x5C3FD0
static TigRect stru_5C3FD0 = { 160, 263, 150, 20 };

// 0x5C3FE0
static TigRect stru_5C3FE0 = { 160, 263, 150, 20 };

// 0x5C3FF0
static TigRect stru_5C3FF0 = { 560, 203, 150, 20 };

// 0x5C4000
static bool dword_5C4000 = true;

// 0x5C4004
static bool dword_5C4004 = true;

// 0x64C2F8
static char byte_64C2F8[128];

// 0x5C4008
static TextEdit stru_5C4008 = {
    0,
    byte_64C2F8,
    23,
    sub_5494C0,
    sub_549540,
    NULL,
};

// 0x5C4030
static int dword_5C4030[2] = {
    229,
    27,
};

// 0x5C4038
static int dword_5C4038 = 171;

// 0x5C403C
static int dword_5C403C = 327;

// 0x5C4040
static int dword_5C4040[3][3] = {
    { 0, 0, 0 },
    { 97, 61, 42 },
    { 100, 0, 0 },
};

// 0x5C4064
static int dword_5C4064[3] = {
    200,
    200,
    200,
};

// 0x5C4070
static int dword_5C4070[3] = {
    -1,
    2,
    -1,
};

// 0x5C407C
static char off_5C407C[] = "...";

// 0x5C5818
static MainMenuButtonInfo mainmenu_ui_mainmenu_no_multiplayer_buttons[] = {
    { 410, 143, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_SINGLE_PLAYER, 0, 0, { 0 }, -1 },
    { 410, 193, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_OPTIONS, 0, 0, { 0 }, -1 },
    { 410, 243, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_CREDITS, 0, 0, { 0 }, -1 },
    { 410, 293, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_0, 0, 0, { 0 }, -1 },
};

// 0x5C4170
static MainMenuWindowInfo mainmenu_ui_mainmenu_window_info = {
    329,
    mainmenu_ui_create_mainmenu,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    460,
    SDL_arraysize(mainmenu_ui_mainmenu_no_multiplayer_buttons),
    mainmenu_ui_mainmenu_no_multiplayer_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    0,
};

// 0x5C4208
static MainMenuWindowInfo mainmenu_ui_mainmenu_in_play_window_info = {
    329,
    NULL,
    NULL,
    0,
    NULL,
    mainmenu_ui_press_mainmenu_in_play,
    NULL,
    NULL,
    NULL,
    30,
    SDL_arraysize(mainmenu_ui_mainmenu_in_play_buttons),
    mainmenu_ui_mainmenu_in_play_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    0,
};

// 0x5C42A0
static MainMenuWindowInfo mainmenu_ui_mainmenu_in_play_locked_window_info = {
    329,
    NULL,
    NULL,
    0,
    NULL,
    mainmenu_ui_press_mainmenu_in_play_locked,
    NULL,
    NULL,
    NULL,
    70,
    SDL_arraysize(mainmenu_ui_mainmenu_in_play_locked_buttons),
    mainmenu_ui_mainmenu_in_play_locked_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    0,
};

// 0x5C4338
static MainMenuWindowInfo stru_5C4338 = {
    329,
    NULL,
    NULL,
    0,
    NULL,
    mainmenu_ui_press_mainmenu_in_play,
    NULL,
    NULL,
    NULL,
    30,
    SDL_arraysize(stru_5C3860),
    stru_5C3860,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    0,
};

// 0x5C43D0
static MainMenuButtonInfo mainmenu_ui_options_buttons[] = {
    { 130, 221, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 130, 261, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 130, 301, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 130, 341, -1, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
};

// 0x5C4490
static TigRect stru_5C4490 = { 84, 67, 89, 89 };

// 0x5C44A0
static MainMenuWindowInfo mainmenu_ui_options_window_info = {
    556,
    mainmenu_ui_create_options,
    mainmenu_ui_destroy_options,
    0,
    NULL,
    mainmenu_ui_press_options,
    NULL,
    NULL,
    NULL,
    4000,
    SDL_arraysize(mainmenu_ui_options_buttons),
    mainmenu_ui_options_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    2,
};

// 0x5C4538
static TigRect stru_5C4538 = { 150, 273, 240, 40 };

// 0x5C4548
static MainMenuButtonInfo mainmenu_ui_load_game_buttons[] = {
    { 675, 55, 321, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7020 },
    { 69, 55, 323, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7002 },
    { 115, 396, 32, TIG_BUTTON_HANDLE_INVALID, -1, -1, 0, { 0 }, 7022 },
};

// 0x5C45D8
static MainMenuButtonInfo stru_5C45D8 = {
    499,
    82,
    747, // texttoggle.art
    TIG_BUTTON_HANDLE_INVALID,
    -1,
    -1,
    0,
    { 0 },
    7025, // "Toggle Info Display"
};

// 0x5C4608
static MainMenuWindowInfo mainmenu_ui_load_game_window_info = {
    745,
    mainmenu_ui_load_game_create,
    mainmenu_ui_load_game_destroy,
    0,
    mainmenu_ui_load_game_button_pressed,
    mainmenu_ui_load_game_button_released,
    NULL,
    NULL,
    NULL,
    -1,
    SDL_arraysize(mainmenu_ui_load_game_buttons),
    mainmenu_ui_load_game_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    mainmenu_ui_load_game_refresh,
    mainmenu_ui_load_game_execute,
    { 42, 120, 145, 213 },
    mainmenu_ui_load_game_mouse_up,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    5,
};

// 0x5C46B0
static TigRect stru_5C46B0 = { 0, 0, 468, 300 };

// 0x5C46C0
static TigRect stru_5C46C0 = { 281, 55, 468, 300 };

// 0x5C46D0
static TigRect stru_5C46D0 = { 289, 192, 451, 18 };

// 0x5C46E0
static TigRect stru_5C46E0 = { 484, 98, 451, 18 };

// 0x5C46F0
static TigRect stru_5C46F0 = { 281, 212, 465, 18 };

// 0x5C4700
static TigRect stru_5C4700 = { 448, 138, 139, 18 };

// 0x5C4710
static TigRect stru_5C4710 = { 448, 126, 139, 18 };

// 0x5C4720
static TigRect stru_5C4720 = { 547, 126, 109, 18 };

// 0x5C4730
static TigRect stru_5C4730 = { 294, 173, 441, 18 };

// 0x5C4740
static TigRect stru_5C4740[4] = {
    { 288, 232, 456, 18 },
    { 300, 252, 432, 18 },
    { 329, 272, 386, 18 },
    { 348, 292, 338, 18 },
};

// 0x5C4780
static TigRect stru_5C4780 = { 84, 10, 89, 89 };

// 0x5C4790
static bool dword_5C4790 = true;

// 0x5C4798
static TigRect stru_5C4798 = { 213, 111, 12, 232 };

// 0x5C47A8
static MainMenuButtonInfo mainmenu_ui_save_game_buttons[] = {
    { 675, 55, 321, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7021 },
    { 69, 55, 323, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7002 },
    { 115, 396, 32, TIG_BUTTON_HANDLE_INVALID, -1, -1, 0, { 0 }, 7022 },
};

// 0x5C4838
static MainMenuButtonInfo stru_5C4838 = {
    499,
    82,
    747,
    TIG_BUTTON_HANDLE_INVALID,
    -1,
    -1,
    0,
    { 0 },
    7025,
};

// 0x5C4868
static MainMenuWindowInfo mainmenu_ui_save_game_window_info = {
    745, // saveloadbackground.art
    mainmenu_ui_save_game_create,
    mainmenu_ui_save_game_destroy,
    0,
    mainmenu_ui_save_game_button_pressed,
    mainmenu_ui_save_game_button_released,
    NULL,
    NULL,
    NULL,
    -1,
    SDL_arraysize(mainmenu_ui_save_game_buttons),
    mainmenu_ui_save_game_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    mainmenu_ui_save_game_refresh,
    mainmenu_ui_save_game_execute,
    { 42, 120, 145, 213 },
    mainmenu_ui_save_game_mouse_up,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    5,
};

// 0x5C4900
static MainMenuWindowInfo mainmenu_ui_last_save_window_info = {
    2, // "black.art"
    mainmenu_ui_last_save_create,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    -1,
    0,
    NULL,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    mainmenu_ui_last_save_refresh,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    5,
};

// 0x5C4998
static MainMenuButtonInfo mainmenu_ui_single_player_buttons[] = {
    { 410, 143, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_PICK_NEW_OR_PREGEN, 0, 0, { 0 }, -1 },
    { 410, 193, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_LOAD_GAME, 0, 0, { 0 }, -1 },
    { 410, 243, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_LAST_SAVE_GAME, 0, 0, { 0 }, -1 },
    { 410, 293, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_INTRO, 0, 0, { 0 }, -1 },
    { 410, 343, -1, TIG_BUTTON_HANDLE_INVALID, -2, 0, 0, { 0 }, -1 },
};

// 0x5C4A88
static MainMenuWindowInfo mainmenu_ui_single_player_window_info = {
    331,
    mainmenu_ui_create_single_player,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    50,
    SDL_arraysize(mainmenu_ui_single_player_buttons),
    mainmenu_ui_single_player_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    2,
};

// 0x5C4B20
static MainMenuButtonInfo mainmenu_ui_pick_new_or_pregen_buttons[] = {
    { 410, 143, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_PREGEN_CHAR, 0, 0, { 0 }, -1 },
    { 410, 193, -1, TIG_BUTTON_HANDLE_INVALID, MM_WINDOW_NEW_CHAR, 0, 0, { 0 }, -1 },
    { 410, 243, -1, TIG_BUTTON_HANDLE_INVALID, -2, 0, 0, { 0 }, -1 },
};

// 0x5C4BB0
static MainMenuWindowInfo mainmenu_ui_pick_new_or_pregen_window_info = {
    329,
    mainmenu_ui_pick_new_or_pregen_create,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    420,
    SDL_arraysize(mainmenu_ui_pick_new_or_pregen_buttons),
    mainmenu_ui_pick_new_or_pregen_buttons,
    0,
    0,
    0xD,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    NULL,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    5,
};

// 0x5C4C48
static MainMenuButtonInfo mainmenu_ui_new_char_buttons[] = {
    { 675, 55, 321, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7003 },
    { 69, 55, 323, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7002 },
    { 43, 118, 283, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 244, 118, 284, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 42, 331, 757, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 255, 332, 758, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 42, 381, 757, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 255, 382, 758, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 42, 281, 757, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 255, 282, 758, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
};

// 0x5C4E28
static MainMenuWindowInfo mainmenu_ui_new_char_window_info = {
    765, // createcharacterbase.art
    mainmenu_ui_new_char_create,
    NULL,
    1,
    NULL,
    mainmenu_ui_new_char_button_released,
    mainmenu_ui_new_char_button_hover,
    mainmenu_ui_new_char_button_leave,
    mainmenu_ui_new_char_mouse_idle,
    -1,
    8, // TODO: `mainmenu_ui_new_char_buttons` has 10 buttons, figure out why only 8 are used.
    mainmenu_ui_new_char_buttons,
    0,
    0,
    0x5,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    mainmenu_ui_new_char_refresh,
    mainmenu_ui_new_char_execute,
    { 184, 35, 145, 196 },
    NULL,
    { 347, 42, 10, 184 },
    NULL,
    0,
    0,
    0,
    0,
    0xB,
};

// 0x5C4EC0
static TigRect mainmenu_ui_shared_char_stats_title_rect = { 339, 23, 409, 19 };

// 0x5C4ED0
static TigRect mainmenu_ui_shared_char_name_rect = { 46, 183, 228, 19 };

// 0x5C4EE0
static TigRect mainmenu_ui_new_char_name_rect = { 46, 183, 228, 18 };

// 0x5C4EF0
static TigRect mainmenu_ui_shared_char_gender_rect = { 66, 243, 186, 19 };

// 0x5C4F00
static TigRect mainmenu_ui_new_char_gender_hover_rect = { 22, 231, 276, 39 };

// 0x5C4F10
static TigRect mainmenu_ui_shared_char_race_rect = { 66, 293, 186, 19 };

// 0x5C4F20
static TigRect mainmenu_ui_new_char_race_hover_rect = { 22, 281, 276, 40 };

// 0x5C4F30
static TigRect mainmenu_ui_shared_char_background_rect = { 66, 343, 186, 19 };

// 0x5C4F40
static TigRect stru_5C4F40 = { 22, 331, 276, 40 };

// 0x5C4F50
static TigRect stru_5C4F50[15] = {
    { 346, 46, 50, 15 },
    { 346, 66, 50, 15 },
    { 346, 86, 50, 15 },
    { 346, 106, 50, 15 },
    { 406, 46, 50, 15 },
    { 406, 66, 50, 15 },
    { 406, 86, 50, 15 },
    { 406, 106, 50, 15 },
    { 472, 46, 139, 15 },
    { 472, 66, 139, 15 },
    { 472, 86, 139, 15 },
    { 472, 106, 139, 15 },
    { 618, 46, 142, 15 },
    { 618, 65, 147, 15 },
    { 618, 86, 147, 15 },
};

// 0x5C5040
static TigRect stru_5C5040 = { 618, 105, 145, 15 };

// 0x5C5050
static TigRect mainmenu_ui_shared_char_stats_view_rect = { 346, 46, 729, 74 };

// 0x5C5060
static TigRect mainmenu_ui_shared_char_portrait_rect = { 95, 28, 64, 64 };

// 0x5C5070
static TigRect mainmenu_ui_shared_char_desc_view_rect = { 350, 155, 395, 212 };

// 0x5C5080
static TigRect mainmenu_ui_shared_char_desc_title_rect = { 350, 159, 395, 19 };

// 0x5C5090
static TigRect mainmenu_ui_shared_char_desc_body_rect = { 350, 185, 395, 182 };

// 0x5C50A0
static MainMenuButtonInfo mainmenu_ui_new_char_name_button = {
    45,
    224,
    -1,
    TIG_BUTTON_HANDLE_INVALID,
    -1,
    -1,
    0,
    { 0 },
    0,
};

// 0x5C50D0
static MainMenuButtonInfo stru_5C50D0 = {
    22,
    272,
    -1,
    TIG_BUTTON_HANDLE_INVALID,
    -1,
    -1,
    0,
    { 0 },
    0,
};

// 0x5C5100
static MainMenuButtonInfo stru_5C5100 = {
    22,
    322,
    -1,
    TIG_BUTTON_HANDLE_INVALID,
    -1,
    -1,
    0,
    { 0 },
    0,
};

// 0x5C5130
static int dword_5C5130[] = {
    STAT_STRENGTH,
    STAT_CONSTITUTION,
    STAT_DEXTERITY,
    STAT_BEAUTY,
    STAT_INTELLIGENCE,
    STAT_WILLPOWER,
    STAT_PERCEPTION,
    STAT_CHARISMA,
    STAT_CARRY_WEIGHT,
    STAT_DAMAGE_BONUS,
    STAT_AC_ADJUSTMENT,
    STAT_SPEED,
    STAT_HEAL_RATE,
    STAT_POISON_RECOVERY,
    STAT_REACTION_MODIFIER,
};

// 0x5C5170
static struct {
    int body_type;
    bool available_for_female;
} stru_5C5170[] = {
    /*     RACE_HUMAN */ { TIG_ART_CRITTER_BODY_TYPE_HUMAN, true },
    /*     RACE_DWARF */ { TIG_ART_CRITTER_BODY_TYPE_DWARF, false },
    /*       RACE_ELF */ { TIG_ART_CRITTER_BODY_TYPE_ELF, true },
    /*  RACE_HALF_ELF */ { TIG_ART_CRITTER_BODY_TYPE_HUMAN, true },
    /*     RACE_GNOME */ { TIG_ART_CRITTER_BODY_TYPE_HALFLING, false },
    /*  RACE_HALFLING */ { TIG_ART_CRITTER_BODY_TYPE_HALFLING, false },
    /*  RACE_HALF_ORC */ { TIG_ART_CRITTER_BODY_TYPE_HUMAN, true },
    /* RACE_HALF_OGRE */ { TIG_ART_CRITTER_BODY_TYPE_HALF_OGRE, false },
};

// 0x5C51B0
static MainMenuButtonInfo mainmenu_ui_pregen_char_buttons[] = {
    { 675, 55, 321, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7003 },
    { 69, 55, 323, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7002 },
    { 43, 118, 283, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
    { 244, 118, 284, TIG_BUTTON_HANDLE_INVALID, -1, 0, 0, { 0 }, -1 },
};

// 0x5C5270
static MainMenuWindowInfo mainmenu_ui_pregen_char_window_info = {
    766,
    mainmenu_ui_pregen_char_create,
    NULL,
    1,
    NULL,
    mainmenu_ui_pregen_char_button_released,
    NULL,
    NULL,
    NULL,
    -1,
    SDL_arraysize(mainmenu_ui_pregen_char_buttons),
    mainmenu_ui_pregen_char_buttons,
    0,
    0,
    0x5,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    mainmenu_ui_pregen_char_refresh,
    mainmenu_ui_pregen_char_execute,
    { 184, 35, 145, 196 },
    NULL,
    { 347, 42, 10, 184 },
    NULL,
    0,
    0,
    0,
    0,
    0xB,
};

// 0x5C5308
static int mainmenu_ui_pregen_char_idx = 1;

// 0x5C5310
static MainMenuButtonInfo mainmenu_ui_charedit_buttons[] = {
    { 675, 55, 321, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7003 },
    { 69, 55, 323, TIG_BUTTON_HANDLE_INVALID, -2, -1, 1, { 0 }, 7002 },
};

// 0x5C5370
static MainMenuWindowInfo mainmenu_ui_charedit_info = {
    -1,
    mainmenu_ui_charedit_create,
    mainmenu_ui_charedit_destroy,
    0,
    NULL,
    mainmenu_ui_charedit_button_released,
    NULL,
    NULL,
    NULL,
    -1,
    SDL_arraysize(mainmenu_ui_charedit_buttons),
    mainmenu_ui_charedit_buttons,
    0,
    0,
    0x5,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    mainmenu_ui_charedit_refresh,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    0xC,
};

// 0x5C5408
static MainMenuButtonInfo mainmenu_ui_shop_buttons[] = {
    { 675, 55, 321, TIG_BUTTON_HANDLE_INVALID, -1, -1, 1, { 0 }, 7003 },
    { 69, 55, 323, TIG_BUTTON_HANDLE_INVALID, -2, -1, 1, { 0 }, 7002 },
};

// 0x5C5468
static MainMenuWindowInfo mainmenu_ui_shop_info = {
    -1,
    mainmenu_ui_shop_create,
    mainmenu_ui_shop_destroy,
    0,
    NULL,
    mainmenu_ui_shop_button_released,
    NULL,
    NULL,
    NULL,
    -1,
    SDL_arraysize(mainmenu_ui_shop_buttons),
    mainmenu_ui_shop_buttons,
    0,
    0,
    0x5,
    {
        { -1, 0, 0 },
        { -1, 0, 0 },
    },
    mainmenu_ui_shop_refresh,
    NULL,
    { 0 },
    NULL,
    { 0 },
    NULL,
    0,
    0,
    0,
    -1,
    0xE,
};

// 0x5C3A40
static MainMenuWindowInfo* main_menu_window_info[MM_WINDOW_COUNT] = {
    /*                       MM_WINDOW_0 */ &stru_5C3AB0,
    /*                       MM_WINDOW_1 */ &stru_5C3B48,
    /*                MM_WINDOW_MAINMENU */ &mainmenu_ui_mainmenu_window_info,
    /*        MM_WINDOW_MAINMENU_IN_PLAY */ &mainmenu_ui_mainmenu_in_play_window_info,
    /* MM_WINDOW_MAINMENU_IN_PLAY_LOCKED */ &mainmenu_ui_mainmenu_in_play_locked_window_info,
    /*           MM_WINDOW_SINGLE_PLAYER */ &mainmenu_ui_single_player_window_info,
    /*                 MM_WINDOW_OPTIONS */ &mainmenu_ui_options_window_info,
    /*               MM_WINDOW_LOAD_GAME */ &mainmenu_ui_load_game_window_info,
    /*               MM_WINDOW_SAVE_GAME */ &mainmenu_ui_save_game_window_info,
    /*          MM_WINDOW_LAST_SAVE_GAME */ &mainmenu_ui_last_save_window_info,
    /*                   MM_WINDOW_INTRO */ &mainmenu_ui_intro_info,
    /*      MM_WINDOW_PICK_NEW_OR_PREGEN */ &mainmenu_ui_pick_new_or_pregen_window_info,
    /*                MM_WINDOW_NEW_CHAR */ &mainmenu_ui_new_char_window_info,
    /*             MM_WINDOW_PREGEN_CHAR */ &mainmenu_ui_pregen_char_window_info,
    /*                MM_WINDOW_CHAREDIT */ &mainmenu_ui_charedit_info,
    /*                    MM_WINDOW_SHOP */ &mainmenu_ui_shop_info,
    /*                 MM_WINDOW_CREDITS */ &mainmenu_ui_credits_window_info,
    /*                      MM_WINDOW_26 */ &stru_5C4338,
};

// 0x64B870
static S64B870 stru_64B870[2];

// 0x64B898
static GameSaveInfo mainmenu_ui_gsi;

// 0x64BBF8
static GameSaveList mainmenu_ui_gsl;

// 0x64BC04
static tig_font_handle_t dword_64BC04[3];

// 0x64BC10
static tig_font_handle_t dword_64BC10[3];

// 0x64BC1C
static char byte_64BC1C[1000];

// 0x64C004
static MainMenuWindowType mainmenu_ui_window_stack[50];

// 0x64C0CC
static tig_font_handle_t dword_64C0CC[2][3];

// 0x64C0F0
static char byte_64C0F0[128];

// 0x64C170
static tig_font_handle_t mainmenu_ui_fonts_tbl[MM_FONT_COUNT][MM_COLOR_COUNT];

// 0x64C210
static tig_font_handle_t dword_64C210[2];

// 0x64C218
static tig_font_handle_t dword_64C218[2];

// 0x64C220
static ScrollbarId stru_64C220;

// 0x64C228
static tig_font_handle_t dword_64C228[2][3];

// 0x64C240
static tig_font_handle_t dword_64C240;

// 0x64C244
static MainMenuType mainmenu_ui_type;

// 0x64C260
static ScrollbarUiControlInfo stru_64C260;

// 0x64C2F4
static mes_file_handle_t mainmenu_ui_rules_mainmenu_mes_file;

// 0x64C378
static int dword_64C378;

// 0x64C37C
static char* dword_64C37C;

// 0x64C380
static bool mainmenu_ui_initialized;

// 0x64C384
static bool mainmenu_ui_active;

// 0x64C388
static bool dword_64C388;

// 0x64C38C
static bool dword_64C38C;

// 0x64C390
static int mainmenu_ui_num_windows;

// 0x64C394
static char byte_64C394[128];

// 0x64C414
static MainMenuWindowType mainmenu_ui_window_type;

// 0x64C418
static bool mainmenu_ui_start_new_game;

// 0x64C41C
int64_t* dword_64C41C;

// 0x64C420
int dword_64C420;

// 0x64C424
static bool mainmenu_ui_auto_equip_items_on_start;

// 0x64C428
static bool dword_64C428;

// 0x64C42C
static int dword_64C42C[3];

// 0x64C438
static bool mainmenu_ui_was_compact_interface;

// 0x64C43C
static int dword_64C43C;

// 0x64C440
static int dword_64C440;

// 0x64C444
static bool mainmenu_ui_gsi_loaded;

// 0x64C448
static int mainmenu_ui_progressbar_max_value;

// 0x64C44C
static int mainmenu_ui_progressbar_value;

// 0x64C450
static bool dword_64C450;

// 0x64C454
static ChareditMode dword_64C454;

// 0x64C458
static MainMenuUiNewCharHoverMode mainmenu_ui_new_char_hover_mode;

// 0x64C460
static int64_t qword_64C460;

// 0x64C468
static int dword_64C468;

// 0x540930
bool mainmenu_ui_init(GameInitInfo* init_info)
{
    int index;
    TigFont font_desc;
    MesFileEntry mes_file_entry;

    (void)init_info;

    if (mainmenu_ui_mainmenu_mes_file == MES_FILE_HANDLE_INVALID) {
        if (!mes_load("mes\\mainmenu.mes", &mainmenu_ui_mainmenu_mes_file)) {
            return false;
        }
    }

    if (!mes_load("rules\\mainmenu.mes", &mainmenu_ui_rules_mainmenu_mes_file)) {
        return false;
    }

    settings_register(&settings, "show version", "0", NULL);

    mainmenu_fonts_init();

    for (index = 0; index < 3; index++) {
        font_desc.flags = 0;
        if (index == 1) {
            font_desc.flags = TIG_FONT_BLEND_ADD;
        }
        tig_art_interface_id_create(dword_5C403C, 0, 0, 0, &(font_desc.art_id));
        font_desc.str = NULL;
        font_desc.color = tig_color_make(dword_5C4040[index][0], dword_5C4040[index][1], dword_5C4040[index][2]);
        tig_font_create(&font_desc, &(dword_64C228[0][index]));

        tig_art_interface_id_create(dword_5C403C, 0, 0, 0, &(font_desc.art_id));
        font_desc.color = index < 2
            ? tig_color_make(dword_5C4040[index][0], dword_5C4040[index][1], dword_5C4040[index][2])
            : tig_color_make(240, 15, 15);
        tig_font_create(&font_desc, &(dword_64C228[1][index]));
    }

    for (index = 0; index < 3; index++) {
        font_desc.flags = 0;
        tig_art_interface_id_create(dword_5C4038, 0, 0, 0, &(font_desc.art_id));
        font_desc.str = NULL;
        font_desc.color = tig_color_make(dword_64C42C[index], dword_64C42C[index], dword_64C42C[index]);
        tig_font_create(&font_desc, &(dword_64C0CC[0][index]));

        tig_art_interface_id_create(dword_5C4038, 0, 0, 0, &(font_desc.art_id));
        font_desc.color = index < 2
            ? tig_color_make(dword_64C42C[index], dword_64C42C[index], dword_64C42C[index])
            : tig_color_make(240, 15, 15);
        tig_font_create(&font_desc, &(dword_64C0CC[1][index]));
    }

    for (index = 0; index < 3; index++) {
        font_desc.flags = 0;
        tig_art_interface_id_create(dword_5C4038, 0, 0, 0, &(font_desc.art_id));
        font_desc.str = NULL;
        font_desc.color = tig_color_make(dword_5C4064[index], dword_5C4064[index], dword_5C4064[index]);
        tig_font_create(&font_desc, &(dword_64BC04[index]));

        tig_art_interface_id_create(dword_5C4038, 0, 0, 0, &(font_desc.art_id));
        font_desc.color = index < 2
            ? tig_color_make(dword_5C4064[index], dword_5C4064[index], dword_5C4064[index])
            : tig_color_make(240, 15, 15);
        tig_font_create(&font_desc, &(dword_64BC10[index]));
    }

    for (index = 0; index < 2; index++) {
        font_desc.flags = 0;
        tig_art_interface_id_create(dword_5C4030[index], 0, 0, 0, &(font_desc.art_id));
        font_desc.str = NULL;
        font_desc.color = tig_color_make(250, 250, 250);
        tig_font_create(&font_desc, &(dword_64C210[index]));

        font_desc.flags = 0;
        tig_art_interface_id_create(dword_5C4030[index], 0, 0, 0, &(font_desc.art_id));
        font_desc.str = NULL;
        font_desc.color = tig_color_make(255, 50, 50);
        tig_font_create(&font_desc, &(dword_64C218[index]));
    }

    font_desc.flags = 0;
    tig_art_interface_id_create(dword_5C4030[0], 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 210, 0);
    tig_font_create(&font_desc, &dword_64C240);

    mes_file_entry.num = 500;
    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
    strncpy(byte_64C2F8, mes_file_entry.str, 23);

    mainmenu_ui_initialized = true;
    mainmenu_ui_window_type = MM_WINDOW_0;

    dword_64C388 = true;
    mainmenu_ui_start(MM_TYPE_DEFAULT);
    dword_64C388 = false;

    gamelib_thumbnail_size_set(468, 300);
    mainmenu_ui_start_new_game = false;
    sub_549A80();
    dword_64C37C = NULL;

    return true;
}

// 0x541050
void mainmenu_ui_exit()
{
    int index;

    sub_5412E0(true);
    mainmenu_fonts_exit();

    for (index = 0; index < 3; index++) {
        tig_font_destroy(dword_64C228[0][index]);
        tig_font_destroy(dword_64C228[1][index]);
        tig_font_destroy(dword_64C0CC[0][index]);
        tig_font_destroy(dword_64C0CC[1][index]);
        tig_font_destroy(dword_64BC04[index]);
        tig_font_destroy(dword_64BC10[index]);
    }

    for (index = 0; index < 2; index++) {
        tig_font_destroy(dword_64C210[index]);
        tig_font_destroy(dword_64C218[index]);
    }

    tig_font_destroy(dword_64C240);
    mes_unload(mainmenu_ui_rules_mainmenu_mes_file);
    mes_unload(mainmenu_ui_mainmenu_mes_file);

    if (dword_64C41C != NULL) {
        FREE(dword_64C41C);
    }

    mainmenu_ui_initialized = false;
}

// 0x541150
void sub_541150()
{
    mainmenu_ui_window_type = MM_WINDOW_MAINMENU;
    mainmenu_ui_start_new_game = false;
    mainmenu_ui_auto_equip_items_on_start = false;
}

// 0x541170
void sub_541170()
{
    dword_5C4000 = true;
}

// 0x541180
void mainmenu_ui_start(MainMenuType type)
{
    tig_art_id_t art_id;

    if (!mainmenu_ui_active) {
        mainmenu_ui_num_windows = 0;

        if (type != MM_TYPE_OPTIONS) {
            sub_45B320();
        }

        tig_art_interface_id_create(0, 0, 0, 0, &art_id);
        tig_mouse_cursor_set_art_id(art_id);
        inven_ui_destroy();

        if (type == MM_TYPE_DEFAULT && !dword_5C4000) {
            type = MM_TYPE_1;
        }

        mainmenu_ui_type = type;

        switch (type) {
        case MM_TYPE_IN_PLAY:
            mainmenu_ui_window_type = MM_WINDOW_MAINMENU_IN_PLAY;
            dword_5C4004 = false;
            mainmenu_ui_open();
            object_hover_obj_set(OBJ_HANDLE_NULL);
            break;
        case MM_TYPE_IN_PLAY_LOCKED:
            mainmenu_ui_window_type = MM_WINDOW_MAINMENU_IN_PLAY_LOCKED;
            dword_5C4004 = false;
            mainmenu_ui_open();
            object_hover_obj_set(OBJ_HANDLE_NULL);
            break;
        case MM_TYPE_OPTIONS:
            mainmenu_ui_window_type = MM_WINDOW_OPTIONS;
            dword_5C4004 = false;
            mainmenu_ui_open();
            object_hover_obj_set(OBJ_HANDLE_NULL);
            break;
        case MM_TYPE_5:
            mainmenu_ui_window_type = MM_WINDOW_26;
            dword_5C4004 = false;
            mainmenu_ui_open();
            object_hover_obj_set(OBJ_HANDLE_NULL);
            break;
        default:
            if (tig_mouse_hide() != TIG_OK) {
                tig_debug_printf("mainmenu_ui_start: ERROR: tig_mouse_hide failed!\n");
                exit(EXIT_FAILURE);
            }
            mainmenu_ui_window_type = MM_WINDOW_0;
            dword_5C4004 = true;
            mainmenu_ui_open();
            object_hover_obj_set(OBJ_HANDLE_NULL);
            break;
        }
    }
}

// 0x5412D0
void sub_5412D0()
{
    sub_5412E0(false);
}

// 0x5412E0
void sub_5412E0(bool a1)
{
    int64_t pc_obj;
    int map;
    int64_t x;
    int64_t y;
    FadeData fade_data;
    TeleportData teleport_data;
    MesFileEntry mes_file_entry;
    DateTime datetime;
    TimeEvent timeevent;

    if (mainmenu_ui_active) {
        gameuilib_wants_mainmenu_unset();

        pc_obj = player_get_local_pc_obj();

        if (mainmenu_ui_auto_equip_items_on_start) {
            if (item_wield_get(pc_obj, ITEM_INV_LOC_WEAPON) == OBJ_HANDLE_NULL) {
                item_wield_best(pc_obj, ITEM_INV_LOC_WEAPON, OBJ_HANDLE_NULL);
            }
            if (item_wield_get(pc_obj, ITEM_INV_LOC_ARMOR) == OBJ_HANDLE_NULL) {
                item_wield_best(pc_obj, ITEM_INV_LOC_ARMOR, OBJ_HANDLE_NULL);
            }
            mainmenu_ui_auto_equip_items_on_start = false;
        }

        if (mainmenu_ui_window_type != MM_WINDOW_0 || !stru_5C36B0[mainmenu_ui_type][1]) {
            if (mainmenu_ui_start_new_game) {
                mainmenu_ui_start_new_game = false;

                map = map_by_type(MAP_TYPE_START_MAP);
                if (map == 0) {
                    tig_debug_printf("MMUI: ERROR: Teleport/World Loc Failure!\n");
                    exit(EXIT_FAILURE);
                }
                if (!map_get_starting_location(map, &x, &y)) {
                    tig_debug_printf("MMUI: ERROR: Teleport/World Loc Failure!\n");
                    exit(EXIT_FAILURE);
                }

                fade_data.flags = 0;
                fade_data.color = 0;
                fade_data.steps = 64;
                fade_data.duration = 3.0f;
                gfade_run(&fade_data);

                teleport_data.flags = TELEPORT_MOVIE1 | TELEPORT_MOVIE2 | TELEPORT_FADE_IN;
                teleport_data.obj = pc_obj;
                teleport_data.loc = LOCATION_MAKE(x, y);
                teleport_data.map = map;
                teleport_data.movie1 = 1;
                teleport_data.movie_flags1 = 0;
                teleport_data.movie2 = 7;
                teleport_data.movie_flags2 = 0;
                teleport_data.fade_in.flags = FADE_IN;
                teleport_data.fade_in.steps = 64;
                teleport_data.fade_in.duration = 3.0f;
                teleport_data.fade_in.color = tig_color_make(0, 0, 0);
                teleport_do(&teleport_data);

                gsound_stop_all(0);

                mes_file_entry.num = 6000; // "Please Wait"
                mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                sub_557FD0(mes_file_entry.str);

                timeevent.type = TIMEEVENT_TYPE_NEWSPAPERS;
                sub_45A950(&datetime, 86400000 - sub_45AD70());
                timeevent_add_delay(&timeevent, &datetime);

                wmap_rnd_schedule();
            } else {
                if (dword_5C4004) {
                    sub_40FED0();
                }
                gamelib_draw();
            }
        }

        mainmenu_ui_close(false);
    }

    intgame_refresh_cursor();
    sub_5507D0(0);
    if (!a1) {
        intgame_draw_bars();

        if (mainmenu_ui_was_compact_interface) {
            intgame_toggle_interface();
            mainmenu_ui_was_compact_interface = false;
        }
    }
    sub_45B340();
}

// 0x541590
bool mainmenu_ui_handle()
{
    tig_timestamp_t timestamp;
    TigMessage msg;

    mainmenu_ui_was_compact_interface = intgame_is_compact_interface();
    if (mainmenu_ui_was_compact_interface) {
        intgame_toggle_interface();
    }

    if (mainmenu_ui_window_type < MM_WINDOW_MAINMENU) {
        mainmenu_ui_close(false);
        mainmenu_ui_window_type = MM_WINDOW_MAINMENU;
        mainmenu_ui_open();

        if (tig_mouse_show() != TIG_OK) {
            tig_debug_printf("mainmenu_ui_handle: ERROR: tig_mouse_show failed!\n");
        }
    }

    broadcast_ui_close();

    while (mainmenu_ui_active) {
        tig_ping();
        tig_timer_now(&timestamp);
        timeevent_ping(timestamp);
        gamelib_ping();
        iso_redraw();
        tig_window_display();

        while (tig_message_dequeue(&msg) == TIG_OK) {
            switch (msg.type) {
            case TIG_MESSAGE_QUIT:
                if (mainmenu_ui_confirm_quit() == TIG_WINDOW_MODAL_DIALOG_CHOICE_OK) {
                    return false;
                }
                break;
            case TIG_MESSAGE_REDRAW:
                gamelib_redraw();
                break;
            }
        }

        tig_window_display();
    }

    return true;
}

// 0x541680
bool mainmenu_ui_is_active()
{
    return mainmenu_ui_active;
}

// 0x541690
TigWindowModalDialogChoice mainmenu_ui_confirm_quit()
{
    return mainmenu_ui_confirm(5100); // "Are you sure you want to quit?"
}

// 0x5416A0
TigWindowModalDialogChoice mainmenu_ui_confirm(int num)
{
    MesFileEntry mes_file_entry;
    TigWindowModalDialogInfo modal_info;
    TigWindowModalDialogChoice choice;

    mes_file_entry.num = num;
    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

    modal_info.type = TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL;
    modal_info.x = 237;
    modal_info.y = 232;
    modal_info.text = mes_file_entry.str;
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_OK] = 'y';
    modal_info.keys[TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL] = 'n';
    modal_info.process = NULL;
    modal_info.redraw = gamelib_redraw;
    hrp_center(&(modal_info.x), &(modal_info.y));
    tig_window_modal_dialog(&modal_info, &choice);

    return choice;
}

// 0x541710
void mainmenu_ui_reset()
{
    inven_ui_destroy();
    charedit_close();
    sleep_ui_close();
    wmap_ui_close();
    logbook_ui_close();
    fate_ui_close();
    schematic_ui_close();
    gamelib_reset();
    gameuilib_reset();
}

// 0x541740
void mainmenu_ui_open()
{
    if (!mainmenu_ui_active) {
        dword_5C3FB8 = -1;
        if (main_menu_window_info[mainmenu_ui_window_type]->init_func != NULL) {
            main_menu_window_info[mainmenu_ui_window_type]->init_func();
        } else {
            mainmenu_ui_create_window();
        }

        if (mainmenu_ui_active) {
            if (!dword_64C38C) {
                mainmenu_ui_push_window_stack(mainmenu_ui_window_type);
            }
        }

        dword_64C38C = false;
    }
}

// 0x5417A0
void mainmenu_ui_close(bool back)
{
    if (main_menu_window_info[mainmenu_ui_window_type]->exit_func != NULL) {
        main_menu_window_info[mainmenu_ui_window_type]->exit_func();
    }

    sub_546DD0();

    if (back) {
        mainmenu_ui_window_type = mainmenu_ui_pop_window_stack();
        if (mainmenu_ui_window_type != MM_WINDOW_0) {
            mainmenu_ui_open();
        }
    }
}

// 0x5417E0
MainMenuWindowType mainmenu_ui_pop_window_stack()
{
    if (mainmenu_ui_num_windows > 0) {
        if (--mainmenu_ui_num_windows > 0) {
            return mainmenu_ui_window_stack[--mainmenu_ui_num_windows];
        }
    } else {
        mainmenu_ui_num_windows = 0;
    }
    return MM_WINDOW_0;
}

// 0x541810
void mainmenu_ui_push_window_stack(MainMenuWindowType window_type)
{
    mainmenu_ui_window_stack[mainmenu_ui_num_windows++] = window_type;
}

// 0x541830
void sub_541830(char* dst, const char* src)
{
    while (*src != '\0') {
        if (src[0] == '\\' && src[1] == 't') {
            strcpy(dst, "    ");
            dst += 5;
            src += 2;
        } else {
            *dst++ = *src++;
        }
    }

    *dst = '\0';
}

// 0x5418A0
void sub_5418A0(char* str, TigRect* rect, tig_font_handle_t font, unsigned int flags)
{
    TigVideoBuffer* vb;
    TigRect text_rect;
    TigRect dirty_rect;
    char* newline;
    char* chunk;
    TigFont font_desc;
    size_t pos;

    if (tig_window_vbid_get(dword_5C3624, &vb) != TIG_OK) {
        return;
    }

    text_rect = *rect;

    while (str != NULL) {
        newline = strstr(str, "\\n");
        if (newline != NULL) {
            *newline = '\0';
        }

        sub_541830(byte_64BC1C, str);

        chunk = byte_64BC1C;
        if ((flags & 0x1) != 0) {
            font_desc.width = 0;
            font_desc.height = 0;
            font_desc.str = chunk;
            font_desc.flags = 0;
            tig_font_measure(&font_desc);

            if (font_desc.width > rect->width) {
                pos = strlen(str);
                while (pos > 0 && font_desc.width > rect->width) {
                    chunk[pos--] = '\0';
                    font_desc.width = 0;
                    font_desc.height = 0;
                    font_desc.str = chunk;
                    font_desc.flags = 0;
                    tig_font_measure(&font_desc);
                }
            }
        } else if ((flags & 0x02) != 0) {
            font_desc.width = 0;
            font_desc.height = 0;
            font_desc.str = chunk;
            font_desc.flags = 0;
            tig_font_measure(&font_desc);

            if (font_desc.width > rect->width) {
                pos = strlen(str);
                while (pos > 0 && font_desc.width > rect->width) {
                    chunk++;
                    pos--;
                    font_desc.width = 0;
                    font_desc.height = 0;
                    font_desc.str = chunk;
                    font_desc.flags = 0;
                    tig_font_measure(&font_desc);
                }
            }
        }

        tig_font_push(font);
        if (tig_font_write(vb, chunk, &text_rect, &dirty_rect) != TIG_OK) {
            tig_debug_printf("MainMenu-UI: mmUITextWrite_func: ERROR: Couldn't write text: '%s'!\n", byte_64BC1C);
        }
        tig_font_pop();

        text_rect.y += dirty_rect.height;
        text_rect.height -= dirty_rect.height;

        if (newline != NULL) {
            *newline = '\\';
            newline += 2;
        }

        str = newline;
    }
}

// 0x541AA0
void mainmenu_ui_create_mainmenu()
{
    mainmenu_ui_window_type = MM_WINDOW_MAINMENU;
    mainmenu_ui_create_window();
    ++dword_64C43C;
    mainmenu_ui_draw_version();
}

// 0x541AC0
void mainmenu_ui_draw_version()
{
    TigRect rect;
    char version[40];

    if (settings_get_value(&settings, SHOW_VERSION_KEY) == 0) {
        return;
    }

    if (!gamelib_copy_version(version, NULL, NULL)) {
        return;
    }

    rect.x = 10;
    rect.y = 575;
    rect.width = 400;
    rect.height = 20;

    tig_font_push(dword_64BC04[0]);
    if (tig_window_text_write(dword_5C3624, version, &rect) != TIG_OK) {
        tig_debug_printf("MainMenuUI: ERROR: GameLib_Version_String Failed to draw!\n");
    }
    tig_font_pop();
}

// 0x541B50
bool mainmenu_ui_press_mainmenu_in_play(tig_button_handle_t button_handle)
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (button_handle == window->buttons[3].button_handle) {
        if (mainmenu_ui_confirm_quit() != TIG_WINDOW_MODAL_DIALOG_CHOICE_OK) {
            return false;
        }

        tig_button_hide(button_handle);

        return sub_549310(button_handle);
    }

    if (button_handle == window->buttons[4].button_handle) {
        if (stru_5C36B0[mainmenu_ui_type][0]) {
            sub_5412D0();
            return false;
        }

        mainmenu_ui_close(true);

        if (mainmenu_ui_window_type == MM_WINDOW_0) {
            sub_5412D0();
            return false;
        }

        return false;
    }

    return false;
}

// 0x541BE0
bool mainmenu_ui_press_mainmenu_in_play_locked(tig_button_handle_t button_handle)
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (button_handle == window->buttons[1].button_handle) {
        if (mainmenu_ui_confirm_quit() != TIG_WINDOW_MODAL_DIALOG_CHOICE_OK) {
            return false;
        }

        tig_button_hide(button_handle);

        return sub_549310(button_handle);
    }

    if (button_handle == window->buttons[2].button_handle) {
        if (stru_5C36B0[mainmenu_ui_type][0]) {
            sub_5412D0();
            return false;
        }

        mainmenu_ui_close(true);

        if (mainmenu_ui_window_type == MM_WINDOW_0) {
            sub_5412D0();
            return false;
        }

        return false;
    }

    return false;
}

// 0x541C70
void mainmenu_ui_create_options()
{
    PcLens pc_lens;
    int64_t pc_obj;
    int64_t loc;

    sub_541D40();
    mainmenu_ui_window_type = MM_WINDOW_OPTIONS;
    mainmenu_ui_create_window_func(false);
    dword_64C440 = 0;
    options_ui_start(OPTIONS_UI_TAB_GAME, dword_5C3624, stru_5C36B0[mainmenu_ui_type][1] == 0);
    mainmenu_ui_draw_version();

    pc_lens.window_handle = dword_5C3624;
    pc_lens.rect = &stru_5C4490;
    tig_art_interface_id_create(670, 0, 0, 0, &pc_lens.art_id);
    if (stru_5C36B0[mainmenu_ui_type][0]) {
        pc_obj = player_get_local_pc_obj();
        loc = obj_field_int64_get(pc_obj, OBJ_F_LOCATION);
        location_origin_set(loc);
        intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);
    } else {
        intgame_pc_lens_do(PC_LENS_MODE_BLACKOUT, &pc_lens);
    }

    tig_window_display();
}

// 0x541D40
void sub_541D40()
{
    inven_ui_destroy();
    charedit_close();
    sleep_ui_close();
    wmap_ui_close();
    logbook_ui_close();
    fate_ui_close();
    schematic_ui_close();
}

// 0x541D70
void mainmenu_ui_destroy_options()
{
    options_ui_close();
    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
}

// 0x541D90
bool mainmenu_ui_press_options(tig_button_handle_t button_handle)
{
    int index;

    for (index = 0; index < 4; index++) {
        if (mainmenu_ui_options_buttons[index].button_handle == button_handle) {
            break;
        }
    }

    if (index >= 4) {
        return options_ui_handle_button_pressed(button_handle);
    }

    if (index != 3) {
        sub_541E20(index);
        return true;
    }

    if (mainmenu_ui_window_type == MM_WINDOW_OPTIONS) {
        if (!options_ui_load_module()) {
            return true;
        }
    }

    if (stru_5C36B0[mainmenu_ui_type][0]) {
        sub_5412D0();
    } else {
        mainmenu_ui_close(true);
        if (mainmenu_ui_window_type == MM_WINDOW_0) {
            sub_5412D0();
        }
    }

    return false;
}

// 0x541E20
void sub_541E20(int a1)
{
    if (dword_64C440 != a1 && options_ui_load_module()) {
        options_ui_close();

        dword_64C440 = a1;
        switch (dword_64C440) {
        case 0:
            options_ui_start(OPTIONS_UI_TAB_GAME, dword_5C3624, stru_5C36B0[mainmenu_ui_type][1] == 0);
            break;
        case 1:
            options_ui_start(OPTIONS_UI_TAB_VIDEO, dword_5C3624, stru_5C36B0[mainmenu_ui_type][1] == 0);
            break;
        case 2:
            options_ui_start(OPTIONS_UI_TAB_AUDIO, dword_5C3624, stru_5C36B0[mainmenu_ui_type][1] == 0);
            break;
        }
    }
}

// 0x541F20
void mainmenu_ui_load_game_create()
{
    MainMenuWindowInfo* window;
    int64_t pc_obj;
    PcLens pc_lens;

    mainmenu_ui_window_type = MM_WINDOW_LOAD_GAME;
    window = main_menu_window_info[mainmenu_ui_window_type];

    sub_542200();

    if (dword_64C37C) {
        gamelib_savelist_create_module(dword_64C37C, &mainmenu_ui_gsl);
    } else {
        gamelib_savelist_create(&mainmenu_ui_gsl);
    }

    gamelib_savelist_sort(&mainmenu_ui_gsl, GAME_SAVE_LIST_ORDER_DATE, false);

    window->cnt = mainmenu_ui_gsl.count;
    if (window->selected_index == -1) {
        if (window->cnt > 0) {
            const char* path = gamelib_last_save_name();
            unsigned int index;

            window->selected_index = 0;

            if (path != NULL && *path != '\0') {
                for (index = 0; index < mainmenu_ui_gsl.count; index++) {
                    if (strcmp(mainmenu_ui_gsl.names[index], path) == 0) {
                        window->selected_index = index;
                        break;
                    }
                }
            }
        }
    } else if (window->selected_index >= window->cnt) {
        window->selected_index = window->cnt > 0 ? 0 : -1;
    }

    window->max_top_index = window->cnt - window->content_rect.height / 20 - 1;
    if (window->max_top_index < 0) {
        window->max_top_index = 0;
    }
    window->top_index = 0;

    mainmenu_ui_create_window_func(false);

    if (!main_menu_button_create_ex(&stru_5C45D8, 0, 0, 0x2)) {
        tig_debug_printf("MainMenu-UI: mainmenu_ui_create_load_game: ERROR: Failed to create button.\n");
        exit(EXIT_FAILURE);
    }

    stru_64C260.scrollbar_rect = stru_5C4798;
    stru_64C260.flags = 0x39F;
    stru_64C260.min_value = 0;
    stru_64C260.max_value = window->max_top_index + 1;
    if (stru_64C260.max_value > 0) {
        stru_64C260.max_value--;
    }
    stru_64C260.value = window->selected_index < 7 ? 0 : window->selected_index;
    stru_64C260.field_2C = 1;
    stru_64C260.field_3C = sub_542280;
    stru_64C260.field_40 = sub_5422A0;
    stru_64C260.content_rect.x = 34;
    stru_64C260.content_rect.y = 110;
    stru_64C260.content_rect.width = 195;
    stru_64C260.content_rect.height = 232;
    scrollbar_ui_control_create(&stru_64C220, &stru_64C260, dword_5C3624);

    dword_64C450 = false;

    pc_obj = player_get_local_pc_obj();

    pc_lens.window_handle = dword_5C3624;
    pc_lens.rect = &stru_5C4780;
    tig_art_interface_id_create(746, 0, 0, 0, &(pc_lens.art_id));

    if (pc_obj != OBJ_HANDLE_NULL) {
        location_origin_set(obj_field_int64_get(pc_obj, OBJ_F_LOCATION));
    }

    if (map_by_type(MAP_TYPE_SHOPPING_MAP) == map_current_map()) {
        intgame_pc_lens_do(PC_LENS_MODE_BLACKOUT, &pc_lens);
    } else {
        intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);
        dword_64C450 = true;
    }

    scrollbar_ui_control_redraw(stru_64C220);
    tig_window_display();
}

// 0x542200
void sub_542200()
{
    inven_ui_destroy();
    charedit_close();
    sleep_ui_close();
    wmap_ui_close();
    logbook_ui_close();
    fate_ui_close();
    schematic_ui_close();
}

// 0x542230
void mainmenu_ui_load_game_destroy()
{
    scrollbar_ui_control_destroy(stru_64C220);

    if (mainmenu_ui_gsi_loaded) {
        gamelib_saveinfo_exit(&mainmenu_ui_gsi);
        mainmenu_ui_gsi_loaded = false;
    }

    gamelib_savelist_destroy(&mainmenu_ui_gsl);

    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
    if (dword_64C37C != NULL)
        FREE(dword_64C37C);
    dword_64C37C = NULL;
}

// 0x542280
void sub_542280(int a1)
{
    main_menu_window_info[mainmenu_ui_window_type]->top_index = a1;
}

// 0x5422A0
void sub_5422A0(TigRect* rect)
{
    (void)rect;

    main_menu_window_info[mainmenu_ui_window_type]->refresh_func(NULL);
}

// 0x5422C0
bool mainmenu_ui_load_game_execute(int btn)
{
    int index;
    char name[256];

    (void)btn;

    index = main_menu_window_info[mainmenu_ui_window_type]->selected_index;
    if (index == -1) {
        return false;
    }

    strncpy(name, mainmenu_ui_gsl.names[index], 8);
    name[8] = '\0';

    return sub_5432B0(name);
}

// 0x542420
bool mainmenu_ui_load_game_button_pressed(tig_button_handle_t button_handle)
{
    if (button_handle != stru_5C45D8.button_handle) {
        return false;
    }

    dword_5C4790 = 0;
    main_menu_window_info[mainmenu_ui_window_type]->refresh_func(&stru_5C46C0);

    return true;
}

// 0x542460
bool mainmenu_ui_load_game_button_released(tig_button_handle_t button_handle)
{
    MainMenuWindowInfo* window;
    int index;

    window = main_menu_window_info[mainmenu_ui_window_type];

    for (index = 0; index < 2; index++) {
        if (button_handle == window->buttons[index].button_handle) {
            sub_5480C0(index + 2);
            return true;
        }
    }

    if (button_handle == window->buttons[2].button_handle) {
        gsound_play_sfx(0, 1);
        mainmenu_ui_load_game_handle_delete();
        return true;
    }

    if (button_handle == stru_5C45D8.button_handle) {
        dword_5C4790 = 1;
        window->refresh_func(&stru_5C46C0);
        return true;
    }

    return false;
}

// 0x5424F0
void mainmenu_ui_load_game_mouse_up(int x, int y)
{
    MainMenuWindowInfo* window;

    (void)x;

    window = main_menu_window_info[mainmenu_ui_window_type];
    window->selected_index = window->top_index + y / 20;
    if (window->selected_index >= window->cnt) {
        window->selected_index = -1;
    }
    sub_542560();
    window->refresh_func(NULL);
    scrollbar_ui_control_redraw(stru_64C220);
}

// 0x542560
void sub_542560()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (mainmenu_ui_gsi_loaded) {
        gamelib_saveinfo_exit(&mainmenu_ui_gsi);
        mainmenu_ui_gsi_loaded = false;

        if (window->selected_index > -1
            && gamelib_saveinfo_load(mainmenu_ui_gsl.names[window->selected_index], &mainmenu_ui_gsi)) {
            mainmenu_ui_gsi_loaded = true;
        }
    }
}

// 0x5425C0
void mainmenu_ui_load_game_refresh(TigRect* rect)
{
    MainMenuWindowInfo* window;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigVideoBufferData video_buffer_data;
    TigWindowBlitInfo win_blit_info;
    MesFileEntry mes_file_entry;
    char str[20];
    tig_font_handle_t font;
    int area;
    char* area_name;
    char* story_state_desc;

    window = main_menu_window_info[mainmenu_ui_window_type];
    tig_art_interface_id_create(window->background_art_num, 0, 0, 0, &art_id);
    tig_art_frame_data(art_id, &art_frame_data);
    if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
        src_rect.x = stru_5C4798.x;
        src_rect.y = stru_5C4798.y;
        src_rect.width = stru_5C4798.width + 1;
        src_rect.height = stru_5C4798.height + 1;

        dst_rect.x = stru_5C4798.x;
        dst_rect.y = stru_5C4798.y;
        dst_rect.width = stru_5C4798.width + 1;
        dst_rect.height = stru_5C4798.height + 1;

        art_blit_info.art_id = art_id;
        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(dword_5C3624, &art_blit_info);
    }

    if (rect == NULL
        || (window->content_rect.x < rect->x + rect->width
            && window->content_rect.y < rect->y + rect->height
            && rect->x < window->content_rect.x + window->content_rect.width
            && rect->y < window->content_rect.y + window->content_rect.height)) {
        dst_rect.x = window->content_rect.x;
        dst_rect.y = window->content_rect.y;
        dst_rect.width = window->content_rect.width;
        dst_rect.height = window->content_rect.height + 5;

        if (tig_window_fill(dword_5C3624, &dst_rect, tig_color_make(0, 0, 0)) != TIG_OK) {
            tig_debug_printf("mmUIMPLoadGameRefreshFunc: ERROR: tig_window_fill2 failed!\n");
            exit(EXIT_FAILURE);
        }

        dst_rect.height -= 5;

        if (mainmenu_ui_gsl.count != 0) {
            int index;
            int max_y;
            char* name;

            max_y = dst_rect.y + dst_rect.height - 1;

            dst_rect.width -= 4;
            dst_rect.height = 20;

            for (index = window->top_index; index < window->cnt; index++) {
                if (dst_rect.y >= max_y) {
                    break;
                }

                font = window->selected_index == index ? dword_64C240 : dword_64C210[0];
                tig_font_push(font);
                name = sub_543040(index);
                if (*name != '\0') {
                    sub_542D00(name, &dst_rect, font);
                }
                dst_rect.y += 20;
                tig_font_pop();
            }
        }
    }

    if (rect == NULL
        || (stru_5C46C0.x < rect->x + rect->width
            && stru_5C46C0.y < rect->y + rect->height
            && rect->x < stru_5C46C0.x + stru_5C46C0.width
            && rect->y < stru_5C46C0.y + stru_5C46C0.height)) {
        if (window->selected_index > -1) {
            if (!mainmenu_ui_gsi_loaded
                && mainmenu_ui_gsl.count > 0
                && gamelib_saveinfo_load(mainmenu_ui_gsl.names[window->selected_index], &mainmenu_ui_gsi)) {
                mainmenu_ui_gsi_loaded = true;
            }

            if (mainmenu_ui_gsi_loaded) {
                if (mainmenu_ui_gsi.thumbnail_video_buffer != NULL) {
                    if (tig_video_buffer_data(mainmenu_ui_gsi.thumbnail_video_buffer, &video_buffer_data) == TIG_OK) {
                        stru_5C46B0.width = video_buffer_data.width;
                        stru_5C46B0.height = video_buffer_data.height;

                        stru_5C46C0.width = video_buffer_data.width;
                        stru_5C46C0.height = video_buffer_data.height;

                        win_blit_info.type = TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW;
                        win_blit_info.vb_blit_flags = 0;
                        win_blit_info.src_video_buffer = mainmenu_ui_gsi.thumbnail_video_buffer;
                        win_blit_info.src_rect = &stru_5C46B0;
                        win_blit_info.dst_window_handle = dword_5C3624;
                        win_blit_info.dst_rect = &stru_5C46C0;

                        if (tig_window_blit(&win_blit_info) != TIG_OK) {
                            tig_debug_printf("MMUI: ERROR: mmUIMPLoadGameRefreshFunc FAILED to refresh!\n");
                        }
                    }
                }

                font = dword_64C210[0];
                tig_font_push(font);

                if (mainmenu_ui_gsi.version == 25) {
                    if (dword_5C4790) {
                        sub_542DF0(mainmenu_ui_gsi.pc_name, &stru_5C46D0, font);
                        if (mainmenu_ui_gsi.pc_portrait != 0) {
                            portrait_draw_native(OBJ_HANDLE_NULL,
                                mainmenu_ui_gsi.pc_portrait,
                                dword_5C3624,
                                stru_5C46E0.x,
                                stru_5C46E0.y);
                        }

                        mes_file_entry.num = 5051; // "Level %d"
                        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                        sprintf(str, mes_file_entry.str, mainmenu_ui_gsi.pc_level);
                        sub_542DF0(str, &stru_5C4720, font);

                        sub_542DF0(mainmenu_ui_gsi.description, &stru_5C4730, font);

                        if (map_by_type(MAP_TYPE_START_MAP) == mainmenu_ui_gsi.map) {
                            area = area_get_nearest_area_in_range(mainmenu_ui_gsi.pc_location, true);
                        } else if (!map_get_area(mainmenu_ui_gsi.map, &area)) {
                            area = 0;
                        }

                        if (area > 0) {
                            area_name = area_get_name(area);
                        } else {
                            mes_file_entry.num = 5050; // "Unknown location."
                            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                            area_name = mes_file_entry.str;
                        }

                        sub_542DF0(area_name, &stru_5C46F0, font);

                        datetime_format_date(&(mainmenu_ui_gsi.datetime), str);
                        sub_542EA0(str, &stru_5C4710, font);

                        datetime_format_time(&(mainmenu_ui_gsi.datetime), str);
                        sub_542EA0(str, &stru_5C4700, font);

                        story_state_desc = script_story_state_info(mainmenu_ui_gsi.story_state);
                        if (story_state_desc != NULL && *story_state_desc != '\0') {
                            mmUITextWriteCenteredToArray(story_state_desc, stru_5C4740, 4, font);
                        }
                    }
                } else {
                    mes_file_entry.num = 5004; // "Version Mismatch!"
                    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                    sub_542DF0(mes_file_entry.str, &stru_5C46D0, font);
                }
                tig_font_pop();
            } else {
                tig_window_fill(dword_5C3624, &stru_5C46C0, tig_color_make(0, 0, 0));

                font = dword_64C218[0];
                tig_font_push(font);
                mes_file_entry.num = 5005; // "Empty"
                mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                sub_542DF0(mes_file_entry.str, &stru_5C46D0, font);
                tig_font_pop();
            }
        } else {
            tig_window_fill(dword_5C3624, &stru_5C46C0, tig_color_make(0, 0, 0));
        }

        tig_art_interface_id_create(748, 0, 0, 0, &art_id);
        if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK
            && tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.height;

            dst_rect.x = 281;
            dst_rect.y = 55;
            dst_rect.width = art_frame_data.width;
            dst_rect.height = art_frame_data.height;

            art_blit_info.art_id = art_id;
            art_blit_info.flags = 0;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            if (tig_window_blit_art(dword_5C3624, &art_blit_info) != TIG_OK) {
                tig_debug_printf("MMUI: mmUIMPLoadGameRefreshFunc: ERROR: FAILED to refresh!\n");
            }
        }
    }

    mmUIWinRefreshScrollBar();
}

// 0x542D00
void sub_542D00(char* str, TigRect* rect, tig_font_handle_t font)
{
    TigFont font_desc;
    TigRect text_rect;

    font_desc.width = 0;
    font_desc.height = 0;
    font_desc.str = str;
    font_desc.flags = 0;
    tig_font_measure(&font_desc);

    text_rect = *rect;

    if (font_desc.width < rect->width) {
        sub_5418A0(str, &text_rect, font, 0);
        return;
    }

    font_desc.width = 0;
    font_desc.height = 0;
    font_desc.str = off_5C407C;
    font_desc.flags = 0;
    tig_font_measure(&font_desc);

    text_rect.width -= font_desc.width;
    sub_5418A0(str, &text_rect, font, 1);

    text_rect.x += font_desc.width;
    text_rect.width = font_desc.width;
    sub_5418A0(off_5C407C, &text_rect, font, 0);
}

// 0x542DF0
void sub_542DF0(char* str, TigRect* rect, tig_font_handle_t font)
{
    TigFont font_desc;
    TigRect text_rect;

    font_desc.width = 0;
    font_desc.height = 0;
    font_desc.str = str;
    font_desc.flags = 0;
    tig_font_measure(&font_desc);

    text_rect = *rect;
    if (font_desc.width < rect->width) {
        text_rect.x += (text_rect.width - font_desc.width) / 2;
        text_rect.width = font_desc.width + 20;
    }

    sub_5418A0(str, &text_rect, font, 0);
}

// 0x542EA0
void sub_542EA0(char* str, TigRect* rect, tig_font_handle_t font)
{
    TigFont font_desc;
    TigRect text_rect;

    font_desc.width = 0;
    font_desc.height = 0;
    font_desc.str = str;
    font_desc.flags = 0;
    tig_font_measure(&font_desc);

    text_rect = *rect;
    if (font_desc.width < rect->width) {
        text_rect.x -= font_desc.width;
        text_rect.width = font_desc.width + 10;
    }

    sub_5418A0(str, &text_rect, font, 0);
}

// 0x542F50
void mmUITextWriteCenteredToArray(const char* str, TigRect* rects, int cnt, tig_font_handle_t font)
{
    // TODO: Incomplete.
}

// 0x543040
char* sub_543040(int index)
{
    if (mainmenu_ui_gsl.names != NULL) {
        return mainmenu_ui_gsl.names[index] + 8;
    } else {
        return "";
    }
}

// 0x543060
void sub_543060()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (window->selected_index > 0) {
        window->selected_index--;
        if (window->selected_index < window->top_index) {
            scrollbar_ui_control_set(stru_64C220, SCROLLBAR_CURRENT_VALUE, window->selected_index);
        }
        gsound_play_sfx(0, 1);
        sub_542560();
        window->refresh_func(NULL);
        scrollbar_ui_control_redraw(stru_64C220);
    }
}

// 0x5430D0
void sub_5430D0()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (window->selected_index < window->cnt - 1) {
        window->selected_index++;
        if (window->selected_index > window->top_index + window->content_rect.height / 20) {
            scrollbar_ui_control_set(stru_64C220, SCROLLBAR_CURRENT_VALUE, window->selected_index - window->content_rect.height / 20);
        }
        gsound_play_sfx(0, 1);
        sub_542560();
        window->refresh_func(NULL);
        scrollbar_ui_control_redraw(stru_64C220);
    }
}

// 0x543160
bool mainmenu_ui_load_game_handle_delete()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (window->selected_index == -1) {
        return false;
    }

    // "Are you sure you want to delete the save game?"
    if (mainmenu_ui_confirm(5150) != TIG_WINDOW_MODAL_DIALOG_CHOICE_OK) {
        return false;
    }

    if (!gamelib_delete(mainmenu_ui_gsl.names[window->selected_index])) {
        return false;
    }

    if (mainmenu_ui_gsi_loaded) {
        gamelib_saveinfo_exit(&mainmenu_ui_gsi);
        mainmenu_ui_gsi_loaded = false;
    }

    gamelib_savelist_destroy(&mainmenu_ui_gsl);
    gamelib_savelist_create(&mainmenu_ui_gsl);

    gamelib_savelist_sort(&mainmenu_ui_gsl, GAME_SAVE_LIST_ORDER_DATE, false);
    window->selected_index = -1;
    window->cnt--;
    window->refresh_func(NULL);

    return true;
}

// 0x543220
bool sub_543220()
{
    const char* path;
    char name[256];
    bool rc;

    if (mainmenu_ui_active) {
        return false;
    }

    path = gamelib_last_save_name();
    if (path[0] == '\0' || !gamelib_saveinfo_load(path, &mainmenu_ui_gsi)) {
        return false;
    }
    mainmenu_ui_gsi_loaded = true;

    strncpy(name, path, 8);
    name[8] = '\0';

    rc = sub_5432B0(name);
    if (mainmenu_ui_gsi_loaded) {
        gamelib_saveinfo_exit(&mainmenu_ui_gsi);
        mainmenu_ui_gsi_loaded = false;
    }

    return rc;
}

// 0x5432B0
bool sub_5432B0(const char* name)
{
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    sub_542200();

    if (mainmenu_ui_gsi.version == 25) {
        mainmenu_ui_reset();
        sub_40DAB0();

        if (gamelib_load(name)) {
            gamelib_current_mode_name_set(mainmenu_ui_gsi.module_name);

            mes_file_entry.num = 5000; // "Game Loaded Successfully."
            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

            ui_message.type = UI_MSG_TYPE_EXCLAMATION;
            ui_message.str = mes_file_entry.str;
            sub_460630(&ui_message);

            mainmenu_ui_start_new_game = false;
            sub_5412D0();

            return true;
        }

        mainmenu_ui_reset();
    }

    mes_file_entry.num = 5001; // "Save Game Corrupt!  Load Failed!"
    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

    ui_message.type = UI_MSG_TYPE_EXCLAMATION;
    ui_message.str = mes_file_entry.str;
    sub_460630(&ui_message);

    return false;
}

// 0x543380
void mainmenu_ui_save_game_create()
{
    MainMenuWindowInfo* window;
    int64_t pc_obj;
    PcLens pc_lens;

    mainmenu_ui_window_type = MM_WINDOW_SAVE_GAME;
    window = main_menu_window_info[mainmenu_ui_window_type];

    sub_542200();
    gamelib_savelist_create(&mainmenu_ui_gsl);
    gamelib_savelist_sort(&mainmenu_ui_gsl, GAME_SAVE_LIST_ORDER_DATE, false);
    byte_64C2F8[0] = '\0';
    window->cnt = mainmenu_ui_gsl.count + 1;
    if (mainmenu_ui_gsl.count != 0) {
        window->selected_index = 1;
    } else {
        window->selected_index = -1;
    }

    window->max_top_index = window->cnt - window->content_rect.height / 20 - 1;
    if (window->max_top_index < 0) {
        window->max_top_index = 0;
    }

    window->top_index = 0;
    mainmenu_ui_create_window_func(false);

    if (!main_menu_button_create_ex(&stru_5C4838, 0, 0, 2)) {
        tig_debug_printf("MainMenu-UI: mainmenu_ui_create_save_game: ERROR: Failed to create button.\n");
        exit(EXIT_FAILURE);
    }

    stru_64C260.scrollbar_rect = stru_5C4798;
    stru_64C260.flags = 0x39F;
    stru_64C260.min_value = 0;
    stru_64C260.max_value = window->max_top_index + 1;
    if (stru_64C260.max_value > 0) {
        stru_64C260.max_value--;
    }
    stru_64C260.value = 0;
    stru_64C260.field_2C = 1;
    stru_64C260.field_3C = sub_542280;
    stru_64C260.field_40 = sub_5422A0;
    stru_64C260.content_rect.x = 34;
    stru_64C260.content_rect.y = 110;
    stru_64C260.content_rect.width = 195;
    stru_64C260.content_rect.height = 232;
    scrollbar_ui_control_create(&stru_64C220, &stru_64C260, dword_5C3624);

    pc_obj = player_get_local_pc_obj();

    pc_lens.window_handle = dword_5C3624;
    pc_lens.rect = &stru_5C4780;
    tig_art_interface_id_create(746, 0, 0, 0, &(pc_lens.art_id));

    if (pc_obj != OBJ_HANDLE_NULL) {
        location_origin_set(obj_field_int64_get(pc_obj, OBJ_F_LOCATION));
    }

    intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);
    scrollbar_ui_control_redraw(stru_64C220);

    tig_window_display();
}

// 0x543580
void mainmenu_ui_save_game_destroy()
{
    scrollbar_ui_control_destroy(stru_64C220);

    if (mainmenu_ui_gsi_loaded) {
        gamelib_saveinfo_exit(&mainmenu_ui_gsi);
        mainmenu_ui_gsi_loaded = false;
    }

    gamelib_savelist_destroy(&mainmenu_ui_gsl);
    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);
}

// 0x5435D0
bool mainmenu_ui_save_game_execute(int btn)
{
    int v1;
    char fname[COMPAT_MAX_FNAME];
    const char* name;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    int num;

    (void)btn;

    v1 = main_menu_window_info[mainmenu_ui_window_type]->selected_index;
    if (v1 == -1) {
        return false;
    }

    if (v1 > 0) {
        name = strcpy(fname, mainmenu_ui_gsl.names[v1 - 1]);
        strcpy(byte_64C2F8, mainmenu_ui_gsl.names[v1 - 1] + 8);
        fname[8] = '\0';

        if (mainmenu_ui_confirm(5064)) {
            return false;
        }
    } else {
        gamelib_savelist_sort(&mainmenu_ui_gsl, GAME_SAVE_LIST_ORDER_NAME, false);

        if (mainmenu_ui_gsl.count > 0) {
            if (SDL_toupper(mainmenu_ui_gsl.names[0][4]) == 'A') {
                if (mainmenu_ui_gsl.count > 1
                    && mainmenu_ui_gsl.names[1] != NULL) {
                    strncpy(fname, mainmenu_ui_gsl.names[1], 8);
                    fname[8] = '\0';
                    num = atoi(&(fname[4])) + 1;
                    if (num >= 9999) {
                        return false;
                    }

                    strcpy(fname, mainmenu_ui_gsl.names[0]);
                    sprintf(&(fname[4]), "%04d", num);
                    name = fname;
                } else {
                    name = "Slot0000";
                }
            } else {
                if (mainmenu_ui_gsl.names[0] != NULL) {
                    strncpy(fname, mainmenu_ui_gsl.names[0], 8);
                    fname[8] = '\0';
                    num = atoi(&(fname[4])) + 1;
                    if (num >= 9999) {
                        return false;
                    }

                    strcpy(fname, mainmenu_ui_gsl.names[0]);
                    sprintf(&(fname[4]), "%04d", num);
                    name = fname;
                } else {
                    name = "Slot0000";
                }
            }
        } else {
            name = "Slot0000";
        }
    }

    sub_542200();

    if (byte_64C2F8[0] != '\0') {
        char* pch = byte_64C2F8;
        while (*pch == ' ') {
            pch++;
        }

        if (*pch == '\0') {
            byte_64C2F8[0] = '\0';
        }
    }

    if (byte_64C2F8[0] == '\0') {
        strcpy(byte_64C2F8, name);
    }

    if (!gamelib_save(name, byte_64C2F8)) {
        mes_file_entry.num = 5003; // "Save Game Corrupt!  Save Failed!"
        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_EXCLAMATION;
        ui_message.str = mes_file_entry.str;
        sub_460630(&ui_message);

        return false;
    }

    mes_file_entry.num = 5002; // "Game Saved Successfully."
    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

    ui_message.type = UI_MSG_TYPE_EXCLAMATION;
    ui_message.str = mes_file_entry.str;
    sub_460630(&ui_message);
    sub_5412D0();
    byte_64C2F8[0] = '\0';

    return true;
}

// 0x543850
bool mainmenu_ui_save_game_button_pressed(tig_button_handle_t button_handle)
{
    if (button_handle != stru_5C4838.button_handle) {
        return false;
    }

    dword_5C4790 = 0;
    main_menu_window_info[mainmenu_ui_window_type]->refresh_func(&stru_5C46C0);

    return true;
}

// 0x543890
bool mainmenu_ui_save_game_button_released(tig_button_handle_t button_handle)
{
    MainMenuWindowInfo* window;
    int index;

    window = main_menu_window_info[mainmenu_ui_window_type];
    for (index = 0; index < 2; index++) {
        if (button_handle == window->buttons[index].button_handle) {
            sub_5480C0(index + 2);
            return true;
        }
    }

    if (button_handle == window->buttons[2].button_handle) {
        gsound_play_sfx(0, 1);
        mainmenu_ui_save_game_handle_delete();
        return true;
    }

    if (button_handle == stru_5C4838.button_handle) {
        dword_5C4790 = 1;
        window->refresh_func(&stru_5C46C0);
        return true;
    }

    return false;
}

// 0x543920
void mainmenu_ui_save_game_mouse_up(int x, int y)
{
    MainMenuWindowInfo* window;

    (void)x;

    window = main_menu_window_info[mainmenu_ui_window_type];
    window->selected_index = window->top_index + y / 20;
    if (window->selected_index >= window->cnt) {
        window->selected_index = window->cnt - 1;
    }
    sub_544290();
    window->refresh_func(NULL);
    scrollbar_ui_control_redraw(stru_64C220);
}

// 0x543990
void mainmenu_ui_save_game_refresh(TigRect* rect)
{
    MainMenuWindowInfo* window;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigVideoBufferData video_buffer_data;
    TigWindowBlitInfo win_blit_info;
    MesFileEntry mes_file_entry;
    char str[20];
    tig_font_handle_t font;
    int area;
    char* area_name;
    char* story_state_desc;

    window = main_menu_window_info[mainmenu_ui_window_type];
    tig_art_interface_id_create(window->background_art_num, 0, 0, 0, &art_id);
    tig_art_frame_data(art_id, &art_frame_data);
    if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
        src_rect.x = stru_5C4798.x;
        src_rect.y = stru_5C4798.y;
        src_rect.width = stru_5C4798.width + 1;
        src_rect.height = stru_5C4798.height + 1;

        dst_rect.x = stru_5C4798.x;
        dst_rect.y = stru_5C4798.y;
        dst_rect.width = stru_5C4798.width + 1;
        dst_rect.height = stru_5C4798.height + 1;

        art_blit_info.art_id = art_id;
        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(dword_5C3624, &art_blit_info);
    }

    if (rect == NULL
        || (window->content_rect.x < rect->x + rect->width
            && window->content_rect.y < rect->y + rect->height
            && rect->x < window->content_rect.x + window->content_rect.width
            && rect->y < window->content_rect.y + window->content_rect.height)) {
        dst_rect = window->content_rect;

        if (tig_window_fill(dword_5C3624, &dst_rect, tig_color_make(0, 0, 0)) != TIG_OK) {
            tig_debug_printf("mmUIMPSaveGameRefreshFunc: ERROR: tig_window_fill2 failed!\n");
            exit(EXIT_FAILURE);
        }

        int max_y = dst_rect.y + dst_rect.height - 1;

        dst_rect.height = 20;
        dst_rect.width -= 4;

        for (int idx = window->top_index; idx < window->cnt; idx++) {
            if (dst_rect.y >= max_y) {
                break;
            }

            font = window->selected_index == idx ? dword_64C240 : dword_64C210[0];
            tig_font_push(font);

            char* name;
            if (idx > 0) {
                name = sub_543040(idx - 1);
            } else if (idx == 0) {
                if (textedit_ui_is_focused()) {
                    name = NULL;
                    sub_544100(byte_64C2F8, &dst_rect, font);
                } else {
                    mes_file_entry.num = 5055;
                    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                    name = mes_file_entry.str;
                }
            }

            if (name != NULL && *name != '\0') {
                sub_542D00(name, &dst_rect, font);
            }

            dst_rect.y += 20;
            tig_font_pop();
        }
    }

    if (rect == NULL
        || (stru_5C46C0.x < rect->x + rect->width
            && stru_5C46C0.y < rect->y + rect->height
            && rect->x < stru_5C46C0.x + stru_5C46C0.width
            && rect->y < stru_5C46C0.y + stru_5C46C0.height)) {
        if (window->selected_index > 0) {
            if (!mainmenu_ui_gsi_loaded
                && mainmenu_ui_gsl.count > 0
                && gamelib_saveinfo_load(mainmenu_ui_gsl.names[window->selected_index - 1], &mainmenu_ui_gsi)) {
                mainmenu_ui_gsi_loaded = true;
            }

            if (mainmenu_ui_gsi_loaded) {
                if (mainmenu_ui_gsi.thumbnail_video_buffer != NULL) {
                    if (tig_video_buffer_data(mainmenu_ui_gsi.thumbnail_video_buffer, &video_buffer_data) == TIG_OK) {
                        stru_5C46B0.width = video_buffer_data.width;
                        stru_5C46B0.height = video_buffer_data.height;

                        stru_5C46C0.width = video_buffer_data.width;
                        stru_5C46C0.height = video_buffer_data.height;

                        win_blit_info.type = TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW;
                        win_blit_info.vb_blit_flags = 0;
                        win_blit_info.src_video_buffer = mainmenu_ui_gsi.thumbnail_video_buffer;
                        win_blit_info.src_rect = &stru_5C46B0;
                        win_blit_info.dst_window_handle = dword_5C3624;
                        win_blit_info.dst_rect = &stru_5C46C0;

                        if (tig_window_blit(&win_blit_info) != TIG_OK) {
                            tig_debug_printf("MMUI: ERROR: mmUIMPSaveGameRefreshFunc FAILED to refresh!\n");
                        }
                    }
                }

                font = dword_64C210[0];
                tig_font_push(font);

                if (dword_5C4790) {
                    sub_542DF0(mainmenu_ui_gsi.pc_name, &stru_5C46D0, font);
                    if (mainmenu_ui_gsi.pc_portrait != 0) {
                        portrait_draw_native(OBJ_HANDLE_NULL,
                            mainmenu_ui_gsi.pc_portrait,
                            dword_5C3624,
                            stru_5C46E0.x,
                            stru_5C46E0.y);
                    }

                    mes_file_entry.num = 5051; // "Level %d"
                    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                    sprintf(str, mes_file_entry.str, mainmenu_ui_gsi.pc_level);
                    sub_542DF0(str, &stru_5C4720, font);

                    sub_542DF0(mainmenu_ui_gsi.description, &stru_5C4730, font);

                    if (map_by_type(MAP_TYPE_START_MAP) == mainmenu_ui_gsi.map) {
                        area = area_get_nearest_area_in_range(mainmenu_ui_gsi.pc_location, true);
                    } else if (!map_get_area(mainmenu_ui_gsi.map, &area)) {
                        area = 0;
                    }

                    if (area > 0) {
                        area_name = area_get_name(area);
                    } else {
                        mes_file_entry.num = 5050; // "Unknown location."
                        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                        area_name = mes_file_entry.str;
                    }

                    sub_542DF0(area_name, &stru_5C46F0, font);

                    datetime_format_date(&(mainmenu_ui_gsi.datetime), str);
                    sub_542EA0(str, &stru_5C4710, font);

                    datetime_format_time(&(mainmenu_ui_gsi.datetime), str);
                    sub_542EA0(str, &stru_5C4700, font);

                    story_state_desc = script_story_state_info(mainmenu_ui_gsi.story_state);
                    if (story_state_desc != NULL && *story_state_desc != '\0') {
                        mmUITextWriteCenteredToArray(story_state_desc, stru_5C4740, 4, font);
                    }
                }

                tig_font_pop();
            } else {
                tig_window_fill(dword_5C3624, &stru_5C46C0, tig_color_make(0, 0, 0));

                font = dword_64C218[0];
                tig_font_push(font);
                mes_file_entry.num = 5005; // "Empty"
                mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                sub_542DF0(mes_file_entry.str, &stru_5C46D0, font);
                tig_font_pop();
            }
        } else {
            tig_window_fill(dword_5C3624, &stru_5C46C0, tig_color_make(0, 0, 0));
        }

        tig_art_interface_id_create(748, 0, 0, 0, &art_id);
        if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK
            && tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.height;

            dst_rect.x = 281;
            dst_rect.y = 55;
            dst_rect.width = art_frame_data.width;
            dst_rect.height = art_frame_data.height;

            art_blit_info.art_id = art_id;
            art_blit_info.flags = 0;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            if (tig_window_blit_art(dword_5C3624, &art_blit_info) != TIG_OK) {
                // FIXME: Misleading message.
                tig_debug_printf("MMUI: mmUIMPLoadGameRefreshFunc: ERROR: FAILED to refresh!\n");
            }
        }
    }

    mmUIWinRefreshScrollBar();
}

// 0x544100
void sub_544100(const char* str, TigRect* rect, tig_font_handle_t font)
{
    char mutable_str[200];
    TigFont font_desc;
    TigRect text_rect;

    strcpy(mutable_str, str);
    strcat(mutable_str, "|");

    tig_font_push(font);
    font_desc.width = 0;
    font_desc.height = 0;
    font_desc.str = mutable_str;
    font_desc.flags = 0;
    tig_font_measure(&font_desc);

    text_rect = *rect;

    if (font_desc.width < rect->width) {
        text_rect.x += (rect->width - font_desc.width) / 2;
        text_rect.width = font_desc.width;
        sub_5418A0(mutable_str, &text_rect, font, 0);
    }

    tig_font_pop();
}

// 0x544210
void sub_544210()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (window->selected_index > 0) {
        window->selected_index--;
        gsound_play_sfx(0, 1);
        sub_544290();
        window->refresh_func(NULL);
    }
}

// 0x544250
void sub_544250()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (window->selected_index < window->cnt - 1) {
        window->selected_index++;
        gsound_play_sfx(0, 1);
        sub_544290();
        window->refresh_func(NULL);
    }
}

// 0x544290
void sub_544290()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    sub_549450();

    if (!mainmenu_ui_gsi_loaded) {
        if (window->selected_index == 0) {
            byte_64C2F8[0] = 0;
            sub_5493C0(byte_64C2F8, 23);
        }
        return;
    }

    gamelib_saveinfo_exit(&mainmenu_ui_gsi);

    mainmenu_ui_gsi_loaded = false;
    if (window->selected_index > 0) {
        if (gamelib_saveinfo_load(mainmenu_ui_gsl.names[window->selected_index - 1], &mainmenu_ui_gsi)) {
            mainmenu_ui_gsi_loaded = true;
        }
    } else {
        byte_64C2F8[0] = 0;
        sub_5493C0(byte_64C2F8, 23);
    }
}

// 0x544320
bool mainmenu_ui_save_game_handle_delete()
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];

    if (window->selected_index <= 0) {
        return false;
    }

    // "Are you sure you want to delete the save game?"
    if (mainmenu_ui_confirm(5150) != TIG_WINDOW_MODAL_DIALOG_CHOICE_OK) {
        return false;
    }

    if (!gamelib_delete(mainmenu_ui_gsl.names[window->selected_index - 1])) {
        return false;
    }

    if (mainmenu_ui_gsi_loaded) {
        gamelib_saveinfo_exit(&mainmenu_ui_gsi);
        mainmenu_ui_gsi_loaded = false;
    }

    gamelib_savelist_destroy(&mainmenu_ui_gsl);
    gamelib_savelist_create(&mainmenu_ui_gsl);
    gamelib_savelist_sort(&mainmenu_ui_gsl, GAME_SAVE_LIST_ORDER_DATE, false);
    window->selected_index = -1;
    window->cnt--;
    window->refresh_func(NULL);

    return true;
}

// 0x544440
void mainmenu_ui_last_save_create()
{
    const char* path;
    TigRect rect;
    MesFileEntry mes_file_entry;
    char name[256];

    mainmenu_ui_window_type = MM_WINDOW_LAST_SAVE_GAME;
    mainmenu_ui_create_window();
    mainmenu_ui_active = false;

    path = gamelib_last_save_name();
    if (path[0] != '\0') {
        if (mainmenu_ui_gsi_loaded) {
            gamelib_saveinfo_exit(&mainmenu_ui_gsi);
            mainmenu_ui_gsi_loaded = false;
        }

        if (gamelib_saveinfo_load(path, &mainmenu_ui_gsi)) {
            mainmenu_ui_gsi_loaded = true;
            mainmenu_ui_active = true;

            rect.x = 0;
            rect.y = 0;
            rect.width = 800;
            rect.height = 600;
            tig_window_fill(dword_5C3624, &rect, tig_color_make(0, 0, 0));

            rect.x = 340;
            rect.y = 210;
            rect.width = 200;
            rect.height = 20;

            mes_file_entry.num = 5; // "Loading Save Game..."
            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
            tig_window_text_write(dword_5C3624, mes_file_entry.str, &rect);

            tig_mouse_hide();
            tig_window_display();
            strncpy(name, path, 8);
            name[8] = '\0';
            sub_5432B0(name);

            if (mainmenu_ui_gsi_loaded) {
                gamelib_saveinfo_exit(&mainmenu_ui_gsi);
                mainmenu_ui_gsi_loaded = false;
            }
        }
    } else {
        mainmenu_ui_active = true;
        mainmenu_ui_close(true);
        dword_64C38C = true;
    }

    tig_mouse_show();
}

// 0x5445F0
void mainmenu_ui_intro_create()
{
    mainmenu_ui_window_type = MM_WINDOW_INTRO;
    gmovie_play(1, 0, 0);
    gmovie_play(7, 0, 0);
    mainmenu_ui_num_windows++;
    mainmenu_ui_pop_window_stack();
    mainmenu_ui_window_type = MM_WINDOW_SINGLE_PLAYER;
    mainmenu_ui_open();
    dword_64C38C = true;
}

// 0x544640
void mainmenu_ui_credits_create()
{
    mainmenu_ui_window_type = MM_WINDOW_CREDITS;
    mainmenu_ui_num_windows++;
    mainmenu_ui_pop_window_stack();
    mainmenu_ui_window_type = MM_WINDOW_MAINMENU;
    mainmenu_ui_open();
    dword_64C38C = true;
    slide_ui_start(SLIDE_UI_TYPE_CREDITS);

    if (mainmenu_ui_active) {
        if (main_menu_window_info[mainmenu_ui_window_type]->refresh_func != NULL) {
            main_menu_window_info[mainmenu_ui_window_type]->refresh_func(NULL);
        }
    }
}

// 0x544690
void mainmenu_ui_last_save_refresh(TigRect* rect)
{
    (void)rect;
}

// 0x5446A0
void mainmenu_ui_create_single_player()
{
    mainmenu_ui_window_type = MM_WINDOW_SINGLE_PLAYER;
    mainmenu_ui_create_window();
    mainmenu_ui_draw_version();
    sub_5576B0();
}

// 0x5446D0
void mainmenu_ui_pick_new_or_pregen_create()
{
    dword_64C454 = CHAREDIT_MODE_CREATE;
    mainmenu_ui_window_type = MM_WINDOW_PICK_NEW_OR_PREGEN;
    mainmenu_ui_create_window();
    mainmenu_ui_draw_version();
}

// 0x5446F0
void mainmenu_ui_new_char_create()
{
    PlayerCreateInfo player_create_info;

    mainmenu_ui_new_char_hover_mode = MMUI_NEW_CHAR_HOVER_MODE_BACKGROUND;
    mainmenu_ui_window_type = MM_WINDOW_NEW_CHAR;

    player_create_info_init(&player_create_info);
    player_create_info.loc = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    player_create_info.basic_prototype = 16066;
    if (!player_obj_create_player(&player_create_info)) {
        tig_debug_printf("MainMenu-UI: mainmenu_ui_create_pick_new_or_pregen: ERROR: Player Creation Failed!\n");
        exit(EXIT_FAILURE);
    }

    mainmenu_ui_create_window();
    if (!main_menu_button_create(&mainmenu_ui_new_char_name_button, mainmenu_ui_new_char_name_rect.width + 2, mainmenu_ui_new_char_name_rect.height + 2)) {
        tig_debug_printf("MainMenu-UI: mainmenu_ui_create_new_char: ERROR: Failed to create button.\n");
    }
}

// 0x5447B0
void mainmenu_ui_new_char_refresh(TigRect* rect)
{
    int64_t pc_obj;
    char* str;
    MesFileEntry mes_file_entry;

    pc_obj = player_get_local_pc_obj();
    mmUISharedCharRefreshFunc(pc_obj, rect);

    if (rect == NULL
        || (mainmenu_ui_shared_char_name_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_name_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_name_rect.x + mainmenu_ui_shared_char_name_rect.width
            && rect->y < mainmenu_ui_shared_char_name_rect.y + mainmenu_ui_shared_char_name_rect.height)) {
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_name_rect, tig_color_make(0, 0, 0)) != TIG_OK) {
            tig_debug_printf("MainMenu-UI: mmUINewCharRefreshFunc: ERROR: Window Fill Failed.\n");
        }

        str = byte_64C2F8[0] != '\0' ? byte_64C2F8 : " ";

        // FIXME: Useless.
        mes_file_entry.num = 500; // "Choose Name"
        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

        if (textedit_ui_is_focused()) {
            sub_544100(str, &mainmenu_ui_shared_char_name_rect, dword_64C218[1]);
        } else {
            sub_542DF0(str, &mainmenu_ui_shared_char_name_rect, dword_64C218[1]);
        }
    }
}

// 0x5448E0
void mmUISharedCharRefreshFunc(int64_t obj, TigRect* rect)
{
    int race;
    int gender;
    int portrait;
    int background;
    MesFileEntry mes_file_entry;
    tig_font_handle_t font;
    int index;
    char str[32];

    race = stat_level_get(obj, STAT_RACE);
    gender = stat_level_get(obj, STAT_GENDER);

    if (rect == NULL
        || (mainmenu_ui_shared_char_portrait_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_portrait_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_portrait_rect.x + mainmenu_ui_shared_char_portrait_rect.width
            && rect->y < mainmenu_ui_shared_char_portrait_rect.y + mainmenu_ui_shared_char_portrait_rect.height)) {
        portrait = portrait_get(obj);
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_portrait_rect, tig_color_make(0, 0, 0)) != TIG_OK) {
            tig_debug_printf("MainMenu-UI: mmUINewCharRefreshFunc: ERROR: Window Fill #0 Failed.\n");
        }
        portrait_draw_128x128(obj,
            portrait,
            dword_5C3624,
            mainmenu_ui_shared_char_portrait_rect.x,
            mainmenu_ui_shared_char_portrait_rect.y);
    }

    font = dword_64C218[0];
    tig_font_push(font);
    if (rect == NULL
        || (mainmenu_ui_shared_char_gender_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_gender_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_gender_rect.x + mainmenu_ui_shared_char_gender_rect.width
            && rect->y < mainmenu_ui_shared_char_gender_rect.y + mainmenu_ui_shared_char_gender_rect.height)) {
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_gender_rect, tig_color_make(0, 0, 0)) == TIG_OK) {
            mes_file_entry.num = 740 + gender; // "Female", "Male"
            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
            sub_542DF0(mes_file_entry.str, &mainmenu_ui_shared_char_gender_rect, font);
        }
    }

    if (rect == NULL
        || (mainmenu_ui_shared_char_race_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_race_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_race_rect.x + mainmenu_ui_shared_char_race_rect.width
            && rect->y < mainmenu_ui_shared_char_race_rect.y + mainmenu_ui_shared_char_race_rect.height)) {
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_race_rect, tig_color_make(0, 0, 0)) == TIG_OK) {
            mes_file_entry.num = 720 + race; // "Human", "Dwarf", "Elf", etc.
            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
            sub_542DF0(mes_file_entry.str, &mainmenu_ui_shared_char_race_rect, font);
        }
    }

    if (rect == NULL
        || (mainmenu_ui_shared_char_background_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_background_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_background_rect.x + mainmenu_ui_shared_char_background_rect.width
            && rect->y < mainmenu_ui_shared_char_background_rect.y + mainmenu_ui_shared_char_background_rect.height)) {
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_background_rect, tig_color_make(0, 0, 0)) == TIG_OK) {
            background = background_text_get(obj);
            sub_542DF0(background_description_get_name(background),
                &mainmenu_ui_shared_char_background_rect,
                font);
        }
    }
    tig_font_pop();

    if (rect == NULL
        || (mainmenu_ui_shared_char_desc_view_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_desc_view_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_desc_view_rect.x + mainmenu_ui_shared_char_desc_view_rect.width
            && rect->y < mainmenu_ui_shared_char_desc_view_rect.y + mainmenu_ui_shared_char_desc_view_rect.height)) {
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_desc_view_rect, tig_color_make(0, 0, 0)) == TIG_OK) {
            if (mainmenu_ui_new_char_hover_mode == MMUI_NEW_CHAR_HOVER_MODE_GENDER) {
                font = dword_64C210[0];
                tig_font_push(font);
                mes_file_entry.num = 742 + gender;
                mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                if (mes_file_entry.str[0] != '\0') {
                    sub_542DF0(mes_file_entry.str, &mainmenu_ui_shared_char_desc_body_rect, font);
                } else {
                    tig_debug_printf("MainMenu-UI: mmUISharedCharRefreshFunc: ERROR: Failed to find Racial Description!\n");
                }
                tig_font_pop();
            } else {
                background = background_text_get(obj);
                if (background > 1000 && mainmenu_ui_new_char_hover_mode == MMUI_NEW_CHAR_HOVER_MODE_BACKGROUND) {
                    font = dword_64C210[0];
                    tig_font_push(font);
                    sub_542DF0(background_description_get_body(background),
                        &mainmenu_ui_shared_char_desc_view_rect,
                        font);
                    tig_font_pop();
                } else {
                    font = dword_64C210[1];
                    tig_font_push(font);
                    mes_file_entry.num = 745; // "Race"
                    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                    if (mes_file_entry.str[0] != '\0') {
                        sub_542DF0(mes_file_entry.str, &mainmenu_ui_shared_char_desc_title_rect, font);
                    } else {
                        tig_debug_printf("MainMenu-UI: mmUISharedCharRefreshFunc: ERROR: Failed to find Racial Description!\n");
                    }
                    tig_font_pop();

                    font = dword_64C210[0];
                    tig_font_push(font);
                    mes_file_entry.num = 770 + 2 * race + gender;
                    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                    if (mes_file_entry.str[0] != '\0') {
                        sub_542DF0(mes_file_entry.str, &mainmenu_ui_shared_char_desc_body_rect, font);
                    } else {
                        tig_debug_printf("MainMenu-UI: mmUISharedCharRefreshFunc: ERROR: Failed to find Racial Description!\n");
                    }
                    tig_font_pop();
                }
            }
        }
    }

    if (rect == NULL
        || (mainmenu_ui_shared_char_stats_view_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_stats_view_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_stats_view_rect.x + mainmenu_ui_shared_char_stats_view_rect.width
            && rect->y < mainmenu_ui_shared_char_stats_view_rect.y + mainmenu_ui_shared_char_stats_view_rect.height)) {
        font = dword_64C210[1];
        tig_font_push(font);
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_stats_title_rect, tig_color_make(0, 0, 0)) == TIG_OK) {
            mes_file_entry.num = 739; // "Initial Stats"
            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
            sub_542DF0(mes_file_entry.str,
                &mainmenu_ui_shared_char_stats_title_rect,
                font);
        }
        tig_font_pop();

        font = dword_64C210[0];
        tig_font_push(font);
        for (index = 0; index < 15; index++) {
            if (tig_window_fill(dword_5C3624, &(stru_5C4F50[index]), tig_color_make(0, 0, 0)) == TIG_OK) {
                sprintf(str,
                    "%s: %d",
                    stat_short_name(dword_5C5130[index]),
                    stat_level_get(obj, dword_5C5130[index]));
                sub_5418A0(str, &(stru_5C4F50[index]), font, 0);
            }
        }
        tig_font_pop();
    }
}

// 0x544FF0
bool mainmenu_ui_new_char_button_released(tig_button_handle_t button_handle)
{
    int index;
    int64_t pc_obj;
    MainMenuWindowInfo* window;
    int portrait;
    int background;

    for (index = 0; index < 10; index++) {
        if (button_handle == mainmenu_ui_new_char_buttons[index].button_handle) {
            break;
        }
    }

    if (index >= 10) {
        if (button_handle == mainmenu_ui_new_char_name_button.button_handle
            && sub_549520() == NULL) {
            strcpy(byte_64C0F0, byte_64C2F8);
            byte_64C2F8[0] = '\0';
            sub_5493C0(byte_64C2F8, 23);
        }

        return true;
    }

    pc_obj = player_get_local_pc_obj();
    window = main_menu_window_info[mainmenu_ui_window_type];

    switch (index) {
    case 0:
        if (window->execute_func != NULL && !window->execute_func(0)) {
            return true;
        }

        mainmenu_ui_close(false);
        mainmenu_ui_window_type = MM_WINDOW_CHAREDIT;
        mainmenu_ui_open();
        return true;
    case 1:
        mainmenu_ui_close(true);
        return true;
    case 2:
        portrait = portrait_get(pc_obj);
        if (!portrait_find_prev(pc_obj, &portrait)) {
            portrait_find_last(pc_obj, &portrait);
        }
        obj_field_int32_set(pc_obj, OBJ_F_CRITTER_PORTRAIT, portrait);
        window->refresh_func(&mainmenu_ui_shared_char_portrait_rect);
        return true;
    case 3:
        portrait = portrait_get(pc_obj);
        if (!portrait_find_next(pc_obj, &portrait)) {
            portrait_find_first(pc_obj, &portrait);
        }
        obj_field_int32_set(pc_obj, OBJ_F_CRITTER_PORTRAIT, portrait);
        window->refresh_func(&mainmenu_ui_shared_char_portrait_rect);
        return true;
    case 4:
        if (mainmenu_ui_new_char_prev_race(pc_obj)) {
            window->refresh_func(NULL);
        }
        return true;
    case 5:
        if (mainmenu_ui_new_char_next_race(pc_obj)) {
            window->refresh_func(NULL);
        }
        return true;
    case 6:
        background = background_get(pc_obj);
        if (mainmenu_ui_new_char_prev_background(pc_obj, &background)) {
            background_clear(pc_obj);
            background_set(pc_obj, background, background_get_description(background));
            window->refresh_func(NULL);
        }
        return true;
    case 7:
        background = background_get(pc_obj);
        if (mainmenu_ui_new_char_next_background(pc_obj, &background)) {
            background_clear(pc_obj);
            background_set(pc_obj, background, background_get_description(background));
            window->refresh_func(NULL);
        }
        return true;
    case 8:
        if (!dword_5C3620 && mainmenu_ui_new_char_prev_gender(pc_obj)) {
            window->refresh_func(NULL);
        }
        return true;
    case 9:
        if (!dword_5C3620 && mainmenu_ui_new_char_next_gender(pc_obj)) {
            window->refresh_func(NULL);
        }
        return true;
    default:
        return true;
    }
}

// 0x5452C0
bool mainmenu_ui_new_char_next_background(int64_t obj, int* background_ptr)
{
    if (background_find_next(obj, background_ptr)) {
        return true;
    }

    if (background_find_first(obj, background_ptr)) {
        return true;
    }

    return false;
}

// 0x545300
bool mainmenu_ui_new_char_prev_background(int64_t obj, int* background_ptr)
{
    if (background_find_prev(obj, background_ptr)) {
        return true;
    }

    while (background_find_next(obj, background_ptr)) {
    }

    return true;
}

// 0x545350
bool mainmenu_ui_new_char_prev_gender(int64_t obj)
{
    if (stat_level_get(obj, STAT_GENDER) == GENDER_FEMALE) {
        background_clear(obj);
        return mainmenu_ui_new_char_set_gender(obj, GENDER_MALE);
    } else {
        background_clear(obj);
        return mainmenu_ui_new_char_set_gender(obj, GENDER_FEMALE);
    }
}

// 0x5453A0
bool mainmenu_ui_new_char_set_gender(int64_t obj, int gender)
{
    int race;
    int portrait;

    if (stat_level_get(obj, STAT_GENDER) == gender) {
        return false;
    }

    race = stat_level_get(obj, STAT_RACE);
    if (gender == GENDER_FEMALE
        && !stru_5C5170[race].available_for_female) {
        return false;
    }

    object_set_gender_and_race(obj, stru_5C5170[race].body_type, gender, race);
    object_set_current_aid(obj, obj_field_int32_get(obj, OBJ_F_CURRENT_AID));

    if (portrait_find_first(obj, &portrait)) {
        obj_field_int32_set(obj, OBJ_F_CRITTER_PORTRAIT, portrait);
    }

    return true;
}

// 0x545440
bool mainmenu_ui_new_char_next_gender(int64_t obj)
{
    if (stat_level_get(obj, STAT_GENDER) == GENDER_MALE) {
        background_clear(obj);
        return mainmenu_ui_new_char_set_gender(obj, GENDER_FEMALE);
    } else {
        background_clear(obj);
        return mainmenu_ui_new_char_set_gender(obj, GENDER_MALE);
    }
}

// 0x545490
bool mainmenu_ui_new_char_prev_race(int64_t obj)
{
    int race;

    race = stat_level_get(obj, STAT_RACE);
    if (race > 0) {
        if (stat_level_get(obj, STAT_GENDER) == GENDER_FEMALE) {
            do {
                race--;
            } while (race >= 0 && !stru_5C5170[race].available_for_female);

            if (race < 0) {
                return false;
            }

            if (!stru_5C5170[race].available_for_female) {
                return false;
            }
        } else {
            race--;
        }

        if (race < 0) {
            return false;
        }
    } else {
        race = 7;
        if (stat_level_get(obj, STAT_GENDER) == GENDER_FEMALE) {
            while (race >= 0 && !stru_5C5170[race].available_for_female) {
                race--;
            }

            if (race < 0) {
                return false;
            }

            if (!stru_5C5170[race].available_for_female) {
                return false;
            }
        }
    }

    background_clear(obj);
    mainmenu_ui_new_char_set_race(obj, race);

    return true;
}

// 0x545550
void mainmenu_ui_new_char_set_race(int64_t obj, int race)
{
    int gender;
    int portrait;

    gender = stat_level_get(obj, STAT_GENDER);
    if (gender == GENDER_FEMALE) {
        gender = !stru_5C5170[race].available_for_female
            ? GENDER_MALE
            : GENDER_FEMALE;
    }

    object_set_gender_and_race(obj, stru_5C5170[race].body_type, gender, race);
    object_set_current_aid(obj, obj_field_int32_get(obj, OBJ_F_CURRENT_AID));

    if (portrait_find_first(obj, &portrait)) {
        obj_field_int32_set(obj, OBJ_F_CRITTER_PORTRAIT, portrait);
    }
}

// 0x5455D0
bool mainmenu_ui_new_char_next_race(int64_t obj)
{
    int race;

    race = stat_level_get(obj, STAT_RACE);
    if (race < 7) {
        if (stat_level_get(obj, STAT_GENDER) == GENDER_FEMALE) {
            do {
                race++;
            } while (race < 7 && !stru_5C5170[race].available_for_female);

            if (race >= 8) {
                // FIXME: Unreachable.
                return false;
            }

            if (!stru_5C5170[race].available_for_female) {
                race = RACE_HUMAN;
            }
        } else {
            race++;
        }
    } else {
        race = RACE_HUMAN;
        if (stat_level_get(obj, STAT_GENDER) == GENDER_FEMALE) {
            while (!stru_5C5170[race].available_for_female) {
                race++;
            }

            if (race >= 8) {
                // FIXME: Unreachable.
                return false;
            }

            if (!stru_5C5170[race].available_for_female) {
                return false;
            }
        }
    }

    if (race < 8) {
        background_clear(obj);
        mainmenu_ui_new_char_set_race(obj, race);
        return true;
    }

    return false;
}

// 0x545690
bool mainmenu_ui_new_char_button_hover(tig_button_handle_t button_handle)
{
    (void)button_handle;

    return true;
}

// 0x5456A0
bool mainmenu_ui_new_char_button_leave(tig_button_handle_t button_handle)
{
    (void)button_handle;

    return true;
}

// 0x5456B0
void mainmenu_ui_new_char_mouse_idle(int x, int y)
{
    MainMenuUiNewCharHoverMode mode;

    y -= 43;
    if (x >= mainmenu_ui_new_char_gender_hover_rect.x
        && y >= mainmenu_ui_new_char_gender_hover_rect.y
        && x < mainmenu_ui_new_char_gender_hover_rect.x + mainmenu_ui_new_char_gender_hover_rect.width
        && y < mainmenu_ui_new_char_gender_hover_rect.y + mainmenu_ui_new_char_gender_hover_rect.height) {
        mode = MMUI_NEW_CHAR_HOVER_MODE_GENDER;
    } else if (x >= mainmenu_ui_new_char_race_hover_rect.x
        && y >= mainmenu_ui_new_char_race_hover_rect.y
        && x < mainmenu_ui_new_char_race_hover_rect.x + mainmenu_ui_new_char_race_hover_rect.width
        && y < mainmenu_ui_new_char_race_hover_rect.y + mainmenu_ui_new_char_race_hover_rect.height) {
        mode = MMUI_NEW_CHAR_HOVER_MODE_RACE;
    } else {
        mode = MMUI_NEW_CHAR_HOVER_MODE_BACKGROUND;
    }

    if (mainmenu_ui_new_char_hover_mode != mode) {
        mainmenu_ui_new_char_hover_mode = mode;
        main_menu_window_info[mainmenu_ui_window_type]->refresh_func(&mainmenu_ui_shared_char_desc_view_rect);
    }
}

// 0x545780
bool mainmenu_ui_new_char_execute(int btn)
{
    int64_t pc_obj;
    MesFileEntry mes_file_entry;

    (void)btn;

    pc_obj = player_get_local_pc_obj();

    mes_file_entry.num = 500; // "Choose Name"
    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
    if (byte_64C2F8[0] == '\0') {
        strcpy(byte_64C2F8, mes_file_entry.str);
    }

    if (strcmp(byte_64C2F8, mes_file_entry.str) == 0) {
        mes_file_entry.num = 506; // "You must choose a name."
        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
        sub_550770(-1, mes_file_entry.str);
        return false;
    }

    obj_field_string_set(pc_obj, OBJ_F_PC_PLAYER_NAME, byte_64C2F8);
    ui_spell_maintain_refresh();
    mainmenu_ui_auto_equip_items_on_start = true;

    return true;
}

// 0x545870
void mainmenu_ui_pregen_char_create()
{
    mainmenu_ui_window_type = MM_WINDOW_PREGEN_CHAR;
    mainmenu_ui_pregen_char_idx = 1;
    qword_64C460 = objp_perm_lookup(obj_get_id(sub_4685A0(BP_MERWIN_TUMBLEBROOK)));
    mainmenu_ui_create_window();
}

// 0x5458D0
void mainmenu_ui_pregen_char_refresh(TigRect* rect)
{
    char* name;

    mmUISharedCharRefreshFunc(qword_64C460, rect);
    if (rect == NULL
        || (mainmenu_ui_shared_char_name_rect.x < rect->x + rect->width
            && mainmenu_ui_shared_char_name_rect.y < rect->y + rect->height
            && rect->x < mainmenu_ui_shared_char_name_rect.x + mainmenu_ui_shared_char_name_rect.width
            && rect->y < mainmenu_ui_shared_char_name_rect.y + mainmenu_ui_shared_char_name_rect.height)) {
        tig_font_push(dword_64C218[1]);
        if (tig_window_fill(dword_5C3624, &mainmenu_ui_shared_char_name_rect, tig_color_make(0, 0, 0)) == TIG_OK) {
            obj_field_string_get(qword_64C460, OBJ_F_PC_PLAYER_NAME, &name);
            if (name != NULL) {
                sub_542DF0(name, &mainmenu_ui_shared_char_name_rect, dword_64C218[1]);
                FREE(name);
            }
        }
        tig_font_pop();
    }
}

// 0x5459F0
bool mainmenu_ui_pregen_char_button_released(tig_button_handle_t button_handle)
{
    int index;
    int64_t pc_obj;
    MainMenuWindowInfo* window;

    for (index = 0; index < 4; index++) {
        if (button_handle == mainmenu_ui_pregen_char_buttons[index].button_handle) {
            break;
        }
    }

    if (index >= 4) {
        return true;
    }

    pc_obj = player_get_local_pc_obj();
    window = main_menu_window_info[mainmenu_ui_window_type];

    switch (index) {
    case 0:
        if (window->execute_func != NULL && !window->execute_func(0)) {
            return true;
        }
        sub_5412D0();
        return true;
    case 1:
        mainmenu_ui_close(true);
        return true;
    case 2:
        if (mainmenu_ui_pregen_char_idx > 1) {
            mainmenu_ui_pregen_char_idx--;
        } else {
            mainmenu_ui_pregen_char_idx = mainmenu_ui_pregen_char_cnt - 1;
        }
        qword_64C460 = objp_perm_lookup(obj_get_id(sub_4685A0(mainmenu_ui_pregen_char_idx + BP_GENERIC_PC)));
        window->refresh_func(NULL);
        return true;
    case 3:
        if (mainmenu_ui_pregen_char_idx < mainmenu_ui_pregen_char_cnt - 1) {
            mainmenu_ui_pregen_char_idx++;
        } else {
            mainmenu_ui_pregen_char_idx = 1;
        }
        qword_64C460 = objp_perm_lookup(obj_get_id(sub_4685A0(mainmenu_ui_pregen_char_idx + BP_GENERIC_PC)));
        window->refresh_func(NULL);
        return true;
    default:
        return true;
    }
}

// 0x545B60
bool mainmenu_ui_pregen_char_execute(int btn)
{
    PlayerCreateInfo player_create_info;
    MesFileEntry mes_file_entry;
    int index;
    int flag;

    (void)btn;

    mainmenu_ui_start_new_game = true;

    player_create_info_init(&player_create_info);
    player_create_info.loc = obj_field_int64_get(player_get_local_pc_obj(), OBJ_F_LOCATION);
    player_create_info.basic_prototype = mainmenu_ui_pregen_char_idx + BP_GENERIC_PC;
    if (!player_obj_create_player(&player_create_info)) {
        tig_debug_printf("MainMenu-UI: mmUIPregenCharExecuteFunc: ERROR: Player Creation Failed!\n");
        exit(EXIT_FAILURE);
    }

    for (index = 0; index < mainmenu_ui_pregen_char_cnt - 1; index++) {
        mes_file_entry.num = 551 + index;
        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
        flag = atoi(mes_file_entry.str);
        if (flag > 0) {
            script_global_flag_set(flag, mainmenu_ui_pregen_char_idx - 1 == index);
        }
    }

    item_wield_best_all(player_create_info.obj, OBJ_HANDLE_NULL);
    sub_5412D0();

    return true;
}

// 0x545C50
void mainmenu_ui_charedit_create()
{
    mainmenu_ui_window_type = MM_WINDOW_CHAREDIT;
    mainmenu_ui_create_window();
}

// 0x545C60
void mainmenu_ui_charedit_destroy()
{
    if (charedit_is_created()) {
        charedit_close();
    }
}

// 0x545C70
bool mainmenu_ui_charedit_button_released(tig_button_handle_t button_handle)
{
    if (button_handle == mainmenu_ui_charedit_buttons[0].button_handle) {
        sub_5480C0(2);
        return true;
    }

    if (button_handle == mainmenu_ui_charedit_buttons[1].button_handle) {
        mainmenu_ui_close(true);
        return true;
    }

    return false;
}

// 0x545DF0
void mainmenu_ui_charedit_refresh(TigRect* rect)
{
    int64_t pc_obj;

    (void)rect;

    pc_obj = player_get_local_pc_obj();
    if (!charedit_open(pc_obj, dword_64C454)) {
        mainmenu_ui_close(true);
    }
    dword_64C454 = CHAREDIT_MODE_3;
}

// 0x545E20
void mainmenu_ui_shop_create()
{
    mainmenu_ui_window_type = MM_WINDOW_SHOP;
    mainmenu_ui_create_window();
}

// 0x545E30
void mainmenu_ui_shop_destroy()
{
    if (inven_ui_is_created()) {
        inven_ui_destroy();
    }
}

// 0x545E40
bool mainmenu_ui_shop_button_released(tig_button_handle_t button_handle)
{
    if (button_handle == mainmenu_ui_shop_buttons[0].button_handle) {
        sub_5412D0();
        return true;
    }

    if (button_handle == mainmenu_ui_shop_buttons[1].button_handle) {
        mainmenu_ui_close(true);
        return true;
    }

    return false;
}

// 0x545E80
void mainmenu_ui_shop_refresh(TigRect* rect)
{
    int64_t pc_obj;
    LocRect loc_rect;
    ObjectList objects;
    int64_t npc_obj;
    int64_t substitute_inventory_obj;

    (void)rect;

    pc_obj = player_get_local_pc_obj();
    item_generate_inventory(pc_obj);
    if (map_by_type(MAP_TYPE_SHOPPING_MAP) == 0) {
        sub_5412D0();
        return;
    }

    loc_rect.x1 = 0;
    loc_rect.y1 = 0;
    location_limits_get(&(loc_rect.x2), &(loc_rect.y2));
    object_list_rect(&loc_rect, OBJ_TM_NPC, &objects);

    if (objects.head != NULL) {
        npc_obj = objects.head->obj;
    } else {
        npc_obj = OBJ_HANDLE_NULL;
    }
    object_list_destroy(&objects);

    mainmenu_ui_start_new_game = true;

    if (npc_obj == OBJ_HANDLE_NULL) {
        sub_5412D0();
        return;
    }

    sub_4C0F50(npc_obj, pc_obj);
    sub_463E20(npc_obj);

    substitute_inventory_obj = critter_substitute_inventory_get(npc_obj);
    if (substitute_inventory_obj != OBJ_HANDLE_NULL) {
        sub_463E20(substitute_inventory_obj);
    }

    if (!inven_ui_open(pc_obj, npc_obj, INVEN_UI_MODE_BARTER)) {
        sub_5412D0();
        return;
    }
}

// 0x5461A0
bool main_menu_button_create(MainMenuButtonInfo* info, int width, int height)
{
    return main_menu_button_create_ex(info, width, height, TIG_BUTTON_MOMENTARY);
}

// 0x5461C0
bool main_menu_button_create_ex(MainMenuButtonInfo* info, int width, int height, unsigned int flags)
{
    TigButtonData button_data;
    int index;

    button_data.flags = flags;
    button_data.x = info->x;
    button_data.y = info->y;

    if (info->art_num != -1) {
        tig_art_interface_id_create(info->art_num, 0, 0, 0, &(button_data.art_id));
    } else {
        button_data.art_id = TIG_ART_ID_INVALID;
        button_data.width = width;
        button_data.height = height;
    }

    if ((info->flags & 0x1) != 0) {
        for (index = 0; index < 3; index++) {
            if (info->x >= stru_5C3680[index].x
                && info->x < stru_5C3680[index].x + stru_5C3680[index].width
                && info->y + 441 >= stru_5C3680[index].y
                && info->y + 441 < stru_5C3680[index].y + stru_5C3680[index].height) {
                break;
            }
        }

        if (index >= 3) {
            return false;
        }

        button_data.window_handle = dword_5C3670[index];
        button_data.x -= stru_5C3680[index].x;
    } else {
        button_data.window_handle = dword_5C3624;
        button_data.y -= stru_5C3628.y;
    }

    if ((info->flags & 0x2) != 0) {
        button_data.flags |= TIG_BUTTON_TOGGLE;
    }

    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    if (info->art_num != -1) {
        button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
        button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    } else {
        button_data.mouse_up_snd_id = -1;
        button_data.mouse_down_snd_id = SND_INTERFACE_MORPHTEXT_CLICK;
        button_data.mouse_enter_snd_id = SND_INTERFACE_MORPHTEXT_HOVER;
    }

    return tig_button_create(&button_data, &(info->button_handle)) == TIG_OK;
}

// 0x546330
void mainmenu_ui_create_window()
{
    mainmenu_ui_create_window_func(true);
}

// 0x546340
void mainmenu_ui_create_window_func(bool should_display)
{
    MainMenuWindowInfo* window;
    MainMenuButtonInfo* button;
    MesFileEntry mes_file_entry;
    TigArtBlitInfo art_blit_info;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigFont font_desc;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect text_rect;
    TigWindowData window_data;
    tig_art_id_t art_id;
    tig_font_handle_t font;
    tig_window_handle_t window_handle;
    bool v1 = false;
    int idx;
    int rc;

    if (dword_64C388) {
        should_display = false;
    }

    if (mainmenu_ui_active) {
        return;
    }

    window = main_menu_window_info[mainmenu_ui_window_type];
    if (window->background_art_num != -1) {
        tig_art_interface_id_create(window->background_art_num, 0, 0, 0, &art_id);
        if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
            if (art_frame_data.height == 600) {
                stru_5C3628 = stru_5C3638;
            } else {
                stru_5C3628 = stru_5C3648;
                v1 = true;
            }
        }

        if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
            window_data.flags = TIG_WINDOW_ALWAYS_ON_TOP | TIG_WINDOW_MESSAGE_FILTER;
            window_data.rect = stru_5C3628;
            window_data.background_color = art_anim_data.color_key;
            window_data.color_key = art_anim_data.color_key;
            window_data.message_filter = sub_546EE0;
            hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

            src_rect.x = stru_5C3628.x;
            src_rect.y = 0;
            src_rect.width = stru_5C3628.width;
            src_rect.height = stru_5C3628.height;

            dst_rect.x = 0;
            dst_rect.y = 0;
            dst_rect.width = stru_5C3628.width;
            dst_rect.height = stru_5C3628.height;

            art_blit_info.flags = 0;
            art_blit_info.art_id = art_id;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;

            if (tig_window_create(&window_data, &dword_5C3624) != TIG_OK) {
                tig_debug_printf("mainmenu_ui_create_window_func: ERROR: tig_art_anim_data failed!\n");
                exit(EXIT_SUCCESS); // FIXME: Should be `EXIT_FAILURE`.
            }

            tig_window_blit_art(dword_5C3624, &art_blit_info);
        }
    } else {
        v1 = true;
    }

    if (v1) {
        v1 = false;

        tig_art_interface_id_create(335, 0, 0, 0, &art_id);
        if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
            window_data.flags = TIG_WINDOW_ALWAYS_ON_TOP | TIG_WINDOW_MESSAGE_FILTER;
            window_data.background_color = art_anim_data.color_key;
            window_data.color_key = art_anim_data.color_key;
            window_data.message_filter = sub_546EE0;

            for (idx = 0; idx < 3; idx++) {
                window_data.rect.x = stru_5C3680[idx].x;
                window_data.rect.y = stru_5C3680[idx].y;
                window_data.rect.width = stru_5C3680[idx].width;
                window_data.rect.height = stru_5C3680[idx].height;
                hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

                src_rect.x = stru_5C3680[idx].x;
                src_rect.y = 0;
                src_rect.width = stru_5C3680[idx].width;
                src_rect.height = stru_5C3680[idx].height;

                dst_rect.x = 0;
                dst_rect.y = 0;
                dst_rect.width = src_rect.width;
                dst_rect.height = src_rect.height;

                art_blit_info.flags = 0;
                art_blit_info.art_id = art_id;
                art_blit_info.src_rect = &src_rect;
                art_blit_info.dst_rect = &dst_rect;

                rc = tig_window_create(&window_data, &(dword_5C3670[idx]));
                if (rc != TIG_OK) {
                    tig_debug_printf("MainMenu-UI: mainmenu_ui_create_window_func: ERROR: tig_window_create failed: Result: %d!\n", rc);
                    exit(EXIT_FAILURE);
                }

                tig_window_blit_art(dword_5C3670[idx], &art_blit_info);

                v1 = true;
            }
        }

        tig_art_interface_id_create(336, 0, 0, 0, &art_id);
        if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
            window_data.flags = TIG_WINDOW_ALWAYS_ON_TOP | TIG_WINDOW_MESSAGE_FILTER;
            window_data.rect = stru_5C3660;
            window_data.background_color = art_anim_data.color_key;
            window_data.color_key = art_anim_data.color_key;
            window_data.message_filter = sub_546EE0;
            hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);

            src_rect.x = stru_5C3660.x;
            src_rect.y = 0;
            src_rect.width = stru_5C3660.width;
            src_rect.height = stru_5C3660.height;

            dst_rect.x = 0;
            dst_rect.y = 0;
            dst_rect.width = stru_5C3660.width;
            dst_rect.height = stru_5C3660.height;

            art_blit_info.flags = 0;
            art_blit_info.art_id = art_id;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;

            if (tig_window_create(&window_data, &dword_5C3658) != TIG_OK) {
                if (!v1) {
                    tig_debug_printf("mainmenu_ui_create_window_func: ERROR: tig_art_anim_data2 failed!\n");
                    exit(EXIT_SUCCESS); // FIXME: Should be `EXIT_FAILURE`.
                }
            }

            tig_window_blit_art(dword_5C3658, &art_blit_info);
        } else {
            if (!v1) {
                tig_debug_printf("mainmenu_ui_create_window_func: ERROR: tig_art_anim_data2 failed!\n");
                exit(EXIT_SUCCESS); // FIXME: Should be `EXIT_FAILURE`.
            }
        }
    }

    if (window->background_art_num != -1) {
        src_rect.x = 0;
        src_rect.y = 0;
        art_blit_info.flags = 0;

        for (idx = 0; idx < 2; idx++) {
            if (window->field_3C[idx].field_0 != -1) {
                tig_art_interface_id_create(window->field_3C[idx].field_0, 0, 0, 0, &art_id);
                stru_64B870[idx].art_id = art_id;
                if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
                    tig_debug_printf("mainmenu_ui_create_window_func: ERROR: tig_art_frame_data failed!\n");
                    exit(EXIT_FAILURE);
                }

                src_rect.width = art_frame_data.width;
                src_rect.height = art_frame_data.height;

                dst_rect.x = window->field_3C[idx].x;
                dst_rect.y = window->field_3C[idx].y;
                dst_rect.width = art_frame_data.width;
                dst_rect.height = art_frame_data.height;

                art_blit_info.art_id = art_id;
                tig_window_blit_art(dword_5C3624, &art_blit_info);
            }
        }

        sub_547EF0();
    }

    mes_file_entry.num = window->num;
    font_desc.width = 0;
    font_desc.height = 0;

    if ((window->refresh_text_flags & 0x1) != 0) {
        if ((window->refresh_text_flags & 0x8) != 0) {
            font = dword_64C228[0][0];
        } else {
            font = dword_64C0CC[0][0];
        }
    } else {
        if ((window->refresh_text_flags & 0x10) != 0) {
            font = dword_64C218[0];
        } else {
            font = dword_64C210[0];
        }
    }

    for (idx = 0; idx < window->num_buttons; idx++) {
        button = &(window->buttons[idx]);

        if ((button->flags & 0x1) != 0) {
            int j;

            for (j = 0; j < 3; j++) {
                if (button->x >= stru_5C3680[j].x
                    && button->y + 441 >= stru_5C3680[j].y
                    && button->x < stru_5C3680[j].x + stru_5C3680[j].width
                    && button->y + 441 < stru_5C3680[j].y + stru_5C3680[j].height) {
                    break;
                }
            }

            if (j >= 3) {
                tig_debug_printf("mainmenu_ui_create_window_func: ERROR: j >= MM_UI_NUM_ROTWIN_COVERS!\n");
                exit(EXIT_FAILURE);
            }
            window_handle = dword_5C3670[j];
        } else {
            window_handle = dword_5C3624;
        }

        if (mes_file_entry.num != -1
            && (button->flags & 0x4) == 0) {
            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
            tig_font_push(font);
            font_desc.str = mes_file_entry.str;
            font_desc.width = 0;
            font_desc.height = 0;
            font_desc.flags = 0;
            tig_font_measure(&font_desc);
            tig_font_pop();

            if ((window->refresh_text_flags & 0x20) == 0) {
                button->rect.width = font_desc.width;
                button->rect.height = font_desc.height;

                if ((window->refresh_text_flags & 0x04) != 0) {
                    button->x -= font_desc.width / 2;
                }
            }

            text_rect = button->rect;
            text_rect.x = button->x - window->field_30;
            text_rect.y = button->y - window->field_34;
            mainmenu_ui_refresh_text(window_handle,
                mes_file_entry.str,
                &text_rect,
                window->refresh_text_flags | 0x20);

            mes_file_entry.num += 10;

            if (button->field_14 == 0 && (button->flags & 0x08) == 0) {
                if (mes_search(mainmenu_ui_mainmenu_mes_file, &mes_file_entry)) {
                    mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
                    button->field_14 = SDL_toupper(mes_file_entry.str[0]);
                } else {
                    tig_debug_printf("MainMenu: Error: Can't Find Hotkey!");
                    button->field_14 = -1;
                }
            }

            mes_file_entry.num -= 9;
        }

        if (!main_menu_button_create(button, font_desc.width, font_desc.height)) {
            tig_debug_printf("mainmenu_ui_create_window_func: ERROR: main_menu_button_create failed!\n");
            exit(EXIT_FAILURE);
        }
    }

    window->refresh_text_flags |= 0x20;
    mainmenu_ui_active = true;

    if (window->refresh_func != NULL) {
        window->refresh_func(NULL);
    }

    if (should_display) {
        tig_window_display();
    }
}

// 0x546B40
void mainmenu_ui_refresh_text(tig_window_handle_t window_handle, const char* str, TigRect* rect, unsigned int flags)
{
    TigRect text_rect;
    TigFont font_desc;
    TigArtAnimData art_anim_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    tig_font_handle_t* fonts;
    tig_art_id_t art_id;
    int pass;

    text_rect = *rect;

    if ((flags & 0x1) == 0) {
        fonts = (flags & 0x10) != 0 ? dword_64C0CC[0] : dword_64C210;

        if ((flags & 0x20) == 0) {
            tig_font_push(fonts[0]);
            font_desc.width = 0;
            font_desc.height = 0;
            font_desc.str = str;
            font_desc.flags = 0;
            tig_font_measure(&font_desc);
            tig_font_pop();

            text_rect.width = font_desc.width;
            text_rect.height = font_desc.height;
            if ((flags & 0x4) != 0) {
                text_rect.x -= font_desc.width / 2;
            }
        }

        tig_font_push(fonts[0]);
        if (tig_window_text_write(window_handle, str, &text_rect) != TIG_OK) {
            tig_debug_printf("mainmenu_ui_refresh_text: ERROR: tig_window_text_write failed!\n");
        }
        tig_font_pop();
    } else {
        if ((flags & 0x08) != 0) {
            fonts = dword_64C228[(flags & 0x2) != 0 ? 1 : 0];
        } else {
            fonts = dword_64C0CC[(flags & 0x2) != 0 ? 1 : 0];
        }

        if ((flags & 0x20) == 0) {
            tig_font_push(fonts[0]);
            font_desc.width = 0;
            font_desc.height = 0;
            font_desc.str = str;
            font_desc.flags = 0;
            tig_font_measure(&font_desc);
            tig_font_pop();

            text_rect.width = font_desc.width;
            text_rect.height = font_desc.height;
            if ((flags & 0x4) != 0) {
                text_rect.x -= font_desc.width / 2;
            }
        }

        if (main_menu_window_info[mainmenu_ui_window_type]->background_art_num != -1) {
            tig_art_interface_id_create(main_menu_window_info[mainmenu_ui_window_type]->background_art_num, 0, 0, 0, &art_id);
            if (tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
                src_rect.x = rect->x;
                src_rect.y = rect->y;
                src_rect.width = rect->width + 1;
                src_rect.height = rect->height + 1;

                dst_rect.x = rect->x;
                dst_rect.y = rect->y;
                dst_rect.width = rect->width + 1;
                dst_rect.height = rect->height + 1;

                art_blit_info.flags = 0;
                art_blit_info.art_id = art_id;
                art_blit_info.src_rect = &src_rect;
                art_blit_info.dst_rect = &dst_rect;
                tig_window_blit_art(dword_5C3624, &art_blit_info);
            }
        }

        for (pass = 0; pass < 3; pass++) {
            tig_font_push(fonts[pass]);
            text_rect.x += dword_5C4070[pass];
            text_rect.y += dword_5C4070[pass];
            if (tig_window_text_write(window_handle, str, &text_rect) != TIG_OK) {
                tig_debug_printf("mainmenu_ui_refresh_text: ERROR: tig_window_text_write2 failed!\n");
            }
            tig_font_pop();
        }
    }
}

// 0x546DD0
void sub_546DD0()
{
    int index;

    if (mainmenu_ui_active) {
        sub_549450();
        timeevent_clear_all_typed(TIMEEVENT_TYPE_MAINMENU);

        for (index = 0; index < 2; index++) {
            stru_64B870[index].art_id = TIG_ART_ID_INVALID;
        }

        if (dword_5C3624 != TIG_WINDOW_HANDLE_INVALID
            && tig_window_destroy(dword_5C3624) == TIG_OK) {
            dword_5C3624 = TIG_WINDOW_HANDLE_INVALID;
        }

        for (index = 0; index < 3; index++) {
            if (dword_5C3670[index] != TIG_WINDOW_HANDLE_INVALID
                && tig_window_destroy(dword_5C3670[index]) == TIG_OK) {
                dword_5C3670[index] = TIG_WINDOW_HANDLE_INVALID;
            }
        }

        if (dword_5C3658 != TIG_WINDOW_HANDLE_INVALID
            && tig_window_destroy(dword_5C3658) == TIG_OK) {
            dword_5C3658 = TIG_WINDOW_HANDLE_INVALID;
        }

        mainmenu_ui_active = false;
    }
}

// 0x546E80
void mainmenu_ui_create_shared_radio_buttons()
{
    MainMenuWindowInfo* info;
    tig_button_handle_t group[2];

    info = main_menu_window_info[mainmenu_ui_window_type];
    group[0] = info->buttons[4].button_handle;
    group[1] = info->buttons[5].button_handle;
    if (tig_button_radio_group_create(2, group, info->flags & 1) != TIG_OK) {
        tig_debug_printf("mainmenu_ui_create_shared_radio_buttons: ERROR: tig_button_radio_group failed!\n");
    }
}

// 0x546EE0
bool sub_546EE0(TigMessage* msg)
{
    MainMenuWindowInfo* window;
    int idx;
    MesFileEntry mes_file_entry;
    MesFileEntry description_mes_file_entry;
    UiMessage ui_message;
    char str[MAX_STRING];
    int v2;

    // Convert mouse position from screen coordinate system to centered 800x600
    // area.
    if (msg->type == TIG_MESSAGE_MOUSE) {
        TigRect rect = { 0, 0, 800, 600 };
        hrp_apply(&rect, GRAVITY_CENTER_HORIZONTAL | GRAVITY_CENTER_VERTICAL);
        msg->data.mouse.x -= rect.x;
        msg->data.mouse.y -= rect.y;
    }

    window = main_menu_window_info[mainmenu_ui_window_type];

    if (slide_ui_is_active()) {
        return false;
    }

    if (msg->type == TIG_MESSAGE_MOUSE) {
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN) {
            if (window->scrollbar_rect.width > 0
                && msg->data.mouse.x >= window->content_rect.x
                && msg->data.mouse.y >= window->content_rect.y
                && msg->data.mouse.x < window->content_rect.x + window->content_rect.width
                && msg->data.mouse.y < window->content_rect.y + window->content_rect.height) {
                if (window->mouse_down_func != NULL) {
                    window->mouse_down_func(msg->data.mouse.x - window->scrollbar_rect.x, msg->data.mouse.y - window->scrollbar_rect.y - stru_5C3628.y);
                    return true;
                }
            }

            if (mainmenu_ui_window_type != MM_WINDOW_SAVE_GAME
                || (msg->data.mouse.x < stru_5C3648.x
                    || msg->data.mouse.y < stru_5C3648.y
                    || msg->data.mouse.x >= stru_5C3648.x + stru_5C3648.width
                    || msg->data.mouse.y >= stru_5C3648.y + stru_5C3648.height)) {
                sub_549450();
            }
        }

        switch (msg->data.mouse.event) {
        case TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP:
            switch (mainmenu_ui_window_type) {
            case MM_WINDOW_0:
            case MM_WINDOW_1:
                mainmenu_ui_close(false);
                mainmenu_ui_window_type = MM_WINDOW_MAINMENU;
                mainmenu_ui_open();
                return true;
            case MM_WINDOW_OPTIONS:
                if (intgame_pc_lens_check_pt(msg->data.mouse.x, msg->data.mouse.y)) {
                    if (stru_5C36B0[mainmenu_ui_type][0]) {
                        if (!options_ui_load_module()) {
                            sub_5412D0();
                        }
                    } else {
                        gsound_play_sfx(0, 1);
                        mainmenu_ui_close(true);
                    }
                    return true;
                }
                break;
            case MM_WINDOW_LOAD_GAME:
                if (intgame_pc_lens_check_pt(msg->data.mouse.x, msg->data.mouse.y)) {
                    if (dword_64C450) {
                        sub_5412D0();
                    } else {
                        mainmenu_ui_close(true);
                    }
                    return true;
                }
                break;
            case MM_WINDOW_SAVE_GAME:
                if (intgame_pc_lens_check_pt(msg->data.mouse.x, msg->data.mouse.y)) {
                    sub_5412D0();
                    return true;
                }
                break;
            }

            if (window->mouse_up_func != NULL
                && window->content_rect.width > 0
                && msg->data.mouse.x >= window->content_rect.x
                && msg->data.mouse.y - stru_5C3628.y >= window->content_rect.y
                && msg->data.mouse.x < window->content_rect.x + window->content_rect.width
                && msg->data.mouse.y - stru_5C3628.y < window->content_rect.y + window->content_rect.height) {
                gsound_play_sfx(0, 1);
                window->mouse_up_func(msg->data.mouse.x - window->content_rect.x, msg->data.mouse.y - window->content_rect.y - stru_5C3628.y);
                return true;
            }

            return false;
        case TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP:
        case TIG_MESSAGE_MOUSE_MIDDLE_BUTTON_UP:
            switch (mainmenu_ui_window_type) {
            case MM_WINDOW_0:
            case MM_WINDOW_1:
                mainmenu_ui_close(false);
                mainmenu_ui_window_type = MM_WINDOW_MAINMENU;
                mainmenu_ui_open();
                return true;
            }
            return false;
        case TIG_MESSAGE_MOUSE_IDLE:
            if (window->mouse_idle_func != NULL) {
                window->mouse_idle_func(msg->data.mouse.x, msg->data.mouse.y);
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    if (msg->type == TIG_MESSAGE_BUTTON) {
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_PRESSED:
            if (window->button_press_func != NULL && window->button_press_func(msg->data.button.button_handle)) {
                return true;
            }
            return false;
        case TIG_BUTTON_STATE_RELEASED:
            if (window->button_release_func != NULL && window->button_release_func(msg->data.button.button_handle)) {
                return true;
            }

            for (idx = 0; idx < window->num_buttons; idx++) {
                if (msg->data.button.button_handle == window->buttons[idx].button_handle) {
                    if (window->buttons[idx].field_10 > 0) {
                        if (window->execute_func != NULL) {
                            dword_5C3FB8 = window->execute_func(idx);
                            if (dword_5C3FB8 == 0) {
                                return true;
                            }
                        }

                        mainmenu_ui_close(false);
                        mainmenu_ui_window_type = window->buttons[idx].field_10;
                        mainmenu_ui_open();
                        return true;
                    }

                    if (window->buttons[idx].field_10 == 0) {
                        sub_5412D0();
                        if (stru_5C36B0[mainmenu_ui_type][1]
                            || mainmenu_ui_window_type == MM_WINDOW_MAINMENU
                            || mainmenu_ui_window_type == MM_WINDOW_MAINMENU_IN_PLAY) {
                            mainmenu_ui_exit_game();
                        }
                        mainmenu_ui_window_type = MM_WINDOW_0;
                        return true;
                    }

                    if (window->buttons[idx].field_10 == -2) {
                        mainmenu_ui_close(true);
                        if (mainmenu_ui_window_type == MM_WINDOW_0) {
                            sub_5412D0();
                        }
                    }
                    return true;
                }
            }

            return false;
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            if (window->button_hover_func != NULL) {
                window->button_hover_func(msg->data.button.button_handle);
            }

            for (idx = 0; idx < window->num_buttons; idx++) {
                if (msg->data.button.button_handle == window->buttons[idx].button_handle) {
                    break;
                }
            }

            if (idx < window->num_buttons) {
                if (window->num != -1) {
                    mainmenu_ui_refresh_button_text(idx, 0x2);
                }

                v2 = window->buttons[idx].field_2C;
            } else {
                v2 = -1;
            }

            if (v2 == 0) {
                if (msg->data.button.button_handle == stru_5C45D8.button_handle) {
                    v2 = stru_5C45D8.field_2C;
                } else if (msg->data.button.button_handle == stru_5C4838.button_handle) {
                    v2 = stru_5C4838.field_2C;
                } else {
                    return false;
                }
            }

            if (v2 > 0) {
                mes_file_entry.num = 6999;
                if (!mes_search(mainmenu_ui_mainmenu_mes_file, &mes_file_entry)) {
                    tig_debug_printf("MMUI: ERROR: Hover Text for button is Unreachable!\n");
                    return false;
                }

                mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

                description_mes_file_entry.num = v2;
                mes_get_msg(mainmenu_ui_mainmenu_mes_file, &description_mes_file_entry);

                sprintf(str, "%s\n%s", mes_file_entry.str, description_mes_file_entry.str);

                ui_message.type = UI_MSG_TYPE_FEEDBACK;
                ui_message.str = str;
                sub_550750(&ui_message);
                return true;
            }

            return false;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            if (window->button_leave_func != NULL) {
                window->button_leave_func(msg->data.button.button_handle);
            }

            for (idx = 0; idx < window->num_buttons; idx++) {
                if (msg->data.button.button_handle == window->buttons[idx].button_handle) {
                    break;
                }
            }

            if (idx < window->num_buttons) {
                if (window->num != -1) {
                    mainmenu_ui_refresh_button_text(idx, 0);
                }

                v2 = window->buttons[idx].field_2C;
            } else {
                v2 = -1;
            }

            if (v2 == 0) {
                if (msg->data.button.button_handle == stru_5C45D8.button_handle) {
                    v2 = stru_5C45D8.field_2C;
                } else if (msg->data.button.button_handle == stru_5C4838.button_handle) {
                    v2 = stru_5C4838.field_2C;
                } else {
                    return false;
                }
            }

            if (v2 > 0) {
                sub_550720();
                return true;
            }

            return false;
        default:
            return false;
        }
    }

    if (msg->type == TIG_MESSAGE_KEYBOARD) {
        if (!msg->data.keyboard.pressed) {
            switch (mainmenu_ui_window_type) {
            case MM_WINDOW_0:
                return false;
            case MM_WINDOW_1:
                mainmenu_ui_close(false);
                mainmenu_ui_window_type = 2;
                mainmenu_ui_open();
                return true;
            case MM_WINDOW_MAINMENU:
                if (msg->data.keyboard.scancode == SDL_SCANCODE_ESCAPE
                    && dword_64C43C > 1) {
                    gsound_play_sfx(0, 1);
                    sub_5412D0();
                    mainmenu_ui_window_type = 0;
                    mainmenu_ui_exit_game();
                }
                return false;
            case MM_WINDOW_OPTIONS:
                if (msg->data.keyboard.scancode == SDL_SCANCODE_O) {
                    if (!stru_5C36B0[mainmenu_ui_type][1]) {
                        sub_5412D0();
                    }
                    return true;
                }
                return false;
            case MM_WINDOW_LOAD_GAME:
                switch (msg->data.keyboard.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    gsound_play_sfx(0, 1);
                    mainmenu_ui_close(true);
                    if (mainmenu_ui_window_type == MM_WINDOW_0) {
                        sub_5412D0();
                    }
                    return true;
                case SDL_SCANCODE_UP:
                    sub_543060();
                    return true;
                case SDL_SCANCODE_DOWN:
                    sub_5430D0();
                    return true;
                case SDL_SCANCODE_BACKSPACE:
                case SDL_SCANCODE_DELETE:
                    gsound_play_sfx(0, 1);
                    mainmenu_ui_load_game_handle_delete();
                    return true;
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_KP_ENTER:
                    sub_5480C0(2);
                    return true;
                }
                return false;
            case MM_WINDOW_SAVE_GAME:
                switch (msg->data.keyboard.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    gsound_play_sfx(0, 1);
                    mainmenu_ui_close(true);
                    if (mainmenu_ui_window_type == MM_WINDOW_0) {
                        sub_5412D0();
                    }
                    return true;
                case SDL_SCANCODE_UP:
                    sub_544210();
                    return true;
                case SDL_SCANCODE_DOWN:
                    sub_544250();
                    return true;
                case SDL_SCANCODE_BACKSPACE:
                case SDL_SCANCODE_DELETE:
                    gsound_play_sfx(0, 1);
                    mainmenu_ui_save_game_handle_delete();
                    return true;
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_KP_ENTER:
                    sub_5480C0(2);
                    return true;
                }
                return false;
            default:
                switch (msg->data.keyboard.scancode) {
                case SDL_SCANCODE_ESCAPE:
                    gsound_play_sfx(0, 1);
                    mainmenu_ui_close(true);
                    if (mainmenu_ui_window_type == MM_WINDOW_0) {
                        sub_5412D0();
                    }
                    return true;
                }
                return false;
            }
        } else {
            if (sub_549A60()) {
                return false;
            }

            if ((mainmenu_ui_window_type == MM_WINDOW_SHOP || mainmenu_ui_window_type == MM_WINDOW_CHAREDIT)
                && msg->data.keyboard.key == SDLK_RETURN
                && iso_interface_window_get() != ROTWIN_TYPE_QUANTITY) {
                gsound_play_sfx(0, 1);
                sub_5480C0(2);
                return true;
            }

            for (idx = 0; idx < window->num_buttons; idx++) {
                bool hidden;
                tig_button_is_hidden(window->buttons[idx].button_handle, &hidden);
                if (!hidden && SDL_toupper(msg->data.keyboard.key) == window->buttons[idx].field_14) {
                    break;
                }
            }

            if (idx >= window->num_buttons) {
                return false;
            }

            if (window->buttons[idx].art_num == -1) {
                gsound_play_sfx(SND_INTERFACE_MORPHTEXT_CLICK, 1);
            } else {
                gsound_play_sfx(0, 1);
            }

            if (window->buttons[idx].field_10 > 0) {
                if (window->execute_func != NULL) {
                    dword_5C3FB8 = window->execute_func(idx);
                    if (!dword_5C3FB8) {
                        return true;
                    }
                }

                mainmenu_ui_close(false);
                mainmenu_ui_window_type = window->buttons[idx].field_10;
                mainmenu_ui_open();
                return true;
            }

            if (window->buttons[idx].field_10 == 0) {
                sub_5412D0();
                if (stru_5C36B0[mainmenu_ui_type][1]
                    || mainmenu_ui_window_type == MM_WINDOW_MAINMENU
                    || mainmenu_ui_window_type == MM_WINDOW_MAINMENU_IN_PLAY) {
                    mainmenu_ui_exit_game();
                }
                mainmenu_ui_window_type = MM_WINDOW_0;
                return true;
            }

            if (window->buttons[idx].field_10 == -2) {
                mainmenu_ui_close(true);
                if (mainmenu_ui_window_type == MM_WINDOW_0) {
                    sub_5412D0();
                }
                return true;
            }

            if (window->buttons[idx].field_10 == -1) {
                switch (mainmenu_ui_window_type) {
                case MM_WINDOW_MAINMENU_IN_PLAY:
                    switch (idx) {
                    case 3:
                        if (mainmenu_ui_confirm_quit() == TIG_WINDOW_MODAL_DIALOG_CHOICE_OK) {
                            // FIXME: Looks wrong.
                            tig_button_hide(3);

                            return sub_549310(3);
                        }
                        return true;
                    case 4:
                        if (!stru_5C36B0[mainmenu_ui_type][0]) {
                            mainmenu_ui_close(true);

                            if (mainmenu_ui_window_type == MM_WINDOW_0) {
                                sub_5412D0();
                            }
                        }
                        return true;
                    }

                    return true;
                case MM_WINDOW_MAINMENU_IN_PLAY_LOCKED:
                    switch (idx) {
                    case 1:
                        if (mainmenu_ui_confirm_quit() == TIG_WINDOW_MODAL_DIALOG_CHOICE_OK) {
                            // FIXME: Looks wrong.
                            tig_button_hide(1);

                            return sub_549310(1);
                        }
                        return true;
                    case 2:
                        if (!stru_5C36B0[mainmenu_ui_type][0]) {
                            mainmenu_ui_close(true);

                            if (mainmenu_ui_window_type == MM_WINDOW_0) {
                                sub_5412D0();
                            }
                        }
                        return true;
                    }

                    return true;
                }

                return true;
            }

            return true;
        }
    }

    return false;
}

// 0x547E00
void mainmenu_ui_refresh_button_text(int btn, unsigned int flags)
{
    MainMenuWindowInfo* window;
    MainMenuButtonInfo* button;
    bool hidden;
    MesFileEntry mes_file_entry;
    TigRect rect;

    window = main_menu_window_info[mainmenu_ui_window_type];
    button = &(window->buttons[btn]);

    // FIXME: Result is not being checked.
    tig_button_is_hidden(button->button_handle, &hidden);
    if (!hidden) {
        mes_file_entry.num = window->num;
        if ((button->flags & 0x1) != 0) {
            tig_debug_printf("mainmenu_ui_refresh_button_text: ERROR: flags wrong!\n");
            exit(EXIT_FAILURE);
        }

        if (window->num != -1 && (button->flags & 0x4) == 0) {
            mes_file_entry.num = window->num + btn;
            mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);

            rect = button->rect;
            rect.x = button->x - window->field_30;
            rect.y = button->y - window->field_34;
            mainmenu_ui_refresh_text(dword_5C3624,
                mes_file_entry.str,
                &rect,
                window->refresh_text_flags | flags);
        }
    }
}

// 0x547EF0
void sub_547EF0()
{
    TigArtAnimData art_anim_data;
    DateTime datetime;
    TimeEvent timeevent;
    int index;

    for (index = 0; index < 2; index++) {
        if (tig_art_anim_data(stru_64B870[index].art_id, &art_anim_data) == TIG_OK
            && art_anim_data.num_frames > 1) {
            stru_64B870[index].max_frame = art_anim_data.num_frames - 1;
            stru_64B870[index].fps = 1000 / art_anim_data.fps;
            stru_64B870[index].x = main_menu_window_info[mainmenu_ui_window_type]->field_3C[index].x;
            stru_64B870[index].y = main_menu_window_info[mainmenu_ui_window_type]->field_3C[index].y;

            timeevent.type = TIMEEVENT_TYPE_MAINMENU;
            timeevent.params[0].integer_value = index;
            sub_45A950(&datetime, stru_64B870[index].fps);
            timeevent_add_delay(&timeevent, &datetime);
        }
    }
}

// 0x547F90
bool mainmenu_ui_process_callback(TimeEvent* timeevent)
{
    int index;
    tig_art_id_t next_art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    DateTime datetime;
    TimeEvent next_timeevent;

    index = timeevent->params[0].integer_value;
    if (stru_64B870[index].art_id == TIG_ART_ID_INVALID) {
        return true;
    }

    next_art_id = tig_art_id_frame_inc(stru_64B870[index].art_id);
    if (tig_art_id_frame_get(next_art_id) >= stru_64B870[index].max_frame) {
        next_art_id = tig_art_id_frame_set(next_art_id, 0);
    }

    if (tig_art_frame_data(next_art_id, &art_frame_data) == TIG_OK) {
        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.x = stru_64B870[index].x;
        dst_rect.y = stru_64B870[index].y;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.height;

        art_blit_info.flags = 0;
        art_blit_info.art_id = next_art_id;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(dword_5C3624, &art_blit_info);
    } else {
        tig_debug_printf("main_menu_ui_process_callback: ERROR: tig_art_frame_data failed!\n");
    }

    next_timeevent.type = TIMEEVENT_TYPE_MAINMENU;
    next_timeevent.params[0].integer_value = index;
    sub_45A950(&datetime, stru_64B870[index].fps);
    timeevent_add_delay(&next_timeevent, &datetime);

    return true;
}

// 0x5480C0
void sub_5480C0(int a1)
{
    MainMenuWindowInfo* window;

    window = main_menu_window_info[mainmenu_ui_window_type];
    switch (a1) {
    case 0:
        sub_548FF0(window->top_index - 1);
        return;
    case 1:
        sub_548FF0(window->top_index + 1);
        return;
    case 2:
        if (window->execute_func != NULL) {
            dword_5C3FB8 = window->execute_func(-1);
            if (!dword_5C3FB8) {
                return;
            }
        }
        if (mainmenu_ui_active) {
            if (mainmenu_ui_window_type == MM_WINDOW_SHOP) {
                sub_5412D0();
            } else {
                mainmenu_ui_close(false);
                mainmenu_ui_window_type++;
                mainmenu_ui_open();
            }
        }
        return;
    case 3:
        if (mainmenu_ui_window_type != MM_WINDOW_OPTIONS || options_ui_load_module()) {
            mainmenu_ui_close(true);
        }
        return;
    case 4:
        window->flags &= ~0x1;
        if ((window->flags & 0x2) != 0) {
            window->selected_index = 0;
        }
        if ((window->flags & 0x4) != 0) {
            if (window->selected_index > 0) {
                window->selected_index--;
            }
        }
        if (window->refresh_func != NULL) {
            window->refresh_func(NULL);
        }
        return;
    case 5:
        window->flags |= 0x1;
        if ((window->flags & 0x2) != 0) {
            window->selected_index = -1;
        }
        if ((window->flags & 0x4) != 0) {
            if (window->selected_index < window->cnt - 1) {
                window->selected_index++;
            }
        }
        if (window->refresh_func != NULL) {
            window->refresh_func(NULL);
        }
        return;
    }
}

// 0x548F10
void mmUIWinRefreshScrollBar()
{
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;
    TigArtFrameData art_frame_data;
    tig_art_id_t art_id;
    MainMenuWindowInfo* curr_window_info;

    curr_window_info = main_menu_window_info[mainmenu_ui_window_type];
    if (curr_window_info->scrollbar_rect.width > 0) {
        tig_art_interface_id_create(316, 0, 0, 0, &art_id);
        if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
            tig_debug_printf("mmUIWinRefreshScrollBar: ERROR: tig_art_frame_data failed!\n");
            exit(EXIT_FAILURE);
        }

        src_rect.x = 0;
        src_rect.y = curr_window_info->top_index + art_frame_data.height / 2 - curr_window_info->scrollbar_rect.height / 2;
        src_rect.width = art_frame_data.width;
        src_rect.height = curr_window_info->scrollbar_rect.height;

        dst_rect.x = curr_window_info->scrollbar_rect.x;
        dst_rect.y = curr_window_info->scrollbar_rect.y;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = curr_window_info->scrollbar_rect.height;

        blit_info.flags = 0;
        blit_info.art_id = art_id;
        blit_info.src_rect = &src_rect;
        blit_info.dst_rect = &dst_rect;

        tig_window_blit_art(dword_5C3624, &blit_info);
    }
}

// 0x548FF0
void sub_548FF0(int a1)
{
    MainMenuWindowInfo* curr_window_info;

    curr_window_info = main_menu_window_info[mainmenu_ui_window_type];
    if (a1 < 0) {
        a1 = 0;
    } else if (a1 > curr_window_info->max_top_index) {
        a1 = curr_window_info->max_top_index;
    }

    if (curr_window_info->top_index != a1) {
        curr_window_info->top_index = a1;
        mmUIWinRefreshScrollBar();
        if (curr_window_info->refresh_func != NULL) {
            curr_window_info->refresh_func(NULL);
        }
    }
}

// 0x549310
bool sub_549310(tig_button_handle_t button_handle)
{
    if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_show(button_handle);
    }

    if (mainmenu_ui_active) {
        mainmenu_ui_close(false);
        mainmenu_ui_window_type = MM_WINDOW_MAINMENU;
        mainmenu_ui_num_windows = 0;
        mainmenu_ui_type = !dword_5C4000 ? MM_TYPE_1 : MM_TYPE_DEFAULT;
        mainmenu_ui_open();
    } else {
        gameuilib_wants_mainmenu_set();
    }

    if (!gamelib_mod_load(gamelib_default_module_name_get())
        || !gameuilib_mod_load()) {
        tig_debug_printf("MainMenu: Unable to load default module %s.\n",
            gamelib_default_module_name_get());
        exit(EXIT_SUCCESS); // FIXME: Should be EXIT_FAILURE.
    }

    mainmenu_ui_reset();

    return true;
}

// 0x5493C0
void sub_5493C0(char* buffer, int size)
{
    MainMenuWindowInfo* curr_window_info;

    if (!dword_64C428) {
        if (byte_64C2F8[0] != '\0') {
            strcpy(byte_64C0F0, buffer);
        }

        stru_5C4008.size = size;
        stru_5C4008.flags = mainmenu_ui_window_type == MM_WINDOW_SAVE_GAME
            ? TEXTEDIT_PATH_SAFE
            : 0;
        stru_5C4008.buffer = buffer;
        textedit_ui_focus(&stru_5C4008);
        dword_64C428 = true;

        curr_window_info = main_menu_window_info[mainmenu_ui_window_type];
        curr_window_info->refresh_func(&(curr_window_info->content_rect));

        sub_549A40();
    }
}

// 0x549450
void sub_549450()
{
    MainMenuWindowInfo* curr_window_info;

    if (dword_64C428) {
        if (stru_5C4008.buffer[0] == '\0') {
            strcpy(stru_5C4008.buffer, byte_64C0F0);
        }
        textedit_ui_unfocus(&stru_5C4008);
        dword_64C428 = false;

        curr_window_info = main_menu_window_info[mainmenu_ui_window_type];
        curr_window_info->refresh_func(&(curr_window_info->content_rect));

        sub_549A50();
    }
}

// 0x5494C0
void sub_5494C0(TextEdit* textedit)
{
    MesFileEntry mes_file_entry;

    if (textedit->buffer[0] == '\0' && mainmenu_ui_window_type != MM_WINDOW_SAVE_GAME) {
        mes_file_entry.num = 500; // "Choose Name"
        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
        strncpy(byte_64C2F8, mes_file_entry.str, 23);
    }

    sub_549450();

    if (mainmenu_ui_window_type == MM_WINDOW_SAVE_GAME) {
        mainmenu_ui_save_game_execute(-1);
    }
}

// 0x549520
char* sub_549520()
{
    return dword_64C428 != 0 ? stru_5C4008.buffer : NULL;
}

// 0x549540
void sub_549540(TextEdit* textedit)
{
    MainMenuWindowInfo* curr_window_info;

    (void)textedit;

    curr_window_info = main_menu_window_info[mainmenu_ui_window_type];
    curr_window_info->refresh_func(&(curr_window_info->content_rect));

    if (mainmenu_ui_window_type == MM_WINDOW_SAVE_GAME) {
        scrollbar_ui_control_redraw(stru_64C220);
    }
}

// 0x549580
void mainmenu_ui_exit_game()
{
    tig_debug_printf("mainmenu_ui_exit_game: Exiting Game!\n");
    gameuilib_mod_unload();
    gamelib_mod_unload();
    gameuilib_exit();
    gamelib_exit();
    tig_exit();
    tig_memory_print_stats(TIG_MEMORY_STATS_PRINT_GROUPED_BLOCKS);
    exit(EXIT_SUCCESS);
}

// 0x5495F0
void mainmenu_ui_progressbar_init(int max_value)
{
    mainmenu_ui_progressbar_max_value = max_value;

    if (mainmenu_ui_active) {
        if (main_menu_window_info[mainmenu_ui_window_type]->refresh_func != NULL) {
            main_menu_window_info[mainmenu_ui_window_type]->refresh_func(&stru_5C4538);
        }
    }
}

// 0x549620
void mainmenu_ui_progressbar_update(int value)
{
    mainmenu_ui_progressbar_value = value;

    if (mainmenu_ui_active) {
        if (main_menu_window_info[mainmenu_ui_window_type]->refresh_func != NULL) {
            main_menu_window_info[mainmenu_ui_window_type]->refresh_func(&stru_5C4538);
        }
    }
}

// 0x5496C0
MainMenuWindowInfo* sub_5496C0(int index)
{
    return main_menu_window_info[index];
}

// 0x5496D0
MainMenuWindowType mainmenu_ui_window_type_get()
{
    return mainmenu_ui_window_type;
}

// 0x5496E0
void mainmenu_ui_feedback_saving()
{
    mainmenu_ui_feedback(5060); // "Saving..."
}

// 0x5496F0
void mainmenu_ui_feedback(int num)
{
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    mes_file_entry.num = num;
    if (mes_search(mainmenu_ui_mainmenu_mes_file, &mes_file_entry)) {
        mes_get_msg(mainmenu_ui_mainmenu_mes_file, &mes_file_entry);
        ui_message.type = UI_MSG_TYPE_FEEDBACK;
        ui_message.str = mes_file_entry.str;
        sub_550750(&ui_message);
        tig_window_display();
    }
}

// 0x549750
void mainmenu_ui_feedback_saving_completed()
{
    mainmenu_ui_feedback(5062); // "Save completed."
}

// 0x549760
void mainmenu_ui_feedback_cannot_save_in_tb()
{
    mainmenu_ui_feedback(5065); // "Cannot save during turn-based combat when it isn't your turn."
}

// 0x549770
void mainmenu_ui_feedback_loading()
{
    mainmenu_ui_feedback(5061); // "Loading..."
}

// 0x549780
void mainmenu_ui_feedback_loading_completed()
{
    mainmenu_ui_feedback(5063); // "Load completed."
}

// 0x549820
tig_window_handle_t sub_549820()
{
    return dword_5C3624;
}

// 0x549830
void mainmenu_ui_window_type_set(MainMenuWindowType window_type)
{
    mainmenu_ui_window_type = window_type;
}

// 0x549840
mes_file_handle_t mainmenu_ui_mes_file()
{
    return mainmenu_ui_mainmenu_mes_file;
}

// 0x549850
void mainmenu_fonts_init()
{
    int fnt;
    int clr;
    TigFont font_info;

    font_info.flags = 0;
    font_info.str = NULL;

    for (fnt = 0; fnt < MM_FONT_COUNT; fnt++) {
        tig_art_interface_id_create(mainmenu_font_nums[fnt], 0, 0, 0, &(font_info.art_id));

        for (clr = 0; clr < MM_COLOR_COUNT; clr++) {
            font_info.color = tig_color_make(mainmenu_font_colors[clr][0],
                mainmenu_font_colors[clr][1],
                mainmenu_font_colors[clr][2]);
            tig_font_create(&font_info, &(mainmenu_ui_fonts_tbl[fnt][clr]));
        }
    }
}

// 0x549910
void mainmenu_fonts_exit()
{
    int fnt;
    int clr;

    for (fnt = 0; fnt < MM_FONT_COUNT; fnt++) {
        for (clr = 0; clr < MM_COLOR_COUNT; clr++) {
            tig_font_destroy(mainmenu_ui_fonts_tbl[fnt][clr]);
        }
    }
}

// 0x549940
tig_font_handle_t mainmenu_ui_font(MainMenuFont font, MainMenuColor color)
{
    return mainmenu_ui_fonts_tbl[font][color];
}

// 0x549960
void sub_549960()
{
    int index;

    for (index = 0; index < main_menu_window_info[mainmenu_ui_window_type]->num_buttons; index++) {
        mainmenu_ui_refresh_button_text(index, 0);
    }
}

// 0x549990
void sub_549990(int* a1, int num)
{
    memcpy(&(mainmenu_ui_window_stack[1]), a1, sizeof(*a1) * num);
    mainmenu_ui_num_windows = num + 1;
}

// 0x549A40
void sub_549A40()
{
    dword_64C468++;
}

// 0x549A50
void sub_549A50()
{
    dword_64C468--;
}

// 0x549A60
int sub_549A60()
{
    return dword_64C468;
}

// 0x549A70
void sub_549A70()
{
    dword_5C3620 = false;
}

// 0x549A80
void sub_549A80()
{
    int64_t obj;

    if (!dword_5C3620) {
        obj = objp_perm_lookup(obj_get_id(sub_4685A0(BP_VICTORIA_WARRINGTON)));
        if (obj != OBJ_HANDLE_NULL
            && tig_art_exists(obj_field_int32_get(obj, OBJ_F_CURRENT_AID)) == TIG_OK) {
            dword_5C3620 = false;
            mainmenu_ui_pregen_char_cnt = 13;
            mainmenu_ui_new_char_window_info.num_buttons = 10;
        } else {
            dword_5C3620 = true;
        }
    }
}
