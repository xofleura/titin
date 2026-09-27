#include <stdint.h>
#include "display.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36D76289
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8

#define DISPLAY_MAX_WIDTH 1280
#define DISPLAY_MAX_HEIGHT 720

static int framebuffer_found;

static uint64_t framebuffer_address_value;
static uint32_t framebuffer_pitch_value;
static uint32_t framebuffer_width_value;
static uint32_t framebuffer_height_value;
static uint8_t framebuffer_bpp_value;

static uint32_t framebuffer_buffer[
    DISPLAY_MAX_WIDTH *
    DISPLAY_MAX_HEIGHT
];

static uint32_t mouse_saved_pixels[16][16];

static uint32_t mouse_saved_x;
static uint32_t mouse_saved_y;

static uint32_t mouse_cursor_x;
static uint32_t mouse_cursor_y;

static int mouse_saved;
static int mouse_cursor_visible;

static const uint16_t mouse_cursor[16] = {
    0x8000,
    0xC000,
    0xE000,
    0xF000,
    0xF800,
    0xFC00,
    0xFE00,
    0xFF00,
    0xFF80,
    0xFFC0,
    0xFEE0,
    0xC660,
    0xC300,
    0x0180,
    0x0180,
    0x0000
};

static const uint8_t font[96][5] = {
    {0, 0, 0, 0, 0},
    {0, 0, 95, 0, 0},
    {0, 7, 0, 7, 0},
    {20, 127, 20, 127, 20},
    {36, 42, 127, 42, 18},
    {35, 19, 8, 100, 98},
    {54, 73, 85, 34, 80},
    {0, 0, 7, 0, 0},
    {0, 28, 34, 65, 0},
    {0, 65, 34, 28, 0},
    {20, 8, 62, 8, 20},
    {8, 8, 62, 8, 8},
    {0, 80, 48, 0, 0},
    {8, 8, 8, 8, 8},
    {0, 96, 96, 0, 0},
    {32, 16, 8, 4, 2},

    {62, 81, 73, 69, 62},
    {0, 66, 127, 64, 0},
    {66, 97, 81, 73, 70},
    {33, 65, 69, 75, 49},
    {24, 20, 18, 127, 16},
    {39, 69, 69, 69, 57},
    {60, 74, 73, 73, 48},
    {1, 113, 9, 5, 3},
    {54, 73, 73, 73, 54},
    {6, 73, 73, 41, 30},
    {0, 54, 54, 0, 0},
    {0, 86, 54, 0, 0},
    {8, 20, 34, 65, 0},
    {20, 20, 20, 20, 20},
    {0, 65, 34, 20, 8},
    {2, 1, 2, 4, 2},
    {50, 73, 121, 65, 62},

    {126, 17, 17, 17, 126},
    {127, 73, 73, 73, 54},
    {62, 65, 65, 65, 34},
    {127, 65, 65, 34, 28},
    {127, 73, 73, 73, 65},
    {127, 9, 9, 9, 1},
    {62, 65, 73, 73, 122},
    {127, 8, 8, 8, 127},
    {0, 65, 127, 65, 0},
    {32, 64, 65, 63, 1},
    {127, 8, 20, 34, 65},
    {127, 64, 64, 64, 64},
    {127, 2, 12, 2, 127},
    {127, 4, 8, 16, 127},
    {62, 65, 65, 65, 62},
    {127, 9, 9, 9, 6},
    {62, 65, 81, 33, 94},
    {127, 9, 25, 41, 70},
    {70, 73, 73, 73, 49},
    {1, 1, 127, 1, 1},
    {63, 64, 64, 64, 63},
    {31, 32, 64, 32, 31},
    {127, 32, 24, 32, 127},
    {99, 20, 8, 20, 99},
    {3, 4, 120, 4, 3},
    {97, 81, 73, 69, 67},

    {0, 127, 65, 65, 0},
    {2, 4, 8, 16, 32},
    {0, 65, 65, 127, 0},
    {4, 2, 1, 2, 4},
    {64, 64, 64, 64, 64},
    {0, 1, 2, 4, 0},

    {32, 84, 84, 120, 64},
    {127, 40, 68, 68, 56},
    {56, 68, 68, 68, 40},
    {56, 68, 68, 40, 127},
    {56, 84, 84, 84, 24},
    {8, 126, 9, 1, 2},
    {24, 164, 164, 152, 124},
    {127, 8, 4, 4, 120},
    {0, 68, 125, 64, 0},
    {64, 128, 128, 125, 0},
    {127, 16, 40, 68, 0},
    {0, 65, 127, 64, 0},
    {124, 4, 120, 4, 120},
    {124, 8, 4, 4, 120},
    {56, 68, 68, 68, 56},
    {252, 36, 36, 36, 24},
    {24, 36, 36, 24, 252},
    {124, 8, 4, 4, 8},
    {72, 84, 84, 84, 36},
    {4, 63, 68, 64, 32},
    {60, 64, 64, 32, 124},
    {28, 32, 64, 32, 28},
    {60, 64, 48, 64, 60},
    {68, 40, 16, 40, 68},
    {12, 144, 144, 144, 124},
    {68, 100, 84, 76, 68},

    {0, 8, 54, 65, 0},
    {0, 0, 127, 0, 0},
    {0, 65, 54, 8, 0},
    {2, 1, 2, 4, 2},
    {60, 90, 165, 90, 60}
};

