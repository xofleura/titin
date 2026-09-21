#include <stdint.h>
#include <limits.h>
#include "tfs.h"

static tfs_object objects[TFS_OBJECT_LIMIT];

static uint64_t object_count;
static uint64_t current_object;

static char current_path[TFS_PATH_SIZE];

static uint64_t tfs_string_length(
    const char *text
)
{
    uint64_t length = 0;

    if (text == 0) {
        return 0;
    }

    while (text[length] != '\0') {
        length++;
    }

    return length;
}

static int tfs_string_equal(
    const char *a,
    const char *b
)
{
    uint64_t i = 0;

    if (a == 0 || b == 0) {
        return 0;
    }

    while (a[i] != '\0' &&
           b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }

        i++;
    }

    return a[i] == '\0' &&
           b[i] == '\0';
}

static int tfs_string_copy(
    char *destination,
    uint64_t size,
    const char *source
)
{
    uint64_t i = 0;

    if (destination == 0 ||
        source == 0 ||
        size == 0) {
        return 0;
    }

    while (source[i] != '\0') {
        if (i >= size - 1) {
            return 0;
        }

        destination[i] = source[i];
        i++;
    }

    destination[i] = '\0';

    return 1;
}

static uint64_t tfs_next_component(
    const char *path,
    uint64_t *position,
    char *component,
    uint64_t component_size
)
{
    uint64_t i = 0;
    uint64_t position_value;

    if (path == 0 ||
        position == 0 ||
        component == 0 ||
        component_size == 0) {
        return 0;
    }

    position_value = *position;

    while (path[position_value] == '>') {
        position_value++;
    }

    if (path[position_value] == '\0') {
        *position = position_value;
        component[0] = '\0';
        return 0;
    }

    while (path[position_value] != '\0' &&
           path[position_value] != '>') {
        if (i >= component_size - 1) {
            component[0] = '\0';
            return 0;
        }

        component[i] =
            path[position_value];

        i++;
        position_value++;
    }

    component[i] = '\0';

    *position = position_value;

    return i;
}

static uint64_t tfs_resolve_path(
    const char *path
)
{
    uint64_t current;
    uint64_t position = 0;
    char component[TFS_NAME_SIZE];

    if (path == 0 ||
        path[0] == '\0') {
        return UINT64_MAX;
    }

    if (path[0] == '>') {
        current = 0;
    } else {
        current = current_object;
    }

    while (1) {
        uint64_t length =
            tfs_next_component(
                path,
                &position,
                component,
                sizeof(component)
            );

        if (length == 0) {
            break;
        }

        if (tfs_string_equal(
                component,
                "."
            )) {
            continue;
        }

        if (tfs_string_equal(
                component,
                ".."
            )) {
            if (current != 0) {
                current =
                    objects[current].parent;
            }

            continue;
        }

        current =
            tfs_find(
                current,
                component
            );

        if (current == UINT64_MAX) {
            return UINT64_MAX;
        }

        if (path[position] == '\0') {
            break;
        }

        position++;
    }

    return current;
}

static void tfs_update_current_path(void)
{
    uint64_t chain[TFS_OBJECT_LIMIT];
    uint64_t count = 0;
    uint64_t current = current_object;
    uint64_t position = 0;

    current_path[0] = '\0';

    if (current == 0) {
        current_path[0] = '>';
        current_path[1] = '\0';
        return;
    }

    while (current != 0 &&
           count < TFS_OBJECT_LIMIT) {
        chain[count] = current;
        count++;

        current =
            objects[current].parent;
    }

    current_path[position] = '>';
    position++;

    while (count > 0) {
        const char *name;
        uint64_t length;

        count--;

        name =
            objects[chain[count]].name;

        length =
            tfs_string_length(name);

        if (position + length + 1 >=
            TFS_PATH_SIZE) {
            current_path[0] = '>';
            current_path[1] = '\0';
            return;
        }

        for (uint64_t i = 0;
             i < length;
             i++) {
            current_path[position++] =
                name[i];
        }

        if (count > 0) {
            current_path[position++] =
                '>';
        }
    }

    current_path[position] = '\0';
}

