#ifndef TIG_MESSAGE_H_
#define TIG_MESSAGE_H_

#include "tig/timer.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int(TigMessageKeyboardCallback)(int);

typedef enum TigMessageType {
    TIG_MESSAGE_MOUSE,
    TIG_MESSAGE_BUTTON,
    TIG_MESSAGE_MENU,
    TIG_MESSAGE_QUIT,
    TIG_MESSAGE_TEXT_INPUT,
    TIG_MESSAGE_KEYBOARD,
    TIG_MESSAGE_PING,
    TIG_MESSAGE_TYPE_7,
    TIG_MESSAGE_REDRAW,
} TigMessageType;

typedef enum TigMessageMouseEvent {
    TIG_MESSAGE_MOUSE_LEFT_BUTTON_DOWN,
    TIG_MESSAGE_MOUSE_LEFT_BUTTON_UP,
    TIG_MESSAGE_MOUSE_RIGHT_BUTTON_DOWN,
    TIG_MESSAGE_MOUSE_RIGHT_BUTTON_UP,
    TIG_MESSAGE_MOUSE_MIDDLE_BUTTON_DOWN,
    TIG_MESSAGE_MOUSE_MIDDLE_BUTTON_UP,
    TIG_MESSAGE_MOUSE_MOVE,
    TIG_MESSAGE_MOUSE_IDLE,
    TIG_MESSAGE_MOUSE_WHEEL,
} TigMessageMouseEvent;

typedef struct TigMouseMessageData {
    int x;
    int y;
    int dx;
    int dy;
    TigMessageMouseEvent event;
    bool repeat;
} TigMouseMessageData;

typedef struct TigButtonMessageData {
    tig_button_handle_t button_handle;
    int state;
    int x;
    int y;
} TigButtonMessageData;

typedef struct TigMenuMessageData {
    int menu_bar_handle;
    int drop_down_index;
    int menu_item_index;
} TigMenuMessageData;

typedef struct TigQuitMessageData {
    int exit_code;
} TigQuitMessageData;

typedef struct TigKeyboardMessageData {
    SDL_Keycode key;
    SDL_Scancode scancode;
    bool pressed;
} TigKeyboardMessageData;

typedef struct TigTextInputEvent {
    const char* text;
} TigTextInputEvent;

typedef struct TigMessage {
    tig_timestamp_t timestamp;
    TigMessageType type;
    union {
        TigMouseMessageData mouse;
        TigButtonMessageData button;
        TigMenuMessageData menu;
        TigQuitMessageData quit;
        TigKeyboardMessageData keyboard;
        TigTextInputEvent text;
    } data;
} TigMessage;

int tig_message_init(TigInitInfo* init_info);
void tig_message_exit();
void tig_message_ping();
int tig_message_enqueue(TigMessage* message);
int tig_message_set_key_handler(TigMessageKeyboardCallback* callback, int key);

// Pulls next message from the game's message queue and returns `TIG_OK`.
//
// If the queue is empty returns `TIG_ERR_MESSAGE_QUEUE_EMPTY`.
int tig_message_dequeue(TigMessage* message);

// Adds `TIG_MESSAGE_QUIT` message to the game's message queue and
// returns `TIG_OK`.
int tig_message_post_quit(int exit_code);

#ifdef __cplusplus
}
#endif

#endif /* TIG_MESSAGE_H_ */
