#ifndef TITIN_CONSOLE_H
#define TITIN_CONSOLE_H

#include <stdint.h>

struct titin_window;

void console_start(void);

void console_bind_window(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height
);

void console_bind_fullscreen(void);

int console_create_window(
    struct titin_window *window
);

int console_close_window(
    struct titin_window *window
);

int console_focus_window(
    struct titin_window *window
);

int console_window_is_terminal(
    struct titin_window *window
);

struct titin_window *console_active_window(void);

void console_follow_window(
    struct titin_window *window
);

void console_render(void);

void console_render_window(
    struct titin_window *window
);

void console_clear(void);

void console_put_char(
    char c
);

void console_backspace(void);

void console_newline(void);

void console_prompt(void);

uint8_t console_cursor_x(void);

uint8_t console_cursor_y(void);

void console_cursor_set(
    uint8_t x,
    uint8_t y
);

void console_write_at(
    uint8_t x,
    uint8_t y,
    char c
);

void console_fill_at(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    char c
);

void console_redraw_line(
    uint8_t x,
    uint8_t y,
    const char *text,
    uint64_t cursor
);

#endif
