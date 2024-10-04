#ifndef __LIS3DH_DRIVER_H
#define __LIS3DH_DRIVER_H

#include "i2c_driver.h"
#include "stdbool.h"
#include "stm32l0xx_hal.h"
#include "system_communication.h"

#define LIS3DH_MAX_I2C_SIZE 6U

enum lis3dh_interrupt_state {
    LIS3DH_INTERRUPT_CLEAR = 0,
    LIS3DH_INTERRUPT_TRIGGERED = 1,
    // I2C transaction to clear interrupt register is pending
    LIS3DH_INTERRUPT_CLEARING = 2,
};

// Flag for the interrupt
extern volatile int lis3dh_interrupt1_flag;

/*
    CTRL_REG1
*/
enum odr {
    LIS3DH_POWER_DOWN = 0x0,
    LIS3DH_1_HZ = 0x1,
    LIS3DH_10_HZ = 0x2,
    LIS3DH_25_HZ = 0x3,
    LIS3DH_50_HZ = 0x4,
    LIS3DH_100_HZ = 0x5,
    LIS3DH_200_HZ = 0x6,
    LIS3DH_400_HZ = 0x7,
    // Notice skipping 0x8 as that is for low power mode only
    LIS3DH_1344_HZ = 0x9
};

union control_reg1 {
    struct __attribute__((packed)) {
        bool xen : 1;
        bool yen : 1;
        bool zen : 1;
        bool lpen : 1;
        enum odr odr : 4;
    };
    uint8_t as_byte;
};

/*
    CTRL_REG2
*/

enum hpm {
    LIS3DH_HIGH_PASS_MODE_NORMAL_RESET =
        0x0,  // Reset by reading REFERENCE register
    LIS3DH_HIGH_PASS_MODE_REF_SIGNAL = 0x1,
    LIS3DH_HIGH_PASS_MODE_NORMAL = 0x2,
    LIS3DH_HIGH_PASS_MODE_AUTO_RESET = 0x3
};

enum hpcf {
    LIS3DH_HIGH_PASS_FILTER_VERY_HIGH = 0x0,
    LIS3DH_HIGH_PASS_FILTER_HIGH = 0x1,
    LIS3DH_HIGH_PASS_FILTER_MEDIUM = 0x2,
    LIS3DH_HIGH_PASS_FILTER_LOW = 0x3
};

union control_reg2 {
    struct __attribute__((packed)) {
        bool hp_ia1 : 1;
        bool hp_ia2 : 1;
        bool hpclick : 1;
        bool fds : 1;
        enum hpcf hpcf : 2;
        enum hpm hpm : 2;
    };
    uint8_t as_byte;
};

/*
    CTRL_REG3
*/

union control_reg3 {
    struct __attribute__((packed)) {
        bool : 1;
        bool i1_overrun : 1;
        bool i1_wtm : 1;
        bool i1_321da : 1;
        bool i1_zyxda : 1;
        bool i1_ia2 : 1;
        bool i1_ia1 : 1;
        bool i1_click : 1;
    };
    uint8_t as_byte;
};

/*
    CTRL_REG4
*/

enum fs {
    LIS3DH_SCALE_2G = 0x0,
    LIS3DH_SCALE_4G = 0x1,
    LIS3DH_SCALE_8G = 0x2,
    LIS3DH_SCALE_16G = 0x3
};

enum st {
    LIS3DH_SELF_TEST_NORMAL = 0x0,
    LIS3DH_SELF_TEST_0 = 0x1,
    LIS3DH_SELF_TEST_1 = 0x2,
};

union control_reg4 {
    struct __attribute__((packed)) {
        bool sim : 1;
        enum st st : 2;
        bool hr : 1;
        enum fs fs : 2;
        bool ble : 1;
        bool bdu : 1;
    };
    uint8_t as_byte;
};

/*
    CTRL_REG5
*/

union control_reg5 {
    struct __attribute__((packed)) {
        bool d4d_int2 : 1;
        bool lir_int2 : 1;
        bool d4d_int1 : 1;
        bool lir_int1 : 1;
        bool : 1;
        bool : 1;
        bool fifo_en : 1;
        bool boot : 1;
    };
    uint8_t as_byte;
};

