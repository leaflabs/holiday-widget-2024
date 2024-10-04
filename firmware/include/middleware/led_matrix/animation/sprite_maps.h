#ifndef __SPRITE_MAPS_H__
#define __SPRITE_MAPS_H__

#include "sprite.h"

/*
 * In this file, define the arrays for the sprites.
 * This should be included once in the led_matrix file so the software
 * renderer has access to the sprite data
 */

/*
 * These are example sprites and can be removed.
 */

/*
 * Define arrays here
 */

// clang-format off
const uint8_t star_img[] = {
    0, 4, 0, 
    4, 4, 4, 
    0, 4, 0,
};

const uint8_t arrow_img[] = {
    0, 0, 4, 0, 0, 
    0, 4, 4, 4, 0, 
    4, 4, 4, 4, 4, 
    0, 0, 4, 0, 0, 
    0, 0, 4, 0, 0,
};

const uint8_t paddle_img[] = {
    4,
    4,
    4,
    4,
};
// clang-format on

/*
 * Define sprite objects here
 */
const struct sprite star = {.data = star_img, .width = 3, .height = 3};

const struct sprite arrow = {.data = arrow_img, .width = 5, .height = 5};

const struct sprite paddle = {.data = paddle_img, .width = 1, .height = 4};

#endif /* __SPRITE_MAPS_H__ */