void tfs_start(void)
{
    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        objects[i].id = i;
        objects[i].parent = 0;
        objects[i].type = TFS_DATA;
        objects[i].size = 0;
        objects[i].name[0] = '\0';
        objects[i].active = 0;

        for (uint64_t j = 0;
             j < TFS_DATA_SIZE;
             j++) {
            objects[i].data[j] = 0;
        }
    }

    object_count = 1;
    current_object = 0;

    objects[0].id = 0;
    objects[0].parent = 0;
    objects[0].type = TFS_CONTAINER;
    objects[0].size = 0;
    objects[0].active = 1;

    tfs_string_copy(
        objects[0].name,
        TFS_NAME_SIZE,
        ""
    );

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

    tfs_update_current_path();
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

uint64_t tfs_create(
    uint64_t parent,
    const char *name,
    tfs_object_type type
)
{
    uint64_t id;

    if (name == 0) {
        return UINT64_MAX;
    }

    if (parent >= TFS_OBJECT_LIMIT) {
        return UINT64_MAX;
    }

    if (!objects[parent].active) {
        return UINT64_MAX;
    }

    if (type != TFS_CONTAINER &&
        type != TFS_DATA) {
        return UINT64_MAX;
    }

    if (tfs_string_length(name) >=
        TFS_NAME_SIZE) {
        return UINT64_MAX;
    }

    if (tfs_find(parent, name) != UINT64_MAX) {
        return UINT64_MAX;
    }

    if (object_count >= TFS_OBJECT_LIMIT) {
        return UINT64_MAX;
    }

    id = 0;

    while (id < TFS_OBJECT_LIMIT) {
        if (!objects[id].active) {
            break;
        }

        id++;
    }

    if (id >= TFS_OBJECT_LIMIT) {
        return UINT64_MAX;
    }

    objects[id].id = id;
    objects[id].parent = parent;
    objects[id].type = type;
    objects[id].size = 0;
    objects[id].active = 1;

    tfs_string_copy(
        objects[id].name,
        TFS_NAME_SIZE,
        name
    );

    for (uint64_t i = 0;
         i < TFS_DATA_SIZE;
         i++) {
        objects[id].data[i] = 0;
    }

    object_count++;

    return id;
}

uint64_t tfs_find(
    uint64_t parent,
    const char *name
)
{
    if (parent >= TFS_OBJECT_LIMIT ||
        name == 0) {
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

        if (tfs_string_equal(
                objects[i].name,
                name
            )) {
            return i;
        }
    }

    return UINT64_MAX;
}

uint64_t tfs_find_path(
    const char *path
)
{
    uint64_t current;
    uint64_t position = 0;
    char component[TFS_NAME_SIZE];

    if (path == 0 ||
        path[0] != '>') {
        return UINT64_MAX;
    }

    if (path[1] == '\0') {
        return 0;
    }

    current = 0;

    while (1) {
        uint64_t length =
            tfs_next_component(
                path,
                &position,
                component,
                sizeof(component)
            );

        if (length == 0) {
            break;
        }

        current =
            tfs_find(
                current,
                component
            );

        if (current == UINT64_MAX) {
            return UINT64_MAX;
        }

        if (path[position] == '\0') {
            break;
        }

        position++;
    }

    return current;
}

uint64_t tfs_create_path(
    const char *path,
    tfs_object_type type
)
{
    uint64_t current = 0;
    uint64_t position = 0;
    char component[TFS_NAME_SIZE];

    if (path == 0 ||
        path[0] != '>') {
        return UINT64_MAX;
    }

    if (path[1] == '\0') {
        return type == TFS_CONTAINER
            ? 0
            : UINT64_MAX;
    }

    while (1) {
        uint64_t length =
            tfs_next_component(
                path,
                &position,
                component,
                sizeof(component)
            );

        if (length == 0) {
            break;
        }

        uint64_t child =
            tfs_find(
                current,
                component
            );

        if (child == UINT64_MAX) {
            if (path[position] != '\0') {
                return UINT64_MAX;
            }

            child =
                tfs_create(
                    current,
                    component,
                    type
                );

            return child;
        }

        current = child;

        if (path[position] == '\0') {
            return current;
        }

        if (objects[current].type !=
            TFS_CONTAINER) {
            return UINT64_MAX;
        }

        position++;
    }

    return current;
}

int tfs_remove_path(
    const char *path
)
{
    uint64_t id =
        tfs_resolve_path(path);

    if (id == UINT64_MAX ||
        id == 0) {
        return 0;
    }

    if (id == current_object) {
        return 0;
    }

    if (tfs_child_count(id) != 0) {
        return 0;
    }

    objects[id].active = 0;
    objects[id].size = 0;
    objects[id].name[0] = '\0';

    object_count--;

    return 1;
}

