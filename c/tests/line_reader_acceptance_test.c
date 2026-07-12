#include "fixture.h"
#include "line_reader.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct captured_lines {
    unsigned char bytes[1024];
    size_t length;
};

static int capture_line(void *context, const unsigned char *line,
                        size_t line_length) {
    struct captured_lines *captured = context;

    if (captured->length + line_length + 1 > sizeof(captured->bytes)) {
        return -1;
    }
    memcpy(captured->bytes + captured->length, line, line_length);
    captured->length += line_length;
    captured->bytes[captured->length++] = '\n';
    return 0;
}

int main(int argc, char **argv) {
    static const char *const names[] = {"complete-line", "crlf-split",
                                        "one-byte"};
    size_t case_index;

    assert(argc == 2);
    for (case_index = 0; case_index < sizeof(names) / sizeof(names[0]);
         case_index++) {
        struct fixture fixture;
        struct line_reader reader;
        struct captured_lines captured = {0};
        size_t *chunks = NULL;
        size_t chunk_count = 0;
        size_t chunk_index;
        size_t offset = 0;

        assert(fixture_load(argv[1], "http-streams", names[case_index],
                            &fixture) == 0);
        assert(fixture_load_chunk_sizes(argv[1], "http-streams",
                                        names[case_index], fixture.input_len,
                                        &chunks, &chunk_count) == 0);
        line_reader_init(&reader);
        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
            assert(line_reader_feed(&reader, fixture.input + offset,
                                    chunks[chunk_index], capture_line,
                                    &captured) == 0);
            offset += chunks[chunk_index];
        }

        assert(captured.length == fixture.expected_len);
        assert(memcmp(captured.bytes, fixture.expected, captured.length) == 0);
        line_reader_free(&reader);
        free(chunks);
        fixture_free(&fixture);
    }
    return 0;
}
