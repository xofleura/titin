#ifndef TITIN_TFS_H
#define TITIN_TFS_H

#include <stdint.h>

#define TFS_OBJECT_LIMIT 128
#define TFS_NAME_SIZE 32
#define TFS_DATA_SIZE 256
#define TFS_PATH_SIZE 256

typedef enum {
    TFS_CONTAINER,
    TFS_DATA
} tfs_object_type;

typedef struct {
    uint64_t id;
    uint64_t parent;
    tfs_object_type type;
    uint64_t size;
    char name[TFS_NAME_SIZE];
    uint8_t data[TFS_DATA_SIZE];
    uint8_t active;
} tfs_object;

void tfs_start(void);

uint64_t tfs_object_count(void);

const tfs_object *tfs_object_get(
    uint64_t id
);

uint64_t tfs_create(
    uint64_t parent,
    const char *name,
    tfs_object_type type
);

uint64_t tfs_find(
    uint64_t parent,
    const char *name
);

uint64_t tfs_find_path(
    const char *path
);

uint64_t tfs_create_path(
    const char *path,
    tfs_object_type type
);

int tfs_remove_path(
    const char *path
);

uint64_t tfs_child_count(
    uint64_t parent
);

const tfs_object *tfs_child(
    uint64_t parent,
    uint64_t index
);

int tfs_write(
    uint64_t id,
    const uint8_t *data,
    uint64_t size
);

uint64_t tfs_read(
    uint64_t id,
    uint8_t *data,
    uint64_t size
);

uint64_t tfs_current(void);

int tfs_change_directory(
    const char *path
);

int tfs_exit_directory(void);

const char *tfs_current_path(void);

#endif
