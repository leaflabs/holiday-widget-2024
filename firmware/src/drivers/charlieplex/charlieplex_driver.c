#include "charlieplex_driver.h"

#include <stdbool.h>

#include "logging.h"
#include "stm32l072xx.h"
#include "utils.h"
// #pragma GCC push_options
// #pragma GCC optimize ("O3")   // Apply high optimization level
static const uint8_t default_leds[NUM_LEDS] = {0};

static volatile uint32_t t1, t2, duration;

struct charlieplex_driver_config {
    uint32_t prescaler;
    uint32_t refresh_rate;
    TIM_TypeDef *tim;
} __attribute__((aligned(4)));

struct charlieplex_driver_context {
    TIM_HandleTypeDef htim;
    volatile uint8_t *leds[2];
    volatile uint32_t moder_buffer[2][__NUM_LED_FRAMES];
    volatile uint8_t current_buffer;
    volatile bool update_requested;
    volatile enum led_frame current_frame;
} __attribute__((aligned(4)));

struct charlieplex_driver {
    const struct charlieplex_driver_config *config;
    struct charlieplex_driver_context *context;
} __attribute__((aligned(4)));

static const struct charlieplex_driver_config config = {
    .refresh_rate = 180U,
    .tim = TIM7,
};

static struct charlieplex_driver_context context = {
    .htim = {0},
    .leds =
        {
            default_leds,
            default_leds,
        },
    .current_buffer = 0,
    .update_requested = false,
    .current_frame = LED_FRAME_1,
};

static const struct charlieplex_driver charlieplex_driver_inst = {
    .config = &config,
    .context = &context,
};

const struct charlieplex_driver *const charlieplex_driver =
    &charlieplex_driver_inst;

/*
    List of all LEDs possible to turn on.

    The DEFAULT_LED takes up the 0th index to allow for
    writing 0 to have no effect. This is used to avoid checking
    if an led is on or not.

    Setting an LED requires setting the led's value in an array to
    itself. Ex: leds[D9] = D9;
*/

/*
    List every control line (GPIO pin)
    so we can index into an array with the value.
*/
enum controls {
    CTRL0 = 0x0,
    CTRL1 = 0x1,
    CTRL2 = 0x2,
    CTRL3 = 0x3,
    CTRL4 = 0x4,
    CTRL5 = 0x5,
    CTRL6 = 0x6,
    CTRL7 = 0x7,
    CTRL8 = 0x8,
    CTRL9 = 0x9,
    CTRL10 = 0xA,
    CTRL11 = 0xB,
    CTRL12 = 0xC,
    CTRL13 = 0xD,
    NO_CTRL = 0xE,
};

enum leds {
    DUMMY_LED,
    D67,
    D5,
    D9,
    D7,
    D57,
    D58,
    D83,

    D17,
    D71,
    D19,
    D18,
    D59,
    D24,
    D23,

    D75,
    D12,
    D13,
    D11,
    D25,
    D79,
    D26,

    D68,
    D6,
    D10,
    D8,
    D60,
    D61,
    D84,

    D20,
    D72,
    D22,
    D21,
    D62,
    D27,
    D80,

    D76,
    D15,
    D16,
    D14,
    D28,
    D32,
    D33,

    D29,
    D31,
    D30,
    D36,
    D37,
    D35,
    D34,
};

// Count of LEDs in the selected frame
static const uint8_t led_count[__NUM_LED_FRAMES] = {
    [LED_FRAME_1] = 11U, [LED_FRAME_2] = 11U, [LED_FRAME_3] = 11U,
    [LED_FRAME_4] = 8U,  [LED_FRAME_5] = 8U,
};

