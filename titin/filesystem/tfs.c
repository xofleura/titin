#include <stdint.h>
#include "tfs.h"

static tfs_object objects[TFS_OBJECT_LIMIT];
static uint64_t object_count = 0;
static uint64_t current_object = 0;
static char current_path[TFS_PATH_SIZE];

static uint64_t text_length(
    const char *text
)
{
    uint64_t length = 0;

    while (text[length] != '\0') {
        length++;
    }

    return length;
}

static void text_copy(
    char *destination,
    const char *source,
    uint64_t size
)
{
    uint64_t i = 0;

    if (size == 0) {
        return;
    }

    while (
        source[i] != '\0' &&
        i < size - 1
    ) {
        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';
}

static int text_match(
    const char *left,
    const char *right
)
{
    uint64_t i = 0;

    while (
        left[i] != '\0' &&
        right[i] != '\0'
    ) {
        if (left[i] != right[i]) {
            return 0;
        }

        i++;
    }

    return left[i] == '\0' &&
           right[i] == '\0';
}

static int path_copy(
    char *destination,
    const char *source
)
{
    uint64_t length =
        text_length(source);

    if (length >= TFS_PATH_SIZE) {
        return 0;
    }

    text_copy(
        destination,
        source,
        TFS_PATH_SIZE
    );

    return 1;
}

static int path_from_object(
    uint64_t id,
    char *destination
)
{
    uint64_t chain[TFS_OBJECT_LIMIT];
    uint64_t count = 0;
    uint64_t current = id;

    if (id >= TFS_OBJECT_LIMIT ||
        !objects[id].active) {
        return 0;
    }

    while (current != 0) {
        if (count >= TFS_OBJECT_LIMIT) {
            return 0;
        }

        if (current >= TFS_OBJECT_LIMIT ||
            !objects[current].active) {
            return 0;
        }

        chain[count] = current;
        count++;

        current = objects[current].parent;
    }

    uint64_t position = 0;

    if (position >= TFS_PATH_SIZE - 1) {
        return 0;
    }

    destination[position] = '>';
    position++;

    for (uint64_t i = count;
         i > 0;
         i--) {
        const char *name =
            objects[chain[i - 1]].name;

        uint64_t length =
            text_length(name);

        if (position + length >=
            TFS_PATH_SIZE) {
            return 0;
        }

        for (uint64_t j = 0;
             j < length;
             j++) {
            destination[position] =
                name[j];

            position++;
        }

        if (i > 1) {
            if (position >=
                TFS_PATH_SIZE - 1) {
                return 0;
            }

            destination[position] = '>';
            position++;
        }
    }

    destination[position] = '\0';

    return 1;
}

void tfs_start(void)
{
    object_count = 1;
    current_object = 0;

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        objects[i].active = 0;
        objects[i].id = i;
        objects[i].parent = 0;
        objects[i].type = TFS_CONTAINER;
        objects[i].size = 0;
        objects[i].name[0] = '\0';

        for (uint64_t j = 0;
             j < TFS_DATA_SIZE;
             j++) {
            objects[i].data[j] = 0;
        }
    }

    objects[0].active = 1;
    objects[0].id = 0;
    objects[0].parent = 0;
    objects[0].type = TFS_CONTAINER;
    objects[0].size = 0;
    objects[0].name[0] = '\0';

    current_path[0] = '>';
    current_path[1] = '\0';

    tfs_create(
        0,
        "system",
        TFS_CONTAINER
    );

    tfs_create(
        0,
        "users",
        TFS_CONTAINER
    );

    tfs_create(
        0,
        "applications",
        TFS_CONTAINER
    );

    tfs_create(
        0,
        "data",
        TFS_CONTAINER
    );

    tfs_create(
        0,
        "temporary",
        TFS_CONTAINER
    );

    tfs_create(
        0,
        "devices",
        TFS_CONTAINER
    );
}

uint64_t tfs_object_count(void)
{
    return object_count;
}

const tfs_object *tfs_object_get(
    uint64_t id
)
{
    if (id >= TFS_OBJECT_LIMIT) {
        return 0;
    }

    if (!objects[id].active) {
        return 0;
    }

    return &objects[id];
}

