#ifndef RAW_HTTP_SERVER_HTTP_ECHO_SERVER_H
#define RAW_HTTP_SERVER_HTTP_ECHO_SERVER_H

#include "tcp_server.h"

/* Accept one POST /echo request and write either its binary echo or a 400. */
int http_echo_server_serve_once(struct tcp_server *server);

#endif
