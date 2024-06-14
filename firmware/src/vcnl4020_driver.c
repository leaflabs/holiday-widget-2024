#include "vcnl4020_driver.h"

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"

#define VCNL4020_ADDRESS 0x13

// define all important i2c register addresses
#define COMMAND_REGISTER 0x80
#define PROXIMITY_RATE_REGISTER 0x82
#define IR_LED_CURRENT_REGISTER 0x83
#define AMBIENT_LIGHT_PARAMETER_REGISTER 0x84
#define AMBIENT_LIGHT_RESULT_REGISTER_H 0x85
#define AMBIENT_LIGHT_RESULT_REGISTER_L 0x86
#define PROXIMITY_RESULT_REGISTER_H 0x87
#define PROXIMITY_RESULT_REGISTER_L 0x88
#define INTERRUPT_CONTROL_REGISTER 0x89
#define LOW_THRESHOLD_REGISTER_H 0x8A
#define LOW_THRESHOLD_REGISTER_L 0x8B
#define HIGH_THRESHOLD_REGISTER_H 0x8C
#define HIGH_THRESHOLD_REGISTER_L 0x8D
#define INTERRUPT_STATUS_REGISTER 0x8E

#define ENABLE_SELF_TIMER 0x1

// Global interrupt flag for the vcnl4020 sensor
volatile uint8_t vcnl4020_interrupt_flag = 0;

void vcnl4020_driver_init(const struct vcnl4020_config *config,
                          const struct vcnl4020_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    /*
        Set up the interrupt pin
    */
    __HAL_RCC_GPIOC_CLK_ENABLE();  // enable the clock

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_IT_FALLING;  // Falling voltage triggers
    gpio.Pull = GPIO_NOPULL;           // Already pulled high
    HAL_GPIO_Init(GPIOC, &gpio);

    // Enable the interrupt for Pin 1
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

    // Buffer for sending and receiving data
    uint8_t buffer[2] = {0};

    /*
        First clear the command register just incase it has values.
        We must read the old value to preserve the value of some bits
    */

    // Read the old value
    i2c_driver_read_registers(i2c, VCNL4020_ADDRESS, COMMAND_REGISTER, buffer,
                              1);
    buffer[0] &= 0xE0;  // Clear last 5 bits: 11100000

    // Now write it back
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS, COMMAND_REGISTER, buffer,
                               1);

    /*
        Set Register #2: Rate of proximity measurement
    */

    buffer[0] =
        config->proximity_rate_register.as_byte;  // Use the user argument
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS, PROXIMITY_RATE_REGISTER,
                               buffer, 1);

    /*
        Set Register #3: IR LED current value
    */

    // Read the register to preserve certain bits
    i2c_driver_read_registers(i2c, VCNL4020_ADDRESS, IR_LED_CURRENT_REGISTER,
                              buffer, 1);

    buffer[0] &= 0xC0;  // Clear last 6 bits: 11000000
    buffer[0] |=
        config->ir_led_current_register.as_byte;  // Use the user argument
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS, IR_LED_CURRENT_REGISTER,
                               buffer, 1);

    /*
        Set Register #4: Ambient Light Parameter Register
    */

    buffer[0] = config->ambient_light_parameter_register
                    .as_byte;  // use the user argumnet
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS,
                               AMBIENT_LIGHT_PARAMETER_REGISTER, buffer, 1);

    /*
        Set Register #9: Interrupt Control Register
    */
    buffer[0] =
        config->interrupt_control_register.as_byte;  // Use the user argument
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS,
                               INTERRUPT_CONTROL_REGISTER, buffer, 1);

    /*
        Set Threshold values
    */

    // Define the low and hight thresholds
    uint16_t low = config->interrupt_thresholds.low;
    uint16_t high = config->interrupt_thresholds.high;

    // If the user selects the thresholds are for als, multiply by 4 to scale
    // them back from lux to cnts.
    if (config->interrupt_control_register.thresh_prox_als ==
        VCNL4020_THRESH_ALS) {
        low <<= 2;   // Multiply by 4
        high <<= 2;  // Multiply by 4
    }

    /*
        Set Register #10:11: Low Threshold High and Low byte
        - Send over the high (buffer[0]) and low (buffer[1]) byte for the low
       threshold
    */
    buffer[0] = low >> 8;      // Get upper 8 bits
    buffer[1] = low & (0xFF);  // Get lower 8 bits
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS, LOW_THRESHOLD_REGISTER_H,
                               buffer, 2);

    /*
        Set Register #12:13: High Threshold High and Low byte
        - Send over the high (buffer[0]) and low (buffer[1]) byte for the low
       threshold
    */
    buffer[0] = high >> 8;      // Get upper 8 bits
    buffer[1] = high & (0xFF);  // Get lower 8 bits;
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS, HIGH_THRESHOLD_REGISTER_H,
                               buffer, 2);

    /*
        Set Register #1: Command Register
    */

    // First read the register to not overwrite the bits
    i2c_driver_read_registers(i2c, VCNL4020_ADDRESS, COMMAND_REGISTER, buffer,
                              1);

    buffer[0] |= config->command_register.as_byte;  // Use the user arguments
    buffer[0] |= ENABLE_SELF_TIMER;  // Set the timer enable to yes.

    // Now send it over with the changes made
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS, COMMAND_REGISTER, buffer,
                               1);

    /*
        Clear Interrupt flag.
        Just incase any interrupts were triggered beforehand, wipe the
        register by writing a 1 to each interrupt bit.
    */
    vcnl4020_clear_interrupt(context, NULL,
                             NULL);  // No handler function or args. Just clear
}

