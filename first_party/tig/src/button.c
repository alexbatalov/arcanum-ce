#include "tig/button.h"

#include "tig/art.h"
#include "tig/debug.h"
#include "tig/rect.h"
#include "tig/sound.h"
#include "tig/window.h"

// The maximum number of buttons.
#define MAX_BUTTONS 400

typedef unsigned int TigButtonUsage;

#define TIG_BUTTON_USAGE_FREE 0x01u
#define TIG_BUTTON_USAGE_RADIO 0x02u
#define TIG_BUTTON_USAGE_FORCE_HIDDEN 0x04u

typedef struct TigButton {
    /* 0000 */ TigButtonUsage usage;
    /* 0004 */ TigButtonFlags flags;
    /* 0008 */ tig_window_handle_t window_handle;
    /* 000C */ TigRect rect;
    /* 001C */ tig_art_id_t art_id;
    /* 0020 */ TigButtonState state;
    /* 0024 */ int mouse_down_snd_id;
    /* 0028 */ int mouse_up_snd_id;
    /* 002C */ int mouse_enter_snd_id;
    /* 0030 */ int mouse_exit_snd_id;
    /* 0034 */ int group;
} TigButton;

static int tig_button_free_index();
static int tig_button_handle_to_index(tig_button_handle_t button_handle);
static tig_button_handle_t tig_button_index_to_handle(int index);
static void tig_button_play_sound(tig_button_handle_t button_handle, int event);
static tig_button_handle_t tig_button_radio_group_get_selected(int group);
static void sub_5387B0(int group);
static void sub_5387D0();

// 0x5C26F8
static tig_button_handle_t tig_button_pressed_button_handle = TIG_BUTTON_HANDLE_INVALID;

// 0x5C26FC
static tig_button_handle_t tig_button_hovered_button_handle = TIG_BUTTON_HANDLE_INVALID;

// 0x630D60
static TigButton buttons[MAX_BUTTONS];

// 0x6364E0
static bool busy;

// 0x537B00
int tig_button_init(TigInitInfo* init_info)
{
    int button_index;

    (void)init_info;

    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        buttons[button_index].usage = TIG_BUTTON_USAGE_FREE;
    }

    return TIG_OK;
}

// 0x537B20
void tig_button_exit()
{
    int button_index;
    tig_button_handle_t button_handle;

    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        if ((buttons[button_index].usage & TIG_BUTTON_USAGE_FREE) == 0) {
            button_handle = tig_button_index_to_handle(button_index);
            tig_button_destroy(button_handle);
        }
    }
}

// 0x537B50
int tig_button_create(TigButtonData* button_data, tig_button_handle_t* button_handle)
{
    int button_index;
    int rc;
    TigWindowData window_data;
    TigArtFrameData art_frame_data;
    TigButton* btn;

    button_index = tig_button_free_index();
    if (button_index == -1) {
        return TIG_ERR_OUT_OF_HANDLES;
    }

    btn = &(buttons[button_index]);

    btn->window_handle = button_data->window_handle;
    btn->group = -1;

    rc = tig_window_data(button_data->window_handle, &window_data);
    if (rc != TIG_OK) {
        return rc;
    }

    btn->flags = button_data->flags;

    if ((window_data.flags & TIG_WINDOW_HIDDEN) != 0) {
        btn->flags |= TIG_BUTTON_HIDDEN;
        btn->usage |= TIG_BUTTON_USAGE_FORCE_HIDDEN;
    }

    if ((btn->flags & TIG_BUTTON_LATCH) != 0) {
        btn->flags |= TIG_BUTTON_TOGGLE;
    }

    btn->rect.x = window_data.rect.x + button_data->x;
    btn->rect.y = window_data.rect.y + button_data->y;

    if (button_data->art_id != TIG_ART_ID_INVALID) {
        rc = tig_art_frame_data(button_data->art_id, &art_frame_data);
        if (rc != TIG_OK) {
            return rc;
        }

        btn->rect.width = art_frame_data.width;
        btn->rect.height = art_frame_data.height;
    } else {
        btn->rect.width = button_data->width;
        btn->rect.height = button_data->height;
    }

    btn->art_id = button_data->art_id;

    if ((button_data->flags & TIG_BUTTON_ON) != 0) {
        btn->state = TIG_BUTTON_STATE_PRESSED;
    } else {
        btn->state = TIG_BUTTON_STATE_MOUSE_OUTSIDE;
    }

    btn->mouse_down_snd_id = button_data->mouse_down_snd_id;
    btn->mouse_up_snd_id = button_data->mouse_up_snd_id;
    btn->mouse_enter_snd_id = button_data->mouse_enter_snd_id;
    btn->mouse_exit_snd_id = button_data->mouse_exit_snd_id;

    *button_handle = tig_button_index_to_handle(button_index);

    rc = tig_window_button_add(button_data->window_handle, *button_handle);
    if (rc != TIG_OK) {
        return rc;
    }

    if ((button_data->flags & TIG_BUTTON_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(btn->rect));
        tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    }

    btn->usage &= ~TIG_BUTTON_USAGE_FREE;

    return TIG_OK;
}

