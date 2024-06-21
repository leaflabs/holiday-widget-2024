#include "charlieplex_driver.h"

#include "stm32l0xx_hal.h"

void charlieplex_driver_init(void) {
    // Enable clocks
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Do the basic initalization steps to activate all pins
    // used for charlieplexing
    charlieplex_driver_reset_pins();
}

void charlieplex_driver_reset_pins(void) {
    // Struct for initalization
    GPIO_InitTypeDef gpio = {0};

    /*
        Set all pins to Input so they are high impedance.
    */

    // Port B
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_10 |
               GPIO_PIN_11;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);

    // Port A
    gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}

/*
    Define the bitmasks to clear the GPIOA and GPIOB
    MODER registers. These defines are left in this file
    to 'hide' them from other files
*/

// Sets pins 0 and 1 to INPUT
#define SET_TO_INPUT_A ~(3 << 0 | 3 << 2)

// Sets pins 0, 1, 4, 5, 10, 11 to INPUT
#define SET_TO_INPUT_B ~(3 << 0 | 3 << 2 | 3 << 8 | 3 << 10 | 3 << 20 | 3 << 22)

/*
    Before writing the next subframe, reset every pin to high impedance
    (INPUT) so all leds turn off. HAL_GPIO_Init checks for every combination
    of the typedef but only one option is set here. So to bypass this,
    we write directly to MODER to set each pin to INPUT.

    The 'SET_TO_INPUT_x' defines above have the masks: 0b11 (3) shifted
    to the correct location and bitwise negated to allow for a single &=
    to clear all pins.
*/
static inline void charlieplex_clear(void) {
    // Clearing all control lines is as simple as writing each pin to INPUT
    GPIOA->MODER &= SET_TO_INPUT_A;
    GPIOB->MODER &= SET_TO_INPUT_B;
}

/*
    In order to set a control line to HIGH,
    the MODER register bits for the pin need
    to be set to OUTPUT (Maks is 0b01) and the BRR register
    needs a 1 written to the corrosponding bit.

    BRR is used rather than ODR as it is only a write so
    there is no risk of interrupts messing with what was
    read before modify/writing it back.

    'mode' should contain the mask (01) already shifted the
    correct number of bits so the user only needs to |= the value.

    'brr' should contain the GPIO pin value so they only need to
    set it. This value can also be used for BSRR.
*/
struct mode_odr {
    uint32_t mode;
    uint32_t brr;
};

/*
    Save the preshifted values to set a pin to OUTPUT
    and to set a pin to write 'HIGH'.
    To set to OUTPUT mode, |= the 'mode' value
    To set output to HIGH, set the BRR register to 'brr'

    The first entry to this array are the dummy values to have
    no effect. Writing 0x0 to OUTPUT and 0x0 to BRR will not change
    the state of the register.
*/
static const struct mode_odr mode_odr_array[] = {
    {0x0, 0x0},  // Write all zeros to MODER and BRR to have no effect

    // Actual mode_brr values begin here. These contain the preshifted
    // Values to enable OUTPUT mode and to set the BRR register
    {0x1, GPIO_PIN_0},
    {0x4, GPIO_PIN_1},
    {0x100000, GPIO_PIN_10},
    {0x400000, GPIO_PIN_11},
    {0x100, GPIO_PIN_4},
    {0x400, GPIO_PIN_5},
    {0x1, GPIO_PIN_0},
    {0x4, GPIO_PIN_1}};

/*
    For each control line (enum controls), define the GPIOx port and
    GPIO_PIN_x pin
*/
struct control_port_pin {
    GPIO_TypeDef *port;
    uint32_t pin;
};

