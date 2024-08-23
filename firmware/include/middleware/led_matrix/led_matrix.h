#ifndef __LED_MATRIX_H__
#define __LED_MATRIX_H__

#include "charlieplex_driver.h"
#include "stm32l0xx_hal.h"

/*
 * Defines how large the buffers are for storing assembled, loaded and
 * renderered frames
 */
#define LED_MATRIX_BUFFER_SIZE 2

/*
 * Create a map so the widget controller can reference animation frames
 *      Note: New animations must be added here (and remapped in
 *      include/animation_frames.h so they be referenced).
 */
enum animation_map {
    ANIM_TEST_ANIMATION,
    ANIM_SAVED_ANIMATION,
    ANIM_RUNTIME_ANIMATION,
};

/*
    This is the storage format for saving a frame.
    The max value for an led is defined in the animation_frames.h
    file
*/
struct led_matrix {
    uint8_t mat[N_DIMENSIONS][N_DIMENSIONS];
};

/*
    The pipeline and design for the led matrix is as follows:
    The base type for a frame is a struct led_matrx. This encodes a
    single frame's data. When rendering, we wanted to allow for dimming and
    also repeat frames so, each frame is expanded out into a number of
    subframes.

    Dimming is achieved through leaving some leds off for part of the sub
    frames.

    A common issue with drawing is image tearing, which happens when writing
    to a buffer while the buffer is being displayed. This is avoided by
    creating two buffers and providing a pointer for where the writing should
    go and where the drawer should read from. Two pointers, the 'front' and
    'back' pointers select which buffer should be used for reading and writing.
    Flipping the pointers lets you avoid a memcpy and prevents image tearing.

    Function Descriptions:

    The loader function loads the next frame of animation and saves it to a
    pointer for the assembler to use.

    The assembler uses a struct led_matrix and creates a set number of
    subframes. The ratio of on-time and off-time provided by the led_matrix
    determines how often a single led is on. A single call to the assembler
    will process a single led for all of the subframes. The assembler can
    take a new struct led_matrix as input from any function, as long as:
        (1). It updates the struct led_matrix *current_frame pointer value.
        (2). It unsets the 'assembler_needs_frame' flag.
        (3). It only updates the 'current_frame' pointer when requested.
    These are inplace to avoid image tearing.

    The drawer takes in a complete list of sub frames and draws them.
    There is no logic to alert the assembler when finished. It just
    draws whatever is in the front buffer and wraps around when necessary.
    A new frame is drawn by swapping the front and back buffer pointers.
*/

/* Provide access to the lengths of the animations */
uint32_t get_anim_length(enum animation_map anim);

/*
    Setup the led matrix's gpio pins
*/
void led_matrix_setup(void);

/*
 * Loads the animation frame specified one pixel per iteration.
 * Data is stored in the specified slot in 'matrix_buff' for the context.
 * Signals when finished.
 */
void led_matrix_loader_run(void);

/*
 * This function only focuses on rendering stuff
 * Renders a single led per iteration. Stores the data in the specified slot
 * in 'matrix_buff' for the context. Signals when finished.
 */
void led_matrix_renderer_run(void);

/*
 * Assembles all the subframes for a single led per iteration.
 * Signals when finished.
 */
void led_matrix_assembler_run(void);

void pause_led_matrix();
void unpause_led_matrix();

enum scroll_speed {
    SCROLL_SPEED_FAST = 1,
    SCROLL_SPEED_MODERATE = 2,
    SCROLL_SPEED_SLOW = 4,
};

size_t led_matrix_scroll_text(const char *text, enum scroll_speed speed);

#endif /* __LED_MATRIX_H__ */
