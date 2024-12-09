#ifndef __PHYSICS_ENGINE_ENVIRONMENT_H__
#define __PHYSICS_ENGINE_ENVIRONMENT_H__
#include <stdint.h>

#include "entity.h"
#include "environment.h"
#include "physics_engine_events.h"
#define MAX_ENTITIES 32

#include "entity.h"

/* Enumeration of entity creation errors */
enum entity_creation_error;

/* Entity creation result structure */
struct entity_creation_result;

/* Environment structure */
struct physics_engine_environment;

/* Enumeration of entity creation errors */
enum entity_creation_error {
    ENTITY_CREATION_SUCCESS,
    ENTITY_CREATION_INVALID_TYPE,
    ENTITY_CREATION_INVALID_POSITION,
    ENTITY_CREATION_INVALID_RADIUS,
    ENTITY_CREATION_OUT_OF_BOUNDS,
    ENTITY_CREATION_TOO_MANY_ENTITIES,
    ENTITY_CREATION_UNKNOWN_ERROR,
};

/* Entity creation result structure */
struct entity_creation_result {
    enum entity_creation_error error;
    struct entity *entity;
} __attribute__((aligned(4)));

/* Environment structure */
struct physics_engine_environment {
    struct entity entities[MAX_ENTITIES] __attribute__((aligned(4)));
    uint32_t num_of_entities __attribute__((aligned(4)));
    bool paused;
} __attribute__((aligned(4)));

/* Add an entity to the given environment based on the provided init struct */
struct entity_creation_result add_entity(
    struct physics_engine_environment *env,
    struct entity_init_struct *init_struct);

/* Updates the provided physics engine environment */
void physics_engine_environment_update(
    struct physics_engine_event_queue *event_queue,
    struct physics_engine_environment *env, uint32_t delta_t);

/* Prints the provided physics engine environment */
void print_physics_engine_environment(
    const struct physics_engine_environment *env);

void physics_engine_environment_pause(struct physics_engine_environment *env);

void physics_engine_environment_unpause(struct physics_engine_environment *env);

#endif
