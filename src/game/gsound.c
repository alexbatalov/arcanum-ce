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

#define TWO 2
#define TWENTY_FIVE 25

typedef struct Sound {
    /* 0000 */ bool song;
    /* 0001 */ bool loop;
    /* 0002 */ bool over;
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

typedef struct SoundScheme {
    /* 0000 */ int scheme_num;
    /* 0004 */ int scheme_idx;
    /* 0008 */ int count;
    /* 000C */ Sound sounds[TWENTY_FIVE];
} SoundScheme;

static const char* gsound_build_sound_path(const char* name);
static void sub_41AFB0(SoundScheme* scheme);
static tig_sound_handle_t gsound_play_sfx_func(const char* path, int loops, int volume, int extra_volume, int id);
static void recalc_positional_sounds_volume();
static void recalc_positional_sound_volume(tig_sound_handle_t sound_handle);
static void sub_41B420(int64_t x, int64_t y, int* volume_ptr, int* extra_volume_ptr, TigSoundPositionalSize size);
static tig_sound_handle_t gsound_play_sfx_ex(int id, int loops, int volume, int extra_volume);
static tig_sound_handle_t gsound_play_sfx_at_xy_ex(int id, int loops, int64_t x, int64_t y, int size);
static tig_sound_handle_t gsound_play_sfx_at_loc_ex(int id, int loops, int64_t location, int size);
static void sub_41BA20(int fade_duration, int index);
static void sub_41BAC0(int fade_duration);
static void sub_41BAF0();
static void sub_41BCD0(const char* name, char* buffer);
static bool sub_41BD10(int scheme_idx, int* a2);
static void sub_41BE20(int scheme_idx);
static Sound* sub_41BF70(SoundScheme* scheme, const char* path);
static void sub_41C260(char* str);
static void sub_41C290(const char* str, const char* key, int* value1, int* value2);
static void set_listener_xy(int64_t x, int64_t y);
static void gsound_set_defaults(int min_radius, int max_radius, int min_pan_distance, int max_pan_distance);
static int gsound_effects_volume_get();
static void gsound_effects_volume_changed();
static int gsound_voice_volume_get();
static void gsound_voice_volume_changed();
static int gsound_music_volume_get();
static void gsound_music_volume_changed();
static int adjust_volume(tig_sound_handle_t sound_handle, int volume);

// 0x5A0F38
static const char* gsound_base_sound_path = "sound\\";

// 0x5D1A18
static int gsound_voice_volume;

// 0x5D1A20
static int64_t sound_maximum_pan_distance;

// 0x5D1A28
static int64_t qword_5D1A28;

// 0x5D1A30
static int dword_5D1A30[TWO];

// 0x5D1A38
static int dword_5D1A38[TWO];

// 0x5D1A40
static mes_file_handle_t gsound_scheme_list_mes_file;

// 0x5D1A44
static int gsound_music_volume;

// 0x5D1A48
static tig_sound_handle_t gsound_combat_music_handle;

// 0x5D1A4C
static tig_sound_handle_t gsound_combat_tension_handle;

// 0x5D1A50
static int64_t gsound_listener_x;

// 0x5D1A58
static int64_t gsound_listener_y;

// 0x5D1A68
static mes_file_handle_t* gsound_sfx_mes_files;

// 0x5D1A60
static int64_t sound_minimum_pan_distance;

// 0x5D1A6C
static bool gsound_initialized;

// 0x5D1A70
static int gsound_lock_cnt;

// 0x5D1A78
static int64_t sound_minimum_radius[TIG_SOUND_SIZE_COUNT];

// 0x5D1A98
static SoundScheme stru_5D1A98[TWO];

// 0x5D5480
static mes_file_handle_t gsound_scheme_index_mes_file;

// 0x5D5484
static int dword_5D5484[TWO];

// NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
// compatibility.
//
// 0x5D5594
static int dword_5D5594;

// 0x5D5598
static mes_file_handle_t gsound_snd_user_mes_file;

// 0x5D559C
static int gsound_effects_volume;

// 0x5D55A0
static int sound_maximum_volume[TIG_SOUND_SIZE_COUNT];

// 0x5D55B0
static int gsound_sfx_mes_files_count;

// 0x5D55B8
static int64_t sound_maximum_radius[TIG_SOUND_SIZE_COUNT];

// NOTE: It's `bool`, but needs to be 4 byte integer because of saving/reading
// compatibility.
//
// 0x5D55D8
static int gsound_combat_music_active;

// 0x5D55E0
static int64_t qword_5D55E0;

// 0x5D55E8
static int64_t qword_5D55E8;

// 0x41A940
int gsound_resolve_path(int sound_id, char* path)
{
    MesFileEntry mes_file_entry;
    int index;

    if (gsound_initialized) {
        mes_file_entry.num = sound_id;

        for (index = 0; index < gsound_sfx_mes_files_count; index++) {
            if (mes_search(gsound_sfx_mes_files[index], &mes_file_entry)) {
                sprintf(path, "%s%s", gsound_base_sound_path, mes_file_entry.str);
                return TIG_OK;
            }
        }

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

// 0x41AA30
bool gsound_init(GameInitInfo* init_info)
{
    mes_file_handle_t tmp_mes_file;
    MesFileEntry mes_file_entry;
    int index;
    int positional_sound_params;
    int x;
    int y;

    (void)init_info;

    mes_load(gsound_build_sound_path("snd_00index.mes"), &tmp_mes_file);
    gsound_sfx_mes_files_count = mes_num_entries(tmp_mes_file);
    if (gsound_sfx_mes_files_count == 0) {
        return false;
    }

    gsound_sfx_mes_files = (mes_file_handle_t*)CALLOC(gsound_sfx_mes_files_count, sizeof(*gsound_sfx_mes_files));

    mes_file_entry.num = 0;
    for (index = 0; index < gsound_sfx_mes_files_count; index++) {
        gsound_sfx_mes_files[index] = MES_FILE_HANDLE_INVALID;
        mes_find_next(tmp_mes_file, &mes_file_entry);
        mes_load(gsound_build_sound_path(mes_file_entry.str), &(gsound_sfx_mes_files[index]));
    }

    mes_unload(tmp_mes_file);

    mes_load(gsound_build_sound_path("SchemeIndex.mes"), &gsound_scheme_index_mes_file);
    mes_load(gsound_build_sound_path("SchemeList.mes"), &gsound_scheme_list_mes_file);

    for (index = 0; index < 2; index++) {
        sub_41AFB0(&(stru_5D1A98[index]));
    }

    gsound_initialized = true;

    x = 400;
    y = 300;
    hrp_center(&x, &y);
    set_listener_xy(x, y);

    // TODO: Figure out if these needs to be adjusted for HRP.
    gsound_set_defaults(150, 800, 150, 400);

    sound_maximum_volume[TIG_SOUND_SIZE_LARGE] = 100;

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

        mes_file_entry.num = 10;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_radius[TIG_SOUND_SIZE_SMALL] = atoi(mes_file_entry.str);
        } else {
            sound_minimum_radius[TIG_SOUND_SIZE_SMALL] = 50;
        }

        mes_file_entry.num = 11;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_radius[TIG_SOUND_SIZE_SMALL] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_radius[TIG_SOUND_SIZE_SMALL] = 150;
        }

        mes_file_entry.num = 12;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = 40;
        }

        mes_file_entry.num = 20;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_radius[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_minimum_radius[TIG_SOUND_SIZE_MEDIUM] = 50;
        }

        mes_file_entry.num = 21;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_radius[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_radius[TIG_SOUND_SIZE_MEDIUM] = 400;
        }

        mes_file_entry.num = 22;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_volume[TIG_SOUND_SIZE_MEDIUM] = 70;
        }

        mes_file_entry.num = 30;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_minimum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = atoi(mes_file_entry.str);
        } else {
            sound_minimum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = 50;
        }

        mes_file_entry.num = 31;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_radius[TIG_SOUND_SIZE_EXTRA_LARGE] = 1500;
        }

        mes_file_entry.num = 32;
        if (mes_search(tmp_mes_file, &mes_file_entry)) {
            sound_maximum_volume[TIG_SOUND_SIZE_EXTRA_LARGE] = atoi(mes_file_entry.str);
        } else {
            sound_maximum_volume[TIG_SOUND_SIZE_EXTRA_LARGE] = 100;
        }

        if (positional_sound_params != 0) {
            recalc_positional_sounds_volume();
        }

        mes_unload(tmp_mes_file);
    }

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

