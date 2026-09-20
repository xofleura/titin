#ifndef TITIN_LINE_H
#define TITIN_LINE_H

#include <stdint.h>

void input_line_clear(void);
void input_line_add(char c);
void input_line_backspace(void);
void input_line_left(void);
void input_line_right(void);
const char *input_line_get(void);
uint64_t input_line_length(void);
uint64_t input_line_cursor(void);

#endif
