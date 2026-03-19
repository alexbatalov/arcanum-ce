#include "ui/compact_ui.h"

#include <stdio.h>

#include "game/critter.h"
#include "game/hrp.h"
#include "game/player.h"
#include "game/stat.h"
#include "game/timeevent.h"
#include "ui/anim_ui.h"
#include "ui/dialog_ui.h"
#include "ui/intgame.h"
#include "ui/mainmenu_ui.h"

#define MAX_COMPONENTS 2

typedef struct CompactUiComponent CompactUiComponent;

typedef bool(CompactUiComponentInit)(CompactUiComponent* comp);
typedef void(CompactUiComponentDraw)(int a1);

typedef struct CompactUiComponent {
    /* 0000 */ const char* name;
    /* 0004 */ tig_window_handle_t window_handle;
    /* 0008 */ CompactUiComponentInit* init;
    /* 000C */ void (*field_C)(void);
    /* 0010 */ TigWindowMessageFilterFunc* message_filter;
    /* 0014 */ CompactUiComponentDraw* draw;
    /* 0018 */ TigRect rect;
    Gravity gravity;
    /* 0028 */ int r;
    /* 002C */ int g;
    /* 0030 */ int b;
    /* 0034 */ int field_34;
} CompactUiComponent;

static bool compact_ui_health_bar_init(CompactUiComponent* comp);
static void sub_569050(void);
static bool compact_ui_health_bar_message_filter(TigMessage* msg);
static void compact_ui_health_bar_draw(int a1);
static void sub_569550(tig_window_handle_t window_handle, TigRect* rect, int color);
static bool compact_ui_hotkey_bar_init(CompactUiComponent* comp);
static void sub_569580(void);
static bool compact_ui_hotkey_bar_message_filter(TigMessage* msg);
static void compact_ui_hotkey_bar_draw(int a1);

// 0x5CA198
static tig_window_handle_t compact_ui_message_window_handle = TIG_WINDOW_HANDLE_INVALID;

// 0x5CA1A0
static TigRect compact_ui_message_window_frame = { 210, 479, 385, 84 };

// 0x5CA1B0
static CompactUiComponent compact_ui_components[MAX_COMPONENTS] = {
    {
        "Health/Fatigue Bar",
        TIG_WINDOW_HANDLE_INVALID,
        compact_ui_health_bar_init,
        sub_569050,
        compact_ui_health_bar_message_filter,
        compact_ui_health_bar_draw,
        { 751, 498, 49, 102 },
        GRAVITY_RIGHT | GRAVITY_BOTTOM,
        0,
        155,
        0,
        0,
    },
    {
        "Hotkey Bar",
        TIG_WINDOW_HANDLE_INVALID,
        compact_ui_hotkey_bar_init,
        sub_569580,
        compact_ui_hotkey_bar_message_filter,
        compact_ui_hotkey_bar_draw,
        { 25, 25, 25, 25 },
        GRAVITY_LEFT | GRAVITY_TOP,
        0,
        0,
        0,
        0,
    },
};

// 0x67B968
static bool compact_ui_initialized;

// 0x67B96C
static bool compact_ui_created;

// 0x5689D0
bool compact_ui_init(GameInitInfo* init_info)
{
    int index;

    (void)init_info;

    for (index = 0; index < MAX_COMPONENTS; index++) {
        compact_ui_components[index].window_handle = TIG_WINDOW_HANDLE_INVALID;
    }

    compact_ui_created = false;
    compact_ui_initialized = true;

    return true;
}

// 0x568A00
void compact_ui_exit(void)
{
    int index;

    for (index = 0; index < MAX_COMPONENTS; index++) {
        if (compact_ui_components[index].window_handle != TIG_WINDOW_HANDLE_INVALID) {
            tig_window_destroy(compact_ui_components[index].window_handle);
        }
    }

    compact_ui_initialized = false;
}

// 0x568A30
void compact_ui_reset(void)
{
}

