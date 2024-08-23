#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utils.h"

// Ring Buffer Structure
struct ring_buffer {
    void *buffer;         // Pointer to statically allocated memory
    size_t element_size;  // Size of each element
    size_t capacity;      // Buffer capacity (must be a power of 2)
    size_t head;          // Write index
    size_t tail;          // Read index
};

// Initialize the ring buffer with statically allocated memory
#define RING_BUFFER_INIT(rb, buf, elem_size, buf_capacity)              \
    do {                                                                \
        if (!IS_POWER_OF_2(buf_capacity)) {                             \
            LOG_ERR("Ring buffer size must be a power of 2 - got <%d>", \
                    buf_capacity);                                      \
            while (1);                                                  \
        }                                                               \
        (rb)->buffer = (buf);                                           \
        (rb)->element_size = (elem_size);                               \
        (rb)->capacity = (buf_capacity);                                \
        (rb)->head = 0;                                                 \
        (rb)->tail = 0;                                                 \
    } while (0)

// Check if the ring buffer is empty
static inline bool ring_buffer_is_empty(const struct ring_buffer *rb) {
    return rb->head == rb->tail;
}

// Check if the ring buffer is full
static inline bool ring_buffer_is_full(const struct ring_buffer *rb) {
    return ((rb->head + 1) & (rb->capacity - 1)) == rb->tail;
}

// Add an element to the ring buffer
int ring_buffer_push(struct ring_buffer *rb, const void *data);

// Remove an element from the ring buffer
int ring_buffer_pop(struct ring_buffer *rb, void *data);

// Peek at the front element without removing it
int ring_buffer_peek(struct ring_buffer *rb, void *data);

#endif /*__RING_BUFFER_H__*/