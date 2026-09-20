#ifndef TITIN_COMMAND_H
#define TITIN_COMMAND_H

typedef void (*titin_command_function)(void);

typedef struct {
    const char *name;
    titin_command_function function;
} titin_command;

void command_write(const char *text);
void command_write_char(char c);
void command_newline(void);
void command_clear(void);
void command_list(void);

#define TITIN_COMMAND(name, function) \
    const titin_command titin_command_##function \
    __attribute__((section("titin_commands"), used)) = { \
        name, \
        function \
    }

#endif
