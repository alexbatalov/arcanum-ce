#include "tig/window.h"

#include <ctype.h>

#include "tig/art.h"
#include "tig/bmp.h"
#include "tig/button.h"
#include "tig/color.h"
#include "tig/core.h"
#include "tig/debug.h"
#include "tig/font.h"
#include "tig/mouse.h"
#include "tig/rect.h"
#include "tig/video.h"

#define TIG_WINDOW_MAX 50
#define TIG_WINDOW_BUTTON_MAX 200

// The following constants define layout and visual style of modal dialog
// created by `tig_window_modal_dialog`.
//
// NOTE: For engine-like thing like TIG it's too much hardcoded stuff. Either
// it should not be part of the engine at all, or at least art ids should be
// externalized into `TigWindowModalDialogInfo`.

#define MODAL_DIALOG_WIDTH 325
#define MODAL_DIALOG_HEIGHT 136

#define MODAL_DIALOG_TEXT_X 30
#define MODAL_DIALOG_TEXT_Y 14
#define MODAL_DIALOG_TEXT_WIDTH 265
#define MODAL_DIALOG_TEXT_HEIGHT 65

#define MODAL_DIALOG_CENTER_BUTTON_X 149
#define MODAL_DIALOG_CENTER_BUTTON_Y 102

#define MODAL_DIALOG_OK_BUTTON_X 73
#define MODAL_DIALOG_OK_BUTTON_Y 102

#define MODAL_DIALOG_CANCEL_BUTTON_X 225
#define MODAL_DIALOG_CANCEL_BUTTON_Y 102

#define MODAL_DIALOG_FONT_ART_NUM 229
#define MODAL_DIALOG_BACKGROUND_ART_NUM 822
#define MODAL_DIALOG_OK_BUTTON_ART_NUM 823
#define MODAL_DIALOG_CANCEL_BUTTON_ART_NUM 824

typedef enum TigWindowUsage {
    TIG_WINDOW_USAGE_FREE = 1 << 0,
} TigWindowUsafe;

typedef struct TigWindow {
    /* 0000 */ unsigned int usage;
    /* 0004 */ unsigned int flags;
    /* 0008 */ TigRect frame;
    /* 0018 */ TigRect bounds;
    /* 0028 */ unsigned int background_color;
    /* 002C */ unsigned int color_key;
    /* 0030 */ TigVideoBuffer* video_buffer;
    /* 0034 */ TigVideoBuffer* secondary_video_buffer;
    /* 0038 */ int num_buttons;
    /* 003C */ tig_button_handle_t buttons[TIG_WINDOW_BUTTON_MAX];
    /* 035C */ TigWindowMessageFilterFunc* message_filter;
} TigWindow;

static int tig_window_free_index();
static int tig_window_handle_to_index(tig_window_handle_t window_handle);
static tig_window_handle_t tig_window_index_to_handle(int window_index);
static void push_window_stack(tig_window_handle_t window_handle);
static bool pop_window_stack(tig_window_handle_t window_handle);
static bool tig_window_modal_dialog_message_filter(TigMessage* msg);
static void tig_window_modal_dialog_close();
static void tig_window_modal_dialog_refresh(TigRect* rect);
static bool tig_window_modal_dialog_create_buttons(int type, tig_window_handle_t window_handle);
static bool tig_window_modal_dialog_init();
static void tig_window_modal_dialog_exit();

// 0x5BED98
static tig_window_handle_t tig_window_modal_dialog_window_handle = TIG_WINDOW_HANDLE_INVALID;

// 0x5BEDA0
static TigRect tig_window_modal_dialog_bounds = { 0, 0, MODAL_DIALOG_WIDTH, MODAL_DIALOG_HEIGHT };

// 0x604758
static TigWindowModalDialogInfo tig_window_modal_dialog_info;

// 0x604778
static TigWindow windows[TIG_WINDOW_MAX];

// 0x60F038
static tig_button_handle_t tig_window_modal_dialog_button_handles[TIG_WINDOW_MODAL_DIALOG_CHOICE_COUNT];

// 0x60F040
static TigWindowModalDialogChoice tig_message_modal_dialog_choice;

// 0x60F044
static tig_window_handle_t tig_window_stack[TIG_WINDOW_MAX];

// 0x60F10C
static int tig_window_ctx_flags;

// 0x60F110
static TigRect tig_window_screen_rect;

// 0x60F120
static int dword_60F120;

// 0x60F124
static bool tig_window_initialized;

// 0x60F128
static int tig_window_num_windows;

// 0x60F12C
static TigRectListNode* tig_window_dirty_rects;

// 0x60F130
static tig_font_handle_t tig_window_modal_dialog_font;

// 0x51CAD0
int tig_window_init(TigInitInfo* init_info)
{
    int index;

    tig_window_num_windows = 0;
    tig_window_dirty_rects = NULL;

    tig_window_screen_rect.x = 0;
    tig_window_screen_rect.y = 0;
    tig_window_screen_rect.width = init_info->width;
    tig_window_screen_rect.height = init_info->height;

    for (index = 0; index < TIG_WINDOW_MAX; index++) {
        windows[index].usage = TIG_WINDOW_USAGE_FREE;
    }

    dword_60F120 = 1;
    tig_window_ctx_flags = init_info->flags;
    tig_window_initialized = true;

    return TIG_OK;
}

// 0x51CB30
void tig_window_exit()
{
    int window_index;
    tig_window_handle_t window_handle;
    TigRectListNode* curr;

    for (window_index = 0; window_index < TIG_WINDOW_MAX; window_index++) {
        if ((windows[window_index].usage & TIG_WINDOW_USAGE_FREE) == 0) {
            window_handle = tig_window_index_to_handle(window_index);
            tig_window_destroy(window_handle);
        }
    }

    while (tig_window_dirty_rects != NULL) {
        curr = tig_window_dirty_rects;
        tig_window_dirty_rects = curr->next;
        tig_rect_node_destroy(curr);
    }

    tig_window_initialized = false;
}

