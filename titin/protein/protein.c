#include <stdint.h>
#include "../filesystem/tfs.h"
#include "protein.h"
#include "command.h"
#include "amino/amino.h"

static int protein_prefix(
    const char *prefix,
    const char *command
)
{
    uint64_t i = 0;

    while (prefix[i] != '\0') {
        if (command[i] == '\0' ||
            prefix[i] != command[i]) {
            return 0;
        }

        i++;
    }

    return 1;
}

static int protein_copy(
    char *result,
    uint64_t result_size,
    const char *command
)
{
    uint64_t i = 0;

    if (result_size == 0) {
        return 0;
    }

    while (command[i] != '\0') {
        if (i >= result_size - 1) {
            return 0;
        }

        result[i] = command[i];
        i++;
    }

    result[i] = '\0';

    return 1;
}

static uint64_t protein_command_length(
    const char *line
)
{
    uint64_t length = 0;

    while (
        line[length] != '\0' &&
        line[length] != ' ' &&
        line[length] != '\t'
    ) {
        length++;
    }

    return length;
}

static int protein_name_match(
    const char *line,
    const char *command
)
{
    uint64_t i = 0;

    while (command[i] != '\0') {
        if (line[i] != command[i]) {
            return 0;
        }

        i++;
    }

    return line[i] == '\0' ||
           line[i] == ' ' ||
           line[i] == '\t';
}

static uint64_t protein_command_directory(void)
{
    uint64_t applications =
        tfs_find(
            0,
            "applications"
        );

    if (applications == UINT64_MAX) {
        return UINT64_MAX;
    }

    return tfs_find(
        applications,
        "commands"
    );
}

static int protein_extract_command_name(
    const char *line,
    char *name,
    uint64_t size
)
{
    uint64_t length =
        protein_command_length(line);

    if (length == 0 ||
        length >= size) {
        return 0;
    }

    for (uint64_t i = 0;
         i < length;
         i++) {
        name[i] = line[i];
    }

    name[length] = '\0';

    return 1;
}

void protein_execute(
    const char *line
)
{
    char command_name[TFS_NAME_SIZE];

    uint64_t command_length =
        protein_command_length(line);

    if (command_length == 0) {
        return;
    }

    if (!protein_extract_command_name(
            line,
            command_name,
            sizeof(command_name)
        )) {
        command_write(
            "Unknown command: "
        );

        command_write(line);
        command_write("\n");

        return;
    }

    const char *arguments =
        line + command_length;

    while (*arguments == ' ' ||
           *arguments == '\t') {
        arguments++;
    }

    if (amino_execute_filesystem_command(
            command_name,
            arguments
        )) {
        return;
    }

    command_write(
        "Unknown command: "
    );

    command_write(line);
    command_write("\n");
}

int protein_complete(
    const char *prefix,
    char *result,
    uint64_t result_size
)
{
    uint64_t commands =
        protein_command_directory();

    if (commands == UINT64_MAX) {
        return 0;
    }

    const tfs_object *match = 0;
    uint64_t matches = 0;

    uint64_t count =
        tfs_child_count(commands);

    for (uint64_t i = 0;
         i < count;
         i++) {
        const tfs_object *object =
            tfs_child(
                commands,
                i
            );

        if (object == 0) {
            continue;
        }

        if (object->type != TFS_DATA) {
            continue;
        }

        if (protein_prefix(
                prefix,
                object->name
            )) {
            match = object;
            matches++;
        }
    }

    if (matches != 1 ||
        match == 0) {
        return 0;
    }

    return protein_copy(
        result,
        result_size,
        match->name
    );
}
