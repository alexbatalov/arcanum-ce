#include "ui/combat_ui.h"

#include "game/anim.h"
#include "game/combat.h"
#include "game/hrp.h"
#include "game/obj.h"
#include "game/player.h"
#include "ui/anim_ui.h"
#include "ui/intgame.h"
#include "ui/textedit_ui.h"
#include "ui/wmap_ui.h"

typedef enum ApColor {
    AP_COLOR_GREEN,
    AP_COLOR_ORANGE,
    AP_COLOR_RED,
    AP_COLOR_COUNT,
} ApColor;

#define THREE 3
#define TWENTY 20

static void sub_56ECE0(int a1);
static void sub_56ED00();
static void combat_ui_create();
static void sub_56EF40();
static void combat_ui_destroy();
static void sub_56EFA0(int a1);
static void sub_56F2F0(tig_window_handle_t window_handle, TigRect* rect, tig_color_t color);
static void sub_56F430(int a1);
static void sub_56F840();
static void sub_56F990(int64_t obj);
static bool combat_ui_message_filter(TigMessage* msg);

// 0x5CAA20
static TigRect combat_ui_compact_ap_bar_frame = { 617, 577, 124, 14 };

// 0x5CAA30
static tig_art_id_t combat_ui_ap_bar_loading_indicator_art_id = TIG_ART_ID_INVALID;

// 0x5CAA38
static UiButtonInfo combat_ui_end_turn_button_info = { 320, 9, 297, TIG_BUTTON_HANDLE_INVALID };

// 0x5CAA48
static int dword_5CAA48[TWENTY] = {
    22,
    35,
    48,
    61,
    74,
    93,
    106,
    119,
    132,
    145,
    164,
    177,
    190,
    203,
    216,
    235,
    248,
    261,
    274,
    287,
};

// 0x5CAA18
static tig_window_handle_t combat_ui_ap_bar_window_handle = TIG_WINDOW_HANDLE_INVALID;

// 0x5CAA1C
static tig_window_handle_t combat_ui_compact_ap_bar_window_handle = TIG_WINDOW_HANDLE_INVALID;

// 0x5CAA98
static int combat_ui_ap_icon_nums[AP_COLOR_COUNT] = {
    /*  GREEN */ 295, // ap_green.art
    /* ORANGE */ 296, // ap_orange.art
    /*    RED */ 315, // ap_red.art
};

// 0x680EA8
static tig_art_id_t combat_ui_ap_icon_art_ids[AP_COLOR_COUNT];

// 0x680EB4
static int dword_680EB4;

// 0x680EB8
static bool combat_ui_initialized;

// 0x680EBC
static bool combat_ui_created;

// 0x680EC0
static TigRect combat_ui_ap_bar_loading_indicator_frame;

// 0x56EA90
bool combat_ui_init(GameInitInfo* init_info)
{
    CombatCallbacks callbacks;
    int index;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigWindowData window_data;

    callbacks.field_0 = sub_56ECE0;
    callbacks.field_4 = sub_56ED00;
    callbacks.field_8 = sub_56EF40;
    callbacks.field_C = sub_56F430;
    callbacks.field_10 = sub_56F990;

    if (!combat_set_callbacks(&callbacks)) {
        return false;
    }

    for (index = 0; index < THREE; index++) {
        if (tig_art_interface_id_create(combat_ui_ap_icon_nums[index], 0, 0, 0, &(combat_ui_ap_icon_art_ids[index])) != TIG_OK) {
            return false;
        }
    }

    tig_art_interface_id_create(601, 0, 0, 0, &art_id);

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    if (tig_window_data(init_info->iso_window_handle, &window_data) != TIG_OK) {
        return false;
    }

    combat_ui_ap_bar_loading_indicator_frame.x = window_data.rect.x + window_data.rect.width - art_frame_data.width - 2;
    combat_ui_ap_bar_loading_indicator_frame.y = window_data.rect.y + art_frame_data.height + 2;

    tig_art_interface_id_create(601, 0, 0, 0, &combat_ui_ap_bar_loading_indicator_art_id);

    if (tig_art_frame_data(combat_ui_ap_bar_loading_indicator_art_id, &art_frame_data) != TIG_OK) {
        return false;
    }

    combat_ui_ap_bar_loading_indicator_frame.width = art_frame_data.width;
    combat_ui_ap_bar_loading_indicator_frame.height = art_frame_data.height;

    combat_ui_initialized = true;

    return true;
}

