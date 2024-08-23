#include "acceleration.h"

#include "futures.h"
#include "logging.h"
#include "lsm6dsm_driver.h"
#include "system_communication.h"
#include "utils.h"

extern struct driver_comm_message_passing acceleration_comm;

static void acceleration_update_data(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    acceleration_comm.data.acceleration.x = lsm6dsm_driver_get_x_acc(dev);
    acceleration_comm.data.acceleration.y = lsm6dsm_driver_get_y_acc(dev);
    acceleration_comm.data.acceleration.z = lsm6dsm_driver_get_z_acc(dev);
}

void acceleration_setup(void) {
    // Now init the driver
    int ret = lsm6dsm_driver_init(lsm6dsm);
    if (ret != 0) {
        lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_ERROR);
        LOG_ERR("Failed to initialize LSM6DSM");
    } else {
        lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_READY);
        LOG_DBG("LSM6DSM initialized succesfully");
    }
}

void acceleration_run(void) {
    switch (lsm6dsm_driver_get_it_state(lsm6dsm)) {
        case LSM6DSM_INTERRUPT_CLEAR: {
            /* Check if an interrupt needs to be processed */
            if (lsm6dsm_driver_get_int1(lsm6dsm)) {
                acceleration_comm.inactive_flag ^= 1;
                lsm6dsm_driver_clear_int1(lsm6dsm);
            }

            if (lsm6dsm_driver_get_int2(lsm6dsm)) {
                lsm6dsm_driver_set_it_state(lsm6dsm,
                                            LSM6DSM_INTERRUPT_TRIGGERED);
                lsm6dsm_driver_clear_int2(lsm6dsm);
            }

            /* State Machine Start */
            switch (lsm6dsm_driver_get_state(lsm6dsm)) {
                case LSM6DSM_PRE_INIT: {
                    LOG_ERR("LSM6DSM not initalized properly");
                } break;

                case LSM6DSM_READY: {
                    switch (acceleration_comm.request.type) {
                        case REQUEST_TYPE_NONE: {
                            // Nothing to do so leave
                        } break;

                        case REQUEST_TYPE_DATA: {
                            acceleration_comm.request.status =
                                REQUEST_STATUS_RECEIVED;
                            int ret =
                                lsm6dsm_driver_request_acceleration(lsm6dsm);
                            if (ret < 0) {
                                lsm6dsm_driver_set_state(lsm6dsm,
                                                         LSM6DSM_ERROR);
                            } else {
                                lsm6dsm_driver_set_state(lsm6dsm,
                                                         LSM6DSM_PENDING);
                            }
                        } break;

                        case REQUEST_TYPE_ENTER_LP: {
                            // LSM6DSM automatically enters low power mode
                            // acceleration_comm.request.status =
                            // REQUEST_STATUS_RECEIVED;
                            acceleration_comm.request.status =
                                REQUEST_STATUS_FINISHED;
                        } break;

                        case REQUEST_TYPE_EXIT_LP: {
                            // LSM6DSM automatically enters low power mode
                            // acceleration_comm.request.status =
                            // REQUEST_STATUS_RECEIVED;
                            acceleration_comm.request.status =
                                REQUEST_STATUS_FINISHED;
                        } break;
                    }
                } break;

                case LSM6DSM_PENDING: {
                    switch (lsm6dsm_driver_get_acceleration_request_status(
                        lsm6dsm)) {
                        case FUTURE_WAITING: {
                            // Do nothing
                        } break;

                        case FUTURE_FINISHED: {
                            lsm6dsm_driver_process_acceleration(lsm6dsm);
                            lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_READY);

                            // Update the communication values
                            acceleration_comm.request.status =
                                REQUEST_STATUS_FINISHED;
                            acceleration_update_data(lsm6dsm);
                        } break;

                        case FUTURE_ERROR: {
                            lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_ERROR);
                        } break;
                    }
                } break;

                case LSM6DSM_ERROR: {
                    LOG_ERR("LSM6DSM had an error");

                    // Prevent leaving error state by interrupt
                    lsm6dsm_driver_set_it_state(lsm6dsm,
                                                LSM6DSM_INTERRUPT_CLEAR);
                } break;
            }
            /* State Machine End */
        } break;

        case LSM6DSM_INTERRUPT_TRIGGERED: {
            int ret = lsm6dsm_driver_request_tilt_it_source(lsm6dsm);
            if (ret != 0) {
                lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_ERROR);
                lsm6dsm_driver_set_it_state(lsm6dsm, LSM6DSM_INTERRUPT_CLEAR);
            } else {
                ret = lsm6dsm_driver_request_tap_it_source(lsm6dsm);
                if (ret != 0) {
                    lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_ERROR);
                    lsm6dsm_driver_set_it_state(lsm6dsm,
                                                LSM6DSM_INTERRUPT_CLEAR);
                } else {
                    lsm6dsm_driver_set_it_state(lsm6dsm,
                                                LSM6DSM_INTERRUPT_GET_SOURCE);
                }
            }
        } break;
        case LSM6DSM_INTERRUPT_GET_SOURCE: {
            switch (lsm6dsm_driver_get_tilt_it_source_request_status(lsm6dsm)) {
                case FUTURE_WAITING: {
                    // Do nothing
                } break;

                case FUTURE_FINISHED: {
                    switch (lsm6dsm_driver_get_tap_it_source_request_status(
                        lsm6dsm)) {
                        case FUTURE_WAITING: {
                            // Do nothing
                        } break;

                        case FUTURE_FINISHED: {
                            lsm6dsm_driver_process_tilt_it_source(lsm6dsm);
                            lsm6dsm_driver_process_tap_it_source(lsm6dsm);
                            lsm6dsm_driver_set_it_state(
                                lsm6dsm, LSM6DSM_INTERRUPT_CLEAR);
                            LOG_DBG("LSM6DSM INTERRUPT FINISHED");
                        } break;

                        case FUTURE_ERROR: {
                            lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_ERROR);
                            lsm6dsm_driver_set_it_state(
                                lsm6dsm, LSM6DSM_INTERRUPT_CLEAR);
                        } break;
                    }
                } break;

                case FUTURE_ERROR: {
                    lsm6dsm_driver_set_state(lsm6dsm, LSM6DSM_ERROR);
                    lsm6dsm_driver_set_it_state(lsm6dsm,
                                                LSM6DSM_INTERRUPT_CLEAR);
                } break;
            }
        } break;
        default:
            LOG_ERR("In default");
            break;
    }
}
