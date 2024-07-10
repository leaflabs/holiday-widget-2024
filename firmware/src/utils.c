#include "utils.h"

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "system_communication.h"
#include "uart_logger.h"

void bit_print(uint8_t *bytes, int length) {
    // Iterate length times over the array
    for (int l = 0; l < length; l++) {
        // Get the element
        uint8_t byte = bytes[l];

        // Go throught all 8 bits but reverse because we want LSB on the right
        for (int i = 7; i >= 0; i--) {
            // Get the 'ith' bit from this byte and shift back to be either 1 or
            // 0
            uint8_t bit = (byte & (1 << i)) >> i;
            uart_logger_send(
                "%c",
                bit + '0');  // print the bit as either '1' or '0' in ascii
        }
    }

    // And send a newline
    uart_logger_send("\n\r");
}

void print_available_i2c_devices(void) {
    uint8_t online_count = 0;
    uint64_t high_mask = 0;
    uint64_t low_mask = 0;

    for (uint8_t i = 0; i < 128; i++) {
        if (i2c_device_is_ready(&i2c1_context, i)) {
            online_count++;
            if (i < 64) {
                low_mask |= 1 << i;
            } else {
                high_mask |= 1 << (i - 64);
            }
        }
    }

    uart_logger_send("Scanned I2C addresses: %d\r\n", online_count);

    for (uint8_t i = 0; i < 128; i++) {
        if ((i < 64 && low_mask & (1 << i)) ||
            (i < 128 && high_mask & (1 << (i - 64)))) {
            uart_logger_send("     Address found: 0x%02x\r\n", i);
        }
    }
    uart_logger_send("End of scan\r\n\r\n");
}
