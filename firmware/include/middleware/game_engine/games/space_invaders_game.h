#ifndef __SPACE_INVADERS_GAME_H__
#define __SPACE_INVADERS_GAME_H__
#include <stdint.h>

#include "game_entity.h"
#include "game_state.h"
#include "lsm6dsm_driver.h"
#include "physics_engine.h"
#include "random_number_generator.h"

#define USER_BULLET_PERIOD 2750U
#define ENEMY_BULLET_PERIOD 3000U

/************************/
/*  User Ship Settings  */
/************************/

/* Start position of user ship */
#define SPACE_INVADERS_USER_SHIP_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(2, 6), BOTTOM_RIGHT_POSITION_FROM_GRID(2, 6)

/* Start velocity of user ship */
#define SPACE_INVADERS_USER_SHIP_START_VELOCITY \
    (velocity) {                                \
        0, 0                                    \
    }

/* Start acceleration of user ship */
#define SPACE_INVADERS_USER_SHIP_START_ACCELERATION \
    (acceleration) {                                \
        0, 0                                        \
    }

/* opacity of user ship */
#define SPACE_INVADERS_USER_SHIP_OPACITY (uint8_t)255

/* Is the user ship solid */
#define SPACE_INVADERS_USER_SHIP_SOLID true

/* User ship sprite */
#define SPACE_INVADERS_USER_SHIP_SPRITE (&small_ball)

/*************************/
/*  Enemy Ship Settings  */
/*************************/

#define SPACE_INVADERS_NUM_OF_ENEMY_SHIPS 11 /* Can't exceed 5?? */

static const position
    ENEMY_SHIP_START_POSITIONS_TOP_LEFT[SPACE_INVADERS_NUM_OF_ENEMY_SHIPS] = {
        TOP_LEFT_POSITION_FROM_GRID(0, 0), TOP_LEFT_POSITION_FROM_GRID(2, 0),
        TOP_LEFT_POSITION_FROM_GRID(4, 0), TOP_LEFT_POSITION_FROM_GRID(6, 0),
        TOP_LEFT_POSITION_FROM_GRID(1, 1), TOP_LEFT_POSITION_FROM_GRID(3, 1),
        TOP_LEFT_POSITION_FROM_GRID(5, 1), TOP_LEFT_POSITION_FROM_GRID(0, 2),
        TOP_LEFT_POSITION_FROM_GRID(2, 2), TOP_LEFT_POSITION_FROM_GRID(4, 2),
        TOP_LEFT_POSITION_FROM_GRID(6, 2),
};

static const position
    ENEMY_SHIP_START_POSITIONS_BOTTOM_RIGHT[SPACE_INVADERS_NUM_OF_ENEMY_SHIPS] =
        {
            BOTTOM_RIGHT_POSITION_FROM_GRID(0, 0),
            BOTTOM_RIGHT_POSITION_FROM_GRID(2, 0),
            BOTTOM_RIGHT_POSITION_FROM_GRID(4, 0),
            BOTTOM_RIGHT_POSITION_FROM_GRID(6, 0),
            BOTTOM_RIGHT_POSITION_FROM_GRID(1, 1),
            BOTTOM_RIGHT_POSITION_FROM_GRID(3, 1),
            BOTTOM_RIGHT_POSITION_FROM_GRID(5, 1),
            BOTTOM_RIGHT_POSITION_FROM_GRID(0, 2),
            BOTTOM_RIGHT_POSITION_FROM_GRID(2, 2),
            BOTTOM_RIGHT_POSITION_FROM_GRID(4, 2),
            BOTTOM_RIGHT_POSITION_FROM_GRID(6, 2),
};

/* Start position of enemy ships */
#define SPACE_INVADERS_ENEMY_SHIP_START_POSITION(i) \
    ENEMY_SHIP_START_POSITIONS_TOP_LEFT[i],         \
        ENEMY_SHIP_START_POSITIONS_BOTTOM_RIGHT[i]

