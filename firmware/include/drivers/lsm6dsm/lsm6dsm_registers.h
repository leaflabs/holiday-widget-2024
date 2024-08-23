#ifndef __LSM6DSM_REGISTERS_H__
#define __LSM6DSM_REGISTERS_H__
#include <stdint.h>

#include "stm32l072xx.h"

/**
 * This file contains all of the register
 * definitions for the LSM6DSM.
 */

#define LSM6DSM_ID 0x6AU

/*
    Configuration access for embedded functions
*/
#define LSM6DSM_FUNC_CFG_ACCESS 0x01U
struct lsm6dsm_func_cfg_access {
    uint8_t not_used_01 : 5;
    uint8_t func_cfg_en : 3;
};

/*
    Sensor synchronization time frame configuration
*/
#define LSM6DSM_SENSOR_SYNC_TIME_FRAME 0x04U
struct lsm6dsm_sensor_sync_time_frame {
    uint8_t tph : 4;
    uint8_t not_used_01 : 4;
};

/*
    Sensor synchronization resolution ratio configuration
*/
#define LSM6DSM_SENSOR_SYNC_RES_RATIO 0x05U
struct lsm6dsm_sensor_sync_res_ratio {
    uint8_t rr : 2;
    uint8_t not_used_01 : 6;
};

/*
    FIFO control register 1
*/
#define LSM6DSM_FIFO_CTRL1 0x06U
struct lsm6dsm_fifo_ctrl1 {
    uint8_t fth : 8;
};

/*
    FIFO control register 2
*/
#define LSM6DSM_FIFO_CTRL2 0x07U
struct lsm6dsm_fifo_ctrl2 {
    uint8_t fth : 3;
    uint8_t fifo_temp_en : 1;
    uint8_t not_used_01 : 2;
    uint8_t timer_pedo_fifo_drdy : 1;
    uint8_t timer_pedo_fifo_en : 1;
};

/*
    FIFO control register 3
*/
#define LSM6DSM_FIFO_CTRL3 0x08U
struct lsm6dsm_fifo_ctrl3 {
    uint8_t dec_fifo_xl : 3;
    uint8_t dec_fifo_gyro : 3;
    uint8_t not_used_01 : 2;
};

/*
    FIFO control register 4
*/
#define LSM6DSM_FIFO_CTRL4 0x09U
struct lsm6dsm_fifo_ctrl4 {
    uint8_t dec_ds3_fifo : 3;
    uint8_t dec_ds4_fifo : 3;
    uint8_t only_high_data : 1;
    uint8_t stop_on_fth : 1;
};

/*
    FIFO control register 5
*/
#define LSM6DSM_FIFO_CTRL5 0x0AU
struct lsm6dsm_fifo_ctrl5 {
    uint8_t fifo_mode : 3;
    uint8_t odr_fifo : 4;
    uint8_t not_used_01 : 1;
};

/*
    Data ready pulse configuration register
*/
#define LSM6DSM_DRDY_PULSE_CFG 0x0BU
struct lsm6dsm_drdy_pulse_cfg {
    uint8_t int2_wrist_tilt : 1;
    uint8_t not_used_01 : 6;
    uint8_t drdy_pulsed : 1;
};

/*
    Interrupt control register 1
*/
#define LSM6DSM_INT1_CTRL 0x0DU
struct lsm6dsm_int1_ctrl {
    uint8_t int1_drdy_xl : 1;
    uint8_t int1_drdy_g : 1;
    uint8_t int1_boot : 1;
    uint8_t int1_fth : 1;
    uint8_t int1_fifo_ovr : 1;
    uint8_t int1_full_flag : 1;
    uint8_t int1_sign_mot : 1;
    uint8_t int1_step_detector : 1;
};

/*
    Interrupt control register 2
*/
#define LSM6DSM_INT2_CTRL 0x0EU
struct lsm6dsm_int2_ctrl {
    uint8_t int2_drdy_xl : 1;
    uint8_t int2_drdy_g : 1;
    uint8_t int2_drdy_temp : 1;
    uint8_t int2_fth : 1;
    uint8_t int2_fifo_ovr : 1;
    uint8_t int2_full_flag : 1;
    uint8_t int2_step_count_ov : 1;
    uint8_t int2_step_delta : 1;
};

#define LSM6DSM_WHO_AM_I 0x0FU

/*
    Accelerometer control register 1
*/
#define LSM6DSM_CTRL1_XL 0x10U
struct lsm6dsm_ctrl1_xl {
    uint8_t bw0_xl : 1;
    uint8_t lpf1_bw_sel : 1;
    uint8_t fs_xl : 2;
    uint8_t odr_xl : 4;
};

/*
    Gyroscope control register 2
*/
#define LSM6DSM_CTRL2_G 0x11U
struct lsm6dsm_ctrl2_g {
    uint8_t not_used_01 : 1;
    uint8_t fs_g : 3;
    uint8_t odr_g : 4;
};

/*
    Control register 3
*/
#define LSM6DSM_CTRL3_C 0x12U
struct lsm6dsm_ctrl3_c {
    uint8_t sw_reset : 1;
    uint8_t ble : 1;
    uint8_t if_inc : 1;
    uint8_t sim : 1;
    uint8_t pp_od : 1;
    uint8_t h_lactive : 1;
    uint8_t bdu : 1;
    uint8_t boot : 1;
};

/*
    Control register 4
*/
#define LSM6DSM_CTRL4_C 0x13U
struct lsm6dsm_ctrl4_c {
    uint8_t not_used_01 : 1;
    uint8_t lpf1_sel_g : 1;
    uint8_t i2c_disable : 1;
    uint8_t drdy_mask : 1;
    uint8_t den_drdy_int1 : 1;
    uint8_t int2_on_int1 : 1;
    uint8_t sleep : 1;
    uint8_t den_xl_en : 1;
};

/*
    Control register 5
*/
#define LSM6DSM_CTRL5_C 0x14U
struct lsm6dsm_ctrl5_c {
    uint8_t st_xl : 2;
    uint8_t st_g : 2;
    uint8_t den_lh : 1;
    uint8_t rounding : 3;
};

