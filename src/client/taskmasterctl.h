#ifndef TASKMASTERCTL_H
#define TASKMASTERCTL_H

#include "headers/libunixsocket.h"
#include <stdlib.h>
#include <stdbool.h>

#include "input/readline.h"

int initialize_socket(const char *socket_path);
int read_socket(bool doWrite);

typedef struct
{
	int sfd;
	char *buffer;
	int buffer_len;
	program_t *programs;
	size_t programs_len;

	bool fatal_error;
} client_t;

extern client_t g_client;

#endif
