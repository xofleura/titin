#include <stdint.h>
#include "console.h"
#include "../display/display.h"
#include "../window/window.h"

#define TERMINAL_LIMIT 8

#define TERMINAL_COLUMNS 160
#define TERMINAL_ROWS 60

#define TERMINAL_CHARACTER_WIDTH 12
#define TERMINAL_CHARACTER_HEIGHT 16
#define TERMINAL_TEXT_SCALE 2

typedef struct {
    char character;
} terminal_cell;

typedef struct {
    uint8_t cursor_x;
    uint8_t cursor_y;

    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;

    uint32_t columns;
    uint32_t rows;

    titin_window *window;

    terminal_cell cells[
        TERMINAL_COLUMNS *
        TERMINAL_ROWS
    ];
} titin_terminal;

static titin_terminal terminals[
    TERMINAL_LIMIT
];

static uint64_t terminal_count;
static uint64_t active_terminal;
static int fullscreen_visible;

static void console_initialize_terminal(
    titin_terminal *terminal
)
{
    if (terminal == 0) {
        return;
    }

    terminal->cursor_x = 0;
    terminal->cursor_y = 0;

    terminal->x = 0;
    terminal->y = 0;
    terminal->width = 0;
    terminal->height = 0;

    terminal->columns =
        TERMINAL_COLUMNS;

    terminal->rows =
        TERMINAL_ROWS;

    terminal->window = 0;

    for (
        uint32_t y = 0;
        y < TERMINAL_ROWS;
        y++
    ) {
        for (
            uint32_t x = 0;
            x < TERMINAL_COLUMNS;
            x++
        ) {
            uint64_t index =
                (uint64_t)y *
                TERMINAL_COLUMNS +
                x;

            terminal->cells[index]
                .character = ' ';
        }
    }
}

static void console_copy_terminal(
    titin_terminal *destination,
    const titin_terminal *source
)
{
    if (
        destination == 0 ||
        source == 0
    ) {
        return;
    }

    destination->cursor_x =
        source->cursor_x;

    destination->cursor_y =
        source->cursor_y;

    destination->x =
        source->x;

    destination->y =
        source->y;

    destination->width =
        source->width;

    destination->height =
        source->height;

    destination->columns =
        source->columns;

    destination->rows =
        source->rows;

    destination->window =
        source->window;

    for (
        uint32_t y = 0;
        y < TERMINAL_ROWS;
        y++
    ) {
        for (
            uint32_t x = 0;
            x < TERMINAL_COLUMNS;
            x++
        ) {
            uint64_t index =
                (uint64_t)y *
                TERMINAL_COLUMNS +
                x;

            destination->cells[index] =
                source->cells[index];
        }
    }
}

static void console_bind_surface(
    titin_terminal *terminal
)
{
    if (terminal == 0) {
        return;
    }

    if (terminal->window == 0) {
        terminal->x = 0;
        terminal->y = 0;

        terminal->width =
            display_width();

        terminal->height =
            display_height();

        terminal->columns =
            terminal->width /
            TERMINAL_CHARACTER_WIDTH;

        terminal->rows =
            terminal->height /
            TERMINAL_CHARACTER_HEIGHT;
    } else {
        titin_window *window =
            terminal->window;

        terminal->x =
            window_content_x(
                window
            );

        terminal->y =
            window_content_y(
                window
            );

        terminal->width =
            window_content_width(
                window
            );

        terminal->height =
            window_content_height(
                window
            );

        terminal->columns =
            terminal->width /
            TERMINAL_CHARACTER_WIDTH;

        terminal->rows =
            terminal->height /
            TERMINAL_CHARACTER_HEIGHT;
    }

    if (
        terminal->columns >
        TERMINAL_COLUMNS
    ) {
        terminal->columns =
            TERMINAL_COLUMNS;
    }

    if (
        terminal->rows >
        TERMINAL_ROWS
    ) {
        terminal->rows =
            TERMINAL_ROWS;
    }

    if (
        terminal->columns == 0
    ) {
        terminal->columns = 1;
    }

    if (
        terminal->rows == 0
    ) {
        terminal->rows = 1;
    }

    if (
        terminal->cursor_x >=
        terminal->columns
    ) {
        terminal->cursor_x =
            (uint8_t)(
                terminal->columns - 1
            );
    }

    if (
        terminal->cursor_y >=
        terminal->rows
    ) {
        terminal->cursor_y =
            (uint8_t)(
                terminal->rows - 1
            );
    }
}