/*
    Control register 6
*/
#define LSM6DSM_CTRL6_C 0x15U
struct lsm6dsm_ctrl6_c {
    uint8_t ftype : 2;
    uint8_t not_used_01 : 1;
    uint8_t usr_off_w : 1;
    uint8_t xl_hm_mode : 1;
    uint8_t den_mode : 3;
};

/*
    Gyroscope control register 7
*/
#define LSM6DSM_CTRL7_G 0x16U
struct lsm6dsm_ctrl7_g {
    uint8_t not_used_01 : 2;
    uint8_t rounding_status : 1;
    uint8_t not_used_02 : 1;
    uint8_t hpm_g : 2;
    uint8_t hp_en_g : 1;
    uint8_t g_hm_mode : 1;
};

/*
    Accelerometer control register 8
*/
#define LSM6DSM_CTRL8_XL 0x17U
struct lsm6dsm_ctrl8_xl {
    uint8_t low_pass_on_6d : 1;
    uint8_t not_used_01 : 1;
    uint8_t hp_slope_xl_en : 1;
    uint8_t input_composite : 1;
    uint8_t hp_ref_mode : 1;
    uint8_t hpcf_xl : 2;
    uint8_t lpf2_xl_en : 1;
};

/*
    Accelerometer control register 9
*/
#define LSM6DSM_CTRL9_XL 0x18U
struct lsm6dsm_ctrl9_xl {
    uint8_t not_used_01 : 2;
    uint8_t soft_en : 1;
    uint8_t not_used_02 : 1;
    uint8_t den_xl_g : 1;
    uint8_t den_z : 1;
    uint8_t den_y : 1;
    uint8_t den_x : 1;
};

/*
    Control register 10
*/
#define LSM6DSM_CTRL10_C 0x19U
struct lsm6dsm_ctrl10_c {
    uint8_t sign_motion_en : 1;
    uint8_t pedo_rst_step : 1;
    uint8_t func_en : 1;
    uint8_t tilt_en : 1;
    uint8_t pedo_en : 1;
    uint8_t timer_en : 1;
    uint8_t not_used_01 : 1;
    uint8_t wrist_tilt_en : 1;
};

/*
    Master configuration register
*/
#define LSM6DSM_MASTER_CONFIG 0x1AU
struct lsm6dsm_master_config {
    uint8_t master_on : 1;
    uint8_t iron_en : 1;
    uint8_t pass_through_mode : 1;
    uint8_t pull_up_en : 1;
    uint8_t start_config : 1;
    uint8_t not_used_01 : 1;
    uint8_t data_valid_sel_fifo : 1;
    uint8_t drdy_on_int1 : 1;
};

/*
    Wake-up source register
*/
#define LSM6DSM_WAKE_UP_SRC 0x1BU
struct lsm6dsm_wake_up_src {
    uint8_t z_wu : 1;
    uint8_t y_wu : 1;
    uint8_t x_wu : 1;
    uint8_t wu_ia : 1;
    uint8_t sleep_state_ia : 1;
    uint8_t ff_ia : 1;
    uint8_t not_used_01 : 2;
};

/*
    Tap source register
*/
#define LSM6DSM_TAP_SRC 0x1CU
struct lsm6dsm_tap_src {
    uint8_t z_tap : 1;
    uint8_t y_tap : 1;
    uint8_t x_tap : 1;
    uint8_t tap_sign : 1;
    uint8_t double_tap : 1;
    uint8_t single_tap : 1;
    uint8_t tap_ia : 1;
    uint8_t not_used_01 : 1;
};

/*
    6D/4D detection source register
*/
#define LSM6DSM_D6D_SRC 0x1DU
struct lsm6dsm_d6d_src {
    uint8_t xl : 1;
    uint8_t xh : 1;
    uint8_t yl : 1;
    uint8_t yh : 1;
    uint8_t zl : 1;
    uint8_t zh : 1;
    uint8_t d6d_ia : 1;
    uint8_t den_drdy : 1;
};

/*
    Status register
*/
#define LSM6DSM_STATUS_REG 0x1EU
struct lsm6dsm_status_reg {
    uint8_t xlda : 1;
    uint8_t gda : 1;
    uint8_t tda : 1;
    uint8_t not_used_01 : 5;
};

/*
    SPI auxiliary status register
*/
#define LSM6DSM_STATUS_SPIAUX 0x1EU
struct lsm6dsm_status_spiaux {
    uint8_t xlda : 1;
    uint8_t gda : 1;
    uint8_t gyro_settling : 1;
    uint8_t not_used_01 : 5;
};

#define LSM6DSM_OUT_TEMP_L 0x20U
#define LSM6DSM_OUT_TEMP_H 0x21U
#define LSM6DSM_OUTX_L_G 0x22U
#define LSM6DSM_OUTX_H_G 0x23U
#define LSM6DSM_OUTY_L_G 0x24U
#define LSM6DSM_OUTY_H_G 0x25U
#define LSM6DSM_OUTZ_L_G 0x26U
#define LSM6DSM_OUTZ_H_G 0x27U
#define LSM6DSM_OUTX_L_XL 0x28U
#define LSM6DSM_OUTX_H_XL 0x29U
#define LSM6DSM_OUTY_L_XL 0x2AU
#define LSM6DSM_OUTY_H_XL 0x2BU
#define LSM6DSM_OUTZ_L_XL 0x2CU
#define LSM6DSM_OUTZ_H_XL 0x2DU

