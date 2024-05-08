#include "cjson/cJSON.h"
#include "taskmaster.h"
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const cJSON *get_value(const cJSON *const object, const char *const key, bool optional, int target_type)
{
	if (object == NULL)
		return NULL;
	if (key == NULL || key[0] == '\0')
		return NULL;
	const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key);
	if (value == NULL && !optional)
	{
		fprintf(stderr, "Error: key \"%s\" not found\n", key);
		return NULL;
	}
	if (value != NULL && ((value->type & 0xFF) & target_type) == 0)
	{
		fprintf(stderr, "Error: key \"%s\" is not of type %d\n", key, target_type);
		return NULL;
	}
	return value;
}

#define get_key_from_json(object, key, optional, target_type)          \
	const cJSON *key = get_value(object, #key, optional, target_type); \
	if (!optional && key == NULL)                                      \
		return FAILURE;

status_t parse_config(const cJSON *const processes_config, process_t *processes)
{
	const cJSON *process = NULL;
	int i = 0;
	cJSON_ArrayForEach(process, processes_config)
	{
		get_key_from_json(process, name, false, cJSON_String);
		get_key_from_json(process, cmd, false, cJSON_String);
		get_key_from_json(process, numprocs, false, cJSON_Number);
		get_key_from_json(process, env, true, cJSON_Object);
		get_key_from_json(process, umask, true, cJSON_String);
		get_key_from_json(process, stopsignal, true, cJSON_String | cJSON_Number);

		if (parse_envs(env, &processes[i]) == FAILURE)
			return FAILURE;
		if (!assign_non_empty_string(&processes[i].name, name->string, name->valuestring))
			return FAILURE;
		if (!assign_non_empty_string(&processes[i].cmd, cmd->string, cmd->valuestring))
			return FAILURE;
		if (!assign_non_zero_int(&processes[i].numprocs, numprocs->string, numprocs->valueint))
			return FAILURE;
		if (!assign_umask(&processes[i].umask, umask))
			return FAILURE;
		if (!assign_signal(&processes[i].stopsignal, stopsignal))
			return FAILURE;
		i++;
	}
	return SUCCESS;
}

void processes_default_value(process_t *processes, int processes_len)
{
	for (int i = 0; i < processes_len; i++)
	{
		processes[i].name = NULL;
		processes[i].cmd = NULL;
		processes[i].numprocs = 1;
		processes[i].envs = NULL;
		processes[i].envs_count = 0;
		processes[i].umask = 0022;
		processes[i].stopsignal = 15; // SIGTERM
	}
}

void print_config(const process_t *const processes, int processes_len)
{
	for (int i = 0; i < processes_len; i++)
	{
		printf("%d = {\n", i);
		printf("\tname: %s\n", processes[i].name);
		printf("\tcmd: %s\n", processes[i].cmd);
		printf("\tnumprocs: %d\n", processes[i].numprocs);
		printf("\tumask: 0%o\n", processes[i].umask);
		for (int j = 0; j < signal_list_len; j++)
		{
			if (signal_list[j].value == processes[i].stopsignal)
			{
				printf("\tstopsignal: %s\n", signal_list[j].name);
				break;
			}
		}
		printf("\tenvs: %d\n", processes[i].envs_count);
		for (int j = 0; j < processes[i].envs_count; j++)
		{
			printf("\t[%d]: %s=%s\n", j, processes[i].envs[j].key, processes[i].envs[j].value);
		}
		printf("}\n");
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
		if (processes[i].envs != NULL)
		{
			free(processes[i].envs);
			processes[i].envs = NULL;
		}
	}
	free(processes);
}

status_t init_config(const char *const config)
{
	cJSON *processes_config = cJSON_Parse(config);
	// printf("%s\n", cJSON_Print(processes_config));
	if (processes_config == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			fprintf(stderr, "Error before: %s\n", error_ptr);
		}
		cJSON_Delete(processes_config);
		return FAILURE;
	}
	int processes_len = cJSON_GetArraySize(processes_config);
	process_t *processes = malloc(sizeof(process_t) * processes_len);
	if (processes == NULL)
	{
		cJSON_Delete(processes_config);
		return FAILURE;
	}
	processes_default_value(processes, processes_len);

	if (parse_config(processes_config, processes) == FAILURE)
	{
		cJSON_Delete(processes_config);
		free(processes);
		return FAILURE;
	}
	print_config(processes, processes_len);
	cJSON_Delete(processes_config);
	free_processes(processes, processes_len);
	return FAILURE;
}
