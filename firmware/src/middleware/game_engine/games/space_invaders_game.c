#include "space_invaders_game.h"

#include "game.h"
#include "logging.h"
#include "music_player.h"
#include "utils.h"

extern void pause_game_engine(int32_t duration);
extern void unpause_game_engine(void);

#define SPACE_INVADERS_FREEZE_TIME 2000

#define SPACE_INVADERS_USER_SHIP_ENTITY_IDX 0
#define SPACE_INVADERS_ENEMY_SHIP_FIRST_ENTITY_IDX 1
#define SPACE_INVADERS_ENEMY_SHIP_LAST_ENTITY_IDX \
    (SPACE_INVADERS_ENEMY_SHIP_FIRST_ENTITY_IDX + \
     SPACE_INVADERS_NUM_OF_ENEMY_SHIPS - 1)
#define SPACE_INVADERS_USER_BULLET_FIRST_ENTITY_IDX \
    (SPACE_INVADERS_ENEMY_SHIP_LAST_ENTITY_IDX + 1)
#define SPACE_INVADERS_USER_BULLET_LAST_ENTITY_IDX \
    (SPACE_INVADERS_USER_BULLET_FIRST_ENTITY_IDX + \
     SPACE_INVADERS_MAX_USER_BULLETS - 1)
#define SPACE_INVADERS_ENEMY_BULLET_FIRST_ENTITY_IDX \
    (SPACE_INVADERS_USER_BULLET_LAST_ENTITY_IDX + 1)
#define SPACE_INVADERS_ENEMY_BULLET_LAST_ENTITY_IDX \
    (SPACE_INVADERS_ENEMY_BULLET_FIRST_ENTITY_IDX + \
     SPACE_INVADERS_MAX_ENEMY_BULLETS - 1)

#define SPACE_INVADERS_LAST_ENTITY_IDX \
    SPACE_INVADERS_ENEMY_BULLET_LAST_ENTITY_IDX

/* Evaluates to true if the give value is a pointer to the user paddle entity,
 * else false */
#define IS_USER_SHIP_ENTITY(__ent__)                                         \
    (IS_ENTITY_POINTER(__ent__) && ((struct entity *)__ent__)->entity_idx == \
                                       SPACE_INVADERS_USER_SHIP_ENTITY_IDX)

/* Evaluates to true if the give value is a pointer to an enemy ship entity,
 * else false */
#define IS_ENEMY_SHIP_ENTITY(__ent__)                  \
    (IS_ENTITY_POINTER(__ent__) &&                     \
     ((struct entity *)__ent__)->entity_idx >=         \
         SPACE_INVADERS_ENEMY_SHIP_FIRST_ENTITY_IDX && \
     ((struct entity *)__ent__)->entity_idx <=         \
         SPACE_INVADERS_ENEMY_SHIP_LAST_ENTITY_IDX)

/* Evaluates to true if the give value is a pointer to a user bullet entity,
 * else false */
#define IS_USER_BULLET_ENTITY(__ent__)                  \
    (IS_ENTITY_POINTER(__ent__) &&                      \
     ((struct entity *)__ent__)->entity_idx >=          \
         SPACE_INVADERS_USER_BULLET_FIRST_ENTITY_IDX && \
     ((struct entity *)__ent__)->entity_idx <=          \
         SPACE_INVADERS_USER_BULLET_LAST_ENTITY_IDX)

/* Evaluates to true if the give value is a pointer to an enemy bullet entity,
 * else false */
#define IS_ENEMY_BULLET_ENTITY(__ent__)                  \
    (IS_ENTITY_POINTER(__ent__) &&                       \
     ((struct entity *)__ent__)->entity_idx >=           \
         SPACE_INVADERS_ENEMY_BULLET_FIRST_ENTITY_IDX && \
     ((struct entity *)__ent__)->entity_idx <=           \
         SPACE_INVADERS_ENEMY_BULLET_LAST_ENTITY_IDX)

