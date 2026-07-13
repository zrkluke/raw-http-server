#ifndef RAW_HTTP_SERVER_REQUEST_PARSER_H
#define RAW_HTTP_SERVER_REQUEST_PARSER_H

#include <stddef.h>

enum request_state {
    REQUEST_INCOMPLETE,
    REQUEST_COMPLETE,
    REQUEST_INVALID,
};

struct request_parser {
    enum request_state state;
    int saw_first_line;
    int line_has_bytes;
    int pending_cr;
};

void request_parser_init(struct request_parser *parser);
enum request_state request_parser_feed(struct request_parser *parser,
                                       const unsigned char *chunk,
                                       size_t chunk_length);
const char *request_state_name(enum request_state state);

#endif