// 0x537CE0
int tig_button_destroy(tig_button_handle_t button_handle)
{
    int button_index;
    int rc;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_destroy: ERROR: Attempt to reference Empty ButtonID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    button_index = tig_button_handle_to_index(button_handle);
    rc = tig_window_button_remove(buttons[button_index].window_handle, button_handle);
    if (rc != TIG_OK) {
        return rc;
    }

    if (tig_button_pressed_button_handle == button_handle || tig_button_hovered_button_handle == button_handle) {
        sub_5387D0();
    }

    tig_window_invalidate_rect(&(buttons[button_index].rect));
    tig_button_refresh_rect(buttons[button_index].window_handle, &(buttons[button_index].rect));
    buttons[button_index].usage = TIG_BUTTON_USAGE_FREE;

    return TIG_OK;
}

// 0x537D70
int tig_button_data(tig_button_handle_t button_handle, TigButtonData* button_data)
{
    int button_index;
    TigWindowData window_data;
    int rc;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_data: ERROR: Attempt to reference Empty ButtonID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    button_index = tig_button_handle_to_index(button_handle);
    button_data->flags = buttons[button_index].flags;
    button_data->window_handle = buttons[button_index].window_handle;

    rc = tig_window_data(buttons[button_index].window_handle, &window_data);
    if (rc != TIG_OK) {
        return rc;
    }

    button_data->x = buttons[button_index].rect.x - window_data.rect.x;
    button_data->y = buttons[button_index].rect.y - window_data.rect.y;
    button_data->art_id = buttons[button_index].art_id;
    button_data->width = buttons[button_index].rect.width;
    button_data->height = buttons[button_index].rect.height;
    button_data->mouse_down_snd_id = buttons[button_index].mouse_down_snd_id;
    button_data->mouse_up_snd_id = buttons[button_index].mouse_up_snd_id;
    button_data->mouse_enter_snd_id = buttons[button_index].mouse_enter_snd_id;
    button_data->mouse_exit_snd_id = buttons[button_index].mouse_exit_snd_id;

    return TIG_OK;
}

// 0x537E20
int tig_button_refresh_rect(int window_handle, TigRect* rect)
{
    tig_button_handle_t* window_buttons;
    int num_window_buttons;
    TigWindowData window_data;
    int rc;
    int index;
    int button_index;
    TigRect src_rect;
    TigRect dest_rect;
    unsigned int art_id;
    TigArtBlitInfo blit_info;

    if (busy) {
        return TIG_OK;
    }

    num_window_buttons = tig_window_button_list(window_handle, &window_buttons);

    rc = tig_window_data(window_handle, &window_data);
    if (rc != TIG_OK) {
        return rc;
    }

    if (num_window_buttons != 0) {
        busy = true;

        for (index = 0; index < num_window_buttons; index++) {
            button_index = tig_button_handle_to_index(window_buttons[index]);

            if (tig_rect_intersection(&(buttons[button_index].rect), rect, &src_rect) == TIG_OK
                && (buttons[button_index].flags & TIG_BUTTON_HIDDEN) == 0) {
                dest_rect.width = src_rect.width;
                dest_rect.height = src_rect.height;
                dest_rect.x = src_rect.x - window_data.rect.x;
                dest_rect.y = src_rect.y - window_data.rect.y;

                src_rect.x -= buttons[button_index].rect.x;
                src_rect.y -= buttons[button_index].rect.y;

                art_id = buttons[button_index].art_id;
                if (art_id != TIG_ART_ID_INVALID) {
                    if (buttons[button_index].state != TIG_BUTTON_STATE_MOUSE_OUTSIDE) {
                        art_id = tig_art_id_frame_inc(art_id);

                        if (buttons[button_index].state == TIG_BUTTON_STATE_MOUSE_INSIDE) {
                            art_id = tig_art_id_frame_inc(art_id);
                        }
                    }

                    blit_info.art_id = art_id;
                    blit_info.src_rect = &src_rect;
                    blit_info.flags = 0;
                    blit_info.dst_rect = &dest_rect;
                    tig_window_blit_art(window_handle, &blit_info);
                }
            }
        }

        busy = false;
    }

    return TIG_OK;
}

