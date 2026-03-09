#ifndef ARCANUM_GAME_GMOVIE_H_
#define ARCANUM_GAME_GMOVIE_H_

#include "game/context.h"

// NOTE: Gmovie flags are the same as TIG but have slightly different values.
typedef unsigned int GameMovieFlags;

#define GAME_MOVIE_FADE_IN 0x1u
#define GAME_MOVIE_FADE_OUT 0x2u
#define GAME_MOVIE_IGNORE_KEYBOARD 0x4u
#define GAME_MOVIE_NO_FINAL_FLIP 0x8u

bool gmovie_mod_load(void);
void gmovie_mod_unload(void);
void gmovie_play(int movie, GameMovieFlags flags, int sound_track);
void gmovie_play_path(const char* path, GameMovieFlags flags, int sound_track);

#endif /* ARCANUM_GAME_GMOVIE_H_ */
