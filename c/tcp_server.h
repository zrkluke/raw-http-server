#ifndef RAW_HTTP_SERVER_TCP_SERVER_H
#define RAW_HTTP_SERVER_TCP_SERVER_H

#include <stddef.h>

struct tcp_server {
    int listener_fd;
    unsigned short port;
};

int tcp_server_open(struct tcp_server *server, const char *address,
                    unsigned short port);
int tcp_server_accept_once(struct tcp_server *server, unsigned char **received,
                           size_t *received_length);
int tcp_server_respond_once(struct tcp_server *server,
                            const unsigned char *response,
                            size_t response_length, unsigned char **received,
                            size_t *received_length);
unsigned short tcp_server_port(const struct tcp_server *server);
void tcp_server_close(struct tcp_server *server);

#endif