// 0x41AF80
const char* gsound_build_sound_path(const char* name)
{
    // 0x5D5490
    static char path[TIG_MAX_PATH];

    sprintf(path, "%s%s", gsound_base_sound_path, name);

    return path;
}

// 0x41AFB0
void sub_41AFB0(SoundScheme* scheme)
{
    int index;

    scheme->scheme_num = 0;
    scheme->count = 0;

    for (index = 0; index < TWENTY_FIVE; index++) {
        memset(&(scheme->sounds[index]), 0, sizeof(*scheme->sounds));
        scheme->sounds[index].sound_handle = TIG_SOUND_HANDLE_INVALID;
    }
}

// 0x41AFF0
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

// 0x41B060
void gsound_reset()
{
    qword_5D1A28 = 0;
    qword_5D55E8 = 0;
    qword_5D55E0 = 0;
    gsound_combat_music_active = false;
    gsound_lock_cnt = 0;
    gsound_stop_all(0);
}

// 0x41B0A0
bool gsound_mod_load()
{
    mes_load(gsound_build_sound_path("snd_user.mes"), &gsound_snd_user_mes_file);
    return true;
}

// 0x41B0D0
void gsound_mod_unload()
{
    mes_unload(gsound_snd_user_mes_file);
    gsound_snd_user_mes_file = MES_FILE_HANDLE_INVALID;
}

