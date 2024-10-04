#ifndef __GENERIC_GPIO_H__
#define __GENERIC_GPIO_H__
#include "stm32l072xx.h"

struct gpio_pin_config {
    GPIO_TypeDef *port;
    uint16_t pin;
};

#define GPIO_PIN(bank, number)                       \
    (const struct gpio_pin_config) {                 \
        .port = GPIO##bank, .pin = GPIO_PIN_##number \
    }

#define GPIOx_CLK_ENABLE(__GPIOx__)           \
    do {                                      \
        switch ((uint32_t)__GPIOx__) {        \
            case GPIOA_BASE:                  \
                __HAL_RCC_GPIOA_CLK_ENABLE(); \
                break;                        \
            case GPIOB_BASE:                  \
                __HAL_RCC_GPIOB_CLK_ENABLE(); \
                break;                        \
            case GPIOC_BASE:                  \
                __HAL_RCC_GPIOC_CLK_ENABLE(); \
                break;                        \
            case GPIOD_BASE:                  \
                __HAL_RCC_GPIOD_CLK_ENABLE(); \
                break;                        \
            case GPIOE_BASE:                  \
                __HAL_RCC_GPIOE_CLK_ENABLE(); \
                break;                        \
            case GPIOH_BASE:                  \
                __HAL_RCC_GPIOH_CLK_ENABLE(); \
                break;                        \
            default:                          \
                break;                        \
        }                                     \
    } while (0)

#endif