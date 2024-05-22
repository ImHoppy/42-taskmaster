#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "input/readline.h"
#include "taskmasterctl.h"

#define CLEAR_LINE "\033[A\33[2KT\r"

client_t g_client = {0};

int read_socket(bool doWrite)
{
	if (!g_client.buffer)
	{
		g_client.buffer = calloc(sizeof(char), 1024 + 1);
		g_client.buffer_len = 1024;
		if (!g_client.buffer)
		{
			fprintf(stderr, "Failed to allocate memory for the buffer\n");
			g_client.fatal_error = true;
			return -1;
		}
	}
	bzero(g_client.buffer, g_client.buffer_len);
	char buf[1025] = {0};
	int total_read = 0;
	int n = 0;
	while ((n = read(g_client.sfd, buf, 1024)) > 0)
	{
		total_read += n;
		if (n < 0)
		{
			fprintf(stderr, "Failed to read response from the server\n");
			g_client.fatal_error = true;
			break;
		}
		if (n == 0)
			break;
		if (doWrite)
			write(1, buf, n);
		if (total_read < g_client.buffer_len)
			strcat(g_client.buffer, buf);
		else
		{
			g_client.buffer = realloc(g_client.buffer, g_client.buffer_len + 1024 + 1);
			g_client.buffer_len += 1024;
			strcat(g_client.buffer, buf);
		}
		if (n < 1024)
			break;
	}
	return n;
}

int main(int ac, char **av)
{
	char *line, *s;

	g_client.sfd = initialize_socket(ac > 1 ? av[1] : "/tmp/taskmasterd.sock");
	if (g_client.sfd < 0)
	{
		fprintf(stderr, "Failed to connect to the server\n");
		exit(1);
	}
	initialize_readline(); /* Bind our completer. */

	/* Loop reading and executing lines until the user quits. */
	while (g_client.fatal_error == false)
	{
		line = readline("> ");

		if (!line)
			break;

		/* Remove leading and trailing whitespace from the line.
		   Then, if there is anything left, add it to the history list
		   and execute it. */
		s = stripwhite(line);

		if (*s)
		{
			add_history(s);
			execute_line(s);
		}

		free(line);
	}
	printf("Have a nice day!\n");
	free(g_client.buffer);
	rl_clear_history();
	exit(0);
}
