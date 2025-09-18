#include "ui/hotkey_ui.h"

#include "game/gamelib.h"
#include "game/gsound.h"
#include "game/hrp.h"
#include "game/item.h"
#include "game/mt_item.h"
#include "game/obj_private.h"
#include "game/object.h"
#include "game/player.h"
#include "game/sfx.h"
#include "game/spell.h"
#include "ui/intgame.h"
#include "ui/inven_ui.h"
#include "ui/mainmenu_ui.h"
#include "ui/skill_ui.h"
#include "ui/textedit_ui.h"

static bool intgame_save_hotkey(Hotkey* hotkey, TigFile* stream);
static bool intgame_load_hotkey(Hotkey* hotkey, TigFile* stream);
static int sub_57E460();
static void intgame_hotkey_mouse_load(tig_art_id_t art_id, bool a2);
static void sub_57ED60(Hotkey* hotkey, int a2);
static void sub_57EE30(int64_t a1, int inventory_location);
static bool button_create_no_art(UiButtonInfo* button_info, int width, int height);

// 0x5CB494
static int dword_5CB494[10] = {
    173,
    174,
    175,
    176,
    177,
    178,
    179,
    180,
    181,
    172,
};

// 0x5CB4BC
static int dword_5CB4BC[10] = {
    198,
    237,
    276,
    315,
    354,
    418,
    456,
    495,
    534,
    573,
};

// 0x5CB4E4
int dword_5CB4E4 = -1;

// 0x683510
static tig_window_handle_t hotkey_secondary_window_handle;

// 0x683518
static Hotkey stru_683518[2];

// 0x6835C8
static TigRect stru_6835C8;

// 0x6835D8
static tig_window_handle_t hotkey_primary_window_handle;

// 0x6835E0
static Hotkey stru_6835E0[10];

// 0x683950
static Hotkey stru_683950;

// 0x6839A8
static int hotkey_slot_width;

// 0x6839AC
static int hotkey_slot_height;

// 0x6839B0
int dword_6839B0;

// 0x57D700
bool hotkey_ui_init(GameInitInfo* init_info)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    int index;
    Hotkey* hotkey;

    (void)init_info;

    hotkey_primary_window_handle = TIG_WINDOW_HANDLE_INVALID;
    hotkey_secondary_window_handle = TIG_WINDOW_HANDLE_INVALID;

    if (tig_art_interface_id_create(dword_5CB494[0], 0, 0, 0, &art_id) != TIG_OK) {
        tig_debug_printf("hotkey_ui_init: ERROR: tig_art_interface_id_create failed!\n");
        exit(EXIT_FAILURE);
    }

    tig_art_frame_data(art_id, &art_frame_data);
    hotkey_slot_width = art_frame_data.width;
    hotkey_slot_height = art_frame_data.height;

    for (index = 0; index < 2; index++) {
        hotkey = &(stru_683518[index]);
        hotkey->type = HOTKEY_SKILL;
        hotkey->data = sub_557B50(index);
        intgame_recent_action_button_get(index)->art_num = sub_579F70(hotkey->data);
    }

    for (index = 0; index < 10; index++) {
        hotkey = &(stru_6835E0[index]);
        sub_57E5A0(hotkey);
        hotkey->slot = index;
        hotkey->data = -1;
        hotkey->info.art_num = dword_5CB494[index];
        hotkey->info.button_handle = TIG_BUTTON_HANDLE_INVALID;
        hotkey->info.x = dword_5CB4BC[index] - stru_6835C8.x;
        hotkey->info.y = 445;
    }

    return true;
}

// 0x57D800
void hotkey_ui_exit()
{
}

// 0x57D810
void hotkey_ui_resize(GameResizeInfo* resize_info)
{
    (void)resize_info;

    intgame_recent_action_button_position_set(0, 69, 548);
    intgame_recent_action_button_position_set(1, 114, 548);
}

// 0x57D840
bool hotkey_ui_start(tig_window_handle_t primary_window_handle, TigRect* rect, tig_window_handle_t secondary_window_handle, bool fullscreen)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int index;
    Hotkey* hotkey;

    stru_6835C8 = *rect;
    hotkey_primary_window_handle = primary_window_handle;
    hotkey_secondary_window_handle = secondary_window_handle;

    if (tig_art_interface_id_create(dword_5CB494[0], 0, 0, 0, &art_id) != TIG_OK) {
        tig_debug_printf("hotkey_ui_start: ERROR: tig_art_interface_id_create failed!\n");
        exit(EXIT_FAILURE);
    }

    tig_art_frame_data(art_id, &art_frame_data);

    hotkey_slot_width = art_frame_data.width;
    hotkey_slot_height = art_frame_data.height;

    if (hotkey_secondary_window_handle != TIG_WINDOW_HANDLE_INVALID) {
        for (index = 0; index < 2; index++) {
            hotkey = &(stru_683518[index]);
            intgame_recent_action_button_get(index)->art_num = hotkey->type == 2 || hotkey->type != 3
                ? sub_579F70(hotkey->data)
                : spell_icon(hotkey->data);
            intgame_button_create_ex(hotkey_secondary_window_handle, &stru_6835C8, intgame_recent_action_button_get(index), 0x1);
        }
    }

    if (fullscreen) {
        if (tig_art_interface_id_create(184, 0, 0, 0, &art_id) == TIG_OK) {
            tig_art_frame_data(art_id, &art_frame_data);

            src_rect.x = dword_5CB4BC[0] - 2;
            src_rect.y = 1;
            src_rect.width = 411;
            src_rect.height = 37;

            dst_rect.x = 0;
            dst_rect.y = 0;
            dst_rect.width = 411;
            dst_rect.height = 37;

            art_blit_info.flags = 0;
            art_blit_info.art_id = art_id;
            art_blit_info.src_rect = &src_rect;
            art_blit_info.dst_rect = &dst_rect;
            tig_window_blit_art(hotkey_primary_window_handle, &art_blit_info);
        }
    }

    for (index = 0; index < 10; index++) {
        hotkey = &(stru_6835E0[index]);
        hotkey->info.x = dword_5CB4BC[index] - stru_6835C8.x;
        hotkey->info.y = 445;
        intgame_hotkey_refresh(index);

        if (!button_create_no_art(&(hotkey->info), hotkey_slot_width, hotkey_slot_height)) {
            tig_debug_printf("hotkey_ui_start: ERROR: button_create_no_art failed!\n");
            exit(EXIT_FAILURE);
        }
    }

    return true;
}

