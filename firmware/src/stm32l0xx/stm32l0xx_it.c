#include "stm32l0xx_it.h"

#include "logging.h"
#include "lsm6dsm_driver.h"
#include "music_player.h"
#include "stm32l0xx_hal.h"
#include "system_communication.h"
#include "vcnl4020_driver.h"

void NMI_Handler(void) {
    while (1);
}

void HardFault_Handler(void) {
    while(1);
}

void SVC_Handler(void) {
    while (1);
}

void PendSV_Handler(void) {
    while (1);
}

void SysTick_Handler(void) {
    HAL_IncTick();
}

// User IRQ functions go down here

void EXTI2_3_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void EXTI4_15_IRQHandler(void) {
    if (EXTI->PR & EXTI_PR_PR5) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    }
    if (EXTI->PR & EXTI_PR_PR6) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    }
    if (EXTI->PR & EXTI_PR_PR7) {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if (pin == GPIO_PIN_5) {
        vcnl4020_interrupt_flag = 1;
    } else if (pin == GPIO_PIN_6) {
        lsm6dsm_driver_set_int1(lsm6dsm);
    } else if (pin == GPIO_PIN_7) {
        lsm6dsm_driver_set_int2(lsm6dsm);
    }
}

/*
    IRQ handler for the i2c interrupts
*/
void I2C1_IRQHandler(void) {
    HAL_I2C_EV_IRQHandler(&i2c1_context.i2c);  // Event Handler
    HAL_I2C_ER_IRQHandler(&i2c1_context.i2c);  // Error Handler
}

void DMA1_Channel4_5_6_7_IRQHandler(void) {
    HAL_DMA_IRQHandler(&music_player.config.hdma_tim2_durations);
    HAL_DMA_IRQHandler(&i2c1_context.i2c.hdmarx);
    HAL_DMA_IRQHandler(&i2c1_context.i2c.hdmatx);
    HAL_DMA_IRQHandler(uart.hdmatx);
}

/* DMA1 IRQ Handler for Channels 2 & 3 */
void DMA1_Channel2_3_IRQHandler(void) {
    HAL_DMA_IRQHandler(&music_player.config.hdma_tim2_notes);
    HAL_DMA_IRQHandler(&music_player.config.hdma_dac);
}

void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&uart);
    uart_busy = false;
}