// 0x56EBE0
void combat_ui_exit()
{
    combat_ui_initialized = false;
}

// 0x56EBF0
void combat_ui_reset()
{
    sub_56EF40();
}

// 0x56EC00
void combat_ui_resize(GameResizeInfo* resize_info)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;

    tig_art_interface_id_create(601, 0, 0, 0, &art_id);

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        tig_debug_printf("combat_ui_resize: ERROR: tig_art_frame_data failed!\n");
        return;
    }

    combat_ui_ap_bar_loading_indicator_frame.x = resize_info->window_rect.x + resize_info->window_rect.width - art_frame_data.width - 2;
    combat_ui_ap_bar_loading_indicator_frame.y = resize_info->window_rect.y + art_frame_data.height + 4;

    if (combat_ui_created) {
        if (intgame_is_compact_interface()) {
            tig_window_hide(combat_ui_ap_bar_window_handle);
        } else {
            tig_window_show(combat_ui_ap_bar_window_handle);
        }

        if (combat_ui_created) {
            if (intgame_is_compact_interface()) {
                tig_window_show(combat_ui_compact_ap_bar_window_handle);
            } else {
                tig_window_hide(combat_ui_compact_ap_bar_window_handle);
            }
        }
    }
}

// 0x56ECE0
void sub_56ECE0(int a1)
{
    intgame_refresh_cursor();
    if (a1) {
        wmap_ui_close();
    }
}

// 0x56ED00
void sub_56ED00()
{
    if (combat_ui_created) {
        sub_56EF40();
    }
    combat_ui_create();
    sub_56F430(0);
}

// 0x56ED20
void combat_ui_create()
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigRect rect;
    TigWindowData window_data;
    TigArtBlitInfo blit_info;

    if (tig_art_interface_id_create(294, 0, 0, 0, &art_id) != TIG_OK) {
        return;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    if (tig_art_anim_data(art_id, &art_anim_data) != TIG_OK) {
        return;
    }

    rect.x = 222;
    rect.y = 412;
    rect.width = art_frame_data.width;
    rect.height = art_frame_data.height;

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect = rect;
    window_data.background_color = art_anim_data.color_key;
    window_data.color_key = art_anim_data.color_key;
    window_data.message_filter = combat_ui_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_BOTTOM);

    if (tig_window_create(&window_data, &combat_ui_ap_bar_window_handle) != TIG_OK) {
        return;
    }

    rect.x = 0;
    rect.y = 0;

    blit_info.flags = 0;
    blit_info.art_id = art_id;
    blit_info.src_rect = &rect;
    blit_info.dst_rect = &rect;
    tig_window_blit_art(combat_ui_ap_bar_window_handle, &blit_info);

    if (!intgame_button_create_ex(combat_ui_ap_bar_window_handle, &rect, &combat_ui_end_turn_button_info, TIG_BUTTON_MOMENTARY)) {
        return;
    }

    combat_ui_created = true;
    dword_680EB4 = 0;

    combat_ui_refresh();
    sub_56F430(0);

    if (intgame_is_compact_interface()) {
        tig_window_hide(combat_ui_ap_bar_window_handle);
    }

    window_data.flags = TIG_WINDOW_TRANSPARENT | TIG_WINDOW_MESSAGE_FILTER;
    window_data.rect = combat_ui_compact_ap_bar_frame;
    window_data.background_color = art_anim_data.color_key;
    window_data.color_key = tig_color_make(0, 0, 255);
    window_data.message_filter = combat_ui_message_filter;
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_BOTTOM);

    if (tig_window_create(&window_data, &combat_ui_compact_ap_bar_window_handle) != TIG_OK) {
        return;
    }

    sub_56EFA0(0);

    if (!intgame_is_compact_interface()) {
        tig_window_hide(combat_ui_compact_ap_bar_window_handle);
    }
}

