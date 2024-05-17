#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include "socket_server.h"
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

	if (add_epoll_event(server_socket, server_socket->sfd, EPOLLIN, NULL) < 0)
	{
		fprintf(stderr, "Error adding epoll event: %s\n", strerror(errno));
		return 1;
	}

	return 0;
}

int add_epoll_event(server_socket_t *server_socket, int fd, uint32_t events, void *data)
{
	struct epoll_event ev = {0};
	ev.events = events;
	ev.data.ptr = data;
	return epoll_ctl(server_socket->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int remove_epoll_event(server_socket_t *server_socket, int fd)
{
	return epoll_ctl(server_socket->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

static void shift_clients(client_data_t *clients[], int *clients_count, int index)
{
	for (int i = index; i < *clients_count - 1; i++)
	{
		clients[i] = clients[i + 1];
		clients[i]->index = i;
	}
	(*clients_count)--;
}

static status_t parse_message(taskmaster_t *taskmaster, client_data_t *client, char *buf)
{
	if (strcmp(buf, "list") == 0)
	{
		for (int i = 0; i < taskmaster->processes_len; i++)
		{
			process_t *process = &taskmaster->processes[i];
			for (uint32_t j = 0; j < process->config.numprocs; j++)
			{
				dprintf(client->fd, "%s-%d\n", process->children[j].name, process->children[j].state);
			}
			write(client->fd, "\0", 1);
		}
	}
	return SUCCESS;
}

int handle_epoll(taskmaster_t *taskmaster, server_socket_t *server_socket)
{
	static client_data_t *clients[MAX_EPOLL_EVENTS] = {0};
	static int clients_count = 0;

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
			if (events[i].data.ptr != NULL)
			{
				client_data_t *client = events[i].data.ptr;
				shift_clients(clients, &clients_count, client->index);
				free(client);
			}
			else
			{
				// socket has error
				return -1;
			}
		}
		else if (events[i].events & EPOLLIN)
		{
			// Server, new client
			if (events[i].data.ptr == NULL)
			{
				if (clients_count >= MAX_EPOLL_EVENTS)
				{
					fprintf(stderr, "Too many clients\n");
					continue;
				}
				int client_fd = accept_unix_stream_socket(server_socket->sfd, SOCK_NONBLOCK | SOCK_CLOEXEC);
				if (client_fd < 0)
				{
					fprintf(stderr, "Error accepting client: %s\n", strerror(errno));
					continue;
				}
				client_data_t *client = calloc(1, sizeof(client_data_t));

				if (client == NULL || add_epoll_event(server_socket, client_fd, EPOLLIN, client) < 0)
				{
					fprintf(stderr, "Error adding client epoll event: %s\n", strerror(errno));
					close(client_fd);
					free(client);
					continue;
				}
				printf("Client connected with fd: %d\n", client_fd);
				client->fd = client_fd;
				client->index = clients_count;
				clients[clients_count++] = client;
			}
			else if (events[i].data.ptr != NULL)
			{
				client_data_t *client = events[i].data.ptr;
				char buf[1024] = {0};
				ssize_t n = read(client->fd, buf, sizeof(buf));
				if (n <= 0)
				{
					// Error on read
					if (n < 0)
					{
						fprintf(stderr, "Error reading from client: %s\n", strerror(errno));
					}
					// Client disconnected
					remove_epoll_event(server_socket, client->fd);
					close(client->fd);
					shift_clients(clients, &clients_count, client->index);
					free(client);
				}
				else
				{
					fprintf(stdout, "Received: %s\n", buf);
					parse_message(taskmaster, client, buf);
				}
			}
		}
		if (events[i].events & EPOLLOUT)
		{
			// Server
			if (events[i].data.ptr == NULL)
			{
			}
			else
			{
				client_data_t *client = events[i].data.ptr;
				if (client->to_send && client->buf != NULL)
				{
					ssize_t n = write(client->fd, client->buf, strlen(client->buf));
					if (n < 0)
					{
						fprintf(stderr, "Error writing to client: %s\n", strerror(errno));
						remove_epoll_event(server_socket, client->fd);
						close(client->fd);
						shift_clients(clients, &clients_count, client->index);
						free(client);
					}
					else
					{
						client->to_send = false;
						free(client->buf);
						client->buf = NULL;
					}
				}
			}
		}
	}
	return 0;
}
