#include "ui/schematic_ui.h"

#include <stdio.h>

#include "game/critter.h"
#include "game/gsound.h"
#include "game/item.h"
#include "game/mes.h"
#include "game/obj.h"
#include "game/object.h"
#include "game/player.h"
#include "game/proto.h"
#include "game/snd.h"
#include "game/tech.h"
#include "ui/intgame.h"
#include "ui/tb_ui.h"

#define SCHEMATIC_F_NAME 0
#define SCHEMATIC_F_DESCRIPTION 1
#define SCHEMATIC_F_ART_NUM 2
#define SCHEMATIC_F_ITEM_1 3
#define SCHEMATIC_F_ITEM_2 4
#define SCHEMATIC_F_PROD 5
#define SCHEMATIC_F_QTY 6

typedef enum SchematicUiButton {
    SCHEMATIC_UI_BUTTON_PREV,
    SCHEMATIC_UI_BUTTON_NEXT,
    SCHEMATIC_UI_BUTTON_COMBINE,
    SCHEMATIC_UI_BUTTON_LEARNED,
    SCHEMATIC_UI_BUTTON_FOUND,
    SCHEMATIC_UI_BUTTON_COUNT,
} SchematicUiButton;

typedef enum SchematicUiView {
    SCHEMATIC_UI_VIEW_LEARNED,
    SCHEMATIC_UI_VIEW_FOUND,
} SchematicUiView;

typedef enum SchematicUiReadiness {
    SCHEMATIC_UI_READINESS_OK,
    SCHEMATIC_UI_READINESS_NO_ITEMS,
    SCHEMATIC_UI_READINESS_NO_EXPERTISE,
    SCHEMATIC_UI_READINESS_COUNT,
} SchematicUiReadiness;

static void sub_56D0D0(void);
static void schematic_ui_create(void);
static void schematic_ui_destroy(void);
static bool schematic_ui_message_filter(TigMessage* msg);
static int tech_from_schematic(int schematic);
static int schematic_ui_current_id(void);
static void schematic_ui_parse_items(const char* str, int* items);
static void schematic_ui_redraw(void);
static void schematic_ui_draw_component(int ingr, SchematicInfo* schematic_info, bool* a3, bool* a4);

/**
 * 0x5CA818
 */
static tig_window_handle_t schematic_ui_window = TIG_WINDOW_HANDLE_INVALID;

/**
 * Defines the frame of the schematic UI window (in the original screen
 * coordinate system, that is 800x600).
 *
 * 0x5CA820
 */
static TigRect schematic_ui_window_rect = { 0, 41, 800, 400 };

/**
 * Defines art numbers of the indicator icons for appropriate readiness state.
 *
 * 0x5CA830
 */
static int schematic_ui_combine_indicator_icons[SCHEMATIC_UI_READINESS_COUNT] = {
    /*           SCHEMATIC_UI_READINESS_OK */ 360,
    /*     SCHEMATIC_UI_READINESS_NO_ITEMS */ 359,
    /* SCHEMATIC_UI_READINESS_NO_EXPERTISE */ 358,
};

/**
 * Defines a rect where PC lens is located in the schematic UI.
 *
 * 0x5CA840
 */
static TigRect schematic_ui_pc_lens_rect = { 50, 26, 89, 89 };

/**
 * Defines schematic UI navigation/action buttons.
 *
 * 0x5CA850
 */
static UiButtonInfo schematic_ui_buttons[SCHEMATIC_UI_BUTTON_COUNT] = {
    /*    SCHEMATIC_UI_BUTTON_PREV */ { 204, 57, 372, TIG_BUTTON_HANDLE_INVALID },
    /*    SCHEMATIC_UI_BUTTON_NEXT */ { 687, 57, 373, TIG_BUTTON_HANDLE_INVALID },
    /* SCHEMATIC_UI_BUTTON_COMBINE */ { 76, 365, 357, TIG_BUTTON_HANDLE_INVALID },
    /* SCHEMATIC_UI_BUTTON_LEARNED */ { 29, 201, 6, TIG_BUTTON_HANDLE_INVALID },
    /*   SCHEMATIC_UI_BUTTON_FOUND */ { 29, 289, 6, TIG_BUTTON_HANDLE_INVALID },
};

/**
 * Defines tech discipline tab buttons.
 *
 * 0x5CA8A0
 */
static UiButtonInfo schematic_ui_tabs[TECH_COUNT] = {
    /*    TECH_HERBOLOGY */ { 707, 58, 355, TIG_BUTTON_HANDLE_INVALID },
    /*    TECH_CHEMISTRY */ { 707, 103, 356, TIG_BUTTON_HANDLE_INVALID },
    /*     TECH_ELECTRIC */ { 707, 148, 361, TIG_BUTTON_HANDLE_INVALID },
    /*   TECH_EXPLOSIVES */ { 707, 193, 362, TIG_BUTTON_HANDLE_INVALID },
    /*          TECH_GUN */ { 707, 238, 363, TIG_BUTTON_HANDLE_INVALID },
    /*   TECH_MECHANICAL */ { 707, 283, 364, TIG_BUTTON_HANDLE_INVALID },
    /*       TECH_SMITHY */ { 707, 328, 368, TIG_BUTTON_HANDLE_INVALID },
    /* TECH_THERAPEUTICS */ { 707, 373, 369, TIG_BUTTON_HANDLE_INVALID },
};

/**
 * Defines component item frames.
 *
 * 0x5CA920
 */
static TigRect schematic_ui_component_rects[] = {
    { 579, 181, 99, 67 },
    { 579, 277, 99, 67 },
};

/**
 * Defines component item tech discipline icon frames.
 *
 * 0x5CA940
 */
static TigRect schematic_ui_component_icon_rects[] = {
    { 583, 182, 50, 50 },
    { 583, 278, 50, 50 },
};

/**
 * Defines component item complexity score frames.
 *
 * 0x5CA960
 */
static TigRect schematic_ui_component_complexity_rects[] = {
    { 658, 232, 50, 50 },
    { 658, 328, 50, 50 },
};

/**
 * Button handle for the "Component 1" slot.
 *
 * 0x680DE0
 */
static tig_button_handle_t schematic_ui_component1_button;

/**
 * Array tracking the number of learned schematics per tech discipline.
 *
 * 0x680DE4
 */
static int schematic_ui_num_learned_schematics_by_tech[TECH_COUNT];

/**
 * Array tracking the number of found schematics per tech discipline.
 *
 * 0x680E04
 */