// LED and Control mapping arrays for ENABLE and DISABLE modes
static const enum controls mode[2][NUM_LEDS] = {
    [DISABLE] = {[0 ... NUM_LEDS - 1] =
                     NO_CTRL},  // Set all entries to NO_CTRL for DISABLE mode
    [ENABLE] = {
        [D67] = CTRL4,  [D5] = CTRL1,   [D9] = CTRL3,   [D7] = CTRL2,
        [D57] = CTRL0,  [D58] = CTRL1,  [D59] = CTRL2,  [D83] = CTRL3,
        [D17] = CTRL0,  [D71] = CTRL4,  [D19] = CTRL3,  [D18] = CTRL2,
        [D24] = CTRL1,  [D23] = CTRL0,  [D75] = CTRL4,  [D12] = CTRL1,
        [D13] = CTRL3,  [D11] = CTRL0,  [D25] = CTRL2,  [D79] = CTRL4,
        [D26] = CTRL5,  [D68] = CTRL9,  [D6] = CTRL6,   [D10] = CTRL8,
        [D8] = CTRL7,   [D60] = CTRL5,  [D61] = CTRL6,  [D84] = CTRL8,
        [D20] = CTRL5,  [D72] = CTRL9,  [D22] = CTRL8,  [D21] = CTRL7,
        [D62] = CTRL7,  [D27] = CTRL6,  [D80] = CTRL9,  [D76] = CTRL9,
        [D15] = CTRL6,  [D16] = CTRL8,  [D14] = CTRL5,  [D28] = CTRL7,
        [D32] = CTRL10, [D33] = CTRL12, [D29] = CTRL11, [D31] = CTRL13,
        [D30] = CTRL12, [D36] = CTRL11, [D37] = CTRL13, [D35] = CTRL10,
        [D34] = CTRL13,
    }};

// Frame LED mapping
static const enum leds frame_led_map[__NUM_LED_FRAMES][11] = {
    [LED_FRAME_1] = {D5, D7, D9, D67, D6, D8, D10, D68, D29, D30, D31},
    [LED_FRAME_2] = {D11, D12, D13, D75, D14, D15, D16, D76, D35, D36, D37},
    [LED_FRAME_3] = {D17, D18, D19, D71, D20, D21, D22, D72, D32, D33, D34},
    [LED_FRAME_4] = {D23, D24, D25, D79, D26, D27, D28, D80},
    [LED_FRAME_5] = {D57, D58, D59, D83, D60, D61, D62, D84},
};

// Control pin and mode mappings
static const uint32_t ctrl_pin_map[2][15] = {
    [DISABLE] = {[CTRL0... CTRL13] = 0U, [NO_CTRL] = 0U},
    [ENABLE] = {[CTRL0] = GPIO_PIN_0,
                [CTRL1] = GPIO_PIN_1,
                [CTRL2] = GPIO_PIN_2,
                [CTRL3] = GPIO_PIN_3,
                [CTRL4] = GPIO_PIN_4,
                [CTRL5] = GPIO_PIN_5,
                [CTRL6] = GPIO_PIN_6,
                [CTRL7] = GPIO_PIN_7,
                [CTRL8] = GPIO_PIN_8,
                [CTRL9] = GPIO_PIN_9,
                [CTRL10] = GPIO_PIN_10,
                [CTRL11] = GPIO_PIN_11,
                [CTRL12] = GPIO_PIN_12,
                [CTRL13] = GPIO_PIN_13,
                [NO_CTRL] = 0U}};

// Control mode mapping for MODER registers
static const uint32_t ctrl_output_mode_map[] = {
    [CTRL0] = 0x1,        [CTRL1] = 0x4,        [CTRL2] = 0x10,
    [CTRL3] = 0x40,       [CTRL4] = 0x100,      [CTRL5] = 0x400,
    [CTRL6] = 0x1000,     [CTRL7] = 0x4000,     [CTRL8] = 0x10000,
    [CTRL9] = 0x40000,    [CTRL10] = 0x100000,  [CTRL11] = 0x400000,
    [CTRL12] = 0x1000000, [CTRL13] = 0x4000000, [NO_CTRL] = 0x0};

// Frame control MODER and ODR values
static const uint32_t led_frame_ctrl_moder[] = {
    [LED_FRAME_1] = (ctrl_output_mode_map[CTRL0] | ctrl_output_mode_map[CTRL5] |
                     ctrl_output_mode_map[CTRL10]),
    [LED_FRAME_2] = (ctrl_output_mode_map[CTRL2] | ctrl_output_mode_map[CTRL7] |
                     ctrl_output_mode_map[CTRL12]),
    [LED_FRAME_3] = (ctrl_output_mode_map[CTRL1] | ctrl_output_mode_map[CTRL6] |
                     ctrl_output_mode_map[CTRL11]),
    [LED_FRAME_4] = (ctrl_output_mode_map[CTRL3] | ctrl_output_mode_map[CTRL8] |
                     ctrl_output_mode_map[CTRL13]),
    [LED_FRAME_5] = (ctrl_output_mode_map[CTRL4] | ctrl_output_mode_map[CTRL9]),
};

