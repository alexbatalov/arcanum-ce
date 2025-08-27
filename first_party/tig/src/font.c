#include "tig/font.h"

#include "tig/art.h"
#include "tig/color.h"
#include "tig/memory.h"
#include "tig/rect.h"
#include "tig/video.h"

#define FONT_STACK_SIZE 20

static int sub_535850(TigVideoBuffer* video_buffer, const char* str, int length, TigArtBlitInfo* blit_info, bool shadow);
static int sub_535C40(tig_art_id_t font_art_id, const char* str, int max_width, int* width_ptr);
static bool tig_font_glyph_data(tig_art_id_t font_art_id, int ch, int* width_ptr, int* height_ptr, int* dx_ptr, int* dy_ptr);

// 0x630CFC
static int tig_font_stack_index;

// 0x630D00
static TigFont* tig_font_stack[FONT_STACK_SIZE];

// 0x630D50
static tig_color_t tig_font_shadow_color;

// 0x630D54
static tig_font_handle_t tig_font_default_font;

// 0x5351D0
int tig_font_init(TigInitInfo* init_info)
{
    TigFont font_data;

    (void)init_info;

    tig_font_stack_index = -1;
    tig_font_shadow_color = tig_color_make(10, 10, 10);

    font_data.flags = TIG_FONT_SHADOW;
    tig_art_misc_id_create(TIG_ART_SYSTEM_FONT, 0, &(font_data.art_id));
    font_data.scale = 1.5f;
    font_data.str = NULL;
    font_data.color = tig_color_make(255, 255, 255);
    font_data.underline_color = font_data.color;
    font_data.strike_through_color = font_data.color;
    tig_font_create(&font_data, &tig_font_default_font);
    tig_font_push(tig_font_default_font);

    return TIG_OK;
}

// 0x5352C0
void tig_font_exit()
{
    tig_font_pop();
    tig_font_destroy(tig_font_default_font);
}

// 0x5352E0
void tig_font_create(TigFont* font_data, tig_font_handle_t* font_handle_ptr)
{
    TigFont* copy;
    TigArtAnimData art_anim_data;

    copy = (TigFont*)MALLOC(sizeof(TigFont));
    memcpy(copy, font_data, sizeof(TigFont));

    if (tig_art_anim_data(font_data->art_id, &art_anim_data) != TIG_OK) {
        // Fatal error - missing font art.
        exit(EXIT_FAILURE);
    }

    *font_handle_ptr = (tig_font_handle_t)copy;
}

// 0x535330
void tig_font_destroy(tig_font_handle_t font_handle)
{
    FREE((TigFont*)font_handle);
}

// 0x535340
int tig_font_push(tig_font_handle_t font_handle)
{
    TigFont* font = (TigFont*)font_handle;

    tig_font_stack_index++;
    if (tig_font_stack_index < FONT_STACK_SIZE) {
        tig_font_stack[tig_font_stack_index] = font;
    } else {
        // Font #0 is the default and therefore should be skipped.
        tig_font_stack_index = 1;
        tig_font_stack[tig_font_stack_index] = font;
    }

    return tig_font_stack_index;
}

// 0x535380
void tig_font_pop()
{
    if (tig_font_stack_index > 0) {
        tig_font_stack_index--;
    }
}