// 0x57DA50
void hotkey_ui_end()
{
    int index;
    tig_button_handle_t button_handle;

    for (index = 0; index < 2; index++) {
        button_handle = intgame_recent_action_button_get(index)->button_handle;
        if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_destroy(button_handle);
            intgame_recent_action_button_get(index)->button_handle = TIG_BUTTON_HANDLE_INVALID;
        }
    }

    for (index = 0; index < 10; index++) {
        if (stru_6835E0[index].info.button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_destroy(stru_6835E0[index].info.button_handle);
            stru_6835E0[index].info.button_handle = TIG_BUTTON_HANDLE_INVALID;
        }
    }
}

// 0x57DAB0
void sub_57DAB0()
{
    int index;
    tig_button_handle_t button_handle;
    tig_art_id_t art_id;

    dword_6839B0 = false;

    for (index = 0; index < 2; index++) {
        sub_57E5A0(&(stru_683518[index]));
        stru_683518[index].type = 2;
        stru_683518[index].data = sub_557B50(index);
        intgame_recent_action_button_get(index)->art_num = sub_579F70(stru_683518[index].data);

        button_handle = intgame_recent_action_button_get(index)->button_handle;
        if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
            tig_art_interface_id_create(intgame_recent_action_button_get(index)->art_num, 0, 0, 0, &art_id);
            tig_button_set_art(button_handle, art_id);
        }
    }
}

// 0x57DB40
bool hotkey_ui_save(TigFile* stream)
{
    int index;

    for (index = 0; index < 2; index++) {
        stru_683518[index].slot = -1;
        if (!intgame_save_hotkey(&(stru_683518[index]), stream)) {
            return false;
        }
    }

    for (index = 0; index < 10; index++) {
        if (!intgame_save_hotkey(&(stru_6835E0[index]), stream)) {
            return false;
        }
    }

    return true;
}

// 0x57DBA0
bool hotkey_ui_load(GameLoadInfo* load_info)
{
    int index;
    Hotkey hotkey;

    for (index = 0; index < 2; index++) {
        if (!intgame_load_hotkey(&hotkey, load_info->stream)) {
            return false;
        }

        sub_57EFA0(hotkey.type, hotkey.data, hotkey.item_obj.obj);
    }

    for (index = 0; index < 10; index++) {
        if (!intgame_load_hotkey(&(stru_6835E0[index]), load_info->stream)) {
            return false;
        }
    }

    return true;
}

// 0x57DC20
void sub_57DC20()
{
    stru_683950.slot = -1;
    stru_683950.type = HOTKEY_ITEM;
    sub_4440E0(inven_ui_drag_item_obj_get(), &(stru_683950.item_obj));
    dword_6839B0 = true;
}

// 0x57DC60
bool hotkey_ui_process_event(TigMessage* msg)
{
    int index;

    switch (msg->type) {
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_RELEASED:
            for (index = 0; index < 2; index++) {
                if (msg->data.button.button_handle == intgame_recent_action_button_get(index)->button_handle) {
                    sub_54FCF0(&(stru_683518[index]));
                    return true;
                }
            }
            for (index = 0; index < 10; index++) {
                if (msg->data.button.button_handle == stru_6835E0[index].info.button_handle) {
                    if (dword_6839B0 || inven_ui_drag_item_obj_get() != OBJ_HANDLE_NULL) {
                        if (inven_ui_drag_item_obj_get() != OBJ_HANDLE_NULL) {
                            sub_57DC20();
                        }
                        sub_57E8D0(TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP);
                    } else {
                        sub_54FCF0(&(stru_6835E0[index]));
                    }
                    return true;
                }
            }
            return false;
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            if (msg->data.button.button_handle == intgame_recent_action_button_get(0)->button_handle) {
                sub_550150(&(stru_683518[0]));
                return true;
            }
            if (msg->data.button.button_handle == intgame_recent_action_button_get(1)->button_handle) {
                sub_550150(&(stru_683518[1]));
                return true;
            }
            index = sub_57E460();
            if (index < 10 && stru_6835E0[index].type) {
                sub_550150(&(stru_6835E0[index]));
                return true;
            }
            return false;
        default:
            return false;
        }
    case TIG_MESSAGE_KEYBOARD:
        if (!textedit_ui_is_focused()
            && !mainmenu_ui_is_active()
            && msg->data.keyboard.key == SDLK_A
            && msg->data.keyboard.pressed) {
            sub_54FCF0(stru_683518);
            gsound_play_sfx(0, 1);
            return true;
        }
        return false;
    default:
        return false;
    }
}

