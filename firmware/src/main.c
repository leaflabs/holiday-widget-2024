#include "main.h"

#include "i2c_driver.h"
#include "lis3dh_driver.h"
#include "stm32l0xx_hal.h"
#include "tmp102_driver.h"
#include "uart_logger.h"
#include "vcnl4020_driver.h"

// How many addresses we can store from an i2c_driver_scan call
#define MAX_ADDRESSES 10U

// Masks to get low and high thresholds from the status register for vcnl4020
// interrupt
#define LOW_THRESH_MSK 0x2
#define HIGH_THRESH_MSK 0x1
/*
    i2c_scan_arg is the struct passed into the i2c_scan_handler
    to save the addresses found
*/
struct i2c_scan_arg {
    uint8_t addrs[MAX_ADDRESSES];  // Upto MAX_ADDRS number of addresses
    size_t index;                  // Keep track of which element we are on
};

/*
    lis3dh_interrupt_arg is the struct passed into the lis3dh_interrupt_handler
   for whatever it may be used for.
*/
struct lis3dh_interrupt_arg {
    uint32_t
        numInterrupts;  // Keep track of how many interrupts have been triggered
};

/*
    vcnl4020_interrupt_arg is the struct passed into the
   vcnl4020_interrupt_handler for whatever it may be used for.
*/
struct vcnl4020_interrupt_arg {
    // Keep track of how many low and high threshholds were hit
    uint32_t num_low_thresh;
    uint32_t num_high_thresh;
};

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

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
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
void GPIO_Init() {
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

/*
    Set up the HAL, clock and all peripherals

    'i2c' is the handle for the I2C driver
*/
static void system_init(I2C_HandleTypeDef *i2c) {
    // Init the HAL
    HAL_Init();

    // Init the system clock
    system_clock_init();

    // Set up the GPIO pins for the leds
    GPIO_Init();

    // Set up the uart for communication with the computer
    uart_logger_init();

    // Initalize the i2c device so it can send and receive
    i2c_driver_init(i2c, I2C1);
}

/*
    User defined handler for the i2c_driver_scan function.
    Takes in an i2c address and a void* for a struct or regular type

    The handler can do what it wants with the address and the type passed in
*/
void i2c_scan_handler(uint8_t address, void *scan_arg) {
    // Convert the argument to the type of choice
    struct i2c_scan_arg *arg = scan_arg;

    // If we have space, save the address
    if (arg->index < MAX_ADDRESSES) {
        // Save the address in an array
        arg->addrs[arg->index++] = address;
    }
}

/*
    User defined handler for the lis3dh_interrupt1_clear function.
    Takes the status register value and a void* for a struct or type
    to be passed in.

    The handler can do what it wants based on the status
*/
void lis3dh_interrupt_handler(uint8_t status, void *interrupt_arg) {
    // Convert the argument to the type of choice
    struct lis3dh_interrupt_arg *arg = interrupt_arg;

    arg->numInterrupts++;  // Add an extra interrupt occurence

    uart_logger_send(
        "Proessing interrupt from handler... Total interrupts: %u\r\n",
        arg->numInterrupts);
}

/*
    user defined handler for the vcnl4020_interrupt_clear function.
    Takes the status register vlaue and a void* for a struct or type
    to be passed in.

    The handler can do what it wants to based on the status
*/
void vcnl4020_interrupt_handler(uint8_t status, void *interrupt_arg) {
    // Convert the argument to the type of choice
    struct vcnl4020_interrupt_arg *arg = interrupt_arg;

    // Check for high threshold
    if ((status & HIGH_THRESH_MSK) != 0) {
        arg->num_high_thresh++;
    }
    // Check for low threshold
    if ((status & LOW_THRESH_MSK) != 0) {
        arg->num_low_thresh++;
    }

    uart_logger_send(
        "Processing interrupt from vcnl4020: Num low: %d, Num high: %d\r\n",
        arg->num_low_thresh, arg->num_high_thresh);
}

int main(void) {
    // I2c handler for all sensors that use it
    static I2C_HandleTypeDef i2c = {0};

    // Set up the system and peripherals
    system_init(&i2c);

    // Turn 8 and 11 on initially so we can have the leds flash alternating
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8 | GPIO_PIN_11, 1);

    // Scan all devices availbale over i2c
    //      Pass in the handler defined above and our struct
    //      to be passed into the handler
    struct i2c_scan_arg scan_argument = {0};  // Define our argument struct
    i2c_driver_scan(&i2c, i2c_scan_handler,
                    &scan_argument);  // Pass in handler and argument

    // Now print out the addresses found
    uart_logger_send("Scanned I2C addresses: %d\r\n", scan_argument.index);

    for (int i = 0; i < scan_argument.index; i++) {
        uart_logger_send("     Address found: 0x%02x\r\n",
                         scan_argument.addrs[i]);
    }
    uart_logger_send("End of scan\n\r\n\r");

    /*
        Set up the tmp102 sensor
    */
    static struct tmp102_config tmp102_config = {
        .conversion_rate =
            TMP102_1_0_HZ  // Update temperature 1 time per second
    };

    static struct tmp102_context tmp102_context = {.i2c = &i2c};

    tmp102_driver_init(&tmp102_config, &tmp102_context);

    /*
        Set up the LIS3DH sensor and all the structs for it
    */

    // Set up our configuration struct for the lis3dh accelerometer.
    // Each configuration register can be configured.
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
             // Yes filter data going to the output registers
             .fds = 1,
             // Filter a 'low' amount of drift in the signal
             .hpcf = LIS3DH_HIGH_PASS_FILTER_LOW,
             // We want normal reset mode. Reset meaning that reading the
             // REFERENCE register will reset the high pass filter value
             .hpm = LIS3DH_HIGH_PASS_MODE_NORMAL_RESET},

        .reg3 =
            {// Enable interrupt 1 to be 'attached' to the ia1 bit (as opposed
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
             // 2G is the max range for our data. Yields higher accuracy for low
             // acceleration environments
             .fs = LIS3DH_SCALE_2G,
             // Do not change endianness or blocking data updates
             .ble = 0,
             .bdu = 0},

        .reg5 =
            {// Latch reading the INT1_SRC register to clearning the interrupt
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
            {// Interrupt when x's or y's acceleration is higher than threshold
             .xlow = 0,
             .xhigh = 1,  // Set here
             .ylow = 0,
             .yhigh = 1,  // Set here
             .zlow = 0,
             .zhigh = 0,
             // Trigger interrupt if x OR y happens. AND would be both have to
             // happen
             .it_mode = LIS3DH_OR_MODE},
        // Set the threshold to .15 * g to trigger
        .acceleration_threshold = 0.15f,
        // Set the duration to be at least 20 miliseconds
        .acceleration_duration = 20.0f};

    // Create our context struct to store the results of function calls
    static struct lis3dh_context lis3dh_context = {.i2c = &i2c};

    // Create our interrupt handler argument
    static struct lis3dh_interrupt_arg interrupt_arg = {0};

    // Now init the driver
    lis3dh_driver_init(&lis3dh_config, &lis3dh_context);

    /*
        Now initalize the VCNL4020 sensor for proximity
    */
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
                .current_value =
                    VCNL4020_200_MA  // 200 ma of current to the sensor. Yields
                                     // more tests so more accurate results
            },

        .ambient_light_parameter_register =
            {
                .averaging_func =
                    VCNL4020_32_CONV,  // 32 conversions -> average of 32 light
                                       // sensor readings. Less accurate but
                                       // faster
                .offset_enable = 1,    // Enable offset so any drift is removed
                                       // from light sensor readings
                .als_rate = VCNL4020_AR_5  // 5 samples per second
            },

        .interrupt_control_register =
            {
                .thresh_prox_als =
                    VCNL4020_THRESH_ALS,  // Enable threshold triggers on the
                                          // als sensor
                .thresh_enable =
                    1,  // Allow thresholds to trigger the interrupt pin
                .interrupt_count = VCNL4020_TC_2  // Must be over or under the
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

    // Create our context struct
    struct vcnl4020_context vcnl4020_context = {.i2c = &i2c};

    // Declare the interrupt argument for the interrupt handler
    struct vcnl4020_interrupt_arg vcnl4020_interrupt_arg = {0};

    // Init the driver here
    vcnl4020_driver_init(&vcnl4020_config, &vcnl4020_context);

    while (1) {
        // Get the temperature from the tmp102 sensor
        tmp102_driver_read(&tmp102_context);

        // Print it out
        uart_logger_send("Temp is: %f C, %f F\r\n\r\n",
                         tmp102_context.temperature,
                         (9.0f / 5.0f * tmp102_context.temperature) + 32);

        // Get the current acceleration
        lis3dh_driver_read_all(&lis3dh_context);

        // Print it out too
        uart_logger_send("Acceleration data: %f %f %f [g]\r\n\r\n",
                         lis3dh_context.x_acc, lis3dh_context.y_acc,
                         lis3dh_context.z_acc);

        // Get the current proximity and light intensity
        vcnl4020_driver_read_all(&vcnl4020_context);

        // Print it out too
        uart_logger_send("Proximity: %d [cnt], Light: %d [lux]\r\n\r\n",
                         vcnl4020_context.proximity_cnt,
                         vcnl4020_context.als_lux);

        // Flip all leds to make
        HAL_GPIO_TogglePin(GPIOC,
                           GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);

        // For now, perform n waits while checking if the interrupt flag was
        // enabled
        for (int n = 0; n < 100; n++) {
            // Check if the interrupt was triggered
            if (lis3dh_interrupt1_flag == 1) {
                // Call the clear function which calls our handler
                lis3dh_clear_interrupt1(
                    &lis3dh_context, lis3dh_interrupt_handler, &interrupt_arg);
            }

            // Check if this interrupt was triggered
            if (vcnl4020_interrupt_flag == 1) {
                vcnl4020_clear_interrupt(&vcnl4020_context,
                                         vcnl4020_interrupt_handler,
                                         &vcnl4020_interrupt_arg);
            }

            HAL_Delay(10);
        }
    }
}
