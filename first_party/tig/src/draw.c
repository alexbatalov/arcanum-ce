// DRAW
// ---
//
// The functions of this module does not draw anything. It only provide
// accessors to line properties which are used by `tig_video_buffer_line`
// exclusively. I'm not sure why this couple of accessors was put in a separate
// module with it's own init/exit functions. As for the name (`draw`) it is
// based on location in binary which appears between file cache and drop-down
// (named `cache` and `menu` respectively).
//
// The names of symbols in this module are rather crappy.

#include "tig/draw.h"

// 0x5C2810
static TigDrawLineModeInfo tig_draw_line_mode_info = { TIG_DRAW_LINE_MODE_NORMAL, 1 };

// 0x63650C
static TigDrawLineStyleInfo tig_draw_line_style_info;

// 0x538EE0
int tig_draw_init(TigInitInfo* init_info)
{
    (void)init_info;

    return TIG_OK;
}

// 0x538EF0
void tig_draw_exit()
{
}

// 0x538F00
void tig_draw_set_line_mode(TigDrawLineModeInfo* mode_info)
{
    tig_draw_line_mode_info = *mode_info;
}

// 0x538F20
void tig_draw_get_line_mode(TigDrawLineModeInfo* mode_info)
{
    *mode_info = tig_draw_line_mode_info;
}

// 0x538F40
void tig_draw_set_line_style(TigDrawLineStyleInfo* style_info)
{
    tig_draw_line_style_info = *style_info;
}

// 0x538F50
void tig_draw_get_line_style(TigDrawLineStyleInfo* style_info)
{
    *style_info = tig_draw_line_style_info;
}