// 0x535390
void tig_font_measure(TigFont* font)
{
    const char* str;
    int width;
    int height;
    int glyph_width;
    int glyph_height;
    int glyph_dx;
    int glyph_dy;
    int pos;
    int lines;
    int line_length;
    int line_width;

    str = font->str;
    width = font->width;
    height = font->height;
    memcpy(font, tig_font_stack[tig_font_stack_index], sizeof(TigFont));
    font->str = str;
    font->width = width;
    font->height = height;

    width = 0;
    height = 0;

    if (str != NULL) {
        glyph_height = 0;
        lines = 1;
        if (font->width != 0) {
            if (!tig_font_glyph_data(tig_font_stack[tig_font_stack_index]->art_id, str[0], &glyph_width, &glyph_height, &glyph_dx, &glyph_dy)) {
                font->height = 0;
                return;
            }

            pos = 0;
            for (;;) {
                line_length = sub_535C40(tig_font_stack[tig_font_stack_index]->art_id, &(str[pos]), font->width, &line_width);
                if (line_length == -1) {
                    font->height = 0;
                    return;
                }

                pos += line_length;
                if (str[pos] == '\0') {
                    break;
                }

                lines++;
                pos++;
            }
        } else {
            pos = 0;
            while (str[pos] != '\0') {
                if (!tig_font_glyph_data(tig_font_stack[tig_font_stack_index]->art_id, str[pos], &glyph_width, &glyph_height, &glyph_dx, &glyph_dy)) {
                    font->height = 0;
                    return;
                }

                if (str[pos + 1] != '\0') {
                    width += glyph_dx;
                } else {
                    width += glyph_width;
                }

                pos++;
            }
        }

        height = glyph_height * lines;

        if ((font->flags & TIG_FONT_SHADOW) != 0) {
            ++height;
            ++width;
        }

        if ((font->flags & TIG_FONT_SCALE) != 0) {
            width = (int)((float)width * font->scale);
            height = (int)((float)height * font->scale);
        }
    }

    if (font->width == 0 || str == NULL) {
        font->width = width;
    }

    font->height = height;
}

// 0x535570
int tig_font_write(TigVideoBuffer* video_buffer, const char* str, const TigRect* rect, TigRect* dirty_rect)
{
    TigArtBlitInfo blit_info;
    TigRect dst_rect;
    TigRect src_rect;
    int glyph_dx;
    int glyph_dy;
    int glyph_height;
    int glyph_width;
    int max_width;
    int max_y;
    int min_dx;
    int num_passes;
    int pass;
    bool shadow;
    int dst_rect_x;
    const char* remainder;

    // NOTE: Initialize to keep compiler happy.
    max_y = 0;

    if (str == NULL) {
        return TIG_OK;
    }

    if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_SHADOW) != 0) {
        num_passes = 2;
        shadow = true;
    } else {
        num_passes = 1;
        shadow = false;
    }

    blit_info.dst_rect = &dst_rect;
    blit_info.src_rect = &src_rect;
    blit_info.dst_video_buffer = video_buffer;

    src_rect.x = 0;
    src_rect.y = 0;

    min_dx = rect->width;
    max_width = 0;

    for (pass = 0; pass < num_passes; pass++) {
        blit_info.flags = 0;

        if (!shadow) {
            blit_info.flags |= TIG_ART_BLT_BLEND_COLOR_CONST;

            if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_BLEND_ADD) != 0) {
                blit_info.flags |= TIG_ART_BLT_BLEND_ADD;
            }

            if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_NO_ALPHA_BLEND) == 0) {
                blit_info.flags |= TIG_ART_BLT_BLEND_ALPHA_SRC;
            }

            blit_info.color = tig_font_stack[tig_font_stack_index]->color;
        }

        dst_rect.x = rect->x;
        dst_rect.y = rect->y;

        if (shadow) {
            blit_info.flags |= TIG_ART_BLT_BLEND_COLOR_CONST;
            ++dst_rect.x;
            ++dst_rect.y;
            blit_info.color = tig_font_shadow_color;
        }

        remainder = str;

        if (!tig_font_glyph_data(tig_font_stack[tig_font_stack_index]->art_id, ' ', &glyph_width, &glyph_height, &glyph_dx, &glyph_dy)) {
            return TIG_ERR_GENERIC;
        }

        max_y = rect->y;
        dst_rect_x = dst_rect.x;

        if (glyph_height <= rect->height) {
            int line_length;
            int line_width;
            int dx;
            int rc;

            while (1) {
                line_length = sub_535C40(tig_font_stack[tig_font_stack_index]->art_id, remainder, rect->width, &line_width);
                if (line_length == -1) {
                    break;
                }

                if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_CENTERED) != 0) {
                    dx = (rect->width - line_width) / 2;
                } else {
                    dx = 0;
                }

                if (dx < min_dx) {
                    min_dx = dx;
                }

                if (line_width > max_width) {
                    max_width = line_width;
                }

                dst_rect.x += dx;

                if (dx != 0 && !shadow) {
                    dst_rect.x = dst_rect_x + dx;
                }

                rc = sub_535850(video_buffer, remainder, line_length + 1, &blit_info, shadow);
                if (rc != TIG_OK) {
                    return rc;
                }

                dst_rect.y += glyph_height;
                dst_rect.x = dst_rect_x;

                max_y += glyph_height;

                if (remainder[line_length] == '\0') {
                    break;
                }

                remainder += line_length + 1;
                if (rect->y + rect->height - max_y < glyph_height) {
                    break;
                }
            }
        }

        shadow = false;
    }

    if (dirty_rect != NULL) {
        dirty_rect->x = rect->x + min_dx;
        dirty_rect->y = rect->y;
        dirty_rect->width = max_width;
        dirty_rect->height = max_y - rect->y + 1;

        if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_SHADOW) != 0) {
            ++dirty_rect->width;
            ++dirty_rect->height;
        }
    }

    return TIG_OK;
}

