#include "pong_game.h"

#include "game.h"
#include "logging.h"
#include "music_player.h"
#include "utils.h"

extern void pause_game_engine(int32_t duration);
extern void unpause_game_engine(void);

#define PONG_FREEZE_TIME 2000

#define PONG_USER_PADDLE_ENTITY_IDX 0
#define PONG_OPPONENT_PADDLE_ENTITY_IDX 1
#define PONG_BALL_ENTITY_IDX 2

#define PONG_LAST_ENTITY_IDX PONG_BALL_ENTITY_IDX

/* Evaluates to true if the given value is a pointer to the user paddle entity,
 * else false */
#define IS_USER_PADDLE_ENTITY(__ent__) \
    (IS_ENTITY_POINTER(__ent__) &&     \
     ((struct entity *)__ent__)->entity_idx == PONG_USER_PADDLE_ENTITY_IDX)

/* Evaluates to true if the given value is a pointer to the opponent paddle
 * entity, else false */
#define IS_OPPONENT_PADDLE_ENTITY(__ent__)                                   \
    (IS_ENTITY_POINTER(__ent__) && ((struct entity *)__ent__)->entity_idx == \
                                       PONG_OPPONENT_PADDLE_ENTITY_IDX)

/* Evaluates to true if the given value is a pointer to the ball entity,
 * else false */
#define IS_BALL_ENTITY(__ent__)    \
    (IS_ENTITY_POINTER(__ent__) && \
     ((struct entity *)__ent__)->entity_idx == PONG_BALL_ENTITY_IDX)

enum entity_creation_error pong_game_init(struct pong_game *pong_game) {
    const struct pong_game_config *config = &pong_game->config;
    struct pong_game_context *context = &pong_game->context;

    game_common_init(&context->game_common);

    /* Set scores to 0 */
    context->user_score = 0;
    context->opponent_score = 0;

    /******************/
    /*  Add Entities  */
    /******************/
    struct entity_creation_result result;

    /* Add user paddle entity to environment and initialize game entity */
    result = add_entity(&context->game_common.environment,
                        config->user_paddle_init_struct);
    if (result.error != ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Failed to create user paddle entity: %d", result.error);
        return result.error;
    } else if (!game_entity_init(&context->user_paddle, result.entity,
                                 PONG_USER_PADDLE_SPRITE)) {
        LOG_ERR("Failed to create user paddle game entity");
        result.error = ENTITY_CREATION_INVALID_TYPE;
        return result.error;
    }

    /* Add oppponent paddle entity */
    result = add_entity(&context->game_common.environment,
                        config->opponent_paddle_init_struct);
    if (result.error != ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Failed to create opponent paddle entity: %d", result.error);
        return result.error;
    } else if (!game_entity_init(&context->opponent_paddle, result.entity,
                                 PONG_OPPONENT_PADDLE_SPRITE)) {
        LOG_ERR("Failed to create opponent paddle game entity");
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
                                 PONG_BALL_SPRITE)) {
        LOG_ERR("Failed to create ball game entity");
        result.error = ENTITY_CREATION_INVALID_TYPE;
        return result.error;
    }

    /* Activate relevant entities */
    activate_game_entity(&context->user_paddle);
    activate_game_entity(&context->opponent_paddle);
    activate_game_entity(&context->ball);

    return result.error;
}

void pong_opponent_scores(struct pong_game *pong_game) {
    struct pong_game_context *context = &pong_game->context;

    enum music_player_error error =
        music_player_play_song(&music_player, FAILURE_SOUND);
    if (error != MUSIC_PLAYER_NO_ERROR) {
        LOG_ERR("Music player failed to play failure sound: %d", error);
    }

    if (++context->opponent_score >= PONG_WINNING_SCORE) {
        deactivate_game_entity(&context->user_paddle);
        deactivate_game_entity(&context->opponent_paddle);
        deactivate_game_entity(&context->ball);

        context->game_common.game_state = GAME_STATE_YOU_LOSE;
    } else {
        set_game_entity_position(
            &context->ball, ((struct rectangle){PONG_BALL_START_POSITION}).p1);
        set_entity_velocity(context->ball.entity, PONG_BALL_START_VELOCITY);

        physics_engine_environment_pause(&context->game_common.environment);
        context->game_common.game_state = GAME_STATE_SCORE_CHANGE;
    }
}

void pong_user_scores(struct pong_game *pong_game) {
    struct pong_game_context *context = &pong_game->context;
    enum music_player_error error =
        music_player_play_song(&music_player, SUCCESS_SOUND);
    if (error != MUSIC_PLAYER_NO_ERROR) {
        LOG_ERR("Music player failed to play success sound: %d", error);
    }
    if (++context->user_score >= PONG_WINNING_SCORE) {
        deactivate_game_entity(&context->user_paddle);
        deactivate_game_entity(&context->opponent_paddle);
        deactivate_game_entity(&context->ball);

        context->game_common.game_state = GAME_STATE_YOU_WIN;
    } else {
        set_game_entity_position(
            &context->ball, ((struct rectangle){PONG_BALL_START_POSITION}).p1);
        set_entity_velocity(context->ball.entity, PONG_BALL_START_VELOCITY);

        physics_engine_environment_pause(&context->game_common.environment);
        context->game_common.game_state = GAME_STATE_SCORE_CHANGE;
    }
}

