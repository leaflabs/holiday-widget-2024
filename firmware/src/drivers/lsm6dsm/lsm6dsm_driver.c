#include "lsm6dsm_driver.h"

#include <errno.h>

#include "arm_math.h"
#include "futures.h"
#include "generic_gpio.h"
#include "i2c_driver.h"
#include "logging.h"
#include "lsm6dsm_registers.h"
#include "system_communication.h"
#include "utils.h"

#define LSM6DSM_MAX_I2C_SIZE 6U

/* Converts the given value in degrees to radians */
#define DEGREES_TO_RADIANS(__degrees__) ((__degrees__ * PI) / 180.0f)

/*
    Converts the given threshold in radians to the nearest
    equivalent valid LSM6DSM A_WRIST_TILT_THS register value.
    If an invalid threshold is provided, evaluates to the default
    register value of 0x20 (π/4 radians).
*/
#define LSM6DSM_WT_THRESHOLD_FROM_RADIANS(__radians__) \
    (__radians__ >= 0U && __radians__ < (PI / 2.0f)    \
         ? (uint8_t)(arm_sin_f32(__radians__) * 64.0f) \
         : 0x20)

/*
    Converts the given threshold in degrees to the nearest
    equivalent valid LSM6DSM A_WRIST_TILT_THS register value.
    If an invalid threshold is provided, evaluates to the default
    register value of 0x20 (30 degrees).
*/
#define LSM6DSM_WT_THRESHOLD_FROM_DEGREES(__degrees__) \
    (LSM6DSM_WT_THRESHOLD_FROM_RADIANS(DEGREES_TO_RADIANS(__degrees__)))

/*
    Converts the given latency in milliseconds to the nearest
    equivalent valid LSM6DSM A_WRIST_TILT_LAT register value.
    To avoid truncation error from integer division, choose
    latency values that are integer multiples of 40 milliseconds.
    If an invalid latency is provided, evaluates to the default
    register value of 0x0F (600 milliseconds).
*/
#define LSM6DSM_WT_LATENCY_FROM_MS(__ms__)                       \
    (__ms__ >= 0U && __ms__ <= 10200U ? (uint8_t)(__ms__ / 40.0) \
                                      : (uint8_t)0x0F)

/*
    Converts the given threshold in milli-g's to the nearest
    equivalent valid WK_THS field value of LSM6DSM WAKE_UP_THS register.
    If an invalid threshold is provided, evaluates to arbitrary default
    value of 0x01 (FS_XL/64 g).
*/
/* clang-format off */
#define LSM6DSM_WK_THRESHOLD_FROM_MG(__mg__, __fs_xl__)                     \
    (__fs_xl__ >= LSM6DSM_2g                                                \
    && __fs_xl__ <= LSM6DSM_8g                                              \
    && __mg__ >= 0.0f                                                       \
    && __mg__ <= lsm6dsm_fs_xl_value_mg[__fs_xl__]                          \
    ? (uint8_t)((__mg__ * 64.0) / lsm6dsm_fs_xl_value_mg[__fs_xl__])        \
    : (uint8_t)0x01)
/* clang-format on */

/*
    Converts the given sleep duration in ms to the nearest
    equivalent valid SLEEP_DUR field value of LSM6DSM WAKE_UP_DUR register.
    If an invalid threshold is provided, evaluates to arbitrary default
    value of 0x01 (512/ODR_XL g).
*/
/* clang-format off */
#define LSM6DSM_WK_DURATION_FROM_MS(__ms__, __odr_xl__)                 \
    (__odr_xl__ >= LSM6DSM_XL_ODR_OFF                                   \
    && __odr_xl__ <= LSM6DSM_XL_ODR_1Hz6                                \
    && __ms__ >= 0.0f                                                   \
    && __ms__ <= 15.0f * 512.0f * lsm6dsm_odr_xl_value_ms[__odr_xl__]   \
    ? (uint8_t)(__ms__/(lsm6dsm_odr_xl_value_ms[__odr_xl__]*512.0f))    \
    : (uint8_t)0x01)
/* clang-format on */

/*
    Converts the given threshold in milli-g's to the nearest
    equivalent valid TAP_THS field value of LSM6DSM TAP_THS_6D register.
    If an invalid threshold is provided, evaluates to arbitrary default
    value of 0x01 (FS_XL/32 g).
*/
/* clang-format off */
#define LSM6DSM_TAP_THRESHOLD_FROM_MG(__mg__, __fs_xl__)                    \
    (__fs_xl__ >= LSM6DSM_2g                                                \
    && __fs_xl__ <= LSM6DSM_8g                                              \
    && __mg__ >= lsm6dsm_fs_xl_value_mg[__fs_xl__]/32.0f                    \
    && __mg__ <= lsm6dsm_fs_xl_value_mg[__fs_xl__]                          \
    ? (uint8_t)((__mg__ * 32.0f) / lsm6dsm_fs_xl_value_mg[__fs_xl__])       \
    : (uint8_t)0x01)
/* clang-format on */

/*
    Converts the given duration in milliseconds to the nearest
    equivalent valid DUR field value of LSM6DSM INT_DUR2 register.
    If an invalid duration is provided, evaluates to default
    value of 0x00 (32/ODR_XL g; 16/ODR_XL g if 0x00).
*/
/* clang-format off */
#define LSM6DSM_TAP_DURATION_FROM_MS(__ms__, __odr_xl__)                   \
    (__odr_xl__ >= LSM6DSM_XL_ODR_OFF                                      \
    && __odr_xl__ <= LSM6DSM_XL_ODR_1Hz6                                   \
    && __ms__ >= 32.0f * lsm6dsm_odr_xl_value_ms[__odr_xl__]               \
    && __ms__ <= (0b1111 * 32.0f * lsm6dsm_odr_xl_value_ms[__odr_xl__])    \
    ? (uint8_t)(__ms__/(lsm6dsm_odr_xl_value_ms[__odr_xl__]*32.0f))        \
    : (uint8_t)0x00)
/* clang-format on */

/*
    Converts the given quiet time in milliseconds to the nearest
    equivalent valid QUIET field value of LSM6DSM INT_DUR2 register.
    If an invalid quiet time is provided, evaluates to default
    value of 0x00 (4/ODR_XL g; 2/ODR_XL g if 0x00).
*/
/* clang-format off */
#define LSM6DSM_TAP_QUIET_FROM_MS(__ms__, __odr_xl__)                  \
    (__odr_xl__ >= LSM6DSM_XL_ODR_OFF                                  \
    && __odr_xl__ <= LSM6DSM_XL_ODR_1Hz6                               \
    && __ms__ >= 4.0f * lsm6dsm_odr_xl_value_ms[__odr_xl__]            \
    && __ms__ <= (0b11 * 4.0f * lsm6dsm_odr_xl_value_ms[__odr_xl__])   \
    ? (uint8_t)(__ms__/(lsm6dsm_odr_xl_value_ms[__odr_xl__]*4.0f))     \
    : (uint8_t)0x00)
/* clang-format on */

/*
    Converts the given shock time in milliseconds to the nearest
    equivalent valid SHOCK field value of LSM6DSM INT_DUR2 register.
    If an invalid shock time is provided, evaluates to default
    value of 0x00 (8/ODR_XL g; 4/ODR_XL g if 0x00).
*/
/* clang-format off */
#define LSM6DSM_TAP_SHOCK_FROM_MS(__ms__, __odr_xl__)                  \
    (__odr_xl__ >= LSM6DSM_XL_ODR_OFF                                  \
    && __odr_xl__ <= LSM6DSM_XL_ODR_1Hz6                               \
    && __ms__ >= 8.0f * lsm6dsm_odr_xl_value_ms[__odr_xl__]            \
    && __ms__ <= (0b11 * 8.0f * lsm6dsm_odr_xl_value_ms[__odr_xl__])   \
    ? (uint8_t)(__ms__/(lsm6dsm_odr_xl_value_ms[__odr_xl__]*8.0f))     \
    : (uint8_t)0x00)
/* clang-format on */

/* LSM6DSM Driver Configuration Structure */
struct lsm6dsm_driver_config {
    struct accelerometer_config {
        enum lsm6dsm_fs_xl data_scale;
        enum lsm6dsm_odr_xl data_rate;
        enum lsm6dsm_bw0_xl analog_bandwidth;
        enum lsm6dsm_input_composite low_pass_filter;
    } accelerometer;

    struct gyroscope_config {
        enum lsm6dsm_fs_g data_scale;
        enum lsm6dsm_odr_g data_rate;
        enum lsm6dsm_lpf1_sel_g high_pass_filter;
    } gyroscope;

    struct tilt_config {
        struct lsm6dsm_a_wrist_tilt_mask source;
        float threshold_deg;
        uint16_t latency_ms;
    } tilt;

    struct tap_config {
        bool tap_x, tap_y, tap_z;
        float threshold_mg;
        uint16_t dur_ms;
        uint16_t quite_ms;
        uint16_t shock_ms;
    } tap;

    struct wakeup_config {
        enum lsm6dsm_inact_en action;
        float threshold_mg;
        uint16_t sleep_dur_ms;
    } wakeup;

    struct gpio_pin_config int1_pin;
    struct gpio_pin_config int2_pin;
};

/* LSM6DSM Driver Context Structure */
struct lsm6dsm_driver_context {
    union lsm6dsm_reg i2c_transaction_buffer[LSM6DSM_MAX_I2C_SIZE];
    struct lsm6dsm_wrist_tilt_ia wrist_tilt_flags;
    struct lsm6dsm_tap_src single_tap_flags;
    struct lsm6dsm_func_src2 it_source;

    struct i2c_driver_context *i2c_context;
    struct i2c_request request;
    struct i2c_request tilt_it_source_request;
    struct i2c_request it_source_request;
    struct i2c_request tap_it_source_request;

    enum lsm6dsm_state state;
    enum lsm6dsm_it_state it_state;

    /* Accelerometer Data */
    float x_acc, y_acc, z_acc;

    /* Gyroscope Data */
    float x_ang, y_ang, z_ang;

    volatile bool int1_flag, int2_flag;

    float (*acc_conversion)(int16_t lsb);
    float (*ang_conversion)(int16_t lsb);