/*
    Sensor hub register 1
*/
#define LSM6DSM_SENSORHUB1_REG 0x2EU
struct lsm6dsm_sensorhub1_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 2
*/
#define LSM6DSM_SENSORHUB2_REG 0x2FU
struct lsm6dsm_sensorhub2_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 3
*/
#define LSM6DSM_SENSORHUB3_REG 0x30U
struct lsm6dsm_sensorhub3_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 4
*/
#define LSM6DSM_SENSORHUB4_REG 0x31U
struct lsm6dsm_sensorhub4_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 5
*/
#define LSM6DSM_SENSORHUB5_REG 0x32U
struct lsm6dsm_sensorhub5_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 6
*/
#define LSM6DSM_SENSORHUB6_REG 0x33U
struct lsm6dsm_sensorhub6_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 7
*/
#define LSM6DSM_SENSORHUB7_REG 0x34U
struct lsm6dsm_sensorhub7_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 8
*/
#define LSM6DSM_SENSORHUB8_REG 0x35U
struct lsm6dsm_sensorhub8_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 9
*/
#define LSM6DSM_SENSORHUB9_REG 0x36U
struct lsm6dsm_sensorhub9_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 10
*/
#define LSM6DSM_SENSORHUB10_REG 0x37U
struct lsm6dsm_sensorhub10_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 11
*/
#define LSM6DSM_SENSORHUB11_REG 0x38U
struct lsm6dsm_sensorhub11_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 12
*/
#define LSM6DSM_SENSORHUB12_REG 0x39U
struct lsm6dsm_sensorhub12_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    FIFO status register 1
*/
#define LSM6DSM_FIFO_STATUS1 0x3AU
struct lsm6dsm_fifo_status1 {
    uint8_t diff_fifo : 8;
};

/*
    FIFO status register 2
*/
#define LSM6DSM_FIFO_STATUS2 0x3BU
struct lsm6dsm_fifo_status2 {
    uint8_t diff_fifo : 3;
    uint8_t not_used_01 : 1;
    uint8_t fifo_empty : 1;
    uint8_t fifo_full_smart : 1;
    uint8_t over_run : 1;
    uint8_t waterm : 1;
};

/*
    FIFO status register 3
*/
#define LSM6DSM_FIFO_STATUS3 0x3CU
struct lsm6dsm_fifo_status3 {
    uint8_t fifo_pattern : 8;
};

/*
    FIFO status register 4
*/
#define LSM6DSM_FIFO_STATUS4 0x3DU
struct lsm6dsm_fifo_status4 {
    uint8_t fifo_pattern : 2;
    uint8_t not_used_01 : 6;
};

#define LSM6DSM_FIFO_DATA_OUT_L 0x3EU
#define LSM6DSM_FIFO_DATA_OUT_H 0x3FU
#define LSM6DSM_TIMESTAMP0_REG 0x40U
#define LSM6DSM_TIMESTAMP1_REG 0x41U
#define LSM6DSM_TIMESTAMP2_REG 0x42U
#define LSM6DSM_STEP_TIMESTAMP_L 0x49U
#define LSM6DSM_STEP_TIMESTAMP_H 0x4AU
#define LSM6DSM_STEP_COUNTER_L 0x4BU
#define LSM6DSM_STEP_COUNTER_H 0x4CU

/*
    Sensor hub register 13
*/
#define LSM6DSM_SENSORHUB13_REG 0x4DU
struct lsm6dsm_sensorhub13_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 14
*/
#define LSM6DSM_SENSORHUB14_REG 0x4EU
struct lsm6dsm_sensorhub14_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 15
*/
#define LSM6DSM_SENSORHUB15_REG 0x4FU
struct lsm6dsm_sensorhub15_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 16
*/
#define LSM6DSM_SENSORHUB16_REG 0x50U
struct lsm6dsm_sensorhub16_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 17
*/
#define LSM6DSM_SENSORHUB17_REG 0x51U
struct lsm6dsm_sensorhub17_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Sensor hub register 18
*/
#define LSM6DSM_SENSORHUB18_REG 0x52U
struct lsm6dsm_sensorhub18_reg {
    uint8_t bit0 : 1;
    uint8_t bit1 : 1;
    uint8_t bit2 : 1;
    uint8_t bit3 : 1;
    uint8_t bit4 : 1;
    uint8_t bit5 : 1;
    uint8_t bit6 : 1;
    uint8_t bit7 : 1;
};

/*
    Function source register 1
*/
#define LSM6DSM_FUNC_SRC1 0x53U
struct lsm6dsm_func_src1 {
    uint8_t sensorhub_end_op : 1;
    uint8_t si_end_op : 1;
    uint8_t hi_fail : 1;
    uint8_t step_overflow : 1;
    uint8_t step_detected : 1;
    uint8_t tilt_ia : 1;
    uint8_t sign_motion_ia : 1;
    uint8_t step_count_delta_ia : 1;
};

/*
    Function source register 2
*/
#define LSM6DSM_FUNC_SRC2 0x54U
struct lsm6dsm_func_src2 {
    uint8_t wrist_tilt_ia : 1;
    uint8_t not_used_01 : 2;
    uint8_t slave0_nack : 1;
    uint8_t slave1_nack : 1;
    uint8_t slave2_nack : 1;
    uint8_t slave3_nack : 1;
    uint8_t not_used_02 : 1;
};

/*
    Wrist tilt interrupt register
*/
#define LSM6DSM_WRIST_TILT_IA 0x55U
struct lsm6dsm_wrist_tilt_ia {
    uint8_t not_used_01 : 2;
    uint8_t wrist_tilt_ia_zneg : 1;
    uint8_t wrist_tilt_ia_zpos : 1;
    uint8_t wrist_tilt_ia_yneg : 1;
    uint8_t wrist_tilt_ia_ypos : 1;
    uint8_t wrist_tilt_ia_xneg : 1;
    uint8_t wrist_tilt_ia_xpos : 1;
};

/*
    Tap configuration register
*/
#define LSM6DSM_TAP_CFG 0x58U
struct lsm6dsm_tap_cfg {
    uint8_t lir : 1;
    uint8_t tap_z_en : 1;
    uint8_t tap_y_en : 1;
    uint8_t tap_x_en : 1;
    uint8_t slope_fds : 1;
    uint8_t inact_en : 2;
    uint8_t interrupts_enable : 1;
};

