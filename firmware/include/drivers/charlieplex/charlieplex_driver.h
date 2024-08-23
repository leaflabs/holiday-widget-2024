#ifndef CHARLIEPLEX_DRIVER_H
#define CHARLIEPLEX_DRIVER_H

#include "stm32l0xx_hal.h"

/*

    How this driver works

    Drawing a frame requires setting which
    leds should be on, and then calling:
    charlieplex_driver_draw(...);

    The input is an array of uint8_t's where each element corrosponds
    to the enum value in 'enum leds'. To set an led, set the led's index
    to itself. Ex: arr[D9] = D9;. To turn an led off, set it to 0:
    arr[D9] = 0;

    Turning on and off LEDs:
    An LED is turned on if and only if its two GPIO pins (control lines) are
    pulled to their corrosponding state. In other terms, one GPIO pin is pulled
    HIGH and the other is pulled LOW. This allows current to flow from the one
    pin to the other while turning on the led. To avoid other LEDs from turning
    on, they are set to INPUT mode (aka High Impedance) which prevents current
    from flowing through that pin and turning on other leds.

    So the steps to turn on an led are pull one control line HIGH and another
    control line LOW. Multiple leds can be on at the same time if they share
    the same HIGH control line or same LOW control line or do not share any
    control lines. This is evident (and used) with Control lines 0 and 4, 1 and
    5, 2 and 6, and 3 and 7 as their HIGH contol lines are never used between
    any LEDs they share. The mapping of 0 to 4 is arbitary. As long as the
    leds that rely on the control lines do not share between each other, those
    leds can be on at the same time.

    This driver splits a 'frame' to draw into 4 'sub-frames' to allow for
    multiple leds to be 'on' (visibly bright) despite sharing opposing control
    lines. Drawing a frame requires going through all 4 sub frames in quick
    succession to give every LED a chance to be on without conflicting with
    other LEDs that share its control lines. Each subframe is setup so the LEDs
    that share a HIGH control line are on at the same time.


    Performance metrics:
    - Running on an STM32L072

    Current draw per led: 3.92 mA
    Max current draw per control line: 11.78 mA

    Per the datasheet:
    Max current a single GPIO pin can source/sink: 25 mA
    Therefore, this driver is within the current limit.

    Average time to draw a frame:
    With 24 leds being checked: 1.127 ms
    With 49 leds being checked: 1.89 ms

*/

// Size of the n x n matrix
#define N_DIMENSIONS 7

// Number of leds and the size of the control array
#define NUM_LEDS (N_DIMENSIONS * N_DIMENSIONS) + 1

enum led_frame {
    LED_FRAME_1,
    LED_FRAME_2,
    LED_FRAME_3,
    LED_FRAME_4,
    LED_FRAME_5,
    __NUM_LED_FRAMES,
};

struct charlieplex_driver;

/*
    Set up the GPIO pins and clocks for charlieplexing and
    set all leds to off.
*/
void charlieplex_driver_init(void);

/*
    Reset all led pins to INPUT which turns all leds off.
*/
void charlieplex_driver_reset_pins(void);

/*
    Draw a 'frame' of leds.

    'leds' is a pointer to an array of 50 elements. Each slot corrosponds to
    the enum 'leds' defined above.
*/
void charlieplex_driver_draw(uint8_t *leds);

void pause_charlieplex_driver(void);

void unpause_charlieplex_driver(void);

void get_duration();

extern const struct charlieplex_driver *const charlieplex_driver;

#endif
