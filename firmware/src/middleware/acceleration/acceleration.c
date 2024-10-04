#include "acceleration.h"

#include "lis3dh_driver.h"
#include "system_communication.h"
#include "uart_logger.h"

extern struct driver_comm_message_passing acceleration_comm;

static struct lis3dh_context lis3dh_context = {.i2c_context = &i2c1_context,
                                               .comm = &acceleration_comm};

// Function to save the context data to the communication device for
// acceleration data
static void acceleration_update_data(struct driver_comm_message_passing *comm,
                                     struct lis3dh_context *context) {
    comm->data.acceleration.x = context->x_acc;
    comm->data.acceleration.y = context->y_acc;
    comm->data.acceleration.z = context->z_acc;
}

void acceleration_setup(void) {
    static struct lis3dh_config lis3dh_config = {
        .reg1 =
            {// Enable the x, y, and z axis for recording acceleration data
             .xen = 1,
             .yen = 1,
             .zen = 1,
             // No low power mode because we lose accuracy
             .lpen = 0,
             // Set the chip to process at 400 hz for measuring acceleration
             // data
             .odr = LIS3DH_400_HZ},

        .reg2 =
            {// No high pass filter on data going to int1, int2 or the click
             // interrupt
             .hp_ia1 = 0,
             .hp_ia2 = 0,
             .hpclick = 0,
             // Do not filter data going to the output registers
             .fds = 0,
             // Filter a 'low' amount of drift in the signal
             .hpcf = LIS3DH_HIGH_PASS_FILTER_LOW,
             // We want normal reset mode. Reset meaning that reading the
             // REFERENCE register will reset the high pass filter value
             .hpm = LIS3DH_HIGH_PASS_MODE_NORMAL_RESET},

        .reg3 =
            {// Enable interrupt 1 to be 'attached' to the ia1 bit (as
             // opposed
             // to the ia2 bit for the second int_src register)
             .i1_overrun = 0,
             .i1_wtm = 0,
             .i1_321da = 0,
             .i1_zyxda = 0,
             .i1_ia2 = 0,
             .i1_ia1 = 1,  // Set it here
             .i1_click = 0},

        .reg4 =
            {// Do not mess with SPI mode
             .sim = 0,
             // No self test enabled (normal execution)
             .st = LIS3DH_SELF_TEST_NORMAL,
             // Select high resolution output mode
             .hr = 1,
             // 2G is the max range for our data. Yields higher accuracy for
             // low
             // acceleration environments
             .fs = LIS3DH_SCALE_2G,
             // Do not change endianness or blocking data updates
             .ble = 0,
             .bdu = 0},

        .reg5 =
            {// Latch reading the INT1_SRC register to clearning the
             // interrupt
             // flag
             .d4d_int2 = 0,
             .lir_int2 = 0,
             .d4d_int1 = 0,
             .lir_int1 = 1,  // Set it here
                             // Do not enable fifo or rebooting memory contents
             .fifo_en = 0,
             .boot = 0},

        .reg6 =
            {// We cannot use INT2 and will leave the polarity as is
             .int_polarity = 0,
             .i2_act = 0,
             .i2_boot = 0,
             .i2_ia2 = 0,
             .i2_ia1 = 0,
             .i2_click = 0},

        .int1 =
            {// Interrupt when x's or y's acceleration is higher than
             // threshold
             .xlow = 0,
             .xhigh = 1,  // Set here
             .ylow = 0,
             .yhigh = 1,  // Set here
             .zlow = 0,
             .zhigh = 0,
             // Trigger interrupt if x OR y happens. AND would be both have
             // to
             // happen
             .it_mode = LIS3DH_OR_MODE},
        // Set the threshold to .15 * g to trigger
        .acceleration_threshold = 0.15f,
        // Set the duration to be at least 20 miliseconds
        .acceleration_duration = 20.0f};

    // Now init the driver
    int ret = lis3dh_driver_init(&lis3dh_config, &lis3dh_context);
    if (ret < 0)
        lis3dh_context.state = LIS3DH_ERROR;
}

void acceleration_run(void) {
    struct i2c_request *request = &lis3dh_context.request;
    struct i2c_request *it_request = &lis3dh_context.it_request;
    struct driver_comm_message_passing *comm = lis3dh_context.comm;

    switch (lis3dh_context.it_state) {
        case LIS3DH_INTERRUPT_CLEAR: {
            /* Check if an interrupt needs to be processed */
            if (lis3dh_interrupt1_flag == 1) {
                lis3dh_context.it_state = LIS3DH_INTERRUPT_TRIGGERED;
                lis3dh_interrupt1_flag = 0;
            }

            /* State Machine Start */
            switch (lis3dh_context.state) {
                case LIS3DH_PRE_INIT: {
                    uart_logger_send("LIS3DH not initalized properly\r\n");
                } break;

                case LIS3DH_READY: {
                    switch (comm->request.type) {
                        case REQUEST_TYPE_NONE: {
                            // Nothing to do so leave
                        } break;

                        case REQUEST_TYPE_DATA: {
                            comm->request.status = REQUEST_STATUS_RECEIVED;
                            int ret = lis3dh_driver_request_acceleration(
                                &lis3dh_context);
                            if (ret < 0)
                                lis3dh_context.state = LIS3DH_ERROR;
                            else
                                lis3dh_context.state = LIS3DH_PENDING;

                        } break;

                        case REQUEST_TYPE_ENTER_LP: {
                            comm->request.status = REQUEST_STATUS_RECEIVED;
                            lis3dh_driver_enter_low_power(&lis3dh_context);
                            comm->request.status = REQUEST_STATUS_FINISHED;

                        } break;

                        case REQUEST_TYPE_EXIT_LP: {
                            comm->request.status = REQUEST_STATUS_RECEIVED;
                            lis3dh_driver_exit_low_power(&lis3dh_context);
                            comm->request.status = REQUEST_STATUS_FINISHED;
                        } break;
                    }
                } break;

                case LIS3DH_PENDING: {
                    switch (future_get_state(&request->future)) {
                        case FUTURE_WAITING: {
                            // Do nothing
                        } break;

                        case FUTURE_FINISHED: {
                            lis3dh_driver_process_acceleration(&lis3dh_context);
                            lis3dh_context.state = LIS3DH_READY;
                            // Update the communication values
                            comm->request.status = REQUEST_STATUS_FINISHED;
                            acceleration_update_data(comm, &lis3dh_context);
                        } break;

                        case FUTURE_ERROR: {
                            lis3dh_context.state = LIS3DH_ERROR;
                        } break;
                    }
                } break;

                case LIS3DH_ERROR: {
                    uart_logger_send("[ERROR] LIS3DH had an error\r\n");

                    // Prevent leaving error state by interrupt
                    lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
                } break;
            }
            /* State Machine End */
        } break;

        case LIS3DH_INTERRUPT_TRIGGERED: {
            int ret = lis3dh_driver_request_it_clear(&lis3dh_context);
            if (ret < 0) {
                lis3dh_context.state = LIS3DH_ERROR;
                lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
            } else
                lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEARING;
        } break;

        case LIS3DH_INTERRUPT_CLEARING: {
            switch (future_get_state(&it_request->future)) {
                case FUTURE_WAITING: {
                    // Do nothing
                } break;

                case FUTURE_FINISHED: {
                    lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
                    uart_logger_send(
                        "\r\nLIS3DH INTERRUPT FINISHED: 0x%x\r\n\r\n",
                        it_request->buffer[0]);
                } break;

                case FUTURE_ERROR: {
                    lis3dh_context.state = LIS3DH_ERROR;
                    lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
                } break;
            }

        } break;
    }
}
