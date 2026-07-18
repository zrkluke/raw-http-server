#include "fixture.h"
#include "http_echo_server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void append(unsigned char **bytes, size_t *length, const void *source,
                   size_t source_length) {
    unsigned char *grown = realloc(*bytes, *length + source_length);
    assert(grown != NULL);
    *bytes = grown;
    memcpy(*bytes + *length, source, source_length);
    *length += source_length;
}

static void write_fragmented(int client, const unsigned char *request,
                             size_t request_length) {
    size_t offset = 0;
    size_t size = 1;
    while (offset < request_length) {
        size_t available = request_length - offset;
        size_t count = size < available ? size : available;
        assert(write(client, request + offset, count) == (ssize_t)count);
        offset += count;
        size = size % 7 + 1;
    }
}

static void client_case(unsigned short port, const unsigned char *request,
                        size_t request_length, int expected_status,
                        const unsigned char *expected_body,
                        size_t expected_body_length) {
    struct sockaddr_in address = {0};
    unsigned char response[4096];
    size_t response_length = 0;
    int client;
    int header_length;
    char header[256];

    client = socket(AF_INET, SOCK_STREAM, 0);
    assert(client >= 0);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    assert(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) == 1);
    assert(connect(client, (const struct sockaddr *)&address, sizeof(address)) == 0);
    write_fragmented(client, request, request_length);
    for (;;) {
        ssize_t count = read(client, response + response_length,
                             sizeof(response) - response_length);
        assert(count >= 0);
        if (count == 0) break;
        response_length += (size_t)count;
        assert(response_length < sizeof(response));
    }
    header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
        expected_status, expected_status == 200 ? "OK" : "Bad Request",
        expected_status == 200 ? "image/bmp" : "text/plain", expected_body_length);
    assert(header_length > 0);
    assert(response_length == (size_t)header_length + expected_body_length);
    assert(memcmp(response, header, (size_t)header_length) == 0);
    if (expected_body_length != 0) {
        assert(memcmp(response + header_length, expected_body, expected_body_length) == 0);
    }
    assert(close(client) == 0);
}

static void run_case(const unsigned char *request, size_t request_length,
                     int expected_status, const unsigned char *expected_body,
                     size_t expected_body_length) {
    struct tcp_server server;
    pid_t child;
    int status;

    assert(tcp_server_open(&server, "127.0.0.1", 0) == 0);
    child = fork();
    assert(child >= 0);
    if (child == 0) {
        client_case(tcp_server_port(&server), request, request_length,
                    expected_status, expected_body, expected_body_length);
        _exit(0);
    }
    assert(http_echo_server_serve_once(&server) == 0);
    assert(waitpid(child, &status, 0) == child);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
    tcp_server_close(&server);
}

static unsigned char *payload_from_fixture(const char *root, size_t *length) {
    unsigned char *image = NULL;
    size_t image_length = 0;
    unsigned char *payload;

    assert(fixture_load_named(root, "binary", "echo-bmp", "image.bmp",
                              &image, &image_length) == 0);
    payload = malloc(image_length + 2);
    assert(payload != NULL);
    memcpy(payload, image, image_length);
    payload[image_length] = '\r';
    payload[image_length + 1] = '\n';
    free(image);
    *length = image_length + 2;
    return payload;
}

static unsigned char *content_length_request(const unsigned char *payload,
                                             size_t payload_length,
                                             size_t *length) {
    char header[256];
    int header_length = snprintf(header, sizeof(header),
        "POST /echo HTTP/1.1\r\nHost: fixture.test\r\nContent-Type: image/bmp\r\nContent-Length: %zu\r\n\r\n",
        payload_length);
    unsigned char *request = NULL;
    assert(header_length > 0);
    append(&request, length, header, (size_t)header_length);
    append(&request, length, payload, payload_length);
    return request;
}

static unsigned char *chunked_request(const unsigned char *payload,
                                      size_t payload_length, size_t *length) {
    static const size_t sizes[] = {1, 17};
    static const char head[] = "POST /echo HTTP/1.1\r\nHost: fixture.test\r\n"
                               "Content-Type: image/bmp\r\nTransfer-Encoding: Chunked\r\n\r\n";
    unsigned char *request = NULL;
    size_t offset = 0;
    size_t index;
    append(&request, length, head, sizeof(head) - 1);
    for (index = 0; offset < payload_length; index++) {
        size_t size = sizes[index % 2];
        char line[32];
        int line_length;
        if (size > payload_length - offset) size = payload_length - offset;
        line_length = snprintf(line, sizeof(line), "%zx\r\n", size);
        assert(line_length > 0);
        append(&request, length, line, (size_t)line_length);
        append(&request, length, payload + offset, size);
        append(&request, length, "\r\n", 2);
        offset += size;
    }
    {
        static const char end[] = "0\r\nX-Trailer: done\r\n\r\n";
        append(&request, length, end, sizeof(end) - 1);
    }
    return request;
}

int main(int argc, char **argv) {
    unsigned char *payload;
    unsigned char *request;
    size_t payload_length;
    size_t request_length;
    static const char ambiguous[] = "POST /echo HTTP/1.1\r\nHost: fixture.test\r\n"
                                    "Content-Length: 60\r\nTransfer-Encoding: chunked\r\n\r\n";

    assert(argc == 2);
    payload = payload_from_fixture(argv[1], &payload_length);
    request_length = 0;
    request = content_length_request(payload, payload_length, &request_length);
    run_case(request, request_length, 200, payload, payload_length);
    free(request);
    request_length = 0;
    request = chunked_request(payload, payload_length, &request_length);
    run_case(request, request_length, 200, payload, payload_length);
    free(request);
    run_case((const unsigned char *)ambiguous, sizeof(ambiguous) - 1, 400,
             NULL, 0);
    free(payload);
    return 0;
}
