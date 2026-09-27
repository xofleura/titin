#include <stdint.h>

#include "../console/console.h"
#include "../input/keyboard.h"
#include "../input/line.h"
#include "../input/mouse.h"
#include "../protein/protein.h"
#include "../protein/amino/amino.h"
#include "../origin/origin.h"
#include "../task/task.h"
#include "../filesystem/tfs.h"
#include "../storage/storage.h"
#include "../display/display.h"
#include "../window/window.h"
#include "../interrupt/interrupt.h"
#include "../editor/editor.h"

#define HISTORY_SIZE 16
#define COMPLETION_SIZE 128
#define USERNAME_SIZE 32

static char history[
    HISTORY_SIZE
][128];

static uint64_t history_count = 0;
static int64_t history_position = -1;

static uint8_t line_x = 0;
static uint8_t line_y = 0;

static int32_t last_mouse_x = -1;
static int32_t last_mouse_y = -1;

static titin_window *terminal_window = 0;
static titin_window *dragged_window = 0;

static uint64_t terminal_number = 1;

static uint8_t previous_mouse_buttons = 0;

static void boot_write(
    const char *text
)
{
    while (*text != '\0') {
        console_put_char(*text);
        text++;
    }
}

static void boot_status(
    const char *name
)
{
    boot_write(name);
    boot_write(" ........ online\n");
}

static void boot_number(
    uint64_t value
)
{
    char buffer[21];
    uint64_t position = 0;

    if (value == 0) {
        console_put_char('0');
        return;
    }

    while (value > 0) {
        buffer[position++] =
            '0' +
            (char)(value % 10);

        value /= 10;
    }

    while (position > 0) {
        position--;

        console_put_char(
            buffer[position]
        );
    }
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
        destination[i] =
            source[i];

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

    if (
        history_count <
        HISTORY_SIZE
    ) {
        copy_text(
            history[history_count],
            line
        );

        history_count++;

        return;
    }

    for (
        uint64_t i = 1;
        i < HISTORY_SIZE;
        i++
    ) {
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

    for (
        uint64_t i = 0;
        text[i] != '\0';
        i++
    ) {
        input_line_add(
            text[i]
        );
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
        history[
            history_position
        ]
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
            history[
                history_position
            ]
        );

        return;
    }

    history_position = -1;

    line_replace("");
}

static void command_complete(void)
{
    char result[
        COMPLETION_SIZE
    ];

    if (
        input_line_length() == 0
    ) {
        return;
    }

    if (!protein_complete(
        input_line_get(),
        result,
        COMPLETION_SIZE
    )) {
        return;
    }

    line_replace(result);
}

