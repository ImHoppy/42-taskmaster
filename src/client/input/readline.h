#ifndef TM_READLINE_H
#define TM_READLINE_H

#include <stddef.h>

typedef int (*CMDFunction)(char *);
typedef struct
{
	char *name;		  /* User printable name of the function. */
	CMDFunction func; /* Function to call to do the job. */
	char *doc;		  /* Documentation for this function.  */
} COMMAND;

/* Forward declarations. */
char *stripwhite(char *string);
const COMMAND *find_command(char *name);
void initialize_readline(void);
int execute_line(char *line);

/* The names of functions that actually do the manipulation. */
int com_list(char *);
int com_start(char *);
int com_restart(char *);
int com_stop(char *);
int com_status(char *);
int com_help(char *);
int com_quit(char *);

/* A structure which contains information on the commands this program
   can understand. */
static const COMMAND commands[] = {
	{"start", com_start, "Start program NAME"},
	{"restart", com_restart, "Restart program NAME"},
	{"stop", com_stop, "Stop program NAME"},
	{"status", com_status, "Get status of program NAME"},
	{"list", com_list, "List of current programs"},
	{"quit", com_quit, "Quit this program"},
	{"help", com_help, "Display this text"},
	{"?", com_help, "Synonym for `help'"},
	{(char *)NULL, (CMDFunction)NULL, (char *)NULL},
};

typedef struct
{
	char *name;
	int status;
} program_t;

/* On the real project, this should be a list of programs that are currently running. */
static const program_t programs[] = {
	{"test", 0},
	{"nginx", 0},
	{"node", 0},
	{NULL, 0},
};

#endif