// 0x56EF40
void sub_56EF40()
{
    combat_ui_destroy();
}

// 0x56EF50
void combat_ui_destroy()
{
    if (!combat_ui_created) {
        return;
    }

    if (tig_window_destroy(combat_ui_ap_bar_window_handle) == TIG_OK) {
        combat_ui_ap_bar_window_handle = TIG_WINDOW_HANDLE_INVALID;
    }

    if (combat_ui_compact_ap_bar_window_handle != TIG_WINDOW_HANDLE_INVALID) {
        tig_window_destroy(combat_ui_compact_ap_bar_window_handle);
        combat_ui_compact_ap_bar_window_handle = TIG_WINDOW_HANDLE_INVALID;
    }

    combat_ui_created = false;
}

// 0x56EFA0
void sub_56EFA0(int a1)
{
    int v1 = 0;
    int action_points;
    int v4;
    TigRect rect;

    if (combat_ui_compact_ap_bar_window_handle == TIG_WINDOW_HANDLE_INVALID) {
        return;
    }

    if (!player_is_local_pc_obj(combat_turn_based_whos_turn_get())) {
        return;
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = combat_ui_compact_ap_bar_frame.width;
    rect.height = combat_ui_compact_ap_bar_frame.height;
    tig_window_fill(combat_ui_compact_ap_bar_window_handle,
        &rect,
        tig_color_make(0, 0, 0));

    rect.x += 1;
    rect.y += 1;
    rect.width -= 2;
    rect.height -= 2;
    tig_window_box(combat_ui_compact_ap_bar_window_handle,
        &rect,
        tig_color_make(0, 0, 255));

    rect.x = 0;
    rect.y = 0;
    rect.width = combat_ui_compact_ap_bar_frame.width;
    rect.height = combat_ui_compact_ap_bar_frame.height;

    action_points = combat_required_action_points_get();
    v4 = a1 - action_points;
    if (v4 >= 25) {
        v1 = 0;
        action_points = 0;
        v4 = 25;
    } else if (action_points > a1) {
        v1 = action_points - a1;
        action_points = a1;
        v4 = 0;
    }

    if (action_points + v4 > 25) {
        action_points = 25 - v4;
    }

    if (v1 + action_points + v4 > 25) {
        v1 = 25 - action_points - v4;
    }

    rect.x = 2;
    rect.width = 120 * v4 / 25;
    if (rect.width > 0) {
        sub_56F2F0(combat_ui_compact_ap_bar_window_handle,
            &rect,
            tig_color_make(0, 255, 0));
    }

    rect.x += rect.width;
    rect.width = 120 * action_points / 25;
    if (rect.width > 0) {
        sub_56F2F0(combat_ui_compact_ap_bar_window_handle,
            &rect,
            tig_color_make(255, 128, 0));
    }

    rect.x += rect.width;
    rect.width = 120 * v1 / 25;
    if (rect.width > 0) {
        sub_56F2F0(combat_ui_compact_ap_bar_window_handle,
            &rect,
            tig_color_make(255, 0, 0));
    }
}

// 0x56F2F0
void sub_56F2F0(tig_window_handle_t window_handle, TigRect* rect, tig_color_t color)
{
    bool odd = false;
    int cnt;
    TigRect line_rect;
    tig_color_t color1;
    tig_color_t color2;

    line_rect.x = rect->x;
    line_rect.y = 2;
    line_rect.width = 1;
    line_rect.height = rect->height - 4;

    cnt = rect->width / 2;

    if ((rect->width & 1) != 0) {
        odd = true;
    }

    if ((rect->x & 1) != 0) {
        color1 = color;
        color2 = tig_color_make(0, 0, 0);
    } else {
        color1 = tig_color_make(0, 0, 0);
        color2 = color;
    }

    line_rect.x = rect->x;

    while (cnt > 0) {
        tig_window_fill(window_handle, &line_rect, color1);
        line_rect.x++;
        tig_window_fill(window_handle, &line_rect, color2);
        line_rect.x++;
        cnt--;
    }

    if (odd) {
        tig_window_fill(window_handle, &line_rect, color1);
    }
}

// 0x56F430
void sub_56F430(int a1)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int action_points;
    int green = 0;
    int orange = 0;
    int red = 0;
    int knob = 0;

    if (!combat_ui_created) {
        return;
    }

    sub_56EFA0(a1);

    if (tig_art_interface_id_create(294, 0, 0, 0, &art_id) == TIG_OK
        && tig_art_frame_data(art_id, &art_frame_data) == TIG_OK
        && tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        art_blit_info.flags = 0;
        art_blit_info.art_id = art_id;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &src_rect;
        tig_window_blit_art(combat_ui_ap_bar_window_handle, &art_blit_info);
    }

    if (a1 != 0) {
        action_points = combat_required_action_points_get();
        orange = action_points;
        green = a1 - action_points;
        if (green >= TWENTY) {
            green = TWENTY;
            orange = 0;
            red = 0;
        } else if (action_points > a1) {
            orange = a1;
            red = action_points - a1;
            green = 0;
        }

        if (orange + green > TWENTY) {
            orange = TWENTY - green;
        }

        if (red + orange + green > TWENTY) {
            red = TWENTY - orange - green;
        }
    }

    if (tig_art_frame_data(art_id, &art_frame_data) == TIG_OK) {
        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.x = 0;
        dst_rect.y = 10;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.height;

        art_blit_info.flags = 0;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;

        art_blit_info.art_id = combat_ui_ap_icon_art_ids[AP_COLOR_GREEN];
        while (green > 0) {
            dst_rect.x = dword_5CAA48[knob];
            tig_window_blit_art(combat_ui_ap_bar_window_handle, &art_blit_info);
            green--;
            knob++;
        }

        art_blit_info.art_id = combat_ui_ap_icon_art_ids[AP_COLOR_ORANGE];
        while (orange > 0) {
            dst_rect.x = dword_5CAA48[knob];
            tig_window_blit_art(combat_ui_ap_bar_window_handle, &art_blit_info);
            orange--;
            knob++;
        }

        art_blit_info.art_id = combat_ui_ap_icon_art_ids[AP_COLOR_RED];
        while (red > 0) {
            dst_rect.x = dword_5CAA48[knob];
            tig_window_blit_art(combat_ui_ap_bar_window_handle, &art_blit_info);
            red--;
            knob++;
        }
    }
}

