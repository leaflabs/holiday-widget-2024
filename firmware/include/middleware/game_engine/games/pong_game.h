#ifndef __PONG_GAME_H__
#define __PONG_GAME_H__
#include "game_entity.h"
#include "game_state.h"
#include "lsm6dsm_driver.h"
#include "physics_engine.h"
#include "system_communication.h"

#define PONG_WINNING_SCORE 3

/**************************/
/*  User Paddle Settings  */
/**************************/

/* Start position of user paddle */
#define PONG_USER_PADDLE_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(0, 2), BOTTOM_RIGHT_POSITION_FROM_GRID(0, 5)

/* Start velocity of user paddle */
#define PONG_USER_PADDLE_START_VELOCITY \
    (velocity) {                        \
        0, 0                            \
    }

/* Start acceleration of user paddle */
#define PONG_USER_PADDLE_START_ACCELERATION \
    (acceleration) {                        \
        0, 0                                \
    }

/* Is the user paddle solid */
#define PONG_USER_PADDLE_SOLID true

/* User paddle sprite */
#define PONG_USER_PADDLE_SPRITE (&vertical_paddle)

/******************************/
/*  Opponent Paddle Settings  */
/******************************/

/* Start position of opponent paddle */
#define PONG_OPPONENT_PADDLE_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(6, 2), BOTTOM_RIGHT_POSITION_FROM_GRID(6, 5)

/* Start velocity of opponent paddle */
#define PONG_OPPONENT_PADDLE_START_VELOCITY \
    (velocity) {                            \
        0, 10                               \
    }

/* Start acceleration of opponent paddle */
#define PONG_OPPONENT_PADDLE_START_ACCELERATION \
    (acceleration) {                            \
        0, 0                                    \
    }

/* Is the opponent paddle solid */
#define PONG_OPPONENT_PADDLE_SOLID true

/* Opponent paddle sprite */
#define PONG_OPPONENT_PADDLE_SPRITE (&vertical_paddle)

/*******************/
/*  Ball Settings  */
/*******************/

/* Start position of ball */
#define PONG_BALL_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(3, 3), BOTTOM_RIGHT_POSITION_FROM_GRID(3, 3)

/* Start velocity of ball */
#define PONG_BALL_START_VELOCITY \
    (velocity) {                 \
        -10, 0                   \
    }

/* Start acceleration of ball */
#define PONG_BALL_START_ACCELERATION \
    (acceleration) {                 \
        0, 0                         \
    }

/* Is the ball solid */
#define PONG_BALL_SOLID true

/* Ball sprite */
#define PONG_BALL_SPRITE (&small_ball)

#define PONG_BALL_MAX_VELOCITY (14)

#define PONG_BALL_MIN_X_VELOCITY (1)

#define PONG_MIN_COLLISION_DEBOUNCE_MS 200

/* Pong game configuration struct */
struct pong_game_config {
    /* User paddle initialization struct */
    const struct entity_init_struct *const user_paddle_init_struct;

    /* Opponent paddle initialization struct */
    const struct entity_init_struct *const opponent_paddle_init_struct;

    /* Ball initialization struct */
    const struct entity_init_struct *const ball_init_struct;
};

/* Pong game context struct */
struct pong_game_context {
    struct physics_engine_environment environment;
    struct physics_engine_event_queue event_queue;
    union {
        struct {
            struct game_entity user_paddle;
            struct game_entity opponent_paddle;
            struct game_entity ball;
        };
        struct game_entity game_entities[3];
    };
    uint8_t user_score;
    uint8_t opponent_score;
    enum game_state game_state;
};

struct pong_game {
    const struct pong_game_config config;
    struct pong_game_context context;
};

enum entity_creation_error pong_game_init(struct pong_game *pong_game);

void pong_opponent_scores(struct pong_game *pong_game);

void pong_user_scores(struct pong_game *pong_game);

static const struct entity_init_struct pong_user_paddle_init_struct = {
    .rectangle = (struct rectangle){PONG_USER_PADDLE_START_POSITION},
    .mass = INFINITE_MASS,
    .velocity = PONG_USER_PADDLE_START_VELOCITY,
    .acceleration = PONG_USER_PADDLE_START_ACCELERATION,
    .solid = PONG_USER_PADDLE_SOLID,
};

static const struct entity_init_struct pong_opponent_paddle_init_struct = {
    .rectangle = (struct rectangle){PONG_OPPONENT_PADDLE_START_POSITION},
    .mass = INFINITE_MASS,
    .velocity = PONG_OPPONENT_PADDLE_START_VELOCITY,
    .acceleration = PONG_OPPONENT_PADDLE_START_ACCELERATION,
    .solid = PONG_OPPONENT_PADDLE_SOLID,
};

static const struct entity_init_struct pong_ball_init_struct = {
    .rectangle = (struct rectangle){PONG_BALL_START_POSITION},
    .mass = LARGE_MASS,
    .velocity = PONG_BALL_START_VELOCITY,
    .acceleration = PONG_BALL_START_ACCELERATION,
    .solid = PONG_OPPONENT_PADDLE_SOLID,
};

#define CREATE_PONG_GAME()                                                \
    (struct pong_game) {                                                  \
        .config =                                                         \
            {                                                             \
                .user_paddle_init_struct = &pong_user_paddle_init_struct, \
                .opponent_paddle_init_struct =                            \
                    &pong_opponent_paddle_init_struct,                    \
                .ball_init_struct = &pong_ball_init_struct,               \
            },                                                            \
        .context = {0},                                                   \
    }

void pong_game_process_event_queue(struct pong_game *pong_game);

void pong_game_process_input(struct pong_game *pong_game,
                             tilt_flags tilt_flags);

void pong_game_reset(struct pong_game *pong_game);

#endif