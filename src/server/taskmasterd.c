
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

char *load_config_file(const char *config_file)
{
	FILE *config = fopen(config_file, "r");
	if (config == NULL)
	{
		log_error("opening config file");
		return NULL;
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
		return NULL;
	}
	fread(config_str, sizeof(char), size_file, config);
	fclose(config);
	return config_str;
}

int load_log_file(const char *log_file)
{
	const char *file = log_file ? log_file : "/tmp/taskmasterd.log";
	g_taskmaster.log_fp = fopen(file, "w");
	if (g_taskmaster.log_fp == NULL)
	{
		log_error("opening log file %s", file);
		free_taskmaster();
		return 1;
	}
	log_add_fp(g_taskmaster.log_fp, LOG_INFO);
	return 0;
}

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
	g_taskmaster.config_file = av[1];

	signal(SIGINT, handle_sigint);
	signal(SIGHUP, reload_config);

	char *config_str = load_config_file(g_taskmaster.config_file);
	if (config_str == NULL)
		return 1;

	status_t ret = init_config(config_str);
	free(config_str);
	if (ret == FAILURE)
	{
		log_error("init_config failed");
		free_taskmaster();
		return 1;
	}

	if (load_log_file(g_taskmaster.logfile) == 1)
		return 1;

	server_socket_t server_socket = {0};
	const char *file = g_taskmaster.serverfile ? g_taskmaster.serverfile : "/tmp/taskmasterd.sock";
	server_socket.sfd = create_unix_server_socket(file, LIBSOCKET_STREAM, SOCK_CLOEXEC);
	if (server_socket.sfd < 0)
	{
		log_error("creating unix server socket: %s: %s", file, strerror(errno));
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
		if (handle_epoll(&server_socket) == FAILURE)
			break;
		// Handle each process
		handler();
	}
	free_taskmaster();
	close(server_socket.epoll_fd);
	close(server_socket.sfd);
	fclose(g_taskmaster.log_fp);
	return 0;
}
