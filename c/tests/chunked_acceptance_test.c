#include "chunked_parser.h"
#include "fixture.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct captured_trailers {
    unsigned char bytes[1024];
    size_t length;
};

static size_t trim_line_end(const unsigned char *bytes, size_t length) {
    while (length > 0 && (bytes[length - 1] == '\n' || bytes[length - 1] == '\r')) {
        length -= 1;
    }
    return length;
}

static enum chunked_parser_state expected_state(const unsigned char *bytes,
                                                size_t length) {
    length = trim_line_end(bytes, length);
    if (length == strlen("invalid") && memcmp(bytes, "invalid", length) == 0) {
        return CHUNKED_PARSER_INVALID;
    }
    assert(length == strlen("complete"));
    assert(memcmp(bytes, "complete", length) == 0);
    return CHUNKED_PARSER_COMPLETE;
}

static int append_bytes(struct captured_trailers *captured,
                        const unsigned char *bytes, size_t length) {
    if (captured->length + length > sizeof(captured->bytes)) return -1;
    memcpy(captured->bytes + captured->length, bytes, length);
    captured->length += length;
    return 0;
}

static int capture_trailers(const struct chunked_parser *parser,
                            struct captured_trailers *captured) {
    size_t index;

    for (index = 0;; index += 1) {
        const struct header_field *field = chunked_parser_trailer_at(parser, index);

        if (field == NULL) break;
        if (append_bytes(captured, field->name, field->name_length) != 0 ||
            append_bytes(captured, (const unsigned char *)"|", 1) != 0 ||
            append_bytes(captured, field->value, field->value_length) != 0 ||
            append_bytes(captured, (const unsigned char *)"\n", 1) != 0) {
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
    static const char *const names[] = {
        "basic", "fragmented-trailers", "invalid-size", "missing-data-delimiter",
    };
    size_t case_index;

    assert(argc == 2);
    for (case_index = 0; case_index < sizeof(names) / sizeof(names[0]); case_index += 1) {
        struct fixture fixture;
        struct chunked_parser *parser;
        unsigned char *expected_state_bytes = NULL;
        unsigned char *expected_trailers = NULL;
        unsigned char *expected_remaining = NULL;
        size_t expected_state_length = 0;
        size_t expected_trailers_length = 0;
        size_t expected_remaining_length = 0;
        size_t *chunks = NULL;
        size_t chunk_count = 0;
        size_t chunk_index;
        size_t offset = 0;
        size_t actual_length = 0;
        const unsigned char *actual;
        struct captured_trailers trailers = {0};

        assert(fixture_load(argv[1], "chunked", names[case_index], &fixture) == 0);
        assert(fixture_load_named(argv[1], "chunked", names[case_index], "state.txt",
                                  &expected_state_bytes, &expected_state_length) == 0);
        assert(fixture_load_named(argv[1], "chunked", names[case_index], "trailers.txt",
                                  &expected_trailers, &expected_trailers_length) == 0);
        assert(fixture_load_named(argv[1], "chunked", names[case_index], "remaining.bin",
                                  &expected_remaining, &expected_remaining_length) == 0);
        assert(fixture_load_chunk_sizes(argv[1], "chunked", names[case_index],
                                        fixture.input_len, &chunks, &chunk_count) == 0);

        parser = chunked_parser_new();
        assert(parser != NULL);
        for (chunk_index = 0; chunk_index < chunk_count; chunk_index += 1) {
            assert(chunked_parser_feed(parser, fixture.input + offset,
                                       chunks[chunk_index]) == 0);
            offset += chunks[chunk_index];
        }

        assert(chunked_parser_state(parser) ==
               expected_state(expected_state_bytes, expected_state_length));
        actual = chunked_parser_body(parser, &actual_length);
        assert(actual_length == fixture.expected_len);
        if (actual_length != 0) assert(memcmp(actual, fixture.expected, actual_length) == 0);
        assert(capture_trailers(parser, &trailers) == 0);
        assert(trailers.length == expected_trailers_length);
        if (trailers.length != 0) {
            assert(memcmp(trailers.bytes, expected_trailers, trailers.length) == 0);
        }
        actual = chunked_parser_remaining(parser, &actual_length);
        assert(actual_length == expected_remaining_length);
        if (actual_length != 0) assert(memcmp(actual, expected_remaining, actual_length) == 0);

        chunked_parser_free(parser);
        free(chunks);
        free(expected_state_bytes);
        free(expected_trailers);
        free(expected_remaining);
        fixture_free(&fixture);
    }
    return 0;
}
