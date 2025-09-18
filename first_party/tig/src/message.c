#include "tig/message.h"

#include "tig/button.h"
#include "tig/core.h"
#include "tig/kb.h"
#include "tig/memory.h"
#include "tig/mouse.h"
#include "tig/movie.h"
#include "tig/timer.h"
#include "tig/video.h"
#include "tig/window.h"

// Maximum number of key handler.
#define MAX_KEY_HANDLERS 16

typedef struct TigMessageListNode {
    /* 0000 */ TigMessage message;
    /* 001C */ struct TigMessageListNode* next;
} TigMessageListNode;

typedef struct TigMessageKeyboardHandler {
    /* 0000 */ int key;
    /* 0004 */ TigMessageKeyboardCallback* callback;
} TigMessageKeyboardHandler;

static TigMessageListNode* tig_message_node_acquire();
static void tig_message_node_reserve();
static void tig_message_node_release(TigMessageListNode* node);

// 0x62B1A8
static bool tig_message_key_triggers[MAX_KEY_HANDLERS];

// 0x62B1F0
static TigMessageKeyboardHandler tig_message_key_handlers[MAX_KEY_HANDLERS];

// 0x62B278
static SDL_Mutex* tig_message_mutex;

// 0x62B290
static TigMessageListNode* tig_message_empty_node_head;

// 0x62B294
static TigMessageListNode* tig_message_queue_head;

// 0x62B298
static int tig_message_key_handlers_count;

// 0x52B6D0
int tig_message_init(TigInitInfo* init_info)
{
    (void)init_info;

    tig_message_mutex = SDL_CreateMutex();

    tig_message_empty_node_head = NULL;
    tig_message_queue_head = NULL;
    tig_message_key_handlers_count = 0;

    return TIG_OK;
}

// 0x52B750
void tig_message_exit()
{
    TigMessageListNode* next;

    while (tig_message_queue_head != NULL) {
        next = tig_message_queue_head->next;
        FREE(tig_message_queue_head);
        tig_message_queue_head = next;
    }

    while (tig_message_empty_node_head != NULL) {
        next = tig_message_empty_node_head->next;
        FREE(tig_message_empty_node_head);
        tig_message_empty_node_head = next;
    }

    SDL_DestroyMutex(tig_message_mutex);
    tig_message_mutex = NULL;
}

