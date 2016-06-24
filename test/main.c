#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "../libtrace.h"


void func121(){
	trace_msg("%s - %d run.\n",__func__,__LINE__);
	trace_backtrace_sym(__func__);
}

void func12(){
	trace_msg("%s - %d run.\n",__func__,__LINE__);
	func121();
}


void func2(){
	trace_msg("%s - %d run.\n",__func__,__LINE__);
	func121();
};

void func1(){
	trace_msg("%s - %d run.\n",__func__,__LINE__);
	func12();
};

int main(int argc,char * argv[]){

	trace_msg("%s - %d run.\n",__func__,__LINE__);
	func1();
	func2();

	return 0;
}