// 0x535850
int sub_535850(TigVideoBuffer* video_buffer, const char* str, int length, TigArtBlitInfo* blit_info, bool shadow)
{
    tig_art_id_t glyph_art_id;
    int blt_dst_rect_x;
    int blt_dst_rect_y;
    int glyph_dx;
    int glyph_dy;
    int glyph_height;
    int glyph_width;
    int pos;
    int rc;

    // NOTE: Initialize to make compiler happy.
    glyph_dy = 0;
    rc = TIG_OK;

    blt_dst_rect_x = blit_info->dst_rect->x;
    blt_dst_rect_y = blit_info->dst_rect->y;

    for (pos = 0; pos < length; pos++) {
        if (str[pos] == '\0') {
            break;
        }

        if (str[pos] != '\n') {
            glyph_art_id = tig_art_id_frame_set(tig_font_stack[tig_font_stack_index]->art_id,
                str[pos] - 31);

            if (!tig_font_glyph_data(glyph_art_id, str[pos], &glyph_width, &glyph_height, &glyph_dx, &glyph_dy)) {
                return TIG_ERR_GENERIC;
            }

            if (str[pos] != '\t') {
                blit_info->art_id = glyph_art_id;
                blit_info->src_rect->width = glyph_width;
                blit_info->src_rect->height = glyph_height;
                blit_info->dst_rect->width = glyph_width;
                blit_info->dst_rect->height = glyph_height;

                if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_SCALE) != 0) {
                    blit_info->dst_rect->width = (int)((float)blit_info->dst_rect->width * tig_font_stack[tig_font_stack_index]->scale);
                    blit_info->dst_rect->height = (int)((float)blit_info->dst_rect->height * tig_font_stack[tig_font_stack_index]->scale);
                }

                rc = tig_art_blit(blit_info);
                if (rc != TIG_OK) {
                    return rc;
                }
            }

            if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_SCALE) != 0) {
                blit_info->dst_rect->x += (int)((float)glyph_dx * tig_font_stack[tig_font_stack_index]->scale);
            } else {
                blit_info->dst_rect->x += glyph_dx;
            }
        }
    }

    if (*str != '\0'
        && (tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_UNDERLINE) != 0) {
        int y = blit_info->dst_rect->y;

        blit_info->dst_rect->width = blit_info->dst_rect->x - blt_dst_rect_x;
        blit_info->dst_rect->height = 1;
        blit_info->dst_rect->x = blt_dst_rect_x;
        blit_info->dst_rect->y = blt_dst_rect_y;

        if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_SCALE) != 0) {
            blit_info->dst_rect->y += (int)((float)glyph_dy * tig_font_stack[tig_font_stack_index]->scale);
        } else {
            blit_info->dst_rect->y += glyph_dy;
        }

        if (shadow) {
            ++blit_info->dst_rect->x;
            ++blit_info->dst_rect->y;
            --blit_info->dst_rect->width;
            tig_video_buffer_fill(video_buffer,
                blit_info->dst_rect,
                tig_color_make(10, 10, 10));
            --blit_info->dst_rect->x;
            --blit_info->dst_rect->y;
            ++blit_info->dst_rect->width;
        }

        rc = tig_video_buffer_fill(video_buffer,
            blit_info->dst_rect,
            tig_font_stack[tig_font_stack_index]->underline_color);

        blit_info->dst_rect->y = y;
    }

    if (*str != '\0'
        && (tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_STRIKE_THROUGH) != 0) {
        int y = blit_info->dst_rect->y;

        blit_info->dst_rect->width = blit_info->dst_rect->x - blt_dst_rect_x;
        blit_info->dst_rect->height = 1;
        blit_info->dst_rect->x = blt_dst_rect_x;
        blit_info->dst_rect->y = blt_dst_rect_y;

        if ((tig_font_stack[tig_font_stack_index]->flags & TIG_FONT_SCALE) != 0) {
            blit_info->dst_rect->y += (int)((float)glyph_dy * tig_font_stack[tig_font_stack_index]->scale / 2.0);
        } else {
            blit_info->dst_rect->y += glyph_dy / 2;
        }

        if (shadow) {
            ++blit_info->dst_rect->x;
            ++blit_info->dst_rect->y;
            --blit_info->dst_rect->width;
            rc = tig_video_buffer_fill(video_buffer,
                blit_info->dst_rect,
                tig_color_make(10, 10, 10));
            --blit_info->dst_rect->x;
            --blit_info->dst_rect->y;
            ++blit_info->dst_rect->width;
        }

        tig_video_buffer_fill(video_buffer,
            blit_info->dst_rect,
            tig_font_stack[tig_font_stack_index]->strike_through_color);

        blit_info->dst_rect->y = y;
    }

    return rc;
}

