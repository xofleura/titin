#include <stdint.h>
#include "../console/console.h"
#include "../input/keyboard.h"
#include "../filesystem/tfs.h"
#include "editor.h"

#define EDITOR_BUFFER_SIZE TFS_DATA_SIZE
#define EDITOR_WIDTH 80

#define EDITOR_TEXT_FIRST 3
#define EDITOR_TEXT_LAST 22
#define EDITOR_STATUS 23
#define EDITOR_FOOTER 24

#define EDITOR_TEXT_X 7
#define EDITOR_TEXT_WIDTH 71

static uint8_t buffer[EDITOR_BUFFER_SIZE];
static uint64_t length = 0;
static uint64_t cursor = 0;
static uint8_t modified = 0;

static uint64_t editor_line_start(
    uint64_t position
)
{
    while (
        position > 0 &&
        buffer[position - 1] != '\n'
    ) {
        position--;
    }

    return position;
}

static uint64_t editor_line_end(
    uint64_t position
)
{
    while (
        position < length &&
        buffer[position] != '\n'
    ) {
        position++;
    }

    return position;
}

static uint64_t editor_column(
    uint64_t position
)
{
    return position -
        editor_line_start(position);
}

static uint64_t editor_visual_column(
    uint64_t position
)
{
    uint64_t start =
        editor_line_start(position);

    uint64_t column = 0;

    while (start < position) {
        if (buffer[start] == '\t') {
            column += 4;
        } else {
            column++;
        }

        start++;
    }

    return column;
}

static uint64_t editor_line_number(
    uint64_t position
)
{
    uint64_t line = 1;

    for (
        uint64_t i = 0;
        i < position;
        i++
    ) {
        if (buffer[i] == '\n') {
            line++;
        }
    }

    return line;
}

static uint64_t editor_line_count(void)
{
    uint64_t count = 1;

    for (
        uint64_t i = 0;
        i < length;
        i++
    ) {
        if (buffer[i] == '\n') {
            count++;
        }
    }

    return count;
}

static uint64_t editor_position_for_line(
    uint64_t line
)
{
    uint64_t current = 1;
    uint64_t position = 0;

    while (
        position < length &&
        current < line
    ) {
        if (buffer[position] == '\n') {
            current++;
        }

        position++;
    }

    return position;
}

static void editor_clear_buffer(void)
{
    for (
        uint64_t i = 0;
        i < EDITOR_BUFFER_SIZE;
        i++
    ) {
        buffer[i] = 0;
    }

    length = 0;
    cursor = 0;
    modified = 0;
}

static void editor_load(
    uint64_t file
)
{
    editor_clear_buffer();

    length = tfs_read(
        file,
        buffer,
        EDITOR_BUFFER_SIZE
    );

    if (cursor > length) {
        cursor = length;
    }

    modified = 0;
}

static void editor_put(
    uint8_t x,
    uint8_t y,
    char c
)
{
    console_write_at(
        x,
        y,
        c
    );
}

static void editor_text(
    uint8_t x,
    uint8_t y,
    const char *text
)
{
    while (
        *text != '\0' &&
        x < EDITOR_WIDTH
    ) {
        editor_put(
            x,
            y,
            *text
        );

        x++;
        text++;
    }
}

static void editor_number(
    uint8_t x,
    uint8_t y,
    uint64_t value,
    uint8_t width
)
{
    char digits[20];
    uint64_t count = 0;

    if (value == 0) {
        digits[0] = '0';
        count = 1;
    } else {
        while (value > 0) {
            digits[count] =
                '0' +
                (char)(value % 10);

            value /= 10;
            count++;
        }
    }

    while (count < width) {
        editor_put(
            x,
            y,
            ' '
        );

        x++;
        width--;
    }

    while (count > 0) {
        count--;

        editor_put(
            x,
            y,
            digits[count]
        );

        x++;
    }
}

static void editor_fill_row(
    uint8_t y,
    char character
)
{
    console_fill_at(
        0,
        y,
        EDITOR_WIDTH,
        1,
        character
    );
}

static void editor_draw_border(
    uint8_t y
)
{
    editor_fill_row(
        y,
        ' '
    );

    editor_put(
        0,
        y,
        '+'
    );

    for (
        uint8_t x = 1;
        x < EDITOR_WIDTH - 1;
        x++
    ) {
        editor_put(
            x,
            y,
            '-'
        );
    }

    editor_put(
        EDITOR_WIDTH - 1,
        y,
        '+'
    );
}

