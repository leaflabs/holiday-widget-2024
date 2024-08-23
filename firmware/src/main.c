#include "acceleration.h"
#include "ambient_light.h"
#include "charlieplex_driver.h"
#include "game_engine.h"
#include "job_queue.h"
#include "led_matrix.h"
#include "music_player.h"
#include "system_communication.h"
#include "tones.h"
#include "utils.h"
#include "widget_controller.h"
#include "widget_system.h"

void my_task() {
    set_game(SNOWFALL_GAME);
}

int main(void) {
    widget_system_init();

    job_add(&system_communication_setup, JOB_INIT);

    job_add(&widget_controller_setup, JOB_INIT);

    job_add(&acceleration_setup, JOB_INIT);
    job_add(&ambient_light_setup, JOB_INIT);
    job_add(&led_matrix_setup, JOB_INIT);
    job_add(&music_player_setup, JOB_INIT);
    job_add(&game_engine_setup, JOB_INIT);

    job_add(&uart_logger_run, JOB_RUN_RUN);
    job_add(&system_communication_run, JOB_RUN_RUN);

    job_add(&acceleration_run, JOB_RUN_RUN);
    job_add(&ambient_light_run, JOB_RUN_RUN);

    job_add(&led_matrix_loader_run, JOB_RUN_RUN);
    job_add(&led_matrix_renderer_run, JOB_RUN_RUN);
    job_add(&led_matrix_assembler_run, JOB_RUN_RUN);

    job_add(&music_player_run, JOB_RUN_RUN);
    job_add(&game_engine_run, JOB_RUN_RUN);
    job_add(&widget_controller_run, JOB_RUN_RUN);

    job_add(&my_task, JOB_RUN_ENTRY);

    while (1) {
        job_state_machine_run();
    }
}