    tilt_flags tilt_flags;
    tap_flags tap_flags;
};

static const struct lsm6dsm_driver_config lsm6dsm_config = {
    .accelerometer =
        {
            .data_scale = LSM6DSM_2g,
            .data_rate = LSM6DSM_XL_ODR_208Hz,
            .analog_bandwidth = LSM6DSM_XL_ANA_BW_400Hz,
            .low_pass_filter = LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_100,
        },
    .gyroscope =
        {
            .data_scale = LSM6DSM_250dps,
            .data_rate = LSM6DSM_GY_ODR_52Hz,
            .high_pass_filter = LSM6DSM_HP_260mHz_LP1_STRONG,
        },
    .tilt =
        {
            .source =
                {
                    .wrist_tilt_mask_xneg = ENABLE,
                    .wrist_tilt_mask_xpos = ENABLE,
                    .wrist_tilt_mask_yneg = ENABLE,
                    .wrist_tilt_mask_ypos = ENABLE,
                    .wrist_tilt_mask_zneg = DISABLE,
                    .wrist_tilt_mask_zpos = DISABLE,
                },
            .threshold_deg = 15.0,
            .latency_ms = 560,
        },

    .tap =
        {
            .tap_x = DISABLE,
            .tap_y = ENABLE,
            .tap_z = DISABLE,
            .threshold_mg = 180.0f,  // 156.25f,
            .dur_ms = 770,
            .quite_ms = 10,
            .shock_ms = 39,
        },

    .wakeup =
        {
            .action = LSM6DSM_XL_12Hz5_GY_PD,
            .threshold_mg = 70.0,
            .sleep_dur_ms = 36900.0,
        },

    .int1_pin = GPIO_PIN(A, 6),
    .int2_pin = GPIO_PIN(A, 7),
};

static struct lsm6dsm_driver_context lsm6dsm_context = {
    .i2c_context = &i2c1_context,
};

static struct lsm6dsm_driver lsm6dsm_inst = {
    .config = &lsm6dsm_config,
    .context = &lsm6dsm_context,
};

const struct lsm6dsm_driver *const lsm6dsm = &lsm6dsm_inst;

/***********************************/
/*  Raw Data Conversion Functions  */
/***********************************/

static float lsm6dsm_driver_from_fs2g_to_mg(int16_t lsb);
static float lsm6dsm_driver_from_fs4g_to_mg(int16_t lsb);
static float lsm6dsm_driver_from_fs8g_to_mg(int16_t lsb);
static float lsm6dsm_driver_from_fs16g_to_mg(int16_t lsb);
static float lsm6dsm_driver_from_fs125dps_to_mdps(int16_t lsb);
static float lsm6dsm_driver_from_fs250dps_to_mdps(int16_t lsb);
static float lsm6dsm_driver_from_fs500dps_to_mdps(int16_t lsb);
static float lsm6dsm_driver_from_fs1000dps_to_mdps(int16_t lsb);
static float lsm6dsm_driver_from_fs2000dps_to_mdps(int16_t lsb);
static float lsm6dsm_driver_from_lsb_to_celsius(int16_t lsb);

/***********************************/
/* LSM6DSM Configuration Functions */
/***********************************/

static int lsm6dsm_driver_set_reset(struct lsm6dsm_driver_context *context,
                                    uint8_t val);
static int lsm6dsm_driver_get_reset(struct lsm6dsm_driver_context *context,
                                    uint8_t *val);
static int lsm6dsm_driver_set_block_data_update(
    struct lsm6dsm_driver_context *context, uint8_t val);
static int lsm6dsm_driver_get_block_data_update(
    struct lsm6dsm_driver_context *context, uint8_t *val);
static int lsm6dsm_driver_set_mem_bank(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_func_cfg_en val);
static int lsm6dsm_driver_get_mem_bank(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_func_cfg_en *val);
static int lsm6dsm_driver_set_wrist_tilt_sens(
    struct lsm6dsm_driver_context *context, uint8_t val);

static int lsm6dsm_driver_get_wrist_tilt_sens(
    struct lsm6dsm_driver_context *context, uint8_t *val);
static int lsm6dsm_driver_set_tilt_src(struct lsm6dsm_driver_context *context,
                                       struct lsm6dsm_a_wrist_tilt_mask *val);

static int lsm6dsm_driver_get_tilt_src(struct lsm6dsm_driver_context *context,
                                       struct lsm6dsm_a_wrist_tilt_mask *val);
static int lsm6dsm_driver_set_tilt_latency(
    struct lsm6dsm_driver_context *context, uint8_t val);

static int lsm6dsm_driver_get_tilt_latency(
    struct lsm6dsm_driver_context *context, uint8_t *val);

static int lsm6dsm_driver_set_tilt_threshold(
    struct lsm6dsm_driver_context *context, uint8_t val);
static int lsm6dsm_driver_get_tilt_threshold(
    struct lsm6dsm_driver_context *context, uint8_t *val);
static int lsm6dsm_driver_set_int_notification(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_lir val);
static int lsm6dsm_driver_get_int_notification(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_lir *val);
static int lsm6dsm_driver_set_sleep_threshold(
    struct lsm6dsm_driver_context *context, uint8_t val);
static int lsm6dsm_driver_get_sleep_threshold(
    struct lsm6dsm_driver_context *context, uint8_t *val);
static int lsm6dsm_driver_set_sleep_dur(struct lsm6dsm_driver_context *context,
                                        uint8_t val);
static int lsm6dsm_driver_get_sleep_dur(struct lsm6dsm_driver_context *context,
                                        uint8_t *val);
static int lsm6dsm_driver_set_act_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_inact_en val);
static int lsm6dsm_driver_get_act_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_inact_en *val);
/*
    Set the value of tap_detection_on_z
*/
static int lsm6dsm_driver_set_tap_detection_on_z(
    struct lsm6dsm_driver_context *context, uint8_t val);

/*
    Get the value of tap_detection_on_z
*/
static int lsm6dsm_driver_get_tap_detection_on_z(
    struct lsm6dsm_driver_context *context, uint8_t *val);

/*
    Set the value of tap_detection_on_y
*/
static int lsm6dsm_driver_set_tap_detection_on_y(
    struct lsm6dsm_driver_context *context, uint8_t val);
/*
    Get the value of tap_detection_on_y
*/
static int lsm6dsm_driver_get_tap_detection_on_y(
    struct lsm6dsm_driver_context *context, uint8_t *val);

/*
    Set the value of tap_detection_on_x
*/
static int lsm6dsm_driver_set_tap_detection_on_x(
    struct lsm6dsm_driver_context *context, uint8_t val);

/*
    Get the value of tap_detection_on_x
*/
static int lsm6dsm_driver_get_tap_detection_on_x(
    struct lsm6dsm_driver_context *context, uint8_t *val);

/*
    Set the value of tap_threshold_x
*/
static int lsm6dsm_driver_set_tap_threshold_x(
    struct lsm6dsm_driver_context *context, uint8_t val);

/*
    Get the value of tap_threshold_x
*/
static int lsm6dsm_driver_get_tap_threshold_x(
    struct lsm6dsm_driver_context *context, uint8_t *val);

/*
    Set the value of tap_shock
*/
static int lsm6dsm_driver_set_tap_shock(struct lsm6dsm_driver_context *context,
                                        uint8_t val);

/*
    Get the value of tap_shock
*/
static int lsm6dsm_driver_get_tap_shock(struct lsm6dsm_driver_context *context,
                                        uint8_t *val);

/*
    Set the value of tap_quiet
*/
static int lsm6dsm_driver_set_tap_quiet(struct lsm6dsm_driver_context *context,
                                        uint8_t val);

/*
    Get the value of tap_quiet
*/
static int lsm6dsm_driver_get_tap_quiet(struct lsm6dsm_driver_context *context,
                                        uint8_t *val);

/*
    Set the value of tap_dur
*/
static int lsm6dsm_driver_set_tap_dur(struct lsm6dsm_driver_context *context,
                                      uint8_t val);

/*
    Get the value of tap_dur
*/
static int lsm6dsm_driver_get_tap_dur(struct lsm6dsm_driver_context *context,
                                      uint8_t *val);

/*
  Set the values of single_double_tap
*/
static int lsm6dsm_driver_set_tap_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_single_double_tap val);

/*
  Get the values of single_double_tap
*/
static int lsm6dsm_driver_get_tap_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_single_double_tap *val);
/*
  Set the interrupt which is routed to the INT1 pin
*/
static int lsm6dsm_driver_set_pin_int1_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int1_route val);

/*
  Get the interrupt which is routed to the INT1 pin
*/
static int lsm6dsm_driver_get_pin_int1_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int1_route *val);

/*
  Set the interrupt which is routed to the INT2 pin
*/
static int lsm6dsm_driver_set_pin_int2_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int2_route val);

/*
  Get the interrupt which is routed to the INT2 pin
*/
static int lsm6dsm_driver_get_pin_int2_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int2_route *val);

/*******************************************/
/*  Accelerometer Configuration Functions  */
/*******************************************/

static int lsm6dsm_driver_set_xl_full_scale(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_fs_xl val);
static int lsm6dsm_driver_get_xl_full_scale(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_fs_xl *val);
static int lsm6dsm_driver_set_xl_data_rate(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_odr_xl val);
static int lsm6dsm_driver_get_xl_data_rate(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_odr_xl *val);
static int lsm6dsm_driver_set_xl_filter_analog(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_bw0_xl val);
static int lsm6dsm_driver_get_xl_filter_analog(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_bw0_xl *val);
static int lsm6dsm_driver_set_xl_lp2_bandwidth(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_input_composite val);
static int lsm6dsm_driver_get_xl_lp2_bandwidth(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_input_composite *val);

/***************************************/
/*  Gyroscope Configuration Functions  */
/***************************************/

static int lsm6dsm_driver_set_gy_full_scale(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_fs_g val);
static int lsm6dsm_driver_get_gy_full_scale(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_fs_g *val);
static int lsm6dsm_driver_set_gy_data_rate(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_odr_g val);
static int lsm6dsm_driver_get_gy_data_rate(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_odr_g *val);
static int lsm6dsm_driver_set_gy_band_pass(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_lpf1_sel_g val);
static int lsm6dsm_driver_get_gy_band_pass(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_lpf1_sel_g *val);

