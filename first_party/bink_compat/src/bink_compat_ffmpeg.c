// FFmpeg-based backend for the Bink compatibility layer.
//
// The original game links against RAD's `binkw32.dll`, which is Windows-only.
// This backend reimplements the small subset of the Bink API the engine uses
// (see `bink_compat.h`) on top of FFmpeg's Bink 1 decoder, so movies play on
// platforms where the DLL is unavailable (macOS, Linux).
//
// FFmpeg (libavcodec/libavformat/libswscale/libswresample) is LGPL-2.1. It is
// loaded at runtime with `dlopen` and no GPL components are used, so this
// backend does not affect the licensing of the rest of the program. Loading
// lazily (rather than link-time) means the game still starts and simply skips
// movies when FFmpeg is not installed, instead of failing to launch.

#include "bink_compat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dlfcn.h>

#include <SDL3/SDL_timer.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>

// FFmpeg entry points resolved at runtime from the shared libraries. Using a
// function-pointer table lets us `dlopen` the libraries lazily and skip movie
// playback gracefully when they are absent.
static struct {
    int loaded;

    unsigned (*avformat_version)(void);

    int (*avformat_open_input)(AVFormatContext**, const char*, const AVInputFormat*, AVDictionary**);
    void (*avformat_close_input)(AVFormatContext**);
    int (*avformat_find_stream_info)(AVFormatContext*, AVDictionary**);
    int (*av_read_frame)(AVFormatContext*, AVPacket*);

    const AVCodec* (*avcodec_find_decoder)(enum AVCodecID);
    AVCodecContext* (*avcodec_alloc_context3)(const AVCodec*);
    void (*avcodec_free_context)(AVCodecContext**);
    int (*avcodec_parameters_to_context)(AVCodecContext*, const AVCodecParameters*);
    int (*avcodec_open2)(AVCodecContext*, const AVCodec*, AVDictionary**);
    int (*avcodec_send_packet)(AVCodecContext*, const AVPacket*);
    int (*avcodec_receive_frame)(AVCodecContext*, AVFrame*);

    AVFrame* (*av_frame_alloc)(void);
    void (*av_frame_free)(AVFrame**);
    void (*av_frame_unref)(AVFrame*);
    AVPacket* (*av_packet_alloc)(void);
    void (*av_packet_free)(AVPacket**);
    void (*av_packet_unref)(AVPacket*);
    void* (*av_malloc)(size_t);
    void (*av_free)(void*);
    void (*av_channel_layout_default)(AVChannelLayout*, int);

    struct SwsContext* (*sws_getContext)(int, int, enum AVPixelFormat, int, int, enum AVPixelFormat, int, SwsFilter*, SwsFilter*, const double*);
    void (*sws_freeContext)(struct SwsContext*);
    int (*sws_scale)(struct SwsContext*, const uint8_t* const*, const int*, int, int, uint8_t* const*, const int*);

    int (*swr_alloc_set_opts2)(struct SwrContext**, const AVChannelLayout*, enum AVSampleFormat, int, const AVChannelLayout*, enum AVSampleFormat, int, int, void*);
    int (*swr_init)(struct SwrContext*);
    void (*swr_free)(struct SwrContext**);
    int (*swr_convert)(struct SwrContext*, uint8_t**, int, const uint8_t**, int);
    int64_t (*swr_get_out_samples)(struct SwrContext*, int);
} ff;