// 0x537FA0
int tig_button_free_index()
{
    int button_index;

    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        if ((buttons[button_index].usage & TIG_BUTTON_USAGE_FREE) != 0) {
            return button_index;
        }
    }

    return -1;
}

// 0x537FC0
int tig_button_handle_to_index(tig_button_handle_t button_handle)
{
    return (int)button_handle;
}

// 0x537FD0
tig_button_handle_t tig_button_index_to_handle(int index)
{
    return (tig_button_handle_t)index;
}

// 0x537FE0
void tig_button_state_change(tig_button_handle_t button_handle, TigButtonState state)
{
    // 0x6364E4
    static bool in_state_change;

    int button_index;
    TigButtonState old_state;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_state_change: ERROR: Attempt to reference Empty ButtonID!\n");
        return;
    }

    button_index = tig_button_handle_to_index(button_handle);
    old_state = buttons[button_index].state;

    if (old_state == TIG_BUTTON_STATE_PRESSED) {
        if (state != TIG_BUTTON_STATE_RELEASED) {
            return;
        }
        state = TIG_BUTTON_STATE_MOUSE_OUTSIDE;
    } else {
        if (state == TIG_BUTTON_STATE_RELEASED) {
            state = TIG_BUTTON_STATE_MOUSE_OUTSIDE;
        }
    }

    if (old_state != state) {
        if (!in_state_change) {
            if (state == 0) {
                in_state_change = true;
                if ((buttons[button_index].usage & TIG_BUTTON_USAGE_RADIO) != 0) {
                    sub_5387B0(buttons[button_index].group);
                }
                in_state_change = false;
            }
        }

        buttons[button_index].state = state;
        tig_button_refresh_rect(buttons[button_index].window_handle, &(buttons[button_index].rect));
        tig_window_invalidate_rect(&(buttons[button_index].rect));
    }
}