enum entity_creation_error space_invaders_game_init(
    struct space_invaders_game *space_invaders_game) {
    const struct space_invaders_game_config *config =
        &space_invaders_game->config;
    struct space_invaders_game_context *context = &space_invaders_game->context;

    game_common_init(&context->game_common);

    /******************/
    /*  Add Entities  */
    /******************/
    struct entity_creation_result result;

    /* Add user ship entity */
    result = add_entity(&context->game_common.environment,
                        config->user_ship_init_struct);
    if (result.error != ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Failed to create user ship entity: %d", result.error);
        return result.error;
    } else if (!game_entity_init(&context->user_ship, result.entity,
                                 SPACE_INVADERS_USER_SHIP_SPRITE)) {
        LOG_ERR("Failed to create user ship game entity");
        result.error = ENTITY_CREATION_INVALID_TYPE;
        return result.error;
    }

    /* Add enemy ship entities */
    for (int i = 0; i < SPACE_INVADERS_NUM_OF_ENEMY_SHIPS; i++) {
        result = add_entity(&context->game_common.environment,
                            config->enemy_ship_init_struct);
        if (result.error != ENTITY_CREATION_SUCCESS) {
            LOG_ERR("Failed to create enemy ship entity %d: %d", i,
                    result.error);
            return result.error;
        } else {
            bool ret = game_entity_init(&context->enemy_ships[i], result.entity,
                                        SPACE_INVADERS_ENEMY_SHIP_SPRITE);
            if (!ret) {
                LOG_ERR("Failed to create enemy ship game entity %d", i);
                result.error = ENTITY_CREATION_INVALID_TYPE;
                return result.error;
            }
        }
        set_game_entity_position(&context->enemy_ships[i],
                                 ENEMY_SHIP_START_POSITIONS_TOP_LEFT[i]);
    }

    /* Add user bullet entities */
    for (int i = 0; i < SPACE_INVADERS_MAX_USER_BULLETS; i++) {
        result = add_entity(&context->game_common.environment,
                            config->user_bullet_init_struct);
        if (result.error != ENTITY_CREATION_SUCCESS) {
            LOG_ERR("Failed to create user bullet entity %d: %d", i,
                    result.error);
            return result.error;
        } else if (!game_entity_init(&context->user_bullets[i], result.entity,
                                     SPACE_INVADERS_USER_BULLET_SPRITE)) {
            LOG_ERR("Failed to create user bullet game entity %d", i);
            result.error = ENTITY_CREATION_INVALID_TYPE;
            return result.error;
        }
    }

    /* Add enemy bullet entities */
    for (int i = 0; i < SPACE_INVADERS_MAX_ENEMY_BULLETS; i++) {
        result = add_entity(&context->game_common.environment,
                            config->enemy_bullet_init_struct);
        if (result.error != ENTITY_CREATION_SUCCESS) {
            LOG_ERR("Failed to create enemy bullet entity %d: %d", i,
                    result.error);
            return result.error;
        } else if (!game_entity_init(&context->enemy_bullets[i], result.entity,
                                     SPACE_INVADERS_ENEMY_BULLET_SPRITE)) {
            LOG_ERR("Failed to create enemy bullet game entity %d", i);
            result.error = ENTITY_CREATION_INVALID_TYPE;
            return result.error;
        }
    }

    activate_game_entity(&context->user_ship);
    for (int i = 0; i < SPACE_INVADERS_NUM_OF_ENEMY_SHIPS; i++) {
        activate_game_entity(&context->enemy_ships[i]);
    }

    /* Reset lives */
    context->num_of_user_bullets = 0;
    context->num_of_enemy_bullets = 0;
    context->enemies_remaining = SPACE_INVADERS_NUM_OF_ENEMY_SHIPS;
    context->lives = 3;
    context->last_enemy_bullet_time = HAL_GetTick();

    return 0;
}

