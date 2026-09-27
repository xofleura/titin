#include <stdint.h>
#include "window.h"
#include "../display/display.h"
#include "../console/console.h"

#define WINDOW_LIMIT 16
#define WINDOW_TITLE_HEIGHT 32
#define WINDOW_BORDER_SIZE 2

#define DRAG_OUTLINE_LIMIT 8192

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t color;
} drag_pixel;

static titin_window windows[
    WINDOW_LIMIT
];

static titin_window *window_order[
    WINDOW_LIMIT
];

static uint64_t windows_used;

static titin_window *focused_window;

static uint32_t drag_offset_x;
static uint32_t drag_offset_y;

static drag_pixel drag_pixels[
    DRAG_OUTLINE_LIMIT
];

static uint32_t drag_pixel_count;

static int drag_outline_visible;

static void window_copy_title(
    char *destination,
    const char *source
)
{
    uint64_t i = 0;

    if (source == 0) {
        destination[0] = '\0';
        return;
    }

    while (
        source[i] != '\0' &&
        i < WINDOW_TITLE_SIZE - 1
    ) {
        destination[i] =
            source[i];

        i++;
    }

    destination[i] = '\0';
}

static volatile uint32_t *window_framebuffer(void)
{
    return (volatile uint32_t *)(uintptr_t)
        display_address();
}

static uint32_t window_framebuffer_pitch(void)
{
    return display_pitch() / 4;
}

static void drag_outline_hide(void)
{
    if (
        !drag_outline_visible ||
        !display_available()
    ) {
        return;
    }

    volatile uint32_t *framebuffer =
        window_framebuffer();

    uint32_t pitch =
        window_framebuffer_pitch();

    for (
        uint32_t i = 0;
        i < drag_pixel_count;
        i++
    ) {
        framebuffer[
            drag_pixels[i].y *
                pitch +
            drag_pixels[i].x
        ] =
            drag_pixels[i].color;
    }

    drag_outline_visible = 0;
    drag_pixel_count = 0;
}

static void drag_outline_pixel(
    uint32_t x,
    uint32_t y
)
{
    if (
        x >= display_width() ||
        y >= display_height()
    ) {
        return;
    }

    if (
        drag_pixel_count >=
        DRAG_OUTLINE_LIMIT
    ) {
        return;
    }

    volatile uint32_t *framebuffer =
        window_framebuffer();

    uint32_t pitch =
        window_framebuffer_pitch();

    drag_pixels[
        drag_pixel_count
    ].x = x;

    drag_pixels[
        drag_pixel_count
    ].y = y;

    drag_pixels[
        drag_pixel_count
    ].color =
        framebuffer[
            y * pitch +
            x
        ];

    drag_pixel_count++;

    framebuffer[
        y * pitch +
        x
    ] = 0xA0A0A0;
}

static void drag_outline_horizontal(
    uint32_t x,
    uint32_t y,
    uint32_t width
)
{
    for (
        uint32_t i = 0;
        i < width;
        i++
    ) {
        drag_outline_pixel(
            x + i,
            y
        );
    }
}

static void drag_outline_vertical(
    uint32_t x,
    uint32_t y,
    uint32_t height
)
{
    for (
        uint32_t i = 0;
        i < height;
        i++
    ) {
        drag_outline_pixel(
            x,
            y + i
        );
    }
}

static void drag_outline_show(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height
)
{
    if (
        !display_available()
    ) {
        return;
    }

    drag_pixel_count = 0;

    if (
        width == 0 ||
        height == 0
    ) {
        return;
    }

    drag_outline_horizontal(
        x,
        y,
        width
    );

    if (height > 1) {
        drag_outline_horizontal(
            x,
            y + height - 1,
            width
        );
    }

    if (height > 2) {
        drag_outline_vertical(
            x,
            y + 1,
            height - 2
        );

        if (width > 1) {
            drag_outline_vertical(
                x + width - 1,
                y + 1,
                height - 2
            );
        }
    }

    if (
        width > 2 &&
        height > WINDOW_TITLE_HEIGHT + 1
    ) {
        drag_outline_horizontal(
            x + 1,
            y + WINDOW_TITLE_HEIGHT,
            width - 2
        );
    }

    drag_outline_visible = 1;
}

static void drag_outline_update(
    titin_window *window
)
{
    if (window == 0) {
        return;
    }

    drag_outline_hide();

    drag_outline_show(
        window->drag_x,
        window->drag_y,
        window->width,
        window->height
    );
}

