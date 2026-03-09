#ifndef ARCANUM_UI_TYPES_H_
#define ARCANUM_UI_TYPES_H_

#include <tig/tig.h>

#include "game/obj.h"
#include "game/object.h"

typedef struct UiButtonInfo {
    int x;
    int y;
    int art_num;
    tig_button_handle_t button_handle;
} UiButtonInfo;

typedef struct MainMenuButtonInfo {
    /* 0000 */ int x;
    /* 0004 */ int y;
    /* 0008 */ int art_num;
    /* 000C */ tig_button_handle_t button_handle;
    /* 0010 */ int field_10;
    /* 0014 */ int field_14;
    /* 0018 */ int flags;
    /* 001C */ TigRect rect;
    /* 002C */ int field_2C;
} MainMenuButtonInfo;

typedef struct MainMenuWindowInfoF3C {
    int field_0;
    int x;
    int y;
} MainMenuWindowInfoF3C;

typedef struct MainMenuWindowInfo {
    /* 0000 */ int background_art_num;
    /* 0004 */ void (*init_func)(void);
    /* 0008 */ void (*exit_func)(void);
    /* 000C */ unsigned int flags;
    /* 0010 */ bool (*button_press_func)(tig_button_handle_t button_handle);
    /* 0014 */ bool (*button_release_func)(tig_button_handle_t button_handle);
    /* 0018 */ bool (*button_hover_func)(tig_button_handle_t button_handle);
    /* 001C */ bool (*button_leave_func)(tig_button_handle_t button_handle);
    /* 0020 */ void (*mouse_idle_func)(int x, int y);
    /* 0024 */ int num;
    /* 0028 */ int num_buttons;
    /* 002C */ MainMenuButtonInfo* buttons;
    /* 0030 */ int field_30;
    /* 0034 */ int field_34;
    /* 0038 */ int refresh_text_flags;
    /* 003C */ MainMenuWindowInfoF3C field_3C[2];
    /* 0054 */ void (*refresh_func)(TigRect* rect);
    /* 0058 */ bool (*execute_func)(int btn);
    /* 005C */ TigRect content_rect;
    /* 006C */ void (*mouse_up_func)(int x, int y);
    /* 0070 */ TigRect scrollbar_rect;
    /* 0080 */ void (*mouse_down_func)(int x, int y);
    /* 0084 */ int top_index;
    /* 0088 */ int max_top_index;
    /* 008C */ int cnt;
    /* 0090 */ int selected_index;
    /* 0094 */ int field_94;
} MainMenuWindowInfo;

typedef uint32_t HotkeyFlags;

#define HOTKEY_DRAGGED 0x0001u

typedef enum HotkeyType {
    HOTKEY_NONE,
    HOTKEY_ITEM,
    HOTKEY_SKILL,
    HOTKEY_SPELL,
    HOTKEY_ITEM_SPELL,
} HotkeyType;

typedef struct Hotkey {
    /* 0000 */ int slot;
    /* 0004 */ HotkeyFlags flags;
    /* 0008 */ HotkeyType type;
    /* 000C */ int data;
    /* 0010 */ FollowerInfo item_obj;
    /* 0040 */ int count;
    /* 0044 */ tig_art_id_t art_id;
    /* 0048 */ UiButtonInfo info;
} Hotkey;

#endif /* ARCANUM_UI_TYPES_H_ */
