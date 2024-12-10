#include "game_engine.h"

#include "imp23absu_driver.h"
#include "led_matrix.h"
#include "logging.h"
#include "lsm6dsm_driver.h"
#include "music_player.h"
#include "physics_engine.h"
#include "printf/printf.h"
#include "stm32l0xx_hal_conf.h"
#include "system_communication.h"
#include "utils.h"

extern volatile bool update_requested;
extern struct driver_comm_shared_memory led_matrix_comm;

#define TICKS_TO_MS(__prescaler__, __clockdiv__, __ticks__) \
    ((__ticks__ * 1000 * __prescaler__ * __clockdiv__) / (MSI_VALUE))

#define CLOCK_DIVISION_VAL_TO_ENUM(__clockdiv__)                            \
    (__clockdiv__ == 1                                                      \
         ? TIM_CLOCKDIVISION_DIV1                                           \
         : (__clockdiv__ == 2 ? TIM_CLOCKDIVISION_DIV2                      \
                              : (__clockdiv__ == 4 ? TIM_CLOCKDIVISION_DIV4 \
                                                   : TIM_CLOCKDIVISION_DIV1)))

struct game_engine game_engine = {
    .config =
        {
            .tim = TIM21,
            .prescaler = 40U,
            .clock_division = 4,
        },
    .context =
        (struct game_engine_context){
            .physics_engine = (struct physics_engine){0},
            .pong_game = CREATE_PONG_GAME(),
            .space_invaders_game = CREATE_SPACE_INVADERS_GAME(),
            .snowfall_game = CREATE_SNOWFALL_GAME(),
            .brick_breaker_game = CREATE_BRICK_BREAKER_GAME(),
            .fft_game = CREATE_FFT_GAME(),
            .htim = {0},
        },
};

static void game_engine_init(struct game_engine *game_engine);
static bool game_engine_set_game(struct game_engine *game_engine,
                                 enum game_type game_type);

