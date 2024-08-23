#ifndef __UTILS_H
#define __UTILS_H

#include "logging.h"
#include "stm32l0xx_hal.h"

#define CLAMP(val, min, max) \
    ((val) < (min) ? (min) : ((val) > (max) ? (max) : (val)))

#define CONTAINER_OF(ptr, type, field) \
    ((type *)(((char *)(ptr)) - offsetof(type, field)))

// Macro to ensure capacity is a power of 2
#define IS_POWER_OF_2(x) (((x) & ((x) - 1)) == 0)

#define TIM_CONFIGURE_UPDATE_FREQUENCY(__htim__, __freq__)       \
    do {                                                         \
        uint32_t pclk_freq = HAL_RCC_GetPCLK1Freq();             \
        uint32_t prescaler = 0U;                                 \
        uint32_t period = 0U;                                    \
        uint32_t clock_div = TIM_CLOCKDIVISION_DIV1;             \
                                                                 \
        /* Try different clock divisions (DIV1, DIV2, DIV4) */   \
        if ((pclk_freq / (__freq__ * 65536)) < 1U) {             \
            clock_div = TIM_CLOCKDIVISION_DIV4;                  \
            pclk_freq /= 4;                                      \
        } else if ((pclk_freq / (__freq__ * 65536)) < 2U) {      \
            clock_div = TIM_CLOCKDIVISION_DIV2;                  \
            pclk_freq /= 2;                                      \
        }                                                        \
                                                                 \
        /* Calculate prescaler and period */                     \
        prescaler = (pclk_freq / __freq__) / 65536;              \
        period = (pclk_freq / (__freq__ * (prescaler + 1))) - 1; \
                                                                 \
        /* Populate the TIM handle with calculated values */     \
        (__htim__)->Init.Prescaler = prescaler;                  \
        (__htim__)->Init.Period = period;                        \
        (__htim__)->Init.ClockDivision = clock_div;              \
                                                                 \
    } while (0)

#define TIM_GET_ACTUAL_UPDATE_FREQUENCY(htim)                               \
    ({                                                                      \
        uint32_t pclk_freq = HAL_RCC_GetPCLK1Freq();                        \
        uint32_t clock_div_factor = 1U;                                     \
                                                                            \
        /* Determine clock division factor */                               \
        if ((htim)->Init.ClockDivision == TIM_CLOCKDIVISION_DIV2) {         \
            clock_div_factor = 2U;                                          \
        } else if ((htim)->Init.ClockDivision == TIM_CLOCKDIVISION_DIV4) {  \
            clock_div_factor = 4U;                                          \
        }                                                                   \
                                                                            \
        /* Calculate actual frequency */                                    \
        uint32_t actual_freq =                                              \
            pclk_freq / (clock_div_factor * ((htim)->Init.Prescaler + 1U) * \
                         ((htim)->Init.Period + 1U));                       \
        actual_freq;                                                        \
    })

/*
    Prints out the contents of 'data' in binary over the uart.
    Allows for printing an array of types in binary.

    'data' is a pointer to the memory location to print.
    'size' is the size of the type in bytes
    'num_elements' is the number of elements to print. This can be
        1 in cases of printing a single variable
*/
void bit_print(const void* const data, size_t size, size_t num_elements);

/*
    Print out all available i2c devices associated with
    i2c1_context.
*/
void print_available_i2c_devices(void);

#endif
