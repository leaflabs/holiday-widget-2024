#ifndef __BRICK_BREAKER_GAME_H__
#define __BRICK_BREAKER_GAME_H__
#include "game_common.h"
#include "game_entity.h"
#include "lsm6dsm_driver.h"
#include "physics_engine.h"
#include "random_number_generator.h"

#define BRICK_BREAKER_LIVES 3

/**************************/
/*  User Paddle Settings  */
/**************************/

/* Start position of user paddle */
#define BRICK_BREAKER_USER_PADDLE_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(2, 6), BOTTOM_RIGHT_POSITION_FROM_GRID(4, 6)

/* Start velocity of user paddle */
#define BRICK_BREAKER_USER_PADDLE_START_VELOCITY \
    (velocity) {                                 \
        0, 0                                     \
    }

/* Start acceleration of user paddle */
#define BRICK_BREAKER_USER_PADDLE_START_ACCELERATION \
    (acceleration) {                                 \
        0, 0                                         \
    }

/* Is the user paddle solid */
#define BRICK_BREAKER_USER_PADDLE_SOLID true

/* User paddle sprite */
#define BRICK_BREAKER_USER_PADDLE_SPRITE (&horizontal_paddle)

/*******************/
/*  Ball Settings  */
/*******************/

/* Start position of ball */
#define BRICK_BREAKER_BALL_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(3, 4), BOTTOM_RIGHT_POSITION_FROM_GRID(3, 4)

/* Start velocity of ball */
#define BRICK_BREAKER_BALL_START_VELOCITY \
    (velocity) {                          \
        0, 10                             \
    }

/* Start acceleration of ball */
#define BRICK_BREAKER_BALL_START_ACCELERATION \
    (acceleration) {                          \
        0, 0                                  \
    }

/* Is the ball solid */
#define BRICK_BREAKER_BALL_SOLID true

/* Ball sprite */
#define BRICK_BREAKER_BALL_SPRITE (&small_ball)

/* Max ball velocity*/
#define BRICK_BREAKER_BALL_MAX_VELOCITY (12)

/********************/
/*  Brick Settings  */
/********************/

#define BRICK_BREAKER_NUM_OF_BRICKS 28

static const position
    BRICK_START_POSITIONS_TOP_LEFT[BRICK_BREAKER_NUM_OF_BRICKS] = {
        TOP_LEFT_POSITION_FROM_GRID(0, 0), TOP_LEFT_POSITION_FROM_GRID(1, 0),
        TOP_LEFT_POSITION_FROM_GRID(2, 0), TOP_LEFT_POSITION_FROM_GRID(3, 0),
        TOP_LEFT_POSITION_FROM_GRID(4, 0), TOP_LEFT_POSITION_FROM_GRID(5, 0),
        TOP_LEFT_POSITION_FROM_GRID(6, 0), TOP_LEFT_POSITION_FROM_GRID(0, 1),
        TOP_LEFT_POSITION_FROM_GRID(1, 1), TOP_LEFT_POSITION_FROM_GRID(2, 1),
        TOP_LEFT_POSITION_FROM_GRID(3, 1), TOP_LEFT_POSITION_FROM_GRID(4, 1),
        TOP_LEFT_POSITION_FROM_GRID(5, 1), TOP_LEFT_POSITION_FROM_GRID(6, 1),
        TOP_LEFT_POSITION_FROM_GRID(0, 2), TOP_LEFT_POSITION_FROM_GRID(1, 2),
        TOP_LEFT_POSITION_FROM_GRID(2, 2), TOP_LEFT_POSITION_FROM_GRID(3, 2),
        TOP_LEFT_POSITION_FROM_GRID(4, 2), TOP_LEFT_POSITION_FROM_GRID(5, 2),
        TOP_LEFT_POSITION_FROM_GRID(6, 2), TOP_LEFT_POSITION_FROM_GRID(0, 3),
        TOP_LEFT_POSITION_FROM_GRID(1, 3), TOP_LEFT_POSITION_FROM_GRID(2, 3),
        TOP_LEFT_POSITION_FROM_GRID(3, 3), TOP_LEFT_POSITION_FROM_GRID(4, 3),
        TOP_LEFT_POSITION_FROM_GRID(5, 3), TOP_LEFT_POSITION_FROM_GRID(6, 3),
};

