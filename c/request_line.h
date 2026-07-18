#ifndef RAW_HTTP_SERVER_REQUEST_LINE_H
#define RAW_HTTP_SERVER_REQUEST_LINE_H

#include <stddef.h>

enum request_line_result {
    REQUEST_LINE_INVALID = 0,
    REQUEST_LINE_VALID = 1,
};

struct request_line {
    const unsigned char *method;
    size_t method_length;
    const unsigned char *target;
    size_t target_length;
    const unsigned char *version;
    size_t version_length;
};

int request_line_parse(const unsigned char *line, size_t line_length,
                       struct request_line *parsed);

#endif
