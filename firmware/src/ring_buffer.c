#include "ring_buffer.h"

#include "logging.h"

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

int ring_buffer_push_n(struct ring_buffer *rb, const void *data, size_t len) {
    if (ring_buffer_available_to_write(rb) < len) {
        return -1;  // Not enough space in buffer
    }

    size_t first_part = rb->capacity - rb->head;

    // Copy in one operation if the data fits without wrapping
    size_t to_write = len > first_part ? first_part : len;
    memcpy(&rb->buffer[rb->head], data, to_write * rb->element_size);

    // If wrapping occurs, copy the remaining data to the beginning of the
    // buffer
    if (len > first_part) {
        memcpy(&rb->buffer[0], data + to_write, len - to_write);
    }

    rb->head =
        (rb->head + len) & (rb->capacity - 1);  // Update head with wrap-around
    return 0;
}

int ring_buffer_pop_n(struct ring_buffer *rb, const void *data, size_t len) {
    if (ring_buffer_available_to_read(rb) < len) {
        return -1;  // Not enough data in buffer
    }

    size_t first_part = rb->capacity - rb->tail;

    // Copy in one operation if the data fits without wrapping
    size_t to_read = len > first_part ? first_part : len;
    memcpy(data, &rb->buffer[rb->tail], to_read * rb->element_size);

    // If wrapping occurs, copy the remaining data from the beginning of the
    // buffer
    if (len > first_part) {
        memcpy(data + to_read, &rb->buffer[0],
               (len - to_read) * rb->element_size);
    }

    rb->tail =
        (rb->tail + len) & (rb->capacity - 1);  // Update tail with wrap-around
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

void ring_buffer_flush(struct ring_buffer *rb) {
    rb->head = 0;
    rb->tail = 0;
}

size_t ring_buffer_available_to_write(struct ring_buffer *rb) {
    if (rb->head >= rb->tail) {
        return (rb->capacity - (rb->head - rb->tail));
    } else {
        return (rb->tail - rb->head - 1);
    }
}

size_t ring_buffer_available_to_read(struct ring_buffer *rb) {
    if (rb->head >= rb->tail) {
        return (rb->head - rb->tail);
    } else {
        return (rb->capacity - (rb->tail - rb->head - 1));
    }
}