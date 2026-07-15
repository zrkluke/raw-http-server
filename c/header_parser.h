#ifndef RAW_HTTP_SERVER_HEADER_PARSER_H
#define RAW_HTTP_SERVER_HEADER_PARSER_H

#include <stddef.h>

enum header_parser_state {
    HEADER_PARSER_INVALID = 0,
    HEADER_PARSER_INCOMPLETE = 1,
    HEADER_PARSER_COMPLETE = 2,
};

struct header_field {
    const unsigned char *name;
    size_t name_length;
    const unsigned char *value;
    size_t value_length;
};

struct header_parser {
    unsigned char *line;
    size_t line_length;
    size_t line_capacity;
    int pending_cr;
    enum header_parser_state state;
    struct header_field *fields;
    size_t field_count;
    size_t field_capacity;
};

void header_parser_init(struct header_parser *parser);
int header_parser_feed(struct header_parser *parser,
                       const unsigned char *bytes, size_t length);
enum header_parser_state header_parser_state(const struct header_parser *parser);
const struct header_field *header_parser_field_at(
    const struct header_parser *parser, size_t index);
void header_parser_free(struct header_parser *parser);

#endif
