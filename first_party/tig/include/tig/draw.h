#ifndef TIG_DRAW_H_
#define TIG_DRAW_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum TigDrawLineMode {
    TIG_DRAW_LINE_MODE_NORMAL = 0,

    // NOTE: Do not use. This mode is not implemented correctly. Due to apparent
    // bug the original game (and this implementation) only respects this mode
    // in 16-bpp. This value is never used in the game.
    TIG_DRAW_LINE_MODE_THICK = 1,
} TigDrawLineMode;

typedef struct TigDrawLineModeInfo {
    TigDrawLineMode mode;

    // NOTE: Do not use. See `TIG_DRAW_LINE_MODE_THICK` for bug in the original
    // code (as well as this implementation).
    int thickness;
} TigDrawLineModeInfo;

typedef enum TigDrawLineStyle {
    TIG_LINE_STYLE_SOLID = 0,
    TIG_LINE_STYLE_DOTTED = 1,

    // NOTE: The pattern is 3-2 (hardcoded in `tig_video_buffer_line`).
    TIG_LINE_STYLE_DASHED = 2,
} TigDrawLineStyle;

typedef struct TigDrawLineStyleInfo {
    TigDrawLineStyle style;
} TigDrawLineStyleInfo;

int tig_draw_init(TigInitInfo* init_info);
void tig_draw_exit();
void tig_draw_set_line_mode(TigDrawLineModeInfo* line_info);
void tig_draw_get_line_mode(TigDrawLineModeInfo* line_info);
void tig_draw_set_line_style(TigDrawLineStyleInfo* style_info);
void tig_draw_get_line_style(TigDrawLineStyleInfo* style_info);

#ifdef __cplusplus
}
#endif

#endif /* TIG_DRAW_H_ */
