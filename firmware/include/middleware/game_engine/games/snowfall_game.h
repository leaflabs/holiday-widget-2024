#ifndef __SNOWFALL_GAME_H__
#define __SNOWFALL_GAME_H__
#include "game_entity.h"
#include "game_state.h"
#include "physics_engine.h"

#define SNOWFALL_MAX_SNOWFLAKES 30

#define SNOWFALL_MIN_DURATION_MS 1500  // 1000

/**************************/
/*  Snowflake Settings  */
/**************************/

/* Start position of snowflake */
#define SNOWFALL_SNOWFLAKE_START_POSITION \
    TOP_LEFT_POSITION_FROM_GRID(0, 0), BOTTOM_RIGHT_POSITION_FROM_GRID(0, 0)

/* Start velocity of snowflake */
#define SNOWFALL_SNOWFLAKE_START_VELOCITY \
    (velocity) {                          \
        0, 2                              \
    }
/* Start acceleration of snowflake */
#define SNOWFALL_SNOWFLAKE_START_ACCELERATION \
    (acceleration) {                          \
        0, 0                                  \
    }

/* Opacity of snowflake */
#define SNOWFALL_SNOWFLAKE_OPACITY (uint8_t)255

/* Are snowflakes solid */
#define SNOWFALL_SNOWFLAKE_SOLID false

/* Snowflake sprite */
#define SNOWFALL_SNOWFLAKE_SPRITE (&small_ball)

struct snowfall_game_config {
    /* Snowman top initialization struct */
    const struct entity_init_struct *const snowflake_init_struct;

    /* Snowman top boundary conditions */
    const struct boundary_conditions *const snowflake_boundary_conditions;
} __attribute__((aligned(4)));

struct snowfall_game_context {
    struct physics_engine_environment environment;
    struct physics_engine_event_queue event_queue;
    union {
        struct {
            struct game_entity snowflakes[SNOWFALL_MAX_SNOWFLAKES];
        };
        struct game_entity game_entities[SNOWFALL_MAX_SNOWFLAKES];
    };
    enum game_state game_state;
    uint8_t num_snowflakes;
};

struct snowfall_game {
    const struct snowfall_game_config config;
    struct snowfall_game_context context;
};

enum entity_creation_error snowfall_game_init(
    struct snowfall_game *snowfall_game);

static const struct boundary_conditions snowflake_boundary_conditions = {
    .left_boundary_action = BOUNDARY_ACTION_BOUNCE,
    .right_boundary_action = BOUNDARY_ACTION_BOUNCE,
    .top_boundary_action = BOUNDARY_ACTION_BOUNCE,
    .bottom_boundary_action = BOUNDARY_ACTION_BOUNCE,
};

/* Snowflake initialization struct */
static const struct entity_init_struct snowflake_init_struct = {
    .rectangle = (struct rectangle){SNOWFALL_SNOWFLAKE_START_POSITION},
    .mass = MODERATE_MASS,
    .velocity = SNOWFALL_SNOWFLAKE_START_VELOCITY,
    .acceleration = SNOWFALL_SNOWFLAKE_START_ACCELERATION,
    .opacity = SNOWFALL_SNOWFLAKE_OPACITY,
    .solid = SNOWFALL_SNOWFLAKE_SOLID,
};

#define CREATE_SNOWFALL_GAME()                                   \
    (struct snowfall_game) {                                     \
        .config =                                                \
            {                                                    \
                .snowflake_init_struct = &snowflake_init_struct, \
                .snowflake_boundary_conditions =                 \
                    &snowflake_boundary_conditions,              \
            },                                                   \
        .context = {0},                                          \
    }

void update_snowfall_game(struct snowfall_game *snowfall_game,
                          const struct random_number_generator *rng,
                          uint32_t delta_t);
void snowfall_game_process_event_queue(struct snowfall_game *snowfall_game);

#endif