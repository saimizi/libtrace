#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <string.h>
#include <stdarg.h>


#include "libtrace.h"



static int trace_fd = -1;

#define DEBUGFS_MNT_POINT	"/tmp/debug"
#define TRACE_MAKER		DEBUGFS_MNT_POINT"/tracing/trace_marker"


void __attribute__ ((constructor)) libtrace_init(void){
	if (access(DEBUGFS_MNT_POINT,F_OK) != 0) {
		if (mkdir(DEBUGFS_MNT_POINT,0777) == -1)
			return;
	}

	/* if already mounted, just error */
	mount("nodev", DEBUGFS_MNT_POINT, "debugfs",0,NULL);

	trace_fd = open(TRACE_MAKER,O_WRONLY);
}

void __attribute__ ((destructor)) libtrace_fini(void){
	if (trace_fd >= 0)
		close(trace_fd);
}

void trace_msg(const char *fmt,...){
	va_list ap;
	char buf[MAX_TRACE_MSG_SIZE];
	int n;

	if (trace_fd < 0)
		return;
	
	va_start(ap,fmt);
	n = vsnprintf(buf,MAX_TRACE_MSG_SIZE,fmt,ap);
	va_end(ap);

	write(trace_fd, buf, n);
}
