#include "game_entity.h"

#include "logging.h"

bool game_entity_init(struct game_entity *game_entity, struct entity *entity,
                      const struct sprite *sprite) {
    position pos = entity->rectangle.p1;

    if (game_entity == NULL) {
        LOG_ERR(
            "Failed to initialize game entity: <game_entity> must not be NULL");
        return false;
    }

    if (entity == NULL) {
        LOG_ERR("Failed to initialize game entity: <entity> must not be NULL");
        return false;
    }

    game_entity->entity = entity;
    game_entity->sprite =
        (struct sprite_component){.map = sprite,
                                  .x = GET_POSITION_GRID_X(pos),
                                  .y = GET_POSITION_GRID_Y(pos)};

    return true;
}

void set_game_entity_position(struct game_entity *game_entity,
                              position new_position) {
    set_entity_position(game_entity->entity, new_position);
    game_entity->sprite.x = GET_POSITION_GRID_X(new_position);
    game_entity->sprite.y = GET_POSITION_GRID_Y(new_position);
}

void set_game_entity_position_relative(struct game_entity *game_entity,
                                       position relative_position) {
    set_entity_position_relative(game_entity->entity, relative_position);
    position new_position = get_entity_position(game_entity->entity);
    game_entity->sprite.x = GET_POSITION_GRID_X(new_position);
    game_entity->sprite.y = GET_POSITION_GRID_Y(new_position);
}

void set_game_entity_velocity(struct game_entity *game_entity,
                              velocity new_velocity) {
    set_entity_velocity(game_entity->entity, new_velocity);
}
void set_game_entity_velocity_relative(struct game_entity *game_entity,
                                       velocity relative_velocity) {
    set_entity_velocity_relative(game_entity->entity, relative_velocity);
}

position get_game_entity_position(struct game_entity *game_entity) {
    return get_entity_position(game_entity->entity);
}

position get_game_entity_sprite_position(struct game_entity *game_entity) {
    return (position){.x = game_entity->sprite.x, .y = game_entity->sprite.y};
}

velocity get_game_entity_velocity(struct game_entity *game_entity) {
    return get_entity_velocity(game_entity->entity);
}

void activate_game_entity(struct game_entity *game_entity) {
    activate_entity(game_entity->entity);
}
void deactivate_game_entity(struct game_entity *game_entity) {
    deactivate_entity(game_entity->entity);
}

bool game_entity_is_active(struct game_entity *game_entity) {
    return game_entity->entity->active;
}

void print_game_entity(struct game_entity *game_entity) {
    LOG_INF("\tEntity: %d", game_entity->entity->entity_idx);
    LOG_INF("\t\t(%d, %d)", game_entity->entity->rectangle.p1.x,
            game_entity->entity->rectangle.p1.y);
    LOG_INF("\tSprite: %d", &game_entity->sprite);
    LOG_INF("\t\t(%d, %d)", game_entity->sprite.x, game_entity->sprite.y);
}