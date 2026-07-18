#ifndef RAW_HTTP_SERVER_BODY_PARSER_H
#define RAW_HTTP_SERVER_BODY_PARSER_H

#include <stddef.h>

enum body_parser_state {
    BODY_PARSER_INVALID = 0,
    BODY_PARSER_INCOMPLETE = 1,
    BODY_PARSER_COMPLETE = 2,
};

struct body_parser {
    enum body_parser_state state;
    size_t content_length;
    unsigned char *body;
    size_t body_length;
    size_t body_capacity;
    unsigned char *remaining;
    size_t remaining_length;
    size_t remaining_capacity;
};

int body_parser_init(struct body_parser *parser,
                     const unsigned char *content_length,
                     size_t content_length_length, int content_length_present);
int body_parser_feed(struct body_parser *parser, const unsigned char *bytes,
                     size_t length);
enum body_parser_state body_parser_state(const struct body_parser *parser);
const unsigned char *body_parser_body(const struct body_parser *parser,
                                      size_t *length);
const unsigned char *body_parser_remaining(const struct body_parser *parser,
                                           size_t *length);
void body_parser_free(struct body_parser *parser);

#endif
