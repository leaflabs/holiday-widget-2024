#include "physics_engine.h"

void physics_engine_init(struct physics_engine *physics_engine) {
    struct physics_engine_context *context = &physics_engine->context;

    context->environment = NULL;
    context->event_queue = NULL;

    random_number_generator_init(&context->random_number_generator);
}

void physics_engine_update(struct physics_engine *physics_engine,
                           uint32_t delta_t) {
    struct physics_engine_context *context = &physics_engine->context;
    int ret = random_number_generator_update(&context->random_number_generator);
    if (ret != 0) {
        LOG_ERR("Random number generator failed to update with error code: %d",
                ret);
    }

    if (context->environment != NULL) {
        physics_engine_environment_update(context->event_queue,
                                          context->environment, delta_t);
    }
}

void physics_engine_set_context(struct physics_engine *physics_engine,
                                struct physics_engine_environment *environment,
                                struct ring_buffer *event_queue) {
    struct physics_engine_context *context = &physics_engine->context;

    context->environment = environment;
    context->event_queue = event_queue;
}
