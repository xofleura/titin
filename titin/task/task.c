#include <stdint.h>
#include "task.h"
#include "../scheduler/scheduler.h"

static titin_task tasks[TASK_LIMIT];
static uint64_t task_count_value = 0;
static titin_task *current_task = 0;
static task_context kernel_context;

extern void task_switch(
    task_context *old_context,
    task_context *new_context
);

extern void task_bootstrap(void);

void task_system_start(void)
{
    task_count_value = 0;
    current_task = 0;

    kernel_context.stack_pointer = 0;
    kernel_context.rbp = 0;
    kernel_context.rbx = 0;
    kernel_context.r12 = 0;
    kernel_context.r13 = 0;
    kernel_context.r14 = 0;
    kernel_context.r15 = 0;
    kernel_context.instruction_pointer = 0;

    scheduler_start();
}

titin_task *task_create(
    uint64_t id,
    const char *name,
    task_entry entry
)
{
    if (task_count_value >= TASK_LIMIT) {
        return 0;
    }

    titin_task *task =
        &tasks[task_count_value];

    task->id = id;
    task->name = name;
    task->state = TASK_DORMANT;
    task->entry = entry;

    uint64_t stack =
        (uint64_t)&task->stack[TASK_STACK_SIZE];

    stack &= ~0xFULL;

    stack -= 8;

    *(uint64_t *)stack = 0;

    task->context.stack_pointer = stack;
    task->context.rbp = 0;
    task->context.rbx = 0;
    task->context.r12 = 0;
    task->context.r13 = 0;
    task->context.r14 = 0;
    task->context.r15 = 0;
    task->context.instruction_pointer =
        (uint64_t)task_bootstrap;

    task_count_value++;

    return task;
}

void task_run(titin_task *task)
{
    if (task == 0 ||
        task->entry == 0 ||
        task->state != TASK_READY) {
        return;
    }

    current_task = task;
    task->state = TASK_RUNNING;

    scheduler_remove(task);

    task_switch(
        &kernel_context,
        &task->context
    );

    task->state = TASK_STOPPED;
    current_task = 0;
}

void task_yield(void)
{
    titin_task *current = current_task;

    if (current == 0) {
        return;
    }

    titin_task *next =
        scheduler_next(current);

    if (next == 0 ||
        next == current) {
        return;
    }

    current->state = TASK_READY;
    scheduler_enqueue(current);

    next->state = TASK_RUNNING;
    scheduler_remove(next);

    current_task = next;

    task_switch(
        &current->context,
        &next->context
    );

    current_task = current;
    current->state = TASK_RUNNING;
}

titin_task *task_current(void)
{
    return current_task;
}

void task_finish(void)
{
    titin_task *finished = current_task;

    if (finished == 0) {
        for (;;) {
            __asm__ volatile ("cli");
            __asm__ volatile ("hlt");
        }
    }

    finished->state = TASK_STOPPED;
    scheduler_remove(finished);
    current_task = 0;

    task_switch(
        &finished->context,
        &kernel_context
    );

    for (;;) {
        __asm__ volatile ("cli");
        __asm__ volatile ("hlt");
    }
}

uint64_t task_count(void)
{
    return task_count_value;
}

const titin_task *task_get(uint64_t index)
{
    if (index >= task_count_value) {
        return 0;
    }

    return &tasks[index];
}

const char *task_state_name(
    task_state state
)
{
    switch (state) {
        case TASK_DORMANT:
            return "dormant";

        case TASK_READY:
            return "ready";

        case TASK_RUNNING:
            return "running";

        case TASK_STOPPED:
            return "stopped";
    }

    return "unknown";
}