// 0x57DE00
bool sub_57DE00()
{
    return dword_6839B0;
}

// 0x57DE10
bool intgame_save_hotkey(Hotkey* hotkey, TigFile* stream)
{
    if (tig_file_fwrite(&(hotkey->type), sizeof(hotkey->type), 1, stream) != 1) return false;
    if (tig_file_fwrite(&(hotkey->slot), sizeof(hotkey->slot), 1, stream) != 1) return false;

    switch (hotkey->type) {
    case HOTKEY_ITEM:
        if (tig_file_fwrite(&(hotkey->item_obj.field_8.objid), sizeof(hotkey->item_obj.field_8.objid), 1, stream) != 1) return false;
        if (tig_file_fwrite(&(hotkey->count), sizeof(hotkey->count), 1, stream) != 1) return false;
        break;
    case HOTKEY_SKILL:
        if (tig_file_fwrite(&(hotkey->data), sizeof(hotkey->data), 1, stream) != 1) return false;
        break;
    case HOTKEY_SPELL:
        if (tig_file_fwrite(&(hotkey->data), sizeof(hotkey->data), 1, stream) != 1) return false;
        break;
    case HOTKEY_ITEM_SPELL:
        if (tig_file_fwrite(&(hotkey->item_obj.field_8.objid), sizeof(hotkey->item_obj.field_8.objid), 1, stream) != 1) return false;
        if (tig_file_fwrite(&(hotkey->data), sizeof(hotkey->data), 1, stream) != 1) return false;
        break;
    }

    return true;
}

// 0x57DEF0
bool intgame_load_hotkey(Hotkey* hotkey, TigFile* stream)
{
    if (tig_file_fread(&(hotkey->type), sizeof(hotkey->type), 1, stream) != 1) {
        return false;
    }

    if (tig_file_fread(&(hotkey->slot), sizeof(hotkey->slot), 1, stream) != 1) {
        return false;
    }

    hotkey->flags = 0;

    switch (hotkey->type) {
    case HOTKEY_ITEM:
        if (tig_file_fread(&(hotkey->item_obj.field_8.objid), sizeof(hotkey->item_obj.field_8.objid), 1, stream) != 1) {
            return false;
        }

        hotkey->item_obj.obj = objp_perm_lookup(hotkey->item_obj.field_8.objid);
        if (hotkey->item_obj.obj == OBJ_HANDLE_NULL) {
            tig_debug_printf("hotkey_ui: intgame_load_hotkey: ERROR: Load of object FAILED to match!!!\n");
            hotkey->type = HOTKEY_NONE;
        }

        sub_4440E0(hotkey->item_obj.obj, &(hotkey->item_obj));
        hotkey->art_id = sub_554BE0(hotkey->item_obj.obj);

        if (tig_file_fread(&(hotkey->count), sizeof(hotkey->count), 1, stream) != 1) {
            return false;
        }

        break;
    case HOTKEY_SKILL:
        sub_4440E0(OBJ_HANDLE_NULL, &(hotkey->item_obj));

        if (tig_file_fread(&(hotkey->data), sizeof(hotkey->data), 1, stream) != 1) {
            return false;
        }

        hotkey->info.art_num = sub_579F70(hotkey->data);
        tig_art_interface_id_create(hotkey->info.art_num, 0, 0, 0, &(hotkey->art_id));
        hotkey->count = -1;

        break;
    case HOTKEY_SPELL:
        sub_4440E0(OBJ_HANDLE_NULL, &hotkey->item_obj);

        if (tig_file_fread(&(hotkey->data), sizeof(hotkey->data), 1, stream) != 1) {
            return false;
        }

        hotkey->info.art_num = spell_icon(hotkey->data);
        tig_art_interface_id_create(hotkey->info.art_num, 0, 0, 0, &(hotkey->art_id));
        hotkey->count = -1;

        break;
    case HOTKEY_ITEM_SPELL:
        if (tig_file_fread(&(hotkey->item_obj.field_8.objid), sizeof(ObjectID), 1, stream) != 1) {
            return false;
        }

        hotkey->item_obj.obj = objp_perm_lookup(hotkey->item_obj.field_8.objid);
        if (hotkey->item_obj.obj == OBJ_HANDLE_NULL) {
            return false;
        }

        sub_4440E0(hotkey->item_obj.obj, &(hotkey->item_obj));
        hotkey->art_id = sub_554BE0(hotkey->item_obj.obj);

        if (tig_file_fread(&(hotkey->data), sizeof(hotkey->data), 1, stream) != 1) {
            return false;
        }

        hotkey->info.art_num = spell_icon(hotkey->data);
        tig_art_interface_id_create(hotkey->info.art_num, 0, 0, 0, &(hotkey->art_id));
        hotkey->count = -1;

        break;
    }

    if (hotkey->slot != -1) {
        intgame_hotkey_refresh(hotkey->slot);
    }

    return true;
}

