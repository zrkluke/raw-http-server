#include "fixture.h"
#include "tcp_server.h"

#include <assert.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static void send_fixture(const struct fixture *fixture, unsigned short port) {
    struct sockaddr_in address = {0};
    size_t offset = 0;
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
    assert(close(client) == 0);
}

static int connect_to_port(unsigned short port) {
    struct sockaddr_in address = {0};
    int client;
    int result;

    client = socket(AF_INET, SOCK_STREAM, 0);
    assert(client >= 0);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    assert(inet_pton(AF_INET, "127.0.0.1", &address.sin_addr) == 1);
    result = connect(client, (const struct sockaddr *)&address, sizeof(address));
    assert(close(client) == 0);
    return result;
}

int main(int argc, char **argv) {
    struct fixture fixture;
    struct tcp_server server;
    unsigned char *received = NULL;
    size_t received_length = 0;
    unsigned short port;
    pid_t client;
    int status;

    assert(argc == 2);
    assert(fixture_load(argv[1], "tcp", "listen-connect", &fixture) == 0);
    assert(tcp_server_open(&server, "127.0.0.1", 0) == 0);
    port = tcp_server_port(&server);
    assert(port != 0);

    client = fork();
    assert(client >= 0);
    if (client == 0) {
        send_fixture(&fixture, port);
        _exit(0);
    }

    assert(tcp_server_accept_once(&server, &received, &received_length) == 0);
    assert(waitpid(client, &status, 0) == client);
    assert(WIFEXITED(status));
    assert(WEXITSTATUS(status) == 0);
    assert(received_length == fixture.expected_len);
    assert(memcmp(received, fixture.expected, received_length) == 0);
    assert(connect_to_port(port) != 0);

    free(received);
    tcp_server_close(&server);
    fixture_free(&fixture);
    return 0;
}
