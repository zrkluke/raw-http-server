#include "body_parser.h"
#include "fixture.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static size_t trim_line_end(const unsigned char *bytes, size_t length) {
    while (length > 0 && (bytes[length - 1] == '\n' || bytes[length - 1] == '\r')) {
        length--;
    }
    return length;
}

static enum body_parser_state expected_state(const unsigned char *bytes,
                                             size_t length) {
    length = trim_line_end(bytes, length);
    if (length == strlen("invalid") && memcmp(bytes, "invalid", length) == 0) {
        return BODY_PARSER_INVALID;
    }
    if (length == strlen("incomplete") && memcmp(bytes, "incomplete", length) == 0) {
        return BODY_PARSER_INCOMPLETE;
    }
    assert(length == strlen("complete"));
    assert(memcmp(bytes, "complete", length) == 0);
    return BODY_PARSER_COMPLETE;
}

int main(int argc, char **argv) {
    static const char *const names[] = {
        "absent", "complete", "partial", "invalid-length", "over-delivered",
    };
    size_t case_index;

    assert(argc == 2);
    for (case_index = 0; case_index < sizeof(names) / sizeof(names[0]); case_index++) {
        struct fixture fixture;
        struct body_parser parser;
        unsigned char *content_length = NULL;
        unsigned char *expected_state_bytes = NULL;
        unsigned char *expected_remaining = NULL;
        size_t content_length_length = 0;
        size_t expected_state_length = 0;
        size_t expected_remaining_length = 0;
        size_t *chunks = NULL;
        size_t chunk_count = 0;
        size_t chunk_index;
        size_t offset = 0;
        size_t actual_length = 0;
        const unsigned char *actual;
        int content_length_present;

        assert(fixture_load(argv[1], "bodies", names[case_index], &fixture) == 0);
        assert(fixture_load_named(argv[1], "bodies", names[case_index],
                                  "content-length.txt", &content_length,
                                  &content_length_length) == 0);
        assert(fixture_load_named(argv[1], "bodies", names[case_index], "state.txt",
                                  &expected_state_bytes, &expected_state_length) == 0);
        assert(fixture_load_named(argv[1], "bodies", names[case_index],
                                  "remaining.bin", &expected_remaining,
                                  &expected_remaining_length) == 0);
        assert(fixture_load_chunk_sizes(argv[1], "bodies", names[case_index],
                                        fixture.input_len, &chunks, &chunk_count) == 0);

        content_length_length = trim_line_end(content_length, content_length_length);
        content_length_present = !(content_length_length == strlen("absent") &&
                                   memcmp(content_length, "absent",
                                          content_length_length) == 0);
        assert(body_parser_init(&parser, content_length, content_length_length,
                                content_length_present) == 0);
        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
            assert(body_parser_feed(&parser, fixture.input + offset,
                                    chunks[chunk_index]) == 0);
            offset += chunks[chunk_index];
        }

        assert(body_parser_state(&parser) ==
               expected_state(expected_state_bytes, expected_state_length));
        actual = body_parser_body(&parser, &actual_length);
        assert(actual_length == fixture.expected_len);
        if (actual_length != 0) {
            assert(memcmp(actual, fixture.expected, actual_length) == 0);
        }
        actual = body_parser_remaining(&parser, &actual_length);
        assert(actual_length == expected_remaining_length);
        if (actual_length != 0) {
            assert(memcmp(actual, expected_remaining, actual_length) == 0);
        }

        body_parser_free(&parser);
        free(chunks);
        free(content_length);
        free(expected_state_bytes);
        free(expected_remaining);
        fixture_free(&fixture);
    }
    return 0;
}
