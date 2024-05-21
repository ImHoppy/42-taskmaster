#include <time.h>
#include <stdlib.h>
int main(int ac, char **av)
{
	if (ac < 2)
		return 1;
	int time = atoi(av[1]);
	int c = clock();
	while ((clock() - c) / CLOCKS_PER_SEC < time)
		;
	return 0;
}
