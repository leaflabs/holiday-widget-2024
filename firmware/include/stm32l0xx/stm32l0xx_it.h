#ifndef __STM32L0xx_IT_H
#define __STM32L0xx_IT_H

#include "stm32l0xx_hal.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

// User defined IRQ functions
void EXTI2_3_IRQHandler(void);
void EXTI4_15_IRQHandler(void);
void HAL_GPIO_EXTI_Callback(uint16_t pin);
void I2C1_IRQHandler(void);
void DMA1_Channel4_5_6_7_IRQHandler(void);
void DMA1_Channel2_3_IRQHandler(void);
void USART2_IRQHandler(void);

#endif
