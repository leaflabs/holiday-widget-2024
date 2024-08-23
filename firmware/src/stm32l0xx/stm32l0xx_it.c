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

/* Structure to hold the fault context */
typedef struct {
    uint32_t R0;
    uint32_t R1;
    uint32_t R2;
    uint32_t R3;
    uint32_t R12;
    uint32_t LR;
    uint32_t PC;
    uint32_t PSR;
} HardFaultRegisters;

volatile HardFaultRegisters hard_fault_context;

/*
void HardFault_Handler(void) {
    LOG_ERR("Hard Fault!");
    while(1);
}
*/

/*
void HardFault_Handler(void) __attribute__((naked));
void _handler_c(uint32_t *stack_pointer) __attribute__((used));
void HardFault_Handler(void) {
    __asm volatile(
        "movs r0, #4 \n"    // Move 4 into r0
        "mov r1, lr \n"     // Move lr into r1
        "tst r0, r1 \n"     // Test if the EXC_RETURN bit (bit 2) is set
        "beq _MSP_used \n"  // If bit 2 is 0, MSP is being used
        "mrs r0, psp \n"    // Load Process Stack Pointer into r0
        "b _handler_c \n"   // Branch to C handler
        "_MSP_used: \n"
        "mrs r0, msp \n"   // Load Main Stack Pointer into r0
        "b _handler_c \n"  // Branch to C handler
    );
}

void _handler_c(uint32_t *stack_pointer) {
    // Stack pointer points to the stack frame that was saved at the time of the
    // fault
    hard_fault_context.R0 = stack_pointer[0];
    hard_fault_context.R1 = stack_pointer[1];
    hard_fault_context.R2 = stack_pointer[2];
    hard_fault_context.R3 = stack_pointer[3];
    hard_fault_context.R12 = stack_pointer[4];
    hard_fault_context.LR = stack_pointer[5];
    hard_fault_context.PC = stack_pointer[6];
    hard_fault_context.PSR = stack_pointer[7];

    // Now hard_fault_context contains all the key register values at the time
    // of the fault
    LOG_ERR("HARD FAULT!");
    LOG_ERR("R0: 0x%x", hard_fault_context.R0);
    LOG_ERR("R1: 0x%x", hard_fault_context.R1);
    LOG_ERR("R2: 0x%x", hard_fault_context.R2);
    LOG_ERR("R3: 0x%x", hard_fault_context.R3);
    LOG_ERR("R12: 0x%x", hard_fault_context.R12);
    LOG_ERR("LR: 0x%x", hard_fault_context.LR);
    LOG_ERR("PC: 0x%x", hard_fault_context.PC);
    LOG_ERR("PSR: 0x%x", hard_fault_context.PSR);
    // Enter an infinite loop to allow you to inspect the variables via a
debugger while (1) {
        // Optionally, you could toggle an LED here to indicate the fault
    }
}
*/
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
        // Pin 5 is the INT pin for the vcnl4020 sensor
        vcnl4020_interrupt_flag = 1;
    } else if (pin == GPIO_PIN_6) {
        LOG_DBG("LSM6DSM INT1");
        lsm6dsm_driver_set_int1(lsm6dsm);
    } else if (pin == GPIO_PIN_7) {
        LOG_DBG("LSM6DSM INT2");
        lsm6dsm_driver_set_int2(lsm6dsm);
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

void DMA1_Channel4_5_6_7_IRQHandler(void) {
    I2C_HandleTypeDef *i2c = &i2c1_context.i2c;
    HAL_DMA_IRQHandler(i2c->hdmarx);
    HAL_DMA_IRQHandler(i2c->hdmatx);
    HAL_DMA_IRQHandler(uart.hdmatx);
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

void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&uart);
    uart_busy = false;
}