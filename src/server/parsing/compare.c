#include "taskmasterd.h"

static bool compare_string(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
	{
		if (a == NULL && b == NULL)
			return true;
		return false;
	}
	return strcmp(a, b) == 0;
}

static bool compare_array_string(char **a_cmd, char **b_cmd)
{
	int a_i = 0;
	int b_i = 0;

	if (a_cmd == NULL || b_cmd == NULL)
	{
		if (a_cmd == NULL && b_cmd == NULL)
			return true;
		return false;
	}

	while (a_cmd[a_i] && b_cmd[b_i])
	{
		if (strcmp(a_cmd[a_i], b_cmd[b_i]) != 0)
			return false;
		a_i++;
		b_i++;
	}
	return a_cmd[a_i] == b_cmd[b_i];
}

static bool compare_exitcodes(bool a_exitcodes[256], bool b_exitcodes[256])
{
	for (int i = 0; i < 256; i++)
	{
		if (a_exitcodes[i] != b_exitcodes[i])
			return false;
	}
	return true;
}

/// @brief Check each variable in process_t.config
/// @param a Left hand to check
/// @param b Right hand to check
/// @return true if the config is equal
bool compare_process_config(process_t *a, process_t *b)
{
	if (!compare_array_string(a->config.cmd, b->config.cmd))
		return false;
	if (a->config.numprocs != b->config.numprocs)
		return false;
	if (a->config.envs_count != b->config.envs_count || !compare_array_string(a->config.envs, b->config.envs))
		return false;
	if (a->config.umask != b->config.umask)
		return false;
	if (a->config.stopsignal != b->config.stopsignal)
		return false;
	if (a->config.stoptime != b->config.stoptime)
		return false;
	if (a->config.startretries != b->config.startretries)
		return false;
	if (a->config.starttime != b->config.starttime)
		return false;
	if (a->config.autostart != b->config.autostart)
		return false;
	if (a->config.autorestart != b->config.autorestart)
		return false;
	if (!compare_exitcodes(a->config.exitcodes, b->config.exitcodes))
		return false;
	if (!compare_string(a->config.stdout_logfile, b->config.stdout_logfile))
		return false;
	if (!compare_string(a->config.stderr_logfile, b->config.stderr_logfile))
		return false;
	if (!compare_string(a->config.workingdir, b->config.workingdir))
		return false;
	return true;
}
