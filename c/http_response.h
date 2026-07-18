#ifndef RAW_HTTP_SERVER_HTTP_RESPONSE_H
#define RAW_HTTP_SERVER_HTTP_RESPONSE_H

#include <stddef.h>

int http_response_build(int status_code, const char *reason,
                        const char *content_type, const unsigned char *body,
                        size_t body_length, unsigned char **response,
                        size_t *response_length);
int http_response_basic_ok(unsigned char **response, size_t *response_length);

#endif