// 0x51CB90
int tig_window_create(TigWindowData* window_data, tig_window_handle_t* window_handle_ptr)
{
    int window_index;
    TigWindow* win;
    TigVideoBufferCreateInfo vb_create_info;
    int rc;

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    if (tig_window_num_windows >= TIG_WINDOW_MAX) {
        return TIG_ERR_OUT_OF_HANDLES;
    }

    window_index = tig_window_free_index();
    win = &(windows[window_index]);

    win->flags = window_data->flags;

    if ((window_data->flags & TIG_WINDOW_MODAL) != 0) {
        win->flags |= TIG_WINDOW_MESSAGE_FILTER;
    }

    win->message_filter = window_data->message_filter;
    win->frame = window_data->rect;
    win->bounds.x = 0;
    win->bounds.y = 0;
    win->bounds.width = window_data->rect.width;
    win->bounds.height = window_data->rect.height;
    win->background_color = window_data->background_color;
    win->color_key = window_data->color_key;
    win->num_buttons = 0;

    vb_create_info.flags = 0;

    if ((window_data->flags & TIG_WINDOW_TRANSPARENT) != 0) {
        vb_create_info.flags |= TIG_VIDEO_BUFFER_CREATE_COLOR_KEY;
    }

    if ((window_data->flags & TIG_WINDOW_VIDEO_MEMORY) != 0) {
        vb_create_info.flags |= TIG_VIDEO_BUFFER_CREATE_VIDEO_MEMORY;
    } else {
        vb_create_info.flags |= TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
    }

    if ((window_data->flags & TIG_WINDOW_RENDER_TARGET) != 0) {
        vb_create_info.flags |= TIG_VIDEO_BUFFER_CREATE_RENDER_TARGET;
    }

    vb_create_info.width = window_data->rect.width;
    vb_create_info.height = window_data->rect.height;
    vb_create_info.background_color = window_data->background_color;
    vb_create_info.color_key = window_data->color_key;

    rc = tig_video_buffer_create(&vb_create_info, &(win->video_buffer));
    if (rc != TIG_OK) {
        return rc;
    }

    if ((tig_window_ctx_flags & TIG_INITIALIZE_SCRATCH_BUFFER) != 0) {
        if ((window_data->flags & TIG_WINDOW_TRANSPARENT) != 0) {
            vb_create_info.flags &= ~TIG_VIDEO_BUFFER_CREATE_COLOR_KEY;

            rc = tig_video_buffer_create(&vb_create_info, &(win->secondary_video_buffer));
            if (rc != TIG_OK) {
                tig_video_buffer_destroy(win->video_buffer);
                return rc;
            }
        }
    }

    *window_handle_ptr = tig_window_index_to_handle(window_index);
    push_window_stack(*window_handle_ptr);

    if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(win->frame));
    }

    win->usage = 0;

    return TIG_OK;
}

// 0x51CD30
int tig_window_destroy(tig_window_handle_t window_handle)
{
    int window_index;
    TigWindow* win;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_destroy: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    rc = tig_window_button_destroy(window_handle);
    if (rc != TIG_OK) {
        return rc;
    }

    if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(win->frame));
    }

    tig_video_buffer_destroy(win->video_buffer);

    if ((tig_window_ctx_flags & TIG_INITIALIZE_SCRATCH_BUFFER) != 0
        && (win->flags & TIG_WINDOW_TRANSPARENT) != 0) {
        tig_video_buffer_destroy(win->secondary_video_buffer);
    }

    pop_window_stack(window_handle);

    win->usage = TIG_WINDOW_USAGE_FREE;

    return TIG_OK;
}

// 0x51CDE0
int tig_window_button_destroy(tig_window_handle_t window_handle)
{
    int window_index;
    TigWindow* win;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_button_destroy: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    while (win->num_buttons > 0) {
        rc = tig_button_destroy(win->buttons[0]);
        if (rc != TIG_OK) {
            return rc;
        }
    }

    return TIG_OK;
}

// 0x51CE50
int tig_window_message_filter_set(tig_window_handle_t window_handle, TigWindowMessageFilterFunc* func)
{
    int window_index;
    TigWindow* win;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_message_filter_set: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    if (func == NULL) {
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    if ((win->flags & TIG_WINDOW_MESSAGE_FILTER) == 0) {
        return TIG_ERR_INVALID_PARAM;
    }

    win->message_filter = func;

    return TIG_OK;
}

// 0x51CEB0
int tig_window_data(tig_window_handle_t window_handle, TigWindowData* window_data)
{
    int window_index;
    TigWindow* win;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_data: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    window_data->flags = win->flags;
    window_data->rect = win->frame;
    window_data->background_color = win->background_color;
    window_data->color_key = win->color_key;
    window_data->message_filter = win->message_filter;

    return TIG_OK;
}

// 0x51CF40
int tig_window_display()
{
    int rc;
    TigRectListNode* node;
    TigMouseState mouse_state;
    TigRect* mouse_frame;
    bool show_mouse = false;

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    if (tig_window_dirty_rects == NULL) {
        return TIG_OK;
    }

    rc = tig_mouse_get_state(&mouse_state);
    if (rc != TIG_OK) {
        return rc;
    }

    mouse_frame = (mouse_state.flags & TIG_MOUSE_STATE_HIDDEN) == 0 ? &(mouse_state.frame) : NULL;

    node = tig_window_dirty_rects;
    while (node != NULL) {
        tig_window_dirty_rects = node->next;
        if (!show_mouse
            && mouse_frame != NULL
            && mouse_frame->x < node->rect.x + node->rect.width
            && mouse_frame->y < node->rect.y + node->rect.height
            && node->rect.x < mouse_frame->x + mouse_frame->width
            && node->rect.y < mouse_frame->y + mouse_frame->height) {
            show_mouse = true;
        }

        sub_51D050(&(node->rect), mouse_frame, NULL, 0, 0, TIG_WINDOW_TOP);
        tig_rect_node_destroy(node);

        node = tig_window_dirty_rects;
    }

    if (show_mouse) {
        tig_mouse_display();
    }

    tig_video_display_fps();

    tig_video_flip();

    return TIG_OK;
}

// 0x51D050
void sub_51D050(TigRect* src_rect, TigRect* mouse_rect, TigVideoBuffer* dst_video_buffer, int dx, int dy, int top_window_index)
{
    TigVideoBufferBlitInfo vb_blit_info;
    TigRectListNode* head;
    TigRectListNode* node;
    TigRect dirty_rect;
    TigRect clips[4];
    TigRect blt_src_rect;
    TigRect blt_dst_rect;
    TigWindow* wins[20];
    TigRect rects[20];
    int rc;
    int v45;
    int v47;
    int num_clips;
    int index;
    int v38 = 0;

    rc = tig_rect_intersection(src_rect, &tig_window_screen_rect, &dirty_rect);
    if (rc != TIG_OK) {
        return;
    }

    if (dst_video_buffer != NULL) {
        v45 = src_rect->x - dx;
        v47 = src_rect->y - dy;
    } else {
        v45 = 0;
        v47 = 0;
    }

    if (mouse_rect != NULL) {
        num_clips = tig_rect_clip(&dirty_rect, mouse_rect, clips);
        if (num_clips <= 0) {
            return;
        }

        head = NULL;
        for (index = 0; index < num_clips; index++) {
            node = tig_rect_node_create();
            if (node == NULL) {
                break;
            }

            node->rect = clips[index];
            node->next = head;
            head = node;
        }
    } else {
        head = tig_rect_node_create();
        if (head == NULL) {
            return;
        }

        head->rect = dirty_rect;
        head->next = NULL;
    }

    if (top_window_index == TIG_WINDOW_TOP) {
        top_window_index = tig_window_num_windows - 1;
    }

    while (top_window_index >= 0 && head != NULL) {
        tig_window_handle_t window_handle = tig_window_stack[top_window_index];
        unsigned int window_index = tig_window_handle_to_index(window_handle);
        TigWindow* win = &(windows[window_index]);

        if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
            TigRectListNode* curr = head;
            TigRectListNode* prev = NULL;

            while (curr != NULL) {
                rc = tig_rect_intersection(&(curr->rect), &(win->frame), &dirty_rect);
                if (rc == TIG_OK) {
                    // TODO: Not sure how to represent it one to one.
                    bool cont;
                    TigVideoBuffer* src_video_buffer = win->video_buffer;
                    if ((win->flags & TIG_WINDOW_TRANSPARENT) == 0) {
                        cont = true;
                    } else if ((tig_window_ctx_flags & TIG_INITIALIZE_SCRATCH_BUFFER) != 0) {
                        sub_51D050(&dirty_rect,
                            mouse_rect,
                            win->secondary_video_buffer,
                            dirty_rect.x - win->frame.x,
                            dirty_rect.y - win->frame.y,
                            window_index - 1);
                        src_video_buffer = win->secondary_video_buffer;
                        cont = true;
                    } else if (v38 < 20) {
                        wins[v38] = win;
                        rects[v38] = dirty_rect;
                        v38++;
                        cont = false;
                    } else {
                        cont = true;
                    }

                    if (cont) {
                        blt_src_rect.x = dirty_rect.x - win->frame.x;
                        blt_src_rect.y = dirty_rect.y - win->frame.y;
                        blt_src_rect.width = dirty_rect.width;
                        blt_src_rect.height = dirty_rect.height;

                        if ((win->flags & TIG_WINDOW_TRANSPARENT) != 0
                            && (tig_window_ctx_flags & TIG_INITIALIZE_SCRATCH_BUFFER) != 0) {
                            vb_blit_info.flags = 0;
                            vb_blit_info.src_video_buffer = win->video_buffer;
                            vb_blit_info.src_rect = &blt_src_rect;
                            vb_blit_info.dst_video_buffer = win->secondary_video_buffer;
                            vb_blit_info.dst_rect = &blt_src_rect;
                            tig_video_buffer_blit(&vb_blit_info);
                        }

                        blt_dst_rect.x = dirty_rect.x - v45;
                        blt_dst_rect.y = dirty_rect.y - v47;
                        blt_dst_rect.width = blt_src_rect.width;
                        blt_dst_rect.height = blt_src_rect.height;

                        if (dst_video_buffer != NULL) {
                            vb_blit_info.flags = 0;
                            vb_blit_info.src_video_buffer = src_video_buffer;
                            vb_blit_info.src_rect = &blt_src_rect;
                            vb_blit_info.dst_video_buffer = dst_video_buffer;
                            vb_blit_info.dst_rect = &blt_dst_rect;
                            tig_video_buffer_blit(&vb_blit_info);
                        } else {
                            tig_video_blit(src_video_buffer, &blt_src_rect, &blt_dst_rect);
                        }

                        num_clips = tig_rect_clip(&(curr->rect), &(win->frame), clips);
                        for (index = 0; index < num_clips; index++) {
                            node = tig_rect_node_create();
                            if (node == NULL) {
                                break;
                            }

                            node->rect = clips[index];
                            node->next = curr->next;
                            curr->next = node;
                        }

                        if (curr == head) {
                            head = curr->next;
                            tig_rect_node_destroy(curr);
                            curr = head;
                        } else {
                            prev->next = curr->next;
                            tig_rect_node_destroy(curr);
                            curr = prev->next;
                        }

                        continue;
                    }
                }
                prev = curr;
                curr = curr->next;
            }
        }

        top_window_index--;
    }

    while (head != NULL) {
        node = head;
        head = head->next;
        tig_video_fill(&(node->rect), 0);
        tig_rect_node_destroy(node);
    }

    --v38;
    while (v38 >= 0) {
        blt_src_rect.x = rects[v38].x - wins[v38]->frame.x;
        blt_src_rect.y = rects[v38].y - wins[v38]->frame.y;
        blt_src_rect.width = rects[v38].width;
        blt_src_rect.height = rects[v38].height;

        blt_dst_rect.x = rects[v38].x - v45;
        blt_dst_rect.y = rects[v38].y - v47;
        blt_dst_rect.width = rects[v38].width;
        blt_dst_rect.height = rects[v38].height;

        if (dst_video_buffer != NULL) {
            vb_blit_info.flags = 0;
            vb_blit_info.src_video_buffer = wins[v38]->video_buffer;
            vb_blit_info.dst_video_buffer = dst_video_buffer;
            vb_blit_info.src_rect = &blt_src_rect;
            vb_blit_info.dst_rect = &blt_dst_rect;
            tig_video_buffer_blit(&vb_blit_info);
        } else {
            tig_video_blit(wins[v38]->video_buffer, &blt_src_rect, &blt_dst_rect);
        }

        v38--;
    }
}