static int lsm6dsm_ll_init(const struct lsm6dsm_driver *const dev) {
    const struct lsm6dsm_driver_config *config = dev->config;
    struct lsm6dsm_driver_context *context = dev->context;

    /*
        Restore LSM6DSM to default configuration
    */

    /* Set the reset bit */
    int ret = lsm6dsm_driver_set_reset(context, ENABLE);
    if (ret < 0) {
        LOG_ERR("Failed to send reset request: %d", ret);
        return -EIO;
    }

    /* Wait for reset bit to go LOW */
    uint8_t rst;
    do {
        ret = lsm6dsm_driver_get_reset(context, &rst);
        if (ret < 0) {
            LOG_ERR("Failed to get reset value: %d", ret);
            return -EIO;
        }
    } while (rst);

    /*
        Configure scale and data rate
    */

    /*  Enable Block Data Update */
    ret = lsm6dsm_driver_set_block_data_update(context, ENABLE);
    if (ret < 0) {
        LOG_ERR("Failed to enable block data update: %d", ret);
        return -EIO;
    }

    /* Set Output Data Rate for accelerometer */
    ret = lsm6dsm_driver_set_xl_data_rate(context,
                                          config->accelerometer.data_rate);
    if (ret < 0) {
        LOG_ERR("Failed to set accelerometer output data rate: %d", ret);
        return -EIO;
    }

    /* Set Output Data Rate for gyroscope */
    ret = lsm6dsm_driver_set_gy_data_rate(context, config->gyroscope.data_rate);
    if (ret < 0) {
        LOG_ERR("Failed to set gyroscope output data rate: %d", ret);
        return -EIO;
    }

    /* Set Full Scale for accelerometer */
    ret = lsm6dsm_driver_set_xl_full_scale(context,
                                           config->accelerometer.data_scale);
    if (ret < 0) {
        LOG_ERR("Failed to set accelerometer full scale: %d", ret);
        return -EIO;
    }

    /* Set Full Scale for gyroscope */
    ret =
        lsm6dsm_driver_set_gy_full_scale(context, config->gyroscope.data_scale);
    if (ret < 0) {
        LOG_ERR("Failed to set gyroscope full scale: %d", ret);
        return -EIO;
    }

    /*
        Configure filters
    */

    ret = lsm6dsm_driver_set_xl_filter_analog(
        context, config->accelerometer.analog_bandwidth);
    if (ret < 0) {
        LOG_ERR("Failed to set accelerometer analog filter: %d", ret);
        return -EIO;
    }

    ret = lsm6dsm_driver_set_xl_lp2_bandwidth(
        context, config->accelerometer.low_pass_filter);
    if (ret < 0) {
        LOG_ERR("Failed to set accelerometer low pass filter: %d", ret);
        return -EIO;
    }

    ret = lsm6dsm_driver_set_gy_band_pass(context,
                                          config->gyroscope.high_pass_filter);
    if (ret < 0) {
        LOG_ERR("Failed to set gyroscope high pass filter: %d", ret);
        return -EIO;
    }

    /*
        Configure Activity/Inactivity recognition for low power mode
    */

    ret = lsm6dsm_driver_set_act_mode(context, config->wakeup.action);
    if (ret < 0) {
        LOG_ERR("Failed to set active/inactive mode: %d", ret);
        return -EIO;
    }

    ret = lsm6dsm_driver_set_sleep_dur(
        context, LSM6DSM_WK_DURATION_FROM_MS(config->wakeup.sleep_dur_ms,
                                             config->accelerometer.data_rate));
    if (ret < 0) {
        LOG_ERR("Failed to set sleep duration: %d", ret);
        return -EIO;
    }

    ret = lsm6dsm_driver_set_sleep_threshold(
        context,
        LSM6DSM_WK_THRESHOLD_FROM_MG(config->wakeup.threshold_mg,
                                     config->accelerometer.data_scale));
    if (ret < 0) {
        LOG_ERR("Failed to set sleep threshold: %d", ret);
        return -EIO;
    }

    /*
        Configure Absolute Wrist Tilt (AWT)
    */

    ret = lsm6dsm_driver_set_tilt_src(context, &config->tilt.source);
    if (ret < 0) {
        LOG_ERR("Error setting wrist tilt source: %d", ret);
    } else {
        LOG_DBG("Wrist tilt source set to 0x%x", config->tilt.source);
    }

    struct lsm6dsm_a_wrist_tilt_mask val;

    ret = lsm6dsm_driver_get_tilt_src(context, &val);

    ret = lsm6dsm_driver_set_tilt_threshold(
        context, LSM6DSM_WT_THRESHOLD_FROM_DEGREES(config->tilt.threshold_deg));
    if (ret < 0) {
        LOG_ERR("Error setting wrist tilt threshold: %d", ret);
    } else {
        LOG_DBG("Wrist tilt threshold set to %1.3f degrees",
                config->tilt.threshold_deg);
    }

    LOG_INF("Latency: %d", LSM6DSM_WT_LATENCY_FROM_MS(config->tilt.latency_ms));
    ret = lsm6dsm_driver_set_tilt_latency(context, (uint8_t)0x01);
    if (ret < 0) {
        LOG_ERR("Error setting wrist tilt latency: %d", ret);
    } else {
        LOG_DBG("Wrist tilt latency set to %d ms", config->tilt.latency_ms);
    }

    ret = lsm6dsm_driver_set_wrist_tilt_sens(context, ENABLE);
    if (ret < 0) {
        LOG_ERR("Error enabling wrist tilt sense: %d", ret);
    } else {
        LOG_DBG("Wrist tilt sense enabled successfully");
    }

    /*
        Configure Single Tap
    */

    lsm6dsm_driver_set_tap_detection_on_x(context, config->tap.tap_x);
    lsm6dsm_driver_set_tap_detection_on_y(context, config->tap.tap_y);
    lsm6dsm_driver_set_tap_detection_on_z(context, config->tap.tap_z);

    lsm6dsm_driver_set_tap_threshold_x(
        context,
        LSM6DSM_TAP_THRESHOLD_FROM_MG(config->tap.threshold_mg,
                                      config->accelerometer.data_scale));

    lsm6dsm_driver_set_tap_quiet(
        context, LSM6DSM_TAP_QUIET_FROM_MS(config->tap.quite_ms,
                                           config->accelerometer.data_rate));

    lsm6dsm_driver_set_tap_shock(
        context, LSM6DSM_TAP_SHOCK_FROM_MS(config->tap.shock_ms,
                                           config->accelerometer.data_rate));

    lsm6dsm_driver_set_tap_dur(
        context, LSM6DSM_TAP_DURATION_FROM_MS(config->tap.dur_ms,
                                              config->accelerometer.data_rate));

    lsm6dsm_driver_set_tap_mode(context, LSM6DSM_BOTH_SINGLE_DOUBLE);

    /*
        Configure LSM6DSM Interrupts
    */

    /* Route the ACT interrupt to the INT1 pin */
    struct lsm6dsm_int1_route r1 = {.int1_inact_state = ENABLE};
    ret = lsm6dsm_driver_set_pin_int1_route(context, r1);
    if (ret < 0) {
        LOG_ERR("Error setting INT2 route: %d", ret);
    }

    /* Route the AWT and double tap interrupt to the INT2 pin */
    struct lsm6dsm_int2_route r2 = {.int2_wrist_tilt = ENABLE,
                                    .int2_double_tap = ENABLE};
    ret = lsm6dsm_driver_set_pin_int2_route(context, r2);
    if (ret < 0) {
        LOG_ERR("Error setting INT2 route: %d", ret);
    }

    /* Set the interrupt notification mode to latched */
    ret = lsm6dsm_driver_set_int_notification(
        context, LSM6DSM_INT_PULSED);  // LSM6DSM_INT_LATCHED);
    if (ret < 0) {
        LOG_ERR("Error setting INT2 notification: %d", ret);
    }

    return ret;
}

