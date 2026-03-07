#include "game/gsound.h"

#include <math.h>
#include <stdio.h>

#include "game/gamelib.h"
#include "game/hrp.h"
#include "game/location.h"
#include "game/mes.h"
#include "game/player.h"
#include "game/random.h"
#include "game/timeevent.h"

/**
 * Number of concurrent sound scheme slots.
 */
#define GSOUND_SCHEME_SLOTS 2

/**
 * Maximum number of sound entries in a single scheme.
 */
#define GSOUND_SCHEME_MAX_SOUNDS 25

/**
 * Maximum number of consecutive `SchemeList.mes` entries examined when loading
 * a sound scheme.
 */
#define GSOUND_SCHEME_MAX_ENTRIES 100

/**
 * Maximum raw volume level used by the TIG sound system.
 */
#define GSOUND_VOLUME_MAX 127

/**
 * Raw stereo balance value for a perfectly centred sound.
 */
#define GSOUND_BALANCE_CENTER 64

/**
 * Fade-out time (in milliseconds) applied when transitioning between sound
 * schemes or stopping looping songs.
 */
#define GSOUND_SCHEME_FADE_MS 25

/**
 * Minimum time (in milliseconds) between `gsound_scheme_update` calls.
 */
#define GSOUND_PING_INTERVAL_MS 250

/**
 * Number of combat tension music files available.
 */
#define GSOUND_COMBAT_MUSIC_TRACKS 6

/**
 * Y-axis scale factor applied when computing the isometric distance between
 * a sound source and the listener.
 *
 * In isometric view, a one-unit world-Y step spans twice the screen-space
 * distance of the equivalent world-X step, so Y deltas are multiplied by this
 * value before squaring.
 */
#define GSOUND_ISOMETRIC_Y_SCALE 2

/**
 * Describes one sound entry within a sound scheme.
 *
 * Entries fall into two broad categories based on the `song` flag:
 *
 *  - Songs: a looping song plays indefinitely while the current in-game hour
 *    falls within specified time range. A transition song plays exactly once,
 *    when it finishes the scheme that was playing before is automatically
 *    restored.
 *
 *  - Ambients: short SFX clips fired randomly by `gsound_scheme_update` based
 *    on it's `frequency`, the active hour window, and randomized volume/balance
 *    ranges.
 */
typedef struct Sound {
    /* 0000 */ bool song;
    /* 0001 */ bool loop;
    /* 0002 */ bool is_transition;
    /* 0004 */ int frequency;
    /* 0008 */ int time_start;
    /* 000C */ int time_end;
    /* 0010 */ int balance_left;
    /* 0014 */ int balance_right;
    /* 0018 */ int volume_min;
    /* 001C */ int volume_max;
    /* 0020 */ tig_sound_handle_t sound_handle;
    /* 0024 */ char path[TIG_MAX_PATH];
} Sound;

/**
 * Describes a sound scheme.
 */
typedef struct SoundScheme {
    /* 0000 */ int scheme_num;
    /* 0004 */ int scheme_idx;
    /* 0008 */ int count;
    /* 000C */ Sound sounds[GSOUND_SCHEME_MAX_SOUNDS];
} SoundScheme;

static const char* gsound_build_sound_path(const char* name);
static void gsound_scheme_reset(SoundScheme* scheme);
static tig_sound_handle_t gsound_play_sfx_func(const char* path, int loops, int volume, int extra_volume, int id);
static void recalc_positional_sounds_volume();
static void recalc_positional_sound_volume(tig_sound_handle_t sound_handle);
static void gsound_calc_positional_params(int64_t x, int64_t y, int* volume_ptr, int* extra_volume_ptr, TigSoundPositionalSize size);
static tig_sound_handle_t gsound_play_sfx_ex(int id, int loops, int volume, int extra_volume);
static tig_sound_handle_t gsound_play_sfx_at_xy_ex(int id, int loops, int64_t x, int64_t y, int size);
static tig_sound_handle_t gsound_play_sfx_at_loc_ex(int id, int loops, int64_t location, int size);
static void gsound_scheme_stop(int fade_duration, int index);
static void gsound_scheme_stop_all(int fade_duration);
static void gsound_update();
static void gsound_scheme_path(const char* name, char* buffer);
static bool gsound_scheme_is_active(int scheme_idx, int* index_ptr);
static void gsound_scheme_load(int scheme_idx);
static Sound* gsound_scheme_parse_entry(SoundScheme* scheme, const char* path);
static void gsound_normalize_path(char* str);
static void gsound_scheme_parse_param(const char* str, const char* key, int* value1, int* value2);
static void set_listener_xy(int64_t x, int64_t y);
static void gsound_set_defaults(int min_radius, int max_radius, int min_pan_distance, int max_pan_distance);
static int gsound_effects_volume_get();
static void gsound_effects_volume_changed();
static int gsound_voice_volume_get();
static void gsound_voice_volume_changed();
static int gsound_music_volume_get();
static void gsound_music_volume_changed();
static int adjust_volume(tig_sound_handle_t sound_handle, int volume);

/**
 * 0x5A0F38
 */
static const char* gsound_base_sound_path = "sound\\";

/**
 * Raw voice volume.
 *
 * 0x5D1A18
 */
static int gsound_voice_volume;

/**
 * Sounds at this X-range and above are played hard to that side (fully panned).
 *
 * 0x5D1A20
 */
static int64_t sound_maximum_pan_distance;

/**
 * World-space location that defines the coordinate origin for positional
 * sounds.
 *
 * This value is set on the first call to `gsound_play_sfx_at_loc_ex` and
 * is updated when local PC location changes.
 *
 * 0x5D1A28
 */
static int64_t gsound_origin_loc;

/**
 * Scheme indicesthat were active when combat music started. Restored by
 * `gsound_stop_combat_music` once combat ends.
 *
 * 0x5D1A30
 */
static int gsound_pre_combat_scheme_idxs[GSOUND_SCHEME_SLOTS];

/**
 * Scheme indices to restore once the current transition (one-shot) song
 * finishes playing.
 *
 * 0x5D1A38
 */
static int gsound_post_song_scheme_idxs[GSOUND_SCHEME_SLOTS];

/**
 * "SchemeList.mes"
 *
 * 0x5D1A40
 */
static mes_file_handle_t gsound_scheme_list_mes_file;

/**
 * Raw music volume.
 *
 * 0x5D1A44
 */
static int gsound_music_volume;

/**
 * Combat music handle.
 *
 * 0x5D1A48
 */
static tig_sound_handle_t gsound_combat_music_handle;

/**
 * Short sound effect played as a transition between generic music and combat
 * music.
 *
 * 0x5D1A4C
 */
static tig_sound_handle_t gsound_combat_tension_handle;

/**
 * X-axis coordinate of the listener's position in the normalized coordinate
 * space (relative to the origin).
 *
 * 0x5D1A50
 */
static int64_t gsound_listener_x;

/**
 * Y-axis coordinate of the listener's position in the normalized coordinate
 * space (relative to the origin).
 *
 * 0x5D1A58
 */
static int64_t gsound_listener_y;

/**
 * Dynamic array of message SFX message files.
 *
 * 0x5D1A68
 */
static mes_file_handle_t* gsound_sfx_mes_files;

/**
 * Sounds closer than this are played centered.
 *
 * 0x5D1A60
 */
static int64_t sound_minimum_pan_distance;

/**
 * Flag indicating whether the gsound module has been initialized.
 *
 * 0x5D1A6C
 */
static bool gsound_initialized;

/**
 * Lock counter.
 *
 * The value of `0` means there is no lock. The value is incremented each time
 * `gsound_lock` is called, and decremented on `gsound_unlock`.
 *
 * 0x5D1A70
 */