/*
    Tap threshold and 6D configuration register
*/
#define LSM6DSM_TAP_THS_6D 0x59U
struct lsm6dsm_tap_ths_6d {
    uint8_t tap_ths : 5;
    uint8_t sixd_ths : 2;
    uint8_t d4d_en : 1;
};

/*
    Interrupt duration register
*/
#define LSM6DSM_INT_DUR2 0x5AU
struct lsm6dsm_int_dur2 {
    uint8_t shock : 2;
    uint8_t quiet : 2;
    uint8_t dur : 4;
};

/*
    Wake-up threshold register
*/
#define LSM6DSM_WAKE_UP_THS 0x5BU
struct lsm6dsm_wake_up_ths {
    uint8_t wk_ths : 6;
    uint8_t not_used_01 : 1;
    uint8_t single_double_tap : 1;
};

/*
    Wake-up duration register
*/
#define LSM6DSM_WAKE_UP_DUR 0x5CU
struct lsm6dsm_wake_up_dur {
    uint8_t sleep_dur : 4;
    uint8_t timer_hr : 1;
    uint8_t wake_dur : 2;
    uint8_t ff_dur : 1;
};

/*
    Free-fall configuration register
*/
#define LSM6DSM_FREE_FALL 0x5DU
struct lsm6dsm_free_fall {
    uint8_t ff_ths : 3;
    uint8_t ff_dur : 5;
};

/*
    MD1 configuration register
*/
#define LSM6DSM_MD1_CFG 0x5EU
struct lsm6dsm_md1_cfg {
    uint8_t int1_timer : 1;
    uint8_t int1_tilt : 1;
    uint8_t int1_6d : 1;
    uint8_t int1_double_tap : 1;
    uint8_t int1_ff : 1;
    uint8_t int1_wu : 1;
    uint8_t int1_single_tap : 1;
    uint8_t int1_inact_state : 1;
};

/*
    MD2 configuration register
*/
#define LSM6DSM_MD2_CFG 0x5FU
struct lsm6dsm_md2_cfg {
    uint8_t int2_iron : 1;
    uint8_t int2_tilt : 1;
    uint8_t int2_6d : 1;
    uint8_t int2_double_tap : 1;
    uint8_t int2_ff : 1;
    uint8_t int2_wu : 1;
    uint8_t int2_single_tap : 1;
    uint8_t int2_inact_state : 1;
};

/*
    Master command code register
*/
#define LSM6DSM_MASTER_CMD_CODE 0x60U
struct lsm6dsm_master_cmd_code {
    uint8_t master_cmd_code : 8;
};

/*
    Sensor sync SPI error code register
*/
#define LSM6DSM_SENS_SYNC_SPI_ERROR_CODE 0x61U
struct lsm6dsm_sens_sync_spi_error_code {
    uint8_t error_code : 8;
};

#define LSM6DSM_OUT_MAG_RAW_X_L 0x66U
#define LSM6DSM_OUT_MAG_RAW_X_H 0x67U
#define LSM6DSM_OUT_MAG_RAW_Y_L 0x68U
#define LSM6DSM_OUT_MAG_RAW_Y_H 0x69U
#define LSM6DSM_OUT_MAG_RAW_Z_L 0x6AU
#define LSM6DSM_OUT_MAG_RAW_Z_H 0x6BU

/*
    OIS interrupt configuration register
*/
#define LSM6DSM_INT_OIS 0x6FU
struct lsm6dsm_int_ois {
    uint8_t not_used_01 : 6;
    uint8_t lvl2_ois : 1;
    uint8_t int2_drdy_ois : 1;
};

/*
    OIS control register 1
*/
#define LSM6DSM_CTRL1_OIS 0x70U
struct lsm6dsm_ctrl1_ois {
    uint8_t ois_en_spi2 : 1;
    uint8_t fs_g_ois : 3;
    uint8_t mode4_en : 1;
    uint8_t sim_ois : 1;
    uint8_t lvl1_ois : 1;
    uint8_t ble_ois : 1;
};

/*
    OIS control register 2
*/
#define LSM6DSM_CTRL2_OIS 0x71U
struct lsm6dsm_ctrl2_ois {
    uint8_t hp_en_ois : 1;
    uint8_t ftype_ois : 2;
    uint8_t not_used_01 : 1;
    uint8_t hpm_ois : 2;
    uint8_t not_used_02 : 2;
};

/*
    OIS control register 3
*/
#define LSM6DSM_CTRL3_OIS 0x72U
struct lsm6dsm_ctrl3_ois {
    uint8_t st_ois_clampdis : 1;
    uint8_t st_ois : 2;
    uint8_t filter_xl_conf_ois : 2;
    uint8_t fs_xl_ois : 2;
    uint8_t den_lh_ois : 1;
};

#define LSM6DSM_X_OFS_USR 0x73U
#define LSM6DSM_Y_OFS_USR 0x74U
#define LSM6DSM_Z_OFS_USR 0x75U

/*
    Slave 0 address configuration
*/
#define LSM6DSM_SLV0_ADD 0x02U
struct lsm6dsm_slv0_add {
    uint8_t rw_0 : 1;
    uint8_t slave0_add : 7;
};

/*
    Slave 0 subaddress configuration
*/
#define LSM6DSM_SLV0_SUBADD 0x03U
struct lsm6dsm_slv0_subadd {
    uint8_t slave0_reg : 8;
};

/*
    Slave 0 configuration register
*/
#define LSM6DSM_SLAVE0_CONFIG 0x04U
struct lsm6dsm_slave0_config {
    uint8_t slave0_numop : 3;
    uint8_t src_mode : 1;
    uint8_t aux_sens_on : 2;
    uint8_t slave0_rate : 2;
};

/*
    Slave 1 address configuration
*/
#define LSM6DSM_SLV1_ADD 0x05U
struct lsm6dsm_slv1_add {
    uint8_t r_1 : 1;
    uint8_t slave1_add : 7;
};

