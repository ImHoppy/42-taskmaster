
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <strings.h>

#include "shared.h"
#include "taskmasterd.h"
#include "socket_server.h"

static process_child_t *find_process_by_name(char *program_name, process_t **process)
{
	for (int i = 0; i < g_taskmaster.processes_len; i++)
	{
		process_t *current_process = &g_taskmaster.processes[i];
		for (uint32_t j = 0; j < current_process->config.numprocs; j++)
		{
			if (strcmp(current_process->children[j].name, program_name) == 0)
			{
				if (process)
					*process = current_process;
				return &current_process->children[j];
			}
		}
	}
	return NULL;
}

static process_t *find_program_by_name(char *program_name)
{
	for (int i = 0; i < g_taskmaster.processes_len; i++)
	{
		process_t *current_process = &g_taskmaster.processes[i];
		if (strcmp(current_process->config.name, program_name) == 0)
		{
			return current_process;
		}
	}
	return NULL;
}

static status_t start_child(process_t *current_process, process_child_t *process_child)
{
	if (process_child->state != STOPPED &&
		process_child->state != EXITED &&
		process_child->state != FATAL &&
		process_child->state != NON_STARTED)
	{
		return FAILURE;
	}
	if (start_handling(process_child, current_process) == FAILURE)
		return FAILURE;
	return SUCCESS;
}

int com_start(client_data_t *client, char *program_name)
{
	process_t *current_process = find_program_by_name(program_name);
	if (current_process != NULL)
	{
		for (size_t i = 0; i < current_process->config.numprocs; i++)
		{
			process_child_t *process_child = &current_process->children[i];
			if (process_child)
				start_child(current_process, process_child);
		}
		dprintf(client->fd, "Starting all %s\n", program_name);
		return 0;
	}

	process_child_t *process_child = find_process_by_name(program_name, &current_process);
	if (process_child == NULL)
	{
		dprintf(client->fd, "Program %s not found\n", program_name);
		return 1;
	}
	if (start_child(current_process, process_child) == FAILURE)
	{
		dprintf(client->fd, "Program %s is already running\n", program_name);
		return 1;
	}
	dprintf(client->fd, "Starting %s\n", program_name);
	return 0;
}

void murder_child(process_child_t *child, uint8_t stopsignal)
{
	if (child->pid > 0)
	{
		kill(child->pid, stopsignal);
		child->state = STOPPING;
		child->starting_stop = time(NULL);
	}
}

int com_restart(client_data_t *client, char *program_name)
{
	process_t *current_process = find_program_by_name(program_name);
	if (current_process != NULL)
	{
		for (size_t i = 0; i < current_process->config.numprocs; i++)
		{
			process_child_t *process_child = &current_process->children[i];
			if (process_child && (process_child->state == RUNNING || process_child->state == STOPPED))
			{
				process_child->need_restart = true;
				murder_child(process_child, current_process->config.stopsignal);
			}
		}
		dprintf(client->fd, "Restarting all %s\n", program_name);
		return 0;
	}

	process_child_t *process_child = find_process_by_name(program_name, &current_process);
	if (process_child == NULL)
	{
		dprintf(client->fd, "Program %s not found\n", program_name);
		return 1;
	}
	if (process_child->state != RUNNING && process_child->state != STOPPED)
	{
		dprintf(client->fd, "Program %s is not running\n", program_name);
		return 1;
	}

	process_child->need_restart = true;
	murder_child(process_child, current_process->config.stopsignal);
	dprintf(client->fd, "Restarting %s\n", program_name);
	return 0;
}

int com_stop(client_data_t *client, char *program_name)
{
	process_t *current_process = find_program_by_name(program_name);
	if (current_process != NULL)
	{
		for (size_t i = 0; i < current_process->config.numprocs; i++)
		{
			process_child_t *process_child = &current_process->children[i];
			if (process_child && process_child->state == RUNNING)
			{
				murder_child(process_child, current_process->config.stopsignal);
			}
		}
		dprintf(client->fd, "Stopping all %s\n", program_name);
		return 0;
	}
	process_child_t *process_child = find_process_by_name(program_name, &current_process);
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
	dprintf(client->fd, "Stopping %s\n", program_name);
	murder_child(process_child, current_process->config.stopsignal);
	return 0;
}

int com_status(client_data_t *client, char *program_name)
{
	process_t *current_process = find_program_by_name(program_name);
	if (current_process != NULL)
	{
		for (size_t i = 0; i < current_process->config.numprocs; i++)
		{
			process_child_t *process_child = &current_process->children[i];
			if (process_child)
			{
				dprintf(client->fd, "%s %s\n", process_child->name, state_to_string(process_child->state));
			}
		}
		return 0;
	}
	process_child_t *process_child = find_process_by_name(program_name, NULL);
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
	char buffer[1024] = {0};
	size_t index = 0;
	for (int i = 0; i < g_taskmaster.processes_len; i++)
	{
		process_t *process = &g_taskmaster.processes[i];
		for (uint32_t j = 0; j < process->config.numprocs; j++)
		{
			dprintf(client->fd, "%s %d;", process->children[j].name, process->children[j].state);
			if (index > (1024 - 100))
			{
				write(client->fd, buffer, index);
				bzero(buffer, index);
				index = 0;
			}
			index += snprintf(buffer + index, 1024 - index, "%s %d;", process->children[j].name, process->children[j].state);
		}
	}
	dprintf(client->fd, "\n");
	return 0;
}