/*
    CTRL_REG6
*/

union control_reg6 {
    struct __attribute__((packed)) {
        bool : 1;
        bool int_polarity : 1;
        bool : 1;
        bool i2_act : 1;
        bool i2_boot : 1;
        bool i2_ia2 : 1;
        bool i2_ia1 : 1;
        bool i2_click : 1;
    };
    uint8_t as_byte;
};

/*
    INT1_CFG
*/

enum it_mode {
    LIS3DH_OR_MODE = 0x0,
    LIS3DH_MOVEMENT_MODE = 0x1,
    LIS3DH_AND_MODE = 0x2,
    LIS3DH_POSITION_MODE = 0x3
};

union interrupt1_config {
    struct __attribute__((packed)) {
        bool xlow : 1;
        bool xhigh : 1;
        bool ylow : 1;
        bool yhigh : 1;
        bool zlow : 1;
        bool zhigh : 1;
        enum it_mode it_mode : 2;
    };
    uint8_t as_byte;
};

/*
    Configuration struct for initalization
*/
struct lis3dh_config {
    union control_reg1 reg1;
    union control_reg2 reg2;
    union control_reg3 reg3;
    union control_reg4 reg4;
    union control_reg5 reg5;
    union control_reg6 reg6;
    union interrupt1_config int1;
    float acceleration_threshold;
    float acceleration_duration;
};

/*
    Three states for this driver.
    PRE_INIT - device is not initalized yet
    READY - device is ready to use and read data
    PENDING - device is waiting for i2c to complete
    ERROR - device had an issue
*/
enum lis3dh_state {
    LIS3DH_PRE_INIT,
    LIS3DH_READY,
    LIS3DH_PENDING,
    LIS3DH_ERROR
};

/*
    Context struct for the lis3dh sensor
*/
struct lis3dh_context {
    // i2c context for communication
    struct i2c_driver_context *i2c_context;

    struct i2c_request request;
    struct i2c_request it_request;

    struct driver_comm_message_passing *comm;

    // Record the state of the driver
    volatile enum lis3dh_state state;
    volatile enum lis3dh_interrupt_state it_state;
    // Last x,y,z acceleration values
    float x_acc;
    float y_acc;
    float z_acc;

    // Record the data rate and data scale
    enum odr data_rate;
    enum fs data_scale;

    uint8_t i2c_transaction_buffer[LIS3DH_MAX_I2C_SIZE];
    uint8_t it_status_buffer[1];
};

/*
    Initalizes the lis3dh sensor and sets up the context struct.
    'config' is a lis3dh_config struct for each control register and data for
   the interrupt 'context' is the context struct for the sensor.

    Prerequisite: the I2C_HandleTypeDef in lis3dh_context must be a valid
   pointer in order for this function to work.

    Returns 0 on success.
*/
int lis3dh_driver_init(struct lis3dh_config *config,
                       struct lis3dh_context *context);

/*
    Request to read the acceleration data from the sensor

    returns 0 if the i2c transaction was added sucessfully
*/
int lis3dh_driver_request_acceleration(struct lis3dh_context *context);

/*
    Convert the raw acceleration data received from the i2c transaction
    into a usable format and save in the context struct
*/
void lis3dh_driver_process_acceleration(struct lis3dh_context *context);

/*
    Request to clear the interrupt flag on the sensor

    returns 0 if the i2c transaction was added sucessfully
*/
int lis3dh_driver_request_it_clear(struct lis3dh_context *context);

/*
 * Puts the LIS3DH sensor in low power mode. Leaves on interrupts
 * and reduces the data rate to save power.
 *
 * I2C transactions finish before leaving function
 *
 * This sensor is left on as it will be used to wake
 * up the stm when moved
 */
int lis3dh_driver_enter_low_power(struct lis3dh_context *context);

/*
 * Puts the LIS3DH sensor into high resolution mode. Re-enables
 * everything low power mode changed.
 *
 * I2C transactions finished before leaving function
 */
int lis3dh_driver_exit_low_power(struct lis3dh_context *context);

#endif