/*
    Slave 1 subaddress configuration
*/
#define LSM6DSM_SLV1_SUBADD 0x06U
struct lsm6dsm_slv1_subadd {
    uint8_t slave1_reg : 8;
};

/*
    Slave 1 configuration register
*/
#define LSM6DSM_SLAVE1_CONFIG 0x07U
struct lsm6dsm_slave1_config {
    uint8_t slave1_numop : 3;
    uint8_t not_used_01 : 2;
    uint8_t write_once : 1;
    uint8_t slave1_rate : 2;
};

/*
    Slave 2 address configuration
*/
#define LSM6DSM_SLV2_ADD 0x08U
struct lsm6dsm_slv2_add {
    uint8_t r_2 : 1;
    uint8_t slave2_add : 7;
};

/*
    Slave 2 subaddress configuration
*/
#define LSM6DSM_SLV2_SUBADD 0x09U
struct lsm6dsm_slv2_subadd {
    uint8_t slave2_reg : 8;
};

/*
    Slave 2 configuration register
*/
#define LSM6DSM_SLAVE2_CONFIG 0x0AU
struct lsm6dsm_slave2_config {
    uint8_t slave2_numop : 3;
    uint8_t not_used_01 : 3;
    uint8_t slave2_rate : 2;
};

/*
    Slave 3 address configuration
*/
#define LSM6DSM_SLV3_ADD 0x0BU
struct lsm6dsm_slv3_add {
    uint8_t r_3 : 1;
    uint8_t slave3_add : 7;
};

/*
    Slave 3 subaddress configuration
*/
#define LSM6DSM_SLV3_SUBADD 0x0CU
struct lsm6dsm_slv3_subadd {
    uint8_t slave3_reg : 8;
};

/*
    Slave 3 configuration register
*/
#define LSM6DSM_SLAVE3_CONFIG 0x0DU
struct lsm6dsm_slave3_config {
    uint8_t slave3_numop : 3;
    uint8_t not_used_01 : 3;
    uint8_t slave3_rate : 2;
};

/*
    Datawrite source mode sub-slave 0 configuration
*/
#define LSM6DSM_DATAWRITE_SRC_MODE_SUB_SLV0 0x0EU
struct lsm6dsm_datawrite_src_mode_sub_slv0 {
    uint8_t slave_dataw : 8;
};

/*
    Pedometer threshold minimum configuration
*/
#define LSM6DSM_CONFIG_PEDO_THS_MIN 0x0FU
struct lsm6dsm_config_pedo_ths_min {
    uint8_t ths_min : 5;
    uint8_t not_used_01 : 2;
    uint8_t pedo_fs : 1;
};

#define LSM6DSM_SM_THS 0x13U
#define LSM6DSM_PEDO_DEB_REG 0x14U

/*
    Pedometer debounce register
*/
struct lsm6dsm_pedo_deb_reg {
    uint8_t deb_step : 3;
    uint8_t deb_time : 5;
};

#define LSM6DSM_STEP_COUNT_DELTA 0x15U
#define LSM6DSM_MAG_SI_XX 0x24U
#define LSM6DSM_MAG_SI_XY 0x25U
#define LSM6DSM_MAG_SI_XZ 0x26U
#define LSM6DSM_MAG_SI_YX 0x27U
#define LSM6DSM_MAG_SI_YY 0x28U
#define LSM6DSM_MAG_SI_YZ 0x29U
#define LSM6DSM_MAG_SI_ZX 0x2AU
#define LSM6DSM_MAG_SI_ZY 0x2BU
#define LSM6DSM_MAG_SI_ZZ 0x2CU
#define LSM6DSM_MAG_OFFX_L 0x2DU
#define LSM6DSM_MAG_OFFX_H 0x2EU
#define LSM6DSM_MAG_OFFY_L 0x2FU
#define LSM6DSM_MAG_OFFY_H 0x30U
#define LSM6DSM_MAG_OFFZ_L 0x31U
#define LSM6DSM_MAG_OFFZ_H 0x32U
#define LSM6DSM_A_WRIST_TILT_LAT 0x50U
#define LSM6DSM_A_WRIST_TILT_THS 0x54U
#define LSM6DSM_A_WRIST_TILT_MASK 0x59U

/*
    Wrist tilt mask configuration
*/
struct lsm6dsm_a_wrist_tilt_mask {
    uint8_t not_used_01 : 2;
    uint8_t wrist_tilt_mask_zneg : 1;
    uint8_t wrist_tilt_mask_zpos : 1;
    uint8_t wrist_tilt_mask_yneg : 1;
    uint8_t wrist_tilt_mask_ypos : 1;
    uint8_t wrist_tilt_mask_xneg : 1;
    uint8_t wrist_tilt_mask_xpos : 1;
};

