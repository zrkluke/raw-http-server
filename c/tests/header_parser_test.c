#include "header_parser.h"

#include <assert.h>
#include <string.h>

int main(void) {
    static const unsigned char first[] = "HOST: example.com\r";
    static const unsigned char second[] = "\nX-Note: \t hello \t\r\n\r\n";
    struct header_parser parser;
    const struct header_field *field;

    header_parser_init(&parser);
    assert(header_parser_feed(&parser, first, sizeof(first) - 1) == 0);
    assert(header_parser_state(&parser) == HEADER_PARSER_INCOMPLETE);
    assert(header_parser_feed(&parser, second, sizeof(second) - 1) == 0);
    assert(header_parser_state(&parser) == HEADER_PARSER_COMPLETE);

    field = header_parser_field_at(&parser, 0);
    assert(field != NULL);
    assert(field->name_length == strlen("host"));
    assert(memcmp(field->name, "host", field->name_length) == 0);
    assert(field->value_length == strlen("example.com"));
    assert(memcmp(field->value, "example.com", field->value_length) == 0);
    field = header_parser_field_at(&parser, 1);
    assert(field != NULL);
    assert(field->name_length == strlen("x-note"));
    assert(memcmp(field->name, "x-note", field->name_length) == 0);
    assert(field->value_length == strlen("hello"));
    assert(memcmp(field->value, "hello", field->value_length) == 0);
    assert(header_parser_field_at(&parser, 2) == NULL);

    header_parser_free(&parser);
    return 0;
}
