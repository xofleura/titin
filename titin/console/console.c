#include <stdint.h>
#include "console.h"

#define VIDEO_MEMORY 0xB8000
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
#define VGA_INDEX_PORT 0x3D4
#define VGA_DATA_PORT 0x3D5

static volatile uint16_t *video =
    (volatile uint16_t *)VIDEO_MEMORY;

static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

static inline void vga_out(
    uint16_t port,
    uint8_t value
)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void vga_cursor_shape(void)
{
    vga_out(VGA_INDEX_PORT, 0x0A);
    vga_out(VGA_DATA_PORT, 14);

    vga_out(VGA_INDEX_PORT, 0x0B);
    vga_out(VGA_DATA_PORT, 15);
}

static void vga_cursor_move(
    uint8_t x,
    uint8_t y
)
{
    uint16_t position =
        (uint16_t)y * SCREEN_WIDTH + x;

    vga_out(VGA_INDEX_PORT, 0x0F);
    vga_out(
        VGA_DATA_PORT,
        position & 0xFF
    );

    vga_out(VGA_INDEX_PORT, 0x0E);
    vga_out(
        VGA_DATA_PORT,
        (position >> 8) & 0xFF
    );
}

static void console_scroll(void)
{
    for (
        uint64_t y = 1;
        y < SCREEN_HEIGHT;
        y++
    ) {
        for (
            uint64_t x = 0;
            x < SCREEN_WIDTH;
            x++
        ) {
            video[
                (y - 1) * SCREEN_WIDTH + x
            ] = video[
                y * SCREEN_WIDTH + x
            ];
        }
    }

    for (
        uint64_t x = 0;
        x < SCREEN_WIDTH;
        x++
    ) {
        video[
            (SCREEN_HEIGHT - 1) *
            SCREEN_WIDTH + x
        ] = (uint16_t)' ' | 0x0700;
    }

    cursor_y =
        SCREEN_HEIGHT - 1;
}

void console_cursor_set(
    uint8_t x,
    uint8_t y
)
{
    if (x >= SCREEN_WIDTH) {
        x = SCREEN_WIDTH - 1;
    }

    if (y >= SCREEN_HEIGHT) {
        y = SCREEN_HEIGHT - 1;
    }

    cursor_x = x;
    cursor_y = y;

    vga_cursor_move(
        cursor_x,
        cursor_y
    );
}

uint8_t console_cursor_x(void)
{
    return cursor_x;
}

uint8_t console_cursor_y(void)
{
    return cursor_y;
}

void console_write_at(
    uint8_t x,
    uint8_t y,
    char c
)
{
    if (x >= SCREEN_WIDTH ||
        y >= SCREEN_HEIGHT) {
        return;
    }

    video[
        y * SCREEN_WIDTH + x
    ] = (uint16_t)c | 0x0700;
}

void console_fill_at(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    char c
)
{
    for (
        uint16_t row = 0;
        row < height;
        row++
    ) {
        for (
            uint16_t column = 0;
            column < width;
            column++
        ) {
            uint16_t screen_x =
                (uint16_t)x + column;

            uint16_t screen_y =
                (uint16_t)y + row;

            if (
                screen_x >= SCREEN_WIDTH ||
                screen_y >= SCREEN_HEIGHT
            ) {
                continue;
            }

            video[
                screen_y * SCREEN_WIDTH +
                screen_x
            ] = (uint16_t)c | 0x0700;
        }
    }
}

void console_redraw_line(
    uint8_t x,
    uint8_t y,
    const char *text,
    uint64_t cursor
)
{
    uint64_t i = 0;

    while (
        x + i < SCREEN_WIDTH
    ) {
        video[
            y * SCREEN_WIDTH +
            x + i
        ] = (uint16_t)' ' | 0x0700;

        i++;
    }

    i = 0;

    while (
        text[i] != '\0' &&
        x + i < SCREEN_WIDTH
    ) {
        video[
            y * SCREEN_WIDTH +
            x + i
        ] = (uint16_t)text[i] | 0x0700;

        i++;
    }

    cursor_x =
        x + cursor;

    cursor_y = y;

    if (cursor_x >= SCREEN_WIDTH) {
        cursor_x =
            SCREEN_WIDTH - 1;
    }

    vga_cursor_move(
        cursor_x,
        cursor_y
    );
}

void console_clear(void)
{
    for (
        uint64_t i = 0;
        i < SCREEN_WIDTH *
            SCREEN_HEIGHT;
        i++
    ) {
        video[i] =
            (uint16_t)' ' | 0x0700;
    }

    cursor_x = 0;
    cursor_y = 0;

    vga_cursor_move(
        cursor_x,
        cursor_y
    );
}

void console_put_char(char c)
{
    if (c == '\n') {
        console_newline();
        return;
    }

    video[
        cursor_y *
        SCREEN_WIDTH +
        cursor_x
    ] = (uint16_t)c | 0x0700;

    cursor_x++;

    if (
        cursor_x >= SCREEN_WIDTH
    ) {
        console_newline();
        return;
    }

    vga_cursor_move(
        cursor_x,
        cursor_y
    );
}

void console_backspace(void)
{
    if (cursor_x == 0) {
        return;
    }

    cursor_x--;

    video[
        cursor_y *
        SCREEN_WIDTH +
        cursor_x
    ] = (uint16_t)' ' | 0x0700;

    vga_cursor_move(
        cursor_x,
        cursor_y
    );
}

void console_newline(void)
{
    cursor_x = 0;
    cursor_y++;

    if (
        cursor_y >= SCREEN_HEIGHT
    ) {
        console_scroll();
    }

    vga_cursor_move(
        cursor_x,
        cursor_y
    );
}

void console_prompt(void)
{
    console_put_char('-');
    console_put_char(' ');
}

static void console_write(
    const char *text
)
{
    for (
        uint64_t i = 0;
        text[i] != '\0';
        i++
    ) {
        console_put_char(text[i]);
    }
}

void console_start(void)
{
    console_clear();
    vga_cursor_shape();

    console_write("\n");
    console_write(" |-|    *\n");
    console_write(" |-|   _    *  __\n");
    console_write(" |-|   |  *    |/'\n");
    console_write(" |-|   |~*~~~o~|\n");
    console_write(" |-|   |  O o *|\n");
    console_write("/___\\  |o___O__|\n");
    console_write("\n");
    console_write("Titin 0.1\n");
}

