
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
		log_error("opening config file");
		return 1;
	}

	int size_file = 0;
	fseek(config, 0, SEEK_END);
	size_file = ftell(config);
	fseek(config, 0, SEEK_SET);

	char *config_str = calloc(sizeof(char), size_file + 1);
	if (config_str == NULL)
	{
		log_error("malloc failed");
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
		log_error("init_config failed");
		free_taskmaster();
		return 1;
	}

	FILE *log = fopen(g_taskmaster.logfile ? g_taskmaster.logfile : "/tmp/taskmasterd.log", "w");
	if (log == NULL)
	{
		log_error("opening log file");
		free_taskmaster();
		return 1;
	}
	log_add_fp(log, LOG_INFO);

	server_socket_t server_socket = {0};
	server_socket.sfd = create_unix_server_socket(g_taskmaster.serverfile ? g_taskmaster.serverfile : "/tmp/taskmasterd.sock", LIBSOCKET_STREAM, SOCK_CLOEXEC);
	if (server_socket.sfd < 0)
	{
		log_error("creating unix server socket: %s", strerror(errno));
		free_taskmaster();
		return 1;
	}

	if (init_epoll(&server_socket) < 0)
	{
		log_error("creating epoll instance: %s", strerror(errno));
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
	fclose(log);
	return 0;
}
