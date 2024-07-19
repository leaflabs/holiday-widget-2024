#ifndef __ANIMATION_FRAMES_H__
#define __ANIMATION_FRAMES_H__

/*
    This should only be included in the led_matrix.c file
*/

#include "led_matrix.h"

/*
    The max value should be much smaller than the subframe count
    since close values leads to no difference between brightness.
    In addition, the max value should be pretty small because large
    values leads to flickering due to how often an led can be off
    in the sub frames
*/

#define LED_MATRIX_SUB_FRAME_COUNT_POWER 4
#define LED_MATRIX_SUB_FRAME_COUNT (1 << LED_MATRIX_SUB_FRAME_COUNT_POWER)

// Max brightness of an led
#define MAX_VALUE 4

struct led_matrix
    saved_animation[] =
        {
            {.mat =
                 {
                     {4, 4, 4, 4, 4, 4, 4},
                     {3, 3, 3, 3, 3, 3, 3},
                     {2, 2, 2, 2, 2, 2, 2},
                     {1, 1, 1, 1, 1, 1, 1},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                 }},
            {.mat =
                 {
                     {0, 0, 0, 0, 0, 0, 0},
                     {4, 4, 4, 4, 4, 4, 4},
                     {3, 3, 3, 3, 3, 3, 3},
                     {2, 2, 2, 2, 2, 2, 2},
                     {1, 1, 1, 1, 1, 1, 1},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                 }},
            {.mat =
                 {
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {4, 4, 4, 4, 4, 4, 4},
                     {3, 3, 3, 3, 3, 3, 3},
                     {2, 2, 2, 2, 2, 2, 2},
                     {1, 1, 1, 1, 1, 1, 1},
                     {0, 0, 0, 0, 0, 0, 0},
                 }},
            {.mat =
                 {
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {4, 4, 4, 4, 4, 4, 4},
                     {3, 3, 3, 3, 3, 3, 3},
                     {2, 2, 2, 2, 2, 2, 2},
                     {1, 1, 1, 1, 1, 1, 1},
                 }},
            {.mat =
                 {
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {4, 4, 4, 4, 4, 4, 4},
                     {3, 3, 3, 3, 3, 3, 3},
                     {2, 2, 2, 2, 2, 2, 2},
                 }},
            {.mat =
                 {
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {4, 4, 4, 4, 4, 4, 4},
                     {3, 3, 3, 3, 3, 3, 3},
                 }},
            {.mat =
                 {
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {0, 0, 0, 0, 0, 0, 0},
                     {4, 4, 4, 4, 4, 4, 4},
                 }},

};

#endif /* __ANIMATION_FRAMES_H__ */
