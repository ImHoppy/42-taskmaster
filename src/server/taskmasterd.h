#ifndef TASKMASTER_H
#define TASKMASTER_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "cJSON.h"

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

typedef enum
{
	STOPPED = 1 << 0,
	STARTING = 1 << 1,
	RUNNING = 1 << 2,
	BACKOFF = 1 << 3,
	STOPPING = 1 << 4,
	EXITED = 1 << 5,
	FATAL = 1 << 6,
	UNKNOWN = 1 << 7,
} process_state_t;

typedef enum autorestart_t
{
	NEVER = 0,
	ALWAYS = 1,
	UNEXPECTED = 2,
} autorestart_t;

typedef struct
{
	const char *name;
	const char *cmd;
	uint32_t numprocs;
	env_t *envs;
	int envs_count;
	int umask;
	uint8_t stopsignal;
	// (ref stopwaitsecs) seconds to wait to receive a SIGCHLD before sending a SIGKILL
	uint32_t stoptime;
	// number of retries before the process is considered failed
	uint32_t startretries;
	// (ref startsecs) seconds to wait for the process to start before giving up
	uint32_t starttime;
	bool autostart;
	autorestart_t autorestart;
	bool exitcodes[256];
	const char *stdout_logfile;
	const char *stderr_logfile;
	const char *workingdir;
} process_config_t;

typedef struct
{
	process_state_t state;
	process_config_t config;
	clock_t starting_time;
} process_t;

typedef struct {
	cJSON *processes_config;
	process_t *processes;
	int processes_len;
} taskmaster_t;


static const struct
{
	const char *name;
	int value;
}

signal_list[] = {
	{"TERM", 15},
	{"HUP", 1},
	{"INT", 2},
	{"QUIT", 3},
	{"KILL", 9},
	{"USR1", 10},
	{"USR2", 12},
};

static const int signal_list_len = sizeof(signal_list) / sizeof(signal_list[0]);

//
// Parsing
//
status_t init_config(const char *const config, taskmaster_t *taskmaster);
bool parse_envs(const cJSON *const envs_obj, process_t *processes);
bool assign_exitcodes(bool exitcodes[256], const cJSON *const exitcodes_obj);
bool assign_non_empty_string(const char **variable, const cJSON *const value);
bool assign_string(const char **variable, const cJSON *const value);
bool assign_bool(bool *variable, const cJSON *const value);
bool assign_non_zero_uint32(uint32_t *variable, const cJSON *const value);
bool assign_umask(int *umask_var, const cJSON *const umask);
bool assign_signal(uint8_t *stopsignal, const cJSON *const signal);
bool assign_non_negative(uint32_t *variable, const cJSON *const value);
bool assign_autorestart(autorestart_t *variable, const cJSON *const value);

status_t routine(taskmaster_t *taskmaster);

void free_processes(process_t *const processes, int processes_len);

#endif