static int schematic_ui_num_found_schematics_by_tech[TECH_COUNT];

/**
 * Last page for "Learned schematics" view.
 *
 * 0x680E24
 */
static int schematic_ui_learned_page;

/**
 * 0x680E28
 */
static int64_t schematic_ui_component2_obj;

/**
 * Last tech discipline for "Learned schematics" view.
 *
 * 0x680E30
 */
static int schematic_ui_learned_tech;

/**
 * 0x680E34
 */
static tig_font_handle_t schematic_ui_name_font;

/**
 * 0x680E38
 */
static tig_font_handle_t schematic_ui_description_font;

/**
 * Button handle for the "Component 2" slot.
 *
 * 0x680E3C
 */
static tig_button_handle_t schematic_ui_component2_button;

/**
 * Current view mode of the schematic UI.
 *
 * 0x680E40
 */
static SchematicUiView schematic_ui_view;

/**
 * "schematic.mes"
 *
 * 0x680E44
 */
static mes_file_handle_t schematic_ui_rules_mes_file;

/**
 * Total number of schematics in the current view.
 *
 * 0x680E48
 */
static int schematic_ui_cur_num_schematics;

/**
 * Pointer to the array of schematic counts by tech discipline.
 *
 * This is an unowned pointer to `schematic_ui_num_learned_schematics_by_tech`
 * or `schematic_ui_num_found_schematics_by_tech` depending the current view
 * mode.
 *
 * 0x680E4C
 */
static int* schematic_ui_cur_num_schematics_by_tech;

/**
 * Array of found schematic IDs.
 *
 * 0x680E50
 */
static int* schematic_ui_found_schematics;

/**
 * Total number of learned schematics.
 *
 * 0x680E54
 */
static int schematic_ui_num_learned_schematics;

/**
 * Current page index in the schematic UI.
 *
 * 0x680E58
 */
static int schematic_ui_cur_page;

/**
 * Pointer to array of schematic IDs for the current view.
 *
 * This is an unowned pointer to `schematic_ui_learned_schematics` or
 * `schematic_ui_found_schematics` depending the current view mode.
 *
 * 0x680E5C
 */
static int* schematic_ui_cur_schematics;

/**
 * 0x680E60
 */
static int64_t schematic_ui_secondary_obj;

/**
 * Last page for "Found schematics" view.
 *
 * 0x680E68
 */
static int schematic_ui_found_page;

/**
 * Current tech discipline in the schematic UI.
 *
 * 0x680E6C
 */
static int schematic_ui_cur_tech;

/**
 * 0x680E70
 */
static int64_t schematic_ui_primary_obj;

/**
 * "schematic_text.mes"
 *
 * 0x680E78
 */
static mes_file_handle_t schematic_ui_text_mes_file;

/**
 * Readiness state for combining items in the current schematic.
 *
 * 0x680E7C
 */
static SchematicUiReadiness schematic_ui_readiness;

/**
 * Last tech discipline for "Found schematics" view.
 *
 * 0x680E80
 */
static int schematic_ui_found_tech;

/**
 * Total number of found schematics.
 *
 * 0x680E84
 */
static int schematic_ui_num_found_schematics;

/**
 * 0x680E88
 */
static tig_font_handle_t schematic_ui_icons17_font;

/**
 * 0x680E8C
 */
static tig_font_handle_t schematic_ui_icons32_font;

/**
 * 0x680E90
 */
static int64_t schematic_ui_component1_obj;

/**
 * Array of learned schematic IDs.
 *
 * 0x680E98
 */
static int* schematic_ui_learned_schematics;

/**
 * Flag indicating whether the schematic UI is initialized.
 *
 * 0x680E9C
 */
static bool schematic_ui_initialized;

/**
 * Flag indicating whether the schematic UI is currently displayed.
 *
 * 0x680EA0
 */
static bool schematic_ui_created;

/**
 * Called when the game is initialized.
 *
 * 0x56CD60
 */
