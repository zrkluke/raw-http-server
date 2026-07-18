#include "http_echo_server.h"

#include "body_parser.h"
#include "chunked_parser.h"
#include "header_parser.h"
#include "http_response.h"
#include "request_line.h"
#include "request_parser.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int append_bytes(unsigned char **bytes, size_t *length, size_t *capacity,
                        const unsigned char *source, size_t source_length) {
    unsigned char *grown;
    size_t required;
    size_t next;

    if (source_length == 0) return 0;
    if (*length > SIZE_MAX - source_length) return -1;
    required = *length + source_length;
    if (required > *capacity) {
        next = *capacity == 0 ? 4096 : *capacity;
        while (next < required) {
            if (next > SIZE_MAX / 2) { next = required; break; }
            next *= 2;
        }
        grown = realloc(*bytes, next);
        if (grown == NULL) return -1;
        *bytes = grown;
        *capacity = next;
    }
    memcpy(*bytes + *length, source, source_length);
    *length = required;
    return 0;
}

static size_t find_bytes(const unsigned char *bytes, size_t length,
                         const char *needle, size_t needle_length) {
    size_t index;
    if (length < needle_length) return SIZE_MAX;
    for (index = 0; index <= length - needle_length; index++) {
        if (memcmp(bytes + index, needle, needle_length) == 0) return index;
    }
    return SIZE_MAX;
}

static int write_all(int client, const unsigned char *bytes, size_t length) {
    size_t offset = 0;
    while (offset < length) {
        ssize_t written = write(client, bytes + offset, length - offset);
        if (written < 0) { if (errno == EINTR) continue; return -1; }
        if (written == 0) return -1;
        offset += (size_t)written;
    }
    return 0;
}

static int equals_ascii_case(const unsigned char *bytes, size_t length,
                             const char *literal) {
    size_t index;
    if (length != strlen(literal)) return 0;
    for (index = 0; index < length; index++) {
        unsigned char byte = bytes[index];
        unsigned char expected = (unsigned char)literal[index];
        if (byte >= 'A' && byte <= 'Z') byte = (unsigned char)(byte + ('a' - 'A'));
        if (byte != expected) return 0;
    }
    return 1;
}

static int send_response(int client, int status, const char *reason,
                         const char *type, const unsigned char *body,
                         size_t body_length) {
    unsigned char *response = NULL;
    size_t response_length = 0;
    int result;
    if (http_response_build(status, reason, type, body, body_length,
                            &response, &response_length) != 0) return -1;
    result = write_all(client, response, response_length);
    free(response);
    return result;
}

static int select_framing(const struct header_parser *headers,
                          const unsigned char **content_length,
                          size_t *content_length_length, int *chunked) {
    size_t index = 0;
    const struct header_field *field;
    int saw_length = 0;
    int saw_transfer = 0;
    *content_length = NULL;
    *content_length_length = 0;
    *chunked = 0;
    while ((field = header_parser_field_at(headers, index++)) != NULL) {
        if (field->name_length == strlen("content-length") &&
            memcmp(field->name, "content-length", field->name_length) == 0) {
            if (saw_length) return -1;
            saw_length = 1;
            *content_length = field->value;
            *content_length_length = field->value_length;
        } else if (field->name_length == strlen("transfer-encoding") &&
                   memcmp(field->name, "transfer-encoding", field->name_length) == 0) {
            if (saw_transfer || !equals_ascii_case(field->value, field->value_length,
                                                   "chunked")) return -1;
            saw_transfer = 1;
            *chunked = 1;
        }
    }
    return saw_length + saw_transfer == 1 ? 0 : -1;
}

