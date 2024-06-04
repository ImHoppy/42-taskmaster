

#include "logging.h"
#include <syslog.h>

#define MAX_CALLBACKS 32

typedef struct
{
	log_LogFn fn;
	void *udata;
	int level;
} log_callback_t;

static struct
{
	int level;
	bool quiet;
	log_callback_t callbacks[MAX_CALLBACKS];
} g_logger;

static const char *level_strings[] = {
	"TRACE",
	"DEBUG",
	"INFO",
	"WARN",
	"ERROR",
	"FATAL",
};

static const char *level_colors[] = {
	"\x1b[94m",
	"\x1b[36m",
	"\x1b[32m",
	"\x1b[33m",
	"\x1b[31m",
	"\x1b[35m",
};

static void stdout_callback(log_Event *ev)
{
	char buf[16];
	buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
	fprintf(
		ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
		buf, level_colors[ev->level], level_strings[ev->level],
		ev->file, ev->line);
	vfprintf(ev->udata, ev->fmt, ev->ap);
	fprintf(ev->udata, "\n");
	fflush(ev->udata);
}

static void file_callback(log_Event *ev)
{
	char buf[64];
	buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';
	fprintf(
		ev->udata, "%s %-5s %s:%d: ",
		buf, level_strings[ev->level], ev->file, ev->line);
	vfprintf(ev->udata, ev->fmt, ev->ap);
	fprintf(ev->udata, "\n");
	fflush(ev->udata);
}

static void syslog_callback(log_Event *ev)
{
	static const int to_syslog[] = {
		LOG_NOTICE,
		LOG_DEBUG,
		LOG_INFO,
		LOG_WARNING,
		LOG_ERR,
		LOG_CRIT,
	};
	printf("%d %s\n", to_syslog[ev->level], ev->fmt);
	vsyslog(to_syslog[ev->level], ev->fmt, ev->ap);
}

const char *log_level_string(int level)
{
	return level_strings[level];
}

void log_set_level(int level)
{
	g_logger.level = level;
}

void log_set_quiet(bool enable)
{
	g_logger.quiet = enable;
}

int log_add_callback(log_LogFn fn, void *udata, int level)
{
	for (int i = 0; i < MAX_CALLBACKS; i++)
	{
		if (!g_logger.callbacks[i].fn)
		{
			g_logger.callbacks[i] = (log_callback_t){fn, udata, level};
			return 0;
		}
	}
	return -1;
}

int log_add_fp(FILE *fp, int level)
{
	return log_add_callback(file_callback, fp, level);
}

int log_add_syslog(int level)
{
	return log_add_callback(syslog_callback, NULL, level);
}

static void init_event(log_Event *ev, void *udata)
{
	if (!ev->time)
	{
		time_t t = time(NULL);
		ev->time = localtime(&t);
	}
	ev->udata = udata;
}

void log_log(int level, const char *file, int line, const char *fmt, ...)
{
	log_Event ev = {
		.fmt = fmt,
		.file = file,
		.line = line,
		.level = level,
	};

	if (!g_logger.quiet && level >= g_logger.level)
	{
		init_event(&ev, stderr);
		va_start(ev.ap, fmt);
		stdout_callback(&ev);
		va_end(ev.ap);
	}

	for (int i = 0; i < MAX_CALLBACKS && g_logger.callbacks[i].fn; i++)
	{
		log_callback_t *cb = &g_logger.callbacks[i];
		if (level >= cb->level)
		{
			init_event(&ev, cb->udata);
			va_start(ev.ap, fmt);
			cb->fn(&ev);
			va_end(ev.ap);
		}
	}
}
