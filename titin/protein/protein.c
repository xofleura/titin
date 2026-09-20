#include <stdint.h>
#include "protein.h"
#include "command.h"
#include "amino/amino.h"

extern const titin_command __start_titin_commands[];
extern const titin_command __stop_titin_commands[];

extern const amino_command __start_amino_commands[];
extern const amino_command __stop_amino_commands[];

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

void protein_execute(
    const char *line
)
{
    uint64_t command_length =
        protein_command_length(line);

    const amino_command *amino =
        __start_amino_commands;

    while (amino < __stop_amino_commands) {
        if (protein_name_match(
                line,
                amino->name)) {
            const char *arguments =
                line + command_length;

            while (*arguments == ' ' ||
                   *arguments == '\t') {
                arguments++;
            }

            amino_execute(
                amino->source,
                arguments
            );

            return;
        }

        amino++;
    }

    if (command_length == 0) {
        return;
    }

    const titin_command *command =
        __start_titin_commands;

    while (command < __stop_titin_commands) {
        if (protein_name_match(
                line,
                command->name)) {
            command->function();
            return;
        }

        command++;
    }

    command_write("Unknown command: ");
    command_write(line);
    command_write("\n");
}

int protein_complete(
    const char *prefix,
    char *result,
    uint64_t result_size
)
{
    const char *match = 0;
    uint64_t matches = 0;

    const amino_command *amino =
        __start_amino_commands;

    while (amino < __stop_amino_commands) {
        if (protein_prefix(
                prefix,
                amino->name)) {
            match = amino->name;
            matches++;
        }

        amino++;
    }

    const titin_command *command =
        __start_titin_commands;

    while (command < __stop_titin_commands) {
        if (protein_prefix(
                prefix,
                command->name)) {
            match = command->name;
            matches++;
        }

        command++;
    }

    if (matches != 1) {
        return 0;
    }

    return protein_copy(
        result,
        result_size,
        match
    );
}
