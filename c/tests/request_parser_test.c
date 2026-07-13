#include "request_parser.h"

#include <assert.h>

static void test_incomplete_head(void) {
    struct request_parser parser;

    request_parser_init(&parser);
    assert(request_parser_feed(
               &parser,
               (const unsigned char *)"GET / HTTP/1.1\r\nHost: example\r",
               30) == REQUEST_INCOMPLETE);
}

static void test_complete_head_across_crlf_boundaries(void) {
    struct request_parser parser;

    request_parser_init(&parser);
    assert(request_parser_feed(&parser, (const unsigned char *)"GET / HTTP/1.1\r",
                               15) == REQUEST_INCOMPLETE);
    assert(request_parser_feed(&parser,
                               (const unsigned char *)"\nHost: example\r\n\r",
                               17) == REQUEST_INCOMPLETE);
    assert(request_parser_feed(&parser, (const unsigned char *)"\n", 1) ==
           REQUEST_COMPLETE);
}

static void test_invalid_line_endings(void) {
    struct request_parser parser;

    request_parser_init(&parser);
    assert(request_parser_feed(&parser, (const unsigned char *)"GET / HTTP/1.1\n",
                               15) == REQUEST_INVALID);

    request_parser_init(&parser);
    assert(request_parser_feed(&parser, (const unsigned char *)"GET / HTTP/1.1\r",
                               15) == REQUEST_INCOMPLETE);
    assert(request_parser_feed(&parser, (const unsigned char *)"x", 1) ==
           REQUEST_INVALID);
}

static void test_terminal_state_is_preserved(void) {
    struct request_parser parser;

    request_parser_init(&parser);
    assert(request_parser_feed(&parser,
                               (const unsigned char *)"GET / HTTP/1.1\r\n\r\n",
                               18) == REQUEST_COMPLETE);
    assert(request_parser_feed(&parser, (const unsigned char *)"unexpected", 10) ==
           REQUEST_COMPLETE);
}

int main(void) {
    test_incomplete_head();
    test_complete_head_across_crlf_boundaries();
    test_invalid_line_endings();
    test_terminal_state_is_preserved();
    return 0;
}