// Open a versioned FFmpeg shared library, trying the platform's common naming.
// The configured FFmpeg library directory is tried first so the libraries are
// found even outside the loader's default search path (e.g. Homebrew on
// macOS); the bare SONAME is used as a fallback (standard install on Linux).
static void* ff_dlopen(const char* base, int version)
{
    char name[512];
    void* handle;

#if defined(__APPLE__)
    const char* pattern = "lib%s.%d.dylib";
#else
    const char* pattern = "lib%s.so.%d";
#endif

#ifdef FFMPEG_LIBDIR
    {
        char leaf[64];
        snprintf(leaf, sizeof(leaf), pattern, base, version);
        snprintf(name, sizeof(name), "%s/%s", FFMPEG_LIBDIR, leaf);
        handle = dlopen(name, RTLD_NOW | RTLD_GLOBAL);
        if (handle != NULL) {
            return handle;
        }
    }
#endif

    snprintf(name, sizeof(name), pattern, base, version);
    return dlopen(name, RTLD_NOW | RTLD_GLOBAL);
}

#define FF_SYM(handle, member, symbol)                       \
    do {                                                     \
        *(void**)(&ff.member) = dlsym((handle), (symbol));   \
        if (ff.member == NULL) {                             \
            return false;                                    \
        }                                                    \
    } while (0)

// The sound system callback registered by the engine via `BinkSetSoundSystem`.
// The engine provides an SDL_mixer-backed `BINKSND` implementation; we drive it
// exactly as the real DLL would: open it once, then push decoded PCM through
// its Ready/Lock/Unlock protocol.
static BINKSNDSYSOPEN bink_snd_sys_open;
static void* bink_snd_sys_param;
static unsigned bink_snd_track;

typedef struct BinkContext {
    BINK public_;

    AVFormatContext* fmt;
    int video_stream;
    int audio_stream;

    AVCodecContext* video_ctx;
    AVCodecContext* audio_ctx;
    struct SwsContext* sws;
    struct SwrContext* swr;

    AVFrame* frame;
    AVPacket* packet;

    // Most recently decoded video frame, converted to BGRA and ready for
    // `BinkCopyToBuffer`.
    uint8_t* rgb_data;
    int rgb_linesize;

    // Sound sink provided by the engine.
    BINKSND snd;
    int snd_open;

    // S16 interleaved scratch buffer for resampled audio.
    uint8_t* audio_buf;
    int audio_buf_size;

    // Frame pacing.
    double frame_ms;      // Wall-clock duration of one frame in milliseconds.
    Uint64 start_ticks;   // Playback start time (SDL ticks).
    int started;

    int eof;
} BinkContext;

static void feed_audio(BinkContext* ctx, AVFrame* frame);

int BINKCALL BinkSetSoundSystem(BINKSNDSYSOPEN open, void* param)
{
    bink_snd_sys_open = open;
    bink_snd_sys_param = param;
    return 1;
}

void BINKCALL BinkSetSoundTrack(unsigned track)
{
    bink_snd_track = track;
}

BINKSNDOPEN BINKCALL BinkOpenMiles(void* param)
{
    (void)param;
    return NULL;
}

int BINKCALL BinkDDSurfaceType(void* lpDDS)
{
    (void)lpDDS;
    return 0;
}

