#ifndef __UART_LOGGER_H
#define __UART_LOGGER_H

void uart_logger_init();

void uart_logger_send(const char *s, ...);

#endif
