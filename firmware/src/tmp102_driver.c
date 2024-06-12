#include "tmp102_driver.h"

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "uart_logger.h"

// I2C address for the tmp102 sensor
#define TMP102_ADDRESS 0x48U

// Register to read the temperature from the tmp102 sensor as defined in the
// datasheet
#define TEMPERATURE_REGISTER 0x00U

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

float tmp102_driver_read(I2C_HandleTypeDef *i2c) {
    // Buffer to send request and hold results
    uint8_t buff[2] = {0};

    // Set the address to read from
    buff[0] = TEMPERATURE_REGISTER;

    // Send request to read the temperature using the i2c_driver function
    // Receive two bytes into buff as a result
    i2c_driver_read_registers_with_stop(i2c, TMP102_ADDRESS, buff, 2);

    // Get the temperature from the bytes
    float temp = convert_temp(buff[0], buff[1]);

    return temp;
}
