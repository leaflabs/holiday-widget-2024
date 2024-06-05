#include "uart_logger.h"

#include "printf/printf.h"
#include "stm32l0xx_hal.h"

// Handle for the uart. Only lives here.
static UART_HandleTypeDef uart;

// Size of the buffer for uart messages
static const size_t loggerBufferSize = 100;

// Timeout for the hal transmit in miliseconds
static const int transmitTimeout = 1000;

/*
 * uart_init initalizes all features for the uart to work properly.
 * It must be called before using any functions in this file.
 *
 */
void uart_logger_init() {
    // Setup the Uart parameters
    uart.Instance = USART2;
    uart.Init.BaudRate = 9600;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_TX_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&uart);
}

/*
 * Logger takes in a format string and a varaible number
 * of arguments to be printed using the UART
 *
 */
void uart_logger_send(const char *s, ...) {
    // This is the buffer that snprintf will write into
    char str[loggerBufferSize];

    // Get the variadic arguments (the '...' from the function prototype).
    va_list args;
    va_start(args, s);  // s is the argument right before the ...

    // Pass the arguments and the format string to vsnprintf. The 'v' means
    // variadic so we can pass a va_list. Also get the number of chars written
    int len = vsnprintf_(str, loggerBufferSize, s, args);

    // Transmit the string we just formatted over the UART.
    HAL_UART_Transmit(&uart, (const uint8_t *)str, len, transmitTimeout);

    // And cleanup
    va_end(args);
}
