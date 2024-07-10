#include "acceleration.h"
#include "ambient_light.h"
#include "job_queue.h"
#include "system_communication.h"
#include "temperature.h"
#include "utils.h"
#include "widget_system.h"

int main(void) {
    widget_system_init();

    job_add(&system_communication_setup, JOB_INIT);
    job_add(&print_available_i2c_devices, JOB_INIT);

    job_add(&temperature_setup, JOB_INIT);
    job_add(&acceleration_setup, JOB_INIT);
    job_add(&ambient_light_setup, JOB_INIT);

    job_add(&system_communication_run, JOB_RUN_RUN);

    job_add(&temperature_run, JOB_RUN_RUN);
    job_add(&acceleration_run, JOB_RUN_RUN);
    job_add(&ambient_light_run, JOB_RUN_RUN);

    while (1) {
        job_state_machine_run();
    }
}