// 0x41B0F0
bool gsound_load(GameLoadInfo* load_info)
{
    int temp[TWO];
    int index;

    for (index = 0; index < TWO; index++) {
        if (tig_file_fread(&(temp[index]), sizeof(*temp), 1, load_info->stream) != 1) {
            return false;
        }
    }

    if (tig_file_fread(&dword_5D5594, sizeof(dword_5D5594), 1, load_info->stream) != 1) {
        return false;
    }

    for (index = 0; index < TWO; index++) {
        if (tig_file_fread(&(dword_5D1A38[index]), sizeof(*dword_5D1A38), 1, load_info->stream) != 1) {
            return false;
        }
    }

    if (tig_file_fread(&gsound_combat_music_active, sizeof(gsound_combat_music_active), 1, load_info->stream) != 1) {
        return false;
    }

    for (index = 0; index < TWO; index++) {
        if (tig_file_fread(&(dword_5D1A30[index]), sizeof(*dword_5D1A30), 1, load_info->stream) != 1) {
            return false;
        }
    }

    gsound_play_scheme(temp[0], temp[1]);

    return true;
}

// 0x41B1D0
bool gsound_save(TigFile* stream)
{
    int index;
    int scheme_idx;

    for (index = 0; index < TWO; index++) {
        scheme_idx = stru_5D1A98[index].scheme_num != 0 ? stru_5D1A98[index].scheme_idx : 0;
        if (tig_file_fwrite(&scheme_idx, sizeof(scheme_idx), 1, stream) != 1) {
            return false;
        }
    }

    if (tig_file_fwrite(&dword_5D5594, sizeof(dword_5D5594), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < TWO; index++) {
        if (tig_file_fwrite(&(dword_5D1A38[index]), sizeof(*dword_5D1A38), 1, stream) != 1) {
            return false;
        }
    }

    if (tig_file_fwrite(&gsound_combat_music_active, sizeof(gsound_combat_music_active), 1, stream) != 1) {
        return false;
    }

    for (index = 0; index < TWO; index++) {
        if (tig_file_fwrite(&(dword_5D1A30[index]), sizeof(*dword_5D1A30), 1, stream) != 1) {
            return false;
        }
    }

    return true;
}

// 0x41B2A0
void gsound_ping(tig_timestamp_t timestamp)
{
    // 0x5D548C
    static tig_timestamp_t dword_5D548C;

    if (!gsound_initialized) {
        return;
    }

    if (timestamp - dword_5D548C > 250) {
        dword_5D548C = timestamp;
        sub_41BAF0();
    }
}

// 0x41B2D0
void gsound_flush()
{
    gsound_reset();
}

// 0x41B2E0
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

// 0x41B3A0
void recalc_positional_sounds_volume()
{
    tig_sound_enumerate_positional(recalc_positional_sound_volume);
}

// 0x41B3B0
void recalc_positional_sound_volume(tig_sound_handle_t sound_handle)
{
    int64_t x;
    int64_t y;
    TigSoundPositionalSize size;
    int volume;
    int extra_volume;

    tig_sound_get_position(sound_handle, &x, &y);
    size = tig_sound_get_positional_size(sound_handle);
    sub_41B420(x, y, &volume, &extra_volume, size);
    tig_sound_set_volume(sound_handle, adjust_volume(sound_handle, volume));
    tig_sound_set_extra_volume(sound_handle, extra_volume);
}

// 0x41B420
void sub_41B420(int64_t x, int64_t y, int* volume_ptr, int* extra_volume_ptr, TigSoundPositionalSize size)
{
    int extra_volume = 64;
    int volume = 127;
    int64_t distance;
    int64_t dx;

    distance = (int64_t)sqrt((double)((x - gsound_listener_x) * (x - gsound_listener_x) + 2 * (y - gsound_listener_y) * 2 * (y - gsound_listener_y)));

    dx = x - gsound_listener_x;
    if (dx < 0) {
        dx = gsound_listener_x - x;
    }

    if (distance < sound_minimum_radius[size]) {
        volume = 127;
    } else if (distance > sound_maximum_radius[size]) {
        volume = 0;
    } else if (sound_maximum_radius[size] > sound_minimum_radius[size]) {
        volume = (int)(127 * (sound_minimum_radius[size] - distance) / (sound_maximum_radius[size] - sound_minimum_radius[size])) + 127;
        if (volume < 0) {
            volume = 0;
        } else if (volume > 127) {
            volume = 127;
        }
        extra_volume = 64;
    }

    if (x < gsound_listener_x) {
        if (dx > sound_maximum_pan_distance) {
            extra_volume = 0;
        } else if (dx > sound_minimum_pan_distance) {
            extra_volume = (int)(64 * (sound_minimum_pan_distance - dx) / (sound_maximum_pan_distance - sound_minimum_pan_distance)) + 64;
            if (extra_volume < 0) {
                extra_volume = 0;
            } else if (extra_volume > 127) {
                extra_volume = 127;
            }
        }
    }

    if (x > gsound_listener_x) {
        if (dx > sound_maximum_pan_distance) {
            extra_volume = 127;
        } else if (dx > sound_minimum_pan_distance) {
            extra_volume = (int)(64 * (dx - sound_minimum_pan_distance) / (sound_maximum_pan_distance - sound_minimum_pan_distance)) + 64;
            if (extra_volume < 0) {
                extra_volume = 0;
            } else if (extra_volume > 127) {
                extra_volume = 127;
            }
        }
    }

    if (volume_ptr != NULL) {
        *volume_ptr = volume;
    }

    if (extra_volume_ptr != NULL) {
        *extra_volume_ptr = extra_volume;
    }
}

// 0x41B720
tig_sound_handle_t gsound_play_sfx_ex(int id, int loops, int volume, int extra_volume)
{
    char path[TIG_MAX_PATH];

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    gsound_resolve_path(id, path);
    return gsound_play_sfx_func(path, loops, volume, extra_volume, id);
}

// 0x41B780
tig_sound_handle_t gsound_play_sfx(int id, int loops)
{
    return gsound_play_sfx_ex(id, loops, 127, 64);
}

// 0x41B7A0
tig_sound_handle_t gsound_play_sfx_at_xy(int id, int loops, int64_t x, int64_t y)
{
    return gsound_play_sfx_at_xy_ex(id, loops, x, y, TIG_SOUND_SIZE_LARGE);
}

// 0x41B7D0
tig_sound_handle_t gsound_play_sfx_at_xy_ex(int id, int loops, int64_t x, int64_t y, int size)
{
    int volume;
    int extra_volume;
    tig_sound_handle_t sound_handle;

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    sub_41B420(x, y, &volume, &extra_volume, size);

    sound_handle = gsound_play_sfx_ex(id, loops, volume, extra_volume);
    tig_sound_set_position(sound_handle, x, y);
    tig_sound_set_positional_size(sound_handle, size);

    return sound_handle;
}

// 0x41B850
tig_sound_handle_t gsound_play_sfx_at_loc(int id, int loops, int64_t location)
{
    return gsound_play_sfx_at_loc_ex(id, loops, location, TIG_SOUND_SIZE_LARGE);
}

// 0x41B870
tig_sound_handle_t gsound_play_sfx_at_loc_ex(int id, int loops, int64_t location, int size)
{
    int64_t x;
    int64_t y;

    if (!gsound_initialized) {
        return TIG_SOUND_HANDLE_INVALID;
    }

    if (qword_5D1A28 == OBJ_HANDLE_NULL) {
        qword_5D1A28 = location;
        location_xy(location, &qword_5D55E8, &qword_5D55E0);
    }

    location_xy(location, &x, &y);

    return gsound_play_sfx_at_xy_ex(id, loops, x - qword_5D55E8, y - qword_5D55E0, size);
}

// 0x41B930
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

// 0x41B980
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

// 0x41BA20
void sub_41BA20(int fade_duration, int index)
{
    SoundScheme* scheme;
    Sound* sound;
    int snd;

    scheme = &(stru_5D1A98[index]);
    if (scheme->scheme_num != 0) {
        if (!dword_5D5594) {
            dword_5D1A38[index] = scheme->scheme_idx;
        }

        for (snd = 0; snd < TWENTY_FIVE; snd++) {
            sound = &(scheme->sounds[snd]);
            if (sound->loop) {
                if (sound->sound_handle != TIG_SOUND_HANDLE_INVALID) {
                    tig_sound_stop(sound->sound_handle, fade_duration);
                    sound->sound_handle = TIG_SOUND_HANDLE_INVALID;
                }
            }
        }

        scheme->scheme_num = 0;
        sub_41AFB0(scheme);
    } else {
        if (!dword_5D5594) {
            dword_5D1A38[index] = 0;
        }
    }
}

// 0x41B9E0
int gsound_range(int64_t obj)
{
    TigSoundPositionalSize size;

    size = gsound_size(obj);
    return (int)sound_maximum_radius[size] / 40;
}

// 0x41BAC0
void sub_41BAC0(int fade_duration)
{
    int index;

    if (gsound_initialized) {
        for (index = 0; index < TWO; index++) {
            sub_41BA20(fade_duration, index);
        }
    }
}

// 0x41BAF0
void sub_41BAF0()
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

    for (type = 0; type < TWO; type++) {
        scheme = &(stru_5D1A98[type]);
        for (idx = 0; idx < scheme->count; idx++) {
            sound = &(scheme->sounds[idx]);
            if (!sound->song) {
                if (hour >= sound->time_start
                    && hour <= sound->time_end
                    && random_between(0, 999) < sound->frequency) {
                    sub_41BCD0(sound->path, path);
                    if (sound->volume_max > sound->volume_min) {
                        volume = sound->volume_min
                            + random_between(0, sound->volume_max - sound->volume_min - 1);
                    } else {
                        volume = sound->volume_min;
                    }

                    if (sound->balance_right > sound->balance_left) {
                        extra_volume = sound->balance_left
                            + random_between(0, sound->balance_right - sound->balance_left - 1);
                    } else {
                        extra_volume = sound->balance_left;
                    }

                    gsound_play_sfx_func(path, 1, volume, extra_volume, 0);
                }
            } else if (sound->over) {
                if (!tig_sound_is_playing(sound->sound_handle)) {
                    if (dword_5D5594) {
                        gsound_play_scheme(dword_5D1A38[0], dword_5D1A38[1]);
                        dword_5D5594 = false;
                    }
                }
            } else if (sound->loop) {
                if (hour < sound->time_start || hour > sound->time_end) {
                    if (sound->sound_handle != TIG_SOUND_HANDLE_INVALID) {
                        tig_sound_stop(sound->sound_handle, 25);
                        sound->sound_handle = TIG_SOUND_HANDLE_INVALID;
                    }
                } else {
                    if (sound->sound_handle == TIG_SOUND_HANDLE_INVALID) {
                        sub_41BCD0(sound->path, path);
                        tig_sound_create(&(sound->sound_handle), TIG_SOUND_TYPE_MUSIC);
                        tig_sound_set_volume(sound->sound_handle, gsound_music_volume * (sound->volume_min) / 100);
                        tig_sound_play_streamed_indefinitely(sound->sound_handle, path, 25, TIG_SOUND_HANDLE_INVALID);
                    }
                }
            }
        }
    }
}

