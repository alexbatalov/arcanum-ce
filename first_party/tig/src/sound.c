#include "tig/sound.h"

#include <stdio.h>

#include <mss_compat.h>

#include "tig/core.h"
#include "tig/debug.h"
#include "tig/file.h"
#include "tig/file_cache.h"
#include "tig/timer.h"

#define FIRST_VOICE_HANDLE 0
#define FIRST_MUSIC_HANDLE 2
#define FIRST_EFFECT_HANDLE 6
#define SOUND_HANDLE_MAX 60

typedef unsigned int TigSoundFlags;

#define TIG_SOUND_STREAMED 0x01u
#define TIG_SOUND_MEMORY 0x02u
#define TIG_SOUND_FADE_OUT 0x04u
#define TIG_SOUND_WAIT 0x08u
#define TIG_SOUND_FADE_IN 0x10u
#define TIG_SOUND_DESTROY 0x20u
#define TIG_SOUND_EFFECT 0x80u
#define TIG_SOUND_MUSIC 0x100u
#define TIG_SOUND_VOICE 0x200u

typedef struct TigSound {
    /* 0000 */ unsigned char active; // boolean
    /* 0004 */ TigSoundFlags flags;
    /* 0008 */ int fade_duration;
    /* 000C */ int fade_step;
    /* 0010 */ int loops;
    /* 0014 */ HSTREAM audio_stream;
    /* 0018 */ HAUDIO audio_handle;
    /* 001C */ tig_sound_handle_t next_sound_handle;
    /* 0020 */ char path[TIG_MAX_PATH];
    /* 0124 */ int id;
    /* 0128 */ int volume;
    /* 012C */ int extra_volume;
    /* 0130 */ TigFileCacheEntry* file_cache_entry;
    /* 0134 */ bool positional;
    /* 0138 */ int64_t positional_x;
    /* 0140 */ int64_t positional_y;
    /* 0148 */ TigSoundPositionalSize positional_size;
} TigSound;

static void tig_sound_update();
static void tig_sound_stop_from_destroy(tig_sound_handle_t sound_handle, int fade_duration);
static int tig_sound_acquire_handle(TigSoundType type);
static void tig_sound_reset_sound(TigSound* sound);
static int tig_sound_play_streamed(tig_sound_handle_t sound_handle, const char* name, int loops, int fade_duration, tig_sound_handle_t prev_sound_handle);

// Convenience.
static inline bool sound_handle_is_valid(tig_sound_handle_t sound_handle)
{
    return sound_handle >= 0 && sound_handle < SOUND_HANDLE_MAX;
}

// 0x5C246C
static TigSoundFlags tig_sound_type_flags[TIG_SOUND_TYPE_COUNT] = {
    TIG_SOUND_EFFECT,
    TIG_SOUND_MUSIC,
    TIG_SOUND_VOICE,
};

// 0x62B2C0
static TigSoundFilePathResolver* tig_sound_file_path_resolver;

// 0x62B328
static TigSound tig_sounds[SOUND_HANDLE_MAX];

// 0x6301E8
static tig_sound_handle_t tig_sound_next_effect_handle;

// 0x6301EC
static bool tig_sound_initialized;

// 0x6301F0
static TigFileCache* tig_sound_cache;

// 0x6301F4
static int tig_sound_effects_volume;

// 0x532D40
int tig_sound_init(TigInitInfo* init_info)
{
    // COMPAT: Load `mss32.dll`.
    mss_compat_init();

    tig_sound_initialized = false;
    tig_sound_next_effect_handle = FIRST_EFFECT_HANDLE;

    if (AIL_quick_startup(1, 0, 22050, 16, 2)) {
        tig_sound_initialized = true;
    }

    tig_sound_set_file_path_resolver(init_info->sound_file_path_resolver);

    // Create a file cache for 20 files, approx. 1 MB total.
    tig_sound_cache = tig_file_cache_create(20, 1000000);

    return TIG_OK;
}

// 0x532DB0
void tig_sound_exit()
{
    if (tig_sound_initialized) {
        tig_sound_initialized = false;
        tig_sound_stop_all(0);
        tig_file_cache_destroy(tig_sound_cache);
        AIL_quick_shutdown();
    }

    // COMPAT: Unload `mss32.dll`.
    mss_compat_exit();
}