int lsm6dsm_driver_init(const struct lsm6dsm_driver *const dev) {
    const struct lsm6dsm_driver_config *config = dev->config;
    struct lsm6dsm_driver_context *context = dev->context;
    struct i2c_request *request = &context->request;
    struct i2c_request *tilt_it_source_request =
        &context->tilt_it_source_request;
    struct i2c_request *it_source_request = &context->it_source_request;
    struct i2c_request *tap_it_source_request = &context->tap_it_source_request;

    LOG_DBG("Initialized LSM6DSM");

    context->state = LSM6DSM_ERROR;
    context->it_state = LSM6DSM_INTERRUPT_CLEAR;

    /****************************/
    /* Configure MCU Interrupts */
    /****************************/

    /* Declare GPIO initialization structure */
    GPIO_InitTypeDef gpio = {0};

    /* Define common interrupt parameters */
    gpio.Mode = GPIO_MODE_IT_RISING;
    gpio.Pull = GPIO_NOPULL;

    /*
        Configure INT1 Pin
    */

    /* Enable Clock */
    GPIOx_CLK_ENABLE(config->int1_pin.port);

    /* Set pin field */
    gpio.Pin = config->int1_pin.pin;

    /* Initialize INT1 */
    HAL_GPIO_Init(config->int1_pin.port, &gpio);

    /*
        Configure INT2 Pin
    */

    /* Enable Clock */
    GPIOx_CLK_ENABLE(config->int2_pin.port);

    /* Set pin field */
    gpio.Pin = config->int2_pin.pin;

    /* Initialize INT1 */
    HAL_GPIO_Init(config->int2_pin.port, &gpio);

    /* Configure NVIC so the interrupt can fire */
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

    /**************************/
    /* Configure I2C Requests */
    /**************************/
    LOG_DBG("Configuring I2C requests");
    /*
        Configure I2C request for standard transactions
    */
    request->address = LSM6DSM_ID;
    request->buffer = context->i2c_transaction_buffer;
    tilt_it_source_request->num_bytes = 1;
    request->future.state = FUTURE_WAITING;
    request->future.error_number = 0;

    /*
        Configure I2C request for acquiring AWT interrupt source
    */
    tilt_it_source_request->action = I2C_READ;
    tilt_it_source_request->address = LSM6DSM_ID;
    tilt_it_source_request->ireg = LSM6DSM_WRIST_TILT_IA;
    tilt_it_source_request->buffer = (uint8_t *)&context->wrist_tilt_flags;
    tilt_it_source_request->num_bytes = 1;
    tilt_it_source_request->future.state = FUTURE_WAITING;
    tilt_it_source_request->future.error_number = 0;

    /*
        Configure I2C request for clearing AWT interrupt
    */
    it_source_request->action = I2C_READ;
    it_source_request->address = LSM6DSM_ID;
    it_source_request->ireg = LSM6DSM_FUNC_SRC2;
    it_source_request->buffer = (uint8_t *)&context->it_source;
    it_source_request->num_bytes = 1;
    it_source_request->future.state = FUTURE_WAITING;
    it_source_request->future.error_number = 0;

    tap_it_source_request->action = I2C_READ;
    tap_it_source_request->address = LSM6DSM_ID;
    tap_it_source_request->ireg = LSM6DSM_TAP_SRC;
    tap_it_source_request->buffer = (uint8_t *)&context->single_tap_flags;
    tap_it_source_request->num_bytes = 1;
    tap_it_source_request->future.state = FUTURE_WAITING;
    tap_it_source_request->future.error_number = 0;

    /*********************/
    /* Configure LSM6DSM */
    /*********************/

    int ret = lsm6dsm_ll_init(dev);
    if (ret != 0) {
        LOG_ERR("Failed to initialize low-level lsm6dsm driver");
    } else {
        LOG_DBG("Low level LSM6DSM driver initialized successfully");
    }

    /*
        Determine the appropriate acceleration conversion function
    */
    LOG_DBG("Determining appropriate acceleration conversion function");
    switch (config->accelerometer.data_scale) {
        case LSM6DSM_2g: {
            context->acc_conversion = lsm6dsm_driver_from_fs2g_to_mg;
        } break;
        case LSM6DSM_4g: {
            context->acc_conversion = lsm6dsm_driver_from_fs4g_to_mg;
        } break;
        case LSM6DSM_8g: {
            context->acc_conversion = lsm6dsm_driver_from_fs8g_to_mg;
        } break;
        case LSM6DSM_16g: {
            context->acc_conversion = lsm6dsm_driver_from_fs16g_to_mg;
        } break;
        default:
            LOG_ERR(
                "Failed to initialize LSM6DSM: Invalid accelerometer data "
                "scale - %d",
                config->accelerometer.data_scale);
            return -1;
    }

    /*
        Determine the appropriate angular rate conversion function
    */
    LOG_DBG("Determining appropriate gyroscope conversion function");
    switch (config->gyroscope.data_scale) {
        case LSM6DSM_125dps: {
            context->ang_conversion = lsm6dsm_driver_from_fs125dps_to_mdps;
        } break;
        case LSM6DSM_250dps: {
            context->ang_conversion = lsm6dsm_driver_from_fs250dps_to_mdps;
        } break;
        case LSM6DSM_500dps: {
            context->ang_conversion = lsm6dsm_driver_from_fs500dps_to_mdps;
        } break;
        case LSM6DSM_1000dps: {
            context->ang_conversion = lsm6dsm_driver_from_fs1000dps_to_mdps;
        } break;
        case LSM6DSM_2000dps: {
            context->ang_conversion = lsm6dsm_driver_from_fs2000dps_to_mdps;
        } break;
        default:
            LOG_ERR(
                "Failed to initialize LSM6DSM: Invalid gyroscope data scale - "
                "%d",
                config->accelerometer.data_scale);
            return -1;
    }

    /* Set the interrupt(s) to triggered so we can clear any previous calls */
    context->int1_flag = false;  // true; FIXME: unused right now
    context->int2_flag = true;

    /* Device is ready to use */
    context->state = LSM6DSM_READY;

    if (ret != 0) {
        LOG_ERR("Error initializing LSM6DSM: %d", ret);
    } else {
        LOG_DBG("LSM6DSM initialized successfully");
    }

    return ret;
}

void lsm6dsm_driver_set_int1(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    context->int1_flag = true;
}

void lsm6dsm_driver_set_int2(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    context->int2_flag = true;
}

void lsm6dsm_driver_clear_int1(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    context->int1_flag = false;
}

void lsm6dsm_driver_clear_int2(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    context->int2_flag = false;
}

bool lsm6dsm_driver_get_int1(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->int1_flag;
}

bool lsm6dsm_driver_get_int2(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->int2_flag;
}

int lsm6dsm_driver_request_acceleration(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;
    struct i2c_request *request = &context->request;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_OUTX_L_XL;
    request->num_bytes = 6;

    return i2c_enqueue_request(context->i2c_context, request);
}

int lsm6dsm_driver_request_angular_rate(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;
    struct i2c_request *request = &context->request;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_OUTX_L_G;
    request->num_bytes = 6;

    return i2c_enqueue_request(context->i2c_context, request);
}

int lsm6dsm_driver_request_tilt_it_source(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return i2c_enqueue_request(context->i2c_context,
                               &context->tilt_it_source_request);
}

int lsm6dsm_driver_request_it_source(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return i2c_enqueue_request(context->i2c_context,
                               &context->it_source_request);
}

int lsm6dsm_driver_request_tap_it_source(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return i2c_enqueue_request(context->i2c_context,
                               &context->tap_it_source_request);
}

int lsm6dsm_driver_get_acceleration_request_status(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return future_get_state(&context->request.future);
}

int lsm6dsm_driver_get_angular_rate_request_status(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return future_get_state(&context->request.future);
}

int lsm6dsm_driver_get_tilt_it_source_request_status(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return future_get_state(&context->tilt_it_source_request.future);
}

int lsm6dsm_driver_get_it_source_request_status(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return future_get_state(&context->it_source_request.future);
}

int lsm6dsm_driver_get_tap_it_source_request_status(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return future_get_state(&context->tap_it_source_request.future);
}

void lsm6dsm_driver_process_acceleration(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;
    struct i2c_request *request = &context->request;

    /* Should probably just use shifts here */
    int16_t x_acc = (int16_t)((uint8_t *)request->buffer)[1];
    x_acc = (x_acc * 256) + (int16_t)((uint8_t *)request->buffer)[0];
    int16_t y_acc = (int16_t)((uint8_t *)request->buffer)[3];
    y_acc = (y_acc * 256) + (int16_t)((uint8_t *)request->buffer)[2];
    int16_t z_acc = (int16_t)((uint8_t *)request->buffer)[5];
    z_acc = (z_acc * 256) + (int16_t)((uint8_t *)request->buffer)[4];

    context->x_acc = context->acc_conversion(x_acc);
    context->y_acc = context->acc_conversion(y_acc);
    context->z_acc = context->acc_conversion(z_acc);
}

void lsm6dsm_driver_process_angular_rate(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;
    struct i2c_request *request = &context->request;

    /* Should probably just use shifts here */
    int16_t x_ang = (int16_t)((uint8_t *)request->buffer)[1];
    x_ang = (x_ang * 256) + (int16_t)((uint8_t *)request->buffer)[0];
    int16_t y_ang = (int16_t)((uint8_t *)request->buffer)[3];
    y_ang = (y_ang * 256) + (int16_t)((uint8_t *)request->buffer)[2];
    int16_t z_ang = (int16_t)((uint8_t *)request->buffer)[5];
    z_ang = (z_ang * 256) + (int16_t)((uint8_t *)request->buffer)[4];

    context->x_ang = context->ang_conversion(x_ang);
    context->y_ang = context->ang_conversion(y_ang);
    context->z_ang = context->ang_conversion(z_ang);
}

bool lsm6dsm_driver_it_source_is_tilt(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->it_source.wrist_tilt_ia;
}

void lsm6dsm_driver_process_tilt_it_source(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;
    struct i2c_request *request = &context->tilt_it_source_request;

    context->tilt_flags = *(tilt_flags *)&request->buffer[0];
}

void lsm6dsm_driver_process_tap_it_source(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;
    struct i2c_request *request = &context->tap_it_source_request;

    context->tap_flags = *(tap_flags *)&request->buffer[0];
}

/***********************************/
/*  Raw Data Conversion Functions  */
/***********************************/

static float lsm6dsm_driver_from_fs2g_to_mg(int16_t lsb) {
    return ((float)lsb * 0.061f);
}

static float lsm6dsm_driver_from_fs4g_to_mg(int16_t lsb) {
    return ((float)lsb * 0.122f);
}

static float lsm6dsm_driver_from_fs8g_to_mg(int16_t lsb) {
    return ((float)lsb * 0.244f);
}

static float lsm6dsm_driver_from_fs16g_to_mg(int16_t lsb) {
    return ((float)lsb * 0.488f);
}

static float lsm6dsm_driver_from_fs125dps_to_mdps(int16_t lsb) {
    return ((float)lsb * 4.375f);
}

static float lsm6dsm_driver_from_fs250dps_to_mdps(int16_t lsb) {
    return ((float)lsb * 8.750f);
}

static float lsm6dsm_driver_from_fs500dps_to_mdps(int16_t lsb) {
    return ((float)lsb * 17.50f);
}

static float lsm6dsm_driver_from_fs1000dps_to_mdps(int16_t lsb) {
    return ((float)lsb * 35.0f);
}

static float lsm6dsm_driver_from_fs2000dps_to_mdps(int16_t lsb) {
    return ((float)lsb * 70.0f);
}

static float lsm6dsm_driver_from_lsb_to_celsius(int16_t lsb) {
    return (((float)lsb / 256.0f) + 25.0f);
}

/***********************************/
/* LSM6DSM Configuration Functions */
/***********************************/

static int lsm6dsm_driver_set_reset(struct lsm6dsm_driver_context *context,
                                    uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL3_C;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    context->i2c_transaction_buffer[0].ctrl3_c.sw_reset = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    return ret;
}
static int lsm6dsm_driver_get_reset(struct lsm6dsm_driver_context *context,
                                    uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL3_C;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    *val = context->i2c_transaction_buffer[0].ctrl3_c.sw_reset;

    return ret;
}
static int lsm6dsm_driver_set_block_data_update(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL3_C;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    context->i2c_transaction_buffer[0].ctrl3_c.bdu = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    return ret;
}

