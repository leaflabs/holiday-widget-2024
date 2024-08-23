#include "uart_logger.h"

#include "printf/printf.h"
#include "stm32l0xx_hal.h"

// Handle for the uart. Only lives here.
UART_HandleTypeDef uart;

volatile bool uart_busy = false;

// Size of the buffer for uart messages
static const size_t loggerBufferSize = 100;

// Timeout for the hal transmit in miliseconds
static const int transmitTimeout = 1000;

#define BUFFER_SIZE (1024)  // Must be a power of 2
#define BUFFER_MASK (BUFFER_SIZE - 1)

struct ring_buffer {
    unsigned char buffer[BUFFER_SIZE];
    unsigned int head;
    unsigned int tail;
};

void ring_buffer_init(struct ring_buffer *rb) {
    rb->head = 0;
    rb->tail = 0;
}

int ring_buffer_is_empty(struct ring_buffer *rb) {
    return rb->head == rb->tail;
}

int ring_buffer_is_full(struct ring_buffer *rb) {
    return ((rb->head + 1) & BUFFER_MASK) == rb->tail;
}

int ring_buffer_put(struct ring_buffer *rb, unsigned char data) {
    if (ring_buffer_is_full(rb)) {
        return -1;  // Buffer full
    }

    rb->buffer[rb->head] = data;
    rb->head = (rb->head + 1) & BUFFER_MASK;
    return 0;
}

int ring_buffer_get(struct ring_buffer *rb, unsigned char *data) {
    if (ring_buffer_is_empty(rb)) {
        return -1;  // Buffer empty
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) & BUFFER_MASK;
    return 0;
}

unsigned int ring_buffer_available_to_write(struct ring_buffer *rb) {
    if (rb->head >= rb->tail) {
        return (BUFFER_SIZE - (rb->head - rb->tail) - 1);
    } else {
        return (rb->tail - rb->head - 1);
    }
}

unsigned int ring_buffer_available_to_read(struct ring_buffer *rb) {
    if (rb->head >= rb->tail) {
        return (rb->head - rb->tail);
    } else {
        return (BUFFER_SIZE - (rb->tail - rb->head));
    }
}

int ring_buffer_puts(struct ring_buffer *rb, unsigned char *data, size_t len) {
    if (ring_buffer_available_to_write(rb) < len) {
        return -1;  // Not enough space in buffer
    }

    size_t first_part = BUFFER_SIZE - rb->head;

    // Copy in one operation if the data fits without wrapping
    size_t to_write = len > first_part ? first_part : len;
    memcpy(&rb->buffer[rb->head], data, to_write);

    // If wrapping occurs, copy the remaining data to the beginning of the
    // buffer
    if (len > first_part) {
        memcpy(&rb->buffer[0], data + to_write, len - to_write);
    }

    rb->head = (rb->head + len) & BUFFER_MASK;  // Update head with wrap-around
    return 0;
}

int ring_buffer_gets(struct ring_buffer *rb, unsigned char *data, size_t len) {
    if (ring_buffer_available_to_read(rb) < len) {
        return -1;  // Not enough data in buffer
    }

    size_t first_part = BUFFER_SIZE - rb->tail;

    // Copy in one operation if the data fits without wrapping
    size_t to_read = len > first_part ? first_part : len;
    memcpy(data, &rb->buffer[rb->tail], to_read);

    // If wrapping occurs, copy the remaining data from the beginning of the
    // buffer
    if (len > first_part) {
        memcpy(data + to_read, &rb->buffer[0], len - to_read);
    }

    rb->tail = (rb->tail + len) & BUFFER_MASK;  // Update tail with wrap-around
    return 0;
}

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

    ring_buffer_init(&uart_buffer);
}

void uart_logger_send(const char *s, ...) {
    char str[loggerBufferSize];

    va_list args;
    va_start(args, s);

    // Pass the arguments and the format string to vsnprintf.
    int len = vsnprintf_(str, loggerBufferSize, s, args);

    // Transmit the string we just formatted over the UART.
    ring_buffer_puts(&uart_buffer, str, len);

    // And cleanup
    va_end(args);
}

int uart_logger_send_bytes(const char *bytes, size_t len) {
    return ring_buffer_puts(&uart_buffer, bytes, len);
}

void uart_logger_run(void) {
    if (!uart_busy) {
        unsigned int len = ring_buffer_available_to_read(&uart_buffer);
        if (len != 0) {
            static char str[1024];
            int ret = ring_buffer_gets(&uart_buffer, str, len);
            if (ret == 0) {
                // Transmit the string we just formatted over the UART.
                HAL_UART_Transmit_DMA(&uart, (const uint8_t *)str, len);
                uart_busy = true;
            }
        }
    }
}