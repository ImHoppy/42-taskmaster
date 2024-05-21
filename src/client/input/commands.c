
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "readline.h"
#include "taskmasterctl.h"

/* When non-zero, this global means the user is done using this program. */
extern int done;

static int valid_argument(char *caller, char *arg);

int com_start(char *arg)
{
	if (!valid_argument("start", arg))
		return 1;

	assert("Not implemented yet");
	return 0;
}

int com_restart(char *arg)
{
	if (!valid_argument("restart", arg))
		return 1;

	assert("Not implemented yet");
	return 0;
}

int com_stop(char *arg)
{
	if (!valid_argument("stop", arg))
		return 1;

	assert("Not implemented yet");
	return 0;
}

/* Get status of program NAME */
int com_status(char *arg)
{
	if (!valid_argument("status", arg))
		return 1;

	assert("Not implemented yet");
	return 0;
}

/* List programs in taskmaster configuration. */
int com_list(char *arg)
{
	(void)arg;
	assert("Not implemented yet");
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