// 0x532DE0
void tig_sound_ping()
{
    // 0x739E84
    static tig_timestamp_t tig_sound_ping_timestamp;

    if (!tig_sound_initialized) {
        return;
    }

    if (tig_ping_timestamp < tig_sound_ping_timestamp - 1000) {
        tig_sound_ping_timestamp = tig_ping_timestamp;
    }

    if (tig_ping_timestamp > tig_sound_ping_timestamp + 1000) {
        tig_sound_ping_timestamp = tig_ping_timestamp + 100;
        tig_sound_update();
    } else if (tig_ping_timestamp >= tig_sound_ping_timestamp) {
        tig_sound_ping_timestamp += 100;
        tig_sound_update();
    }
}

// 0x532E30
void tig_sound_update()
{
    int index;
    TigSound* snd;
    int new_volume;

    if (!tig_sound_initialized) {
        return;
    }

    for (index = 0; index < SOUND_HANDLE_MAX; index++) {
        snd = &(tig_sounds[index]);
        if (snd->active != 0 && (snd->flags & TIG_SOUND_WAIT) == 0) {
            if ((snd->flags & TIG_SOUND_FADE_OUT) != 0) {
                snd->fade_step++;
                if (snd->fade_step <= snd->fade_duration) {
                    new_volume = snd->volume * (snd->fade_duration - snd->fade_step) / snd->fade_duration;
                } else {
                    snd->flags &= ~TIG_SOUND_FADE_OUT;
                    if (snd->next_sound_handle >= 0) {
                        TigSound* next_snd = &(tig_sounds[snd->next_sound_handle]);
                        next_snd->flags &= ~TIG_SOUND_WAIT;
                        next_snd->flags |= TIG_SOUND_FADE_IN;
                    }
                    new_volume = 0;
                }

                if ((snd->flags & TIG_SOUND_STREAMED) != 0) {
                    AIL_set_stream_volume(snd->audio_stream, new_volume);
                    snd->flags |= TIG_SOUND_DESTROY;
                } else if ((snd->flags & TIG_SOUND_MEMORY) != 0) {
                    AIL_quick_set_volume(snd->audio_handle, (new_volume * snd->volume / 128), snd->extra_volume);
                    snd->flags |= TIG_SOUND_DESTROY;
                } else {
                    snd->flags |= TIG_SOUND_DESTROY;
                }
            } else if ((snd->flags & TIG_SOUND_FADE_IN) != 0) {
                if (snd->fade_step == 0) {
                    AIL_start_stream(snd->audio_stream);
                }

                snd->fade_step++;

                new_volume = snd->volume;
                if (snd->fade_step <= snd->fade_duration) {
                    new_volume = snd->fade_step * snd->volume / snd->fade_duration;
                } else {
                    snd->flags &= ~TIG_SOUND_FADE_IN;
                }

                if ((snd->flags & TIG_SOUND_STREAMED) != 0) {
                    AIL_set_stream_volume(snd->audio_stream, new_volume);
                } else if ((snd->flags & TIG_SOUND_MEMORY) != 0) {
                    AIL_quick_set_volume(snd->audio_handle, new_volume, 64);
                }
            } else {
                if ((snd->flags & TIG_SOUND_MEMORY) != 0) {
                    if (AIL_quick_status(snd->audio_handle) == QSTAT_DONE) {
                        snd->flags |= TIG_SOUND_DESTROY;
                    }
                }

                if ((snd->flags & TIG_SOUND_DESTROY) != 0) {
                    tig_sound_destroy(index);
                }
            }
        }
    }
}

// 0x533000
void tig_sound_set_file_path_resolver(TigSoundFilePathResolver* func)
{
    tig_sound_file_path_resolver = func;
}

// 0x533010
bool tig_sound_is_initialized()
{
    return tig_sound_initialized;
}

