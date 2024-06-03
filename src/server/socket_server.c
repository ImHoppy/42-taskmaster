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
		log_error("Error creating epoll instance: %s", strerror(errno));
		return -1;
	}

	if (add_epoll_event(server_socket, server_socket->sfd, EPOLLIN, NULL) < 0)
	{
		log_error("Error adding epoll event: %s", strerror(errno));
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

static status_t parse_message(client_data_t *client, char *buf)
{
	char *cmd = strtok(buf, " ");
	if (cmd == NULL)
		return FAILURE;
	static const COMMAND commands[] = {
		{"start", com_start},
		{"restart", com_restart},
		{"stop", com_stop},
		{"status", com_status},
		{"list", com_list},
		{NULL, NULL},
	};

	for (int i = 0; commands[i].name != NULL; i++)
	{
		if (strcmp(commands[i].name, cmd) == 0)
		{
			char *arg = strtok(NULL, " ");
			if (arg && arg[strlen(arg) - 1] == '\n')
				arg[strlen(arg) - 1] = '\0';
			if (arg == NULL)
				arg = "";
			if (commands[i].func(client, arg) < 0)
			{
				log_error("Error executing command: %s", strerror(errno));
				return FAILURE;
			}
			return SUCCESS;
		}
	}
	return SUCCESS;
}

int handle_epoll(server_socket_t *server_socket)
{
	static client_data_t *clients[MAX_EPOLL_EVENTS] = {0};
	static int clients_count = 0;

	struct epoll_event *events = server_socket->events;

	int nfds = epoll_wait(server_socket->epoll_fd, events, MAX_EPOLL_EVENTS, 500); // 500ms timeout

	if (nfds < 0)
	{
		if (errno == EINTR)
			return 0;
		log_error("Error in epoll_wait: %s", strerror(errno));
		return -1;
	}

	for (int i = 0; i < nfds; i++)
	{
		if (events[i].events & EPOLLERR)
		{
			log_error("Error in epoll event");
			remove_epoll_event(server_socket, events[i].data.fd);
			if (events[i].data.ptr != NULL)
			{
				client_data_t *client = events[i].data.ptr;
				close(client->fd);
				shift_clients(clients, &clients_count, client->index);
				free(client);
				events[i].data.ptr = NULL;
			}
			else
			{
				// socket server has error
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
					log_error("Too many clients");
					continue;
				}
				int client_fd = accept_unix_stream_socket(server_socket->sfd, SOCK_NONBLOCK | SOCK_CLOEXEC);
				if (client_fd < 0)
				{
					log_error("Error accepting client: %s", strerror(errno));
					continue;
				}
				client_data_t *client = calloc(1, sizeof(client_data_t));

				if (client == NULL || add_epoll_event(server_socket, client_fd, EPOLLIN, client) < 0)
				{
					log_error("Error adding client epoll event: %s", strerror(errno));
					close(client_fd);
					free(client);
					continue;
				}
				log_info("Client connected with fd: %d", client_fd);
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
						log_error("Error reading from client: %s", strerror(errno));
					}
					// Client disconnected
					log_info("Client disconnected with fd: %d", client->fd);
					remove_epoll_event(server_socket, client->fd);
					close(client->fd);
					shift_clients(clients, &clients_count, client->index);
					free(client);
				}
				else
				{
					log_debug("Received: %s", buf);
					parse_message(client, buf);
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
						log_error("Error writing to client: %s", strerror(errno));
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