static int lsm6dsm_driver_get_block_data_update(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL3_C;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    *val = context->i2c_transaction_buffer[0].ctrl3_c.bdu;

    return ret;
}

static int lsm6dsm_driver_set_mem_bank(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_func_cfg_en val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_FUNC_CFG_ACCESS;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    context->i2c_transaction_buffer[0].func_cfg_access.func_cfg_en = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    return ret;
}

static int lsm6dsm_driver_get_mem_bank(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_func_cfg_en *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_FUNC_CFG_ACCESS;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    switch (context->i2c_transaction_buffer[0].func_cfg_access.func_cfg_en) {
        case LSM6DSM_USER_BANK: {
            *val = LSM6DSM_USER_BANK;
        } break;

        case LSM6DSM_BANK_B: {
            *val = LSM6DSM_BANK_B;
        } break;

        default: {
            *val = LSM6DSM_USER_BANK;
        } break;
    }

    return ret;
}

static int lsm6dsm_driver_set_wrist_tilt_sens(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL10_C;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    context->i2c_transaction_buffer[0].ctrl10_c.wrist_tilt_en = val;
    if (val != 0x00U) {
        context->i2c_transaction_buffer[0].ctrl10_c.func_en = val;
    }
    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    return ret;
}

static int lsm6dsm_driver_get_wrist_tilt_sens(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL10_C;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    *val = context->i2c_transaction_buffer[0].ctrl10_c.wrist_tilt_en;

    return ret;
}

static int lsm6dsm_driver_set_tilt_src(struct lsm6dsm_driver_context *context,
                                       struct lsm6dsm_a_wrist_tilt_mask *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_BANK_B);
    if (ret != 0) {
        return ret;
    }

    request->action = I2C_WRITE;
    request->ireg = LSM6DSM_A_WRIST_TILT_MASK;
    request->num_bytes = 1;
    request->buffer[0] = *(uint8_t *)val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_USER_BANK);

    return ret;
}

static int lsm6dsm_driver_get_tilt_src(struct lsm6dsm_driver_context *context,
                                       struct lsm6dsm_a_wrist_tilt_mask *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_BANK_B);
    if (ret != 0) {
        return ret;
    }

    request->action = I2C_READ;
    request->ireg = LSM6DSM_A_WRIST_TILT_MASK;
    request->num_bytes = 1;
    request->buffer = context->i2c_transaction_buffer;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_USER_BANK);
    if (ret != 0) {
        return ret;
    }

    *val = context->i2c_transaction_buffer[0].a_wrist_tilt_mask;

    return ret;
}

static int lsm6dsm_driver_set_tilt_latency(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_BANK_B);
    if (ret != 0) {
        return ret;
    }

    request->action = I2C_WRITE;
    request->ireg = LSM6DSM_A_WRIST_TILT_LAT;
    request->num_bytes = 1;
    request->buffer[0] = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_USER_BANK);

    return ret;
}

static int lsm6dsm_driver_get_tilt_latency(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_BANK_B);
    if (ret != 0) {
        return ret;
    }

    request->action = I2C_READ;
    request->ireg = LSM6DSM_A_WRIST_TILT_LAT;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_USER_BANK);
    if (ret != 0) {
        return ret;
    }

    *val = context->i2c_transaction_buffer[0].byte;

    return ret;
}

static int lsm6dsm_driver_set_tilt_threshold(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_BANK_B);
    if (ret != 0) {
        return ret;
    }

    request->action = I2C_WRITE;
    request->ireg = LSM6DSM_A_WRIST_TILT_THS;
    request->num_bytes = 1;
    request->buffer[0] = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_USER_BANK);

    return ret;
}

static int lsm6dsm_driver_get_tilt_threshold(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_BANK_B);
    if (ret != 0) {
        return ret;
    }

    request->action = I2C_READ;
    request->ireg = LSM6DSM_A_WRIST_TILT_THS;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    ret = lsm6dsm_driver_set_mem_bank(context, LSM6DSM_USER_BANK);
    if (ret != 0) {
        return ret;
    }

    *val = context->i2c_transaction_buffer[0].byte;

    return ret;
}

static int lsm6dsm_driver_set_int_notification(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_lir val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    context->i2c_transaction_buffer[0].tap_cfg.lir = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    return ret;
}

static int lsm6dsm_driver_get_int_notification(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_lir *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    switch (context->i2c_transaction_buffer[0].tap_cfg.lir) {
        case LSM6DSM_INT_PULSED: {
            *val = LSM6DSM_INT_PULSED;
        } break;

        case LSM6DSM_INT_LATCHED: {
            *val = LSM6DSM_INT_LATCHED;
        } break;

        default: {
            *val = LSM6DSM_INT_PULSED;
        } break;
    }

    return ret;
}

static int lsm6dsm_driver_set_sleep_threshold(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_WAKE_UP_THS;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    context->i2c_transaction_buffer[0].wake_up_ths.wk_ths = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    return ret;
}

static int lsm6dsm_driver_get_sleep_threshold(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_WAKE_UP_THS;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    *val = context->i2c_transaction_buffer[0].wake_up_ths.wk_ths;

    return ret;
}

static int lsm6dsm_driver_set_sleep_dur(struct lsm6dsm_driver_context *context,
                                        uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_WAKE_UP_DUR;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    request->action = I2C_WRITE;
    context->i2c_transaction_buffer[0].wake_up_dur.sleep_dur = val;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    return ret;
}

static int lsm6dsm_driver_get_sleep_dur(struct lsm6dsm_driver_context *context,
                                        uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_WAKE_UP_DUR;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    *val = context->i2c_transaction_buffer[0].wake_up_dur.wake_dur;

    return ret;
}

static int lsm6dsm_driver_set_act_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_inact_en val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].tap_cfg.inact_en = (uint8_t)val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

static int lsm6dsm_driver_get_act_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_inact_en *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    switch (context->i2c_transaction_buffer->tap_cfg.inact_en) {
        case LSM6DSM_PROPERTY_DISABLE: {
            *val = LSM6DSM_PROPERTY_DISABLE;
        } break;

        case LSM6DSM_XL_12Hz5_GY_NOT_AFFECTED: {
            *val = LSM6DSM_XL_12Hz5_GY_NOT_AFFECTED;
        } break;

        case LSM6DSM_XL_12Hz5_GY_SLEEP: {
            *val = LSM6DSM_XL_12Hz5_GY_SLEEP;
        } break;

        case LSM6DSM_XL_12Hz5_GY_PD: {
            *val = LSM6DSM_XL_12Hz5_GY_PD;
        } break;

        default: {
            *val = LSM6DSM_PROPERTY_DISABLE;
        } break;
    }

    return ret;
}

/*
    Set the value of tap_detection_on_z
*/
static int lsm6dsm_driver_set_tap_detection_on_z(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].tap_cfg.tap_z_en = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_detection_on_z
*/
static int lsm6dsm_driver_get_tap_detection_on_z(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    *val = context->i2c_transaction_buffer[0].tap_cfg.tap_z_en;

    return ret;
}

/*
    Set the value of tap_detection_on_y
*/
static int lsm6dsm_driver_set_tap_detection_on_y(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].tap_cfg.tap_y_en = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_detection_on_y
*/
static int lsm6dsm_driver_get_tap_detection_on_y(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    *val = context->i2c_transaction_buffer[0].tap_cfg.tap_y_en;

    return ret;
}

/*
    Set the value of tap_detection_on_x
*/
static int lsm6dsm_driver_set_tap_detection_on_x(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].tap_cfg.tap_x_en = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_detection_on_x
*/
static int lsm6dsm_driver_get_tap_detection_on_x(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_CFG;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    *val = context->i2c_transaction_buffer[0].tap_cfg.tap_x_en;

    return ret;
}

/*
    Set the value of tap_threshold_x
*/
static int lsm6dsm_driver_set_tap_threshold_x(
    struct lsm6dsm_driver_context *context, uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_THS_6D;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].tap_ths_6d.tap_ths = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_threshold_x
*/
static int lsm6dsm_driver_get_tap_threshold_x(
    struct lsm6dsm_driver_context *context, uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_TAP_THS_6D;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    *val = context->i2c_transaction_buffer[0].tap_ths_6d.tap_ths;

    return ret;
}

/*
    Set the value of tap_shock
*/
static int lsm6dsm_driver_set_tap_shock(struct lsm6dsm_driver_context *context,
                                        uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT_DUR2;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].int_dur2.shock = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_shock
*/
static int lsm6dsm_driver_get_tap_shock(struct lsm6dsm_driver_context *context,
                                        uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT_DUR2;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    *val = context->i2c_transaction_buffer[0].int_dur2.shock;

    return ret;
}

/*
    Set the value of tap_quiet
*/
static int lsm6dsm_driver_set_tap_quiet(struct lsm6dsm_driver_context *context,
                                        uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT_DUR2;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].int_dur2.quiet = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_quiet
*/
static int lsm6dsm_driver_get_tap_quiet(struct lsm6dsm_driver_context *context,
                                        uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT_DUR2;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    *val = context->i2c_transaction_buffer[0].int_dur2.quiet;

    return ret;
}

/*
    Set the value of tap_dur
*/
static int lsm6dsm_driver_set_tap_dur(struct lsm6dsm_driver_context *context,
                                      uint8_t val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT_DUR2;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].int_dur2.dur = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_dur
*/
static int lsm6dsm_driver_get_tap_dur(struct lsm6dsm_driver_context *context,
                                      uint8_t *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT_DUR2;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    *val = context->i2c_transaction_buffer[0].int_dur2.dur;

    return ret;
}

/*
    Set the value of tap_mode
*/
static int lsm6dsm_driver_set_tap_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_single_double_tap val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_WAKE_UP_THS;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        context->i2c_transaction_buffer[0].wake_up_ths.single_double_tap = val;

        request->action = I2C_WRITE;
        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
    Get the value of tap_mode
*/
static int lsm6dsm_driver_get_tap_mode(struct lsm6dsm_driver_context *context,
                                       enum lsm6dsm_single_double_tap *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_WAKE_UP_THS;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    switch (context->i2c_transaction_buffer[0].wake_up_ths.single_double_tap) {
        case LSM6DSM_ONLY_SINGLE: {
            *val = LSM6DSM_ONLY_SINGLE;
        } break;

        case LSM6DSM_BOTH_SINGLE_DOUBLE: {
            *val = LSM6DSM_BOTH_SINGLE_DOUBLE;
        } break;

        default: {
            *val = LSM6DSM_ONLY_SINGLE;
        } break;
    }

    return ret;
}

