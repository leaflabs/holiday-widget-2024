#include "tmp102_driver.h"

#include <errno.h>

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "uart_logger.h"
#include "utils.h"

// I2C address for the tmp102 sensor
#define TMP102_ADDRESS 0x48U

// Register to read the temperature from the tmp102 sensor as defined in the
// datasheet
#define TEMPERATURE_REGISTER 0x00U
#define CONFIGURATION_REGISTER 0x01U

// bit location for CR0 and CR1 bits
#define CR0_LOCATION 0x6

int tmp102_driver_init(const struct tmp102_config *config,
                       struct tmp102_context *context) {
    struct i2c_request *request = &context->request;
    struct i2c_driver_context *i2c_context = context->i2c_context;
    int ret = 0;

    context->state = TMP102_PRE_INIT;

    // Set up the i2c request for the whole driver
    request->address = TMP102_ADDRESS;
    request->buffer = context->i2c_transaction_buffer;
    request->future.state = FUTURE_WAITING;
    request->future.error_number = 0;

    /* Setup the configuration register */
    request->action = I2C_READ;
    request->ireg = CONFIGURATION_REGISTER;
    request->num_bytes = 2;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // second byte has the CR0 and CR1 bits
    // Clear the CR1 and CR0 bits so we can set them
    request->buffer[1] &= 0x3F;  // Clear the first two bits: 0b00111111
    request->buffer[1] |= (config->conversion_rate << CR0_LOCATION);

    request->action = I2C_WRITE;
    request->ireg = CONFIGURATION_REGISTER;
    request->num_bytes = 2;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // Device is ready to use
    context->state = TMP102_READY;

    return 0;
}

/*
    Given the two bytes to make up a temp, where byte1 is the
    left half-word and byte2 is the right half-word,
    return the temp in C as a float

*/
static inline float convert_temp(uint8_t byte1, uint8_t byte2) {
    // First byte needs to be shifted left 4 as it is bits 4:12
    // Second byte needs to be shifted right 4 as it is bits 0:3 bit has 4
    // trailing 0s
    int16_t binaryTemp = (byte1 << 4) | (byte2 >> 4);

    // Check if a negative number or not. Having a 1 in the 12th bit means its
    // negative
    if (binaryTemp & (0x1 << 11)) {
        binaryTemp = ~binaryTemp;  // Twos complement all the bits

        // Set the first 4 bits back to 0 as they should not be
        // complemented since we have 16 bits for binaryTemp
        binaryTemp &= ~((0x1 << 15) | (0x1 << 14) | (0x1 << 13) | (0x1 << 12));

        binaryTemp++;              // Add 1
        binaryTemp = -binaryTemp;  // Now negate the value since it is negative
    }

    float temp = binaryTemp * 0.0625f;  // Every bit is 0.0625 degrees C so
                                        // multiply to get the temp as a float

    return temp;
}

void tmp102_driver_process_temperature(struct tmp102_context *context) {
    struct i2c_request *request = &context->request;
    context->temperature = convert_temp(request->buffer[0], request->buffer[1]);
}

int tmp102_driver_request_temperature(struct tmp102_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;

    context->request.action = I2C_READ;
    context->request.ireg = TEMPERATURE_REGISTER;
    context->request.num_bytes = 2;

    // Submit request for temperature bytes
    return i2c_enqueue_request(i2c_context, request);
}
