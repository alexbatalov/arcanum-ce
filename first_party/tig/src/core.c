#include "tig/core.h"

#include "tig/art.h"
#include "tig/button.h"
#include "tig/color.h"
#include "tig/debug.h"
#include "tig/draw.h"
#include "tig/file.h"
#include "tig/file_cache.h"
#include "tig/font.h"
#include "tig/kb.h"
#include "tig/memory.h"
#include "tig/message.h"
#include "tig/mouse.h"
#include "tig/movie.h"
#include "tig/palette.h"
#include "tig/rect.h"
#include "tig/sound.h"
#include "tig/str_parse.h"
#include "tig/timer.h"
#include "tig/video.h"
#include "tig/window.h"

typedef int(TigInitFunc)(TigInitInfo* init_info);
typedef void(TigExitFunc)();

typedef struct TigModule {
    const char* name;
    TigInitFunc* init_func;
    TigExitFunc* exit_func;
} TigModule;

// NOTE: Original code is slightly different. It has two separate arrays of
// init and exit funcs, and does not have human-readable name. This approach is
// borrowed from ToEE.
static TigModule modules[] = {
    { "memory", tig_memory_init, tig_memory_exit },
    { "debug", tig_debug_init, tig_debug_exit },
    { "rect", tig_rect_init, tig_rect_exit },
    { "file", tig_file_init, tig_file_exit },
    { "color", tig_color_init, tig_color_exit },
    { "video", tig_video_init, tig_video_exit },
    { "palette", tig_palette_init, tig_palette_exit },
    { "window", tig_window_init, tig_window_exit },
    { "timer", tig_timer_init, tig_timer_exit },
    { "kb", tig_kb_init, tig_kb_exit },
    { "art", tig_art_init, tig_art_exit },
    { "mouse", tig_mouse_init, tig_mouse_exit },
    { "message", tig_message_init, tig_message_exit },
    { "button", tig_button_init, tig_button_exit },
    { "font", tig_font_init, tig_font_exit },
    { "draw", tig_draw_init, tig_draw_exit },
    { "str_parse", tig_str_parse_init, tig_str_parse_exit },
    { "file_cache", tig_file_cache_init, tig_file_cache_exit },
    { "sound", tig_sound_init, tig_sound_exit },
    { "movie", tig_movie_init, tig_movie_exit },
};

// 0x60F23C
static bool tig_initialized;

// 0x60F240
static bool in_tig_exit;

// 0x60F244
static bool tig_active;

// 0x739F34
tig_timestamp_t tig_ping_timestamp;

// 0x51F130
int tig_init(TigInitInfo* init_info)
{
    size_t index;
    int rc;

    if (tig_initialized) {
        return TIG_ERR_ALREADY_INITIALIZED;
    }

    if (!SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
        tig_debug_printf("Error initializing SDL: %s\n", SDL_GetError());
        return TIG_ERR_GENERIC;
    }

    atexit(SDL_Quit);

    for (index = 0; index < SDL_arraysize(modules); ++index) {
        // NOTE: For unknown reason skipping sound is done this way instead
        // of checking for appropriate flag in `tig_sound_init`.
        if ((init_info->flags & TIG_INITIALIZE_NO_SOUND) != 0
            && modules[index].init_func == tig_sound_init) {
            continue;
        }

        rc = modules[index].init_func(init_info);
        if (rc != TIG_OK) {
            tig_debug_printf("Error initializing TIG: %s\n", modules[index].name);

            // Unwind initialized modules.
            while (index > 0) {
                modules[--index].exit_func();
            }

            return rc;
        }
    }

    atexit(tig_exit);

    tig_initialized = true;

    return TIG_OK;
}

// 0x51F1D0
void tig_exit(void)
{
    size_t index;

    if (!tig_initialized) {
        return;
    }

    if (!in_tig_exit) {
        in_tig_exit = true;

        index = SDL_arraysize(modules);
        while (index > 0) {
            modules[--index].exit_func();
        }

        tig_initialized = false;

        in_tig_exit = false;
    }
}

// 0x51F220
void tig_ping()
{
    tig_timer_now(&tig_ping_timestamp);

    tig_mouse_ping();
    tig_message_ping();
    tig_sound_ping();
    tig_art_ping();
}

// NOTE: Purpose is unclear, both this function and `tig_ping` are public.
//
// 0x51F250
void sub_51F250()
{
    tig_ping();
}

// 0x51F300
void tig_set_active(bool is_active)
{
    tig_active = is_active;
    tig_mouse_set_active(is_active);
    tig_sound_set_active(is_active);
}

// 0x51F320
bool tig_get_active()
{
    return tig_active;
}
