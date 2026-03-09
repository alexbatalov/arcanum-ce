#include "game/gmovie.h"

#include <stdio.h>

#include "game/gsound.h"
#include "game/mes.h"

/**
 * "movies.mes"
 *
 * 0x59F02C
 */
static mes_file_handle_t movies_mes_file = MES_FILE_HANDLE_INVALID;

/**
 * Called when a module is being loaded.
 *
 * 0x40DE20
 */
bool gmovie_mod_load(void)
{
    // Load "movies.mes" (might be absent).
    if (!mes_load("movies\\movies.mes", &movies_mes_file)) {
        movies_mes_file = MES_FILE_HANDLE_INVALID;
    }

    return true;
}

/**
 * Called when a module is being unloaded.
 *
 * 0x40DE50
 */
void gmovie_mod_unload(void)
{
    // Unload "movies.mes".
    if (movies_mes_file != MES_FILE_HANDLE_INVALID) {
        mes_unload(movies_mes_file);
        movies_mes_file = MES_FILE_HANDLE_INVALID;
    }
}

/**
 * Plays a game movie specified by ID (from "movies.mes" file).
 *
 * 0x40DE70
 */
void gmovie_play(int movie, GameMovieFlags flags, int sound_track)
{
    MesFileEntry mes_file_entry;
    char path[TIG_MAX_PATH];

    // Check if "movies.mes" was successfully initialized.
    if (movies_mes_file == MES_FILE_HANDLE_INVALID) {
        return;
    }

    // Search for file name in "movies.mes".
    mes_file_entry.num = movie;
    if (!mes_search(movies_mes_file, &mes_file_entry)) {
        return;
    }

    // Build a full movie path and play it.
    sprintf(path, "movies\\%s", mes_file_entry.str);
    gmovie_play_path(path, flags, sound_track);
}

/**
 * Plays a game movie specified by its file path.
 *
 * 0x40DEE0
 */
void gmovie_play_path(const char* path, GameMovieFlags flags, int sound_track)
{
    char temp_path[TIG_MAX_PATH];
    unsigned int movie_flags;

    // Copy movie file from DAT archive to file system. This is needed because
    // underlying BINK player needs a file path.
    if (!tig_file_extract(path, temp_path)) {
        return;
    }

    // Lock game sound system to ensure exclusive audio access during movie
    // playback.
    gsound_lock();

    // Convert game movie flags to TIG movie flags.
    movie_flags = 0;

    if ((flags & GAME_MOVIE_FADE_IN) != 0) {
        movie_flags |= TIG_MOVIE_FADE_IN;
    }

    if ((flags & GAME_MOVIE_FADE_OUT) != 0) {
        movie_flags |= TIG_MOVIE_FADE_OUT;
    }

    if ((flags & GAME_MOVIE_IGNORE_KEYBOARD) != 0) {
        movie_flags |= TIG_MOVIE_IGNORE_KEYBOARD;
    }

    if ((flags & GAME_MOVIE_NO_FINAL_FLIP) != 0) {
        movie_flags |= TIG_MOVIE_NO_FINAL_FLIP;
    }

    // Play the movie. This is a blocking call, which will return when movie
    // playback ends.
    tig_movie_play(temp_path, movie_flags, sound_track);

    // Resume sound system.
    gsound_unlock();
}