uint64_t tfs_find(
    uint64_t parent,
    const char *name
)
{
    if (parent >= TFS_OBJECT_LIMIT) {
        return UINT64_MAX;
    }

    if (!objects[parent].active) {
        return UINT64_MAX;
    }

    if (objects[parent].type !=
        TFS_CONTAINER) {
        return UINT64_MAX;
    }

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        if (!objects[i].active) {
            continue;
        }

        if (objects[i].parent != parent) {
            continue;
        }

        if (text_match(
                objects[i].name,
                name)) {
            return objects[i].id;
        }
    }

    return UINT64_MAX;
}

uint64_t tfs_create(
    uint64_t parent,
    const char *name,
    tfs_object_type type
)
{
    const tfs_object *parent_object =
        tfs_object_get(parent);

    if (parent_object == 0) {
        return UINT64_MAX;
    }

    if (parent_object->type !=
        TFS_CONTAINER) {
        return UINT64_MAX;
    }

    if (type != TFS_CONTAINER &&
        type != TFS_DATA) {
        return UINT64_MAX;
    }

    uint64_t length =
        text_length(name);

    if (length == 0 ||
        length >= TFS_NAME_SIZE) {
        return UINT64_MAX;
    }

    for (uint64_t i = 0;
         i < length;
         i++) {
        if (name[i] == '>') {
            return UINT64_MAX;
        }
    }

    if (tfs_find(parent, name) !=
        UINT64_MAX) {
        return UINT64_MAX;
    }

    for (uint64_t i = 1;
         i < TFS_OBJECT_LIMIT;
         i++) {
        if (objects[i].active) {
            continue;
        }

        objects[i].active = 1;
        objects[i].id = i;
        objects[i].parent = parent;
        objects[i].type = type;
        objects[i].size = 0;

        text_copy(
            objects[i].name,
            name,
            TFS_NAME_SIZE
        );

        for (uint64_t j = 0;
             j < TFS_DATA_SIZE;
             j++) {
            objects[i].data[j] = 0;
        }

        object_count++;

        return i;
    }

    return UINT64_MAX;
}

uint64_t tfs_find_path(
    const char *path
)
{
    if (path == 0) {
        return UINT64_MAX;
    }

    if (path[0] != '>') {
        return UINT64_MAX;
    }

    if (path[1] == '\0') {
        return 0;
    }

    uint64_t current = 0;
    uint64_t position = 1;

    while (path[position] != '\0') {
        char name[TFS_NAME_SIZE];
        uint64_t length = 0;

        while (
            path[position] != '\0' &&
            path[position] != '>'
        ) {
            if (length >=
                TFS_NAME_SIZE - 1) {
                return UINT64_MAX;
            }

            name[length] =
                path[position];

            length++;
            position++;
        }

        if (length == 0) {
            return UINT64_MAX;
        }

        name[length] = '\0';

        current = tfs_find(
            current,
            name
        );

        if (current == UINT64_MAX) {
            return UINT64_MAX;
        }

        if (path[position] == '>') {
            position++;

            if (path[position] == '\0') {
                return UINT64_MAX;
            }
        }
    }

    return current;
}

uint64_t tfs_create_path(
    const char *path,
    tfs_object_type type
)
{
    if (path == 0) {
        return UINT64_MAX;
    }

    if (path[0] != '>') {
        return UINT64_MAX;
    }

    if (path[1] == '\0') {
        return UINT64_MAX;
    }

    uint64_t current = 0;
    uint64_t position = 1;

    while (path[position] != '\0') {
        char name[TFS_NAME_SIZE];
        uint64_t length = 0;

        while (
            path[position] != '\0' &&
            path[position] != '>'
        ) {
            if (length >=
                TFS_NAME_SIZE - 1) {
                return UINT64_MAX;
            }

            name[length] =
                path[position];

            length++;
            position++;
        }

        if (length == 0) {
            return UINT64_MAX;
        }

        name[length] = '\0';

        int final =
            path[position] == '\0';

        uint64_t existing =
            tfs_find(
                current,
                name
            );

        if (existing != UINT64_MAX) {
            if (final) {
                return UINT64_MAX;
            }

            current = existing;
        } else {
            tfs_object_type next_type =
                final
                    ? type
                    : TFS_CONTAINER;

            current = tfs_create(
                current,
                name,
                next_type
            );

            if (current == UINT64_MAX) {
                return UINT64_MAX;
            }
        }

        if (path[position] == '>') {
            position++;

            if (path[position] == '\0') {
                return UINT64_MAX;
            }
        }
    }

    return current;
}