// 0x51D570
int tig_window_fill(tig_window_handle_t window_handle, TigRect* rect, int color)
{
    int window_index;
    TigWindow* win;
    TigRect normalized;
    TigRect clamped_normalized_rect;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_fill: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    if (rect != NULL) {
        normalized = *rect;
    } else {
        normalized.x = 0;
        normalized.y = 0;
        normalized.width = win->frame.width;
        normalized.height = win->frame.height;
    }

    if (tig_rect_intersection(&normalized, &(win->bounds), &clamped_normalized_rect) != TIG_OK) {
        return TIG_OK;
    }

    rc = tig_video_buffer_fill(win->video_buffer, &clamped_normalized_rect, color);
    if (rc != TIG_OK) {
        return rc;
    }

    clamped_normalized_rect.x += win->frame.x;
    clamped_normalized_rect.y += win->frame.y;
    if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&clamped_normalized_rect);
        tig_button_refresh_rect(window_handle, &clamped_normalized_rect);
    }

    return TIG_OK;
}

// 0x51D6B0
int tig_window_line(tig_window_handle_t window_handle, TigLine* line, int color)
{
    int window_index;
    TigWindow* win;
    TigRect dirty_rect;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_line: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    if (tig_line_intersection(&(win->bounds), line) != TIG_OK) {
        return TIG_OK;
    }

    if (tig_line_bounding_box(line, &dirty_rect) != TIG_OK) {
        return TIG_ERR_NO_INTERSECTION;
    }

    rc = tig_video_buffer_line(win->video_buffer, line, &dirty_rect, color);

    dirty_rect.x += win->frame.x;
    dirty_rect.y += win->frame.y;

    if (rc == 0) {
        if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
            tig_window_invalidate_rect(&dirty_rect);
            tig_button_refresh_rect(window_handle, &dirty_rect);
        }
    }

    return rc;
}

// 0x51D7B0
int tig_window_box(tig_window_handle_t window_handle, TigRect* rect, int color)
{
    int rc;
    TigRect side_rect;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_box: ERROR: Attempt to reference Empty WinID!\n");
        return 12;
    }

    side_rect.x = rect->x;
    side_rect.y = rect->y;
    side_rect.width = rect->width;
    side_rect.height = 1;
    rc = tig_window_fill(window_handle, &side_rect, color);
    if (rc != TIG_OK) {
        return rc;
    }

    side_rect.x = rect->x;
    side_rect.y = rect->y;
    side_rect.width = 1;
    side_rect.height = rect->height;
    rc = tig_window_fill(window_handle, &side_rect, color);
    if (rc != TIG_OK) {
        return rc;
    }

    side_rect.x = rect->x + rect->width - 1;
    side_rect.y = rect->y;
    side_rect.width = 1;
    side_rect.height = rect->height;
    rc = tig_window_fill(window_handle, &side_rect, color);
    if (rc != TIG_OK) {
        return rc;
    }

    side_rect.x = rect->x;
    side_rect.y = rect->y + rect->height - 1;
    side_rect.width = rect->width;
    side_rect.height = 1;
    rc = tig_window_fill(window_handle, &side_rect, color);
    if (rc != TIG_OK) {
        return rc;
    }

    return TIG_OK;
}

