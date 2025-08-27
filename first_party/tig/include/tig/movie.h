#ifndef TIG_MOVIE_H_
#define TIG_MOVIE_H_

#include "tig/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int TigMovieFlags;

#define TIG_MOVIE_IGNORE_KEYBOARD 0x01u
#define TIG_MOVIE_FADE_IN 0x02u
#define TIG_MOVIE_FADE_OUT 0x04u
#define TIG_MOVIE_NO_FINAL_FLIP 0x08u

// Initializes movie system.
int tig_movie_init(TigInitInfo* init_info);

// Shutdowns movie system.
void tig_movie_exit();

// Play given movie file.
int tig_movie_play(const char* path, TigMovieFlags movie_flags, int sound_track);

// Returns `true` if movie is currently being played.
bool tig_movie_is_playing();

#ifdef __cplusplus
}
#endif

#endif /* TIG_MOVIE_H_ */
