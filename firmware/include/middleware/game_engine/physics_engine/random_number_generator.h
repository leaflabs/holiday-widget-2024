#ifndef __RANDOM_NUMBER_GENERATOR_H__
#define __RANDOM_NUMBER_GENERATOR_H__
#include "ring_buffer.h"
#include "stm32l0xx_hal_rng.h"

#define RNG_BUFFER_SIZE 32

struct random_number_generator_context {
    RNG_HandleTypeDef hrng;
    struct ring_buffer buffer;
    uint32_t rng_buf[RNG_BUFFER_SIZE];
} __attribute__((aligned(4)));

struct random_number_generator {
    struct random_number_generator_context context;
};

int random_number_generator_init(struct random_number_generator *rng);
int random_number_generator_update(struct random_number_generator *rng);
uint32_t random_number_generator_get_next(struct random_number_generator *rng);
uint32_t random_number_generator_get_next_in_n(
    struct random_number_generator *rng, uint32_t n);
#endif /*__RANDOM_NUMBER_GENERATOR_H__*/