/*
    List each GPIOx port and the GPIO_PIN_x pin for each control line as
    a struct for fast access. Struct referenced above.
*/
static const struct control_port_pin ports_and_pins[9] = {
    {GPIOA, GPIO_PIN_0},  // Default port and pin. Used only for dummy writes

    // Actual values begin here
    {GPIOA, GPIO_PIN_0},
    {GPIOA, GPIO_PIN_1},
    {GPIOB, GPIO_PIN_10},
    {GPIOB, GPIO_PIN_11},
    {GPIOB, GPIO_PIN_4},
    {GPIOB, GPIO_PIN_5},
    {GPIOB, GPIO_PIN_0},
    {GPIOB, GPIO_PIN_1}};

/*
    Turn on a control line by setting the GPIO pin mode to
    OUTPUT and writing a 1 to the correct bit for BSRR.
    BSRR lets the user set the pin to HIGH without using a
    read-modify-write on ODR.

    'ctrl' is the enum value corrosponding to which control
    line to set. this is used to index into the ports_and_pins
    array for the GPIOx and GPIO_PIN_x values and also to get
    the correct MODER mask and BSRR value

*/
static inline void charlieplex_switch_on(enum controls ctrl) {
    // Get the port and pin for this control line
    struct control_port_pin portpin = ports_and_pins[ctrl];

    // Set mode to output
    portpin.port->MODER |= mode_odr_array[ctrl].mode;

    // BSRR because we want to set the pin
    portpin.port->BSRR = mode_odr_array[ctrl].brr;
}

/*
    Bring a control line to LOW by setting the GPIO pin mode
    to OUTPUT and writing a 1 to the correct bit for BRR.
    BRR lets the user set the pin to LOW without using a
    read-modify-write on ODR.

    'ctrl' is the enum value corrosponding to which control
    line to set. This is used to index into the ports_and_pins
    array for the GPIOx and GPIO_PIN_x values and also to get
    the correct MODER mask and BRR value.
*/
static inline void charlieplex_switch_off(enum controls ctrl) {
    // Get the port and pin for this control line
    struct control_port_pin portpin = ports_and_pins[ctrl];

    // Set mode to output
    portpin.port->MODER |= mode_odr_array[ctrl].mode;

    // BRR because we want to unset the pin
    portpin.port->BRR = mode_odr_array[ctrl].brr;
}

/*
    To turn an LED on, the correct gpio pin must be set to HIGH and
    the other correct gpio pin must be set to LOW. These are the control
    lines mentioned above. For each LED (defined in enum leds), index into
    the array to get the correct control line to pull low to turn it on.
    These values are obtained from the schematic for the board as it defines
    which lines need to be set high or low to turn on an LED.

    A 'high' version of this array is not needed as they are manually set to
    avoid redundant writes to turn on a control line when setting several pins
    in the same 'sub frame'

    Notice the DEFAULT_CONTROL entry which is for dummy reads. This is to map
    the 'enum leds' indexes to this array correctly.
*/
static const enum controls low[25] = {
    DEFAULT_CONTROL,  // Dummy slot to match up with 'enum leds'
    CTRL1,           CTRL2, CTRL3, CTRL0, CTRL1, CTRL3, CTRL0, CTRL2,
    CTRL3,           CTRL0, CTRL1, CTRL2, CTRL5, CTRL6, CTRL7, CTRL4,
    CTRL5,           CTRL7, CTRL4, CTRL6, CTRL7, CTRL4, CTRL5, CTRL6};

/*
    Turns on an led by switching the correct control line to LOW.
    You must bring the correct control line to HIGH before calling the
    function. This is an optimization to prevent several leds with the
    same 'HIGH' control line from redundantly turning it on with several
    function calls.

    'led' is an 'enum leds' entry which is used to get the corrosponding
    control line from the above array and setting that to LOW.

    Writing a led value of 0 (DEFAULT_LED) does not turn on or off any leds.
    A call with a value of 0 has no effect and does not modify any other
    LEDs already enabled
*/
static inline void charlieplex_led_on(enum leds led) {
    // Switch off the lower half since the 'high' pin is already set high
    enum controls low_ctrl = low[led];

    // Switch off this control line
    charlieplex_switch_off(low_ctrl);
}

