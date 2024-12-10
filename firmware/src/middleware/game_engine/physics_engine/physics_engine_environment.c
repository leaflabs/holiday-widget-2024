#include "physics_engine_environment.h"

#include <stdbool.h>

#include "environment.h"
#include "logging.h"
#include "physics_engine_events.h"
#include "utils.h"

static bool rectangles_overlap(const struct rectangle *r1,
                               const struct rectangle *r2) {
    return !(r1->p2.x < r2->p1.x ||  // r1 is to the left of r2
             r1->p1.x > r2->p2.x ||  // r1 is to the right of r2
             r1->p1.y > r2->p2.y ||  // r1 is below r2
             r1->p2.y < r2->p1.y);   // r1 is above r2
}

static void update_rectangle_entity(struct ring_buffer *event_queue,
                                    struct entity *ent, uint32_t delta_t) {
    const velocity velocity = ent->velocity;
    struct rectangle *rectangle = &ent->rectangle;
    int32_t max_x_move, max_y_move;

    if (velocity.x == 0) {
        goto y_move;
    }

    int32_t x_displacement = velocity.x * delta_t;
    if (velocity.x > 0) {
        max_x_move = ENVIRONMENT_MAX_X - rectangle->p2.x;
        if (x_displacement >= max_x_move) {
            rectangle->p1.x += max_x_move;
            rectangle->p2.x += max_x_move;

            struct physics_engine_event event = {
                .type = OUT_OF_BOUNDS_EVENT,
                .out_of_bounds_event =
                    {
                        .type = OUT_OF_BOUNDS_RIGHT,
                        .ent = ent,
                    },
            };
            if (ring_buffer_push(event_queue, &event)) {
                LOG_ERR(
                    "Failed to add out of bounds (right) event to event queue: "
                    "event queue full");
            }
        } else {
            rectangle->p1.x += x_displacement;
            rectangle->p2.x += x_displacement;
        }
    } else if (velocity.x < 0) {
        max_x_move = ENVIRONMENT_MIN_X - rectangle->p1.x;
        if (x_displacement <= max_x_move) {
            rectangle->p1.x += max_x_move;
            rectangle->p2.x += max_x_move;
            struct physics_engine_event event = {
                .type = OUT_OF_BOUNDS_EVENT,
                .out_of_bounds_event =
                    {
                        .type = OUT_OF_BOUNDS_LEFT,
                        .ent = ent,
                    },
            };
            if (ring_buffer_push(event_queue, &event)) {
                LOG_ERR(
                    "Failed to add out of bounds (left) event to event queue: "
                    "event queue full");
            }
        } else {
            rectangle->p1.x += x_displacement;
            rectangle->p2.x += x_displacement;
        }
    }
y_move:
    if (velocity.y == 0) {
        return;
    }
    int32_t y_displacement = velocity.y * delta_t;
    if (velocity.y > 0) {
        max_y_move = ENVIRONMENT_MAX_Y - rectangle->p2.y;
        if (y_displacement >= max_y_move) {
            rectangle->p1.y += max_y_move;
            rectangle->p2.y += max_y_move;
            struct physics_engine_event event = {
                .type = OUT_OF_BOUNDS_EVENT,
                .out_of_bounds_event =
                    {
                        .type = OUT_OF_BOUNDS_BOTTOM,
                        .ent = ent,
                    },
            };

            if (ring_buffer_push(event_queue, &event)) {
                LOG_ERR(
                    "Failed to add out of bounds (bottom) event to event "
                    "queue: event queue full");
            }
        } else {
            rectangle->p1.y += y_displacement;
            rectangle->p2.y += y_displacement;
        }
    } else if (velocity.y < 0) {
        max_y_move = ENVIRONMENT_MIN_Y - rectangle->p1.y;
        if (y_displacement <= max_y_move) {
            rectangle->p1.y += max_y_move;
            rectangle->p2.y += max_y_move;
            struct physics_engine_event event = {
                .type = OUT_OF_BOUNDS_EVENT,
                .out_of_bounds_event =
                    {
                        .type = OUT_OF_BOUNDS_TOP,
                        .ent = ent,
                    },
            };
            if (ring_buffer_push(event_queue, &event)) {
                LOG_ERR(
                    "Failed to add out of bounds (top) event to event queue: "
                    "event queue full");
            }
        } else {
            rectangle->p1.y += y_displacement;
            rectangle->p2.y += y_displacement;
        }
    }
}

