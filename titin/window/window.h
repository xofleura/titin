#ifndef TITIN_WINDOW_H
#define TITIN_WINDOW_H

#include <stdint.h>

#define WINDOW_TITLE_SIZE 64

typedef struct titin_window {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;

    uint32_t border_color;
    uint32_t title_color;
    uint32_t content_color;

    char title[WINDOW_TITLE_SIZE];

    int active;
    int dragging;

    uint32_t drag_x;
    uint32_t drag_y;
} titin_window;

void window_system_start(void);

titin_window *window_create(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    const char *title
);

int window_close(
    titin_window *window
);

void window_draw(
    titin_window *window
);

void window_draw_all(void);

uint64_t window_count(void);

titin_window *window_get(
    uint64_t index
);

titin_window *window_at(
    uint32_t x,
    uint32_t y
);

int window_title_hit(
    titin_window *window,
    uint32_t x,
    uint32_t y
);

void window_focus(
    titin_window *window
);

void window_begin_drag(
    titin_window *window,
    uint32_t mouse_x,
    uint32_t mouse_y
);

void window_drag(
    titin_window *window,
    uint32_t mouse_x,
    uint32_t mouse_y
);

void window_end_drag(
    titin_window *window
);

void window_redraw(void);

uint32_t window_content_x(
    titin_window *window
);

uint32_t window_content_y(
    titin_window *window
);

uint32_t window_content_width(
    titin_window *window
);

uint32_t window_content_height(
    titin_window *window
);

#endif
