#include "ui/cyclic_ui.h"

#include "game/mes.h"
#include "game/snd.h"

/**
 * The maximum number of cyclic controls.
 */
#define MAX_CONTROLS 32

typedef struct CyclicUiControl {
    /* 0000 */ CyclicUiControlInfo info;
    /* 002C */ tig_button_handle_t left_button_handle;
    /* 0030 */ tig_button_handle_t right_button_handle;
    /* 0034 */ mes_file_handle_t mes_file;
    /* 0038 */ int value;
    /* 003C */ int max_value;
    /* 0040 */ bool in_use;
} CyclicUiControl;

static bool get_control(CyclicUiControl** ctrl_ptr, int id, const char* function_name);
static bool show_control(CyclicUiControl* ctrl);
static bool hide_control(CyclicUiControl* ctrl);
static bool cyclic_ui_draw_base(CyclicUiControl* ctrl);
static bool cyclic_ui_base_aid(CyclicUiControl* ctrl, tig_art_id_t* art_id_ptr);
static bool cyclic_ui_draw_empty_slot(CyclicUiControl* ctrl);
static bool cyclic_ui_draw_title(CyclicUiControl* ctrl);
static bool cyclic_ui_buttons_create(CyclicUiControl* ctrl);
static void cyclic_ui_buttons_destroy(CyclicUiControl* ctrl);
static bool cyclic_ui_refresh_level(CyclicUiControl* ctrl);
static bool cyclic_ui_draw_bar(CyclicUiControl* ctrl);
static bool cyclic_ui_draw_text(CyclicUiControl* ctrl);
static void cycle_left(CyclicUiControl* ctrl);
static void cycle_right(CyclicUiControl* ctrl);

/**
 * 0x6839B8
 */
static tig_font_handle_t cyclic_ui_morph15_white_font;

/**
 * Array of cyclic UI controls.
 *
 * 0x6839C0
 */
static CyclicUiControl cyclic_ui_controls[MAX_CONTROLS];

/**
 * 0x684240
 */
static tig_font_handle_t cyclic_ui_morph15_gray_font;

/**
 * 0x684244
 */
static tig_font_handle_t cyclic_ui_flare12_red_font;

/**
 * Tracks the last assigned control ID to optimize slot allocation.
 *
 * 0x684248
 */
static int cyclic_ui_prev_id;

/**
 * Indicates whether the cyclic UI system is initialized.
 *
 * 0x68424C
 */
static bool cyclic_ui_initialized;

/**
 * Called when the game is initialized.
 *
 * 0x57F4D0
 */
bool cyclic_ui_init(GameInitInfo* init_info)
{
    int index;
    TigFont font_desc;

    (void)init_info;

    // Prevent re-initialization.
    if (cyclic_ui_initialized) {
        return true;
    }

    // Initialize all control slots as unused.
    for (index = 0; index < MAX_CONTROLS; index++) {
        cyclic_ui_controls[index].in_use = false;
        cyclic_ui_controls[index].left_button_handle = TIG_BUTTON_HANDLE_INVALID;
        cyclic_ui_controls[index].right_button_handle = TIG_BUTTON_HANDLE_INVALID;
        cyclic_ui_controls[index].mes_file = MES_FILE_HANDLE_INVALID;
    }

    cyclic_ui_prev_id = MAX_CONTROLS - 1;

    // Set up fonts.
    if (tig_art_interface_id_create(229, 0, 0, 0, &(font_desc.art_id)) != TIG_OK) {
        return false;
    }

    font_desc.flags = TIG_FONT_CENTERED;
    font_desc.str = NULL;
    font_desc.color = tig_color_make(255, 0, 0);
    tig_font_create(&font_desc, &cyclic_ui_flare12_red_font);

    if (tig_art_interface_id_create(27, 0, 0, 0, &(font_desc.art_id)) != TIG_OK) {
        return false;
    }

    font_desc.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_desc, &cyclic_ui_morph15_white_font);

    font_desc.color = tig_color_make(130, 130, 130);
    tig_font_create(&font_desc, &cyclic_ui_morph15_gray_font);

    cyclic_ui_initialized = true;

    return true;
}

