#include "tcp_server.h"

#include <assert.h>

int main(void) {
    struct tcp_server server;

    assert(tcp_server_open(&server, "not-an-ipv4-address", 0) == -1);
    assert(server.listener_fd == -1);

    assert(tcp_server_open(&server, "127.0.0.1", 0) == 0);
    assert(tcp_server_port(&server) != 0);
    tcp_server_close(&server);
    assert(server.listener_fd == -1);
    assert(tcp_server_port(&server) == 0);
    return 0;
}
