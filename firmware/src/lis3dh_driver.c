#include "lis3dh_driver.h"

#include <errno.h>

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
volatile int lis3dh_interrupt1_flag = 0;

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

int lis3dh_driver_init(struct lis3dh_config *config,
                       struct lis3dh_context *context) {
    struct i2c_request *request = &context->request;
    struct i2c_request *it_request = &context->it_request;
    struct i2c_driver_context *i2c_context = context->i2c_context;
    int ret = 0;

    context->state = LIS3DH_ERROR;  // If we error out and exit early, set the
                                    // state accordingly.
    context->it_state = LIS3DH_INTERRUPT_CLEAR;

    /*
        GPIO PIN for interrupts is PC2.
        This is connected to INT1 or Inertial interrupt 1
    */

    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2;
    // Interrupt is triggered with rising voltage
    gpio.Mode = GPIO_MODE_IT_RISING;
    // Pin is alrelady pulled to ground by sensor
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &gpio);

    // Configure NVIC so the interrupt can fire
    HAL_NVIC_SetPriority(EXTI2_3_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI2_3_IRQn);

    // Set up the i2c request for the whole driver
    request->address = LIS3DH_ADDRESS;
    request->buffer = context->i2c_transaction_buffer;
    request->future.state = FUTURE_WAITING;
    request->future.error_number = 0;

    // CTRL_REG1. Configures data rate and enabling an axis
    request->action = I2C_WRITE;
    request->ireg = CTRL_REG1;
    request->num_bytes = 1;
    request->buffer[0] = config->reg1.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // CTRL_REG2. Configures high pass filter and where it should be applied.
    request->action = I2C_WRITE;
    request->ireg = CTRL_REG2;
    request->buffer[0] = config->reg2.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // CTRL_REG3. Configures INT1 options
    request->action = I2C_WRITE;
    request->ireg = CTRL_REG3;
    request->buffer[0] = config->reg3.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // CTRL_REG4. Configure scale of data and high resolution on.
    request->action = I2C_WRITE;
    request->ireg = CTRL_REG4;
    request->buffer[0] = config->reg4.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // CTRL_REG5. Configure where the interrupt signal goes.
    request->action = I2C_WRITE;
    request->ireg = CTRL_REG5;
    request->buffer[0] = config->reg5.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // CTRL_REG6. Configure INT2 and polarity.
    request->action = I2C_WRITE;
    request->ireg = CTRL_REG6;
    request->buffer[0] = config->reg6.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // INT1_CFG. Configure what triggers an INT1 interrupt
    request->action = I2C_WRITE;
    request->ireg = INT1_CFG;
    request->buffer[0] = config->int1.as_byte;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // Copy relevant values from config to context
    context->data_scale = config->reg4.fs;
    context->data_rate = config->reg1.odr;

    // INT_THS. Configure what acceleration is needed to trigger an interrupt.
    float acceleration_threshold =
        config->acceleration_threshold;  // measured in 'g's
    acceleration_threshold =
        acceleration_threshold * 1000.0;  // convert to 'mg's
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
        return -EINVAL;
    }

    request->action = I2C_WRITE;
    request->ireg = INT1_THS;
    request->buffer[0] = threshold;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // INT_DURATION. Configure how long an event has to be to be recognized
    // Adjust the value of 'acceleration_duration', which is measured in 'ms'
    float acceleration_duration =
        config->acceleration_duration;                       // measured in 'ms'
    acceleration_duration = acceleration_duration / 1000.0;  // Convert to 's'
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
        return -EINVAL;
    }

    request->action = I2C_WRITE;
    request->ireg = INT1_DURATION;
    request->buffer[0] = duration;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    // Finally set up interrupts and their i2c request
    it_request->action = I2C_READ;
    it_request->address = LIS3DH_ADDRESS;
    it_request->ireg = INT1_SRC;
    it_request->buffer = context->it_status_buffer;
    it_request->num_bytes = 1;
    it_request->future.state = FUTURE_WAITING;
    it_request->future.error_number = 0;

    // Set the interrupt to triggered so we can clear any previous calls
    lis3dh_interrupt1_flag = LIS3DH_INTERRUPT_TRIGGERED;

    // Device is ready to use
    context->state = LIS3DH_READY;

    return 0;
}

/*
    Function for after reading all axes of the sensor.
    Processes the data and saves the results into the context
    struct for this driver
*/
void lis3dh_driver_process_acceleration(struct lis3dh_context *context) {
    struct i2c_request *request = &context->request;
    // Shift the HIGH bits over 8
    int16_t x_val = request->buffer[0] | (request->buffer[1] << 8);
    int16_t y_val = request->buffer[2] | (request->buffer[3] << 8);
    int16_t z_val = request->buffer[4] | (request->buffer[5] << 8);

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

int lis3dh_driver_request_acceleration(struct lis3dh_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;

    request->action = I2C_READ;
    // (1 << 7) tells lis3dh to read more than 1 byte
    request->ireg = OUT_X_L | (1 << 7);
    request->num_bytes = 6;

    // Submit a request for the acceleration data
    return i2c_enqueue_request(i2c_context, request);
}

int lis3dh_driver_request_it_clear(struct lis3dh_context *context) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *it_request = &context->it_request;

    return i2c_enqueue_request(i2c_context, it_request);
}
