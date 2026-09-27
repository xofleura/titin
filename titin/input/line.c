#include <stdint.h>
#include "line.h"

#define INPUT_LINE_SIZE 128

static char buffer[INPUT_LINE_SIZE];
static uint64_t length = 0;
static uint64_t cursor = 0;

void input_line_clear(void)
{
    length = 0;
    cursor = 0;
    buffer[0] = '\0';
}

void input_line_add(char c)
{
    if (length >= INPUT_LINE_SIZE - 1) {
        return;
    }

    for (uint64_t i = length; i > cursor; i--) {
        buffer[i] = buffer[i - 1];
    }

    buffer[cursor] = c;
    length++;
    cursor++;
    buffer[length] = '\0';
}

void input_line_backspace(void)
{
    if (cursor == 0) {
        return;
    }

    for (uint64_t i = cursor; i < length; i++) {
        buffer[i - 1] = buffer[i];
    }

    cursor--;
    length--;
    buffer[length] = '\0';
}

void input_line_left(void)
{
    if (cursor > 0) {
        cursor--;
    }
}

void input_line_right(void)
{
    if (cursor < length) {
        cursor++;
    }
}

const char *input_line_get(void)
{
    return buffer;
}

uint64_t input_line_length(void)
{
    return length;
}

uint64_t input_line_cursor(void)
{
    return cursor;
}
