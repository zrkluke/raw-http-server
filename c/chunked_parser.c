#include "chunked_parser.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum chunked_parser_phase {
    CHUNKED_PHASE_SIZE,
    CHUNKED_PHASE_SIZE_LF,
    CHUNKED_PHASE_DATA,
    CHUNKED_PHASE_DATA_CR,
    CHUNKED_PHASE_DATA_LF,
    CHUNKED_PHASE_TRAILERS,
};

struct chunked_parser {
    unsigned char *line;
    size_t line_length;
    size_t line_capacity;
    size_t chunk_remaining;
    unsigned char *body;
    size_t body_length;
    size_t body_capacity;
    unsigned char *remaining;
    size_t remaining_length;
    size_t remaining_capacity;
    struct header_parser trailers;
    enum chunked_parser_phase phase;
    enum chunked_parser_state state;
};

static int reserve_bytes(unsigned char **bytes, size_t *capacity,
                         size_t required) {
    size_t grown_capacity;
    unsigned char *grown;

    if (required <= *capacity) return 0;
    grown_capacity = *capacity == 0 ? 32 : *capacity;
    while (grown_capacity < required) {
        if (grown_capacity > SIZE_MAX / 2) {
            grown_capacity = required;
            break;
        }
        grown_capacity *= 2;
    }
    grown = realloc(*bytes, grown_capacity);
    if (grown == NULL) return -1;
    *bytes = grown;
    *capacity = grown_capacity;
    return 0;
}

static int append_byte(unsigned char **bytes, size_t *length,
                       size_t *capacity, unsigned char byte) {
    if (*length == SIZE_MAX) return -1;
    if (reserve_bytes(bytes, capacity, *length + 1) != 0) return -1;
    (*bytes)[*length] = byte;
    *length += 1;
    return 0;
}

static void mark_invalid(struct chunked_parser *parser) {
    parser->state = CHUNKED_PARSER_INVALID;
}

/* Returns 1 for a valid chunk size, 0 for invalid syntax. */
static int parse_chunk_size(struct chunked_parser *parser, size_t *size) {
    size_t index;
    size_t value = 0;

    if (parser->line_length == 0) return 0;
    for (index = 0; index < parser->line_length; index += 1) {
        unsigned char byte = parser->line[index];
        size_t digit;

        if (byte >= '0' && byte <= '9') {
            digit = (size_t)(byte - '0');
        } else if (byte >= 'a' && byte <= 'f') {
            digit = (size_t)(byte - 'a' + 10);
        } else if (byte >= 'A' && byte <= 'F') {
            digit = (size_t)(byte - 'A' + 10);
        } else {
            return 0;
        }
        if (value > (SIZE_MAX - digit) / 16) return 0;
        value = value * 16 + digit;
    }
    *size = value;
    return 1;
}

static int feed_trailer_byte(struct chunked_parser *parser,
                             unsigned char byte) {
    enum header_parser_state state;

    state = header_parser_state(&parser->trailers);
    if (state == HEADER_PARSER_INVALID) {
        mark_invalid(parser);
        return 0;
    }
    if (state == HEADER_PARSER_COMPLETE) {
        return append_byte(&parser->remaining, &parser->remaining_length,
                           &parser->remaining_capacity, byte);
    }
    if (header_parser_feed(&parser->trailers, &byte, 1) != 0) return -1;
    state = header_parser_state(&parser->trailers);
    if (state == HEADER_PARSER_INVALID) mark_invalid(parser);
    if (state == HEADER_PARSER_COMPLETE) parser->state = CHUNKED_PARSER_COMPLETE;
    return 0;
}

struct chunked_parser *chunked_parser_new(void) {
    struct chunked_parser *parser = calloc(1, sizeof(*parser));

    if (parser == NULL) return NULL;
    parser->phase = CHUNKED_PHASE_SIZE;
    parser->state = CHUNKED_PARSER_INCOMPLETE;
    header_parser_init(&parser->trailers);
    return parser;
}

int chunked_parser_feed(struct chunked_parser *parser,
                        const unsigned char *bytes, size_t length) {
    size_t index;

    if (parser == NULL || (bytes == NULL && length != 0)) return -1;
    for (index = 0; index < length; index += 1) {
        unsigned char byte = bytes[index];

        if (parser->state == CHUNKED_PARSER_INVALID) return 0;
        if (parser->state == CHUNKED_PARSER_COMPLETE) {
            if (append_byte(&parser->remaining, &parser->remaining_length,
                            &parser->remaining_capacity, byte) != 0) {
                return -1;
            }
            continue;
        }

        switch (parser->phase) {
            case CHUNKED_PHASE_SIZE:
                if (byte == '\r') {
                    parser->phase = CHUNKED_PHASE_SIZE_LF;
                } else if (byte == '\n') {
                    mark_invalid(parser);
                } else if (append_byte(&parser->line, &parser->line_length,
                                       &parser->line_capacity, byte) != 0) {
                    return -1;
                }
                break;
            case CHUNKED_PHASE_SIZE_LF: {
                size_t size;

                if (byte != '\n' || !parse_chunk_size(parser, &size)) {
                    mark_invalid(parser);
                    break;
                }
                parser->line_length = 0;
                parser->chunk_remaining = size;
                parser->phase = size == 0 ? CHUNKED_PHASE_TRAILERS
                                          : CHUNKED_PHASE_DATA;
                break;
            }
            case CHUNKED_PHASE_DATA:
                if (append_byte(&parser->body, &parser->body_length,
                                &parser->body_capacity, byte) != 0) {
                    return -1;
                }
                parser->chunk_remaining -= 1;
                if (parser->chunk_remaining == 0) {
                    parser->phase = CHUNKED_PHASE_DATA_CR;
                }
                break;
            case CHUNKED_PHASE_DATA_CR:
                if (byte != '\r') {
                    mark_invalid(parser);
                } else {
                    parser->phase = CHUNKED_PHASE_DATA_LF;
                }
                break;
            case CHUNKED_PHASE_DATA_LF:
                if (byte != '\n') {
                    mark_invalid(parser);
                } else {
                    parser->phase = CHUNKED_PHASE_SIZE;
                }
                break;
            case CHUNKED_PHASE_TRAILERS:
                if (feed_trailer_byte(parser, byte) != 0) return -1;
                break;
        }
    }
    return 0;
}

enum chunked_parser_state chunked_parser_state(
    const struct chunked_parser *parser) {
    return parser == NULL ? CHUNKED_PARSER_INVALID : parser->state;
}

const unsigned char *chunked_parser_body(const struct chunked_parser *parser,
                                         size_t *length) {
    if (length != NULL) *length = parser == NULL ? 0 : parser->body_length;
    return parser == NULL ? NULL : parser->body;
}

const struct header_field *chunked_parser_trailer_at(
    const struct chunked_parser *parser, size_t index) {
    if (parser == NULL) return NULL;
    return header_parser_field_at(&parser->trailers, index);
}

const unsigned char *chunked_parser_remaining(
    const struct chunked_parser *parser, size_t *length) {
    if (length != NULL) *length = parser == NULL ? 0 : parser->remaining_length;
    return parser == NULL ? NULL : parser->remaining;
}

void chunked_parser_free(struct chunked_parser *parser) {
    if (parser == NULL) return;
    free(parser->line);
    free(parser->body);
    free(parser->remaining);
    header_parser_free(&parser->trailers);
    free(parser);
}
