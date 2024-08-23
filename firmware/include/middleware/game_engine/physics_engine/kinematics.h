#ifndef __KINEMATICS_H__
#define __KINEMATICS_H__
#include <stdbool.h>
#include <stdint.h>

#include "environment.h"

extern const uint32_t ANTIALIAS_LEVEL;

/* A 2-Dimensional Vector */
struct vec2 {
    volatile int32_t x;
    volatile int32_t y;
} __attribute__((aligned(4)));

typedef struct vec2 position;
typedef struct vec2 velocity;
typedef struct vec2 acceleration;

/* Entity mass enumeration */
enum mass {
    INFINITE_MASS = 0,
    SMALL_MASS = 1,
    MODERATE_MASS = 2,
    LARGE_MASS = 8,
};

#define TOP_LEFT_POSITION_FROM_GRID(grid_x, grid_y)             \
    (position) {                                                \
        .x = (int32_t)(GRID_MIN + ((grid_x) * GRID_UNIT_SIZE)), \
        .y = (int32_t)(GRID_MIN + ((grid_y) * GRID_UNIT_SIZE)), \
    }
#define BOTTOM_RIGHT_POSITION_FROM_GRID(grid_x, grid_y)                   \
    (position) {                                                          \
        .x = (int32_t)((GRID_MIN + ((grid_x) + 1) * GRID_UNIT_SIZE) - 1), \
        .y = (int32_t)((GRID_MIN + ((grid_y) + 1) * GRID_UNIT_SIZE) - 1), \
    }

#define GET_POSITION_GRID_X(pos) \
    (uint8_t)((pos.x - GRID_MIN + (GRID_UNIT_SIZE / 2)) / GRID_UNIT_SIZE)

#define GET_POSITION_GRID_Y(pos) \
    (uint8_t)((pos.y - GRID_MIN + (GRID_UNIT_SIZE / 2)) / GRID_UNIT_SIZE)

// #define GET_POSITION_GRID_X(pos)
//     (uint8_t)((((pos.x + ENVIRONMENT_MAX_X)) * GRID_SIZE) /
//               (ENVIRONMENT_MAX_X - ENVIRONMENT_MIN_X))

// #define GET_POSITION_GRID_Y(pos)
//     (uint8_t)((((pos.y + ENVIRONMENT_MAX_Y)) * GRID_SIZE) /
//               (ENVIRONMENT_MAX_Y - ENVIRONMENT_MIN_Y))

bool position_in_bounds(position *pos);

#endif