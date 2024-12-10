#include "fft_game.h"

#include "arm_math.h"
#include "imp23absu_driver.h"

#define HISTOGRAM_AMPLITUDE_COEFFICIENT 8.0f

enum entity_creation_error fft_game_init(struct fft_game *fft_game) {
    const struct fft_game_config *config = &fft_game->config;
    struct fft_game_context *context = &fft_game->context;

    game_common_init(&context->game_common);

    /******************/
    /*  Add Entities  */
    /******************/
    struct entity_creation_result result;

    /* Add histogram bar entities */
    for (int i = 0; i < FFT_GAME_NUM_OF_HISTOGRAM_BARS; i++) {
        result = add_entity(&context->game_common.environment,
                            config->histogram_bar_init_struct);
        if (result.error != ENTITY_CREATION_SUCCESS) {
            LOG_ERR("Failed to create histogram bar entity %d: %d", i,
                    result.error);
            return result.error;
        } else {
            bool ret =
                game_entity_init(&context->histogram_bars[i], result.entity,
                                 FFT_HISTOGRAM_BAR_SPRITE);
            if (!ret) {
                LOG_ERR("Failed to create histogram bar game entity %d", i);
                result.error = ENTITY_CREATION_INVALID_TYPE;
                return result.error;
            }
        }
        set_game_entity_position(&context->histogram_bars[i],
                                 HISTOGRAM_BAR_START_POSITIONS_TOP_LEFT[i]);
    }

    /******************/
    /* Initialize FFT */
    /******************/
    // async_fft_init(&context->async_fft, NULL);

    for (int i = 0; i < FFT_GAME_NUM_OF_HISTOGRAM_BARS; i++) {
        activate_game_entity(&context->histogram_bars[i]);
    }

    imp23absu_driver_init();

    return ENTITY_CREATION_SUCCESS;
}

/* Hanning window for reducing spectral leakage */
static const q15_t hanning_window_64[64] = {
    0x0000, 0x0051, 0x0145, 0x02D8, 0x0507, 0x07CC, 0x0B20, 0x0EFB,
    0x1353, 0x181C, 0x1D4B, 0x22D2, 0x28A4, 0x2EB1, 0x34EA, 0x3B40,
    0x41A2, 0x4800, 0x4E49, 0x546E, 0x5A60, 0x600E, 0x656B, 0x6A69,
    0x6EFB, 0x7315, 0x76AE, 0x79BB, 0x7C36, 0x7E18, 0x7F5C, 0x7FFF,
    0x7FFF, 0x7F5C, 0x7E18, 0x7C36, 0x79BB, 0x76AE, 0x7315, 0x6EFB,
    0x6A69, 0x656B, 0x600E, 0x5A60, 0x546E, 0x4E49, 0x4800, 0x41A2,
    0x3B40, 0x34EA, 0x2EB1, 0x28A4, 0x22D2, 0x1D4B, 0x181C, 0x1353,
    0x0EFB, 0x0B20, 0x07CC, 0x0507, 0x02D8, 0x0145, 0x0051, 0x0000};

/* Function to convert 12-bit ADC value to q15 format */
static inline q15_t convert_12bit_to_q15(int16_t value) {
    /*
     * Scale the 12-bit value to 16-bit and adjust
     * to q15_t format by centering around zero
     */
    return (q15_t)(((int32_t)(value << 4)) - 32768);
}

extern const arm_cfft_instance_q15 arm_cfft_sR_q15_len64;
void fft_game_update(struct fft_game *fft_game) {
    static uint16_t raw[FFT_SIZE];  // Raw 12-bit ADC data
    static q15_t input[FFT_SIZE];   // Input array (real data as q15_t)
    static q15_t
        fft_input[FFT_SIZE * 2];  // FFT input array (complex data: real and
                                  // imaginary parts interleaved)
    static q15_t magnitude[FFT_SIZE];  // Magnitude of the FFT (half-spectrum
                                       // for real input)
    const float sample_frequency = imp23absu_get_sample_frequency();

    struct fft_game_context *context = &fft_game->context;

    /* Read FFT_SIZE values from the microphone */
    int values_read = imp23absu_driver_read(raw, FFT_SIZE);
    while (values_read < FFT_SIZE) {
        values_read +=
            imp23absu_driver(&raw[values_read], FFT_SIZE - values_read);
    }

    /* Convert raw 12-bit ADC values to q15_t */
    for (int i = 0; i < FFT_SIZE; i++) {
        input[i] = convert_12bit_to_q15(raw[i]);
    }

    /*
        Calculate the mean and offset the input to
        center around 0 and remove DC component.
    */
    q15_t mean;
    arm_mean_q15(input, FFT_SIZE, &mean);
    arm_offset_q15(input, -mean, input, FFT_SIZE);

    /* Scale input value up to use closer to the full range of values */
    arm_shift_q15(input, 5, input, FFT_SIZE);

    /* Apply hanning window to reduce spectral leakage */
    arm_mult_q15(input, hanning_window_64, input, FFT_SIZE);

    /* Interleave input data with zeroes  */
    for (int i = 0; i < FFT_SIZE; i++) {
        int idx = i << 1;
        fft_input[idx] = input[i];
        fft_input[idx + 1] = 0;
    }

    /* Perform FFT */
    arm_cfft_q15(&arm_cfft_sR_q15_len64, fft_input, 0, 1);

    /* Calculate magnitudes */
    arm_cmplx_mag_q15(fft_input, magnitude, FFT_SIZE * 2);

    q15_t max_value;
    uint32_t max_bin;

    /* Determine bin with largest magnitude */
    arm_max_q15(magnitude, FFT_SIZE / 2, &max_value, &max_bin);

    /* Calculate fundamental frequency */
    uint32_t frequency = (max_bin * sample_frequency) / FFT_SIZE;

    /* Calculate magnitude of up to six harmonics (within nyquist limit) */
    uint32_t harmonics[6] = {0, 0, 0, 0, 0, 0};
    uint8_t harmonics_rendered = 0;
    for (int i = 0; i < 6; i++) {
        uint32_t harmonic = (frequency * (i + 2) * FFT_SIZE) / sample_frequency;
        if (harmonic > (FFT_SIZE / 2) - 1) {
            /*
                Break when the index of the harmonic exceeds half of the FFT
               size i.e. when the frequency of the harmonic exceeds the nyquist
               limit
            */
            break;
        } else {
            harmonics[i] = magnitude[harmonic];
            harmonics_rendered++;
        }
    }

    /* Set first bar height proporitional to magnitude of fundamental frequency
     */
    context->histogram_bars[0].sprite.x = 0;
    context->histogram_bars[0].sprite.y =
        GRID_SIZE -
        ((((float)max_value * HISTOGRAM_AMPLITUDE_COEFFICIENT * GRID_SIZE) /
          (float)INT16_MAX)) -
        1;

    /* Set remaining 6 bar heights proporitional to magnitude of harmonic
     * partials */
    for (int i = 1; i < FFT_GAME_NUM_OF_HISTOGRAM_BARS; i++) {
        context->histogram_bars[i].sprite.x = i;
        context->histogram_bars[i].sprite.y =
            GRID_SIZE -
            ((((float)harmonics[i - 1] * HISTOGRAM_AMPLITUDE_COEFFICIENT *
               GRID_SIZE) /
              (float)INT16_MAX)) -
            1;
    }
}