static int gsound_lock_cnt;

/**
 * Maps sound size class to minimum radius.
 *
 * 0x5D1A78
 */
static int64_t sound_minimum_radius[TIG_SOUND_SIZE_COUNT];

/**
 * Sound schemes.
 *
 * 0x5D1A98
 */
static SoundScheme gsound_active_schemes[GSOUND_SCHEME_SLOTS];

/**
 * "SchemeIndex.mes"
 *
 * 0x5D5480
 */
static mes_file_handle_t gsound_scheme_index_mes_file;

/**
 * Scheme indices saved when `gsound_lock` is first called. Restored by
 * `gsound_unlock` once the lock counter reaches zero.
 *
 * 0x5D5484
 */
static int gsound_pre_lock_scheme_idxs[GSOUND_SCHEME_SLOTS];

/**
 * Flag indicating that transition music is currently playing.
 *
 * NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
 * compatibility.
 *
 * 0x5D5594
 */
static int gsound_pending_scheme_restore;

/**
 * "snd_user.mes"
 *
 * 0x5D5598
 */
static mes_file_handle_t gsound_snd_user_mes_file;

/**
 * Raw sound effects volume.
 *
 * 0x5D559C
 */
static int gsound_effects_volume;

/**
 * Maps sound size class to maximum volume.
 *
 * 0x5D55A0
 */
static int sound_maximum_volume[TIG_SOUND_SIZE_COUNT];

/**
 * The number of entries in `gsound_sfx_mes_files` array.
 *
 * 0x5D55B0
 */
static int gsound_sfx_mes_files_count;

/**
 * Maps sound size class to maximum radius.
 *
 * 0x5D55B8
 */
static int64_t sound_maximum_radius[TIG_SOUND_SIZE_COUNT];

/**
 * Flag indicating that combat music is currently playing.
 *
 * While active, scheme changes requested via `gsound_play_scheme` are deferred
 * into `gsound_pre_combat_scheme_idxs` instead of being applied immediately.
 *
 * NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
 * compatibility.
 *
 * 0x5D55D8
 */
static int gsound_combat_music_active;

/**
 * Cached Y-axis coordinate of gsound_origin_location.
 *
 * 0x5D55E0
 */
static int64_t gsound_origin_y;

/**
 * Cached X-axis coordinate of gsound_origin_location.
 *
 * 0x5D55E8
 */
static int64_t gsound_origin_x;

/**
 * Resolves a numeric sound ID to its filesystem path.
 *
 * 0x41A940
 */
int gsound_resolve_path(int sound_id, char* path)
{
    MesFileEntry mes_file_entry;
    int index;

    if (gsound_initialized) {
        mes_file_entry.num = sound_id;

        // Search the base SFX message files in load order.
        for (index = 0; index < gsound_sfx_mes_files_count; index++) {
            if (mes_search(gsound_sfx_mes_files[index], &mes_file_entry)) {
                sprintf(path, "%s%s", gsound_base_sound_path, mes_file_entry.str);
                return TIG_OK;
            }
        }

        // Fallback to the module-specific file.
        if (gsound_snd_user_mes_file != MES_FILE_HANDLE_INVALID) {
            if (mes_search(gsound_snd_user_mes_file, &mes_file_entry)) {
                sprintf(path, "%s%s", gsound_base_sound_path, mes_file_entry.str);
                return TIG_OK;
            }
        }
    }

    path[0] = '\0';
    return TIG_ERR_GENERIC;
}

/**
 * Called when the game is initialized.
 *
 * 0x41AA30
 */
bool gsound_init(GameInitInfo* init_info)
{
    mes_file_handle_t tmp_mes_file;
    MesFileEntry mes_file_entry;
    int index;
    int positional_sound_params;
    int x;
    int y;

    (void)init_info;

    // Load the index file that lists all SFX message file names.
    mes_load(gsound_build_sound_path("snd_00index.mes"), &tmp_mes_file);
    gsound_sfx_mes_files_count = mes_num_entries(tmp_mes_file);
    if (gsound_sfx_mes_files_count == 0) {
        return false;
    }

    // Allocate the array and load each named SFX message file.
    gsound_sfx_mes_files = (mes_file_handle_t*)CALLOC(gsound_sfx_mes_files_count, sizeof(*gsound_sfx_mes_files));

    mes_file_entry.num = 0;
    for (index = 0; index < gsound_sfx_mes_files_count; index++) {
        gsound_sfx_mes_files[index] = MES_FILE_HANDLE_INVALID;
        mes_find_next(tmp_mes_file, &mes_file_entry);
        mes_load(gsound_build_sound_path(mes_file_entry.str), &(gsound_sfx_mes_files[index]));
    }

    mes_unload(tmp_mes_file);

    // Load scheme configuration files used by background music and ambient
    // audio.
    mes_load(gsound_build_sound_path("SchemeIndex.mes"), &gsound_scheme_index_mes_file);
    mes_load(gsound_build_sound_path("SchemeList.mes"), &gsound_scheme_list_mes_file);

    // Initialize both scheme slots to the empty state.
    for (index = 0; index < 2; index++) {
        gsound_scheme_reset(&(gsound_active_schemes[index]));
    }

    gsound_initialized = true;

    // Place the listener at the center of the viewport.
    x = 400;
    y = 300;
    hrp_center(&x, &y);
    set_listener_xy(x, y);

    // TODO: Figure out if these needs to be adjusted for HRP.
    gsound_set_defaults(150, 800, 150, 400);

    // Large sounds default to full volume before `soundparams.mes` is applied.
    sound_maximum_volume[TIG_SOUND_SIZE_LARGE] = 100;

    // Load per-size positional audio parameters. Each size has a min radius
    // (below which full volume plays), a max radius (beyond which the sound is
    // inaudible), and optionally a max volume cap.
    mes_load(gsound_build_sound_path("soundparams.mes"), &tmp_mes_file);

    if (tmp_mes_file != MES_FILE_HANDLE_INVALID) {
        positional_sound_params = 0;

        mes_file_entry.num = 1;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_radius[TIG_SOUND_SIZE_LARGE] = atoi(mes_file_entry.str);
            positional_sound_params++;
        }

        mes_file_entry.num = 2;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_radius[TIG_SOUND_SIZE_LARGE] = atoi(mes_file_entry.str);
            positional_sound_params++;
        }

        mes_file_entry.num = 3;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_pan_distance = atoi(mes_file_entry.str);
            positional_sound_params++;
        }

        mes_file_entry.num = 4;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_pan_distance = atoi(mes_file_entry.str);
            positional_sound_params++;
        }

        // Small, minimum radius.
        mes_file_entry.num = 10;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_radius[TIG_SOUND_SIZE_SMALL] = atoi(mes_file_entry.str);
        } else {
            sound_minimum_radius[TIG_SOUND_SIZE_SMALL] = 50;
        }

        // Small, maximum radius.
        mes_file_entry.num = 11;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_radius[TIG_SOUND_SIZE_SMALL] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_radius[TIG_SOUND_SIZE_SMALL] = 150;
        }

        // Small, maximum volume.
        mes_file_entry.num = 12;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = 40;
        }

        // Medium, minimum radius.
        mes_file_entry.num = 20;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_radius[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_minimum_radius[TIG_SOUND_SIZE_MEDIUM] = 50;
        }

        // Medium, maximum radius.
        mes_file_entry.num = 21;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_radius[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_radius[TIG_SOUND_SIZE_MEDIUM] = 400;
        }

        // Medium, maximum volume.
        mes_file_entry.num = 22;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = 70;
        }

        // Extra Large, minimum radius.
        mes_file_entry.num = 30;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = atoi(mes_file_entry.str);
        } else {
            sound_minimum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = 50;
        }

        // Extra Large, maximum radius.
        mes_file_entry.num = 31;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = 1500;
        }

        // Extra Large, maximum volume.
        mes_file_entry.num = 32;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_volume[TIG_SOUND_SIZE_EXTRA_LARGE] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_volume[TIG_SOUND_SIZE_EXTRA_LARGE] = 100;
        }

        // Only recalculate positional volumes if at least one of the LARGE
        // radius/pan parameters was overridden. Other sizes already have
        // their own defaults set above.
        if (positional_sound_params != 0) {
            recalc_positional_sounds_volume();
        }

        mes_unload(tmp_mes_file);
    }

    // Register sound-type volume settings and initialize them.
    settings_register(&settings, EFFECTS_VOLUME_KEY, "5", gsound_effects_volume_changed);
    settings_register(&settings, VOICE_VOLUME_KEY, "5", gsound_voice_volume_changed);
    settings_register(&settings, MUSIC_VOLUME_KEY, "5", gsound_music_volume_changed);
    gsound_effects_volume_changed();
    gsound_voice_volume_changed();
    gsound_music_volume_changed();
    gsound_combat_music_active = false;
    gsound_lock_cnt = 0;

    return true;
}