// 0x535C40
int sub_535C40(tig_art_id_t font_art_id, const char* str, int max_width, int* width_ptr)
{
    int glyph_width;
    int glyph_height;
    int glyph_dx;
    int glyph_dy;
    int pos;
    int prev;
    int width;
    int total_width;

    total_width = 0;
    prev = -1;
    pos = 0;
    while (str[pos] != '\0') {
        if (str[pos] == '\n') {
            // Found NL, that's what we were looking for.
            prev = pos;
            *width_ptr = total_width;
            break;
        }

        if (!tig_font_glyph_data(font_art_id, str[pos], &glyph_width, &glyph_height, &glyph_dx, &glyph_dy)) {
            return -1;
        }

        if (str[pos] == ' ') {
            // Found SPACE, save it as a candidate to be a line break.
            prev = pos;
            *width_ptr = total_width;
        }

        width = str[pos + 1] != '\0' ? glyph_dx : glyph_width;

        if (total_width + width > max_width) {
            break;
        }

        total_width += glyph_dx;

        if (str[pos] == '-') {
            // Found HYPHEN, save it a candidate to be a line break.
            prev = pos;
            *width_ptr = total_width;
        }

        pos++;
    }

    if (str[pos] != '\0') {
        if (prev != pos) {
            pos = prev;
        }
    } else {
        *width_ptr = total_width;
    }

    return pos;
}

// 0x535D10
bool tig_font_glyph_data(tig_art_id_t font_art_id, int ch, int* width_ptr, int* height_ptr, int* dx_ptr, int* dy_ptr)
{
    TigArtFrameData glyph_frame_data;
    tig_art_id_t glyph_art_id;

    glyph_art_id = tig_art_id_frame_set(font_art_id, (unsigned char)ch - 31);
    if (tig_art_frame_data(glyph_art_id, &glyph_frame_data) != TIG_OK) {
        return false;
    }

    *width_ptr = glyph_frame_data.width;
    *height_ptr = glyph_frame_data.height;
    *dx_ptr = glyph_frame_data.hot_x;
    *dy_ptr = glyph_frame_data.hot_y;

    if ((unsigned char)ch == '\t') {
        *dx_ptr += 20;
    }

    return true;
}