/**
 * Called when the game shuts down.
 *
 * 0x57F680
 */
void cyclic_ui_exit(void)
{
    int index;

    if (!cyclic_ui_initialized) {
        return;
    }

    // Destroy all active controls.
    for (index = 0; index < MAX_CONTROLS; index++) {
        if (cyclic_ui_controls[index].in_use) {
            cyclic_ui_control_destroy(index, true);
        }
    }

    // Clean up font resources.
    tig_font_destroy(cyclic_ui_flare12_red_font);
    tig_font_destroy(cyclic_ui_morph15_white_font);

    // FIX: Memory leak.
    tig_font_destroy(cyclic_ui_morph15_gray_font);
}

/**
 * Handles button press events for cyclic UI controls.
 *
 * Returns `true` if the `button_handle` belongs to one of cyclic UI controls
 * and the press was handled, `false` otherwise.
 *
 * 0x57F6D0
 */
bool cyclic_ui_handle_button_pressed(tig_button_handle_t button_handle)
{
    int index;
    CyclicUiControl* ctrl;

    for (index = 0; index < MAX_CONTROLS; index++) {
        ctrl = &(cyclic_ui_controls[index]);
        if (ctrl->in_use) {
            if (button_handle == ctrl->left_button_handle) {
                cycle_left(ctrl);
                return true;
            } else if (button_handle == ctrl->right_button_handle) {
                cycle_right(ctrl);
                return true;
            }
        }
    }

    return false;
}

/**
 * Initializes a cyclic UI control configuration with default values.
 *
 * 0x57F720
 */
void cyclic_ui_control_init(CyclicUiControlInfo* info)
{
    info->x = 0;
    info->y = 0;
    info->type = 0;
    info->max_value = 0;
    info->title = NULL;
    info->mes_file_path = NULL;
    info->text_array = NULL;
    info->text_array_size = 0;
    info->value_changed_callback = NULL;
    info->window_handle = TIG_WINDOW_HANDLE_INVALID;
    info->visible = true;
    info->enabled = true;
}

/**
 * Creates a new cyclic UI control.
 *
 * Returns `true` if control was successfully created, `false` otherwise.
 *
 * 0x57F750
 */
bool cyclic_ui_control_create(CyclicUiControlInfo* info, int* id_ptr)
{
    int index;
    CyclicUiControl* ctrl;

    // Start searching for a free slot.
    index = cyclic_ui_prev_id + 1;
    if (index >= MAX_CONTROLS) {
        index = 0;
    }

    // Find a free slot or detect if all slots are used.
    while (index != cyclic_ui_prev_id && cyclic_ui_controls[index].in_use) {
        index++;

        // Wrap around.
        if (index == MAX_CONTROLS) {
            index = 0;
        }
    }

    if (index == cyclic_ui_prev_id) {
        tig_debug_println("Error, cyclic_ui_control_create:  Too many cyclic ui controls.");
        return false;
    }

    ctrl = &(cyclic_ui_controls[index]);
    ctrl->info = *info;
    ctrl->value = 0;

    // Initialize max value based on control type.
    switch (ctrl->info.type) {
    case CYCLIC_UI_CONTROL_NUMERIC_BAR:
        // The default scale for numeric bars is 0-10, probably representing
        // 0-100% with 10% increments.
        ctrl->max_value = 10;
        break;
    case CYCLIC_UI_CONTROL_MESSAGE_FILE:
        if (!mes_load(ctrl->info.mes_file_path, &(ctrl->mes_file))) {
            tig_debug_printf("Error, cyclic_ui_control_create:  Unable to load message file [%s]\n", ctrl->info.mes_file_path);
            return false;
        }
        ctrl->max_value = mes_num_entries(ctrl->mes_file) - 1;
        break;
    case CYCLIC_UI_CONTROL_TEXT_ARRAY:
        if (ctrl->info.text_array == NULL) {
            tig_debug_println("Error, cyclic_ui_control_create:  Control type is cui_text_array, but text_array is NULL");
            return false;
        }
        ctrl->max_value = ctrl->info.text_array_size - 1;
        break;
    default:
        tig_debug_println("Error, cyclic_ui_control_create:  Invalid control type");
        return false;
    }

    // FIXME: This should likely be moved to `CYCLIC_UI_CONTROL_NUMERIC_BAR`, as
    // the maximum value for text controls is derived from the message file or
    // text array size.
    if (ctrl->info.max_value != 0) {
        ctrl->max_value = ctrl->info.max_value;
    }

    ctrl->in_use = true;

    // Render visual elements.
    if (ctrl->info.visible) {
        if (!show_control(ctrl)) {
            ctrl->in_use = false;
            return false;
        }
    }

    cyclic_ui_prev_id = index;
    *id_ptr = index;

    return true;
}

