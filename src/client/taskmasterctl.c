#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "input/readline.h"
#include "taskmasterctl.h"

int done = 0;

int main(int ac, char **av)
{
	char *line, *s;
	int sfd = initialize_socket(ac > 1 ? av[1] : "/tmp/taskmasterd.sock");
	if (sfd < 0)
	{
		fprintf(stderr, "Failed to connect to the server\n");
		exit(1);
	}
	initialize_readline(); /* Bind our completer. */

	/* Loop reading and executing lines until the user quits. */
	while (done == 0)
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
			if (write(client.sfd, s, strlen(s)) < 0)
			{
				fprintf(stderr, "Failed to send command to the server\n");
				break;
			}
			char s[1025] = {0};
			int n = 0;
			while ((n = read(client.sfd, s, 1024)) > 0)
			{
				if (n < 0)
				{
					fprintf(stderr, "Failed to read response from the server\n");
					break;
				}
				if (n == 0)
					break;
				write(1, s, n);
				if (n < 1024)
					break;
			}
		}

		free(line);
	}
	printf("Have a nice day!\n");
	rl_clear_history();
	exit(0);
}
