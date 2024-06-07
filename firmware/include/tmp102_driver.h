#ifndef __TMP102_DRIVER_H
#define __TMP102_DRIVER_H

#include "stm32l0xx_hal.h"

/*
    Read the temperature off the device and return it
*/
float tmp102_driver_read(I2C_HandleTypeDef*);

#endif