// 0x57E140
void intgame_hotkey_refresh(int index)
{
    Hotkey* hotkey;
    tig_art_id_t art_id;
    tig_art_id_t inv_art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    char badge[8];

    if (index == -1) {
        return;
    }

    if (index < 0 || index > 10) {
        tig_debug_printf("intgame_hotkey_refresh: ERROR: hotkey is OUT OF RANGE: %d!\n", index);
        return;
    }

    hotkey = &(stru_6835E0[index]);

    if (tig_art_interface_id_create(dword_5CB494[0], 0, 0, 0, &art_id) == TIG_OK) {
        tig_art_frame_data(art_id, &art_frame_data);

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.x = hotkey->info.x;
        dst_rect.y = 4;
        dst_rect.width = hotkey_slot_width;
        dst_rect.height = hotkey_slot_height;

        art_blit_info.flags = 0;
        art_blit_info.art_id = art_id;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;

        if (intgame_is_compact_interface()) {
            tig_window_fill(hotkey_primary_window_handle, &dst_rect, tig_color_make(0, 0, 0));
        }
        tig_window_blit_art(hotkey_primary_window_handle, &art_blit_info);
    } else {
        tig_debug_printf("Intgame: intgame_hotkey_refresh: ERROR: Can't find Interface Art: %d!\n", dword_5CB494[0]);
    }

    if ((hotkey->flags & HOTKEY_DRAGGED) == 0
        && hotkey->type != HOTKEY_NONE
        && hotkey->art_id != TIG_ART_ID_INVALID) {
        art_id = hotkey->art_id;
        if (hotkey->type == HOTKEY_ITEM) {
            inv_art_id = obj_field_int32_get(hotkey->item_obj.obj, OBJ_F_ITEM_INV_AID);
            if (tig_art_frame_data(inv_art_id, &art_frame_data) == TIG_OK
                && art_frame_data.width < 32
                && art_frame_data.height < 32) {
                art_id = inv_art_id;
            }
        }

        tig_art_frame_data(art_id, &art_frame_data);

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.x = hotkey->info.x;
        dst_rect.y = 4;

        if (art_frame_data.width > hotkey_slot_width) {
            dst_rect.width = hotkey_slot_width;
        } else {
            dst_rect.width = art_frame_data.width;

            if (hotkey_slot_width - art_frame_data.width > 0) {
                dst_rect.x += (hotkey_slot_width - art_frame_data.width) / 2;
            }
        }

        if (art_frame_data.height > hotkey_slot_height) {
            dst_rect.height = hotkey_slot_height;
        } else {
            dst_rect.height = art_frame_data.height;

            if (hotkey_slot_height - art_frame_data.height > 0) {
                dst_rect.y += (hotkey_slot_height - art_frame_data.height) / 2;
            }
        }

        art_blit_info.flags = 0;
        art_blit_info.art_id = art_id;
        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(hotkey_primary_window_handle, &art_blit_info);

        if (hotkey->count != -1) {
            // NOTE: Fixed unbalanced `tig_font_pop` in some code paths.
            if (hotkey->count < 10000) {
                tig_font_push(intgame_morph15_white_font);
                SDL_itoa(hotkey->count, badge, 10);
                dst_rect.x = hotkey->info.x + 2;
                dst_rect.y = 0;
                if (tig_window_text_write(hotkey_primary_window_handle, badge, &dst_rect) != TIG_OK) {
                    tig_debug_printf("intgame_hotkey_refresh: ERROR: Failed to do text write of count: hotkey: %d, count: %s, wid: %d!\n",
                        index,
                        badge,
                        hotkey_primary_window_handle);
                }
                tig_font_pop();
            } else {
                tig_debug_printf("intgame_hotkey_refresh: WARNING: Incorrect item count Suspected, correcting!\n");
                hotkey->count = -1;
            }
        }
    }
}

// 0x57E460
int sub_57E460()
{
    TigMouseState mouse_state;
    TigRect rect;
    int index;

    tig_mouse_get_state(&mouse_state);

    rect.y = 440;
    if (intgame_is_compact_interface()) {
        rect.y = 563;
    }

    rect.x = 0;
    hrp_apply(&rect, GRAVITY_CENTER_HORIZONTAL | GRAVITY_BOTTOM);

    if (mouse_state.x < rect.x + dword_5CB4BC[0] - 5
        || mouse_state.y < rect.y
        || mouse_state.x >= rect.x + dword_5CB4BC[9] + dword_5CB4BC[0] - 5 + 32
        || mouse_state.y >= rect.y + 37) {
        return 11;
    }

    for (index = 0; index < 10; index++) {
        if (mouse_state.x >= rect.x + dword_5CB4BC[index]
            && mouse_state.x < rect.x + dword_5CB4BC[index] + 32
            && mouse_state.y < rect.y + 37) {
            break;
        }
    }

    return index;
}

