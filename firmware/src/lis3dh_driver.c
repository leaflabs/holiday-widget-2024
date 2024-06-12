#include "lis3dh_driver.h"

#include "i2c_driver.h"
#include "stm32l0xx_hal.h"
#include "uart_logger.h"
#include "utils.h"

// I2C address for the lis3dh sensor
#define LIS3DH_ADDRESS 0x18

// Control register addresses
#define CTRL_REG1 0x20
#define CTRL_REG2 0x21
#define CTRL_REG3 0x22
#define CTRL_REG4 0x23
#define CTRL_REG5 0x24
#define CTRL_REG6 0x25
#define STATUS_REG 0x27

// Acceleration register addresses
#define OUT_X_L 0x28
#define OUT_X_H 0x29
#define OUT_Y_L 0x2A
#define OUT_Y_H 0x2B
#define OUT_Z_L 0x2C
#define OUT_Z_H 0x2D

// Interrupt 1 register addresses
#define INT1_CFG 0x30
#define INT1_SRC 0x31
#define INT1_THS 0x32
#define INT1_DURATION 0x33

// Flag for if the lis3dh interrupt was triggered
volatile uint8_t lis3dh_interrupt1_flag = 0;

// Map each scale value to the corrosponding 'mg per bit' ratio
static const float data_scale_values[] = {[LIS3DH_SCALE_2G] = 16.0f,
                                          [LIS3DH_SCALE_4G] = 32.0f,
                                          [LIS3DH_SCALE_8G] = 62.0f,
                                          [LIS3DH_SCALE_16G] = 186.0f};

// Map each data rate to its actual value for conversions
const float data_rate_values[] = {
    [LIS3DH_1_HZ] = 1.0f,     [LIS3DH_10_HZ] = 10.0f,
    [LIS3DH_25_HZ] = 25.0f,   [LIS3DH_50_HZ] = 50.0f,
    [LIS3DH_100_HZ] = 100.0f, [LIS3DH_200_HZ] = 200.0f,
    [LIS3DH_400_HZ] = 400.0f, [LIS3DH_1344_HZ] = 1344.0f};

