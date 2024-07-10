#include <futures.h>

inline void future_await(struct future *future) {
    while (atomic_load(&future->state) == FUTURE_WAITING)
        ;
}

inline int future_is_waiting(struct future *future) {
    return atomic_load(&future->state) == FUTURE_WAITING;
}

inline int future_is_finished(struct future *future) {
    return atomic_load(&future->state) == FUTURE_FINISHED;
}

inline int future_is_errored(struct future *future) {
    return atomic_load(&future->state) == FUTURE_ERROR;
}

inline int future_get_state(struct future *future) {
    return atomic_load(&future->state);
}

inline void future_set_waiting(struct future *future) {
    atomic_store(&future->state, FUTURE_WAITING);
}

inline void future_finish(struct future *future) {
    atomic_store(&future->state, FUTURE_FINISHED);
}

inline void future_error_out(struct future *future, int error_number) {
    future->error_number = error_number;
    atomic_store(&future->state, FUTURE_ERROR);
}