// 0x51D8D0
int tig_window_blit(TigWindowBlitInfo* win_blit_info)
{
    TigVideoBufferBlitInfo vb_blit_info;
    TigRect dirty_rect;
    unsigned int src_window_index;
    unsigned int dst_window_index;

    switch (win_blit_info->type) {
    case TIG_WINDOW_BLIT_WINDOW_TO_WINDOW:
        src_window_index = tig_window_handle_to_index(win_blit_info->src_window_handle);
        vb_blit_info.src_video_buffer = windows[src_window_index].video_buffer;

        dst_window_index = tig_window_handle_to_index(win_blit_info->dst_window_handle);
        vb_blit_info.dst_video_buffer = windows[dst_window_index].video_buffer;

        dirty_rect = *win_blit_info->dst_rect;
        dirty_rect.x += windows[dst_window_index].frame.x;
        dirty_rect.y += windows[dst_window_index].frame.y;

        if ((windows[dst_window_index].flags & TIG_WINDOW_HIDDEN) == 0) {
            tig_window_invalidate_rect(&dirty_rect);
        }
        break;
    case TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW:
        vb_blit_info.src_video_buffer = win_blit_info->src_video_buffer;

        dst_window_index = tig_window_handle_to_index(win_blit_info->dst_window_handle);
        vb_blit_info.dst_video_buffer = windows[dst_window_index].video_buffer;

        dirty_rect = *win_blit_info->dst_rect;
        dirty_rect.x += windows[dst_window_index].frame.x;
        dirty_rect.y += windows[dst_window_index].frame.y;

        if ((windows[dst_window_index].flags & TIG_WINDOW_HIDDEN) == 0) {
            tig_window_invalidate_rect(&dirty_rect);
        }
        break;
    case TIG_WINDOW_BLT_WINDOW_TO_VIDEO_BUFFER:
        src_window_index = tig_window_handle_to_index(win_blit_info->src_window_handle);
        vb_blit_info.src_video_buffer = windows[src_window_index].video_buffer;
        vb_blit_info.dst_video_buffer = win_blit_info->dst_video_buffer;
        break;
    default:
        return TIG_ERR_INVALID_PARAM;
    }

    vb_blit_info.flags = win_blit_info->vb_blit_flags;
    vb_blit_info.src_rect = win_blit_info->src_rect;
    vb_blit_info.alpha[0] = win_blit_info->alpha[0];
    vb_blit_info.alpha[1] = win_blit_info->alpha[1];
    vb_blit_info.alpha[2] = win_blit_info->alpha[2];
    vb_blit_info.alpha[3] = win_blit_info->alpha[3];

    // TODO: Looks odd, investigate.
    // vb_blit_info.field_10 = win_blit_info->field_18;
    // vb_blit_info.field_14 = win_blit_info->field_1C;
    // vb_blit_info.field_18 = win_blit_info->field_20;
    // vb_blit_info.field_1C = win_blit_info->field_24;

    vb_blit_info.dst_rect = win_blit_info->dst_rect;
    return tig_video_buffer_blit(&vb_blit_info);
}

// 0x51DA80
int tig_window_blit_art(tig_window_handle_t window_handle, TigArtBlitInfo* blit_info)
{
    TigWindow* win;
    int window_index;
    int rc;
    TigRect rect;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_blit_art: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    blit_info->dst_video_buffer = win->video_buffer;

    rc = tig_art_blit(blit_info);
    if (rc != TIG_OK) {
        return rc;
    }

    rect = *(blit_info->dst_rect);
    rect.x += win->frame.x;
    rect.y += win->frame.y;

    if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&rect);
        tig_button_refresh_rect(window_handle, &rect);
    }

    return rc;
}

// 0x51DB40
int tig_window_scroll(tig_window_handle_t window_handle, int dx, int dy)
{
    int window_index;
    TigWindow* window;
    TigRect src_rect;
    TigRect dst_rect;
    TigVideoBufferBlitInfo vb_blit_info;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_scroll: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    window = &(windows[window_index]);

    src_rect = window->bounds;
    dst_rect = window->bounds;

    if (dx < 0) {
        dst_rect.width += dx;
        src_rect.x -= dx;
        src_rect.width += dx;
    } else {
        dst_rect.x += dx;
        dst_rect.width -= dx;
        src_rect.width -= dx;
    }

    if (dy < 0) {
        dst_rect.height += dy;
        src_rect.y -= dy;
        src_rect.height += dy;
    } else {
        dst_rect.y += dy;
        dst_rect.height -= dy;
        src_rect.height -= dy;
    }

    vb_blit_info.flags = 0;
    vb_blit_info.src_video_buffer = window->video_buffer;
    vb_blit_info.src_rect = &src_rect;
    vb_blit_info.dst_video_buffer = window->video_buffer;
    vb_blit_info.dst_rect = &dst_rect;

    rc = tig_video_buffer_blit(&vb_blit_info);
    if (rc != TIG_OK) {
        return rc;
    }

    if ((window->flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(window->frame));
    }

    return TIG_OK;
}

// 0x51DC90
int tig_window_scroll_rect(tig_window_handle_t window_handle, TigRect* rect, int dx, int dy)
{
    int window_index;
    TigWindow* window;
    TigRect src_rect;
    TigRect dst_rect;
    TigVideoBufferBlitInfo vb_blit_info;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_scroll_rect: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    window = &(windows[window_index]);

    src_rect = *rect;
    dst_rect = *rect;

    if (dx < 0) {
        dst_rect.width += dx;
        src_rect.x -= dx;
        src_rect.width += dx;
    } else {
        dst_rect.x += dx;
        dst_rect.width -= dx;
        src_rect.width -= dx;
    }

    if (dy < 0) {
        dst_rect.height += dy;
        src_rect.y -= dy;
        src_rect.height += dy;
    } else {
        dst_rect.y += dy;
        dst_rect.height -= dy;
        src_rect.height -= dy;
    }

    vb_blit_info.flags = 0;
    vb_blit_info.src_video_buffer = window->video_buffer;
    vb_blit_info.src_rect = &src_rect;
    vb_blit_info.dst_video_buffer = window->video_buffer;
    vb_blit_info.dst_rect = &dst_rect;

    rc = tig_video_buffer_blit(&vb_blit_info);
    if (rc != TIG_OK) {
        return rc;
    }

    if ((window->flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(window->frame));
    }

    return TIG_OK;
}

