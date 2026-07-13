#include "fixture.h"
#include "request_parser.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
    static const char *const names[] = {
        "incomplete-head",       "complete-head",
        "complete-head-whole",   "complete-head-one-byte",
        "complete-head-fixed",   "bare-lf",
    };
    size_t case_index;

    assert(argc == 2);
    for (case_index = 0; case_index < sizeof(names) / sizeof(names[0]);
         case_index++) {
        struct fixture fixture;
        struct request_parser parser;
        size_t *chunks = NULL;
        size_t chunk_count = 0;
        size_t chunk_index;
        size_t offset = 0;
        enum request_state state = REQUEST_INCOMPLETE;
        char actual[32];
        int length;

        assert(fixture_load(argv[1], "requests", names[case_index],
                            &fixture) == 0);
        assert(fixture_load_chunk_sizes(argv[1], "requests", names[case_index],
                                        fixture.input_len, &chunks,
                                        &chunk_count) == 0);

        request_parser_init(&parser);
        for (chunk_index = 0; chunk_index < chunk_count; chunk_index++) {
            state = request_parser_feed(&parser, fixture.input + offset,
                                        chunks[chunk_index]);
            offset += chunks[chunk_index];
        }

        length = snprintf(actual, sizeof(actual), "%s\n",
                          request_state_name(state));
        assert(length > 0 && (size_t)length < sizeof(actual));
        assert((size_t)length == fixture.expected_len);
        assert(memcmp(actual, fixture.expected, fixture.expected_len) == 0);

        free(chunks);
        fixture_free(&fixture);
    }
    return 0;
}