static struct game_entity *get_random_front_enemy(
    struct space_invaders_game *space_invaders_game,
    const struct random_number_generator *rng) {
    struct space_invaders_game_context *context = &space_invaders_game->context;
    struct game_entity *front_most_ships[GRID_SIZE] = {NULL};

    for (int i = 0; i < SPACE_INVADERS_NUM_OF_ENEMY_SHIPS; i++) {
        struct game_entity *enemy_ship = &context->enemy_ships[i];
        if (game_entity_is_active(enemy_ship)) {
            position p1 = get_game_entity_position(enemy_ship);
            uint32_t col = (p1.x + ENVIRONMENT_MAX_X) / GRID_UNIT_SIZE;
            if (front_most_ships[col] == NULL ||
                (get_game_entity_position(front_most_ships[col]).y <= p1.y)) {
                front_most_ships[col] = enemy_ship;
            }
        }
    }
    uint32_t available_ships = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (front_most_ships[i] != NULL) {
            available_ships++;
        }
    }

    uint32_t idx = random_number_generator_get_next_in_n(rng, available_ships);
    uint32_t active_ship_idx = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (front_most_ships[i] != NULL) {
            if (active_ship_idx++ == idx) {
                return front_most_ships[i];
            }
        }
    }
    return NULL;
}

enum entity_creation_error space_invaders_shoot_user_bullet(
    struct space_invaders_game *space_invaders_game) {
    struct space_invaders_game_context *context = &space_invaders_game->context;
    position user_ship_position = get_game_entity_position(&context->user_ship);

    if (context->num_of_user_bullets >= SPACE_INVADERS_MAX_USER_BULLETS) {
        LOG_ERR("Number of user_bullets exceeds max number of user_bullets: %d",
                context->num_of_user_bullets);
        /* Probably want to make my own errors for this */
        return ENTITY_CREATION_TOO_MANY_ENTITIES;
    }
    struct game_entity *bullet = NULL;
    for (int i = 0; i < SPACE_INVADERS_MAX_USER_BULLETS; i++) {
        bullet = &context->user_bullets[i];
        if (!bullet->entity->active) {
            break;
        }
    }
    if (bullet == NULL) {
        LOG_ERR("bullet == NULL");
        while (1);
    }
    position bullet_position =
        (position){user_ship_position.x, user_ship_position.y - GRID_UNIT_SIZE};

    set_game_entity_position(bullet, bullet_position);
    activate_game_entity(bullet);

    bool overlap(struct rectangle * r1, struct rectangle * r2) {
        return !(r1->p2.x < r2->p1.x ||  // r1 is to the left of r2
                 r1->p1.x > r2->p2.x ||  // r1 is to the right of r2
                 r1->p1.y > r2->p2.y ||  // r1 is below r2
                 r1->p2.y < r2->p1.y);
    }

    if (overlap(&bullet->entity->rectangle,
                &context->user_ship.entity->rectangle)) {
        LOG_ERR("User Bullet ALREADY overlaps with User Ship");
    }

    context->num_of_user_bullets++;
    LOG_DBG("Shot User Bullet <%d> - incremented to %d",
            bullet->entity->entity_idx, context->num_of_user_bullets);
    return ENTITY_CREATION_SUCCESS;
}

enum entity_creation_error space_invaders_shoot_enemy_bullet(
    struct space_invaders_game *space_invaders_game,
    const struct random_number_generator *rng) {
    struct space_invaders_game_context *context = &space_invaders_game->context;
    struct game_entity *enemy_ship =
        get_random_front_enemy(space_invaders_game, rng);
    position enemy_ship_position = get_game_entity_position(enemy_ship);

    if (context->num_of_enemy_bullets >= SPACE_INVADERS_MAX_ENEMY_BULLETS) {
        LOG_ERR(
            "Number of enemy_bullets exceeds max number of enemy_bullets: "
            "%d",
            context->num_of_enemy_bullets);
        // Probably want to make my own errors for this
        return ENTITY_CREATION_TOO_MANY_ENTITIES;
    }
    struct game_entity *bullet = NULL;
    for (int i = 0; i < SPACE_INVADERS_MAX_ENEMY_BULLETS; i++) {
        bullet = &context->enemy_bullets[i];
        if (!bullet->entity->active) {
            break;
        }
    }
    if (bullet == NULL) {
        LOG_ERR("bullet == NULL");
        while (1);
    }
    position bullet_position = (position){
        enemy_ship_position.x, enemy_ship_position.y + GRID_UNIT_SIZE};

    set_game_entity_position(bullet, bullet_position);
    activate_game_entity(bullet);

    context->num_of_enemy_bullets++;
    LOG_DBG("Shot Enemy Bullet <%d> - incremented to %d",
            bullet->entity->entity_idx, context->num_of_enemy_bullets);
    return ENTITY_CREATION_SUCCESS;
}

