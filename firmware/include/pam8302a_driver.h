#ifndef __PAM8302A_DRIVER_H__
#define __PAM8302A_DRIVER_H__
#include "stdbool.h"
#include "stm32l0xx_hal.h"

/* PAM8302A structure */
struct pam8302a_driver {
    bool initialized;
    bool enabled;
};

/* Initializes GPIO */
int pam8302a_driver_init();

/* Enables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_enable();

/* Disables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_disable();

#endif