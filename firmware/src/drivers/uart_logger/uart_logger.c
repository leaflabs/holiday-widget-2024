#include "uart_logger.h"

#include "printf/printf.h"
#include "ring_buffer.h"
#include "stm32l0xx_hal.h"

// Handle for the uart. Only lives here.
UART_HandleTypeDef uart;

volatile bool uart_busy = false;

// Size of the buffer for uart messages
static const size_t loggerBufferSize = 100;

// Timeout for the hal transmit in miliseconds
static const int transmitTimeout = 1000;

#define UART_BUFFER_SIZE 128  // Must be a power of 2

static char __uart_buffer[UART_BUFFER_SIZE];

static struct ring_buffer uart_buffer;

/*
 * uart_init initalizes all features for the uart to work properly.
 * It must be called before using any functions in this file.
 *
 */
void uart_logger_init(USART_TypeDef *instance, uint32_t baud_rate) {
    // Setup the Uart parameters
    uart.Instance = instance;
    uart.Init.BaudRate = baud_rate;
    uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1;
    uart.Init.Parity = UART_PARITY_NONE;
    uart.Init.Mode = UART_MODE_TX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    uart.Init.OverSampling = UART_OVERSAMPLING_16;
    uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&uart);

    RING_BUFFER_INIT(&uart_buffer, __uart_buffer, sizeof(__uart_buffer[0]),
                     UART_BUFFER_SIZE);
}

void uart_logger_send(const char *s, ...) {
    char str[loggerBufferSize];

    va_list args;
    va_start(args, s);

    // Pass the arguments and the format string to vsnprintf.
    int len = vsnprintf_(str, loggerBufferSize, s, args);

    // Transmit the string we just formatted over the UART.
    ring_buffer_push_n(&uart_buffer, str, len);

    // And cleanup
    va_end(args);
}

int uart_logger_send_bytes(const char *bytes, size_t len) {
    return ring_buffer_push_n(&uart_buffer, bytes, len);
}

void uart_logger_run(void) {
    if (!uart_busy) {
        unsigned int len = ring_buffer_available_to_read(&uart_buffer);
        if (len != 0) {
            static char str[1024];
            int ret = ring_buffer_pop_n(&uart_buffer, str, len);
            if (ret == 0) {
                // Transmit the string we just formatted over the UART.
                HAL_UART_Transmit_DMA(&uart, (const uint8_t *)str, len);
                uart_busy = true;
            }
        }
    }
}