static void editor_draw_header(
    const char *path
)
{
    editor_fill_row(
        0,
        ' '
    );

    editor_put(
        0,
        0,
        '+'
    );

    for (
        uint8_t x = 1;
        x < EDITOR_WIDTH - 1;
        x++
    ) {
        editor_put(
            x,
            0,
            '-'
        );
    }

    editor_put(
        EDITOR_WIDTH - 1,
        0,
        '+'
    );

    editor_fill_row(
        1,
        ' '
    );

    editor_put(
        0,
        1,
        '|'
    );

    editor_text(
        2,
        1,
        "zm"
    );

    editor_put(
        5,
        1,
        ' '
    );

    uint8_t x = 6;

    while (
        *path != '\0' &&
        x < EDITOR_WIDTH - 1
    ) {
        editor_put(
            x,
            1,
            *path
        );

        x++;
        path++;
    }

    editor_put(
        EDITOR_WIDTH - 1,
        1,
        '|'
    );

    editor_draw_border(2);
}

static void editor_draw_line(
    uint64_t line,
    uint8_t screen_y
)
{
    editor_fill_row(
        screen_y,
        ' '
    );

    editor_put(
        0,
        screen_y,
        '|'
    );

    editor_number(
        2,
        screen_y,
        line,
        3
    );

    editor_put(
        5,
        screen_y,
        '|'
    );

    editor_put(
        6,
        screen_y,
        ' '
    );

    uint64_t position =
        editor_position_for_line(line);

    uint64_t end =
        editor_line_end(position);

    uint64_t visual = 0;

    while (
        position < end &&
        visual < EDITOR_TEXT_WIDTH
    ) {
        char character =
            (char)buffer[position];

        if (character == '\t') {
            for (
                uint64_t i = 0;
                i < 4 &&
                visual < EDITOR_TEXT_WIDTH;
                i++
            ) {
                editor_put(
                    EDITOR_TEXT_X +
                    visual,
                    screen_y,
                    ' '
                );

                visual++;
            }
        } else {
            editor_put(
                EDITOR_TEXT_X +
                visual,
                screen_y,
                character
            );

            visual++;
        }

        position++;
    }

    editor_put(
        EDITOR_WIDTH - 1,
        screen_y,
        '|'
    );
}

static uint64_t editor_display_start(void)
{
    uint64_t line =
        editor_line_number(cursor);

    uint64_t visible =
        EDITOR_TEXT_LAST -
        EDITOR_TEXT_FIRST +
        1;

    if (line <= visible) {
        return 1;
    }

    return line -
        visible +
        1;
}

static void editor_draw_status(void)
{
    editor_draw_border(
        EDITOR_STATUS
    );

    editor_fill_row(
        EDITOR_FOOTER,
        ' '
    );

    editor_put(
        0,
        EDITOR_FOOTER,
        '|'
    );

    editor_number(
        2,
        EDITOR_FOOTER,
        editor_line_number(cursor),
        3
    );

    editor_put(
        5,
        EDITOR_FOOTER,
        ':'
    );

    editor_number(
        6,
        EDITOR_FOOTER,
        editor_column(cursor) + 1,
        3
    );

    editor_text(
        11,
        EDITOR_FOOTER,
        "  "
    );

    editor_number(
        13,
        EDITOR_FOOTER,
        length,
        4
    );

    editor_text(
        18,
        EDITOR_FOOTER,
        " bytes"
    );

    if (modified) {
        editor_text(
            65,
            EDITOR_FOOTER,
            "[MODIFIED]"
        );
    } else {
        editor_text(
            67,
            EDITOR_FOOTER,
            "[SAVED]"
        );
    }

    editor_put(
        EDITOR_WIDTH - 1,
        EDITOR_FOOTER,
        '|'
    );
}

static void editor_set_cursor(void)
{
    uint64_t display_start =
        editor_display_start();

    uint64_t line =
        editor_line_number(cursor);

    uint64_t visual_column =
        editor_visual_column(cursor);

    uint64_t x =
        EDITOR_TEXT_X +
        visual_column;

    uint64_t y =
        EDITOR_TEXT_FIRST +
        line -
        display_start;

    if (x >= EDITOR_WIDTH - 1) {
        x = EDITOR_WIDTH - 2;
    }

    if (y < EDITOR_TEXT_FIRST) {
        y = EDITOR_TEXT_FIRST;
    }

    if (y > EDITOR_TEXT_LAST) {
        y = EDITOR_TEXT_LAST;
    }

    console_cursor_set(
        (uint8_t)x,
        (uint8_t)y
    );
}

static void editor_render(void)
{
    editor_draw_header(
        tfs_current_path()
    );

    uint64_t display_start =
        editor_display_start();

    uint64_t total_lines =
        editor_line_count();

    for (
        uint8_t screen_y =
            EDITOR_TEXT_FIRST;
        screen_y <= EDITOR_TEXT_LAST;
        screen_y++
    ) {
        uint64_t line =
            display_start +
            screen_y -
            EDITOR_TEXT_FIRST;

        if (line <= total_lines) {
            editor_draw_line(
                line,
                screen_y
            );
        } else {
            editor_fill_row(
                screen_y,
                ' '
            );

            editor_put(
                0,
                screen_y,
                '|'
            );

            editor_put(
                5,
                screen_y,
                '|'
            );

            editor_put(
                EDITOR_WIDTH - 1,
                screen_y,
                '|'
            );
        }
    }

    editor_draw_status();

    editor_set_cursor();
}

