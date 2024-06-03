
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "readline.h"
#include "shared.h"
#include "taskmasterctl.h"

/* When non-zero, this global means the user is done using this program. */
extern int done;

static int valid_argument(char *caller, char *arg);

int com_start(char *arg)
{
	if (!valid_argument("start", arg))
		return 1;

	dprintf(g_client.sfd, "start %s\n", arg);
	read_socket(false);
	printf("%s\n", g_client.buffer);
	return 0;
}

int com_restart(char *arg)
{
	if (!valid_argument("restart", arg))
		return 1;

	dprintf(g_client.sfd, "restart %s\n", arg);
	read_socket(false);
	printf("%s\n", g_client.buffer);
	return 0;
}

int com_stop(char *arg)
{
	if (!valid_argument("stop", arg))
		return 1;

	dprintf(g_client.sfd, "stop %s\n", arg);
	read_socket(false);
	printf("%s\n", g_client.buffer);
	return 0;
}

/* Get status of program NAME */
int com_status(char *arg)
{
	if (!valid_argument("status", arg))
		return 1;

	dprintf(g_client.sfd, "status %s\n", arg);
	read_socket(false);
	printf("%s\n", g_client.buffer);
	return 0;
}

void request_list(void)
{
	const char *s = "list";
	if (write(g_client.sfd, s, strlen(s)) < 0)
	{
		g_client.fatal_error = true;
		fprintf(stderr, "Failed to send command to the server\n");
		return;
	}
	read_socket(false);

	size_t total_programs = 0;
	for (int i = 0; g_client.buffer[i] && i < g_client.buffer_len; i++)
	{
		if (g_client.buffer[i] == ';')
			total_programs++;
	}
	for (size_t i = 0; i < g_client.programs_len; i++)
	{
		free(g_client.programs[i].name);
		g_client.programs[i].name = NULL;
	}
	if (g_client.programs_len < total_programs)
	{
		g_client.programs = realloc(g_client.programs, (total_programs + 1) * sizeof(program_t));
		assert(g_client.programs && "Failed to allocate memory for programs");
		g_client.programs_len = total_programs;
	}
	bzero(g_client.programs, (total_programs + 1) * sizeof(program_t));

	char *line = strtok(g_client.buffer, ";");
	int i = 0;
	while (line)
	{
		char *saveptr;
		char *name = strtok_r(line, " ", &saveptr);
		char *status = strtok_r(NULL, " ", &saveptr);
		if (name && status)
		{
			g_client.programs[i].name = strdup(name);
			g_client.programs[i].status = atoi(status);
			i++;
		}
		line = strtok(NULL, ";");
	}
}

/* List programs in taskmaster configuration. */
int com_list(char *arg)
{
	(void)arg;
	request_list();
	for (size_t i = 0; i < g_client.programs_len; i++)
	{
		printf("%s\t- %s\n", g_client.programs[i].name, state_to_string(g_client.programs[i].status));
	}
	return 0;
}

/* The user wishes to quit using this program.  Just set DONE non-zero. */
int com_quit(char *arg)
{
	(void)arg;
	g_client.fatal_error = 1;
	return (0);
}

/* Print out help for ARG, or for all of the commands if ARG is
   not present. */
int com_help(char *arg)
{
	register int i;
	int printed = 0;

	for (i = 0; commands[i].name; i++)
	{
		if ((!arg || !*arg) || (strcmp(arg, commands[i].name) == 0))
		{
			printf("%s\t\t%s.\n", commands[i].name, commands[i].doc);
			printed++;
		}
	}

	if (!printed)
	{
		printf("No commands match `%s'.  Possibilties are:\n", arg);

		for (i = 0; commands[i].name; i++)
		{
			/* Print in six columns. */
			if (printed == 6)
			{
				printed = 0;
				printf("\n");
			}

			printf("%s\t", commands[i].name);
			printed++;
		}

		if (printed)
			printf("\n");
	}
	return (0);
}

/* Return non-zero if ARG is a valid argument for CALLER, else print
   an error message and return zero. */
static int valid_argument(char *caller, char *arg)
{
	if (!arg || !*arg)
	{
		fprintf(stderr, "%s: Argument required.\n", caller);
		return (0);
	}

	return (1);
}
