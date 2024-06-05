#include "stm32l0xx_hal.h"

void HAL_MspInit(void) {
    // Enable the SYS clock and PWR clock
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_UART_MspInit(UART_HandleTypeDef *uart) {
    (void)uart;

    // Now set up the GPIO pins for uart, which are PA2 and PA3

    // Init the GPIOA and USART2 clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    // Set up both pins
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio.Alternate = GPIO_AF4_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uart) {
    (void)uart;

    __HAL_RCC_USART2_CLK_DISABLE();

    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2 | GPIO_PIN_3);
}
