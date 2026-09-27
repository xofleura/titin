#ifndef TITIN_DISPLAY_H
#define TITIN_DISPLAY_H

#include <stdint.h>

void display_start(
    uint32_t multiboot_magic,
    uint32_t multiboot_info
);

int display_available(void);

uint32_t display_width(void);

uint32_t display_height(void);

uint32_t display_pitch(void);

uint8_t display_bpp(void);

uint64_t display_address(void);

void display_begin_frame(void);

void display_present(void);

void display_present_region(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height
);

void display_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
);

void display_rectangle(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
);

void display_clear(
    uint32_t color
);

void display_text(
    uint32_t x,
    uint32_t y,
    const char *text,
    uint32_t color,
    uint32_t scale
);

void display_mouse_cursor_hide(void);

void display_mouse_cursor(
    uint32_t x,
    uint32_t y
);

#endif
