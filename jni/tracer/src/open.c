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
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

static openat_t _openat;

static int open_init(void)
{
	if (!_openat) {
		_openat = (openat_t) dlsym(RTLD_NEXT, "openat");
		if (!_openat)
    		_openat = (openat_t) dlsym(RTLD_NEXT, "__open");
	}

	if (!_openat) {
		errno = EIO;
		err("\"_openat\":0");
		return -1;
	}

	return 0;
}

static int __OPENAT(const char *op, int fd, const char* path, int flags, int mode)
{
	if (open_init() == -1)
		return -1;

	/* skip if asked to or no file creation request */
	if (!_notify || !(flags & O_CREAT))
		return _openat(fd, path, flags, mode);

	/* track only newly created files */	
	int ret, exists = 1;
	if (fd == AT_FDCWD) {

		/* check if file already exists */
		if (access(path, F_OK) == -1 && errno == ENOENT)
			exists = 0;

		/* actual syscall */
		ret = _openat(AT_FDCWD, path, flags, mode);
		if (ret != -1 && !exists && notify(ret, S_IFREG, path))
			notify_create(op, path);

	} else {

		/* get absolute path */
		char abs_path[PATH_MAX];
		ret = fd_to_path(fd, abs_path);
		if (ret > 0) {
			snprintf(&abs_path[ret], PATH_MAX - ret, "/%s", path);
			info("\"abs_path\":\"%s\"", abs_path);
			if (access(abs_path, F_OK) == -1 && errno == ENOENT)
				exists = 0;
		}

		/* actual syscall */
		ret = _openat(fd, path, flags, mode);
		if (ret != -1 && !exists && notify(ret, S_IFREG, abs_path))
			notify_create(op, abs_path);
	}

	return ret;
}

int creat(const char* path, mode_t mode) {
	return __OPENAT("creat (create)", AT_FDCWD, path, O_CREAT | O_TRUNC | O_WRONLY, mode);
}
__strong_alias(creat64, creat);

int open(const char *path, int flags, ...)
{
	mode_t mode = 0;
	if ((flags & O_CREAT) != 0) {
		va_list args;
		va_start(args, flags);
		mode = (mode_t) va_arg(args, int);
		va_end(args);
	}
	return __OPENAT("open (create)", AT_FDCWD, path, flags, mode);
}
__strong_alias(open64, open);

int openat(int fd, const char *path, int flags, ...)
{
	mode_t mode = 0;
	if ((flags & O_CREAT) != 0) {
		va_list args;
		va_start(args, flags);
		mode = (mode_t) va_arg(args, int);
		va_end(args);
	}
	return __OPENAT("openat (create)", fd, path, flags, mode);
}
__strong_alias(openat64, openat);

