#include "taskmasterd.h"

void free_taskmaster(taskmaster_t *taskmaster)
{
	free_processes(taskmaster->processes, taskmaster->processes_len);
	cJSON_Delete(taskmaster->json_config);
	taskmaster->json_config = NULL;
}

void free_children(process_t *process)
{
	for (uint32_t i = 0; i < process->config.numprocs; i++)
	{
		if (process->children[i].name != NULL)
		{
			free(process->children[i].name);
			process->children[i].name = NULL;
		}
	}
	free(process->children);
	process->children = NULL;
}

void free_config(process_config_t *config)
{
	if (config->envs != NULL)
	{
		for (int key_index = 0; config->envs[key_index]; key_index++)
		{
			free(config->envs[key_index]);
			config->envs[key_index] = NULL;
		}
		free(config->envs);
		config->envs = NULL;
	}
	if (config->cmd != NULL)
	{
		for (int key_index = 0; config->cmd[key_index]; key_index++)
		{
			free(config->cmd[key_index]);
			config->cmd[key_index] = NULL;
		}
		free(config->cmd);
		config->cmd = NULL;
	}
}

void free_processes(process_t *const processes, int processes_len)
{
	if (processes == NULL || processes_len <= 0)
	{
		return;
	}
	for (int i = 0; i < processes_len; i++)
	{
		free_config(&processes[i].config);
		if (processes[i].children != NULL)
		{
			free_children(&processes[i]);
		}
	}

	free(processes);
}