// 0x568A40
bool compact_ui_create(void)
{
    int index;
    CompactUiComponent* comp;
    TigWindowData window_data;

    if (!compact_ui_initialized) {
        // FIXME: Should be false?
        return true;
    }

    if (compact_ui_created) {
        compact_ui_destroy();
        return true;
    }

    for (index = 0; index < MAX_COMPONENTS; index++) {
        comp = &(compact_ui_components[index]);

        window_data.flags = TIG_WINDOW_MESSAGE_FILTER | TIG_WINDOW_TRANSPARENT;
        window_data.rect = comp->rect;
        window_data.background_color = tig_color_make(0, 0, 0);
        window_data.color_key = tig_color_make(comp->r, comp->g, comp->b);
        window_data.message_filter = comp->message_filter;
        hrp_apply(&(window_data.rect), comp->gravity);

        if (tig_window_create(&window_data, &(comp->window_handle)) != TIG_OK) {
            tig_debug_printf("Compact_UI: could not create window component: %s\n", comp->name);
            return false;
        }

        if (!comp->init(comp)) {
            tig_debug_printf("Compact_UI: could not initialize window component: %s\n", comp->name);
            return false;
        }

        comp->draw(0);
    }

    window_data.rect = compact_ui_message_window_frame;
    window_data.flags = 0;
    window_data.color_key = tig_color_make(5, 5, 5);
    hrp_apply(&(window_data.rect), GRAVITY_CENTER_HORIZONTAL | GRAVITY_BOTTOM);

    if (tig_window_create(&window_data, &compact_ui_message_window_handle) != TIG_OK) {
        return false;
    }

    tig_window_hide(compact_ui_message_window_handle);

    compact_ui_created = true;

    return true;
}

// 0x568C90
bool compact_ui_destroy(void)
{
    int index;

    for (index = 0; index < MAX_COMPONENTS; index++) {
        if (compact_ui_components[index].window_handle != TIG_WINDOW_HANDLE_INVALID) {
            tig_window_destroy(compact_ui_components[index].window_handle);
            compact_ui_components[index].window_handle = TIG_WINDOW_HANDLE_INVALID;
        }
    }

    if (compact_ui_message_window_handle != TIG_WINDOW_HANDLE_INVALID) {
        tig_window_destroy(compact_ui_message_window_handle);
        compact_ui_message_window_handle = TIG_WINDOW_HANDLE_INVALID;
    }

    compact_ui_created = false;

    return true;
}

// 0x568CF0
void compact_ui_draw(void)
{
    int index;

    if (!compact_ui_created) {
        return;
    }

    for (index = 0; index < MAX_COMPONENTS; index++) {
        if (compact_ui_components[index].draw != NULL) {
            compact_ui_components[index].draw(0);
        }
    }
}

// 0x568D20
tig_window_handle_t compact_ui_message_window_acquire(void)
{
    TigRect rect;

    if (compact_ui_message_window_handle == TIG_WINDOW_HANDLE_INVALID) {
        return TIG_WINDOW_HANDLE_INVALID;
    }

    if (tig_window_is_hidden(compact_ui_message_window_handle)) {
        rect.x = 0;
        rect.y = 0;
        rect.width = compact_ui_message_window_frame.width;
        rect.height = compact_ui_message_window_frame.height;

        tig_window_fill(compact_ui_message_window_handle, &rect, tig_color_make(5, 5, 5));
        tig_window_box(compact_ui_message_window_handle, &rect, tig_color_make(9, 9, 9));

        if (!dialog_ui_is_in_dialog(player_get_local_pc_obj())) {
            if (tig_window_show(compact_ui_message_window_handle) != TIG_OK) {
                return TIG_WINDOW_HANDLE_INVALID;
            }
        }
    }

    anim_ui_event_remove(ANIM_UI_EVENT_TYPE_HIDE_COMPACT_UI, -1);
    anim_ui_event_add_delay(ANIM_UI_EVENT_TYPE_HIDE_COMPACT_UI, -1, 3000);

    return compact_ui_message_window_handle;
}