// 0x57E500
void intgame_hotkey_mouse_load(tig_art_id_t art_id, bool a2)
{
    int dx = 0;
    int dy = 0;
    tig_art_id_t cursor_art_id;
    TigArtFrameData art_frame_data;

    intgame_refresh_cursor();
    if (art_id != TIG_ART_ID_INVALID) {
        if (!a2) {
            dx = 2;
            dy = 2;
        }

        cursor_art_id = tig_mouse_cursor_get_art_id();
        tig_mouse_hide();
        tig_mouse_cursor_set_art_id(art_id);
        tig_mouse_cursor_overlay(cursor_art_id, dx, dy);

        if (a2) {
            tig_mouse_cursor_set_art_id(art_id);
            if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
                tig_debug_printf("intgame_hotkey_mouse_load: ERROR: tig_art_frame_data failed!\n");
                exit(EXIT_FAILURE);
            }

            dy += art_frame_data.height / 2;
        }

        tig_mouse_cursor_set_offset(dx, dy);
        tig_mouse_show();
    }
}

// 0x57E5A0
void sub_57E5A0(Hotkey* hotkey)
{
    hotkey->item_obj.obj = OBJ_HANDLE_NULL;
    hotkey->type = HOTKEY_NONE;
    hotkey->flags = 0;
    hotkey->art_id = TIG_ART_ID_INVALID;
    hotkey->item_obj.field_8.objid.type = OID_TYPE_NULL;
    dword_5CB4E4 = -1;
}

// 0x57E5D0
bool hotkey_ui_begin_drag()
{
    int index;
    Hotkey* hotkey;
    tig_art_id_t art_id;
    UiButtonInfo button_info;
    int spl;
    bool v1 = true;

    if (dword_6839B0) {
        return false;
    }

    stru_683950.item_obj.obj = OBJ_HANDLE_NULL;
    stru_683950.item_obj.field_8.objid.type = OID_TYPE_NULL;

    index = sub_57E460();
    if (index < 10) {
        hotkey = &(stru_6835E0[index]);
        if ((hotkey->flags & HOTKEY_DRAGGED) != 0) {
            return false;
        }

        if (hotkey->type != HOTKEY_NONE) {
            dword_5CB4E4 = index;
            switch (hotkey->type) {
            case HOTKEY_ITEM:
                stru_683950.type = hotkey->type;
                stru_683950.item_obj = hotkey->item_obj;
                sub_573630(stru_683950.item_obj.obj);
                art_id = TIG_ART_ID_INVALID;
                v1 = false;
                break;
            case HOTKEY_SKILL:
                stru_683950.type = hotkey->type;
                stru_683950.data = hotkey->data;
                tig_art_interface_id_create(sub_579F70(stru_683950.data), 0, 0, 0, &art_id);
                break;
            case HOTKEY_SPELL:
                stru_683950.type = hotkey->type;
                stru_683950.data = hotkey->data;
                tig_art_interface_id_create(spell_icon(stru_683950.data), 0, 0, 0, &art_id);
                break;
            case HOTKEY_ITEM_SPELL:
                stru_683950.type = hotkey->type;
                stru_683950.item_obj = hotkey->item_obj;
                stru_683950.data = hotkey->data;
                tig_art_interface_id_create(spell_icon(stru_683950.data), 0, 0, 0, &art_id);
                break;
            default:
                // Should be unreachable.
                assert(0);
            }

            stru_683950.slot = index;
            hotkey->flags |= HOTKEY_DRAGGED;
            intgame_hotkey_refresh(index);
            intgame_hotkey_mouse_load(art_id, v1);
            dword_6839B0 = true;
            return true;
        }
    }

    if (!intgame_is_compact_interface()) {
        index = sub_557CF0();
        if (index < 5) {
            stru_683950.type = HOTKEY_SPELL;
            stru_683950.data = index + 5 * sub_557AB0();
            sub_557AC0(sub_557AB0(), index, &button_info);
            tig_art_interface_id_create(button_info.art_num, 0, 0, 0, &art_id);
            intgame_hotkey_mouse_load(art_id, true);
            dword_6839B0 = true;
            return true;
        }

        index = sub_557B60();
        if (index < 4) {
            stru_683950.type = HOTKEY_SKILL;
            stru_683950.data = index;
            tig_art_interface_id_create(sub_579F70(index), 0, 0, 0, &art_id);
            intgame_hotkey_mouse_load(art_id, true);
            dword_6839B0 = true;
            return true;
        }

        index = sub_557C00();
        if (index < 5) {
            spl = mt_item_spell(sub_557B00(), index);
            stru_683950.type = HOTKEY_ITEM_SPELL;
            sub_4440E0(sub_557B00(), &(stru_683950.item_obj));
            stru_683950.data = spl;
            stru_683950.info.art_num = spell_icon(spl);
            tig_art_interface_id_create(stru_683950.info.art_num, 0, 0, 0, &art_id);
            intgame_hotkey_mouse_load(art_id, true);
            dword_6839B0 = true;
            return true;
        }
    }

    return true;
}

// 0x57E8B0
void sub_57E8B0()
{
    sub_575770();
    intgame_refresh_cursor();
    dword_6839B0 = false;
    dword_5CB4E4 = -1;
}