/**
 * Destroys a cyclic control.
 *
 * 0x57F8A0
 */
void cyclic_ui_control_destroy(int id, bool exiting)
{
    CyclicUiControl* ctrl;

    if (!get_control(&ctrl, id, __FUNCTION__)) {
        return;
    }

    if (ctrl->info.type == CYCLIC_UI_CONTROL_MESSAGE_FILE) {
        mes_unload(ctrl->mes_file);
    }

    if (exiting) {
        // NOTE: I guess this code path assumes that since we are in
        // `cyclic_ui_exit`, there is no need to destroy individual buttons. By
        // destroying the parent window or the entire button system, we can
        // prevent handle leaks.
        ctrl->left_button_handle = TIG_BUTTON_HANDLE_INVALID;
        ctrl->right_button_handle = TIG_BUTTON_HANDLE_INVALID;
        ctrl->in_use = false;
    } else {
        if (ctrl->info.visible) {
            hide_control(ctrl);
        }
        ctrl->in_use = false;
    }
}

/**
 * Shows or hides a cyclic control.
 *
 * 0x57F910
 */
void cyclic_ui_control_show(int id, bool visible)
{
    CyclicUiControl* ctrl;

    if (!get_control(&ctrl, id, __FUNCTION__)) {
        return;
    }

    // Skip if visibility state is unchanged.
    if (visible != ctrl->info.visible) {
        return;
    }

    // Show or hide the control.
    if (visible) {
        if (show_control(ctrl)) {
            ctrl->info.visible = true;
        }
    } else {
        if (hide_control(ctrl)) {
            ctrl->info.visible = false;
        }
    }
}

/**
 * Enables or disables a cyclic control.
 *
 * 0x57F970
 */
void cyclic_ui_control_enable(int id, bool enabled)
{
    CyclicUiControl* ctrl;

    if (!get_control(&ctrl, id, __FUNCTION__)) {
        return;
    }

    // Skip if enabled state is unchanged.
    if (enabled == ctrl->info.enabled) {
        return;
    }

    ctrl->info.enabled = enabled;

    // Skip visual updates if not visible.
    if (!ctrl->info.visible) {
        return;
    }

    // Destroy buttons if disabling.
    if (!ctrl->info.enabled) {
        cyclic_ui_buttons_destroy(ctrl);
    }

    // Refresh display to reflect the new state.
    show_control(ctrl);
}

/**
 * Sets the value of a cyclic control.
 *
 * 0x57F9D0
 */
void cyclic_ui_control_set(int id, int value)
{
    CyclicUiControl* ctrl;

    if (!get_control(&ctrl, id, __FUNCTION__)) {
        return;
    }

    if (value < 0) {
        tig_debug_println("Error, cyclic_ui_control_set:  value is sub-zero");
        return;
    }

    if (value > ctrl->max_value) {
        tig_debug_printf("Error, cyclic_ui_control_set:  value exceeds limit (v:%d l:%d)\n",
            value,
            ctrl->max_value);
        return;
    }

    ctrl->value = value;

    if (ctrl->info.visible) {
        cyclic_ui_refresh_level(ctrl);
    }
}