static const uint32_t led_frame_ctrl_odr[] = {
    [LED_FRAME_1] = (ctrl_pin_map[ENABLE][CTRL0] | ctrl_pin_map[ENABLE][CTRL5] |
                     ctrl_pin_map[ENABLE][CTRL10]),
    [LED_FRAME_2] = (ctrl_pin_map[ENABLE][CTRL2] | ctrl_pin_map[ENABLE][CTRL7] |
                     ctrl_pin_map[ENABLE][CTRL12]),
    [LED_FRAME_3] = (ctrl_pin_map[ENABLE][CTRL1] | ctrl_pin_map[ENABLE][CTRL6] |
                     ctrl_pin_map[ENABLE][CTRL11]),
    [LED_FRAME_4] = (ctrl_pin_map[ENABLE][CTRL3] | ctrl_pin_map[ENABLE][CTRL8] |
                     ctrl_pin_map[ENABLE][CTRL13]),
    [LED_FRAME_5] = (ctrl_pin_map[ENABLE][CTRL4] | ctrl_pin_map[ENABLE][CTRL9]),
};

static const uint32_t low[NUM_LEDS] = {
    [DUMMY_LED] = NO_CTRL, [D67] = CTRL4,  [D5] = CTRL1,   [D9] = CTRL3,
    [D7] = CTRL2,          [D57] = CTRL0,  [D58] = CTRL1,  [D59] = CTRL2,
    [D83] = CTRL3,         [D17] = CTRL0,  [D71] = CTRL4,  [D19] = CTRL3,
    [D18] = CTRL2,         [D24] = CTRL1,  [D23] = CTRL0,  [D75] = CTRL4,
    [D12] = CTRL1,         [D13] = CTRL3,  [D11] = CTRL0,  [D25] = CTRL2,
    [D79] = CTRL4,         [D26] = CTRL5,  [D68] = CTRL9,  [D6] = CTRL6,
    [D10] = CTRL8,         [D8] = CTRL7,   [D60] = CTRL5,  [D61] = CTRL6,
    [D84] = CTRL8,         [D20] = CTRL5,  [D72] = CTRL9,  [D22] = CTRL8,
    [D21] = CTRL7,         [D62] = CTRL7,  [D27] = CTRL6,  [D80] = CTRL9,
    [D76] = CTRL9,         [D15] = CTRL6,  [D16] = CTRL8,  [D14] = CTRL5,
    [D28] = CTRL7,         [D32] = CTRL10, [D33] = CTRL12, [D29] = CTRL11,
    [D31] = CTRL13,        [D30] = CTRL12, [D36] = CTRL11, [D37] = CTRL13,
    [D35] = CTRL10,        [D34] = CTRL13,
};

static const uint32_t low_moder[NUM_LEDS] = {
    [DUMMY_LED] = ctrl_output_mode_map[low[DUMMY_LED]],
    [D67] = ctrl_output_mode_map[low[D67]],
    [D5] = ctrl_output_mode_map[low[D5]],
    [D9] = ctrl_output_mode_map[low[D9]],
    [D7] = ctrl_output_mode_map[low[D7]],
    [D57] = ctrl_output_mode_map[low[D57]],
    [D58] = ctrl_output_mode_map[low[D58]],
    [D59] = ctrl_output_mode_map[low[D59]],
    [D83] = ctrl_output_mode_map[low[D83]],
    [D17] = ctrl_output_mode_map[low[D17]],
    [D71] = ctrl_output_mode_map[low[D71]],
    [D19] = ctrl_output_mode_map[low[D19]],
    [D18] = ctrl_output_mode_map[low[D18]],
    [D24] = ctrl_output_mode_map[low[D24]],
    [D23] = ctrl_output_mode_map[low[D23]],
    [D75] = ctrl_output_mode_map[low[D75]],
    [D12] = ctrl_output_mode_map[low[D12]],
    [D13] = ctrl_output_mode_map[low[D13]],
    [D11] = ctrl_output_mode_map[low[D11]],
    [D25] = ctrl_output_mode_map[low[D25]],
    [D79] = ctrl_output_mode_map[low[D79]],
    [D26] = ctrl_output_mode_map[low[D26]],
    [D68] = ctrl_output_mode_map[low[D68]],
    [D6] = ctrl_output_mode_map[low[D6]],
    [D10] = ctrl_output_mode_map[low[D10]],
    [D8] = ctrl_output_mode_map[low[D8]],
    [D60] = ctrl_output_mode_map[low[D60]],
    [D61] = ctrl_output_mode_map[low[D61]],
    [D84] = ctrl_output_mode_map[low[D84]],
    [D20] = ctrl_output_mode_map[low[D20]],
    [D72] = ctrl_output_mode_map[low[D72]],
    [D22] = ctrl_output_mode_map[low[D22]],
    [D21] = ctrl_output_mode_map[low[D21]],
    [D62] = ctrl_output_mode_map[low[D62]],
    [D27] = ctrl_output_mode_map[low[D27]],
    [D80] = ctrl_output_mode_map[low[D80]],
    [D76] = ctrl_output_mode_map[low[D76]],
    [D15] = ctrl_output_mode_map[low[D15]],
    [D16] = ctrl_output_mode_map[low[D16]],
    [D14] = ctrl_output_mode_map[low[D14]],
    [D28] = ctrl_output_mode_map[low[D28]],
    [D32] = ctrl_output_mode_map[low[D32]],
    [D33] = ctrl_output_mode_map[low[D33]],
    [D29] = ctrl_output_mode_map[low[D29]],
    [D31] = ctrl_output_mode_map[low[D31]],
    [D30] = ctrl_output_mode_map[low[D30]],
    [D36] = ctrl_output_mode_map[low[D36]],
    [D37] = ctrl_output_mode_map[low[D37]],
    [D35] = ctrl_output_mode_map[low[D35]],
    [D34] = ctrl_output_mode_map[low[D34]],
};

