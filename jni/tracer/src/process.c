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

#include "common.h"
#include "ops.h"
#include "notify.h"
#include "pthread.h"

static openat_t _openat;
static read_t _read;
static close_t _close;

static void override_defaults()
{
	if (!_openat) {
		_openat = (openat_t) dlsym(RTLD_NEXT, "openat");
		if (!_openat)
    		_openat = (openat_t) dlsym(RTLD_NEXT, "__open");
	}

	if (!_read) {
		_read = (read_t) dlsym(RTLD_NEXT, "read");
		if (!_read)
    		_read = (read_t) dlsym(RTLD_NEXT, "__read");
	}

	if (!_close) {
		_close = (close_t) dlsym(RTLD_NEXT, "close");
		if (!_close)
    		_close = (close_t) dlsym(RTLD_NEXT, "__close");
	}
}

static char *read_process_name()
{
	char *procname = (char *)malloc(PATH_MAX);
	if (!procname)
		NULL;

	override_defaults();

	ssize_t len = -1;
	int fd = _openat(AT_FDCWD, "/proc/self/cmdline", O_RDONLY);
	if (fd >= 0)
		len = _read(fd, procname, PATH_MAX - 1);

  	if (len > 0) {
		if (len >= PATH_MAX) {
			ALOGE("\"Process name too long (truncated)\":%d", len);
			len = PATH_MAX - 1;
		}
		procname[len] = '\0';
	} else {
		snprintf(procname, 255, "%d", getuid());
	}

	if(fd >= 0)
		_close(fd);

	return procname;
}

/* retrieve process name */
const char *process_name(void)
{
	static uint64_t procname_pid;

	char *procname = NULL;
	char *old_procname;

	pid_t old_pid, new_pid = getpid();
	uint64_t old_procname_pid;
	uint64_t new_procname_pid;

	while (1) {

		old_procname_pid = __sync_fetch_and_or(&procname_pid, 0);
		old_procname = (char *)((uint32_t)(old_procname_pid >> 32));
		old_pid = (old_procname_pid & -1UL);

		if (old_pid == new_pid && strcmp(old_procname, "zygote"))
			return old_procname;

		if (!procname) {
			procname = read_process_name();
			if (!procname)
				continue;
		}

		new_procname_pid = (((uint64_t)((uint32_t)procname) << 32)) |
							((uint32_t)new_pid);
		if (__sync_bool_compare_and_swap(&procname_pid, old_procname_pid,
											new_procname_pid)) {
			if (old_procname)
				free(old_procname);
			return procname;
		} else {
			free(procname);
			procname = NULL;
		}
	}

	return NULL;
}