// 0x57E8D0
bool sub_57E8D0(TigMessageMouseEvent mouse_event)
{
    int64_t parent_obj;
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    int obj_type;
    int index;
    Hotkey* hotkey;
    int sound_id;
    int64_t v2;
    int64_t v3;
    unsigned int flags;

    if (!dword_6839B0) {
        dword_5CB4E4 = -1;
        return false;
    }

    if (mouse_event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP
        && inven_ui_drag_item_obj_get() == OBJ_HANDLE_NULL) {
        return false;
    }

    sub_573730();

    if (stru_683950.item_obj.obj != OBJ_HANDLE_NULL) {
        if (item_parent(stru_683950.item_obj.obj, &parent_obj)
            && !player_is_local_pc_obj(parent_obj)) {
            dword_6839B0 = false;
            sub_575770();
            intgame_refresh_cursor();
            dword_5CB4E4 = -1;
            return false;
        }

        obj_type = obj_field_int32_get(stru_683950.item_obj.obj, OBJ_F_TYPE);
        if (obj_type == OBJ_TYPE_AMMO
            || obj_type == OBJ_TYPE_GOLD) {
            dword_6839B0 = false;
            sub_575770();
            intgame_refresh_cursor();
            dword_5CB4E4 = -1;
            return false;
        }

        if (sub_462A30(parent_obj, stru_683950.item_obj.obj)) {
            mes_file_entry.num = 4000; // "You already have a duplicate item on the hotkey bank."
            mes_get_msg(intgame_hotkey_mes_file(), &mes_file_entry);

            ui_message.type = UI_MSG_TYPE_FEEDBACK;
            ui_message.str = mes_file_entry.str;
            sub_550750(&ui_message);

            dword_6839B0 = false;
            sub_575770();
            intgame_refresh_cursor();
            dword_5CB4E4 = -1;
            return false;
        }
    }

    index = sub_57E460();
    if (index < 10) {
        hotkey = &(stru_6835E0[index]);
        if (dword_5CB4E4 == index) {
            sound_id = sfx_item_sound(hotkey->item_obj.obj, player_get_local_pc_obj(), OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
            if (sound_id != -1) {
                gsound_play_sfx(sound_id, 1);
            }

            hotkey->flags &= ~HOTKEY_DRAGGED;
            intgame_hotkey_refresh(index);

            dword_6839B0 = false;
            sub_573840();
            intgame_refresh_cursor();
            dword_5CB4E4 = -1;
            return true;
        }

        if (dword_5CB4E4 != -1) {
            sub_57E5A0(&(stru_6835E0[dword_5CB4E4]));
        }

        v2 = hotkey->item_obj.obj;
        v3 = OBJ_HANDLE_NULL; // NOTE: To silence compiler warnings.
        if (v2 != OBJ_HANDLE_NULL) {
            if (item_parent(v2, &v3)) {
                item_drop(v2);
            } else {
                v2 = OBJ_HANDLE_NULL;
            }
        }

        if (dword_5CB4E4 != -1) {
            if (hotkey->type == HOTKEY_NONE) {
                intgame_hotkey_refresh(dword_5CB4E4);
            } else {
                sub_57ED60(hotkey, dword_5CB4E4);
            }
            dword_5CB4E4 = -1;
        }

        hotkey->count = -1;
        if (hotkey->info.button_handle != TIG_BUTTON_HANDLE_INVALID) {
            sub_57F210(index);
        }

        hotkey->type = stru_683950.type;

        switch (stru_683950.type) {
        case HOTKEY_ITEM:
            sub_573840();
            item_inventory_location_get(stru_683950.item_obj.obj); // FIXME: Useless.
            flags = obj_field_int32_get(stru_683950.item_obj.obj, OBJ_F_ITEM_FLAGS);
            flags &= ~OIF_NO_DISPLAY;
            obj_field_int32_set(stru_683950.item_obj.obj, OBJ_F_ITEM_FLAGS, flags);
            item_location_set(stru_683950.item_obj.obj, hotkey->slot + 2000);
            inven_ui_update(player_get_local_pc_obj());
            hotkey->item_obj = stru_683950.item_obj;
            hotkey->art_id = sub_554BE0(stru_683950.item_obj.obj);
            hotkey->count = item_count_items_matching_prototype(player_get_local_pc_obj(), hotkey->item_obj.obj);
            break;
        case HOTKEY_SKILL:
            hotkey->data = stru_683950.data;
            hotkey->info.art_num = sub_579F70(stru_683950.data);
            tig_art_interface_id_create(hotkey->info.art_num, 0, 0, 0, &(hotkey->art_id));
            break;
        case HOTKEY_SPELL:
            hotkey->data = stru_683950.data;
            hotkey->info.art_num = spell_icon(stru_683950.data);
            tig_art_interface_id_create(hotkey->info.art_num, 0, 0, 0, &(hotkey->art_id));
            break;
        case HOTKEY_ITEM_SPELL:
            hotkey->item_obj = stru_683950.item_obj;
            hotkey->data = stru_683950.data;
            hotkey->info.art_num = spell_icon(stru_683950.data);
            tig_art_interface_id_create(hotkey->info.art_num, 0, 0, 0, &(hotkey->art_id));
            break;
        }

        if (v2 != OBJ_HANDLE_NULL && v3 != OBJ_HANDLE_NULL) {
            item_transfer(v2, v3);
        }

        hotkey->type = stru_683950.type;
        hotkey->flags = 0;
        intgame_hotkey_refresh(index);
    } else {
        if (stru_683950.type == HOTKEY_ITEM) {
            if (inven_ui_is_created()) {
                dword_6839B0 = false;
                dword_5CB4E4 = -1;
            }

            return false;
        }
    }

    if (stru_683950.item_obj.obj != OBJ_HANDLE_NULL) {
        sound_id = sfx_item_sound(stru_683950.item_obj.obj, player_get_local_pc_obj(), OBJ_HANDLE_NULL, ITEM_SOUND_DROP);
        if (sound_id != -1) {
            gsound_play_sfx(sound_id, 1);
        }
    }

    dword_6839B0 = false;
    intgame_refresh_cursor();
    dword_5CB4E4 = -1;

    return true;
}

// 0x57ED60
void sub_57ED60(Hotkey* hotkey, int a2)
{
    stru_6835E0[dword_5CB4E4].flags = 0;
    stru_6835E0[dword_5CB4E4].type = hotkey->type;
    stru_6835E0[dword_5CB4E4].data = hotkey->data;
    intgame_hotkey_refresh(a2);
}

// 0x57EDA0
bool sub_57EDA0(TigMessageMouseEvent mouse_event)
{
    if (inven_ui_drag_item_obj_get() != OBJ_HANDLE_NULL) {
        stru_683950.slot = -1;
        stru_683950.type = HOTKEY_ITEM;
        sub_4440E0(inven_ui_drag_item_obj_get(), &(stru_683950.item_obj));
        dword_6839B0 = true;
    }

    return sub_57E8D0(mouse_event);
}

// 0x57EDF0
void sub_57EDF0(int64_t obj, int64_t a2, int a3)
{
    if (player_is_local_pc_obj(obj)) {
        sub_57EE30(a2, a3);
    } else {
        item_location_set(a2, a3);
    }
}

// 0x57EE30
void sub_57EE30(int64_t obj, int inventory_location)
{
    int64_t v1 = OBJ_HANDLE_NULL;
    Hotkey* hotkey;

    hotkey = &(stru_6835E0[inventory_location - 2000]);

    if (hotkey->type != HOTKEY_NONE) {
        if (hotkey->item_obj.obj != OBJ_HANDLE_NULL) {
            item_parent(hotkey->item_obj.obj, &v1);
            item_drop(hotkey->item_obj.obj);
            if (v1 != OBJ_HANDLE_NULL) {
                item_transfer(hotkey->item_obj.obj, v1);
            }
        }
    }

    item_location_set(obj, inventory_location);
    sub_57EED0(obj, inventory_location);
}

// 0x57EED0
bool sub_57EED0(int64_t obj, int inventory_location)
{
    int64_t v1 = OBJ_HANDLE_NULL;
    Hotkey* hotkey;

    hotkey = &(stru_6835E0[inventory_location - 2000]);

    if (hotkey->type != HOTKEY_NONE) {
        if (hotkey->item_obj.obj != OBJ_HANDLE_NULL) {
            item_parent(hotkey->item_obj.obj, &v1);
            item_drop(hotkey->item_obj.obj);
            if (v1 != OBJ_HANDLE_NULL) {
                item_transfer(hotkey->item_obj.obj, v1);
            }
        }
    }

    hotkey->type = HOTKEY_ITEM;
    sub_4440E0(obj, &(hotkey->item_obj));
    hotkey->art_id = sub_554BE0(obj);
    hotkey->count = item_count_items_matching_prototype(player_get_local_pc_obj(), obj);
    intgame_hotkey_refresh(hotkey->slot);

    return true;
}

// 0x57EF90
void sub_57EF90(int index)
{
    sub_57F210(index);
}

// 0x57EFA0
void sub_57EFA0(int type, int data, int64_t item_obj)
{
    tig_art_id_t new_art_id;
    tig_art_id_t old_art_id;
    tig_button_handle_t button_handle;

    switch (type) {
    case HOTKEY_ITEM:
        if (stru_683518[0].type == HOTKEY_ITEM
            && stru_683518[0].item_obj.obj == item_obj) {
            return;
        }
        // FIXME: `new_art_id` is never set in this code path.
        new_art_id = TIG_ART_ID_INVALID;
        break;
    case HOTKEY_SKILL:
        if (stru_683518[0].type == HOTKEY_SKILL
            && stru_683518[0].data == data) {
            return;
        }
        tig_art_interface_id_create(sub_579F70(data), 0, 0, 0, &new_art_id);
        break;
    case HOTKEY_SPELL:
        if (stru_683518[0].type == HOTKEY_SPELL
            && stru_683518[0].data == data) {
            return;
        }
        tig_art_interface_id_create(spell_icon(data), 0, 0, 0, &new_art_id);
        break;
    case HOTKEY_ITEM_SPELL:
        if (stru_683518[0].type == HOTKEY_ITEM_SPELL
            && stru_683518[0].data == data
            && stru_683518[0].item_obj.obj == item_obj) {
            return;
        }
        tig_art_interface_id_create(spell_icon(data), 0, 0, 0, &new_art_id);
        break;
    default:
        return;
    }

    stru_683518[1].type = stru_683518[0].type;
    stru_683518[1].data = stru_683518[0].data;
    sub_4440E0(stru_683518[0].item_obj.obj, &stru_683518[1].item_obj);
    intgame_recent_action_button_get(1)->art_num = intgame_recent_action_button_get(0)->art_num;
    button_handle = intgame_recent_action_button_get(1)->button_handle;
    if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
        tig_art_interface_id_create(intgame_recent_action_button_get(1)->art_num, 0, 0, 0, &old_art_id);
        tig_button_set_art(button_handle, old_art_id);
    }

    stru_683518[0].type = type;
    stru_683518[0].data = data;
    sub_4440E0(item_obj, &stru_683518[1].item_obj); // FIXME: Probably wrong, should be [0].
    intgame_recent_action_button_get(0)->art_num = tig_art_num_get(new_art_id);
    button_handle = intgame_recent_action_button_get(0)->button_handle;
    if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
        tig_button_set_art(button_handle, new_art_id);
    }
}

