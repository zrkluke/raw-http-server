#include "http_response.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    static const unsigned char expected[] =
        "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: 13\r\n"
        "Connection: close\r\n\r\nHello, World!";
    unsigned char *response = NULL;
    size_t response_length = 0;

    assert(http_response_basic_ok(&response, &response_length) == 0);
    assert(response_length == sizeof(expected) - 1);
    assert(memcmp(response, expected, response_length) == 0);

    free(response);
    return 0;
}
