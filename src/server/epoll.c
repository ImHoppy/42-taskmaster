#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include "epoll.h"
#include "taskmasterd.h"
#include "headers/libunixsocket.h"

int init_epoll(server_socket_t *server_socket)
{
	server_socket->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (server_socket->epoll_fd < 0)
	{
		fprintf(stderr, "Error creating epoll instance: %s\n", strerror(errno));
		return -1;
	}

	if (add_epoll_event(server_socket, server_socket->sfd, EPOLLIN) < 0)
	{
		fprintf(stderr, "Error adding epoll event: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int add_epoll_event(server_socket_t *server_socket, int fd, uint32_t events)
{
	struct epoll_event ev = {0};
	ev.events = events;
	ev.data.fd = fd;
	return epoll_ctl(server_socket->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int remove_epoll_event(server_socket_t *server_socket, int fd)
{
	return epoll_ctl(server_socket->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

int handle_epoll(server_socket_t *server_socket)
{
	struct epoll_event *events = server_socket->events;

	int nfds = epoll_wait(server_socket->epoll_fd, events, MAX_EPOLL_EVENTS, 0);

	if (nfds < 0)
	{
		fprintf(stderr, "Error in epoll_wait: %s\n", strerror(errno));
		return -1;
	}

	for (int i = 0; i < nfds; i++)
	{
		if (events[i].events & EPOLLERR)
		{
			fprintf(stderr, "Error in epoll event\n");
			remove_epoll_event(server_socket, events[i].data.fd);
			close(events[i].data.fd);
		}
		else if (events[i].events & EPOLLIN)
		{
			if (events[i].data.fd == server_socket->sfd)
			{
				accept_unix_stream_socket(server_socket->sfd, SOCK_NONBLOCK | SOCK_CLOEXEC);
			}
			// Handle incoming data
		}
		if (events[i].events & EPOLLOUT)
		{
			// Handle outgoing data
		}
	}
	return 0;
}