// 0x568E70
void compact_ui_message_window_box(void)
{
    TigRect rect;

    if (compact_ui_message_window_handle == TIG_WINDOW_HANDLE_INVALID) {
        return;
    }

    if (tig_window_is_hidden(compact_ui_message_window_handle)) {
        return;
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = compact_ui_message_window_frame.width;
    rect.height = compact_ui_message_window_frame.height;
    tig_window_box(compact_ui_message_window_handle, &rect, tig_color_make(9, 9, 9));
}

// 0x568F20
void compact_ui_message_window_hide(void)
{
    if (compact_ui_message_window_handle != TIG_WINDOW_HANDLE_INVALID) {
        tig_window_hide(compact_ui_message_window_handle);
    }
}

// 0x568F40
void compact_ui_message_window_release(void)
{
    TigRect rect;

    if (compact_ui_message_window_handle != TIG_WINDOW_HANDLE_INVALID) {
        rect.x = 0;
        rect.y = 0;
        rect.width = compact_ui_message_window_frame.width;
        rect.height = compact_ui_message_window_frame.height;

        tig_window_fill(compact_ui_message_window_handle, &rect, tig_color_make(5, 5, 5));
        tig_window_box(compact_ui_message_window_handle, &rect, tig_color_make(9, 9, 9));
    }

    anim_ui_event_remove(ANIM_UI_EVENT_TYPE_HIDE_COMPACT_UI, -1);
    compact_ui_message_window_hide();
}

// 0x569040
bool compact_ui_health_bar_init(CompactUiComponent* comp)
{
    (void)comp;

    return true;
}

// 0x569050
void sub_569050(void)
{
}

// 0x569060
bool compact_ui_health_bar_message_filter(TigMessage* msg)
{
    return sub_54B5D0(msg);
}

// 0x569070
void compact_ui_health_bar_draw(int a1)
{
    TigRect rect1;
    TigRect rect2;
    TigLine line;
    TigFont font_desc;
    char str[12];
    int64_t pc_obj;
    int poison;

    (void)a1;

    rect1.x = 0;
    rect1.y = 0;
    rect1.width = 49;
    rect1.height = 102;
    tig_window_fill(compact_ui_components[0].window_handle,
        &rect1,
        compact_ui_components[0].field_34);

    rect2.x = 0;
    rect2.y = 0;
    rect2.width = rect1.width;
    rect2.height = rect1.height;
    tig_window_box(compact_ui_components[0].window_handle,
        &rect2,
        tig_color_make(9, 9, 9));

    line.x1 = 24;
    line.y1 = 1;
    line.x2 = 24;
    line.y2 = rect1.height - 2;
    tig_window_line(compact_ui_components[0].window_handle,
        &line,
        tig_color_make(9, 9, 9));

    pc_obj = player_get_local_pc_obj();

    rect1.x = 1;
    rect1.width = 23;
    rect1.y = 100 * (100 * object_hp_damage_get(pc_obj) / object_hp_max(pc_obj)) / 100 + 1;
    if (rect1.y > 100) {
        rect1.y = 100;
    }
    rect1.height = 100 - rect1.y;

    poison = stat_level_get(pc_obj, STAT_POISON_LEVEL);
    if (poison > 0) {
        sub_569550(compact_ui_components[0].window_handle,
            &rect1,
            tig_color_make(0, 255, 0));

        sprintf(str, "%02d", poison);
        tig_font_push(mainmenu_ui_font(MM_FONT_FLARE12, MM_COLOR_WHITE));
        font_desc.width = 0;
        font_desc.str = str;
        tig_font_measure(&font_desc);
        rect1.x += (23 - font_desc.width) / 2;
        rect1.y += 40;
        rect1.width = font_desc.width;
        rect1.height = font_desc.height;
        tig_window_text_write(compact_ui_components[0].window_handle, str, &rect1);
        tig_font_pop();
        rect1.y -= 40;
    } else {
        sub_569550(compact_ui_components[0].window_handle,
            &rect1,
            tig_color_make(255, 0, 0));
    }

    sprintf(str, "%02d", object_hp_current(pc_obj));
    tig_font_push(mainmenu_ui_font(MM_FONT_FLARE12, MM_COLOR_WHITE));
    font_desc.width = 0;
    font_desc.str = str;
    tig_font_measure(&font_desc);
    rect1.x += (23 - font_desc.width) / 2;
    rect1.width = font_desc.width;
    rect1.height = font_desc.height;
    tig_window_text_write(compact_ui_components[0].window_handle, str, &rect1);
    tig_font_pop();

    rect1.x = 25;
    rect1.width = 23;
    rect1.y = 100 * (100 * critter_fatigue_damage_get(pc_obj) / critter_fatigue_max(pc_obj)) / 100 + 1;
    if (rect1.y > 100) {
        rect1.y = 100;
    }
    rect1.height = 100 - rect1.y;
    sub_569550(compact_ui_components[0].window_handle,
        &rect1,
        tig_color_make(0, 0, 255));

    sprintf(str, "%02d", critter_fatigue_current(pc_obj));
    tig_font_push(mainmenu_ui_font(MM_FONT_FLARE12, MM_COLOR_WHITE));
    font_desc.width = 0;
    font_desc.str = str;
    tig_font_measure(&font_desc);
    rect1.x += (23 - font_desc.width) / 2;
    rect1.width = font_desc.width;
    rect1.height = font_desc.height;
    tig_window_text_write(compact_ui_components[0].window_handle,
        str,
        &rect1);
    tig_font_pop();
}

// 0x569550
void sub_569550(tig_window_handle_t window_handle, TigRect* rect, int color)
{
    tig_window_fill(window_handle, rect, color);
}

// 0x569570
bool compact_ui_hotkey_bar_init(CompactUiComponent* comp)
{
    (void)comp;

    return true;
}

// 0x569580
void sub_569580(void)
{
}

// 0x569590
bool compact_ui_hotkey_bar_message_filter(TigMessage* msg)
{
    (void)msg;

    return false;
}

// 0x5695A0
void compact_ui_hotkey_bar_draw(int a1)
{
    (void)a1;
}
