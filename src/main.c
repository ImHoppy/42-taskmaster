#include "cjson/cJSON.h"
#include "taskmaster.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

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
	init_config(config_str);

	free(config_str);
	fclose(config);
	return 0;
}
