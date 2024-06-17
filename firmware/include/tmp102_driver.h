#ifndef __TMP102_DRIVER_H
#define __TMP102_DRIVER_H

#include <stdint.h>

#include "stm32l0xx_hal.h"

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

// Context for the sensor
struct tmp102_context {
    I2C_HandleTypeDef *i2c;

    // Last recorded temperature
    float temperature;
};

/*
    Initalize the tmp102 sensor and set the conversion rate
    'config' is the configuration settings for the sensor
    'context' holds the relevant data for the driver
*/
void tmp102_driver_init(const struct tmp102_config *config,
                        const struct tmp102_context *context);

/*
    Read the temperature off the device and return it
*/
void tmp102_driver_read(struct tmp102_context *context);

#endif
