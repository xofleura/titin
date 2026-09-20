#include <stdint.h>
#include "amino.h"
#include "../command.h"
#include "../../system/info.h"
#include "../../task/task.h"
#include "../../filesystem/tfs.h"

extern const amino_command __start_amino_commands[];
extern const amino_command __stop_amino_commands[];

static const char *amino_next_line(
    const char *line
)
{
    while (*line != '\0' &&
           *line != '\n') {
        line++;
    }

    if (*line == '\n') {
        line++;
    }

    return line;
}

static const char *amino_skip_spaces(
    const char *text
)
{
    while (*text == ' ' ||
           *text == '\t') {
        text++;
    }

    return text;
}

static int amino_word(
    const char *line,
    const char *word
)
{
    uint64_t i = 0;

    while (word[i] != '\0') {
        if (line[i] != word[i]) {
            return 0;
        }

        i++;
    }

    return line[i] == ' ' ||
           line[i] == '\t' ||
           line[i] == '\n' ||
           line[i] == '\0';
}

static void amino_write_text(
    const char *text
)
{
    if (*text != '"') {
        return;
    }

    text++;

    while (*text != '\0' &&
           *text != '"' &&
           *text != '\n') {
        command_write_char(*text);
        text++;
    }
}

static void amino_info(
    const char *text
)
{
    text = amino_skip_spaces(text);

    if (amino_word(text, "version")) {
        command_write(
            system_info_version()
        );

        return;
    }

    if (amino_word(text, "architecture")) {
        command_write(
            system_info_architecture()
        );

        return;
    }
}

static void amino_write_number(
    uint64_t value
)
{
    char buffer[21];
    uint64_t position = 0;

    if (value == 0) {
        command_write_char('0');
        return;
    }

    while (value > 0) {
        buffer[position] =
            '0' + (char)(value % 10);

        value /= 10;
        position++;
    }

    while (position > 0) {
        position--;

        command_write_char(
            buffer[position]
        );
    }
}

static void amino_tasks(void)
{
    uint64_t count =
        task_count();

    command_write("ID  NAME      STATE");
    command_newline();

    for (uint64_t i = 0;
         i < count;
         i++) {
        const titin_task *task =
            task_get(i);

        if (task == 0) {
            continue;
        }

        amino_write_number(
            task->id
        );

        command_write("   ");

        command_write(
            task->name
        );

        command_write("      ");

        command_write(
            task_state_name(
                task->state
            )
        );

        command_newline();
    }
}

static void amino_fs_list(
    uint64_t parent,
    uint64_t depth
)
{
    if (depth >= TFS_OBJECT_LIMIT) {
        return;
    }

    uint64_t count =
        tfs_child_count(parent);

    for (uint64_t i = 0;
         i < count;
         i++) {
        const tfs_object *object =
            tfs_child(parent, i);

        if (object == 0) {
            continue;
        }

        if (object->id == parent) {
            continue;
        }

        for (uint64_t j = 0;
             j < depth;
             j++) {
            command_write("  ");
        }

        command_write("|- ");

        if (object->type ==
            TFS_CONTAINER) {
            command_write(">");
        }

        command_write(object->name);
        command_newline();

        if (object->type ==
            TFS_CONTAINER) {
            amino_fs_list(
                object->id,
                depth + 1
            );
        }
    }
}

static void amino_fs_status(void)
{
    command_write("TFS");
    command_newline();

    command_write("Objects: ");

    amino_write_number(
        tfs_object_count()
    );

    command_newline();

    command_write("Location: ");
    command_write(
        tfs_current_path()
    );

    command_newline();
}

static void amino_fs_create(
    const char *path
)
{
    uint64_t id =
        tfs_create_path(
            path,
            TFS_CONTAINER
        );

    if (id == UINT64_MAX) {
        command_write("Unable to create: ");
        command_write(path);
        command_newline();

        return;
    }

    command_write("Created: ");
    command_write(path);
    command_newline();
}

static void amino_fs_file_create(
    const char *path
)
{
    uint64_t id;

    if (path[0] == '>') {
        id = tfs_create_path(
            path,
            TFS_DATA
        );
    } else {
        id = tfs_create(
            tfs_current(),
            path,
            TFS_DATA
        );
    }

    if (id == UINT64_MAX) {
        command_write(
            "Unable to create file: "
        );

        command_write(path);
        command_newline();

        return;
    }

    command_write("Created: ");

    if (path[0] != '>') {
    const char *current =
        tfs_current_path();

    command_write(current);

    if (current[1] != '\0') {
        command_write(">");
    }
}

    command_write(path);
    command_newline();
}

