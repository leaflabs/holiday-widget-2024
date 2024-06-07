#include "uart_logger.h"
#include "stm32l0xx_hal.h"

void bit_print(uint8_t *bytes, int length) {
    // Iterate length times over the array
    for (int l = 0; l < length; l++) {

        // Get the element
        uint8_t byte = bytes[l];

        // Go throught all 8 bits
        for (int i = 0; i < 8; i++) {
            // Get the 'ith' bit from this byte and shift back to be either 1 or
            // 0
            uint8_t bit = (byte & (1 << i)) >> i;
            uart_logger_send("%c", bit + '0'); // print the bit as either '1' or '0' in ascii
        }

    }

    // And finally end
    uart_logger_send("\n\r");
}
