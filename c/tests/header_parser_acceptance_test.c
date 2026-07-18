#include "fixture.h"
#include "header_parser.h"

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

static int capture_result(const struct header_parser *parser,
                          struct captured_result *captured) {
    static const unsigned char incomplete[] = "incomplete\n";
    static const unsigned char invalid[] = "invalid\n";
    static const unsigned char complete[] = "complete\n";
    enum header_parser_state state = header_parser_state(parser);
    size_t index;

    if (state == HEADER_PARSER_INCOMPLETE) {
        return append_bytes(captured, incomplete, sizeof(incomplete) - 1);
    }
    if (state == HEADER_PARSER_INVALID) {
        return append_bytes(captured, invalid, sizeof(invalid) - 1);
    }
    if (append_bytes(captured, complete, sizeof(complete) - 1) != 0) {
        return -1;
    }
    for (index = 0;; index++) {
        const struct header_field *field = header_parser_field_at(parser, index);

        if (field == NULL) {
            break;
        }
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
        "valid-basic",        "repeated-name",  "case-insensitive",
        "optional-whitespace", "empty-section", "fragmented",
        "incomplete-value",  "missing-colon",  "leading-whitespace",
        "bare-lf",            "bare-cr",
    };
    size_t case_index;

    assert(argc == 2);
    for (case_index = 0; case_index < sizeof(names) / sizeof(names[0]);
         case_index++) {
        struct fixture fixture;
        struct header_parser parser;
        struct captured_result captured = {0};
        size_t *chunks = NULL;
        size_t chunk_count = 0;
        size_t chunk_index;
        size_t offset = 0;

        assert(fixture_load(argv[1], "headers", names[case_index], &fixture) ==
               0);
        assert(fixture_load_chunk_sizes(argv[1], "headers", names[case_index],
                                        fixture.input_len, &chunks,
                                        &chunk_count) == 0);
        header_parser_init(&parser);
        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
            assert(header_parser_feed(&parser, fixture.input + offset,
                                      chunks[chunk_index]) == 0);
            offset += chunks[chunk_index];
        }
        assert(capture_result(&parser, &captured) == 0);
        assert(captured.length == fixture.expected_len);
        assert(memcmp(captured.bytes, fixture.expected, captured.length) == 0);

        header_parser_free(&parser);
        free(chunks);
        fixture_free(&fixture);
    }
    return 0;
}
