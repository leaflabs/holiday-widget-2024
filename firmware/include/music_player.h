#ifndef __MUSIC_PLAYER_H__
#define __MUSIC_PLAYER_H__
#include "music_player_core.h"

/* Set up the music player */
void music_player_setup(void);

/* Run through the music player's state machine */
void music_player_run(void);

/* Play the provided song - returns MUSIC_PLAYER_RUN_ERROR on failure */
enum music_player_error music_player_play_song(enum Song song);

/* Stop the current song - returns MUSIC_PLAYER_STOP_ERROR on failure */
enum music_player_error music_player_abort_song();

/* Returns true if a song is currently playing, else false */
bool music_player_is_song_playing();

/* Returns music player error code */
enum music_player_error music_player_get_error();

#endif