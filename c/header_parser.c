#include "header_parser.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int reserve_line(struct header_parser *parser, size_t required) {
    size_t capacity;
    unsigned char *grown;

    if (required <= parser->line_capacity) return 0;
    capacity = parser->line_capacity == 0 ? 32 : parser->line_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }
    grown = realloc(parser->line, capacity);
    if (grown == NULL) return -1;
    parser->line = grown;
    parser->line_capacity = capacity;
    return 0;
}

static int reserve_fields(struct header_parser *parser, size_t required) {
    size_t capacity;
    struct header_field *grown;

    if (required <= parser->field_capacity) return 0;
    capacity = parser->field_capacity == 0 ? 4 : parser->field_capacity;
    while (capacity < required) {
        if (capacity > SIZE_MAX / 2) {
            capacity = required;
            break;
        }
        capacity *= 2;
    }
    if (capacity > SIZE_MAX / sizeof(*parser->fields)) return -1;
    grown = realloc(parser->fields, capacity * sizeof(*parser->fields));
    if (grown == NULL) return -1;
    parser->fields = grown;
    parser->field_capacity = capacity;
    return 0;
}

static int is_optional_whitespace(unsigned char byte) {
    return byte == ' ' || byte == '\t';
}

static unsigned char ascii_lower(unsigned char byte) {
    if (byte >= 'A' && byte <= 'Z') return (unsigned char)(byte + ('a' - 'A'));
    return byte;
}

static unsigned char *copy_bytes(const unsigned char *bytes, size_t length) {
    unsigned char *copy = malloc(length == 0 ? 1 : length);

    if (copy != NULL && length != 0) memcpy(copy, bytes, length);
    return copy;
}

static int append_field(struct header_parser *parser,
                        const unsigned char *name, size_t name_length,
                        const unsigned char *value, size_t value_length) {
    size_t index;
    unsigned char *name_copy;
    unsigned char *value_copy;

    if (parser->field_count == SIZE_MAX) return -1;
    name_copy = copy_bytes(name, name_length);
    if (name_copy == NULL) return -1;
    for (index = 0; index < name_length; index += 1) {
        name_copy[index] = ascii_lower(name_copy[index]);
    }
    value_copy = copy_bytes(value, value_length);
    if (value_copy == NULL) {
        free(name_copy);
        return -1;
    }
    if (reserve_fields(parser, parser->field_count + 1) != 0) {
        free(name_copy);
        free(value_copy);
        return -1;
    }
    index = parser->field_count;
    parser->fields[index] = (struct header_field){
        .name = name_copy,
        .name_length = name_length,
        .value = value_copy,
        .value_length = value_length,
    };
    parser->field_count += 1;
    return 0;
}

/* Returns 1 for valid syntax, 0 for invalid syntax, and -1 for allocation failure. */
static int parse_header_line(struct header_parser *parser) {
    size_t colon = 0;
    size_t value_start;
    size_t value_end;

    if (parser->line_length == 0 || is_optional_whitespace(parser->line[0])) return 0;
    while (colon < parser->line_length && parser->line[colon] != ':') {
        unsigned char byte = parser->line[colon];
        if (byte <= ' ' || byte == 0x7fU) return 0;
        colon += 1;
    }
    if (colon == 0 || colon == parser->line_length) return 0;

    value_start = colon + 1;
    value_end = parser->line_length;
    while (value_start < value_end && is_optional_whitespace(parser->line[value_start])) {
        value_start += 1;
    }
    while (value_end > value_start && is_optional_whitespace(parser->line[value_end - 1])) {
        value_end -= 1;
    }
    return append_field(parser, parser->line, colon,
                        parser->line + value_start, value_end - value_start) == 0 ? 1 : -1;
}

void header_parser_init(struct header_parser *parser) {
    if (parser != NULL) {
        *parser = (struct header_parser){.state = HEADER_PARSER_INCOMPLETE};
    }
}

int header_parser_feed(struct header_parser *parser,
                       const unsigned char *bytes, size_t length) {
    size_t index;

    if (parser == NULL || (bytes == NULL && length != 0)) return -1;
    if (parser->state != HEADER_PARSER_INCOMPLETE) return 0;

    for (index = 0; index < length; index += 1) {
        unsigned char byte = bytes[index];
        if (parser->pending_cr) {
            int parsed;
            if (byte != '\n') {
                parser->state = HEADER_PARSER_INVALID;
                return 0;
            }
            parser->pending_cr = 0;
            if (parser->line_length == 0) {
                parser->state = HEADER_PARSER_COMPLETE;
                return 0;
            }
            parsed = parse_header_line(parser);
            if (parsed < 0) return -1;
            if (parsed == 0) {
                parser->state = HEADER_PARSER_INVALID;
                return 0;
            }
            parser->line_length = 0;
        } else if (byte == '\r') {
            parser->pending_cr = 1;
        } else if (byte == '\n') {
            parser->state = HEADER_PARSER_INVALID;
            return 0;
        } else {
            if (reserve_line(parser, parser->line_length + 1) != 0) return -1;
            parser->line[parser->line_length] = byte;
            parser->line_length += 1;
        }
    }
    return 0;
}

enum header_parser_state header_parser_state(const struct header_parser *parser) {
    return parser == NULL ? HEADER_PARSER_INVALID : parser->state;
}

const struct header_field *header_parser_field_at(
    const struct header_parser *parser, size_t index) {
    if (parser == NULL || index >= parser->field_count) return NULL;
    return &parser->fields[index];
}

void header_parser_free(struct header_parser *parser) {
    size_t index;

    if (parser == NULL) return;
    free(parser->line);
    for (index = 0; index < parser->field_count; index += 1) {
        free((void *)parser->fields[index].name);
        free((void *)parser->fields[index].value);
    }
    free(parser->fields);
    *parser = (struct header_parser){0};
}
