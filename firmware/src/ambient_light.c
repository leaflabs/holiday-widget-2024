#include "ambient_light.h"

#include "system_communication.h"
#include "uart_logger.h"
#include "vcnl4020_driver.h"

extern struct driver_comm_message_passing ambient_light_comm;

static struct vcnl4020_context vcnl4020_context = {.i2c_context = &i2c1_context,
                                                   .comm = &ambient_light_comm};

static void ambient_light_update_data(struct driver_comm_message_passing *comm,
                                      struct vcnl4020_context *context) {
    comm->data.ambient_light.proximity = context->proximity_cnt;
    comm->data.ambient_light.als = context->als_lux;
}

void ambient_light_setup(void) {
    const struct vcnl4020_config vcnl4020_config = {
        .command_register =
            {
                .prox_en = 1,  // Enable proximity
                .als_en = 1    // Enable als
            },

        .proximity_rate_register =
            {
                .proximity_rate =
                    VCNL4020_PR_125_00  // 125.00 measurements per second
            },

        .ir_led_current_register =
            {
                .current_value = VCNL4020_200_MA  // 200 ma of current to
                                                  // the sensor. Yields
                // more tests so more accurate results
            },

        .ambient_light_parameter_register =
            {
                .averaging_func =
                    VCNL4020_32_CONV,  // 32 conversions -> average of 32
                                       // light sensor readings. Less
                                       // accurate but faster
                .offset_enable = 1,    // Enable offset so any drift is
                                       // removed from light sensor readings
                .als_rate = VCNL4020_AR_10,  // 5 samples per second
            },

        .interrupt_control_register =
            {
                .thresh_prox_als =
                    VCNL4020_THRESH_ALS,  // Enable threshold triggers on
                                          // the als sensor
                .thresh_enable =
                    1,  // Allow thresholds to trigger the interrupt pin
                .interrupt_count = VCNL4020_TC_1  // Must be over or under the
                                                  // threshold at least 2 times
            },

        .interrupt_thresholds = {
            // If VCNL4020_THRESH_ALS enabled:
            .low = 200,    // Below 200 lux
            .high = 10000  // Above 10000 lux

            // If VCNL4020_THRESH_PROX enabled:
            // .low = 2000, // Below 2000 cnts
            // .high = 5000 // Above 5000 cnts
        }};

    int ret = vcnl4020_driver_init(&vcnl4020_config, &vcnl4020_context);
    if (ret < 0) vcnl4020_context.state = VCNL4020_ERROR;
}

void ambient_light_run(void) {
    struct i2c_request *request = &vcnl4020_context.request;
    struct i2c_request *it_request = &vcnl4020_context.it_request;
    struct driver_comm_message_passing *comm = vcnl4020_context.comm;

    // First check if we have an interrupt to process
    switch (vcnl4020_context.it_state) {
        case VCNL4020_INTERRUPT_CLEAR: {
            /* Check if an interrupt needs to be processed */
            if (vcnl4020_interrupt_flag == 1) {
                vcnl4020_context.it_state = VCNL4020_INTERRUPT_TRIGGERED;
                vcnl4020_interrupt_flag = 0;  // Unset for next trigger
            }

            /* State Machine Start */
            switch (vcnl4020_context.state) {
                case VCNL4020_PRE_INIT: {
                    uart_logger_send("VCNL4020 not initalized properly\r\n");
                } break;

                case VCNL4020_READY: {
                    switch (comm->request.type) {
                        case REQUEST_TYPE_NONE: {
                            // Do nothing
                        } break;

                        case REQUEST_TYPE_DATA: {
                            int ret = vcnl4020_driver_request_als_prox(
                                &vcnl4020_context);
                            if (ret < 0)
                                vcnl4020_context.state = VCNL4020_ERROR;
                            else
                                vcnl4020_context.state = VCNL4020_PENDING;

                        } break;

                        case REQUEST_TYPE_ENTER_LP: {
                            comm->request.status = REQUEST_STATUS_RECEIVED;
                            vcnl4020_driver_enter_low_power(&vcnl4020_context);
                            comm->request.status = REQUEST_STATUS_FINISHED;

                        } break;

                        case REQUEST_TYPE_EXIT_LP: {
                            comm->request.status = REQUEST_STATUS_RECEIVED;
                            vcnl4020_driver_exit_low_power(&vcnl4020_context);
                            comm->request.status = REQUEST_STATUS_FINISHED;
                        } break;
                    }
                } break;

                case VCNL4020_PENDING: {
                    switch (future_get_state(&request->future)) {
                        case FUTURE_WAITING: {
                            // Do nothing
                        } break;

                        case FUTURE_FINISHED: {
                            vcnl4020_driver_process_als_prox(&vcnl4020_context);
                            vcnl4020_context.state = VCNL4020_READY;
                            // Update communication values
                            comm->request.status = REQUEST_STATUS_FINISHED;
                            ambient_light_update_data(comm, &vcnl4020_context);
                        } break;

                        case FUTURE_ERROR: {
                            vcnl4020_context.state = VCNL4020_ERROR;
                        } break;
                    }
                } break;

                case VCNL4020_ERROR: {
                    uart_logger_send("[ERROR] VCNL4020 had an error\r\n");

                    // Prevent leaving error state by an interrupt
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                } break;
            }
            /* State Machine End */
        } break;

        case VCNL4020_INTERRUPT_TRIGGERED: {
            int ret = vcnl4020_driver_request_it_clear_read(&vcnl4020_context);
            if (ret < 0) {
                vcnl4020_context.state = VCNL4020_ERROR;
                vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
            } else
                vcnl4020_context.it_state = VCNL4020_INTERRUPT_READING;
        } break;

        case VCNL4020_INTERRUPT_READING: {
            switch (future_get_state(&it_request->future)) {
                case FUTURE_WAITING: {
                    // No action
                } break;

                case FUTURE_FINISHED: {
                    int ret = vcnl4020_driver_request_it_clear_write(
                        &vcnl4020_context);
                    if (ret < 0) {
                        vcnl4020_context.state = VCNL4020_ERROR;
                        vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                    } else
                        vcnl4020_context.it_state = VCNL4020_INTERRUPT_WRITING;
                } break;

                case FUTURE_ERROR: {
                    vcnl4020_context.state = VCNL4020_ERROR;
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                } break;
            }
        } break;

        case VCNL4020_INTERRUPT_WRITING: {
            switch (future_get_state(&it_request->future)) {
                case FUTURE_WAITING: {
                    // No action
                } break;

                case FUTURE_FINISHED: {
                    uart_logger_send("\r\nVCNL4020 INTERRUPT FINISHED\r\n\r\n");
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                } break;

                case FUTURE_ERROR: {
                    vcnl4020_context.state = VCNL4020_ERROR;
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                } break;
            }
        } break;
    }
}
