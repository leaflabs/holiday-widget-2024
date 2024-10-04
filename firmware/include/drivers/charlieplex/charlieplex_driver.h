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

/*
    List every control line (GPIO pin)
    so we can index into an array with the value.

    Notice the DEFAULT_CONTROL. This allows for dummy writes
    by taking up the 0 index
*/
enum controls {
    DEFAULT_CONTROL = 0x0,
    CTRL0 = 0x1,
    CTRL1 = 0x2,
    CTRL2 = 0x3,
    CTRL3 = 0x4,
    CTRL4 = 0x5,
    CTRL5 = 0x6,
    CTRL6 = 0x7,
    CTRL7 = 0x8
};

// Size of the n x n matrix
#define N_DIMENSIONS 7

// Space for unused slots
#define DUMMY_SLOTS 1

// Number of leds and the size of the control array
#define NUM_LEDS ((N_DIMENSIONS * N_DIMENSIONS) + DUMMY_SLOTS)

/*
    List of all LEDs possible to turn on.

    The DEFAULT_LED takes up the 0th index to allow for
    writing 0 to have no effect. This is used to avoid checking
    if an led is on or not.

    Setting an LED requires setting the led's value in an array to
    itself. Ex: leds[D9] = D9;

    As of now, not all leds are in the hardware so dummy slots
    are added to simulate the full matrix. Leds that are
    labeled as D_* are dummy slots.
*/
enum leds {
    DEFAULT_LED,  // this is 0x0 and will not turn on any leds
    D7,
    D5,
    D6,
    D23,
    D24,
    D_1,
    D_2,

    D28,
    D10,
    D8,
    D21,
    D25,
    D_3,
    D_4,

    D9,
    D26,
    D19,
    D13,
    D20,
    D_5,
    D_6,

    D27,
    D16,
    D15,
    D22,
    D11,
    D_7,
    D_8,

    D18,
    D17,
    D14,
    D12,
    D_9,
    D_10,
    D_11,

    D_12,
    D_13,
    D_14,
    D_15,
    D_16,
    D_17,
    D_18,

    D_19,
    D_20,
    D_21,
    D_22,
    D_23,
    D_24,
    D_25
};

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

#endif
