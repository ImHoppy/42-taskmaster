#include "taskmasterd.h"

status_t child_creation(process_t *process, uint32_t child_index)
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
		free_taskmaster();
		exit(127);
	}
	else if (pid > 0)
	{
		fprintf(stdout, "%s: Child is created %d\n", process->children[child_index].name, pid);
		process->children[child_index].pid = pid;
	}

	return SUCCESS;
}

static void murder_child(process_child_t *child)
{
	if (child->pid > 0)
	{
		kill(child->pid, SIGINT);
		child->pid = 0;
	}
}

void backoff_handling(process_child_t *child, process_t *current_process)
{
	if (child->retries_number < current_process->config.startretries)
	{
		if (child->backoff_time == 0)
		{
			child->backoff_time = time(NULL);
		}
		else if ((time(NULL) - child->backoff_time) >= 5)
		{
			fprintf(stdout, "%s: Child is BACKOFF \n", child->name);
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

void running_handling(process_child_t *child, process_t *current_process)
{
	uint32_t start_spent_time =
		(time(NULL) - child->starting_time);

	if (start_spent_time == current_process->config.starttime)
	{
		child->state = RUNNING;
		child->retries_number = 0;
		fprintf(stdout, "%s: Child is running\n", child->name);
	}
	else if (current_process->config.starttime == 0)
	{
		child->state = RUNNING;
		child->retries_number = 0;
		fprintf(stdout, "%s: Child is running\n", child->name);
	}
}

status_t start_handling(process_child_t *child, process_t *current_process, uint32_t child_index)
{
	murder_child(child);
	if (child_creation(current_process, child_index) == FAILURE)
		return FAILURE;

	child->state = STARTING;
	child->starting_time = time(NULL);
	fprintf(stdout, "%s: Child is restarting\n", child->name);

	return SUCCESS;
}

bool is_exit_code_accepted(int status, bool exitcodes[256])
{
	return exitcodes[status];
}

status_t exit_handling(int status, process_child_t *child, process_t *current_process, uint32_t child_index)
{
	if (WIFSIGNALED(status))
	{
		child->pid = 0;
		if (child->state == STOPPING)
			child->state = STOPPED;
		fprintf(stderr, "%s: Child is terminated because of signal non-intercepted %d\n", child->name, WTERMSIG(status));
	}
	if (WIFEXITED(status))
	{
		child->pid = 0;
		fprintf(stderr, "%s: Child is terminated with exit code %d and state %s\n",
				child->name, WEXITSTATUS(status), state_to_string(child->state));
		// When program exit in STARTING state
		if (child->state == STARTING)
		{
			child->state = BACKOFF;
			child->retries_number++;
		}
		// When program exit but have autorestart enabled
		else if (child->state == RUNNING)
		{
			if (current_process->config.autorestart == ALWAYS)
			{
				if (start_handling(child, current_process, child_index) == FAILURE)
					return FAILURE;
			}
			else if (current_process->config.autorestart == UNEXPECTED)
			{
				child->state = EXITED;
				fprintf(stderr, "%s: Child is exited: %s\n", child->name, state_to_string(child->state));
				if (!is_exit_code_accepted(WEXITSTATUS(status), current_process->config.exitcodes))
				{
					fprintf(stderr, "%s: Restarting because of exit code is not accepted\n", child->name);
					if (start_handling(child, current_process, child_index) == FAILURE)
						return FAILURE;
				}
			}
		}

		if (child->state == STOPPING)
			child->state = STOPPED;
	}
	return SUCCESS;
}

status_t handler()
{
	int process_number = cJSON_GetArraySize(g_taskmaster.json_config);
	for (int i = 0; i < process_number; i++)
	{
		int status;

		process_t *current_process = &(g_taskmaster.processes[i]);
		for (uint32_t child_index = 0; child_index < current_process->config.numprocs; child_index++)
		{
			process_child_t *child = &(current_process->children[child_index]);

			if ((child->state == NON_STARTED && current_process->config.autostart) ||
				(child->state == EXITED &&
				 current_process->config.autorestart == ALWAYS))
			{
				fprintf(stderr, "%s: START %d\n", child->name, child->pid);
				if (start_handling(child, current_process, child_index) == FAILURE)
					return FAILURE;
			}
			else if (child->state == STARTING)
				running_handling(child, current_process);
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
				fprintf(stderr, "%s: Waitpid error %d\n", child->name, child->pid);
				return FAILURE;
			}
			else if (ret > 0)
			{
				exit_handling(status, child, current_process, child_index);
			}
		}
	}
	return SUCCESS;
}

void murder_my_children()
{
	for (int process_index = 0; process_index < g_taskmaster.processes_len; process_index++)
	{
		process_t *current_process = &(g_taskmaster.processes[process_index]);
		for (uint32_t child_index = 0; child_index < current_process->config.numprocs; child_index++)
		{
			murder_child(&(current_process->children[child_index]));
		}
	}
}
