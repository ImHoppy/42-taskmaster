#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include "taskmasterd.h"

#define MAX_EPOLL_EVENTS 10

typedef struct
{
	int sfd;
	int epoll_fd;
	struct epoll_event events[MAX_EPOLL_EVENTS];
} server_socket_t;

typedef struct
{
	int index;
	int fd;
	bool to_send;
	char *buf;
} client_data_t;

int init_epoll(server_socket_t *server_socket);
int add_epoll_event(server_socket_t *server_socket, int fd, uint32_t events, void *data);
int handle_epoll(server_socket_t *server_socket);

#endif
