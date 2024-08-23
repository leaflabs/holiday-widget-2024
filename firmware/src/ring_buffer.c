#include "ring_buffer.h"

// Add an element to the ring buffer
int ring_buffer_push(struct ring_buffer *rb, const void *data) {
    if (ring_buffer_is_full(rb)) {
        return -1;  // Buffer is full
    }

    // Copy data to the buffer
    memcpy((void *)rb->buffer + (rb->head * rb->element_size), data,
           rb->element_size);

    // Increment head (modulo capacity using bitmask)
    rb->head = (rb->head + 1) & (rb->capacity - 1);

    return 0;
}

// Remove an element from the ring buffer
int ring_buffer_pop(struct ring_buffer *rb, void *data) {
    if (ring_buffer_is_empty(rb)) {
        return -1;  // Buffer is empty
    }
    // Copy data from the buffer
    memcpy(data, (void *)rb->buffer + (rb->tail * rb->element_size),
           rb->element_size);

    // Increment tail (modulo capacity using bitmask)
    rb->tail = (rb->tail + 1) & (rb->capacity - 1);

    return 0;
}

// Peek at the front element without removing it
int ring_buffer_peek(struct ring_buffer *rb, void *data) {
    if (ring_buffer_is_empty(rb)) {
        return -1;  // Buffer is empty
    }
    // Copy data from the tail
    memcpy(data, (uint8_t *)rb->buffer + (rb->tail * rb->element_size),
           rb->element_size);

    return 0;
}
