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
    CTRL7 = 0x8,
    CTRL8 = 0x9,
    CTRL9 = 0xA,
    CTRL10 = 0xB,
    CTRL11 = 0xC,
    CTRL12 = 0xD,
    CTRL13 = 0xE
};

// Size of the n x n matrix
#define N_DIMENSIONS 7

// Number of leds and the size of the control array
#define NUM_LEDS (N_DIMENSIONS * N_DIMENSIONS)

/*
    List of all LEDs possible to turn on.

    The DEFAULT_LED takes up the 0th index to allow for
    writing 0 to have no effect. This is used to avoid checking
    if an led is on or not.

    Setting an LED requires setting the led's value in an array to
    itself. Ex: leds[D9] = D9;
*/

enum leds {
    DEFAULT_LED,  // this is 0x0 and will not turn on any leds
    D67,
    D5,
    D9,
    D7,
    D57,
    D58,
    D83,

    D17,
    D71,
    D19,
    D18,
    D59,
    D24,
    D23,

    D75,
    D12,
    D13,
    D11,
    D25,
    D79,
    D26,

    D68,
    D6,
    D10,
    D8,
    D60,
    D61,
    D84,

    D20,
    D72,
    D22,
    D21,
    D62,
    D27,
    D80,

    D76,
    D15,
    D16,
    D14,
    D28,
    D32,
    D33,

    D29,
    D31,
    D30,
    D36,
    D37,
    D35,
    D34,
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