/**
 * Retrieves the current value of a cyclic control.
 *
 * 0x57FA40
 */
int cyclic_ui_control_get(int id)
{
    CyclicUiControl* ctrl;

    if (!get_control(&ctrl, id, __FUNCTION__)) {
        return 0;
    }

    return ctrl->value;
}

/**
 * Internal helper to retrieve an active control by ID.
 *
 * Returns `true` if the control ID is valid and is active, `false` otherwise.
 *
 * 0x57FA70
 */
bool get_control(CyclicUiControl** ctrl_ptr, int id, const char* function_name)
{
    if (id < 0 || id >= MAX_CONTROLS) {
        tig_debug_printf("Error, %s:  id out of range\n", function_name);
        *ctrl_ptr = NULL;
        return false;
    }

    if (!cyclic_ui_controls[id].in_use) {
        tig_debug_printf("Error, %s:  id not in use\n", function_name);
        *ctrl_ptr = NULL;
        return false;
    }

    *ctrl_ptr = &(cyclic_ui_controls[id]);
    return true;
}

/**
 * Displays a control.
 *
 * 0x57FAD0
 */
bool show_control(CyclicUiControl* ctrl)
{
    // Draw background.
    if (!cyclic_ui_draw_base(ctrl)) {
        return false;
    }

    // Draw title.
    if (!cyclic_ui_draw_title(ctrl)) {
        return false;
    }

    // Draw value (liquid bar or text, depending on control type).
    if (!cyclic_ui_refresh_level(ctrl)) {
        return false;
    }

    // Create cycle buttons if enabled.
    if (ctrl->info.enabled) {
        if (!cyclic_ui_buttons_create(ctrl)) {
            return false;
        }
    }

    return true;
}

/**
 * Hides a control, leaving an empty slot.
 *
 * 0x57FB20
 */
bool hide_control(CyclicUiControl* ctrl)
{
    if (ctrl->info.enabled) {
        cyclic_ui_buttons_destroy(ctrl);
    }

    return cyclic_ui_draw_empty_slot(ctrl);
}

/**
 * Draws the base graphic for a control.
 *
 * 0x57FB50
 */
bool cyclic_ui_draw_base(CyclicUiControl* ctrl)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo blit_info;
    TigRect src_rect;
    TigRect dst_rect;

    if (!cyclic_ui_base_aid(ctrl, &art_id)) {
        return false;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_base:  Unable to get frame data");
        return false;
    }

    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    dst_rect.x = ctrl->info.x;
    dst_rect.y = ctrl->info.y;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;

    blit_info.flags = 0;
    blit_info.art_id = art_id;
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;

    if (tig_window_blit_art(ctrl->info.window_handle, &blit_info) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_base:  Blit failed");
        return false;
    }

    return true;
}

/**
 * Retrieves the art ID for a control based on it's type.
 *
 * 0x57FC20
 */
bool cyclic_ui_base_aid(CyclicUiControl* ctrl, tig_art_id_t* art_id_ptr)
{
    int num;
    tig_art_id_t art_id;

    switch (ctrl->info.type) {
    case CYCLIC_UI_CONTROL_NUMERIC_BAR:
        num = 688; // "optionslider.art"
        break;
    case CYCLIC_UI_CONTROL_MESSAGE_FILE:
    case CYCLIC_UI_CONTROL_TEXT_ARRAY:
        num = 671; // "optionmultichoice.art"
        break;
    default:
        tig_debug_println("Error, cyclic_ui_base_aid:  Invalid control type");
        return false;
    }

    if (tig_art_interface_id_create(num, 0, 0, 0, &art_id) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_base_aid:  Can't get aid");
        return false;
    }

    *art_id_ptr = art_id;
    return true;
}

/**
 * Draws an empty slot for a control.
 *
 * 0x57FC90
 */
