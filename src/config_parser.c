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

status_t parse_envs(const cJSON *const envs_obj, process_t *processes)
{
	uint32_t envs_count = cJSON_GetArraySize(envs_obj);
	if (envs_obj == NULL || envs_count == 0)
	{
		processes->envs = NULL;
		processes->envs_count = 0;
		return SUCCESS;
	}

	if (envs_count > UINT16_MAX)
	{
		fprintf(stderr, "Error: envs count must be less than %d\n", UINT16_MAX);
		return FAILURE;
	}

	processes->envs = malloc(sizeof(env_t) * envs_count);
	if (processes->envs == NULL)
	{
		fprintf(stderr, "Error: malloc failed\n");
		return FAILURE;
	}

	const cJSON *any_env = NULL;
	int env_index = 0;
	cJSON_ArrayForEach(any_env, envs_obj)
	{
		if (!cJSON_IsString(any_env) && (any_env->valuestring == NULL))
		{
			fprintf(stderr, "Error: env %s is not a string\n", any_env->string);
			return FAILURE;
		}

		processes->envs[env_index].key = any_env->string;
		processes->envs[env_index].value = any_env->valuestring;
		env_index++;
	}
	processes->envs_count = env_index;

	return SUCCESS;
}
bool assign_non_empty_string(const char **variable, const char *variable_name, char *str)
{
	if (str == NULL || str[0] == '\0')
	{
		fprintf(stderr, "Error: %s must not be empty\n", variable_name);
		return false;
	}
	*variable = str;
	return true;
}

bool assign_non_zero_int(int *variable, const char *variable_name, int value)
{
	if (value <= 0)
	{
		fprintf(stderr, "Error: %s must be greater than 0\n", variable_name);
		return false;
	}
	*variable = value;
	return true;
}

bool assign_umask(int *umask_var, const cJSON *const umask)
{
	if (umask == NULL)
		return true;
	if (strlen(umask->valuestring) != 3)
		return false;

	for (int i = 0; i < 3; i++)
	{
		if (umask->valuestring[i] < '0' || umask->valuestring[i] > '7')
		{
			fprintf(stderr, "Error: umask must be 3 digits between 0 and 7\n");
			return false;
		}
	}
	*umask_var = strtol(umask->valuestring, NULL, 8);
	return true;
}

static const struct
{
	const char *name;
	int value;
} signal_list[] = {
	{"TERM", 15},
	{"HUP", 1},
	{"INT", 2},
	{"QUIT", 3},
	{"KILL", 9},
	{"USR1", 10},
	{"USR2", 12},
};
static const int signal_list_len = sizeof(signal_list) / sizeof(signal_list[0]);

bool assign_signal(uint8_t *stopsignal, const cJSON *const signal)
{
	if (signal == NULL)
		return true;

	if (signal->type == cJSON_Number)
	{
		for (int i = 0; i < signal_list_len; i++)
		{
			if (signal_list[i].value == signal->valueint)
			{
				*stopsignal = signal_list[i].value;
				return true;
			}
		}
	}
	else if (signal->type == cJSON_String)
	{
		for (int i = 0; i < signal_list_len; i++)
		{
			if (strcmp(signal_list[i].name, signal->valuestring) == 0)
			{
				*stopsignal = signal_list[i].value;
				return true;
			}
		}
	}
	else
	{
		fprintf(stderr, "Error: stopsignal must be a string or number\n");
		return false;
	}
	fprintf(stderr, "Error: stopsignal %s unknown, must be one of", signal->valuestring);
	for (int i = 0; i < signal_list_len; i++)
	{
		fprintf(stderr, " %s", signal_list[i].name);
	}
	fprintf(stderr, "\n");
	return false;
}

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
	free(processes);
	return FAILURE;
}