/**
 * Builds a full path to a named sound asset by prepending base sound path.
 *
 * Returns a pointer to an internal static buffer.
 *
 * 0x41AF80
 */
const char* gsound_build_sound_path(const char* name)
{
    // 0x5D5490
    static char path[TIG_MAX_PATH];

    sprintf(path, "%s%s", gsound_base_sound_path, name);

    return path;
}

/**
 * Resets a scheme slot to the empty/inactive state.
 *
 * 0x41AFB0
 */
void gsound_scheme_reset(SoundScheme* scheme)
{
    int index;

    scheme->scheme_num = 0;
    scheme->count = 0;

    for (index = 0; index < GSOUND_SCHEME_MAX_SOUNDS; index++) {
        memset(&(scheme->sounds[index]), 0, sizeof(*scheme->sounds));
        scheme->sounds[index].sound_handle = TIG_SOUND_HANDLE_INVALID;
    }
}

/**
 * Called when the game shuts down.
 *
 * 0x41AFF0
 */
void gsound_exit()
{
    int index;

    gsound_initialized = false;

    for (index = 0; index < gsound_sfx_mes_files_count; index++) {
        mes_unload(gsound_sfx_mes_files[index]);
    }

    FREE(gsound_sfx_mes_files);
    gsound_sfx_mes_files = NULL;

    gsound_sfx_mes_files_count = 0;

    mes_unload(gsound_scheme_index_mes_file);
    mes_unload(gsound_scheme_list_mes_file);
}

/**
 * Called when the game is being reset.
 *
 * 0x41B060
 */
void gsound_reset()
{
    gsound_origin_loc = 0;
    gsound_origin_x = 0;
    gsound_origin_y = 0;
    gsound_combat_music_active = false;
    gsound_lock_cnt = 0;
    gsound_stop_all(0);
}

/**
 * Called when a module is being loaded.
 *
 * 0x41B0A0
 */
bool gsound_mod_load()
{
    mes_load(gsound_build_sound_path("snd_user.mes"), &gsound_snd_user_mes_file);
    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x41B0D0
 */
void gsound_mod_unload()
{
    mes_unload(gsound_snd_user_mes_file);
    gsound_snd_user_mes_file = MES_FILE_HANDLE_INVALID;
}

/**
 * Called when the game is being loaded.
 *
 * 0x41B0F0
 */
bool gsound_load(GameLoadInfo* load_info)
{
    int temp[GSOUND_SCHEME_SLOTS];
    int index;

    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        if (tig_file_fread(&(temp[index]), sizeof(*temp), 1, load_info->stream) != 1) {
            return false;
        }
    }

    if (tig_file_fread(&gsound_pending_scheme_restore, sizeof(gsound_pending_scheme_restore), 1, load_info->stream) != 1) {
        return false;
    }

    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        if (tig_file_fread(&(gsound_post_song_scheme_idxs[index]), sizeof(*gsound_post_song_scheme_idxs), 1, load_info->stream) != 1) {
            return false;
        }
    }

    if (tig_file_fread(&gsound_combat_music_active, sizeof(gsound_combat_music_active), 1, load_info->stream) != 1) {
        return false;
    }

    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        if (tig_file_fread(&(gsound_pre_combat_scheme_idxs[index]), sizeof(*gsound_pre_combat_scheme_idxs), 1, load_info->stream) != 1) {
            return false;
        }
    }

    gsound_play_scheme(temp[0], temp[1]);

    return true;
}

/**
 * Called when the game is being saved.
 *
 * 0x41B1D0
 */
