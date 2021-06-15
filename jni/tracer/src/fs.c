#define LOG_TAG "Storage-IODBG-UIUC"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>
#include <log/logger.h>

#include "process.h"
#include "common.h"
#include "ops.h"
#include "notify.h"
#include "pthread.h"

int _notify = 0;

int fd_to_path(int fdesc, char *path)
{
	if (!path || fdesc == -1) {
		//ALOGE("fd_to_path: fd=%d path: %s", fdesc, path ? "OK" : "NULL");
		return -1;
	}

    /* try to figure out the path name */
    char printbuf[128];
    snprintf(printbuf, sizeof(printbuf), "/proc/%d/fd/%i", getpid(), fdesc);
    int len = readlink(printbuf, path, PATH_MAX - 1);
    if(len < 0) {
        //ALOGE("readlink(%s) failed: %s", printbuf, strerror(errno));
		return -1;
    }

	path[len] = '\0';
	return len;
}

static int skip_path(const char *path)
{
	if (path && (!strncmp(path, "/mnt/asec/", strlen("/mnt/asec")) ||
		!strncmp(path, "/mnt/secure/", strlen("/mnt/secure"))))
		return 1;

	return path && strncmp(path, "/system/", strlen("/system/")) &&
		strncmp(path, "/sdcard/", strlen("/sdcard/")) &&
		strncmp(path, "/mnt/", strlen("/mnt/")) &&
		strncmp(path, "/storage/", strlen("/storage/")) &&
		strncmp(path, "/data/", strlen("/data/"));
		//strncmp(path, "/cache/", strlen("/cache/")) != 0 &&
		//strncmp(path, "/persist/", strlen("/persist/")) != 0 &&
}

int notify(int fd, int flags, const char *path)
{
	if (skip_path(path))
		return 0;

	int ret = -1;
	struct stat stbuf;

	if (fd > 0)
		ret = fstat(fd, &stbuf);
	else
		ret = lstat(path, &stbuf);

	/* do not skip if unable to access */
	if (ret == -1 && (errno == ENOENT || errno == EPERM))
		return (flags & S_IFMT);

	if (ret == -1) {
		ALOGE("stat(%s) failed: %s", path, strerror(errno));
		return 0;
	}

	int mode = stbuf.st_mode & S_IFMT;
	if (!(flags & mode)) {
		ALOGE("%s mode mistmatch expected: %d found: %d",
				path, flags, mode);
		return 0;
	}

	return mode;
}

static void init(void) __attribute__ ((constructor));
static void fini(void) __attribute__ ((destructor));

extern void init_signal(void);

static void init(void)
{
	/* get process name */
	const char *procname = process_name();

	/* skip known processes */
	if (!strcmp(procname, "logcat") ||
		!strcmp(procname, "/system/bin/sdcard") ||
		!strcmp(procname, "sdcard") ||
		!strcmp(procname, "/system/bin/mpdecision") ||
		!strcmp(procname, "mpdecision") ||
		!strcmp(procname, "/system/xbin/su") ||
		!strcmp(procname, "su") ||
		!strcmp(procname, "/sbin/adbd") ||
		!strcmp(procname, "adbd") ||
		!strcmp(procname, "/system/bin/sensors.qcom") ||
		!strcmp(procname, "sensors.qcom") ||
		!strcmp(procname, "edu.buffalo.cse.phonelab.conductor") ||
		!strcmp(procname, "srg.cs.illinois.edu.storagestudy") ||
		!strcmp(procname, "storaged") ||
		!strcmp(procname, "/system/bin/storaged"))
		return;

	_notify = 1;

	pthread_common(1, 1);

	/* register exit function */
	atexit(fini);

	/* register signal handler */
	init_signal();
}

static void fini(void)
{
	pthread_common(0, 1);
}
