#include "tmp102_driver.h"

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "uart_logger.h"

// I2C address for the tmp102 sensor
#define TMP102_ADDRESS 0x48U

// Register to read the temperature from the tmp102 sensor as defined in the
// datasheet
#define TEMPERATURE_REGISTER 0x00U
#define CONFIGURATION_REGISTER 0x01U

// bit location for CR0 and CR1 bits
#define CR0_LOCATION 0x6

void tmp102_driver_init(const struct tmp102_config *config,
                        const struct tmp102_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    // Buffer to send data
    uint8_t buffer[2] = {0};

    // First we need to load the configuration register to preserve its values
    i2c_driver_read_registers(i2c, TMP102_ADDRESS, CONFIGURATION_REGISTER,
                              buffer, 2);

    // second byte has the CR0 and CR1 bits
    // Clear the CR1 and CR0 bits so we can set them
    buffer[1] &= 0x3F;  // Clear the first two bits: 0b00111111

    // Set the CR0 and CR1 bits to the configured rate
    buffer[1] |= (config->conversion_rate << CR0_LOCATION);

    // Write the configuration registers back
    i2c_driver_write_registers(i2c, TMP102_ADDRESS, CONFIGURATION_REGISTER,
                               buffer, 2);
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

        binaryTemp++;    // Add 1
        binaryTemp = -binaryTemp;  // Now negate the value since it is negative
    }

    float temp = binaryTemp * 0.0625f;  // Every bit is 0.0625 degrees C so
                                        // multiply to get the temp as a float

    return temp;
}

void tmp102_driver_read(struct tmp102_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    // Buffer to hold results
    uint8_t buff[2] = {0};

    // Get the two temperature bytes
    i2c_driver_read_registers(i2c, TMP102_ADDRESS, TEMPERATURE_REGISTER, buff,
                              2);

    // Get the temperature from the bytes
    float temp = convert_temp(buff[0], buff[1]);

    // Save the results
    context->temperature = temp;
}