void lis3dh_driver_init(struct lis3dh_config *config,
                        struct lis3dh_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    /*
        GPIO PIN for interrupts is PC2.
        This is connected to INT1 or Inertial interrupt 1
    */

    // Enable the clock
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // Configure the pin to accept interrupts
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2;
    gpio.Mode =
        GPIO_MODE_IT_RISING;  // Interrupt is triggered on rising voltage
    gpio.Pull = GPIO_NOPULL;  // Pin is already pulled to ground by sensor
    HAL_GPIO_Init(GPIOC, &gpio);

    // Configure NVIC so the interrupt can fire
    HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

    /*
        Configure each control register on the sensor
    */

    // CTRL_REG1. Configures data rate and enabling an axis
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, CTRL_REG1,
                               &config->reg1.as_byte, 1);

    // CTRL_REG2. Configures high pass filter and where it should be applied.
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, CTRL_REG2,
                               &config->reg2.as_byte, 1);

    // CTRL_REG3. Configures INT1 options
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, CTRL_REG3,
                               &config->reg3.as_byte, 1);

    // CTRL_REG4. Configure scale of data and high resolution on.
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, CTRL_REG4,
                               &config->reg4.as_byte, 1);

    // CTRL_REG5. Configure where the interrupt signal goes.
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, CTRL_REG5,
                               &config->reg5.as_byte, 1);

    // CTRL_REG5. Configure INT2 and polarity.
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, CTRL_REG6,
                               &config->reg6.as_byte, 1);

    // INT1_CFG. Configure what triggers an INT1 interrupt
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, INT1_CFG,
                               &config->int1.as_byte, 1);

    // Copy relevant values from config to context
    context->data_scale = config->reg4.fs;
    context->data_rate = config->reg1.odr;

    // INT_THS. Configure what acceleration is needed to trigger an interrupt.
    float acceleration_threshold =
        config->acceleration_threshold;  // measured in 'g's
    acceleration_threshold = acceleration_threshold * 1000.0; // convert to 'mg's
    uint8_t threshold = 0;  // This will be the 8 bits to write to the register

    // Make sure the sensor is in the range of values
    if (config->reg4.fs <= LIS3DH_SCALE_16G) {
        // Get the conversion from the array based on the scale value
        threshold = acceleration_threshold / data_scale_values[config->reg4.fs];
    }

    // Make sure threshold value is a valid output. The MSB cannot be 1
    if ((threshold & (1 << 7)) != 0) {
        uart_logger_send(
            "FATAL ERROR: Invalid threshold value. MSB is not 0 though it "
            "should be:\r\n");
        bit_print(&threshold, 1);
        while (1)
            ;  // Spin forever because the user needs to change the settings
    }

    // Write the register now
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, INT1_THS, &threshold, 1);

    // INT_DURATION. Configure how long an event has to be to be recognized
    // Adjust the value of 'acceleration_duration', which is measured in 'ms'
    float acceleration_duration =
        config->acceleration_duration;                      // measured in 'ms'
    acceleration_duration = acceleration_duration / 1000.0; // Convert to 's'
    uint8_t duration = 0;  // This will be the 8 bits to write to the register

    // Make sure it is in the acceptable range
    if (config->reg1.odr <= LIS3DH_1344_HZ) {
        // Multiply by the frequency to get the binary value
        duration = acceleration_duration * data_rate_values[config->reg1.odr];
    }

    // Make sure the duration is a valid value. MSB cannot be 1
    if ((duration & (1 << 7)) != 0) {
        uart_logger_send(
            "FATAL ERROR: Invalid duration value. MSB is not 0 though it "
            "should be:\r\n");
        bit_print(&duration, 1);
        while (1)
            ;  // Spin forever because the user needs to change the settings
    }

    // Now write the duration value
    i2c_driver_write_registers(i2c, LIS3DH_ADDRESS, INT1_DURATION, &duration,
                               1);

    // Finally, read the interrupt register to clear it incase a previous flag
    // (in the sensor) was not cleared
    lis3dh_clear_interrupt1(context, NULL,
                            NULL);  // No handler or args passed in
}

void lis3dh_clear_interrupt1(struct lis3dh_context *context,
                             void (*user_handler)(uint8_t, void *),
                             void *user_handler_args) {
    I2C_HandleTypeDef *i2c = context->i2c;

    // Remove the interrupt flag and read the src register to remove the
    // interrupt and call the user defined handler First clear the flag
    lis3dh_interrupt1_flag = 0;  // Reset

    // Read the interrupt 1 source (INT1_SRC) register to clear the sensor's
    // interrupt flag
    uint8_t status = 0;
    i2c_driver_read_registers(i2c, LIS3DH_ADDRESS, INT1_SRC, &status, 1);

    // Call the user defined handler if it exists
    if (user_handler != NULL) {
        user_handler(status, user_handler_args);
    }
}

void lis3dh_driver_read_all(struct lis3dh_context *context) {
    I2C_HandleTypeDef *i2c = context->i2c;

    uint8_t messages[6];
    // To read continuously on the lis3dh, we need to set the 8th bit of the
    // address to a 1. Hence the OUT_X_L | (1 << 7)
    i2c_driver_read_registers(i2c, LIS3DH_ADDRESS, OUT_X_L | (1 << 7), messages,
                              6);

    // Shift the HIGH bits over 8
    int16_t x_val = messages[0] | (messages[1] << 8);
    int16_t y_val = messages[2] | (messages[3] << 8);
    int16_t z_val = messages[4] | (messages[5] << 8);

    // Get our conversion to go from binary to 'mgs' and 'mgs' to 'g'
    // 'int16_t' * 1/data_scale = 'mg'
    // 'mg' / 1000 = 'g'
    float conversion =
        1.0f / (data_scale_values[context->data_scale] * 1000.0f);

    // Convert to 'g's
    float x_acc = x_val * conversion;
    float y_acc = y_val * conversion;
    float z_acc = z_val * conversion;

    // Save the values in the struct
    context->x_acc = x_acc;
    context->y_acc = y_acc;
    context->z_acc = z_acc;
}
