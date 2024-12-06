#include "pam8302a_driver.h"

#include "uart_logger.h"

/* Global PAM8302A Driver Instance */
struct pam8302a_driver pam8302a_driver = {
    .config =
        {
            .enable_pin = GPIO_PIN(B, 12),
        },
    .context = {.initialized = false, .enabled = false},
};

/* Initializes GPIO */
static void gpio_init(struct pam8302a_driver *pam8302a_driver) {
    struct pam8302a_driver_config *cfg = &pam8302a_driver->config;

    /* Declare GPIO initialization structure */
    GPIO_InitTypeDef gpio = {0};

    /************************/
    /* Configure Enable Pin */
    /************************/

    /* GPIOx Peripheral Clock Enable */
    GPIOx_CLK_ENABLE(cfg->enable_pin.port);

    /* Configure Enable Pin as high-speed push-pull output */
    gpio.Pin = cfg->enable_pin.pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLDOWN;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    /* Initialize GPIO port with configured structure */
    HAL_GPIO_Init(cfg->enable_pin.port, &gpio);
}

/* Enables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_enable(struct pam8302a_driver *pam8302a_driver) {
    struct pam8302a_driver_config *cfg = &pam8302a_driver->config;
    struct pam8302a_driver_context *context = &pam8302a_driver->context;
    if (context->initialized) {
        HAL_GPIO_WritePin(cfg->enable_pin.port, cfg->enable_pin.pin,
                          GPIO_PIN_SET);
        context->enabled = true;
        return 0;
    } else {
        return -1;
    }
}

/* Disables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_disable(struct pam8302a_driver *pam8302a_driver) {
    struct pam8302a_driver_config *cfg = &pam8302a_driver->config;
    struct pam8302a_driver_context *context = &pam8302a_driver->context;
    if (context->initialized) {
        HAL_GPIO_WritePin(cfg->enable_pin.port, cfg->enable_pin.pin,
                          GPIO_PIN_RESET);
        context->enabled = false;
        return 0;
    } else {
        return -1;
    }
}

/* Initializes PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_init(struct pam8302a_driver *pam8302a_driver) {
    struct pam8302a_driver_context *context = &pam8302a_driver->context;

    /* If enabled, disable before continuing */
    if (context->enabled) {
        int ret;
        if ((ret = pam8302a_driver_disable(pam8302a_driver))) {
            return ret;
        }
    }

    /* Clear initialization flag */
    context->initialized = false;

    /* Initialize GPIO */
    gpio_init(pam8302a_driver);

    /* Reflect post-init state */
    context->enabled = false;
    context->initialized = true;
    return 0;
}
