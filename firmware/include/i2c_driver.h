#ifndef __I2C_DRIVER_H
#define __I2C_DRIVER_H

#include <stdatomic.h>

#include "futures.h"
#include "stm32l0xx_hal.h"

/*
    Details of this driver:

    This driver is non blocking. Calling an i2c_driver_read/write_register
    function adds an i2c_request to a queue which is processed one by one.

    After an i2c transaction has finished, a callback is called from the HAL
    which then dequeues the next item (if available) and continues
    the process. A lock prevents race conditions.

    The queue is lock-free, unconditionally exiting and failing on the overfull
    condition.

    For initalization functions for drivers, blocking is allowed
    and ignoring the return value since we are still in initalization and
    there are no other items being added to the queue.
*/

/*
    Allow for compile time selection of queue size. Else, define the default
   here
*/
#ifndef QUEUE_LENGTH_POWER
#define QUEUE_LENGTH_POWER 4
#endif

#define QUEUE_LENGTH (1 << QUEUE_LENGTH_POWER)

/*
 * Every i2c request has these qualities.
 */
struct i2c_request {
    enum { I2C_READ = 0x0, I2C_WRITE = 0x1 } action;
    // What I2C address to talk to
    uint8_t address;
    // What internal register to use
    uint8_t ireg;
    // Buffer to write data into
    uint8_t *buffer;
    // Number of bytes to read/write
    size_t num_bytes;

    struct future future;
};

struct i2c_queue_context {
    struct i2c_request *queue[QUEUE_LENGTH];

    size_t start;  // Dequeue from start
    size_t end;    // Enqueue to end
    size_t n_elts_used;

    // non-NULL iff this request is pending or hasn't yet been fully dequeued
    struct i2c_request *current_request;

    _Atomic enum { I2C_USED = 0, I2C_FREE = 1 } i2c_bus_in_use;
};

struct i2c_driver_context {
    I2C_HandleTypeDef i2c;
    struct i2c_queue_context queue_context;
};

/*
    Initalizes the i2c handle and the context so the queue and interrupts can
   work correctly.

    'context' is a reference to an i2c_driver_context object that
    should be statically allocated.

    'instance' is which i2c instance to associate with

    Returns 0 on success.
*/
int i2c_driver_init(struct i2c_driver_context *context, I2C_TypeDef *instance);

/*
 * Returns true if i2c device with given address is ready.
 */
int i2c_device_is_ready(struct i2c_driver_context *i2c_context,
                        uint8_t address);

/*
 * Enqueue a transaction to the context specified.
 * returns 0 if the transaction was added sucessfully
 */
int i2c_enqueue_request(struct i2c_driver_context *i2c_context,
                        struct i2c_request *request);

/*
 * Checks the queue and if there is data to process, sends the next transaction
 * to the bus. This is designed to be run repeatedly in the main loop. For
 * transactions outside of this paradigm, see `i2c_blocking_enqueue` next.
 */
void i2c_queue_process_one(struct i2c_driver_context *i2c_context);

/*
 * Enqueues a transaction then calls `i2c_queue_process_one` until it is
 * fully processed. Used mostly for driver initialization of I2C devices.
 */
int i2c_blocking_enqueue(struct i2c_driver_context *i2c_context,
                         struct i2c_request *request);

#endif
