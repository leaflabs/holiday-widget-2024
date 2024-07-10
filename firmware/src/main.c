#include "main.h"

#include "charlieplex_driver.h"
#include "i2c_driver.h"
#include "job_queue.h"
#include "lis3dh_driver.h"
#include "stm32l0xx_hal.h"
#include "tmp102_driver.h"
#include "uart_logger.h"
#include "utils.h"
#include "vcnl4020_driver.h"

// How many addresses we can store from an i2c_driver_scan call
#define MAX_ADDRESSES 10U

// Masks to get low and high thresholds from the status register for vcnl4020
// interrupt
#define LOW_THRESH_MSK 0x2
#define HIGH_THRESH_MSK 0x1

// Context structs for our drivers.
struct i2c_driver_context i2c1_context = {0};
static struct tmp102_context tmp102_context = {.i2c_context = &i2c1_context};
static struct lis3dh_context lis3dh_context = {.i2c_context = &i2c1_context};
static struct vcnl4020_context vcnl4020_context = {.i2c_context =
                                                       &i2c1_context};
static struct charlieplex_context charlieplex_context = {0};

/*
 * Basic system clock initalization code
 *
 */
void system_clock_init(void) {
    RCC_OscInitTypeDef rcc_osc_init = {0};
    RCC_ClkInitTypeDef rcc_clk_init = {0};

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    /* Enable MSI Oscillator */
    rcc_osc_init.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    rcc_osc_init.MSIState = RCC_MSI_ON;
    rcc_osc_init.MSICalibrationValue = 0;
    rcc_osc_init.MSIClockRange = RCC_MSIRANGE_5;
    rcc_osc_init.PLL.PLLState = RCC_PLL_NONE;

    if (HAL_RCC_OscConfig(&rcc_osc_init) != HAL_OK) {
        // error message here
    }

    /*
     * Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     * clocks' dividers.
     */
    rcc_clk_init.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                              RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);

    rcc_clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
    rcc_clk_init.AHBCLKDivider = RCC_SYSCLK_DIV2;
    rcc_clk_init.APB1CLKDivider = RCC_HCLK_DIV1;
    rcc_clk_init.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&rcc_clk_init, FLASH_LATENCY_0) != HAL_OK) {
        // Error message here
    }
}

/*
 * Custom delay made to test clock speeds
 *
 */
void delay(volatile uint32_t time) {
    for (volatile uint32_t i = 0; i < time; i++) {
        // Just do some wasteful calculations
        for (volatile uint32_t j = 0; j < 50; j++) {
            __asm volatile("nop");
            __asm volatile("nop");
        }
    }
}

/*
 * Initalize everything for the GPIO pins
 *
 */
void GPIO_Init(void) {
    // Enable the GPIOC clock
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};

    // Set up LED Pins (8-11);
    gpio.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(GPIOC, &gpio);
}

/**** SETUP FUNCTIONS ****/

/*
    Set up the HAL, clock and all peripherals

    'i2c' is the handle for the i2c_driver_context
*/
static void system_init() {
    // Init the HAL
    HAL_Init();

    // Init the system clock
    system_clock_init();

    // Set up the GPIO pins for the leds
    GPIO_Init();

    // Set up the uart for communication with the computer
    uart_logger_init(USART2, 38400U);

    // Initalize the i2c device so it can send and receive
    i2c_driver_init(&i2c1_context, I2C1);
}

void print_available_i2c_devices(void) {
    uint8_t online_count = 0;
    uint64_t high_mask = 0;
    uint64_t low_mask = 0;

    for (uint8_t i = 0; i < 128; i++) {
        if (i2c_device_is_ready(&i2c1_context, i)) {
            online_count++;
            if (i < 64) {
                low_mask |= 1 << i;
            } else {
                high_mask |= 1 << (i - 64);
            }
        }
    }

    uart_logger_send("Scanned I2C addresses: %d\r\n", online_count);

    for (uint8_t i = 0; i < 128; i++) {
        if ((i < 64 && low_mask & (1 << i)) ||
            (i < 128 && high_mask & (1 << (i - 64)))) {
            uart_logger_send("     Address found: 0x%02x\r\n", i);
        }
    }
    uart_logger_send("End of scan\r\n\r\n");
}

void tmp102_setup(void) {
    static struct tmp102_config tmp102_config = {
        .conversion_rate =
            TMP102_8_0_HZ  // Update temperature 8 times per second
    };

    int ret = tmp102_driver_init(&tmp102_config, &tmp102_context);
    if (ret < 0) tmp102_context.state = TMP102_ERROR;
}

