#ifndef __PAM8302A_DRIVER_H__
#define __PAM8302A_DRIVER_H__
#include "generic_gpio.h"
#include "stdbool.h"
#include "utils.h"

/* PAM8302A driver config structure */
struct pam8302a_driver_config {
    const struct gpio_pin_config enable_pin;
};

/* PAM8302A driver context structure */
struct pam8302a_driver_context {
    bool initialized;
    bool enabled;
};

/* PAM8302A driver structure */
struct pam8302a_driver {
    struct pam8302a_driver_config config;
    struct pam8302a_driver_context context;
};

/* Initializes GPIO */
int pam8302a_driver_init(struct pam8302a_driver *pam8302a_driver);

/* Enables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_enable(struct pam8302a_driver *pam8302a_driver);

/* Disables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_disable(struct pam8302a_driver *pam8302a_driver);

/* Global PAM8302A Driver Instance */
extern struct pam8302a_driver pam8302a_driver;

#endif