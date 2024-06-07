#include "main.h"

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "tmp102_driver.h"
#include "uart_logger.h"

// How many addresses we can store from an i2c_driver_scan call
#define MAX_ADDRESSES 10U

/*
    i2c_scan_arg is the struct passed into the i2c_scan_handler
    to save the addresses found
*/
struct i2c_scan_arg {
    uint8_t addrs[MAX_ADDRESSES];  // Upto MAX_ADDRS number of addresses
    size_t index;                  // Keep track of which element we are on
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

    while (1) {
        // Get the temperature from the tmp102 sensor
        float temp = tmp102_driver_read(&i2c);

        // Print it out
        uart_logger_send("Temp is: %f C, %f F\r\n\r\n", temp,
                         (9.0f / 5.0f * temp) + 32);

        // Flip all leds to make
        HAL_GPIO_TogglePin(GPIOC,
                           GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11);

        // Wait 1 second
        HAL_Delay(1000);
    }
}
