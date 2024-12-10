#ifndef __GAME_H__
#define __GAME_H__
#include "brick_breaker_game.h"
#include "fft_game.h"
#include "pong_game.h"
#include "snowfall_game.h"
#include "space_invaders_game.h"

union game {
    struct pong_game *pong_game;
    struct snowfall_game *snowfall_game;
    struct brick_breaker_game *brick_breaker_game;
    struct space_invaders_game *space_invaders_game;
    struct fft_game *fft_game;
};

#endif