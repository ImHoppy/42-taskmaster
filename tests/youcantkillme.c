#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

void signal_hand()
{
	printf("Bouuuh looser\n");
}

int main(int ac, char **av)
{
	signal(SIGTERM, signal_hand);
	while (1)
	{
		sleep(1);
	}
	return 0;
}
