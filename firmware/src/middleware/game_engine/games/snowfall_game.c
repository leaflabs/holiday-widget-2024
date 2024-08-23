#include "snowfall_game.h"

#include "music_player.h"

enum entity_creation_error snowfall_game_init(
    struct snowfall_game *snowfall_game) {
    const struct snowfall_game_config *config = &snowfall_game->config;
    struct snowfall_game_context *context = &snowfall_game->context;

    /* Clear environment instance */
    context->environment = (struct physics_engine_environment){0};

    /* Clear event queue */
    context->event_queue = (struct physics_engine_event_queue){0};
    physics_engine_event_queue_init(&context->event_queue);

    /******************/
    /*  Add Entities  */
    /******************/
    struct entity_creation_result result;

    /* Add enemy ship entities */
    for (int i = 0; i < SNOWFALL_MAX_SNOWFLAKES; i++) {
        result =
            add_entity(&context->environment, config->snowflake_init_struct);
        if (result.error != ENTITY_CREATION_SUCCESS) {
            LOG_ERR("Failed to create snowflake entity %d: %d", i,
                    result.error);
            return result.error;
        } else {
            bool ret = game_entity_init(&context->snowflakes[i], result.entity,
                                        SNOWFALL_SNOWFLAKE_SPRITE,
                                        config->snowflake_boundary_conditions);
            if (!ret) {
                LOG_ERR("Failed to create snowflake game entity %d", i);
                result.error = ENTITY_CREATION_INVALID_TYPE;
                return result.error;
            }
        }
    }

    context->num_snowflakes = 0;
    context->game_state = GAME_STATE_IN_PROGRESS;

    return 0;
}

void update_snowfall_game(struct snowfall_game *snowfall_game,
                          const struct random_number_generator *rng,
                          uint32_t delta_t) {
    struct snowfall_game_context *context = &snowfall_game->context;
    static uint32_t time_elapsed = 0;
    static enum Song current_song = NO_SONG;

    if (!music_player_is_song_playing(&music_player)) {
        switch (current_song) {
            case JINGLE_BELLS:
                current_song = WE_WISH_YOU_A_MERRY_CHRISTMAS;
                break;
            case WE_WISH_YOU_A_MERRY_CHRISTMAS:
                current_song = DECK_THE_HALLS;
                break;
            case DECK_THE_HALLS:  // fall-through
            case NO_SONG:
                current_song = JINGLE_BELLS;
                break;
        }
        // current_song = SILENT_TEST_SOUND;
        enum music_player_error error =
            music_player_play_song(&music_player, current_song);
        if (error != MUSIC_PLAYER_NO_ERROR) {
            LOG_ERR("Failed to play song <%d>: %d", current_song, error);
        } else {
            LOG_ERR("Snowfall now playing <%d>", current_song);
        }
    }

    time_elapsed += delta_t;
    if (time_elapsed < SNOWFALL_MIN_DURATION_MS) {
        return;
    }

    time_elapsed -= SNOWFALL_MIN_DURATION_MS;
    if (context->num_snowflakes < SNOWFALL_MAX_SNOWFLAKES) {
        uint8_t entity_idx = 0;
        struct game_entity *snowflake = &context->snowflakes[entity_idx++];
        while (entity_idx < SNOWFALL_MAX_SNOWFLAKES &&
               snowflake->entity->active) {
            snowflake = &context->snowflakes[entity_idx++];
        }

        uint32_t random_number;
        if (entity_idx == SNOWFALL_MAX_SNOWFLAKES) {
            LOG_ERR("Ran out of snowflakes");
            return;
        }
        /*if (col == 1) {
            random_number = random_number_generator_get_next_in_n(rng, 3);
            set_game_entity_position(snowflake,
        TOP_LEFT_POSITION_FROM_GRID(random_number + 2, 0)); col = 2; } else {
            random_number = random_number_generator_get_next_in_n(rng, 2);
            if (col == 0) {
                set_game_entity_position(snowflake,
        TOP_LEFT_POSITION_FROM_GRID(random_number, 0)); col = 1; } else {
                set_game_entity_position(snowflake,
        TOP_LEFT_POSITION_FROM_GRID(random_number + 5, 0)); col = 0;
            }
        }*/
        static int32_t prev[] = {-1, -1, -1, -1, -1};
        static uint8_t prev_idx = 0;
        random_number = random_number_generator_get_next_in_n(rng, GRID_SIZE);
        while (random_number == prev[0] || random_number == prev[1] ||
               random_number == prev[2] || random_number == prev[3] ||
               random_number == prev[4]) {
            random_number =
                random_number_generator_get_next_in_n(rng, GRID_SIZE);
        }

        prev[prev_idx] = random_number;
        prev_idx++;
        prev_idx %= 3;

        set_game_entity_position(snowflake,
                                 TOP_LEFT_POSITION_FROM_GRID(random_number, 0));

        activate_game_entity(snowflake);
        context->num_snowflakes++;
    }
}

void snowfall_game_process_event_queue(struct snowfall_game *snowfall_game) {
    struct snowfall_game_context *context = &snowfall_game->context;
    struct physics_engine_event_queue *event_queue = &context->event_queue;

    struct physics_engine_event event;

    while (physics_engine_event_queue_dequeue(event_queue, &event)) {
        switch (event.type) {
            case OUT_OF_BOUNDS_EVENT: {
                deactivate_entity(event.out_of_bounds_event.ent);
                context->num_snowflakes--;
            } break;
            case COLLISION_EVENT: {
                // deactivate_entity(event.collision_event.ent2);
                // context->num_snowflakes--;
            } break;
        }
    }
}