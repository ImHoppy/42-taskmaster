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
		int len = strcspn(process->config.cmd, " ");
		char *command_path = calloc(sizeof(char), len + 1);
		strncpy(command_path, process->config.cmd, len);
		fprintf(stdout, "Command_path %s\n", command_path);

		// while (getpid() % 2 == 0)
		// {
		// }

		// execve(command_path, process->config.cmd, process->config.envs);
		exit(1);
	}
	else if (pid > 0)
	{
		fprintf(stdout, "Child is created %d\n", pid);
		process->children[child_index].pid = pid;
	}

	return SUCCESS;
}

void exit_handling(process_child_t *child)
{
	if (child->state == RUNNING)
	{
		child->state = STOPPING;
		// time is spending between each status ?
		child->state = STOPPED;
	}
}

void restart_handling(process_child_t *child)
{
	if (child->state == FATAL)
	{
		child->retries_number = 0;
		child->state = STARTING;
		child->starting_time = clock();
	}
}

status_t handler(taskmaster_t *taskmaster)
{
	int process_number = cJSON_GetArraySize(taskmaster->processes_config);
	while (1)
	{
		for (int i = 0; i < process_number; i++)
		{
			int status;

			process_t *current_process = &(taskmaster->processes[i]);
			for (uint32_t child_index = 0; child_index < current_process->config.numprocs; child_index++)
			{
				process_child_t *child = &(current_process->children[child_index]);

				if ((child->state == STOPPED ||
					 child->state == EXITED) &&
					current_process->config.autostart == true)
				{
					if (child_creation(current_process, child_index) == FAILURE)
						return FAILURE;
					child->state = STARTING;
					child->starting_time = clock();
				}
				else if (child->state == STARTING)
				{
					uint32_t time_spent =
						(clock() - child->starting_time) / CLOCKS_PER_SEC;

					// if process->pid == 0, create only fork and skip next check
					if (time_spent == current_process->config.starttime)
					{
						child->state = RUNNING;
					}
					else if (current_process->config.starttime == 0)
					{
						child->state = RUNNING;
					}
				}
				//  else if (child->state == BACKOFF)
				// {
				// 	//
				// 	if (current_process->config.startretries <
				// child->retries_number)
				// 	{
				// 		// backof time < 5

				// 		// backof time >= 5 -> state starting, set pid = 0
				// 	} else {
				// 		child->state = FATAL;
				// 	}
				// }

				// WNOHANG --> avoid blocking of father
				//  Error : -1, 0 (with WNOHANG) if children has not changed is state, PID
				//  children (success) WIFSIGNALED(status) --> TRUE if exit with signal
				//  						--> WTERMSIG(status) to
				//  obtain signal number waitpid();

				// Exit !user action
				//  		uint32_t time_spent = (clock() - process->starting_time)
				//  / CLOCKS_PER_SEC; if (process->state == STARTING && time_spent <
				//  process->config.starttime) { 	process->state = BACKOFF;
				//  	process->retries_number++;
				//  }

				// WNOHANG : Avoid blocking if child is not terminated
				status = 0;
				if (child->pid == 0)
				{
					continue;
				}
				int ret = waitpid(child->pid, &status, WNOHANG);
				if (ret == -1)
				{
					fprintf(stderr, "Error when waitpid %d\n", child->pid);
				}
				else if (ret > 0)
				{
					if (WIFSIGNALED(status))
						fprintf(
							stderr,
							"Child is terminated because of signal non-intercepted %d\n",
							WTERMSIG(status));
					if (WIFEXITED(status))
						fprintf(stderr, "Child is terminated with exit code %d\n",
								WEXITSTATUS(status));
					child->pid = 0;
				}
			}
		}
	}
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
// Signal SIGCONT ?
