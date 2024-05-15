
#include "cJSON.h"
#include "taskmasterd.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include "headers/libunixsocket.h"

int main(int ac, char **av)
{
	if (ac < 2 && av[1] == NULL)
	{
		fprintf(stderr, "Usage: %s <config_file>\n", av[0]);
		return 1;
	}
	FILE *config = fopen(av[1], "r");
	if (config == NULL)
	{
		fprintf(stderr, "Error opening config file\n");
		return 1;
	}

	int size_file = 0;
	fseek(config, 0, SEEK_END);
	size_file = ftell(config);
	fseek(config, 0, SEEK_SET);

	char *config_str = calloc(sizeof(char), size_file + 1);
	if (config_str == NULL)
	{
		fprintf(stderr, "Error: malloc failed\n");
		fclose(config);
		return 1;
	}
	fread(config_str, sizeof(char), size_file, config);

	taskmaster_t taskmaster = {0};
	status_t ret = init_config(config_str, &taskmaster);
	free(config_str);
	fclose(config);
	if (ret == FAILURE)
	{
		fprintf(stderr, "Error: init_config failed\n");
		return 1;
	}

	ret = create_unix_server_socket("/tmp/taskmasterd.sock", LIBSOCKET_STREAM, 0);
	if (ret == FAILURE)
	{
		fprintf(stderr, "Error creating unix server socket: %s\n", strerror(errno));
		free_taskmaster(&taskmaster);
		return 1;
	}


	while (1)
	{
		// Handle each process
		handler(&taskmaster);
	}
	free_taskmaster(&taskmaster);
	return 0;
}