static titin_terminal *console_find_window(
    titin_window *window
)
{
    if (window == 0) {
        return 0;
    }

    for (
        uint64_t i = 0;
        i < terminal_count;
        i++
    ) {
        if (
            terminals[i].window ==
            window
        ) {
            return &terminals[i];
        }
    }

    return 0;
}

static void console_scroll(
    titin_terminal *terminal
)
{
    if (
        terminal == 0 ||
        terminal->rows == 0
    ) {
        return;
    }

    for (
        uint32_t y = 1;
        y < terminal->rows;
        y++
    ) {
        for (
            uint32_t x = 0;
            x < terminal->columns;
            x++
        ) {
            uint64_t from =
                (uint64_t)y *
                TERMINAL_COLUMNS +
                x;

            uint64_t to =
                (uint64_t)(y - 1) *
                TERMINAL_COLUMNS +
                x;

            terminal->cells[to] =
                terminal->cells[from];
        }
    }

    uint32_t last_row =
        terminal->rows - 1;

    for (
        uint32_t x = 0;
        x < terminal->columns;
        x++
    ) {
        uint64_t index =
            (uint64_t)last_row *
            TERMINAL_COLUMNS +
            x;

        terminal->cells[index]
            .character = ' ';
    }

    terminal->cursor_y =
        (uint8_t)last_row;
}

void console_start(void)
{
    terminal_count = 1;
    active_terminal = 0;
    fullscreen_visible = 1;

    console_initialize_terminal(
        &terminals[0]
    );

    console_bind_fullscreen();

    console_put_char('T');
    console_put_char('i');
    console_put_char('t');
    console_put_char('i');
    console_put_char('n');
    console_put_char(' ');
    console_put_char('0');
    console_put_char('.');
    console_put_char('5');
    console_newline();
}

void console_bind_window(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height
)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    terminal->window = 0;

    terminal->x = x;
    terminal->y = y;
    terminal->width = width;
    terminal->height = height;

    terminal->columns =
        width /
        TERMINAL_CHARACTER_WIDTH;

    terminal->rows =
        height /
        TERMINAL_CHARACTER_HEIGHT;

    if (
        terminal->columns >
        TERMINAL_COLUMNS
    ) {
        terminal->columns =
            TERMINAL_COLUMNS;
    }

    if (
        terminal->rows >
        TERMINAL_ROWS
    ) {
        terminal->rows =
            TERMINAL_ROWS;
    }

    if (
        terminal->columns == 0
    ) {
        terminal->columns = 1;
    }

    if (
        terminal->rows == 0
    ) {
        terminal->rows = 1;
    }
}

void console_bind_fullscreen(void)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    terminal->window = 0;

    terminal->x = 0;
    terminal->y = 0;

    terminal->width =
        display_width();

    terminal->height =
        display_height();

    terminal->columns =
        terminal->width /
        TERMINAL_CHARACTER_WIDTH;

    terminal->rows =
        terminal->height /
        TERMINAL_CHARACTER_HEIGHT;

    if (
        terminal->columns >
        TERMINAL_COLUMNS
    ) {
        terminal->columns =
            TERMINAL_COLUMNS;
    }

    if (
        terminal->rows >
        TERMINAL_ROWS
    ) {
        terminal->rows =
            TERMINAL_ROWS;
    }

    if (
        terminal->columns == 0
    ) {
        terminal->columns = 1;
    }

    if (
        terminal->rows == 0
    ) {
        terminal->rows = 1;
    }
}

int console_create_window(
    struct titin_window *window
)
{
    if (
        window == 0 ||
        terminal_count >=
        TERMINAL_LIMIT
    ) {
        return 0;
    }

    titin_terminal *terminal =
        &terminals[terminal_count];

    console_initialize_terminal(
        terminal
    );

    terminal->window =
        window;

    active_terminal =
        terminal_count;

    terminal_count++;

    fullscreen_visible = 0;

    console_bind_surface(
        terminal
    );

    return 1;
}