void update_space_invaders_game(struct space_invaders_game *space_invaders_game,
                                const struct random_number_generator *rng) {
    struct space_invaders_game_context *context = &space_invaders_game->context;
    uint32_t current_time = HAL_GetTick();

    if (current_time - context->last_enemy_bullet_time >= ENEMY_BULLET_PERIOD) {
        context->last_enemy_bullet_time = current_time;
        space_invaders_shoot_enemy_bullet(space_invaders_game, rng);
    }

    if (current_time - context->last_user_bullet_time >= USER_BULLET_PERIOD) {
        context->last_user_bullet_time = current_time;
        space_invaders_shoot_user_bullet(space_invaders_game);
    }
}

static void handle_user_ship_enemy_bullet_collision(
    struct space_invaders_game *space_invaders_game, struct entity *user_ship,
    struct entity *enemy_bullet) {
    struct space_invaders_game_context *context = &space_invaders_game->context;
    deactivate_entity(enemy_bullet);
    context->num_of_enemy_bullets--;
    LOG_DBG("Enemy Bullet <%d> hit User Ship: Decremented enemy bullets - %d",
            enemy_bullet->entity_idx, context->num_of_enemy_bullets);
    enum music_player_error error =
        music_player_play_song(&music_player, FAILURE_SOUND);
    if (error != MUSIC_PLAYER_NO_ERROR) {
        LOG_ERR("Music player failed to play failure sound: %d", error);
    }
    if (--context->lives <= 0) {
        deactivate_game_entity(&context->user_ship);
        for (int i = 0; i < SPACE_INVADERS_NUM_OF_ENEMY_SHIPS; i++) {
            deactivate_game_entity(&context->enemy_ships[i]);
        }
        for (int i = 0; i < SPACE_INVADERS_MAX_USER_BULLETS; i++) {
            deactivate_game_entity(&context->user_bullets[i]);
        }
        for (int i = 0; i < SPACE_INVADERS_MAX_ENEMY_BULLETS; i++) {
            deactivate_game_entity(&context->enemy_bullets[i]);
        }

        context->game_common.game_state = GAME_STATE_YOU_LOSE;
    } else {
        physics_engine_environment_pause(&context->game_common.environment);
        context->game_common.game_state = GAME_STATE_SCORE_CHANGE;
    }

    pause_game_engine(SPACE_INVADERS_FREEZE_TIME);
}

static void handle_enemy_ship_user_bullet_collision(
    struct space_invaders_game *space_invaders_game, struct entity *enemy_ship,
    struct entity *user_bullet) {
    struct space_invaders_game_context *context = &space_invaders_game->context;

    deactivate_entity(enemy_ship);
    deactivate_entity(user_bullet);
    context->num_of_user_bullets--;
    LOG_DBG(
        "User Bullet <%d> hit Enemy Ship <%d>: Decremented enemy bullets - %d",
        user_bullet->entity_idx, enemy_ship->entity_idx,
        context->num_of_enemy_bullets);
    if (--context->enemies_remaining <= 0) {
        deactivate_game_entity(&context->user_ship);
        for (int i = 0; i < SPACE_INVADERS_NUM_OF_ENEMY_SHIPS; i++) {
            deactivate_game_entity(&context->enemy_ships[i]);
        }
        for (int i = 0; i < SPACE_INVADERS_MAX_USER_BULLETS; i++) {
            deactivate_game_entity(&context->user_bullets[i]);
        }
        for (int i = 0; i < SPACE_INVADERS_MAX_ENEMY_BULLETS; i++) {
            deactivate_game_entity(&context->enemy_bullets[i]);
        }
        /* Display win screen */
        /*LOG_INF("You win!");
        space_invaders_game_reset(space_invaders_game);
        LOG_INF("Game Reset");*/
        enum music_player_error error =
            music_player_play_song(&music_player, SUCCESS_SOUND);
        if (error != MUSIC_PLAYER_NO_ERROR) {
            LOG_ERR("Music player failed to play success sound: %d", error);
        }
        context->game_common.game_state = GAME_STATE_YOU_WIN;
    }
}