// 0x5380A0
int tig_button_state_get(tig_button_handle_t button_handle, TigButtonState* state)
{
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        tig_debug_printf("tig_button_state_get: ERROR: Attempt to reference Empty ButtonID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    button_index = tig_button_handle_to_index(button_handle);
    if (state == NULL) {
        return TIG_ERR_INVALID_PARAM;
    }

    if ((buttons[button_index].usage & TIG_BUTTON_USAGE_FREE) != 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    *state = buttons[button_index].state;

    return TIG_OK;
}

// 0x5380F0
tig_button_handle_t tig_button_get_at_position(int x, int y)
{
    TigArtAnimData art_anim_data;
    TigButton* btn;
    tig_window_handle_t window_handle;
    tig_button_handle_t* window_buttons;
    int num_window_buttons;
    int index;
    int button_index;
    unsigned int color;

    if (tig_window_get_at_position(x, y, &window_handle) != TIG_OK) {
        return TIG_BUTTON_HANDLE_INVALID;
    }

    num_window_buttons = tig_window_button_list(window_handle, &window_buttons);

    for (index = 0; index < num_window_buttons; index++) {
        button_index = tig_button_handle_to_index(window_buttons[index]);
        btn = &(buttons[button_index]);

        if ((btn->flags & TIG_BUTTON_HIDDEN) == 0
            && x >= btn->rect.x
            && y >= btn->rect.y
            && x < btn->rect.x + btn->rect.width
            && y < btn->rect.y + btn->rect.height) {
            if (btn->art_id == TIG_ART_ID_INVALID) {
                return window_buttons[index];
            }

            if (sub_502E50(btn->art_id, x - btn->rect.x, y - btn->rect.y, &color) == TIG_OK
                && tig_art_anim_data(btn->art_id, &art_anim_data) == TIG_OK
                && color != art_anim_data.color_key) {
                return window_buttons[index];
            }
        }
    }

    return TIG_BUTTON_HANDLE_INVALID;
}

// 0x538220
bool tig_button_process_mouse_msg(TigMouseMessageData* mouse)
{
    TigMessage msg;
    tig_button_handle_t button_handle;
    int button_index;

    if (mouse->event != TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN
        && mouse->event != TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP
        && mouse->event != TIG_MESSAGE_MOUSE_MOVE) {
        return false;
    }

    msg.type = TIG_MESSAGE_BUTTON;
    msg.data.button.x = mouse->x;
    msg.data.button.y = mouse->y;

    button_handle = tig_button_get_at_position(mouse->x, mouse->y);
    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        if (tig_button_hovered_button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_state_change(tig_button_hovered_button_handle, TIG_BUTTON_STATE_MOUSE_OUTSIDE);

            tig_timer_now(&(msg.timestamp));
            msg.data.button.button_handle = tig_button_hovered_button_handle;
            msg.data.button.state = TIG_BUTTON_STATE_MOUSE_OUTSIDE;
            tig_message_enqueue(&msg);

            tig_button_play_sound(tig_button_hovered_button_handle, TIG_BUTTON_STATE_MOUSE_OUTSIDE);

            tig_button_hovered_button_handle = TIG_BUTTON_HANDLE_INVALID;
        }

        if (tig_button_pressed_button_handle != TIG_BUTTON_HANDLE_INVALID) {
            if (mouse->event == TIG_MESSAGE_MOUSE_MOVE) {
                tig_button_state_change(tig_button_pressed_button_handle, TIG_BUTTON_STATE_RELEASED);
            } else if (mouse->event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP) {
                tig_button_pressed_button_handle = TIG_BUTTON_HANDLE_INVALID;
            }
        }

        return false;
    }

    if (tig_button_pressed_button_handle != TIG_BUTTON_HANDLE_INVALID && mouse->event == TIG_MESSAGE_MOUSE_MOVE) {
        if (tig_button_pressed_button_handle == button_handle) {
            tig_button_state_change(button_handle, TIG_BUTTON_STATE_PRESSED);

            if (tig_button_hovered_button_handle != button_handle) {
                tig_timer_now(&(msg.timestamp));
                msg.data.button.button_handle = button_handle;
                msg.data.button.state = TIG_BUTTON_STATE_MOUSE_INSIDE;
                tig_message_enqueue(&msg);

                tig_button_play_sound(button_handle, TIG_BUTTON_STATE_MOUSE_INSIDE);

                tig_button_hovered_button_handle = button_handle;
            }
        } else {
            tig_button_state_change(tig_button_pressed_button_handle, TIG_BUTTON_STATE_RELEASED);
        }

        return false;
    }

    if (mouse->event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN) {
        int new_state;

        button_index = tig_button_handle_to_index(button_handle);

        new_state = TIG_BUTTON_STATE_PRESSED;
        if (tig_button_pressed_button_handle == TIG_BUTTON_HANDLE_INVALID) {
            if ((buttons[button_index].flags & TIG_BUTTON_TOGGLE) != 0) {
                if (buttons[button_index].state == TIG_BUTTON_STATE_PRESSED
                    && (buttons[button_index].flags & TIG_BUTTON_LATCH) != 0) {
                    return true;
                }

                if ((buttons[button_index].usage & TIG_BUTTON_USAGE_RADIO) != 0) {
                    sub_5387B0(buttons[button_index].group);
                }

                new_state = buttons[button_index].state == TIG_BUTTON_STATE_PRESSED
                    ? TIG_BUTTON_STATE_RELEASED
                    : TIG_BUTTON_STATE_PRESSED;
                tig_button_state_change(button_handle, new_state);

                tig_button_hovered_button_handle = TIG_BUTTON_HANDLE_INVALID;
            } else {
                tig_button_pressed_button_handle = button_handle;
                tig_button_state_change(button_handle, TIG_BUTTON_STATE_PRESSED);
            }

            if (tig_button_pressed_button_handle == TIG_BUTTON_HANDLE_INVALID) {
                tig_timer_now(&(msg.timestamp));
                msg.data.button.button_handle = button_handle;
                msg.data.button.state = new_state;
                tig_message_enqueue(&msg);

                tig_button_play_sound(button_handle, new_state);

                return true;
            }
        }

        if (tig_button_pressed_button_handle == button_handle) {
            tig_timer_now(&(msg.timestamp));
            msg.data.button.button_handle = button_handle;
            msg.data.button.state = new_state;
            tig_message_enqueue(&msg);

            tig_button_play_sound(button_handle, new_state);

            return true;
        }

        return false;
    }

    if (mouse->event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP) {
        if (tig_button_pressed_button_handle != button_handle) {
            tig_button_pressed_button_handle = TIG_BUTTON_HANDLE_INVALID;
            return false;
        }

        button_index = tig_button_handle_to_index(button_handle);

        if ((buttons[button_index].flags & TIG_BUTTON_TOGGLE) == 0) {
            tig_button_pressed_button_handle = TIG_BUTTON_HANDLE_INVALID;
            tig_button_hovered_button_handle = button_handle;
            tig_button_state_change(button_handle, TIG_BUTTON_STATE_RELEASED);
            tig_button_state_change(button_handle, TIG_BUTTON_STATE_MOUSE_INSIDE);

            tig_timer_now(&(msg.timestamp));
            msg.data.button.button_handle = button_handle;
            msg.data.button.state = TIG_BUTTON_STATE_RELEASED;
            tig_message_enqueue(&msg);

            tig_button_play_sound(button_handle, TIG_BUTTON_STATE_RELEASED);
            return true;
        }

        return false;
    }

    if (tig_button_hovered_button_handle != button_handle
        && tig_button_pressed_button_handle == TIG_BUTTON_HANDLE_INVALID
        && mouse->event == TIG_MESSAGE_MOUSE_MOVE) {
        button_index = tig_button_handle_to_index(button_handle);

        if (tig_button_hovered_button_handle != TIG_BUTTON_HANDLE_INVALID) {
            if ((buttons[button_index].flags & TIG_BUTTON_TOGGLE) == 0
                || buttons[button_index].state != TIG_BUTTON_STATE_PRESSED) {
                tig_button_state_change(tig_button_hovered_button_handle, TIG_BUTTON_STATE_MOUSE_OUTSIDE);
            }

            tig_timer_now(&(msg.timestamp));
            msg.data.button.button_handle = tig_button_hovered_button_handle;
            msg.data.button.state = TIG_BUTTON_STATE_MOUSE_OUTSIDE;
            tig_message_enqueue(&msg);

            tig_button_play_sound(tig_button_hovered_button_handle, TIG_BUTTON_STATE_MOUSE_OUTSIDE);
        }

        tig_button_hovered_button_handle = button_handle;

        if ((buttons[button_index].flags & TIG_BUTTON_TOGGLE) == 0
            || buttons[button_index].state != TIG_BUTTON_STATE_PRESSED) {
            tig_button_state_change(tig_button_hovered_button_handle, TIG_BUTTON_STATE_MOUSE_INSIDE);
        }

        tig_timer_now(&(msg.timestamp));
        msg.data.button.button_handle = tig_button_hovered_button_handle;
        msg.data.button.state = TIG_BUTTON_STATE_MOUSE_INSIDE;
        tig_message_enqueue(&msg);

        tig_button_play_sound(tig_button_hovered_button_handle, TIG_BUTTON_STATE_MOUSE_INSIDE);

        return false;
    }

    return false;
}

// 0x5385C0
void tig_button_play_sound(tig_button_handle_t button_handle, int event)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        // FIXME: Wrong function name.
        tig_debug_printf("tig_button_state_change: ERROR: Attempt to reference Empty ButtonID!\n");
        return;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    switch (event) {
    case TIG_BUTTON_STATE_PRESSED:
        tig_sound_quick_play(btn->mouse_down_snd_id);
        break;
    case TIG_BUTTON_STATE_RELEASED:
        tig_sound_quick_play(btn->mouse_up_snd_id);
        break;
    case TIG_BUTTON_STATE_MOUSE_INSIDE:
        tig_sound_quick_play(btn->mouse_enter_snd_id);
        break;
    case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
        tig_sound_quick_play(btn->mouse_exit_snd_id);
        break;
    }
}