int console_close_window(
    struct titin_window *window
)
{
    if (window == 0) {
        return 0;
    }

    uint64_t index = UINT64_MAX;

    for (
        uint64_t i = 1;
        i < terminal_count;
        i++
    ) {
        if (
            terminals[i].window ==
            window
        ) {
            index = i;
            break;
        }
    }

    if (
        index == UINT64_MAX
    ) {
        return 0;
    }

    for (
        uint64_t i = index;
        i + 1 < terminal_count;
        i++
    ) {
        console_copy_terminal(
            &terminals[i],
            &terminals[i + 1]
        );
    }

    terminal_count--;

    if (
        terminal_count == 1
    ) {
        active_terminal = 0;
        fullscreen_visible = 0;
        return 1;
    }

    if (
        active_terminal > index
    ) {
        active_terminal--;
    } else if (
        active_terminal == index
    ) {
        if (
            active_terminal >=
            terminal_count
        ) {
            active_terminal =
                terminal_count - 1;
        }
    }

    fullscreen_visible = 0;

    console_bind_surface(
        &terminals[active_terminal]
    );

    return 1;
}

int console_focus_window(
    struct titin_window *window
)
{
    if (window == 0) {
        return 0;
    }

    titin_terminal *terminal =
        console_find_window(
            window
        );

    if (terminal == 0) {
        return 0;
    }

    active_terminal =
        (uint64_t)(
            terminal -
            terminals
        );

    fullscreen_visible = 0;

    console_bind_surface(
        terminal
    );

    return 1;
}

int console_window_is_terminal(
    struct titin_window *window
)
{
    if (window == 0) {
        return 0;
    }

    return console_find_window(
        window
    ) != 0;
}

struct titin_window *console_active_window(void)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return 0;
    }

    return terminals[
        active_terminal
    ].window;
}

void console_follow_window(
    struct titin_window *window
)
{
    if (window == 0) {
        return;
    }

    titin_terminal *terminal =
        console_find_window(
            window
        );

    if (terminal == 0) {
        return;
    }

    console_bind_surface(
        terminal
    );
}

static void console_render_terminal(
    titin_terminal *terminal
)
{
    if (
        terminal == 0 ||
        !display_available()
    ) {
        return;
    }

    console_bind_surface(
        terminal
    );

    uint32_t x =
        terminal->x;

    uint32_t y =
        terminal->y;

    uint32_t columns =
        terminal->columns;

    uint32_t rows =
        terminal->rows;

    if (
        columns == 0 ||
        rows == 0
    ) {
        return;
    }

    for (
        uint32_t row = 0;
        row < rows;
        row++
    ) {
        for (
            uint32_t column = 0;
            column < columns;
            column++
        ) {
            uint64_t index =
                (uint64_t)row *
                TERMINAL_COLUMNS +
                column;

            char character =
                terminal->cells[index]
                    .character;

            if (
                character == '\0'
            ) {
                character = ' ';
            }

            char text[2];

            text[0] = character;
            text[1] = '\0';

            display_text(
                x +
                    column *
                    TERMINAL_CHARACTER_WIDTH,
                y +
                    row *
                    TERMINAL_CHARACTER_HEIGHT,
                text,
                0xD0D0D0,
                TERMINAL_TEXT_SCALE
            );
        }
    }

    if (
        terminal ==
        &terminals[active_terminal]
    ) {
        if (
            terminal->cursor_x <
            columns &&
            terminal->cursor_y <
            rows
        ) {
            display_rectangle(
                x +
                    terminal->cursor_x *
                    TERMINAL_CHARACTER_WIDTH +
                    2,
                y +
                    terminal->cursor_y *
                    TERMINAL_CHARACTER_HEIGHT +
                    TERMINAL_CHARACTER_HEIGHT -
                    2,
                TERMINAL_CHARACTER_WIDTH - 2,
                2,
                0xE0E0E0
            );
        }
    }
}

void console_render_window(
    struct titin_window *window
)
{
    if (window == 0) {
        return;
    }

    titin_terminal *terminal =
        console_find_window(
            window
        );

    if (terminal == 0) {
        return;
    }

    console_bind_surface(
        terminal
    );

    console_render_terminal(
        terminal
    );
}

void console_render(void)
{
    if (!fullscreen_visible) {
        return;
    }

    for (
        uint64_t i = 0;
        i < terminal_count;
        i++
    ) {
        titin_terminal *terminal =
            &terminals[i];

        if (
            terminal->window != 0
        ) {
            continue;
        }

        console_render_terminal(
            terminal
        );
    }
}

void console_clear(void)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    for (
        uint32_t y = 0;
        y < TERMINAL_ROWS;
        y++
    ) {
        for (
            uint32_t x = 0;
            x < TERMINAL_COLUMNS;
            x++
        ) {
            uint64_t index =
                (uint64_t)y *
                TERMINAL_COLUMNS +
                x;

            terminal->cells[index]
                .character = ' ';
        }
    }

    terminal->cursor_x = 0;
    terminal->cursor_y = 0;
}