// 0x57F160
bool button_create_no_art(UiButtonInfo* button_info, int width, int height)
{
    TigButtonData button_data;

    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = hotkey_primary_window_handle;
    button_data.x = button_info->x;
    button_data.y = button_info->y - 441;
    button_data.art_id = TIG_ART_ID_INVALID;
    button_data.width = width;
    button_data.height = height;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;
    return tig_button_create(&button_data, &(button_info->button_handle)) == TIG_OK;
}

// 0x57F1D0
void sub_57F1D0(int index)
{
    Hotkey* hotkey;

    hotkey = &(stru_6835E0[index]);

    if ((hotkey->flags & HOTKEY_DRAGGED) != 0) {
        return;
    }

    if (hotkey->type == HOTKEY_NONE) {
        return;
    }

    gsound_play_sfx(0, 1);
    sub_54FCF0(hotkey);
}

// 0x57F210
void sub_57F210(int index)
{
    sub_57E5A0(&(stru_6835E0[index]));
    intgame_hotkey_refresh(index);
}

// 0x57F240
Hotkey* sub_57F240(int index)
{
    return &(stru_6835E0[index]);
}

// 0x57F260
bool sub_57F260()
{
    bool ret = false;
    int index;
    Hotkey* hotkey;

    for (index = 0; index < 10; index++) {
        hotkey = sub_57F240(index);
        if (hotkey->type == HOTKEY_ITEM || hotkey->type == HOTKEY_ITEM_SPELL) {
            if ((hotkey->flags & HOTKEY_DRAGGED) != 0) {
                hotkey->flags &= ~HOTKEY_DRAGGED;
                intgame_hotkey_refresh(index);
                ret = true;
            }
        }
    }

    dword_6839B0 = 0;
    dword_5CB4E4 = -1;

    return ret;
}

