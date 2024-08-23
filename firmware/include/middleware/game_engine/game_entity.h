#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__
#include <stdbool.h>

#include "entity.h"
#include "physics_engine_environment.h"
#include "sprite.h"

union game;

/* Enumeration of possible actions at boundaries */
enum boundary_action {
    BOUNDARY_ACTION_STOP,
    BOUNDARY_ACTION_BOUNCE,
    BOUNDARY_ACTION_FUNCTION,
};

/* Boundary conditions structure */
struct boundary_conditions {
    void (*left_boundary_function)(union game *game);
    void (*right_boundary_function)(union game *game);
    void (*top_boundary_function)(union game *game);
    void (*bottom_boundary_function)(union game *game);
    enum boundary_action left_boundary_action;
    enum boundary_action right_boundary_action;
    enum boundary_action top_boundary_action;
    enum boundary_action bottom_boundary_action;
};

struct game_entity {
    struct entity *entity;
    const struct boundary_conditions *boundary_conditions;
    struct sprite_component sprite;
} __attribute__((aligned(4)));

bool game_entity_init(
    struct game_entity *game_entity, struct entity *entity,
    const struct sprite *sprite,
    const struct boundary_conditions *const boundary_conditions);

void set_game_entity_position(struct game_entity *game_entity,
                              position new_position);
void set_game_entity_position_relative(struct game_entity *game_entity,
                                       position relative_position);
void set_game_entity_velocity(struct game_entity *game_entity,
                              velocity new_velocity);
void set_game_entity_velocity_relative(struct game_entity *game_entity,
                                       velocity relative_velocity);
position get_game_entity_position(struct game_entity *game_entity);
position get_game_entity_sprite_position(struct game_entity *game_entity);
velocity get_game_entity_velocity(struct game_entity *game_entity);

void activate_game_entity(struct game_entity *game_entity);
void deactivate_game_entity(struct game_entity *game_entity);

bool game_entity_is_active(struct game_entity *game_entity);
void print_game_entity(struct game_entity *game_entity);
#include "game.h"
#endif