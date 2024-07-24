#include "pam8302a_driver.h"

#include "uart_logger.h"

/* Private PAM8302A instance */
static struct pam8302a_driver pam8302a_driver;

/* Initializes GPIO */
static void GPIO_init(void) {
    /* GPIOB Peripheral Clock Enable */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Declare GPIO initialization structure */
    GPIO_InitTypeDef gpio = {0};

    /* Configure Pin B12 (audio_en) as high-speed push-pull output */
    gpio.Pin = GPIO_PIN_12;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    /* Initialize GPIO port B with configured structure */
    HAL_GPIO_Init(GPIOB, &gpio);
}

/* Enables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_enable() {
    if (pam8302a_driver.initialized) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
        pam8302a_driver.enabled = true;
        return 0;
    } else {
        return -1;
    }
}

/* Disables PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_disable() {
    if (pam8302a_driver.initialized) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
        pam8302a_driver.enabled = false;
        return 0;
    } else {
        return -1;
    }
}

/* Initializes PAM8302A Amplifier, returns 0 on success, -1 on failure */
int pam8302a_driver_init(void) {
    if (pam8302a_driver.enabled) {
        pam8302a_driver_disable();
    }

    pam8302a_driver.initialized = false;

    GPIO_init();

    pam8302a_driver.enabled = false;
    pam8302a_driver.initialized = true;
    return 0;
}