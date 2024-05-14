#include "cJSON.h"
#include "taskmasterd.h"
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

bool parse_envs(const cJSON *const envs_obj, process_t *processes)
{
	uint32_t envs_count = cJSON_GetArraySize(envs_obj);
	if (envs_obj == NULL || envs_count == 0)
	{
		processes->config.envs = NULL;
		processes->config.envs_count = 0;
		return true;
	}

	if (envs_count > UINT16_MAX)
	{
		fprintf(stderr, "Error: envs count must be less than %d\n", UINT16_MAX);
		return false;
	}

	processes->config.envs = calloc(envs_count + 1, sizeof(char *));
	if (processes->config.envs == NULL)
	{
		fprintf(stderr, "Error: malloc failed\n");
		return false;
	}

	const cJSON *any_env = NULL;
	int env_index = 0;
	cJSON_ArrayForEach(any_env, envs_obj)
	{
		if (!cJSON_IsString(any_env) && any_env->valuestring == NULL)
		{
			fprintf(stderr, "Error: env %s is not a string\n", any_env->string);
			return false;
		}
		processes->config.envs[env_index] = calloc(strlen(any_env->string) + strlen(any_env->valuestring) + 2, sizeof(char));
		strncat(processes->config.envs[env_index], any_env->string, strlen(any_env->string));
		strncat(processes->config.envs[env_index], "=", 1);
		strncat(processes->config.envs[env_index], any_env->valuestring, strlen(any_env->valuestring));

		env_index++;
	}
	processes->config.envs_count = env_index;

	return true;
}
static bool assign_exitcode(bool exitcodes[256], int exitcode)
{
	if (exitcode < 0 || exitcode > 255)
	{
		fprintf(stderr, "Error: exitcode must be between 0 and 255\n");
		return false;
	}
	exitcodes[exitcode] = true;
	return true;
}

bool assign_exitcodes(bool exitcodes[256], const cJSON *const exitcodes_obj)
{
	if (exitcodes_obj == NULL || exitcodes == NULL)
		return true;

	exitcodes[0] = false;
	if ((exitcodes_obj->type & 0xFF) == cJSON_Array)
	{
		if (cJSON_GetArraySize(exitcodes_obj) == 0)
		{
			fprintf(stderr, "Error: exitcodes must not be empty\n");
			return false;
		}
		const cJSON *exitcode = NULL;
		cJSON_ArrayForEach(exitcode, exitcodes_obj)
		{
			if ((exitcode->type & 0xFF) != cJSON_Number)
			{
				fprintf(stderr, "Error: exitcode must be a number\n");
				return false;
			}
			if (!assign_exitcode(exitcodes, exitcode->valueint))
				return false;
		}
	}
	else if ((exitcodes_obj->type & 0xFF) == cJSON_Number)
		assign_exitcode(exitcodes, exitcodes_obj->valueint);
	else
	{
		fprintf(stderr, "Error: exitcodes must be an array or a number\n");
		return false;
	}

	return true;
}

bool assign_non_empty_string(const char **variable, const cJSON *const value)
{
	if (value == NULL || variable == NULL)
		return true;
	if (value->valuestring == NULL || value->valuestring[0] == '\0')
	{
		fprintf(stderr, "Error: %s must not be empty\n", value->string);
		return false;
	}
	*variable = value->valuestring;
	return true;
}

bool assign_string(const char **variable, const cJSON *const value)
{
	if (value == NULL || variable == NULL)
		return true;
	*variable = value->valuestring;
	return true;
}

bool assign_bool(bool *variable, const cJSON *const value)
{
	if (value == NULL || variable == NULL)
		return true;
	*variable = (value->type & 0xFF) == cJSON_True;
	return true;
}

bool assign_non_zero_uint32(uint32_t *variable, const cJSON *const value)
{
	if (value == NULL || variable == NULL)
		return true;
	if (value->valueint <= 0 || value->valueint > INT32_MAX)
	{
		fprintf(stderr, "Error: %s must be greater than 0\n", value->string);
		return false;
	}
	*variable = value->valueint;
	return true;
}

bool assign_umask(int *umask_var, const cJSON *const umask)
{
	if (umask == NULL || umask_var == NULL)
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

bool assign_signal(uint8_t *stopsignal, const cJSON *const signal)
{
	if (signal == NULL || stopsignal == NULL)
		return true;

	if ((signal->type & 0xFF) == cJSON_Number)
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
	else if ((signal->type & 0xFF) == cJSON_String)
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

bool assign_non_negative(uint32_t *variable, const cJSON *const value)
{
	if (value == NULL || variable == NULL)
		return true;
	if (value->valueint < 0 || value->valueint > INT32_MAX)
		return false;

	*variable = value->valueint;
	return true;
}

bool assign_autorestart(autorestart_t *variable, const cJSON *const value)
{
	if (value == NULL || variable == NULL)
		return true;
	if ((value->type & 0xFF) == cJSON_True)
		*variable = ALWAYS;
	else if ((value->type & 0xFF) == cJSON_False)
		*variable = NEVER;
	else if ((value->type & 0xFF) == cJSON_String && strcmp(value->valuestring, "unexpected") == 0)
		*variable = UNEXPECTED;
	else
	{
		fprintf(stderr, "Error: autorestart must be a boolean or \"unexpected\"\n");
		return false;
	}
	return true;
}