static void charlieplex_driver_prepare_frames(uint8_t *leds) {
    struct charlieplex_driver_context *context = charlieplex_driver->context;
    for (int led_frame = 0; led_frame < __NUM_LED_FRAMES; led_frame++) {
        uint32_t moder = led_frame_ctrl_moder[led_frame];
        enum leds *frame_leds = frame_led_map[led_frame];
        for (int i = 0; i < led_count[led_frame]; i++) {
            moder |= low_moder
                [leds[frame_leds
                          [i]]];  // ctrl_output_mode_map[mode[leds[led_idx]][led_idx]];
        }
        context->moder_buffer[context->current_buffer ^ 1U][led_frame] = moder;
    }
}

/*
    Draw a 'frame' of leds.

    'leds' is a pointer to an array of 50 elements. Each slot corrosponds to
    the enum 'leds' defined above.
*/
void charlieplex_driver_draw(uint8_t *leds) {
    struct charlieplex_driver_context *context = charlieplex_driver->context;
    // context->leds[context->current_buffer ^ 1U] = leds;
    charlieplex_driver_prepare_frames(leds);
    context->update_requested = true;
}

static inline void charlieplex_driver_draw_frame(uint8_t *leds,
                                                 enum led_frame led_frame) {
    struct charlieplex_driver_context *context = charlieplex_driver->context;
    GPIOC->ODR = 0U;
    GPIOC->MODER = context->moder_buffer[context->current_buffer][led_frame];
    GPIOC->ODR = led_frame_ctrl_odr[led_frame];
    /*
    GPIOC->ODR = 0U;
    uint32_t moder = led_frame_ctrl_moder[led_frame];
    enum leds *frame_leds = frame_led_map[led_frame];
    for (int i = 0; i < led_count[led_frame]; i++) {
        uint8_t led_idx = frame_leds[i];
        moder |= low_moder[leds[led_idx]];  //
    ctrl_output_mode_map[mode[leds[led_idx]][led_idx]];
    }
    GPIOC->MODER = moder;
    GPIOC->ODR = led_frame_ctrl_odr[led_frame];
    */
}

static int tim7_init(struct charlieplex_driver *charlieplex_driver) {
    const struct charlieplex_driver_config *cfg = charlieplex_driver->config;
    struct charlieplex_driver_context *context = charlieplex_driver->context;

    /* TIM7 Peripheral Clock Enable */
    __HAL_RCC_TIM7_CLK_ENABLE();

    TIM_CONFIGURE_UPDATE_FREQUENCY(&context->htim, cfg->refresh_rate);

    context->htim.Instance = cfg->tim;
    context->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    context->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&context->htim) != HAL_OK) {
        /* TIM7 Initialization Error */
        return -1;
    }
    LOG_INF("Desired frequency: %x", cfg->refresh_rate);
    LOG_INF("Actual frequency: %d",
            TIM_GET_ACTUAL_UPDATE_FREQUENCY(&context->htim));

    TIM_ClockConfigTypeDef clock_source_config = {0};
    clock_source_config.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&context->htim, &clock_source_config)) {
        /* TIM7 Clock Source Configuration Error */
        return -1;
    }

    /* Enable interrupt on update */
    TIM7->DIER = TIM_DIER_UIE;
    HAL_NVIC_SetPriority(TIM7_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(TIM7_IRQn);

    return 0;
}

