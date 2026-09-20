#ifndef TITIN_TASK_H
#define TITIN_TASK_H

#include <stdint.h>

#define TASK_LIMIT 64
#define TASK_STACK_SIZE 16384

typedef void (*task_entry)(void);

typedef enum {
    TASK_DORMANT,
    TASK_READY,
    TASK_RUNNING,
    TASK_STOPPED
} task_state;

typedef struct {
    uint64_t stack_pointer;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t instruction_pointer;
} task_context;

typedef struct {
    uint64_t id;
    const char *name;
    task_state state;
    task_entry entry;
    task_context context;
    uint8_t stack[TASK_STACK_SIZE];
} titin_task;

void task_system_start(void);

titin_task *task_create(
    uint64_t id,
    const char *name,
    task_entry entry
);

void task_run(titin_task *task);

void task_yield(void);

titin_task *task_current(void);

void task_finish(void);

uint64_t task_count(void);

const titin_task *task_get(
    uint64_t index
);

const char *task_state_name(
    task_state state
);

#endif
