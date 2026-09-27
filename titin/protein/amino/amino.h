#ifndef TITIN_AMINO_H
#define TITIN_AMINO_H

typedef struct {
    const char *name;
    const char *source;
} amino_command;

void amino_execute(
    const char *source,
    const char *arguments
);

void amino_list_commands(void);

void amino_install_filesystem_commands(void);

int amino_execute_filesystem_command(
    const char *name,
    const char *arguments
);

#endif
