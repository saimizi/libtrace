#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include <sys/inotify.h>

#define libfflag_err(fmt, ...) \
	fprintf(stderr, "[libfflag] %s() Error: "fmt, __func__, ## __VA_ARGS__)

#ifdef DEBUG
#define libfflag_dbg(fmt, ...) \
	fprintf(stderr, "[libfflag] %s() - %d Debug: "\
		fmt, __func__, __LINE__,  ## __VA_ARGS__)
#else
#define libfflag_dbg(fmt, ...)
#endif


#define FFLAG_DIR		"/tmp/fflag"
#define FFLAG_DIR_PERM		0744

#define NAME_BUFSZ			128
#define MAX_FLAG_SIZE		(NAME_BUFSZ - strlen(FFLAG_DIR) - 2)

#define EVENT_BUF_LEN	(1 * (sizeof(struct inotify_event) + MAX_FLAG_SIZE + 1))

void print_event(struct inotify_event *i)
{
#ifdef DEBUG
	printf(" wd =%2d; ", i->wd);
	if (i->cookie > 0)
		printf("cookie =%4d; ", i->cookie);
	printf("mask = ");
	if (i->mask & IN_ACCESS)
		printf("IN_ACCESS ");
	if (i->mask & IN_ATTRIB)
		printf("IN_ATTRIB ");
	if (i->mask & IN_CLOSE_NOWRITE)
		printf("IN_CLOSE_NOWRITE ");
	if (i->mask & IN_CLOSE_WRITE)
		printf("IN_CLOSE_WRITE ");
	if (i->mask & IN_CREATE)
		printf("IN_CREATE ");
	if (i->mask & IN_DELETE)
		printf("IN_DELETE ");
	if (i->mask & IN_DELETE_SELF)
		printf("IN_DELETE_SELF ");
	if (i->mask & IN_IGNORED)
		printf("IN_IGNORED ");
	if (i->mask & IN_ISDIR)
		printf("IN_ISDIR ");
	if (i->mask & IN_MODIFY)
		printf("IN_MODIFY ");
	if (i->mask & IN_MOVE_SELF)
		printf("IN_MOVE_SELF ");
	if (i->mask & IN_MOVED_FROM)
		printf("IN_MOVED_FROM ");
	if (i->mask & IN_MOVED_TO)
		printf("IN_MOVED_TO ");
	if (i->mask & IN_OPEN)
		printf("IN_OPEN ");
	if (i->mask & IN_Q_OVERFLOW)
		printf("IN_Q_OVERFLOW ");
	if (i->mask & IN_UNMOUNT)
		printf("IN_UNMOUNT ");

	printf("\n");

	if (i->len > 0)
		printf(" name = %s\n", i->name);
#endif
}

static inline int is_fflag_dir_perm_ok(mode_t mode)
{
	return (mode & FFLAG_DIR_PERM) == FFLAG_DIR_PERM;
}

static int is_fflag_dir_ok(const char *dir)
{
	int ret = -1;

	do {
		struct stat	st;

		if (stat(FFLAG_DIR, &st) < 0) {
			ret = 0;
			break;
		}

		if (!S_ISDIR(st.st_mode)) {
			libfflag_err("FFlag directory %s is invalid.\n", dir);
			break;
		}

#if 0
		if (!is_fflag_dir_perm_ok(st.st_mode))
			break;
#endif

		ret = 1;

	} while (0);

	return ret;
}

int is_flag_ok(const char *flag)
{
	int ret = 0;

	do {
		if (!flag) {
			libfflag_err("Flag is NULL.\n");
			break;
		}

		if (strlen(flag) > MAX_FLAG_SIZE) {
			libfflag_err("Flag %s is too long (%ld > MAX=%ld).\n",
				flag, (long)strlen(flag), MAX_FLAG_SIZE);
			break;
		}

		ret = 1;

	} while (0);

	return ret;
}

int initcheck(void)
{
	int ret = -1;

	do {
		int tmpret = 0;

		tmpret = is_fflag_dir_ok(FFLAG_DIR);
		/* <0 : some error happned */
		if (tmpret < 0)
			break;

		/* >0 : directory exists, OK */
		if (tmpret > 0) {
			ret = 0;
			break;
		}

		/* =0 : no directory, create it */
		tmpret = mkdir(FFLAG_DIR, FFLAG_DIR_PERM);

		/* =0 : successed to create it */
		if (!tmpret) {
			ret = 0;
			break;
		}

		/*
		 * <0 EEXIST:
		 *	failed to create it, but someone else create it
		 */
		if ((tmpret < 2) && (errno == EEXIST)) {
			ret = 0;
			break;
		}

		/* some error happened */


	} while (0);

	return ret;
}

int is_flag_on(const char *flag)
{
	int ret = 0;
	char buf[NAME_BUFSZ];
	struct stat	st;

	do {
		if (!is_flag_ok(flag))
			break;

		if (initcheck() < 0)
			break;

		sprintf(buf, "%s/%s", FFLAG_DIR, flag);
		if (!stat(buf, &st)) {
			ret = 1;
			break;
		}
	} while (0);

	return ret;
}

int wait_flag_on(const char *flag)
{
	int ret = -1;
	char buf[NAME_BUFSZ];
	char *event_buf;

	do {
		int inotifyFd;
		struct stat	st;

		if (!is_flag_ok(flag))
			break;

		if (initcheck() < 0)
			break;


		event_buf = malloc(EVENT_BUF_LEN);
		if (!event_buf) {
			libfflag_err("No memeory.\n");
			break;
		}

		inotifyFd = inotify_init();
		if (inotifyFd < 0) {
			libfflag_err("Failed to init inotify(%d): %s.\n",
					errno, strerror(errno));
			break;
		}

		if (inotify_add_watch(inotifyFd,
				FFLAG_DIR, IN_ALL_EVENTS) < 0) {
			libfflag_err("Failed to add inotify watch (%d): %s.\n",
				errno, strerror(errno));
			break;
		}

		/* Flag has already been set */
		sprintf(buf, "%s/%s", FFLAG_DIR, flag);
		if (!stat(buf, &st)) {
			ret = 0;
			break;
		}

		while (1) {
			ssize_t numRead;
			char *p;

			numRead = read(inotifyFd, event_buf, EVENT_BUF_LEN);
			if (numRead < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;

				libfflag_err("Failed to read inotify events (%d): %s.\n",
					errno, strerror(errno));
				ret = -2;
				break;
			}

			if (numRead == 0)
				continue;

			p = event_buf;
			while (p < event_buf + numRead) {
				struct inotify_event *event;

#define ERR_EVENTS		(IN_IGNORED | IN_MOVE_SELF)
#define WAIT_FLAG_ON_EVENTS	(IN_CREATE | IN_MOVED_TO | ERR_EVENTS)

				event = (struct inotify_event *)p;
				print_event(event);

				if ((event->mask & WAIT_FLAG_ON_EVENTS)) {
					if (event->mask & ERR_EVENTS) {
						libfflag_err("Wait failed. %s is removed?\n",
							FFLAG_DIR);
						ret = -2;
						break;
					}

					if ((event->len > 0) &&
						!strcmp(flag, event->name)) {
						ret = 0;
						break;
					}
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
	char *event_buf;

	do {
		struct stat	st;

		if (!is_flag_ok(flag))
			break;

		if (initcheck() < 0)
			break;


		event_buf = malloc(EVENT_BUF_LEN);
		if (!event_buf) {
			libfflag_err("No memeory.\n");
			break;
		}

		int inotifyFd;

		inotifyFd = inotify_init();
		if (inotifyFd < 0) {
			libfflag_err("Failed to init inotify(%d):%s.\n",
					errno, strerror(errno));
			break;
		}

		if (inotify_add_watch(inotifyFd, FFLAG_DIR,
				IN_ALL_EVENTS) < 0) {
			libfflag_err("Failed to add inotify watch (%d):%s.\n",
					errno, strerror(errno));
			break;
		}

		/* Flag is not set yet */
		sprintf(buf, "%s/%s", FFLAG_DIR, flag);
		if (stat(buf, &st)) {
			ret = 0;
			break;
		}

		while (1) {
			ssize_t numRead;
			char *p;

			numRead = read(inotifyFd, event_buf, EVENT_BUF_LEN);
			if (numRead < 0) {
				if (errno == EAGAIN || errno == EINTR)
					continue;

				libfflag_err("Failed to read inotify events(%d): %s.\n",
					errno, strerror(errno));
				ret = -2;
				break;
			}

			if (numRead == 0)
				continue;

			p = event_buf;
			while (p < event_buf + numRead) {
				struct inotify_event *event;

#define WAIT_FLAG_OFF_EVENTS	(IN_DELETE | IN_MOVED_FROM | ERR_EVENTS)

				event = (struct inotify_event *)p;
				print_event(event);

				if ((event->mask & WAIT_FLAG_OFF_EVENTS)) {
					if (event->mask & ERR_EVENTS) {
						libfflag_err("Wait failed. %s is removed?\n",
							FFLAG_DIR);
						ret = -2;
						break;
					}

					if ((event->len > 1) &&
						!strcmp(flag, event->name)) {
						ret = 0;
						break;
					}
				}

				p += sizeof(struct inotify_event) + event->len;
			}

			if (ret != -1)
				break;
		}
	} while (0);

	return ret;
}

int set_flag(const char *flag)
{
	int ret = -1;
	char buf[NAME_BUFSZ];
	char *event_buf;

	do {
		struct stat	st;

		if (!is_flag_ok(flag))
			break;

		if (initcheck() < 0)
			break;

		/* Flag is set yet */
		sprintf(buf, "%s/%s", FFLAG_DIR, flag);
		if (!stat(buf, &st)) {
			ret = 0;
			break;
		}

		if (open(buf, O_CREAT, 0744) == -1) {
			if (errno == EEXIST) {
				ret = 0;
				break;
			}

			libfflag_err("Failed to create FLAG %s (%d): %s.\n",
					flag, errno, strerror(errno));
			break;
		}

		ret = 0;

	} while (0);

	return ret;
}

int clear_flag(const char *flag)
{
	int ret = -1;
	char buf[NAME_BUFSZ];
	char *event_buf;

	do {
		struct stat	st;

		if (!is_flag_ok(flag))
			break;

		if (initcheck() < 0)
			break;

		/* Flag is set yet */
		sprintf(buf, "%s/%s", FFLAG_DIR, flag);
		if (stat(buf, &st)) {
			ret = 0;
			break;
		}

		if (unlink(buf) == -1) {
			if (errno == ENOENT) {
				ret = 0;
				break;
			}

			libfflag_err("Failed to clear flag (%d): %s.\n",
				errno, strerror(errno));
			break;
		}

		ret = 0;

	} while (0);

	return ret;
}

