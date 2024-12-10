#include "led_matrix.h"

#include <stdbool.h>

#include "animation_frames.h"
#include "charlieplex_driver.h"
#include "logging.h"
#include "sprite.h"
#include "sprite_maps.h"
#include "string.h"
#include "system_communication.h"

#define _ANTIALIAS_LEVEL 1
#if _ANTIALIAS_LEVEL <= 0
#error "_ANTIALIAS_LEVEL must be greater than 0"
#endif

const uint32_t ANTIALIAS_LEVEL = _ANTIALIAS_LEVEL;

volatile bool update_requested;

/* Type to represent the led matrix thats usable for the charlieplex code */
struct matrix_instance {
    uint8_t matrix[NUM_LEDS];
};

/* Container of matrix_instances to make a single frame */
struct frame_instance {
    struct matrix_instance frame;
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
        update_requested = true;
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
    struct game_entity *input = comm->data.led_matrix.renderer.entities;
    uint32_t num_entities = comm->data.led_matrix.renderer.num_entities;

    struct led_matrix *output = get_matrix_entry(context, output_slot);

    // First, reset the pixel
    output->mat[cur_row][cur_col] = 0;

#if _ANTIALIAS_LEVEL == 1
    // Now iterate over all sprites in the buffer and draw them
    for (uint32_t i = 0; i < num_entities; i++) {
        if (game_entity_is_active(&input[i])) {
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
            if (cur_row >= y_pos && cur_col >= x_pos && dy < height &&
                dx < width) {
                int brightness = sprite->data[dx + dy * width];
                output->mat[cur_row][cur_col] = brightness;
            }
        }
    }
#else
    // Now iterate over all sprites in the buffer and draw them
    for (uint32_t i = 0; i < num_entities; i++) {
        if (game_entity_is_active(&input[i])) {
            struct sprite_component *sc = &input[i].sprite;
            const struct sprite *sprite = sc->map;
            int x_pos = sc->x;
            int y_pos = sc->y;
            int width = sprite->width * _ANTIALIAS_LEVEL;
            int height = sprite->height * _ANTIALIAS_LEVEL;

            // Determine the bounds of the sprite in terms of LED matrix pixels
            int x1 = x_pos / _ANTIALIAS_LEVEL;             // Left boundary
            int y1 = y_pos / _ANTIALIAS_LEVEL;             // Top boundary
            int x2 = (x_pos + width) / _ANTIALIAS_LEVEL;   // Right boundary
            int y2 = (y_pos + height) / _ANTIALIAS_LEVEL;  // Bottom boundary

            // Calculate the sub-pixel boundaries for the current LED matrix
            // pixel
            int cur_x_subpixel_start = cur_col * _ANTIALIAS_LEVEL;
            int cur_x_subpixel_end = (cur_col + 1) * _ANTIALIAS_LEVEL;
            int cur_y_subpixel_start = cur_row * _ANTIALIAS_LEVEL;
            int cur_y_subpixel_end = (cur_row + 1) * _ANTIALIAS_LEVEL;

            // Check if the sprite overlaps with the current pixel
            if (cur_col >= x1 && cur_col <= x2 && cur_row >= y1 &&
                cur_row <= y2) {
                // Calculate overlap area between the sprite and the current
                // pixel
                int overlap_x_start = (x_pos > cur_x_subpixel_start)
                                          ? x_pos
                                          : cur_x_subpixel_start;
                int overlap_x_end = ((x_pos + width) < cur_x_subpixel_end)
                                        ? (x_pos + width)
                                        : cur_x_subpixel_end;
                int overlap_y_start = (y_pos > cur_y_subpixel_start)
                                          ? y_pos
                                          : cur_y_subpixel_start;
                int overlap_y_end = ((y_pos + height) < cur_y_subpixel_end)
                                        ? (y_pos + height)
                                        : cur_y_subpixel_end;

                // Calculate the overlap area
                int overlap_width = overlap_x_end - overlap_x_start;
                int overlap_height = overlap_y_end - overlap_y_start;

                // Calculate overlap proportion relative to the entire pixel
                // area
                float overlap_area = (float)(overlap_width * overlap_height) /
                                     (_ANTIALIAS_LEVEL * _ANTIALIAS_LEVEL);

                // Get the brightness of the corresponding sprite pixel
                int dx = (overlap_x_start - x_pos) / _ANTIALIAS_LEVEL;
                int dy = (overlap_y_start - y_pos) / _ANTIALIAS_LEVEL;
                int brightness = sprite->data[dx + dy * sprite->width];

                // Add the scaled brightness to the output pixel
                output->mat[cur_row][cur_col] +=
                    (int)(brightness * overlap_area);

                // Clamp the pixel brightness to the maximum value
                if (output->mat[cur_row][cur_col] > 4) {
                    output->mat[cur_row][cur_col] = 4;
                }
            }
        }
    }
#endif
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
        update_requested = true;
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

    if (led) {
        output->frame.matrix[index] = index;  // 1U;
    } else {
        output->frame.matrix[index] = 0U;
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
        charlieplex_driver_draw(output->frame.matrix);
    }

    // Update values
    comm->data.led_matrix.assembler.row = cur_row;
    comm->data.led_matrix.assembler.col = cur_col;
}

void pause_led_matrix() {
    pause_charlieplex_driver();
}
void unpause_led_matrix() {
    unpause_charlieplex_driver();
}

size_t led_matrix_scroll_text(const char *text, enum scroll_speed speed) {
    static size_t scroll_position = 0;
    static char *prev_text = NULL;

    if (prev_text != NULL) {
        if (strcmp(text, prev_text) != 0) {
            scroll_position = 0;
            LOG_INF("Warning: scroll text interrupted by new text");
        }
    } else {
        led_matrix_comm.data.led_matrix.renderer.active = false;
        led_matrix_comm.data.led_matrix.renderer.finished = true;
        led_matrix_comm.data.led_matrix.loader.active = true;
        led_matrix_comm.data.led_matrix.loader.finished = true;
        led_matrix_comm.data.led_matrix.loader.input_anim =
            ANIM_RUNTIME_ANIMATION;
    }

    prev_text = text;

    if (led_matrix_comm.data.led_matrix.loader.finished) {
        generate_frame(text, strlen(text), scroll_position++ / speed,
                       animation_map_values[ANIM_RUNTIME_ANIMATION][0].mat);

        if (scroll_position >= strlen(text) * (N_DIMENSIONS * speed)) {
            scroll_position = 0;
            prev_text = NULL;
            led_matrix_comm.data.led_matrix.loader.active = false;
            led_matrix_comm.data.led_matrix.loader.finished = true;
            led_matrix_comm.data.led_matrix.renderer.active = true;
            led_matrix_comm.data.led_matrix.renderer.finished = true;
            led_matrix_comm.data.led_matrix.renderer.row = 0;
            led_matrix_comm.data.led_matrix.renderer.col = 0;

            return 0;
        }
    }

    return strlen(text) * (N_DIMENSIONS << speed) - scroll_position;
}
