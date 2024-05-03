#ifndef TASKMASTER_H
#define TASKMASTER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
	SUCCESS = 0,
	FAILURE = -1
} status_t;

typedef struct
{
	const char *key;
	const char *value;
} env_t;

typedef struct
{
	const char *name;
	const char *cmd;
	int numprocs;
	env_t *envs;
	int envs_count;
} process_t;

//
// Parsing
//
status_t init_config(const char *const config);

#endif