static void handle_user_bullet_enemy_bullet_collision(
    struct space_invaders_game *space_invaders_game, struct entity *user_bullet,
    struct entity *enemy_bullet) {
    struct space_invaders_game_context *context = &space_invaders_game->context;

    deactivate_entity(user_bullet);
    deactivate_entity(enemy_bullet);
    context->num_of_user_bullets--;
    context->num_of_enemy_bullets--;
    LOG_DBG(
        "User Bullet <%d> and Enemy Bullet <%d> collision: decremented "
        "both bullets - %d, %d",
        user_bullet->entity_idx, enemy_bullet->entity_idx,
        context->num_of_user_bullets, context->num_of_enemy_bullets);
}

void space_invaders_game_process_event_queue(
    struct space_invaders_game *space_invaders_game) {
    struct space_invaders_game_context *context = &space_invaders_game->context;
    struct physics_engine_event_queue *event_queue = &context->game_common.event_queue;

    struct physics_engine_event event;

    while (physics_engine_event_queue_dequeue(event_queue, &event)) {
        switch (event.type) {
            case OUT_OF_BOUNDS_EVENT: {
                struct physics_engine_out_of_bounds_event *out_of_bounds_event =
                    &event.out_of_bounds_event;
                struct entity *ent = out_of_bounds_event->ent;

                switch (out_of_bounds_event->type) {
                    case OUT_OF_BOUNDS_TOP:
                        if (IS_USER_BULLET_ENTITY(ent)) {
                            deactivate_entity(ent);
                            context->num_of_user_bullets--;
                            LOG_DBG(
                                "User Bullet <%d> out of bounds: decremented "
                                "user "
                                "bullets - %d",
                                ent->entity_idx, context->num_of_user_bullets);
                        } else if (IS_USER_SHIP_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: User Ship "
                                "OUT_OF_BOUNDS_TOP");
                        } else if (IS_ENEMY_SHIP_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: Enemy Ship "
                                "<%d> OUT_OF_BOUNDS_TOP",
                                ent->entity_idx);
                        } else if (IS_ENEMY_BULLET_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: Enemy Bullet "
                                "<%d> OUT_OF_BOUNDS_TOP",
                                ent->entity_idx);
                        } else {
                            LOG_ERR(
                                "Unexpected out of bounds event: Unknown "
                                "entity <%d> OUT_OF_BOUNDS_TOP",
                                ent->entity_idx);
                        }

                        break;

                    case OUT_OF_BOUNDS_BOTTOM:
                        if (IS_ENEMY_BULLET_ENTITY(ent)) {
                            deactivate_entity(ent);
                            context->num_of_enemy_bullets--;
                            LOG_DBG(
                                "Enemy Bullet <%d> out of bounds: decremented "
                                "enemy "
                                "bullets - %d",
                                ent->entity_idx, context->num_of_enemy_bullets);
                        } else if (IS_USER_SHIP_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: User Ship "
                                "OUT_OF_BOUNDS_BOTTOM");
                        } else if (IS_ENEMY_SHIP_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: Enemy Ship "
                                "<%d> OUT_OF_BOUNDS_BOTTOM",
                                ent->entity_idx);
                        } else if (IS_USER_BULLET_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: User Bullet "
                                "<%d> OUT_OF_BOUNDS_BOTTOM",
                                ent->entity_idx);
                        } else {
                            LOG_ERR(
                                "Unexpected out of bounds event: Unknown "
                                "entity <%d> OUT_OF_BOUNDS_BOTTOM",
                                ent->entity_idx);
                        }

                        break;

                    case OUT_OF_BOUNDS_LEFT: /* fall-through */
                    case OUT_OF_BOUNDS_RIGHT:
                        if (IS_USER_SHIP_ENTITY(ent)) {
                            break;
                        } else if (IS_ENEMY_SHIP_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: Enemy Ship %s",
                                physics_engine_out_of_bounds_type_to_str
                                    [out_of_bounds_event->type]);
                        } else if (IS_USER_BULLET_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: User Bullet "
                                "<%d> %s",
                                ent->entity_idx,
                                physics_engine_out_of_bounds_type_to_str
                                    [out_of_bounds_event->type]);
                        } else if (IS_ENEMY_BULLET_ENTITY(ent)) {
                            LOG_ERR(
                                "Unexpected out of bounds event: Enemy Bullet "
                                "<%d> %s",
                                ent->entity_idx,
                                physics_engine_out_of_bounds_type_to_str
                                    [out_of_bounds_event->type]);
                        } else {
                            LOG_ERR(
                                "Unexpected out of bounds event: Unknown "
                                "entity <%d> %s",
                                ent->entity_idx,
                                physics_engine_out_of_bounds_type_to_str
                                    [out_of_bounds_event->type]);
                        }

                        break;
                    default:
                        LOG_ERR("Unknown out of bound event type: %d",
                                out_of_bounds_event->type);
                        break;
                }
            } break;
            case COLLISION_EVENT: {
                struct physics_engine_collision_event *collision_event =
                    &event.collision_event;
                struct entity *ent1 = collision_event->ent1;
                struct entity *ent2 = collision_event->ent2;

                if (IS_USER_SHIP_ENTITY(ent1)) {
                    if (IS_ENEMY_BULLET_ENTITY(ent2)) {
                        handle_user_ship_enemy_bullet_collision(
                            space_invaders_game, ent1, ent2);
                    } else if (IS_USER_BULLET_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: User Bullet <%d> hit "
                            "User Ship",
                            ent2->entity_idx);
                    } else if (IS_ENEMY_SHIP_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: Enemy Ship <%d> hit "
                            "User Ship",
                            ent2->entity_idx);
                    } else if (IS_USER_SHIP_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: User Ship hit User "
                            "Ship?!");
                    } else {
                        LOG_ERR(
                            "Unexpected collision event: User Ship hit by "
                            "unknown entity <%d>",
                            ent2->entity_idx);
                    }
                } else if (IS_ENEMY_SHIP_ENTITY(ent1)) {
                    if (IS_USER_BULLET_ENTITY(ent2)) {
                        handle_enemy_ship_user_bullet_collision(
                            space_invaders_game, ent1, ent2);
                    } else if (IS_ENEMY_BULLET_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: Enemy Bullet <%d> hit "
                            "Enemy Ship <%d>",
                            ent2->entity_idx, ent1->entity_idx);
                    } else if (IS_ENEMY_SHIP_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: Enemy Ship <%d> hit "
                            "Enemy Ship <%d>",
                            ent2->entity_idx, ent1->entity_idx);
                    } else if (IS_USER_SHIP_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: User Ship hit Enemy "
                            "Ship <%d>",
                            ent1->entity_idx);
                    } else {
                        LOG_ERR(
                            "Unexpected collision event: Enemy Ship <%d> hit "
                            "by unknown entity <%d>",
                            ent1->entity_idx, ent2->entity_idx);
                    }
                } else if (IS_USER_BULLET_ENTITY(ent1)) {
                    if (IS_ENEMY_SHIP_ENTITY(ent2)) {
                        handle_enemy_ship_user_bullet_collision(
                            space_invaders_game, ent2, ent1);
                    } else if (IS_ENEMY_BULLET_ENTITY(ent2)) {
                        handle_user_bullet_enemy_bullet_collision(
                            space_invaders_game, ent1, ent2);
                    } else if (IS_USER_SHIP_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: User Ship hit User "
                            "Bullet <%d>",
                            ent1->entity_idx);
                    } else if (IS_USER_BULLET_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: User Bullet <%d> hit "
                            "User Bullet <%d>",
                            ent2->entity_idx, ent1->entity_idx);
                    } else {
                        LOG_ERR(
                            "Unexpected collision event: User Bullet <%d> hit "
                            "by unknown entity <%d>",
                            ent1->entity_idx, ent2->entity_idx);
                    }
                } else if (IS_ENEMY_BULLET_ENTITY(ent1)) {
                    if (IS_USER_SHIP_ENTITY(ent2)) {
                        handle_user_ship_enemy_bullet_collision(
                            space_invaders_game, ent2, ent1);
                    } else if (IS_USER_BULLET_ENTITY(ent2)) {
                        handle_user_bullet_enemy_bullet_collision(
                            space_invaders_game, ent2, ent1);
                    } else if (IS_ENEMY_SHIP_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: Enemy Bullet <%d> hit "
                            "Enemy Ship <%d>",
                            ent1->entity_idx, ent2->entity_idx);
                    } else if (IS_ENEMY_BULLET_ENTITY(ent2)) {
                        LOG_ERR(
                            "Unexpected collision event: Enemy Bullet <%d> hit "
                            "Enemy Bullet <%d>",
                            ent1->entity_idx, ent2->entity_idx);
                    } else {
                        LOG_ERR(
                            "Unexpected collision event: Enemy Bullet <%d> hit "
                            "by unknown entity <%d>",
                            ent1->entity_idx, ent2->entity_idx);
                    }
                } else {
                    LOG_ERR("Unsupported collision: ent1=<%d>, ent2=<%d>",
                            ent1->entity_idx, ent2->entity_idx);
                }
            } break;
            default: {
                LOG_ERR("Unknown event type: %d", event.type);
            } break;
        }
    }
}