HBINK BINKCALL BinkOpen(const char* name, unsigned flags)
{
    BinkContext* ctx;
    const AVCodec* codec;
    int i;

    (void)flags;

    if (!ff.loaded) {
        return NULL;
    }

    ctx = (BinkContext*)calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        return NULL;
    }

    ctx->video_stream = -1;
    ctx->audio_stream = -1;

    if (ff.avformat_open_input(&ctx->fmt, name, NULL, NULL) < 0) {
        free(ctx);
        return NULL;
    }

    if (ff.avformat_find_stream_info(ctx->fmt, NULL) < 0) {
        goto fail;
    }

    for (i = 0; i < (int)ctx->fmt->nb_streams; i++) {
        enum AVMediaType type = ctx->fmt->streams[i]->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO && ctx->video_stream < 0) {
            ctx->video_stream = i;
        } else if (type == AVMEDIA_TYPE_AUDIO && ctx->audio_stream < 0) {
            ctx->audio_stream = i;
        }
    }

    if (ctx->video_stream < 0) {
        goto fail;
    }

    // Video decoder.
    {
        AVCodecParameters* par = ctx->fmt->streams[ctx->video_stream]->codecpar;
        codec = ff.avcodec_find_decoder(par->codec_id);
        if (codec == NULL) {
            goto fail;
        }

        ctx->video_ctx = ff.avcodec_alloc_context3(codec);
        if (ctx->video_ctx == NULL) {
            goto fail;
        }

        if (ff.avcodec_parameters_to_context(ctx->video_ctx, par) < 0) {
            goto fail;
        }

        if (ff.avcodec_open2(ctx->video_ctx, codec, NULL) < 0) {
            goto fail;
        }

        ctx->public_.Width = (unsigned)ctx->video_ctx->width;
        ctx->public_.Height = (unsigned)ctx->video_ctx->height;
        ctx->public_.Frames = (unsigned)ctx->fmt->streams[ctx->video_stream]->nb_frames;
        ctx->public_.FrameNum = 0;

        // Derive the per-frame wall-clock duration for pacing.
        {
            AVStream* st = ctx->fmt->streams[ctx->video_stream];
            AVRational fr = st->avg_frame_rate;
            if (fr.num <= 0 || fr.den <= 0) {
                fr = st->r_frame_rate;
            }
            if (fr.num > 0 && fr.den > 0) {
                ctx->frame_ms = 1000.0 * (double)fr.den / (double)fr.num;
            } else {
                ctx->frame_ms = 1000.0 / 15.0; // Bink default fallback.
            }

            // Bink streams frequently report nb_frames == 0. The engine's play
            // loop stops when FrameNum reaches Frames, so derive a frame count
            // from the stream duration when it is missing, otherwise the movie
            // ends after a single frame.
            if (ctx->public_.Frames == 0 && fr.num > 0 && fr.den > 0 && st->duration > 0) {
                double secs = (double)st->duration * av_q2d(st->time_base);
                ctx->public_.Frames = (unsigned)(secs * (double)fr.num / (double)fr.den + 0.5);
            }
            if (ctx->public_.Frames == 0) {
                // Neither a frame count nor a duration is available. Leave
                // Frames at 0; playback termination falls back to the decode
                // EOF handling in BinkDoFrame/BinkNextFrame rather than a fixed
                // count, so the movie cannot hang re-blitting the last frame.
                ctx->public_.Frames = 1;
            }
        }

        // Converter to BGRA, matching the engine's XRGB8888 surface (byte order
        // B, G, R, X in little-endian memory).
        ctx->sws = ff.sws_getContext(ctx->video_ctx->width,
            ctx->video_ctx->height,
            ctx->video_ctx->pix_fmt,
            ctx->video_ctx->width,
            ctx->video_ctx->height,
            AV_PIX_FMT_BGRA,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL);
        if (ctx->sws == NULL) {
            goto fail;
        }

        ctx->rgb_linesize = ctx->video_ctx->width * 4;
        ctx->rgb_data = (uint8_t*)ff.av_malloc((size_t)ctx->rgb_linesize * ctx->video_ctx->height);
        if (ctx->rgb_data == NULL) {
            goto fail;
        }
    }

    // Audio decoder (optional).
    if (ctx->audio_stream >= 0 && bink_snd_sys_open != NULL) {
        AVCodecParameters* par = ctx->fmt->streams[ctx->audio_stream]->codecpar;
        const AVCodec* acodec = ff.avcodec_find_decoder(par->codec_id);
        if (acodec != NULL) {
            ctx->audio_ctx = ff.avcodec_alloc_context3(acodec);
            if (ctx->audio_ctx != NULL
                && ff.avcodec_parameters_to_context(ctx->audio_ctx, par) >= 0
                && ff.avcodec_open2(ctx->audio_ctx, acodec, NULL) >= 0) {
                AVChannelLayout out_layout;
                int out_chans = ctx->audio_ctx->ch_layout.nb_channels;

                ff.av_channel_layout_default(&out_layout, out_chans);

                // Resample whatever the decoder outputs to interleaved S16, the
                // format the engine's sound sink expects.
                ff.swr_alloc_set_opts2(&ctx->swr,
                    &out_layout,
                    AV_SAMPLE_FMT_S16,
                    ctx->audio_ctx->sample_rate,
                    &ctx->audio_ctx->ch_layout,
                    ctx->audio_ctx->sample_fmt,
                    ctx->audio_ctx->sample_rate,
                    0,
                    NULL);

                if (ctx->swr != NULL && ff.swr_init(ctx->swr) >= 0) {
                    BINKSNDOPEN sndopen = bink_snd_sys_open(bink_snd_sys_param);
                    if (sndopen != NULL
                        && sndopen(&ctx->snd,
                               (u32)ctx->audio_ctx->sample_rate,
                               16,
                               out_chans,
                               0,
                               (HBINK)ctx)
                            != 0) {
                        ctx->snd_open = 1;
                    }
                }
            }
        }
    }

    ctx->frame = ff.av_frame_alloc();
    ctx->packet = ff.av_packet_alloc();
    if (ctx->frame == NULL || ctx->packet == NULL) {
        goto fail;
    }

    return (HBINK)ctx;

