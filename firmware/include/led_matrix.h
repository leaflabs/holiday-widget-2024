#ifndef __LED_MATRIX_H__
#define __LED_MATRIX_H__

#include "charlieplex_driver.h"
#include "stm32l0xx_hal.h"

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

/*
    Setup the led matrix's gpio pins
*/
void led_matrix_setup(void);

/*
    Load the next frame of animation if requested by the assembler
*/
void led_matrix_loader_run(void);

/*
    Assemble a set of subframes based on a single frame.
    Update the front buffer for the drawer function
    when assembling is complete and request a new frame
*/
void led_matrix_assembler_run(void);

/*
    Draw the subframes to the led matrix.
*/
void led_matrix_drawer_run(void);

#endif /* __LED_MATRIX_H__ */