// 0x51DDC0
int tig_window_copy(tig_window_handle_t dst_window_handle, TigRect* dst_rect, tig_window_handle_t src_window_handle, TigRect* src_rect)
{
    int dst_window_index;
    int src_window_index;
    TigVideoBufferBlitInfo vb_blit_info;
    int rc;
    TigRect dirty_rect;

    if (dst_window_handle == TIG_WINDOW_HANDLE_INVALID || src_window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_copy: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    dst_window_index = tig_window_handle_to_index(dst_window_handle);
    src_window_index = tig_window_handle_to_index(src_window_handle);

    vb_blit_info.flags = 0;
    vb_blit_info.src_video_buffer = windows[src_window_index].video_buffer;
    vb_blit_info.src_rect = src_rect;
    vb_blit_info.dst_video_buffer = windows[dst_window_index].video_buffer;
    vb_blit_info.dst_rect = dst_rect;

    rc = tig_video_buffer_blit(&vb_blit_info);
    if (rc != TIG_OK) {
        return rc;
    }

    if ((windows[dst_window_index].flags & TIG_WINDOW_HIDDEN) == 0) {
        dirty_rect.x = dst_rect->x + windows[dst_window_index].frame.x;
        dirty_rect.y = dst_rect->y + windows[dst_window_index].frame.y;
        dirty_rect.width = dst_rect->width;
        dirty_rect.width = dst_rect->height;
        tig_window_invalidate_rect(&dirty_rect);
    }

    return TIG_OK;
}

// 0x51DEA0
int tig_window_copy_from_vbuffer(tig_window_handle_t dst_window_handle, TigRect* dst_rect, TigVideoBuffer* src_video_buffer, TigRect* src_rect)
{
    int dst_window_index;
    TigVideoBufferBlitInfo vb_blit_info;
    int rc;
    TigRect dirty_rect;

    if (dst_window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_copy_from_vbuffer: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    dst_window_index = tig_window_handle_to_index(dst_window_handle);

    vb_blit_info.flags = 0;
    vb_blit_info.src_video_buffer = src_video_buffer;
    vb_blit_info.src_rect = src_rect;
    vb_blit_info.dst_video_buffer = windows[dst_window_index].video_buffer;
    vb_blit_info.dst_rect = dst_rect;

    rc = tig_video_buffer_blit(&vb_blit_info);
    if (rc != TIG_OK) {
        return rc;
    }

    if ((windows[dst_window_index].flags & TIG_WINDOW_HIDDEN) == 0) {
        dirty_rect.x = dst_rect->x + windows[dst_window_index].frame.x;
        dirty_rect.y = dst_rect->y + windows[dst_window_index].frame.y;
        dirty_rect.width = dst_rect->width;
        dirty_rect.height = dst_rect->height;
        tig_window_invalidate_rect(&dirty_rect);
    }

    return TIG_OK;
}

// NOTE: The purpose of this alias is unclear. Not used.
//
// 0x51DF60
int sub_51DF60(tig_window_handle_t dst_window_handle, TigRect* dst_rect, TigVideoBuffer* src_video_buffer, TigRect* src_rect)
{
    return tig_window_copy_from_vbuffer(dst_window_handle, dst_rect, src_video_buffer, src_rect);
}

// 0x51DF80
int tig_window_copy_to_vbuffer(tig_window_handle_t src_window_handle, TigRect* src_rect, TigVideoBuffer* dst_video_buffer, TigRect* dst_rect)
{
    int src_window_index;
    TigVideoBufferBlitInfo vb_blit_info;

    if (src_window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_copy_to_vbuffer: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    src_window_index = tig_window_handle_to_index(src_window_handle);

    vb_blit_info.flags = 0;
    vb_blit_info.src_video_buffer = windows[src_window_index].video_buffer;
    vb_blit_info.src_rect = src_rect;
    vb_blit_info.dst_video_buffer = dst_video_buffer;
    vb_blit_info.dst_rect = dst_rect;

    return tig_video_buffer_blit(&vb_blit_info);
}

// 0x51DFF0
int tig_window_copy_from_bmp(tig_window_handle_t window_handle, TigRect* dst_rect, TigBmp* bmp, TigRect* src_rect)
{
    TigWindow* win;
    TigRect dirty_rect;
    int window_index;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_copy_from_bmp: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    rc = tig_bmp_copy_to_video_buffer(bmp, src_rect, win->video_buffer, dst_rect);
    if (rc == TIG_OK) {
        if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
            dirty_rect = *dst_rect;
            dirty_rect.x += win->frame.x;
            dirty_rect.y += win->frame.y;
            tig_window_invalidate_rect(&dirty_rect);
        }
    }

    return rc;
}

// 0x51E0A0
int tig_window_tint(tig_window_handle_t window_handle, TigRect* rect, int a3, int a4)
{
    int window_index;
    TigWindow* win;
    TigRect dirty_rect;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_tint: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    if (tig_rect_intersection(rect, &(win->bounds), &dirty_rect) != TIG_OK) {
        // No intersection, nothing to refresh.
        return TIG_OK;
    }

    rc = tig_video_buffer_tint(win->video_buffer, &dirty_rect, a3, a4);

    dirty_rect.x += win->frame.x;
    dirty_rect.y += win->frame.y;

    if (rc == TIG_OK) {
        if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
            tig_window_invalidate_rect(&dirty_rect);
            tig_button_refresh_rect(window_handle, &dirty_rect);
        }
    }

    return rc;
}

// 0x51E190
int tig_window_text_write(tig_window_handle_t window_handle, const char* str, TigRect* rect)
{
    int window_index;
    TigWindow* win;
    TigRect dirty_rect;
    int rc;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_text_write: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    if (rect->x < win->bounds.x
        || rect->y < win->bounds.y
        || rect->x + rect->width > win->bounds.x + win->bounds.width
        || rect->y + rect->height > win->bounds.y + win->bounds.height) {
        return TIG_ERR_INVALID_PARAM;
    }

    rc = tig_font_write(win->video_buffer, str, rect, &dirty_rect);

    dirty_rect.x += win->frame.x;
    dirty_rect.y += win->frame.y;

    if (rc == TIG_OK) {
        if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
            tig_window_invalidate_rect(&dirty_rect);
            tig_button_refresh_rect(window_handle, &dirty_rect);
        }
    }

    return rc;
}

// 0x51E2A0
int tig_window_free_index()
{
    int window_index;

    for (window_index = 0; window_index < TIG_WINDOW_MAX; window_index++) {
        if ((windows[window_index].usage & TIG_WINDOW_USAGE_FREE) != 0) {
            return window_index;
        }
    }

    return -1;
}

// 0x51E2C0
int tig_window_handle_to_index(tig_window_handle_t window_handle)
{
    return (int)window_handle;
}

// 0x51E2D0
tig_window_handle_t tig_window_index_to_handle(int window_index)
{
    return (tig_window_handle_t)window_index;
}

