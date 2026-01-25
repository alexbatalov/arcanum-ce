#include "ui/follower_ui.h"

#include <stdio.h>

#include "game/ai.h"
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

#define FOLLOWER_UI_SLOTS 6

typedef enum FollowerUiButton {
    FOLLOWER_UI_BUTTON_TOGGLE = FOLLOWER_UI_SLOTS,
    FOLLOWER_UI_BUTTON_SCROLL_UP,
    FOLLOWER_UI_BUTTON_SCROLL_DOWN,
    FOLLOWER_UI_BUTTON_COUNT,
} FollowerUiButton;

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

// 0x5CA360
static TigRect follower_ui_button_rects[FOLLOWER_UI_BUTTON_COUNT] = {
    { 11, 50, 40, 49 },
    { 11, 112, 40, 49 },
    { 11, 174, 40, 49 },
    { 11, 236, 40, 49 },
    { 11, 298, 40, 49 },
    { 11, 360, 40, 49 },
    { 0, 428, 67, 13 },
    { 0, 415, 33, 13 },
    { 33, 415, 34, 13 },
};

// 0x5CA3F0
static TigRect follower_ui_special_button_rects_normal_mode[FOLLOWER_UI_BUTTON_COUNT - FOLLOWER_UI_SLOTS] = {
    { 0, 428, 67, 13 },
    { 0, 415, 33, 13 },
    { 33, 415, 34, 13 },
};

// 0x5CA420
static TigRect follower_ui_special_button_rects_compact_mode[FOLLOWER_UI_BUTTON_COUNT - FOLLOWER_UI_SLOTS] = {
    { 0, 587, 67, 13 },
    { 0, 574, 33, 13 },
    { 33, 574, 34, 13 },
};

// 0x5CA450
static TigRect follower_ui_drop_down_menu_entry_rect = { 10, 3, 118, 15 };

// 0x67BB38
static tig_button_handle_t follower_ui_buttons[FOLLOWER_UI_BUTTON_COUNT];

// 0x67BB5C
static tig_font_handle_t follower_ui_drop_down_menu_item_highlighted_color;

// 0x67BB60
static tig_window_handle_t follower_ui_windows[FOLLOWER_UI_BUTTON_COUNT];

// 0x67BB88
static TigRect follower_ui_drop_down_menu_rects[8];

// 0x67BC08
static FollowerInfo* follower_ui_followers;

// 0x67BC10
static int follower_ui_top_index;

// 0x67BC14
static int follower_ui_followers_capacity;

// 0x67BC18
static tig_font_handle_t follower_ui_drop_down_menu_item_disabled_color;

// 0x67BC20
static int64_t follower_ui_commander_obj;

// 0x67BC28
static tig_font_handle_t follower_ui_drop_down_menu_item_normal_color;

// 0x67BC2C
static mes_file_handle_t follower_ui_mes_file;

// 0x67BC30
static tig_button_handle_t follower_ui_drop_menu_item_buttons[FOLLOWER_UI_COMMAND_COUNT];

// 0x67BC50
static int64_t follower_ui_subordinate_obj;

// 0x67BC58
static int follower_ui_followers_count;

// 0x67BC5C
static bool follower_ui_initialized;

// NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
// compatibility.
//
// 0x67BC60
static int follower_ui_visible;

// 0x67BC64
static bool in_update;

// 0x67BC0C
static tig_window_handle_t follower_ui_drop_down_menu_window;

// 0x56A4A0
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

    tig_art_interface_id_create(826, 0, 0, 0, &art_id);
    tig_art_frame_data(art_id, &art_frame_data);

    for (index = 0; index < FOLLOWER_UI_SLOTS; index++) {
        follower_ui_drop_down_menu_rects[index].x = follower_ui_button_rects[index].x + follower_ui_button_rects[index].width;
        follower_ui_drop_down_menu_rects[index].y = follower_ui_button_rects[index].y;
        follower_ui_drop_down_menu_rects[index].width = art_frame_data.width;
        follower_ui_drop_down_menu_rects[index].height = art_frame_data.height;
    }

    follower_ui_followers_capacity = 10;
    follower_ui_followers_count = 0;
    follower_ui_top_index = 0;
    follower_ui_followers = (FollowerInfo*)MALLOC(sizeof(*follower_ui_followers) * follower_ui_followers_capacity);

    font_desc.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &follower_ui_drop_down_menu_item_normal_color);

    font_desc.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(128, 128, 128);
    tig_font_create(&font_desc, &follower_ui_drop_down_menu_item_disabled_color);

    font_desc.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font_desc.art_id));
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 210, 0);
    tig_font_create(&font_desc, &follower_ui_drop_down_menu_item_highlighted_color);

    follower_ui_initialized = true;
    follower_ui_visible = true;
    follower_ui_drop_down_menu_window = TIG_WINDOW_HANDLE_INVALID;

    return true;
}