static void update_game(struct game_engine *game_engine, uint32_t delta_t) {
    static char score_string[16];
    struct game_engine_context *context = &game_engine->context;
    switch (context->current_game) {
        case PONG_GAME:
            switch (context->pong_game.context.game_common.game_state) {
                case GAME_STATE_IN_PROGRESS:
                    for (int i = 0; i < context->pong_game.context.game_common
                                            .environment.num_of_entities;
                         i++) {
                        context->pong_game.context.game_entities[i].sprite.x =
                            GET_POSITION_GRID_X(
                                context->pong_game.context.game_entities[i]
                                    .entity->rectangle.p1);
                        context->pong_game.context.game_entities[i].sprite.y =
                            GET_POSITION_GRID_Y(
                                context->pong_game.context.game_entities[i]
                                    .entity->rectangle.p1);
                    }
                    pong_game_process_event_queue(&context->pong_game);
                    pong_game_process_input(
                        &context->pong_game,
                        lsm6dsm_driver_get_tilt_flags(lsm6dsm));
                    lsm6dsm_driver_clear_tilt_flags(lsm6dsm);
                    break;

                case GAME_STATE_SCORE_CHANGE:
                    snprintf_(score_string, 16, " %d-%d",
                              context->pong_game.context.user_score,
                              context->pong_game.context.opponent_score);
                    if (led_matrix_scroll_text(score_string,
                                               SCROLL_SPEED_MODERATE) == 0) {
                        physics_engine_environment_unpause(
                            &context->pong_game.context.game_common
                                 .environment);
                        context->pong_game.context.game_common.game_state =
                            GAME_STATE_IN_PROGRESS;
                    }
                    break;

                case GAME_STATE_YOU_WIN:
                    if (led_matrix_scroll_text(" YOU WIN!",
                                               SCROLL_SPEED_MODERATE) == 0) {
                        pong_game_reset(&context->pong_game);
                    }
                    break;

                case GAME_STATE_YOU_LOSE:
                    if (led_matrix_scroll_text(" YOU LOSE!",
                                               SCROLL_SPEED_MODERATE) == 0) {
                        pong_game_reset(&context->pong_game);
                    }
                    break;
                default:
                    LOG_ERR("In Default");
                    break;
            }
            break;
        case SPACE_INVADERS_GAME:
            switch (
                context->space_invaders_game.context.game_common.game_state) {
                case GAME_STATE_IN_PROGRESS:
                    for (int i = 0;
                         i < context->space_invaders_game.context.game_common
                                 .environment.num_of_entities;
                         i++) {
                        context->space_invaders_game.context.game_entities[i]
                            .sprite.x = GET_POSITION_GRID_X(
                            context->space_invaders_game.context
                                .game_entities[i]
                                .entity->rectangle.p1);
                        context->space_invaders_game.context.game_entities[i]
                            .sprite.y = GET_POSITION_GRID_Y(
                            context->space_invaders_game.context
                                .game_entities[i]
                                .entity->rectangle.p1);
                    }

                    update_space_invaders_game(&context->space_invaders_game,
                                               &context->physics_engine.context
                                                    .random_number_generator);
                    space_invaders_game_process_event_queue(
                        &context->space_invaders_game);
                    space_invaders_game_process_input(
                        &context->space_invaders_game,
                        lsm6dsm_driver_get_tilt_flags(lsm6dsm));
                    lsm6dsm_driver_clear_tilt_flags(lsm6dsm);
                    break;
                case GAME_STATE_SCORE_CHANGE:
                    snprintf_(score_string, 16, " %d LIVES",
                              context->space_invaders_game.context.lives);
                    if (led_matrix_scroll_text(score_string,
                                               SCROLL_SPEED_MODERATE) == 0) {
                        physics_engine_environment_unpause(
                            &context->space_invaders_game.context.game_common
                                 .environment);
                        context->space_invaders_game.context.game_common
                            .game_state = GAME_STATE_IN_PROGRESS;
                    }
                    break;
                case GAME_STATE_YOU_WIN:
                    if (led_matrix_scroll_text(" YOU WIN!",
                                               SCROLL_SPEED_MODERATE) == 0) {
                        space_invaders_game_reset(
                            &context->space_invaders_game);
                    }
                    break;

                case GAME_STATE_YOU_LOSE:
                    if (led_matrix_scroll_text(" YOU LOSE!",
                                               SCROLL_SPEED_MODERATE) == 0) {
                        space_invaders_game_reset(
                            &context->space_invaders_game);
                    }
                    break;
                default:
                    LOG_ERR("In Default");
                    break;
            }
            break;
        case SNOWFALL_GAME:
            for (int i = 0; i < context->snowfall_game.context.game_common
                                    .environment.num_of_entities;
                 i++) {
                context->snowfall_game.context.game_entities[i].sprite.x =
                    GET_POSITION_GRID_X(
                        context->snowfall_game.context.game_entities[i]
                            .entity->rectangle.p1);
                context->snowfall_game.context.game_entities[i].sprite.y =
                    GET_POSITION_GRID_Y(
                        context->snowfall_game.context.game_entities[i]
                            .entity->rectangle.p1);
            }
            update_snowfall_game(
                &context->snowfall_game,
                &context->physics_engine.context.random_number_generator,
                delta_t);
            snowfall_game_process_event_queue(&context->snowfall_game);
            break;
        case BRICK_BREAKER_GAME:
            switch (
                context->brick_breaker_game.context.game_common.game_state) {
                case GAME_STATE_IN_PROGRESS:
                    for (int i = 0;
                         i < context->brick_breaker_game.context.game_common
                                 .environment.num_of_entities;
                         i++) {
                        context->brick_breaker_game.context.game_entities[i]
                            .sprite.x = GET_POSITION_GRID_X(
                            context->brick_breaker_game.context.game_entities[i]
                                .entity->rectangle.p1);
                        context->brick_breaker_game.context.game_entities[i]
                            .sprite.y = GET_POSITION_GRID_Y(
                            context->brick_breaker_game.context.game_entities[i]
                                .entity->rectangle.p1);
                    }
                    brick_breaker_game_process_event_queue(
                        &context->brick_breaker_game,
                        &context->physics_engine.context
                             .random_number_generator);
                    brick_breaker_game_process_input(
                        &context->brick_breaker_game,
                        lsm6dsm_driver_get_tilt_flags(lsm6dsm));
                    lsm6dsm_driver_clear_tilt_flags(lsm6dsm);
                    break;

                case GAME_STATE_SCORE_CHANGE:
                    snprintf_(score_string, 16, " %d LIVES",
                              context->brick_breaker_game.context.lives);
                    if (led_matrix_scroll_text(score_string,
                                               SCROLL_SPEED_MODERATE) == 0) {
                        physics_engine_environment_unpause(
                            &context->brick_breaker_game.context.game_common
                                 .environment);
                        context->brick_breaker_game.context.game_common
                            .game_state = GAME_STATE_IN_PROGRESS;
                    }
                    break;

                case GAME_STATE_YOU_WIN:
                    if (led_matrix_scroll_text(" YOU WIN!",
                                               SCROLL_SPEED_MODERATE) == 0) {
                        brick_breaker_game_reset(&context->brick_breaker_game);
                    }
                    break;

                case GAME_STATE_YOU_LOSE:
                    if (led_matrix_scroll_text(" YOU LOSE!",
                                               SCROLL_SPEED_MODERATE) == 0) {
                        brick_breaker_game_reset(&context->brick_breaker_game);
                    }
                    break;
                default:
                    LOG_ERR("In Default");
                    break;
            }
            break;
        case FFT_GAME:
            fft_game_update(&context->fft_game);
            break;
        case NO_GAME:
            break;
        default:
            LOG_ERR("Unknown game type: %d", context->current_game);
            break;
    }
}

