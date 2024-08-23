#ifndef __GAME_ENGINE_H__
#define __GAME_ENGINE_H__
#include "brick_breaker_game.h"
#include "pong_game.h"
#include "snowfall_game.h"
#include "space_invaders_game.h"

enum game_type {
    PONG_GAME,
    SPACE_INVADERS_GAME,
    SNOWFALL_GAME,
    BRICK_BREAKER_GAME,
    NUM_OF_GAMES,
    NO_GAME,
};

static const char *game_type_to_str[] = {
    [PONG_GAME] = "Pong Game",
    [SPACE_INVADERS_GAME] = "Asteroids Game",
    [SNOWFALL_GAME] = "Snowfall Game",
    [BRICK_BREAKER_GAME] = "Brick Breaker Game",
    [NUM_OF_GAMES] = "Number of Games",
    [NO_GAME] = "No Game",
};

struct game_engine_config {
    TIM_TypeDef *tim;
    uint32_t prescaler;
    uint32_t clock_division;
};

struct game_engine_context {
    struct physics_engine physics_engine __attribute__((aligned(4)));
    struct pong_game pong_game __attribute__((aligned(4)));
    struct space_invaders_game space_invaders_game __attribute__((aligned(4)));
    struct snowfall_game snowfall_game __attribute__((aligned(4)));
    struct brick_breaker_game brick_breaker_game __attribute__((aligned(4)));

    enum game_type current_game __attribute__((aligned(4)));

    TIM_HandleTypeDef htim;

    uint32_t pause_time;
    int32_t pause_duration;
    bool paused;
} __attribute__((aligned(4)));

struct game_engine {
    const struct game_engine_config config;
    struct game_engine_context context;
};

void game_engine_setup(void);
void game_engine_run(void);
void set_game(enum game_type game);
void pause_game_engine(int32_t duration);
void unpause_game_engine(void);
enum game_state game_engine_get_current_game_state();
#endif
