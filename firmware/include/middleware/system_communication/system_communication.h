#ifndef __SYSTEM_COMMUNICATION_H__
#define __SYSTEM_COMMUNICATION_H__

#include <stdbool.h>

#include "entity.h"
#include "led_matrix.h"
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

/* Use when communication is in the form of sending requests */
struct driver_comm_message_passing {
    // Details about the request itself
    struct {
        enum request_type type;
        enum request_status status;
    } request;

    // The data for a request (if applicable)
    union {
        struct {
            float x, y, z;
        } acceleration;

        struct {
            uint16_t proximity, als;
        } ambient_light;

        struct {
            float temperature;
        } temperature;
    } data;

    // Details about the spacing of requests. Used by widget controller
    struct {
        uint32_t delay_ms;   // How many miliseconds between data requests
        uint32_t last_time;  // Base time to count from
    } timing;
};

/* Use when communication is in the form of sharing data */
struct driver_comm_shared_memory {
    union {
        struct {
            /*
             * The 'row', 'col', 'sub_frame_cntr' and 'duration_cntr' variables
             * are updated by the appropriate function, and do not need to be
             * set each time new data is supplied.
             * But in the case of doing a pipeline flush, resettting these
             * values is needed so they all start at the beginning.
             *
             * The 'finished' variables are updated by the appropriate functions
             * when they have completed the task. The widget controller should
             * unset these once they process the flag
             *
             * All other variables are used by the widget_controller to specify
             * what the appropriate function should be doing
             */
            struct {
                bool active;                    // Should the loader be on
                bool finished;                  // Is the loader finished
                enum animation_map input_anim;  // Which animation to read from
                uint32_t input_frame;           // Which frame to read from
                uint32_t output_slot;           // Which slot to write to
                uint32_t row;                   // Which row to process
                uint32_t col;                   // Which column to process
            } loader;

            struct {
                bool active;              // Should the renderer be on
                bool finished;            // Is the renderer finished
                struct entity *entities;  // Array of entities to draw
                uint32_t num_entities;    // How many sprites are in the array
                uint32_t output_slot;     // Which slot to write to
                uint32_t row;             // Which row to process
                uint32_t col;             // Which column to process
            } renderer;

            struct {
                bool active;           // Should the assembler be on
                bool finished;         // Is the assembler finished
                uint32_t input_slot;   // Which slot to read from
                uint32_t output_slot;  // which slot to write to
                uint32_t row;          // Which row to process
                uint32_t col;          // Which column to process
            } assembler;

            struct {
                bool active;              // Should the drawer be on
                bool finished;            // Is the drawer finished
                uint32_t input_slot;      // Which slot to read from
                uint32_t num_draws;       // How many times to draw
                uint32_t sub_frame_cntr;  // Tracks which subframe to draw
                uint32_t duration_cntr;   // Tracks how many subframes are drawn
            } drawer;

        } led_matrix;

    } data;
};

// Regarding the status
bool request_is_finished(struct driver_comm_message_passing *device);
bool request_is_received(struct driver_comm_message_passing *device);
bool request_is_not_seen(struct driver_comm_message_passing *device);

// Regarding the requests
bool request_is_no_request(struct driver_comm_message_passing *device);
bool request_is_time_ready(struct driver_comm_message_passing *device);
bool request_is_not_busy(struct driver_comm_message_passing *device);

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
