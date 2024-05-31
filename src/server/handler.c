#include "taskmasterd.h"

static void close_fd(int stdout_fd, int stderr_fd)
{
	if (stdout_fd > 0)
		close(stdout_fd);
	if (stderr_fd > 0)
		close(stderr_fd);
}

status_t child_creation(process_t *process, process_child_t *child)
{
	pid_t pid = fork();

	if (pid == -1)
	{
		log_fatal("Error occured during the fork.");
		return FAILURE;
	}
	else if (pid == 0)
	{
		int stdout_fd = -1;
		int stderr_fd = -1;

		if (process->config.stdout_logfile)
		{
			stdout_fd = open(process->config.stdout_logfile, O_CREAT | O_RDWR | O_APPEND, 0644);
			if (stdout_fd < 0)
				log_error("%s: Error in opening stdout: %s", process->config.name, strerror(errno));
			if (dup2(stdout_fd, STDOUT_FILENO) < 0)
				log_error("%s: Error in dup2 stdout: %s", process->config.name, strerror(errno));
		}

		if (process->config.stderr_logfile)
		{
			stderr_fd = open(process->config.stderr_logfile, O_CREAT | O_RDWR | O_APPEND, 0644);
			if (stderr_fd < 0)
				log_error("%s: Error in opening stderr: %s", process->config.name, strerror(errno));
			if (dup2(stderr_fd, STDERR_FILENO) < 0)
				log_error("%s: Error in dup2 stderr: %s", process->config.name, strerror(errno));
		}

		close_fd(stdout_fd, stderr_fd);

		if (process->config.workingdir)
		{
			if (chdir(process->config.workingdir) < 0)
			{
				log_error("%s: Error in chdir %s: %s", process->config.name, process->config.workingdir, strerror(errno));
				free_taskmaster();
				exit(126);
			}
		}
		umask(process->config.umask);
		execve(process->config.cmd[0], process->config.cmd, process->config.envs);
		free_taskmaster();
		exit(127);
	}
	else if (pid > 0)
	{
		child->pid = pid;
	}

	return SUCCESS;
}

status_t backoff_handling(process_child_t *child, process_t *current_process)
{
	if (child->retries_number < current_process->config.startretries)
	{
		if (child->backoff_time == 0)
		{
			child->backoff_time = time(NULL);
		}
		else if ((time(NULL) - child->backoff_time) >= 5)
		{
			log_debug("%s: Retrying to start", child->name);
			child->backoff_time = 0;
			if (start_handling(child, current_process) == FAILURE)
				return FAILURE;
		}
	}
	else
	{
		child->state = FATAL;
		child->backoff_time = 0;
	}
	return SUCCESS;
}

void running_handling(process_child_t *child, process_t *current_process)
{
	uint32_t start_spent_time =
		(time(NULL) - child->starting_time);

	if (start_spent_time == current_process->config.starttime ||
		current_process->config.starttime == 0)
	{
		child->state = RUNNING;
		child->retries_number = 0;
		log_debug("%s has finish starting in %d seconds", child->name, start_spent_time);
	}
}

status_t start_handling(process_child_t *child, process_t *current_process)
{
	if (child->pid > 0)
	{
		log_fatal("start_handling when child already exist");
		return SUCCESS;
	}
	if (child_creation(current_process, child) == FAILURE)
		return FAILURE;

	child->state = STARTING;
	child->starting_time = time(NULL);
	log_info("Starting %s", child->name);

	return SUCCESS;
}

bool is_exit_code_accepted(int status, bool exitcodes[256])
{
	return exitcodes[status];
}

status_t exit_handling(int status, process_child_t *child, process_t *current_process)
{
	if (WIFSIGNALED(status))
	{
		child->pid = 0;
		if (child->state == STOPPING)
			child->state = STOPPED;
		else
			child->state = EXITED;
		log_debug("%s: Child is terminated because of signal non-intercepted %d", child->name, WTERMSIG(status));
	}
	if (WIFEXITED(status))
	{
		child->pid = 0;
		log_info("%s: Child is terminated with exit code %d and state %s",
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
				if (start_handling(child, current_process) == FAILURE)
					return FAILURE;
			}
			else if (current_process->config.autorestart == UNEXPECTED)
			{
				child->state = EXITED;
				if (!is_exit_code_accepted(WEXITSTATUS(status), current_process->config.exitcodes))
				{
					log_debug("%s: Restarting because of exit code is not accepted", child->name);
					if (start_handling(child, current_process) == FAILURE)
						return FAILURE;
				}
			}
		}
		else if (child->state == STOPPING)
		{
			child->state = STOPPED;
		}
	}
	return SUCCESS;
}

status_t handler()
{
	int process_number = g_taskmaster.processes_len;
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
				if (start_handling(child, current_process) == FAILURE)
					return FAILURE;
				log_debug("New child: %s with pid: %d", child->name, child->pid);
			}
			else if (child->state == STARTING)
				running_handling(child, current_process);
			else if (child->state == BACKOFF)
			{
				if (backoff_handling(child, current_process) == FAILURE)
					return FAILURE;
			}
			else if (child->state == STOPPING && (time(NULL) - child->starting_stop) >= current_process->config.stoptime)
			{
				child->state = STOPPED;
				log_info("Hard stopping %s", child->name);
				if (child->pid != 0)
					kill(child->pid, SIGKILL);
			}
			else if (child->state == STOPPED && child->need_restart && child->pid == 0)
			{
				child->need_restart = false;
				if (start_handling(child, current_process) == FAILURE)
					return FAILURE;
			}

			status = 0;
			if (child->pid == 0)
			{
				continue;
			}
			int ret = waitpid(child->pid, &status, WNOHANG);
			if (ret == -1)
			{
				log_fatal("%s: Waitpid error with pid: %d", child->name, child->pid);
				return FAILURE;
			}
			else if (ret > 0)
			{
				exit_handling(status, child, current_process);
			}
		}
	}
	return SUCCESS;
}
