#include "taskmasterd.h"

void free_paths(char **paths)
{
	for (uint32_t path_index = 0; paths[path_index]; path_index++)
	{
		free(paths[path_index]);
	}
	free(paths);
}

void free_taskmaster()
{
	free_processes(g_taskmaster.processes, g_taskmaster.processes_len);
	cJSON_Delete(g_taskmaster.json_config);
	g_taskmaster.json_config = NULL;
}

void free_processes(process_t *const processes, int processes_len)
{
	if (processes == NULL || processes_len <= 0)
	{
		return;
	}
	for (int i = 0; i < processes_len; i++)
	{
		if (processes[i].config.envs != NULL)
		{
			for (int key_index = 0; processes[i].config.envs[key_index]; key_index++)
			{
				free(processes[i].config.envs[key_index]);
				processes[i].config.envs[key_index] = NULL;
			}
			free(processes[i].config.envs);
			processes[i].config.envs = NULL;
		}
		if (processes[i].config.cmd != NULL)
		{
			for (int key_index = 0; processes[i].config.cmd[key_index]; key_index++)
			{
				free(processes[i].config.cmd[key_index]);
				processes[i].config.cmd[key_index] = NULL;
			}
			free(processes[i].config.cmd);
			processes[i].config.cmd = NULL;
		}
		if (processes[i].children != NULL)
		{
			free(processes[i].children);
			processes[i].children = NULL;
		}
	}

	free(processes);
}
