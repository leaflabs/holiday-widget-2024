#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__
#include <stdbool.h>

#include "entity.h"
#include "physics_engine_environment.h"
#include "sprite.h"

union game;

struct game_entity {
    struct entity *entity;
    struct sprite_component sprite;
} __attribute__((aligned(4)));

bool game_entity_init(struct game_entity *game_entity, struct entity *entity,
                      const struct sprite *sprite);

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