static volatile uint32_t *framebuffer_pointer(void)
{
    return (volatile uint32_t *)(uintptr_t)
        framebuffer_address_value;
}

static uint32_t framebuffer_pixels_per_line(void)
{
    return framebuffer_pitch_value / 4;
}

void display_start(
    uint32_t multiboot_magic,
    uint32_t multiboot_info
)
{
    framebuffer_found = 0;
    framebuffer_address_value = 0;
    framebuffer_pitch_value = 0;
    framebuffer_width_value = 0;
    framebuffer_height_value = 0;
    framebuffer_bpp_value = 0;

    mouse_saved = 0;
    mouse_cursor_visible = 0;
    mouse_cursor_x = 0;
    mouse_cursor_y = 0;

    if (
        multiboot_magic !=
        MULTIBOOT2_BOOTLOADER_MAGIC
    ) {
        return;
    }

    uint32_t total_size =
        *(uint32_t *)(uintptr_t)
            multiboot_info;

    uint32_t position = 8;

    while (
        position < total_size
    ) {
        uint32_t type =
            *(uint32_t *)(uintptr_t)(
                multiboot_info +
                position
            );

        uint32_t size =
            *(uint32_t *)(uintptr_t)(
                multiboot_info +
                position + 4
            );

        if (
            type ==
            MULTIBOOT_TAG_TYPE_END
        ) {
            break;
        }

        if (
            type ==
            MULTIBOOT_TAG_TYPE_FRAMEBUFFER
        ) {
            framebuffer_address_value =
                *(uint64_t *)(uintptr_t)(
                    multiboot_info +
                    position + 8
                );

            framebuffer_pitch_value =
                *(uint32_t *)(uintptr_t)(
                    multiboot_info +
                    position + 16
                );

            framebuffer_width_value =
                *(uint32_t *)(uintptr_t)(
                    multiboot_info +
                    position + 20
                );

            framebuffer_height_value =
                *(uint32_t *)(uintptr_t)(
                    multiboot_info +
                    position + 24
                );

            framebuffer_bpp_value =
                *(uint8_t *)(uintptr_t)(
                    multiboot_info +
                    position + 28
                );

            if (
                framebuffer_bpp_value != 32 ||
                framebuffer_width_value >
                    DISPLAY_MAX_WIDTH ||
                framebuffer_height_value >
                    DISPLAY_MAX_HEIGHT
            ) {
                return;
            }

            framebuffer_found = 1;

            return;
        }

        if (size == 0) {
            break;
        }

        position +=
            (size + 7) & ~7;
    }
}

