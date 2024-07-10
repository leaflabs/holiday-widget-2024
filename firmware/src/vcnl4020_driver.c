#include "vcnl4020_driver.h"

#include <errno.h>
#include <stdatomic.h>

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "utils.h"

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
volatile int vcnl4020_interrupt_flag = 0;

int vcnl4020_driver_init(const struct vcnl4020_config *config,
                         struct vcnl4020_context *context) {
    struct i2c_request *request = &context->request;
    struct i2c_request *it_request = &context->it_request;
    struct i2c_driver_context *i2c_context = context->i2c_context;
    int ret = 0;

    context->state = VCNL4020_PRE_INIT;
    context->it_state = VCNL4020_INTERRUPT_CLEAR;

    /*
        Set up the interrupt pin
    */
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_IT_FALLING;  // Falling voltage triggers
    gpio.Pull = GPIO_NOPULL;           // Already pulled high
    HAL_GPIO_Init(GPIOC, &gpio);

    // Enable the interrupt for Pin 1
    HAL_NVIC_SetPriority(EXTI0_1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);

    // Set up the i2c request for the whole driver
    request->address = VCNL4020_ADDRESS;
    request->buffer = context->i2c_transaction_buffer;
    request->future.state = FUTURE_WAITING;
    request->future.error_number = 0;

    /*
        First clear the command register just incase it has values.
        We must read the old value to presreve the value of some bits
    */
    request->action = I2C_READ;
    request->ireg = COMMAND_REGISTER;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->buffer[0] &= 0xE0;  // Clear last 5 bits: 11100000

    // Now write it back
    request->action = I2C_WRITE;
    request->ireg = COMMAND_REGISTER;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    /* Set Register #2: Rate of proximity measurement */

    request->action = I2C_WRITE;
    request->ireg = PROXIMITY_RATE_REGISTER;
    request->buffer[0] = config->proximity_rate_register.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    /* Set Register #3: IR LED current value */

    // Read the old value to preserve some bits
    request->action = I2C_READ;
    request->ireg = IR_LED_CURRENT_REGISTER;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    request->buffer[0] &= 0xC0;  // Clear last 6 bits: 11000000
    request->buffer[0] |= config->ir_led_current_register.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    /* Set Register #4: Ambient Light Parameter Register */
    request->action = I2C_WRITE;
    request->ireg = AMBIENT_LIGHT_PARAMETER_REGISTER;
    request->buffer[0] = config->ambient_light_parameter_register.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    /* Set Register #9: Interrupt Control Register */
    request->action = I2C_WRITE;
    request->ireg = INTERRUPT_CONTROL_REGISTER;
    request->buffer[0] = config->interrupt_control_register.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    /* Set Threshold values */

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
    request->action = I2C_WRITE;
    request->ireg = LOW_THRESHOLD_REGISTER_H;
    request->num_bytes = 2;
    request->buffer[0] = low >> 8;      // Get upper 8 bits
    request->buffer[1] = low & (0xFF);  // Get lower 8 bits

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    /*
        Set Register #12:13: High Threshold High and Low byte
        - Send over the high (buffer[0]) and low (buffer[1]) byte for the low
       threshold
    */
    request->action = I2C_WRITE;
    request->ireg = HIGH_THRESHOLD_REGISTER_H;
    request->buffer[0] = high >> 8;      // Get upper 8 bits
    request->buffer[1] = high & (0xFF);  // Get lower 8 bits

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    /* Set Register #1: Command Register */

    // First read the register to not overwrite the bits
    request->action = I2C_READ;
    request->ireg = COMMAND_REGISTER;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    request->buffer[0] |= config->command_register.as_byte;
    request->buffer[0] |= ENABLE_SELF_TIMER;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // Finally set up interrupts and their i2c request
    it_request->action = I2C_READ;
    it_request->address = VCNL4020_ADDRESS;
    it_request->ireg = INTERRUPT_STATUS_REGISTER;
    it_request->buffer = context->it_status_buffer;
    it_request->num_bytes = 1;
    it_request->future.state = FUTURE_WAITING;
    it_request->future.error_number = 0;

    // Clear the internal register by saying we have an interrupt to clear
    vcnl4020_interrupt_flag = VCNL4020_INTERRUPT_TRIGGERED;

    context->state = VCNL4020_READY;

    return 0;
}

/*
    This callback function is called after the i2c transaction has finished
    for reading both als and proximity. The passed in argument is the
   vcnl4020_context and allows for updating the context's als and proximity
   variables after the new data is aquired.
*/
void vcnl4020_driver_process_als_prox(struct vcnl4020_context *context) {
    struct i2c_request *request = &context->request;

    // index 0 is the high byte so shift over
    uint16_t als = (request->buffer[0] << 8) | request->buffer[1];
    // Convert to lux by multiplying by .25 lux / count
    uint16_t lux = als >> 2;  // Multiply by 0.25. AKA r shift by 2

    // index 2 is the high byte so shift over
    uint16_t prox = (request->buffer[2] << 8) | request->buffer[3];
    // Update both variables
    context->proximity_cnt = prox;
    context->als_lux = lux;
}

int vcnl4020_driver_request_als_prox(struct vcnl4020_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    // Ambient light high is the starting register. Read both H and L registers
    // but 2x due to both als and proximity
    request->action = I2C_READ;
    request->ireg = AMBIENT_LIGHT_RESULT_REGISTER_H;
    request->num_bytes = 4;

    return i2c_enqueue_request(i2c_context, request);
}

int vcnl4020_driver_request_it_clear_read(struct vcnl4020_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *it_request = &context->it_request;

    it_request->action = I2C_READ;
    return i2c_enqueue_request(i2c_context, it_request);
}

int vcnl4020_driver_request_it_clear_write(struct vcnl4020_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *it_request = &context->it_request;

    it_request->action = I2C_WRITE;
    return i2c_enqueue_request(i2c_context, it_request);
}
