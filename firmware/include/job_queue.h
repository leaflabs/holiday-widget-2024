#ifndef __JOB_QUEUE_H__
#define __JOB_QUEUE_H__

#include <stdbool.h>

/*
 * A job is a function called, without argument, during
 * the course of program execution. There are five states:
 *  * `JOB_INIT` jobs are run exactly once at the beginning of
 * execution.
 *  * `JOB_RUN_ENTRY` jobs are run exactly once when
 * we enter the run state.
 *  * `JOB_RUN_RUN` jobs are run
 * consecutively for as long as we are in the run state.
 *  * `JOB_ERROR_ENTRY` jobs are run exactly once as we enter the
 * error state.
 *  * `JOB_ERROR_RUN` jobs are run consecutively for
 * as long as we are in the error state.
 *
 * Jobs may be added to the job queue with `job_add`. It is recommended
 * that _all_ such jobs are added before beginning to call
 * `job_state_machine_run`. Jobs are run in the order in which `job_add` is
 * called.
 */
typedef void (*job_fn_t)(void);

enum job_type {
    JOB_INIT = 0,
    JOB_RUN_ENTRY,
    JOB_RUN_RUN,
    JOB_ERROR_ENTRY,
    JOB_ERROR_RUN,
    N_JOB_TYPES,
};

int job_add(job_fn_t fn, enum job_type type);

/*
 * `job_state_machine_run` should be called for as long as the main loop runs.
 */
void job_state_machine_run(void);

/*
 * Any job may call `job_error_state_enter` for an _unrecoverable error_.
 * It will halt all other ordinary job execution and hijack the main loop!
 */
void job_error_state_enter(int error);

#endif /* __JOB_QUEUE_H__ */
