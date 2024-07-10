#ifndef __SYSTEM_COMMUNICATION_H__
#define __SYSTEM_COMMUNICATION_H__

#include "stm32l0xx_hal.h"

// Allow for global access of the i2c1 device
extern struct i2c_driver_context i2c1_context;

/*
    Set up UART and I2C communication for widget to computer and
    stm32 chip to sensors
*/
void system_communication_setup(void);

/*
    Check if there are requests on the i2c queue and process them
*/
void system_communication_run(void);

#endif /* __SYSTEM_COMMUNICATION_H__ */
