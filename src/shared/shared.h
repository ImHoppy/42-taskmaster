#ifndef SHARED_H
#define SHARED_H

#include <stdlib.h>
#include <stdbool.h>

typedef enum
{
	STOPPED,
	STARTING,
	RUNNING,
	BACKOFF,
	STOPPING,
	EXITED,
	FATAL,
	UNKNOWN,
} process_state_t;

const char *state_to_string(process_state_t state);

char **split_space(const char *to_split);
size_t ft_intlen(long n);
int ft_stris(const char *s, int (*f)(int));

#endif
