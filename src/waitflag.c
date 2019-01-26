#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include <sys/inotify.h>
#include <libgen.h>

#define FFLAG_DIR		"/tmp/fflag"
#define FFLAG_DIR_PERM		(S_IRWXU | S_IRGRP | S_IROTH)
#define FFLAG_DIR_PERM_MASK	(~S_IFMT & ~S_ISUID & ~S_ISGID & S_ISVTX)

#define NAME_BUFSZ			128
#define MAX_FLAG_SIZE		(NAME_BUFSZ - strlen(FFLAG_DIR) - 2)

#define EVENT_BUF_LEN 		(1 * (sizeof(struct inotify_event) + NAME_MAX + 1))
char *event_buf;

static inline int is_fflag_dir_perm_ok(mode_t mode)
{
	return (mode & FFLAG_DIR_PERM) == FFLAG_DIR_PERM;
}

static int is_fflag_dir_ok(const char *dir)
{
	int ret = -1;
	
	do {
		struct stat	st;

		if (stat(FFLAG_DIR, &st) < 0){
			ret = 0;
			break;
		}

		if (!S_ISDIR(st.st_mode))
			break;

		if (!is_fflag_dir_perm_ok(st.st_mode))
			break;

		ret = 1;

	} while (0);

	return ret;
}

int wait_flag_on(const char *flag)
{
	int ret = -1;
	char buf[NAME_BUFSZ];

	do {
		struct stat	st;

		if (!flag)
			break;

		if (strlen(flag) > MAX_FLAG_SIZE)
			break;

		sprintf(buf, "%s/%s", FFLAG_DIR, flag);

		if (!stat(buf, &st)) {
			ret = 0;
			break;
		}

		int inotifyFd;

		inotifyFd = inotify_init();
		if (inotifyFd < 0)
			break;

		if (inotify_add_watch(inotifyFd, FFLAG_DIR, IN_CREATE) < 0)
			break;

		while (1) {
			ssize_t numRead;
			char *p;

			numRead = read(inotifyFd, event_buf, EVENT_BUF_LEN);
			if (numRead < 0)
				break;

			if (numRead == 0)
				continue;

			p = event_buf;
			while (p < event_buf + numRead) {
				struct inotify_event *event;

				event = (struct inotify_event *)p;
				if (event->name && !strcmp(flag, event->name)) {
					ret = 0;
					break;
				}

				p += sizeof(struct inotify_event) + event->len;
			}

			if (ret != -1)
				break;

		}
	} while (0);

	return ret;
}

int wait_flag_off(const char *flag)
{
	int ret = -1;
	char buf[NAME_BUFSZ];

	do {
		struct stat	st;

		if (!flag)
			break;

		if (strlen(flag) > MAX_FLAG_SIZE)
			break;

		sprintf(buf, "%s/%s", FFLAG_DIR, flag);

		if (stat(buf, &st)) {
			ret = 0;
			break;
		}

		int inotifyFd;

		inotifyFd = inotify_init();
		if (inotifyFd < 0)
			break;

		if (inotify_add_watch(inotifyFd, FFLAG_DIR, IN_DELETE | IN_DELETE_SELF) < 0)
			break;

		while (1) {
			ssize_t numRead;
			char *p;

			numRead = read(inotifyFd, event_buf, EVENT_BUF_LEN);
			if (numRead < 0)
				break;

			if (numRead == 0)
				continue;

			p = event_buf;
			while (p < event_buf + numRead) {
				struct inotify_event *event;

				event = (struct inotify_event *)p;
				if (event->mask & IN_DELETE) {
					if (event->name && !strcmp(flag, event->name)) {
						ret = 0;
						break;
					}
				} else {
					ret = -2;
					break;
				}

				p += sizeof(struct inotify_event) + event->len;
			}

			if (ret != -1)
				break;
		}
	} while (0);

	return ret;
}

int main(int argc, char *argv[])
{
	int ret  = 1;

	do {
		int tmpret = 0;

		if (argc < 2)
			break;

		tmpret = is_fflag_dir_ok(FFLAG_DIR);
		if (tmpret < 0)
			break;
		else if (!tmpret && mkdir(FFLAG_DIR, FFLAG_DIR_PERM)) 
			break;

		event_buf = malloc(EVENT_BUF_LEN);
		if (!event_buf)
			break;

		if (!strcmp(basename(argv[0]), "waitflagon")){
			ret = wait_flag_on(argv[1]);
			break;
		}
		
		if (!strcmp(basename(argv[0]), "waitflagoff")){
			ret = wait_flag_off(argv[1]);

			break;
		}


	} while (0);

#if 0
	if (!ret)
		printf("%s yes \n", basename(argv[0]));
	else
		printf("%s no\n", basename(argv[0]));
#endif

	/* if success return 0 otherwise return 1 */
	exit(ret!=0);
}
