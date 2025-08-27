#ifndef TIG_WINDOW_H_
#define TIG_WINDOW_H_

#include "tig/rect.h"
#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TIG_WINDOW_HANDLE_INVALID ((tig_window_handle_t)(-1))

#define TIG_WINDOW_TOP (-2)

typedef unsigned int TigWindowFlags;

#define TIG_WINDOW_TRANSPARENT 0x0001
#define TIG_WINDOW_MESSAGE_FILTER 0x0002
#define TIG_WINDOW_MODAL 0x0004
#define TIG_WINDOW_ALWAYS_ON_TOP 0x0008
#define TIG_WINDOW_VIDEO_MEMORY 0x0010
#define TIG_WINDOW_HIDDEN 0x0020
#define TIG_WINDOW_RENDER_TARGET 0x0040
#define TIG_WINDOW_ALWAYS_ON_BOTTOM 0x0080

typedef bool(TigWindowMessageFilterFunc)(TigMessage* msg);

typedef struct TigWindowData {
    /* 0000 */ TigWindowFlags flags;
    /* 0004 */ TigRect rect;
    /* 0014 */ unsigned int background_color;
    /* 0018 */ unsigned int color_key;
    /* 001C */ TigWindowMessageFilterFunc* message_filter;
} TigWindowData;

typedef enum TigWindowBltType {
    // Blits `src_window_handle` to `dst_window_handle`.
    TIG_WINDOW_BLIT_WINDOW_TO_WINDOW = 1,

    // Blits `src_video_buffer` to `dst_window_handle`.
    TIG_WINDOW_BLIT_VIDEO_BUFFER_TO_WINDOW = 2,

    // Blits `src_window_handle` to `dst_window_buffer`.
    TIG_WINDOW_BLT_WINDOW_TO_VIDEO_BUFFER = 3,
} TigWindowBltType;

typedef struct TigWindowBlitInfo {
    /* 0000 */ int type;
    /* 0004 */ unsigned int vb_blit_flags;
    /* 0008 */ tig_window_handle_t src_window_handle;
    /* 000C */ TigVideoBuffer* src_video_buffer;
    /* 0010 */ TigRect* src_rect;
    /* 0014 */ uint8_t alpha[4];
    /* 0018 */ int field_18;
    /* 001C */ tig_window_handle_t dst_window_handle;
    /* 0020 */ TigVideoBuffer* dst_video_buffer;
    /* 0024 */ TigRect* dst_rect;
} TigWindowBlitInfo;

typedef enum TigWindowModalDialogType {
    // The modal dialog contains a single OK-like button (green checkmark).
    TIG_WINDOW_MODAL_DIALOG_TYPE_OK,

    // The modal dialog contains a single CANCEL-like button (red cross).
    TIG_WINDOW_MODAL_DIALOG_TYPE_CANCEL,

    // The modal dialog contains two buttons (OK and CANCEL).
    TIG_WINDOW_MODAL_DIALOG_TYPE_OK_CANCEL,
} TigWindowModalDialogType;

typedef enum TigWindowModalDialogChoice {
    TIG_WINDOW_MODAL_DIALOG_CHOICE_OK,
    TIG_WINDOW_MODAL_DIALOG_CHOICE_CANCEL,
    TIG_WINDOW_MODAL_DIALOG_CHOICE_COUNT,
} TigWindowModalDialogChoice;

typedef bool(TigWindowDialogProcess)(TigWindowModalDialogChoice* choice_ptr);
typedef void(TigWindowDialogRedraw)();

typedef struct TigWindowModalDialogInfo {
    /* 0000 */ int type;
    /* 0004 */ int x;
    /* 0008 */ int y;
    /* 000C */ const char* text;
    /* 0010 */ TigWindowDialogProcess* process;
    /* 0014 */ unsigned char keys[2];
    /* 0018 */ TigWindowDialogRedraw* redraw;
} TigWindowModalDialogInfo;

int tig_window_init(TigInitInfo* init_info);
void tig_window_exit();
int tig_window_create(TigWindowData* window_data, tig_window_handle_t* window_handle_ptr);
int tig_window_destroy(tig_window_handle_t window_handle);
int tig_window_button_destroy(tig_window_handle_t window_handle);
int tig_window_message_filter_set(tig_window_handle_t window_handle, TigWindowMessageFilterFunc* func);
int tig_window_data(tig_window_handle_t window_handle, TigWindowData* window_data);
int tig_window_display();
void sub_51D050(TigRect* src_rect, TigRect* dst_rect, TigVideoBuffer* dst_video_buffer, int dx, int dy, int top_window_index);
int tig_window_fill(tig_window_handle_t window_handle, TigRect* rect, int color);
int tig_window_line(tig_window_handle_t window_handle, TigLine* line, int color);
int tig_window_box(tig_window_handle_t window_handle, TigRect* rect, int color);
int tig_window_blit(TigWindowBlitInfo* win_blit_info);
int tig_window_blit_art(tig_window_handle_t window_handle, TigArtBlitInfo* blit_info);
int tig_window_scroll(tig_window_handle_t window_handle, int dx, int dy);
int tig_window_scroll_rect(tig_window_handle_t window_handle, TigRect* rect, int dx, int dy);
int tig_window_copy(tig_window_handle_t dst_window_handle, TigRect* dst_rect, tig_window_handle_t src_window_handle, TigRect* src_rect);
int tig_window_copy_from_vbuffer(tig_window_handle_t dst_window_handle, TigRect* dst_rect, TigVideoBuffer* src_video_buffer, TigRect* src_rect);
int tig_window_copy_to_vbuffer(tig_window_handle_t src_window_handle, TigRect* src_rect, TigVideoBuffer* dst_video_buffer, TigRect* dst_rect);
int tig_window_copy_from_bmp(tig_window_handle_t window_handle, TigRect* dst_rect, TigBmp* bmp, TigRect* src_rect);
int tig_window_tint(tig_window_handle_t window_handle, TigRect* rect, int a3, int a4);
int tig_window_text_write(tig_window_handle_t window_handle, const char* text, TigRect* rect);
void tig_window_invalidate_rect(TigRect* rect);
int tig_window_button_add(tig_window_handle_t window_handle, tig_button_handle_t button_handle);
int tig_window_button_remove(tig_window_handle_t window_handle, tig_button_handle_t button_handle);
int tig_window_button_list(tig_window_handle_t window_handle, tig_button_handle_t** buttons);
int tig_window_get_at_position(int x, int y, tig_window_handle_t* window_handle_ptr);
bool tig_window_filter_message(TigMessage* msg);
int sub_51E850(tig_window_handle_t window_handle);
int tig_window_move_on_top(tig_window_handle_t window_handle);
int tig_window_show(tig_window_handle_t window_handle);
int tig_window_hide(tig_window_handle_t window_handle);
bool tig_window_is_hidden(tig_window_handle_t window_handle);
int sub_51E9E0();
bool sub_51EA00();
int tig_window_vbid_get(tig_window_handle_t window_handle, TigVideoBuffer** video_buffer_ptr);
int tig_window_modal_dialog(TigWindowModalDialogInfo* modal_info, TigWindowModalDialogChoice* choice_ptr);

#ifdef __cplusplus
}
#endif

#endif /* TIG_WINDOW_H_ */
