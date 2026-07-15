#include "fixture.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int read_file(const char *path, unsigned char **data, size_t *length) {
    FILE *file = fopen(path, "rb");
    long file_length;
    unsigned char *buffer;

    if (file == NULL) {
        return -1;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return -1;
    }
    file_length = ftell(file);
    if (file_length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    buffer = malloc(file_length == 0 ? 1 : (size_t)file_length);
    if (buffer == NULL) {
        fclose(file);
        return -1;
    }
    if (fread(buffer, 1, (size_t)file_length, file) != (size_t)file_length) {
        free(buffer);
        fclose(file);
        return -1;
    }
    if (fclose(file) != 0) {
        free(buffer);
        return -1;
    }

    *data = buffer;
    *length = (size_t)file_length;
    return 0;
}

int fixture_load(const char *root, const char *group, const char *name,
                 struct fixture *fixture) {
    char input_path[1024];
    char expected_path[1024];

    *fixture = (struct fixture){0};
    if (snprintf(input_path, sizeof(input_path), "%s/%s/%s/input.bin", root,
                 group, name) >= (int)sizeof(input_path) ||
        snprintf(expected_path, sizeof(expected_path),
                 "%s/%s/%s/expected.bin", root, group,
                 name) >= (int)sizeof(expected_path)) {
        return -1;
    }
    if (read_file(input_path, &fixture->input, &fixture->input_len) != 0) {
        return -1;
    }
    if (read_file(expected_path, &fixture->expected,
                  &fixture->expected_len) != 0) {
        fixture_free(fixture);
        return -1;
    }
    return 0;
}

int fixture_load_named(const char *root, const char *group, const char *name,
                       const char *filename, unsigned char **data,
                       size_t *length) {
    char path[1024];

    if (snprintf(path, sizeof(path), "%s/%s/%s/%s", root, group, name,
                 filename) >= (int)sizeof(path)) {
        return -1;
    }
    return read_file(path, data, length);
}

int fixture_load_chunk_sizes(const char *root, const char *group,
                             const char *name, size_t input_length,
                             size_t **chunks, size_t *chunk_count) {
    char path[1024];
    unsigned char *data = NULL;
    size_t data_length = 0;
    size_t *sizes = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t total = 0;
    size_t index = 0;

    *chunks = NULL;
    *chunk_count = 0;
    if (snprintf(path, sizeof(path), "%s/%s/%s/chunks.txt", root, group,
                 name) >= (int)sizeof(path) ||
        read_file(path, &data, &data_length) != 0) {
        return -1;
    }

    while (index < data_length) {
        size_t size = 0;
        size_t *grown;

        while (index < data_length && isspace(data[index])) {
            index++;
        }
        if (index == data_length) {
            break;
        }
        if (!isdigit(data[index])) {
            free(data);
            free(sizes);
            return -1;
        }
        while (index < data_length && isdigit(data[index])) {
            unsigned char digit = (unsigned char)(data[index] - '0');

            if (size > (SIZE_MAX - digit) / 10) {
                free(data);
                free(sizes);
                return -1;
            }
            size = size * 10 + digit;
            index++;
        }
        if (size == 0 || total > SIZE_MAX - size) {
            free(data);
            free(sizes);
            return -1;
        }
        if (count == capacity) {
            capacity = capacity == 0 ? 4 : capacity * 2;
            grown = realloc(sizes, capacity * sizeof(*sizes));
            if (grown == NULL) {
                free(data);
                free(sizes);
                return -1;
            }
            sizes = grown;
        }
        sizes[count++] = size;
        total += size;
    }

    free(data);
    if (count == 0 || total != input_length) {
        free(sizes);
        return -1;
    }

    *chunks = sizes;
    *chunk_count = count;
    return 0;
}

void fixture_free(struct fixture *fixture) {
    free(fixture->input);
    free(fixture->expected);
    *fixture = (struct fixture){0};
}
