#include "body_parser.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int reserve(unsigned char **bytes, size_t *capacity, size_t required) {
    size_t grown_capacity;
    unsigned char *grown;

    if (required <= *capacity) return 0;
    grown_capacity = *capacity == 0 ? 16 : *capacity;
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

static int append(unsigned char **destination, size_t *length,
                  size_t *capacity, const unsigned char *source,
                  size_t source_length) {
    size_t required;

    if (source_length == 0) return 0;
    if (*length > SIZE_MAX - source_length) return -1;
    required = *length + source_length;
    if (reserve(destination, capacity, required) != 0) return -1;
    memcpy(*destination + *length, source, source_length);
    *length = required;
    return 0;
}

static int parse_content_length(const unsigned char *value, size_t length,
                                size_t *result) {
    size_t index;
    size_t parsed = 0;

    if (length == 0) return -1;
    for (index = 0; index < length; index += 1) {
        unsigned char byte = value[index];
        unsigned char digit;

        if (byte < '0' || byte > '9') return -1;
        digit = (unsigned char)(byte - '0');
        if (parsed > (SIZE_MAX - digit) / 10) return -1;
        parsed = parsed * 10 + digit;
    }
    *result = parsed;
    return 0;
}

int body_parser_init(struct body_parser *parser,
                     const unsigned char *content_length,
                     size_t content_length_length, int content_length_present) {
    if (parser == NULL || (content_length_present && content_length == NULL)) {
        return -1;
    }

    *parser = (struct body_parser){.state = BODY_PARSER_COMPLETE};
    if (!content_length_present) return 0;
    if (parse_content_length(content_length, content_length_length,
                             &parser->content_length) != 0) {
        parser->state = BODY_PARSER_INVALID;
        return 0;
    }
    if (parser->content_length != 0) parser->state = BODY_PARSER_INCOMPLETE;
    return 0;
}

int body_parser_feed(struct body_parser *parser, const unsigned char *bytes,
                     size_t length) {
    size_t needed;
    size_t consume;

    if (parser == NULL || (bytes == NULL && length != 0)) return -1;
    if (parser->state != BODY_PARSER_INCOMPLETE) {
        return append(&parser->remaining, &parser->remaining_length,
                      &parser->remaining_capacity, bytes, length);
    }

    needed = parser->content_length - parser->body_length;
    consume = length < needed ? length : needed;
    if (append(&parser->body, &parser->body_length, &parser->body_capacity,
               bytes, consume) != 0 ||
        append(&parser->remaining, &parser->remaining_length,
               &parser->remaining_capacity,
               bytes == NULL ? NULL : bytes + consume,
               length - consume) != 0) {
        return -1;
    }
    if (parser->body_length == parser->content_length) {
        parser->state = BODY_PARSER_COMPLETE;
    }
    return 0;
}

enum body_parser_state body_parser_state(const struct body_parser *parser) {
    return parser == NULL ? BODY_PARSER_INVALID : parser->state;
}

const unsigned char *body_parser_body(const struct body_parser *parser,
                                      size_t *length) {
    if (length != NULL) *length = parser == NULL ? 0 : parser->body_length;
    return parser == NULL ? NULL : parser->body;
}

const unsigned char *body_parser_remaining(const struct body_parser *parser,
                                           size_t *length) {
    if (length != NULL) *length = parser == NULL ? 0 : parser->remaining_length;
    return parser == NULL ? NULL : parser->remaining;
}

void body_parser_free(struct body_parser *parser) {
    if (parser == NULL) return;
    free(parser->body);
    free(parser->remaining);
    *parser = (struct body_parser){0};
}
