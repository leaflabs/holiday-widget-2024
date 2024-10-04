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
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_5;
    gpio.Mode = GPIO_MODE_IT_FALLING;  // Falling voltage triggers
    gpio.Pull = GPIO_NOPULL;           // Already pulled high
    HAL_GPIO_Init(GPIOA, &gpio);

    // Enable the interrupt for Pin 1
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

    // Set up the i2c request for the whole driver
    request->address = VCNL4020_ADDRESS;
    request->buffer = context->i2c_transaction_buffer;
    request->future.state = FUTURE_WAITING;
    request->future.error_number = 0;

    /*
        First clear the command register so we can adjust registers safely
    */
    ret = i2c_clear_and_set_bits(i2c_context, request, COMMAND_REGISTER, 0xE0,
                                 0x00);  // Clear last 5 bits
    if (ret < 0) {
        return ret;
    }

    /* Set Register #2: Rate of proximity measurement */
    ret = i2c_write_bits(i2c_context, request, PROXIMITY_RATE_REGISTER,
                         config->proximity_rate_register.as_byte);
    if (ret < 0) {
        return ret;
    }

    /* Set Register #3: IR LED current value */
    ret = i2c_clear_and_set_bits(
        i2c_context, request, IR_LED_CURRENT_REGISTER, 0xC0,
        config->ir_led_current_register.as_byte);  // Clear last 6 bits
    if (ret < 0) {
        return ret;
    }

    /* Set Register #4: Ambient Light Parameter Register */
    ret = i2c_write_bits(i2c_context, request, AMBIENT_LIGHT_PARAMETER_REGISTER,
                         config->ambient_light_parameter_register.as_byte);
    if (ret < 0) {
        return ret;
    }

    /* Set Register #9: Interrupt Control Register */
    ret = i2c_write_bits(i2c_context, request, INTERRUPT_CONTROL_REGISTER,
                         config->interrupt_control_register.as_byte);
    if (ret < 0) {
        return ret;
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
    */
    ret = i2c_write_bits(i2c_context, request, LOW_THRESHOLD_REGISTER_H,
                         low >> 8);  // Get upper 8 bits
    if (ret < 0) {
        return ret;
    }

    ret = i2c_write_bits(i2c_context, request, LOW_THRESHOLD_REGISTER_L,
                         low & 0xFF);  // Get lower 8 bits
    if (ret < 0) {
        return ret;
    }

    /*
        Set Register #12:13: High Threshold High and Low byte
    */
    ret = i2c_write_bits(i2c_context, request, HIGH_THRESHOLD_REGISTER_H,
                         high >> 8);  // Get upper 8 bits
    if (ret < 0) {
        return ret;
    }

    ret = i2c_write_bits(i2c_context, request, HIGH_THRESHOLD_REGISTER_L,
                         high & 0xFF);  // Get lower 8 bits
    if (ret < 0) {
        return ret;
    }

    /* Set Register #1: Command Register */
    ret = i2c_clear_and_set_bits(
        i2c_context, request, COMMAND_REGISTER, 0xFF,
        config->command_register.as_byte |
            ENABLE_SELF_TIMER);  // Enable the device now that we have finished
                                 // adjusting the configurations
    if (ret < 0) {
        return ret;
    }

    // Now save what is enabled for low power mode
    context->cmd_reg = config->command_register.as_byte;

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

int vcnl4020_driver_enter_low_power(struct vcnl4020_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret = 0;

    // Disable all measurements and turn off the state machine
    ret = i2c_clear_and_set_bits(i2c_context, request, COMMAND_REGISTER, 0xE0,
                                 0x00);
    return ret;
}

int vcnl4020_driver_exit_low_power(struct vcnl4020_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret = 0;

    // Reenable the measurements requested and turn on state machine
    ret = i2c_clear_and_set_bits(i2c_context, request, COMMAND_REGISTER, 0xFF,
                                 context->cmd_reg | ENABLE_SELF_TIMER);

    return ret;
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
