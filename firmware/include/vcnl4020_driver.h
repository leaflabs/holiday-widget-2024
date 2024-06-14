#ifndef __VCNL4020_DRIVER_H
#define __VCNL4020_DRIVER_H
#include <stdbool.h>

#include "stm32l0xx_hal.h"

/*
    Command Register
*/

union command_register {
    struct __attribute__((packed)) {
        bool : 1;  // This is the enable self time measure, but the driver will
                   // control when this is turned on.
        bool prox_en : 1;  // Proximity enable
        bool als_en : 1;   // Ambient enable
        uint8_t : 5;       // Do not allow writing the rest of the bits
    };
    uint8_t as_byte;
};

/*
    Proximity Rate Register
*/

// Read as: VCNL4020_PR_x_yz -> x.yz, where x.yz is measurements/s
// Ex: VCNL4020_PR_1_95 -> 1.95 measurements/s
enum proximity_rate {
    VCNL4020_PR_1_95 = 0x0,
    VCNL4020_PR_3_90 = 0x1,
    VCNL4020_PR_7_81 = 0x2,
    VCNL4020_PR_16_62 = 0x3,
    VCNL4020_PR_31_25 = 0x4,
    VCNL4020_PR_62_50 = 0x5,
    VCNL4020_PR_125_00 = 0x6,
    VCNL4020_PR_250_00 = 0x7
};

union proximity_rate_register {
    struct __attribute__((packed)) {
        enum proximity_rate
            proximity_rate : 3;  // Defines the samples per second for the
                                 // proximity sensor
        uint8_t : 5;  // Unused
    };
    uint8_t as_byte;
};

/*
    IR LED Current Register
*/

// Limit what values the user can enter using an enum
enum current_value {
    VCNL4020_0_MA = 0,
    VCNL4020_10_MA = 1,
    VCNL4020_20_MA = 2,
    VCNL4020_30_MA = 3,
    VCNL4020_40_MA = 4,
    VCNL4020_50_MA = 5,
    VCNL4020_60_MA = 6,
    VCNL4020_70_MA = 7,
    VCNL4020_80_MA = 8,
    VCNL4020_90_MA = 9,
    VCNL4020_100_MA = 10,
    VCNL4020_110_MA = 11,
    VCNL4020_120_MA = 12,
    VCNL4020_130_MA = 13,
    VCNL4020_140_MA = 14,
    VCNL4020_150_MA = 15,
    VCNL4020_160_MA = 16,
    VCNL4020_170_MA = 17,
    VCNL4020_180_MA = 18,
    VCNL4020_190_MA = 19,
    VCNL4020_200_MA = 20,
};

union ir_led_current_register {
    struct __attribute__((packed)) {
        enum current_value current_value : 6;  // Defines what current draw
                                               // should be set for the led
        uint8_t : 2;  // Reserved
    };
    uint8_t as_byte;
};

/*
    Ambient Light parameter Register
*/

enum averaging_func {
    VCNL4020_1_CONV = 0,
    VCNL4020_2_CONV = 1,
    VCNL4020_4_CONV = 2,
    VCNL4020_8_CONV = 3,
    VCNL4020_16_CONV = 4,
    VCNL4020_32_CONV = 5,
    VCNL4020_64_CONV = 6,
    VCNL4020_128_CONV = 7,
};

enum als_rate {
    VCNL4020_AR_1 = 0x0,
    VCNL4020_AR_2 = 0x1,
    VCNL4020_AR_3 = 0x2,
    VCNL4020_AR_4 = 0x3,
    VCNL4020_AR_5 = 0x4,
    VCNL4020_AR_6 = 0x5,
    VCNL4020_AR_8 = 0x6,
    VCNL4020_AR_10 = 0x7,
};

union ambient_light_parameter_register {
    struct __attribute__((packed)) {
        enum averaging_func averaging_func : 3;  // Defines how many averages
                                                 // are taken per data point
        bool offset_enable : 1;      // Defines if an offset should be used to
                                     // remove drift with signal
        enum als_rate als_rate : 3;  // Defines what sample rate the als uses
        bool : 1;  // Hiding bit: Not allowing const conversion feature
    };
    uint8_t as_byte;
};

