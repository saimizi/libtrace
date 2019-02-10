#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "libfflag.h"

int main(int argc, char *argv[])
{
	int ret  = 1;

	do {
		if (!strcmp(basename(argv[0]), "waitfflagon")) {
			ret = wait_flag_on(argv[1]);
			break;
		}

		if (!strcmp(basename(argv[0]), "waitfflagoff")) {
			ret = wait_flag_off(argv[1]);
			break;
		}

		if (!strcmp(basename(argv[0]), "setfflag")) {
			ret = set_flag(argv[1]);
			break;
		}

		if (!strcmp(basename(argv[0]), "clearfflag")) {
			ret = clear_flag(argv[1]);
			break;
		}

		if (!strcmp(basename(argv[0]), "isfflagon")) {
			ret = !is_flag_on(argv[1]);
			break;
		}

	} while (0);

	/* if success return 0 otherwise return 1 */
	exit(ret != 0);
}
