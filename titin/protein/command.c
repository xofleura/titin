#include <stdint.h>
#include "../console/console.h"
#include "command.h"

extern const titin_command __start_titin_commands[];
extern const titin_command __stop_titin_commands[];

void command_write(const char *text)
{
    for (uint64_t i = 0; text[i] != '\0'; i++) {
        console_put_char(text[i]);
    }
}

void command_write_char(char c)
{
    console_put_char(c);
}

void command_newline(void)
{
    console_newline();
}

void command_clear(void)
{
    console_clear();
}

void command_list(void)
{
    const titin_command *command = __start_titin_commands;

    while (command < __stop_titin_commands) {
        command_write(command->name);
        command_newline();
        command++;
    }
}
