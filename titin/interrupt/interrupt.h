#ifndef TITIN_INTERRUPT_H
#define TITIN_INTERRUPT_H

#include <stdint.h>

void interrupt_start(void);

void interrupt_enable(void);

void interrupt_disable(void);

void interrupt_handler(
    uint64_t vector
);

#endif
