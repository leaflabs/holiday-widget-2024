#ifndef __FFT_GAME_H__
#define __FFT_GAME_H__
#include "game_common.h"
#include "game_entity.h"
#include "physics_engine.h"

/* Size of the FFT */
#define FFT_SIZE (64U)

#if !IS_POWER_OF_2(FFT_SIZE)
#error "FFT_SIZE must be a power of 2"
#endif

/**************************/
/* Histogram Bar Settings */
/**************************/

#define FFT_GAME_NUM_OF_HISTOGRAM_BARS 7

static const position
    HISTOGRAM_BAR_START_POSITIONS_TOP_LEFT[FFT_GAME_NUM_OF_HISTOGRAM_BARS] = {
        TOP_LEFT_POSITION_FROM_GRID(0, 0), TOP_LEFT_POSITION_FROM_GRID(1, 0),
        TOP_LEFT_POSITION_FROM_GRID(2, 0), TOP_LEFT_POSITION_FROM_GRID(3, 0),
        TOP_LEFT_POSITION_FROM_GRID(4, 0), TOP_LEFT_POSITION_FROM_GRID(5, 0),
        TOP_LEFT_POSITION_FROM_GRID(6, 0),
};

static const position
    HISTOGRAM_BAR_START_POSITIONS_BOTTOM_RIGHT[FFT_GAME_NUM_OF_HISTOGRAM_BARS] =
        {
            BOTTOM_RIGHT_POSITION_FROM_GRID(0, 6),
            BOTTOM_RIGHT_POSITION_FROM_GRID(1, 6),
            BOTTOM_RIGHT_POSITION_FROM_GRID(2, 6),
            BOTTOM_RIGHT_POSITION_FROM_GRID(3, 6),
            BOTTOM_RIGHT_POSITION_FROM_GRID(4, 6),
            BOTTOM_RIGHT_POSITION_FROM_GRID(5, 6),
            BOTTOM_RIGHT_POSITION_FROM_GRID(6, 6),
};

/* Start velocity of histogram bars */
#define FFT_HISTOGRAM_BAR_START_VELOCITY \
    (velocity) {                         \
        0, 0                             \
    }

/* Start acceleration of histogram bars */
#define FFT_HISTOGRAM_BAR_START_ACCELERATION \
    (acceleration) {                         \
        0, 0                                 \
    }

/* Is histogram bar solid */
#define FFT_HISTOGRAM_BAR_SOLID true

/* Enemy ship sprite */
#define FFT_HISTOGRAM_BAR_SPRITE (&histogram_bar)

struct fft_game_config {
    /* Histogram bar initialization struct */
    const struct entity_init_struct *const histogram_bar_init_struct;
};

struct fft_game_context {
    struct game_common game_common;
    union {
        struct {
            struct game_entity histogram_bars[FFT_GAME_NUM_OF_HISTOGRAM_BARS];
        };
        struct game_entity game_entities[FFT_GAME_NUM_OF_HISTOGRAM_BARS];
    };
};

struct fft_game {
    const struct fft_game_config config;
    struct fft_game_context context;
};

enum entity_creation_error fft_game_init(struct fft_game *fft_game);

void fft_game_update(struct fft_game *fft_game);

static const struct entity_init_struct fft_histogram_bar_init_struct = {
    .rectangle =
        (struct rectangle){HISTOGRAM_BAR_START_POSITIONS_TOP_LEFT[0],
                           HISTOGRAM_BAR_START_POSITIONS_BOTTOM_RIGHT[0]},
    .mass = INFINITE_MASS,
    .velocity = FFT_HISTOGRAM_BAR_START_VELOCITY,
    .acceleration = FFT_HISTOGRAM_BAR_START_ACCELERATION,
    .solid = FFT_HISTOGRAM_BAR_SOLID,
};

#define CREATE_FFT_GAME()                                                    \
    (struct fft_game) {                                                      \
        .config =                                                            \
            {                                                                \
                .histogram_bar_init_struct = &fft_histogram_bar_init_struct, \
            },                                                               \
        .context = {0},                                                      \
    }

#endif /*__FFT_GAME_H__*/