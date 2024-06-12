#include "stm32l0xx_it.h"

#include "lis3dh_driver.h"
#include "stm32l0xx_hal.h"

void NMI_Handler(void) {
    while (1)
        ;
};

void HardFault_Handler(void) {
    while (1)
        ;
};

void SVC_Handler(void) {
    while (1)
        ;
};

void PendSV_Handler(void) {
    while (1)
        ;
};

void SysTick_Handler(void) {
    HAL_IncTick();
}

// User IRQ functions go down here

void EXTI2_3_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    // Pin 2 is the INT1 pin for the lis3dh snesor
    if (pin == GPIO_PIN_2) {
        lis3dh_interrupt1_flag = 1;  // Set the flag
    }
}
