#include "stm32l0xx_it.h"

#include "lis3dh_driver.h"
#include "music_player.h"
#include "stm32l0xx_hal.h"
#include "system_communication.h"
#include "vcnl4020_driver.h"

void NMI_Handler(void) {
    while (1);
}

void HardFault_Handler(void) {
    while (1);
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

void EXTI0_1_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI2_3_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    // Pin 2 is the INT1 pin for the lis3dh snesor
    if (pin == GPIO_PIN_2) {
        lis3dh_interrupt1_flag = 1;
    }
    // Pin 1 is the INT pin for the vcnl4020 sensor
    if (pin == GPIO_PIN_1) {
        vcnl4020_interrupt_flag = 1;
    }
}

/*
    IRQ handler for the i2c interrupts
*/
void I2C1_IRQHandler(void) {
    I2C_HandleTypeDef *i2c = &i2c1_context.i2c;
    HAL_I2C_EV_IRQHandler(i2c);  // Event Handler
    HAL_I2C_ER_IRQHandler(i2c);  // Error Handler
}

/* DMA1 IRQ Handler for Channels 2 & 3 */
void DMA1_Channel2_3_IRQHandler(void) {
    struct music_player_config *cfg = &music_player.config;
    HAL_DMA_IRQHandler(&cfg->hdma_tim2_notes);
    HAL_DMA_IRQHandler(&cfg->hdma_dac);
}

/* DMA1 IRQ Handler for Channel 1 */
void DMA1_Channel1_IRQHandler(void) {
    struct music_player_config *cfg = &music_player.config;
    HAL_DMA_IRQHandler(&cfg->hdma_tim2_durations);
}