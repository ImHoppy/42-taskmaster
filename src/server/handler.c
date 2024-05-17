#include "taskmasterd.h"

status_t child_creation(taskmaster_t *taskmaster, process_t *process, uint32_t child_index)
{
	pid_t pid = fork();

	if (pid == -1)
	{
		fprintf(stderr, "Error occured during the fork.");
		return FAILURE;
	}
	else if (pid == 0)
	{
		execve(process->config.cmd[0], process->config.cmd, process->config.envs);
		free_taskmaster(taskmaster);
		exit(127);
	}
	else if (pid > 0)
	{
		fprintf(stdout, "Child is created %d\n", pid);
		process->children[child_index].pid = pid;
	}

	return SUCCESS;
}

void backoff_handling(process_child_t *child, process_t *current_process)
{
	if (child->retries_number < current_process->config.startretries)
	{
		if (child->backoff_time == 0)
		{
			child->backoff_time = clock();
		}
		else if ((clock() - child->backoff_time) / CLOCKS_PER_SEC >= 5)
		{
			printf("BACKOFF\n");
			child->state = STARTING;
			child->backoff_time = 0;
		}
	}
	else
	{
		child->state = FATAL;
		child->backoff_time = 0;
	}
}

void starting_handling(process_child_t *child, process_t *current_process)
{
	uint32_t start_spent_time =
		(clock() - child->starting_time) / CLOCKS_PER_SEC;

	if (start_spent_time == current_process->config.starttime)
	{
		child->state = RUNNING;
	}
	else if (current_process->config.starttime == 0)
	{
		child->state = RUNNING;
	}
}

status_t auto_start_handling(taskmaster_t *taskmaster, process_child_t *child, process_t *current_process, uint32_t child_index)
{
	if (child_creation(taskmaster, current_process, child_index) == FAILURE)
		return FAILURE;
	child->state = STARTING;
	child->starting_time = clock();
	printf("Child is starting\n");

	return SUCCESS;
}

void exit_handling(int status, process_child_t *child)
{
	if (WIFSIGNALED(status))
		fprintf(stderr, "Child is terminated because of signal non-intercepted %d\n", WTERMSIG(status));
	if (WIFEXITED(status))
	{
		fprintf(stderr, "Child is terminated with exit code %d and state %d\n",
				WEXITSTATUS(status), child->state);
		// When program exit in STARTING state
		if (child->state == STARTING)
		{
			child->state = BACKOFF;
			child->retries_number++;
		}
	}
}

status_t handler(taskmaster_t *taskmaster)
{
	int process_number = cJSON_GetArraySize(taskmaster->json_config);
	for (int i = 0; i < process_number; i++)
	{
		int status;

		process_t *current_process = &(taskmaster->processes[i]);
		for (uint32_t child_index = 0; child_index < current_process->config.numprocs; child_index++)
		{
			process_child_t *child = &(current_process->children[child_index]);

			if ((child->state == STOPPED || child->state == EXITED) &&
				current_process->config.autostart == true)
			{
				if (auto_start_handling(taskmaster, child, current_process, child_index) == FAILURE)
					return FAILURE;
			}
			else if (child->state == STARTING)
				starting_handling(child, current_process);
			else if (child->state == BACKOFF)
				backoff_handling(child, current_process);

			status = 0;
			if (child->pid == 0)
			{
				continue;
			}
			int ret = waitpid(child->pid, &status, WNOHANG);
			if (ret == -1)
			{
				fprintf(stderr, "Waitpid error %d\n", child->pid);
				return FAILURE;
			}
			else if (ret > 0)
			{
				exit_handling(status, child);
				child->pid = 0;
			}
		}
	}
	return SUCCESS;
}

void murder_my_children(taskmaster_t *taskmaster)
{
	for (int process_index = 0; process_index < taskmaster->processes_len; process_index++)
	{
		process_t *current_process = &(taskmaster->processes[process_index]);
		for (uint32_t child_index = 0; child_index < current_process->config.numprocs; child_index++)
		{
			process_child_t *child = &(current_process->children[child_index]);
			if (child->pid > 0)
			{
				kill(child->pid, SIGINT);
				child->pid = 0;
			}
		}
	}
}