static int username_valid(
    const char *username
)
{
    uint64_t length = 0;

    while (
        username[length] != '\0'
    ) {
        if (
            length >=
            USERNAME_SIZE - 1
        ) {
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

static void username_render(void)
{
    window_redraw();

    const titin_mouse_state *mouse =
        mouse_state();

    display_mouse_cursor(
        (uint32_t)mouse->x,
        (uint32_t)mouse->y
    );
}

static void username_setup(void)
{
    char username[
        USERNAME_SIZE
    ];

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

        line_x =
            console_cursor_x();

        line_y =
            console_cursor_y();

        username_render();

        for (;;) {
            char key =
                keyboard_read();

            if (key == '\b') {
                input_line_backspace();
                line_redraw();
                username_render();
                continue;
            }

            if (key == '\n') {
                break;
            }

            if (
                key == KEY_LEFT ||
                key == KEY_RIGHT ||
                key == KEY_UP ||
                key == KEY_DOWN ||
                key == KEY_TAB ||
                key == KEY_NEW_TERMINAL ||
                key == KEY_CLOSE_TERMINAL
            ) {
                continue;
            }

            if (
                input_line_length() <
                USERNAME_SIZE - 1
            ) {
                input_line_add(key);
                line_redraw();
                username_render();
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

            username_render();

            continue;
        }

        char path[
            USERNAME_SIZE + 7
        ];

        path[0] = '>';
        path[1] = 'u';
        path[2] = 's';
        path[3] = 'e';
        path[4] = 'r';
        path[5] = 's';
        path[6] = '>';

        uint64_t i = 0;

        while (
            username[i] != '\0'
        ) {
            path[7 + i] =
                username[i];

            i++;
        }

        path[7 + i] = '\0';

        if (
            tfs_create_path(
                path,
                TFS_CONTAINER
            ) == UINT64_MAX
        ) {
            boot_write(
                "Unable to create user environment.\n"
            );

            username_render();

            continue;
        }

        boot_write(
            "Created "
        );

        boot_write(path);

        boot_write("\n");

        if (
            !tfs_change_directory(path)
        ) {
            boot_write(
                "Unable to enter user environment.\n"
            );

            username_render();

            continue;
        }

        username_render();

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

static void terminal_open(void)
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;

    width = 800;
    height = 480;

    x =
        120 +
        (uint32_t)(
            (terminal_number % 5) * 45
        );

    y =
        80 +
        (uint32_t)(
            (terminal_number % 4) * 45
        );

    if (
        display_width() > 40 &&
        x + width > display_width()
    ) {
        x = 20;

        width =
            display_width() - 40;
    }

    if (
        display_height() > 40 &&
        y + height > display_height()
    ) {
        y = 20;

        height =
            display_height() - 40;
    }

    titin_window *window =
        window_create(
            x,
            y,
            width,
            height,
            "Terminal"
        );

    if (window == 0) {
        return;
    }

    if (!console_create_window(window)) {
        window_close(window);
        return;
    }

    terminal_window =
        window;

    terminal_number++;

    input_line_clear();

    history_position = -1;

    kernel_prompt();

    line_x =
        console_cursor_x();

    line_y =
        console_cursor_y();

    window_redraw();
}

static void terminal_close(void)
{
    titin_window *window =
        console_active_window();

    if (
        window == 0 ||
        !console_window_is_terminal(window)
    ) {
        return;
    }

    if (
        window ==
        terminal_window
    ) {
        terminal_window = 0;
    }

    console_close_window(
        window
    );

    window_close(
        window
    );

    terminal_window =
        console_active_window();

    input_line_clear();

    history_position = -1;

    kernel_prompt();

    line_x =
        console_cursor_x();

    line_y =
        console_cursor_y();

    window_redraw();
}

static void process_key(
    char key
)
{
    /*
     * ZM owns the keyboard while its
     * window is the active console.
     *
     * If ZM closes, restore the shell
     * terminal and create the prompt there.
     */
    if (
        editor_is_open() &&
        editor_window() ==
        console_active_window()
    ) {
        editor_process_key(key);

        if (!editor_is_open()) {
            if (
                terminal_window != 0 &&
                terminal_window->active
            ) {
                window_focus(
                    terminal_window
                );

                console_focus_window(
                    terminal_window
                );

                input_line_clear();

                history_position = -1;

                kernel_prompt();

                line_x =
                    console_cursor_x();

                line_y =
                    console_cursor_y();
            }

            window_redraw();
        }

        return;
    }

    if (key == KEY_NEW_TERMINAL) {
        terminal_open();
        return;
    }

    if (key == KEY_CLOSE_TERMINAL) {
        terminal_close();
        return;
    }

    if (key == KEY_UP) {
        history_up();
        window_redraw();
        return;
    }

    if (key == KEY_DOWN) {
        history_down();
        window_redraw();
        return;
    }

    if (key == KEY_LEFT) {
        input_line_left();

        console_cursor_set(
            line_x +
                input_line_cursor(),
            line_y
        );

        window_redraw();

        return;
    }

    if (key == KEY_RIGHT) {
        input_line_right();

        console_cursor_set(
            line_x +
                input_line_cursor(),
            line_y
        );

        window_redraw();

        return;
    }

    if (key == KEY_TAB) {
        command_complete();
        window_redraw();
        return;
    }

    if (key == KEY_VT3) {
        return;
    }

    if (key == '\f') {
        console_clear();

        input_line_clear();

        history_position = -1;

        kernel_prompt();

        line_x =
            console_cursor_x();

        line_y =
            console_cursor_y();

        window_redraw();

        return;
    }

    if (key == '\b') {
        input_line_backspace();

        line_redraw();

        history_position = -1;

        window_redraw();

        return;
    }

    if (key == '\n') {
        console_newline();

        const char *line =
            input_line_get();

        if (line[0] != '\0') {
            history_add(line);

            protein_execute(
                line
            );

            storage_save();
        }

        /*
         * Some commands, such as ZM,
         * replace the active console with
         * another window.
         *
         * Do not create the shell prompt
         * until that window closes.
         */
        if (
            editor_is_open()
        ) {
            window_redraw();
            return;
        }

        input_line_clear();

        history_position = -1;

        kernel_prompt();

        line_x =
            console_cursor_x();

        line_y =
            console_cursor_y();

        window_redraw();

        return;
    }

    input_line_add(key);

    line_redraw();

    history_position = -1;

    window_redraw();
}

static void console_follow_terminal_window(void)
{
    if (terminal_window == 0) {
        return;
    }

    console_follow_window(
        terminal_window
    );
}

static void process_mouse(void)
{
    const titin_mouse_state *mouse =
        mouse_state();

    uint8_t left =
        mouse->buttons & 1;

    uint8_t previous_left =
        previous_mouse_buttons & 1;

    if (
        left &&
        !previous_left
    ) {
        titin_window *window =
            window_at(
                (uint32_t)mouse->x,
                (uint32_t)mouse->y
            );

        if (window != 0) {
            window_focus(
                window
            );

            if (
                editor_is_open() &&
                window ==
                editor_window()
            ) {
                console_focus_window(
                    window
                );
            } else if (
                console_window_is_terminal(
                    window
                )
            ) {
                console_focus_window(
                    window
                );

                terminal_window =
                    window;

                input_line_clear();

                history_position = -1;

                line_x =
                    console_cursor_x();

                line_y =
                    console_cursor_y();
            }

            if (
                window_title_hit(
                    window,
                    (uint32_t)mouse->x,
                    (uint32_t)mouse->y
                )
            ) {
                dragged_window =
                    window;

                display_mouse_cursor_hide();

                window_begin_drag(
                    window,
                    (uint32_t)mouse->x,
                    (uint32_t)mouse->y
                );

                display_mouse_cursor(
                    (uint32_t)mouse->x,
                    (uint32_t)mouse->y
                );
            }
        }
    }

    if (
        left &&
        previous_left &&
        dragged_window != 0
    ) {
        if (
            mouse->x != last_mouse_x ||
            mouse->y != last_mouse_y
        ) {
            display_mouse_cursor_hide();

            window_drag(
                dragged_window,
                (uint32_t)mouse->x,
                (uint32_t)mouse->y
            );

            display_mouse_cursor(
                (uint32_t)mouse->x,
                (uint32_t)mouse->y
            );
        }
    }

    if (
        !left &&
        previous_left
    ) {
        if (dragged_window != 0) {
            display_mouse_cursor_hide();

            window_end_drag(
                dragged_window
            );

            if (
                dragged_window ==
                terminal_window
            ) {
                console_follow_terminal_window();
            }

            dragged_window = 0;

            window_redraw();

            display_mouse_cursor(
                (uint32_t)mouse->x,
                (uint32_t)mouse->y
            );
        }
    }

    previous_mouse_buttons =
        mouse->buttons;
}

static void render_frame(void)
{
    window_redraw();

    const titin_mouse_state *mouse =
        mouse_state();

    display_mouse_cursor(
        (uint32_t)mouse->x,
        (uint32_t)mouse->y
    );
}

static void render_mouse_move(
    int32_t old_x,
    int32_t old_y,
    int32_t new_x,
    int32_t new_y
)
{
    uint32_t minimum_x;
    uint32_t minimum_y;
    uint32_t maximum_x;
    uint32_t maximum_y;

    display_mouse_cursor_hide();

    display_mouse_cursor(
        (uint32_t)new_x,
        (uint32_t)new_y
    );

    minimum_x =
        old_x < new_x ?
        (uint32_t)old_x :
        (uint32_t)new_x;

    minimum_y =
        old_y < new_y ?
        (uint32_t)old_y :
        (uint32_t)new_y;

    maximum_x =
        old_x > new_x ?
        (uint32_t)old_x :
        (uint32_t)new_x;

    maximum_y =
        old_y > new_y ?
        (uint32_t)old_y :
        (uint32_t)new_y;

    if (minimum_x > 0) {
        minimum_x--;
    }

    if (minimum_y > 0) {
        minimum_y--;
    }

    maximum_x += 17;
    maximum_y += 17;

    if (
        maximum_x >
        display_width()
    ) {
        maximum_x =
            display_width();
    }

    if (
        maximum_y >
        display_height()
    ) {
        maximum_y =
            display_height();
    }

    display_present_region(
        minimum_x,
        minimum_y,
        maximum_x - minimum_x,
        maximum_y - minimum_y
    );
}

void kernel_main(
    uint32_t multiboot_magic,
    uint32_t multiboot_info
)
{
    display_start(
        multiboot_magic,
        multiboot_info
    );

    interrupt_start();

    window_system_start();

    if (display_available()) {
        display_clear(
            0x202020
        );
    }

    terminal_window =
        window_create(
            120,
            80,
            1040,
            560,
            "Terminal"
        );

    mouse_start();

    interrupt_enable();

    console_start();

    if (terminal_window != 0) {
        console_create_window(
            terminal_window
        );
    } else {
        console_bind_fullscreen();
    }

    boot_status("Core");

    task_system_start();

    origin_start();

    boot_status("Origin");
    boot_status("Console");
    boot_status("Protein");
    boot_status("Amino");

    if (display_available()) {
        boot_status("Display");

        boot_write(
            "Resolution ..... "
        );

        boot_number(
            display_width()
        );

        boot_write("x");

        boot_number(
            display_height()
        );

        boot_write("\n");
    }

    tfs_start();

    boot_status("TFS");

    int restored =
        storage_load();

    if (restored) {
        boot_status("Storage");

        uint64_t users =
            tfs_find(
                0,
                "users"
            );

        if (
            users != UINT64_MAX
        ) {
            uint64_t user_count =
                tfs_child_count(
                    users
                );

            if (user_count > 0) {
                const tfs_object *user =
                    tfs_child(
                        users,
                        0
                    );

                if (user != 0) {
                    char path[
                        TFS_PATH_SIZE
                    ];

                    uint64_t position = 0;

                    path[position++] = '>';
                    path[position++] = 'u';
                    path[position++] = 's';
                    path[position++] = 'e';
                    path[position++] = 'r';
                    path[position++] = 's';
                    path[position++] = '>';

                    for (
                        uint64_t i = 0;
                        user->name[i] != '\0' &&
                        position <
                            TFS_PATH_SIZE - 1;
                        i++
                    ) {
                        path[position++] =
                            user->name[i];
                    }

                    path[position] =
                        '\0';

                    tfs_change_directory(
                        path
                    );
                }
            }
        }
    }

    if (!restored) {
        amino_install_filesystem_commands();

        username_setup();

        storage_save();
    }

    console_newline();

    boot_write(
        "System ready."
    );

    console_newline();

    input_line_clear();

    kernel_prompt();

    line_x =
        console_cursor_x();

    line_y =
        console_cursor_y();

    const titin_mouse_state *initial_mouse =
        mouse_state();

    last_mouse_x =
        initial_mouse->x;

    last_mouse_y =
        initial_mouse->y;

    previous_mouse_buttons =
        initial_mouse->buttons;

    render_frame();

    for (;;) {
        process_mouse();

        const titin_mouse_state *mouse =
            mouse_state();

        if (
            mouse->x != last_mouse_x ||
            mouse->y != last_mouse_y
        ) {
            if (dragged_window == 0) {
                render_mouse_move(
                    last_mouse_x,
                    last_mouse_y,
                    mouse->x,
                    mouse->y
                );
            }

            last_mouse_x =
                mouse->x;

            last_mouse_y =
                mouse->y;
        }

        char key =
            keyboard_poll();

        if (key != 0) {
            process_key(key);
        }
    }
}
