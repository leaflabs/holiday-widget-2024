#ifndef __UTILS_H
#define __UTILS_H

#include "stm32l0xx_hal.h"

/*
    Prints out the contents of 'data' in binary over the uart.
    Allows for printing an array of types in binary.

    'data' is a pointer to the memory location to print.
    'size' is the size of the type in bytes
    'num_elements' is the number of elements to print. This can be
        1 in cases of printing a single variable
*/
void bit_print(const void* const data, size_t size, size_t num_elements);

/*
    Print out all available i2c devices associated with
    i2c1_context.
*/
void print_available_i2c_devices(void);

#endif