int display_available(void)
{
    return framebuffer_found;
}

uint32_t display_width(void)
{
    return framebuffer_width_value;
}

uint32_t display_height(void)
{
    return framebuffer_height_value;
}

uint32_t display_pitch(void)
{
    return framebuffer_pitch_value;
}

uint8_t display_bpp(void)
{
    return framebuffer_bpp_value;
}

uint64_t display_address(void)
{
    return framebuffer_address_value;
}

void display_begin_frame(void)
{
}

void display_present(void)
{
    display_present_region(
        0,
        0,
        framebuffer_width_value,
        framebuffer_height_value
    );
}

void display_present_region(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height
)
{
    if (!framebuffer_found) {
        return;
    }

    if (
        x >= framebuffer_width_value ||
        y >= framebuffer_height_value
    ) {
        return;
    }

    if (
        width >
        framebuffer_width_value - x
    ) {
        width =
            framebuffer_width_value - x;
    }

    if (
        height >
        framebuffer_height_value - y
    ) {
        height =
            framebuffer_height_value - y;
    }

    int restore_cursor =
        mouse_cursor_visible;

    uint32_t saved_cursor_x =
        mouse_cursor_x;

    uint32_t saved_cursor_y =
        mouse_cursor_y;

    if (restore_cursor) {
        display_mouse_cursor_hide();
    }

    volatile uint32_t *framebuffer =
        framebuffer_pointer();

    uint32_t pixels_per_line =
        framebuffer_pixels_per_line();

    for (
        uint32_t row = 0;
        row < height;
        row++
    ) {
        volatile uint32_t *destination =
            framebuffer +
            (y + row) *
                pixels_per_line +
            x;

        uint32_t *source =
            framebuffer_buffer +
            (y + row) *
                DISPLAY_MAX_WIDTH +
            x;

        for (
            uint32_t column = 0;
            column < width;
            column++
        ) {
            destination[column] =
                source[column];
        }
    }

    if (restore_cursor) {
        display_mouse_cursor(
            saved_cursor_x,
            saved_cursor_y
        );
    }
}

void display_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
)
{
    if (!framebuffer_found) {
        return;
    }

    if (
        x >= framebuffer_width_value ||
        y >= framebuffer_height_value
    ) {
        return;
    }

    framebuffer_buffer[
        y * DISPLAY_MAX_WIDTH +
        x
    ] = color;
}

void display_rectangle(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
)
{
    if (!framebuffer_found) {
        return;
    }

    if (
        x >= framebuffer_width_value ||
        y >= framebuffer_height_value
    ) {
        return;
    }

    if (
        width >
        framebuffer_width_value - x
    ) {
        width =
            framebuffer_width_value - x;
    }

    if (
        height >
        framebuffer_height_value - y
    ) {
        height =
            framebuffer_height_value - y;
    }

    for (
        uint32_t row = 0;
        row < height;
        row++
    ) {
        uint32_t *line =
            framebuffer_buffer +
            (y + row) *
                DISPLAY_MAX_WIDTH +
            x;

        for (
            uint32_t column = 0;
            column < width;
            column++
        ) {
            line[column] = color;
        }
    }
}

void display_clear(
    uint32_t color
)
{
    if (!framebuffer_found) {
        return;
    }

    display_rectangle(
        0,
        0,
        framebuffer_width_value,
        framebuffer_height_value,
        color
    );
}