void lis3dh_setup(void) {
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
    if (ret < 0) lis3dh_context.state = LIS3DH_ERROR;
}

void vcnl4020_setup(void) {
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

/**** RUN FUNCTIONS ****/
void vcnl4020_run(void) {
    struct i2c_request *request = &vcnl4020_context.request;
    struct i2c_request *it_request = &vcnl4020_context.it_request;

    // First check if we have an interrupt to process
    switch (vcnl4020_context.it_state) {
        case VCNL4020_INTERRUPT_CLEAR:
            /* Check if an interrupt needs to be processed */
            if (vcnl4020_interrupt_flag == 1) {
                vcnl4020_context.it_state = VCNL4020_INTERRUPT_TRIGGERED;
                vcnl4020_interrupt_flag = 0;  // Unset for next trigger
            }

            /* State Machine Start */
            switch (vcnl4020_context.state) {
                case VCNL4020_PRE_INIT:
                    uart_logger_send("VCNL4020 not initalized properly\r\n");
                    break;

                case VCNL4020_READY:
                    uart_logger_send("Prox: %d [cnt], ALS: %d [lux]\r\n",
                                     vcnl4020_context.proximity_cnt,
                                     vcnl4020_context.als_lux);

                    int ret =
                        vcnl4020_driver_request_als_prox(&vcnl4020_context);
                    if (ret < 0)
                        vcnl4020_context.state = VCNL4020_ERROR;
                    else
                        vcnl4020_context.state = VCNL4020_PENDING;
                    break;

                case VCNL4020_PENDING:
                    switch (future_get_state(&request->future)) {
                        case FUTURE_WAITING:
                            // Do nothing
                            break;

                        case FUTURE_FINISHED:
                            vcnl4020_driver_process_als_prox(&vcnl4020_context);
                            vcnl4020_context.state = VCNL4020_READY;
                            break;

                        case FUTURE_ERROR:
                            vcnl4020_context.state = VCNL4020_ERROR;
                    }
                    break;

                case VCNL4020_ERROR:
                    uart_logger_send("[ERROR] VCNL4020 had an error\r\n");

                    // Prevent leaving error state by an interrupt
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                    break;
            }
            /* State Machine End */
            break;

        case VCNL4020_INTERRUPT_TRIGGERED:
            int ret = vcnl4020_driver_request_it_clear_read(&vcnl4020_context);
            if (ret < 0) {
                vcnl4020_context.state = VCNL4020_ERROR;
                vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
            } else
                vcnl4020_context.it_state = VCNL4020_INTERRUPT_READING;
            break;

        case VCNL4020_INTERRUPT_READING:
            switch (future_get_state(&it_request->future)) {
                case FUTURE_WAITING:
                    // No action
                    break;

                case FUTURE_FINISHED:
                    int ret = vcnl4020_driver_request_it_clear_write(
                        &vcnl4020_context);
                    if (ret < 0) {
                        vcnl4020_context.state = VCNL4020_ERROR;
                        vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                    } else
                        vcnl4020_context.it_state = VCNL4020_INTERRUPT_WRITING;
                    break;

                case FUTURE_ERROR:
                    vcnl4020_context.state = VCNL4020_ERROR;
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                    break;
            }
            break;

        case VCNL4020_INTERRUPT_WRITING:
            switch (future_get_state(&it_request->future)) {
                case FUTURE_WAITING:
                    // No action
                    break;

                case FUTURE_FINISHED:
                    uart_logger_send("\r\nVCNL4020 INTERRUPT FINISHED\r\n\r\n");
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                    break;

                case FUTURE_ERROR:
                    vcnl4020_context.state = VCNL4020_ERROR;
                    vcnl4020_context.it_state = VCNL4020_INTERRUPT_CLEAR;
                    break;
            }
            break;
    }
}

void lis3dh_run(void) {
    struct i2c_request *request = &lis3dh_context.request;
    struct i2c_request *it_request = &lis3dh_context.it_request;

    switch (lis3dh_context.it_state) {
        case LIS3DH_INTERRUPT_CLEAR:
            /* Check if an interrupt needs to be processed */
            if (lis3dh_interrupt1_flag == 1) {
                lis3dh_context.it_state = LIS3DH_INTERRUPT_TRIGGERED;
                lis3dh_interrupt1_flag = 0;
            }

            /* State Machine Start */
            switch (lis3dh_context.state) {
                case LIS3DH_PRE_INIT:
                    uart_logger_send("LIS3DH not initalized properly\r\n");
                    break;

                case LIS3DH_READY:
                    uart_logger_send("Acceleration: %f %f %f [g]\r\n",
                                     lis3dh_context.x_acc, lis3dh_context.y_acc,
                                     lis3dh_context.z_acc);

                    int ret =
                        lis3dh_driver_request_acceleration(&lis3dh_context);
                    if (ret < 0)
                        lis3dh_context.state = LIS3DH_ERROR;
                    else
                        lis3dh_context.state = LIS3DH_PENDING;
                    break;

                case LIS3DH_PENDING:
                    switch (future_get_state(&request->future)) {
                        case FUTURE_WAITING:
                            // Do nothing
                            break;

                        case FUTURE_FINISHED:
                            lis3dh_driver_process_acceleration(&lis3dh_context);
                            lis3dh_context.state = LIS3DH_READY;
                            break;

                        case FUTURE_ERROR:
                            lis3dh_context.state = LIS3DH_ERROR;
                    }
                    break;

                case LIS3DH_ERROR:
                    uart_logger_send("[ERROR] LIS3DH had an error\r\n");

                    // Prevent leaving error state by interrupt
                    lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
                    break;
            }
            /* State Machine End */
            break;

        case LIS3DH_INTERRUPT_TRIGGERED:
            int ret = lis3dh_driver_request_it_clear(&lis3dh_context);
            if (ret < 0) {
                lis3dh_context.state = LIS3DH_ERROR;
                lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
            } else
                lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEARING;
            break;

        case LIS3DH_INTERRUPT_CLEARING:
            switch (future_get_state(&it_request->future)) {
                case FUTURE_WAITING:
                    // Do nothing
                    break;

                case FUTURE_FINISHED:
                    lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
                    uart_logger_send(
                        "\r\nLIS3DH INTERRUPT FINISHED: 0x%x\r\n\r\n",
                        it_request->buffer[0]);
                    break;

                case FUTURE_ERROR:
                    lis3dh_context.state = LIS3DH_ERROR;
                    lis3dh_context.it_state = LIS3DH_INTERRUPT_CLEAR;
                    break;
            }

            break;
    }
}

void tmp102_run(void) {
    struct i2c_request *request = &tmp102_context.request;

    switch (tmp102_context.state) {
        case TMP102_PRE_INIT:
            uart_logger_send("TMP102 not initalized properly\r\n");
            break;

        case TMP102_READY:
            uart_logger_send("Temperature: %f [C] %f [F]\r\n",
                             tmp102_context.temperature,
                             (tmp102_context.temperature * 9.0f / 5.0f) + 32);

            int ret = tmp102_driver_request_temperature(&tmp102_context);
            if (ret < 0)
                tmp102_context.state = TMP102_ERROR;
            else
                tmp102_context.state = TMP102_PENDING;
            break;

        case TMP102_PENDING:
            switch (future_get_state(&request->future)) {
                case FUTURE_WAITING:
                    // Do nothing
                    break;

                case FUTURE_FINISHED:
                    tmp102_driver_process_temperature(&tmp102_context);
                    tmp102_context.state = TMP102_READY;
                    break;

                case FUTURE_ERROR:
                    tmp102_context.state = TMP102_ERROR;
                    break;
            }
            break;

        case TMP102_ERROR:
            uart_logger_send("[ERROR] TMP102 had an error\r\n");
            // Stay in this state
            break;
    }
}

void i2c_run(void) {
    i2c_queue_process_one(&i2c1_context);
}

int main(void) {
    job_add(&system_init, JOB_INIT);
    job_add(&print_available_i2c_devices, JOB_INIT);
    job_add(&tmp102_setup, JOB_INIT);
    job_add(&lis3dh_setup, JOB_INIT);
    job_add(&vcnl4020_setup, JOB_INIT);
    // job_add(&charlieplex_driver_init, JOB_INIT);

    // Global function to process all i2c input
    job_add(&i2c_run, JOB_RUN_RUN);
    job_add(&tmp102_run, JOB_RUN_RUN);
    job_add(&vcnl4020_run, JOB_RUN_RUN);
    job_add(&lis3dh_run, JOB_RUN_RUN);

    while (1) {
        job_state_machine_run();
    }
}
