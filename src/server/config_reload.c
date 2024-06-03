
#include "cJSON.h"
#include "taskmasterd.h"

process_t *find_process_config_by_name(process_t *processes, int processes_len, const char *name)
{
	for (int i = 0; i < processes_len; i++)
	{
		if (strcmp(processes[i].config.name, name) == 0)
			return &processes[i];
	}
	return NULL;
}

void reload_config()
{
	log_info("Configuration file %s reloaded.", g_taskmaster.config_file);

	char *config_str = load_config_file(g_taskmaster.config_file);
	if (config_str == NULL)
	{
		log_error("Failed to reload non existing configuration file.");
		return;
	}

	cJSON *json_config = cJSON_Parse(config_str);
	free(config_str);

	program_json_t valid_program = check_valid_json(json_config);
	if (valid_program.fail)
	{
		log_error("Missing programs in configuration file.");
		cJSON_Delete(json_config);
		return;
	}

	process_t *new_processes = calloc(sizeof(process_t), valid_program.len);
	if (new_processes == NULL)
	{
		g_taskmaster.running = false;
		cJSON_Delete(json_config);
		return;
	}
	processes_default_config(new_processes, valid_program.len);

	if (parse_config(valid_program.programs, new_processes) == FAILURE || check_unique_name(new_processes, valid_program.len) == FAILURE)
	{
		g_taskmaster.running = false;
		cJSON_Delete(json_config);
		free_processes(new_processes, valid_program.len);
		return;
	}

	init_children(new_processes, valid_program.len);

	for (size_t i = 0; i < valid_program.len; i++)
	{
		process_t *process = find_process_config_by_name(g_taskmaster.processes, g_taskmaster.processes_len, new_processes[i].config.name);
		// Non existing on old config
		if (process == NULL)
		{
			continue;
		}
		// changed config
		if (!compare_process_config(&new_processes[i], process))
		{
			// process KILL
			for (size_t child_index = 0; child_index < process->config.numprocs; child_index++)
			{
				murder_child(&g_taskmaster.processes[i].children[child_index], SIGKILL);
				int status;
				waitpid(g_taskmaster.processes[i].children[child_index].pid, &status, 0);
			}
		}
		else
		{
			// unchanged
			process_child_t *tmp = new_processes[i].children;
			new_processes[i].children = process->children;
			process->children = tmp;
		}
	}

	// Kill all process non existing in new config
	for (int i = 0; i < g_taskmaster.processes_len; i++)
	{
		process_t *process = find_process_config_by_name(new_processes, valid_program.len, g_taskmaster.processes[i].config.name);
		if (process == NULL)
		{
			for (size_t child_index = 0; child_index < g_taskmaster.processes[i].config.numprocs; child_index++)
			{
				murder_child(&g_taskmaster.processes[i].children[child_index], SIGKILL);
				int status;
				waitpid(g_taskmaster.processes[i].children[child_index].pid, &status, 0);
			}
		}
	}

	process_t *tmp = g_taskmaster.processes;
	g_taskmaster.processes = new_processes;
	new_processes = tmp;

	int len_tmp = g_taskmaster.processes_len;
	g_taskmaster.processes_len = valid_program.len;
	valid_program.len = len_tmp;

	free_processes(new_processes, valid_program.len);
	cJSON_Delete(g_taskmaster.json_config);
	g_taskmaster.json_config = json_config;
}