bool cyclic_ui_draw_empty_slot(CyclicUiControl* ctrl)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo blit_info;
    TigRect src_rect;
    TigRect dst_rect;

    // Retrieve base graphic.
    if (!cyclic_ui_base_aid(ctrl, &art_id)) {
        return false;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_empty_slot:  Unable to get frame data");
        return false;
    }

    // Set up src slot rect.
    //
    // FIX: There is no specific "empty slot" graphic. The original code uses
    // the top-left slot (origin: 261x81, size: 213x56) from the options
    // background image as a template. However, this is incorrect, as the
    // background mimics wood, and each empty slot is quite different. Since
    // cyclic controls are used exclusively in the options UI, and  we know that
    // each control occupies exactly one slot, we can simply reuse the
    // destination position (adjusted for the top bar) as the source. This
    // approach ensures that every empty slot will have the appropriate
    // background.
    src_rect.x = ctrl->info.x;
    src_rect.y = ctrl->info.y - 41;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    // Set up dst slot rect.
    dst_rect.x = ctrl->info.x;
    dst_rect.y = ctrl->info.y;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;

    tig_art_interface_id_create(669, 0, 0, 0, &art_id);

    blit_info.flags = 0;
    blit_info.art_id = art_id;
    blit_info.src_rect = &src_rect;
    blit_info.dst_rect = &dst_rect;

    // Blit the empty slot graphic.
    if (tig_window_blit_art(ctrl->info.window_handle, &blit_info) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_empty_slot:  Blit failed");
        return false;
    }

    return true;
}

/**
 * Draws the title for a control.
 *
 * 0x57FD80
 */
bool cyclic_ui_draw_title(CyclicUiControl* ctrl)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigRect text_rect;

    // Retrieve base graphic.
    if (!cyclic_ui_base_aid(ctrl, &art_id)) {
        return false;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_title:  Unable to get base frame data for size reference");
        return false;
    }

    // Draw title if provided.
    if (ctrl->info.title != NULL && ctrl->info.title[0] != '\0') {
        text_rect.x = ctrl->info.x;
        text_rect.y = ctrl->info.y + 2;
        text_rect.width = art_frame_data.width;
        text_rect.height = art_frame_data.height - 2;

        if (ctrl->info.enabled) {
            tig_font_push(cyclic_ui_morph15_white_font);
        } else {
            tig_font_push(cyclic_ui_morph15_gray_font);
        }

        if (tig_window_text_write(ctrl->info.window_handle, ctrl->info.title, &text_rect) != TIG_OK) {
            tig_debug_println("Error, cyclic_ui_draw_title:  Text write failed");
            tig_font_pop();
            return false;
        }

        tig_font_pop();
    }

    return true;
}

/**
 * Creates cycle buttons for a control.
 *
 * 0x57FE60
 */
bool cyclic_ui_buttons_create(CyclicUiControl* ctrl)
{
    TigButtonData button_data;

    // Configure common button properties.
    button_data.flags = TIG_BUTTON_MOMENTARY;
    button_data.window_handle = ctrl->info.window_handle;
    button_data.mouse_down_snd_id = SND_INTERFACE_BUTTON_MEDIUM;
    button_data.mouse_up_snd_id = SND_INTERFACE_BUTTON_MEDIUM_RELEASE;
    button_data.mouse_enter_snd_id = -1;
    button_data.mouse_exit_snd_id = -1;

    // Create left button.
    if (tig_art_interface_id_create(667, 0, 0, 0, &(button_data.art_id)) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_buttons_create:  Can't get aid for left button art");
        return false;
    }

    button_data.x = ctrl->info.x + 6;
    button_data.y = ctrl->info.y + 21;

    if (tig_button_create(&button_data, &(ctrl->left_button_handle)) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_buttons_create:  Failed to create left button");
        return false;
    }

    // Create right button.
    if (tig_art_interface_id_create(668, 0, 0, 0, &(button_data.art_id)) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_buttons_create:  Can't get aid for right button art");
        return false;
    }

    button_data.x = ctrl->info.x + 180;
    button_data.y = ctrl->info.y + 21;

    if (tig_button_create(&button_data, &(ctrl->right_button_handle)) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_buttons_create:  Failed to create right button");
        return false;
    }

    return true;
}

