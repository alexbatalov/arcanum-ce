#ifndef TIG_SOUND_H_
#define TIG_SOUND_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int tig_sound_handle_t;

#define TIG_SOUND_HANDLE_INVALID ((tig_sound_handle_t)(-1))

typedef enum TigSoundType {
    TIG_SOUND_TYPE_EFFECT,
    TIG_SOUND_TYPE_MUSIC,
    TIG_SOUND_TYPE_VOICE,
    TIG_SOUND_TYPE_COUNT,
} TigSoundType;

// Represents the size of positional sound.
typedef enum TigSoundPositionalSize {
    TIG_SOUND_SIZE_SMALL,
    TIG_SOUND_SIZE_MEDIUM,
    TIG_SOUND_SIZE_LARGE,
    TIG_SOUND_SIZE_EXTRA_LARGE,
    TIG_SOUND_SIZE_COUNT,
} TigSoundPositionalSize;

// Signature of a callback used by `tig_sound_enumerate_positional`.
typedef void(TigSoundEnumerateFunc)(tig_sound_handle_t sound_handle);

// Initializes the sound subsystem.
int tig_sound_init(TigInitInfo* init_info);

// Shuts down the sound subsystem.
void tig_sound_exit();

// Handle sound events.
void tig_sound_ping();

// Sets a callback function to resolve sound ID to a file path.
void tig_sound_set_file_path_resolver(TigSoundFilePathResolver* func);

// Returns `true` if SOUND subsystem is initialized.
bool tig_sound_is_initialized();

// Stops a specified sound.
//
// Use `fade_duration` to gracefully fade out volume to 0.
//
// In either case (with fading or immediately) the sound handle is released and
// should not be used again.
void tig_sound_stop(tig_sound_handle_t sound_handle, int fade_duration);

// Stops all active sounds.
//
// Use `fade_duration` to gracefully fade out volume to 0.
//
// In either case (with fading or immediately) all sound handles will be
// released and should not be used again.
void tig_sound_stop_all(int fade_duration);

// Creates a new sound of a given type.
//
// NOTE: For unknown reason this function returns `TIG_OK` even if sound handle
// was not allocated (probably because sound system is out of free handles).
int tig_sound_create(tig_sound_handle_t* sound_handle, TigSoundType type);

// Destroys a specified sound and release all associated resources.
void tig_sound_destroy(tig_sound_handle_t sound_handle);

// Plays a sound effect from a specified path.
//
// The sound is loaded into memory at once and cached, thus suitable for sound
// effects.
int tig_sound_play(tig_sound_handle_t sound_handle, const char* path, int id);

// Plays a sound effect with a given ID.
//
// This function is a shortcut to `tig_sound_play` which resolves path using the
// callback specified by `tig_sound_set_file_path_resolver`.
int tig_sound_play_id(tig_sound_handle_t sound_handle, int id);

// Plays streaming sound from a specified path forever.
//
// This function is intended for playing music. You're responsible for stopping
// it or switching tracks.
int tig_sound_play_streamed_indefinitely(tig_sound_handle_t sound_handle, const char* name, int fade_duration, tig_sound_handle_t prev_sound_handle);

// Plays streaming sound from a specified path once.
//
// This function is intended for playing music or speech sounds.
int tig_sound_play_streamed_once(tig_sound_handle_t sound_handle, const char* name, int fade_duration, tig_sound_handle_t prev_sound_handle);

// Returns the number of loops of a specified sound.
//
// The returned value is not "remaining number of loops", but the number of
// loops the sound was configured with in the first place.
//
// NOTE: The returned value is 1 if SOUND subsystem is not initialized. This 1
// likely means `TIG_ERR_NOT_INITIALIZED` error code.
int tig_sound_get_loops(tig_sound_handle_t sound_handle);

// Sets the number of loops of a specified sound.
//
// This function has to be called before sound is started. It has no effect on
// already started sound. It should only be used to configure looping of sound
// effects, because streaming audio is intended to played either once or
// indefinitely.
void tig_sound_set_loops(tig_sound_handle_t sound_handle, int loops);

// Returns the current volume of a specified sound.
int tig_sound_get_volume(tig_sound_handle_t sound_handle);

// Sets the volume of a specified sound.
void tig_sound_set_volume(tig_sound_handle_t sound_handle, int volume);

// Sets the volume of all sounds of a given type.
void tig_sound_set_volume_by_type(TigSoundType type, int volume);

// Returns the type of a specified sound.
TigSoundType tig_sound_get_type(tig_sound_handle_t sound_handle);

// Sets the type of a specified sound.
void tig_sound_set_type(tig_sound_handle_t sound_handle, TigSoundType type);

// Returns the size of a specified sound.
TigSoundPositionalSize tig_sound_get_positional_size(tig_sound_handle_t sound_handle);

// Sets the size of a specified sound.
//
// The sound system does not use this value in any way. You are responsible to
// add meaning to sound size when processing positional sounds.
void tig_sound_set_positional_size(tig_sound_handle_t sound_handle, TigSoundPositionalSize size);

// Returns the extra volume of a specified sound.
int tig_sound_get_extra_volume(tig_sound_handle_t sound_handle);

// Sets the extra volume of a specified sound.
//
// This function has no effect on streamed sounds.
void tig_sound_set_extra_volume(tig_sound_handle_t sound_handle, int extra_volume);

// Checks if a sound effect with a given ID is currently playing.
bool tig_sound_is_playing_id(int id);

// Checks if a sound is currently playing.
bool tig_sound_is_playing(tig_sound_handle_t sound_handle);

// Checks if a sound is active.
bool tig_sound_is_active(tig_sound_handle_t sound_handle);

// Sets the coordinates of a specified sound.
//
// The sound system itself does not define own coordinate system and have no
// notion of listener, so these coordinates have no meaning to sound system.
//
// When listener's position changes you're reponsible to enumerate positional
// sounds with `tig_sound_enumerate_positional`, obtain these coordinates with
// `tig_sound_get_position`, calculate distance between the two and finally
// change volume with `tig_sound_set_volume` to simulate effects of distance.
void tig_sound_set_position(tig_sound_handle_t sound_handle, int64_t x, int64_t y);

// Returns the coordinates of a specified sound.
void tig_sound_get_position(tig_sound_handle_t sound_handle, int64_t* x, int64_t* y);

// Checks if a sound is positional.
bool tig_sound_is_positional(tig_sound_handle_t sound_handle);

// Executes a given callback for each active positional sound.
void tig_sound_enumerate_positional(TigSoundEnumerateFunc* func);

// Sets volume for sound effects played with `tig_sound_quick_play`.
void tig_sound_quick_play_set_volume(int volume);

// Hassle-free shortcut for playing sound effects.
void tig_sound_quick_play(int id);

void tig_sound_set_active(bool is_active);

#ifdef __cplusplus
}
#endif

#endif /* TIG_SOUND_H_ */