/* Start velocity of enemy ships */
#define SPACE_INVADERS_ENEMY_SHIP_START_VELOCITY \
    (velocity) {                                 \
        0, 0                                     \
    }

/* Start acceleration of enemy ships */
#define SPACE_INVADERS_ENEMY_SHIP_START_ACCELERATION \
    (acceleration) {                                 \
        0, 0                                         \
    }

/* Opacity of enemy ship */
#define SPACE_INVADERS_ENEMY_SHIP_OPACITY (uint8_t)255

/* Is enemy ship solid */
#define SPACE_INVADERS_ENEMY_SHIP_SOLID true

/* Enemy ship sprite */
#define SPACE_INVADERS_ENEMY_SHIP_SPRITE (&small_ball)

/**************************/
/*  User Bullet Settings  */
/**************************/

/* Number of user bullets */
#define SPACE_INVADERS_MAX_USER_BULLETS 5

/* Start position of user bullets */
#define SPACE_INVADERS_USER_BULLET_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(0, 0), BOTTOM_RIGHT_POSITION_FROM_GRID(0, 0)

/* Start velocity of user bullets */
#define SPACE_INVADERS_USER_BULLET_START_VELOCITY \
    (velocity) {                                  \
        0, -9                                     \
    }

/* Start acceleration of user bullets */
#define SPACE_INVADERS_USER_BULLET_START_ACCELERATION \
    (acceleration) {                                  \
        0, 0                                          \
    }

/* Opacity of user bullets */
#define SPACE_INVADERS_USER_BULLET_OPACITY (uint8_t)255

/* Is user bullet solid */
#define SPACE_INVADERS_USER_BULLET_SOLID false

/* User bullet sprite */
#define SPACE_INVADERS_USER_BULLET_SPRITE (&small_ball)

/**************************/
/*  Enemy Bullet Settings  */
/**************************/

/* Number of enemy bullets */
#define SPACE_INVADERS_MAX_ENEMY_BULLETS 5

/* Start position of enemy bullets */
#define SPACE_INVADERS_ENEMY_BULLET_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(0, 0), BOTTOM_RIGHT_POSITION_FROM_GRID(0, 0)

/* Start velocity of enemy bullets */
#define SPACE_INVADERS_ENEMY_BULLET_START_VELOCITY \
    (velocity) {                                   \
        0, 7                                       \
    }

/* Start acceleration of enemy bullets */
#define SPACE_INVADERS_ENEMY_BULLET_START_ACCELERATION \
    (acceleration) {                                   \
        0, 0                                           \
    }

/* Opacity of enemy bullets */
#define SPACE_INVADERS_ENEMY_BULLET_OPACITY (uint8_t)255

/* Is enemy bullet solid */
#define SPACE_INVADERS_ENEMY_BULLET_SOLID false

/* Enemy bullet sprite */
#define SPACE_INVADERS_ENEMY_BULLET_SPRITE (&small_ball)

struct space_invaders_game_config {
    /* User ship initialization struct */
    const struct entity_init_struct *const user_ship_init_struct;

    /* Enemy ship initialization structs */
    const struct entity_init_struct *const enemy_ship_init_struct;

    /* User bullet initialization structs */
    const struct entity_init_struct *const user_bullet_init_struct;

    /* Enemy bullet initialization structs */
    const struct entity_init_struct *const enemy_bullet_init_struct;

    /* User ship boundary conditions */
    const struct boundary_conditions *const user_ship_boundary_conditions;

    /* Enemy ship boundary conditions */
    const struct boundary_conditions *const enemy_ship_boundary_conditions;

    /* User bullet boundary conditions */
    const struct boundary_conditions *const user_bullet_boundary_conditions;

    /* Enemy bullet boundary conditions */
    const struct boundary_conditions *const enemy_bullet_boundary_conditions;
};