int tfs_remove_path(
    const char *path
)
{
    uint64_t object =
        tfs_find_path(path);

    if (object == UINT64_MAX) {
        return 0;
    }

    if (object == 0) {
        return 0;
    }

    if (object == current_object) {
        return 0;
    }

    if (tfs_child_count(object) != 0) {
        return 0;
    }

    if (!objects[object].active) {
        return 0;
    }

    objects[object].active = 0;

    if (object_count > 0) {
        object_count--;
    }

    return 1;
}

uint64_t tfs_child_count(
    uint64_t parent
)
{
    if (parent >= TFS_OBJECT_LIMIT) {
        return 0;
    }

    if (!objects[parent].active) {
        return 0;
    }

    if (objects[parent].type !=
        TFS_CONTAINER) {
        return 0;
    }

    uint64_t count = 0;

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        if (!objects[i].active) {
            continue;
        }

        if (objects[i].parent != parent) {
            continue;
        }

        count++;
    }

    return count;
}

const tfs_object *tfs_child(
    uint64_t parent,
    uint64_t index
)
{
    if (parent >= TFS_OBJECT_LIMIT) {
        return 0;
    }

    if (!objects[parent].active) {
        return 0;
    }

    if (objects[parent].type !=
        TFS_CONTAINER) {
        return 0;
    }

    uint64_t position = 0;

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        if (!objects[i].active) {
            continue;
        }

        if (objects[i].parent != parent) {
            continue;
        }

        if (position == index) {
            return &objects[i];
        }

        position++;
    }

    return 0;
}

int tfs_write(
    uint64_t id,
    const uint8_t *data,
    uint64_t size
)
{
    if (data == 0) {
        return 0;
    }

    if (size > TFS_DATA_SIZE) {
        return 0;
    }

    if (id >= TFS_OBJECT_LIMIT) {
        return 0;
    }

    if (!objects[id].active) {
        return 0;
    }

    if (objects[id].type != TFS_DATA) {
        return 0;
    }

    for (uint64_t i = 0;
         i < size;
         i++) {
        objects[id].data[i] = data[i];
    }

    objects[id].size = size;

    return 1;
}

uint64_t tfs_read(
    uint64_t id,
    uint8_t *data,
    uint64_t size
)
{
    if (data == 0) {
        return 0;
    }

    if (id >= TFS_OBJECT_LIMIT) {
        return 0;
    }

    if (!objects[id].active) {
        return 0;
    }

    if (objects[id].type != TFS_DATA) {
        return 0;
    }

    if (size > objects[id].size) {
        size = objects[id].size;
    }

    for (uint64_t i = 0;
         i < size;
         i++) {
        data[i] = objects[id].data[i];
    }

    return size;
}

uint64_t tfs_current(void)
{
    return current_object;
}

int tfs_change_directory(
    const char *path
)
{
    uint64_t object =
        tfs_find_path(path);

    if (object == UINT64_MAX) {
        return 0;
    }

    const tfs_object *target =
        tfs_object_get(object);

    if (target == 0) {
        return 0;
    }

    if (target->type != TFS_CONTAINER) {
        return 0;
    }

    char new_path[TFS_PATH_SIZE];

    if (!path_from_object(
            object,
            new_path)) {
        return 0;
    }

    if (!path_copy(
            current_path,
            new_path)) {
        return 0;
    }

    current_object = object;

    return 1;
}

int tfs_exit_directory(void)
{
    if (current_object == 0) {
        return 1;
    }

    uint64_t parent =
        objects[current_object].parent;

    char new_path[TFS_PATH_SIZE];

    if (!path_from_object(
            parent,
            new_path)) {
        return 0;
    }

    if (!path_copy(
            current_path,
            new_path)) {
        return 0;
    }

    current_object = parent;

    return 1;
}

const char *tfs_current_path(void)
{
    return current_path;
}

static void storage_put64(
    uint8_t *buffer,
    uint64_t offset,
    uint64_t value
)
{
    for (uint64_t i = 0;
         i < 8;
         i++) {
        buffer[offset + i] =
            (uint8_t)(value >> (i * 8));
    }
}

static uint64_t storage_get64(
    const uint8_t *buffer,
    uint64_t offset
)
{
    uint64_t value = 0;

    for (uint64_t i = 0;
         i < 8;
         i++) {
        value |=
            ((uint64_t)buffer[offset + i])
            << (i * 8);
    }

    return value;
}

