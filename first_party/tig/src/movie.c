#include "tig/movie.h"

#include <bink_compat.h>

#include <SDL3_mixer/SDL_mixer.h>

#include "tig/color.h"
#include "tig/core.h"
#include "tig/kb.h"
#include "tig/message.h"
#include "tig/video.h"
#include "tig/window.h"

static bool tig_movie_do_frame();

// 0x62B2BC
static HBINK tig_movie_bink;

static TigVideoBuffer* tig_movie_video_buffer;
static int tig_movie_screen_width;
static int tig_movie_screen_height;

typedef struct SdlMixerSoundData {
    u8* buffer;
    u32 min_buffer_size;
    SDL_AudioStream* stream;
    MIX_Track* track;
    s32 paused;
    float volume;
    MIX_StereoGains pan;
} SdlMixerSoundData;

static BINKSNDOPEN BINKCALL SdlMixerSystemOpen(void* param);
static s32 BINKCALL SdlMixerOpen(struct BINKSND* snd, u32 freq, s32 bits, s32 chans, u32 flags, HBINK bnk);
static s32 BINKCALL SdlMixerReady(struct BINKSND* snd);
static s32 BINKCALL SdlMixerLock(struct BINKSND* snd, u8** addr, u32* len);
static s32 BINKCALL SdlMixerUnlock(struct BINKSND* snd, u32 filled);
static void BINKCALL SdlMixerSetVolume(struct BINKSND* snd, s32 volume);
static void BINKCALL SdlMixerSetPan(struct BINKSND* snd, s32 pan);
static s32 BINKCALL SdlMixerPause(struct BINKSND* snd, s32 status);
static s32 BINKCALL SdlMixerSetOnOff(struct BINKSND* snd, s32 status);
static void BINKCALL SdlMixerClose(struct BINKSND* snd);
static void setup_track(struct BINKSND* snd);
static int ref_mixer();
static void unref_mixer();

static MIX_Mixer* mixer;
static int mixer_refcount;

// 0x5314F0
int tig_movie_init(TigInitInfo* init_info)
{
    // COMPAT: Load `binkw32.dll`.
    bink_compat_init();

    BinkSetSoundSystem(SdlMixerSystemOpen, NULL);

    tig_movie_screen_width = init_info->width;
    tig_movie_screen_height = init_info->height;

    return TIG_OK;
}

// 0x531520
void tig_movie_exit()
{
    // COMPAT: Free binkw32.dll
    bink_compat_exit();
}

// 0x531530
int tig_movie_play(const char* path, TigMovieFlags movie_flags, int sound_track)
{
    unsigned int bink_open_flags = 0;
    TigMessage message;
    bool stop;
    int key = -1;
    TigVideoBufferCreateInfo vb_create_info;

    if (path == NULL) {
        tig_video_fade(0, 0, 0.0f, 1);
        return TIG_ERR_INVALID_PARAM;
    }

    if (sound_track != 0) {
        BinkSetSoundTrack(sound_track);
        bink_open_flags |= BINKSNDTRACK;
    }

    tig_movie_bink = BinkOpen(path, bink_open_flags);
    if (tig_movie_bink == NULL) {
        tig_video_fade(0, 0, 0.0f, 1);
        return TIG_ERR_IO;
    }

    vb_create_info.flags = TIG_VIDEO_BUFFER_CREATE_SYSTEM_MEMORY;
    vb_create_info.background_color = 0;
    vb_create_info.color_key = 0;
    vb_create_info.width = tig_movie_bink->Width;
    vb_create_info.height = tig_movie_bink->Height;

    if (tig_video_buffer_create(&vb_create_info, &tig_movie_video_buffer) != TIG_OK) {
        BinkClose(tig_movie_bink);
        return TIG_ERR_GENERIC;
    }

    if ((movie_flags & TIG_MOVIE_FADE_IN) != 0) {
        tig_video_fade(0, 64, 3.0f, 0);
    }

    // Reset to black.
    tig_video_fill(NULL, tig_color_make(0, 0, 0));
    tig_video_flip();

    tig_video_fade(0, 0, 0.0f, 1);

    // Clear message queue.
    while (tig_message_dequeue(&message) == TIG_OK) {
    }

    stop = false;
    while (!stop && tig_movie_do_frame()) {
        tig_video_flip();
        tig_ping();

        while (tig_message_dequeue(&message) == TIG_OK) {
            if ((movie_flags & TIG_MOVIE_IGNORE_KEYBOARD) == 0
                && message.type == TIG_MESSAGE_KEYBOARD
                && message.data.keyboard.pressed == 1) {
                key = message.data.keyboard.key;
                stop = true;
            }
        }
    }

    if (key != -1) {
        // Wait until the key is released.
        while (tig_kb_is_key_pressed(key)) {
            tig_ping();
        }
    }

    // Clear message queue.
    while (tig_message_dequeue(&message) == TIG_OK) {
    }

    tig_video_buffer_destroy(tig_movie_video_buffer);
    tig_movie_video_buffer = NULL;

    BinkClose(tig_movie_bink);
    tig_movie_bink = NULL;

    tig_window_invalidate_rect(NULL);

    if ((movie_flags & TIG_MOVIE_FADE_OUT) != 0) {
        tig_video_fade(0, 0, 0.0f, 0);
    }

    if ((movie_flags & TIG_MOVIE_NO_FINAL_FLIP) == 0) {
        tig_video_fill(NULL, tig_color_make(0, 0, 0));
        tig_video_flip();
    }

    return TIG_OK;
}