static void window_raise(
    titin_window *window
)
{
    if (
        window == 0 ||
        !window->active
    ) {
        return;
    }

    uint64_t position = 0;

    for (
        uint64_t i = 0;
        i < windows_used;
        i++
    ) {
        if (
            window_order[i] ==
            window
        ) {
            position = i;
            break;
        }
    }

    if (
        position ==
        windows_used - 1
    ) {
        return;
    }

    for (
        uint64_t i = position;
        i + 1 < windows_used;
        i++
    ) {
        window_order[i] =
            window_order[i + 1];
    }

    window_order[
        windows_used - 1
    ] = window;
}

void window_system_start(void)
{
    windows_used = 0;
    focused_window = 0;
    drag_outline_visible = 0;
    drag_pixel_count = 0;

    for (
        uint64_t i = 0;
        i < WINDOW_LIMIT;
        i++
    ) {
        windows[i].active = 0;
        windows[i].dragging = 0;
        windows[i].title[0] = '\0';
        windows[i].drag_x = 0;
        windows[i].drag_y = 0;
        window_order[i] = 0;
    }
}

titin_window *window_create(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    const char *title
)
{
    if (
        windows_used >=
        WINDOW_LIMIT
    ) {
        return 0;
    }

    titin_window *window =
        &windows[windows_used];

    window->x = x;
    window->y = y;
    window->width = width;
    window->height = height;

    window->drag_x = x;
    window->drag_y = y;

    window->border_color =
        0x404040;

    window->title_color =
        0x303030;

    window->content_color =
        0x181818;

    window_copy_title(
        window->title,
        title
    );

    window->active = 1;
    window->dragging = 0;

    window_order[
        windows_used
    ] = window;

    windows_used++;

    focused_window =
        window;

    return window;
}

int window_close(
    titin_window *window
)
{
    if (
        window == 0 ||
        !window->active
    ) {
        return 0;
    }

    if (
        window->dragging
    ) {
        drag_outline_hide();
        window->dragging = 0;
    }

    uint64_t position =
        UINT64_MAX;

    for (
        uint64_t i = 0;
        i < windows_used;
        i++
    ) {
        if (
            window_order[i] ==
            window
        ) {
            position = i;
            break;
        }
    }

    if (
        position == UINT64_MAX
    ) {
        return 0;
    }

    for (
        uint64_t i = position;
        i + 1 < windows_used;
        i++
    ) {
        window_order[i] =
            window_order[i + 1];
    }

    windows_used--;

    window_order[
        windows_used
    ] = 0;

    window->active = 0;
    window->dragging = 0;

    if (
        focused_window ==
        window
    ) {
        focused_window = 0;

        if (
            windows_used > 0
        ) {
            focused_window =
                window_order[
                    windows_used - 1
                ];
        }
    }

    return 1;
}

void window_draw(
    titin_window *window
)
{
    if (
        window == 0 ||
        !window->active ||
        !display_available()
    ) {
        return;
    }

    uint32_t title_color =
        window->title_color;

    uint32_t border_color =
        window->border_color;

    if (
        window ==
        focused_window
    ) {
        title_color =
            0x505050;

        border_color =
            0x707070;
    }

    display_rectangle(
        window->x,
        window->y,
        window->width,
        window->height,
        border_color
    );

    if (
        window->width <=
            WINDOW_BORDER_SIZE * 2 ||
        window->height <=
            WINDOW_BORDER_SIZE * 2
    ) {
        return;
    }

    display_rectangle(
        window->x +
            WINDOW_BORDER_SIZE,
        window->y +
            WINDOW_BORDER_SIZE,
        window->width -
            WINDOW_BORDER_SIZE * 2,
        WINDOW_TITLE_HEIGHT,
        title_color
    );

    uint32_t content_x =
        window_content_x(window);

    uint32_t content_y =
        window_content_y(window);

    uint32_t content_width =
        window_content_width(window);

    uint32_t content_height =
        window_content_height(window);

    if (
        content_width > 0 &&
        content_height > 0
    ) {
        display_rectangle(
            content_x,
            content_y,
            content_width,
            content_height,
            window->content_color
        );
    }

    display_text(
        window->x + 12,
        window->y + 8,
        window->title,
        0xE0E0E0,
        2
    );
}

void window_draw_all(void)
{
    if (!display_available()) {
        return;
    }

    for (
        uint64_t i = 0;
        i < windows_used;
        i++
    ) {
        titin_window *window =
            window_order[i];

        if (window == 0) {
            continue;
        }

        window_draw(
            window
        );
    }
}

uint64_t window_count(void)
{
    return windows_used;
}

titin_window *window_get(
    uint64_t index
)
{
    if (
        index >=
        windows_used
    ) {
        return 0;
    }

    return window_order[index];
}

