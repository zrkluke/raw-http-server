#include "line_reader.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int line_reader_reserve(struct line_reader *reader, size_t required) {
    unsigned char *grown;
    size_t capacity;

    if (required <= reader->pending_capacity) {
        return 0;
    }

    capacity = reader->pending_capacity == 0 ? 64 : reader->pending_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }

    grown = realloc(reader->pending, capacity);
    if (grown == NULL) {
        return -1;
    }
    reader->pending = grown;
    reader->pending_capacity = capacity;
    return 0;
}

void line_reader_init(struct line_reader *reader) {
    *reader = (struct line_reader){0};
}

void line_reader_free(struct line_reader *reader) {
    free(reader->pending);
    *reader = (struct line_reader){0};
}

int line_reader_feed(struct line_reader *reader, const unsigned char *chunk,
                     size_t chunk_length, line_reader_emit_fn emit,
                     void *context) {
    size_t required;
    size_t index = 0;

    if (reader == NULL || emit == NULL || (chunk == NULL && chunk_length > 0)) {
        return -1;
    }
    if (reader->pending_length > SIZE_MAX - chunk_length) {
        return -1;
    }
    required = reader->pending_length + chunk_length;
    if (line_reader_reserve(reader, required) != 0) {
        return -1;
    }
    if (chunk_length > 0) {
        memcpy(reader->pending + reader->pending_length, chunk, chunk_length);
        reader->pending_length = required;
    }

    while (index + 1 < reader->pending_length) {
        size_t remaining;

        if (reader->pending[index] != '\r' || reader->pending[index + 1] != '\n') {
            index++;
            continue;
        }
        if (emit(context, reader->pending, index) != 0) {
            return -1;
        }
        remaining = reader->pending_length - index - 2;
        memmove(reader->pending, reader->pending + index + 2, remaining);
        reader->pending_length = remaining;
        index = 0;
    }

    return 0;
}
