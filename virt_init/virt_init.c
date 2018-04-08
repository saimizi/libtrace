#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <limits.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define STACK_SIZE (8 * 1024 * 1024)
struct system_info {
	unsigned long sysid;
	char *rootdev;
	char *root;
	char *rootfstype;
	char *init_prog;
	pid_t real_init_pid;
	char *init_prog_stack;
};

int is_system_info_ok(struct system_info *si)
{
	int ret = 0;
	struct stat stat_buf;

	do {
		int tmpret;

		if (!si)
			break;

		tmpret = stat(si->rootdev, &stat_buf);
		if (ret || !S_ISDIR(stat_buf.st_mode))
			break;

		if (!si->root)
			break;

		if (!si->init_prog || !si->init_prog_stack)
			break;

		if (!si->rootfstype || strcmp(si->rootfstype, "ext4"))
			break;

		ret = 1;
	} while (0);

	return ret;

}

int system_init(void *arg)
{
	int ret = 0;
	struct system_info *si = (struct system_info *)arg;

	do {
		char *const arginit_prog[] = { si->init_prog, NULL };
		char *const argenv[] = { NULL };

		ret = chroot(si->root);
		if (ret < 0) {
			ret = -1;
			fprintf(stderr, "Failed to chroot(): %s(%d).",
				strerror(errno),
				errno);
			break;
		}

		ret = execve(arginit_prog[0], arginit_prog, argenv);
	} while (0);

	exit(ret);
}

int create_system(struct system_info *si)
{
	int ret = -1;
	char *mount_point = NULL;
	char *real_path = NULL;
	struct stat stat_buf;
	char *child_stack = NULL;

	do {
		ssize_t path_len = 0;

		if (!is_system_info_ok(si)) {
			fprintf(stderr, "Invalid system info.");
			break;
		}

		real_path = (char *)malloc(sizeof(char) * PATH_MAX);
		if (!real_path) {
			ret = -1;
			fprintf(stderr, "Failed to malloc().");
			break;
		}

		path_len = readlink(si->rootdev, real_path, PATH_MAX);
		if (path_len < 0) {
			ret = -1;
			fprintf(stderr, "Failed to readlink(): %s(%d).",
				strerror(errno),
				errno);
			break;
		}
		real_path[path_len] = '\0';

		mount_point = si->root;
		sprintf(mount_point, "/virtsys/sys-%lu", si->sysid);
		if (!access(mount_point, F_OK)) {
			ret = mkdir(mount_point, 0777);
			if (ret) {
				ret = -1;
				fprintf(stderr, "Failed to mkdir(): %s(%d).",
					strerror(errno),
					errno);
				break;
			}
		}

		ret = stat(si->rootdev, &stat_buf);
		if (S_ISBLK(stat_buf.st_mode)) {
			ret = mount(real_path,
				mount_point,
				si->rootfstype,
				0,
				NULL);
		} else if (S_ISREG(stat_buf.st_mode)) {
			/* Image file, mount by using loop device */
			ret = mount(real_path,
				mount_point,
				si->rootfstype,
				0,
				"-o loop");
		} else {
			ret = -1;
		}

		if (ret < 0) {
			ret = -1;
			fprintf(stderr, "Failed to mount():%s(%d).",
					strerror(errno),
					errno);
			break;
		}

		si->real_init_pid = clone(system_init,
				si->init_prog_stack + STACK_SIZE,
				CLONE_NEWPID | SIGCHLD,
				(void *) si);
		if (si->real_init_pid < 0) {
			ret = -1;
			fprintf(stderr, "Failed to mount():%s(%d).",
					strerror(errno),
					errno);
			umount(mount_point);
			break;
		}

		ret = 0;

	} while (0);

	if (real_path)
		free(real_path);

	return ret;
}

unsigned char systemid;
int main(int argc, char *argv[])
{
	int ret = 0;
	struct system_info si;

	do {

		si.sysid = systemid++;
		si.rootdev = argv[1];
		si.root = (char *)malloc(sizeof(char) * PATH_MAX);
		si.rootfstype = "ext4";
		si.init_prog = "/sbin/init";
		si.init_prog_stack = (char *) malloc(sizeof(char) * STACK_SIZE);

		if (!is_system_info_ok(&si)) {
			fprintf(stderr, "Failed to create systemid info.");
			break;
		}

		ret = create_system(&si);
		if (ret < 0)
			break;

		while (1) {
			int status;

			waitpid(-1, &status, 0);

		}
	} while (0);

	exit(EXIT_SUCCESS);
}