void console_put_char(
    char c
)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    console_bind_surface(
        terminal
    );

    if (c == '\n') {
        console_newline();
        return;
    }

    if (c == '\r') {
        terminal->cursor_x = 0;
        return;
    }

    if (c == '\b') {
        console_backspace();
        return;
    }

    if (
        terminal->cursor_x >=
        terminal->columns
    ) {
        console_newline();
    }

    uint64_t index =
        (uint64_t)
            terminal->cursor_y *
        TERMINAL_COLUMNS +
        terminal->cursor_x;

    terminal->cells[index]
        .character = c;

    terminal->cursor_x++;

    if (
        terminal->cursor_x >=
        terminal->columns
    ) {
        console_newline();
    }
}

void console_backspace(void)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    console_bind_surface(
        terminal
    );

    if (
        terminal->cursor_x == 0
    ) {
        return;
    }

    terminal->cursor_x--;

    uint64_t index =
        (uint64_t)
            terminal->cursor_y *
        TERMINAL_COLUMNS +
        terminal->cursor_x;

    terminal->cells[index]
        .character = ' ';
}

void console_newline(void)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    console_bind_surface(
        terminal
    );

    terminal->cursor_x = 0;
    terminal->cursor_y++;

    if (
        terminal->cursor_y >=
        terminal->rows
    ) {
        console_scroll(
            terminal
        );
    }
}

void console_prompt(void)
{
    console_put_char('-');
    console_put_char(' ');
}

uint8_t console_cursor_x(void)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return 0;
    }

    return terminals[
        active_terminal
    ].cursor_x;
}

uint8_t console_cursor_y(void)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return 0;
    }

    return terminals[
        active_terminal
    ].cursor_y;
}

void console_cursor_set(
    uint8_t x,
    uint8_t y
)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    console_bind_surface(
        terminal
    );

    if (
        x < terminal->columns
    ) {
        terminal->cursor_x = x;
    }

    if (
        y < terminal->rows
    ) {
        terminal->cursor_y = y;
    }
}

void console_write_at(
    uint8_t x,
    uint8_t y,
    char c
)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    console_bind_surface(
        terminal
    );

    if (
        x >= terminal->columns ||
        y >= terminal->rows
    ) {
        return;
    }

    uint64_t index =
        (uint64_t)y *
        TERMINAL_COLUMNS +
        x;

    terminal->cells[index]
        .character = c;
}

void console_fill_at(
    uint8_t x,
    uint8_t y,
    uint8_t width,
    uint8_t height,
    char c
)
{
    if (
        active_terminal >=
        terminal_count
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    console_bind_surface(
        terminal
    );

    for (
        uint32_t row = 0;
        row < height;
        row++
    ) {
        uint32_t current_y =
            (uint32_t)y + row;

        if (
            current_y >=
            terminal->rows
        ) {
            break;
        }

        for (
            uint32_t column = 0;
            column < width;
            column++
        ) {
            uint32_t current_x =
                (uint32_t)x +
                column;

            if (
                current_x >=
                terminal->columns
            ) {
                break;
            }

            uint64_t index =
                (uint64_t)current_y *
                TERMINAL_COLUMNS +
                current_x;

            terminal->cells[index]
                .character = c;
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
    if (
        active_terminal >=
        terminal_count ||
        text == 0
    ) {
        return;
    }

    titin_terminal *terminal =
        &terminals[active_terminal];

    console_bind_surface(
        terminal
    );

    if (
        y >= terminal->rows
    ) {
        return;
    }

    uint32_t position =
        x;

    while (
        *text != '\0' &&
        position <
            terminal->columns
    ) {
        uint64_t index =
            (uint64_t)y *
            TERMINAL_COLUMNS +
            position;

        terminal->cells[index]
            .character = *text;

        position++;
        text++;
    }

    while (
        position <
        terminal->columns
    ) {
        uint64_t index =
            (uint64_t)y *
            TERMINAL_COLUMNS +
            position;

        terminal->cells[index]
            .character = ' ';

        position++;
    }

    terminal->cursor_x =
        (uint8_t)(
            (uint32_t)x +
            cursor
        );

    terminal->cursor_y =
        y;

    if (
        terminal->cursor_x >=
        terminal->columns
    ) {
        terminal->cursor_x =
            (uint8_t)(
                terminal->columns - 1
            );
    }
}