int http_echo_server_serve_once(struct tcp_server *server) {
    unsigned char chunk[4096];
    unsigned char *request = NULL;
    size_t request_length = 0, request_capacity = 0, head_end, line_end;
    int client = -1, result = -1, protocol_error = 0, chunked = 0;
    struct request_parser request_parser;
    struct request_line line;
    struct header_parser headers = {0};
    const unsigned char *content_length = NULL, *body = NULL;
    size_t content_length_length = 0, body_length = 0;
    struct body_parser body_parser = {0};
    struct chunked_parser *chunked_parser = NULL;

    if (server == NULL || server->listener_fd < 0) return -1;
    client = accept(server->listener_fd, NULL, NULL);
    if (client < 0) return -1;
    tcp_server_close(server);
    for (;;) {
        head_end = find_bytes(request, request_length, "\r\n\r\n", 4);
        if (head_end != SIZE_MAX) { head_end += 4; break; }
        {
            ssize_t count = read(client, chunk, sizeof(chunk));
            if (count <= 0 || append_bytes(&request, &request_length, &request_capacity,
                                           chunk, (size_t)count) != 0) goto done;
        }
    }
    request_parser_init(&request_parser);
    if (request_parser_feed(&request_parser, request, head_end) != REQUEST_COMPLETE) {
        protocol_error = 1; goto response;
    }
    line_end = find_bytes(request, head_end, "\r\n", 2);
    if (line_end == SIZE_MAX || request_line_parse(request, line_end, &line) != REQUEST_LINE_VALID ||
        line.method_length != 4 || memcmp(line.method, "POST", 4) != 0 ||
        line.target_length != 5 || memcmp(line.target, "/echo", 5) != 0) {
        protocol_error = 1; goto response;
    }
    header_parser_init(&headers);
    if (header_parser_feed(&headers, request + line_end + 2,
                           head_end - line_end - 2) != 0 ||
        header_parser_state(&headers) != HEADER_PARSER_COMPLETE ||
        select_framing(&headers, &content_length, &content_length_length, &chunked) != 0) {
        protocol_error = 1; goto response_headers;
    }
    if (chunked) {
        chunked_parser = chunked_parser_new();
        if (chunked_parser == NULL || chunked_parser_feed(chunked_parser, request + head_end,
                                                          request_length - head_end) != 0) goto response_headers;
        while (chunked_parser_state(chunked_parser) == CHUNKED_PARSER_INCOMPLETE) {
            ssize_t count = read(client, chunk, sizeof(chunk));
            if (count <= 0 || chunked_parser_feed(chunked_parser, chunk, (size_t)count) != 0) goto response_headers;
        }
        if (chunked_parser_state(chunked_parser) != CHUNKED_PARSER_COMPLETE) { protocol_error = 1; goto response_headers; }
        body = chunked_parser_body(chunked_parser, &body_length);
    } else {
        if (body_parser_init(&body_parser, content_length, content_length_length, 1) != 0 ||
            body_parser_feed(&body_parser, request + head_end, request_length - head_end) != 0) goto response_headers;
        while (body_parser_state(&body_parser) == BODY_PARSER_INCOMPLETE) {
            ssize_t count = read(client, chunk, sizeof(chunk));
            if (count <= 0 || body_parser_feed(&body_parser, chunk, (size_t)count) != 0) goto response_body;
        }
        if (body_parser_state(&body_parser) != BODY_PARSER_COMPLETE) { protocol_error = 1; goto response_body; }
        body = body_parser_body(&body_parser, &body_length);
    }
response_headers:
    if (protocol_error) result = send_response(client, 400, "Bad Request", "text/plain", NULL, 0);
    else result = send_response(client, 200, "OK", "image/bmp", body, body_length);
    goto done_headers;
response_body:
    body_parser_free(&body_parser);
response:
    result = send_response(client, 400, "Bad Request", "text/plain", NULL, 0);
done_headers:
    if (chunked_parser != NULL) chunked_parser_free(chunked_parser);
    if (!chunked) body_parser_free(&body_parser);
    header_parser_free(&headers);
done:
    if (client >= 0) close(client);
    free(request);
    return result;
}