bool gsound_save(TigFile* stream)
{
    int index;
    int scheme_idx;

    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        scheme_idx = gsound_active_schemes[index].scheme_num != 0 ? gsound_active_schemes[index].scheme_idx : 0;
        if (tig_file_fwrite(&scheme_idx, sizeof(scheme_idx), 1, stream) != 1) {
            return false;
        }
    }

    if (tig_file_fwrite(&gsound_pending_scheme_restore, sizeof(gsound_pending_scheme_restore), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        if (tig_file_fwrite(&(gsound_post_song_scheme_idxs[index]), sizeof(*gsound_post_song_scheme_idxs), 1, stream) != 1) {
            return false;
        }
    }

    if (tig_file_fwrite(&gsound_combat_music_active, sizeof(gsound_combat_music_active), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        if (tig_file_fwrite(&(gsound_pre_combat_scheme_idxs[index]), sizeof(*gsound_pre_combat_scheme_idxs), 1, stream) != 1) {
            return false;
        }
    }

    return true;
}

/**
 * Called every frame.
 *
 * 0x41B2A0
 */
void gsound_ping(tig_timestamp_t timestamp)
{
    // 0x5D548C
    static tig_timestamp_t prev;

    if (!gsound_initialized) {
        return;
    }

    // Throttle updates.
    if (timestamp - prev > GSOUND_PING_INTERVAL_MS) {
        prev = timestamp;
        gsound_update();
    }
}

/**
 * Called when a map is being closed.
 *
 * 0x41B2D0
 */
void gsound_flush()
{
    gsound_reset();
}

/**
 * Creates and starts an SFX sound from a specified file path.
 *
 * 0x41B2E0
 */
tig_sound_handle_t gsound_play_sfx_func(const char* path, int loops, int volume, int extra_volume, int id)
{
    tig_sound_handle_t sound_handle;

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    if (gsound_lock_cnt) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    tig_sound_create(&sound_handle, TIG_SOUND_TYPE_EFFECT);
    tig_sound_set_loops(sound_handle, loops);
    tig_sound_set_volume(sound_handle, volume * gsound_effects_volume / 100);
    tig_sound_set_extra_volume(sound_handle, extra_volume);
    tig_sound_play(sound_handle, path, id);
    if (!tig_sound_is_active(sound_handle)) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    return sound_handle;
}

/**
 * Recalculates volume and balance for all currently active positional sounds.
 *
 * Called after the listener position or the positional audio parameters
 * (radii, pan distances) change.
 *
 * 0x41B3A0
 */
void recalc_positional_sounds_volume()
{
    tig_sound_enumerate_positional(recalc_positional_sound_volume);
}

/**
 * Recalculates volume and balance for a single positional sound.
 *
 * 0x41B3B0
 */
void recalc_positional_sound_volume(tig_sound_handle_t sound_handle)
{
    int64_t x;
    int64_t y;
    TigSoundPositionalSize size;
    int volume;
    int extra_volume;

    tig_sound_get_position(sound_handle, &x, &y);
    size = tig_sound_get_positional_size(sound_handle);
    gsound_calc_positional_params(x, y, &volume, &extra_volume, size);
    tig_sound_set_volume(sound_handle, adjust_volume(sound_handle, volume));
    tig_sound_set_extra_volume(sound_handle, extra_volume);
}

/**
 * Computes the raw volume and stereo balance for a sound at the given world
 * position relative to the current listener.
 *
 * Volume attenuation:
 *   - distance < sound_minimum_radius[size]: full volume (`GSOUND_VOLUME_MAX`)
 *   - distance > sound_maximum_radius[size]: silent (0)
 *   - in between: linear interpolation
 *
 * Stereo panning:
 *   - |dx| < sound_minimum_pan_distance: centred (`GSOUND_BALANCE_CENTER`)
 *   - |dx| > sound_maximum_pan_distance: hard left (0) or hard right (`GSOUND_VOLUME_MAX`)
 *   - in between: linear interpolation
 *
 * 0x41B420
 */
void gsound_calc_positional_params(int64_t x, int64_t y, int* volume_ptr, int* extra_volume_ptr, TigSoundPositionalSize size)
{
    int extra_volume = GSOUND_BALANCE_CENTER;
    int volume = GSOUND_VOLUME_MAX;
    int64_t distance;
    int64_t dx;
    int64_t dy;

    // Compute isometric distance. Y deltas are doubled to account for the
    // isometric projection mapping.
    dx = x - gsound_listener_x;
    dy = GSOUND_ISOMETRIC_Y_SCALE * (y - gsound_listener_y);
    distance = (int64_t)sqrt((double)(dx * dx + dy * dy));

    // Absolute horizontal offset used for stereo panning.
    dx = x - gsound_listener_x;
    if (dx < 0) {
        dx = gsound_listener_x - x;
    }

    // Volume attenuation based on distance.
    if (distance < sound_minimum_radius[size]) {
        // Within the inner radius: full volume, no attenuation.
        volume = GSOUND_VOLUME_MAX;
    } else if (distance > sound_maximum_radius[size]) {
        // Beyond the outer radius: completely inaudible.
        volume = 0;
    } else if (sound_maximum_radius[size] > sound_minimum_radius[size]) {
        // Linear fade between the inner and outer radii.
        volume = (int)(GSOUND_VOLUME_MAX * (sound_minimum_radius[size] - distance) / (sound_maximum_radius[size] - sound_minimum_radius[size])) + GSOUND_VOLUME_MAX;
        if (volume < 0) {
            volume = 0;
        } else if (volume > GSOUND_VOLUME_MAX) {
            volume = GSOUND_VOLUME_MAX;
        }
        extra_volume = GSOUND_BALANCE_CENTER;
    }

    // Stereo panning: shift balance leftward when source is left of the
    // listener.
    if (x < gsound_listener_x) {
        if (dx > sound_maximum_pan_distance) {
            // Beyond maximum distance: fully panned left.
            extra_volume = 0;
        } else if (dx > sound_minimum_pan_distance) {
            // Linear shift depending on the distance.
            extra_volume = (int)(GSOUND_BALANCE_CENTER * (sound_minimum_pan_distance - dx) / (sound_maximum_pan_distance - sound_minimum_pan_distance)) + GSOUND_BALANCE_CENTER;
            if (extra_volume < 0) {
                extra_volume = 0;
            } else if (extra_volume > GSOUND_VOLUME_MAX) {
                extra_volume = GSOUND_VOLUME_MAX;
            }
        } else {
            // Within the minimum distance: center is already the default.
        }
    }

    // Stereo panning: shift balance rightward when source is right of the
    // listener.
    if (x > gsound_listener_x) {
        if (dx > sound_maximum_pan_distance) {
            // Beyond maxium distance: fully panned right.
            extra_volume = GSOUND_VOLUME_MAX;
        } else if (dx > sound_minimum_pan_distance) {
            // Linear shift depending on the distance.
            extra_volume = (int)(GSOUND_BALANCE_CENTER * (dx - sound_minimum_pan_distance) / (sound_maximum_pan_distance - sound_minimum_pan_distance)) + GSOUND_BALANCE_CENTER;
            if (extra_volume < 0) {
                extra_volume = 0;
            } else if (extra_volume > GSOUND_VOLUME_MAX) {
                extra_volume = GSOUND_VOLUME_MAX;
            }
        } else {
            // Within the minimum distance: center is already the default.
        }
    }

    if (volume_ptr != NULL) {
        *volume_ptr = volume;
    }

    if (extra_volume_ptr != NULL) {
        *extra_volume_ptr = extra_volume;
    }
}

/**
 * Plays an SFX identified by `id` with explicit volume and balance.
 *
 * 0x41B720
 */
tig_sound_handle_t gsound_play_sfx_ex(int id, int loops, int volume, int extra_volume)
{
    char path[TIG_MAX_PATH];

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    gsound_resolve_path(id, path);
    return gsound_play_sfx_func(path, loops, volume, extra_volume, id);
}

/**
 * Plays an SFX at full volume and centred stereo balance.
 *
 * 0x41B780
 */
tig_sound_handle_t gsound_play_sfx(int id, int loops)
{
    return gsound_play_sfx_ex(id, loops, GSOUND_VOLUME_MAX, GSOUND_BALANCE_CENTER);
}

/**
 * Plays an SFX positionally at screen-space coordinates using the default
 * LARGE size for distance attenuation.
 *
 * 0x41B7A0
 */
tig_sound_handle_t gsound_play_sfx_at_xy(int id, int loops, int64_t x, int64_t y)
{
    return gsound_play_sfx_at_xy_ex(id, loops, x, y, TIG_SOUND_SIZE_LARGE);
}

/**
 * Plays an SFX positionally at screen-space coordinates with an explicit
 * positional size for distance attenuation.
 *
 * 0x41B7D0
 */
tig_sound_handle_t gsound_play_sfx_at_xy_ex(int id, int loops, int64_t x, int64_t y, int size)
{
    int volume;
    int extra_volume;
    tig_sound_handle_t sound_handle;

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    gsound_calc_positional_params(x, y, &volume, &extra_volume, size);

    sound_handle = gsound_play_sfx_ex(id, loops, volume, extra_volume);
    tig_sound_set_position(sound_handle, x, y);
    tig_sound_set_positional_size(sound_handle, size);

    return sound_handle;
}

/**
 * Plays an SFX positionally at a map location using the default LARGE size.
 *
 * 0x41B850
 */
tig_sound_handle_t gsound_play_sfx_at_loc(int id, int loops, int64_t location)
{
    return gsound_play_sfx_at_loc_ex(id, loops, location, TIG_SOUND_SIZE_LARGE);
}

/**
 * Plays an SFX positionally at a map location with an explicit positional size.
 *
 * If no positional audio origin has been established yet, this location
 * becomes the origin. All subsequent positional coordinates are expressed
 * relative to it.
 *
 * 0x41B870
 */
tig_sound_handle_t gsound_play_sfx_at_loc_ex(int id, int loops, int64_t location, int size)
{
    int64_t x;
    int64_t y;

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    // Establish the coordinate origin the first time a positional sound is played.
    if (gsound_origin_loc == 0) {
        gsound_origin_loc = location;
        location_xy(location, &gsound_origin_x, &gsound_origin_y);
    }

    location_xy(location, &x, &y);

    // Convert to origin-relative coordinates before forwarding to the XY variant.
    return gsound_play_sfx_at_xy_ex(id, loops, x - gsound_origin_x, y - gsound_origin_y, size);
}

/**
 * Plays an SFX positionally at the current location of a game object.
 *
 * 0x41B930
 */
tig_sound_handle_t gsound_play_sfx_on_obj(int id, int loops, int64_t obj)
{
    int64_t location;
    TigSoundPositionalSize size;

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    location = obj_field_int64_get(obj, OBJ_F_LOCATION);
    size = gsound_size(obj);

    return gsound_play_sfx_at_loc_ex(id, loops, location, size);
}

/**
 * Retrieves the appropriate positional sound size category for a game object.
 *
 * Scenery objects may be tagged with small, medium, or extra-large size flags.
 * All other objects (and untagged scenery) default to LARGE.
 *
 * 0x41B980
 */
TigSoundPositionalSize gsound_size(int64_t obj)
{
    unsigned int scenery_flags;

    if (obj_field_int32_get(obj, OBJ_F_TYPE) == OBJ_TYPE_SCENERY) {
        scenery_flags = obj_field_int32_get(obj, OBJ_F_SCENERY_FLAGS);
        if ((scenery_flags & OSCF_SOUND_SMALL) != 0) {
            return TIG_SOUND_SIZE_SMALL;
        }
        if ((scenery_flags & OSCF_SOUND_MEDIUM) != 0) {
            return TIG_SOUND_SIZE_MEDIUM;
        }
        if ((scenery_flags & OSCF_SOUND_EXTRA_LARGE) != 0) {
            return TIG_SOUND_SIZE_EXTRA_LARGE;
        }
    }

    return TIG_SOUND_SIZE_LARGE;
}

/**
 * Stops all looping sounds in one scheme slot and marks the slot as inactive.
 *
 * If no transition song restore is already pending, saves the departing
 * scheme's index into gsound_post_song_scheme_idxs[slot] so it can be
 * resumed after a transition song finishes.
 *
 * 0x41BA20
 */
void gsound_scheme_stop(int fade_duration, int index)
{
    SoundScheme* scheme;
    Sound* sound;
    int snd;

    scheme = &(gsound_active_schemes[index]);
    if (scheme->scheme_num != 0) {
        // Save the departing scheme index for potential restoration after a
        // transition song ends, unless a restore is already queued.
        if (!gsound_pending_scheme_restore) {
            gsound_post_song_scheme_idxs[index] = scheme->scheme_idx;
        }

        // Fade out all looping sounds in this scheme.
        for (snd = 0; snd < GSOUND_SCHEME_MAX_SOUNDS; snd++) {
            sound = &(scheme->sounds[snd]);
            if (sound->loop) {
                if (sound->sound_handle != TIG_SOUND_HANDLE_INVALID) {
                    tig_sound_stop(sound->sound_handle, fade_duration);
                    sound->sound_handle = TIG_SOUND_HANDLE_INVALID;
                }
            }
        }

        // Mark the slot as empty and clear its state.
        scheme->scheme_num = 0;
        gsound_scheme_reset(scheme);
    } else {
        // Slot was already empty. Record a zero restore target.
        if (!gsound_pending_scheme_restore) {
            gsound_post_song_scheme_idxs[index] = 0;
        }
    }
}

/**
 * Calculates the approximate hearing range (in tiles) of a game object.
 *
 * 0x41B9E0
 */
int gsound_range(int64_t obj)
{
    TigSoundPositionalSize size;

    size = gsound_size(obj);
    return (int)sound_maximum_radius[size] / 40;
}

/**
 * Stops all active scheme slots, fading out over `fade_duration` milliseconds.
 *
 * 0x41BAC0
 */
void gsound_scheme_stop_all(int fade_duration)
{
    int index;

    if (gsound_initialized) {
        for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
            gsound_scheme_stop(fade_duration, index);
        }
    }
}