// 0x56F660
void combat_ui_refresh()
{
    int64_t obj;
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtAnimData art_anim_data;
    TigRect rect;
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;

    if (!combat_ui_created) {
        return;
    }

    sub_56F840();

    obj = combat_turn_based_whos_turn_get();
    if (!player_is_local_pc_obj(obj)) {
        if (dword_680EB4 >= TWENTY) {
            dword_680EB4 = 0;
        }

        if (tig_art_interface_id_create(294, 0, 0, 0, &art_id) == TIG_OK
            && tig_art_frame_data(art_id, &art_frame_data) == TIG_OK
            && tig_art_anim_data(art_id, &art_anim_data) == TIG_OK) {
            rect.x = 0;
            rect.y = 0;
            rect.width = art_frame_data.width;
            rect.height = art_frame_data.height;

            blit_info.flags = 0;
            blit_info.art_id = art_id;
            blit_info.src_rect = &rect;
            blit_info.dst_rect = &rect;
            tig_window_blit_art(combat_ui_ap_bar_window_handle, &blit_info);
        }

        if (tig_art_frame_data(art_id, &art_frame_data)) {
            src_rect.x = 0;
            src_rect.y = 0;
            src_rect.width = art_frame_data.width;
            src_rect.height = art_frame_data.height;

            dst_rect.x = dword_5CAA48[dword_680EB4];
            dst_rect.y = 10;
            dst_rect.width = art_frame_data.width;
            dst_rect.height = art_frame_data.height;

            blit_info.flags = 0;
            blit_info.art_id = combat_ui_ap_icon_art_ids[AP_COLOR_GREEN];
            blit_info.src_rect = &src_rect;
            blit_info.dst_rect = &dst_rect;
            tig_window_blit_art(combat_ui_ap_bar_window_handle, &blit_info);

            dst_rect.x = dword_5CAA48[dword_680EB4 - 7 < 0 ? dword_680EB4 + TWENTY - 7 : dword_680EB4 - 7];
            tig_window_blit_art(combat_ui_ap_bar_window_handle, &blit_info);

            dst_rect.x = dword_5CAA48[dword_680EB4 + 7 >= TWENTY ? dword_680EB4 - TWENTY + 7 : dword_680EB4 + 7];
            tig_window_blit_art(combat_ui_ap_bar_window_handle, &blit_info);
        }

        dword_680EB4++;
    }

    if (combat_ui_created) {
        anim_ui_event_add_delay(ANIM_UI_EVENT_TYPE_REFRESH_COMBAT_UI, -1, 100);
    }
}

