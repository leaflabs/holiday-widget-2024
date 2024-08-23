#ifndef __ANIMATION_FRAMES_H__
#define __ANIMATION_FRAMES_H__

#include "led_matrix.h"

extern volatile struct led_matrix runtime_animation[1];

extern volatile struct led_matrix *animation_map_values[3];

extern uint32_t animation_map_lens[3];

#endif /* __ANIMATION_FRAMES_H__ */