static bool handle_collision(struct entity *e1, struct entity *e2) {
    // Simple elastic collision response by inverting velocities
    // For more accurate physics, we would need to calculate the collision
    // response based on mass, velocity, etc.
    if (rectangles_overlap(&e1->rectangle, &e2->rectangle)) {
        if (!e1->solid || !e2->solid) {
            return true;
        }
        int32_t mass_diff, mass_sum;
        if (e1->mass != INFINITE_MASS && e2->mass != INFINITE_MASS) {
            mass_diff = e1->mass - e2->mass;
            mass_sum = e1->mass + e2->mass;

            int32_t e1_vel_x = e1->velocity.x;
            int32_t e1_vel_y = e1->velocity.y;

            e1->velocity.x = ((mass_diff * e1->velocity.x) / mass_sum) +
                             (((e2->mass << 1) * e2->velocity.x) / mass_sum);
            e1->velocity.y = ((mass_diff * e1->velocity.y) / mass_sum) +
                             (((e2->mass << 1) * e2->velocity.y) / mass_sum);

            e2->velocity.x = (((e1->mass << 1) * e1_vel_x) / mass_sum) +
                             ((-mass_diff * e2->velocity.x) / mass_sum);
            e2->velocity.y = (((e1->mass << 1) * e1_vel_y) / mass_sum) +
                             ((-mass_diff * e2->velocity.y) / mass_sum);
        } else if (e1->mass == INFINITE_MASS && e2->mass != INFINITE_MASS) {
            e2->velocity.x = (e1->velocity.x) - e2->velocity.x;
            e2->velocity.y = (e1->velocity.y) - e2->velocity.y;

        } else if (e1->mass != INFINITE_MASS && e2->mass == INFINITE_MASS) {
            e1->velocity.x = (e2->velocity.x) - e1->velocity.x;
            e1->velocity.y = (e2->velocity.y) - e1->velocity.y;
        } else {
            /* What happens if two object of infinite mass collide?
            Maybe both of their velocities should just for to 0? */
            e1->velocity.x = 0;
            e1->velocity.y = 0;
            e2->velocity.x = 0;
            e2->velocity.y = 0;
            /*e1->velocity.x = -e1->velocity.x;
            e1->velocity.y = -e1->velocity.y;
            e2->velocity.x = -e2->velocity.x;
            e2->velocity.y = -e2->velocity.y;*/
        }
        return true;
    }
    return false;
}

struct entity_creation_result add_entity(
    struct physics_engine_environment *env,
    struct entity_init_struct *init_struct) {
    struct entity_creation_result result = {.error = ENTITY_CREATION_SUCCESS,
                                            .entity = NULL};
    enum entity_validation_error validation_error;

    switch ((validation_error = validate_entity_init_struct(init_struct))) {
        case ENTITY_VALID:
            break;
        case ENTITY_INVALID_POSITION:
            result.error = ENTITY_CREATION_INVALID_POSITION;
            return result;
        default:
            LOG_ERR("Unknown entity_validation_error: %d", validation_error);
            result.error = ENTITY_CREATION_UNKNOWN_ERROR;
            return result;
    }

    /* Ensure that the maximum entity count is not exceeded */
    if (env->num_of_entities >= MAX_ENTITIES) {
        result.error = ENTITY_CREATION_TOO_MANY_ENTITIES;
        return result;
    }

    /* Grab the next available entity slot */
    struct entity *new_entity = &env->entities[env->num_of_entities];

    /* Set entity position */
    new_entity->rectangle = init_struct->rectangle;

    /* Set entity mass */
    new_entity->mass = init_struct->mass;

    /* Set entity velocity */
    new_entity->velocity = init_struct->velocity;

    /* Set entity acceleration */
    new_entity->acceleration = init_struct->acceleration;

    /* Set entity solid flag */
    new_entity->solid = init_struct->solid;

    /* Initialize entity as inactive */
    new_entity->active = false;

    /* Set and increment entity idx */
    new_entity->entity_idx = env->num_of_entities;
    env->num_of_entities += 1;

    /* Place initialized entity pointer in result */
    result.entity = new_entity;

    return result;
}

