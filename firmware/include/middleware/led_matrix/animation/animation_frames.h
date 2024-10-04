#ifndef __ANIMATION_FRAMES_H__
#define __ANIMATION_FRAMES_H__

#include "led_matrix.h"

/*
    Combine both the generated and non generated frames of data.
    The user is responsible for making sure names do not collide.
*/
#include "generated_animation_frames.h"
#include "non_generated_animation_frames.h"

// This is where we map animations to the arrays that represent them
struct led_matrix *animation_map_values[] = {
    [ANIM_TEST_ANIMATION] = test_animation,
    [ANIM_SAVED_ANIMATION] = saved_animation};

// And map the lengths of the animations
uint32_t animation_map_lens[] = {
    [ANIM_TEST_ANIMATION] = sizeof(test_animation) / sizeof(struct led_matrix),
    [ANIM_SAVED_ANIMATION] =
        sizeof(saved_animation) / sizeof(struct led_matrix),
};

#endif /* __ANIMATION_FRAMES_H__ */