bool schematic_ui_init(GameInitInfo* init_info)
{
    int num;
    MesFileEntry mes_file_entry;
    int tech;
    TigFont font;
    int idx;

    (void)init_info;

    // Load schematic rules (required).
    if (!mes_load("rules\\schematic.mes", &schematic_ui_rules_mes_file)) {
        return false;
    }

    // Load UI messages (required).
    if (!mes_load("mes\\schematic_text.mes", &schematic_ui_text_mes_file)) {
        mes_unload(schematic_ui_rules_mes_file);
        return false;
    }

    // Reset schematic counts per tech discipline.
    for (tech = 0; tech < TECH_COUNT; tech++) {
        schematic_ui_num_learned_schematics_by_tech[tech] = 0;
    }

    // Count and store learned schematics from the message file (range
    // 2000-3999, 7 fields per rule).
    schematic_ui_num_learned_schematics = mes_entries_count_in_range(schematic_ui_rules_mes_file, 2000, 3999) / 7;
    if (schematic_ui_num_learned_schematics > 0) {
        schematic_ui_learned_schematics = (int*)MALLOC(sizeof(int) * schematic_ui_num_learned_schematics);

        // Populate learned schematics.
        idx = 0;
        for (num = 2000; num < 3999; num += 10) {
            mes_file_entry.num = num;
            if (mes_search(schematic_ui_rules_mes_file, &mes_file_entry)) {
                schematic_ui_learned_schematics[idx] = num;

                tech = tech_from_schematic(num);
                schematic_ui_num_learned_schematics_by_tech[tech]++;

                idx++;
            }
        }
    }

    // Set up fonts.
    font.flags = 0;
    tig_art_interface_id_create(229, 0, 0, 0, &(font.art_id));
    font.str = NULL;
    font.color = tig_color_make(0, 0, 0);
    tig_font_create(&font, &schematic_ui_description_font);

    font.flags = 0;
    tig_art_interface_id_create(300, 0, 0, 0, &(font.art_id));
    font.str = NULL;
    font.color = tig_color_make(0, 0, 0);
    tig_font_create(&font, &schematic_ui_icons17_font);

    font.flags = 0;
    tig_art_interface_id_create(370, 0, 0, 0, &(font.art_id));
    font.str = NULL;
    font.color = tig_color_make(0, 0, 0);
    tig_font_create(&font, &schematic_ui_icons32_font);

    font.flags = 0;
    tig_art_interface_id_create(371, 0, 0, 0, &(font.art_id));
    font.str = NULL;
    font.color = tig_color_make(0, 0, 0);
    tig_font_create(&font, &schematic_ui_name_font);

    schematic_ui_initialized = true;
    schematic_ui_primary_obj = OBJ_HANDLE_NULL;
    schematic_ui_secondary_obj = OBJ_HANDLE_NULL;
    sub_56D0D0();

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x56D040
 */
void schematic_ui_exit(void)
{
    mes_unload(schematic_ui_rules_mes_file);
    mes_unload(schematic_ui_text_mes_file);

    tig_font_destroy(schematic_ui_description_font);
    tig_font_destroy(schematic_ui_icons17_font);
    tig_font_destroy(schematic_ui_icons32_font);
    tig_font_destroy(schematic_ui_name_font);

    if (schematic_ui_num_learned_schematics > 0) {
        FREE(schematic_ui_learned_schematics);
    }

    schematic_ui_initialized = false;
}

/**
 * Called when the game is being reset.
 *
 * 0x56D0B0
 */
void schematic_ui_reset(void)
{
    schematic_ui_primary_obj = OBJ_HANDLE_NULL;
    schematic_ui_secondary_obj = OBJ_HANDLE_NULL;
    sub_56D0D0();
}

// 0x56D0D0
void sub_56D0D0(void)
{
    schematic_ui_readiness = SCHEMATIC_UI_READINESS_NO_ITEMS;

    schematic_ui_cur_tech = 0;
    schematic_ui_found_tech = 0;
    schematic_ui_learned_tech = 0;

    schematic_ui_cur_page = 0;
    schematic_ui_found_page = 0;
    schematic_ui_learned_page = 0;

    schematic_ui_cur_num_schematics = schematic_ui_num_learned_schematics;
    schematic_ui_cur_num_schematics_by_tech = schematic_ui_num_learned_schematics_by_tech;
    schematic_ui_cur_schematics = schematic_ui_learned_schematics;

    schematic_ui_view = SCHEMATIC_UI_VIEW_LEARNED;
}

/**
 * Toggles the schematic UI.
 *
 * 0x56D130
 */
void schematic_ui_toggle(int64_t primary_obj, int64_t secondary_obj)
{
    int tech;
    int index;

    // Close schematic UI if it's currently visible.
    if (schematic_ui_created) {
        schematic_ui_close();
        return;
    }

    // Validate critters.
    if (primary_obj == OBJ_HANDLE_NULL
        || secondary_obj == OBJ_HANDLE_NULL
        || critter_is_dead(primary_obj)
        || critter_is_dead(secondary_obj)) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_MAIN)) {
        return;
    }

    if (!intgame_mode_set(INTGAME_MODE_SCHEMATIC)) {
        return;
    }

    schematic_ui_primary_obj = primary_obj;
    schematic_ui_secondary_obj = secondary_obj;

    // Reset state for NPCs.
    if (obj_field_int32_get(primary_obj, OBJ_F_TYPE) == OBJ_TYPE_NPC) {
        sub_56D0D0();
    }

    // Initialize number of known schematics. The number of found schematics is
    // set to `0` and will be initialized separately.
    for (tech = 0; tech < TECH_COUNT; tech++) {
        schematic_ui_num_learned_schematics_by_tech[tech] = tech_degree_get(primary_obj, tech);
        schematic_ui_num_found_schematics_by_tech[tech] = 0;
    }

    // Only player characters can have found schematics (NPCs does not have
    // appropriate field).
    if (obj_field_int32_get(primary_obj, OBJ_F_TYPE) == OBJ_TYPE_PC) {
        schematic_ui_num_found_schematics = obj_arrayfield_length_get(primary_obj, OBJ_F_PC_SCHEMATICS_FOUND_IDX);
        if (schematic_ui_num_found_schematics > 0) {
            // Load found schematics into the appropriate array, and track
            // number of found schematics per tech.
            schematic_ui_found_schematics = (int*)MALLOC(sizeof(int) * schematic_ui_num_found_schematics);
            for (index = 0; index < schematic_ui_num_found_schematics; index++) {
                schematic_ui_found_schematics[index] = obj_arrayfield_uint32_get(primary_obj, OBJ_F_PC_SCHEMATICS_FOUND_IDX, index);

                tech = tech_from_schematic(schematic_ui_found_schematics[index]);
                schematic_ui_num_found_schematics_by_tech[tech]++;
            }
        }
    } else {
        schematic_ui_num_found_schematics = 0;
    }

    // Set current schematic data based on view mode.
    if (schematic_ui_view == SCHEMATIC_UI_VIEW_FOUND) {
        schematic_ui_cur_num_schematics_by_tech = schematic_ui_num_found_schematics_by_tech;
        schematic_ui_cur_schematics = schematic_ui_found_schematics;
        schematic_ui_cur_num_schematics = schematic_ui_num_found_schematics;
    }

    schematic_ui_create();
}

/**
 * Closes the schematic UI.
 *
 * 0x56D2D0
 */
void schematic_ui_close(void)
{
    if (schematic_ui_created && intgame_mode_set(INTGAME_MODE_MAIN)) {
        schematic_ui_destroy();
        schematic_ui_primary_obj = OBJ_HANDLE_NULL;
        schematic_ui_secondary_obj = OBJ_HANDLE_NULL;
    }
}

/**
 * Creates the schematic UI window.
 *
 * 0x56D310
 */
