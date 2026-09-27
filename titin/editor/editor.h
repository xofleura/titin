#ifndef TITIN_EDITOR_H
#define TITIN_EDITOR_H

#include <stdint.h>

struct titin_window;

void editor_open(
    const char *path
);

void editor_process_key(
    char key
);

int editor_is_open(void);

struct titin_window *editor_window(void);

void editor_close(void);

#endif