// 0x56A6E0
void follower_ui_create(int index)
{
    TigWindowData window_data;
    TigButtonData button_data;

    window_data.flags = TIG_WINDOW_HIDDEN;
    if (index == FOLLOWER_UI_BUTTON_TOGGLE) {
        window_data.message_filter = follower_ui_message_filter;
        window_data.flags |= TIG_WINDOW_MESSAGE_FILTER;
    } else {
        window_data.message_filter = NULL;
    }

    window_data.rect = follower_ui_button_rects[index];

    switch (index) {
    case FOLLOWER_UI_BUTTON_TOGGLE:
    case FOLLOWER_UI_BUTTON_SCROLL_UP:
    case FOLLOWER_UI_BUTTON_SCROLL_DOWN:
        hrp_apply(&(window_data.rect), GRAVITY_LEFT | GRAVITY_BOTTOM);
        break;
    default:
        hrp_apply(&(window_data.rect), GRAVITY_LEFT | GRAVITY_TOP);
        break;
    }

    tig_window_create(&window_data, &(follower_ui_windows[index]));

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    button_data.x = 0;
    button_data.y = 0;

    switch (index) {
    case FOLLOWER_UI_BUTTON_TOGGLE:
        tig_art_interface_id_create(501, 0, 0, 0, &(button_data.art_id));
        break;
    case FOLLOWER_UI_BUTTON_SCROLL_UP:
        tig_art_interface_id_create(506, 0, 0, 0, &(button_data.art_id));
        break;
    case FOLLOWER_UI_BUTTON_SCROLL_DOWN:
        tig_art_interface_id_create(508, 0, 0, 0, &(button_data.art_id));
        break;
    default:
        button_data.art_id = TIG_ART_ID_INVALID;
        button_data.width = follower_ui_button_rects[index].width;
        button_data.height = follower_ui_button_rects[index].height;
        break;
    }

    button_data.window_handle = follower_ui_windows[index];
    tig_button_create(&button_data, &(follower_ui_buttons[index]));
}