void schematic_ui_create(void)
{
    int index;
    tig_button_handle_t buttons[TECH_COUNT];
    TigButtonData button_data;
    PcLens pc_lens;

    // Skip if UI is already created.
    if (schematic_ui_created) {
        return;
    }

    if (!intgame_big_window_lock(schematic_ui_message_filter, &schematic_ui_window)) {
        tig_debug_printf("schematic_ui_create: ERROR: intgame_big_window_lock failed!\n");
        exit(0);
    }

    // Create navigation and action buttons.
    for (index = 0; index < SCHEMATIC_UI_BUTTON_COUNT; index++) {
        intgame_button_create_ex(schematic_ui_window, &schematic_ui_window_rect, &(schematic_ui_buttons[index]), 1);
    }

    // CE: There are two buttons in the interface to switch learned vs. found
    // schematics, but there is no way to tell what we're currently looking at.
    // Improve it by arranging learned/found buttons into a radio group.
    buttons[0] = schematic_ui_buttons[SCHEMATIC_UI_BUTTON_LEARNED].button_handle;
    buttons[1] = schematic_ui_buttons[SCHEMATIC_UI_BUTTON_FOUND].button_handle;
    tig_button_radio_group_create(2, buttons, schematic_ui_view);

    // Create tech discipline tabs.
    for (index = 0; index < TECH_COUNT; index++) {
        intgame_button_create_ex(schematic_ui_window, &schematic_ui_window_rect, &(schematic_ui_tabs[index]), 1);
        buttons[index] = schematic_ui_tabs[index].button_handle;
    }

    // Create radio group for tech tabs to ensure mutual exclusivity.
    tig_button_radio_group_create(TECH_COUNT, buttons, schematic_ui_cur_tech);

    // Set up component slot buttons.
    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = schematic_ui_window;
    button_data.art_id = TIG_ART_ID_INVALID;
    button_data.mouse_down_snd_id = -1;
    button_data.mouse_up_snd_id = -1;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    // Component 1 button.
    button_data.x = schematic_ui_component_rects[0].x;
    button_data.y = schematic_ui_component_rects[0].y;
    button_data.width = schematic_ui_component_rects[0].width;
    button_data.height = schematic_ui_component_rects[0].height;
    tig_button_create(&button_data, &schematic_ui_component1_button);

    // Component 2 button.
    button_data.x = schematic_ui_component_rects[1].x;
    button_data.y = schematic_ui_component_rects[1].y;
    button_data.width = schematic_ui_component_rects[1].width;
    button_data.height = schematic_ui_component_rects[1].height;
    tig_button_create(&button_data, &schematic_ui_component2_button);

    schematic_ui_redraw();

    // Center viewport on the player (so that the lens display proper
    // surroundings).
    location_origin_set(obj_field_int64_get(schematic_ui_primary_obj, OBJ_F_LOCATION));

    // Enable the PC lens.
    pc_lens.window_handle = schematic_ui_window;
    pc_lens.rect = &schematic_ui_pc_lens_rect;
    tig_art_interface_id_create(231, 0, 0, 0, &(pc_lens.art_id));
    intgame_pc_lens_do(PC_LENS_MODE_PASSTHROUGH, &pc_lens);

    // Play sound effect for opening the UI.
    gsound_play_sfx(SND_INTERFACE_BOOK_OPEN, 1);

    schematic_ui_created = true;
}

/**
 * Destroys the schematic UI.
 *
 * 0x56D4D0
 */
void schematic_ui_destroy(void)
{
    if (!schematic_ui_created) {
        return;
    }

    // Disable the PC lens.
    intgame_pc_lens_do(PC_LENS_MODE_NONE, NULL);

    intgame_big_window_unlock();
    schematic_ui_window = TIG_WINDOW_HANDLE_INVALID;

    if (schematic_ui_num_found_schematics > 0) {
        FREE(schematic_ui_found_schematics);
    }

    // Play sound effect for closing the UI.
    gsound_play_sfx(SND_INTERFACE_BOOK_CLOSE, 1);

    schematic_ui_created = false;
}

/**
 * Called when an event occurred.
 *
 * 0x56D530
 */
