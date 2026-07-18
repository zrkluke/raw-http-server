#include "request_line.h"

#include <string.h>

int request_line_parse(const unsigned char *line, size_t line_length,
                       struct request_line *parsed) {
    size_t separators[2];
    size_t separator_count = 0;
    size_t index;
    size_t version_length;

    if (line == NULL || parsed == NULL) {
        return REQUEST_LINE_INVALID;
    }
    *parsed = (struct request_line){0};

    for (index = 0; index < line_length; index++) {
        if (line[index] == '\r' || line[index] == '\n' || line[index] == '\t') {
            return REQUEST_LINE_INVALID;
        }
        if (line[index] == ' ') {
            if (separator_count == sizeof(separators) / sizeof(separators[0])) {
                return REQUEST_LINE_INVALID;
            }
            separators[separator_count++] = index;
        }
    }

    if (separator_count != sizeof(separators) / sizeof(separators[0]) ||
        separators[0] == 0 || separators[1] == separators[0] + 1 ||
        separators[1] + 1 == line_length) {
        return REQUEST_LINE_INVALID;
    }

    version_length = line_length - separators[1] - 1;
    if (version_length != strlen("HTTP/1.1") ||
        memcmp(line + separators[1] + 1, "HTTP/1.1", version_length) != 0) {
        return REQUEST_LINE_INVALID;
    }

    parsed->method = line;
    parsed->method_length = separators[0];
    parsed->target = line + separators[0] + 1;
    parsed->target_length = separators[1] - separators[0] - 1;
    parsed->version = line + separators[1] + 1;
    parsed->version_length = version_length;
    return REQUEST_LINE_VALID;
}
