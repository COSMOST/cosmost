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
#include <sys/inotify.h>

#include <cutils/log.h>
#include <log/logger.h>

#include "common.h"
#include "ops.h"
#include "fs.h"

void close_common(int fd)
{
	(void)fd;
	read_close(fd);
	pread64_close(fd);
	fread_close(fd);
	write_close(fd);
	fwrite_close(fd);
	pwrite64_close(fd);
	mmap_close(fd);
}

static fclose_t _fclose;
static void fclose_init()
{
	if (!_fclose) {
		_fclose = (fclose_t) dlsym(RTLD_NEXT, "fclose");
		if (!_fclose)
    		_fclose = (fclose_t) dlsym(RTLD_NEXT, "__fclose");
	}
}

int fclose(FILE *fp)
{
	fclose_init();
	if (!_fclose) {
		errno = EIO;
		ALOGE("NULL _fclose!");
		return -1;
	}

	int fd = fileno(fp);
	if (fd != -1 && _notify)
		close_common(fd);

	return _fclose(fp);
}

static close_t _close;
void close_init()
{
	if (!_close) {
		_close = (close_t) dlsym(RTLD_NEXT, "close");
		if (!_close)
    		_close = (close_t) dlsym(RTLD_NEXT, "__close");
	}
}

int close(int fd)
{
	close_init();
	if (!_close) {
		errno = EIO;
		ALOGE("NULL _close!");
		return -1;
	}

	if (fd != -1 && _notify)
		close_common(fd);

	return _close(fd);
}
