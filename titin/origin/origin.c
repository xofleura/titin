#include <stdint.h>
#include "origin.h"
#include "../task/task.h"
#include "../scheduler/scheduler.h"
#include "../protein/command.h"

static void origin_task_entry(void)
{
    command_write("Origin: beginning.\n");

    task_yield();

    command_write("Origin: resumed.\n");

    task_yield();

    command_write("Origin: finishing.\n");

    task_finish();
}

static void pulse_task_entry(void)
{
    command_write("Pulse: beginning.\n");

    task_yield();

    command_write("Pulse: resumed.\n");

    task_yield();

    command_write("Pulse: finishing.\n");

    task_finish();
}

void origin_start(void)
{
    titin_task *origin =
        task_create(
            0,
            "Origin",
            origin_task_entry
        );

    titin_task *pulse =
        task_create(
            1,
            "Pulse",
            pulse_task_entry
        );

    if (origin == 0 || pulse == 0) {
        command_write("Task creation failed.\n");
        return;
    }

    origin->state = TASK_READY;
    pulse->state = TASK_READY;

    scheduler_enqueue(origin);
    scheduler_enqueue(pulse);

    task_run(origin);
    task_run(pulse);
}
