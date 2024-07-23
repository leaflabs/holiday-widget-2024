#include "led_matrix.h"

#include <stdbool.h>

#include "animation_frames.h"
#include "charlieplex_driver.h"

/*
    Private context for the functions in this file to communicate with
    each other. Defines the 'API's for the functions
*/
struct led_matrix_context {
    /*
        API for the loader/assembler functions:
        'assembler_current_frame' is the led_matrix to reference when
            generating sub_frames
        'assembler_needs_frame' is the flag the assembler sets to 1 when it
       needs a new frame. This must be handled before the assembler is ran again
    */
    struct led_matrix *assembler_current_frame;
    bool assembler_needs_frame;

    /*
        API for the assembler/drawer functions:
        'front' points to the array of data that the drawer should be displaying
        'back' points to the array of datat that the assembler should write to

        Since these buffers must be 'global' for the two functions to use,
       'img1' and 'img2' are the physical locations that the subframes are
       stored. Never reference these directly.

    */
    uint8_t img1[LED_MATRIX_SUB_FRAME_COUNT][NUM_LEDS];
    uint8_t img2[LED_MATRIX_SUB_FRAME_COUNT][NUM_LEDS];
    uint8_t (*front)[LED_MATRIX_SUB_FRAME_COUNT][NUM_LEDS];
    uint8_t (*back)[LED_MATRIX_SUB_FRAME_COUNT][NUM_LEDS];

    /* Variables used in the job functions */
    int image_loader_counter;
    int assembler_cur_row;
    int assembler_cur_col;
    int draw_job_counter;
};

static struct led_matrix_context led_matrix_context = {
    .assembler_current_frame = NULL,
    .assembler_needs_frame = true,
    .img1 = {{0}},
    .img2 = {{0}},
    .front = &led_matrix_context.img1,
    .back = &led_matrix_context.img2,
    .image_loader_counter = 0,
    .assembler_cur_row = 0,
    .assembler_cur_col = 0,
    .draw_job_counter = 0};

void led_matrix_setup(void) {
    charlieplex_driver_init();
}

/*
    Use 'saved_animation' for now to demonstrate how this works
*/
void led_matrix_loader_run(void) {
    const int num_frames = sizeof(test_animation) / sizeof(struct led_matrix);

    // If the assembler needs a new frame, get the new one
    if (led_matrix_context.assembler_needs_frame) {
        led_matrix_context.assembler_current_frame =
            &test_animation[led_matrix_context.image_loader_counter];

        // Go to next frame and wrap if necessary
        led_matrix_context.image_loader_counter++;
        if (led_matrix_context.image_loader_counter >= num_frames)
            led_matrix_context.image_loader_counter = 0;

        led_matrix_context.assembler_needs_frame = false;
    }
}

void led_matrix_assembler_run(void) {
    // We need the index of the led so we can assign that element to itself
    int index = led_matrix_context.assembler_cur_row * N_DIMENSIONS +
                led_matrix_context.assembler_cur_col + DUMMY_SLOTS;

    // Get the value for this led
    uint32_t led = led_matrix_context.assembler_current_frame
                       ->mat[led_matrix_context.assembler_cur_row]
                            [led_matrix_context.assembler_cur_col];

    // Counters for keeping track of which frames to set as 'on'
    int time_counter = 0;
    int time_on = led;  // The 'on time' is the value of the led in the matrix
    int time_max = LED_MATRIX_MAX_VALUE;

    // now go through each sub_frame and select which bits should be on
    for (int i = 0; i < LED_MATRIX_SUB_FRAME_COUNT; i++) {
        if (time_counter == time_max) time_counter = 0;

        // Set or unset the led
        if (time_counter < time_on) {
            (*led_matrix_context.back)[i][index] = index;
        } else {
            (*led_matrix_context.back)[i][index] = 0U;
        }
        time_counter++;
    }

    // Update index values
    led_matrix_context.assembler_cur_col++;
    if (led_matrix_context.assembler_cur_col >= N_DIMENSIONS) {
        led_matrix_context.assembler_cur_col = 0;
        led_matrix_context.assembler_cur_row++;
    }
    if (led_matrix_context.assembler_cur_row >= N_DIMENSIONS) {
        led_matrix_context.assembler_cur_row = 0;

        // This frame is finished so request a new one
        led_matrix_context.assembler_needs_frame = true;

        // Flip buffers as we are done now.
        // This gives the drawer a new buffer and us the old buffer to overwrite
        uint8_t(*temp)[LED_MATRIX_SUB_FRAME_COUNT][NUM_LEDS] =
            led_matrix_context.front;
        led_matrix_context.front = led_matrix_context.back;
        led_matrix_context.back = temp;
    }
}

void led_matrix_drawer_run() {
    // Draw the current subframe
    charlieplex_driver_draw(
        (*led_matrix_context.front)[led_matrix_context.draw_job_counter]);

    // Continue around the array
    led_matrix_context.draw_job_counter++;
    led_matrix_context.draw_job_counter &= (LED_MATRIX_SUB_FRAME_COUNT - 1);
}
