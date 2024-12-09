#ifndef __IMP23ABSU_DRIVER_H
#define __IMP23ABSU_DRIVER_H
#include <unistd.h>

#include "stdbool.h"
#include "stm32l0xx_hal.h"
#include "utils.h"

/* Size of audio buffer (must be a power of 2) */
#define AUDIO_BUFF_SIZE (128U)

/* Assert that AUDIO_BUFF_SIZE is a power of 2 at compile-time */
#if !IS_POWER_OF_2(AUDIO_BUFF_SIZE)
#error "AUDIO_BUFF_SIZE must be a power of two"
#endif

/* FIFO ring buffer structure */
struct audio_ring_buffer {
    volatile uint16_t data[AUDIO_BUFF_SIZE];
    volatile uint16_t read_idx;
};

/* IMP23ABSU driver structure */
struct imp23absu_driver {
    bool initialized;
    bool enabled;
    ADC_HandleTypeDef adc_handle;
    DMA_HandleTypeDef dma_handle;
    struct audio_ring_buffer audio_buffer;
    uint32_t sample_frequency;
};

/* Initializes IMP23ABSU driver */
int imp23absu_driver_init();

/* Start continuous transfer of ADC conversions into audio buffer */
int imp23absu_driver_enable();

/* Stop continuous transfer of ADC conversions into audio buffer */
int imp23absu_driver_disable();

/* Flush the contents of the audio buffer with 0 */
void imp23absu_driver_flush_buffer();

uint32_t imp23absu_get_sample_frequency();

/**
 * Read the most recent 'len' values from the audio buffer into 'buf',
 * returns the number of values read. 'len' cannot exceed MAX_READ_SIZE.
 */
ssize_t imp23absu_driver_read(uint16_t *buf, size_t len);

#endif
