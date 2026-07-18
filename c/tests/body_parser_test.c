#include "body_parser.h"

#include <assert.h>
#include <string.h>

int main(void) {
    static const unsigned char length[] = "4";
    static const unsigned char first[] = "ca";
    static const unsigned char second[] = "t\nnext\n";
    static const unsigned char invalid[] = "x";
    static const unsigned char bytes[] = "after\n";
    struct body_parser parser;
    const unsigned char *actual;
    size_t actual_length;

    assert(body_parser_init(&parser, length, sizeof(length) - 1, 1) == 0);
    assert(body_parser_feed(&parser, first, sizeof(first) - 1) == 0);
    assert(body_parser_state(&parser) == BODY_PARSER_INCOMPLETE);
    assert(body_parser_feed(&parser, second, sizeof(second) - 1) == 0);
    assert(body_parser_state(&parser) == BODY_PARSER_COMPLETE);
    actual = body_parser_body(&parser, &actual_length);
    assert(actual_length == strlen("cat\n"));
    assert(memcmp(actual, "cat\n", actual_length) == 0);
    actual = body_parser_remaining(&parser, &actual_length);
    assert(actual_length == strlen("next\n"));
    assert(memcmp(actual, "next\n", actual_length) == 0);
    body_parser_free(&parser);

    assert(body_parser_init(&parser, invalid, sizeof(invalid) - 1, 1) == 0);
    assert(body_parser_state(&parser) == BODY_PARSER_INVALID);
    assert(body_parser_feed(&parser, bytes, sizeof(bytes) - 1) == 0);
    actual = body_parser_remaining(&parser, &actual_length);
    assert(actual_length == sizeof(bytes) - 1);
    assert(memcmp(actual, bytes, actual_length) == 0);
    body_parser_free(&parser);

    assert(body_parser_init(&parser, NULL, 0, 0) == 0);
    assert(body_parser_state(&parser) == BODY_PARSER_COMPLETE);
    assert(body_parser_feed(&parser, bytes, sizeof(bytes) - 1) == 0);
    actual = body_parser_remaining(&parser, &actual_length);
    assert(actual_length == sizeof(bytes) - 1);
    assert(memcmp(actual, bytes, actual_length) == 0);
    body_parser_free(&parser);
    return 0;
}
