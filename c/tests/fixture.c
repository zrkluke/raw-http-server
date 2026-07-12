#include "fixture.h"

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

void fixture_free(struct fixture *fixture) {
    free(fixture->input);
    free(fixture->expected);
    *fixture = (struct fixture){0};
}
