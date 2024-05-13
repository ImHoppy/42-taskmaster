#include "taskmasterd.h"

// status_t user_start_process(char *process_name) {
// 	// find process
// 	process_t *process = NULL;

// 	if (process == NULL) {
// 		// repond  à l'user process inconnu
// 		return (FAILURE);
// 	}

// 	if (process->state == STOPPED || process->state == FATAL || process->state == EXITED) {
// 		process->state = STARTING;
// 		// repond ok à user
// 		return (SUCCESS);
// 	}
// 	// Reponse à lúser quiíl ne peut pas
// 	return (FAILURE);
// }

// bool set_running_state(process_t *process, bool user_action) {
// 	if(process == NULL)
// 		return FAILURE;

// 	if (process->state == STOPPED && 
// 	(process->config.autostart || user_action)) {
// 		process->state = STARTING;
// 	}

// 	return SUCCESS;
// }

// // while (1) {
// // 	current_process.state == STARTING {
// // 		current_time = ;
// // 		start_time = current_process.start_time 
// // 		// Check pid still valid and not greater than config startSec 
// // 		if start_time > (current_process.config.startsec * 1000) + current_time {
// // 			current_process.state = RUNNING;
// // 		}
// // 		else if have exited early {
// // 			current_process.tentative++;
// // 				current_process.state = BACKOFF;
// // 			if (current_process.tentative >= current_process.config.startretries)
// // 				current_process.state = FATAL;
// // 		}
// // 	}
// // 	current_process.state == EXITED && current_process.config.autorestart == true {
// // 		state = STARTING
// // 	}
// // }

status_t routine(taskmaster_t *taskmaster) {
	int process_number = taskmaster->processes_len;

	while(1) {
		for(int i = 0; i < process_number; i++) {
			process_t *current_process = &(taskmaster->processes[i]);

			if ((current_process->state == STOPPED || current_process->state == EXITED) \
				&& current_process->config.autostart == true) {
				current_process->state = STARTING;
				current_process->starting_time = clock();
			}
			
			else if (current_process->state == STARTING) {
				uint32_t time_spent = (clock() - current_process->starting_time) / CLOCKS_PER_SEC;
				
				if (time_spent == current_process->config.starttime) { 
					current_process->state = RUNNING;
				} else if (current_process->config.starttime == 0) {
					current_process->state = RUNNING;
				}
			}

			//When exit occured, need to check if the state is STARTING and if time_spent < starttime
			// if it's the case : status = BACKOFF

		}
	}
}
