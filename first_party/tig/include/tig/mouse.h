#ifndef TIG_MOUSE_H_
#define TIG_MOUSE_H_

#include "tig/rect.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TigMouseButton {
    TIG_MOUSE_BUTTON_LEFT,
    TIG_MOUSE_BUTTON_RIGHT,
    TIG_MOUSE_BUTTON_MIDDLE,
    TIG_MOUSE_BUTTON_COUNT,
} TigMouseButton;

typedef uint32_t TigMouseStateFlags;

#define TIG_MOUSE_STATE_HIDDEN 0x001u
#define TIG_MOUSE_STATE_LEFT_MOUSE_DOWN 0x002u
#define TIG_MOUSE_STATE_LEFT_MOUSE_DOWN_REPEAT 0x004u
#define TIG_MOUSE_STATE_LEFT_MOUSE_UP 0x008u
#define TIG_MOUSE_STATE_RIGHT_MOUSE_DOWN 0x010u
#define TIG_MOUSE_STATE_RIGHT_MOUSE_DOWN_REPEAT 0x020u
#define TIG_MOUSE_STATE_RIGHT_MOUSE_UP 0x040u
#define TIG_MOUSE_STATE_MIDDLE_MOUSE_DOWN 0x080u
#define TIG_MOUSE_STATE_MIDDLE_MOUSE_DOWN_REPEAT 0x100u
#define TIG_MOUSE_STATE_MIDDLE_MOUSE_UP 0x200u

typedef struct TigMouseState {
    TigMouseStateFlags flags;
    TigRect frame;
    int offset_x;
    int offset_y;
    int x;
    int y;
    int z;
} TigMouseState;

int tig_mouse_init(TigInitInfo* init_info);
void tig_mouse_exit();
void tig_mouse_set_active(bool active);
void tig_mouse_ping();

// Forwards "mouse move" event from Windows Messages to mouse system.
//
// This function is only used from WNDPROC and should not be called in software
// cursor mode (managed by DirectInput).
void tig_mouse_set_position(int x, int y);

// Forwards "mouse down" and "mouse up" events from Windows Messages to mouse
// system.
//
// This function is only used from WNDPROC and should not be called in software
// cursor mode (managed by DirectInput).
void tig_mouse_set_button(int button, bool pressed);

// Obtains current mouse state and returns `TIG_OK`.
//
// Returns `TIG_ERR_NOT_INITIALIZED` if mouse system was not initialized.
int tig_mouse_get_state(TigMouseState* mouse_state);

// Hides mouse cursor.
int tig_mouse_hide();

// Shows mouse cursor.
int tig_mouse_show();

// Renders mouse cursor to screen.
void tig_mouse_display();

// Refreshes internally managed cursor surface.
//
// This function does nothing in hardware cursor mode.
void tig_mouse_cursor_refresh();

// Sets mouse cursor art.
//
// This function does nothing in hardware cursor mode.
int tig_mouse_cursor_set_art_id(tig_art_id_t art_id);

// Sets art offset from the real cursor position.
void tig_mouse_cursor_set_offset(int x, int y);

// Returns art id of mouse cursor.
//
// In hardware cursor mode the result is undefined.
tig_art_id_t tig_mouse_cursor_get_art_id();

// Adds overlay to the mouse cursor.
int tig_mouse_cursor_overlay(tig_art_id_t art_id, int x, int y);

int sub_500560();
void sub_500570();

void tig_mouse_wheel(int dx, int dy);

#ifdef __cplusplus
}
#endif

#endif /* TIG_MOUSE_H_ */
