#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H
#include "physics_engine_environment.h"
#include "random_number_generator.h"
#include "sprite.h"

#define UPDATE_PERIOD_MS 1

/* Physics engine configuration structure */
struct physics_engine_config;

/* Physics engine context structure */
struct physics_engine_context;

/* Physics engine structure */
struct physics_engine;

/* Physics engine configuration structure */
struct physics_engine_config {};

/* Physics engine context structure */
struct physics_engine_context {
    struct physics_engine_environment *environment __attribute__((aligned(4)));
    struct ring_buffer *event_queue __attribute__((aligned(4)));

    struct random_number_generator random_number_generator
        __attribute__((aligned(4)));
} __attribute__((aligned(4)));

/* Physics engine structure */
struct physics_engine {
    const struct physics_engine_config config;
    struct physics_engine_context context;
} __attribute__((aligned(4)));

void physics_engine_init(struct physics_engine *physics_engine);
void physics_engine_update(struct physics_engine *physics_engine,
                           uint32_t delta_t);

void physics_engine_set_context(struct physics_engine *physics_engine,
                                struct physics_engine_environment *environment,
                                struct ring_buffer *event_queue);

#endif
