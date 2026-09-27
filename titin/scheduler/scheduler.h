#ifndef TITIN_SCHEDULER_H
#define TITIN_SCHEDULER_H

#include "../task/task.h"

void scheduler_start(void);

void scheduler_enqueue(
    titin_task *task
);

void scheduler_remove(
    titin_task *task
);

titin_task *scheduler_next(
    titin_task *current
);

#endif