static void update_game_engine(struct game_engine *game_engine,
                               uint32_t delta_t) {
    struct game_engine_context *context = &game_engine->context;

    if (context->current_game != NO_GAME) {
        update_game(game_engine, delta_t);
        physics_engine_update(&context->physics_engine, delta_t);
    }
}

void game_engine_setup(void) {
    game_engine_init(&game_engine);
}

void game_engine_run(void) {
    static enum game_type previous_game = NO_GAME;
    const struct game_engine_config *cfg = &game_engine.config;
    struct game_engine_context *context = &game_engine.context;

    if (!context->paused) {
        if (context->current_game != previous_game) {
            if (!game_engine_set_game(&game_engine, context->current_game)) {
                LOG_ERR("Failed to set game to %d", context->current_game);
            } else {
                previous_game = context->current_game;
            }
        }

        if (update_requested) {
            if (__HAL_TIM_GET_FLAG(&context->htim, TIM_FLAG_UPDATE)) {
                update_game_engine(
                    &game_engine, TICKS_TO_MS(cfg->prescaler,
                                              cfg->clock_division, UINT16_MAX));
                __HAL_TIM_SET_COUNTER(&context->htim, 0);
                HAL_TIM_Base_Start(&context->htim);
            } else {
                update_game_engine(
                    &game_engine,
                    TICKS_TO_MS(cfg->prescaler, cfg->clock_division,
                                __HAL_TIM_GET_COUNTER(&context->htim)));
                // times[idx++] = __HAL_TIM_GET_COUNTER(&context->htim);
                __HAL_TIM_SET_COUNTER(&context->htim, 0);
            }
            update_requested = false;
        }

    } else {
        if (context->pause_duration > 0) {
            if (HAL_GetTick() - context->pause_time >=
                context->pause_duration) {
                unpause_game_engine();
            }
        }
    }
}

void set_game(enum game_type game) {
    if (game_engine.context.current_game == FFT_GAME && game != FFT_GAME) {
        imp23absu_driver_disable();
    } else if (game_engine.context.current_game != FFT_GAME &&
               game == FFT_GAME) {
        imp23absu_driver_enable();
    }

    game_engine.context.current_game = game;
}