// 0x56A820
void follower_ui_exit()
{
    int index;

    mes_unload(follower_ui_mes_file);

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

// 0x56A880
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

// 0x56A8D0
void follower_ui_resize(GameResizeInfo* resize_info)
{
    TigRect* rects;
    int index;

    (void)resize_info;

    rects = intgame_is_compact_interface()
        ? follower_ui_special_button_rects_compact_mode
        : follower_ui_special_button_rects_normal_mode;

    if (resize_info->window_rect.width - 800 > 134) {
        rects = follower_ui_special_button_rects_compact_mode;
    }

    for (index = FOLLOWER_UI_SLOTS; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        follower_ui_button_rects[index] = rects[index - FOLLOWER_UI_SLOTS];
    }

    for (index = FOLLOWER_UI_SLOTS; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        tig_window_destroy(follower_ui_windows[index]);
    }

    for (index = FOLLOWER_UI_SLOTS; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        follower_ui_create(index);
    }

    follower_ui_refresh();
    follower_ui_toggle();
    follower_ui_toggle();
}

// 0x56A940
bool follower_ui_load(GameLoadInfo* load_info)
{
    if (tig_file_fread(&follower_ui_visible, sizeof(follower_ui_visible), 1, load_info->stream) != 1) return false;
    if (tig_file_fread(&follower_ui_top_index, sizeof(follower_ui_top_index), 1, load_info->stream) != 1) return false;

    follower_ui_refresh();

    return true;
}

// 0x56A990
bool follower_ui_save(TigFile* stream)
{
    if (tig_file_fwrite(&follower_ui_visible, sizeof(follower_ui_visible), 1, stream) != 1) return false;
    if (tig_file_fwrite(&follower_ui_top_index, sizeof(follower_ui_top_index), 1, stream) != 1) return false;

    return true;
}

// Implement player floating text.
static void follower_ui_issue_command_text(int64_t commander, int64_t subordinate, int mes_id)
{
    char name[MAX_STRING];
    Broadcast bcast;
    MesFileEntry mes_entry;
    object_examine(subordinate, commander, name);
    mes_entry.num = mes_id;
    if (mes_search(follower_ui_mes_file, &mes_entry)) {
        sprintf(bcast.str, "%s %s", name, mes_entry.str);
        bcast.loc = obj_field_int64_get(commander, OBJ_F_LOCATION);
        broadcast_msg(commander, &bcast);
    }
}

// 0x56A9D0
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
        switch (msg->type) {
        case TIG_MESSAGE_BUTTON:
            switch (msg->data.button.state) {
            case TIG_BUTTON_STATE_MOUSE_INSIDE:
                for (index = 0; index < FOLLOWER_UI_COMMAND_COUNT; index++) {
                    if (msg->data.button.button_handle == follower_ui_drop_menu_item_buttons[index]) {
                        follower_ui_drop_down_menu_refresh(index);
                        return true;
                    }
                }
                return false;
            case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
                for (index = 0; index < FOLLOWER_UI_COMMAND_COUNT; index++) {
                    if (msg->data.button.button_handle == follower_ui_drop_menu_item_buttons[index]) {
                        follower_ui_drop_down_menu_refresh(-1);
                        return true;
                    }
                }
                return false;
            case TIG_BUTTON_STATE_RELEASED:
                for (index = 0; index < FOLLOWER_UI_COMMAND_COUNT; index++) {
                    if (msg->data.button.button_handle == follower_ui_drop_menu_item_buttons[index]) {
                        follower_ui_drop_down_menu_destroy();
                        switch (index) {
                        case FOLLOWER_UI_COMMAND_WALK:
                        case FOLLOWER_UI_COMMAND_ATTACK:
                            follower_ui_begin_order_mode(index);
                            break;
                        case FOLLOWER_UI_COMMAND_INVENTORY:
                            if (sub_575080(follower_ui_commander_obj, follower_ui_subordinate_obj)) {
                                inven_ui_open(follower_ui_commander_obj, follower_ui_subordinate_obj, INVEN_UI_MODE_BARTER);
                            }
                            break;
                        case FOLLOWER_UI_COMMAND_CHARACTER_SHEET:
                            charedit_open(follower_ui_subordinate_obj, CHAREDIT_MODE_PASSIVE);
                            break;
                        case FOLLOWER_UI_COMMAND_STAY_CLOSE:
                            critter_stay_close(follower_ui_subordinate_obj);
                            follower_ui_issue_command_text(follower_ui_commander_obj, follower_ui_subordinate_obj, FOLLOWER_UI_COMMAND_STAY_CLOSE);
                            break;
                        case FOLLOWER_UI_COMMAND_SPREAD_OUT:
                            critter_spread_out(follower_ui_subordinate_obj);
                            follower_ui_issue_command_text(follower_ui_commander_obj, follower_ui_subordinate_obj, FOLLOWER_UI_COMMAND_SPREAD_OUT);
                            break;
                        case FOLLOWER_UI_COMMAND_BACK_OFF:
                            ai_stop_attacking(follower_ui_subordinate_obj);
                            follower_ui_issue_command_text(follower_ui_commander_obj, follower_ui_subordinate_obj, FOLLOWER_UI_COMMAND_BACK_OFF);
                            break;
                        case FOLLOWER_UI_COMMAND_WAIT:
                            if ((obj_field_int32_get(follower_ui_subordinate_obj, OBJ_F_SPELL_FLAGS) & OSF_MIND_CONTROLLED) != 0) {
                                return true;
                            }
                            ai_npc_wait(follower_ui_subordinate_obj);
                            follower_ui_issue_command_text(follower_ui_commander_obj, follower_ui_subordinate_obj, FOLLOWER_UI_COMMAND_WAIT);
                            break;
                            // FALLTHROUGH
                        default:
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
                follower_ui_drop_down_menu_destroy();
                break;
            }
            break;
        }

        return false;
    }

    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
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

            for (index = 0; index < FOLLOWER_UI_SLOTS; index++) {
                if (follower_ui_top_index + index >= follower_ui_followers_count) {
                    break;
                }

                if (msg->data.mouse.x >= follower_ui_button_rects[index].x
                    && msg->data.mouse.y >= follower_ui_button_rects[index].y
                    && msg->data.mouse.x < follower_ui_button_rects[index].x + follower_ui_button_rects[index].width
                    && msg->data.mouse.y < follower_ui_button_rects[index].y + follower_ui_button_rects[index].height) {
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

// 0x56AFD0
void follower_ui_drop_down_menu_create(int index)
{
    TigWindowData window_data;
    TigButtonData button_data;
    int cmd;

    follower_ui_commander_obj = player_get_local_pc_obj();

    sub_444130(&(follower_ui_followers[follower_ui_top_index + index]));
    follower_ui_subordinate_obj = follower_ui_followers[follower_ui_top_index + index].obj;

    window_data.flags = TIG_WINDOW_MESSAGE_FILTER; 
    window_data.message_filter = follower_ui_message_filter;

    window_data.rect = follower_ui_drop_down_menu_rects[index];
    hrp_apply(&(window_data.rect), GRAVITY_LEFT | GRAVITY_TOP);

    tig_window_create(&window_data, &follower_ui_drop_down_menu_window);

    follower_ui_drop_down_menu_refresh(-1);

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
    
    // Ensure buttons belong to the new window
    button_data.window_handle = follower_ui_drop_down_menu_window;

    for (cmd = 0; cmd < FOLLOWER_UI_COMMAND_COUNT; cmd++) {
        tig_button_create(&button_data, &(follower_ui_drop_menu_item_buttons[cmd]));
        button_data.y += 18;
    }
}

// 0x56B0F0
void follower_ui_drop_down_menu_destroy()
{
    tig_window_handle_t win = follower_ui_drop_down_menu_window;
    follower_ui_drop_down_menu_window = TIG_WINDOW_HANDLE_INVALID;

    if (win != TIG_WINDOW_HANDLE_INVALID) {
        tig_window_destroy(win);
    }
}

// 0x56B110
void follower_ui_begin_order_mode(int cmd)
{
    uint64_t tgt;

    intgame_mode_set(INTGAME_MODE_FOLLOWER);

    if (cmd == FOLLOWER_UI_COMMAND_WALK) {
        tgt = Tgt_Tile;
    } else {
        if (tig_kb_get_modifier(SDL_KMOD_ALT)) {
            tgt = Tgt_Obj_No_Self | Tgt_Obj_No_T_Wall;
        } else {
            tgt = Tgt_Obj_No_ST_Critter_Dead | Tgt_Obj_No_Self | Tgt_Obj_No_T_Wall | Tgt_Non_Party_Critters;
        }
    }

    sub_4F25B0(tgt);
}

// 0x56B180
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
            num = 0;
            bcast.loc = a1->loc;
        } else {
            // Attack
            num = 1;
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

// 0x56B280
void follower_ui_end_order_mode()
{
    intgame_mode_set(INTGAME_MODE_MAIN);
}

// 0x56B290
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
            follower_ui_refresh();
            break;
        }

        follower_ui_draw(follower_ui_windows[index], 503, 0, 0, 100, 100);

        if (intgame_examine_portrait(pc_obj, follower_obj, &portrait)) {
            portrait_draw_32x32(follower_obj,
                portrait,
                follower_ui_windows[index],
                4,
                4);
        } else {
            follower_ui_draw(follower_ui_windows[index], portrait, 4, 4, 100, 50);
        }

        hp_percent = 100 * object_hp_current(follower_obj) / object_hp_max(follower_obj);
        if (stat_level_get(follower_obj, STAT_POISON_LEVEL) > 20) {
            follower_ui_draw(follower_ui_windows[index], 616, 3, 39, hp_percent, 100);
        } else {
            follower_ui_draw(follower_ui_windows[index], 505, 3, 39, hp_percent, 100);
        }

        if (hp_percent < 20) {
            tig_window_tint(follower_ui_windows[index], &rect, color, 0);
        }

        fatigue_percent = 100 * critter_fatigue_current(follower_obj) / critter_fatigue_max(follower_obj);
        follower_ui_draw(follower_ui_windows[index], 504, 3, 44, fatigue_percent, 100);
    }

    in_update = false;
}

// 0x56B4D0
void follower_ui_update_obj(int64_t obj)
{
    if (follower_ui_visible) {
        if (critter_pc_leader_get(obj) == player_get_local_pc_obj()) {
            follower_ui_update();
        }
    }
}

// 0x56B510
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

// 0x56B620
void follower_ui_toggle()
{
    int index;
    tig_art_id_t art_id;

    follower_ui_visible = !follower_ui_visible;
    if (follower_ui_visible) {
        for (index = 0; index < FOLLOWER_UI_SLOTS && index < follower_ui_followers_count; index++) {
            tig_window_show(follower_ui_windows[index]);
        }

        tig_art_interface_id_create(501, 0, 0, 0, &art_id);
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_TOGGLE], art_id);
        update_toggle_button_visibility();
        update_scroll_buttons_visibility();
        follower_ui_update();
    } else {
        for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
            tig_window_hide(follower_ui_windows[index]);
        }

        tig_art_interface_id_create(502, 0, 0, 0, &art_id);
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_TOGGLE], art_id);
        update_toggle_button_visibility();
        update_scroll_buttons_visibility();
    }
}

