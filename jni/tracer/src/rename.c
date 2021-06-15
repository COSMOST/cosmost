#define LOG_TAG "Storage-IODBG-UIUC"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

static renameat_t _renameat;

static int rename_init()
{
	if (!_renameat) {
		_renameat = (renameat_t) dlsym(RTLD_NEXT, "renameat");
		if (!_renameat)
    		_renameat = (renameat_t) dlsym(RTLD_NEXT, "__renameat");
	}

	if (!_renameat) {
		errno = EIO;
		err("\"_renameat\":0");
		return -1;
	}

	return 0;
}

static int __RENAMEAT(const char *op, int fromfd, const char *from,
						int tofd, const char *to)
{
	if (rename_init() == -1)
		return -1;

	if (!_notify)
		return _renameat(fromfd, from, tofd, to);
 
	int ret, flag = 0;
	if (fromfd == AT_FDCWD) {

		/* check if notification is needed */ 
		flag = notify(-1, (S_IFDIR | S_IFREG), from);
		ret = _renameat(AT_FDCWD, from, tofd, to);
		if (!ret && flag)
			notify_rename(op, flag, from, to);

	} else {

		info("\"func\":\"renameat\",\"from\":\"%s\",\"to\":\"%s\"",
				from, to);

		/* check if notification is needed */ 
		char from_abs_path[PATH_MAX];
		ret = fd_to_path(fromfd, from_abs_path);
		if (ret > 0) {
			/* get aboslute from path */
			snprintf(&from_abs_path[ret], PATH_MAX - ret, "/%s", from);
			info("\"func\":\"renameat\",\"from_abs_path\":\"%s\"",
				from_abs_path);
			flag = notify(-1, (S_IFDIR | S_IFREG), from_abs_path);
		}

		ret = _renameat(fromfd, from, tofd, to);
		if (!ret && flag) {
			if (tofd == AT_FDCWD)
				notify_rename(op, flag, from_abs_path, to);
			else {
				/* get aboslute to path */
				char to_abs_path[PATH_MAX];
				ret = fd_to_path(tofd, to_abs_path);
				if (ret > 0) {
					snprintf(&to_abs_path[ret], PATH_MAX - ret, "/%s", to);
					info("\"func\":\"renameat\",\"to_abs_path\":\"%s\"",
							to_abs_path);
					notify_rename(op, flag, from_abs_path, to_abs_path);
				} else
					notify_rename(op, flag, from_abs_path, to);
			}
		}
	}

	return ret;
}

int renameat(int fromfd, const char *from, int tofd, const char *to)
{
	return __RENAMEAT("renameat", fromfd, from, tofd, to);
}

int rename(const char* old_path, const char* new_path)
{
	return __RENAMEAT("rename", AT_FDCWD, old_path, AT_FDCWD, new_path);
}

