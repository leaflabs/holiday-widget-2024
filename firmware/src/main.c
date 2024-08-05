#include "acceleration.h"
#include "ambient_light.h"
#include "job_queue.h"
#include "led_matrix.h"
#include "music_player.h"
#include "system_communication.h"
#include "tones.h"
#include "utils.h"
#include "widget_controller.h"
#include "widget_system.h"

int main(void) {
    widget_system_init();

    job_add(&system_communication_setup, JOB_INIT);
    job_add(&print_available_i2c_devices, JOB_INIT);

    job_add(&acceleration_setup, JOB_INIT);
    job_add(&ambient_light_setup, JOB_INIT);
    job_add(&led_matrix_setup, JOB_INIT);
    job_add(&music_player_setup, JOB_INIT);

    job_add(&system_communication_run, JOB_RUN_RUN);

    job_add(&acceleration_run, JOB_RUN_RUN);
    job_add(&ambient_light_run, JOB_RUN_RUN);

    job_add(&widget_controller_run, JOB_RUN_RUN);

    //    job_add(&led_matrix_loader_run, JOB_RUN_RUN);
    //    job_add(&led_matrix_assembler_run, JOB_RUN_RUN);
    //    job_add(&led_matrix_drawer_run, JOB_RUN_RUN);
    //    job_add(&music_player_run, JOB_RUN_RUN);

    while (1) {
        job_state_machine_run();
    }
}
