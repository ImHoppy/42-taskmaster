#include <time.h>
#include <unistd.h>
#include <stdlib.h>

int main(int ac, char **av)
{
	if (ac < 2)
		return 1;
	int total_time = atoi(av[1]);
	int c = time(NULL);
	while ((time(NULL) - c) < total_time)
	{
		sleep(1);
	}
	return 0;
}
