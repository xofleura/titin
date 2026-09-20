#include <stdint.h>
#include "../console/console.h"
#include "../input/keyboard.h"
#include "../input/line.h"
#include "../protein/protein.h"
#include "../origin/origin.h"
#include "../task/task.h"
#include "../filesystem/tfs.h"

#define HISTORY_SIZE 16
#define COMPLETION_SIZE 128
#define USERNAME_SIZE 32

static char history[HISTORY_SIZE][128];
static uint64_t history_count = 0;
static int64_t history_position = -1;

static uint8_t line_x = 0;
static uint8_t line_y = 0;

static void boot_write(const char *text)
{
    while (*text != '\0') {
        console_put_char(*text);
        text++;
    }
}

static void boot_status(const char *name)
{
    boot_write(name);
    boot_write(" ........ online\n");
}

static void copy_text(
    char *destination,
    const char *source
)
{
    uint64_t i = 0;

    while (
        source[i] != '\0' &&
        i < 127
    ) {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

static void history_add(
    const char *line
)
{
    if (line[0] == '\0') {
        return;
    }

    if (history_count < HISTORY_SIZE) {
        copy_text(
            history[history_count],
            line
        );

        history_count++;

        return;
    }

    for (uint64_t i = 1;
         i < HISTORY_SIZE;
         i++) {
        copy_text(
            history[i - 1],
            history[i]
        );
    }

    copy_text(
        history[HISTORY_SIZE - 1],
        line
    );
}

static void line_redraw(void)
{
    console_redraw_line(
        line_x,
        line_y,
        input_line_get(),
        input_line_cursor()
    );
}

static void line_replace(
    const char *text
)
{
    input_line_clear();

    for (uint64_t i = 0;
         text[i] != '\0';
         i++) {
        input_line_add(text[i]);
    }

    line_redraw();
}

static void history_up(void)
{
    if (history_count == 0) {
        return;
    }

    if (history_position < 0) {
        history_position =
            (int64_t)history_count - 1;
    } else if (
        history_position > 0
    ) {
        history_position--;
    }

    line_replace(
        history[history_position]
    );
}

static void history_down(void)
{
    if (
        history_count == 0 ||
        history_position < 0
    ) {
        return;
    }

    if (
        history_position <
        (int64_t)history_count - 1
    ) {
        history_position++;

        line_replace(
            history[history_position]
        );

        return;
    }

    history_position = -1;
    line_replace("");
}

static void command_complete(void)
{
    char result[COMPLETION_SIZE];

    if (input_line_length() == 0) {
        return;
    }

    if (!protein_complete(
            input_line_get(),
            result,
            COMPLETION_SIZE)) {
        return;
    }

    line_replace(result);
}

static int username_valid(
    const char *username
)
{
    uint64_t length = 0;

    while (username[length] != '\0') {
        if (length >= USERNAME_SIZE - 1) {
            return 0;
        }

        if (
            username[length] == ' ' ||
            username[length] == '\t' ||
            username[length] == '>' ||
            username[length] == '/'
        ) {
            return 0;
        }

        length++;
    }

    return length != 0;
}

static void username_setup(void)
{
    char username[USERNAME_SIZE];

    for (;;) {
        console_newline();

        boot_write(
            "Welcome to Titin.\n"
        );

        console_newline();

        boot_write(
            "Choose your username: "
        );

        input_line_clear();

        line_x = console_cursor_x();
        line_y = console_cursor_y();

        for (;;) {
            char key = keyboard_read();

            if (key == '\b') {
                input_line_backspace();
                line_redraw();
                continue;
            }

            if (key == '\n') {
                break;
            }

            if (key == KEY_LEFT ||
                key == KEY_RIGHT ||
                key == KEY_UP ||
                key == KEY_DOWN ||
                key == KEY_TAB) {
                continue;
            }

            if (input_line_length() <
                USERNAME_SIZE - 1) {
                input_line_add(key);
                line_redraw();
            }
        }

        copy_text(
            username,
            input_line_get()
        );

        console_newline();

        if (!username_valid(username)) {
            boot_write(
                "Invalid username.\n"
            );

            boot_write(
                "Use 1-31 characters without spaces or >.\n"
            );

            continue;
        }

        char path[USERNAME_SIZE + 7];

        path[0] = '>';
        path[1] = 'u';
        path[2] = 's';
        path[3] = 'e';
        path[4] = 'r';
        path[5] = 's';
        path[6] = '>';

        uint64_t i = 0;

        while (username[i] != '\0') {
            path[7 + i] = username[i];
            i++;
        }

        path[7 + i] = '\0';

        if (tfs_create_path(
                path,
                TFS_CONTAINER
            ) == UINT64_MAX) {
            boot_write(
                "Unable to create user environment.\n"
            );

            continue;
        }

        boot_write(
            "Created "
        );

        boot_write(path);

        boot_write(
            "\n"
        );

        if (!tfs_change_directory(path)) {
            boot_write(
                "Unable to enter user environment.\n"
            );

            continue;
        }

        return;
    }
}

static void kernel_prompt(void)
{
    console_put_char('-');
    console_put_char(' ');

    console_put_char('>');

    const char *path =
        tfs_current_path();

    if (path[0] == '>') {
        path++;

        while (*path != '\0') {
            console_put_char(*path);
            path++;
        }
    }

    console_put_char(' ');
}

void kernel_main(void)
{
    console_start();

    console_newline();

    boot_status("Core");

    task_system_start();
    origin_start();

    boot_status("Origin");
    boot_status("Console");
    boot_status("Protein");
    boot_status("Amino");

    tfs_start();
    boot_status("TFS");

    username_setup();

    console_newline();

    boot_write("System ready.\n");

    console_newline();

    input_line_clear();
    kernel_prompt();

    line_x = console_cursor_x();
    line_y = console_cursor_y();

    for (;;) {
        char key = keyboard_read();

        if (key == KEY_UP) {
            history_up();
            continue;
        }

        if (key == KEY_DOWN) {
            history_down();
            continue;
        }

        if (key == KEY_LEFT) {
            input_line_left();

            console_cursor_set(
                line_x + input_line_cursor(),
                line_y
            );

            continue;
        }

        if (key == KEY_RIGHT) {
            input_line_right();

            console_cursor_set(
                line_x + input_line_cursor(),
                line_y
            );

            continue;
        }

        if (key == KEY_TAB) {
            command_complete();
            continue;
        }

        if (key == '\f') {
            console_clear();
            input_line_clear();
            history_position = -1;

            kernel_prompt();

            line_x = console_cursor_x();
            line_y = console_cursor_y();

            continue;
        }

        if (key == '\b') {
            input_line_backspace();
            line_redraw();
            history_position = -1;

            continue;
        }

        if (key == '\n') {
            console_newline();

            const char *line =
                input_line_get();

            if (line[0] != '\0') {
                history_add(line);
                protein_execute(line);
            }

            input_line_clear();
            history_position = -1;

            kernel_prompt();

            line_x = console_cursor_x();
            line_y = console_cursor_y();

            continue;
        }

        input_line_add(key);
        line_redraw();
        history_position = -1;
    }
}
