#ifndef ARCANUM_UI_CYCLIC_UI_H_
#define ARCANUM_UI_CYCLIC_UI_H_

#include "game/context.h"

typedef enum CyclicUiControlType {
    CYCLIC_UI_CONTROL_NUMERIC_BAR,
    CYCLIC_UI_CONTROL_MESSAGE_FILE,
    CYCLIC_UI_CONTROL_TEXT_ARRAY,
} CyclicUiControlType;

typedef void CyclicUiControlValueChanged(int value);

typedef struct CyclicUiControlInfo {
    /* 0000 */ tig_window_handle_t window_handle;
    /* 0004 */ int x;
    /* 0008 */ int y;
    /* 000C */ CyclicUiControlType type;
    /* 0010 */ int max_value;
    /* 0014 */ const char* title;
    /* 0018 */ const char* mes_file_path;
    /* 001C */ const char* const* text_array;
    /* 0020 */ int text_array_size;
    /* 0024 */ CyclicUiControlValueChanged* value_changed_callback;
    /* 0028 */ bool visible;
    /* 0029 */ bool enabled;
} CyclicUiControlInfo;

bool cyclic_ui_init(GameInitInfo* init_info);
void cyclic_ui_exit(void);
bool cyclic_ui_handle_button_pressed(tig_button_handle_t button_handle);
void cyclic_ui_control_init(CyclicUiControlInfo* info);
bool cyclic_ui_control_create(CyclicUiControlInfo* info, int* id_ptr);
void cyclic_ui_control_destroy(int id, bool exiting);
void cyclic_ui_control_show(int id, bool visible);
void cyclic_ui_control_enable(int id, bool enabled);
void cyclic_ui_control_set(int id, int value);
int cyclic_ui_control_get(int id);

#endif /* ARCANUM_UI_CYCLIC_UI_H_ */
