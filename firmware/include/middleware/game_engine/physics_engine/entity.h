#ifndef __ENTITY_H__
#define __ENTITY_H__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kinematics.h"
#include "shapes.h"

/* Enumeration of entity validation errors */
enum entity_validation_error {
    ENTITY_VALID,
    ENTITY_INVALID_TYPE,
    ENTITY_INVALID_POSITION,
};

/* Entity initialization structure */
struct entity_init_struct {
    struct rectangle rectangle __attribute__((aligned(4)));
    velocity velocity __attribute__((aligned(4)));
    acceleration acceleration __attribute__((aligned(4)));
    enum mass mass __attribute__((aligned(4)));
    bool solid __attribute__((aligned(4)));
} __attribute__((aligned(4)));

/* Entity structure */
struct entity {
    struct rectangle rectangle __attribute__((aligned(4)));
    velocity velocity __attribute__((aligned(4)));
    acceleration acceleration __attribute__((aligned(4)));
    enum mass mass __attribute__((aligned(4)));
    bool solid __attribute__((aligned(4)));
    volatile bool active __attribute__((aligned(4)));
    uint8_t entity_idx __attribute__((aligned(4)));
} __attribute__((aligned(4)));

/* Evaluates to true if the give value is a pointer to an entity, else false */
#define IS_ENTITY_POINTER(__p__) \
    (_Generic((__p__), struct entity *: 1, default: 0))

/* Validate an entity_init_struct */
enum entity_validation_error validate_entity_init_struct(
    struct entity_init_struct *init_struct);

/* Setters */
void set_entity_position(struct entity *ent, position new_position);
void set_entity_velocity(struct entity *ent, velocity new_velocity);
void set_entity_acceleration(struct entity *ent, acceleration acceleration);

void set_entity_position_relative(struct entity *ent,
                                  position relative_position);
void set_entity_velocity_relative(struct entity *ent,
                                  velocity relative_velocity);
void set_entity_acceleration_relative(struct entity *ent,
                                      acceleration relative_acceleration);

/* Getters */
position get_entity_position(struct entity *ent);
velocity get_entity_velocity(struct entity *ent);
acceleration get_entity_acceleration(struct entity *ent);

void activate_entity(struct entity *ent);
void deactivate_entity(struct entity *ent);

#endif
