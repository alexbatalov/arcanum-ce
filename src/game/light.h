#ifndef ARCANUM_GAME_LIGHT_H_
#define ARCANUM_GAME_LIGHT_H_

#include "game/context.h"
#include "game/obj.h"
#include "game/timeevent.h"

#define SHADOW_HANDLE_MAX 5

// clang-format off
#define LF_OFF          0x00000001
#define LF_DARK         0x00000002
#define LF_ANIMATING    0x00000004
#define LF_INDOOR       0x00000008
#define LF_OUTDOOR      0x00000010
#define LF_00000020     0x00000020
#define LF_MODIFIED     0x00000080
#define LF_08000000     0x08000000
#define LF_OVERLAY_0    0x10000000
#define LF_OVERLAY_1    0x20000000
#define LF_OVERLAY_2    0x40000000
#define LF_OVERLAY_3    0x80000000
// clang-format on

typedef struct Light {
    /* 0000 */ int64_t obj;
    /* 0008 */ int64_t loc;
    /* 0010 */ int offset_x;
    /* 0014 */ int offset_y;
    /* 0018 */ unsigned int flags;
    /* 001C */ tig_art_id_t art_id;
    /* 0020 */ uint8_t r;
    /* 0021 */ uint8_t b;
    /* 0022 */ uint8_t g;
    /* 0024 */ tig_color_t tint_color;
    /* 0028 */ TigPalette palette;
} Light;

typedef struct Shadow {
    /* 0000 */ tig_art_id_t art_id;
    /* 0004 */ TigPalette palette;
    /* 0008 */ tig_color_t color;
    /* 000C */ struct Shadow* next;
} Shadow;

bool light_init(GameInitInfo* init_info);
void light_exit(void);
void light_resize(GameResizeInfo* resize_info);
void light_update_view(ViewOptions* view_options);
void sub_4D81F0(void);
void light_buffers_lock(void);
void light_buffers_unlock(void);
void light_draw(GameDrawInfo* draw_info);
void light_build_color(uint8_t red, uint8_t green, uint8_t blue, unsigned int* color);
void light_get_color_components(unsigned int color, uint8_t* red, uint8_t* green, uint8_t* blue);
tig_color_t light_get_outdoor_color(void);
tig_color_t light_get_indoor_color(void);
void light_set_colors(tig_color_t indoor_color, tig_color_t outdoor_color);
void light_start_animating(Light* light);
void light_stop_animating(Light* light);
bool light_timeevent_process(TimeEvent* timeevent);
tig_color_t sub_4D9240(int64_t loc, int offset_x, int offset_y);
bool light_is_modified(Light* light);
void light_clear_modified(Light* light);
bool light_read_dif(TigFile* stream, Light** light_ptr);
bool light_write_dif(TigFile* stream, Light* light);
void sub_4D9570(Light* light);
void sub_4D9590(int64_t obj, bool a2);
void sub_4D9990(int64_t obj);
void sub_4D9A90(int64_t obj);
bool shadow_apply(int64_t obj);
void shadow_clear(int64_t obj);
bool sub_4DA360(int x, int y, tig_color_t color, tig_color_t* a4);
void sub_4DC210(int64_t obj, int* colors, int* cnt_ptr);
uint8_t sub_4DCE10(int64_t obj);
void light_set_flags(int64_t obj, unsigned int flags);
void light_unset_flags(int64_t obj, unsigned int flags);
void sub_4DD020(int64_t obj, int64_t loc, int offset_x, int offset_y);
void sub_4DD0A0(int64_t obj, int offset_x, int offset_y);
bool sub_4DD110(Light* light);
void light_get_rect(Light* light, TigRect* rect);
void light_set_flags_internal(Light* light, unsigned int flags);
void light_unset_flags_internal(Light* light, unsigned int flags);
unsigned int light_get_flags(Light* light);
void light_inc_frame(Light* light);
void light_dec_frame(Light* light);
void light_cycle_rotation(Light* light);
void light_set_location(Light* light, int64_t loc);
int64_t light_get_location(Light* light);
void light_set_offset(Light* light, int offset_x, int offset_y);
bool light_read(TigFile* stream, Light** light_ptr);
bool light_write(TigFile* stream, Light* light);
void light_invalidate_rect(TigRect* rect, bool invalidate_objects);

#endif /* ARCANUM_GAME_LIGHT_H_ */
