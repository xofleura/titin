#include <stdint.h>
#include "mouse.h"
#include "../display/display.h"

#define MOUSE_STATUS_OUTPUT 0x01
#define MOUSE_STATUS_INPUT 0x02
#define MOUSE_STATUS_AUX 0x20

#define MOUSE_COMMAND_ENABLE 0xA8
#define MOUSE_COMMAND_READ_CONFIG 0x20
#define MOUSE_COMMAND_WRITE_CONFIG 0x60
#define MOUSE_COMMAND_WRITE_MOUSE 0xD4

#define MOUSE_SET_DEFAULTS 0xF6
#define MOUSE_ENABLE_REPORTING 0xF4

#define MOUSE_ACK 0xFA

static titin_mouse_state state;

static uint8_t packet[3];

static uint8_t packet_position;

static uint8_t mouse_status(void)
{
    uint8_t status;

    __asm__ volatile (
        "inb $0x64, %0"
        : "=a"(status)
    );

    return status;
}

static int mouse_wait_input(void)
{
    uint32_t timeout = 100000;

    while (timeout > 0) {
        if (!(mouse_status() &
              MOUSE_STATUS_INPUT)) {
            return 1;
        }

        timeout--;
    }

    return 0;
}

static int mouse_wait_output(void)
{
    uint32_t timeout = 100000;

    while (timeout > 0) {
        if (mouse_status() &
            MOUSE_STATUS_OUTPUT) {
            return 1;
        }

        timeout--;
    }

    return 0;
}

static uint8_t mouse_read_data(void)
{
    uint8_t value;

    __asm__ volatile (
        "inb $0x60, %0"
        : "=a"(value)
    );

    return value;
}

static void mouse_write_controller(
    uint8_t value
)
{
    if (!mouse_wait_input()) {
        return;
    }

    __asm__ volatile (
        "outb %0, $0x64"
        :
        : "a"(value)
    );
}

static void mouse_write_data(
    uint8_t value
)
{
    if (!mouse_wait_input()) {
        return;
    }

    __asm__ volatile (
        "outb %0, $0x60"
        :
        : "a"(value)
    );
}

static void mouse_flush(void)
{
    uint32_t count = 64;

    while (count > 0) {
        if (!(mouse_status() &
              MOUSE_STATUS_OUTPUT)) {
            return;
        }

        mouse_read_data();

        count--;
    }
}

static int mouse_send(
    uint8_t command
)
{
    mouse_write_controller(
        MOUSE_COMMAND_WRITE_MOUSE
    );

    mouse_write_data(
        command
    );

    if (!mouse_wait_output()) {
        return 0;
    }

    return mouse_read_data() ==
        MOUSE_ACK;
}

void mouse_start(void)
{
    state.x = 640;
    state.y = 360;
    state.movement_x = 0;
    state.movement_y = 0;
    state.buttons = 0;
    state.packets = 0;

    packet_position = 0;

    mouse_flush();

    mouse_write_controller(
        MOUSE_COMMAND_ENABLE
    );

    mouse_flush();

    mouse_write_controller(
        MOUSE_COMMAND_READ_CONFIG
    );

    if (mouse_wait_output()) {
        uint8_t config =
            mouse_read_data();

        config |= 0x02;
        config &= (uint8_t)~0x20;

        mouse_write_controller(
            MOUSE_COMMAND_WRITE_CONFIG
        );

        mouse_write_data(
            config
        );
    }

    mouse_flush();

    mouse_send(
        MOUSE_SET_DEFAULTS
    );

    mouse_flush();

    mouse_send(
        MOUSE_ENABLE_REPORTING
    );

    mouse_flush();

    if (display_available()) {
        state.x =
            display_width() / 2;

        state.y =
            display_height() / 2;
    }
}

void mouse_poll(void)
{
}

void mouse_interrupt(void)
{
    uint8_t status =
        mouse_status();

    if (!(status &
          MOUSE_STATUS_OUTPUT)) {
        return;
    }

    if (!(status &
          MOUSE_STATUS_AUX)) {
        return;
    }

    uint8_t value =
        mouse_read_data();

    if (packet_position == 0) {
        if (!(value & 0x08)) {
            return;
        }
    }

    packet[
        packet_position
    ] = value;

    packet_position++;

    if (packet_position < 3) {
        return;
    }

    packet_position = 0;

    int32_t movement_x =
        (int32_t)packet[1];

    int32_t movement_y =
        (int32_t)packet[2];

    if (packet[0] & 0x10) {
        movement_x -= 256;
    }

    if (packet[0] & 0x20) {
        movement_y -= 256;
    }

    state.movement_x =
        movement_x;

    state.movement_y =
        movement_y;

    state.x += movement_x;
    state.y -= movement_y;

    if (state.x < 0) {
        state.x = 0;
    }

    if (state.y < 0) {
        state.y = 0;
    }

    if (display_available()) {
        if (
            state.x >=
            (int32_t)display_width()
        ) {
            state.x =
                display_width() - 1;
        }

        if (
            state.y >=
            (int32_t)display_height()
        ) {
            state.y =
                display_height() - 1;
        }
    }

    state.buttons =
        packet[0] & 0x07;

    state.packets++;
}

const titin_mouse_state *mouse_state(void)
{
    return &state;
}
