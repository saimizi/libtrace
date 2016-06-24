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
#include <dlfcn.h>

#include <execinfo.h>

#include "libtrace.h"

static int trace_fd = -1;
static int log_fd = -1;

#define DEBUGFS_MNT_POINT	"/tmp/debug"
#define TRACE_MAKER		DEBUGFS_MNT_POINT"/tracing/trace_marker"
#define LOGFILE			"/tmp/libtrace.log"

#define  mode_v			"TRC_M"
#define  output_v		"TRC_O"


void __attribute__ ((constructor)) libtrace_init(void){
	if (access(DEBUGFS_MNT_POINT,F_OK) != 0) {
		if (mkdir(DEBUGFS_MNT_POINT,0777) == -1)
			return;
	}

	/* if already mounted, just error */
	mount("nodev", DEBUGFS_MNT_POINT, "debugfs",0,NULL);

	trace_fd = open(TRACE_MAKER,O_WRONLY);

	unlink(LOGFILE);
	log_fd = open(LOGFILE,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
}

void __attribute__ ((destructor)) libtrace_fini(void){
	if (trace_fd >= 0)
		close(trace_fd);

	if (log_fd >= 0)
		close(log_fd);
}

void output(char * buf, int size){
	char * out_mode = getenv(output_v);
	int outfd = -1;
	
	if (buf == NULL || size <= 0)
		return;

	if (out_mode && (strcmp(out_mode,"FTRACE") == 0)) {
		if (trace_fd > 0)
			outfd = trace_fd;
	} else if (out_mode && (strcmp(out_mode,"LOG") == 0)) {
		if (log_fd > 0)
			outfd = log_fd;
	} else {
		outfd = fileno(stdout);
	}
	
	write(outfd, buf, size);

}

void __trace_msg(const char *fmt,...){
	va_list ap;
	char buf[MAX_TRACE_MSG_SIZE];
	int n;

	va_start(ap,fmt);
	n = vsnprintf(buf,MAX_TRACE_MSG_SIZE,fmt,ap);
	va_end(ap);

	output(buf,n);
}

const char* addr2name(void* address) {
    Dl_info dli;
    if (0 != dladdr(address, &dli)) {
        return dli.dli_sname;
    }   
    return 0;
}

void __cyg_profile_func_enter(void* func_address, void* call_site) {
	const char* func_name = addr2name(func_address); 
	char * trace_mode = getenv(mode_v);

	if (trace_mode && ((strcmp(trace_mode,"FE") == 0) || (strcmp(trace_mode,"FT") == 0) || (strcmp(trace_mode,"ALL") == 0))){
		if (func_name) {
			__trace_msg("%s enter.\n",func_name);
		}

	}   
}

void __cyg_profile_func_exit(void* func_address, void* call_site) {
	const char* func_name = addr2name(func_address); 
	char * trace_mode = getenv(mode_v);

	if (trace_mode && ((strcmp(trace_mode,"FL") == 0) || (strcmp(trace_mode,"FT") == 0) || (strcmp(trace_mode,"ALL") == 0))){
		if (func_name) {
			__trace_msg("%s leave.\n",func_name);
		}   
    	}
}

void trace_msg(const char *fmt,...){
	char * trace_mode = getenv(mode_v);

	if (trace_mode && ((strcmp(trace_mode,"FC") == 0) || (strcmp(trace_mode,"ALL") == 0))){
		va_list ap;
		char buf[MAX_TRACE_MSG_SIZE];
		int n;

		va_start(ap,fmt);
		n = vsnprintf(buf,MAX_TRACE_MSG_SIZE,fmt,ap);
		va_end(ap);

		output(buf,n);
	}
}

void trace_backtrace(void){
	void * buf[1024];
	int symnum = 0;
	char ** symbols;
	int i;
	char * trace_mode = getenv(mode_v);

	if (trace_mode && ((strcmp(trace_mode,"BT") == 0) || (strcmp(trace_mode,"ALL") == 0))){
		symnum = backtrace(buf,1024);
		if (symnum < 1)
			return;
		
		symbols = backtrace_symbols(buf,symnum);
		if (symbols) {
			__trace_msg("===============================\n");
			__trace_msg("Backtrace: %d founds\n",symnum);
			for (i = 0; i < symnum ; i++){
				__trace_msg("\t%s\n",symbols[i]);
			}
			__trace_msg("===============================\n");
		} else {
			__trace_msg("No symbols found.\n");
		}
	}
}

