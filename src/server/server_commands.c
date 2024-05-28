
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "shared.h"
#include "taskmasterd.h"
#include "socket_server.h"

static process_child_t *find_process_by_name(char *program_name)
{
	for (int i = 0; i < g_taskmaster.processes_len; i++)
	{
		process_t *process = &g_taskmaster.processes[i];
		for (uint32_t j = 0; j < process->config.numprocs; j++)
		{
			if (strcmp(process->children[j].name, program_name) == 0)
			{
				return &process->children[j];
			}
		}
	}
	return NULL;
}

int com_start(client_data_t *client, char *program_name)
{
	process_child_t *process_child = find_process_by_name(program_name);
	if (process_child == NULL)
	{
		dprintf(client->fd, "Program %s not found\n", program_name);
		return 1;
	}
	if (process_child->state != STOPPED && process_child->state != EXITED && process_child->state != FATAL)
	{
		dprintf(client->fd, "Program %s is already running\n", program_name);
		return 1;
	}
	process_child->state = STARTING;
	return 0;
}

int com_restart(client_data_t *client, char *program_name)
{
	process_child_t *process_child = find_process_by_name(program_name);
	if (process_child == NULL)
	{
		dprintf(client->fd, "Program %s not found\n", program_name);
		return 1;
	}
	if (process_child->state != RUNNING)
	{
		dprintf(client->fd, "Program %s is not running\n", program_name);
		return 1;
	}
	process_child->state = STOPPING;
	// TODO: Need to specify the child to restart

	return 0;
}

int com_stop(client_data_t *client, char *program_name)
{
	process_child_t *process_child = find_process_by_name(program_name);
	if (process_child == NULL)
	{
		dprintf(client->fd, "Program %s not found\n", program_name);
		return 1;
	}
	if (process_child->state != RUNNING)
	{
		dprintf(client->fd, "Program %s is not running\n", program_name);
		return 1;
	}
	process_child->state = STOPPING;
	return 0;
}

int com_status(client_data_t *client, char *program_name)
{
	process_child_t *process_child = find_process_by_name(program_name);
	if (process_child == NULL)
	{
		dprintf(client->fd, "Program %s not found\n", program_name);
		return 1;
	}
	dprintf(client->fd, "%s %s\n", process_child->name, state_to_string(process_child->state));
	return 0;
}

int com_list(client_data_t *client, char *program_name)
{
	(void)program_name;
	for (int i = 0; i < g_taskmaster.processes_len; i++)
	{
		process_t *process = &g_taskmaster.processes[i];
		for (uint32_t j = 0; j < process->config.numprocs; j++)
		{
			dprintf(client->fd, "%s %d\n", process->children[j].name, process->children[j].state);
		}
	}
	return 0;
}
