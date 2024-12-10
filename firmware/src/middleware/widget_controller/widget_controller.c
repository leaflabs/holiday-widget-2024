#include "widget_controller.h"

#include "game_engine.h"
#include "imp23absu_driver.h"
#include "led_matrix.h"
#include "logging.h"
#include "lsm6dsm_driver.h"
#include "music_player.h"
#include "system_communication.h"

enum widget_state {
    WIDGET_PREINIT,    // One time configuration steps
    WIDGET_BASIC,      // Basic operating mode for the widget
    WIDGET_ENTER_LP1,  // Phase 1 of entering low power mode
    WIDGET_ENTER_LP2,  // Phase 2 of entering low power mode
    WIDGET_EXIT_LP,    // Exiting low power mode
    WIDGET_LP,         // In low power mode
    WIDGET_ERROR       // Error case
};

enum widget_mode {
    WIDGET_MODE_SNOWFALL_GAME,
    WIDGET_MODE_PONG_GAME,
    WIDGET_MODE_SPACE_INVADERS_GAME,
    WIDGET_MODE_BRICK_BREAKER_GAME,
    WIDGET_MODE_FFT,
    __NUM_WIDGET_MODES,
};

struct widget_context {
    enum widget_state state;
    enum widget_mode mode;
    uint32_t timer;  // Used for keeping track of time between events
};

/*
 * Communication structs are initaized here and externed by the appropriate
 * high level driver files for their context structs to get.
 *
 * Define in delay_ms how many miliseconds between sending requests
 */
// Message passing communication
struct driver_comm_message_passing acceleration_comm = {
    .request = {.type = REQUEST_TYPE_NONE, .status = REQUEST_STATUS_UNSEEN},
    .timing = {.delay_ms = 1000}};
struct driver_comm_message_passing ambient_light_comm = {
    .request = {.type = REQUEST_TYPE_NONE, .status = REQUEST_STATUS_UNSEEN},
    .timing = {.delay_ms = 1000}};
// Shared memory communication
struct driver_comm_shared_memory led_matrix_comm = {0};

static struct widget_context context = {.state = WIDGET_PREINIT, .timer = 0};

static void set_all_no_request(void);

static void set_all_enter_lp(void);

static void set_all_exit_lp(void);

static void update_led_matrix(void);

static void update_data(void);

static void update_mode(void) {
    music_player_abort_song(&music_player);
    switch (context.mode) {
        case WIDGET_MODE_PONG_GAME: /*fall-through*/
            set_game(PONG_GAME);
            break;
        case WIDGET_MODE_SPACE_INVADERS_GAME: /*fall-through*/
            set_game(SPACE_INVADERS_GAME);
            break;
        case WIDGET_MODE_BRICK_BREAKER_GAME: /*fall-through*/
            set_game(BRICK_BREAKER_GAME);
            break;
        case WIDGET_MODE_SNOWFALL_GAME:
            set_game(SNOWFALL_GAME);
            break;
        case WIDGET_MODE_FFT:
            set_game(FFT_GAME);
            break;
    }
}

static void next_mode(void) {
    context.mode = (context.mode + 1) % __NUM_WIDGET_MODES;
    update_mode();
}

static void previous_mode(void) {
    context.mode = (context.mode + __NUM_WIDGET_MODES - 1) % __NUM_WIDGET_MODES;
    update_mode();
}

static struct entity ent = (struct entity){
    .active = true,
};

/*
 * Temporary array of entities until game engine is working
 */
static struct game_entity entities[] = {
    {.entity = &ent, .sprite = {.map = &vertical_paddle, .x = 0, .y = 0}},
    {.entity = &ent, .sprite = {.map = &star, .x = 3, .y = 2}}};

