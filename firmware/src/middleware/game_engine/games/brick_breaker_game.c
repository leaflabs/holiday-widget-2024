#include "brick_breaker_game.h"

#include "game.h"
#include "logging.h"
#include "music_player.h"
#include "sprite.h"
#include "utils.h"

#define BRICK_BREAKER_USER_PADDLE_ENTITY_IDX 0
#define BRICK_BREAKER_BALL_ENTITY_IDX 1
#define BRICK_BREAKER_FIRST_BRICK_ENTITY_IDX 2
#define BRICK_BREAKER_LAST_BRICK_ENTITY_IDX \
    (BRICK_BREAKER_FIRST_BRICK_ENTITY_IDX + BRICK_BREAKER_NUM_OF_BRICKS - 1)
#define BRICK_BREAKER_LAST_ENTITY_IDX BRICK_BREAKER_LAST_BRICK_ENTITY_IDX

/* Evaluates to true if the given value is a pointer to the user paddle entity,
 * else false */
#define IS_USER_PADDLE_ENTITY(__ent__)                                       \
    (IS_ENTITY_POINTER(__ent__) && ((struct entity *)__ent__)->entity_idx == \
                                       BRICK_BREAKER_USER_PADDLE_ENTITY_IDX)

/* Evaluates to true if the given value is a pointer to the ball entity,
 * else false */
#define IS_BALL_ENTITY(__ent__)    \
    (IS_ENTITY_POINTER(__ent__) && \
     ((struct entity *)__ent__)->entity_idx == BRICK_BREAKER_BALL_ENTITY_IDX)

/* Evaluates to true if the given value is a pointer to a bick entity,
 * else false */
#define IS_BRICK_ENTITY(__ent__)                 \
    (IS_ENTITY_POINTER(__ent__) &&               \
     ((struct entity *)__ent__)->entity_idx >=   \
         BRICK_BREAKER_FIRST_BRICK_ENTITY_IDX && \
     ((struct entity *)__ent__)->entity_idx <=   \
         BRICK_BREAKER_LAST_BRICK_ENTITY_IDX)

enum entity_creation_error brick_breaker_game_init(
    struct brick_breaker_game *brick_breaker_game) {
    const struct brick_breaker_game_config *config =
        &brick_breaker_game->config;
    struct brick_breaker_game_context *context = &brick_breaker_game->context;

    game_common_init(&context->game_common);

    /******************/
    /*  Add Entities  */
    /******************/
    struct entity_creation_result result;

    /* Add user paddle entity */
    result = add_entity(&context->game_common.environment,
                        config->user_paddle_init_struct);
    if (result.error != ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Failed to create user paddle entity: %d", result.error);
        return result.error;
    } else if (!game_entity_init(&context->user_paddle, result.entity,
                                 BRICK_BREAKER_USER_PADDLE_SPRITE)) {
        LOG_ERR("Failed to create user paddle game entity");
        result.error = ENTITY_CREATION_INVALID_TYPE;
        return result.error;
    }

    /* Add ball entity */
    result =
        add_entity(&context->game_common.environment, config->ball_init_struct);
    if (result.error != ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Failed to create ball entity: %d", result.error);
        return result.error;
    } else if (!game_entity_init(&context->ball, result.entity,
                                 BRICK_BREAKER_BALL_SPRITE)) {
        LOG_ERR("Failed to create user paddle game entity");
        result.error = ENTITY_CREATION_INVALID_TYPE;
        return result.error;
    }

    /* Add brick entities */
    for (int i = 0; i < BRICK_BREAKER_NUM_OF_BRICKS; i++) {
        result = add_entity(&context->game_common.environment,
                            config->brick_init_struct);
        if (result.error != ENTITY_CREATION_SUCCESS) {
            LOG_ERR("Failed to create brick entity %d: %d", i, result.error);
            return result.error;
        } else if (!game_entity_init(&context->bricks[i], result.entity,
                                     BRICK_BREAKER_BRICK_SPRITE)) {
            LOG_ERR("Failed to create brick %d game entity", i);
            result.error = ENTITY_CREATION_INVALID_TYPE;
            return result.error;
        }
        set_game_entity_position(&context->bricks[i],
                                 BRICK_START_POSITIONS_TOP_LEFT[i]);
    }

    activate_game_entity(&context->user_paddle);
    activate_game_entity(&context->ball);

    for (int i = 0; i < BRICK_BREAKER_NUM_OF_BRICKS; i++) {
        activate_game_entity(&context->bricks[i]);
    }

    context->lives = BRICK_BREAKER_LIVES;
    context->bricks_remaining = BRICK_BREAKER_NUM_OF_BRICKS;

    return result.error;
}

