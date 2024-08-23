#ifndef __SHAPES_H__
#define __SHAPES_H__
#include "kinematics.h"

/* Rectangle entity type implementation */
struct rectangle {
    position p1, p2;
} __attribute__((aligned(4)));

bool valid_rectangle(struct rectangle *rectangle);

#endif