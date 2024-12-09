#ifndef __ENVIRONMENT_H__
#define __ENVIRONMENT_H__
#include <stdbool.h>

#define ENVIRONMENT_MAX_X (int32_t) INT16_MAX
#define ENVIRONMENT_MIN_X (int32_t)(INT16_MIN + 1)
#define ENVIRONMENT_MAX_Y (int32_t) INT16_MAX
#define ENVIRONMENT_MIN_Y (int32_t)(INT16_MIN + 1)

#define GRID_SIZE (int32_t)7
#define GRID_UNIT_SIZE (int32_t)((UINT16_MAX - 1) / 7)
#define GRID_MIN (int32_t)(INT16_MIN + 1)

/* A 2-Dimensional Vector */
struct vec2 {
    volatile int32_t x;
    volatile int32_t y;
};

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

/* Rectangle entity type implementation */
struct rectangle {
    position p1, p2;
};

static inline bool position_in_bounds(position *pos) {
    return pos->x <= ENVIRONMENT_MAX_X && pos->x >= ENVIRONMENT_MIN_X &&
           pos->y <= ENVIRONMENT_MAX_Y && pos->y >= ENVIRONMENT_MIN_Y;
}

static inline bool valid_rectangle(struct rectangle *rectangle) {
    return position_in_bounds(&rectangle->p1) &&
           position_in_bounds(&rectangle->p2) &&
           rectangle->p1.x < rectangle->p2.x &&
           rectangle->p1.y < rectangle->p2.y;
}

#endif