#include "music_player.h"

static struct music_player_context context = {
    .current_song = NO_SONG,
    .error = MUSIC_PLAYER_NO_ERROR,
    .state = MUSIC_PLAYER_UNINITIALIZED};

/* This function is called if an error occurs on the duration transfer DMA
 * Channel */
void duration_transfer_error_callback(DMA_HandleTypeDef *hdma) {
    UNUSED(hdma);
    context.error = MUSIC_PLAYER_DURATIONS_DMA_ERROR;
}

/* This function is called if an error occurs on the note transfer DMA Channel
 */
void note_transfer_error_callback(DMA_HandleTypeDef *hdma) {
    UNUSED(hdma);
    context.error = MUSIC_PLAYER_NOTES_DMA_ERROR;
}

/* This function is called whenever a song ends */
void song_complete_callback(DMA_HandleTypeDef *hdma) {
    UNUSED(hdma);
    context.current_song = NO_SONG;
}

/* This function is called whenever a DAC error occurs on Channel 1 */
void dac_error_callback(DAC_HandleTypeDef *hdac) {
    UNUSED(hdac);
    context.error = MUSIC_PLAYER_DAC_DMA_ERROR;
}

/* Set up the music player */
void music_player_setup(void) {
    static const struct music_player_config config = {
        .duration_transfer_error_callback = duration_transfer_error_callback,
        .note_transfer_error_callback = note_transfer_error_callback,
        .song_complete_callback = song_complete_callback,
        .dac_error_callback = dac_error_callback};
    if ((context.error = music_player_core_init(&config, &context)) != 0) {
        context.state = MUSIC_PLAYER_ERROR;
    } else {
        context.state = MUSIC_PLAYER_READY;
    }
}

/* Run through the music player's state machine */
void music_player_run(void) {
    switch (context.state) {
        case MUSIC_PLAYER_UNINITIALIZED:
            /* Music Player Uninitialized */
            uart_logger_send("Music Player not initialized properly\r\n");

            break;
        case MUSIC_PLAYER_READY:
            /* Music Player Ready */
            if (context.current_song != NO_SONG) {
                if ((context.error =
                         music_player_core_play_song(context.current_song))) {
                    context.state = MUSIC_PLAYER_ERROR;
                } else {
                    context.state = MUSIC_PLAYER_BUSY;
                }
            }
            break;
        case MUSIC_PLAYER_BUSY:
            /* Music Player Busy */
            if (context.current_song == NO_SONG) {
                if ((context.error = music_player_core_abort_song())) {
                    context.state = MUSIC_PLAYER_ERROR;
                } else {
                    context.state = MUSIC_PLAYER_READY;
                }
            }
            break;
        case MUSIC_PLAYER_ERROR:
            /* Music Player Error */
            switch (context.error) {
                case MUSIC_PLAYER_DAC_DMA_ERROR:
                    break;
                case MUSIC_PLAYER_DURATIONS_DMA_ERROR:
                    break;
                case MUSIC_PLAYER_NOTES_DMA_ERROR:
                    break;
                case MUSIC_PLAYER_RUN_ERROR:
                    break;
                case MUSIC_PLAYER_STOP_ERROR:
                    break;
                case MUSIC_PLAYER_INITIALIZATION_ERROR:
                    break;
                case MUSIC_PLAYER_NO_ERROR:
                    break;
            }
            uart_logger_send("In MUSIC_PLAYER_ERROR\r\n");
            break;
        default:
            /* Switching over enumerated type so you should never be here */
            uart_logger_send("Unsupported Music Player State: %d\r\n",
                             context.state);
            break;
    }
}

/* Play the provided song - returns MUSIC_PLAYER_RUN_ERROR on failure */
enum music_player_error music_player_play_song(enum Song_t song) {
    if (context.state == MUSIC_PLAYER_READY) {
        context.current_song = song;
        return MUSIC_PLAYER_NO_ERROR;
    }

    return MUSIC_PLAYER_RUN_ERROR;
}

/* Stop the current song - returns MUSIC_PLAYER_STOP_ERROR on failure */
enum music_player_error music_player_abort_song() {
    if (context.state == MUSIC_PLAYER_BUSY) {
        context.current_song = NO_SONG;
        return MUSIC_PLAYER_NO_ERROR;
    }

    return MUSIC_PLAYER_STOP_ERROR;
}

/* Returns true if a song is currently playing, else false */
bool music_player_is_song_playing() {
    return context.current_song == NO_SONG;
}

/* Returns music player error code */
enum music_player_error music_player_get_error() {
    return context.error;
}