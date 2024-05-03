#include "cjson/cJSON.h"
#include "taskmaster.h"
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

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
	if (value != NULL && (value->type & 0xFF) != target_type)
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
	if (envs_obj == NULL || cJSON_GetArraySize(envs_obj) == 0)
	{
		processes->envs = NULL;
		processes->envs_count = 0;
		return SUCCESS;
	}
	processes->envs = malloc(sizeof(env_t) * cJSON_GetArraySize(envs_obj));
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
			fprintf(stderr, "Error: env is not a string\n");
			return FAILURE;
		}

		processes->envs[env_index].key = any_env->string;
		processes->envs[env_index].value = any_env->valuestring;
		env_index++;
	}
	processes->envs_count = env_index;

	return SUCCESS;
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

		if (parse_envs(env, &processes[i]) == FAILURE)
		{
			return FAILURE;
		}

		if (cJSON_IsString(name) && (name->valuestring != NULL) &&
			cJSON_IsString(cmd) && (cmd->valuestring != NULL) &&
			cJSON_IsNumber(numprocs) && (numprocs->valueint > 0))
		{
			processes[i].name = name->valuestring;
			processes[i].cmd = cmd->valuestring;
			processes[i].numprocs = numprocs->valueint;
			i++;
		}
		else
		{
			return FAILURE;
		}
	}
	return SUCCESS;
}

status_t init_config(const char *const config)
{
	cJSON *processes_config = cJSON_Parse(config);
	printf("%s\n", cJSON_Print(processes_config));
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

	process_t *processes = malloc(sizeof(process_t) * cJSON_GetArraySize(processes_config));
	if (processes == NULL)
	{
		cJSON_Delete(processes_config);
		return FAILURE;
	}

	if (parse_config(processes_config, processes) == FAILURE)
	{
		cJSON_Delete(processes_config);
		free(processes);
		return FAILURE;
	}
	cJSON_Delete(processes_config);
	free(processes);
	return FAILURE;
}