/*
  Set the interrupt which is routed to the INT1 pin
*/
static int lsm6dsm_driver_set_pin_int1_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int1_route val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_master_config *master_config =
        (struct lsm6dsm_master_config *)&request->buffer[0];
    struct lsm6dsm_int1_ctrl *int1_ctrl =
        (struct lsm6dsm_int1_ctrl *)&request->buffer[1];
    struct lsm6dsm_md1_cfg *md1_cfg =
        (struct lsm6dsm_md1_cfg *)&request->buffer[2];
    struct lsm6dsm_md2_cfg *md2_cfg =
        (struct lsm6dsm_md2_cfg *)&request->buffer[3];
    struct lsm6dsm_ctrl4_c *ctrl4_c =
        (struct lsm6dsm_ctrl4_c *)&request->buffer[4];
    struct lsm6dsm_tap_cfg *tap_cfg =
        (struct lsm6dsm_tap_cfg *)&request->buffer[5];
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT1_CTRL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        int1_ctrl->int1_drdy_xl = val.int1_drdy_xl;
        int1_ctrl->int1_drdy_g = val.int1_drdy_g;
        int1_ctrl->int1_boot = val.int1_boot;
        int1_ctrl->int1_fth = val.int1_fth;
        int1_ctrl->int1_fifo_ovr = val.int1_fifo_ovr;
        int1_ctrl->int1_full_flag = val.int1_full_flag;
        int1_ctrl->int1_sign_mot = val.int1_sign_mot;
        int1_ctrl->int1_step_detector = val.int1_step_detector;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->action = I2C_READ;
        request->buffer = md1_cfg;
        request->ireg = LSM6DSM_MD1_CFG;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->buffer = md2_cfg;
        request->ireg = LSM6DSM_MD2_CFG;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        md1_cfg->int1_timer = val.int1_timer;
        md1_cfg->int1_tilt = val.int1_tilt;
        md1_cfg->int1_6d = val.int1_6d;
        md1_cfg->int1_double_tap = val.int1_double_tap;
        md1_cfg->int1_ff = val.int1_ff;
        md1_cfg->int1_wu = val.int1_wu;
        md1_cfg->int1_single_tap = val.int1_single_tap;
        md1_cfg->int1_inact_state = val.int1_inact_state;

        request->action = I2C_WRITE;
        request->buffer = md1_cfg;
        request->ireg = LSM6DSM_MD1_CFG;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->action = I2C_READ;
        request->buffer = ctrl4_c;
        request->ireg = LSM6DSM_CTRL4_C;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        ctrl4_c->den_drdy_int1 = val.den_drdy_int1;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->action = I2C_READ;
        request->buffer = master_config;
        request->ireg = LSM6DSM_MASTER_CONFIG;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        master_config->drdy_on_int1 = val.den_drdy_int1;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->action = I2C_READ;
        request->buffer = tap_cfg;
        request->ireg = LSM6DSM_TAP_CFG;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }

        if ((val.int1_6d != 0x00U) || (val.int1_ff != 0x00U) ||
            (val.int1_wu != 0x00U) || (val.int1_single_tap != 0x00U) ||
            (val.int1_double_tap != 0x00U) || (val.int1_inact_state != 0x00U) ||
            (md2_cfg->int2_6d != 0x00U) || (md2_cfg->int2_ff != 0x00U) ||
            (md2_cfg->int2_wu != 0x00U) ||
            (md2_cfg->int2_single_tap != 0x00U) ||
            (md2_cfg->int2_double_tap != 0x00U) ||
            (md2_cfg->int2_inact_state != 0x00U)) {
            tap_cfg->interrupts_enable = ENABLE;
        }

        else {
            tap_cfg->interrupts_enable = DISABLE;
        }
    }

    if (ret == 0) {
        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    /* Restore the request buffer pointer */
    request->buffer = context->i2c_transaction_buffer;

    return ret;
}

/*
  Get the interrupt which is routed to the INT1 pin
*/
static int lsm6dsm_driver_get_pin_int1_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int1_route *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_master_config *master_config =
        (struct lsm6dsm_master_config *)&request[0];
    struct lsm6dsm_int1_ctrl *int1_ctrl =
        (struct lsm6dsm_int1_ctrl *)&request[1];
    struct lsm6dsm_md1_cfg *md1_cfg = (struct lsm6dsm_md1_cfg *)&request[2];
    struct lsm6dsm_ctrl4_c *ctrl4_c = (struct lsm6dsm_ctrl4_c *)&request[3];
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT1_CTRL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        val->int1_drdy_xl = int1_ctrl->int1_drdy_xl;
        val->int1_drdy_g = int1_ctrl->int1_drdy_g;
        val->int1_boot = int1_ctrl->int1_boot;
        val->int1_fth = int1_ctrl->int1_fth;
        val->int1_fifo_ovr = int1_ctrl->int1_fifo_ovr;
        val->int1_full_flag = int1_ctrl->int1_full_flag;
        val->int1_sign_mot = int1_ctrl->int1_sign_mot;
        val->int1_step_detector = int1_ctrl->int1_step_detector;

        request->ireg = LSM6DSM_MD1_CFG;
        request->buffer = md1_cfg;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }

        if (ret == 0) {
            val->int1_timer = md1_cfg->int1_timer;
            val->int1_tilt = md1_cfg->int1_tilt;
            val->int1_6d = md1_cfg->int1_6d;
            val->int1_double_tap = md1_cfg->int1_double_tap;
            val->int1_ff = md1_cfg->int1_ff;
            val->int1_wu = md1_cfg->int1_wu;
            val->int1_single_tap = md1_cfg->int1_single_tap;
            val->int1_inact_state = md1_cfg->int1_inact_state;

            request->ireg = LSM6DSM_CTRL4_C;
            request->buffer = ctrl4_c;

            ret = i2c_blocking_enqueue(i2c_context, request);
            if (ret < 0 || future_is_errored(&request->future)) {
                ret = -EIO;
            }

            if (ret == 0) {
                val->den_drdy_int1 = ctrl4_c->den_drdy_int1;

                request->ireg = LSM6DSM_MASTER_CONFIG;
                request->buffer = master_config;

                ret = i2c_blocking_enqueue(i2c_context, request);
                if (ret < 0 || future_is_errored(&request->future)) {
                    ret = -EIO;
                }

                val->den_drdy_int1 = master_config->drdy_on_int1;
            }
        }
    }

    /* Restore the request buffer pointer */
    request->buffer = context->i2c_transaction_buffer;

    return ret;
}

/*
  Set the interrupt which is routed to the INT2 pin
*/
static int lsm6dsm_driver_set_pin_int2_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int2_route val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_int2_ctrl *int2_ctrl =
        (struct lsm6dsm_int2_ctrl *)&request->buffer[0];
    struct lsm6dsm_md1_cfg *md1_cfg =
        (struct lsm6dsm_md1_cfg *)&request->buffer[1];
    struct lsm6dsm_md2_cfg *md2_cfg =
        (struct lsm6dsm_md2_cfg *)&request->buffer[2];
    struct lsm6dsm_drdy_pulse_cfg *drdy_pulse_cfg =
        (struct lsm6dsm_drdy_pulse_cfg *)&request->buffer[3];
    struct lsm6dsm_tap_cfg *tap_cfg =
        (struct lsm6dsm_tap_cfg *)&request->buffer[4];
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT2_CTRL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        int2_ctrl->int2_drdy_xl = val.int2_drdy_xl;
        int2_ctrl->int2_drdy_g = val.int2_drdy_g;
        int2_ctrl->int2_drdy_temp = val.int2_drdy_temp;
        int2_ctrl->int2_fth = val.int2_fth;
        int2_ctrl->int2_fifo_ovr = val.int2_fifo_ovr;
        int2_ctrl->int2_full_flag = val.int2_full_flag;
        int2_ctrl->int2_step_count_ov = val.int2_step_count_ov;
        int2_ctrl->int2_step_delta = val.int2_step_delta;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->action = I2C_READ;
        request->ireg = LSM6DSM_MD1_CFG;
        request->buffer = md1_cfg;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->ireg = LSM6DSM_MD2_CFG;
        request->buffer = md2_cfg;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        md2_cfg->int2_iron = val.int2_iron;
        md2_cfg->int2_tilt = val.int2_tilt;
        md2_cfg->int2_6d = val.int2_6d;
        md2_cfg->int2_double_tap = val.int2_double_tap;
        md2_cfg->int2_ff = val.int2_ff;
        md2_cfg->int2_wu = val.int2_wu;
        md2_cfg->int2_single_tap = val.int2_single_tap;
        md2_cfg->int2_inact_state = val.int2_inact_state;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->action = I2C_READ;
        request->ireg = LSM6DSM_DRDY_PULSE_CFG;
        request->buffer = drdy_pulse_cfg;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        drdy_pulse_cfg->int2_wrist_tilt = val.int2_wrist_tilt;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    if (ret == 0) {
        request->action = I2C_READ;
        request->ireg = LSM6DSM_TAP_CFG;
        request->buffer = tap_cfg;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }

        if ((md1_cfg->int1_6d != 0x00U) || (md1_cfg->int1_ff != 0x00U) ||
            (md1_cfg->int1_wu != 0x00U) ||
            (md1_cfg->int1_single_tap != 0x00U) ||
            (md1_cfg->int1_double_tap != 0x00U) ||
            (md1_cfg->int1_inact_state != 0x00U) || (val.int2_6d != 0x00U) ||
            (val.int2_ff != 0x00U) || (val.int2_wu != 0x00U) ||
            (val.int2_single_tap != 0x00U) || (val.int2_double_tap != 0x00U) ||
            (val.int2_inact_state != 0x00U)) {
            tap_cfg->interrupts_enable = ENABLE;
        }

        else {
            tap_cfg->interrupts_enable = DISABLE;
        }
    }

    if (ret == 0) {
        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    /* Restore the request buffer pointer */
    request->buffer = context->i2c_transaction_buffer;

    return ret;
}

