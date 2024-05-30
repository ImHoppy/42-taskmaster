
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
	g_taskmaster.log_fp = fopen(log_file ? log_file : "/tmp/taskmasterd.log", "w");
	if (g_taskmaster.log_fp == NULL)
	{
		log_error("opening log file");
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

process_t *find_process_config_by_name(process_t *processes, int processes_len, const char *name)
{
	for (int i = 0; i < processes_len; i++)
	{
		if (strcmp(processes[i].config.name, name) == 0)
			return &processes[i];
	}
	return NULL;
}

void reload_config()
{
	log_info("Configuration file %s reloaded.", g_taskmaster.config_file);

	char *config_str = load_config_file(g_taskmaster.config_file);
	if (config_str == NULL)
	{
		log_error("Failed to reload non existing configuration file.");
		return;
	}

	cJSON *json_config = cJSON_Parse(config_str);
	free(config_str);

	program_json_t valid_program = check_valid_json(json_config);
	if (valid_program.fail)
	{
		log_error("Missing programs in configuration file.");
		cJSON_Delete(g_taskmaster.json_config);
		return;
	}

	process_t *new_processes = calloc(sizeof(process_t), valid_program.len);
	if (new_processes == NULL)
	{
		g_taskmaster.running = false;
		cJSON_Delete(g_taskmaster.json_config);
		return;
	}
	processes_default_config(new_processes, valid_program.len);

	if (parse_config(valid_program.programs, new_processes) == FAILURE || check_unique_name(new_processes, valid_program.len) == FAILURE)
	{
		g_taskmaster.running = false;
		cJSON_Delete(g_taskmaster.json_config);
		free_processes(new_processes, valid_program.len);
		return;
	}

	init_children(new_processes, valid_program.len);

	for (size_t i = 0; i < valid_program.len; i++)
	{
		process_t *process = find_process_config_by_name(g_taskmaster.processes, g_taskmaster.processes_len, new_processes[i].config.name);
		// Non existing on old config
		if (process == NULL)
		{
			continue;
		}
		// changed config
		if (!compare_process_config(&new_processes[i], process))
		{
			// process KILL
			for (size_t child_index = 0; child_index < process->config.numprocs; child_index++)
			{
				murder_child(&process->children[child_index], SIGKILL);
			}
			// &new_processes[i] START
			for (size_t child_index = 0; child_index < new_processes[i].config.numprocs; child_index++)
			{
				// start
				if (new_processes[i].config.autostart)
				{
					start_handling(&new_processes[i].children[child_index], &new_processes[i], child_index);
				}
			}
		}
		else
		{
			// unchanged
			process_child_t *tmp = new_processes[i].children;
			new_processes[i].children = process->children;
			process->children = tmp;
		}
	}

	for (int i = 0; i < g_taskmaster.processes_len; i++)
	{
		process_t *process = find_process_config_by_name(new_processes, valid_program.len, g_taskmaster.processes[i].config.name);
		if (process == NULL)
		{
			for (size_t child_index = 0; child_index < g_taskmaster.processes[i].config.numprocs; child_index++)
			{
				murder_child(&g_taskmaster.processes[i].children[child_index], SIGKILL);
			}
		}
	}

	process_t *tmp = g_taskmaster.processes;
	g_taskmaster.processes = new_processes;
	new_processes = tmp;

	int len_tmp = g_taskmaster.processes_len;
	g_taskmaster.processes_len = valid_program.len;
	valid_program.len = len_tmp;

	free_processes(new_processes, valid_program.len);
	cJSON_Delete(g_taskmaster.json_config);
	g_taskmaster.json_config = json_config;
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
	fclose(g_taskmaster.log_fp);
	return 0;
}