/**
 * Destroys cycle buttons for a control.
 *
 * 0x57FF80
 */
void cyclic_ui_buttons_destroy(CyclicUiControl* ctrl)
{
    tig_button_destroy(ctrl->left_button_handle);
    tig_button_destroy(ctrl->right_button_handle);
    ctrl->left_button_handle = TIG_BUTTON_HANDLE_INVALID;
    ctrl->right_button_handle = TIG_BUTTON_HANDLE_INVALID;
}

/**
 * Updates the visual representation of a control's current value.
 *
 * 0x57FFB0
 */
bool cyclic_ui_refresh_level(CyclicUiControl* ctrl)
{
    switch (ctrl->info.type) {
    case CYCLIC_UI_CONTROL_NUMERIC_BAR:
        return cyclic_ui_draw_bar(ctrl);
    case CYCLIC_UI_CONTROL_MESSAGE_FILE:
    case CYCLIC_UI_CONTROL_TEXT_ARRAY:
        return cyclic_ui_draw_text(ctrl);
    }

    tig_debug_println("Error, cyclic_ui_refresh_level:  Invalid control type");
    return false;
}

/**
 * Draws a gauge bar for a control.
 *
 * 0x580000
 */
bool cyclic_ui_draw_bar(CyclicUiControl* ctrl)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    int width;

    if (tig_art_interface_id_create(623, 0, 0, 0, &art_id) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_bar:  Unable to get liquid aid");
        return false;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_bar:  Unable to get liquid frame data");
        return false;
    }

    // Calculate bar width based on current value.
    width = art_frame_data.width * ctrl->value / ctrl->max_value;

    // Set up src content rect.
    src_rect.x = 0;
    src_rect.y = 0;
    src_rect.width = width;
    src_rect.height = art_frame_data.height;

    // Set up dst content rect.
    dst_rect.x = ctrl->info.x + 47;
    dst_rect.y = ctrl->info.y + 24;
    dst_rect.width = width;
    dst_rect.height = art_frame_data.height;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    // Draw the filled portion of the bar.
    if (width != 0) {
        if (tig_window_blit_art(ctrl->info.window_handle, &art_blit_info) != TIG_OK) {
            tig_debug_println("Error, cyclic_ui_draw_bar:  Liquid blit failed");
            return false;
        }
    }

    // Retrieve base graphic.
    if (!cyclic_ui_base_aid(ctrl, &art_id)) {
        return false;
    }

    // Set up src content rect to be the remainder of the graphic.
    src_rect.x = width + 47;
    src_rect.y = 24;
    src_rect.width = art_frame_data.width - width;

    // Draw the empty portion of the bar.
    if (src_rect.width != 0) {
        dst_rect.x = ctrl->info.x + width + 47;
        dst_rect.y = ctrl->info.y + 24;
        dst_rect.width = art_frame_data.width - width;

        art_blit_info.art_id = art_id;
        if (tig_window_blit_art(ctrl->info.window_handle, &art_blit_info) != TIG_OK) {
            tig_debug_println("Error, cyclic_ui_draw_bar:  Base blit failed");
            return false;
        }
    }

    return true;
}

/**
 * Draws text value for a control.
 *
 * 0x580190
 */
