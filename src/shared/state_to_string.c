#include "shared.h"

static const char *states[] = {
	"STOPPED",
	"STARTING",
	"RUNNING",
	"BACKOFF",
	"STOPPING",
	"EXITED",
	"FATAL",
	"UNKNOWN",
};
static const int states_count = 8;

const char *state_to_string(process_state_t state)
{
	if (state < 0 || state >= states_count)
		return states[states_count - 1];
	return states[state];
}
