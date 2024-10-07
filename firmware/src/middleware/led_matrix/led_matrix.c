#include "led_matrix.h"

#include <stdbool.h>

#include "animation_frames.h"
#include "charlieplex_driver.h"
#include "entity.h"
#include "sprite.h"
#include "sprite_maps.h"
#include "system_communication.h"

/* Type to represent the led matrix thats usable for the charlieplex code */
struct matrix_instance {
    uint8_t matrix[NUM_LEDS];
};

/* Container of matrix_instances to make a single frame */
struct frame_instance {
    struct matrix_instance sub_frame[LED_MATRIX_SUB_FRAME_COUNT];
};

struct led_matrix_context {
    struct led_matrix matrix_buff[LED_MATRIX_BUFFER_SIZE];
    struct frame_instance frame_buff[LED_MATRIX_BUFFER_SIZE];
    struct driver_comm_shared_memory *comm;
};

extern struct driver_comm_shared_memory led_matrix_comm;

static struct led_matrix_context led_matrix_context = {.comm =
                                                           &led_matrix_comm};

/*
 * Helper functions to get the entry from an index
 */
struct led_matrix *get_matrix_entry(struct led_matrix_context *context,
                                    int num) {
    return &context->matrix_buff[num];
}
struct frame_instance *get_frame_entry(struct led_matrix_context *context,
                                       int num) {
    return &context->frame_buff[num];
}

uint32_t get_anim_length(enum animation_map anim) {
    return animation_map_lens[anim];
}

void led_matrix_setup(void) {
    charlieplex_driver_init();
}

void led_matrix_loader_run(void) {
    struct led_matrix_context *context = &led_matrix_context;
    struct driver_comm_shared_memory *comm = context->comm;

    // Is this task active?
    bool loader_on = comm->data.led_matrix.loader.active;
    if (!loader_on) {
        return;
    }

    int cur_row = comm->data.led_matrix.loader.row;
    int cur_col = comm->data.led_matrix.loader.col;
    enum animation_map input_anim = comm->data.led_matrix.loader.input_anim;
    int input_frame = comm->data.led_matrix.loader.input_frame;
    int output_slot = comm->data.led_matrix.loader.output_slot;

    struct led_matrix *output = get_matrix_entry(context, output_slot);
    struct led_matrix *input = animation_map_values[input_anim];

    // Copy a single led over pre frame. This matches the rate of the renderer
    output->mat[cur_row][cur_col] = input[input_frame].mat[cur_row][cur_col];

    // Update index values
    cur_col++;
    if (cur_col >= N_DIMENSIONS) {
        cur_col = 0;
        cur_row++;
    }
    if (cur_row >= N_DIMENSIONS) {
        cur_row = 0;

        // We finished, so request a new frame to load
        comm->data.led_matrix.loader.finished = true;
    }

    // Update values
    comm->data.led_matrix.loader.row = cur_row;
    comm->data.led_matrix.loader.col = cur_col;
}

void led_matrix_renderer_run(void) {
    struct led_matrix_context *context = &led_matrix_context;
    struct driver_comm_shared_memory *comm = context->comm;

    // Is this task active?
    bool renderer_on = comm->data.led_matrix.renderer.active;
    if (!renderer_on) {
        return;
    }

    int cur_row = comm->data.led_matrix.renderer.row;
    int cur_col = comm->data.led_matrix.renderer.col;
    int output_slot = comm->data.led_matrix.renderer.output_slot;
    struct entity *input = comm->data.led_matrix.renderer.entities;
    uint32_t num_entities = comm->data.led_matrix.renderer.num_entities;

    struct led_matrix *output = get_matrix_entry(context, output_slot);

    // First, reset the pixel
    output->mat[cur_row][cur_col] = 0;

    // Now iterate over all sprites in the buffer and draw them
    for (uint32_t i = 0; i < num_entities; i++) {
        struct sprite_component *sc = &input[i].sprite;
        const struct sprite *sprite = sc->map;
        int x_pos = sc->x;
        int y_pos = sc->y;
        int width = sprite->width;
        int height = sprite->height;

        // Get difference so we can also index into the sprite data array
        int dx = cur_col - x_pos;
        int dy = cur_row - y_pos;

        // Add the brightness if we are in the bounds
        if (cur_row >= y_pos && cur_col >= x_pos && dy < height && dx < width) {
            int brightness = sprite->data[dx * width + dy];
            output->mat[cur_row][cur_col] += brightness;
        }
    }

    // Update index values
    cur_col++;
    if (cur_col >= N_DIMENSIONS) {
        cur_col = 0;
        cur_row++;
    }
    if (cur_row >= N_DIMENSIONS) {
        cur_row = 0;

        // We finished, so request a new slot to render to
        comm->data.led_matrix.renderer.finished = true;
    }

    // Update values
    comm->data.led_matrix.renderer.row = cur_row;
    comm->data.led_matrix.renderer.col = cur_col;
}