fail:
    BinkClose((HBINK)ctx);
    return NULL;
}

void BINKCALL BinkClose(HBINK bnk)
{
    BinkContext* ctx = (BinkContext*)bnk;
    if (ctx == NULL) {
        return;
    }

    if (ctx->snd_open && ctx->snd.Close != NULL) {
        ctx->snd.Close(&ctx->snd);
    }

    ff.av_packet_free(&ctx->packet);
    ff.av_frame_free(&ctx->frame);
    ff.av_free(ctx->rgb_data);
    free(ctx->audio_buf);
    ff.sws_freeContext(ctx->sws);
    ff.swr_free(&ctx->swr);
    ff.avcodec_free_context(&ctx->video_ctx);
    ff.avcodec_free_context(&ctx->audio_ctx);
    ff.avformat_close_input(&ctx->fmt);
    free(ctx);
}

// Decode packets until a full video frame is produced. Audio packets
// encountered along the way are decoded and pushed to the sound sink.
int BINKCALL BinkDoFrame(HBINK bnk)
{
    BinkContext* ctx = (BinkContext*)bnk;
    if (ctx == NULL) {
        return 1;
    }

    for (;;) {
        int ret = ff.avcodec_receive_frame(ctx->video_ctx, ctx->frame);
        if (ret == 0) {
            // Got a video frame - convert to BGRA.
            uint8_t* dst[4] = { ctx->rgb_data, NULL, NULL, NULL };
            int dst_linesize[4] = { ctx->rgb_linesize, 0, 0, 0 };
            ff.sws_scale(ctx->sws,
                (const uint8_t* const*)ctx->frame->data,
                ctx->frame->linesize,
                0,
                ctx->video_ctx->height,
                dst,
                dst_linesize);
            ff.av_frame_unref(ctx->frame);
            return 0;
        }

        if (ret != AVERROR(EAGAIN)) {
            // Decoder is drained or errored. Snap the frame counter to the total
            // so the consumer's `FrameNum == Frames` termination check trips,
            // regardless of whether the derived frame count was exact.
            ctx->eof = 1;
            ctx->public_.FrameNum = ctx->public_.Frames;
            return 1;
        }

        // Need more input.
        if (ff.av_read_frame(ctx->fmt, ctx->packet) < 0) {
            // Flush the video decoder at EOF.
            ff.avcodec_send_packet(ctx->video_ctx, NULL);
            ctx->eof = 1;
            continue;
        }

        if (ctx->packet->stream_index == ctx->video_stream) {
            ff.avcodec_send_packet(ctx->video_ctx, ctx->packet);
        } else if (ctx->packet->stream_index == ctx->audio_stream && ctx->audio_ctx != NULL) {
            if (ff.avcodec_send_packet(ctx->audio_ctx, ctx->packet) >= 0) {
                while (ff.avcodec_receive_frame(ctx->audio_ctx, ctx->frame) == 0) {
                    feed_audio(ctx, ctx->frame);
                    ff.av_frame_unref(ctx->frame);
                }
            }
        }

        ff.av_packet_unref(ctx->packet);
    }
}

