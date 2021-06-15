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

#include "fs.h"
#include "ops.h"
#include "notify.h"

static unlinkat_t _unlinkat;

static int unlink_init(void)
{
	if (!_unlinkat) {
		_unlinkat = (unlinkat_t) dlsym(RTLD_NEXT, "unlinkat");
		if (!_unlinkat)
    		_unlinkat = (unlinkat_t) dlsym(RTLD_NEXT, "__unlinkat");
	}

	if (!_unlinkat) {
		errno = EIO;
		err("\"_unlinkat\":0");
		return -1;
	}

	return 0;
}

static int __UNLINKAT(const char *op, int fd, const char *path, int flags)
{
	if (unlink_init() == -1)
		return -1;

	if (!_notify)
		return _unlinkat(fd, path, flags);
 
	int ret, flag = 0;
	if (fd == AT_FDCWD) {

		/* check if notification is needed */ 
		flag = notify(-1, (S_IFREG | S_IFDIR), path);

		ret = _unlinkat(AT_FDCWD, path, flags);
		if (!ret && flag)
			notify_unlink(op, flag, path);

	} else {

		info("\"func\":\"unlinkat\",\"path\":\"%s\"", path);

		/* check if notification is needed */ 
		char abs_path[PATH_MAX];
		ret = fd_to_path(fd, abs_path);
		if (ret > 0) {
			/* get aboslute path */
			snprintf(&abs_path[ret], PATH_MAX - ret, "/%s", path);
			info("\"func\":\"unlinkat\",\"abs_path\":\"%s\"", abs_path);
			flag = notify(-1, (S_IFREG | S_IFDIR), abs_path);
		}

		ret = _unlinkat(AT_FDCWD, path, flags);
		if (!ret && flag)
			notify_unlink(op, flag, abs_path);
	}

	return ret;
}

int unlinkat(int fd, const char *path, int flags)
{
	return __UNLINKAT("unlinkat", fd, path, flags);
}

int unlink(const char *path)
{
	return __UNLINKAT("unlink", AT_FDCWD, path, 0);
}

