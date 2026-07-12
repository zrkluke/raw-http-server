#ifndef RAW_HTTP_SERVER_LINE_READER_H
#define RAW_HTTP_SERVER_LINE_READER_H

#include <stddef.h>

struct line_reader {
    unsigned char *pending;
    size_t pending_length;
    size_t pending_capacity;
};

typedef int (*line_reader_emit_fn)(void *context, const unsigned char *line,
                                   size_t line_length);

void line_reader_init(struct line_reader *reader);
void line_reader_free(struct line_reader *reader);

// The line pointer is valid only for the duration of the emit callback.
int line_reader_feed(struct line_reader *reader, const unsigned char *chunk,
                     size_t chunk_length, line_reader_emit_fn emit,
                     void *context);

#endif
