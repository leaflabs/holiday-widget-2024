#include "snowfall_game.h"

#include "music_player.h"

enum entity_creation_error snowfall_game_init(
    struct snowfall_game *snowfall_game) {
    const struct snowfall_game_config *config = &snowfall_game->config;
    struct snowfall_game_context *context = &snowfall_game->context;

    game_common_init(&context->game_common);

    /******************/
    /*  Add Entities  */
    /******************/
    struct entity_creation_result result;

    /* Add enemy ship entities */
    for (int i = 0; i < SNOWFALL_MAX_SNOWFLAKES; i++) {
        result = add_entity(&context->game_common.environment,
                            config->snowflake_init_struct);
        if (result.error != ENTITY_CREATION_SUCCESS) {
            LOG_ERR("Failed to create snowflake entity %d: %d", i,
                    result.error);
            return result.error;
        } else {
            bool ret = game_entity_init(&context->snowflakes[i], result.entity,
                                        SNOWFALL_SNOWFLAKE_SPRITE);
            if (!ret) {
                LOG_ERR("Failed to create snowflake game entity %d", i);
                result.error = ENTITY_CREATION_INVALID_TYPE;
                return result.error;
            }
        }
    }

    context->num_snowflakes = 0;

    return 0;
}

static struct game_entity *get_next_inactive_snowflake(
    struct snowfall_game *snowfall_game) {
    struct snowfall_game_context *context = &snowfall_game->context;

    uint8_t entity_idx = 0;
    struct game_entity *snowflake = &context->snowflakes[entity_idx++];
    while (entity_idx < SNOWFALL_MAX_SNOWFLAKES && snowflake->entity->active) {
        snowflake = &context->snowflakes[entity_idx++];
    }

    if (entity_idx == SNOWFALL_MAX_SNOWFLAKES) {
        return NULL;
    }

    return snowflake;
}

static void place_next_snowflake(struct snowfall_game *snowfall_game,
                                 const struct random_number_generator *rng) {
    struct snowfall_game_context *context = &snowfall_game->context;

    if (context->num_snowflakes < SNOWFALL_MAX_SNOWFLAKES) {
        struct game_entity *snowflake =
            get_next_inactive_snowflake(snowfall_game);

        if (snowflake == NULL) {
            LOG_ERR("Ran out of snowflakes");
            return;
        }

        static int32_t prev_nums[] = {-1, -1, -1, -1, -1};
        static const uint8_t prev_nums_len =
            sizeof(prev_nums) / sizeof(prev_nums[0]);

        static uint8_t prev_idx = 0;
        uint32_t random_number =
            random_number_generator_get_next_in_n(rng, GRID_SIZE);

        bool invalid_value = true;
        while (ARRAY_CONTAINS(prev_nums, random_number, prev_nums_len)) {
            random_number =
                random_number_generator_get_next_in_n(rng, GRID_SIZE);
        }

        prev_nums[prev_idx] = random_number;
        prev_idx = (prev_idx + 1) % prev_nums_len;

        set_game_entity_position(snowflake,
                                 TOP_LEFT_POSITION_FROM_GRID(random_number, 0));

        activate_game_entity(snowflake);
        context->num_snowflakes++;
    }
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

    place_next_snowflake(snowfall_game, rng);
}

void snowfall_game_process_event_queue(struct snowfall_game *snowfall_game) {
    struct snowfall_game_context *context = &snowfall_game->context;
    struct ring_buffer *event_queue = &context->game_common.event_queue;

    struct physics_engine_event event;

    while (ring_buffer_pop(event_queue, &event) == 0) {
        switch (event.type) {
            case OUT_OF_BOUNDS_EVENT: {
                deactivate_entity(event.out_of_bounds_event.ent);
                context->num_snowflakes--;
            } break;
            case COLLISION_EVENT:
                break;
        }
    }
}