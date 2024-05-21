#include "taskmasterd.h"

static uint32_t get_paths_len(char *path)
{
	int count = 1;
	int pos = 0;

	while ((path = strchr(&(path[pos]), ':')))
	{
		pos++;
		count++;
	}

	return count;
}

static char **split_path_env()
{
	char *path;
	if ((path = getenv("PATH")) == NULL)
		return NULL;

	uint32_t paths_len = get_paths_len(path);

	char **paths = calloc(sizeof(char *), paths_len + 1);
	if (paths == NULL)
	{
		free(path);
		return NULL;
	}

	uint32_t i = 0;
	int pos = 0;
	uint32_t len = 0;
	char *tmp;

	while ((tmp = strchr(&path[pos], ':')) && i < paths_len)
	{
		len = strlen(&(path[pos])) - strlen(tmp);
		paths[i] = calloc(sizeof(char), len + 1);
		if (paths[i] == NULL)
		{

			return NULL;
		}

		strncpy(paths[i], &(path[pos]), len);
		pos = pos + len + 1;
		i++;
	}

	return paths;
}

char *get_absolute_cmd_path(char *cmd)
{
	if (access(cmd, X_OK) == 0)
	{
		return cmd;
	}

	char **paths = split_path_env();
	if (paths == NULL)
	{
		free(cmd);
		return NULL;
	}

	for (int i = 0; paths[i]; i++)
	{
		char *current_path = calloc(sizeof(char), strlen(paths[i]) + strlen(cmd) + 2);
		if (current_path == NULL)
			return NULL;
		strncpy(current_path, paths[i], strlen(paths[i]));
		strncat(current_path, "/", 1);
		strncat(current_path, cmd, strlen(cmd));

		if (access(current_path, X_OK) == 0)
		{
			free(cmd);
			free_paths(paths);
			return current_path;
		}
		else
			free(current_path);
	}

	free(cmd);
	free_paths(paths);
	return NULL;
}