/**
 * Periodic tick for ambient sounds and looping music management.
 *
 * Called at most once per `GSOUND_PING_INTERVAL_MS` by `gsound_ping`.
 *
 * 0x41BAF0
 */
void gsound_update()
{
    int hour;
    int type;
    SoundScheme* scheme;
    int idx;
    Sound* sound;
    char path[TIG_MAX_PATH];
    int volume;
    int extra_volume;

    hour = datetime_current_hour();

    for (type = 0; type < GSOUND_SCHEME_SLOTS; type++) {
        scheme = &(gsound_active_schemes[type]);
        for (idx = 0; idx < scheme->count; idx++) {
            sound = &(scheme->sounds[idx]);
            if (!sound->song) {
                // Ambient SFX: fire randomly when within the active hour
                // window.
                if (hour >= sound->time_start
                    && hour <= sound->time_end
                    && random_between(0, 999) < sound->frequency) {
                    // Build sound path.
                    gsound_scheme_path(sound->path, path);

                    // Pick a random volume within the configured range.
                    if (sound->volume_max > sound->volume_min) {
                        volume = sound->volume_min
                            + random_between(0, sound->volume_max - sound->volume_min - 1);
                    } else {
                        volume = sound->volume_min;
                    }

                    // Pick a random stereo balance within the configured range.
                    if (sound->balance_right > sound->balance_left) {
                        extra_volume = sound->balance_left
                            + random_between(0, sound->balance_right - sound->balance_left - 1);
                    } else {
                        extra_volume = sound->balance_left;
                    }

                    // Fire and forget.
                    gsound_play_sfx_func(path, 1, volume, extra_volume, 0);
                }
            } else if (sound->is_transition) {
                // Transition song: monitor for completion and restore previous
                // scheme.
                if (!tig_sound_is_playing(sound->sound_handle)) {
                    if (gsound_pending_scheme_restore) {
                        gsound_play_scheme(gsound_post_song_scheme_idxs[0], gsound_post_song_scheme_idxs[1]);
                        gsound_pending_scheme_restore = false;
                    }
                }
            } else if (sound->loop) {
                // Looping song: start or stop based on the current hour window.
                if (hour < sound->time_start || hour > sound->time_end) {
                    // Outside the active window - fade out if currently
                    // playing.
                    if (sound->sound_handle != TIG_SOUND_HANDLE_INVALID) {
                        tig_sound_stop(sound->sound_handle, GSOUND_SCHEME_FADE_MS);
                        sound->sound_handle = TIG_SOUND_HANDLE_INVALID;
                    }
                } else {
                    // Inside the active window - start playing if not already.
                    if (sound->sound_handle == TIG_SOUND_HANDLE_INVALID) {
                        gsound_scheme_path(sound->path, path);
                        tig_sound_create(&(sound->sound_handle), TIG_SOUND_TYPE_MUSIC);
                        tig_sound_set_volume(sound->sound_handle, gsound_music_volume * (sound->volume_min) / 100);
                        tig_sound_play_streamed_indefinitely(sound->sound_handle, path, GSOUND_SCHEME_FADE_MS, TIG_SOUND_HANDLE_INVALID);
                    }
                }
            }
        }
    }
}

/**
 * Resolves a scheme sound reference to a filesystem path.
 *
 * 0x41BCD0
 */
void gsound_scheme_path(const char* name, char* buffer)
{
    if (name[0] == '#') {
        gsound_resolve_path(atoi(name + 1), buffer);
    } else {
        sprintf(buffer, "%s%s", gsound_base_sound_path, name);
    }
}

/**
 * Checks whether a scheme is already loaded in one of the active slots.
 *
 * 0x41BD10
 */
bool gsound_scheme_is_active(int scheme_idx, int* index_ptr)
{
    int index;

    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        if (gsound_active_schemes[index].scheme_num != 0) {
            if (gsound_active_schemes[index].scheme_idx == scheme_idx) {
                *index_ptr = index;
                return true;
            }
        }
    }

    return false;
}

