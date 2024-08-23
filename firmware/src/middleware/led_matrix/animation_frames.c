#include "animation_frames.h"

/*
    Combine both the generated and non generated frames of data.
    The user is responsible for making sure names do not collide.
*/
#include "generated_animation_frames.h"
#include "non_generated_animation_frames.h"

volatile struct led_matrix runtime_animation[1] = {
    {
        .mat = {0},
    },
};

// This is where we map animations to the arrays that represent them
volatile struct led_matrix *animation_map_values[3] = {
    [ANIM_TEST_ANIMATION] = test_animation,
    [ANIM_SAVED_ANIMATION] = saved_animation,
    [ANIM_RUNTIME_ANIMATION] = runtime_animation};

// And map the lengths of the animations
uint32_t animation_map_lens[3] = {
    [ANIM_TEST_ANIMATION] = sizeof(test_animation) / sizeof(struct led_matrix),
    [ANIM_SAVED_ANIMATION] =
        sizeof(saved_animation) / sizeof(struct led_matrix),
    [ANIM_RUNTIME_ANIMATION] =
        sizeof(runtime_animation) / sizeof(struct led_matrix),
};