// 0x41BCD0
void sub_41BCD0(const char* name, char* buffer)
{
    if (name[0] == '#') {
        gsound_resolve_path(atoi(name + 1), buffer);
    } else {
        sprintf(buffer, "%s%s", gsound_base_sound_path, name);
    }
}

// 0x41BD10
bool sub_41BD10(int scheme_idx, int* a2)
{
    int index;

    for (index = 0; index < TWO; index++) {
        if (stru_5D1A98[index].scheme_num != 0) {
            if (stru_5D1A98[index].scheme_idx == scheme_idx) {
                *a2 = index;
                return true;
            }
        }
    }

    return false;
}

// 0x41BD50
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

    if (music_scheme_idx == ambient_scheme_idx) {
        ambient_scheme_idx = 0;
    }

    if (gsound_combat_music_active) {
        dword_5D1A30[0] = music_scheme_idx;
        dword_5D1A30[1] = ambient_scheme_idx;
    } else {
        music_scheme_active = sub_41BD10(music_scheme_idx, &music_scheme_slot);
        ambient_scheme_active = sub_41BD10(ambient_scheme_idx, &ambient_scheme_slot);
        if (!music_scheme_active && !ambient_scheme_active) {
            sub_41BAC0(25);
            sub_41BE20(music_scheme_idx);
            sub_41BE20(ambient_scheme_idx);
        } else if (music_scheme_active && !ambient_scheme_active) {
            sub_41BA20(25, 1 - music_scheme_slot);
            sub_41BE20(ambient_scheme_idx);
        } else if (!music_scheme_active && ambient_scheme_active) {
            sub_41BA20(25, 1 - ambient_scheme_slot);
            sub_41BE20(music_scheme_idx);
        }
    }
}

