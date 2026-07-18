#include "line_reader.h"

#include <assert.h>
#include <string.h>

struct captured_lines {
    unsigned char bytes[64];
    size_t length;
};

static int capture_line(void *context, const unsigned char *line,
                        size_t line_length) {
    struct captured_lines *captured = context;

    assert(captured->length + line_length + 1 <= sizeof(captured->bytes));
    memcpy(captured->bytes + captured->length, line, line_length);
    captured->length += line_length;
    captured->bytes[captured->length++] = '\n';
    return 0;
}

int main(void) {
    struct line_reader reader;
    struct captured_lines captured = {0};

    line_reader_init(&reader);
    assert(line_reader_feed(&reader, (const unsigned char *)"one\r\ntw", 7,
                            capture_line, &captured) == 0);
    assert(memcmp(captured.bytes, "one\n", captured.length) == 0);
    assert(line_reader_feed(&reader, (const unsigned char *)"o\r\n", 3,
                            capture_line, &captured) == 0);
    assert(captured.length == strlen("one\ntwo\n"));
    assert(memcmp(captured.bytes, "one\ntwo\n", captured.length) == 0);
    line_reader_free(&reader);

    captured = (struct captured_lines){0};
    line_reader_init(&reader);
    assert(line_reader_feed(&reader, (const unsigned char *)"\r\n", 2,
                            capture_line, &captured) == 0);
    assert(captured.length == 1);
    assert(captured.bytes[0] == '\n');
    line_reader_free(&reader);
    return 0;
}
