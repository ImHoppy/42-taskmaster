
#include "cJSON.h"
#include "taskmasterd.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include "headers/libunixsocket.h"
#include <sys/epoll.h>
#include "socket_server.h"

taskmaster_t g_taskmaster = {0};

void handle_sigint()
{
	printf("Signal handling !\n");
	g_taskmaster.running = false;
}

int main(int ac, char **av)
{
	if (ac < 2 && av[1] == NULL)
	{
		fprintf(stderr, "Usage: %s <config_file>\n", av[0]);
		return 1;
	}
	FILE *config = fopen(av[1], "r");
	if (config == NULL)
	{
		fprintf(stderr, "Error opening config file\n");
		return 1;
	}

	int size_file = 0;
	fseek(config, 0, SEEK_END);
	size_file = ftell(config);
	fseek(config, 0, SEEK_SET);

	char *config_str = calloc(sizeof(char), size_file + 1);
	if (config_str == NULL)
	{
		fprintf(stderr, "Error: malloc failed\n");
		fclose(config);
		return 1;
	}
	fread(config_str, sizeof(char), size_file, config);

	signal(SIGINT, handle_sigint);

	status_t ret = init_config(config_str);
	free(config_str);
	fclose(config);
	if (ret == FAILURE)
	{
		fprintf(stderr, "Error: init_config failed\n");
		free_taskmaster();
		return 1;
	}

	server_socket_t server_socket = {0};
	server_socket.sfd = create_unix_server_socket("/tmp/taskmasterd.sock", LIBSOCKET_STREAM, 0);
	if (server_socket.sfd == FAILURE)
	{
		fprintf(stderr, "Error creating unix server socket: %s\n", strerror(errno));
		free_taskmaster();
		return 1;
	}

	if (init_epoll(&server_socket) < 0)
	{
		fprintf(stderr, "Error creating epoll instance: %s\n", strerror(errno));
		free_taskmaster();
		return 1;
	}
	g_taskmaster.running = true;
	while (g_taskmaster.running)
	{
		if (handle_epoll(&server_socket) < 0)
			break;
		// Handle each process
		handler();
	}
	free_taskmaster();
	close(server_socket.epoll_fd);
	close(server_socket.sfd);

	return 0;
}