// 0x41BE20
void sub_41BE20(int scheme_idx)
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

    for (index = 0; index < TWO; index++) {
        if (stru_5D1A98[index].scheme_num == 0) {
            break;
        }
    }

    if (index == TWO) {
        return;
    }

    scheme = &(stru_5D1A98[index]);
    sub_41AFB0(scheme);

    scheme->scheme_idx = scheme_idx;

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

    for (index = 0; index < 100; index++) {
        mes_file_entry.num = scheme->scheme_num + index;
        if (mes_search(gsound_scheme_list_mes_file, &mes_file_entry)) {
            sound = sub_41BF70(scheme, mes_file_entry.str);
            if (sound != NULL) {
                if (sound->song
                    && !sound->loop) {
                    sub_41BCD0(sound->path, path);
                    tig_sound_create(&(sound->sound_handle), TIG_SOUND_TYPE_MUSIC);
                    tig_sound_set_volume(sound->sound_handle, gsound_music_volume * sound->volume_min / 100);
                    tig_sound_play_streamed_once(sound->sound_handle, path, 25, TIG_SOUND_HANDLE_INVALID);
                }

                if (sound->over) {
                    dword_5D5594 = true;
                }
            }
        }
    }
}

// 0x41BF70
Sound* sub_41BF70(SoundScheme* scheme, const char* path)
{
    Sound* sound = NULL;
    char* copy;
    char* pch;
    int v1;
    int v2;

    copy = STRDUP(path);
    if (copy != NULL) {
        if (scheme->count < TWENTY_FIVE) {
            sound = &(scheme->sounds[scheme->count++]);
            memset(sound, 0, sizeof(*sound));
            sound->song = false;
            sound->loop = false;
            sound->over = false;
            sound->balance_left = 50;
            sound->balance_right = 50;
            sound->frequency = 5;
            sound->time_start = 0;
            sound->time_end = 23;
            sound->volume_min = 100;
            sound->volume_max = 100;
            sound->sound_handle = TIG_SOUND_HANDLE_INVALID;
            sound->path[0] = '\0';
            sub_41C260(copy);

            pch = strchr(copy, ' ');
            if (pch != NULL) {
                *pch++ = '\0';
            }

            strcpy(sound->path, copy);

            if (pch != NULL) {
                sub_41C290(pch, "/vol:", &(sound->volume_min), &(sound->volume_max));

                if (strstr(pch, "/loop") != NULL || strstr(pch, "/anchor") != NULL || strstr(pch, "/over") != NULL) {
                    sound->song = true;

                    if (strstr(pch, "/over") != NULL) {
                        sound->over = true;
                    }

                    if (strstr(pch, "/loop") != NULL) {
                        sound->loop = true;
                        sub_41C290(pch, "/time:", &(sound->time_start), &(sound->time_end));
                    }
                } else {
                    sub_41C290(pch, "/freq:", &(sound->frequency), NULL);
                    sub_41C290(pch, "/time:", &(sound->time_start), &(sound->time_end));
                    sub_41C290(pch, "/bal:", &(sound->balance_left), &(sound->balance_right));

                    v1 = -1;
                    v2 = -1;
                    sub_41C290(pch, "/scatter:", &v1, &v2);

                    if (v1 >= 0) {
                        sound->volume_max = 100;
                        sound->volume_min = 100 - v1;
                        sound->balance_left = 50 - v1 / 2;
                        sound->balance_right = 50 + v1 / 2;
                    }
                }
            }
        }

        FREE(copy);

        if (sound->volume_max < sound->volume_min) {
            sound->volume_max = sound->volume_min;
        }

        if (sound->balance_right < sound->balance_left) {
            sound->balance_right = sound->balance_left;
        }

        // TODO: Strange 64-bit math, check.
        sound->volume_min = 127 * sound->volume_min / 100;
        sound->volume_max = 127 * sound->volume_max / 100;
        sound->balance_left = 127 * sound->balance_left / 100;
        sound->balance_right = 127 * sound->balance_right / 100;

        if (sound->volume_min < 0) {
            sound->volume_min = 0;
        }

        if (sound->volume_min > 127) {
            sound->volume_min = 127;
        }

        if (sound->volume_max < 0) {
            sound->volume_max = 0;
        }

        if (sound->volume_max > 127) {
            sound->volume_max = 127;
        }

        if (sound->balance_left < 0) {
            sound->balance_left = 0;
        }

        // FIXME: Looks wrong, max balance should be 100.
        if (sound->balance_left > 127) {
            sound->balance_left = 0;
        }

        if (sound->balance_right < 0) {
            sound->balance_right = 0;
        }

        // FIXME: Looks wrong, max balance should be 100.
        if (sound->balance_right > 127) {
            sound->balance_right = 127;
        }
    }

    return sound;
}

