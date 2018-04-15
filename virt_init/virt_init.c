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
#include <sys/types.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/loop.h>

#define STACK_SIZE (8 * 1024 * 1024)
struct system_info {
	unsigned long sysid;
	char *rootdev;
	char *root;
	char *rootfstype;
	char *init_prog;
	pid_t real_init_pid;
	int  loop_dev_num;
	char *hostname;
	char *init_prog_stack;
};


#define LOOP_CTL_FILE	"/dev/loop-control"
#define VIRT_ROOT_DIR	"/virtsys"

static int setup_loopdevice(char *image_file)
{
	int ret = -1;
	int loop_ctldev_fd = -1;
	int loop_dev_fd = -1;
	int image_fd = -1;
	int loop_free_num = -1;
	char loop_device[16];


	do {
		if (!image_file)
			break;

		/* Find free loop device */
		loop_ctldev_fd = open(LOOP_CTL_FILE, O_RDWR);
		if (loop_ctldev_fd < 0)
			break;

		ret = ioctl(loop_ctldev_fd, LOOP_CTL_GET_FREE);
		if (ret < 0) {
			fprintf(stderr,
				"LOOP_CTL_GET_FREE failed: %s(%d)",
				strerror(errno),
				errno);
			break;
		}

		sprintf(loop_device, "/dev/loop%d", ret);
		loop_free_num = ret;

		fprintf(stderr, "Setup %s for %s\n", loop_device, image_file);

		image_fd = open(image_file, O_RDWR);
		if (image_file < 0)
			break;

		loop_dev_fd = open(loop_device, O_RDWR);
		if (loop_dev_fd < 0)
			break;

		ret = ioctl(loop_dev_fd, LOOP_SET_FD, image_fd);
		if (ret < 0)
			break;

		ret = loop_free_num;

	} while (0);

	if (loop_ctldev_fd > 0)
		close(loop_ctldev_fd);

	if (loop_dev_fd > 0)
		close(loop_dev_fd);

	if (image_fd > 0)
		close(image_fd);

	return ret;
}

int is_system_info_ok(struct system_info *si)
{
	int ret = 0;
	struct stat stat_buf;

	do {
		int tmpret;

		if (!si)
			break;

		if (!si->rootdev || (access(si->rootdev, F_OK))) {
			fprintf(stderr, " %s fail: %s(%d)\n",
				si->rootdev,
				strerror(errno), errno);
			break;
		}

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
	char *mount_point = NULL;
	char *real_path = NULL;
	struct stat stat_buf;
	struct system_info *si = (struct system_info *)arg;

	do {
		char *const arginit_prog[] = { si->init_prog, NULL };
		char *const argenv[] = { NULL };
		ssize_t path_len = 0;

		if (access(VIRT_ROOT_DIR, F_OK)) {
			ret = mkdir(VIRT_ROOT_DIR, 0777);
			if (ret) {
				ret = -1;
				fprintf(stderr,
					"Failed to mkdir(): %s(%d).\n",
					strerror(errno),
					errno);
				break;
			}
		}

		if (!is_system_info_ok(si)) {
			fprintf(stderr, "Invalid system info.\n");
			break;
		}

		real_path = (char *)malloc(sizeof(char) * PATH_MAX);
		if (!real_path) {
			ret = -1;
			fprintf(stderr, "Failed to malloc().\n");
			break;
		}

		mount_point = si->root;
		sprintf(mount_point, VIRT_ROOT_DIR"/sys-%lu", si->sysid);
		if (access(mount_point, F_OK)) {
			ret = mkdir(mount_point, 0777);
			if (ret) {
				ret = -1;
				fprintf(stderr,
					"Failed to mkdir(): %s(%d).\n",
					strerror(errno),
					errno);
				break;
			}
		}

		ret = stat(si->rootdev, &stat_buf);
		if (S_ISLNK(stat_buf.st_mode)) {
			path_len = readlink(si->rootdev, real_path, PATH_MAX);
			if (path_len < 0) {
				ret = -1;
				fprintf(stderr,
					"Failed to readlink(): %s(%d).\n",
					strerror(errno),
					errno);
				break;
			}
			real_path[path_len] = '\0';
		} else {
			strcpy(real_path, si->rootdev);
		}

		printf("real_path: %s\n", real_path);
		printf("mount_point: %s\n", mount_point);
		printf("root: %s\n", si->root);

		ret = stat(real_path, &stat_buf);
		if (S_ISREG(stat_buf.st_mode)) {
			char loop_device[16];

			si->loop_dev_num = setup_loopdevice(real_path);
			sprintf(loop_device, "/dev/loop%d", si->loop_dev_num);
			ret = mount(loop_device,
				mount_point,
				si->rootfstype,
				0,
				NULL);
		} else if (S_ISBLK(stat_buf.st_mode)) {
			ret = mount(real_path,
				mount_point,
				si->rootfstype,
				0,
				NULL);
		} else {
			ret = -1;
		}

		if (ret < 0) {
			ret = -1;
			fprintf(stderr, "Failed to mount():%s(%d).\n",
					strerror(errno),
					errno);
			break;
		}


		ret = chroot(si->root);
		if (ret < 0) {
			ret = -1;
			fprintf(stderr, "Failed to chroot(): %s(%d).\n",
				strerror(errno),
				errno);
			umount(mount_point);
			break;
		}

		fprintf(stderr, "Booting system (sysid = %d rootfs=%s)...\n",
			si->sysid, si->root);

		if (si->hostname) {
			char *hostname = real_path;
			int len = 0;

			len = sprintf(hostname, "%s%d",
				si->hostname,
				si->sysid);

			hostname[len] = '\0';
			ret = sethostname(hostname, strlen(hostname));
			if (ret < 0) {
				fprintf(stderr,
					"Failed to sethostname (): %s(%d).\n",
					strerror(errno),
					errno);
			} else {
				fprintf(stderr,
					"Host name is set to %s\n",
					hostname);
			}
		}

		if (real_path)
			free(real_path);


		ret = execve(arginit_prog[0], arginit_prog, argenv);
	} while (0);

	if (real_path)
		free(real_path);

	exit(ret);
}

int create_system(struct system_info *si)
{
	int ret = -1;
	char *child_stack = NULL;

	do {
		si->real_init_pid = clone(system_init,
				si->init_prog_stack + STACK_SIZE,
				CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS |
				SIGCHLD, (void *) si);
		if (si->real_init_pid < 0) {
			ret = -1;
			fprintf(stderr, "Failed to mount():%s(%d).\n",
					strerror(errno),
					errno);
			break;
		}

		ret = 0;

	} while (0);

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
		si.loop_dev_num = 0;
		si.hostname = "VirtM";

		if (!is_system_info_ok(&si)) {
			fprintf(stderr, "Failed to create systemid info.\n");
			break;
		}

		ret = create_system(&si);
		if (ret < 0)
			break;

		while (1) {
			int status;

			fprintf(stderr, "Waitting %d\n", si.real_init_pid);

			waitpid(si.real_init_pid, &status, 0);

			if (WIFEXITED(status))
				fprintf(stderr,
					"System quit with status = %d\n",
					WEXITSTATUS(status));

			if (WIFSIGNALED(status)) {
				int result = WTERMSIG(status);

				fprintf(stderr,
					"System quited by signal = %d\n",
					result);

				if (result == 1) {
					fprintf(stderr, "Reboot System ...\n");
					ret = create_system(&si);
					continue;
				}
			}

			break;


		}
	} while (0);

	exit(EXIT_SUCCESS);
}

