#include "random_number_generator.h"

void random_number_generator_init(struct random_number_generator *rng) {
    struct random_number_generator_context *context = &rng->context;
    // Enable RNG clock
    __HAL_RCC_RNG_CLK_ENABLE();

    // Initialize RNG
    context->hrng.Instance = RNG;
    int ret = HAL_RNG_Init(&context->hrng);
    if (ret != 0) {
        LOG_ERR("Failed to initialized RNG: %d", ret);
    }

    // Initialize ring buffer
    RING_BUFFER_INIT(&context->buffer, context->rng_buf, sizeof(uint32_t),
                     RNG_BUFFER_SIZE);
}

int random_number_generator_update(struct random_number_generator *rng) {
    struct random_number_generator_context *context = &rng->context;
    if (!ring_buffer_is_full(&context->buffer)) {
        uint32_t random_number = 0;
        int ret = HAL_RNG_GenerateRandomNumber(&context->hrng, &random_number);
        if (ret != HAL_OK) {
            LOG_ERR("Failed to generate random number: %d", ret);
            LOG_DBG("RNG Status: %d", RNG->SR);
            LOG_DBG("RBG STATE: %u", context->hrng.State);
            LOG_DBG("RNG ERROR: %u", context->hrng.ErrorCode);
        } else {
            ret = ring_buffer_push(&context->buffer, &random_number);
            if (ret != 0) {
                LOG_ERR("Failed to push new random number onto ring buffer: %d",
                        ret);
            }
        }

        return ret;
    }

    return 0;
}

uint32_t random_number_generator_get_next(struct random_number_generator *rng) {
    struct random_number_generator_context *context = &rng->context;
    uint32_t random_number = 0;
    bool ret = ring_buffer_pop(&context->buffer, &random_number);
    if (ret != 0) {
        LOG_ERR(
            "Random number generator exhuasted buffer - increase buffer size");
    }

    /* Always return random_number regardless of success, as it will default to
     * 0 */
    return random_number;
}

uint32_t random_number_generator_get_next_in_n(
    struct random_number_generator *rng, uint32_t n) {
    // Ensure n is greater than 0
    if (n == 0)
        return 0;
    uint32_t random_number = random_number_generator_get_next(rng);

    /* Mask the first 16-bits and multiply by n */
    uint32_t scaled_number = ((random_number & 0x0000FFFF) * n) >> 16;

    return scaled_number;
}