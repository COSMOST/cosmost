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

static __thread char pread_path[PATH_MAX];

#ifdef PER_FILE_BATCHING
#include "batch.h"
static __thread int last_pread64_fd = -1;
static __thread off64_t last_pread64_off = -1;
static __thread size_t last_pread64_size = 0;
static __thread char pread64_iostr[IOSTR_BUFSIZE];
#endif

static pread64_t _pread64;

static int pread64_init(void)
{
	if (!_pread64) {
		_pread64 = (pread64_t) dlsym(RTLD_NEXT, "pread64");
		if (!_pread64)
    		_pread64 = (pread64_t) dlsym(RTLD_NEXT, "__pread64");
	}

	if (!_pread64) {
		errno = EIO;
		err("\"_pread64\":0");
		return -1;
	}

	return 0;
}

void pread64_close(int fd)
{
	(void)fd;
#ifdef PER_FILE_BATCHING
	/* file closing or thread exiting */
	if ((fd == -1 && last_pread64_fd != -1) ||
		(fd != -1 && fd == last_pread64_fd)) {

#ifdef TEST_OPS
		test("\"Stage\":4,\"Op\":\"pread64\",\"Path\":\"%s\"", pread_path);
#endif

		info("\"op\":\"%s\",\"last_pread64_fd\":%d,\"last_pread64_size\":%llu,"
			"\"last_pread64_off\":%lld",
			fd == -1 ? "pread64 (exit)" : "pread64 (close)",
			last_pread64_fd, (uint64_t)last_pread64_size, (int64_t)last_pread64_off);

		notify_read_write(fd == -1 ? "pread64 (exit)" : "pread64 (close)",
							pread_path, last_pread64_fd, last_pread64_size,
							last_pread64_off, pread64_iostr);

		last_pread64_fd = -1;
	}
#endif
}

static ssize_t _PREAD64(const char *op, int fd, void *buf, size_t size, off64_t off)
{
	if (pread64_init() == -1)
		return -1;

	ssize_t ret = _pread64(fd, buf, size, off);

#ifdef TEST_OPS
	if (ret > 0 && fd_to_path(fd, pread_path) != -1 &&
		strncmp(pread_path, "/sys/", strlen("/sys/")) &&
		strncmp(pread_path, "/proc/", strlen("/proc/")) &&
		strncmp(pread_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(pread_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":1,\"Op\":\"pread64\",\"Path\":\"%s\"", pread_path);
#endif

	if (ret > 0 && _notify) {

#ifdef TEST_OPS
	if (strncmp(pread_path, "/sys/", strlen("/sys/")) &&
		strncmp(pread_path, "/proc/", strlen("/proc/")) &&
		strncmp(pread_path, "pipe:[", strlen("pipe:[")) &&
		strncmp(pread_path, "/dev/", strlen("/dev/")))
		test("\"Stage\":2,\"Op\":\"pread64\",\"Path\":\"%s\"", pread_path);
#endif

#ifdef PER_FILE_BATCHING
		if (last_pread64_fd == fd &&
			!batch_read_write_first(op, pread_path, &last_pread64_fd, &last_pread64_size,
										&last_pread64_off, fd, ret, off, pread64_iostr)) {
			return ret;
		}
#endif

		/* different file, dispatch batched read notifications */
		if (fd_to_path(fd, pread_path) != -1 && notify(fd, S_IFREG, pread_path)) {
#ifdef TEST_OPS
			test("\"Stage\":3,\"Op\":\"pread64\",\"Path\":\"%s\"", pread_path);
#endif
#ifdef PER_FILE_BATCHING
			batch_read_write_second(op, pread_path, &last_pread64_fd, &last_pread64_size,
										&last_pread64_off, fd, ret, off, pread64_iostr);
#else
			notify_read_write(op, pread_path, fd, size, off, NULL);
#endif
		}
	}

	return ret;
}

ssize_t pread64(int fd, void *buf, size_t size, off64_t off)
{
	return _PREAD64("pread64", fd, buf, size, off);
}

ssize_t pread(int fd, void* buf, size_t size, off_t off)
{
	return _PREAD64("pread", fd, buf, size, (off64_t)off);
}