int tfs_storage_export(
    uint8_t *buffer,
    uint64_t size
)
{
    if (buffer == 0 ||
        size < TFS_STORAGE_SIZE) {
        return 0;
    }

    for (uint64_t i = 0;
         i < TFS_STORAGE_SIZE;
         i++) {
        buffer[i] = 0;
    }

    buffer[0] = 'T';
    buffer[1] = 'I';
    buffer[2] = 'T';
    buffer[3] = 'I';
    buffer[4] = 'N';

    storage_put64(
        buffer,
        8,
        1
    );

    storage_put64(
        buffer,
        16,
        object_count
    );

    uint64_t offset =
        TFS_STORAGE_SECTOR_SIZE;

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        storage_put64(
            buffer,
            offset + 0,
            objects[i].id
        );

        storage_put64(
            buffer,
            offset + 8,
            objects[i].parent
        );

        storage_put64(
            buffer,
            offset + 16,
            (uint64_t)objects[i].type
        );

        storage_put64(
            buffer,
            offset + 24,
            objects[i].size
        );

        storage_put64(
            buffer,
            offset + 312,
            objects[i].active
        );

        for (uint64_t j = 0;
             j < TFS_NAME_SIZE;
             j++) {
            buffer[offset + 32 + j] =
                (uint8_t)objects[i].name[j];
        }

        for (uint64_t j = 0;
             j < TFS_DATA_SIZE;
             j++) {
            buffer[offset + 64 + j] =
                objects[i].data[j];
        }

        offset += TFS_STORAGE_SECTOR_SIZE;
    }

    return 1;
}

int tfs_storage_import(
    const uint8_t *buffer,
    uint64_t size
)
{
    if (buffer == 0 ||
        size < TFS_STORAGE_SIZE) {
        return 0;
    }

    if (buffer[0] != 'T' ||
        buffer[1] != 'I' ||
        buffer[2] != 'T' ||
        buffer[3] != 'I' ||
        buffer[4] != 'N') {
        return 0;
    }

    if (storage_get64(
            buffer,
            8
        ) != 1) {
        return 0;
    }

    uint64_t stored_count =
        storage_get64(
            buffer,
            16
        );

    if (stored_count == 0 ||
        stored_count > TFS_OBJECT_LIMIT) {
        return 0;
    }

    uint64_t offset =
        TFS_STORAGE_SECTOR_SIZE;

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        uint64_t id =
            storage_get64(
                buffer,
                offset + 0
            );

        uint64_t parent =
            storage_get64(
                buffer,
                offset + 8
            );

        uint64_t type =
            storage_get64(
                buffer,
                offset + 16
            );

        uint64_t object_size =
            storage_get64(
                buffer,
                offset + 24
            );

        uint64_t active =
            storage_get64(
                buffer,
                offset + 312
            );

        if (id != i) {
            return 0;
        }

        if (type > TFS_DATA) {
            return 0;
        }

        if (object_size > TFS_DATA_SIZE) {
            return 0;
        }

        objects[i].id = id;
        objects[i].parent = parent;
        objects[i].type =
            (tfs_object_type)type;
        objects[i].size = object_size;
        objects[i].active =
            active ? 1 : 0;

        for (uint64_t j = 0;
             j < TFS_NAME_SIZE;
             j++) {
            objects[i].name[j] =
                (char)buffer[
                    offset + 32 + j
                ];
        }

        objects[i].name[
            TFS_NAME_SIZE - 1
        ] = '\0';

        for (uint64_t j = 0;
             j < TFS_DATA_SIZE;
             j++) {
            objects[i].data[j] =
                buffer[
                    offset + 64 + j
                ];
        }

        offset += TFS_STORAGE_SECTOR_SIZE;
    }

    if (!objects[0].active ||
        objects[0].type != TFS_CONTAINER) {
        return 0;
    }

    for (uint64_t i = 1;
         i < TFS_OBJECT_LIMIT;
         i++) {
        if (!objects[i].active) {
            continue;
        }

        if (objects[i].parent >=
            TFS_OBJECT_LIMIT) {
            return 0;
        }

        if (!objects[
                objects[i].parent
            ].active) {
            return 0;
        }
    }

    object_count = stored_count;
    current_object = 0;

    current_path[0] = '>';
    current_path[1] = '\0';

    return 1;
}