// 0x56B6F0
void follower_ui_refresh()
{
    int index;
    int64_t pc_obj;
    ObjectList followers;
    ObjectNode* node;

    if (!follower_ui_initialized) {
        return;
    }

    for (index = 0; index < FOLLOWER_UI_BUTTON_COUNT; index++) {
        tig_window_hide(follower_ui_windows[index]);
    }

    follower_ui_followers_count = 0;
    pc_obj = player_get_local_pc_obj();
    object_list_followers(pc_obj, &followers);
    node = followers.head;
    while (node != NULL) {
        follower_ui_followers_count++;
        node = node->next;
    }

    if (follower_ui_followers_count > follower_ui_followers_capacity) {
        follower_ui_followers_capacity = follower_ui_followers_count + 10;
        follower_ui_followers = (FollowerInfo*)REALLOC(follower_ui_followers, sizeof(*follower_ui_followers) * follower_ui_followers_capacity);
    }

    index = 0;
    node = followers.head;
    while (node != NULL) {
        sub_4440E0(node->obj, &(follower_ui_followers[index++]));
        node = node->next;
    }
    follower_ui_followers_count = index;

    object_list_destroy(&followers);

    if (follower_ui_visible) {
        for (index = 0; index < FOLLOWER_UI_SLOTS && index < follower_ui_followers_count; index++) {
            tig_window_show(follower_ui_windows[index]);
        }
    }

    if (follower_ui_followers_count > FOLLOWER_UI_SLOTS) {
        if (follower_ui_top_index > follower_ui_followers_count - FOLLOWER_UI_SLOTS) {
            follower_ui_top_index = follower_ui_followers_count - FOLLOWER_UI_SLOTS;
        }
    } else {
        follower_ui_top_index = 0;
    }

    update_toggle_button_visibility();
    update_scroll_buttons_visibility();

    if (!in_update) {
        follower_ui_update();
    }
}