// 0x538670
int tig_button_radio_group_create(int count, tig_button_handle_t* button_handles, int selected)
{
    int index;
    int group;
    int button_index;
    TigButton* btn;

    if (!(selected >= 0 && selected < count)) {
        return TIG_ERR_INVALID_PARAM;
    }

    // Find new group identifier.
    group = 0;
    for (button_index = 0; button_index < MAX_BUTTONS; button_index++) {
        btn = &(buttons[button_index]);
        if ((btn->usage & TIG_BUTTON_USAGE_FREE) == 0
            && (btn->usage & TIG_BUTTON_USAGE_RADIO) != 0
            && btn->group >= group) {
            group = btn->group + 1;
        }
    }

    for (index = 0; index < count; index++) {
        button_index = tig_button_handle_to_index(button_handles[index]);
        btn = &(buttons[button_index]);

        if ((btn->usage & TIG_BUTTON_USAGE_FREE) != 0) {
            return TIG_ERR_INVALID_PARAM;
        }

        btn->usage |= TIG_BUTTON_USAGE_RADIO;
        btn->flags |= TIG_BUTTON_TOGGLE;
        btn->flags |= TIG_BUTTON_LATCH;
        btn->group = group;
    }

    tig_button_state_change(button_handles[selected], 0);

    return TIG_OK;
}

