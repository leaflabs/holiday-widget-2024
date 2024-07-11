#include "temperature.h"

#include "system_communication.h"
#include "tmp102_driver.h"
#include "uart_logger.h"

static struct tmp102_context tmp102_context = {.i2c_context = &i2c1_context};

void temperature_setup(void) {
    static struct tmp102_config tmp102_config = {
        .conversion_rate =
            TMP102_8_0_HZ  // Update temperature 8 times per second
    };

    int ret = tmp102_driver_init(&tmp102_config, &tmp102_context);
    if (ret < 0) tmp102_context.state = TMP102_ERROR;
}

void temperature_run(void) {
    struct i2c_request *request = &tmp102_context.request;

    switch (tmp102_context.state) {
        case TMP102_PRE_INIT: {
            uart_logger_send("TMP102 not initalized properly\r\n");
        } break;

        case TMP102_READY: {
            uart_logger_send("Temperature: %f [C] %f [F]\r\n",
                             tmp102_context.temperature,
                             (tmp102_context.temperature * 9.0f / 5.0f) + 32);

            int ret = tmp102_driver_request_temperature(&tmp102_context);
            if (ret < 0)
                tmp102_context.state = TMP102_ERROR;
            else
                tmp102_context.state = TMP102_PENDING;
        } break;

        case TMP102_PENDING: {
            switch (future_get_state(&request->future)) {
                case FUTURE_WAITING: {
                    // Do nothing
                } break;

                case FUTURE_FINISHED: {
                    tmp102_driver_process_temperature(&tmp102_context);
                    tmp102_context.state = TMP102_READY;
                } break;

                case FUTURE_ERROR: {
                    tmp102_context.state = TMP102_ERROR;
                } break;
            }
        } break;

        case TMP102_ERROR: {
            uart_logger_send("[ERROR] TMP102 had an error\r\n");
            // Stay in this state
        } break;
    }
}
