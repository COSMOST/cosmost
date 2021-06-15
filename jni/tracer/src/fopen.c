#define LOG_TAG "Storage-IODBG-UIUC"

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/types.h>
#include <libgen.h>
#include <sys/syscall.h>

#include <cutils/log.h>

#include "fs.h"
#include "ops.h"
#include "notify.h"

static fopen_t _fopen;
static int fopen_init(void)
{
	if (!_fopen) {
		_fopen = (fopen_t) dlsym(RTLD_NEXT, "fopen");
		if (!_fopen)
    		_fopen = (fopen_t) dlsym(RTLD_NEXT, "__fopen");
	}

	if (!_fopen) {
		errno = EIO;
		ALOGE("NULL _fopen!");
		return -1;
	}

	return 0;
}

FILE* fopen(const char *path, const char *flags)
{
	if (fopen_init() == -1)
		return NULL;

	int exists = 1;
	if ((strstr(flags, "w+") || strstr(flags, "a+")) &&
		access(path, F_OK) == -1 && errno == ENOENT)
		exists = 0;

	FILE *fp = _fopen(path, flags);
	if (fp && !exists && _notify && notify(fileno(fp), S_IFREG, path))
		notify_create("fopen (create)", path);

	return fp;
}

