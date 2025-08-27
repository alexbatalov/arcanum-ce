#ifndef TIG_TYPES_H_
#define TIG_TYPES_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#define SDL_INCLUDE_STDBOOL_H
#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIG_MAX_PATH 260

typedef unsigned int tig_art_id_t;
typedef unsigned int tig_button_handle_t;
typedef unsigned int tig_color_t;
typedef intptr_t tig_font_handle_t;
typedef unsigned int tig_window_handle_t;

typedef struct TigRect TigRect;
typedef struct TigVideoBuffer TigVideoBuffer;
typedef struct TigPaletteModifyInfo TigPaletteModifyInfo;
typedef struct TigArtBlitInfo TigArtBlitInfo;
typedef struct TigMessage TigMessage;
typedef struct TigBmp TigBmp;

// Palette is rare case of non-opaque pointer. It is an array of 256 elements,
// either `uint16_t` (in 16 bpp video mode), or `uint32_t` (in 24 and 32 bpp
// video mode). Be sure to cast appropriately.
typedef void* TigPalette;

typedef enum TigError {
    // No error.
    TIG_OK = 0,

    // Indicates that the subsystem has not been initialized.
    TIG_ERR_NOT_INITIALIZED = 1,

    // Indicates that the subsystem is already initialized.
    TIG_ERR_ALREADY_INITIALIZED = 2,

    // Indicates that the subsystem has run out of free handles or has used all
    // slots reserved for appropriate objects. This typically means that
    // resources are exhausted.
    TIG_ERR_OUT_OF_HANDLES = 3,

    // Indicates that two rects have no intersection.
    TIG_ERR_NO_INTERSECTION = 4,

    // Indicates that memory allocation has failed.
    TIG_ERR_OUT_OF_MEMORY = 5,

    // Indicates that the screen surface is already locked.
    //
    // This error occurs when an attempt is made to lock a screen surface that
    // is already locked for access.
    TIG_ERR_ALREADY_LOCKED = 6,

    // Indicates that an underlying DirectX operation has failed.
    TIG_ERR_DIRECTX = 7,

    // Indicates that the internal message queue is empty.
    //
    // This error occurs when an attempt is made to dequeue next `TigMessage`
    // from the queue.
    TIG_ERR_MESSAGE_QUEUE_EMPTY = 10,

    // Indicates that a blit to screen operation has failed.
    TIG_ERR_BLIT = 11,

    // Indicates that some function parameter is invalid (i.e. out of range,
    // bad value, etc.).
    TIG_ERR_INVALID_PARAM = 12,

    // Indicates an I/O operation error.
    TIG_ERR_IO = 13,

    // Indicates a network error.
    TIG_ERR_NET = 14,

    // Indicates a generic error.
    TIG_ERR_GENERIC = 16,
} TigError;

typedef unsigned int TigInitFlags;

// Display FPS counter in the top-left corner of the window.
//
// NOTE: The counter is rendered using GDI which severely affects
// performance.
#define TIG_INITIALIZE_FPS 0x0001u

// Use intermediary buffer when rendering windows with color key.
#define TIG_INITIALIZE_SCRATCH_BUFFER 0x0010u

// Use windowed mode (otherwise fullscreen).
#define TIG_INITIALIZE_WINDOWED 0x0020u

// Respect settings provided in `TigInitInfo::x` and `TigInitInfo::y`
// (otherwise the window is centered in the screen).
#define TIG_INITIALIZE_POSITIONED 0x0100u

// Completely disables sound system.
#define TIG_INITIALIZE_NO_SOUND 0x2000u

// Use `TigInitInfo::window_name` to set window name (otherwise it's set to
// the executable name).
#define TIG_INITIALIZE_SET_WINDOW_NAME 0x4000u

typedef int(TigArtFilePathResolver)(tig_art_id_t art_id, char* path);
typedef tig_art_id_t(TigArtIdResetFunc)(tig_art_id_t art_id);
typedef int(TigSoundFilePathResolver)(int sound_id, char* path);

typedef struct TigInitInfo {
    TigInitFlags flags;
    int x;
    int y;
    int width;
    int height;
    int bpp;
    TigArtFilePathResolver* art_file_path_resolver;
    TigArtIdResetFunc* art_id_reset_func;
    TigSoundFilePathResolver* sound_file_path_resolver;
    const char* mss_redist_path;
    unsigned int texture_width;
    unsigned int texture_height;
    const char* window_name;
} TigInitInfo;

#ifdef __cplusplus
}
#endif

#endif /* TIG_TYPES_H_ */
