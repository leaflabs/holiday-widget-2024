#include "kinematics.h"

bool position_in_bounds(position *pos) {
    return pos->x <= ENVIRONMENT_MAX_X && pos->x >= ENVIRONMENT_MIN_X &&
           pos->y <= ENVIRONMENT_MAX_Y && pos->y >= ENVIRONMENT_MIN_Y;
}