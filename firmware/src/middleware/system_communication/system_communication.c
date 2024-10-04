#include "system_communication.h"

#include <stdbool.h>

#include "i2c_driver.h"
#include "uart_logger.h"

bool request_is_finished(struct driver_comm_message_passing *comm) {
    return comm->request.status == REQUEST_STATUS_FINISHED;
}

bool request_is_received(struct driver_comm_message_passing *comm) {
    return comm->request.status == REQUEST_STATUS_RECEIVED;
}

bool request_is_not_seen(struct driver_comm_message_passing *comm) {
    return comm->request.status == REQUEST_STATUS_UNSEEN;
}

bool request_is_no_request(struct driver_comm_message_passing *comm) {
    return comm->request.type == REQUEST_TYPE_NONE;
}

bool request_is_time_ready(struct driver_comm_message_passing *comm) {
    return (HAL_GetTick() - comm->timing.last_time) > comm->timing.delay_ms;
}

/* Used to see if a driver has nothing to process */
bool request_is_not_busy(struct driver_comm_message_passing *comm) {
    return request_is_finished(comm) || request_is_no_request(comm);
}

// This struct is global to the system as several sensors rely on it
struct i2c_driver_context i2c1_context = {0};

void system_communication_setup(void) {
    // Set up the uart for communication with the computer
    uart_logger_init(USART2, 38400U);

    // Initalize the i2c device so it can send and receive
    i2c_driver_init(&i2c1_context, I2C1);
}

void system_communication_run(void) {
    i2c_queue_process_one(&i2c1_context);
}
