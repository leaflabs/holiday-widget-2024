#ifndef __FUTURES_H__
#define __FUTURES_H__

#include <stdatomic.h>

/*
 * A `future` is a primitive for asynchronous I/O.
 * The driver is responsible for implementing its semantics:
 * There is one producer responsible for setting its values, usually
 * the driver itself. There may be more than one consumer which reads its
 * values. Thus, all values should be considered invalid when the `state`
 * is `FUTURE_WAITING`. And values should be set before setting the `state`.
 */
struct future {
    volatile _Atomic enum {
        FUTURE_WAITING = 0,
        FUTURE_FINISHED,
        FUTURE_ERROR
    } state;
    int error_number;
};

void future_await(struct future *future);
int future_is_waiting(struct future *future);
int future_is_finished(struct future *future);
int future_is_errored(struct future *future);
int future_get_state(struct future *future);
void future_set_waiting(struct future *future);
void future_finish(struct future *future);
void future_error_out(struct future *future, int error_number);

#endif /* __FUTURES_H__ */