static int tim21_init(struct game_engine *game_engine) {
    const struct game_engine_config *cfg = &game_engine->config;
    struct game_engine_context *context = &game_engine->context;
    /* TIM21 Peripheral Clock Enable */
    __HAL_RCC_TIM21_CLK_ENABLE();

    // Set TIM21 to 1 tick per ~488 microseconds - max duration:
    // 65535*0.488ms=~32ms
    context->htim.Init.Prescaler = cfg->prescaler;  // 0x0200;
    context->htim.Init.ClockDivision =
        CLOCK_DIVISION_VAL_TO_ENUM(cfg->clock_division);
    context->htim.Init.Period = 0xFFFF;
    context->htim.Instance = cfg->tim;
    context->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    context->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&context->htim) != HAL_OK) {
        /* TIM21 Initialization Error */
        return -1;
    }

    /* Reset the OPM Bit */
    context->htim.Instance->CR1 &= ~TIM_CR1_OPM;

    /* Configure the OPM Mode */
    context->htim.Instance->CR1 |= TIM_OPMODE_SINGLE;

    TIM_ClockConfigTypeDef clock_source_config = {0};
    clock_source_config.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&context->htim, &clock_source_config)) {
        /* TIM Clock Source Configuration Error */
        return -1;
    }

    __HAL_TIM_CLEAR_FLAG(&context->htim, TIM_FLAG_UPDATE);

    return 0;
}

static void game_engine_init(struct game_engine *game_engine) {
    const struct game_engine_config *config = &game_engine->config;
    struct game_engine_context *context = &game_engine->context;

    /* Set current game to NO_GAME */
    context->current_game = NO_GAME;

    int ret = tim21_init(game_engine);
    if (ret != 0) {
        LOG_ERR(
            "Error initializing game engine: failed to initialize TIM21 - %d",
            ret);
    }

    /* Initialize the physics engine */
    physics_engine_init(&context->physics_engine);

    enum entity_creation_error error;

    /* Initialize Asteroids Game */
    if ((error = space_invaders_game_init(&context->space_invaders_game)) !=
        ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Error initializing Asteroids game: %d", error);
    } else {
        LOG_DBG("Asteroids game initialized");
    }

    /* Initialize Snowfall Game */
    if ((error = snowfall_game_init(&context->snowfall_game)) !=
        ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Error initializing Snowfall game: %d", error);
    } else {
        LOG_DBG("Snowfall game initialized");
    }

    /* Initialize Pong Game */
    if ((error = pong_game_init(&context->pong_game)) !=
        ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Error initializing Pong game: %d", error);
    } else {
        LOG_DBG("Pong game initialized");
    }

    /* Initialize Brick Breaker Game */
    if ((error = brick_breaker_game_init(&context->brick_breaker_game)) !=
        ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Error initializing Brick Breaker game: %d", error);
    } else {
        LOG_DBG("Brick Breaker game initialized");
    }

    /* Initialize FFT Game */
    if ((error = fft_game_init(&context->fft_game)) !=
        ENTITY_CREATION_SUCCESS) {
        LOG_ERR("Error initializing FFT game: %d", error);
    } else {
        LOG_DBG("FFT game initialized");
    }

    context->paused = false;

    ret = HAL_TIM_Base_Start(&context->htim);
    if (ret != 0) {
        LOG_ERR("Error initializing game engine: Failed to start TIM21 - %d",
                ret);
    }
}

