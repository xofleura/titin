#include <stdint.h>
#include "scheduler.h"

static titin_task *ready_queue[TASK_LIMIT];
static uint64_t ready_count = 0;

void scheduler_start(void)
{
    ready_count = 0;

    for (uint64_t i = 0;
         i < TASK_LIMIT;
         i++) {
        ready_queue[i] = 0;
    }
}

void scheduler_enqueue(
    titin_task *task
)
{
    if (task == 0) {
        return;
    }

    if (ready_count >= TASK_LIMIT) {
        return;
    }

    for (uint64_t i = 0;
         i < ready_count;
         i++) {
        if (ready_queue[i] == task) {
            return;
        }
    }

    ready_queue[ready_count] = task;
    ready_count++;
}

void scheduler_remove(
    titin_task *task
)
{
    if (task == 0) {
        return;
    }

    for (uint64_t i = 0;
         i < ready_count;
         i++) {
        if (ready_queue[i] != task) {
            continue;
        }

        for (uint64_t j = i + 1;
             j < ready_count;
             j++) {
            ready_queue[j - 1] =
                ready_queue[j];
        }

        ready_queue[ready_count - 1] = 0;
        ready_count--;

        return;
    }
}

titin_task *scheduler_next(
    titin_task *current
)
{
    if (ready_count == 0) {
        return 0;
    }

    if (current == 0) {
        return ready_queue[0];
    }

    for (uint64_t i = 0;
         i < ready_count;
         i++) {
        if (ready_queue[i] != current) {
            continue;
        }

        uint64_t next =
            (i + 1) % ready_count;

        return ready_queue[next];
    }

    return ready_queue[0];
}