/*
    When calling the 'draw' function, there are several steps to correctly
    flash the leds. For easy readability and modifications and for performance
    improvements, the list of 'instructions' are saved in this array.

    'CLR' is the action to 'Clear' all current leds. This sets the mode for all
    GPIO pins to INPUT so they are 'disconnected' from the circuit.

    'FRAMEx' is the action to bring HIGH the two control lines so that turning
    on an LED only requires bringing the other control line. This instruction
    must come before setting any leds.

    'Dx' is the action to turn an led on by bringing its control line LOW.

    Example:
    FRAME1 -> Bring the control lines for frame 1 to high
    D5, D7, D9... -> Bring the other control line to low if that pin is set
    CLR -> Bring the control lines to INPUT so they turn off and are reset

*/
static const uint32_t instrs[] = {
    // Inital clear
    CLR,

    // First frame
    FRAME1, D5, D7, D9, D6, D8, D10, CLR,

    // Second frame
    FRAME2, D11, D12, D13, D14, D15, D16, CLR,

    // Third frame
    FRAME3, D17, D18, D19, D20, D21, D22, CLR,

    // Fourth frame
    FRAME4, D23, D24, D25, D26, D27, D28, CLR};

/*
    Draw the leds specified in the context struct.

    Specifying an LED is as simple as taking the 'enum led' value
    and setting the bit it corrosponds to.

    Ex: To set D14 to be on:
    uint32_t leds = 0;  // No leds on
    leds |= (1 << D14); // Turn on D14 led

    There is no concern for LEDs sharing control lines, that is
    already handled.

    This is a constant time function. Specifying 1 led vs
    all 24 leds will take the same time to light up and turn off.
    This is due to avoiding branches specific to a particular led.

*/
void charlieplex_driver_draw(struct charlieplex_context *context) {
    uint32_t leds = context->leds;

    // Iterate over the list of instructions
    for (int i = 0; i < sizeof(instrs) / sizeof(uint32_t); i++) {
        // Get the current instruction from the array.
        uint32_t instr = instrs[i];

        /*
            Execute the correct functionality of the
            instruction provided from the array.
        */
        switch (instr) {
            // Clear instruction. Turn all control lines to high impedance mode
            // which turns all leds off
            case CLR:
                charlieplex_clear();
                break;

            // Set up frame 1. Switch the control 0 and 4 gpio pins to HIGH
            // so future leds specified will only need to be brought low
            case FRAME1:
                charlieplex_switch_on(CTRL0);
                charlieplex_switch_on(CTRL4);
                break;

            // Set up frame 2. Same execution as above
            case FRAME2:
                charlieplex_switch_on(CTRL2);
                charlieplex_switch_on(CTRL6);
                break;

            // Set up frame 3. Same execution as above
            case FRAME3:
                charlieplex_switch_on(CTRL1);
                charlieplex_switch_on(CTRL5);
                break;

            // Set up frame 4. Same execution as above
            case FRAME4:
                charlieplex_switch_on(CTRL3);
                charlieplex_switch_on(CTRL7);
                break;

            /*
                Default case is that we were provided an LED to turn on.
                The instruction value is also the bit to check if the led
                should be turned on (control line pulled LOW).
            */
            default:
                // Get the bit corrosponding to this LED
                uint32_t bit = leds & (1 << instr);

                // Shift the bit to the 0th index. Now it will either be a
                // 1 or a 0. 1 meaning it was enabled. 0 meaning not enabled
                uint8_t was_enabled = bit >> instr;

                /*
                    Get the 'enum leds' value out of this by multiplying by the
                    led value. So if instr was D7 (index 2 in 'enum leds'), we
                    multiply 'was_enabled' by 2.
                    If the bit was enabled (value of 1), the result is 2.
                    If the bit was not enabled (value of 0), the result is 0
                */
                uint8_t led = was_enabled * instr;

                // Pass in the led to turn on. This is either an led specified
                // in 'enum leds' or it is 0, which has no effect (DEFAULT_LED).
                charlieplex_led_on(led);
                break;
        }
    }
}
