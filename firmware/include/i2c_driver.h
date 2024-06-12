#ifndef __I2C_DRIVER_H
#define __I2C_DRIVER_H

#include "stm32l0xx_hal.h"

/*
    Initalizes the i2c handle so we can communicate
    with the sensors
*/
void i2c_driver_init(I2C_HandleTypeDef*, I2C_TypeDef*);

/*
    Scans for all i2c addresses available (7 bit)
    and call a user defined handler to processs them.

    handler returns void and accepts the address found(uint8_t) and a pointer to
   an argument for the handler (void*)

    This could be a struct or a regular type which will be used in the handler
   to process the address argument is the struct or regular type to be passed
   into the handler
*/
void i2c_driver_scan(I2C_HandleTypeDef *, void (*handler)(uint8_t, void *),
                     void *argument);

/*
    Sends a I2C transmit to tell which register to read and then sends an I2C
    receive to read the data back. These transactions are seperated by a stop
   bit for the specific cases where the sensor needs it.

    'address' is the i2c address to read. This should not be shifted over to
   meet 7 bit addressing as the function will do it for you 'buffer' is the
   buffer to hold the register to read from and to store the results
   'numReceiveBytes' is how many bytes to receive from the transaction

*/
void i2c_driver_read_registers_with_stop(I2C_HandleTypeDef *i2c,
                                         uint8_t address, uint8_t *buffer,
                                         size_t numReceiveBytes);

/*
    Sends an I2C transmit to write 'size' register without a stop bit inbetween
   the write and data.

    'address' is the i2c address of the sensor to read. This should not be
   shifted. 'ireg' is the internal register of the sensor to read 'buffer' is
   the buffer to store the byte into
*/
void i2c_driver_write_registers(I2C_HandleTypeDef *i2c, uint8_t address,
                                uint8_t ireg, uint8_t *buffer, size_t size);

/*
    Sends an I2C transmit to read 'size' registers without a stop bit inbetween
   the internal register and the data.

    'address' is the i2c address of the sensor to read. This should not be
   shifted. 'ireg' is the internal register of the sensor to read. 'buffer' is
   the buffer to store the bytes into 'size' is the number of bytes to read from
   the sensor
*/
void i2c_driver_read_registers(I2C_HandleTypeDef *ic, uint8_t address,
                               uint8_t ireg, uint8_t *buffer, size_t size);
#endif