// 0x41C260
void sub_41C260(char* str)
{
    while (*str != '\0') {
        *str = (char)(unsigned char)SDL_tolower(*(unsigned char*)str);
        str++;
    }
}

// 0x41C290
void sub_41C290(const char* str, const char* key, int* value1, int* value2)
{
    const char* pch;
    char* copy;
    char* space;
    char* sep;

    pch = strstr(str, key);
    if (pch != NULL) {
        copy = STRDUP(pch + strlen(key));
        if (copy != NULL) {
            space = strchr(copy, ' ');
            if (space != NULL) {
                *space = '\0';
            }

            if (value1 != NULL) {
                *value1 = atoi(copy);

                if (value2 != NULL) {
                    sep = strchr(copy, ',');
                    if (sep == NULL) {
                        sep = strchr(copy, '-');
                    }

                    if (sep != NULL) {
                        *value2 = atoi(sep + 1);
                    } else {
                        *value2 = *value1;
                    }
                }
            }

            FREE(copy);
        }
    }
}

// 0x41C340
void gsound_stop_all(int fade_duration)
{
    if (gsound_initialized) {
        sub_41BAC0(fade_duration);
        dword_5D5594 = false;
        tig_sound_stop_all(fade_duration);
    }
}

// 0x41C370
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

// 0x41C3D0
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