void vcnl4020_driver_read_proximity(struct vcnl4020_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    // Buffer for high and low bytes
    uint8_t buffer[2] = {0};
    i2c_driver_read_registers(i2c, VCNL4020_ADDRESS,
                              PROXIMITY_RESULT_REGISTER_H, buffer,
                              2);  // Read both the H and L registers

    // index 0 has the high byte so shift over
    uint16_t prox = (buffer[0] << 8) | buffer[1];

    context->proximity_cnt = prox;  // Save the value
}

void vcnl4020_driver_read_als(struct vcnl4020_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    // Buffer for high and low bytes
    uint8_t buffer[2] = {0};

    i2c_driver_read_registers(i2c, VCNL4020_ADDRESS,
                              AMBIENT_LIGHT_RESULT_REGISTER_H, buffer,
                              2);  // Read both the H and L registers

    // index 0 is the high byte so shift over
    uint16_t als = (buffer[0] << 8) | buffer[1];

    // Convert to lux by multiplying by .25 lux / count
    uint16_t lux = als >> 2;  // Multiply by 0.25. AKA r shift by 2

    context->als_lux = lux;  // Save the value
}

void vcnl4020_driver_read_all(struct vcnl4020_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    // Buffer for high and low bytes for als and prox
    uint8_t buffer[4] = {0};

    // Ambient light high is the starting register
    i2c_driver_read_registers(i2c, VCNL4020_ADDRESS,
                              AMBIENT_LIGHT_RESULT_REGISTER_H, buffer,
                              4);  // Read both the H and L registers x2

    // index 0 is the high byte so shift over
    uint16_t als = (buffer[0] << 8) | buffer[1];

    // Convert to lux by multiplying by .25 lux / count
    uint16_t lux = als >> 2;  // Multiply by 0.25. AKA r shift by 2

    // index 2 is the high byte so shift over
    uint16_t prox = (buffer[2] << 8) | buffer[3];

    context->proximity_cnt = prox;
    context->als_lux = lux;
}

void vcnl4020_clear_interrupt(const struct vcnl4020_context *context,
                              void (*user_handler)(uint8_t, void *),
                              void *user_handler_arg) {
    I2C_HandleTypeDef *i2c = context->i2c;

    // Clear the flag for our program
    vcnl4020_interrupt_flag = 0;

    // Read the register to see which interrupt was triggered
    uint8_t status;
    i2c_driver_read_registers(i2c, VCNL4020_ADDRESS, INTERRUPT_STATUS_REGISTER,
                              &status, 1);

    // Now clear the flag by writing back the status. Each bit that was a 1,
    // signifying that was the interrupt triggered, will still be a 1 and clear
    // the interrupt
    i2c_driver_write_registers(i2c, VCNL4020_ADDRESS, INTERRUPT_STATUS_REGISTER,
                               &status, 1);

    // Call the user handler if defined
    if (user_handler != NULL) {
        user_handler(status, user_handler_arg);
    }
}
