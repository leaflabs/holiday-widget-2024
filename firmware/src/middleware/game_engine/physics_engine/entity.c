#include "entity.h"

#include "logging.h"
#include "physics_engine_environment.h"

enum entity_validation_error validate_entity_init_struct(
    struct entity_init_struct *init_struct) {
    if (!valid_rectangle(&init_struct->rectangle)) {
        return ENTITY_INVALID_POSITION;
    }
    return ENTITY_VALID;
}

void set_entity_position(struct entity *ent, position new_position) {
    int32_t max_x_move, max_y_move;
    position displacement;

    struct rectangle *rectangle = &ent->rectangle;
    displacement = (position){new_position.x - rectangle->p1.x,
                              new_position.y - rectangle->p1.y};
    if (displacement.x >= 0) {
        max_x_move = ENVIRONMENT_MAX_X - rectangle->p2.x;
        if (displacement.x > max_x_move) {
            rectangle->p1.x += max_x_move;
            rectangle->p2.x += max_x_move;
        } else {
            rectangle->p1.x += displacement.x;
            rectangle->p2.x += displacement.x;
        }
    } else {
        max_x_move = ENVIRONMENT_MIN_X - rectangle->p1.x;
        if (displacement.x < max_x_move) {
            rectangle->p1.x += max_x_move;
            rectangle->p2.x += max_x_move;
        } else {
            rectangle->p1.x += displacement.x;
            rectangle->p2.x += displacement.x;
        }
    }

    if (displacement.y >= 0) {
        max_y_move = ENVIRONMENT_MAX_Y - rectangle->p2.y;
        if (displacement.y > max_y_move) {
            rectangle->p1.y += max_y_move;
            rectangle->p2.y += max_y_move;
        } else {
            rectangle->p1.y += displacement.y;
            rectangle->p2.y += displacement.y;
        }
    } else {
        max_y_move = ENVIRONMENT_MIN_Y - rectangle->p1.y;
        if (displacement.y < max_y_move) {
            rectangle->p1.y += max_y_move;
            rectangle->p2.y += max_y_move;
        } else {
            rectangle->p1.y += displacement.y;
            rectangle->p2.y += displacement.y;
        }
    }
}

void set_entity_velocity(struct entity *ent, velocity new_velocity) {
    ent->velocity = new_velocity;
}

void set_entity_acceleration(struct entity *ent, acceleration acceleration) {
    ent->acceleration = acceleration;
}

void set_entity_position_relative(struct entity *ent,
                                  position relative_position) {
    position p = get_entity_position(ent);

    p.x += relative_position.x;
    p.y += relative_position.y;

    set_entity_position(ent, p);
}

void set_entity_velocity_relative(struct entity *ent,
                                  velocity relative_velocity) {
    velocity v = get_entity_velocity(ent);

    v.x += relative_velocity.x;
    v.y += relative_velocity.y;

    set_entity_velocity(ent, v);
}

void set_entity_acceleration_relative(struct entity *ent,
                                      acceleration relative_acceleration) {
    acceleration a = get_entity_acceleration(ent);

    a.x += relative_acceleration.x;
    a.y += relative_acceleration.y;

    set_entity_acceleration(ent, a);
}

position get_entity_position(struct entity *ent) {
    return ent->rectangle.p1;
}

velocity get_entity_velocity(struct entity *ent) {
    return ent->velocity;
}

acceleration get_entity_acceleration(struct entity *ent) {
    return ent->acceleration;
}

void activate_entity(struct entity *ent) {
    ent->active = true;
}

void deactivate_entity(struct entity *ent) {
    ent->active = false;
}