// 0x57F2C0
void hotkey_ui_notify_item_inserted_or_removed(int64_t item_obj, bool removed)
{
    int index;
    Hotkey* hotkey;

    if (removed) {
        for (index = 0; index < 2; index++) {
            hotkey = &(stru_683518[index]);
            if (hotkey->item_obj.obj == item_obj) {
                sub_57E5A0(hotkey);
                sub_57EFA0(2, sub_557B50(index), hotkey->item_obj.obj);
            }
        }
    }
}

// 0x57F340
void hotkey_ui_notify_spell_removed(int spell)
{
    int index;
    Hotkey* hotkey;

    for (index = 0; index < 2; index++) {
        hotkey = &(stru_683518[index]);
        if (hotkey->type == HOTKEY_SPELL && hotkey->data == spell) {
            sub_57E5A0(hotkey);
            hotkey->data = sub_557B50(index);
            sub_57EFA0(HOTKEY_SKILL, hotkey->data, OBJ_HANDLE_NULL);
        }
    }

    for (index = 0; index < 10; index++) {
        hotkey = sub_57F240(index);
        if (hotkey->type == HOTKEY_SPELL && hotkey->data == spell) {
            sub_57E5A0(hotkey);
            intgame_hotkey_refresh(hotkey->slot);
        }
    }
}

// 0x57F3C0
void intgame_hotkeys_recover()
{
    int index;
    tig_button_handle_t button_handle;
    tig_art_id_t art_id;
    Hotkey* hotkey;

    for (index = 0; index < 2; index++) {
        hotkey = &(stru_683518[index]);
        if ((hotkey->type == HOTKEY_ITEM || hotkey->type == HOTKEY_ITEM_SPELL)
            && !sub_444130(&(hotkey->item_obj))) {
            if (!gamelib_in_reset()) {
                tig_debug_printf("Intgame: intgame_hotkeys_recover: ERROR: Active Item Hotkey %d failed to recover!\n", index);
            }

            hotkey->type = HOTKEY_SKILL;
            hotkey->data = sub_557B50(index);
            intgame_recent_action_button_get(index)->art_num = sub_579F70(hotkey->data);

            button_handle = intgame_recent_action_button_get(index)->button_handle;
            if (button_handle != TIG_BUTTON_HANDLE_INVALID) {
                tig_art_interface_id_create(intgame_recent_action_button_get(index)->art_num, 0, 0, 0, &art_id);
                tig_button_set_art(button_handle, art_id);
            }
        }
    }

    for (index = 0; index < 10; index++) {
        hotkey = sub_57F240(index);
        if (hotkey->type == HOTKEY_ITEM || hotkey->type == HOTKEY_ITEM_SPELL) {
            if (hotkey->item_obj.obj != OBJ_HANDLE_NULL) {
                if (!sub_444130(&(hotkey->item_obj))) {
                    tig_debug_printf("Intgame: intgame_hotkeys_recover: ERROR: Item Hotkey %d failed to recover!\n", index);
                    sub_57F210(index);
                }
            }
        }
    }
}