titin_window *window_at(
    uint32_t x,
    uint32_t y
)
{
    for (
        uint64_t i = windows_used;
        i > 0;
        i--
    ) {
        titin_window *window =
            window_order[i - 1];

        if (!window->active) {
            continue;
        }

        if (
            x >= window->x &&
            x <
                window->x +
                window->width &&
            y >= window->y &&
            y <
                window->y +
                window->height
        ) {
            return window;
        }
    }

    return 0;
}

int window_title_hit(
    titin_window *window,
    uint32_t x,
    uint32_t y
)
{
    if (
        window == 0 ||
        !window->active
    ) {
        return 0;
    }

    if (
        x < window->x ||
        x >=
            window->x +
            window->width
    ) {
        return 0;
    }

    if (
        y < window->y ||
        y >=
            window->y +
            WINDOW_BORDER_SIZE +
            WINDOW_TITLE_HEIGHT
    ) {
        return 0;
    }

    return 1;
}

void window_focus(
    titin_window *window
)
{
    if (
        window == 0 ||
        !window->active
    ) {
        return;
    }

    focused_window =
        window;

    window_raise(
        window
    );
}

void window_begin_drag(
    titin_window *window,
    uint32_t mouse_x,
    uint32_t mouse_y
)
{
    if (
        window == 0 ||
        !window->active
    ) {
        return;
    }

    window_focus(
        window
    );

    window->dragging = 1;

    window->drag_x =
        window->x;

    window->drag_y =
        window->y;

    drag_offset_x =
        mouse_x -
        window->x;

    drag_offset_y =
        mouse_y -
        window->y;

    drag_outline_show(
        window->drag_x,
        window->drag_y,
        window->width,
        window->height
    );
}

void window_drag(
    titin_window *window,
    uint32_t mouse_x,
    uint32_t mouse_y
)
{
    if (
        window == 0 ||
        !window->dragging
    ) {
        return;
    }

    if (
        mouse_x >=
        drag_offset_x
    ) {
        window->drag_x =
            mouse_x -
            drag_offset_x;
    } else {
        window->drag_x = 0;
    }

    if (
        mouse_y >=
        drag_offset_y
    ) {
        window->drag_y =
            mouse_y -
            drag_offset_y;
    } else {
        window->drag_y = 0;
    }

    if (display_available()) {
        if (
            window->width <
            display_width() &&
            window->drag_x +
                window->width >
            display_width()
        ) {
            window->drag_x =
                display_width() -
                window->width;
        }

        if (
            window->height <
            display_height() &&
            window->drag_y +
                window->height >
            display_height()
        ) {
            window->drag_y =
                display_height() -
                window->height;
        }
    }

    drag_outline_update(
        window
    );
}

void window_end_drag(
    titin_window *window
)
{
    if (window == 0) {
        return;
    }

    drag_outline_hide();

    window->x =
        window->drag_x;

    window->y =
        window->drag_y;

    window->dragging = 0;

    window_focus(
        window
    );

    console_follow_window(
        window
    );

    window_redraw();
}

uint32_t window_content_x(
    titin_window *window
)
{
    if (window == 0) {
        return 0;
    }

    return window->x +
        WINDOW_BORDER_SIZE;
}

uint32_t window_content_y(
    titin_window *window
)
{
    if (window == 0) {
        return 0;
    }

    return window->y +
        WINDOW_BORDER_SIZE +
        WINDOW_TITLE_HEIGHT;
}

uint32_t window_content_width(
    titin_window *window
)
{
    if (
        window == 0 ||
        window->width <=
            WINDOW_BORDER_SIZE * 2
    ) {
        return 0;
    }

    return window->width -
        WINDOW_BORDER_SIZE * 2;
}

uint32_t window_content_height(
    titin_window *window
)
{
    if (
        window == 0 ||
        window->height <=
            WINDOW_BORDER_SIZE * 2 +
            WINDOW_TITLE_HEIGHT
    ) {
        return 0;
    }

    return window->height -
        WINDOW_BORDER_SIZE * 2 -
        WINDOW_TITLE_HEIGHT;
}

void window_redraw(void)
{
    if (!display_available()) {
        return;
    }

    display_begin_frame();

    display_clear(
        0x202020
    );

    uint64_t count =
        window_count();

    for (
        uint64_t i = 0;
        i < count;
        i++
    ) {
        titin_window *window =
            window_get(i);

        if (
            window == 0 ||
            !window->active
        ) {
            continue;
        }

        window_draw(
            window
        );

        console_render_window(
            window
        );
    }

    console_render();

    display_present();
}