/**
 * Starts playing one or both sound schemes.
 *
 * While combat music is active, scheme changes are deferred  rather than
 * applied immediately.
 *
 * 0x41BD50
 */
void gsound_play_scheme(int music_scheme_idx, int ambient_scheme_idx)
{
    bool music_scheme_active;
    int music_scheme_slot;
    bool ambient_scheme_active;
    int ambient_scheme_slot;

    if (!gsound_initialized) {
        return;
    }

    if (gsound_lock_cnt) {
        return;
    }

    // Prevent the same scheme from occupying both slots simultaneously.
    if (music_scheme_idx == ambient_scheme_idx) {
        ambient_scheme_idx = 0;
    }

    if (gsound_combat_music_active) {
        // Defer scheme change, it will be applied when
        // `gsound_stop_combat_music` is called.
        gsound_pre_combat_scheme_idxs[0] = music_scheme_idx;
        gsound_pre_combat_scheme_idxs[1] = ambient_scheme_idx;
    } else {
        music_scheme_active = gsound_scheme_is_active(music_scheme_idx, &music_scheme_slot);
        ambient_scheme_active = gsound_scheme_is_active(ambient_scheme_idx, &ambient_scheme_slot);
        if (!music_scheme_active && !ambient_scheme_active) {
            // Neither scheme is loaded, replace both slots.
            gsound_scheme_stop_all(GSOUND_SCHEME_FADE_MS);
            gsound_scheme_load(music_scheme_idx);
            gsound_scheme_load(ambient_scheme_idx);
        } else if (music_scheme_active && !ambient_scheme_active) {
            // Music scheme is already playing, only replace the other slot.
            gsound_scheme_stop(GSOUND_SCHEME_FADE_MS, 1 - music_scheme_slot);
            gsound_scheme_load(ambient_scheme_idx);
        } else if (!music_scheme_active && ambient_scheme_active) {
            // Ambient scheme is already playing, only replace the other slot.
            gsound_scheme_stop(GSOUND_SCHEME_FADE_MS, 1 - ambient_scheme_slot);
            gsound_scheme_load(music_scheme_idx);
        } else {
            // Both are already active, no changes required.
        }
    }
}

/**
 * Loads a sound scheme into the first available scheme slot.
 *
 * 0x41BE20
 */
void gsound_scheme_load(int scheme_idx)
{
    SoundScheme* scheme;
    int index;
    MesFileEntry mes_file_entry;
    char* hash;
    Sound* sound;
    char path[TIG_MAX_PATH];

    if (scheme_idx == 0) {
        return;
    }

    // Find the first empty scheme slot.
    for (index = 0; index < GSOUND_SCHEME_SLOTS; index++) {
        if (gsound_active_schemes[index].scheme_num == 0) {
            break;
        }
    }

    if (index == GSOUND_SCHEME_SLOTS) {
        // No free slots, silently fail.
        return;
    }

    scheme = &(gsound_active_schemes[index]);
    gsound_scheme_reset(scheme);

    scheme->scheme_idx = scheme_idx;

    // Look up the scheme in `SchemeIndex.mes`.
    mes_file_entry.num = scheme_idx;
    if (!mes_search(gsound_scheme_index_mes_file, &mes_file_entry)) {
        return;
    }

    hash = strchr(mes_file_entry.str, '#');
    if (hash == NULL) {
        return;
    }

    scheme->scheme_num = atoi(hash + 1);
    scheme->count = 0;

    // Parse up to GSOUND_SCHEME_MAX_ENTRIES consecutive `SchemeList.mes`
    // entries.
    for (index = 0; index < GSOUND_SCHEME_MAX_ENTRIES; index++) {
        mes_file_entry.num = scheme->scheme_num + index;
        if (mes_search(gsound_scheme_list_mes_file, &mes_file_entry)) {
            sound = gsound_scheme_parse_entry(scheme, mes_file_entry.str);
            if (sound != NULL) {
                if (sound->song
                    && !sound->loop) {
                    // One-shot songs are started immediately when the scheme
                    // loads.
                    gsound_scheme_path(sound->path, path);
                    tig_sound_create(&(sound->sound_handle), TIG_SOUND_TYPE_MUSIC);
                    tig_sound_set_volume(sound->sound_handle, gsound_music_volume * sound->volume_min / 100);
                    tig_sound_play_streamed_once(sound->sound_handle, path, GSOUND_SCHEME_FADE_MS, TIG_SOUND_HANDLE_INVALID);
                }

                // If this song is a transition, prepare the scheme restore
                // mechanism.
                if (sound->is_transition) {
                    gsound_pending_scheme_restore = true;
                }
            }
        }
    }
}

/**
 * Parses a raw `SchemeList.mes` entry string into a `Sound` and appends it to the
 * given scheme.
 *
 * 0x41BF70
 */
Sound* gsound_scheme_parse_entry(SoundScheme* scheme, const char* path)
{
    Sound* sound = NULL;
    char* copy;
    char* pch;
    int scatter;
    int tmp;

    copy = STRDUP(path);
    if (copy == NULL) {
        // Out of memory.
        return NULL;
    }

    if (scheme->count >= GSOUND_SCHEME_MAX_SOUNDS) {
        // No more slots.
        return NULL;
    }

    // Claim the next sound slot and apply defaults.
    sound = &(scheme->sounds[scheme->count++]);
    memset(sound, 0, sizeof(*sound));
    sound->song = false;
    sound->loop = false;
    sound->is_transition = false;
    sound->balance_left = 50;
    sound->balance_right = 50;
    sound->frequency = 5;
    sound->time_start = 0;
    sound->time_end = 23;
    sound->volume_min = 100;
    sound->volume_max = 100;
    sound->sound_handle = TIG_SOUND_HANDLE_INVALID;
    sound->path[0] = '\0';

    // Normalize to lowercase so option matching is case-insensitive.
    gsound_normalize_path(copy);

    // Find first space - it separates file name from options.
    pch = strchr(copy, ' ');
    if (pch != NULL) {
        *pch++ = '\0';
    }

    strcpy(sound->path, copy);

    if (pch != NULL) {
        // Parse volume range.
        gsound_scheme_parse_param(pch, "/vol:", &(sound->volume_min), &(sound->volume_max));

        // Presence of `/loop`, `/anchor`, or `/over` marks this as a song.
        if (strstr(pch, "/loop") != NULL || strstr(pch, "/anchor") != NULL || strstr(pch, "/over") != NULL) {
            sound->song = true;

            // `/over`: play once and then restore the previous scheme.
            if (strstr(pch, "/over") != NULL) {
                sound->is_transition = true;
            }

            // `/loop`: repeat indefinitely within the active hour window.
            if (strstr(pch, "/loop") != NULL) {
                sound->loop = true;
                gsound_scheme_parse_param(pch, "/time:", &(sound->time_start), &(sound->time_end));
            }
        } else {
            // Ambient SFX entry: parse all per-sound overrides.
            gsound_scheme_parse_param(pch, "/freq:", &(sound->frequency), NULL);
            gsound_scheme_parse_param(pch, "/time:", &(sound->time_start), &(sound->time_end));
            gsound_scheme_parse_param(pch, "/bal:", &(sound->balance_left), &(sound->balance_right));

            scatter = -1;
            tmp = -1;
            gsound_scheme_parse_param(pch, "/scatter:", &scatter, &tmp);

            if (scatter >= 0) {
                sound->volume_max = 100;
                sound->volume_min = 100 - scatter;
                sound->balance_left = 50 - scatter / 2;
                sound->balance_right = 50 + scatter / 2;
            }
        }
    }

    FREE(copy);

    // Ensure min does not exceed max for volume and balance.
    if (sound->volume_max < sound->volume_min) {
        sound->volume_max = sound->volume_min;
    }

    if (sound->balance_right < sound->balance_left) {
        sound->balance_right = sound->balance_left;
    }

    // Convert percentage-based values to the raw 0-GSOUND_VOLUME_MAX range.
    sound->volume_min = GSOUND_VOLUME_MAX * sound->volume_min / 100;
    sound->volume_max = GSOUND_VOLUME_MAX * sound->volume_max / 100;
    sound->balance_left = GSOUND_VOLUME_MAX * sound->balance_left / 100;
    sound->balance_right = GSOUND_VOLUME_MAX * sound->balance_right / 100;

    // Clamp all converted values to the valid raw range.
    if (sound->volume_min < 0) {
        sound->volume_min = 0;
    } else if (sound->volume_min > GSOUND_VOLUME_MAX) {
        sound->volume_min = GSOUND_VOLUME_MAX;
    }

    if (sound->volume_max < 0) {
        sound->volume_max = 0;
    } else if (sound->volume_max > GSOUND_VOLUME_MAX) {
        sound->volume_max = GSOUND_VOLUME_MAX;
    }

    if (sound->balance_left < 0) {
        sound->balance_left = 0;
    } else if (sound->balance_left > GSOUND_VOLUME_MAX) {
        sound->balance_left = 0;
    }

    if (sound->balance_right < 0) {
        sound->balance_right = 0;
    } else if (sound->balance_right > GSOUND_VOLUME_MAX) {
        sound->balance_right = GSOUND_VOLUME_MAX;
    }

    return sound;
}

