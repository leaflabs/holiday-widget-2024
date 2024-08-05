#ifndef __SYSTEM_COMMUNICATION_H__
#define __SYSTEM_COMMUNICATION_H__

#include <stdbool.h>

#include "stm32l0xx_hal.h"

/* Used for the type of the current request*/
enum request_type {
    REQUEST_TYPE_NONE,      // No request
    REQUEST_TYPE_DATA,      // Request data
    REQUEST_TYPE_ENTER_LP,  // Request enter low power mode
    REQUEST_TYPE_EXIT_LP    // Request exit low power mode
};

/* Used for the status of the current request */
enum request_status {
    REQUEST_STATUS_UNSEEN,    // Set when the Sender makes a new request
    REQUEST_STATUS_RECEIVED,  // Set when the Receiver starts the request
    REQUEST_STATUS_FINISHED   // Set when the Receiver finishes the request
};

/* Used as a container for any type of request data */
union request_data {
    struct {
        float x, y, z;
    } acceleration;

    struct {
        uint16_t proximity;
        uint16_t als;
    } ambient_light;

    struct {
        float temperature;
    } temperature;
};

/* Gives communication between the high level drivers and widget controller */
struct driver_comm {
    // Details about the request itself
    struct {
        enum request_type type;
        enum request_status status;
    } request;

    // Details about the data from the request (if applicable)
    struct {
        enum { DATA_ACC, DATA_ALS, DATA_TEMP } type;
        union request_data data;
    } results;

    // Details about the spacing of requests. Used by widget controller
    struct {
        uint32_t delay_ms;   // How many miliseconds between data requests
        uint32_t last_time;  // Base time to count from
    } timing;
};

// Regarding the status
bool request_is_finished(struct driver_comm *device);
bool request_is_received(struct driver_comm *device);
bool request_is_not_seen(struct driver_comm *device);

// Regarding the requests
bool request_is_no_request(struct driver_comm *device);
bool request_is_time_ready(struct driver_comm *device);
bool request_is_not_busy(struct driver_comm *device);

// Allow for global access of the i2c1 device
extern struct i2c_driver_context i2c1_context;

/*
    Set up UART and I2C communication for widget to computer and
    stm32 chip to sensors
*/
void system_communication_setup(void);

/*
    Check if there are requests on the i2c queue and process them
*/
void system_communication_run(void);

#endif /* __SYSTEM_COMMUNICATION_H__ */
