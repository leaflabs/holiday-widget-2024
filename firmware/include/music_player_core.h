#ifndef __MUSIC_PLAYER_CORE_H__
#define __MUSIC_PLAYER_CORE_H__
#include "pam8302a_driver.h"
#include "tones.h"

/* Enumerated Music Player Errors */
enum music_player_error {
    MUSIC_PLAYER_NO_ERROR,
    MUSIC_PLAYER_INITIALIZATION_ERROR,
    MUSIC_PLAYER_DAC_DMA_ERROR,
    MUSIC_PLAYER_DURATIONS_DMA_ERROR,
    MUSIC_PLAYER_NOTES_DMA_ERROR,
    MUSIC_PLAYER_RUN_ERROR,
    MUSIC_PLAYER_STOP_ERROR
};

/* Enumerated Music Player States */
enum music_player_state {
    MUSIC_PLAYER_UNINITIALIZED,
    MUSIC_PLAYER_READY,
    MUSIC_PLAYER_BUSY,
    MUSIC_PLAYER_ERROR
};

/* Music Player Configuration Structure */
struct music_player_config {
    void (*duration_transfer_error_callback)(DMA_HandleTypeDef *hdma);
    void (*note_transfer_error_callback)(DMA_HandleTypeDef *hdma);
    void (*song_complete_callback)(DMA_HandleTypeDef *hdma);
    void (*dac_error_callback)(DAC_HandleTypeDef *hdac);
};

/* Music Player Context Structure */
struct music_player_context {
    volatile enum Song current_song;
    volatile enum music_player_error error;
    volatile enum music_player_state state;
};

/* Music Player Core Structure (Low Level)*/
struct music_player_core {
    /* TIM Handles */
    TIM_HandleTypeDef tim2_handle;
    TIM_HandleTypeDef tim3_handle;
    TIM_HandleTypeDef tim6_handle;

    /* DAC Handle */
    DAC_HandleTypeDef dac_handle;
};

/* Music Player Structure (High Level)*/
struct music_player {
    struct music_player_config *config;
    struct music_player_context *context;
    struct music_player_core core;
};

/* Initializes Music Player instance, return 0 on success, -1 on failure */
enum music_player_error music_player_core_init(
    struct music_player_config *config, struct music_player_context *context);

/* Plays the given song, returns 0 on success, -1 on failure */
enum music_player_error music_player_core_play_song(enum Song song);

/* Aborts the song playing process, returns 0 on success, -1 on failure */
enum music_player_error music_player_core_abort_song();

/* Singleton Music Player Instance (Not Enforced) */
extern struct music_player music_player;

#endif