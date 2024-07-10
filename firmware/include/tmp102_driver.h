#ifndef __TMP102_DRIVER_H
#define __TMP102_DRIVER_H

#include <stdint.h>

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"

#define TMP102_MAX_I2C_SIZE 2U

// Read as: TMP102_x_yz_HZ -> x.yz hz
// Example: TMP102_0_25_HZ -> 0.25 hz
enum conversion_rate {
    TMP102_0_25_HZ = 0x0,
    TMP102_1_0_HZ = 0x1,
    TMP102_4_0_HZ = 0x2,
    TMP102_8_0_HZ = 0x3
};

// Just to keep uniformity with other drivers, put the conversion rate in a
// configuration struct
struct tmp102_config {
    enum conversion_rate conversion_rate;
};

/*
    Four states as of now.
    PRE_INIT - before the device is initalized
    READY - data is ready to read and device is ready to use
    PENDING - i2c transaction in progress
    ERROR - device had an issue
*/
enum tmp102_state {
    TMP102_PRE_INIT,
    TMP102_READY,
    TMP102_PENDING,
    TMP102_ERROR
};

// Context for the sensor
struct tmp102_context {
    // i2c context for communication
    struct i2c_driver_context *i2c_context;
    struct i2c_request request;  // the state, below, prevents multiple access

    // Record the state of the driver
    volatile enum tmp102_state state;

    // Last recorded temperature
    float temperature;

    uint8_t i2c_transaction_buffer[TMP102_MAX_I2C_SIZE];
};

/*
    Initalize the tmp102 sensor and set the conversion rate
    'config' is the configuration settings for the sensor
    'context' holds the relevant data for the driver

    Returns 0 on success
*/
int tmp102_driver_init(const struct tmp102_config *config,
                       struct tmp102_context *context);

/*
    Convert the raw temperature data from the i2c transaction
    into a usable format and save in the context struct
*/
void tmp102_driver_process_temperature(struct tmp102_context *context);

/*
    Request to read the temperature data off the device

    returns 0 if the i2c transaction was added sucessfully
*/
int tmp102_driver_request_temperature(struct tmp102_context *context);

#endif
