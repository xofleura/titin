#ifndef TITIN_PROTEIN_H
#define TITIN_PROTEIN_H

#include <stdint.h>

void protein_execute(const char *line);
int protein_complete(
    const char *prefix,
    char *result,
    uint64_t result_size
);

#endif
