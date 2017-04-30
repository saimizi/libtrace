#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include <execinfo.h>

#include "libtrace.h"

static int trace_fd = -1;

#define DEBUGFS_MNT_POINT	"/tmp/debug"
#define TRACE_MAKER		DEBUGFS_MNT_POINT"/tracing/trace_marker"

void __attribute__ ((constructor)) libtrace_init(void)
{
	if (access(DEBUGFS_MNT_POINT, F_OK) != 0) {
		if (mkdir(DEBUGFS_MNT_POINT, 0777) == -1)
			return;
	}

	/* if already mounted, just error */
	mount("nodev", DEBUGFS_MNT_POINT, "debugfs", 0, NULL);

	trace_fd = open(TRACE_MAKER, O_WRONLY);
}

void __attribute__ ((destructor)) libtrace_fini(void)
{
	if (trace_fd >= 0)
		close(trace_fd);
}

void output(char *buf, int size)
{
	if (buf == NULL || size <= 0)
		return;

	if (trace_fd > 0)
		write(trace_fd, buf, strlen(buf) + 1);
}

void trace_msg(const char *fmt, ...)
{
	va_list ap;
	char buf[MAX_TRACE_MSG_SIZE];
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf, MAX_TRACE_MSG_SIZE, fmt, ap);
	va_end(ap);

	output(buf, n);
}

void trace_backtrace_sym(const char *fn)
{
	void *buf[1024];
	int symnum = 0;
	char **symbols;
	int i;

	symnum = backtrace(buf, 1024);
	if (symnum < 1)
		return;

	symbols = backtrace_symbols(buf, symnum);
	if (symbols) {
		fprintf(stderr, "===============================\n");

		fprintf(stderr, "%s() Backtrace: %d founds\n",
				fn ? fn : "Func", symnum);

		for (i = 0; i < symnum ; i++)
			fprintf(stderr, "\t%s\n", symbols[i]);

		fprintf(stderr, "===============================\n");
	} else {
		fprintf(stderr, "No symbols found.\n");
	}
}
