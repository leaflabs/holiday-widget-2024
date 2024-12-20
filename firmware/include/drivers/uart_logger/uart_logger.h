#ifndef __UART_LOGGER_H
#define __UART_LOGGER_H

#include <stdbool.h>

#include "stm32l0xx_hal.h"

/*
    Initalize the uart for sending messages

    'instance' specifies which uart instance to associate with
    'baud_rate' specifies the data rate
*/
void uart_logger_init(USART_TypeDef *instance, uint32_t baud_rate);

/*
 * Logger takes in a format string and a varaible number
 * of arguments to be printed using the UART
 */
void uart_logger_send(const char *s, ...);

int uart_logger_send_bytes(const char *bytes, size_t len);

void uart_logger_run(void);

extern UART_HandleTypeDef uart;

extern volatile bool uart_busy;

#endif