/*
  Get the interrupt which is routed to the INT2 pin
*/
static int lsm6dsm_driver_get_pin_int2_route(
    struct lsm6dsm_driver_context *context, struct lsm6dsm_int2_route *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_int2_ctrl *int2_ctrl =
        (struct lsm6dsm_int2_ctrl *)&request->buffer[0];
    struct lsm6dsm_md2_cfg *md2_cfg =
        (struct lsm6dsm_md2_cfg *)&request->buffer[1];
    struct lsm6dsm_drdy_pulse_cfg *drdy_pulse_cfg =
        (struct lsm6dsm_drdy_pulse_cfg *)&request->buffer[2];
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_INT2_CTRL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        val->int2_drdy_xl = int2_ctrl->int2_drdy_xl;
        val->int2_drdy_g = int2_ctrl->int2_drdy_g;
        val->int2_drdy_temp = int2_ctrl->int2_drdy_temp;
        val->int2_fth = int2_ctrl->int2_fth;
        val->int2_fifo_ovr = int2_ctrl->int2_fifo_ovr;
        val->int2_full_flag = int2_ctrl->int2_full_flag;
        val->int2_step_count_ov = int2_ctrl->int2_step_count_ov;
        val->int2_step_delta = int2_ctrl->int2_step_delta;

        request->ireg = LSM6DSM_MD2_CFG;
        request->buffer = md2_cfg;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }

        if (ret == 0) {
            val->int2_iron = md2_cfg->int2_iron;
            val->int2_tilt = md2_cfg->int2_tilt;
            val->int2_6d = md2_cfg->int2_6d;
            val->int2_double_tap = md2_cfg->int2_double_tap;
            val->int2_ff = md2_cfg->int2_ff;
            val->int2_wu = md2_cfg->int2_wu;
            val->int2_single_tap = md2_cfg->int2_single_tap;
            val->int2_inact_state = md2_cfg->int2_inact_state;

            request->ireg = LSM6DSM_DRDY_PULSE_CFG;
            request->buffer = drdy_pulse_cfg;

            ret = i2c_blocking_enqueue(i2c_context, request);
            if (ret < 0 || future_is_errored(&request->future)) {
                ret = -EIO;
            }
            val->int2_wrist_tilt = drdy_pulse_cfg->int2_wrist_tilt;
        }
    }

    /* Restore the request buffer pointer */
    request->buffer = context->i2c_transaction_buffer;

    return ret;
}