/*
    LSM6DSM register union
*/
union lsm6dsm_reg {
    struct lsm6dsm_func_cfg_access func_cfg_access;
    struct lsm6dsm_sensor_sync_time_frame sensor_sync_time_frame;
    struct lsm6dsm_sensor_sync_res_ratio sensor_sync_res_ratio;
    struct lsm6dsm_fifo_ctrl1 fifo_ctrl1;
    struct lsm6dsm_fifo_ctrl2 fifo_ctrl2;
    struct lsm6dsm_fifo_ctrl3 fifo_ctrl3;
    struct lsm6dsm_fifo_ctrl4 fifo_ctrl4;
    struct lsm6dsm_fifo_ctrl5 fifo_ctrl5;
    struct lsm6dsm_drdy_pulse_cfg drdy_pulse_cfg;
    struct lsm6dsm_int1_ctrl int1_ctrl;
    struct lsm6dsm_int2_ctrl int2_ctrl;
    struct lsm6dsm_ctrl1_xl ctrl1_xl;
    struct lsm6dsm_ctrl2_g ctrl2_g;
    struct lsm6dsm_ctrl3_c ctrl3_c;
    struct lsm6dsm_ctrl4_c ctrl4_c;
    struct lsm6dsm_ctrl5_c ctrl5_c;
    struct lsm6dsm_ctrl6_c ctrl6_c;
    struct lsm6dsm_ctrl7_g ctrl7_g;
    struct lsm6dsm_ctrl8_xl ctrl8_xl;
    struct lsm6dsm_ctrl9_xl ctrl9_xl;
    struct lsm6dsm_ctrl10_c ctrl10_c;
    struct lsm6dsm_master_config master_config;
    struct lsm6dsm_wake_up_src wake_up_src;
    struct lsm6dsm_tap_src tap_src;
    struct lsm6dsm_d6d_src d6d_src;
    struct lsm6dsm_status_reg status_reg;
    struct lsm6dsm_status_spiaux status_spiaux;
    struct lsm6dsm_sensorhub1_reg sensorhub1_reg;
    struct lsm6dsm_sensorhub2_reg sensorhub2_reg;
    struct lsm6dsm_sensorhub3_reg sensorhub3_reg;
    struct lsm6dsm_sensorhub4_reg sensorhub4_reg;
    struct lsm6dsm_sensorhub5_reg sensorhub5_reg;
    struct lsm6dsm_sensorhub6_reg sensorhub6_reg;
    struct lsm6dsm_sensorhub7_reg sensorhub7_reg;
    struct lsm6dsm_sensorhub8_reg sensorhub8_reg;
    struct lsm6dsm_sensorhub9_reg sensorhub9_reg;
    struct lsm6dsm_sensorhub10_reg sensorhub10_reg;
    struct lsm6dsm_sensorhub11_reg sensorhub11_reg;
    struct lsm6dsm_sensorhub12_reg sensorhub12_reg;
    struct lsm6dsm_fifo_status1 fifo_status1;
    struct lsm6dsm_fifo_status2 fifo_status2;
    struct lsm6dsm_fifo_status3 fifo_status3;
    struct lsm6dsm_fifo_status4 fifo_status4;
    struct lsm6dsm_sensorhub13_reg sensorhub13_reg;
    struct lsm6dsm_sensorhub14_reg sensorhub14_reg;
    struct lsm6dsm_sensorhub15_reg sensorhub15_reg;
    struct lsm6dsm_sensorhub16_reg sensorhub16_reg;
    struct lsm6dsm_sensorhub17_reg sensorhub17_reg;
    struct lsm6dsm_sensorhub18_reg sensorhub18_reg;
    struct lsm6dsm_func_src1 func_src1;
    struct lsm6dsm_func_src2 func_src2;
    struct lsm6dsm_wrist_tilt_ia wrist_tilt_ia;
    struct lsm6dsm_tap_cfg tap_cfg;
    struct lsm6dsm_tap_ths_6d tap_ths_6d;
    struct lsm6dsm_int_dur2 int_dur2;
    struct lsm6dsm_wake_up_ths wake_up_ths;
    struct lsm6dsm_wake_up_dur wake_up_dur;
    struct lsm6dsm_free_fall free_fall;
    struct lsm6dsm_md1_cfg md1_cfg;
    struct lsm6dsm_md2_cfg md2_cfg;
    struct lsm6dsm_master_cmd_code master_cmd_code;
    struct lsm6dsm_sens_sync_spi_error_code sens_sync_spi_error_code;
    struct lsm6dsm_int_ois int_ois;
    struct lsm6dsm_ctrl1_ois ctrl1_ois;
    struct lsm6dsm_ctrl2_ois ctrl2_ois;
    struct lsm6dsm_ctrl3_ois ctrl3_ois;
    struct lsm6dsm_slv0_add slv0_add;
    struct lsm6dsm_slv0_subadd slv0_subadd;
    struct lsm6dsm_slave0_config slave0_config;
    struct lsm6dsm_slv1_add slv1_add;
    struct lsm6dsm_slv1_subadd slv1_subadd;
    struct lsm6dsm_slave1_config slave1_config;
    struct lsm6dsm_slv2_add slv2_add;
    struct lsm6dsm_slv2_subadd slv2_subadd;
    struct lsm6dsm_slave2_config slave2_config;
    struct lsm6dsm_slv3_add slv3_add;
    struct lsm6dsm_slv3_subadd slv3_subadd;
    struct lsm6dsm_slave3_config slave3_config;
    struct lsm6dsm_datawrite_src_mode_sub_slv0 datawrite_src_mode_sub_slv0;
    struct lsm6dsm_config_pedo_ths_min config_pedo_ths_min;
    struct lsm6dsm_pedo_deb_reg pedo_deb_reg;
    struct lsm6dsm_a_wrist_tilt_mask a_wrist_tilt_mask;
    uint8_t byte;
};

enum lsm6dsm_fs_xl {
    LSM6DSM_2g = 0,
    LSM6DSM_16g = 1,
    LSM6DSM_4g = 2,
    LSM6DSM_8g = 3,
};

static const float lsm6dsm_fs_xl_value_mg[] = {
    [LSM6DSM_2g] = 2000.0f,
    [LSM6DSM_4g] = 4000.0f,
    [LSM6DSM_8g] = 8000.0f,
    [LSM6DSM_16g] = 16000.0f,
};

enum lsm6dsm_odr_xl {
    LSM6DSM_XL_ODR_OFF = 0,
    LSM6DSM_XL_ODR_12Hz5 = 1,
    LSM6DSM_XL_ODR_26Hz = 2,
    LSM6DSM_XL_ODR_52Hz = 3,
    LSM6DSM_XL_ODR_104Hz = 4,
    LSM6DSM_XL_ODR_208Hz = 5,
    LSM6DSM_XL_ODR_416Hz = 6,
    LSM6DSM_XL_ODR_833Hz = 7,
    LSM6DSM_XL_ODR_1k66Hz = 8,
    LSM6DSM_XL_ODR_3k33Hz = 9,
    LSM6DSM_XL_ODR_6k66Hz = 10,
    LSM6DSM_XL_ODR_1Hz6 = 11,
};