void led_matrix_assembler_run(void) {
    struct led_matrix_context *context = &led_matrix_context;
    struct driver_comm_shared_memory *comm = context->comm;

    // Is this task active?
    bool assembler_on = comm->data.led_matrix.assembler.active;
    if (!assembler_on) {
        return;
    }

    int cur_row = comm->data.led_matrix.assembler.row;
    int cur_col = comm->data.led_matrix.assembler.col;
    int input_slot = comm->data.led_matrix.assembler.input_slot;
    int output_slot = comm->data.led_matrix.assembler.output_slot;

    struct led_matrix *input = get_matrix_entry(context, input_slot);
    struct frame_instance *output = get_frame_entry(context, output_slot);

    // We need the index of the led so we can assign that element to itself
    int index = (cur_row * N_DIMENSIONS) + cur_col + 1;

    // Get the value for this led
    uint32_t led = input->mat[cur_row][cur_col];

    // Counters for keeping track of which frames to set as 'on'
    int time_counter = 0;
    int time_on = led;  // The 'on time' is the value of the led in the matrix
    int time_max = LED_MATRIX_MAX_VALUE;

    // now go through each sub_frame and select which bits should be on
    for (int i = 0; i < LED_MATRIX_SUB_FRAME_COUNT; i++) {
        if (time_counter == time_max)
            time_counter = 0;

        // Set or unset the led
        if (time_counter < time_on) {
            output->sub_frame[i].matrix[index] = index;
        } else {
            output->sub_frame[i].matrix[index] = 0U;
        }
        time_counter++;
    }

    // Update index values
    cur_col++;
    if (cur_col >= N_DIMENSIONS) {
        cur_col = 0;
        cur_row++;
    }
    if (cur_row >= N_DIMENSIONS) {
        cur_row = 0;

        // Finished so request new data
        comm->data.led_matrix.assembler.finished = true;
    }

    // Update values
    comm->data.led_matrix.assembler.row = cur_row;
    comm->data.led_matrix.assembler.col = cur_col;
}

void led_matrix_drawer_run(void) {
    struct led_matrix_context *context = &led_matrix_context;
    struct driver_comm_shared_memory *comm = context->comm;

    // Is this task active?
    bool drawer_on = comm->data.led_matrix.drawer.active;
    if (!drawer_on) {
        return;
    }

    int sub_frame_cntr = comm->data.led_matrix.drawer.sub_frame_cntr;
    int duration_cntr = comm->data.led_matrix.drawer.duration_cntr;
    int num_draws = comm->data.led_matrix.drawer.num_draws;

    int input_slot = comm->data.led_matrix.drawer.input_slot;
    struct frame_instance *input = get_frame_entry(context, input_slot);

    // Draw the current subframe
    charlieplex_driver_draw(input->sub_frame[sub_frame_cntr].matrix);

    // Check for end of drawing or looping around the frame
    sub_frame_cntr++;
    duration_cntr++;
    if (duration_cntr >= num_draws) {
        duration_cntr = 0;
        sub_frame_cntr = 0;
        comm->data.led_matrix.drawer.finished = true;
    }
    if (sub_frame_cntr >= LED_MATRIX_SUB_FRAME_COUNT) {
        sub_frame_cntr = 0;
    }

    // Update values
    comm->data.led_matrix.drawer.sub_frame_cntr = sub_frame_cntr;
    comm->data.led_matrix.drawer.duration_cntr = duration_cntr;
}
