#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>

#include "input/readline.h"

int done = 0;

int main(void)
{
	char *line, *s;

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
		}

		free(line);
	}
	printf("Have a nice day!\n");
	rl_clear_history();
	exit(0);
}
