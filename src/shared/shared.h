#ifndef SHARED_H
#define SHARED_H

#include <stdlib.h>
#include <stdbool.h>

char **split_space(const char *to_split);
size_t ft_intlen(long n);
int ft_stris(const char *s, int (*f)(int));

#endif