void BINKCALL BinkNextFrame(HBINK bnk)
{
    BinkContext* ctx = (BinkContext*)bnk;
    if (ctx != NULL) {
        ctx->public_.FrameNum++;
    }
}

// The engine polls this before each frame to pace playback. Return nonzero
// ("wait") until enough wall-clock time has elapsed for the current frame, so
// the movie plays at its natural rate instead of as fast as it can decode.
int BINKCALL BinkWait(HBINK bnk)
{
    BinkContext* ctx = (BinkContext*)bnk;
    Uint64 now;
    double due;

    if (ctx == NULL) {
        return 0;
    }

    now = SDL_GetTicks();

    if (!ctx->started) {
        ctx->started = 1;
        ctx->start_ticks = now;
        return 0;
    }

    // Time at which the current frame is due, relative to playback start.
    due = (double)ctx->public_.FrameNum * ctx->frame_ms;

    if ((double)(now - ctx->start_ticks) < due) {
        return 1;
    }

    return 0;
}

int BINKCALL BinkCopyToBuffer(HBINK bnk, void* dest, int destpitch, unsigned destheight, unsigned destx, unsigned desty, unsigned flags)
{
    BinkContext* ctx = (BinkContext*)bnk;
    unsigned y;
    unsigned rows;

    (void)flags;

    if (ctx == NULL || dest == NULL) {
        return -1;
    }

    rows = ctx->public_.Height;
    if (destheight < rows) {
        rows = destheight;
    }

    for (y = 0; y < rows; y++) {
        const uint8_t* src = ctx->rgb_data + (size_t)y * ctx->rgb_linesize;
        uint8_t* dst = (uint8_t*)dest + (size_t)(y + desty) * destpitch + (size_t)destx * 4;
        memcpy(dst, src, (size_t)ctx->public_.Width * 4);
    }

    return 0;
}

// Push a decoded audio frame through the engine's sound sink, resampling to
// interleaved S16 and honoring the sink's Ready/Lock/Unlock protocol.
static void feed_audio(BinkContext* ctx, AVFrame* frame)
{
    int out_chans;
    int max_out;
    int needed;
    int out_samples;

    if (!ctx->snd_open) {
        return;
    }

    out_chans = ctx->audio_ctx->ch_layout.nb_channels;
    max_out = ff.swr_get_out_samples(ctx->swr, frame->nb_samples);
    needed = max_out * out_chans * 2;

    if (needed > ctx->audio_buf_size) {
        uint8_t* grown = (uint8_t*)realloc(ctx->audio_buf, (size_t)needed);
        if (grown == NULL) {
            return;
        }
        ctx->audio_buf = grown;
        ctx->audio_buf_size = needed;
    }

    out_samples = ff.swr_convert(ctx->swr,
        &ctx->audio_buf,
        max_out,
        (const uint8_t**)frame->extended_data,
        frame->nb_samples);
    if (out_samples <= 0) {
        return;
    }

    {
        int total = out_samples * out_chans * 2;
        int offset = 0;

        // Hand the PCM to the sink in Lock-sized chunks. The sink queues into an
        // SDL audio stream that accepts data unconditionally and plays it at the
        // device rate, so we must push every sample - dropping any would let the
        // remaining audio play back-to-back and run ahead of the video.
        while (offset < total) {
            u8* addr = NULL;
            u32 len = 0;
            int chunk;

            if (ctx->snd.Lock == NULL || !ctx->snd.Lock(&ctx->snd, &addr, &len)) {
                break;
            }

            chunk = total - offset;
            if (chunk > (int)len) {
                chunk = (int)len;
            }

            memcpy(addr, ctx->audio_buf + offset, (size_t)chunk);

            if (ctx->snd.Unlock != NULL) {
                ctx->snd.Unlock(&ctx->snd, (u32)chunk);
            }

            offset += chunk;
        }
    }
}

