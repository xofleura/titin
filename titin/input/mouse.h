#ifndef TITIN_MOUSE_H
#define TITIN_MOUSE_H

#include <stdint.h>

typedef struct {
    int32_t x;
    int32_t y;
    int32_t movement_x;
    int32_t movement_y;
    uint8_t buttons;
    uint64_t packets;
} titin_mouse_state;

void mouse_start(void);

void mouse_poll(void);

void mouse_interrupt(void);

const titin_mouse_state *mouse_state(void);

#endif
