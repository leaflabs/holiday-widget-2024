#include "system_communication.h"

#include "i2c_driver.h"
#include "uart_logger.h"

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