// NOTE: Original code probably provides access to the underlying BINK pointer.
// However this function is only used to check if movie is currently playing
// during message handling, so there is no point to expose it.
//
// 0x5317C0
bool tig_movie_is_playing()
{
    return tig_movie_bink != NULL;
}

// 0x5317D0
bool tig_movie_do_frame()
{
    TigVideoBufferData video_buffer_data;
    TigRect src_rect;
    TigRect dst_rect;

    if (!BinkWait(tig_movie_bink)) {
        BinkDoFrame(tig_movie_bink);

        if (tig_video_buffer_lock(tig_movie_video_buffer) != TIG_OK) {
            return false;
        }

        if (tig_video_buffer_data(tig_movie_video_buffer, &video_buffer_data) != TIG_OK) {
            return false;
        }

        src_rect.x = 0;
        src_rect.y = 0;
        src_rect.width = video_buffer_data.width;
        src_rect.height = video_buffer_data.height;

        dst_rect.x = (tig_movie_screen_width - video_buffer_data.width) / 2;
        dst_rect.y = (tig_movie_screen_height - video_buffer_data.height) / 2;
        dst_rect.width = video_buffer_data.width;
        dst_rect.height = video_buffer_data.height;

        // Copy movie pixels to the video buffer.
        BinkCopyToBuffer(tig_movie_bink,
            video_buffer_data.surface_data.pixels,
            video_buffer_data.pitch,
            video_buffer_data.height,
            0,
            0,
            3);

        tig_video_buffer_unlock(tig_movie_video_buffer);

        // Blit buffer to the center of the screen.
        tig_video_blit(tig_movie_video_buffer, &src_rect, &dst_rect);

        if (tig_movie_bink->FrameNum == tig_movie_bink->Frames) {
            return false;
        }

        BinkNextFrame(tig_movie_bink);
    }

    return true;
}

BINKSNDOPEN BINKCALL SdlMixerSystemOpen(void* param)
{
    (void)param;

    return SdlMixerOpen;
}