// 0x533020
void tig_sound_stop(tig_sound_handle_t sound_handle, int fade_duration)
{
    TigSound* snd;

    if (!tig_sound_initialized) {
        return;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    snd = &(tig_sounds[sound_handle]);
    snd->flags &= ~(TIG_SOUND_WAIT | TIG_SOUND_FADE_IN);
    if ((snd->flags & TIG_SOUND_FADE_OUT) == 0) {
        snd->flags |= TIG_SOUND_FADE_OUT;
        snd->fade_duration = abs(fade_duration);
        snd->fade_step = 0;
    }
}

// NOTE: Purpose is unclear, used only from `tig_sound_destroy`.
//
// 0x533080
void tig_sound_stop_from_destroy(tig_sound_handle_t sound_handle, int fade_duration)
{
    tig_sound_stop(sound_handle, fade_duration);
}

// 0x5330A0
void tig_sound_stop_all(int fade_duration)
{
    int index;

    if (!tig_sound_initialized) {
        return;
    }

    for (index = 0; index < SOUND_HANDLE_MAX; index++) {
        if (tig_sounds[index].active != 0) {
            tig_sound_stop(index, fade_duration);
        }
    }

    tig_sound_next_effect_handle = FIRST_EFFECT_HANDLE;

    tig_sound_update();
}

// 0x5330F0
int tig_sound_create(tig_sound_handle_t* sound_handle, TigSoundType type)
{
    if (!tig_sound_initialized) {
        *sound_handle = TIG_SOUND_HANDLE_INVALID;
        return TIG_OK;
    }

    *sound_handle = tig_sound_acquire_handle(type);
    if (*sound_handle != TIG_SOUND_HANDLE_INVALID) {
        tig_sounds[*sound_handle].flags |= tig_sound_type_flags[type];
        tig_sounds[*sound_handle].loops = 1;
        tig_sounds[*sound_handle].volume = 127;
        tig_sounds[*sound_handle].extra_volume = 64;
    }

    return TIG_OK;
}

// 0x5331A0
int tig_sound_acquire_handle(TigSoundType type)
{
    int index;
    TigSound* snd;

    switch (type) {
    case TIG_SOUND_TYPE_EFFECT:
        for (index = 0; index < SOUND_HANDLE_MAX; index++) {
            snd = &(tig_sounds[tig_sound_next_effect_handle]);
            if (snd->active == 0) {
                tig_sound_reset_sound(snd);
                snd->active = 1;
                return tig_sound_next_effect_handle;
            }

            if (++tig_sound_next_effect_handle >= SOUND_HANDLE_MAX) {
                tig_sound_next_effect_handle = FIRST_EFFECT_HANDLE;
            }
        }
        break;
    case TIG_SOUND_TYPE_MUSIC:
        for (index = FIRST_MUSIC_HANDLE; index < FIRST_EFFECT_HANDLE; index++) {
            snd = &(tig_sounds[index]);
            if (snd->active == 0) {
                tig_sound_reset_sound(snd);
                snd->active = 1;
                return index;
            }
        }
        break;
    case TIG_SOUND_TYPE_VOICE:
        for (index = FIRST_VOICE_HANDLE; index < FIRST_MUSIC_HANDLE; index++) {
            snd = &(tig_sounds[index]);
            if (snd->active == 0) {
                tig_sound_reset_sound(snd);
                snd->active = 1;
                return index;
            }
        }
        break;
    default:
        // Should be unreachable.
        abort();
    }

    tig_debug_printf("No sound handle available for sound type %d! Current sounds are:\n");
    for (index = 0; index < SOUND_HANDLE_MAX; index++) {
        // FIXME: This approach does not respect lower and upper bounds for the
        // requested sound type. Let's say we're out of music handles, it will
        // dump voice and sound handles, which (1) cannot store music handles,
        // and (2) can be empty or obsolete value (no check for `active`).
        tig_debug_printf("%s\n", tig_sounds[index].path);
    }

    return TIG_SOUND_HANDLE_INVALID;
}

// 0x5332F0
void tig_sound_reset_sound(TigSound* sound)
{
    memset(sound, 0, sizeof(*sound));
    sound->next_sound_handle = TIG_SOUND_HANDLE_INVALID;
}

// 0x533310
void tig_sound_destroy(tig_sound_handle_t sound_handle)
{
    TigSound* snd;

    if (!tig_sound_initialized) {
        return;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    snd = &(tig_sounds[sound_handle]);
    if (snd->active != 0) {
        tig_sound_stop_from_destroy(sound_handle, 0);

        if ((snd->flags & TIG_SOUND_STREAMED) != 0) {
            AIL_close_stream(snd->audio_stream);
            snd->active = 0;
        } else if ((snd->flags & TIG_SOUND_MEMORY) != 0) {
            AIL_quick_unload(snd->audio_handle);
            tig_file_cache_release(tig_sound_cache, snd->file_cache_entry);
            snd->active = 0;
        } else {
            snd->active = 0;
        }
    }
}

// 0x5333A0
int tig_sound_play(tig_sound_handle_t sound_handle, const char* path, int id)
{
    TigSound* snd;

    if (!tig_sound_initialized) {
        return TIG_OK;
    }

    // FIXME: No `SOUND_HANDLE_MAX` guard.
    if (!(sound_handle >= 0)) {
        return TIG_OK;
    }

    snd = &(tig_sounds[sound_handle]);
    strcpy(snd->path, path);
    snd->file_cache_entry = tig_file_cache_acquire(tig_sound_cache, path);

    if (snd->file_cache_entry->data != NULL) {
        snd->audio_handle = AIL_quick_load_mem(snd->file_cache_entry->data, snd->file_cache_entry->size);
        AIL_quick_set_volume(snd->audio_handle, snd->volume, snd->extra_volume);
        AIL_quick_play(snd->audio_handle, snd->loops);
        snd->flags |= TIG_SOUND_MEMORY;
        snd->id = id;
    } else {
        snd->active = 0;
    }

    return TIG_OK;
}

// 0x533480
int tig_sound_play_id(tig_sound_handle_t sound_handle, int id)
{
    char path[TIG_MAX_PATH];

    if (!tig_sound_initialized) {
        return TIG_OK;
    }

    tig_sound_file_path_resolver(id, path);

    return tig_sound_play(sound_handle, path, id);
}

// 0x5334D0
int tig_sound_play_streamed(tig_sound_handle_t sound_handle, const char* name, int loops, int fade_duration, tig_sound_handle_t prev_sound_handle)
{
    char path[TIG_MAX_PATH];
    TigSound* snd;
    HDIGDRIVER dig;
    TigSound* prev_snd;

    if (!tig_sound_initialized) {
        return TIG_OK;
    }

    if (!tig_file_extract(name, path)) {
        return TIG_OK;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return TIG_OK;
    }

    snd = &(tig_sounds[sound_handle]);
    snd->fade_duration = abs(fade_duration);
    snd->fade_step = 0;

    AIL_quick_handles(&dig, NULL, NULL);

    snd->flags |= TIG_SOUND_STREAMED;
    snd->audio_stream = AIL_open_stream(dig, path, 0);
    if (snd->audio_stream == NULL) {
        snd->active = 0;
        return TIG_OK;
    }

    strcpy(snd->path, name);
    snd->loops = loops;

    AIL_set_stream_loop_count(snd->audio_stream, loops);

    // FIXME: Fade duration checked twice.
    // FIXME: Prev sound handle should be validated with `sound_handle_is_valid`.
    if (fade_duration != 0 && prev_sound_handle >= 0 && fade_duration > 0) {
        snd->flags |= TIG_SOUND_WAIT;
    } else {
        snd->flags |= TIG_SOUND_FADE_IN;
    }

    // FIXME: Prev sound handle should be validated with `sound_handle_is_valid`.
    if (prev_sound_handle >= 0) {
        prev_snd = &(tig_sounds[prev_sound_handle]);
        if ((prev_snd->flags & TIG_SOUND_FADE_IN) != 0) {
            prev_snd->flags &= ~TIG_SOUND_FADE_IN;
        }

        if ((prev_snd->flags & TIG_SOUND_WAIT) != 0) {
            prev_snd->flags &= ~TIG_SOUND_WAIT;
        }

        prev_snd->flags |= TIG_SOUND_FADE_OUT;
        prev_snd->fade_duration = abs(fade_duration);
        prev_snd->fade_step = 0;
        prev_snd->next_sound_handle = sound_handle;
    }

    return TIG_OK;
}

// 0x533680
int tig_sound_play_streamed_indefinitely(tig_sound_handle_t sound_handle, const char* name, int fade_duration, tig_sound_handle_t prev_sound_handle)
{
    return tig_sound_play_streamed(sound_handle, name, 0, fade_duration, prev_sound_handle);
}

// 0x5336A0
int tig_sound_play_streamed_once(tig_sound_handle_t sound_handle, const char* name, int fade_duration, tig_sound_handle_t prev_sound_handle)
{
    return tig_sound_play_streamed(sound_handle, name, 1, fade_duration, prev_sound_handle);
}

// FIXME: Should return by reference, otherwise there is no way to communicate
// error.
//
// 0x5336C0
int tig_sound_get_loops(tig_sound_handle_t sound_handle)
{
    if (!tig_sound_initialized) {
        // NOTE: Probably `TIG_ERR_NOT_INITIALIZED`.
        return 1;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return 0;
    }

    return tig_sounds[sound_handle].loops;
}

// 0x533700
void tig_sound_set_loops(tig_sound_handle_t sound_handle, int loops)
{
    if (!tig_sound_initialized) {
        return;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    tig_sounds[sound_handle].loops = loops;
}

// FIXME: Should return by reference, otherwise there is no way to communicate
// error.
//
// 0x533730
int tig_sound_get_volume(tig_sound_handle_t sound_handle)
{
    if (!tig_sound_initialized) {
        return 0;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return 0;
    }

    return tig_sounds[sound_handle].volume;
}

// 0x533760
void tig_sound_set_volume(tig_sound_handle_t sound_handle, int volume)
{
    TigSound* snd;

    if (!tig_sound_initialized) {
        return;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    snd = &(tig_sounds[sound_handle]);
    if (snd->volume != volume) {
        snd->volume = volume;

        if ((snd->flags & TIG_SOUND_STREAMED) != 0) {
            AIL_set_stream_volume(snd->audio_stream, volume);
        } else if ((snd->flags & TIG_SOUND_MEMORY) != 0) {
            AIL_quick_set_volume(snd->audio_handle, volume, snd->extra_volume);
        }
    }
}

// 0x5337D0
void tig_sound_set_volume_by_type(TigSoundType type, int volume)
{
    int index;
    TigSound* snd;

    if (!tig_sound_initialized) {
        return;
    }

    for (index = 0; index < SOUND_HANDLE_MAX; index++) {
        snd = &(tig_sounds[index]);
        if (snd->active != 0) {
            if ((tig_sound_type_flags[type] & snd->flags) != 0) {
                tig_sound_set_volume(index, volume);
            }
        }
    }

    if (type == TIG_SOUND_TYPE_EFFECT) {
        tig_sound_quick_play_set_volume(volume);
    }
}

// FIXME: Should return by reference, otherwise there is no way to communicate
// error.
//
// 0x533830
TigSoundType tig_sound_get_type(tig_sound_handle_t sound_handle)
{
    TigSoundType type;

    if (!tig_sound_initialized) {
        return 0;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return 0;
    }

    for (type = 0; type < TIG_SOUND_TYPE_COUNT; type++) {
        if ((tig_sounds[sound_handle].flags & tig_sound_type_flags[type]) != 0) {
            return type;
        }
    }

    return 0;
}

// 0x533880
void tig_sound_set_type(tig_sound_handle_t sound_handle, TigSoundType type)
{
    TigSound* snd;

    if (!tig_sound_initialized) {
        return;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    snd = &(tig_sounds[sound_handle]);
    snd->flags &= ~(TIG_SOUND_EFFECT | TIG_SOUND_MUSIC | TIG_SOUND_VOICE);
    snd->flags |= tig_sound_type_flags[type];
}

// 0x5338D0
TigSoundPositionalSize tig_sound_get_positional_size(tig_sound_handle_t sound_handle)
{
    if (!tig_sound_initialized) {
        return TIG_SOUND_SIZE_LARGE;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return TIG_SOUND_SIZE_LARGE;
    }

    return tig_sounds[sound_handle].positional_size;
}

// 0x533910
void tig_sound_set_positional_size(tig_sound_handle_t sound_handle, TigSoundPositionalSize size)
{
    if (!tig_sound_initialized) {
        return;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    tig_sounds[sound_handle].positional_size = size;
}

// FIXME: Should return by reference, otherwise there is no way to communicate
// error.
//
// 0x533940
int tig_sound_get_extra_volume(tig_sound_handle_t sound_handle)
{
    if (!tig_sound_initialized) {
        return 64;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return 64;
    }

    return tig_sounds[sound_handle].extra_volume;
}

// 0x533980
void tig_sound_set_extra_volume(tig_sound_handle_t sound_handle, int extra_volume)
{
    TigSound* snd;

    if (!tig_sound_initialized) {
        return;
    }

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    snd = &(tig_sounds[sound_handle]);
    if (snd->extra_volume != extra_volume) {
        snd->extra_volume = extra_volume;

        if ((snd->flags & TIG_SOUND_STREAMED) == 0
            && (snd->flags & TIG_SOUND_MEMORY) != 0) {
            AIL_quick_set_volume(snd->audio_handle, snd->volume, extra_volume);
        }
    }
}

// 0x5339E0
bool tig_sound_is_playing_id(int id)
{
    int index;
    TigSound* snd;

    if (!tig_sound_initialized) {
        return false;
    }

    for (index = 0; index < SOUND_HANDLE_MAX; index++) {
        snd = &(tig_sounds[index]);
        if (snd->active != 0 && snd->id == id) {
            return true;
        }
    }

    return false;
}

// 0x533A20
bool tig_sound_is_playing(tig_sound_handle_t sound_handle)
{
    TigSound* snd;

    // FIXME: No `tig_sound_initialized` guard.

    if (!sound_handle_is_valid(sound_handle)) {
        return false;
    }

    snd = &(tig_sounds[sound_handle]);
    if (snd->active != 0) {
        if ((snd->flags & TIG_SOUND_MEMORY) != 0) {
            if (AIL_quick_status(snd->audio_handle) == QSTAT_PLAYING) {
                return true;
            }
        }

        if ((snd->flags & TIG_SOUND_STREAMED) != 0) {
            if (AIL_stream_status(snd->audio_stream) == SMP_PLAYING) {
                return true;
            }
        }
    }

    return false;
}

// 0x533A90
bool tig_sound_is_active(tig_sound_handle_t sound_handle)
{
    // FIXME: No `tig_sound_initialized` guard.

    if (!sound_handle_is_valid(sound_handle)) {
        return false;
    }

    return tig_sounds[sound_handle].active != 0;
}

// 0x533AC0
void tig_sound_cache_flush()
{
    if (tig_sound_initialized) {
        tig_file_cache_flush(tig_sound_cache);
    }
}

// 0x533AE0
const char* tig_sound_cache_stats()
{
    // 0x62B2C4
    static char buffer[100];

    sprintf(buffer,
        "Sound Cache: %u items, %u bytes",
        tig_sound_cache->items_count,
        tig_sound_cache->bytes);
    return buffer;
}

// 0x533B10
void tig_sound_set_position(tig_sound_handle_t sound_handle, int64_t x, int64_t y)
{
    TigSound* snd;

    // FIXME: No `tig_sound_initialized` guard.

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    snd = &(tig_sounds[sound_handle]);
    snd->positional_x = x;
    snd->positional_y = y;
    snd->positional = true;
}

// 0x533B60
void tig_sound_get_position(tig_sound_handle_t sound_handle, int64_t* x, int64_t* y)
{
    TigSound* snd;

    // FIXME: No `tig_sound_initialized` guard.

    if (!sound_handle_is_valid(sound_handle)) {
        return;
    }

    snd = &(tig_sounds[sound_handle]);
    if (snd->active != 0 && snd->positional) {
        *x = snd->positional_x;
        *y = snd->positional_y;
    }
}

// 0x533BC0
bool tig_sound_is_positional(tig_sound_handle_t sound_handle)
{
    TigSound* snd;

    // FIXME: No `tig_sound_initialized` guard.

    if (!sound_handle_is_valid(sound_handle)) {
        return false;
    }

    snd = &(tig_sounds[sound_handle]);
    if (snd->active == 0) {
        return false;
    }

    return snd->positional;
}

// 0x533BF0
void tig_sound_enumerate_positional(TigSoundEnumerateFunc* func)
{
    int index;
    TigSound* snd;

    for (index = 0; index < SOUND_HANDLE_MAX; index++) {
        snd = &(tig_sounds[index]);
        if (snd->active != 0 && snd->positional) {
            func(index);
        }
    }
}

// 0x533C30
void tig_sound_quick_play_set_volume(int volume)
{
    tig_sound_effects_volume = volume;
}

// 0x533C40
void tig_sound_quick_play(int id)
{
    tig_sound_handle_t sound_handle;

    if (!tig_sound_initialized) {
        return;
    }

    if (tig_sound_create(&sound_handle, TIG_SOUND_TYPE_EFFECT) == TIG_OK) {
        tig_sound_play_id(sound_handle, id);
        tig_sound_set_volume(sound_handle, tig_sound_effects_volume);
    }
}

// 0x533C90
void tig_sound_set_active(bool is_active)
{
    HDIGDRIVER dig;

    if (!tig_sound_initialized) {
        return;
    }

    AIL_quick_handles(&dig, NULL, NULL);

    if (is_active) {
        AIL_digital_handle_reacquire(dig);
    } else {
        AIL_digital_handle_release(dig);
    }
}