void widget_controller_setup(void) {
    LOG_INF("[Widget controller initalization]");

    // Turn on all led_matrix functions that we need
    led_matrix_comm.data.led_matrix.loader.active = false;
    led_matrix_comm.data.led_matrix.renderer.active = true;
    led_matrix_comm.data.led_matrix.assembler.active = true;
    led_matrix_comm.data.led_matrix.drawer.active = false;

    // Set to finished so the first cycle of the state machine gives them jobs
    led_matrix_comm.data.led_matrix.loader.finished = true;
    led_matrix_comm.data.led_matrix.renderer.finished = true;
    led_matrix_comm.data.led_matrix.assembler.finished = true;
    led_matrix_comm.data.led_matrix.drawer.finished = true;

    // Stay at the same rate as the other functions
    led_matrix_comm.data.led_matrix.drawer.num_draws = NUM_LEDS;

    /*
     * Set which slots to start in. For now, oscillate between first two slots
     * And alternate which inputs and outputs match up so they don't overwrite
     * each other
     */
    led_matrix_comm.data.led_matrix.loader.input_frame = 0;
    led_matrix_comm.data.led_matrix.loader.output_slot = 0;
    led_matrix_comm.data.led_matrix.renderer.output_slot = 0;
    led_matrix_comm.data.led_matrix.assembler.input_slot = 1;
    led_matrix_comm.data.led_matrix.assembler.output_slot = 0;
    led_matrix_comm.data.led_matrix.drawer.input_slot = 1;

    // Update values for the renderer
    led_matrix_comm.data.led_matrix.renderer.entities = entities;
    led_matrix_comm.data.led_matrix.renderer.num_entities =
        sizeof(entities) / sizeof(struct game_entity);

    context.state = WIDGET_BASIC;
}