/*
  Set accelerometer full scale
*/
int lsm6dsm_driver_set_xl_full_scale(struct lsm6dsm_driver_context *context,
                                     enum lsm6dsm_fs_xl val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl1_xl *ctrl1_xl =
        (struct lsm6dsm_ctrl1_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL1_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        request->action = I2C_WRITE;
        ctrl1_xl->fs_xl = (uint8_t)val;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
  Get accelerometer full scale
*/
int lsm6dsm_driver_get_xl_full_scale(struct lsm6dsm_driver_context *context,
                                     enum lsm6dsm_fs_xl *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl1_xl *ctrl1_xl =
        (struct lsm6dsm_ctrl1_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL1_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    switch (ctrl1_xl->fs_xl) {
        case LSM6DSM_2g: {
            *val = LSM6DSM_2g;
        } break;

        case LSM6DSM_16g: {
            *val = LSM6DSM_16g;
        } break;

        case LSM6DSM_4g: {
            *val = LSM6DSM_4g;
        } break;

        case LSM6DSM_8g: {
            *val = LSM6DSM_8g;
        } break;

        default: {
            *val = LSM6DSM_2g;
        } break;
    }

    return ret;
}

/*
  Set accelerometer data rate
*/
int lsm6dsm_driver_set_xl_data_rate(struct lsm6dsm_driver_context *context,
                                    enum lsm6dsm_odr_xl val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl1_xl *ctrl1_xl =
        (struct lsm6dsm_ctrl1_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL1_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        ctrl1_xl->odr_xl = (uint8_t)val;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
  Get accelerometer data rate
*/
int lsm6dsm_driver_get_xl_data_rate(struct lsm6dsm_driver_context *context,
                                    enum lsm6dsm_odr_xl *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl1_xl *ctrl1_xl =
        (struct lsm6dsm_ctrl1_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL1_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    switch (ctrl1_xl->odr_xl) {
        case LSM6DSM_XL_ODR_OFF: {
            *val = LSM6DSM_XL_ODR_OFF;
        } break;

        case LSM6DSM_XL_ODR_12Hz5: {
            *val = LSM6DSM_XL_ODR_12Hz5;
        } break;

        case LSM6DSM_XL_ODR_26Hz: {
            *val = LSM6DSM_XL_ODR_26Hz;
        } break;

        case LSM6DSM_XL_ODR_52Hz: {
            *val = LSM6DSM_XL_ODR_52Hz;
        } break;

        case LSM6DSM_XL_ODR_104Hz: {
            *val = LSM6DSM_XL_ODR_104Hz;
        } break;

        case LSM6DSM_XL_ODR_208Hz: {
            *val = LSM6DSM_XL_ODR_208Hz;
        } break;

        case LSM6DSM_XL_ODR_416Hz: {
            *val = LSM6DSM_XL_ODR_416Hz;
        } break;

        case LSM6DSM_XL_ODR_833Hz: {
            *val = LSM6DSM_XL_ODR_833Hz;
        } break;

        case LSM6DSM_XL_ODR_1k66Hz: {
            *val = LSM6DSM_XL_ODR_1k66Hz;
        } break;

        case LSM6DSM_XL_ODR_3k33Hz: {
            *val = LSM6DSM_XL_ODR_3k33Hz;
        } break;

        case LSM6DSM_XL_ODR_6k66Hz: {
            *val = LSM6DSM_XL_ODR_6k66Hz;
        } break;

        case LSM6DSM_XL_ODR_1Hz6: {
            *val = LSM6DSM_XL_ODR_1Hz6;
        } break;

        default: {
            *val = LSM6DSM_XL_ODR_OFF;
        } break;
    }

    return ret;
}

static int lsm6dsm_driver_set_xl_filter_analog(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_bw0_xl val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl1_xl *ctrl1_xl =
        (struct lsm6dsm_ctrl1_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL1_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        ctrl1_xl->bw0_xl = (uint8_t)val;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}
static int lsm6dsm_driver_get_xl_filter_analog(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_bw0_xl *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl1_xl *ctrl1_xl =
        (struct lsm6dsm_ctrl1_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL1_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    switch (ctrl1_xl->bw0_xl) {
        case LSM6DSM_XL_ANA_BW_1k5Hz: {
            *val = LSM6DSM_XL_ANA_BW_1k5Hz;
        } break;

        case LSM6DSM_XL_ANA_BW_400Hz: {
            *val = LSM6DSM_XL_ANA_BW_400Hz;
        } break;

        default: {
            *val = LSM6DSM_XL_ANA_BW_1k5Hz;
        } break;
    }

    return ret;
}

static int lsm6dsm_driver_set_xl_lp2_bandwidth(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_input_composite val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl8_xl *ctrl8_xl =
        (struct lsm6dsm_ctrl8_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL8_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    if (ret == 0) {
        ctrl8_xl->input_composite = ((uint8_t)val & 0x10U) >> 4;
        ctrl8_xl->hpcf_xl = (uint8_t)val & 0x03U;
        ctrl8_xl->lpf2_xl_en = 1;
        ctrl8_xl->hp_slope_xl_en = 0;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            return -EIO;
        }
    }

    return ret;
}

static int lsm6dsm_driver_get_xl_lp2_bandwidth(
    struct lsm6dsm_driver_context *context, enum lsm6dsm_input_composite *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl8_xl *ctrl8_xl =
        (struct lsm6dsm_ctrl8_xl *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL8_XL;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        return -EIO;
    }

    if ((ctrl8_xl->lpf2_xl_en == 0x00U) ||
        (ctrl8_xl->hp_slope_xl_en != 0x00U)) {
        *val = LSM6DSM_XL_LP_NOT_AVAILABLE;
    } else {
        switch ((ctrl8_xl->input_composite << 4) + ctrl8_xl->hpcf_xl) {
            case LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_50: {
                *val = LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_50;
            } break;

            case LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_100: {
                *val = LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_100;
            } break;

            case LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_9: {
                *val = LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_9;
            } break;

            case LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_400: {
                *val = LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_400;
            } break;

            case LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_50: {
                *val = LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_50;
            } break;

            case LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_100: {
                *val = LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_100;
            } break;

            case LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_9: {
                *val = LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_9;
            } break;

            case LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_400: {
                *val = LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_400;
            } break;

            default: {
                *val = LSM6DSM_XL_LOW_LAT_LP_ODR_DIV_50;
            } break;
        }
    }

    return ret;
}

/*
  Set gyroscope full scale
*/
int lsm6dsm_driver_set_gy_full_scale(struct lsm6dsm_driver_context *context,
                                     enum lsm6dsm_fs_g val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl2_g *ctrl2_g = (struct lsm6dsm_ctrl2_g *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL2_G;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        ctrl2_g->fs_g = (uint8_t)val;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
  Get gyroscope full scale
*/
int lsm6dsm_driver_get_gy_full_scale(struct lsm6dsm_driver_context *context,
                                     enum lsm6dsm_fs_g *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl2_g *ctrl2_g = (struct lsm6dsm_ctrl2_g *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL2_G;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    switch (ctrl2_g->fs_g) {
        case LSM6DSM_250dps: {
            *val = LSM6DSM_250dps;
        } break;

        case LSM6DSM_125dps: {
            *val = LSM6DSM_125dps;
        } break;

        case LSM6DSM_500dps: {
            *val = LSM6DSM_500dps;
        } break;

        case LSM6DSM_1000dps: {
            *val = LSM6DSM_1000dps;
        } break;

        case LSM6DSM_2000dps: {
            *val = LSM6DSM_2000dps;
        } break;

        default: {
            *val = LSM6DSM_250dps;
        } break;
    }

    return ret;
}

/*
  Set gyroscope data rate
*/
int lsm6dsm_driver_set_gy_data_rate(struct lsm6dsm_driver_context *context,
                                    enum lsm6dsm_odr_g val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl2_g *ctrl2_g = (struct lsm6dsm_ctrl2_g *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL2_G;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        ctrl2_g->odr_g = (uint8_t)val;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }
    }

    return ret;
}

/*
  Get gyroscope data rate
*/
int lsm6dsm_driver_get_gy_data_rate(struct lsm6dsm_driver_context *context,
                                    enum lsm6dsm_odr_g *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl2_g *ctrl2_g = (struct lsm6dsm_ctrl2_g *)request->buffer;
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL2_G;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    switch (ctrl2_g->odr_g) {
        case LSM6DSM_GY_ODR_OFF: {
            *val = LSM6DSM_GY_ODR_OFF;
        } break;

        case LSM6DSM_GY_ODR_12Hz5: {
            *val = LSM6DSM_GY_ODR_12Hz5;
        } break;

        case LSM6DSM_GY_ODR_26Hz: {
            *val = LSM6DSM_GY_ODR_26Hz;
        } break;

        case LSM6DSM_GY_ODR_52Hz: {
            *val = LSM6DSM_GY_ODR_52Hz;
        } break;

        case LSM6DSM_GY_ODR_104Hz: {
            *val = LSM6DSM_GY_ODR_104Hz;
        } break;

        case LSM6DSM_GY_ODR_208Hz: {
            *val = LSM6DSM_GY_ODR_208Hz;
        } break;

        case LSM6DSM_GY_ODR_416Hz: {
            *val = LSM6DSM_GY_ODR_416Hz;
        } break;

        case LSM6DSM_GY_ODR_833Hz: {
            *val = LSM6DSM_GY_ODR_833Hz;
        } break;

        case LSM6DSM_GY_ODR_1k66Hz: {
            *val = LSM6DSM_GY_ODR_1k66Hz;
        } break;

        case LSM6DSM_GY_ODR_3k33Hz: {
            *val = LSM6DSM_GY_ODR_3k33Hz;
        } break;

        case LSM6DSM_GY_ODR_6k66Hz: {
            *val = LSM6DSM_GY_ODR_6k66Hz;
        } break;

        default: {
            *val = LSM6DSM_GY_ODR_OFF;
        } break;
    }

    return ret;
}

/*
  Set gyroscope band-pass filter configuration
*/
int lsm6dsm_driver_set_gy_band_pass(struct lsm6dsm_driver_context *context,
                                    enum lsm6dsm_lpf1_sel_g val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl4_c *ctrl4_c =
        (struct lsm6dsm_ctrl4_c *)&request->buffer[0];
    struct lsm6dsm_ctrl6_c *ctrl6_c =
        (struct lsm6dsm_ctrl6_c *)&request->buffer[1];
    struct lsm6dsm_ctrl7_g *ctrl7_g =
        (struct lsm6dsm_ctrl7_g *)&request->buffer[2];
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL7_G;
    request->buffer = ctrl7_g;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        ctrl7_g->hpm_g = ((uint8_t)val & 0x30U) >> 4;
        ctrl7_g->hp_en_g = ((uint8_t)val & 0x80U) >> 7;

        request->action = I2C_WRITE;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }

        if (ret == 0) {
            request->action = I2C_READ;
            request->ireg = LSM6DSM_CTRL6_C;
            request->buffer = ctrl6_c;

            ret = i2c_blocking_enqueue(i2c_context, request);
            if (ret < 0 || future_is_errored(&request->future)) {
                ret = -EIO;
            }

            if (ret == 0) {
                ctrl6_c->ftype = (uint8_t)val & 0x03U;

                request->action = I2C_WRITE;

                ret = i2c_blocking_enqueue(i2c_context, request);
                if (ret < 0 || future_is_errored(&request->future)) {
                    ret = -EIO;
                }

                if (ret == 0) {
                    request->action = I2C_READ;
                    request->ireg = LSM6DSM_CTRL4_C;
                    request->buffer = ctrl4_c;

                    ret = i2c_blocking_enqueue(i2c_context, request);
                    if (ret < 0 || future_is_errored(&request->future)) {
                        ret = -EIO;
                    }

                    if (ret == 0) {
                        ctrl4_c->lpf1_sel_g = ((uint8_t)val & 0x08U) >> 3;

                        request->action = I2C_WRITE;

                        ret = i2c_blocking_enqueue(i2c_context, request);
                        if (ret < 0 || future_is_errored(&request->future)) {
                            ret = -EIO;
                        }
                    }
                }
            }
        }
    }

    /* Restore the request buffer pointer */
    request->buffer = context->i2c_transaction_buffer;

    return ret;
}

/*
  Get gyroscope band-pass filter configuration
*/
int lsm6dsm_driver_get_gy_band_pass(struct lsm6dsm_driver_context *context,
                                    enum lsm6dsm_lpf1_sel_g *val) {
    struct i2c_driver_context *i2c_context = context->i2c_context;
    struct i2c_request *request = &context->request;
    struct lsm6dsm_ctrl4_c *ctrl4_c =
        (struct lsm6dsm_ctrl4_c *)&request->buffer[0];
    struct lsm6dsm_ctrl6_c *ctrl6_c =
        (struct lsm6dsm_ctrl6_c *)&request->buffer[1];
    struct lsm6dsm_ctrl7_g *ctrl7_g =
        (struct lsm6dsm_ctrl7_g *)&request->buffer[2];
    int ret;

    request->action = I2C_READ;
    request->ireg = LSM6DSM_CTRL6_C;
    request->buffer = ctrl6_c;
    request->num_bytes = 1;

    ret = i2c_blocking_enqueue(i2c_context, request);
    if (ret < 0 || future_is_errored(&request->future)) {
        ret = -EIO;
    }

    if (ret == 0) {
        request->ireg = LSM6DSM_CTRL4_C;
        request->buffer = ctrl4_c;

        ret = i2c_blocking_enqueue(i2c_context, request);
        if (ret < 0 || future_is_errored(&request->future)) {
            ret = -EIO;
        }

        if (ret == 0) {
            request->ireg = LSM6DSM_CTRL7_G;
            request->buffer = ctrl7_g;

            ret = i2c_blocking_enqueue(i2c_context, request);
            if (ret < 0 || future_is_errored(&request->future)) {
                ret = -EIO;
            }

            switch ((ctrl7_g->hp_en_g << 7) + (ctrl7_g->hpm_g << 4) +
                    (ctrl4_c->lpf1_sel_g << 3) + ctrl6_c->ftype) {
                case LSM6DSM_HP_16mHz_LP2: {
                    *val = LSM6DSM_HP_16mHz_LP2;
                } break;

                case LSM6DSM_HP_65mHz_LP2: {
                    *val = LSM6DSM_HP_65mHz_LP2;
                } break;

                case LSM6DSM_HP_260mHz_LP2: {
                    *val = LSM6DSM_HP_260mHz_LP2;
                } break;

                case LSM6DSM_HP_1Hz04_LP2: {
                    *val = LSM6DSM_HP_1Hz04_LP2;
                } break;

                case LSM6DSM_HP_DISABLE_LP1_LIGHT: {
                    *val = LSM6DSM_HP_DISABLE_LP1_LIGHT;
                } break;

                case LSM6DSM_HP_DISABLE_LP1_NORMAL: {
                    *val = LSM6DSM_HP_DISABLE_LP1_NORMAL;
                } break;

                case LSM6DSM_HP_DISABLE_LP_STRONG: {
                    *val = LSM6DSM_HP_DISABLE_LP_STRONG;
                } break;

                case LSM6DSM_HP_DISABLE_LP1_AGGRESSIVE: {
                    *val = LSM6DSM_HP_DISABLE_LP1_AGGRESSIVE;
                } break;

                case LSM6DSM_HP_16mHz_LP1_LIGHT: {
                    *val = LSM6DSM_HP_16mHz_LP1_LIGHT;
                } break;

                case LSM6DSM_HP_65mHz_LP1_NORMAL: {
                    *val = LSM6DSM_HP_65mHz_LP1_NORMAL;
                } break;

                case LSM6DSM_HP_260mHz_LP1_STRONG: {
                    *val = LSM6DSM_HP_260mHz_LP1_STRONG;
                } break;

                case LSM6DSM_HP_1Hz04_LP1_AGGRESSIVE: {
                    *val = LSM6DSM_HP_1Hz04_LP1_AGGRESSIVE;
                } break;

                default: {
                    *val = LSM6DSM_HP_65mHz_LP2;
                } break;
            }
        }
    }

    /* Restore the request buffer pointer */
    request->buffer = context->i2c_transaction_buffer;

    return ret;
}

float lsm6dsm_driver_get_x_acc(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->x_acc;
}
float lsm6dsm_driver_get_y_acc(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->y_acc;
}
float lsm6dsm_driver_get_z_acc(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->z_acc;
}

float lsm6dsm_driver_get_x_ang(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->x_ang;
}

float lsm6dsm_driver_get_y_ang(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->y_ang;
}

float lsm6dsm_driver_get_z_ang(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->z_ang;
}

enum lsm6dsm_state lsm6dsm_driver_get_state(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->state;
}
void lsm6dsm_driver_set_state(const struct lsm6dsm_driver *const dev,
                              enum lsm6dsm_state state) {
    struct lsm6dsm_driver_context *context = dev->context;

    context->state = state;
}

enum lsm6dsm_it_state lsm6dsm_driver_get_it_state(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->it_state;
}

void lsm6dsm_driver_set_it_state(const struct lsm6dsm_driver *const dev,
                                 enum lsm6dsm_it_state it_state) {
    struct lsm6dsm_driver_context *context = dev->context;

    context->it_state = it_state;
}

tilt_flags lsm6dsm_driver_get_tilt_flags(
    const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return context->tilt_flags;
}

void lsm6dsm_driver_clear_tilt_flags(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    memset(&context->tilt_flags, 0, sizeof(context->tilt_flags));
}

tap_flags lsm6dsm_driver_get_tap_flags(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    return *(tap_flags *)&context->tap_flags;
}

void lsm6dsm_driver_clear_tap_flags(const struct lsm6dsm_driver *const dev) {
    struct lsm6dsm_driver_context *context = dev->context;

    memset(&context->tap_flags, 0, sizeof(context->tap_flags));
}