#include "shapes.h"

bool valid_rectangle(struct rectangle *rectangle) {
    return position_in_bounds(&rectangle->p1) &&
           position_in_bounds(&rectangle->p2) &&
           rectangle->p1.x < rectangle->p2.x &&
           rectangle->p1.y < rectangle->p2.y;
}