static void gpio_init() {
    // Enable clocks
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Configure all pins on Port C as input (high impedance)
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 |
               GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |
               GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);
}

// Initializes the Charlieplex driver
void charlieplex_driver_init(void) {
    struct charlieplex_driver_context *context = charlieplex_driver->context;
    int ret;

    gpio_init();
    ret = tim7_init(charlieplex_driver);
    if (ret != 0) {
        LOG_ERR("Failed to initialize TIM7: %d", ret);
    }

    ret = HAL_TIM_Base_Start(&context->htim);
    if (ret != 0) {
        LOG_ERR("Failed to start TIM7: %d", ret);
    }
}
// #pragma GCC push_options
// #pragma GCC optimize ("O1")   // Apply high optimization level
void TIM7_IRQHandler(void) {
    /*static int i = 0;
    if (i == 0) {
        t1 = HAL_GetTick();
    }*/
    // LOG_INF("HERE1");
    struct charlieplex_driver_context *context = charlieplex_driver->context;
    static uint8_t stage = 0;
    static const enum led_frame next_frame[] = {
        [LED_FRAME_1] = LED_FRAME_2, [LED_FRAME_2] = LED_FRAME_3,
        [LED_FRAME_3] = LED_FRAME_4, [LED_FRAME_4] = LED_FRAME_5,
        [LED_FRAME_5] = LED_FRAME_1,
    };
    /*
    // Check if the update interrupt flag is set
    if (__HAL_TIM_GET_FLAG(&context->htim, TIM_FLAG_UPDATE) != RESET) {
        // Check if the update interrupt is enabled
        if (__HAL_TIM_GET_IT_SOURCE(&context->htim, TIM_IT_UPDATE) != RESET) {*/
    // Clear the update interrupt flag
    __HAL_TIM_CLEAR_IT(&context->htim, TIM_IT_UPDATE);
    // LOG_INF("HERE2");
    // static uint8_t count[10];
    // static uint8_t idx = 0;
    // count[idx]++;
    if (context->update_requested) {
        /*idx++;
        if (idx == 10) {
            idx = 0;
            for (int i = 0; i < 10; i++) {
                LOG_INF("Count[%d]: %d", i, count[i]);
                count[i] = 0;
            }
        }*/
        context->current_buffer ^= 1U;
        // context->current_frame = LED_FRAME_1;
        context->update_requested = false;
        // LOG_INF("HERE3");
    }

    /* Write frame every other update to reduce current draw */
    if (stage < 2) {
        GPIOC->MODER = 0U;
        GPIOC->ODR = 0U;
        stage++;
    } else {
        charlieplex_driver_draw_frame(context->leds[context->current_buffer],
                                      context->current_frame);
        // LOG_INF("HERE4");
        context->current_frame = next_frame[context->current_frame];
        stage = 0;
    }
    // LOG_INF("HERE5");
    /*}
}*/
    /*if (i >= 9) {
        t2 = HAL_GetTick();
        duration = t2 - t1;
        LOG_INF("Duration: %1.3fms", (float)duration/10.0f);
        i = 0;
    } else {
        i++;
    }*/
    // LOG_INF("HERE2");
    // asm(" nop");
}
// #pragma GCC pop_options       // Revert to previous optimization level

void pause_charlieplex_driver(void) {
    struct charlieplex_driver_context *context = charlieplex_driver->context;
    int ret = HAL_TIM_Base_Stop(&context->htim);
    if (ret != 0) {
        LOG_ERR("Failed to stop TIM7: %d", ret);
    }
    GPIOC->MODER = 0U;
    GPIOC->ODR = 0U;
}

void unpause_charlieplex_driver(void) {
    struct charlieplex_driver_context *context = charlieplex_driver->context;
    int ret = HAL_TIM_Base_Start(&context->htim);
    if (ret != 0) {
        LOG_ERR("Failed to start TIM7: %d", ret);
    }
}

void get_duration() {
    // LOG_INF("Duration: %1.3f", ((float)duration)/10.0f);
}
// #pragma GCC pop_options       // Revert to previous optimization level