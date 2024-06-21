#ifndef CHARLIEPLEX_DRIVER_H
#define CHARLIEPLEX_DRIVER_H

#include "stm32l0xx_hal.h"

/*

    How this driver works

    Drawing a frame requires setting which
    leds should be on, and then calling:
    charlieplex_driver_draw(&context);

    Setting an led requires fliping the corrosponding bit
    in 'leds' (specified in the context struct).
    The '0th' bit of 'leds' is reserved. This is
    utilized to checking if a bit is set by having
    a zero value do nothing.

    Example program:

    struct charlieplex_context context = {0};

    context.leds |= SET_LED(D5);  // Turn on LED D5
    context.leds |= SET_LED(D14); // Turn on LED D14

    // Draw the 'frame' we specified
    charlieplex_driver_draw(&context);


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

    With 0  LEDs on: .017A
    With 24 LEDs on: .035A
    Current draw due to LEDs: .018A

    Per the datasheet:
    Max current a single GPIO pin can source/sink: .025A
    Therefore, this driver is within the current limit.

    Average time to draw a frame:
    1.84 ms per frame.

*/

// Turn on and LED
#define SET_LED(x) (1 << x)

/*
    Context struct for charlieplexing driver.
    Only stores the uint32_t for specifying which
    leds to turn on
*/
struct charlieplex_context {
    uint32_t leds;
};

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

/*
    List of all LEDs possible to turn on.

    Turning on an led requries setting the correct bit in a 32 bit integer.
    Taking the value from this enum and shifting 1 over that many positions
    will enable the led. Ex: (1 << D5) has the mask to enable the D5 LED.

    The DEFAULT_LED takes up the 0 index to allow for dummy writes.

    The leds listed after the default are ordered so groups of 3 share the
    same control line being HIGH.
*/
enum leds {
    DEFAULT_LED,  // this is 0x0 and will not turn on any leds
    D5,
    D7,
    D9,
    D11,
    D12,
    D13,
    D17,
    D18,
    D19,
    D23,
    D24,
    D25,
    D6,
    D8,
    D10,
    D14,
    D15,
    D16,
    D20,
    D21,
    D22,
    D26,
    D27,
    D28
};

/*
    Specified in the .c file for the driver is an array of instructions
    required to draw a 'frame' of the leds. This is a list of the other
    instructions besides turning on an LED.

    Notice that the 'FRAMEx' and CLR values are mapped after the D28 led so they
    have unique values when using a switch-case to evaluate instructions
*/
enum instructions {
    // 'FRAMEx' instruction sets up a frame for drawing leds.
    FRAME1 = (D28 + 1),
    FRAME2 = (D28 + 2),
    FRAME3 = (D28 + 3),
    FRAME4 = (D28 + 4),

    // Clear instruction to turn off all leds
    CLR = (D28 + 5)
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

    'context' is a pointer to the context struct for this driver.

    In context:
    'leds' is an adjustable mask for specifying which leds should be on.
    Specify an led as 'on' by setting its bit to a 1 based on the 'enum leds'
    value as the bit index.
*/
void charlieplex_driver_draw(struct charlieplex_context *context);

#endif