void widget_controller_run(void) {
    enum led_matrix_function {
        LED_MATRIX_LOADER,
        LED_MATRIX_ASSEMBLER,
        LED_MATRIX_RENDERER,
        LED_MATRIX_DRAWER,
    };
    static bool led_matrix_previous_state[4] = {false, false, false, false};
    switch (context.state) {
        case WIDGET_PREINIT: {
            // Nothing
        } break;

        case WIDGET_BASIC: {
            /*
             * Update data requests
             */
            update_data();

            tap_flags tap_flags = lsm6dsm_driver_get_tap_flags(lsm6dsm);
            if (tap_flags.double_tap && tap_flags.y_tap) {
                // Tap sign is not consistent when directly holding the board -
                // could change with board geometry and enclosure - for now, any
                // double tap advances to next mode
                /*if (tap_flags.tap_sign == 1) {
                    next_mode();
                    LOG_ERR("Next widget mode");
                } else {
                    previous_mode();
                    LOG_ERR("Previous widget mode");
                }*/

                next_mode();

                if (game_engine_get_current_game_state() ==
                    GAME_STATE_IN_PROGRESS) {
                    led_matrix_comm.data.led_matrix.loader.active = false;
                    led_matrix_comm.data.led_matrix.renderer.active = true;
                } else {
                    led_matrix_comm.data.led_matrix.loader.active = true;
                    led_matrix_comm.data.led_matrix.renderer.active = false;
                }
            }

            lsm6dsm_driver_clear_tap_flags(lsm6dsm);

            /*
             * Update LED matrix
             */
            update_led_matrix();

            /*
             * Check for inactivity, cue to enter low power mode
             */
            if (acceleration_comm.inactive_flag) {
                LOG_INF("[Entering Low Power Mode]");
                context.state = WIDGET_ENTER_LP1;

                /* Store previous function states */
                led_matrix_previous_state[LED_MATRIX_LOADER] =
                    led_matrix_comm.data.led_matrix.loader.active;
                led_matrix_previous_state[LED_MATRIX_ASSEMBLER] =
                    led_matrix_comm.data.led_matrix.assembler.active;
                led_matrix_previous_state[LED_MATRIX_RENDERER] =
                    led_matrix_comm.data.led_matrix.renderer.active;
                led_matrix_previous_state[LED_MATRIX_DRAWER] =
                    led_matrix_comm.data.led_matrix.drawer.active;

                // Turn off led matrix functions
                led_matrix_comm.data.led_matrix.loader.active = false;
                led_matrix_comm.data.led_matrix.renderer.active = false;
                led_matrix_comm.data.led_matrix.assembler.active = false;
                led_matrix_comm.data.led_matrix.drawer.active = false;
            }
        } break;

        /*
         * This state is for finishing all outstanding requests on the drivers
         * so we can request them to enter low power without issue.
         */
        case WIDGET_ENTER_LP1: {
            // Wait for all devices to finish any outstanding requests
            if (request_is_not_busy(&acceleration_comm) &&
                request_is_not_busy(&ambient_light_comm)) {
                set_all_enter_lp();

                if (context.mode == WIDGET_MODE_FFT) {
                    // If FFT Game is active when low-power mode is
                    // entered, then disable the IMP23ABSU driver
                    int error = imp23absu_driver_disable();
                    if (error != 0) {
                        LOG_ERR(
                            "Failed to disable IMP23ABSU Driver while entering "
                            "low-power mode: %d",
                            error);
                    }
                } else {
                    // Otherwise, if the music play is active when
                    // low-power mode is entered, then abort the song
                    if (music_player_is_song_playing(&music_player)) {
                        enum music_player_error error =
                            music_player_abort_song(&music_player);
                        if (error != MUSIC_PLAYER_NO_ERROR) {
                            LOG_ERR(
                                "Failed to abort music player while entering "
                                "low-power mode: %d",
                                error);
                        }
                    }
                }
                pause_game_engine(-1);
                pause_led_matrix();

                // Now for phase 2
                context.state = WIDGET_ENTER_LP2;
            }

        } break;

        /*
         * This state is for finishing the low power mode request to make
         * sure all drivers are in low power mode before we enter low power
         * mode
         */
        case WIDGET_ENTER_LP2: {
            // Make sure all devices finished going into lp mode
            if (request_is_finished(&acceleration_comm) &&
                request_is_finished(&ambient_light_comm)) {
                set_all_no_request();

                context.timer = HAL_GetTick();  // Reset the timer

                // Finally, go to low power mode
                LOG_INF("[In Low Power Mode]");
                context.state = WIDGET_LP;
            }
        } break;

        case WIDGET_LP: {
            /*
             * Low power - a land
             * where microcontrollers dream
             * and power, much saved
             *
             * -JB
             */

            if (acceleration_comm.inactive_flag == false) {
                // Tell each sensor to now exit lp mode
                set_all_exit_lp();

                context.state = WIDGET_EXIT_LP;
            }

        } break;

        case WIDGET_EXIT_LP: {
            LOG_INF("[Exiting Low Power Mode]");

            // Make sure all drivers have finished leaving low power mode
            if (request_is_finished(&acceleration_comm) &&
                request_is_finished(&ambient_light_comm)) {
                set_all_no_request();  // Just have them idle

                if (context.mode == WIDGET_MODE_FFT) {
                    int error = imp23absu_driver_enable();
                    if (error != 0) {
                        LOG_ERR(
                            "Failed to enable IMP23ABSU Driver while exiting "
                            "low-power mode: %d",
                            error);
                    }
                }

                context.timer = HAL_GetTick();  // Reset the timer

                unpause_game_engine();

                /* Restore previous function states */
                led_matrix_comm.data.led_matrix.loader.active =
                    led_matrix_previous_state[LED_MATRIX_LOADER];
                led_matrix_comm.data.led_matrix.assembler.active =
                    led_matrix_previous_state[LED_MATRIX_ASSEMBLER];
                led_matrix_comm.data.led_matrix.renderer.active =
                    led_matrix_previous_state[LED_MATRIX_RENDERER];
                led_matrix_comm.data.led_matrix.drawer.active =
                    led_matrix_previous_state[LED_MATRIX_DRAWER];

                unpause_led_matrix();

                // Go back to normal mode
                LOG_INF("[In Basic Mode]");
                context.state = WIDGET_BASIC;
            }

        } break;

        case WIDGET_ERROR: {
            // unused currently but here if needed
        } break;
    }
}

static void set_all_no_request(void) {
    acceleration_comm.request.status = REQUEST_STATUS_UNSEEN;
    acceleration_comm.request.type = REQUEST_TYPE_NONE;
    ambient_light_comm.request.status = REQUEST_STATUS_UNSEEN;
    ambient_light_comm.request.type = REQUEST_TYPE_NONE;
}