bool schematic_ui_message_filter(TigMessage* msg)
{
    MesFileEntry mes_file_entry;
    UiMessage ui_message;
    int tech;

    switch (msg->type) {
    case TIG_MESSAGE_MOUSE:
        // Clicking on the PC lens closes schematic UI.
        if (msg->data.mouse.event == TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP
            && intgame_pc_lens_check_pt(msg->data.mouse.x, msg->data.mouse.y)) {
            schematic_ui_close();
            return true;
        }

        break;
    case TIG_MESSAGE_BUTTON:
        switch (msg->data.button.state) {
        case TIG_BUTTON_STATE_MOUSE_INSIDE:
            // Check mouse hover over "component 1" button.
            if (msg->data.button.button_handle == schematic_ui_component1_button) {
                if (schematic_ui_component1_obj != OBJ_HANDLE_NULL) {
                    sub_57CCF0(schematic_ui_secondary_obj, schematic_ui_component1_obj);
                }
                return true;
            }

            // Check mouse hover over "component 2" button.
            if (msg->data.button.button_handle == schematic_ui_component2_button) {
                if (schematic_ui_component2_obj != OBJ_HANDLE_NULL) {
                    sub_57CCF0(schematic_ui_secondary_obj, schematic_ui_component2_obj);
                }
                return true;
            }

            // Check mouse hover over "Combine" button.
            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_COMBINE].button_handle) {
                mes_file_entry.num = 3;
                mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);
                ui_message.type = UI_MSG_TYPE_FEEDBACK;
                ui_message.str = mes_file_entry.str;
                intgame_message_window_display_msg(&ui_message);
                return true;
            }

            // Check mouse hover over "Learned schematics" button.
            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_LEARNED].button_handle) {
                mes_file_entry.num = 4;
                mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);
                ui_message.type = UI_MSG_TYPE_FEEDBACK;
                ui_message.str = mes_file_entry.str;
                intgame_message_window_display_msg(&ui_message);
                return true;
            }

            // Check mouse hover over "Found schematics" button.
            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_FOUND].button_handle) {
                mes_file_entry.num = 5;
                mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);
                ui_message.type = UI_MSG_TYPE_FEEDBACK;
                ui_message.str = mes_file_entry.str;
                intgame_message_window_display_msg(&ui_message);
                return true;
            }

            // Check mouse hover over tech discipline tabs.
            for (tech = 0; tech < TECH_COUNT; tech++) {
                if (msg->data.button.button_handle == schematic_ui_tabs[tech].button_handle) {
                    ui_message.type = UI_MSG_TYPE_TECH;
                    ui_message.field_8 = tech;
                    intgame_message_window_display_msg(&ui_message);
                    return true;
                }
            }

            break;
        case TIG_BUTTON_STATE_MOUSE_OUTSIDE:
            // Clear feedback when mouse leaves buttons.
            if (msg->data.button.button_handle == schematic_ui_component1_button
                || msg->data.button.button_handle == schematic_ui_component2_button
                || msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_COMBINE].button_handle
                || msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_LEARNED].button_handle
                || msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_FOUND].button_handle) {
                intgame_message_window_clear();
                return true;
            }

            for (tech = 0; tech < TECH_COUNT; tech++) {
                if (msg->data.button.button_handle == schematic_ui_tabs[tech].button_handle) {
                    intgame_message_window_clear();
                    return true;
                }
            }

            break;
        case TIG_BUTTON_STATE_PRESSED:
            // Handle tech discipline tab selection.
            for (tech = 0; tech < TECH_COUNT; tech++) {
                if (msg->data.button.button_handle == schematic_ui_tabs[tech].button_handle) {
                    schematic_ui_cur_tech = tech;
                    schematic_ui_cur_page = 0;
                    schematic_ui_redraw();
                    gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
                    return true;
                }
            }

            // Handle page navigation buttons.
            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_PREV].button_handle) {
                schematic_ui_cur_page--;
                schematic_ui_redraw();
                gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
                return true;
            }

            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_NEXT].button_handle) {
                schematic_ui_cur_page++;
                schematic_ui_redraw();
                gsound_play_sfx(SND_INTERFACE_BOOK_PAGE_TURN, 1);
                return true;
            }

            // Switch to "Learned schematics" view.
            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_LEARNED].button_handle) {
                if (schematic_ui_view == SCHEMATIC_UI_VIEW_FOUND) {
                    tig_button_state_change(schematic_ui_tabs[schematic_ui_cur_tech].button_handle, TIG_BUTTON_STATE_RELEASED);

                    // Save last "Found schmeatics" view settings.
                    schematic_ui_found_page = schematic_ui_cur_page;
                    schematic_ui_found_tech = schematic_ui_cur_tech;

                    // Restore "Learned schematics" view settings.
                    schematic_ui_cur_page = schematic_ui_learned_page;
                    schematic_ui_cur_tech = schematic_ui_learned_tech;

                    schematic_ui_cur_num_schematics = schematic_ui_num_learned_schematics;
                    schematic_ui_cur_num_schematics_by_tech = schematic_ui_num_learned_schematics_by_tech;
                    schematic_ui_cur_schematics = schematic_ui_learned_schematics;
                    schematic_ui_view = SCHEMATIC_UI_VIEW_LEARNED;

                    // Manually restore selection of the appropriate tech
                    // discipline tab.
                    tig_button_state_change(schematic_ui_tabs[schematic_ui_cur_tech].button_handle, TIG_BUTTON_STATE_PRESSED);

                    // Refresh the UI.
                    schematic_ui_redraw();

                    // Play sound effect.
                    gsound_play_sfx(SND_INTERFACE_BOOK_SWITCH, 1);
                }
                return true;
            }

            // Switch to "Found schematics" view.
            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_FOUND].button_handle) {
                if (schematic_ui_view == SCHEMATIC_UI_VIEW_LEARNED) {
                    tig_button_state_change(schematic_ui_tabs[schematic_ui_cur_tech].button_handle, TIG_BUTTON_STATE_RELEASED);

                    // Save last "Learned schematics" view settings.
                    schematic_ui_learned_page = schematic_ui_cur_page;
                    schematic_ui_learned_tech = schematic_ui_cur_tech;

                    // Restore "Found schematics" view settings.
                    schematic_ui_cur_page = schematic_ui_found_page;
                    schematic_ui_cur_tech = schematic_ui_found_tech;

                    schematic_ui_cur_num_schematics = schematic_ui_num_found_schematics;
                    schematic_ui_cur_num_schematics_by_tech = schematic_ui_num_found_schematics_by_tech;
                    schematic_ui_cur_schematics = schematic_ui_found_schematics;
                    schematic_ui_view = SCHEMATIC_UI_VIEW_FOUND;

                    // Manually restore selection of the appropriate tech
                    // discipline tab.
                    tig_button_state_change(schematic_ui_tabs[schematic_ui_cur_tech].button_handle, TIG_BUTTON_STATE_PRESSED);

                    // Refresh the UI.
                    schematic_ui_redraw();

                    // Play sound effect.
                    gsound_play_sfx(SND_INTERFACE_BOOK_SWITCH, 1);
                }
                return true;
            }

            break;
        case TIG_BUTTON_STATE_RELEASED:
            // Handle "Combine" button.
            if (msg->data.button.button_handle == schematic_ui_buttons[SCHEMATIC_UI_BUTTON_COMBINE].button_handle) {
                switch (schematic_ui_readiness) {
                case SCHEMATIC_UI_READINESS_OK:
                    schematic_ui_process(schematic_ui_current_id(), schematic_ui_primary_obj, schematic_ui_secondary_obj);
                    schematic_ui_redraw();
                    break;
                case SCHEMATIC_UI_READINESS_NO_ITEMS:
                    mes_file_entry.num = 0; // "You are missing required item(s)."
                    mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);
                    ui_message.type = UI_MSG_TYPE_EXCLAMATION;
                    ui_message.str = mes_file_entry.str;
                    intgame_message_window_display_msg(&ui_message);
                    break;
                case SCHEMATIC_UI_READINESS_NO_EXPERTISE:
                    mes_file_entry.num = 1; // "You lack the expertise to combine these items."
                    mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);
                    ui_message.type = UI_MSG_TYPE_EXCLAMATION;
                    ui_message.str = mes_file_entry.str;
                    intgame_message_window_display_msg(&ui_message);
                    break;
                case SCHEMATIC_UI_READINESS_COUNT:
                    // Should be unreachable.
                    break;
                }
                return true;
            }
            break;
        }
        break;
    default:
        break;
    }

    return false;
}

/**
 * Determines the tech discipline associated with a schematic.
 *
 * 0x56DB00
 */
int tech_from_schematic(int schematic)
{
    SchematicInfo schematic_info;
    int64_t obj;

    // Handle found schematics - the associated tech is stored in a separate
    // field (`OBJ_F_ITEM_DISCIPLINE`).
    if (schematic >= 4000) {
        schematic_ui_info_get(schematic, &schematic_info);
        obj = sub_4685A0(schematic_info.prod[0]);
        return obj_field_int32_get(obj, OBJ_F_ITEM_DISCIPLINE);
    }

    // Handle learned schematics - the associated tech is derived from a
    // special numbering scheme.
    if (schematic >= 2000) {
        return (schematic - 2000) / 200;
    }

    return 0;
}

/**
 * Retrieves the ID of the currently displayed schematic.
 *
 * 0x56DB60
 */
int schematic_ui_current_id(void)
{
    int index;
    int page;

    if (schematic_ui_cur_num_schematics_by_tech[schematic_ui_cur_tech] == 0) {
        return -1;
    }

    // Iterate through schematics to find the one matching the current tech and
    // page.
    page = 0;
    for (index = 0; index < schematic_ui_cur_num_schematics; index++) {
        if (tech_from_schematic(schematic_ui_cur_schematics[index]) == schematic_ui_cur_tech) {
            if (page == schematic_ui_cur_page) {
                return schematic_ui_cur_schematics[index];
            }
            page++;
        }
    }

    return -1;
}

/**
 * Retrieves information about a schematic from the message file.
 *
 * 0x56DBD0
 */