// 0x538730
tig_button_handle_t sub_538730(tig_button_handle_t button_handle)
{
    int button_index = tig_button_handle_to_index(button_handle);
    return tig_button_radio_group_get_selected(buttons[button_index].group);
}

// 0x538760
tig_button_handle_t tig_button_radio_group_get_selected(int group)
{
    int index;
    TigButton* btn;

    for (index = 0; index < MAX_BUTTONS; index++) {
        btn = &(buttons[index]);
        if ((btn->usage & TIG_BUTTON_USAGE_FREE) == 0
            && (btn->usage & TIG_BUTTON_USAGE_RADIO) != 0
            && btn->group == group
            && btn->state == TIG_BUTTON_STATE_PRESSED) {
            return tig_button_index_to_handle(index);
        }
    }

    return TIG_BUTTON_HANDLE_INVALID;
}

// 0x5387B0
void sub_5387B0(int group)
{
    tig_button_handle_t button_handle = tig_button_radio_group_get_selected(group);
    if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_state_change(button_handle, TIG_BUTTON_STATE_RELEASED);
    }
}

// 0x5387D0
void sub_5387D0()
{
    tig_button_pressed_button_handle = TIG_BUTTON_HANDLE_INVALID;
    tig_button_hovered_button_handle = TIG_BUTTON_HANDLE_INVALID;
}

// 0x5387E0
int tig_button_show(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_HIDDEN) == 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    btn->flags &= ~TIG_BUTTON_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_invalidate_rect(&(btn->rect));

    return TIG_OK;
}

// 0x538840
int tig_button_hide(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_HIDDEN) != 0) {
        if ((btn->usage & TIG_BUTTON_USAGE_FORCE_HIDDEN) != 0) {
            btn->usage &= ~TIG_BUTTON_USAGE_FORCE_HIDDEN;
        }
        return TIG_ERR_INVALID_PARAM;
    }

    btn->flags |= TIG_BUTTON_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_invalidate_rect(&(btn->rect));

    return TIG_OK;
}

// 0x5388B0
int tig_button_is_hidden(tig_button_handle_t button_handle, bool* hidden)
{
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_ERR_INVALID_PARAM;
    }

    if (hidden == NULL) {
        return TIG_ERR_INVALID_PARAM;
    }

    button_index = tig_button_handle_to_index(button_handle);
    *hidden = (buttons[button_index].flags & TIG_BUTTON_HIDDEN) != 0;

    return TIG_OK;
}

// 0x538900
int tig_button_show_force(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_HIDDEN) == 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    if ((btn->usage & TIG_BUTTON_USAGE_FORCE_HIDDEN) == 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    btn->flags &= ~TIG_BUTTON_HIDDEN;
    btn->usage &= ~TIG_BUTTON_USAGE_FORCE_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_invalidate_rect(&(btn->rect));

    return TIG_OK;
}

// 0x538980
int tig_button_hide_force(tig_button_handle_t button_handle)
{
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return TIG_OK;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if ((btn->flags & TIG_BUTTON_HIDDEN) != 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    btn->flags |= TIG_BUTTON_HIDDEN;
    btn->usage |= TIG_BUTTON_USAGE_FORCE_HIDDEN;

    tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    tig_window_invalidate_rect(&(btn->rect));

    return TIG_OK;
}

// 0x5389F0
void tig_button_set_art(tig_button_handle_t button_handle, tig_art_id_t art_id)
{
    TigArtFrameData art_frame_data;
    TigButton* btn;
    int button_index;

    if (button_handle == TIG_BUTTON_HANDLE_INVALID) {
        return;
    }

    button_index = tig_button_handle_to_index(button_handle);
    btn = &(buttons[button_index]);

    if (art_id != TIG_ART_ID_INVALID) {
        tig_art_frame_data(art_id, &art_frame_data);

        btn->rect.width = art_frame_data.width;
        btn->rect.height = art_frame_data.height;
    }

    btn->art_id = art_id;

    if ((btn->flags & TIG_BUTTON_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(btn->rect));
        tig_button_refresh_rect(btn->window_handle, &(btn->rect));
    }
}