// 0x41C450
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

// 0x41C4D0
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

    for (type = 0; type < TWO; type++) {
        if (stru_5D1A98[type].scheme_num != 0) {
            dword_5D1A30[type] = stru_5D1A98[type].scheme_idx;
        } else {
            dword_5D1A30[type] = 0;
        }
    }

    sub_41BAC0(25);

    gsound_combat_music_active = true;

    sprintf(path, "sound\\music\\combat %d.mp3", random_between(1, 6));
    gsound_combat_tension_handle = gsound_play_music_once(path, 0);
    gsound_combat_music_handle = gsound_play_music_indefinitely("sound\\music\\combatmusic.mp3", 0);
}

// 0x41C5A0
void gsound_stop_combat_music(int64_t obj)
{
    if (!gsound_combat_music_active) {
        return;
    }

    if (obj == OBJ_HANDLE_NULL) {
        return;
    }

    if (obj_field_int32_get(obj, OBJ_F_TYPE) != OBJ_TYPE_PC) {
        return;
    }

    tig_sound_destroy(gsound_combat_tension_handle);
    tig_sound_destroy(gsound_combat_music_handle);

    gsound_combat_music_active = false;

    gsound_play_scheme(dword_5D1A30[0], dword_5D1A30[1]);
}

// 0x41C610
void gsound_lock()
{
    int type;

    if (gsound_lock_cnt == 0) {
        for (type = 0; type < TWO; type++) {
            if (stru_5D1A98[type].scheme_num != 0) {
                dword_5D5484[type] = stru_5D1A98[type].scheme_idx;
            } else {
                dword_5D5484[type] = 0;
            }
        }

        sub_41BAC0(0);
    }

    gsound_lock_cnt++;
}