// 0x51E2E0
void push_window_stack(tig_window_handle_t window_handle)
{
    int window_index;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("push_window_stack: ERROR: Attempt to reference Empty WinID!\n");
        return;
    }

    window_index = tig_window_handle_to_index(window_handle);

    if ((windows[window_index].flags & (TIG_WINDOW_ALWAYS_ON_TOP | TIG_WINDOW_MODAL)) != 0) {
        tig_window_stack[tig_window_num_windows++] = window_handle;
    } else if ((windows[window_index].flags & (TIG_WINDOW_ALWAYS_ON_BOTTOM | TIG_WINDOW_MODAL)) != 0) {
        memmove(&(tig_window_stack[1]),
            &(tig_window_stack[0]),
            sizeof(tig_window_handle_t) * tig_window_num_windows);
        tig_window_stack[0] = window_handle;
        tig_window_num_windows++;
    } else {
        int prev_index;
        tig_window_handle_t prev_window_handle;
        int prev_window_index;

        for (prev_index = tig_window_num_windows - 1; prev_index >= 0; prev_index--) {
            prev_window_handle = tig_window_stack[prev_index];
            prev_window_index = tig_window_handle_to_index(prev_window_handle);

            if ((windows[prev_window_index].flags & (TIG_WINDOW_ALWAYS_ON_TOP | TIG_WINDOW_MODAL)) == 0) {
                break;
            }
        }

        // NOTE: Original code is slightly different, but does the same thing -
        // make a room a new window by moving windows in the stack.
        if (prev_index + 1 < tig_window_num_windows) {
            memmove(&(tig_window_stack[prev_index + 1]),
                &(tig_window_stack[prev_index]),
                sizeof(tig_window_handle_t) * (tig_window_num_windows - prev_index));
        }

        tig_window_stack[prev_index + 1] = window_handle;
        tig_window_num_windows++;
    }
}

// 0x51E3E0
bool pop_window_stack(tig_window_handle_t window_handle)
{
    int index;

    for (index = 0; index < tig_window_num_windows; index++) {
        if (tig_window_stack[index] == window_handle) {
            while (index + 1 < tig_window_num_windows) {
                tig_window_stack[index] = tig_window_stack[index + 1];
                index++;
            }

            tig_window_num_windows--;

            return true;
        }
    }

    return false;
}

// 0x51E430
void tig_window_invalidate_rect(TigRect* rect)
{
    TigRect dirty_rect;
    TigRectListNode* node;

    if (!tig_window_initialized) {
        return;
    }

    if (rect != NULL) {
        dirty_rect = *rect;
        if (tig_rect_intersection(&dirty_rect, &tig_window_screen_rect, &dirty_rect) != TIG_OK) {
            return;
        }
    } else {
        dirty_rect = tig_window_screen_rect;
    }

    if (tig_window_dirty_rects != NULL) {
        node = tig_rect_node_create();
        node->rect = dirty_rect;
        node->next = tig_window_dirty_rects;
        tig_window_dirty_rects = node;
    } else {
        tig_window_dirty_rects = tig_rect_node_create();
        if (tig_window_dirty_rects != NULL) {
            tig_window_dirty_rects->rect = dirty_rect;
            tig_window_dirty_rects->next = NULL;
        }
    }
}

// 0x51E530
int tig_window_button_add(tig_window_handle_t window_handle, tig_button_handle_t button_handle)
{
    int window_index;
    TigWindow* win;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_button_add: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    if (win->num_buttons == TIG_WINDOW_BUTTON_MAX) {
        return TIG_ERR_OUT_OF_HANDLES;
    }

    win->buttons[win->num_buttons++] = button_handle;

    return TIG_OK;
}

// 0x51E5A0
int tig_window_button_remove(tig_window_handle_t window_handle, tig_button_handle_t button_handle)
{
    int window_index;
    TigWindow* win;
    int button_index;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_button_remove: ERROR: Attempt to reference Empty WinID!\n");
        return TIG_ERR_INVALID_PARAM;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    for (button_index = 0; button_index < win->num_buttons; button_index++) {
        if (win->buttons[button_index] == button_handle) {
            while (button_index + 1 < win->num_buttons) {
                win->buttons[button_index] = win->buttons[button_index + 1];
                button_index++;
            }

            win->num_buttons--;

            return TIG_OK;
        }
    }

    return TIG_ERR_INVALID_PARAM;
}

// 0x51E640
int tig_window_button_list(tig_window_handle_t window_handle, tig_button_handle_t** buttons_ptr)
{
    int window_index;
    TigWindow* win;

    if (window_handle == TIG_WINDOW_HANDLE_INVALID) {
        tig_debug_printf("tig_window_button_list: ERROR: Attempt to reference Empty WinID!\n");
        *buttons_ptr = NULL;
        return 0;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    *buttons_ptr = win->buttons;

    return win->num_buttons;
}

// 0x51E690
int tig_window_get_at_position(int x, int y, tig_window_handle_t* window_handle_ptr)
{
    int index;
    tig_window_handle_t window_handle;
    int window_index;
    TigWindow* win;
    unsigned int color;

    for (index = tig_window_num_windows - 1; index >= 0; index--) {
        window_handle = tig_window_stack[index];
        window_index = tig_window_handle_to_index(window_handle);
        win = &(windows[window_index]);
        if ((win->flags & TIG_WINDOW_HIDDEN) == 0
            && x >= win->frame.x
            && y >= win->frame.y
            && x <= win->frame.x + win->frame.width
            && y <= win->frame.y + win->frame.height) {
            if ((win->flags & TIG_WINDOW_TRANSPARENT) != 0) {
                if (tig_video_buffer_get_pixel_color(win->video_buffer, x - win->frame.x, y - win->frame.y, &color) == TIG_OK
                    && color == win->color_key) {
                    continue;
                }
            }

            *window_handle_ptr = window_handle;

            return TIG_OK;
        }
    }

    return TIG_ERR_INVALID_PARAM;
}

// 0x51E790
bool tig_window_filter_message(TigMessage* msg)
{
    int index;
    tig_window_handle_t window_handle;
    int window_index;
    TigWindow* win;
    unsigned int flags[TIG_WINDOW_MAX];
    TigWindowMessageFilterFunc* filters[TIG_WINDOW_MAX];
    int cnt;

    if (!dword_60F120) {
        return false;
    }

    cnt = 0;
    for (index = tig_window_num_windows - 1; index >= 0; index--) {
        window_handle = tig_window_stack[index];
        window_index = tig_window_handle_to_index(window_handle);
        win = &(windows[window_index]);
        if ((win->flags & TIG_WINDOW_HIDDEN) == 0
            && (win->flags & TIG_WINDOW_MESSAGE_FILTER) != 0) {
            flags[cnt] = win->flags;
            filters[cnt] = win->message_filter;
            cnt++;
        }
    }

    for (index = 0; index < cnt; index++) {
        if ((filters[index](msg) && msg->type != TIG_MESSAGE_PING)
            || ((flags[index] & TIG_WINDOW_MODAL) != 0 && msg->type != TIG_MESSAGE_PING)) {
            return true;
        }
    }

    return false;
}

// 0x51E850
int sub_51E850(tig_window_handle_t window_handle)
{
    int window_index;

    if (!pop_window_stack(window_handle)) {
        return TIG_ERR_GENERIC;
    }

    push_window_stack(window_handle);

    window_index = tig_window_handle_to_index(window_handle);
    if ((windows[window_index].flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(windows[window_index].frame));
    }

    return TIG_OK;
}

// 0x51E8A0
int tig_window_move_on_top(tig_window_handle_t window_handle)
{
    int window_index;
    TigWindow* win;

    if (!pop_window_stack(window_handle)) {
        return TIG_ERR_GENERIC;
    }

    push_window_stack(window_handle);

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    if ((win->flags & TIG_WINDOW_HIDDEN) == 0) {
        tig_window_invalidate_rect(&(win->frame));
    }

    return TIG_OK;
}

// 0x51E8F0
int tig_window_show(tig_window_handle_t window_handle)
{
    int window_index;
    TigWindow* win;
    int index;

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);
    win->flags &= ~TIG_WINDOW_HIDDEN;
    tig_window_invalidate_rect(&(win->frame));

    for (index = 0; index < win->num_buttons; index++) {
        tig_button_show_force(win->buttons[index]);
    }

    return TIG_OK;
}