static const position
    BRICK_START_POSITIONS_BOTTOM_RIGHT[BRICK_BREAKER_NUM_OF_BRICKS] = {
        BOTTOM_RIGHT_POSITION_FROM_GRID(0, 0),
        BOTTOM_RIGHT_POSITION_FROM_GRID(1, 0),
        BOTTOM_RIGHT_POSITION_FROM_GRID(2, 0),
        BOTTOM_RIGHT_POSITION_FROM_GRID(3, 0),
        BOTTOM_RIGHT_POSITION_FROM_GRID(4, 0),
        BOTTOM_RIGHT_POSITION_FROM_GRID(5, 0),
        BOTTOM_RIGHT_POSITION_FROM_GRID(6, 0),
        BOTTOM_RIGHT_POSITION_FROM_GRID(0, 1),
        BOTTOM_RIGHT_POSITION_FROM_GRID(1, 1),
        BOTTOM_RIGHT_POSITION_FROM_GRID(2, 1),
        BOTTOM_RIGHT_POSITION_FROM_GRID(3, 1),
        BOTTOM_RIGHT_POSITION_FROM_GRID(4, 1),
        BOTTOM_RIGHT_POSITION_FROM_GRID(5, 1),
        BOTTOM_RIGHT_POSITION_FROM_GRID(6, 1),
        BOTTOM_RIGHT_POSITION_FROM_GRID(0, 2),
        BOTTOM_RIGHT_POSITION_FROM_GRID(1, 2),
        BOTTOM_RIGHT_POSITION_FROM_GRID(2, 2),
        BOTTOM_RIGHT_POSITION_FROM_GRID(3, 2),
        BOTTOM_RIGHT_POSITION_FROM_GRID(4, 2),
        BOTTOM_RIGHT_POSITION_FROM_GRID(5, 2),
        BOTTOM_RIGHT_POSITION_FROM_GRID(6, 2),
        BOTTOM_RIGHT_POSITION_FROM_GRID(0, 3),
        BOTTOM_RIGHT_POSITION_FROM_GRID(1, 3),
        BOTTOM_RIGHT_POSITION_FROM_GRID(2, 3),
        BOTTOM_RIGHT_POSITION_FROM_GRID(3, 3),
        BOTTOM_RIGHT_POSITION_FROM_GRID(4, 3),
        BOTTOM_RIGHT_POSITION_FROM_GRID(5, 3),
        BOTTOM_RIGHT_POSITION_FROM_GRID(6, 3),
};

/* Start position of brick i */
#define BRICK_BREAKER_BRICK_START_POSITION(i) \
    BRICK_START_POSITIONS_TOP_LEFT[i], BRICK_START_POSITIONS_BOTTOM_RIGHT[i]

/* Start velocity of brick */
#define BRICK_BREAKER_BRICK_START_VELOCITY \
    (velocity) {                           \
        0, 0                               \
    }

/* Start acceleration of brick */
#define BRICK_BREAKER_BRICK_START_ACCELERATION \
    (acceleration) {                           \
        0, 0                                   \
    }

/* Is the brick solid */
#define BRICK_BREAKER_BRICK_SOLID true

/* Brick sprite */
#define BRICK_BREAKER_BRICK_SPRITE (&small_ball)

/* Brick Breaker game configuration struct */
struct brick_breaker_game_config {
    /* User paddle initialization struct */
    const struct entity_init_struct *const user_paddle_init_struct;

    /* Ball initialization struct */
    const struct entity_init_struct *const ball_init_struct;

    /* Brick initialization struct */
    const struct entity_init_struct *const brick_init_struct;
};

struct brick_breaker_game_context {
    struct game_common game_common;
    union {
        struct {
            struct game_entity user_paddle;
            struct game_entity ball;
            struct game_entity bricks[BRICK_BREAKER_NUM_OF_BRICKS];
        };
        struct game_entity game_entities[(BRICK_BREAKER_NUM_OF_BRICKS + 2)];
    };
    uint8_t lives;
    uint8_t bricks_remaining;
};

struct brick_breaker_game {
    const struct brick_breaker_game_config config;
    struct brick_breaker_game_context context;
};

enum entity_creation_error brick_breaker_game_init(
    struct brick_breaker_game *brick_breaker_game);

void brick_breaker_ball_out_of_bounds(union game *game);

static const struct entity_init_struct brick_breaker_user_paddle_init_struct = {
    .rectangle = (struct rectangle){BRICK_BREAKER_USER_PADDLE_START_POSITION},
    .mass = INFINITE_MASS,
    .velocity = BRICK_BREAKER_USER_PADDLE_START_VELOCITY,
    .acceleration = BRICK_BREAKER_USER_PADDLE_START_ACCELERATION,
    .solid = BRICK_BREAKER_USER_PADDLE_SOLID,
};

static const struct entity_init_struct brick_breaker_ball_init_struct = {
    .rectangle = (struct rectangle){BRICK_BREAKER_BALL_START_POSITION},
    .mass = LARGE_MASS,
    .velocity = BRICK_BREAKER_BALL_START_VELOCITY,
    .acceleration = BRICK_BREAKER_BALL_START_ACCELERATION,
    .solid = BRICK_BREAKER_BALL_SOLID,
};

static const struct entity_init_struct brick_breaker_brick_init_struct = {
    .rectangle = (struct rectangle){BRICK_BREAKER_BRICK_START_POSITION(0)},
    .mass = INFINITE_MASS,
    .velocity = BRICK_BREAKER_BRICK_START_VELOCITY,
    .acceleration = BRICK_BREAKER_BRICK_START_ACCELERATION,
    .solid = BRICK_BREAKER_BRICK_SOLID,
};

#define CREATE_BRICK_BREAKER_GAME()                                    \
    (struct brick_breaker_game) {                                      \
        .config =                                                      \
            {                                                          \
                .user_paddle_init_struct =                             \
                    &brick_breaker_user_paddle_init_struct,            \
                .ball_init_struct = &brick_breaker_ball_init_struct,   \
                .brick_init_struct = &brick_breaker_brick_init_struct, \
            },                                                         \
        .context = {0},                                                \
    }

void brick_breaker_game_process_event_queue(
    struct brick_breaker_game *brick_breaker_game,
    struct random_number_generator *rng);

void brick_breaker_game_process_input(
    struct brick_breaker_game *brick_breaker_game, tilt_flags tilt_flags);

void brick_breaker_game_reset(struct brick_breaker_game *brick_breaker_game);

#endif