void schematic_ui_info_get(int schematic, SchematicInfo* schematic_info)
{
    MesFileEntry mes_file_entry;

    // Special case - schematic title page, exactly the same data structure,
    // but different message number range. Each tech has it's own title,
    // description, and drawings.
    if (schematic == -1) {
        // Schematic title page.
        schematic = 10 * schematic_ui_cur_tech + 6000;
    }

    mes_file_entry.num = schematic + SCHEMATIC_F_NAME;
    mes_get_msg(schematic_ui_rules_mes_file, &mes_file_entry);
    schematic_info->name = atoi(mes_file_entry.str);

    mes_file_entry.num = schematic + SCHEMATIC_F_DESCRIPTION;
    mes_get_msg(schematic_ui_rules_mes_file, &mes_file_entry);
    schematic_info->description = atoi(mes_file_entry.str);

    mes_file_entry.num = schematic + SCHEMATIC_F_ART_NUM;
    mes_get_msg(schematic_ui_rules_mes_file, &mes_file_entry);
    schematic_info->art_num = atoi(mes_file_entry.str);

    mes_file_entry.num = schematic + SCHEMATIC_F_ITEM_1;
    mes_get_msg(schematic_ui_rules_mes_file, &mes_file_entry);
    schematic_ui_parse_items(mes_file_entry.str, schematic_info->item1);

    tig_debug_printf("Read line schematic.mes line %d: %d %d %d\n",
        mes_file_entry.num,
        schematic_info->item1[0],
        schematic_info->item1[1],
        schematic_info->item1[2]);

    mes_file_entry.num = schematic + SCHEMATIC_F_ITEM_2;
    mes_get_msg(schematic_ui_rules_mes_file, &mes_file_entry);
    schematic_ui_parse_items(mes_file_entry.str, schematic_info->item2);

    mes_file_entry.num = schematic + SCHEMATIC_F_PROD;
    mes_get_msg(schematic_ui_rules_mes_file, &mes_file_entry);
    schematic_ui_parse_items(mes_file_entry.str, schematic_info->prod);

    mes_file_entry.num = schematic + SCHEMATIC_F_QTY;
    mes_get_msg(schematic_ui_rules_mes_file, &mes_file_entry);
    schematic_info->qty = atoi(mes_file_entry.str);
}

/**
 * Parses a space-separated string of item IDs into an array.
 *
 * 0x56DD20
 */
void schematic_ui_parse_items(const char* str, int* items)
{
    char mut_str[MAX_STRING];
    char* pch;
    int cnt;

    strcpy(mut_str, str);
    cnt = 0;

    pch = strtok(mut_str, " ");
    if (pch != NULL) {
        do {
            items[cnt++] = atoi(pch);
            pch = strtok(NULL, " ");
        } while (pch != NULL);

        // Fill remaining slots with the last valid ID.
        if (cnt > 0) {
            while (cnt < 3) {
                items[cnt] = items[cnt - 1];
                cnt++;
            }
        }
    }
}

/**
 * Refreshes the schematic UI.
 *
 * 0x56DDC0
 */
void schematic_ui_redraw(void)
{
    TigRect src_rect;
    TigRect dst_rect;
    TigArtBlitInfo blit_info;
    TigArtFrameData art_frame_data;
    TigFont font;
    MesFileEntry mes_file_entry;
    int schematic;
    SchematicInfo schematic_info;
    int64_t obj;
    char icon[2];
    bool have_item1;
    bool have_expertise1;
    bool have_item2;
    bool have_expertise2;

    // Show or hide navigation buttons based on page availability.
    if (schematic_ui_cur_page != 0) {
        tig_button_show(schematic_ui_buttons[SCHEMATIC_UI_BUTTON_PREV].button_handle);
    } else {
        tig_button_hide(schematic_ui_buttons[SCHEMATIC_UI_BUTTON_PREV].button_handle);
    }

    if (schematic_ui_cur_page < schematic_ui_cur_num_schematics_by_tech[schematic_ui_cur_tech] - 1) {
        tig_button_show(schematic_ui_buttons[SCHEMATIC_UI_BUTTON_NEXT].button_handle);
    } else {
        tig_button_hide(schematic_ui_buttons[SCHEMATIC_UI_BUTTON_NEXT].button_handle);
    }

    // Draw background.
    blit_info.flags = 0;
    tig_art_interface_id_create(365, 0, 0, 0, &(blit_info.art_id));

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = schematic_ui_window_rect.width;
    src_rect.height = schematic_ui_window_rect.height;
    blit_info.src_rect = &src_rect;

    dst_rect = src_rect;
    blit_info.dst_rect = &dst_rect;

    tig_window_blit_art(schematic_ui_window, &blit_info);

    // Obtain current schematic info.
    schematic = schematic_ui_current_id();
    schematic_ui_info_get(schematic, &schematic_info);

    // Draw schematic image.
    blit_info.flags = 0;
    tig_art_interface_id_create(schematic_info.art_num, 0, 0, 0, &(blit_info.art_id));
    tig_art_frame_data(blit_info.art_id, &art_frame_data);

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;
    blit_info.src_rect = &src_rect;

    dst_rect.x = 240;
    dst_rect.y = 146;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;
    blit_info.dst_rect = &dst_rect;

    tig_window_blit_art(schematic_ui_window, &blit_info);

    // Draw schematic name.
    mes_file_entry.num = schematic_info.name;
    mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);

    tig_font_push(schematic_ui_name_font);
    font.str = mes_file_entry.str;
    font.width = 0;
    tig_font_measure(&font);

    src_rect.x = 261;
    src_rect.y = 30;
    src_rect.width = font.width;
    src_rect.height = font.height;

    tig_window_text_write(schematic_ui_window, mes_file_entry.str, &src_rect);
    tig_font_pop();

    // Draw schematic description.
    mes_file_entry.num = schematic_info.description;
    mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);

    tig_font_push(schematic_ui_description_font);
    font.str = mes_file_entry.str;
    font.width = 0;
    tig_font_measure(&font);

    src_rect.x = 219;
    src_rect.y = 71;
    src_rect.width = 466;
    src_rect.height = 75;

    tig_window_text_write(schematic_ui_window, mes_file_entry.str, &src_rect);
    tig_font_pop();

    // Draw tech discipline icon.
    obj = sub_4685A0(schematic_info.prod[0]);
    icon[0] = (char)obj_field_int32_get(obj, OBJ_F_ITEM_DISCIPLINE) + '0';
    icon[1] = '\0';

    tig_font_push(schematic_ui_icons32_font);

    src_rect.x = 222;
    src_rect.y = 32;
    src_rect.width = 100;
    src_rect.height = 100;

    tig_window_text_write(schematic_ui_window, icon, &src_rect);
    tig_font_pop();

    // Draw schematic components.
    schematic_ui_draw_component(0, &schematic_info, &have_item1, &have_expertise1);
    schematic_ui_draw_component(1, &schematic_info, &have_item2, &have_expertise2);

    // Determine readiness state based on component checks.
    if (!have_expertise1 || !have_expertise2) {
        schematic_ui_readiness = SCHEMATIC_UI_READINESS_NO_EXPERTISE;
    } else if (!have_item1 || !have_item2) {
        schematic_ui_readiness = SCHEMATIC_UI_READINESS_NO_ITEMS;
    } else {
        schematic_ui_readiness = SCHEMATIC_UI_READINESS_OK;
    }

    // Draw readiness indicator.
    blit_info.flags = 0;
    tig_art_interface_id_create(schematic_ui_combine_indicator_icons[schematic_ui_readiness], 0, 0, 0, &(blit_info.art_id));
    tig_art_frame_data(blit_info.art_id, &art_frame_data);

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;
    blit_info.src_rect = &src_rect;

    dst_rect.x = 43;
    dst_rect.y = 338;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;
    blit_info.dst_rect = &dst_rect;

    tig_window_blit_art(schematic_ui_window, &blit_info);
}

