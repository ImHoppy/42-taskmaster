#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include "epoll.h"

#define MAX_EPOLL_EVENTS 10

typedef struct
{
	int sfd;
	int epoll_fd;
	struct epoll_event events[MAX_EPOLL_EVENTS];
} server_socket_t;

int init_epoll(server_socket_t *server_socket);
int add_epoll_event(server_socket_t *server_socket, int fd, uint32_t events);
int handle_epoll(server_socket_t *server_socket);

#endif