void space_invaders_game_process_input(
    struct space_invaders_game *space_invaders_game, tilt_flags tilt_flags) {
    struct space_invaders_game_context *context = &space_invaders_game->context;

    if (tilt_flags.wrist_tilt_ia_xpos) {
        set_entity_velocity(context->user_ship.entity, (velocity){50, 0});
    } else if (tilt_flags.wrist_tilt_ia_xneg) {
        set_entity_velocity(context->user_ship.entity, (velocity){-50, 0});
    } else {
        set_entity_velocity(context->user_ship.entity, (velocity){0, 0});
    }
}

void space_invaders_game_pause(
    struct space_invaders_game *space_invaders_game) {
    struct space_invaders_game_context *context = &space_invaders_game->context;

    context->last_user_bullet_time =
        HAL_GetTick() - context->last_user_bullet_time;
}

void space_invaders_game_unpause(
    struct space_invaders_game *space_invaders_game) {
    struct space_invaders_game_context *context = &space_invaders_game->context;

    context->last_user_bullet_time =
        HAL_GetTick() - context->last_user_bullet_time;
}

void space_invaders_game_reset(
    struct space_invaders_game *space_invaders_game) {
    struct space_invaders_game_context *context = &space_invaders_game->context;

    /* Deactivate all game entities */
    for (int i = 0; i <= SPACE_INVADERS_LAST_ENTITY_IDX; i++) {
        deactivate_game_entity(&context->game_entities[i]);
    }

    set_game_entity_position(
        &context->user_ship,
        (struct rectangle){SPACE_INVADERS_USER_SHIP_START_POSITION}.p1);
    set_game_entity_velocity(&context->user_ship,
                             SPACE_INVADERS_USER_SHIP_START_VELOCITY);

    for (int i = 0; i < SPACE_INVADERS_NUM_OF_ENEMY_SHIPS; i++) {
        set_game_entity_position(
            &context->enemy_ships[i],
            (struct rectangle){SPACE_INVADERS_ENEMY_SHIP_START_POSITION(i)}.p1);
        set_game_entity_velocity(&context->enemy_ships[i],
                                 SPACE_INVADERS_ENEMY_SHIP_START_VELOCITY);
        activate_game_entity(&context->enemy_ships[i]);
    }

    activate_game_entity(&context->user_ship);

    physics_engine_event_queue_flush(&context->game_common.event_queue);

    /* Reset lives */
    context->num_of_user_bullets = 0;
    context->num_of_enemy_bullets = 0;
    context->enemies_remaining = SPACE_INVADERS_NUM_OF_ENEMY_SHIPS;
    context->lives = 3;
    context->last_enemy_bullet_time = HAL_GetTick();
    context->last_user_bullet_time = HAL_GetTick();

    context->game_common.game_state = GAME_STATE_IN_PROGRESS;
}