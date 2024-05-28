#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>

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

typedef int (*CMDFunction)(client_data_t *, char *);
typedef struct
{
	char *name;
	CMDFunction func;
} COMMAND;
// Commands
int com_start(client_data_t *client, char *program_name);
int com_restart(client_data_t *client, char *program_name);
int com_stop(client_data_t *client, char *program_name);
int com_status(client_data_t *client, char *program_name);
int com_list(client_data_t *client, char *program_name);

#endif
