#include "game_common.h"

void game_common_init(struct game_common *game_common) {
    memset((void *)&game_common->environment, 0,
           sizeof(game_common->environment));
    game_common->event_queue = (struct physics_engine_event_queue){0};
    physics_engine_event_queue_init(&game_common->event_queue);
    game_common->game_state = GAME_STATE_IN_PROGRESS;
};