#ifndef __UTILS_H
#define __UTILS_H

#include "stm32l0xx_hal.h"

/*
    Prints out 'length' number of bytes to the console in binary

    'bytes' is a pointer to an array of 'uint8_t's
    'length' is the number of elements to print.

    (Currently unused)
*/
void bit_print(uint8_t *bytes, int length);

/*
    Print out all available i2c devices associated with
    i2c1_context.
*/
void print_available_i2c_devices(void);

#endif