struct space_invaders_game_context {
    struct physics_engine_environment environment;
    struct physics_engine_event_queue event_queue;
    union {
        struct {
            struct game_entity user_ship;
            struct game_entity enemy_ships[SPACE_INVADERS_NUM_OF_ENEMY_SHIPS];
            struct game_entity user_bullets[SPACE_INVADERS_MAX_USER_BULLETS];
            struct game_entity enemy_bullets[SPACE_INVADERS_MAX_ENEMY_BULLETS];
        };
        struct game_entity game_entities[SPACE_INVADERS_NUM_OF_ENEMY_SHIPS +
                                         SPACE_INVADERS_MAX_USER_BULLETS +
                                         SPACE_INVADERS_MAX_ENEMY_BULLETS + 1];
    } __attribute__((aligned(4)));
    volatile uint32_t num_of_user_bullets __attribute__((aligned(4)));
    volatile uint32_t num_of_enemy_bullets __attribute__((aligned(4)));
    volatile uint32_t last_enemy_bullet_time;
    volatile uint32_t last_user_bullet_time;
    volatile uint8_t enemies_remaining;
    volatile uint8_t lives;
    enum game_state game_state;
} __attribute__((aligned(4)));

struct space_invaders_game {
    const struct space_invaders_game_config config;
    struct space_invaders_game_context context __attribute__((aligned(4)));
};

enum entity_creation_error space_invaders_game_init(
    struct space_invaders_game *space_invaders_game);

void on_enemy_ship_collision(struct entity *ent1, struct entity *ent2);

void on_user_ship_collision(struct entity *ent1, struct entity *ent2);

void on_bullet_collision(struct entity *ent1, struct entity *ent2);

void update_space_invaders_game(struct space_invaders_game *space_invaders_game,
                                const struct random_number_generator *rng);

void user_bullet_out_of_bounds(union game *game);

void enemy_bullet_out_of_bounds(union game *game);

static const struct boundary_conditions
    space_invaders_user_ship_boundary_conditions = {
        .left_boundary_action = BOUNDARY_ACTION_STOP,
        .right_boundary_action = BOUNDARY_ACTION_STOP,
        .top_boundary_action = BOUNDARY_ACTION_STOP,
        .bottom_boundary_action = BOUNDARY_ACTION_STOP,
        .left_boundary_function = NULL,
        .right_boundary_function = NULL,
        .top_boundary_function = NULL,
        .bottom_boundary_function = NULL,
};

static const struct boundary_conditions
    space_invaders_enemy_ship_boundary_conditions = {
        .left_boundary_action = BOUNDARY_ACTION_STOP,
        .right_boundary_action = BOUNDARY_ACTION_STOP,
        .top_boundary_action = BOUNDARY_ACTION_STOP,
        .bottom_boundary_action = BOUNDARY_ACTION_STOP,
        .left_boundary_function = NULL,
        .right_boundary_function = NULL,
        .top_boundary_function = NULL,
        .bottom_boundary_function = NULL,
};

static const struct boundary_conditions
    space_invaders_user_bullet_boundary_conditions = {
        .left_boundary_action = BOUNDARY_ACTION_STOP,
        .right_boundary_action = BOUNDARY_ACTION_STOP,
        .top_boundary_action = BOUNDARY_ACTION_STOP,
        .bottom_boundary_action = BOUNDARY_ACTION_STOP,
};

static const struct boundary_conditions
    space_invaders_enemy_bullet_boundary_conditions = {
        .left_boundary_action = BOUNDARY_ACTION_STOP,
        .right_boundary_action = BOUNDARY_ACTION_STOP,
        .top_boundary_action = BOUNDARY_ACTION_STOP,
        .bottom_boundary_action = BOUNDARY_ACTION_STOP,
};

