#include "utils.h"

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "system_communication.h"
#include "uart_logger.h"

/*
    Helper function for bit_print which will print a single
    type in binary. Every 8 bits will be seperated by a space

    'data' is a pointer to the memory location to print
    'size' is the size of the type in bytes
*/
static void bit_print_helper(const uint8_t *const data, size_t size) {
    for (size_t i = size; i >= 1; i--) {
        uint8_t byte = data[i - 1];

        for (int j = 7; j >= 0; j--) {
            uint8_t bit = (byte >> j) & 1;
            uart_logger_send("%c", bit + '0');
        }

        // Seperate bytes, except for last one
        if (i > 1) uart_logger_send(" ");
    }
}

void bit_print(const void *const data, size_t size, size_t num_elements) {
    if (num_elements == 0 || size == 0) return;

    const uint8_t *const data_ = data;

    for (size_t i = 0; i < num_elements - 1; i++) {
        bit_print_helper(data_ + i * size, size);
        uart_logger_send(", ");
    }

    // Last entry without a comma
    bit_print_helper(data_ + (num_elements - 1) * size, size);
    uart_logger_send("\r\n");
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