/**
 * Renders a component slot in the schematic UI.
 *
 * 0x56E190
 */
void schematic_ui_draw_component(int ingr, SchematicInfo* schematic_info, bool* have_item, bool* have_expertise)
{
    int index;
    TigArtAnimData art_anim_data;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect dst_rect;
    TigRect src_rect;
    int64_t item_obj;
    int tech;
    int complexity;
    TigPalette palette;
    TigPaletteModifyInfo palette_modify_info;
    char str[80];
    float width_ratio;
    float height_ratio;

    // Check availability of the required item.
    if (ingr == 0) {
        for (index = 0; index < 3; index++) {
            item_obj = sub_4685A0(schematic_info->item1[index]);
            if (sub_462540(schematic_ui_primary_obj, item_obj, 0x7) != OBJ_HANDLE_NULL) {
                break;
            }
        }

        if (index == 3) {
            item_obj = sub_4685A0(schematic_info->item1[0]);
            *have_item = false;
        } else {
            *have_item = true;
        }

        schematic_ui_component1_obj = item_obj;
    } else {
        for (index = 0; index < 3; index++) {
            item_obj = sub_4685A0(schematic_info->item2[index]);
            if (sub_462540(schematic_ui_primary_obj, item_obj, 0x7) != OBJ_HANDLE_NULL) {
                break;
            }
        }

        if (index == 3) {
            item_obj = sub_4685A0(schematic_info->item2[0]);
            *have_item = false;
        } else {
            *have_item = true;
        }

        schematic_ui_component2_obj = item_obj;
    }

    // Check expertise for the component's tech discipline.
    tech = obj_field_int32_get(item_obj, OBJ_F_ITEM_DISCIPLINE);
    complexity = -obj_field_int32_get(item_obj, OBJ_F_ITEM_MAGIC_TECH_COMPLEXITY);
    *have_expertise = tech_degree_level_get(schematic_ui_primary_obj, tech) >= complexity;

    // Draw component background based on expertise and item availability.
    if (*have_expertise) {
        if (*have_item) {
            // "schem_have_item.art"
            tig_art_interface_id_create(831, 0, 0, 0, &(art_blit_info.art_id));
        } else {
            // "schem_no_item.art"
            tig_art_interface_id_create(830, 0, 0, 0, &(art_blit_info.art_id));
        }

        art_blit_info.flags = 0;
        if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
            return;
        }

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = art_frame_data.width;
        src_rect.height = art_frame_data.height;

        dst_rect.y = schematic_ui_component_rects[ingr].y;
        dst_rect.x = schematic_ui_component_rects[ingr].x;
        dst_rect.width = art_frame_data.width;
        dst_rect.height = art_frame_data.height;

        art_blit_info.src_rect = &src_rect;
        art_blit_info.dst_rect = &dst_rect;
        tig_window_blit_art(schematic_ui_window, &art_blit_info);
    }

    // Draw component image.
    art_blit_info.flags = 0;

    // Retrieve proper art ID.
    switch (obj_field_int32_get(item_obj, OBJ_F_TYPE)) {
    case OBJ_TYPE_WEAPON:
        art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_WEAPON_PAPER_DOLL_AID);
        break;
    case OBJ_TYPE_ARMOR:
        art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ARMOR_PAPER_DOLL_AID);
        break;
    default:
        art_blit_info.art_id = TIG_ART_ID_INVALID;
        break;
    }

    if (art_blit_info.art_id != TIG_ART_ID_INVALID) {
        art_blit_info.art_id = tig_art_item_id_disposition_set(art_blit_info.art_id, TIG_ART_ITEM_DISPOSITION_SCHEMATIC);
        if (tig_art_exists(art_blit_info.art_id) != TIG_OK) {
            art_blit_info.art_id = TIG_ART_ID_INVALID;
        }
    }

    if (art_blit_info.art_id == TIG_ART_ID_INVALID) {
        art_blit_info.art_id = obj_field_int32_get(item_obj, OBJ_F_ITEM_INV_AID);
    }

    if (tig_art_frame_data(art_blit_info.art_id, &art_frame_data) != TIG_OK) {
        return;
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    dst_rect = schematic_ui_component_rects[ingr];

    // Scale the image to fit the component slot.
    width_ratio = (float)art_frame_data.width / (float)schematic_ui_component_rects[ingr].width;
    height_ratio = (float)art_frame_data.height / (float)schematic_ui_component_rects[ingr].height;

    if (width_ratio > height_ratio && width_ratio > 1.0f) {
        dst_rect.height = (int)(art_frame_data.height / width_ratio);
        dst_rect.y += (schematic_ui_component_rects[ingr].height - dst_rect.height) / 2;
    } else if (height_ratio > width_ratio && height_ratio > 1.0f) {
        dst_rect.width = (int)(art_frame_data.width / height_ratio);
        dst_rect.x += (schematic_ui_component_rects[ingr].width - dst_rect.width) / 2;
    } else {
        dst_rect.width = art_frame_data.width;
        dst_rect.x += (schematic_ui_component_rects[ingr].width - art_frame_data.width) / 2;
        dst_rect.height = art_frame_data.height;
        dst_rect.y += (schematic_ui_component_rects[ingr].height - art_frame_data.height) / 2;
    }

    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    // Apply grayscale effect if expertise is lacking.
    if (!*have_expertise && tig_art_anim_data(art_blit_info.art_id, &art_anim_data) == TIG_OK) {
        palette = tig_palette_create();

        palette_modify_info.flags = TIG_PALETTE_MODIFY_GRAYSCALE;
        palette_modify_info.src_palette = art_anim_data.palette1;
        palette_modify_info.dst_palette = palette;
        tig_palette_modify(&palette_modify_info);

        palette_modify_info.flags = TIG_PALETTE_MODIFY_TINT;
        palette_modify_info.tint_color = tig_color_make(255, 204, 102);
        palette_modify_info.src_palette = palette;
        tig_palette_modify(&palette_modify_info);

        art_blit_info.flags |= TIG_ART_BLT_PALETTE_OVERRIDE;
        art_blit_info.palette = palette;
    } else {
        palette = NULL;
    }

    tig_window_blit_art(schematic_ui_window, &art_blit_info);

    if (palette != NULL) {
        tig_palette_destroy(palette);
    }

    if (complexity > 0) {
        // Draw tech discipline icon.
        str[0] = (char)(tech + '1');
        str[1] = '\0';
        tig_font_push(schematic_ui_icons17_font);
        tig_window_text_write(schematic_ui_window, str, &(schematic_ui_component_icon_rects[ingr]));
        tig_font_pop();

        // Draw complexity score.
        sprintf(str, "%d", complexity);
        tig_font_push(schematic_ui_description_font);
        tig_window_text_write(schematic_ui_window, str, &schematic_ui_component_complexity_rects[ingr]);
        tig_font_pop();
    }
}

