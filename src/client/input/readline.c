
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>
#include <readline/chardefs.h>

#include "readline.h"
#include "taskmasterctl.h"

/* Execute a command line. */
int execute_line(char *line)
{
	register int i;
	char *word;

	/* Isolate the command word. */
	i = 0;
	while (line[i] && whitespace(line[i]))
		i++;
	word = line + i;

	while (line[i] && !whitespace(line[i]))
		i++;

	if (line[i])
		line[i++] = '\0';

	const COMMAND *command = find_command(word);

	if (!command)
	{
		fprintf(stderr, "%s: No such command for taskmasterctl.\n", word);
		return (-1);
	}

	/* Get argument to command, if any. */
	while (whitespace(line[i]))
		i++;

	word = line + i;

	/* Call the function. */
	return ((*(command->func))(word));
}

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
const COMMAND *find_command(char *name)
{
	register int i;

	for (i = 0; commands[i].name; i++)
		if (strcmp(name, commands[i].name) == 0)
			return (&commands[i]);

	return ((COMMAND *)NULL);
}

/* Strip whitespace from the start and end of STRING.  Return a pointer
   into STRING. */
char *stripwhite(char *string)
{
	register char *s, *t;

	for (s = string; whitespace(*s); s++)
		;

	if (*s == 0)
		return (s);

	t = s + strlen(s) - 1;
	while (t > s && whitespace(*t))
		t--;
	*++t = '\0';

	return s;
}

/* **************************************************************** */
/*                                                                  */
/*                  Interface to Readline Completion                */
/*                                                                  */
/* **************************************************************** */

char *command_generator(const char *text, int state);
char *programs_generator(const char *text, int state);
char **tm_completion(const char *text, int start, int end);

/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames
   if not. */
void initialize_readline(void)
{
	/* Allow conditional parsing of the ~/.inputrc file. */
	rl_readline_name = "TaskMaster";

	/* Tell the completer that we want a crack first. */
	rl_attempted_completion_function = tm_completion;
}

/* Attempt to complete on the contents of TEXT.  START and END show the
   region of TEXT that contains the word to complete.  We can use the
   entire line in case we want to do some simple parsing.  Return the
   array of matches, or NULL if there aren't any. */
char **tm_completion(const char *text, int start, int end)
{
	(void)end;
	char **matches;

	matches = (char **)NULL;

	/* If this word is at the start of the line, then it is a command
	   to complete.  Otherwise it is the name of a file in the current
	   directory. */
	if (start == 0)
		matches = rl_completion_matches(text, command_generator);
	if (start > 0)
	{
		request_list();
		// find autocompletion
		matches = rl_completion_matches(text, programs_generator);
	}
	return (matches);
}

/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char *command_generator(const char *text, int state)
{
	static int list_index, len;
	char *name;

	/* If this is a new word to complete, initialize now.  This includes
	   saving the length of TEXT for efficiency, and initializing the index
	   variable to 0. */
	if (!state)
	{
		list_index = 0;
		len = strlen(text);
	}

	/* Return the next name which partially matches from the command list. */
	while ((name = commands[list_index].name))
	{
		list_index++;

		if (strncmp(name, text, len) == 0)
			return (strdup(name));
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}

char *programs_generator(const char *text, int state)
{
	static size_t list_index, len;
	char *name;

	/* If this is a new word to complete, initialize now.  This includes
	   saving the length of TEXT for efficiency, and initializing the index
	   variable to 0. */
	if (!state)
	{
		list_index = 0;
		len = strlen(text);
	}

	if (g_client.programs_len == 0 || g_client.programs == NULL)
		return ((char *)NULL);

	/* Return the next name which partially matches from the programs list. */
	while (list_index < g_client.programs_len)
	{
		name = g_client.programs[list_index].name;
		list_index++;
		if (strncmp(name, text, len) == 0)
			return (strdup(name));
	}

	/* If no names matched, then return NULL. */
	return ((char *)NULL);
}