// 0x52B7B0
void tig_message_ping()
{
    TigMessage message;
    SDL_Event event;
    SDL_Renderer* renderer;
    int index;

    message.type = TIG_MESSAGE_PING;
    message.timestamp = tig_ping_timestamp;
    tig_message_enqueue(&message);

    for (index = 0; index < tig_message_key_handlers_count; index++) {
        if (tig_kb_is_key_pressed(tig_message_key_handlers[index].key)) {
            if (!tig_message_key_triggers[index]) {
                tig_message_key_triggers[index] = true;
                tig_message_key_handlers[index].callback(tig_message_key_handlers[index].key);
            }
        } else {
            tig_message_key_triggers[index] = false;
        }
    }

    if (tig_video_renderer_get(&renderer) != TIG_OK) {
        return;
    }

    while (SDL_PollEvent(&event)) {
        SDL_ConvertEventToRenderCoordinates(renderer, &event);

        switch (event.type) {
        case SDL_EVENT_WINDOW_MOUSE_ENTER:
            tig_set_active(true);
            break;
        case SDL_EVENT_WINDOW_MOUSE_LEAVE:
            tig_set_active(false);
            break;
        case SDL_EVENT_MOUSE_MOTION:
            tig_mouse_set_position((int)event.motion.x, (int)event.motion.y);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            switch (event.button.button) {
            case SDL_BUTTON_LEFT:
                tig_mouse_set_button(TIG_MOUSE_BUTTON_LEFT, event.button.down);
                break;
            case SDL_BUTTON_RIGHT:
                tig_mouse_set_button(TIG_MOUSE_BUTTON_RIGHT, event.button.down);
                break;
            case SDL_BUTTON_MIDDLE:
                tig_mouse_set_button(TIG_MOUSE_BUTTON_MIDDLE, event.button.down);
                break;
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            tig_mouse_wheel(event.wheel.integer_x, event.wheel.integer_y);
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            tig_kb_set_key(event.key.key, event.key.scancode, event.key.down);
            break;
        case SDL_EVENT_TEXT_INPUT:
            message.type = TIG_MESSAGE_TEXT_INPUT;
            message.data.text.text = event.text.text;
            tig_message_enqueue(&message);
            break;
        case SDL_EVENT_QUIT:
            tig_message_post_quit(0);
            break;
        }
    }
}

// 0x52B890
int tig_message_enqueue(TigMessage* message)
{
    TigMessageListNode* node;
    TigMessageListNode* prev;

    SDL_LockMutex(tig_message_mutex);

    node = tig_message_node_acquire();
    node->message = *message;
    node->next = NULL;

    if (message->type == TIG_MESSAGE_REDRAW || tig_message_queue_head == NULL) {
        // Redraw is always placed on top of the message queue.
        node->next = tig_message_queue_head;
        tig_message_queue_head = node;
    } else {
        // Append new node to the end of the message queue.
        prev = tig_message_queue_head;
        while (prev->next != NULL) {
            prev = prev->next;
        }

        prev->next = node;
    }

    SDL_UnlockMutex(tig_message_mutex);

    return TIG_OK;
}

// 0x52B920
int tig_message_set_key_handler(TigMessageKeyboardCallback* callback, int key)
{
    int index;

    if (key < 0) {
        // Bad key code.
        return TIG_ERR_INVALID_PARAM;
    }

    if (callback == NULL) {
        // Find handler to remove.
        for (index = 0; index < MAX_KEY_HANDLERS; index++) {
            if (tig_message_key_handlers[index].key == key) {
                break;
            }
        }

        if (index >= MAX_KEY_HANDLERS) {
            // Not found.
            return TIG_ERR_INVALID_PARAM;
        }

        // Reorder subsequent slots.
        while (index + 1 < tig_message_key_handlers_count) {
            tig_message_key_triggers[index] = tig_message_key_triggers[index + 1];
            tig_message_key_handlers[index] = tig_message_key_handlers[index + 1];
            index++;
        }

        tig_message_key_handlers_count--;

        return TIG_OK;
    }

    if (tig_message_key_handlers_count >= MAX_KEY_HANDLERS) {
        return TIG_ERR_OUT_OF_HANDLES;
    }

    tig_message_key_handlers[tig_message_key_handlers_count].key = key;
    tig_message_key_handlers[tig_message_key_handlers_count].callback = callback;
    tig_message_key_triggers[tig_message_key_handlers_count] = false;
    tig_message_key_handlers_count++;

    return TIG_OK;
}

// 0x52BDA0
int tig_message_dequeue(TigMessage* message)
{
    TigMessageListNode* next;
    TigMessage temp_message;

    if (tig_message_queue_head == NULL) {
        return TIG_ERR_MESSAGE_QUEUE_EMPTY;
    }

    SDL_LockMutex(tig_message_mutex);

    while (tig_message_queue_head != NULL) {
        next = tig_message_queue_head->next;
        temp_message = tig_message_queue_head->message;
        tig_message_node_release(tig_message_queue_head);
        tig_message_queue_head = next;

        if (!tig_movie_is_playing()) {
            if (temp_message.type == TIG_MESSAGE_MOUSE
                && tig_button_process_mouse_msg(&(temp_message.data.mouse))) {
                continue;
            }

            if (tig_window_filter_message(&temp_message)
                && temp_message.type != TIG_MESSAGE_REDRAW) {
                continue;
            }
        }

        *message = temp_message;

        SDL_UnlockMutex(tig_message_mutex);

        return TIG_OK;
    }

    SDL_UnlockMutex(tig_message_mutex);

    return TIG_ERR_MESSAGE_QUEUE_EMPTY;
}

// 0x52BE60
int tig_message_post_quit(int exit_code)
{
    TigMessage message;
    tig_timer_now(&(message.timestamp));
    message.type = TIG_MESSAGE_QUIT;
    message.data.quit.exit_code = exit_code;
    return tig_message_enqueue(&message);
}

// 0x52BED0
TigMessageListNode* tig_message_node_acquire()
{
    TigMessageListNode* node;

    tig_message_node_reserve();

    node = tig_message_empty_node_head;
    tig_message_empty_node_head = node->next;
    node->next = NULL;

    return node;
}

// 0x52BF10
void tig_message_node_reserve()
{
    int index;
    TigMessageListNode* node;

    if (tig_message_empty_node_head == NULL) {
        for (index = 0; index < 32; index++) {
            node = (TigMessageListNode*)MALLOC(sizeof(*node));
            node->next = tig_message_empty_node_head;
            tig_message_empty_node_head = node;
        }
    }
}

// 0x52BF60
void tig_message_node_release(TigMessageListNode* node)
{
    node->next = tig_message_empty_node_head;
    tig_message_empty_node_head = node;
}
