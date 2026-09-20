#ifndef TITIN_CONSOLE_H
#define TITIN_CONSOLE_H

#include <stdint.h>

void console_start(void);
void console_clear(void);
void console_put_char(char c);
void console_backspace(void);
void console_newline(void);
void console_prompt(void);
void console_cursor_set(uint8_t x, uint8_t y);
uint8_t console_cursor_x(void);
uint8_t console_cursor_y(void);
void console_redraw_line(
    uint8_t x,
    uint8_t y,
    const char *text,
    uint64_t cursor
);

#endif
