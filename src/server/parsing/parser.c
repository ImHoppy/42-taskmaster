#include "cJSON.h"
#include "taskmasterd.h"
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const cJSON *get_value(const cJSON *const object, const char *const key, bool optional, int target_type, const char *const type_msg)
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
		fprintf(stderr, "Error: key \"%s\" is not of type %s\n", key, type_msg);
		return NULL;
	}
	return value;
}

#define get_key_from_json(object, key, optional, target_type)                        \
	const cJSON *key = get_value(object, #key, optional, target_type, #target_type); \
	if (!optional && key == NULL)                                                    \
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
		get_key_from_json(process, stoptime, true, cJSON_Number);
		get_key_from_json(process, startretries, true, cJSON_Number);
		get_key_from_json(process, starttime, true, cJSON_Number);
		get_key_from_json(process, autostart, true, cJSON_True | cJSON_False);
		get_key_from_json(process, autorestart, true, cJSON_True | cJSON_False | cJSON_String);
		get_key_from_json(process, exitcodes, true, cJSON_Array | cJSON_Number);
		get_key_from_json(process, stdout_logfile, true, cJSON_String);
		get_key_from_json(process, stderr_logfile, true, cJSON_String);
		get_key_from_json(process, workingdir, true, cJSON_String);

		if (!parse_envs(env, &processes[i]))
			return FAILURE;
		if (!assign_non_empty_string(&processes[i].config.name, name))
			return FAILURE;
		if (!assign_non_empty_string(&processes[i].config.cmd, cmd))
			return FAILURE;
		if (!assign_non_zero_uint32(&processes[i].config.numprocs, numprocs))
			return FAILURE;
		if (!assign_umask(&processes[i].config.umask, umask))
			return FAILURE;
		if (!assign_signal(&processes[i].config.stopsignal, stopsignal))
			return FAILURE;
		if (!assign_non_zero_uint32(&processes[i].config.stoptime, stoptime))
			return FAILURE;
		if (!assign_non_zero_uint32(&processes[i].config.startretries, startretries))
			return FAILURE;
		if (!assign_non_negative(&processes[i].config.starttime, starttime))
			return FAILURE;
		if (!assign_bool(&processes[i].config.autostart, autostart))
			return FAILURE;
		if (!assign_autorestart(&processes[i].config.autorestart, autorestart))
			return FAILURE;
		if (!assign_exitcodes(processes[i].config.exitcodes, exitcodes))
			return FAILURE;
		if (!assign_string(&processes[i].config.stdout_logfile, stdout_logfile))
			return FAILURE;
		if (!assign_string(&processes[i].config.stderr_logfile, stderr_logfile))
			return FAILURE;
		if (!assign_string(&processes[i].config.workingdir, workingdir))
			return FAILURE;
		i++;
	}
	return SUCCESS;
}

void processes_default_value(process_t *processes, int processes_len)
{
	for (int i = 0; i < processes_len; i++)
	{
		processes[i].state = STOPPED;

		processes[i].config.name = NULL;
		processes[i].config.cmd = NULL;
		processes[i].config.numprocs = 1;
		processes[i].config.envs = NULL;
		processes[i].config.envs_count = 0;
		processes[i].config.umask = 0022;
		processes[i].config.stopsignal = 15; // SIGTERM
		processes[i].config.stoptime = 10;
		processes[i].config.startretries = 3;
		processes[i].config.starttime = 1;
		processes[i].config.autostart = true;
		processes[i].config.exitcodes[0] = true;
		processes[i].config.autorestart = UNEXPECTED;
		processes[i].config.stdout_logfile = NULL;
		processes[i].config.stderr_logfile = NULL;
		processes[i].config.workingdir = NULL;
	}
}

void print_config(const process_t *const processes, int processes_len)
{
	for (int i = 0; i < processes_len; i++)
	{
		printf("%d = {\n", i);
		printf("\tname: %s\n", processes[i].config.name);
		printf("\tcmd: %s\n", processes[i].config.cmd);
		printf("\tnumprocs: %d\n", processes[i].config.numprocs);
		printf("\tumask: 0%o\n", processes[i].config.umask);
		for (int j = 0; j < signal_list_len; j++)
		{
			if (signal_list[j].value == processes[i].config.stopsignal)
			{
				printf("\tstopsignal: %s\n", signal_list[j].name);
				break;
			}
		}
		printf("\tenvs: %d\n", processes[i].config.envs_count);
		for (int j = 0; j < processes[i].config.envs_count; j++)
		{
			printf("\t[%d]: %s=%s\n", j, processes[i].config.envs[j].key, processes[i].config.envs[j].value);
		}
		printf("\texitcodes: [ ");
		for (int j = 0; j < 256; j++)
		{
			if (processes[i].config.exitcodes[j])
			{
				printf("%d ", j);
			}
		}
		printf("]\n");
		printf("\tstartretries: %d\n", processes[i].config.startretries);
		printf("\tstarttime: %d\n", processes[i].config.starttime);
		printf("\tstoptime: %d\n", processes[i].config.stoptime);
		printf("\tautostart: %s\n", processes[i].config.autostart ? "true" : "false");
		printf("\tautorestart: %d\n", processes[i].config.autorestart);
		printf("\tstdout_logfile: %s\n", processes[i].config.stdout_logfile);
		printf("\tstderr_logfile: %s\n", processes[i].config.stderr_logfile);
		printf("\tworkingdir: %s\n", processes[i].config.workingdir);
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
		if (processes[i].config.envs != NULL)
		{
			free(processes[i].config.envs);
			processes[i].config.envs = NULL;
		}
	}
	free(processes);
}

status_t init_config(const char *const config, taskmaster_t *taskmaster)
{
	taskmaster->processes_config = cJSON_Parse(config);
	// printf("%s\n", cJSON_Print(processes_config));
	if (taskmaster->processes_config == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			fprintf(stderr, "Error before: %s\n", error_ptr);
		}
		cJSON_Delete(taskmaster->processes_config);
		return FAILURE;
	}

	int processes_len = cJSON_GetArraySize(taskmaster->processes_config);
	if (processes_len <= 0 || (taskmaster->processes_config->type & 0xFF) != cJSON_Array)
	{
		fprintf(stderr, "Error: config must be an array of objects\n");
		cJSON_Delete(taskmaster->processes_config);
		return FAILURE;
	}

	taskmaster->processes = calloc(sizeof(process_t), processes_len);
	if (taskmaster->processes == NULL)
	{
		cJSON_Delete(taskmaster->processes_config);
		return FAILURE;
	}
	processes_default_value(taskmaster->processes, processes_len);

	if (parse_config(taskmaster->processes_config, taskmaster->processes) == FAILURE)
	{
		cJSON_Delete(taskmaster->processes_config);
		free_processes(taskmaster->processes, processes_len);
		return FAILURE;
	}
	print_config(taskmaster->processes, processes_len);

	return SUCCESS;
}