/*
    Interrupt Control Register
*/

enum thresh_prox_als { VCNL4020_THRESH_PROX = 0, VCNL4020_THRESH_ALS = 1 };

enum interrupt_count {
    VCNL4020_TC_1 = 0,
    VCNL4020_TC_2 = 1,
    VCNL4020_TC_4 = 2,
    VCNL4020_TC_8 = 3,
    VCNL4020_TC_16 = 4,
    VCNL4020_TC_32 = 5,
    VCNL4020_TC_64 = 6,
    VCNL4020_TC_128 = 7
};

union interrupt_control_register {
    struct __attribute__((packed)) {
        enum thresh_prox_als
            thresh_prox_als : 1;  // Choose between proximity and als getting to
                                  // trigger interrupts
        bool thresh_enable : 1;  // Allow Enabling interrupts for thresholds
        uint8_t : 3;  // Not allowing proximity or als data ready interrupts.
                      // also 1 extra for n/a
        enum interrupt_count interrupt_count : 3;
    };
    uint8_t as_byte;
};

struct interrupt_thresholds {
    /*
        Units:
        If the user selected VCNL4020_THRESH_PROX, the units are in counts
        of reflected signals.
        If the user selected VCNL4020_THRESH_ALS, the units are in lux.
        - While the units for the sensor are in cnts also for als, the
          conversion is done automatically by the init function. Do not
          scale back to cnts
    */
    uint16_t low;   // Must fall below to trigger an interrupt
    uint16_t high;  // Must exceed to trigger an interrupt
};

/*
    Config struct for initalizing the sensor
*/
struct vcnl4020_config {
    union command_register command_register;
    union proximity_rate_register proximity_rate_register;
    union ir_led_current_register ir_led_current_register;
    union ambient_light_parameter_register ambient_light_parameter_register;
    union interrupt_control_register interrupt_control_register;
    struct interrupt_thresholds interrupt_thresholds;
};

/*
    Context struct for the driver. Holds all relevant information for it to
   operate correctly
*/
struct vcnl4020_context {
    I2C_HandleTypeDef *i2c;

    // Record the values read
    uint16_t proximity_cnt;
    uint16_t als_lux;
};

// Global interrupt flag
extern volatile uint8_t vcnl4020_interrupt_flag;

/*
    Initalize the vcnl4020 sensor based on the user defined
    config struct.
    'config' defines the values for each register in the sensor. It will not be
   modified 'context' holds the relevant data for this driver to work.

    Prerequisite: the I2C_HandleTypeDef in vcnl4020_context must
    be a valid pointer in order for this function to work
*/
void vcnl4020_driver_init(const struct vcnl4020_config *config,
                          const struct vcnl4020_context *context);

/*
    Read the proximity value off the sensor and save it in the
    context struct
    'context' holds the relevant data for this driver to work
*/
void vcnl4020_driver_read_proximity(struct vcnl4020_context *context);

/*
    Read the ambient light value off the sensor and save it in
    the context struct
    'context' holds the relevant data for this driver to work
*/
void vcnl4020_driver_read_als(struct vcnl4020_context *context);

/*
    Read both the proximity data and ambient light data off the
    sensor in one i2c transaction
    'context' holds the relvant data for this driver to work
*/
void vcnl4020_driver_read_all(struct vcnl4020_context *context);

/*
    Clears the interrupt flag for the driver, reads the status
    register off the sensor and clears the sensor's internal
    flag. Then calls the 'user_handler' function while passing
    in the status register's value and 'user_handler_arg'
    'context' holds the relevant data for this driver to work
    'user_handler' is a function pointer to a user defined function
        that is called to process an interrupt
    'user_handler_arg' is a void pointer to any data the user may
        want with their handler function
*/
void vcnl4020_clear_interrupt(const struct vcnl4020_context *context,
                              void (*user_handler)(uint8_t, void *),
                              void *user_handler_arg);

#endif
