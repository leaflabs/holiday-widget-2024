#ifndef __GAME_COMMON_H__
#define __GAME_COMMON_H__
#include "physics_engine_environment.h"
#include "physics_engine_events.h"

enum game_state {
    GAME_STATE_IN_PROGRESS,
    GAME_STATE_SCORE_CHANGE,
    GAME_STATE_YOU_WIN,
    GAME_STATE_YOU_LOSE,
};

struct game_common {
    struct physics_engine_environment environment;
    struct physics_engine_event_queue event_queue;
    enum game_state game_state;
};

void game_common_init(struct game_common *game_common);

#endif /*__GAME_COMMON_H__*/