// 0x51E950
int tig_window_hide(tig_window_handle_t window_handle)
{
    int window_index;
    TigWindow* win;
    int index;

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);
    win->flags |= TIG_WINDOW_HIDDEN;
    tig_window_invalidate_rect(&(win->frame));

    for (index = 0; index < win->num_buttons; index++) {
        tig_button_hide_force(win->buttons[index]);
    }

    return TIG_OK;
}

// 0x51E9B0
bool tig_window_is_hidden(tig_window_handle_t window_handle)
{
    int window_index = tig_window_handle_to_index(window_handle);
    return (windows[window_index].flags & TIG_WINDOW_HIDDEN) != 0;
}

// 0x51E9E0
int sub_51E9E0()
{
    dword_60F120 = !dword_60F120;
    return TIG_OK;
}

// 0x51EA00
bool sub_51EA00()
{
    return dword_60F120;
}

// 0x51EA10
int tig_window_vbid_get(tig_window_handle_t window_handle, TigVideoBuffer** video_buffer_ptr)
{
    int window_index;
    TigWindow* win;

    if (video_buffer_ptr == NULL) {
        return TIG_ERR_GENERIC;
    }

    if (!tig_window_initialized) {
        return TIG_ERR_NOT_INITIALIZED;
    }

    window_index = tig_window_handle_to_index(window_handle);
    win = &(windows[window_index]);

    *video_buffer_ptr = win->video_buffer;

    return TIG_OK;
}

// 0x51EA60
int tig_window_modal_dialog(TigWindowModalDialogInfo* modal_info, TigWindowModalDialogChoice* choice_ptr)
{
    TigMessage msg;
    TigWindowData window_data;

    if (modal_info == NULL) {
        return TIG_ERR_GENERIC;
    }

    if (!tig_window_modal_dialog_init()) {
        return TIG_ERR_GENERIC;
    }

    tig_window_modal_dialog_info = *modal_info;

    window_data.flags = TIG_WINDOW_MODAL | TIG_WINDOW_ALWAYS_ON_TOP;
    window_data.rect.width = MODAL_DIALOG_WIDTH;
    window_data.rect.height = MODAL_DIALOG_HEIGHT;
    window_data.message_filter = tig_window_modal_dialog_message_filter;
    window_data.background_color = tig_color_make(0, 0, 0);
    window_data.rect.x = modal_info->x;
    window_data.rect.y = modal_info->y;

    if (tig_window_create(&window_data, &tig_window_modal_dialog_window_handle) != TIG_OK) {
        return TIG_ERR_GENERIC;
    }

    tig_window_modal_dialog_create_buttons(modal_info->type, tig_window_modal_dialog_window_handle);
    tig_window_modal_dialog_refresh(NULL);

    while (tig_window_modal_dialog_window_handle != TIG_WINDOW_HANDLE_INVALID) {
        tig_ping();

        if (tig_window_modal_dialog_window_handle == TIG_WINDOW_HANDLE_INVALID) {
            break;
        }

        while (tig_message_dequeue(&msg) == TIG_OK) {
            if (msg.type == TIG_MESSAGE_REDRAW) {
                if (modal_info->redraw != NULL) {
                    modal_info->redraw();
                }

                tig_window_invalidate_rect(NULL);
            }
        }

        if (modal_info->process != NULL && modal_info->process(&tig_message_modal_dialog_choice)) {
            tig_window_modal_dialog_close();
        }

        tig_window_display();
    }

    if (choice_ptr != NULL) {
        *choice_ptr = tig_message_modal_dialog_choice;
    }

    tig_window_modal_dialog_exit();

    return TIG_OK;
}

// 0x51EBF0
bool tig_window_modal_dialog_message_filter(TigMessage* msg)
{
    switch (msg->type) {
    case TIG_MESSAGE_KEYBOARD:
        if (msg->data.keyboard.pressed) {
            switch (tig_window_modal_dialog_info.type) {
            case TIG_WINDOW_MODAL_DIALOG_TYPE_OK:
                tig_message_modal_dialog_choice = TIG_WINDOW_MODAL_DIALOG_CHOICE_OK;
                tig_window_modal_dialog_close();
                break;
            case TIG_WINDOW_MODAL_DIALOG_TYPE_CANCEL:
                tig_message_modal_dialog_choice = TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL;
                tig_window_modal_dialog_close();
                break;
            case TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL:
                if (msg->data.keyboard.scancode == SDL_SCANCODE_RETURN
                    || msg->data.keyboard.scancode == SDL_SCANCODE_KP_ENTER
                    || SDL_toupper(msg->data.keyboard.key) == SDL_toupper(tig_window_modal_dialog_info.keys[0])) {
                    tig_message_modal_dialog_choice = TIG_WINDOW_MODAL_DIALOG_CHOICE_OK;
                    tig_window_modal_dialog_close();
                } else if (msg->data.keyboard.scancode == SDL_SCANCODE_ESCAPE
                    || SDL_toupper(msg->data.keyboard.key) == SDL_toupper(tig_window_modal_dialog_info.keys[1])) {
                    tig_message_modal_dialog_choice = TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL;
                    tig_window_modal_dialog_close();
                }
                break;
            }
        }
        break;
    case TIG_MESSAGE_BUTTON:
        if (msg->data.button.state == TIG_BUTTON_STATE_RELEASED) {
            if (msg->data.button.button_handle == tig_window_modal_dialog_button_handles[0]) {
                switch (tig_window_modal_dialog_info.type) {
                case TIG_WINDOW_MODAL_DIALOG_TYPE_OK:
                case TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL:
                    tig_message_modal_dialog_choice = TIG_WINDOW_MODAL_DIALOG_CHOICE_OK;
                    tig_window_modal_dialog_close();
                    break;
                }
            } else if (msg->data.button.button_handle == tig_window_modal_dialog_button_handles[1]) {
                switch (tig_window_modal_dialog_info.type) {
                case TIG_WINDOW_MODAL_DIALOG_TYPE_CANCEL:
                case TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL:
                    tig_message_modal_dialog_choice = TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL;
                    tig_window_modal_dialog_close();
                    break;
                }
            }
        }
        break;
    default:
        break;
    }

    return true;
}

// 0x51ED10
void tig_window_modal_dialog_close()
{
    if (tig_window_modal_dialog_window_handle != TIG_WINDOW_HANDLE_INVALID) {
        if (tig_window_destroy(tig_window_modal_dialog_window_handle) == TIG_OK) {
            tig_window_modal_dialog_window_handle = TIG_WINDOW_HANDLE_INVALID;
        }
    }
}