/**
 * Converts a string to lowercase.
 *
 * 0x41C260
 */
void gsound_normalize_path(char* str)
{
    SDL_strlwr(str);
}

/**
 * Parses a `key=value1[,value2]` parameter from a scheme option string.
 *
 * 0x41C290
 */
void gsound_scheme_parse_param(const char* str, const char* key, int* value1, int* value2)
{
    const char* pch;
    char* copy;
    char* space;
    char* sep;

    pch = strstr(str, key);
    if (pch != NULL) {
        // Extract the value token: from after the key up to the next space.
        copy = STRDUP(pch + strlen(key));
        if (copy != NULL) {
            space = strchr(copy, ' ');
            if (space != NULL) {
                *space = '\0';
            }

            if (value1 != NULL) {
                *value1 = atoi(copy);

                if (value2 != NULL) {
                    // Accept either a comma or a dash as the separator between
                    // the two values.
                    sep = strchr(copy, ',');
                    if (sep == NULL) {
                        sep = strchr(copy, '-');
                    }

                    if (sep != NULL) {
                        *value2 = atoi(sep + 1);
                    } else {
                        // Only one value provided, mirror it to `value2`.
                        *value2 = *value1;
                    }
                }
            }

            FREE(copy);
        }
    }
}

/**
 * Stops all active sounds, cancels the pending scheme restore, and silences
 * the TIG sound layer.
 *
 * 0x41C340
 */
void gsound_stop_all(int fade_duration)
{
    if (gsound_initialized) {
        gsound_scheme_stop_all(fade_duration);
        gsound_pending_scheme_restore = false;
        tig_sound_stop_all(fade_duration);
    }
}

/**
 * Creates and plays a voice-over clip from the given path.
 *
 * Returns `TIG_SOUND_HANDLE_INVALID` if the sound system is locked or the
 * sound fails to start.
 *
 * 0x41C370
 */
tig_sound_handle_t gsound_play_voice(const char* path, int fade_duration)
{
    tig_sound_handle_t sound_handle;

    if (gsound_lock_cnt) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    tig_sound_create(&sound_handle, TIG_SOUND_TYPE_VOICE);
    tig_sound_set_volume(sound_handle, gsound_voice_volume);
    tig_sound_play_streamed_once(sound_handle, path, fade_duration, -1);
    if (!tig_sound_is_active(sound_handle)) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    return sound_handle;
}

/**
 * Creates and plays a music track exactly once from the given path.
 *
 * Returns `TIG_SOUND_HANDLE_INVALID` if the sound system is locked or the
 * sound fails to start.
 *
 * 0x41C3D0
 */
tig_sound_handle_t gsound_play_music_once(const char* path, int fade_duration)
{
    tig_sound_handle_t sound_handle;

    if (gsound_lock_cnt) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    tig_sound_create(&sound_handle, TIG_SOUND_TYPE_MUSIC);
    tig_sound_set_volume(sound_handle, 100 * gsound_music_volume / 100);
    tig_sound_play_streamed_once(sound_handle, path, fade_duration, -1);
    if (!tig_sound_is_active(sound_handle)) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    return sound_handle;
}

/**
 * Creates and plays a music track that loops indefinitely from the given path.
 *
 * Returns `TIG_SOUND_HANDLE_INVALID` if the sound system is locked or the
 * sound fails to start.
 *
 * 0x41C450
 */
tig_sound_handle_t gsound_play_music_indefinitely(const char* path, int fade_duration)
{
    tig_sound_handle_t sound_handle;

    if (gsound_lock_cnt) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    tig_sound_create(&sound_handle, TIG_SOUND_TYPE_MUSIC);
    tig_sound_set_volume(sound_handle, 100 * gsound_music_volume / 100);
    tig_sound_play_streamed_indefinitely(sound_handle, path, fade_duration, -1);
    if (!tig_sound_is_active(sound_handle)) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    return sound_handle;
}

/**
 * Starts combat music for the area triggered by the given NPC.
 *
 * Saves the currently active scheme indices so they can be restored once
 * combat ends, then fades out any playing ambient/music scheme. Plays a
 * randomly selected tension stinger followed by the main looping combat
 * music track.
 *
 * Does nothing if combat music is already active or if `obj` is not an NPC.
 *
 * 0x41C4D0
 */
void gsound_start_combat_music(int64_t obj)
{
    char path[TIG_MAX_PATH];
    int type;

    if (gsound_combat_music_active) {
        return;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_NPC) {
        return;
    }

    // Save active scheme indices so they can be restored after combat.
    for (type = 0; type < GSOUND_SCHEME_SLOTS; type++) {
        if (gsound_active_schemes[type].scheme_num != 0) {
            gsound_pre_combat_scheme_idxs[type] = gsound_active_schemes[type].scheme_idx;
        } else {
            gsound_pre_combat_scheme_idxs[type] = 0;
        }
    }

    // Fade out any currently playing ambient/music scheme.
    gsound_scheme_stop_all(GSOUND_SCHEME_FADE_MS);

    gsound_combat_music_active = true;

    // Play a randomly chosen tension stinger, then loop the main combat track.
    sprintf(path, "sound\\music\\combat %d.mp3", random_between(1, GSOUND_COMBAT_MUSIC_TRACKS));
    gsound_combat_tension_handle = gsound_play_music_once(path, 0);
    gsound_combat_music_handle = gsound_play_music_indefinitely("sound\\music\\combatmusic.mp3", 0);
}

/**
 * Stops combat music and restores the sound scheme that was playing before
 * combat started.
 *
 * Only a player character object can end combat music. Does nothing if combat
 * music is not currently active or if `obj` is NULL.
 *
 * 0x41C5A0
 */
