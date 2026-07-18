#include "fixture.h"
#include "tcp_server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void receive_response(const struct fixture *fixture,
                             unsigned short port) {
    struct sockaddr_in address = {0};
    unsigned char response[1024];
    size_t offset = 0;
    size_t response_length = 0;
    int client;

    client = socket(AF_INET, SOCK_STREAM, 0);
    assert(client >= 0);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    assert(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) == 1);
    assert(connect(client, (const struct sockaddr *)&address, sizeof(address)) ==
           0);

    while (offset < fixture->input_len) {
        ssize_t written = write(client, fixture->input + offset,
                                fixture->input_len - offset);

        assert(written > 0);
        offset += (size_t)written;
    }
    assert(shutdown(client, SHUT_WR) == 0);
    for (;;) {
        ssize_t count = read(client, response + response_length,
                             sizeof(response) - response_length);

        assert(count >= 0);
        if (count == 0) {
            break;
        }
        response_length += (size_t)count;
        assert(response_length < sizeof(response));
    }
    assert(response_length == fixture->expected_len);
    assert(memcmp(response, fixture->expected, response_length) == 0);
    assert(close(client) == 0);
}

int main(int argc, char **argv) {
    struct fixture fixture;
    struct tcp_server server;
    unsigned char *received = NULL;
    size_t received_length = 0;
    pid_t client;
    int status;

    assert(argc == 2);
    assert(fixture_load(argv[1], "responses", "basic", &fixture) == 0);
    assert(tcp_server_open(&server, "127.0.0.1", 0) == 0);

    client = fork();
    assert(client >= 0);
    if (client == 0) {
        receive_response(&fixture, tcp_server_port(&server));
        _exit(0);
    }

    assert(tcp_server_respond_once(&server, fixture.expected,
                                   fixture.expected_len, &received,
                                   &received_length) == 0);
    assert(waitpid(client, &status, 0) == client);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
    assert(received_length == fixture.input_len);
    assert(memcmp(received, fixture.input, received_length) == 0);

    free(received);
    tcp_server_close(&server);
    fixture_free(&fixture);
    return 0;
}
