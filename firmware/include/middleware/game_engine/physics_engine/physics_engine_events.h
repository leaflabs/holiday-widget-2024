#ifndef __PHYSICS_ENGINE_EVENT_H__
#define __PHYSICS_ENGINE_EVENT_H__
#include <stdbool.h>
#include <stdint.h>

#include "entity.h"
#include "ring_buffer.h"
#include "utils.h"
#define EVENT_QUEUE_SIZE 32

#if EVENT_QUEUE_SIZE > INT8_MAX
#error "EVENT_QUEUE_SIZE exceeds max value of data type (int8_t)"
#endif

/* Enumeration of physics engine event types */
enum physics_engine_event_type;

/* Physics engine event structure */
struct physics_engine_event;

/* Enumeration of physics engine event types */
enum physics_engine_event_type {
    OUT_OF_BOUNDS_EVENT,
    COLLISION_EVENT,
};

enum physics_engine_out_of_bounds_type {
    OUT_OF_BOUNDS_LEFT,
    OUT_OF_BOUNDS_RIGHT,
    OUT_OF_BOUNDS_TOP,
    OUT_OF_BOUNDS_BOTTOM,
};

static const char *physics_engine_out_of_bounds_type_to_str[] = {
    [OUT_OF_BOUNDS_LEFT] = "OUT_OF_BOUNDS_LEFT",
    [OUT_OF_BOUNDS_RIGHT] = "OUT_OF_BOUNDS_RIGHT",
    [OUT_OF_BOUNDS_TOP] = "OUT_OF_BOUNDS_TOP",
    [OUT_OF_BOUNDS_BOTTOM] = "OUT_OF_BOUNDS_BOTTOM",
};

struct physics_engine_out_of_bounds_event {
    enum physics_engine_out_of_bounds_type type;
    struct entity *ent;
};

struct physics_engine_collision_event {
    struct entity *ent1;
    struct entity *ent2;
};

/* Physics engine event structure */
struct physics_engine_event {
    enum physics_engine_event_type type;
    union {
        struct physics_engine_out_of_bounds_event out_of_bounds_event;
        struct physics_engine_collision_event collision_event;
    };
};

#endif