static void set_all_enter_lp(void) {
    acceleration_comm.request.status = REQUEST_STATUS_UNSEEN;
    acceleration_comm.request.type = REQUEST_TYPE_ENTER_LP;
    ambient_light_comm.request.status = REQUEST_STATUS_UNSEEN;
    ambient_light_comm.request.type = REQUEST_TYPE_ENTER_LP;
}
static void set_all_exit_lp(void) {
    acceleration_comm.request.status = REQUEST_STATUS_UNSEEN;
    acceleration_comm.request.type = REQUEST_TYPE_EXIT_LP;
    ambient_light_comm.request.status = REQUEST_STATUS_UNSEEN;
    ambient_light_comm.request.type = REQUEST_TYPE_EXIT_LP;
}

static void update_led_matrix(void) {
    /*
     * Process Led matrix functions
     */
    if (led_matrix_comm.data.led_matrix.loader.finished) {
        led_matrix_comm.data.led_matrix.loader.finished = false;

        // Now set the details
        led_matrix_comm.data.led_matrix.loader.input_anim =
            ANIM_RUNTIME_ANIMATION;

        // Cycle through the frames
        led_matrix_comm.data.led_matrix.loader.input_frame++;
        if (led_matrix_comm.data.led_matrix.loader.input_frame >=
            get_anim_length(
                led_matrix_comm.data.led_matrix.loader.input_anim)) {
            led_matrix_comm.data.led_matrix.loader.input_frame = 0;
        }

        // For now, alternate between first two slots
        led_matrix_comm.data.led_matrix.loader.output_slot++;
        led_matrix_comm.data.led_matrix.loader.output_slot &= 1;
    }

    if (led_matrix_comm.data.led_matrix.renderer.finished) {
        led_matrix_comm.data.led_matrix.renderer.finished = false;

        // For now, alternate between first two slots
        led_matrix_comm.data.led_matrix.renderer.output_slot++;
        led_matrix_comm.data.led_matrix.renderer.output_slot &= 1;
    }

    if (led_matrix_comm.data.led_matrix.assembler.finished) {
        led_matrix_comm.data.led_matrix.assembler.finished = false;

        // Alternate between first two slots
        led_matrix_comm.data.led_matrix.assembler.input_slot++;
        led_matrix_comm.data.led_matrix.assembler.input_slot &= 1;
        led_matrix_comm.data.led_matrix.assembler.output_slot++;
        led_matrix_comm.data.led_matrix.assembler.output_slot &= 1;
    }

    if (led_matrix_comm.data.led_matrix.drawer.finished) {
        led_matrix_comm.data.led_matrix.drawer.finished = false;

        // Alternate between first two slots
        led_matrix_comm.data.led_matrix.drawer.input_slot++;
        led_matrix_comm.data.led_matrix.drawer.input_slot &= 1;
    }
}

static void update_data(void) {
    if (request_is_finished(&acceleration_comm)) {
        acceleration_comm.request.status = REQUEST_STATUS_UNSEEN;
        acceleration_comm.request.type = REQUEST_TYPE_NONE;

        LOG_DBG("Acceleration: %f %f %f [g]",
                acceleration_comm.data.acceleration.x,
                acceleration_comm.data.acceleration.y,
                acceleration_comm.data.acceleration.z);
    }

    if (request_is_no_request(&acceleration_comm) &&
        request_is_time_ready(&acceleration_comm)) {
        // Make new request for data
        acceleration_comm.request.status = REQUEST_STATUS_UNSEEN;
        acceleration_comm.request.type = REQUEST_TYPE_DATA;
        acceleration_comm.timing.last_time = HAL_GetTick();
    }

    if (request_is_finished(&ambient_light_comm)) {
        ambient_light_comm.request.status = REQUEST_STATUS_UNSEEN;
        ambient_light_comm.request.type = REQUEST_TYPE_NONE;

        LOG_DBG("Light: %d %d", ambient_light_comm.data.ambient_light.proximity,
                ambient_light_comm.data.ambient_light.als);
    }

    if (request_is_no_request(&acceleration_comm) &&
        request_is_time_ready(&ambient_light_comm)) {
        // Make new request for data
        ambient_light_comm.request.status = REQUEST_STATUS_UNSEEN;
        ambient_light_comm.request.type = REQUEST_TYPE_DATA;
        ambient_light_comm.timing.last_time = HAL_GetTick();
    }
}