/**
 * Processes a schematic combination.
 *
 * 0x56E720
 */
bool schematic_ui_process(int schematic, int64_t primary_obj, int64_t secondary_obj)
{
    SchematicInfo info;
    int64_t obj;
    int ingridient1;
    int ingridient2;
    int64_t ingridient1_obj;
    int64_t ingridient2_obj;
    int prod;
    int64_t loc;
    int qty;
    int64_t prod_obj;

    // Check for title page.
    if (schematic == -1) {
        return false;
    }

    // Retrieve schematic information.
    schematic_ui_info_get(schematic, &info);

    // Find available item for component 1.
    for (ingridient1 = 0; ingridient1 < 3; ingridient1++) {
        obj = sub_4685A0(info.item1[ingridient1]);
        ingridient1_obj = sub_462540(primary_obj, obj, 0x7);
        if (ingridient1_obj != OBJ_HANDLE_NULL) {
            break;
        }
    }
    if (ingridient1 == 3) {
        return false;
    }

    // Find available item for component 2.
    for (ingridient2 = 0; ingridient2 < 3; ingridient2++) {
        obj = sub_4685A0(info.item2[ingridient2]);
        ingridient2_obj = sub_462540(primary_obj, obj, 0x7);
        if (ingridient2_obj != OBJ_HANDLE_NULL) {
            break;
        }
    }
    if (ingridient2 == 3) {
        return false;
    }

    if (ingridient1 != 0) {
        prod = info.prod[ingridient1];
    } else {
        prod = info.prod[ingridient2];
    }

    loc = obj_field_int64_get(primary_obj, OBJ_F_LOCATION);

    // Consume component 1 item.
    if (obj_field_int32_get(ingridient1_obj, OBJ_F_TYPE) == OBJ_TYPE_AMMO) {
        item_ammo_transfer(primary_obj,
            OBJ_HANDLE_NULL,
            1,
            obj_field_int32_get(ingridient1_obj, OBJ_F_AMMO_QUANTITY),
            ingridient1_obj);
    } else {
        object_destroy(ingridient1_obj);
    }

    // Consume component 2 item.
    if (obj_field_int32_get(ingridient2_obj, OBJ_F_TYPE) == OBJ_TYPE_AMMO) {
        item_ammo_transfer(primary_obj,
            OBJ_HANDLE_NULL,
            1,
            obj_field_int32_get(ingridient2_obj, OBJ_F_AMMO_QUANTITY),
            ingridient2_obj);
    } else {
        object_destroy(ingridient2_obj);
    }

    // Create final product items.
    for (qty = 0; qty < info.qty; qty++) {
        if (!object_create(sub_4685A0(prod), loc, &prod_obj)) {
            return false;
        }

        item_transfer(prod_obj, primary_obj);
    }

    ui_schematic_feedback(true, primary_obj, secondary_obj);

    return true;
}

/**
 * Provides feedback for a schematic processing.
 *
 * 0x56E950
 */
bool schematic_ui_feedback(bool success, int64_t primary_obj, int64_t secondary_obj)
{
    MesFileEntry mes_file_entry;
    UiMessage ui_message;

    (void)primary_obj;

    if (success && player_is_local_pc_obj(secondary_obj)) {
        mes_file_entry.num = 2; // "You have successfully combined the items into a new item."
        mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);

        ui_message.type = UI_MSG_TYPE_EXCLAMATION;
        ui_message.str = mes_file_entry.str;
        intgame_message_window_display_msg(&ui_message);

        schematic_ui_redraw();

        gsound_play_sfx(schematic_ui_cur_tech + SND_INTERFACE_COM_HERBAL, 1);
    }

    return true;
}

// 0x56E9D0
char* schematic_ui_product_name(int schematic)
{
    SchematicInfo schematic_info;
    MesFileEntry mes_file_entry;

    schematic_ui_info_get(schematic, &schematic_info);

    mes_file_entry.num = schematic_info.name;
    mes_get_msg(schematic_ui_text_mes_file, &mes_file_entry);
    return mes_file_entry.str;
}

/**
 * Retrieves the product name a learned schematic.
 *
 * 0x56EA10
 */
char* schematic_ui_learned_schematic_product_name(int tech, int degree)
{
    return schematic_ui_product_name(tech_schematic_get(tech, degree));
}

/**
 * Retrieves the component names of a learned schematic.
 *
 * 0x56EA30
 */
void schematic_ui_learned_schematic_component_names(int tech, int degree, char* item1, char* item2)
{
    SchematicInfo schematic_info;
    int64_t obj;

    schematic_ui_info_get(tech_schematic_get(tech, degree), &schematic_info);

    obj = sub_4685A0(schematic_info.item1[0]);
    object_examine(obj, obj, item1);

    obj = sub_4685A0(schematic_info.item2[0]);
    object_examine(obj, obj, item2);
}