static const float lsm6dsm_odr_xl_value_hz[] = {
    [LSM6DSM_XL_ODR_OFF] = 0.0f,       [LSM6DSM_XL_ODR_1Hz6] = 1.6f,
    [LSM6DSM_XL_ODR_12Hz5] = 12.5f,    [LSM6DSM_XL_ODR_26Hz] = 26.0f,
    [LSM6DSM_XL_ODR_52Hz] = 52.0f,     [LSM6DSM_XL_ODR_104Hz] = 104.0f,
    [LSM6DSM_XL_ODR_208Hz] = 208.0f,   [LSM6DSM_XL_ODR_416Hz] = 416.0f,
    [LSM6DSM_XL_ODR_833Hz] = 833.0f,   [LSM6DSM_XL_ODR_1k66Hz] = 1660.0f,
    [LSM6DSM_XL_ODR_3k33Hz] = 3330.0f, [LSM6DSM_XL_ODR_6k66Hz] = 6660.0f,
};

static const float lsm6dsm_odr_xl_value_ms[] = {
    [LSM6DSM_XL_ODR_OFF] = 0.0f,
    [LSM6DSM_XL_ODR_1Hz6] = 1000.0 / 1.6f,
    [LSM6DSM_XL_ODR_12Hz5] = 1000.0 / 12.5f,
    [LSM6DSM_XL_ODR_26Hz] = 1000.0 / 26.0f,
    [LSM6DSM_XL_ODR_52Hz] = 1000.0 / 52.0f,
    [LSM6DSM_XL_ODR_104Hz] = 1000.0 / 104.0f,
    [LSM6DSM_XL_ODR_208Hz] = 1000.0 / 208.0f,
    [LSM6DSM_XL_ODR_416Hz] = 1000.0 / 416.0f,
    [LSM6DSM_XL_ODR_833Hz] = 1000.0 / 833.0f,
    [LSM6DSM_XL_ODR_1k66Hz] = 1000.0 / 1660.0f,
    [LSM6DSM_XL_ODR_3k33Hz] = 1000.0 / 3330.0f,
    [LSM6DSM_XL_ODR_6k66Hz] = 1000.0 / 6660.0f,
};

enum lsm6dsm_fs_g {
    LSM6DSM_250dps = 0,
    LSM6DSM_125dps = 1,
    LSM6DSM_500dps = 2,
    LSM6DSM_1000dps = 4,
    LSM6DSM_2000dps = 6,
};

enum lsm6dsm_odr_g {
    LSM6DSM_GY_ODR_OFF = 0,
    LSM6DSM_GY_ODR_12Hz5 = 1,
    LSM6DSM_GY_ODR_26Hz = 2,
    LSM6DSM_GY_ODR_52Hz = 3,
    LSM6DSM_GY_ODR_104Hz = 4,
    LSM6DSM_GY_ODR_208Hz = 5,
    LSM6DSM_GY_ODR_416Hz = 6,
    LSM6DSM_GY_ODR_833Hz = 7,
    LSM6DSM_GY_ODR_1k66Hz = 8,
    LSM6DSM_GY_ODR_3k33Hz = 9,
    LSM6DSM_GY_ODR_6k66Hz = 10,
};

enum lsm6dsm_usr_off_w {
    LSM6DSM_LSb_1mg = 0,
    LSM6DSM_LSb_16mg = 1,
};

enum lsm6dsm_xl_hm_mode {
    LSM6DSM_XL_HIGH_PERFORMANCE = 0,
    LSM6DSM_XL_NORMAL = 1,
};

enum lsm6dsm_rounding_status {
    LSM6DSM_STAT_RND_DISABLE = 0,
    LSM6DSM_STAT_RND_ENABLE = 1,
};

enum lsm6dsm_g_hm_mode {
    LSM6DSM_GY_HIGH_PERFORMANCE = 0,
    LSM6DSM_GY_NORMAL = 1,
};

struct lsm6dsm_all_sources {
    struct lsm6dsm_wake_up_src wake_up_src;
    struct lsm6dsm_tap_src tap_src;
    struct lsm6dsm_d6d_src d6d_src;
    struct lsm6dsm_status_reg status_reg;
    struct lsm6dsm_func_src1 func_src1;
    struct lsm6dsm_func_src2 func_src2;
    struct lsm6dsm_wrist_tilt_ia wrist_tilt_ia;
    struct lsm6dsm_a_wrist_tilt_mask a_wrist_tilt_mask;
};

enum lsm6dsm_rounding {
    LSM6DSM_ROUND_DISABLE = 0,
    LSM6DSM_ROUND_XL = 1,
    LSM6DSM_ROUND_GY = 2,
    LSM6DSM_ROUND_GY_XL = 3,
    LSM6DSM_ROUND_SH1_TO_SH6 = 4,
    LSM6DSM_ROUND_XL_SH1_TO_SH6 = 5,
    LSM6DSM_ROUND_GY_XL_SH1_TO_SH12 = 6,
    LSM6DSM_ROUND_GY_XL_SH1_TO_SH6 = 7,
};

enum lsm6dsm_func_cfg_en {
    LSM6DSM_USER_BANK = 0,
    LSM6DSM_BANK_A = 4,
    LSM6DSM_BANK_B = 5,
};

enum lsm6dsm_drdy_pulsed_g {
    LSM6DSM_DRDY_LATCHED = 0,
    LSM6DSM_DRDY_PULSED = 1,
};

enum lsm6dsm_ble {
    LSM6DSM_LSB_AT_LOW_ADD = 0,
    LSM6DSM_MSB_AT_LOW_ADD = 1,
};

enum lsm6dsm_slope_fds {
    LSM6DSM_USE_SLOPE = 0,
    LSM6DSM_USE_HPF = 1,
};

enum lsm6dsm_bw0_xl {
    LSM6DSM_XL_ANA_BW_1k5Hz = 0,
    LSM6DSM_XL_ANA_BW_400Hz = 1,
};

