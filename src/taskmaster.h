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
	uint32_t numprocs;
	env_t *envs;
	int envs_count;
	int umask;
	uint8_t stopsignal;
	uint32_t startretries;
	uint32_t starttime;
	bool exitcodes[256];
} process_t;

//
// Parsing
//
status_t init_config(const char *const config);

static const struct
{
	const char *name;
	int value;
} signal_list[] = {
	{"TERM", 15},
	{"HUP", 1},
	{"INT", 2},
	{"QUIT", 3},
	{"KILL", 9},
	{"USR1", 10},
	{"USR2", 12},
};
static const int signal_list_len = sizeof(signal_list) / sizeof(signal_list[0]);

status_t parse_envs(const cJSON *const envs_obj, process_t *processes);
bool assign_exitcodes(bool exitcodes[256], const cJSON *const exitcodes_obj);
bool assign_non_empty_string(const char **variable, const char *variable_name, char *str);
bool assign_non_zero_uint32(uint32_t *variable, const cJSON *const value);
bool assign_umask(int *umask_var, const cJSON *const umask);
bool assign_signal(uint8_t *stopsignal, const cJSON *const signal);
bool assign_non_negative(uint32_t *variable, const cJSON *const value);

#endif
