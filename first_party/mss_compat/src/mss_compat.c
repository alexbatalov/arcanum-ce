#include "mss_compat.h"

#include <stdlib.h>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

struct AUDIO {
    MIX_Audio* audio;
    MIX_Track* track;
};

struct STREAM {
    MIX_Audio* audio;
    MIX_Track* track;
    int loops;
};

static MIX_Mixer* mixer;

void AILCALL AIL_close_stream(HSTREAM stream)
{
    MIX_DestroyTrack(stream->track);
    MIX_DestroyAudio(stream->audio);
    free(stream);
}

int AILCALL AIL_digital_handle_reacquire(HDIGDRIVER drvr)
{
    return 1;
}

int AILCALL AIL_digital_handle_release(HDIGDRIVER drvr)
{
    return 1;
}

HSTREAM AILCALL AIL_open_stream(HDIGDRIVER dig, const char* filename, int stream_mem)
{
    HSTREAM stream = malloc(sizeof(*stream));
    stream->track = MIX_CreateTrack(mixer);
    stream->audio = MIX_LoadAudio(mixer, filename, false);
    MIX_SetTrackAudio(stream->track, stream->audio);
    return stream;
}

void AILCALL AIL_quick_handles(HDIGDRIVER* pdig, HMDIDRIVER* pmdi, HDLSDEVICE* pdls)
{
    if (pdig != NULL) {
        *pdig = NULL;
    }

    if (pmdi != NULL) {
        *pmdi = NULL;
    }

    if (pdls != NULL) {
        *pdls = NULL;
    }
}

HAUDIO AILCALL AIL_quick_load_mem(void const* mem, unsigned size)
{
    HAUDIO audio = malloc(sizeof(*audio));
    audio->track = MIX_CreateTrack(mixer);
    audio->audio = MIX_LoadAudio_IO(mixer, SDL_IOFromConstMem(mem, size), true, true);
    MIX_SetTrackAudio(audio->track, audio->audio);
    return audio;
}

int AILCALL AIL_quick_play(HAUDIO audio, unsigned loop_count)
{
    SDL_PropertiesID props;
    bool ret;

    props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loop_count - 1);
    ret = MIX_PlayTrack(audio->track, props);
    SDL_DestroyProperties(props);

    return ret ? 1 : 0;
}

void AILCALL AIL_quick_set_volume(HAUDIO audio, int volume, int extravol)
{
    (void)extravol;

    MIX_SetTrackGain(audio->track, (float)volume / 128.0f);
}

void AILCALL AIL_quick_shutdown(void)
{
    MIX_DestroyMixer(mixer);
}

int AILCALL AIL_quick_startup(int use_digital, int use_MIDI, unsigned output_rate, int output_bits, int output_channels)
{
    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    if (mixer == NULL) {
        return 0;
    }

    return 1;
}

int AILCALL AIL_quick_status(HAUDIO audio)
{
    if (MIX_TrackPlaying(audio->track)) {
        return QSTAT_PLAYING;
    } else if (MIX_GetTrackRemaining(audio->track) == 0) {
        return QSTAT_DONE;
    } else {
        return 0;
    }
}

void AILCALL AIL_quick_unload(HAUDIO audio)
{
    MIX_DestroyTrack(audio->track);
    MIX_DestroyAudio(audio->audio);
    free(audio);
}

void AILCALL AIL_set_stream_loop_count(HSTREAM stream, int count)
{
    stream->loops = count;
}

void AILCALL AIL_set_stream_volume(HSTREAM stream, int volume)
{
    MIX_SetTrackGain(stream->track, (float)volume / 128.0f);
}

void AILCALL AIL_start_stream(HSTREAM stream)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, stream->loops - 1);
    MIX_PlayTrack(stream->track, props);
    SDL_DestroyProperties(props);
}

int AILCALL AIL_stream_status(HSTREAM stream)
{
    if (MIX_TrackPlaying(stream->track)) {
        return SMP_PLAYING;
    } else {
        return 0;
    }
}

bool mss_compat_init()
{
    if (!MIX_Init()) {
        return false;
    }

    return true;
}

void mss_compat_exit(void)
{
    MIX_Quit();
}