static void editor_insert(
    char character
)
{
    if (
        length >=
        EDITOR_BUFFER_SIZE
    ) {
        return;
    }

    for (
        uint64_t i = length;
        i > cursor;
        i--
    ) {
        buffer[i] =
            buffer[i - 1];
    }

    buffer[cursor] =
        (uint8_t)character;

    length++;
    cursor++;
    modified = 1;
}

static void editor_backspace(void)
{
    if (cursor == 0) {
        return;
    }

    for (
        uint64_t i = cursor;
        i < length;
        i++
    ) {
        buffer[i - 1] =
            buffer[i];
    }

    length--;
    cursor--;
    modified = 1;
}

static void editor_move_left(void)
{
    if (cursor > 0) {
        cursor--;
    }
}

static void editor_move_right(void)
{
    if (cursor < length) {
        cursor++;
    }
}

static void editor_move_up(void)
{
    uint64_t column =
        editor_column(cursor);

    uint64_t current_start =
        editor_line_start(cursor);

    if (current_start == 0) {
        return;
    }

    uint64_t previous_end =
        current_start - 1;

    uint64_t previous_start =
        editor_line_start(
            previous_end
        );

    uint64_t previous_length =
        previous_end -
        previous_start;

    if (column > previous_length) {
        column = previous_length;
    }

    cursor =
        previous_start +
        column;
}

static void editor_move_down(void)
{
    uint64_t column =
        editor_column(cursor);

    uint64_t current_end =
        editor_line_end(cursor);

    if (current_end >= length) {
        return;
    }

    uint64_t next_start =
        current_end + 1;

    uint64_t next_end =
        editor_line_end(
            next_start
        );

    uint64_t next_length =
        next_end -
        next_start;

    if (column > next_length) {
        column = next_length;
    }

    cursor =
        next_start +
        column;
}

static int editor_save(
    uint64_t file
)
{
    if (!tfs_write(
            file,
            buffer,
            length
        )) {
        return 0;
    }

    modified = 0;

    return 1;
}

static uint64_t editor_find_file(
    const char *path
)
{
    if (path[0] == '>') {
        return tfs_find_path(path);
    }

    return tfs_find(
        tfs_current(),
        path
    );
}

static uint64_t editor_create_file(
    const char *path
)
{
    if (path[0] == '>') {
        return tfs_create_path(
            path,
            TFS_DATA
        );
    }

    return tfs_create(
        tfs_current(),
        path,
        TFS_DATA
    );
}

void editor_open(
    const char *path
)
{
    if (
        path == 0 ||
        path[0] == '\0'
    ) {
        return;
    }

    uint64_t file =
        editor_find_file(path);

    if (file == UINT64_MAX) {
        file =
            editor_create_file(path);

        if (file == UINT64_MAX) {
            console_clear();

            editor_text(
                0,
                0,
                "Unable to open"
            );

            console_cursor_set(
                0,
                1
            );

            return;
        }
    }

    const tfs_object *object =
        tfs_object_get(file);

    if (
        object == 0 ||
        object->type != TFS_DATA
    ) {
        console_clear();

        editor_text(
            0,
            0,
            "Not a file"
        );

        console_cursor_set(
            0,
            1
        );

        return;
    }

    editor_load(file);

    console_clear();

    editor_render();

    for (;;) {
        char key =
            keyboard_read();

        if (key == 19) {
            editor_save(file);
            editor_render();
            continue;
        }

        if (key == 17) {
            console_clear();
            return;
        }

        if (key == KEY_LEFT) {
            editor_move_left();
            editor_render();
            continue;
        }

        if (key == KEY_RIGHT) {
            editor_move_right();
            editor_render();
            continue;
        }

        if (key == KEY_UP) {
            editor_move_up();
            editor_render();
            continue;
        }

        if (key == KEY_DOWN) {
            editor_move_down();
            editor_render();
            continue;
        }

        if (key == '\b') {
            editor_backspace();
            editor_render();
            continue;
        }

        if (key == '\n') {
            editor_insert('\n');
            editor_render();
            continue;
        }

        if (key == KEY_TAB) {
            editor_insert('\t');
            editor_render();
            continue;
        }

        if (
            (uint8_t)key >= 32 &&
            (uint8_t)key <= 126
        ) {
            editor_insert(key);
            editor_render();
        }
    }
}