void brick_breaker_ball_out_of_bounds(union game *game) {
    struct brick_breaker_game_context *context =
        &game->brick_breaker_game->context;

    if (--context->lives <= 0) {
        /* Display losing screen */
    } else {
        position ball_positions[2] = {BRICK_BREAKER_BALL_START_POSITION};
        position paddle_positions[2] = {
            BRICK_BREAKER_USER_PADDLE_START_POSITION};

        set_game_entity_position(&context->user_paddle, paddle_positions[0]);
        set_game_entity_position(&context->ball, ball_positions[0]);
        set_entity_velocity(context->ball.entity,
                            BRICK_BREAKER_BALL_START_VELOCITY);
    }
}

brick_breaker_game_process_event_queue(
    struct brick_breaker_game *brick_breaker_game,
    struct random_number_generator *rng) {
    struct brick_breaker_game_context *context = &brick_breaker_game->context;
    struct physics_engine_event_queue *event_queue = &context->game_common.event_queue;

    struct physics_engine_event event;

    while (physics_engine_event_queue_dequeue(event_queue, &event)) {
        switch (event.type) {
            case OUT_OF_BOUNDS_EVENT:
                struct physics_engine_out_of_bounds_event *out_of_bounds_event =
                    &event.out_of_bounds_event;
                if (IS_BALL_ENTITY(out_of_bounds_event->ent)) {
                    switch (out_of_bounds_event->type) {
                        case OUT_OF_BOUNDS_LEFT: /* fall-through */
                        case OUT_OF_BOUNDS_RIGHT:
                            context->ball.entity->velocity.x *= -1;
                            break;
                        case OUT_OF_BOUNDS_TOP:
                            context->ball.entity->velocity.y *= -1;
                            break;
                        case OUT_OF_BOUNDS_BOTTOM: {
                            enum music_player_error error =
                                music_player_play_song(&music_player,
                                                       FAILURE_SOUND);
                            if (error != MUSIC_PLAYER_NO_ERROR) {
                                LOG_ERR(
                                    "Music player failed to play failure "
                                    "sound: %d",
                                    error);
                            }
                            if (--context->lives <= 0) {
                                deactivate_game_entity(&context->user_paddle);
                                deactivate_game_entity(&context->ball);
                                for (int i = 0; i < BRICK_BREAKER_NUM_OF_BRICKS;
                                     i++) {
                                    deactivate_game_entity(&context->bricks[i]);
                                }
                                context->game_common.game_state =
                                    GAME_STATE_YOU_LOSE;
                                /* Display losing screen */
                            } else {
                                position ball_positions[2] = {
                                    BRICK_BREAKER_BALL_START_POSITION};
                                position paddle_positions[2] = {
                                    BRICK_BREAKER_USER_PADDLE_START_POSITION};
                                set_game_entity_position(&context->user_paddle,
                                                         paddle_positions[0]);
                                set_game_entity_position(&context->ball,
                                                         ball_positions[0]);
                                set_entity_velocity(
                                    context->ball.entity,
                                    BRICK_BREAKER_BALL_START_VELOCITY);
                                physics_engine_environment_pause(
                                    &context->game_common.environment);
                                context->game_common.game_state =
                                    GAME_STATE_SCORE_CHANGE;
                            }
                            LOG_INF("Lives: %d", context->lives);
                        } break;
                        default:
                            LOG_ERR("Unknown out of bounds event type: %d",
                                    out_of_bounds_event->type);
                            break;
                    }
                }
                break;
            case COLLISION_EVENT: {
                struct physics_engine_collision_event *collision_event =
                    &event.collision_event;
                struct entity *ball, *other;
                if (collision_event->ent1->entity_idx ==
                    context->ball.entity->entity_idx) {
                    ball = collision_event->ent1;
                    other = collision_event->ent2;
                } else if (collision_event->ent2->entity_idx ==
                           context->ball.entity->entity_idx) {
                    ball = collision_event->ent2;
                    other = collision_event->ent1;
                } else {
                    LOG_ERR("Illegal collision - one entity must be the ball");
                    continue;
                }

                if (IS_USER_PADDLE_ENTITY(other)) {
                    set_entity_position_relative(
                        ball, (position){0, -GRID_UNIT_SIZE / 4});
                    set_entity_velocity_relative(
                        ball,
                        (velocity){
                            (((int32_t)random_number_generator_get_next_in_n(
                                 rng, 8)) -
                             4),
                            0});
                } else if (IS_BRICK_ENTITY(other)) {
                    deactivate_entity(other);

                    if (--context->bricks_remaining <= 0) {
                        deactivate_game_entity(&context->user_paddle);
                        deactivate_game_entity(&context->ball);
                        enum music_player_error error = music_player_play_song(
                            &music_player, SUCCESS_SOUND);
                        if (error != MUSIC_PLAYER_NO_ERROR) {
                            LOG_ERR(
                                "Music player failed to play success sound: %d",
                                error);
                        }
                        context->game_common.game_state = GAME_STATE_YOU_WIN;
                        // Display winning screen
                    }
                }
                /* Clamp the ball velocity to avoid excessive speeds */
                context->ball.entity->velocity.x =
                    CLAMP(context->ball.entity->velocity.x,
                          -BRICK_BREAKER_BALL_MAX_VELOCITY,
                          BRICK_BREAKER_BALL_MAX_VELOCITY);
                context->ball.entity->velocity.y =
                    CLAMP(context->ball.entity->velocity.y,
                          -BRICK_BREAKER_BALL_MAX_VELOCITY,
                          BRICK_BREAKER_BALL_MAX_VELOCITY);

                if (context->ball.entity->velocity.x == 0) {
                    context->ball.entity->velocity.x =
                        (((int32_t)random_number_generator_get_next_in_n(rng,
                                                                         4)) -
                         2);
                }

                if (context->ball.entity->velocity.y == 0) {
                    context->ball.entity->velocity.y =
                        (((int32_t)random_number_generator_get_next_in_n(rng,
                                                                         4)) -
                         2);
                }
            } break;
            default:
                LOG_ERR("Unknown event type: %d", event.type);
                break;
        }
    }
}

