#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "libfflag.h"

int main(int argc, char *argv[])
{
	int ret  = 1;

	do {
		if (!strcmp(basename(argv[0]), "waitflagon")){
			ret = wait_flag_on(argv[1]);
			break;
		}
		
		if (!strcmp(basename(argv[0]), "waitflagoff")){
			ret = wait_flag_off(argv[1]);

			break;
		}


	} while (0);

	/* if success return 0 otherwise return 1 */
	exit(ret!=0);
}
