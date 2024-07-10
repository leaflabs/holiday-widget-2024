#include "job_queue.h"

#include <errno.h>
#include <stdatomic.h>

#define QUEUE_SIZE 16

struct job_queue {
    job_fn_t job_fns[QUEUE_SIZE];
    size_t n_entries;
};

static struct job_queue job_queues[N_JOB_TYPES];

static _Atomic enum job_type job_state = JOB_INIT;

static _Atomic int job_errno = 0;

int job_add(job_fn_t fn, enum job_type type) {
    if (type >= N_JOB_TYPES || type < JOB_INIT) {
        return -EINVAL;
    }

    struct job_queue *queue = &job_queues[type];
    size_t n = queue->n_entries;

    if (n >= QUEUE_SIZE) {
        return -ENOMEM;
    }

    queue->job_fns[n] = fn;
    queue->n_entries++;

    return 0;
}

void job_state_machine_run(void) {
    enum job_type state = atomic_load(&job_state);
    struct job_queue *queue = &job_queues[state];

    static size_t job_n = 0;
    static bool in_error_state = false;

    if (!in_error_state && state == JOB_ERROR_ENTRY) {
        // We have to reset `job_n` here, anamolously, to avoid state elsewhere.
        in_error_state = true;
        job_n = 0;
    }

    static const enum job_type next_state[N_JOB_TYPES] = {
        [JOB_INIT] = JOB_RUN_ENTRY,      [JOB_RUN_ENTRY] = JOB_RUN_RUN,
        [JOB_RUN_RUN] = JOB_RUN_RUN,     [JOB_ERROR_ENTRY] = JOB_ERROR_RUN,
        [JOB_ERROR_RUN] = JOB_ERROR_RUN,
    };

    if (job_n < queue->n_entries) {
        queue->job_fns[job_n]();
        job_n++;
    } else {
        job_n = 0;
        atomic_store(&job_state, next_state[state]);
    }
}

void job_error_state_enter(int error) {
    atomic_store(&job_errno, error);
    atomic_store(&job_state, JOB_ERROR_ENTRY);
}
