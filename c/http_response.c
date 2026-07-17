#include "http_response.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int http_response_build(int status_code, const char *reason,
                        const char *content_type, const unsigned char *body,
                        size_t body_length, unsigned char **response,
                        size_t *response_length) {
    int header_length;
    unsigned char *bytes;

    if (reason == NULL || content_type == NULL || response == NULL ||
        response_length == NULL || (body == NULL && body_length != 0)) {
        return -1;
    }
    *response = NULL;
    *response_length = 0;

    header_length = snprintf(
        NULL, 0,
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status_code, reason, content_type, body_length);
    if (header_length < 0 || (size_t)header_length >= SIZE_MAX - body_length) {
        return -1;
    }

    bytes = malloc((size_t)header_length + body_length + 1);
    if (bytes == NULL) {
        return -1;
    }
    if (snprintf((char *)bytes, (size_t)header_length + 1,
                 "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n"
                 "Connection: close\r\n\r\n",
                 status_code, reason, content_type, body_length) != header_length) {
        free(bytes);
        return -1;
    }
    if (body_length != 0) {
        memcpy(bytes + header_length, body, body_length);
    }
    *response = bytes;
    *response_length = (size_t)header_length + body_length;
    return 0;
}

int http_response_basic_ok(unsigned char **response, size_t *response_length) {
    static const unsigned char body[] = "Hello, World!";

    return http_response_build(200, "OK", "text/plain", body,
                               sizeof(body) - 1, response, response_length);
}
