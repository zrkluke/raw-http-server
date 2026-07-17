#include "tcp_server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static int reserve_bytes(unsigned char **bytes, size_t *capacity,
                         size_t required) {
    unsigned char *grown;
    size_t next_capacity;

    if (required <= *capacity) {
        return 0;
    }

    next_capacity = *capacity == 0 ? 4096 : *capacity;
    while (next_capacity < required) {
        if (next_capacity > SIZE_MAX / 2) {
            next_capacity = required;
            break;
        }
        next_capacity *= 2;
    }

    grown = realloc(*bytes, next_capacity);
    if (grown == NULL) {
        return -1;
    }
    *bytes = grown;
    *capacity = next_capacity;
    return 0;
}

static int read_client(int client, unsigned char **received,
                       size_t *received_length) {
    unsigned char buffer[4096];
    unsigned char *bytes = NULL;
    size_t capacity = 0;
    size_t length = 0;

    for (;;) {
        ssize_t count = read(client, buffer, sizeof(buffer));

        if (count == 0) {
            break;
        }
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            free(bytes);
            return -1;
        }
        if (length > SIZE_MAX - (size_t)count ||
            reserve_bytes(&bytes, &capacity, length + (size_t)count) != 0) {
            free(bytes);
            return -1;
        }
        memcpy(bytes + length, buffer, (size_t)count);
        length += (size_t)count;
    }

    *received = bytes;
    *received_length = length;
    return 0;
}

static int write_all(int client, const unsigned char *response,
                     size_t response_length) {
    size_t offset = 0;

    while (offset < response_length) {
        ssize_t written = write(client, response + offset,
                                response_length - offset);

        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (written == 0) {
            errno = EIO;
            return -1;
        }
        offset += (size_t)written;
    }
    return 0;
}

int tcp_server_open(struct tcp_server *server, const char *address,
                    unsigned short port) {
    struct sockaddr_in socket_address = {0};
    socklen_t socket_address_length = sizeof(socket_address);
    int listener;

    if (server == NULL || address == NULL) {
        errno = EINVAL;
        return -1;
    }

    *server = (struct tcp_server){.listener_fd = -1};
    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0) {
        return -1;
    }

    socket_address.sin_family = AF_INET;
    socket_address.sin_port = htons(port);
    if (inet_pton(AF_INET, address, &socket_address.sin_addr) != 1 ||
        bind(listener, (const struct sockaddr *)&socket_address,
             sizeof(socket_address)) != 0 ||
        listen(listener, 1) != 0 ||
        getsockname(listener, (struct sockaddr *)&socket_address,
                    &socket_address_length) != 0) {
        int saved_errno = errno;

        close(listener);
        errno = saved_errno;
        return -1;
    }

    server->listener_fd = listener;
    server->port = ntohs(socket_address.sin_port);
    return 0;
}

int tcp_server_accept_once(struct tcp_server *server, unsigned char **received,
                           size_t *received_length) {
    int client;

    if (server == NULL || server->listener_fd < 0 || received == NULL ||
        received_length == NULL) {
        errno = EINVAL;
        return -1;
    }
    *received = NULL;
    *received_length = 0;

    client = accept(server->listener_fd, NULL, NULL);
    if (client < 0) {
        return -1;
    }
    tcp_server_close(server);

    if (read_client(client, received, received_length) != 0 ||
        close(client) != 0) {
        free(*received);
        *received = NULL;
        *received_length = 0;
        return -1;
    }
    return 0;
}

int tcp_server_respond_once(struct tcp_server *server,
                            const unsigned char *response,
                            size_t response_length, unsigned char **received,
                            size_t *received_length) {
    int client;
    int result;

    if (server == NULL || server->listener_fd < 0 || response == NULL ||
        response_length == 0 || received == NULL || received_length == NULL) {
        errno = EINVAL;
        return -1;
    }
    *received = NULL;
    *received_length = 0;

    client = accept(server->listener_fd, NULL, NULL);
    if (client < 0) {
        return -1;
    }
    tcp_server_close(server);

    result = read_client(client, received, received_length);
    if (result == 0) {
        result = write_all(client, response, response_length);
    }
    if (close(client) != 0 && result == 0) {
        result = -1;
    }
    if (result != 0) {
        free(*received);
        *received = NULL;
        *received_length = 0;
    }
    return result;
}

unsigned short tcp_server_port(const struct tcp_server *server) {
    return server == NULL ? 0 : server->port;
}

void tcp_server_close(struct tcp_server *server) {
    if (server != NULL && server->listener_fd >= 0) {
        close(server->listener_fd);
        server->listener_fd = -1;
        server->port = 0;
    }
}
