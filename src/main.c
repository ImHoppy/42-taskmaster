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
	char config_str[1024] = {0};

	int size_file = 0;
	fseek(config, 0, SEEK_END);
	size_file = ftell(config);
	fseek(config, 0, SEEK_SET);

	fread(config_str, 1, size_file, config);
	init_config(config_str);

	fclose(config);
	return 0;
}
