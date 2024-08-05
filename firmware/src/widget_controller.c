#include "widget_controller.h"

#include "system_communication.h"

// Regular operation for 10 seconds, 5 seconds of sleep mode
#define WAKE_MS 10000
#define SLEEP_MS 5000

enum widget_state {
    WIDGET_PREINIT,    // One time configuration steps
    WIDGET_BASIC,      // Basic operating mode for the widget
    WIDGET_ENTER_LP1,  // Phase 1 of entering low power mode
    WIDGET_ENTER_LP2,  // Phase 2 of entering low power mode
    WIDGET_EXIT_LP,    // Exiting low power mode
    WIDGET_LP,         // In low power mode
    WIDGET_ERROR       // Error case
};

struct widget_context {
    enum widget_state state;
    uint32_t timer;  // Used for keeping track of time between events
};

/*
 * Communication structs are initaized here and externed by the appropriate
 * high level driver files for their context structs to get.
 *
 * Define in delay_ms how many miliseconds between sending requests
 */
struct driver_comm acceleration_comm = {
    .request = {.type = REQUEST_TYPE_NONE, .status = REQUEST_STATUS_UNSEEN},
    .timing = {.delay_ms = 500}};
struct driver_comm ambient_light_comm = {
    .request = {.type = REQUEST_TYPE_NONE, .status = REQUEST_STATUS_UNSEEN},
    .timing = {.delay_ms = 1000}};

static struct widget_context context = {.state = WIDGET_PREINIT, .timer = 0};

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

void widget_controller_run(void) {
    switch (context.state) {
        case WIDGET_PREINIT: {
            uart_logger_send("[Widget controller initalization]\r\n");
            context.state = WIDGET_BASIC;

        } break;

        case WIDGET_BASIC: {
            if (request_is_finished(&acceleration_comm)) {
                acceleration_comm.request.status = REQUEST_STATUS_UNSEEN;
                acceleration_comm.request.type = REQUEST_TYPE_NONE;

                uart_logger_send("Acceleration: %f %f %f [g]\r\n",
                                 acceleration_comm.results.data.acceleration.x,
                                 acceleration_comm.results.data.acceleration.y,
                                 acceleration_comm.results.data.acceleration.z);
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

                uart_logger_send(
                    "Light: %d %d\r\n",
                    ambient_light_comm.results.data.ambient_light.proximity,
                    ambient_light_comm.results.data.ambient_light.als);
            }

            if (request_is_no_request(&acceleration_comm) &&
                request_is_time_ready(&ambient_light_comm)) {
                // Make new request for data
                ambient_light_comm.request.status = REQUEST_STATUS_UNSEEN;
                ambient_light_comm.request.type = REQUEST_TYPE_DATA;
                ambient_light_comm.timing.last_time = HAL_GetTick();
            }

            // Lets enter low power mode
            if (HAL_GetTick() - context.timer > WAKE_MS) {
                uart_logger_send("[Entering Low Power Mode]\r\n");
                context.state = WIDGET_ENTER_LP1;
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
                uart_logger_send("[In Low Power Mode]\r\n");
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

            // For now, have a timer wake up the widget
            if (HAL_GetTick() - context.timer > SLEEP_MS) {
                // Tell each sensor to now exit lp mode
                set_all_exit_lp();

                context.state = WIDGET_EXIT_LP;
            }

        } break;

        case WIDGET_EXIT_LP: {
            uart_logger_send("[Exiting Low Power Mode]\r\n");

            // Make sure all drivers have finished leaving low power mode
            if (request_is_finished(&acceleration_comm) &&
                request_is_finished(&ambient_light_comm)) {
                set_all_no_request();  // Just have them idle

                context.timer = HAL_GetTick();  // Reset the timer

                // Go back to normal mode
                context.state = WIDGET_BASIC;
            }

        } break;

        case WIDGET_ERROR: {
            // unused currently but here if needed
        } break;
    }
}