pong_game_process_event_queue(struct pong_game *pong_game) {
    struct pong_game_context *context = &pong_game->context;
    struct ring_buffer *event_queue = &context->game_common.event_queue;

    struct physics_engine_event event;

    while (ring_buffer_pop(event_queue, &event) == 0) {
        switch (event.type) {
            case OUT_OF_BOUNDS_EVENT:
                struct physics_engine_out_of_bounds_event *out_of_bounds_event =
                    &event.out_of_bounds_event;
                if (IS_OPPONENT_PADDLE_ENTITY(out_of_bounds_event->ent)) {
                    context->opponent_paddle.entity->velocity.y *= -1;
                } else if (IS_BALL_ENTITY(out_of_bounds_event->ent)) {
                    if (out_of_bounds_event->type == OUT_OF_BOUNDS_BOTTOM ||
                        out_of_bounds_event->type == OUT_OF_BOUNDS_TOP) {
                        context->ball.entity->velocity.y *= -1;
                    } else if (out_of_bounds_event->type ==
                               OUT_OF_BOUNDS_LEFT) {
                        pong_opponent_scores(pong_game);
                    } else {
                        pong_user_scores(pong_game);
                    }
                }
                break;
            case COLLISION_EVENT:
                struct physics_engine_collision_event *collision_event =
                    &event.collision_event;
                static uint32_t last_collision_time;
                static struct entity *last_collision_entity = NULL;
                struct entity *other = NULL;
                if (collision_event->ent1->entity_idx ==
                    context->ball.entity->entity_idx) {
                    other = collision_event->ent2;
                } else {
                    other = collision_event->ent1;
                }
                bool other_is_user = other->entity_idx ==
                                     context->user_paddle.entity->entity_idx;
                if (other_is_user) {
                    context->ball.entity->velocity.y *= -1;
                }
                if (last_collision_entity != NULL) {
                    if (other->entity_idx ==
                        last_collision_entity->entity_idx) {
                        if (HAL_GetTick() - last_collision_time <=
                            PONG_MIN_COLLISION_DEBOUNCE_MS) {
                            if (other->entity_idx ==
                                context->user_paddle.entity->entity_idx) {
                                set_entity_position_relative(
                                    context->ball.entity, (position){-50, 0});
                            } else {
                                set_entity_position_relative(
                                    context->ball.entity, (position){50, 0});
                            }
                        }
                    }
                }
                last_collision_time = HAL_GetTick();
                last_collision_entity = other;

                /* Clamp the ball velocity to avoid excessive speeds */
                context->ball.entity->velocity.x =
                    CLAMP(context->ball.entity->velocity.x,
                          -PONG_BALL_MAX_VELOCITY, PONG_BALL_MAX_VELOCITY);
                context->ball.entity->velocity.y =
                    CLAMP(context->ball.entity->velocity.y,
                          -PONG_BALL_MAX_VELOCITY, PONG_BALL_MAX_VELOCITY);
                if (context->ball.entity->rectangle.p1.x ==
                    context->user_paddle.entity->rectangle.p1.x) {
                    pong_opponent_scores(pong_game);
                } else if (context->ball.entity->rectangle.p1.x ==
                           context->opponent_paddle.entity->rectangle.p1.x) {
                    pong_user_scores(pong_game);
                }
                break;
            default:
                LOG_ERR("Unknown event type: %d", event.type);
                break;
        }
    }
    /* Make sure ball never has x velocity 0 */
    if (context->ball.entity->velocity.x < PONG_BALL_MIN_X_VELOCITY &&
        context->ball.entity->velocity.x >= 0) {
        context->ball.entity->velocity.x = PONG_BALL_MIN_X_VELOCITY;
    } else if (context->ball.entity->velocity.x > -PONG_BALL_MIN_X_VELOCITY &&
               context->ball.entity->velocity.x <= 0) {
        context->ball.entity->velocity.x = -PONG_BALL_MIN_X_VELOCITY;
    }
}

void pong_game_process_input(struct pong_game *pong_game,
                             tilt_flags tilt_flags) {
    struct pong_game_context *context = &pong_game->context;

    if (tilt_flags.wrist_tilt_ia_yneg) {
        set_entity_velocity(context->user_paddle.entity, (velocity){0, 62});
    } else if (tilt_flags.wrist_tilt_ia_ypos) {
        set_entity_velocity(context->user_paddle.entity, (velocity){0, -62});
    } else {
        set_entity_velocity(context->user_paddle.entity, (velocity){0, 0});
    }
}

void pong_game_reset(struct pong_game *pong_game) {
    struct pong_game_context *context = &pong_game->context;

    /* Deactivate all game entities */
    for (int i = 0; i <= PONG_LAST_ENTITY_IDX; i++) {
        deactivate_game_entity(&context->game_entities[i]);
    }

    set_game_entity_position(
        &context->user_paddle,
        (struct rectangle){PONG_USER_PADDLE_START_POSITION}.p1);
    set_game_entity_velocity(&context->user_paddle,
                             PONG_USER_PADDLE_START_VELOCITY);

    set_game_entity_position(
        &context->opponent_paddle,
        (struct rectangle){PONG_OPPONENT_PADDLE_START_POSITION}.p1);
    set_game_entity_velocity(&context->opponent_paddle,
                             PONG_OPPONENT_PADDLE_START_VELOCITY);

    set_game_entity_position(&context->ball,
                             (struct rectangle){PONG_BALL_START_POSITION}.p1);
    set_game_entity_velocity(&context->ball, PONG_BALL_START_VELOCITY);

    ring_buffer_flush(&context->game_common.event_queue);

    /* Activate all game entities */
    for (int i = 0; i <= PONG_LAST_ENTITY_IDX; i++) {
        activate_game_entity(&context->game_entities[i]);
    }

    context->game_common.game_state = GAME_STATE_IN_PROGRESS;

    /* Set scores to 0 */
    context->user_score = 0;
    context->opponent_score = 0;
}