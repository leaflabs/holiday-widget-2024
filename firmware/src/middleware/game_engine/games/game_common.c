#include "game_common.h"

void game_common_init(struct game_common *game_common) {
    memset((void *)&game_common->environment, 0,
           sizeof(game_common->environment));
    memset((void *)game_common->__event_buffer, 0,
           sizeof(game_common->__event_buffer));
    ring_buffer_init(&game_common->event_queue, game_common->__event_buffer,
                     sizeof(game_common->__event_buffer[0]), EVENT_QUEUE_SIZE);
    game_common->game_state = GAME_STATE_IN_PROGRESS;
};