s32 BINKCALL SdlMixerOpen(struct BINKSND* snd, u32 freq, s32 bits, s32 chans, u32 flags, HBINK bnk)
{
    SdlMixerSoundData* snddata;
    SDL_AudioSpec src_spec;
    SDL_AudioSpec dst_spec;

    (void)flags;
    (void)bnk;

    memset(snd, 0, sizeof(*snd));

    if (ref_mixer() == 0) {
        return 0;
    }

    snd->freq = freq;
    snd->bits = bits;
    snd->chans = chans;

    snddata = (SdlMixerSoundData*)snd->snddata;

    src_spec.channels = chans;
    src_spec.freq = freq;
    src_spec.format = SDL_AUDIO_S16LE;
    MIX_GetMixerFormat(mixer, &dst_spec);

    snddata->min_buffer_size = 16536;
    snddata->buffer = SDL_malloc(snddata->min_buffer_size * 2);
    if (snddata->buffer == NULL) {
        return 0;
    }

    snddata->stream = SDL_CreateAudioStream(&src_spec, &dst_spec);
    if (snddata->stream == NULL) {
        SDL_free(snddata->buffer);
        return 0;
    }

    snddata->track = MIX_CreateTrack(mixer);
    if (snddata->track == NULL) {
        SDL_DestroyAudioStream(snddata->stream);
        SDL_free(snddata->buffer);
        return 0;
    }

    if (!MIX_SetTrackAudioStream(snddata->track, snddata->stream)) {
        MIX_DestroyTrack(snddata->track);
        SDL_DestroyAudioStream(snddata->stream);
        SDL_free(snddata->buffer);
        return 0;
    }

    snddata->paused = 0;
    snddata->volume = 1.0f;
    snddata->pan.left = 0.5f;
    snddata->pan.right = 0.5f;
    setup_track(snd);

    snd->Latency = 64;
    snd->Ready = SdlMixerReady;
    snd->Lock = SdlMixerLock;
    snd->Unlock = SdlMixerUnlock;
    snd->Volume = SdlMixerSetVolume;
    snd->Pan = SdlMixerSetPan;
    snd->Pause = SdlMixerPause;
    snd->SetOnOff = SdlMixerSetOnOff;
    snd->Close = SdlMixerClose;

    return 1;
}

s32 BINKCALL SdlMixerReady(struct BINKSND* snd)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    if (snddata->paused && !snd->OnOff) {
        return 0;
    }

    return SDL_GetAudioStreamQueued(snddata->stream) < (int)snddata->min_buffer_size;
}

s32 BINKCALL SdlMixerLock(struct BINKSND* snd, u8** addr, u32* len)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    *addr = snddata->buffer;
    *len = snddata->min_buffer_size;

    return 1;
}

s32 BINKCALL SdlMixerUnlock(struct BINKSND* snd, u32 filled)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    SDL_PutAudioStreamData(snddata->stream, snddata->buffer, filled);
    MIX_PlayTrack(snddata->track, 0);

    return 1;
}

void BINKCALL SdlMixerSetVolume(struct BINKSND* snd, s32 volume)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    if (volume < 0) {
        volume = 0;
    } else if (volume > 32767) {
        volume = 32767;
    }

    snddata->volume = (float)volume / 32767.0f;

    MIX_SetTrackGain(snddata->track, snddata->volume);
}

void BINKCALL SdlMixerSetPan(struct BINKSND* snd, s32 pan)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    if (pan < 0) {
        pan = 0;
    } else if (pan > 65536) {
        pan = 65536;
    }

    snddata->pan.left = (65536.0f - (float)pan) / 65536.0f;
    snddata->pan.right = (float)pan / 65536.0f;

    MIX_SetTrackStereo(snddata->track, &(snddata->pan));
}

s32 BINKCALL SdlMixerPause(struct BINKSND* snd, s32 status)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    if (status == 0) {
        if (snddata->paused == 1) {
            MIX_ResumeTrack(snddata->track);
            snddata->paused = 0;
        }
    } else if (status == 1) {
        if (snddata->paused == 0) {
            MIX_PauseTrack(snddata->track);
            snddata->paused = 1;
        }
    }

    return snddata->paused;
}

s32 BINKCALL SdlMixerSetOnOff(struct BINKSND* snd, s32 status)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    if (status == 0) {
        if (snd->OnOff == 1) {
            MIX_StopTrack(snddata->track, 0);
            snd->OnOff = 0;
        }
    } else if (status == 1) {
        if (snd->OnOff == 0) {
            setup_track(snd);
            snd->OnOff = 1;
        }
    }

    return snd->OnOff;
}

void BINKCALL SdlMixerClose(struct BINKSND* snd)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    MIX_DestroyTrack(snddata->track);
    SDL_DestroyAudioStream(snddata->stream);
    SDL_free(snddata->buffer);

    unref_mixer();
}

void setup_track(struct BINKSND* snd)
{
    SdlMixerSoundData* snddata = (SdlMixerSoundData*)snd->snddata;

    MIX_SetTrackGain(snddata->track, snddata->volume);
    MIX_SetTrackStereo(snddata->track, &(snddata->pan));
}

int ref_mixer()
{
    if (++mixer_refcount == 1) {
        mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
        if (mixer == NULL) {
            --mixer_refcount;
            return 0;
        }
    }

    return 1;
}

void unref_mixer()
{
    if (--mixer_refcount == 0) {
        MIX_DestroyMixer(mixer);
    }
}
