#include "widget_system.h"

#include "stm32l0xx_hal.h"

/*
 * Basic system clock initalization code
 *
 */
static void widget_system_clock_init(void) {
    RCC_OscInitTypeDef rcc_osc_init = {0};
    RCC_ClkInitTypeDef rcc_clk_init = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    /* Enable MSI Oscillator */
    rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    rcc_osc_init.MSIState = RCC_MSI_ON;
    rcc_osc_init.MSICalibrationValue = 0;
    rcc_osc_init.MSIClockRange = RCC_MSIRANGE_5;
    rcc_osc_init.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&rcc_osc_init) != HAL_OK) {
        // error message here
    }

    /*
     * Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     * clocks' dividers.
     */
    rcc_clk_init.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                              RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);

    rcc_clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV2;
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV1;
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_0) != HAL_OK) {
        // Error message here
    }
}

/*
 * Initalize everything for the status led GPIO pins
 *
 */
static void widget_status_led_setup(void) {
    // Enable the GPIOC clock
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    // Set up LED Pins (8-11);
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(GPIOC, &gpio);
}

void widget_system_init(void) {
    // Init the HAL
    HAL_Init();

    // Init the system clock
    widget_system_clock_init();

    // Set up the GPIO pins for the leds
    widget_status_led_setup();
}