// 0x56B850
void update_toggle_button_visibility()
{
    if (follower_ui_followers_count > 0) {
        tig_window_show(follower_ui_windows[FOLLOWER_UI_BUTTON_TOGGLE]);
    } else {
        tig_window_hide(follower_ui_windows[FOLLOWER_UI_BUTTON_TOGGLE]);
    }
}

// 0x56B880
void update_scroll_buttons_visibility()
{
    tig_art_id_t art_id;

    if (follower_ui_followers_count > FOLLOWER_UI_SLOTS && follower_ui_visible) {
        if (follower_ui_top_index != 0) {
            // "follow_scroll_up.art"
            tig_art_interface_id_create(506, 0, 0, 0, &art_id);
        } else {
            // "follow_scroll_up_off.art"
            tig_art_interface_id_create(507, 0, 0, 0, &art_id);
        }
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_SCROLL_UP], art_id);
        tig_window_show(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_UP]);

        if (follower_ui_top_index != follower_ui_followers_count - FOLLOWER_UI_SLOTS) {
            // "follow_scroll_dwn.art"
            tig_art_interface_id_create(508, 0, 0, 0, &art_id);
        } else {
            // "follow_scroll_dwn_off.art"
            tig_art_interface_id_create(509, 0, 0, 0, &art_id);
        }
        tig_button_set_art(follower_ui_buttons[FOLLOWER_UI_BUTTON_SCROLL_DOWN], art_id);
        tig_window_show(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_DOWN]);
    } else {
        tig_window_hide(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_UP]);
        tig_window_hide(follower_ui_windows[FOLLOWER_UI_BUTTON_SCROLL_DOWN]);
    }
}

// 0x56B970
void follower_ui_drop_down_menu_refresh(int highlighted_cmd)
{
    TigRect rect;
    int cmd;
    MesFileEntry mes_file_entry;

    follower_ui_draw(follower_ui_drop_down_menu_window, 826, 0, 0, 100, 100);

    rect = follower_ui_drop_down_menu_entry_rect;
    for (cmd = 0; cmd < FOLLOWER_UI_COMMAND_COUNT; cmd++) {
        mes_file_entry.num = cmd;
        if (mes_search(follower_ui_mes_file, &mes_file_entry)) {
            if ((cmd == FOLLOWER_UI_COMMAND_INVENTORY && sub_575080(follower_ui_commander_obj, follower_ui_subordinate_obj))
                || (cmd != FOLLOWER_UI_COMMAND_WAIT || (obj_field_int32_get(follower_ui_subordinate_obj, OBJ_F_SPELL_FLAGS) & OSF_MIND_CONTROLLED) == 0)) {
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
        rect.y += 18;
    }
}
