#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main(int ac, char **av, char **envp)
{
	for (int i = 0; envp[i]; i++)
	{
		printf("%s\n", envp[i]);
	}
}