bool cyclic_ui_draw_text(CyclicUiControl* ctrl)
{
    tig_art_id_t art_id;
    TigArtFrameData art_frame_data;
    TigArtBlitInfo art_blit_info;
    TigRect src_rect;
    TigRect dst_rect;
    TigRect text_rect;
    MesFileEntry mes_file_entry;
    const char* str;

    if (tig_art_interface_id_create(623, 0, 0, 0, &art_id) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_text:  Unable to get liquid aid for size reference");
        return false;
    }

    if (tig_art_frame_data(art_id, &art_frame_data) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_text:  Unable to get liquid frame data for size reference");
        return false;
    }

    // Retrieve base graphic.
    if (!cyclic_ui_base_aid(ctrl, &art_id)) {
        return false;
    }

    // Set up src content rect.
    src_rect.x = 47;
    src_rect.y = 24;
    src_rect.width = art_frame_data.width;
    src_rect.height = art_frame_data.height;

    // Set up dst content rect.
    dst_rect.x = ctrl->info.x + 47;
    dst_rect.y = ctrl->info.y + 24;
    dst_rect.width = art_frame_data.width;
    dst_rect.height = art_frame_data.height;

    art_blit_info.flags = 0;
    art_blit_info.art_id = art_id;
    art_blit_info.src_rect = &src_rect;
    art_blit_info.dst_rect = &dst_rect;

    // Erase previous content by drawing background.
    if (tig_window_blit_art(ctrl->info.window_handle, &art_blit_info) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_text:  Erase blit failed");
        return false;
    }

    // Retrieve text based on control type.
    switch (ctrl->info.type) {
    case CYCLIC_UI_CONTROL_MESSAGE_FILE:
        mes_file_entry.num = ctrl->value;
        if (mes_search(ctrl->mes_file, &mes_file_entry)) {
            mes_get_msg(ctrl->mes_file, &mes_file_entry);
            str = mes_file_entry.str;
        } else {
            str = NULL;
        }
        break;
    case CYCLIC_UI_CONTROL_TEXT_ARRAY:
        str = ctrl->info.text_array[ctrl->value];
        if (str != NULL && *str == '\0') {
            str = NULL;
        }
        break;
    default:
        str = NULL;
        break;
    }

    if (str == NULL) {
        tig_debug_printf("Warning, cyclic_ui_draw_text:  [%s] is missing line %d\n",
            ctrl->info.mes_file_path,
            ctrl->value);
        return true;
    }

    // Set up dst text rect.
    text_rect.x = ctrl->info.x + 47;
    text_rect.y = ctrl->info.y + 30;
    text_rect.width = art_frame_data.width;
    text_rect.height = art_frame_data.height;

    // Draw text value.
    tig_font_push(cyclic_ui_flare12_red_font);
    if (tig_window_text_write(ctrl->info.window_handle, str, &text_rect) != TIG_OK) {
        tig_debug_println("Error, cyclic_ui_draw_text:  Text write failed");
        tig_font_pop();
        return false;
    }
    tig_font_pop();

    return true;
}

/**
 * Cycles the control's value to the left.
 *
 * 0x580390
 */
void cycle_left(CyclicUiControl* ctrl)
{
    if (ctrl->value > 0) {
        ctrl->value--;
    } else {
        // Numeric bars do not wrap around.
        if (ctrl->info.type == CYCLIC_UI_CONTROL_NUMERIC_BAR) {
            return;
        }

        ctrl->value = ctrl->max_value;
    }

    // Notify callback of value change.
    if (ctrl->info.value_changed_callback != NULL) {
        ctrl->info.value_changed_callback(ctrl->value);
    }

    // Redraw.
    cyclic_ui_refresh_level(ctrl);
}

/**
 * Cycles the control's value to the right.
 *
 * 0x5803D0
 */
void cycle_right(CyclicUiControl* ctrl)
{
    if (ctrl->value < ctrl->max_value) {
        ctrl->value++;
    } else {
        // Numeric bars do not wrap around.
        if (ctrl->info.type == CYCLIC_UI_CONTROL_NUMERIC_BAR) {
            return;
        }

        ctrl->value = 0;
    }

    // Notify callback of value change.
    if (ctrl->info.value_changed_callback != NULL) {
        ctrl->info.value_changed_callback(ctrl->value);
    }

    // Redraw.
    cyclic_ui_refresh_level(ctrl);
}
