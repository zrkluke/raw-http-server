#ifndef RAW_HTTP_SERVER_FIXTURE_H
#define RAW_HTTP_SERVER_FIXTURE_H

#include <stddef.h>

struct fixture {
    unsigned char *input;
    size_t input_len;
    unsigned char *expected;
    size_t expected_len;
};

int fixture_load(const char *root, const char *group, const char *name,
                 struct fixture *fixture);
int fixture_load_chunk_sizes(const char *root, const char *group,
                             const char *name, size_t input_length,
                             size_t **chunks, size_t *chunk_count);
void fixture_free(struct fixture *fixture);

#endif