static void amino_fs_file_remove(
    const char *path
)
{
    uint64_t id;

    if (path[0] == '>') {
        id = tfs_find_path(path);
    } else {
        id = tfs_find(
            tfs_current(),
            path
        );
    }

    if (id == UINT64_MAX) {
        command_write(
            "Unable to remove file: "
        );

        command_write(path);
        command_newline();

        return;
    }

    const tfs_object *object =
        tfs_object_get(id);

    if (object == 0 ||
        object->type != TFS_DATA) {
        command_write(
            "Not a file: "
        );

        command_write(path);
        command_newline();

        return;
    }

    if (!tfs_remove_path(
            path[0] == '>'
                ? path
                : object->name
        )) {
        command_write(
            "Unable to remove file: "
        );

        command_write(path);
        command_newline();

        return;
    }

    command_write("Removed: ");

    if (path[0] != '>') {
        command_write(
            tfs_current_path()
        );

        command_write(">");
    }

    command_write(path);
    command_newline();
}

static void amino_fs_remove(
    const char *path
)
{
    if (!tfs_remove_path(path)) {
        command_write("Unable to remove: ");
        command_write(path);
        command_newline();

        return;
    }

    command_write("Removed: ");
    command_write(path);
    command_newline();
}

static void amino_fs(
    const char *text
)
{
    text = amino_skip_spaces(text);

    if (*text == '\0') {
        amino_fs_status();
        return;
    }

    if (amino_word(text, "list")) {
        command_write(
            tfs_current_path()
        );

        command_newline();

        amino_fs_list(
            tfs_current(),
            0
        );

        return;
    }

    if (amino_word(text, "status")) {
        amino_fs_status();
        return;
    }

    if (amino_word(text, "create")) {
        text += 6;
        text = amino_skip_spaces(text);

        if (*text != '>') {
            command_write(
                "Path must begin with >"
            );

            command_newline();

            return;
        }

        amino_fs_create(text);
        return;
    }

    if (amino_word(text, "file")) {
        text += 4;
        text = amino_skip_spaces(text);

        if (amino_word(text, "create")) {
            text += 6;
            text = amino_skip_spaces(text);

            if (*text == '\0') {
                command_write(
                    "Usage: fs file create name"
                );

                command_newline();

                return;
            }

            amino_fs_file_create(text);
            return;
        }

        if (amino_word(text, "remove")) {
            text += 6;
            text = amino_skip_spaces(text);

            if (*text == '\0') {
                command_write(
                    "Usage: fs file remove name"
                );

                command_newline();

                return;
            }

            amino_fs_file_remove(text);
            return;
        }

        command_write("Usage:");
        command_newline();
        command_write("  fs file create name");
        command_newline();
        command_write("  fs file create >path");
        command_newline();
        command_write("  fs file remove name");
        command_newline();
        command_write("  fs file remove >path");
        command_newline();

        return;
    }

    if (amino_word(text, "remove")) {
        text += 6;
        text = amino_skip_spaces(text);

        if (*text != '>') {
            command_write(
                "Path must begin with >"
            );

            command_newline();

            return;
        }

        amino_fs_remove(text);
        return;
    }

    command_write("Usage:");
    command_newline();
    command_write("  fs");
    command_newline();
    command_write("  fs status");
    command_newline();
    command_write("  fs list");
    command_newline();
    command_write("  fs create >path");
    command_newline();
    command_write("  fs file create name");
    command_newline();
    command_write("  fs file create >path");
    command_newline();
    command_write("  fs file remove name");
    command_newline();
    command_write("  fs file remove >path");
    command_newline();
    command_write("  fs remove >path");
    command_newline();
}

static void amino_enter(
    const char *text
)
{
    text = amino_skip_spaces(text);

    if (*text == '\0') {
        command_write(
            "Usage: enter >path"
        );

        command_newline();

        return;
    }

    if (*text != '>') {
        command_write(
            "Path must begin with >"
        );

        command_newline();

        return;
    }

    if (!tfs_change_directory(text)) {
        command_write("Unable to enter: ");
        command_write(text);
        command_newline();

        return;
    }

    command_write(
        tfs_current_path()
    );

    command_newline();
}

static void amino_exit(void)
{
    if (!tfs_exit_directory()) {
        command_write(
            "Unable to exit directory"
        );

        command_newline();

        return;
    }

    command_write(
        tfs_current_path()
    );

    command_newline();
}

void amino_list_commands(void)
{
    const amino_command *command =
        __start_amino_commands;

    while (command < __stop_amino_commands) {
        command_write(command->name);
        command_newline();
        command++;
    }
}

void amino_execute(
    const char *source,
    const char *arguments
)
{
    const char *line = source;

    while (*line != '\0') {
        line = amino_skip_spaces(line);

        if (amino_word(line, "write")) {
            line += 5;
            line = amino_skip_spaces(line);

            amino_write_text(line);
        } else if (amino_word(line, "newline")) {
            command_newline();
        } else if (amino_word(line, "clear")) {
            command_clear();
        } else if (amino_word(line, "commands")) {
            amino_list_commands();
        } else if (amino_word(line, "info")) {
            line += 4;

            amino_info(line);
        } else if (amino_word(line, "tasks")) {
            amino_tasks();
        } else if (amino_word(line, "fs")) {
            amino_fs(arguments);
        } else if (amino_word(line, "enter")) {
            amino_enter(arguments);
        } else if (amino_word(line, "exit")) {
            amino_exit();
        }

        line = amino_next_line(line);
    }
}