static const struct entity_init_struct space_invaders_user_ship_init_struct = {
    .rectangle = (struct rectangle){SPACE_INVADERS_USER_SHIP_START_POSITION},
    .mass = INFINITE_MASS,
    .velocity = SPACE_INVADERS_USER_SHIP_START_VELOCITY,
    .acceleration = SPACE_INVADERS_USER_SHIP_START_ACCELERATION,
    .opacity = SPACE_INVADERS_USER_SHIP_OPACITY,
    .solid = SPACE_INVADERS_USER_SHIP_SOLID,
};

static const struct entity_init_struct space_invaders_enemy_ship_init_struct = {
    .rectangle = (struct rectangle){ENEMY_SHIP_START_POSITIONS_TOP_LEFT[0],
                                    ENEMY_SHIP_START_POSITIONS_BOTTOM_RIGHT[0]},
    .mass = INFINITE_MASS,
    .velocity = SPACE_INVADERS_ENEMY_SHIP_START_VELOCITY,
    .acceleration = SPACE_INVADERS_ENEMY_SHIP_START_ACCELERATION,
    .opacity = SPACE_INVADERS_ENEMY_SHIP_OPACITY,
    .solid = SPACE_INVADERS_ENEMY_SHIP_SOLID,
};
static const struct entity_init_struct space_invaders_user_bullet_init_struct =
    {
        .rectangle =
            (struct rectangle){SPACE_INVADERS_USER_BULLET_START_POSITION},
        .mass = INFINITE_MASS,
        .velocity = SPACE_INVADERS_USER_BULLET_START_VELOCITY,
        .acceleration = SPACE_INVADERS_USER_BULLET_START_ACCELERATION,
        .opacity = SPACE_INVADERS_USER_BULLET_OPACITY,
        .solid = SPACE_INVADERS_USER_BULLET_SOLID,
};

static const struct entity_init_struct space_invaders_enemy_bullet_init_struct =
    {
        .rectangle =
            (struct rectangle){SPACE_INVADERS_ENEMY_BULLET_START_POSITION},
        .mass = INFINITE_MASS,
        .velocity = SPACE_INVADERS_ENEMY_BULLET_START_VELOCITY,
        .acceleration = SPACE_INVADERS_ENEMY_BULLET_START_ACCELERATION,
        .opacity = SPACE_INVADERS_ENEMY_BULLET_OPACITY,
        .solid = SPACE_INVADERS_ENEMY_BULLET_SOLID,
};

#define CREATE_SPACE_INVADERS_GAME()                                  \
    (struct space_invaders_game) {                                    \
        .config =                                                     \
            {                                                         \
                .user_ship_init_struct =                              \
                    &space_invaders_user_ship_init_struct,            \
                .enemy_ship_init_struct =                             \
                    &space_invaders_enemy_ship_init_struct,           \
                .user_bullet_init_struct =                            \
                    &space_invaders_user_bullet_init_struct,          \
                .enemy_bullet_init_struct =                           \
                    &space_invaders_enemy_bullet_init_struct,         \
                .user_ship_boundary_conditions =                      \
                    &space_invaders_user_ship_boundary_conditions,    \
                .enemy_ship_boundary_conditions =                     \
                    &space_invaders_enemy_ship_boundary_conditions,   \
                .user_bullet_boundary_conditions =                    \
                    &space_invaders_user_bullet_boundary_conditions,  \
                .enemy_bullet_boundary_conditions =                   \
                    &space_invaders_enemy_bullet_boundary_conditions, \
            },                                                        \
        .context = {0},                                               \
    }

void space_invaders_game_process_event_queue(
    struct space_invaders_game *space_invaders_game);

void space_invaders_game_process_input(
    struct space_invaders_game *space_invaders_game, tilt_flags tilt_flags);

void space_invaders_game_pause(struct space_invaders_game *space_invaders_game);

void space_invaders_game_unpause(
    struct space_invaders_game *space_invaders_game);

void space_invaders_game_reset(struct space_invaders_game *space_invaders_game);

#endif