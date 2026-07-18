#include "fixture.h"
#include "line_reader.h"
#include "request_line.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct captured_result {
    unsigned char bytes[1024];
    size_t length;
};

static int append_bytes(struct captured_result *captured,
                        const unsigned char *bytes, size_t length) {
    if (captured->length + length > sizeof(captured->bytes)) {
        return -1;
    }
    memcpy(captured->bytes + captured->length, bytes, length);
    captured->length += length;
    return 0;
}

static int capture_request_line(void *context, const unsigned char *line,
                                size_t line_length) {
    static const unsigned char invalid[] = "invalid\n";
    struct captured_result *captured = context;
    struct request_line parsed;

    if (request_line_parse(line, line_length, &parsed) != REQUEST_LINE_VALID) {
        return append_bytes(captured, invalid, sizeof(invalid) - 1);
    }
    if (append_bytes(captured, parsed.method, parsed.method_length) != 0 ||
        append_bytes(captured, (const unsigned char *)"|", 1) != 0 ||
        append_bytes(captured, parsed.target, parsed.target_length) != 0 ||
        append_bytes(captured, (const unsigned char *)"|", 1) != 0 ||
        append_bytes(captured, parsed.version, parsed.version_length) != 0 ||
        append_bytes(captured, (const unsigned char *)"\n", 1) != 0) {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    static const char *const names[] = {
        "valid-basic",       "valid-query",      "valid-crlf-boundary",
        "valid-field-split", "valid-fixed-four", "valid-one-byte",
        "missing-target",    "leading-space",    "trailing-space",
        "double-space",      "tab-separator",    "unsupported-version",
    };
    size_t case_index;

    assert(argc == 2);
    for (case_index = 0; case_index < sizeof(names) / sizeof(names[0]);
         case_index++) {
        struct fixture fixture;
        struct line_reader reader;
        struct captured_result captured = {0};
        size_t *chunks = NULL;
        size_t chunk_count = 0;
        size_t chunk_index;
        size_t offset = 0;

        assert(fixture_load(argv[1], "request-lines", names[case_index],
                            &fixture) == 0);
        assert(fixture_load_chunk_sizes(argv[1], "request-lines",
                                        names[case_index], fixture.input_len,
                                        &chunks, &chunk_count) == 0);
        line_reader_init(&reader);
        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
            assert(line_reader_feed(&reader, fixture.input + offset,
                                    chunks[chunk_index], capture_request_line,
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
