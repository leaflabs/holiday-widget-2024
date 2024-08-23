#ifndef __LSM6DSM_DRIVER_H__
#define __LSM6DSM_DRIVER_H__
#include <stdbool.h>
#include <stdint.h>

enum lsm6dsm_state {
    LSM6DSM_PRE_INIT,
    LSM6DSM_READY,
    LSM6DSM_PENDING,
    LSM6DSM_ERROR
};

enum lsm6dsm_it_state {
    LSM6DSM_INTERRUPT_CLEAR = 0,
    LSM6DSM_INTERRUPT_TRIGGERED = 1,
    LSM6DSM_INTERRUPT_GET_SOURCE = 2,
};

/* Forward declaration of opaque data structures */
struct lsm6dsm_driver_config;
struct lsm6dsm_driver_context;

struct lsm6dsm_driver {
    const struct lsm6dsm_driver_config *const config;
    struct lsm6dsm_driver_context *const context;
};

typedef struct {
    uint8_t not_used_01 : 2;
    uint8_t wrist_tilt_ia_zneg : 1;
    uint8_t wrist_tilt_ia_zpos : 1;
    uint8_t wrist_tilt_ia_yneg : 1;
    uint8_t wrist_tilt_ia_ypos : 1;
    uint8_t wrist_tilt_ia_xneg : 1;
    uint8_t wrist_tilt_ia_xpos : 1;
} tilt_flags;

typedef struct {
    uint8_t z_tap : 1;
    uint8_t y_tap : 1;
    uint8_t x_tap : 1;
    uint8_t tap_sign : 1;
    uint8_t double_tap : 1;
    uint8_t single_tap : 1;
    uint8_t tap_ia : 1;
    uint8_t not_used_01 : 1;
} tap_flags;

int lsm6dsm_driver_init(const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_set_int1(const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_set_int2(const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_clear_int1(const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_clear_int2(const struct lsm6dsm_driver *const dev);

bool lsm6dsm_driver_get_int1(const struct lsm6dsm_driver *const dev);

bool lsm6dsm_driver_get_int2(const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_request_acceleration(const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_request_angular_rate(const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_request_tilt_it_source(
    const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_request_it_source(const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_request_tap_it_source(
    const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_get_acceleration_request_status(
    const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_get_angular_rate_request_status(
    const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_get_tilt_it_source_request_status(
    const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_get_it_source_request_status(
    const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_get_tap_it_source_request_status(
    const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_process_acceleration(
    const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_process_angular_rate(
    const struct lsm6dsm_driver *const dev);

bool lsm6dsm_driver_it_source_is_tilt(const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_process_tilt_it_source(
    const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_process_tap_it_source(
    const struct lsm6dsm_driver *const dev);

float lsm6dsm_driver_get_x_acc(const struct lsm6dsm_driver *const dev);
float lsm6dsm_driver_get_y_acc(const struct lsm6dsm_driver *const dev);
float lsm6dsm_driver_get_z_acc(const struct lsm6dsm_driver *const dev);

float lsm6dsm_driver_get_x_ang(const struct lsm6dsm_driver *const dev);
float lsm6dsm_driver_get_y_ang(const struct lsm6dsm_driver *const dev);
float lsm6dsm_driver_get_z_ang(const struct lsm6dsm_driver *const dev);

enum lsm6dsm_state lsm6dsm_driver_get_state(
    const struct lsm6dsm_driver *const dev);
void lsm6dsm_driver_set_state(const struct lsm6dsm_driver *const dev,
                              enum lsm6dsm_state state);

enum lsm6dsm_it_state lsm6dsm_driver_get_it_state(
    const struct lsm6dsm_driver *const dev);
void lsm6dsm_driver_set_it_state(const struct lsm6dsm_driver *const dev,
                                 enum lsm6dsm_it_state it_state);

tilt_flags lsm6dsm_driver_get_tilt_flags(
    const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_clear_tilt_flags(const struct lsm6dsm_driver *const dev);

tap_flags lsm6dsm_driver_get_tap_flags(const struct lsm6dsm_driver *const dev);

void lsm6dsm_driver_clear_tap_flags(const struct lsm6dsm_driver *const dev);

int lsm6dsm_driver_enter_low_power_mode(const struct lsm6dsm_driver *const dev);
int lsm6dsm_driver_exit_low_power_mode(const struct lsm6dsm_driver *const dev);

extern const struct lsm6dsm_driver *const lsm6dsm;

#endif /*__LSM6DSM_DRIVER_H__*/