enum lsm6dsm_lpf1_bw_sel {
    LSM6DSM_XL_LP1_ODR_DIV_2 = 0,
    LSM6DSM_XL_LP1_ODR_DIV_4 = 1,
    LSM6DSM_XL_LP1_NOT_AVAILABLE = 2,
};

enum lsm6dsm_input_composite {
    LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_50 = 0x00,
    LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_100 = 0x01,
    LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_9 = 0x02,
    LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_400 = 0x03,
    LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_50 = 0x10,
    LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_100 = 0x11,
    LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_9 = 0x12,
    LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_400 = 0x13,
    LSM6DSM_XL_LP_NOT_AVAILABLE = 0x20,
};

enum lsm6dsm_hpcf_xl {
    LSM6DSM_XL_HP_ODR_DIV_4 = 0x00,
    LSM6DSM_XL_HP_ODR_DIV_100 = 0x01,
    LSM6DSM_XL_HP_ODR_DIV_9 = 0x02,
    LSM6DSM_XL_HP_ODR_DIV_400 = 0x03,
    LSM6DSM_XL_HP_NOT_AVAILABLE = 0x10,
};

enum lsm6dsm_ui_lpf1_bw_sel {
    LSM6DSM_XL_UI_LP1_ODR_DIV_2 = 0,
    LSM6DSM_XL_UI_LP1_ODR_DIV_4 = 1,
    LSM6DSM_XL_UI_LP1_NOT_AVAILABLE = 2,
};

enum lsm6dsm_lpf1_sel_g {
    LSM6DSM_LP2_ONLY = 0x00,

    LSM6DSM_HP_16mHz_LP2 = 0x80,
    LSM6DSM_HP_65mHz_LP2 = 0x90,
    LSM6DSM_HP_260mHz_LP2 = 0xA0,
    LSM6DSM_HP_1Hz04_LP2 = 0xB0,

    LSM6DSM_HP_DISABLE_LP1_LIGHT = 0x0A,
    LSM6DSM_HP_DISABLE_LP1_NORMAL = 0x09,
    LSM6DSM_HP_DISABLE_LP_STRONG = 0x08,
    LSM6DSM_HP_DISABLE_LP1_AGGRESSIVE = 0x0B,

    LSM6DSM_HP_16mHz_LP1_LIGHT = 0x8A,
    LSM6DSM_HP_65mHz_LP1_NORMAL = 0x99,
    LSM6DSM_HP_260mHz_LP1_STRONG = 0xA8,
    LSM6DSM_HP_1Hz04_LP1_AGGRESSIVE = 0xBB,
};

enum lsm6dsm_hp_en_ois {
    LSM6DSM_HP_DISABLE_LP_173Hz = 0x02,
    LSM6DSM_HP_DISABLE_LP_237Hz = 0x01,
    LSM6DSM_HP_DISABLE_LP_351Hz = 0x00,
    LSM6DSM_HP_DISABLE_LP_937Hz = 0x03,

    LSM6DSM_HP_16mHz_LP_173Hz = 0x82,
    LSM6DSM_HP_65mHz_LP_237Hz = 0x91,
    LSM6DSM_HP_260mHz_LP_351Hz = 0xA0,
    LSM6DSM_HP_1Hz04_LP_937Hz = 0xB3,
};

enum lsm6dsm_i2c_disable {
    LSM6DSM_I2C_ENABLE = 0,
    LSM6DSM_I2C_DISABLE = 1,
};

struct lsm6dsm_int1_route {
    uint8_t int1_drdy_xl : 1;
    uint8_t int1_drdy_g : 1;
    uint8_t int1_boot : 1;
    uint8_t int1_fth : 1;
    uint8_t int1_fifo_ovr : 1;
    uint8_t int1_full_flag : 1;
    uint8_t int1_sign_mot : 1;
    uint8_t int1_step_detector : 1;
    uint8_t int1_timer : 1;
    uint8_t int1_tilt : 1;
    uint8_t int1_6d : 1;
    uint8_t int1_double_tap : 1;
    uint8_t int1_ff : 1;
    uint8_t int1_wu : 1;
    uint8_t int1_single_tap : 1;
    uint8_t int1_inact_state : 1;
    uint8_t den_drdy_int1 : 1;
    uint8_t drdy_on_int1 : 1;
};

struct lsm6dsm_int2_route {
    uint8_t int2_drdy_xl : 1;
    uint8_t int2_drdy_g : 1;
    uint8_t int2_drdy_temp : 1;
    uint8_t int2_fth : 1;
    uint8_t int2_fifo_ovr : 1;
    uint8_t int2_full_flag : 1;
    uint8_t int2_step_count_ov : 1;
    uint8_t int2_step_delta : 1;
    uint8_t int2_iron : 1;
    uint8_t int2_tilt : 1;
    uint8_t int2_6d : 1;
    uint8_t int2_double_tap : 1;
    uint8_t int2_ff : 1;
    uint8_t int2_wu : 1;
    uint8_t int2_single_tap : 1;
    uint8_t int2_inact_state : 1;
    uint8_t int2_wrist_tilt : 1;
};

enum lsm6dsm_pp_od {
    LSM6DSM_PUSH_PULL = 0,
    LSM6DSM_OPEN_DRAIN = 1,
};

enum lsm6dsm_h_lactive {
    LSM6DSM_ACTIVE_HIGH = 0,
    LSM6DSM_ACTIVE_LOW = 1,
};

enum lsm6dsm_lir {
    LSM6DSM_INT_PULSED = 0,
    LSM6DSM_INT_LATCHED = 1,
};

enum lsm6dsm_inact_en {
    LSM6DSM_PROPERTY_DISABLE = 0,
    LSM6DSM_XL_12Hz5_GY_NOT_AFFECTED = 1,
    LSM6DSM_XL_12Hz5_GY_SLEEP = 2,
    LSM6DSM_XL_12Hz5_GY_PD = 3,
};

enum lsm6dsm_single_double_tap {
    LSM6DSM_ONLY_SINGLE = 0,
    LSM6DSM_BOTH_SINGLE_DOUBLE = 1,
};

#endif