uint64_t tfs_child_count(
    uint64_t parent
)
{
    uint64_t count = 0;

    if (parent >= TFS_OBJECT_LIMIT) {
        return 0;
    }

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        if (!objects[i].active) {
            continue;
        }

        if (objects[i].parent == parent) {
            count++;
        }
    }

    return count;
}

const tfs_object *tfs_child(
    uint64_t parent,
    uint64_t index
)
{
    uint64_t count = 0;

    if (parent >= TFS_OBJECT_LIMIT) {
        return 0;
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

        if (count == index) {
            return &objects[i];
        }

        count++;
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
    uint64_t count;

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

    count = objects[id].size;

    if (count > size) {
        count = size;
    }

    for (uint64_t i = 0;
         i < count;
         i++) {
        data[i] = objects[id].data[i];
    }

    return count;
}

uint64_t tfs_current(void)
{
    return current_object;
}

int tfs_change_directory(
    const char *path
)
{
    uint64_t id =
        tfs_resolve_path(path);

    if (id == UINT64_MAX) {
        return 0;
    }

    if (objects[id].type !=
        TFS_CONTAINER) {
        return 0;
    }

    current_object = id;

    tfs_update_current_path();

    return 1;
}

int tfs_exit_directory(void)
{
    if (current_object == 0) {
        return 0;
    }

    current_object =
        objects[current_object].parent;

    tfs_update_current_path();

    return 1;
}

const char *tfs_current_path(void)
{
    return current_path;
}

uint64_t tfs_storage_export(
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

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        uint64_t offset =
            i * TFS_STORAGE_RECORD_SIZE;

        if (!objects[i].active) {
            for (uint64_t j = 0;
                 j < 8;
                 j++) {
                buffer[offset + 16 + j] =
                    0xFF;
            }

            continue;
        }

        for (uint64_t j = 0;
             j < 8;
             j++) {
            buffer[offset + j] =
                (objects[i].id >>
                 (j * 8)) & 0xFF;

            buffer[offset + 8 + j] =
                (objects[i].parent >>
                 (j * 8)) & 0xFF;

            buffer[offset + 16 + j] =
                ((uint64_t)objects[i].type >>
                 (j * 8)) & 0xFF;

            buffer[offset + 24 + j] =
                (objects[i].size >>
                 (j * 8)) & 0xFF;
        }

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
    }

    return TFS_STORAGE_SIZE;
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

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        uint64_t offset =
            i * TFS_STORAGE_RECORD_SIZE;

        uint64_t id = 0;
        uint64_t parent = 0;
        uint64_t type = 0;
        uint64_t object_size = 0;

        for (uint64_t j = 0;
             j < 8;
             j++) {
            id |=
                ((uint64_t)buffer[
                    offset + j
                ]) << (j * 8);

            parent |=
                ((uint64_t)buffer[
                    offset + 8 + j
                ]) << (j * 8);

            type |=
                ((uint64_t)buffer[
                    offset + 16 + j
                ]) << (j * 8);

            object_size |=
                ((uint64_t)buffer[
                    offset + 24 + j
                ]) << (j * 8);
        }

        if (type == UINT64_MAX) {
            objects[i].active = 0;
            objects[i].id = i;
            objects[i].parent = 0;
            objects[i].type = TFS_DATA;
            objects[i].size = 0;
            objects[i].name[0] = '\0';
            continue;
        }

        if (id != i) {
            return 0;
        }

        if (parent >= TFS_OBJECT_LIMIT) {
            return 0;
        }

        if (type != TFS_CONTAINER &&
            type != TFS_DATA) {
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
        objects[i].active = 1;

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
                buffer[offset + 64 + j];
        }
    }

    if (!objects[0].active ||
        objects[0].id != 0 ||
        objects[0].parent != 0 ||
        objects[0].type != TFS_CONTAINER) {
        return 0;
    }

    object_count = 0;

    for (uint64_t i = 0;
         i < TFS_OBJECT_LIMIT;
         i++) {
        if (objects[i].active) {
            object_count++;
        }
    }

    current_object = 0;

    tfs_update_current_path();

    return 1;
}
