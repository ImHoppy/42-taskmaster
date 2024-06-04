#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>
#include <syslog.h>

typedef struct
{
	va_list ap;
	const char *fmt;
	const char *file;
	struct tm *time;
	void *udata;
	int line;
	int level;
} log_Event;

typedef void (*log_LogFn)(log_Event *ev);
typedef void (*log_LockFn)(bool lock, void *udata);

enum
{
	LOGGING_TRACE,
	LOGGING_DEBUG,
	LOGGING_INFO,
	LOGGING_WARN,
	LOGGING_ERROR,
	LOGGING_FATAL
};

#define log_trace(...) log_log(LOGGING_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_log(LOGGING_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...) log_log(LOGGING_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) log_log(LOGGING_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_log(LOGGING_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_log(LOGGING_FATAL, __FILE__, __LINE__, __VA_ARGS__)

const char *log_level_string(int level);
void log_set_level(int level);
void log_set_quiet(bool enable);
int log_add_callback(log_LogFn fn, void *udata, int level);
int log_add_fp(FILE *fp, int level);
int log_add_syslog(int level);

void log_log(int level, const char *file, int line, const char *fmt, ...);

#endif