// 0x51ED40
void tig_window_modal_dialog_refresh(TigRect* rect)
{
    TigRect text_rect;
    TigArtBlitInfo blit_info;

    text_rect.x = MODAL_DIALOG_TEXT_X;
    text_rect.y = MODAL_DIALOG_TEXT_Y;
    text_rect.width = MODAL_DIALOG_TEXT_WIDTH;
    text_rect.height = MODAL_DIALOG_TEXT_HEIGHT;

    if (rect == NULL) {
        rect = &tig_window_modal_dialog_bounds;
    }

    if (rect->x < tig_window_modal_dialog_bounds.x + tig_window_modal_dialog_bounds.width
        && rect->y < tig_window_modal_dialog_bounds.y + tig_window_modal_dialog_bounds.height
        && tig_window_modal_dialog_bounds.x < rect->x + rect->width
        && tig_window_modal_dialog_bounds.y < rect->y + rect->height) {

        blit_info.flags = 0;
        blit_info.src_rect = rect;
        blit_info.dst_rect = rect;
        tig_art_interface_id_create(MODAL_DIALOG_BACKGROUND_ART_NUM, 0, 0, 0, &(blit_info.art_id));
        if (tig_window_blit_art(tig_window_modal_dialog_window_handle, &blit_info) == TIG_OK) {
            if (tig_window_modal_dialog_info.text != NULL) {
                tig_font_push(tig_window_modal_dialog_font);
                tig_window_text_write(tig_window_modal_dialog_window_handle,
                    tig_window_modal_dialog_info.text,
                    &text_rect);
                tig_font_pop();
            }
        }
    }
}

// 0x51EE30
bool tig_window_modal_dialog_create_buttons(int type, tig_window_handle_t window_handle)
{
    TigButtonData ok_button_data;
    TigButtonData cancel_button_data;
    TigArtFrameData ok_art_frame_data;
    TigArtFrameData cancel_art_frame_data;

    ok_button_data.mouse_up_snd_id = -1;
    ok_button_data.mouse_down_snd_id = -1;
    ok_button_data.mouse_enter_snd_id = -1;
    ok_button_data.mouse_exit_snd_id = -1;
    ok_button_data.flags = TIG_BUTTON_MOMENTARY;
    ok_button_data.window_handle = window_handle;

    cancel_button_data.mouse_up_snd_id = -1;
    cancel_button_data.mouse_down_snd_id = -1;
    cancel_button_data.mouse_enter_snd_id = -1;
    cancel_button_data.mouse_exit_snd_id = -1;
    cancel_button_data.flags = TIG_BUTTON_MOMENTARY;
    cancel_button_data.window_handle = window_handle;

    switch (type) {
    case TIG_WINDOW_MODAL_DIALOG_TYPE_OK:
        tig_art_interface_id_create(MODAL_DIALOG_OK_BUTTON_ART_NUM, 0, 0, 0, &(ok_button_data.art_id));
        tig_art_frame_data(ok_button_data.art_id, &ok_art_frame_data);
        ok_button_data.x = MODAL_DIALOG_CENTER_BUTTON_X;
        ok_button_data.y = MODAL_DIALOG_CENTER_BUTTON_Y;
        ok_button_data.width = ok_art_frame_data.width;
        ok_button_data.height = ok_art_frame_data.height;
        tig_button_create(&ok_button_data,
            &(tig_window_modal_dialog_button_handles[TIG_WINDOW_MODAL_DIALOG_CHOICE_OK]));
        break;
    case TIG_WINDOW_MODAL_DIALOG_TYPE_CANCEL:
        tig_art_interface_id_create(MODAL_DIALOG_CANCEL_BUTTON_ART_NUM, 0, 0, 0, &(cancel_button_data.art_id));
        tig_art_frame_data(cancel_button_data.art_id, &cancel_art_frame_data);
        cancel_button_data.x = MODAL_DIALOG_CENTER_BUTTON_X;
        cancel_button_data.y = MODAL_DIALOG_CENTER_BUTTON_Y;
        cancel_button_data.width = cancel_art_frame_data.width;
        cancel_button_data.height = cancel_art_frame_data.height;
        tig_button_create(&cancel_button_data,
            &(tig_window_modal_dialog_button_handles[TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL]));
        break;
    case TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL:
        // NOTE: Other cases do not check errors.
        if (tig_art_interface_id_create(MODAL_DIALOG_OK_BUTTON_ART_NUM, 0, 0, 0, &(ok_button_data.art_id)) == TIG_OK
            && tig_art_frame_data(ok_button_data.art_id, &ok_art_frame_data) == TIG_OK
            && tig_art_interface_id_create(MODAL_DIALOG_CANCEL_BUTTON_ART_NUM, 0, 0, 0, &(cancel_button_data.art_id)) == TIG_OK
            && tig_art_frame_data(cancel_button_data.art_id, &cancel_art_frame_data) == TIG_OK) {
            ok_button_data.x = MODAL_DIALOG_OK_BUTTON_X;
            ok_button_data.y = MODAL_DIALOG_OK_BUTTON_Y;
            ok_button_data.width = ok_art_frame_data.width;
            ok_button_data.height = ok_art_frame_data.height;
            if (tig_button_create(&ok_button_data, &(tig_window_modal_dialog_button_handles[TIG_WINDOW_MODAL_DIALOG_CHOICE_OK])) != TIG_OK) {
                return false;
            }

            cancel_button_data.x = MODAL_DIALOG_CANCEL_BUTTON_X;
            cancel_button_data.y = MODAL_DIALOG_CANCEL_BUTTON_Y;
            cancel_button_data.width = cancel_art_frame_data.width;
            cancel_button_data.height = cancel_art_frame_data.height;
            if (tig_button_create(&cancel_button_data, &(tig_window_modal_dialog_button_handles[TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL])) != TIG_OK) {
                return false;
            }
        }
        break;
    }

    return true;
}

// 0x51F050
bool tig_window_modal_dialog_init()
{
    TigFont font_data;

    font_data.flags = 0;
    tig_art_interface_id_create(MODAL_DIALOG_FONT_ART_NUM, 0, 0, 0, &(font_data.art_id));
    font_data.str = 0;
    font_data.color = tig_color_make(255, 255, 255);
    tig_font_create(&font_data, &tig_window_modal_dialog_font);

    tig_window_modal_dialog_button_handles[TIG_WINDOW_MODAL_DIALOG_CHOICE_OK] = TIG_BUTTON_HANDLE_INVALID;
    tig_window_modal_dialog_button_handles[TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL] = TIG_BUTTON_HANDLE_INVALID;

    return true;
}

// 0x51F0F0
void tig_window_modal_dialog_exit()
{
    int index;

    if (tig_window_modal_dialog_font != TIG_FONT_HANDLE_INVALID) {
        tig_font_destroy(tig_window_modal_dialog_font);
        tig_window_modal_dialog_font = TIG_FONT_HANDLE_INVALID;
    }

    for (index = 0; index < TIG_WINDOW_MODAL_DIALOG_CHOICE_COUNT; index++) {
        if (tig_window_modal_dialog_button_handles[index] != TIG_BUTTON_HANDLE_INVALID) {
            tig_button_destroy(tig_window_modal_dialog_button_handles[index]);
        }
    }
}
