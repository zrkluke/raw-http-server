#include "request_line.h"

#include <assert.h>
#include <string.h>

static void assert_valid(const char *line, const char *method, const char *target,
                         const char *version) {
    struct request_line parsed;

    assert(request_line_parse((const unsigned char *)line, strlen(line),
                              &parsed) == REQUEST_LINE_VALID);
    assert(parsed.method_length == strlen(method));
    assert(memcmp(parsed.method, method, parsed.method_length) == 0);
    assert(parsed.target_length == strlen(target));
    assert(memcmp(parsed.target, target, parsed.target_length) == 0);
    assert(parsed.version_length == strlen(version));
    assert(memcmp(parsed.version, version, parsed.version_length) == 0);
}

static void assert_invalid(const char *line) {
    struct request_line parsed;

    assert(request_line_parse((const unsigned char *)line, strlen(line),
                              &parsed) == REQUEST_LINE_INVALID);
}

int main(void) {
    assert_valid("GET /search?q=rust HTTP/1.1", "GET", "/search?q=rust",
                 "HTTP/1.1");
    assert_invalid("GET HTTP/1.1");
    assert_invalid(" GET / HTTP/1.1");
    assert_invalid("GET / HTTP/1.1 ");
    assert_invalid("GET  / HTTP/1.1");
    assert_invalid("GET\t/ HTTP/1.1");
    assert_invalid("GET / HTTP/1.0");
    assert_invalid("GET / HTTP/1.1\r\n");
    return 0;
}
