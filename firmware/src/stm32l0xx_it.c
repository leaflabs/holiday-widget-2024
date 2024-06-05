#include "stm32l0xx_it.h"

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
