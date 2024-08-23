#include "physics_engine_events.h"

#include "logging.h"

#define EVENT_QUEUE_MASK (EVENT_QUEUE_SIZE - 1)

void physics_engine_event_queue_init(
    struct physics_engine_event_queue *event_queue) {
    event_queue->head = 0;
    event_queue->tail = 0;
}

bool physics_engine_event_queue_is_empty(
    struct physics_engine_event_queue *event_queue) {
    return event_queue->head == event_queue->tail;
}

bool physics_engine_event_queue_is_full(
    struct physics_engine_event_queue *event_queue) {
    return ((event_queue->tail + 1) & EVENT_QUEUE_MASK) == event_queue->head;
}

bool physics_engine_event_queue_enqueue(
    struct physics_engine_event_queue *event_queue,
    struct physics_engine_event *event) {
    if (physics_engine_event_queue_is_full(event_queue)) {
        return false;
    }

    event_queue->events[event_queue->tail] = *event;
    event_queue->tail = (event_queue->tail + 1) & EVENT_QUEUE_MASK;

    return true;
}

bool physics_engine_event_queue_dequeue(
    struct physics_engine_event_queue *event_queue,
    struct physics_engine_event *dest) {
    if (physics_engine_event_queue_is_empty(event_queue)) {
        return false;
    }

    *dest = event_queue->events[event_queue->head];
    event_queue->head = (event_queue->head + 1) & EVENT_QUEUE_MASK;

    return true;
}

void physics_engine_event_queue_flush(
    struct physics_engine_event_queue *event_queue) {
    struct physics_engine_event event;
    while (physics_engine_event_queue_dequeue(event_queue, &event));
}

void print_physics_engine_event_queue(
    struct physics_engine_event_queue *event_queue) {
    LOG_ERR("Physics Engine Event Queue:");

    int current = event_queue->head;
    int i = 0;

    while (current != event_queue->tail) {
        struct physics_engine_event event = event_queue->events[current];
        LOG_ERR("\tEvent %d:", i++);
        LOG_ERR("\t\tType %d:", event.type);
        switch (event.type) {
            case OUT_OF_BOUNDS_EVENT:
                LOG_ERR("\t\t\tType: %d", event.out_of_bounds_event.type);
                LOG_ERR("\t\t\tEntity: %d",
                        event.out_of_bounds_event.ent->entity_idx);
                break;
            case COLLISION_EVENT:
                LOG_ERR("\t\t\tEntity 1: %d",
                        event.collision_event.ent1->entity_idx);
                LOG_ERR("\t\t\tEntity 2: %d",
                        event.collision_event.ent2->entity_idx);
                break;
            default:
                LOG_ERR("Unknown event type: %d", event.type);
        }
        current = (current + 1) & EVENT_QUEUE_MASK;
    }
}