// 0x41C660
void gsound_unlock()
{
    if (gsound_lock_cnt > 0) {
        if (--gsound_lock_cnt == 0) {
            gsound_play_scheme(dword_5D5484[0], dword_5D5484[1]);
        }
    }
}

// 0x41C690
void set_listener_xy(int64_t x, int64_t y)
{
    if (!gsound_initialized) {
        return;
    }

    gsound_listener_x = x;
    gsound_listener_y = y;

    recalc_positional_sounds_volume();
}

// 0x41C6D0
void gsound_listener_set(int64_t loc)
{
    int64_t x;
    int64_t y;

    if (!gsound_initialized) {
        return;
    }

    location_xy(loc, &x, &y);

    if (qword_5D1A28 != 0) {
        location_xy(qword_5D1A28, &qword_5D55E8, &qword_5D55E0);
    }

    set_listener_xy(x - qword_5D55E8, y - qword_5D55E0);
}

// 0x41C780
void gsound_move(tig_sound_handle_t sound_handle, int64_t location)
{
    int64_t x;
    int64_t y;
    TigSoundPositionalSize size;
    int volume;
    int extra_volume;

    location_xy(location, &x, &y);
    x -= qword_5D55E8;
    y -= qword_5D55E0;
    tig_sound_set_position(sound_handle, x, y);
    size = tig_sound_get_positional_size(sound_handle);
    sub_41B420(x, y, &volume, &extra_volume, size);
    tig_sound_set_volume(sound_handle, adjust_volume(sound_handle, volume));
    tig_sound_set_extra_volume(sound_handle, extra_volume);
}

// 0x41C850
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

// 0x41C8A0
int gsound_effects_volume_get()
{
    return gsound_effects_volume;
}

// 0x41C8B0
void gsound_effects_volume_changed()
{
    int64_t obj;
    int64_t location;

    gsound_effects_volume = 80 * (127 * settings_get_value(&settings, EFFECTS_VOLUME_KEY) / 10) / 100;
    tig_sound_set_volume_by_type(TIG_SOUND_TYPE_EFFECT, gsound_effects_volume);

    obj = player_get_local_pc_obj();
    if (obj != OBJ_HANDLE_NULL) {
        location = obj_field_int64_get(obj, OBJ_F_LOCATION);
        gsound_listener_set(location);
    }
}

// 0x41C920
int gsound_voice_volume_get()
{
    return gsound_voice_volume;
}

// 0x41C930
void gsound_voice_volume_changed()
{
    gsound_voice_volume = 127 * settings_get_value(&settings, VOICE_VOLUME_KEY) / 10;
    tig_sound_set_volume_by_type(TIG_SOUND_TYPE_VOICE, gsound_voice_volume);
}

// 0x41C970
int gsound_music_volume_get()
{
    return gsound_music_volume;
}

// 0x41C980
void gsound_music_volume_changed()
{
    gsound_music_volume = 80 * (127 * settings_get_value(&settings, MUSIC_VOLUME_KEY) / 10) / 100;
    tig_sound_set_volume_by_type(TIG_SOUND_TYPE_MUSIC, gsound_music_volume);
}

// 0x41C9D0
int adjust_volume(tig_sound_handle_t sound_handle, int volume)
{
    switch (tig_sound_get_type(sound_handle)) {
    case TIG_SOUND_TYPE_EFFECT:
        return volume * gsound_effects_volume_get() / 127;
    case TIG_SOUND_TYPE_MUSIC:
        return volume * gsound_music_volume_get() / 127;
    case TIG_SOUND_TYPE_VOICE:
        return volume * gsound_voice_volume_get() / 127;
    case TIG_SOUND_TYPE_COUNT:
        // Should be unreachable.
        break;
    }

    return volume;
}
