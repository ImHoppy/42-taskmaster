#include "taskmasterd.h"

void free_taskmaster(taskmaster_t *taskmaster)
{
	free_processes(taskmaster->processes, taskmaster->processes_len);
	cJSON_Delete(taskmaster->json_config);
	taskmaster->json_config = NULL;
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