static inline int sign(int x) {
    return (x > 0) - (x < 0);
}

void physics_engine_environment_update(struct ring_buffer *event_queue,
                                       struct physics_engine_environment *env,
                                       uint32_t delta_t) {
    if (env->paused) {
        return;
    }

    uint32_t t0_1 = TIM21->CNT;

    // Update positions
    for (int i = 0; i < env->num_of_entities; i++) {
        struct entity *ent = &env->entities[i];
        if (ent->active) {
            update_rectangle_entity(event_queue, ent, delta_t);

            if (ent->acceleration.x != 0) {
                ent->velocity.x += ent->acceleration.x * delta_t;
            }
            if (ent->acceleration.y != 0) {
                ent->velocity.y += ent->acceleration.y * delta_t;
            }
        }
    }

    uint32_t t1 = TIM21->CNT;

    // Check for collisions
    for (int i = 0; i < env->num_of_entities; i++) {
        struct entity *ent1 = &env->entities[i];
        if (ent1->active) {
            velocity initial_velocity = ent1->velocity;
            uint8_t num_collisions = 0;
            for (int j = i + 1; j < env->num_of_entities; j++) {
                struct entity *ent2 = &env->entities[j];
                /* TODO: Necessary?? */
                if (ent1->active && ent2->active) {
                    /* TODO: Event Queue?? */
                    if (handle_collision(ent1, ent2)) {
                        LOG_DBG(
                            "Collision Occurred between: ent1=<%d> and "
                            "ent2=<%d>",
                            ent1->entity_idx, ent2->entity_idx);
                        struct physics_engine_event event = {
                            .type = COLLISION_EVENT,
                            .collision_event =
                                {
                                    .ent1 = ent1,
                                    .ent2 = ent2,
                                },
                        };
                        if (ring_buffer_push(event_queue, &event)) {
                            LOG_ERR(
                                "Failed to add collision event to event queue: "
                                "event queue full");
                        }
                        num_collisions++;
                        break;
                    }
                }
            }
        }
    }

    uint32_t t2 = TIM21->CNT;
    uint32_t duration1 = t1 - t0_1;
    uint32_t duration2 = t2 - t1;
}

void print_physics_engine_environment(
    const struct physics_engine_environment *env) {
    LOG_INF("Environment:");
    LOG_INF("\tEntity Count: %u", env->num_of_entities);
    LOG_INF("\tEntities:");

    for (int i = 0; i < env->num_of_entities; i++) {
        struct entity ent = env->entities[i];

        LOG_INF("\t\tEntity %u:", i);
        LOG_INF("\t\t\tPosition 1: (%d, %d)", ent.rectangle.p1.x,
                ent.rectangle.p1.y);
        LOG_INF("\t\t\tPosition 2: (%d, %d)", ent.rectangle.p2.x,
                ent.rectangle.p2.y);
        LOG_INF("\t\t\tVelocity: (%d, %d)", ent.velocity.x, ent.velocity.y);
        LOG_INF("\t\t\tSolid: %u", ent.solid);
        LOG_INF("\t\t\tValid: %u", ent.active);
        LOG_INF("\t\t\tIdx: %u", ent.entity_idx);
    }
}

void physics_engine_environment_pause(struct physics_engine_environment *env) {
    env->paused = true;
}

void physics_engine_environment_unpause(
    struct physics_engine_environment *env) {
    env->paused = false;
}