static bool game_engine_set_game(struct game_engine *game_engine,
                                 enum game_type game_type) {
    struct game_engine_context *context = &game_engine->context;
    switch (game_type) {
        case PONG_GAME:
            physics_engine_set_context(
                &context->physics_engine,
                &context->pong_game.context.game_common.environment,
                &context->pong_game.context.game_common.event_queue);
            led_matrix_comm.data.led_matrix.renderer.entities =
                context->pong_game.context.game_entities;
            led_matrix_comm.data.led_matrix.renderer.num_entities =
                context->pong_game.context.game_common.environment
                    .num_of_entities;
            break;
        case SPACE_INVADERS_GAME:
            physics_engine_set_context(
                &context->physics_engine,
                &context->space_invaders_game.context.game_common.environment,
                &context->space_invaders_game.context.game_common.event_queue);
            led_matrix_comm.data.led_matrix.renderer.entities =
                context->space_invaders_game.context.game_entities;
            led_matrix_comm.data.led_matrix.renderer.num_entities =
                context->space_invaders_game.context.game_common.environment
                    .num_of_entities;
            break;
        case SNOWFALL_GAME:
            physics_engine_set_context(
                &context->physics_engine,
                &context->snowfall_game.context.game_common.environment,
                &context->snowfall_game.context.game_common.event_queue);
            led_matrix_comm.data.led_matrix.renderer.entities =
                context->snowfall_game.context.game_entities;
            led_matrix_comm.data.led_matrix.renderer.num_entities =
                context->snowfall_game.context.game_common.environment
                    .num_of_entities;
            break;
        case BRICK_BREAKER_GAME:
            physics_engine_set_context(
                &context->physics_engine,
                &context->brick_breaker_game.context.game_common.environment,
                &context->brick_breaker_game.context.game_common.event_queue);
            led_matrix_comm.data.led_matrix.renderer.entities =
                context->brick_breaker_game.context.game_entities;
            led_matrix_comm.data.led_matrix.renderer.num_entities =
                context->brick_breaker_game.context.game_common.environment
                    .num_of_entities;
            break;
        case FFT_GAME:
            physics_engine_set_context(
                &context->physics_engine,
                &context->fft_game.context.game_common.environment,
                &context->fft_game.context.game_common.event_queue);
            led_matrix_comm.data.led_matrix.renderer.entities =
                context->fft_game.context.game_entities;
            led_matrix_comm.data.led_matrix.renderer.num_entities =
                context->fft_game.context.game_common.environment
                    .num_of_entities;
            break;
        case NO_GAME:
            physics_engine_set_context(&context->physics_engine, NULL, NULL);
            led_matrix_comm.data.led_matrix.renderer.entities = NULL;
            led_matrix_comm.data.led_matrix.renderer.num_entities = 0;
            break;
        default:
            LOG_ERR("Unsupported game: %d", game_type);
            physics_engine_set_context(&context->physics_engine, NULL, NULL);
            led_matrix_comm.data.led_matrix.renderer.entities = NULL;
            led_matrix_comm.data.led_matrix.renderer.num_entities = 0;
            return false;
    }
    LOG_DBG("Game set to %s", game_type_to_str[game_type]);

    /* Flush the LED matrix pipeline */
    led_matrix_comm.data.led_matrix.renderer.row = 0;
    led_matrix_comm.data.led_matrix.renderer.col = 0;
    led_matrix_comm.data.led_matrix.loader.row = 0;
    led_matrix_comm.data.led_matrix.loader.col = 0;
    led_matrix_comm.data.led_matrix.assembler.row = 0;
    led_matrix_comm.data.led_matrix.assembler.col = 0;

    LOG_DBG("Flushed LED Matrix pipeline");

    return true;
}

void pause_game_engine(int32_t duration) {
    game_engine.context.paused = true;
    game_engine.context.pause_time = HAL_GetTick();
    game_engine.context.pause_duration = duration;
    switch (game_engine.context.current_game) {
        case PONG_GAME:
            break;
        case SPACE_INVADERS_GAME:
            space_invaders_game_pause(&game_engine.context.space_invaders_game);
            break;
        case SNOWFALL_GAME:
            break;
        case BRICK_BREAKER_GAME:
            break;
    }

    int ret = HAL_TIM_Base_Stop(&game_engine.context.htim);
    if (ret != 0) {
        LOG_ERR("Error pausing game engine: Failed to stop TIM21 - %d", ret);
    }
}

void unpause_game_engine(void) {
    game_engine.context.paused = false;
    switch (game_engine.context.current_game) {
        case PONG_GAME:
            break;
        case SPACE_INVADERS_GAME:
            space_invaders_game_unpause(
                &game_engine.context.space_invaders_game);
            break;
        case SNOWFALL_GAME:
            break;
        case BRICK_BREAKER_GAME:
            break;
    }

    int ret = HAL_TIM_Base_Start(&game_engine.context.htim);
    if (ret != 0) {
        LOG_ERR("Error unpausing game engine: Failed to start TIM21 - %d", ret);
    }
}

enum game_state game_engine_get_current_game_state() {
    struct game_engine_context *context = &game_engine.context;

    switch (context->current_game) {
        case PONG_GAME:
            return context->pong_game.context.game_common.game_state;
        case SPACE_INVADERS_GAME:
            return context->space_invaders_game.context.game_common.game_state;
        case BRICK_BREAKER_GAME:
            return context->brick_breaker_game.context.game_common.game_state;
        default:
            return GAME_STATE_IN_PROGRESS;
    }
}