void gsound_stop_combat_music(int64_t obj)
{
    if (!gsound_combat_music_active) {
        return;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    // Only a player character can end combat.
    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    tig_sound_destroy(gsound_combat_tension_handle);
    tig_sound_destroy(gsound_combat_music_handle);

    gsound_combat_music_active = false;

    // Resume the scheme that was playing before combat started.
    gsound_play_scheme(gsound_pre_combat_scheme_idxs[0], gsound_pre_combat_scheme_idxs[1]);
}

/**
 * Suppresses all scheme and voice playback until `gsound_unlock` is called.
 *
 * On the first lock, saves the active scheme indices and immediately stops all
 * playing scheme sounds. Nested lock/unlock pairs are supported - the lock
 * counter is incremented on each call, and playback only resumes when it
 * reaches zero.
 *
 * 0x41C610
 */
void gsound_lock()
{
    int type;

    if (gsound_lock_cnt == 0) {
        // Save active scheme indices before silencing playback.
        for (type = 0; type < GSOUND_SCHEME_SLOTS; type++) {
            if (gsound_active_schemes[type].scheme_num != 0) {
                gsound_pre_lock_scheme_idxs[type] = gsound_active_schemes[type].scheme_idx;
            } else {
                gsound_pre_lock_scheme_idxs[type] = 0;
            }
        }

        // Stop all scheme sounds immediately (no fade).
        gsound_scheme_stop_all(0);
    }

    gsound_lock_cnt++;
}

/**
 * Releases one level of sound lock and resumes playback when fully unlocked.
 *
 * Has no effect if the lock counter is already zero.
 *
 * 0x41C660
 */
void gsound_unlock()
{
    if (gsound_lock_cnt > 0) {
        if (--gsound_lock_cnt == 0) {
            gsound_play_scheme(gsound_pre_lock_scheme_idxs[0], gsound_pre_lock_scheme_idxs[1]);
        }
    }
}

/**
 * Sets the listener position in normalized (origin-relative) screen-space
 * coordinates and recalculates all active positional sounds.
 *
 * 0x41C690
 */
void set_listener_xy(int64_t x, int64_t y)
{
    if (!gsound_initialized) {
        return;
    }

    gsound_listener_x = x;
    gsound_listener_y = y;

    recalc_positional_sounds_volume();
}

/**
 * Updates the listener position from a map location.
 *
 * 0x41C6D0
 */
void gsound_listener_set(int64_t loc)
{
    int64_t x;
    int64_t y;

    if (!gsound_initialized) {
        return;
    }

    location_xy(loc, &x, &y);

    // Update origin x/y if present.
    if (gsound_origin_loc != 0) {
        location_xy(gsound_origin_loc, &gsound_origin_x, &gsound_origin_y);
    }

    // Express the listener position in origin-relative coordinates.
    set_listener_xy(x - gsound_origin_x, y - gsound_origin_y);
}

/**
 * Updates the world position of an active positional sound.
 *
 * Converts the new map location to normalized (origin-relative) coordinates,
 * updates the TIG sound position, and recalculates the volume and balance
 * relative to the current listener.
 *
 * 0x41C780
 */
void gsound_move(tig_sound_handle_t sound_handle, int64_t location)
{
    int64_t x;
    int64_t y;
    TigSoundPositionalSize size;
    int volume;
    int extra_volume;

    location_xy(location, &x, &y);

    // Convert to origin-relative coordinates.
    x -= gsound_origin_x;
    y -= gsound_origin_y;

    tig_sound_set_position(sound_handle, x, y);
    size = tig_sound_get_positional_size(sound_handle);
    gsound_calc_positional_params(x, y, &volume, &extra_volume, size);
    tig_sound_set_volume(sound_handle, adjust_volume(sound_handle, volume));
    tig_sound_set_extra_volume(sound_handle, extra_volume);
}

/**
 * Configures the default distance attenuation radii and stereo pan distances
 * for LARGE positional sounds, then recalculates all active positional sounds.
 *
 * 0x41C850
 */
void gsound_set_defaults(int min_radius, int max_radius, int min_pan_distance, int max_pan_distance)
{
    if (!gsound_initialized) {
        return;
    }

    sound_minimum_radius[TIG_SOUND_SIZE_LARGE] = min_radius;
    sound_maximum_radius[TIG_SOUND_SIZE_LARGE] = max_radius;
    sound_minimum_pan_distance = min_pan_distance;
    sound_maximum_pan_distance = max_pan_distance;

    recalc_positional_sounds_volume();
}

/**
 * Returns the current raw effects volume level.
 *
 * 0x41C8A0
 */
int gsound_effects_volume_get()
{
    return gsound_effects_volume;
}

/**
 * Called when `effects volume` setting is changed.
 *
 * Recomputes the effects volume from the current value and applies it to all
 * active effect sounds.
 *
 * The setting value is in the range 0–10. The raw volume is derived by
 * converting to the 0–127 range and then scaling by 80% to leave headroom.
 * Also updates the listener position so positional sound volumes are refreshed.
 *
 * 0x41C8B0
 */
void gsound_effects_volume_changed()
{
    int64_t obj;
    int64_t location;

    // Convert 0–10 setting to raw TIG level, scaled to 80% of maximum.
    gsound_effects_volume = 80 * (GSOUND_VOLUME_MAX * settings_get_value(&settings, EFFECTS_VOLUME_KEY) / 10) / 100;
    tig_sound_set_volume_by_type(TIG_SOUND_TYPE_EFFECT, gsound_effects_volume);

    // Refresh positional sound volumes by re-setting the listener position.
    obj = player_get_local_pc_obj();
    if (obj != OBJ_HANDLE_NULL) {
        location = obj_field_int64_get(obj, OBJ_F_LOCATION);
        gsound_listener_set(location);
    }
}

/**
 * Returns the current raw voice volume level.
 *
 * 0x41C920
 */
int gsound_voice_volume_get()
{
    return gsound_voice_volume;
}

/**
 * Called when `voice volume` setting is changed.
 *
 * Recomputes the voice volume from the current value and applies it to all
 * active voice sounds.
 *
 * The setting value is in the range 0–10; converted directly to the raw
 * 0–127 range without the 80% headroom reduction used for music and effects.
 *
 * 0x41C930
 */
void gsound_voice_volume_changed()
{
    gsound_voice_volume = GSOUND_VOLUME_MAX * settings_get_value(&settings, VOICE_VOLUME_KEY) / 10;
    tig_sound_set_volume_by_type(TIG_SOUND_TYPE_VOICE, gsound_voice_volume);
}

/**
 * Returns the current raw music volume level.
 *
 * 0x41C970
 */
int gsound_music_volume_get()
{
    return gsound_music_volume;
}

/**
 * Called when `music volume` setting is changed.
 *
 * Recomputes the music volume from the current value and applies it to all
 * active music sounds.
 *
 * 0x41C980
 */
void gsound_music_volume_changed()
{
    gsound_music_volume = 80 * (GSOUND_VOLUME_MAX * settings_get_value(&settings, MUSIC_VOLUME_KEY) / 10) / 100;
    tig_sound_set_volume_by_type(TIG_SOUND_TYPE_MUSIC, gsound_music_volume);
}

/**
 * Scales a raw volume level by the master volume for the sound's channel type.
 *
 * 0x41C9D0
 */
int adjust_volume(tig_sound_handle_t sound_handle, int volume)
{
    switch (tig_sound_get_type(sound_handle)) {
    case TIG_SOUND_TYPE_EFFECT:
        return volume * gsound_effects_volume_get() / GSOUND_VOLUME_MAX;
    case TIG_SOUND_TYPE_MUSIC:
        return volume * gsound_music_volume_get() / GSOUND_VOLUME_MAX;
    case TIG_SOUND_TYPE_VOICE:
        return volume * gsound_voice_volume_get() / GSOUND_VOLUME_MAX;
    case TIG_SOUND_TYPE_COUNT:
        // Should be unreachable.
        break;
    }

    return volume;
}
