#ifndef RAW_HTTP_SERVER_CHUNKED_PARSER_H
#define RAW_HTTP_SERVER_CHUNKED_PARSER_H

#include "header_parser.h"

#include <stddef.h>

enum chunked_parser_state {
    CHUNKED_PARSER_INVALID = 0,
    CHUNKED_PARSER_INCOMPLETE = 1,
    CHUNKED_PARSER_COMPLETE = 2,
};

struct chunked_parser;

struct chunked_parser *chunked_parser_new(void);
int chunked_parser_feed(struct chunked_parser *parser,
                        const unsigned char *bytes, size_t length);
enum chunked_parser_state chunked_parser_state(const struct chunked_parser *parser);
const unsigned char *chunked_parser_body(const struct chunked_parser *parser,
                                         size_t *length);
const struct header_field *chunked_parser_trailer_at(
    const struct chunked_parser *parser, size_t index);
const unsigned char *chunked_parser_remaining(const struct chunked_parser *parser,
                                              size_t *length);
void chunked_parser_free(struct chunked_parser *parser);

#endif
