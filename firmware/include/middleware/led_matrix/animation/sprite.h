#ifndef __SPRITE_H__
#define __SPRITE_H__

#include "stdint.h"

/*
 * Sprite objects do not change as they are configured specifically
 * for a single pixel-map to draw
 */
struct sprite {
    const uint8_t *const data;  // The sprite map data
    const uint32_t width;       // Width of the sprite map
    const uint32_t height;      // Height of the sprite map
};

/*
 * Sprite components are the individual instances that have modified positions
 * and are owned by other structs. Specify which sprite to use and its position
 * within the 7x7 matrix.
 */
struct sprite_component {
    const struct sprite *map;  // The sprite this object references
    int x;                     // X position of the sprite
    int y;                     // Y position of the sprite
};

// Provide a list of externed sprite objects so others can reference it
extern const struct sprite star;
extern const struct sprite paddle;
extern const struct sprite arrow;

#endif /* __SPRITE_H__ */
