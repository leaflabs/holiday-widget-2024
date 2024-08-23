#include "music_player.h"

#include "logging.h"
#include "utils.h"

#define GEN_ERROR_STRING(error) [error] = #error

static const char *const error_strings[] = {
    GEN_ERROR_STRING(MUSIC_PLAYER_NO_ERROR),
    GEN_ERROR_STRING(MUSIC_PLAYER_INITIALIZATION_ERROR),
    GEN_ERROR_STRING(MUSIC_PLAYER_DAC_DMA_ERROR),
    GEN_ERROR_STRING(MUSIC_PLAYER_DURATIONS_DMA_ERROR),
    GEN_ERROR_STRING(MUSIC_PLAYER_NOTES_DMA_ERROR),
    GEN_ERROR_STRING(MUSIC_PLAYER_RUN_ERROR),
    GEN_ERROR_STRING(MUSIC_PLAYER_STOP_ERROR),
};

/* Global Music Player Instance */
struct music_player music_player = (struct music_player){
    .config =
        (struct music_player_config){
            .audio_out_pin = GPIO_PIN(A, 4),
            .pam8302a_driver = &pam8302a_driver,
        },
    .context =
        (struct music_player_context){
            .current_song = NO_SONG,
            .error = MUSIC_PLAYER_NO_ERROR,
            .state = MUSIC_PLAYER_UNINITIALIZED,
        },
};

/* Set up the music player */
void music_player_setup(void) {
    struct music_player_context *context = &music_player.context;

    if ((context->error = music_player_core_init(&music_player)) != 0) {
        context->state = MUSIC_PLAYER_ERROR;
    } else {
        context->state = MUSIC_PLAYER_READY;
    }
    context->current_song = NO_SONG;
}

/* Run through the music player's state machine */
void music_player_run(void) {
    struct music_player_context *context = &music_player.context;
    switch (context->state) {
        case MUSIC_PLAYER_UNINITIALIZED:
            /* Music Player Uninitialized */
            LOG_ERR("Music Player not initialized properly");
            break;
        case MUSIC_PLAYER_READY:
            /* Music Player Ready */
            if (context->current_song != NO_SONG) {
                if ((context->error = music_player_core_play_song(
                         &music_player, context->current_song))) {
                    context->state = MUSIC_PLAYER_ERROR;
                } else {
                    context->state = MUSIC_PLAYER_BUSY;
                }
            }
            break;
        case MUSIC_PLAYER_BUSY:
            /* Music Player Busy */
            if (context->current_song == NO_SONG) {
                if ((context->error =
                         music_player_core_abort_song(&music_player))) {
                    context->state = MUSIC_PLAYER_ERROR;
                } else {
                    context->state = MUSIC_PLAYER_READY;
                }
            }
            break;
        case MUSIC_PLAYER_ERROR:
            /* Music Player Error */
            switch (context->error) {
                case MUSIC_PLAYER_NO_ERROR:             /* fallthrough */
                case MUSIC_PLAYER_INITIALIZATION_ERROR: /* fallthrough */
                case MUSIC_PLAYER_DAC_DMA_ERROR:        /* fallthrough */
                case MUSIC_PLAYER_DURATIONS_DMA_ERROR:  /* fallthrough */
                case MUSIC_PLAYER_NOTES_DMA_ERROR:      /* fallthrough */
                case MUSIC_PLAYER_RUN_ERROR:            /* fallthrough */
                case MUSIC_PLAYER_STOP_ERROR:
                    LOG_ERR("%s", error_strings[context->error]);
                    break;
                default:
                    LOG_ERR("Unknown Music Player Error:%d", context->error);
                    break;
            }
            context->state = MUSIC_PLAYER_READY;
            break;
        default:
            /* Switching over enumerated type so you should never be here */
            LOG_ERR("Unsupported Music Player State: %d", context->state);
            break;
    }
}

/* Play the provided song - returns MUSIC_PLAYER_RUN_ERROR on failure */
enum music_player_error music_player_play_song(
    struct music_player *music_player, enum Song song) {
    if (music_player->context.state == MUSIC_PLAYER_READY) {
        music_player->context.current_song = song;
        return MUSIC_PLAYER_NO_ERROR;
    }
    return MUSIC_PLAYER_RUN_ERROR;
}

/* Stop the current song - returns MUSIC_PLAYER_STOP_ERROR on failure */
enum music_player_error music_player_abort_song(
    struct music_player *music_player) {
    if (music_player->context.state == MUSIC_PLAYER_BUSY) {
        music_player->context.current_song = NO_SONG;
        return MUSIC_PLAYER_NO_ERROR;
    }

    return MUSIC_PLAYER_STOP_ERROR;
}

/* Returns true if a song is currently playing, else false */
bool music_player_is_song_playing(struct music_player *music_player) {
    return music_player->context.current_song != NO_SONG;
}

/* Returns music player error code */
enum music_player_error music_player_get_error(
    struct music_player *music_player) {
    return music_player->context.error;
}