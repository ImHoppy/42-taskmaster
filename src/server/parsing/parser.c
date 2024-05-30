#include "cJSON.h"
#include "taskmasterd.h"
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

status_t parse_config(const cJSON *const json_config, process_t *processes)
{
	const cJSON *process = NULL;
	int i = 0;
	cJSON_ArrayForEach(process, json_config)
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
		if (!ft_stris(processes[i].config.name, isalpha))
		{
			fprintf(stderr, "Error: process name \"%s\" must contain only alphabetic characters\n", processes[i].config.name);
			return FAILURE;
		}
		if ((processes[i].config.cmd = split_space(cmd->valuestring)) == NULL)
			return FAILURE;

		if (processes[i].config.cmd[0][0] != '/' && processes[i].config.cmd[0][0] != '.')
		{
			if ((processes[i].config.cmd[0] = get_absolute_cmd_path(processes[i].config.cmd[0])) == NULL)
				return FAILURE;
		}
		else
		{
			if (access(processes[i].config.cmd[0], X_OK) == -1)
				return FAILURE;
		}

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

status_t check_unique_name(process_t *processes, int processes_len)
{
	for (int i = 0; i < processes_len; i++)
	{
		for (int j = i + 1; j < processes_len; j++)
		{
			if (strcmp(processes[i].config.name, processes[j].config.name) == 0)
			{
				fprintf(stderr, "Error: process name \"%s\" is not unique\n", processes[i].config.name);
				return FAILURE;
			}
		}
	}
	return SUCCESS;
}

void processes_default_config(process_t *processes, int processes_len)
{
	for (int i = 0; i < processes_len; i++)
	{
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

		printf("\tcmd: ");
		for (int j = 0; processes[i].config.cmd[j]; j++)
		{
			printf(" %s", processes[i].config.cmd[j]);
		}
		printf("\n");
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
			printf("\tEnv: %s\n", processes[i].config.envs[j]);
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
		printf("\tautorestart: %s\n", processes[i].config.autorestart == UNEXPECTED ? "UNEXPECTED" : (processes[i].config.autorestart == ALWAYS ? "ALWAYS" : "NEVER"));
		printf("\tstdout_logfile: %s\n", processes[i].config.stdout_logfile);
		printf("\tstderr_logfile: %s\n", processes[i].config.stderr_logfile);
		printf("\tworkingdir: %s\n", processes[i].config.workingdir);
		printf("}\n");
	}
}

static void child_default_values(process_child_t *child)
{
	child->state = NON_STARTED;
}

static status_t child_assign_name(process_config_t *config, process_child_t *child, int child_index)
{
	int numprocs_len = ft_intlen(config->numprocs);
	int name_len = strlen(config->name) + (numprocs_len < 2 ? 2 : numprocs_len) + 2;
	child->name = calloc(sizeof(char), name_len);
	if (child->name == NULL)
	{
		return FAILURE;
	}
	snprintf(child->name, name_len, "%s_%02d", config->name, child_index);
	return SUCCESS;
}

program_json_t check_valid_json(cJSON *json)
{
	if (json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			log_error("Error before: %s", error_ptr);
		}
		return (program_json_t){.fail = 1, .len = 0, .programs = NULL};
	}

	if ((json->type & 0xFF) != cJSON_Object)
	{
		log_error("Error: config must be an objects");
		return (program_json_t){.fail = 1, .len = 0, .programs = NULL};
	}

	const cJSON *programs = get_value(json, "programs", false, cJSON_Array, "cJSON_Array");
	if (programs == NULL)
		return (program_json_t){.fail = 1, .len = 0, .programs = NULL};

	int len = cJSON_GetArraySize(programs);
	if ((programs->type & 0xFF) != cJSON_Array || len == 0)
	{
		fprintf(stderr, "Error: config must be an array of objects\n");
		return (program_json_t){.fail = 1, .len = 0, .programs = NULL};
	}

	return (program_json_t){.fail = 0, .len = len, .programs = programs};
}

status_t init_children(process_t *processes, int process_len)
{
	for (int process_index = 0; process_index < process_len; process_index++)
	{
		process_t *process = &(processes[process_index]);
		if (process->children != NULL)
			continue;
		process->children = calloc(sizeof(process_child_t), process->config.numprocs);
		if (process->children == NULL)
			return FAILURE;
		for (uint32_t child_index = 0; child_index < process->config.numprocs; child_index++)
		{
			child_default_values(&(process->children[child_index]));
			if (child_assign_name(&(process->config), &(process->children[child_index]), child_index) == FAILURE)
				return FAILURE;
		}
	}

	return SUCCESS;
}

status_t init_config(const char *const config)
{
	g_taskmaster.json_config = cJSON_Parse(config);
	// printf("%s\n", cJSON_Print(json_config));

	program_json_t valid_json = check_valid_json(g_taskmaster.json_config);
	if (valid_json.fail)
		return FAILURE;

	g_taskmaster.processes_len = valid_json.len;
	const cJSON *programs = valid_json.programs;

	get_key_from_json(g_taskmaster.json_config, serverfile, true, cJSON_String);
	get_key_from_json(g_taskmaster.json_config, logfile, true, cJSON_String);
	if (!assign_non_empty_string(&g_taskmaster.serverfile, serverfile))
		return FAILURE;
	if (!assign_non_empty_string(&g_taskmaster.logfile, logfile))
		return FAILURE;

	g_taskmaster.processes = calloc(sizeof(process_t), g_taskmaster.processes_len);
	if (g_taskmaster.processes == NULL)
		return FAILURE;
	processes_default_config(g_taskmaster.processes, g_taskmaster.processes_len);

	if (parse_config(programs, g_taskmaster.processes) == FAILURE)
		return FAILURE;

	if (check_unique_name(g_taskmaster.processes, g_taskmaster.processes_len) == FAILURE)
		return FAILURE;

	init_children(g_taskmaster.processes, g_taskmaster.processes_len);
	print_config(g_taskmaster.processes, g_taskmaster.processes_len);
	return SUCCESS;
}
