#ifndef __LIBTRACE_H__
#define __LIBTRACE_H__

#define MAX_TRACE_MSG_SIZE	1024

void trace_msg(const char *fmt, ...);
void trace_backtrace(void);
void trace_backtrace_sym(const char *fn);
#define trace_backtrace()	trace_backtrace_sym(NULL)

#endif /*__LIBTRACE_H__*/
