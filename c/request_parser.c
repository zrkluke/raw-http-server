#include "request_parser.h"

void request_parser_init(struct request_parser *parser) {
    *parser = (struct request_parser){.state = REQUEST_INCOMPLETE};
}

enum request_state request_parser_feed(struct request_parser *parser,
                                       const unsigned char *chunk,
                                       size_t chunk_length) {
    size_t index;

    if (parser == NULL || (chunk == NULL && chunk_length > 0)) {
        return REQUEST_INVALID;
    }
    if (parser->state != REQUEST_INCOMPLETE) {
        return parser->state;
    }

    for (index = 0; index < chunk_length; index++) {
        unsigned char byte = chunk[index];

        if (parser->pending_cr) {
            if (byte != '\n') {
                parser->state = REQUEST_INVALID;
                return parser->state;
            }
            parser->pending_cr = 0;
            if (!parser->saw_first_line) {
                if (!parser->line_has_bytes) {
                    parser->state = REQUEST_INVALID;
                    return parser->state;
                }
                parser->saw_first_line = 1;
            } else if (!parser->line_has_bytes) {
                parser->state = REQUEST_COMPLETE;
                return parser->state;
            }
            parser->line_has_bytes = 0;
            continue;
        }

        if (byte == '\r') {
            parser->pending_cr = 1;
        } else if (byte == '\n') {
            parser->state = REQUEST_INVALID;
            return parser->state;
        } else {
            parser->line_has_bytes = 1;
        }
    }

    return parser->state;
}

const char *request_state_name(enum request_state state) {
    switch (state) {
    case REQUEST_COMPLETE:
        return "complete";
    case REQUEST_INVALID:
        return "invalid";
    case REQUEST_INCOMPLETE:
    default:
        return "incomplete";
    }
}