// 0x56F840
void sub_56F840()
{
    int64_t obj;
    TigRect rect;

    if (combat_ui_compact_ap_bar_window_handle == TIG_WINDOW_HANDLE_INVALID) {
        return;
    }

    obj = combat_turn_based_whos_turn_get();
    if (player_is_local_pc_obj(obj)) {
        return;
    }

    rect.x = 2;
    rect.y = 2;
    rect.width = combat_ui_compact_ap_bar_frame.width - 4;
    rect.height = combat_ui_compact_ap_bar_frame.height - 4;
    tig_window_fill(combat_ui_compact_ap_bar_window_handle, &rect, tig_color_make(0, 0, 0));

    rect.x = 120 * dword_680EB4 / 25;
    rect.width = 4;
    rect.height += 4;
    sub_56F2F0(combat_ui_compact_ap_bar_window_handle, &rect, tig_color_make(0, 255, 0));
}

// 0x56F990
void sub_56F990(int64_t obj)
{
    if (obj != OBJ_HANDLE_NULL) {
        player_get_local_pc_obj();
    }
}

// 0x56F9B0
bool combat_ui_message_filter(TigMessage* msg)
{
    int64_t obj;
    AnimID curr_anim_id;
    AnimID fidget_anim_id;

    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        if (msg->data.button.state == TIG_BUTTON_STATE_RELEASED
            && msg->data.button.button_handle == combat_ui_end_turn_button_info.button_handle) {
            obj = combat_turn_based_whos_turn_get();
            if (obj != OBJ_HANDLE_NULL
                && player_is_local_pc_obj(obj)
                && (!sub_423300(obj, &curr_anim_id)
                    || (anim_is_current_goal_type(obj, AG_ANIM_FIDGET, &fidget_anim_id)
                        && anim_id_is_equal(&curr_anim_id, &fidget_anim_id)))) {
                combat_turn_based_next_subturn();
            }
            return true;
        }
        break;
    case TIG_MESSAGE_KEYBOARD:
        if (!textedit_ui_is_focused()
            && !msg->data.keyboard.pressed
            && msg->data.keyboard.scancode == SDL_SCANCODE_E) {
            obj = combat_turn_based_whos_turn_get();
            if (obj != OBJ_HANDLE_NULL
                && player_is_local_pc_obj(obj)
                && (!sub_423300(obj, &curr_anim_id)
                    || (anim_is_current_goal_type(obj, AG_ANIM_FIDGET, &fidget_anim_id)
                        && anim_id_is_equal(&curr_anim_id, &fidget_anim_id)))) {
                combat_turn_based_next_subturn();
            }
            return true;
        }
        break;
    }

    return false;
}
