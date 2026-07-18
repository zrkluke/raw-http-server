#include "chunked_parser.h"

#include <assert.h>
#include <string.h>

static void rejects_bare_lf(void) {
    const unsigned char input[] = "1\n";
    struct chunked_parser *parser = chunked_parser_new();

    assert(parser != NULL);
    assert(chunked_parser_feed(parser, input, sizeof(input) - 1) == 0);
    assert(chunked_parser_state(parser) == CHUNKED_PARSER_INVALID);
    chunked_parser_free(parser);
}

static void keeps_bytes_after_terminal_chunk(void) {
    const unsigned char input[] = "0\r\n\r\nnext";
    const unsigned char *remaining;
    size_t length;
    struct chunked_parser *parser = chunked_parser_new();

    assert(parser != NULL);
    assert(chunked_parser_feed(parser, input, sizeof(input) - 1) == 0);
    assert(chunked_parser_state(parser) == CHUNKED_PARSER_COMPLETE);
    remaining = chunked_parser_remaining(parser, &length);
    assert(length == 4);
    assert(memcmp(remaining, "next", length) == 0);
    chunked_parser_free(parser);
}

int main(void) {
    rejects_bare_lf();
    keeps_bytes_after_terminal_chunk();
    return 0;
}