void brick_breaker_game_process_input(
    struct brick_breaker_game *brick_breaker_game, tilt_flags tilt_flags) {
    struct brick_breaker_game_context *context = &brick_breaker_game->context;

    if (tilt_flags.wrist_tilt_ia_xpos) {
        set_entity_velocity(context->user_paddle.entity, (velocity){55, 0});
    } else if (tilt_flags.wrist_tilt_ia_xneg) {
        set_entity_velocity(context->user_paddle.entity, (velocity){-55, 0});
    } else {
        set_entity_velocity(context->user_paddle.entity, (velocity){0, 0});
    }
}

void brick_breaker_game_reset(struct brick_breaker_game *brick_breaker_game) {
    struct brick_breaker_game_context *context = &brick_breaker_game->context;

    /* Deactivate all game entities */
    for (int i = 0; i <= BRICK_BREAKER_LAST_ENTITY_IDX; i++) {
        deactivate_game_entity(&context->game_entities[i]);
    }

    set_game_entity_position(
        &context->user_paddle,
        (struct rectangle){BRICK_BREAKER_USER_PADDLE_START_POSITION}.p1);
    set_game_entity_velocity(&context->user_paddle,
                             BRICK_BREAKER_USER_PADDLE_START_VELOCITY);

    set_game_entity_position(
        &context->ball,
        (struct rectangle){BRICK_BREAKER_BALL_START_POSITION}.p1);
    set_game_entity_velocity(&context->ball, BRICK_BREAKER_BALL_START_VELOCITY);

    for (int i = 0; i < BRICK_BREAKER_NUM_OF_BRICKS; i++) {
        set_game_entity_position(
            &context->bricks[i],
            (struct rectangle){BRICK_BREAKER_BRICK_START_POSITION(i)}.p1);
        set_game_entity_velocity(&context->bricks[i],
                                 BRICK_BREAKER_BRICK_START_VELOCITY);
    }

    /* Activate all game entities */
    for (int i = 0; i <= BRICK_BREAKER_LAST_ENTITY_IDX; i++) {
        activate_game_entity(&context->game_entities[i]);
    }

    physics_engine_event_queue_flush(&context->game_common.event_queue);

    context->lives = BRICK_BREAKER_LIVES;
    context->bricks_remaining = BRICK_BREAKER_NUM_OF_BRICKS;

    context->game_common.game_state = GAME_STATE_IN_PROGRESS;
}