void display_text(
    uint32_t x,
    uint32_t y,
    const char *text,
    uint32_t color,
    uint32_t scale
)
{
    if (
        !framebuffer_found ||
        text == 0 ||
        scale == 0
    ) {
        return;
    }

    uint32_t original_x = x;

    while (*text != '\0') {
        uint8_t character =
            (uint8_t)*text;

        if (character == '\n') {
            x = original_x;
            y += 8 * scale;
            text++;
            continue;
        }

        if (
            character < 32 ||
            character > 126
        ) {
            x += 6 * scale;
            text++;
            continue;
        }

        const uint8_t *glyph =
            font[
                character - 32
            ];

        for (
            uint32_t column = 0;
            column < 5;
            column++
        ) {
            uint8_t bits =
                glyph[column];

            for (
                uint32_t row = 0;
                row < 7;
                row++
            ) {
                if (
                    bits &
                    (uint8_t)(1 << row)
                ) {
                    display_rectangle(
                        x +
                            column * scale,
                        y +
                            row * scale,
                        scale,
                        scale,
                        color
                    );
                }
            }
        }

        x += 6 * scale;
        text++;
    }
}

void display_mouse_cursor_hide(void)
{
    if (
        !mouse_saved ||
        !framebuffer_found
    ) {
        mouse_cursor_visible = 0;
        mouse_saved = 0;
        return;
    }

    volatile uint32_t *framebuffer =
        framebuffer_pointer();

    uint32_t pixels_per_line =
        framebuffer_pixels_per_line();

    for (
        uint32_t row = 0;
        row < 16;
        row++
    ) {
        for (
            uint32_t column = 0;
            column < 16;
            column++
        ) {
            uint32_t x =
                mouse_saved_x +
                column;

            uint32_t y =
                mouse_saved_y +
                row;

            if (
                x >= framebuffer_width_value ||
                y >= framebuffer_height_value
            ) {
                continue;
            }

            framebuffer[
                y * pixels_per_line +
                x
            ] =
                mouse_saved_pixels[
                    row
                ][column];
        }
    }

    mouse_saved = 0;
    mouse_cursor_visible = 0;
}

void display_mouse_cursor(
    uint32_t x,
    uint32_t y
)
{
    if (!framebuffer_found) {
        return;
    }

    if (
        x >= framebuffer_width_value ||
        y >= framebuffer_height_value
    ) {
        return;
    }

    if (mouse_saved) {
        display_mouse_cursor_hide();
    }

    volatile uint32_t *framebuffer =
        framebuffer_pointer();

    uint32_t pixels_per_line =
        framebuffer_pixels_per_line();

    for (
        uint32_t row = 0;
        row < 16;
        row++
    ) {
        for (
            uint32_t column = 0;
            column < 16;
            column++
        ) {
            uint32_t pixel_x =
                x + column;

            uint32_t pixel_y =
                y + row;

            if (
                pixel_x >= framebuffer_width_value ||
                pixel_y >= framebuffer_height_value
            ) {
                mouse_saved_pixels[
                    row
                ][column] = 0;

                continue;
            }

            mouse_saved_pixels[
                row
            ][column] =
                framebuffer[
                    pixel_y *
                        pixels_per_line +
                    pixel_x
                ];
        }
    }

    mouse_saved_x = x;
    mouse_saved_y = y;

    mouse_cursor_x = x;
    mouse_cursor_y = y;

    mouse_saved = 1;
    mouse_cursor_visible = 1;

    for (
        uint32_t row = 0;
        row < 16;
        row++
    ) {
        for (
            uint32_t column = 0;
            column < 16;
            column++
        ) {
            if (
                mouse_cursor[row] &
                (uint16_t)(
                    0x8000 >> column
                )
            ) {
                uint32_t pixel_x =
                    x + column;

                uint32_t pixel_y =
                    y + row;

                if (
                    pixel_x <
                        framebuffer_width_value &&
                    pixel_y <
                        framebuffer_height_value
                ) {
                    framebuffer[
                        pixel_y *
                            pixels_per_line +
                        pixel_x
                    ] =
                        0xFFFFFF;
                }
            }
        }
    }
}