bool bink_compat_init(void)
{
    void* avcodec;
    void* avformat;
    void* avutil;
    void* swscale;
    void* swresample;

    if (ff.loaded) {
        return true;
    }

    // Load the FFmpeg shared libraries by their major SONAME version. These are
    // left leaked intentionally: the process needs them for its whole lifetime,
    // and there is no matching teardown point in the engine.
    avutil = ff_dlopen("avutil", LIBAVUTIL_VERSION_MAJOR);
    swresample = ff_dlopen("swresample", LIBSWRESAMPLE_VERSION_MAJOR);
    avcodec = ff_dlopen("avcodec", LIBAVCODEC_VERSION_MAJOR);
    avformat = ff_dlopen("avformat", LIBAVFORMAT_VERSION_MAJOR);
    swscale = ff_dlopen("swscale", LIBSWSCALE_VERSION_MAJOR);

    if (avutil == NULL || swresample == NULL || avcodec == NULL
        || avformat == NULL || swscale == NULL) {
        // FFmpeg is not installed. Movies will be skipped; the game still runs.
        return false;
    }

    FF_SYM(avformat, avformat_open_input, "avformat_open_input");
    FF_SYM(avformat, avformat_close_input, "avformat_close_input");
    FF_SYM(avformat, avformat_find_stream_info, "avformat_find_stream_info");
    FF_SYM(avformat, av_read_frame, "av_read_frame");

    FF_SYM(avcodec, avcodec_find_decoder, "avcodec_find_decoder");
    FF_SYM(avcodec, avcodec_alloc_context3, "avcodec_alloc_context3");
    FF_SYM(avcodec, avcodec_free_context, "avcodec_free_context");
    FF_SYM(avcodec, avcodec_parameters_to_context, "avcodec_parameters_to_context");
    FF_SYM(avcodec, avcodec_open2, "avcodec_open2");
    FF_SYM(avcodec, avcodec_send_packet, "avcodec_send_packet");
    FF_SYM(avcodec, avcodec_receive_frame, "avcodec_receive_frame");
    FF_SYM(avcodec, av_packet_alloc, "av_packet_alloc");
    FF_SYM(avcodec, av_packet_free, "av_packet_free");
    FF_SYM(avcodec, av_packet_unref, "av_packet_unref");

    FF_SYM(avutil, av_frame_alloc, "av_frame_alloc");
    FF_SYM(avutil, av_frame_free, "av_frame_free");
    FF_SYM(avutil, av_frame_unref, "av_frame_unref");
    FF_SYM(avutil, av_malloc, "av_malloc");
    FF_SYM(avutil, av_free, "av_free");
    FF_SYM(avutil, av_channel_layout_default, "av_channel_layout_default");

    FF_SYM(swscale, sws_getContext, "sws_getContext");
    FF_SYM(swscale, sws_freeContext, "sws_freeContext");
    FF_SYM(swscale, sws_scale, "sws_scale");

    FF_SYM(swresample, swr_alloc_set_opts2, "swr_alloc_set_opts2");
    FF_SYM(swresample, swr_init, "swr_init");
    FF_SYM(swresample, swr_free, "swr_free");
    FF_SYM(swresample, swr_convert, "swr_convert");
    FF_SYM(swresample, swr_get_out_samples, "swr_get_out_samples");

    ff.loaded = 1;
    return true;
}

void bink_compat_exit(void)
{
}
